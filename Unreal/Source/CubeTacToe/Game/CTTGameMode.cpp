#include "Game/CTTGameMode.h"
#include "Game/CTTGameState.h"
#include "Game/CTTGameInstance.h"
#include "Game/CTTBoardState.h"
#include "Game/CTTAIPlayer.h"
#include "Actors/CTTBoardActor.h"
#include "UI/CTTHUDWidget.h"
#include "UI/CTTGameOverWidget.h"
#include "UI/CTTPauseMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "Actors/CTTOrbitCameraComponent.h"
#include "Game/CTTPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/AudioComponent.h"
#include "CubeTacToe.h"

ACTTGameMode::ACTTGameMode()
{
	GameStateClass = ACTTGameState::StaticClass();
}

void ACTTGameMode::BeginPlay()
{
	Super::BeginPlay();
	AIPlayer = NewObject<UCTTAIPlayer>(this, TEXT("AIPlayer"));
	BoardActor = Cast<ACTTBoardActor>(UGameplayStatics::GetActorOfClass(this, ACTTBoardActor::StaticClass()));
	UE_LOG(LogCTT, Log, TEXT("GameMode: BeginPlay — AIPlayer created, BoardActor=%s"),
	       BoardActor ? TEXT("found") : TEXT("NOT FOUND"));

	// Read settings from GameInstance (written by the main menu widget before level travel)
	if (UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance()))
	{
		GameMode     = GI->PendingGameMode;
		WinCondition = GI->PendingWinCondition;
		HumanPlayer  = GI->PendingHumanPlayer;
		UE_LOG(LogCTT, Log, TEXT("GameMode: Read settings from GameInstance — Mode=%s WinCondition=%s"),
		       *UEnum::GetValueAsString(GameMode), *UEnum::GetValueAsString(WinCondition));

		GI->StartMusic(GI->GameplayMusic);
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;

	// Defer StartNewGame to next frame so GameState::BeginPlay has created BoardState
	GetWorldTimerManager().SetTimerForNextTick(this, &ACTTGameMode::StartNewGame);
}

void ACTTGameMode::StartNewGame()
{
	ACTTGameState* GS = GetGameState<ACTTGameState>();
	if (!ensure(GS)) return;
	if (!ensure(GS->BoardState)) return;

	BoardState = GS->BoardState;
	BoardState->Reset();
	if (BoardActor) BoardActor->ResetVisuals();

	CurrentPlayer        = ECellOwner::PlayerX;
	GamePhase            = EGamePhase::Playing;
	MovesThisTurn        = 0;
	MovesAllowedThisTurn = 1; // First player gets only 1 move on their first turn
	bFirstMoveFaceSet    = false;
	bHasPendingAIMove    = false;

	UE_LOG(LogCTT, Log, TEXT("GameMode: StartNewGame — Mode=%s WinCondition=%s"),
	       *UEnum::GetValueAsString(GameMode),
	       *UEnum::GetValueAsString(WinCondition));
	UE_LOG(LogCTT, Log, TEXT("GameMode: First turn — %s gets 1 move"),
	       *UEnum::GetValueAsString(CurrentPlayer));

	StartTurnTimer();
	OnTurnChanged(CurrentPlayer);

	if (GameMode == EGameMode::VsAI && CurrentPlayer != HumanPlayer)
	{
		ScheduleAIMove();
	}
}

