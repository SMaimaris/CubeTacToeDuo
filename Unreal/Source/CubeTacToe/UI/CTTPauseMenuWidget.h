#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTTPauseMenuWidget.generated.h"

class ACTTGameMode;
class UButton;

/**
 * C++ base for WBP_PauseMenu.
 * Required bound widgets: Btn_Continue, Btn_MainMenu.
 */
UCLASS()
class CUBETACTOE_API UCTTPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Btn_Continue;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Btn_MainMenu;

	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void RequestContinue();

	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void RequestMainMenu();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintPure, Category = "Pause Menu")
	ACTTGameMode* GetCTTGameMode() const;

private:
	UFUNCTION() void OnContinueClicked();
	UFUNCTION() void OnMainMenuClicked();
};
