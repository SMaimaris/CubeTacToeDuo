#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Types/CTTTypes.h"
#include "CTTGameMode.generated.h"

class UCTTBoardState;
class UCTTAIPlayer;
class ACTTBoardActor;
class UCTTHUDWidget;
class UCTTGameOverWidget;
class UCTTPauseMenuWidget;

/**
 * Central state machine. All cross-system coordination flows here.
 * Blueprint child (BP_GameMode) implements all BlueprintImplementableEvents
 * to drive the visual/UI side.
 */
UCLASS()
class CUBETACTOE_API ACTTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACTTGameMode();

	virtual void BeginPlay() override;

	// ---- Config (set from Main Menu before StartNewGame) ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	EGameMode GameMode = EGameMode::LocalTwoPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	EWinCondition WinCondition = EWinCondition::WinAnyFace;

	/** Which player token the human controls in a Vs AI game. AI takes the other token. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	ECellOwner HumanPlayer = ECellOwner::PlayerX;

	// ---- Runtime State ----

	UPROPERTY(BlueprintReadOnly, Category = "State")
	ECellOwner CurrentPlayer = ECellOwner::PlayerX;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EGamePhase GamePhase = EGamePhase::MainMenu;

	/** Moves the current player has placed in this turn (0-based) */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	int32 MovesThisTurn = 0;

	/** How many moves are allowed this turn (1 for the very first turn, 2 after) */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	int32 MovesAllowedThisTurn = 1;

	/** Face used for the first move of the current 2-move turn. Only valid when bFirstMoveFaceSet is true. */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	EFaceIndex FirstMoveFace = EFaceIndex::Top;

	/** True once the first of the two allowed moves has been placed this turn. */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bFirstMoveFaceSet = false;

	/** AI-chosen coordinate waiting for camera animation before being applied. */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FCellCoord PendingAICoord;

	/** True while an AI move has been chosen but not yet applied (camera may be animating). */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bHasPendingAIMove = false;

	/** Seconds each player has to complete their full turn. Editable per game. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float TurnTimerDuration = 60.f;

	/**
	 * Seconds remaining in the current turn.
	 * Returns 0 when the game is not in the Playing phase.
	 */
	UFUNCTION(BlueprintPure, Category = "State")
	float GetTurnTimeRemaining() const;

	// ---- UI Widget Classes (set in BP_GameMode defaults) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Classes")
	TSubclassOf<UCTTHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Classes")
	TSubclassOf<UCTTGameOverWidget> GameOverWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Classes")
	TSubclassOf<UCTTPauseMenuWidget> PauseMenuWidgetClass;

	// ---- UI Widget Instances ----

	UPROPERTY(BlueprintReadOnly, Category = "UI|Instances")
	TObjectPtr<UCTTHUDWidget> HUDWidget;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Instances")
	TObjectPtr<UCTTGameOverWidget> GameOverWidget;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Instances")
	TObjectPtr<UCTTPauseMenuWidget> PauseMenuWidget;

	// ---- Public API (callable from Blueprints / PlayerController) ----

	/** Initialize a new game with the current Config settings */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartNewGame();

	/**
	 * Attempt to place a move for the current player.
	 * Returns the result; also fires BlueprintImplementableEvents as appropriate.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	FMoveResult AttemptMove(FCellCoord Coord);

	/** Return to main menu phase */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void ReturnToMainMenu();

	/** Pause the game and show the pause menu. Only works during Playing phase. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void PauseGame();

	/** Unpause the game and hide the pause menu. Only works during Paused phase. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void ResumeGame();

	/** Toggle between Playing and Paused. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void TogglePause();

	/**
	 * Apply the AI's pending move (stored in PendingAICoord).
	 * Call this from BP_GameMode after the camera has finished rotating to the target face.
	 * If no BP override of OnAIChosenMove is provided, the default implementation calls this immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void ApplyPendingAIMove();

	// ---- Events ----

	/** Pure actor visual; no widget work. Override in BP_GameMode for mesh swaps etc. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnMoveApplied(FCellCoord Coord, ECellOwner Player);

	/**
	 * Called when the AI has chosen its move, before it is applied to the board.
	 * Override in BP_GameMode to rotate the camera to Coord.FaceIndex; call ApplyPendingAIMove()
	 * when the animation is complete.
	 * The default C++ implementation applies the move immediately (no camera animation).
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Events")
	void OnAIChosenMove(FCellCoord Coord);
	virtual void OnAIChosenMove_Implementation(FCellCoord Coord);

	UFUNCTION(BlueprintNativeEvent, Category = "Events")
	void OnTurnChanged(ECellOwner NewPlayer);
	virtual void OnTurnChanged_Implementation(ECellOwner NewPlayer);

	UFUNCTION(BlueprintNativeEvent, Category = "Events")
	void OnFaceWon(EFaceIndex Face, ECellOwner Winner);
	virtual void OnFaceWon_Implementation(EFaceIndex Face, ECellOwner Winner);

	UFUNCTION(BlueprintNativeEvent, Category = "Events")
	void OnGameOver(ECellOwner Winner, bool bIsDraw);
	virtual void OnGameOver_Implementation(ECellOwner Winner, bool bIsDraw);

	/** Fired when the turn timer expires; default C++ implementation calls AdvanceTurn. */
	UFUNCTION(BlueprintNativeEvent, Category = "Events")
	void OnTurnTimerExpired(ECellOwner Player);
	virtual void OnTurnTimerExpired_Implementation(ECellOwner Player);