FMoveResult ACTTGameMode::AttemptMove(FCellCoord Coord)
{
	FMoveResult Empty;
	if (GamePhase != EGamePhase::Playing || !BoardState)
	{
		UE_LOG(LogCTT, Warning, TEXT("AttemptMove: rejected — Phase=%s BoardState=%s"),
		       *UEnum::GetValueAsString(GamePhase),
		       BoardState ? TEXT("valid") : TEXT("null"));
		return Empty;
	}

	// Block human input during AI's turn (including while AI is thinking or camera is animating).
	// bApplyingAIMove is only true when ApplyPendingAIMove is executing — let that through.
	if (GameMode == EGameMode::VsAI && CurrentPlayer != HumanPlayer && !bApplyingAIMove)
	{
		UE_LOG(LogCTT, Log, TEXT("AttemptMove: rejected — it is the AI's turn"));
		return Empty;
	}

	// --- Same-face rule: second move of a 2-move turn must be on a different face ---
	if (MovesAllowedThisTurn == 2 && MovesThisTurn == 1 && bFirstMoveFaceSet)
	{
		if (Coord.FaceIndex == FirstMoveFace)
		{
			UE_LOG(LogCTT, Warning,
			       TEXT("AttemptMove: rejected — second move cannot be on the same face (Face=%d)"),
			       static_cast<int32>(Coord.FaceIndex));
			Empty.bRejectedSameFace = true;
			return Empty;
		}
	}

	UE_LOG(LogCTT, Log, TEXT("AttemptMove: %s → Face=%d Row=%d Col=%d"),
	       *UEnum::GetValueAsString(CurrentPlayer),
	       static_cast<int32>(Coord.FaceIndex), Coord.Row, Coord.Col);

	FMoveResult Result = BoardState->PlaceMove(Coord, CurrentPlayer);
	if (!Result.bValid)
	{
		UE_LOG(LogCTT, Warning, TEXT("AttemptMove: PlaceMove rejected the move"));
		return Result;
	}

	OnMoveApplied(Coord, CurrentPlayer);

	if (Result.bFaceJustResolved &&
	    (Result.FaceWinState == EFaceWinState::WonByX || Result.FaceWinState == EFaceWinState::WonByO))
	{
		ECellOwner FaceWinner = (Result.FaceWinState == EFaceWinState::WonByX)
		                            ? ECellOwner::PlayerX : ECellOwner::PlayerO;
		UE_LOG(LogCTT, Log, TEXT("AttemptMove: Face %d won by %s"),
		       static_cast<int32>(Coord.FaceIndex), *UEnum::GetValueAsString(FaceWinner));
		OnFaceWon(Coord.FaceIndex, FaceWinner);
	}

	// Check game-over; if not over, consume one move from this turn's quota
	if (!CheckAndHandleGameOver(Result))
	{
		// Record the face used by the first move so we can enforce the different-face rule
		if (MovesAllowedThisTurn == 2 && MovesThisTurn == 0)
		{
			FirstMoveFace    = Coord.FaceIndex;
			bFirstMoveFaceSet = true;
		}

		MovesThisTurn++;

		if (MovesThisTurn >= MovesAllowedThisTurn)
		{
			// Quota reached — hand off to the other player
			AdvanceTurn();
		}
		else
		{
			// Same player still has moves left — verify that moves exist on OTHER faces;
			// if none do (rare late-game scenario), auto-advance to avoid a dead lock.
			bool bHasOtherFaceMoves = false;
			for (int32 f = 0; f < 6; ++f)
			{
				if (static_cast<EFaceIndex>(f) == FirstMoveFace) continue;
				if (!BoardState->GetLegalMovesOnFace(static_cast<EFaceIndex>(f)).IsEmpty())
				{
					bHasOtherFaceMoves = true;
					break;
				}
			}

			if (!bHasOtherFaceMoves)
			{
				UE_LOG(LogCTT, Log,
				       TEXT("AttemptMove: no moves available on other faces — auto-advancing turn"));
				AdvanceTurn();
			}
			else
			{
				UE_LOG(LogCTT, Log, TEXT("GameMode: %s has used %d/%d moves — goes again"),
				       *UEnum::GetValueAsString(CurrentPlayer),
				       MovesThisTurn, MovesAllowedThisTurn);
				OnTurnChanged(CurrentPlayer);

				if (GameMode == EGameMode::VsAI && CurrentPlayer != HumanPlayer)
				{
					ScheduleAIMove();
				}
			}
		}
	}

	return Result;
}

