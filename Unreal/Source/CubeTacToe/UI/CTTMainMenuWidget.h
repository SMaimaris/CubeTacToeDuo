#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/CTTTypes.h"
#include "CTTMainMenuWidget.generated.h"

class UButton;
class UComboBoxString;
class UCTTSettingsWidget;

/**
 * C++ base for WBP_MainMenu.
 * Required bound widgets: Btn_Start, Combo_Opponent, Combo_WinCondition.
 * NativeConstruct populates the combo lists and wires all callbacks automatically.
 * The current selection is tracked in PendingGameMode / PendingWinCondition;
 * OnSelectionChanged fires so BP can update any additional highlight visuals.
 */
UCLASS()
class CUBETACTOE_API UCTTMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- Bound widgets ----

	/** Must exist in WBP_MainMenu with this exact name. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Btn_Start;

	/** Quit / Exit game button. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Btn_Quit;

	/** Opens the settings overlay. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Btn_Settings;

	/** Dropdown list for opponent selection (Local Two Player / Vs AI). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> Combo_Opponent;

	/** Dropdown list for win condition selection (Win Any Face / Win Most Faces). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> Combo_WinCondition;

	/** Dropdown list for turn order (Play First / Play Second). Only relevant in Vs AI. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> Combo_PlayerOrder;

	// ---- Pending selection (readable from BP) ----

	UPROPERTY(BlueprintReadOnly, Category = "Selection")
	EGameMode PendingGameMode = EGameMode::VsAI;

	UPROPERTY(BlueprintReadOnly, Category = "Selection")
	EWinCondition PendingWinCondition = EWinCondition::WinAnyFace;

	UPROPERTY(BlueprintReadOnly, Category = "Selection")
	ECellOwner PendingHumanPlayer = ECellOwner::PlayerX;

	// ---- API ----

	/** Apply pending settings and start. Can also be called directly from BP. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void RequestStartGame(EGameMode NewGameMode, EWinCondition NewWinCondition);

	/** Reset session scores, then start. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void RequestResetScoresAndStart(EGameMode NewGameMode, EWinCondition NewWinCondition);

	/** Widget class for the settings overlay. Set in the BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu")
	TSubclassOf<UCTTSettingsWidget> SettingsWidgetClass;

	/** Fired whenever the pending selection changes. Implement in BP for extra visuals. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
	void OnSelectionChanged(EGameMode NewMode, EWinCondition NewCondition);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

private:
	// Option strings — must match exactly between populate and selection handlers
	static const FString OpponentLocal;
	static const FString OpponentAI;
	static const FString WinAnyFace;
	static const FString WinMostFaces;
	static const FString PlayFirst;
	static const FString PlaySecond;

	UFUNCTION() void OnStartClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnSettingsClicked();
	UFUNCTION() UWidget* MakeWhiteComboText(FString Item);
	UFUNCTION() void OnOpponentChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnWinConditionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnPlayerOrderChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	void SetPendingGameMode(EGameMode NewMode);
	void SetPendingWinCondition(EWinCondition NewCondition);
};