private:
	/** Non-owning pointer to GameState's board */
	UPROPERTY()
	TObjectPtr<UCTTBoardState> BoardState;

	/** AI component (created lazily when vs-AI game starts) */
	UPROPERTY()
	TObjectPtr<UCTTAIPlayer> AIPlayer;

	/** Cached reference to the board actor in the level */
	UPROPERTY()
	TObjectPtr<ACTTBoardActor> BoardActor;

	/** True only while ApplyPendingAIMove is calling AttemptMove — lets the AI move bypass the human-input guard. */
	bool bApplyingAIMove = false;

	/** Timer for AI think delay */
	FTimerHandle AITimerHandle;

	/** Timer for camera animation before applying the AI move */
	FTimerHandle AICameraTimerHandle;

	/** Timer for per-turn countdown */
	FTimerHandle TurnTimerHandle;

	/** World time when the current turn started (used by GetTurnTimeRemaining) */
	float TurnStartTime = 0.f;

	/** Saved remaining time when pausing, so we can restore it on resume. */
	float PausedTimeRemaining = 0.f;

	/** Check game-over conditions and fire event if needed */
	bool CheckAndHandleGameOver(const FMoveResult& MoveResult);

	/** Flip current player and fire OnTurnChanged */
	void AdvanceTurn();

	/** Start (or restart) the turn countdown timer */
	void StartTurnTimer();

	/** Called when the turn timer expires */
	void TurnTimerExpired();

	/** Trigger AI move after a short delay */
	void ScheduleAIMove();

	/** Called by timer; runs AI and applies the move */
	void ExecuteAIMove();

	// ---- Camera helpers ----

	/** Find the orbit camera on the possessed pawn, or nullptr. */
	class UCTTOrbitCameraComponent* FindOrbitCamera() const;

	/** Return canonical (Yaw, Pitch) so the camera faces the given cube face. */
	void GetFaceAngles(EFaceIndex Face, float CurrentYaw, float& OutYaw, float& OutPitch) const;

	// ---- Widget helpers ----

	void ShowHUDWidget();
	void ShowGameOverWidget(ECellOwner Winner, bool bIsDraw);
	void ShowPauseMenuWidget();
	void HidePauseMenuWidget();
};