bool ACTTGameMode::CheckAndHandleGameOver(const FMoveResult& MoveResult)
{
	if (!BoardState) return false;

	ECellOwner Winner = ECellOwner::None;
	bool bDraw = false;

	if (WinCondition == EWinCondition::WinAnyFace)
	{
		Winner = BoardState->CheckWinAnyFace();
		if (Winner == ECellOwner::None && BoardState->GetLegalMoves().IsEmpty())
		{
			bDraw = true;
		}
	}
	else
	{
		Winner = BoardState->CheckWinMostFaces();
		if (Winner == ECellOwner::None)
		{
			int32 X = 0, O = 0;
			BoardState->GetFaceWinCounts(X, O);
			UE_LOG(LogCTT, Log, TEXT("CheckGameOver: Face counts — X=%d O=%d"), X, O);
			if (X + O == 6)
			{
				bDraw = (X == O);
				if (!bDraw)
				{
					Winner = (X > O) ? ECellOwner::PlayerX : ECellOwner::PlayerO;
				}
			}
			else if (BoardState->GetLegalMoves().IsEmpty())
			{
				bDraw = true;
			}
		}
	}

	if (Winner != ECellOwner::None || bDraw)
	{
		GamePhase = EGamePhase::GameOver;
		GetWorldTimerManager().ClearTimer(TurnTimerHandle);

		if (bDraw)
		{
			UE_LOG(LogCTT, Log, TEXT("GameMode: DRAW — game over"));
		}
		else
		{
			UE_LOG(LogCTT, Log, TEXT("GameMode: %s WINS — game over"),
			       *UEnum::GetValueAsString(Winner));
		}

		if (UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance()))
		{
			GI->RecordResult(bDraw ? ECellOwner::None : Winner);
		}

		OnGameOver(bDraw ? ECellOwner::None : Winner, bDraw);
		return true;
	}
	return false;
}

void ACTTGameMode::AdvanceTurn()
{
	CurrentPlayer        = (CurrentPlayer == ECellOwner::PlayerX)
	                           ? ECellOwner::PlayerO : ECellOwner::PlayerX;
	MovesThisTurn        = 0;
	MovesAllowedThisTurn = 2; // All turns after the first allow 2 moves
	bFirstMoveFaceSet    = false;

	UE_LOG(LogCTT, Log, TEXT("GameMode: Turn → %s (2 moves allowed)"),
	       *UEnum::GetValueAsString(CurrentPlayer));
	StartTurnTimer();
	OnTurnChanged(CurrentPlayer);

	if (GameMode == EGameMode::VsAI && CurrentPlayer != HumanPlayer)
	{
		ScheduleAIMove();
	}
}

float ACTTGameMode::GetTurnTimeRemaining() const
{
	if (GamePhase == EGamePhase::Paused) return PausedTimeRemaining;
	if (GamePhase != EGamePhase::Playing) return 0.f;
	const float Elapsed = GetWorld()->GetTimeSeconds() - TurnStartTime;
	return FMath::Max(0.f, TurnTimerDuration - Elapsed);
}

void ACTTGameMode::StartTurnTimer()
{
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	TurnStartTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(TurnTimerHandle, this,
	                                &ACTTGameMode::TurnTimerExpired, TurnTimerDuration, false);
	UE_LOG(LogCTT, Log, TEXT("GameMode: Turn timer started (%.1fs) for %s"),
	       TurnTimerDuration, *UEnum::GetValueAsString(CurrentPlayer));
}

void ACTTGameMode::TurnTimerExpired()
{
	if (GamePhase != EGamePhase::Playing) return;
	UE_LOG(LogCTT, Log, TEXT("GameMode: Turn timer expired for %s"), *UEnum::GetValueAsString(CurrentPlayer));
	OnTurnTimerExpired(CurrentPlayer);
}

void ACTTGameMode::ScheduleAIMove()
{
	UE_LOG(LogCTT, Log, TEXT("GameMode: AI move scheduled (0.4s)"));
	GetWorldTimerManager().SetTimer(AITimerHandle, this,
	                                &ACTTGameMode::ExecuteAIMove, 0.4f, false);
}

