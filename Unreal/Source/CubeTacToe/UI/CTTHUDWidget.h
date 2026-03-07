#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/CTTTypes.h"
#include "CTTHUDWidget.generated.h"

class ACTTGameMode;
class ACTTGameState;
class UTextBlock;
class UProgressBar;
class UPanelWidget;

/**
 * C++ base for WBP_HUD.
 *
 * Required bound widgets:  Text_CurrentPlayer, Bar_Timer.
 * Optional bound widgets:  Text_TimeRemaining, Text_MovesRemaining,
 *                          Panel_FaceWins, Text_FacesX, Text_FacesO,
 *                          Text_ScoreX, Text_ScoreO, Text_ScoreDraws.
 *
 * Refresh* functions use BlueprintNativeEvent: C++ updates text/bar by default,
 * BP overrides can layer in animations on top.
 * NativeTick drives the timer bar every frame so no explicit ticking is needed from BP.
 */
UCLASS()
class CUBETACTOE_API UCTTHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- Required bound widgets ----

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CurrentPlayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_Timer;

	// ---- Optional bound widgets ----

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TimeRemaining;

	/** "N move(s) left" this turn */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MovesRemaining;

	/** Container shown only in WinMostFaces mode */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Panel_FaceWins;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_FacesX;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_FacesO;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ScoreX;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ScoreO;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ScoreDraws;

	// ---- Events called by BP_GameMode ----

	/**
	 * Refresh player indicator and move counter.
	 * Call from BP_GameMode's OnTurnChanged: HUDWidget->RefreshTurn(CurrentPlayer, MovesThisTurn, MovesAllowedThisTurn)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HUD")
	void RefreshTurn(ECellOwner CurrentPlayer, int32 MovesThisTurn, int32 MovesAllowed);
	virtual void RefreshTurn_Implementation(ECellOwner CurrentPlayer, int32 MovesThisTurn, int32 MovesAllowed);

	/**
	 * Refresh face-win counts.
	 * Call from BP_GameMode's OnFaceWon: HUDWidget->RefreshFaceWins(FacesX, FacesO)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HUD")
	void RefreshFaceWins(int32 FacesX, int32 FacesO);
	virtual void RefreshFaceWins_Implementation(int32 FacesX, int32 FacesO);

	/**
	 * Refresh session score strip.
	 * Call from BP_GameMode's OnGameOver after scores are recorded.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HUD")
	void RefreshScores(int32 ScoreX, int32 ScoreO, int32 Draws);
	virtual void RefreshScores_Implementation(int32 ScoreX, int32 ScoreO, int32 Draws);

	// ---- Read-only accessors (also usable as UMG bindings) ----

	UFUNCTION(BlueprintPure, Category = "HUD")
	float GetTurnTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "HUD")
	float GetTurnTimeFraction() const;

	UFUNCTION(BlueprintPure, Category = "HUD")
	ECellOwner GetCurrentPlayer() const;

	UFUNCTION(BlueprintPure, Category = "HUD")
	int32 GetMovesThisTurn() const;

	UFUNCTION(BlueprintPure, Category = "HUD")
	int32 GetMovesAllowedThisTurn() const;

	UFUNCTION(BlueprintPure, Category = "HUD")
	EWinCondition GetWinCondition() const;

	UFUNCTION(BlueprintPure, Category = "HUD")
	void GetFaceWinCounts(int32& OutX, int32& OutO) const;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "HUD")
	ACTTGameMode* GetCTTGameMode() const;

	UFUNCTION(BlueprintPure, Category = "HUD")
	ACTTGameState* GetCTTGameState() const;
};
