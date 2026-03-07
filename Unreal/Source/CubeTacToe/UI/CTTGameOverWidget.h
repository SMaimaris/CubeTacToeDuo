#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/CTTTypes.h"
#include "CTTGameOverWidget.generated.h"

class ACTTGameMode;
class UTextBlock;
class UButton;

/**
 * C++ base for WBP_GameOver.
 * Required bound widgets: Text_Result, Btn_PlayAgain, Btn_MainMenu.
 * NativeConstruct wires the buttons automatically.
 * ShowResult is BlueprintNativeEvent: C++ sets the result text by default,
 * BP override can add winner animation on top.
 */
UCLASS()
class CUBETACTOE_API UCTTGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- Required bound widgets ----

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Result;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Btn_PlayAgain;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Btn_MainMenu;

	// ---- API ----

	/**
	 * Display the outcome. Called by BP_GameMode's OnGameOver implementation.
	 * C++ sets Text_Result; override in BP to add animations.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Over")
	void ShowResult(ECellOwner Winner, bool bIsDraw);
	virtual void ShowResult_Implementation(ECellOwner Winner, bool bIsDraw);

	/** Replay with the same GameMode and WinCondition. */
	UFUNCTION(BlueprintCallable, Category = "Game Over")
	void RequestPlayAgain();

	/** Dismiss and return to the main-menu phase. */
	UFUNCTION(BlueprintCallable, Category = "Game Over")
	void RequestMainMenu();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintPure, Category = "Game Over")
	ACTTGameMode* GetCTTGameMode() const;

private:
	UFUNCTION() void OnPlayAgainClicked();
	UFUNCTION() void OnMainMenuClicked();
};