void ACTTGameMode::ExecuteAIMove()
{
	if (GamePhase != EGamePhase::Playing || !BoardState || !AIPlayer)
	{
		UE_LOG(LogCTT, Warning, TEXT("ExecuteAIMove: aborted — invalid state"));
		return;
	}

	// Determine whether the second-move face exclusion applies
	EFaceIndex ExcludedFace = EFaceIndex::Count; // Count = no exclusion
	if (bFirstMoveFaceSet && MovesAllowedThisTurn == 2 && MovesThisTurn == 1)
	{
		// Only exclude if there are actually moves available on other faces
		bool bHasOtherFaceMoves = false;
		for (int32 f = 0; f < 6; ++f)
		{
			if (static_cast<EFaceIndex>(f) == FirstMoveFace) continue;
			if (!BoardState->GetLegalMovesOnFace(static_cast<EFaceIndex>(f)).IsEmpty())
			{
				bHasOtherFaceMoves = true;
				break;
			}
		}
		ExcludedFace = bHasOtherFaceMoves ? FirstMoveFace : EFaceIndex::Count;
	}

	// Capture all game-thread data needed for the background search
	FCTTBoardSnapshot Snapshot = UCTTAIPlayer::TakeSnapshot(BoardState);
	const ECellOwner  AIOwner  = (HumanPlayer == ECellOwner::PlayerX)
	                                 ? ECellOwner::PlayerO : ECellOwner::PlayerX;
	const EWinCondition Cond   = WinCondition;
	const int32 Depth          = AIPlayer->MaxDepth;

	UE_LOG(LogCTT, Log, TEXT("ExecuteAIMove: launching async AI search (ExcludeFace=%d)"),
	       static_cast<int32>(ExcludedFace));

	TWeakObjectPtr<ACTTGameMode> WeakThis(this);

	Async(EAsyncExecution::ThreadPool,
	[WeakThis, Snapshot, AIOwner, Cond, Depth, ExcludedFace]()
	{
		// --- Background thread: pure-C++ minimax, no UObject allocations ---
		FCellCoord AICoord = UCTTAIPlayer::RunSearch(Snapshot, AIOwner, Cond, Depth, ExcludedFace);

		// --- Return to game thread to apply the result ---
		AsyncTask(ENamedThreads::GameThread,
		[WeakThis, AICoord]()
		{
			ACTTGameMode* GM = WeakThis.Get();
			if (!GM || GM->GamePhase != EGamePhase::Playing) return;

			GM->PendingAICoord   = AICoord;
			GM->bHasPendingAIMove = true;

			UE_LOG(LogCTT, Log, TEXT("ExecuteAIMove: AI chose Face=%d Row=%d Col=%d — notifying BP"),
			       static_cast<int32>(AICoord.FaceIndex), AICoord.Row, AICoord.Col);

			// Fire event — BP overrides to rotate camera first, then calls ApplyPendingAIMove.
			// Default C++ implementation applies the move immediately.
			GM->OnAIChosenMove(AICoord);
		});
	});
}

void ACTTGameMode::ApplyPendingAIMove()
{
	if (!bHasPendingAIMove || GamePhase != EGamePhase::Playing)
	{
		UE_LOG(LogCTT, Warning, TEXT("ApplyPendingAIMove: no pending move or wrong phase — ignored"));
		return;
	}

	bHasPendingAIMove = false;
	const FCellCoord CoordToApply = PendingAICoord;

	UE_LOG(LogCTT, Log, TEXT("ApplyPendingAIMove: applying Face=%d Row=%d Col=%d"),
	       static_cast<int32>(CoordToApply.FaceIndex), CoordToApply.Row, CoordToApply.Col);

	bApplyingAIMove = true;
	AttemptMove(CoordToApply);
	bApplyingAIMove = false;
}

// ---- Widget helpers ----

void ACTTGameMode::ShowHUDWidget()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	if (GameOverWidget)
	{
		GameOverWidget->RemoveFromParent();
		GameOverWidget = nullptr;
	}
	if (!HUDWidget && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UCTTHUDWidget>(PC, HUDWidgetClass);
	}
	if (HUDWidget && !HUDWidget->IsInViewport())
	{
		HUDWidget->AddToViewport(1);
	}
	if (HUDWidget)
	{
		HUDWidget->RefreshTurn(CurrentPlayer, MovesThisTurn, MovesAllowedThisTurn);
	}
}

