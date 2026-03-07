#include "UI/CTTHUDWidget.h"
#include "UI/CTTNeonStyle.h"
#include "Game/CTTGameMode.h"
#include "Game/CTTGameState.h"
#include "Game/CTTBoardState.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/PanelWidget.h"
#include "CubeTacToe.h"

void UCTTHUDWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Bar_Timer)
	{
		Bar_Timer->SetWidgetStyle(FCTTNeonStyle::TimerBarStyle());
		Bar_Timer->SetFillColorAndOpacity(FCTTNeonStyle::ElectricBlue());
	}
}

void UCTTHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float Fraction = GetTurnTimeFraction();

	if (Bar_Timer)
	{
		Bar_Timer->SetPercent(Fraction);

		// Tint bar: UltraPink (low time) → ElectricBlue (full time)
		const FLinearColor BarColor = FLinearColor::LerpUsingHSV(
			FCTTNeonStyle::UltraPink(),    // low time
			FCTTNeonStyle::ElectricBlue(), // full time
			Fraction);
		Bar_Timer->SetFillColorAndOpacity(BarColor);
	}

	if (Text_TimeRemaining)
	{
		const int32 SecondsLeft = FMath::CeilToInt(GetTurnTimeRemaining());
		Text_TimeRemaining->SetText(FText::AsNumber(SecondsLeft));
	}
}

// ---- BlueprintNativeEvent implementations ----

void UCTTHUDWidget::RefreshTurn_Implementation(ECellOwner CurrentPlayer, int32 MovesThisTurn, int32 MovesAllowed)
{
	if (Text_CurrentPlayer)
	{
		const bool bIsX = (CurrentPlayer == ECellOwner::PlayerX);
		Text_CurrentPlayer->SetText(FText::FromString(bIsX ? TEXT("X") : TEXT("O")));
		Text_CurrentPlayer->SetColorAndOpacity(FSlateColor(bIsX ? FCTTNeonStyle::PlayerX() : FCTTNeonStyle::PlayerO()));
	}

	if (Text_MovesRemaining)
	{
		const int32 Left = MovesAllowed - MovesThisTurn;
		const FString Str = FString::Printf(TEXT("%d move%s left"), Left, Left == 1 ? TEXT("") : TEXT("s"));
		Text_MovesRemaining->SetText(FText::FromString(Str));
	}

	// Show/hide face-win panel based on active win condition
	if (Panel_FaceWins)
	{
		const bool bShowFacePanel = (GetWinCondition() == EWinCondition::WinMostFaces);
		Panel_FaceWins->SetVisibility(bShowFacePanel ? ESlateVisibility::SelfHitTestInvisible
		                                             : ESlateVisibility::Collapsed);
	}
}

void UCTTHUDWidget::RefreshFaceWins_Implementation(int32 FacesX, int32 FacesO)
{
	if (Text_FacesX)
	{
		Text_FacesX->SetText(FText::AsNumber(FacesX));
		Text_FacesX->SetColorAndOpacity(FSlateColor(FCTTNeonStyle::PlayerX()));
	}
	if (Text_FacesO)
	{
		Text_FacesO->SetText(FText::AsNumber(FacesO));
		Text_FacesO->SetColorAndOpacity(FSlateColor(FCTTNeonStyle::PlayerO()));
	}
}

void UCTTHUDWidget::RefreshScores_Implementation(int32 ScoreX, int32 ScoreO, int32 Draws)
{
	if (Text_ScoreX)    Text_ScoreX->SetText(FText::AsNumber(ScoreX));
	if (Text_ScoreO)    Text_ScoreO->SetText(FText::AsNumber(ScoreO));
	if (Text_ScoreDraws) Text_ScoreDraws->SetText(FText::AsNumber(Draws));
}

// ---- Accessors ----

float UCTTHUDWidget::GetTurnTimeRemaining() const
{
	ACTTGameMode* GM = GetCTTGameMode();
	return GM ? GM->GetTurnTimeRemaining() : 0.f;
}

float UCTTHUDWidget::GetTurnTimeFraction() const
{
	ACTTGameMode* GM = GetCTTGameMode();
	if (!GM || GM->TurnTimerDuration <= 0.f) return 1.f;
	return GM->GetTurnTimeRemaining() / GM->TurnTimerDuration;
}

ECellOwner UCTTHUDWidget::GetCurrentPlayer() const
{
	ACTTGameMode* GM = GetCTTGameMode();
	return GM ? GM->CurrentPlayer : ECellOwner::None;
}

int32 UCTTHUDWidget::GetMovesThisTurn() const
{
	ACTTGameMode* GM = GetCTTGameMode();
	return GM ? GM->MovesThisTurn : 0;
}

int32 UCTTHUDWidget::GetMovesAllowedThisTurn() const
{
	ACTTGameMode* GM = GetCTTGameMode();
	return GM ? GM->MovesAllowedThisTurn : 0;
}

EWinCondition UCTTHUDWidget::GetWinCondition() const
{
	ACTTGameMode* GM = GetCTTGameMode();
	return GM ? GM->WinCondition : EWinCondition::WinAnyFace;
}

void UCTTHUDWidget::GetFaceWinCounts(int32& OutX, int32& OutO) const
{
	OutX = OutO = 0;
	ACTTGameState* GS = GetCTTGameState();
	if (GS && GS->BoardState)
	{
		GS->BoardState->GetFaceWinCounts(OutX, OutO);
	}
}

ACTTGameMode* UCTTHUDWidget::GetCTTGameMode() const
{
	UWorld* World = GetWorld();
	return World ? World->GetAuthGameMode<ACTTGameMode>() : nullptr;
}

ACTTGameState* UCTTHUDWidget::GetCTTGameState() const
{
	UWorld* World = GetWorld();
	return World ? World->GetGameState<ACTTGameState>() : nullptr;
}