void ACTTGameMode::ShowGameOverWidget(ECellOwner Winner, bool bIsDraw)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	if (GameOverWidget)
	{
		GameOverWidget->RemoveFromParent();
		GameOverWidget = nullptr;
	}
	if (GameOverWidgetClass)
	{
		GameOverWidget = CreateWidget<UCTTGameOverWidget>(PC, GameOverWidgetClass);
	}
	if (GameOverWidget)
	{
		GameOverWidget->AddToViewport(2);
		GameOverWidget->ShowResult(Winner, bIsDraw);
	}
	if (HUDWidget)
	{
		if (UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance()))
		{
			HUDWidget->RefreshScores(GI->ScoreX, GI->ScoreO, GI->Draws);
		}
	}
}

void ACTTGameMode::ShowPauseMenuWidget()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	if (!PauseMenuWidget && PauseMenuWidgetClass)
	{
		PauseMenuWidget = CreateWidget<UCTTPauseMenuWidget>(PC, PauseMenuWidgetClass);
	}
	if (PauseMenuWidget && !PauseMenuWidget->IsInViewport())
	{
		PauseMenuWidget->AddToViewport(10);
	}
}

void ACTTGameMode::HidePauseMenuWidget()
{
	if (PauseMenuWidget && PauseMenuWidget->IsInViewport())
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;
	}
}

// ---- BlueprintNativeEvent implementations ----

void ACTTGameMode::OnTurnChanged_Implementation(ECellOwner NewPlayer)
{
	if (GamePhase == EGamePhase::Playing)
	{
		ShowHUDWidget();
	}
	if (HUDWidget)
	{
		HUDWidget->RefreshTurn(NewPlayer, MovesThisTurn, MovesAllowedThisTurn);
	}
}

void ACTTGameMode::OnFaceWon_Implementation(EFaceIndex Face, ECellOwner Winner)
{
	int32 X = 0, O = 0;
	ACTTGameState* GS = GetGameState<ACTTGameState>();
	if (GS && GS->BoardState)
	{
		GS->BoardState->GetFaceWinCounts(X, O);
	}
	if (HUDWidget)
	{
		HUDWidget->RefreshFaceWins(X, O);
	}
}

void ACTTGameMode::OnGameOver_Implementation(ECellOwner Winner, bool bIsDraw)
{
	// Play the lose SFX when the human loses against the AI
	if (!bIsDraw && GameMode == EGameMode::VsAI && Winner != HumanPlayer && HumanLoseVsAISound)
	{
		UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance());
		UAudioComponent* AC = UGameplayStatics::SpawnSound2D(this, HumanLoseVsAISound);
		if (AC && GI && GI->SFXSoundClass)
		{
			AC->SoundClassOverride = GI->SFXSoundClass;
		}
	}

	ShowGameOverWidget(Winner, bIsDraw);
}

void ACTTGameMode::OnAIChosenMove_Implementation(FCellCoord Coord)
{
	// Rotate the cube to show the AI's chosen face, then apply the move.
	ACTTPlayerController* PC = Cast<ACTTPlayerController>(
		GetWorld()->GetFirstPlayerController());

	if (PC)
	{
		PC->NavigateToFace(static_cast<int32>(Coord.FaceIndex));
		GetWorldTimerManager().SetTimer(AICameraTimerHandle, this,
		                                &ACTTGameMode::ApplyPendingAIMove,
		                                PC->FaceNavDuration, false);
	}
	else
	{
		ApplyPendingAIMove();
	}
}

UCTTOrbitCameraComponent* ACTTGameMode::FindOrbitCamera() const
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC) return nullptr;
	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return nullptr;
	return Pawn->FindComponentByClass<UCTTOrbitCameraComponent>();
}

void ACTTGameMode::GetFaceAngles(EFaceIndex Face, float CurrentYaw,
                                  float& OutYaw, float& OutPitch) const
{
	// Camera sits at (Radius·cos(Pitch)·cos(Yaw), Radius·cos(Pitch)·sin(Yaw), Radius·sin(Pitch))
	// and looks toward the origin.  To face a cube face the camera must be on the
	// outward side of that face:
	//   Front (+X)  → Yaw=  0°,   Pitch=  0°
	//   Right (+Y)  → Yaw= 90°,   Pitch=  0°
	//   Back  (-X)  → Yaw=180°,   Pitch=  0°
	//   Left  (-Y)  → Yaw=270°,   Pitch=  0°
	//   Top   (+Z)  → keep Yaw,   Pitch= 70°
	//   Bottom(-Z)  → keep Yaw,   Pitch=-70°
	switch (Face)
	{
	case EFaceIndex::Front:  OutYaw =   0.f; OutPitch =   0.f; break;
	case EFaceIndex::Right:  OutYaw =  90.f; OutPitch =   0.f; break;
	case EFaceIndex::Back:   OutYaw = 180.f; OutPitch =   0.f; break;
	case EFaceIndex::Left:   OutYaw = 270.f; OutPitch =   0.f; break;
	case EFaceIndex::Top:    OutYaw = CurrentYaw; OutPitch =  70.f; break;
	case EFaceIndex::Bottom: OutYaw = CurrentYaw; OutPitch = -70.f; break;
	default:                 OutYaw = CurrentYaw; OutPitch =   0.f; break;
	}
}

void ACTTGameMode::OnTurnTimerExpired_Implementation(ECellOwner Player)
{
	if (GamePhase != EGamePhase::Playing) return;

	const ECellOwner Winner = (Player == ECellOwner::PlayerX) ? ECellOwner::PlayerO : ECellOwner::PlayerX;
	UE_LOG(LogCTT, Log, TEXT("GameMode: Timer expired — %s forfeits, %s wins"),
	       *UEnum::GetValueAsString(Player), *UEnum::GetValueAsString(Winner));

	GamePhase = EGamePhase::GameOver;
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GetWorldTimerManager().ClearTimer(AITimerHandle);
	GetWorldTimerManager().ClearTimer(AICameraTimerHandle);

	if (UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance()))
	{
		GI->RecordResult(Winner);
	}

	OnGameOver(Winner, false);
}

void ACTTGameMode::PauseGame()
{
	if (GamePhase != EGamePhase::Playing) return;

	GamePhase = EGamePhase::Paused;

	// Save remaining turn time and stop all timers
	PausedTimeRemaining = GetTurnTimeRemaining();
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GetWorldTimerManager().ClearTimer(AITimerHandle);
	GetWorldTimerManager().ClearTimer(AICameraTimerHandle);

	UGameplayStatics::SetGamePaused(GetWorld(), true);

	UE_LOG(LogCTT, Log, TEXT("GameMode: PauseGame (%.1fs remaining)"), PausedTimeRemaining);
	ShowPauseMenuWidget();
}

void ACTTGameMode::ResumeGame()
{
	if (GamePhase != EGamePhase::Paused) return;

	HidePauseMenuWidget();
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	GamePhase = EGamePhase::Playing;

	// Restart the turn timer with the saved remaining time
	TurnStartTime = GetWorld()->GetTimeSeconds() - (TurnTimerDuration - PausedTimeRemaining);
	GetWorldTimerManager().SetTimer(TurnTimerHandle, this,
	                                &ACTTGameMode::TurnTimerExpired, PausedTimeRemaining, false);

	// Re-schedule AI move if it was the AI's turn
	if (GameMode == EGameMode::VsAI && CurrentPlayer != HumanPlayer && !bHasPendingAIMove)
	{
		ScheduleAIMove();
	}

	UE_LOG(LogCTT, Log, TEXT("GameMode: ResumeGame (%.1fs remaining)"), PausedTimeRemaining);
}

void ACTTGameMode::TogglePause()
{
	if (GamePhase == EGamePhase::Playing)
		PauseGame();
	else if (GamePhase == EGamePhase::Paused)
		ResumeGame();
}

void ACTTGameMode::ReturnToMainMenu()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	HidePauseMenuWidget();
	GetWorldTimerManager().ClearTimer(AITimerHandle);
	GetWorldTimerManager().ClearTimer(AICameraTimerHandle);
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GamePhase         = EGamePhase::MainMenu;
	bFirstMoveFaceSet = false;
	bHasPendingAIMove = false;
	UE_LOG(LogCTT, Log, TEXT("GameMode: ReturnToMainMenu — travelling to menu map"));

	if (UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance()))
	{
		GI->OpenMainMenuMap();
	}
}
