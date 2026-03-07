#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTTSettingsWidget.generated.h"

class UButton;
class USlider;
class UCheckBox;
class UComboBoxString;
class UTextBlock;

/**
 * C++ base for WBP_Settings.
 *
 * Required bound widgets:
 *   Btn_Back, Slider_MasterVolume, Slider_SFXVolume, Slider_MusicVolume,
 *   Check_MuteAll, Combo_Resolution, Check_VSync, Check_Fullscreen.
 */
UCLASS()
class CUBETACTOE_API UCTTSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- Bound widgets ----

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Btn_Back;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<USlider> Slider_MasterVolume;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<USlider> Slider_SFXVolume;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<USlider> Slider_MusicVolume;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MasterPercent;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SFXPercent;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MusicPercent;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCheckBox> Check_MuteAll;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> Combo_Resolution;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCheckBox> Check_VSync;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCheckBox> Check_Fullscreen;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

private:
	UFUNCTION() void OnBackClicked();
	UFUNCTION() void OnMasterVolumeChanged(float Value);
	UFUNCTION() void OnSFXVolumeChanged(float Value);
	UFUNCTION() void OnMusicVolumeChanged(float Value);
	UFUNCTION() void OnMuteAllChanged(bool bIsChecked);
	UFUNCTION() void OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnVSyncChanged(bool bIsChecked);
	UFUNCTION() void OnFullscreenChanged(bool bIsChecked);
	UFUNCTION() UWidget* MakeWhiteComboText(FString Item);

	void PopulateResolutions();
	void ApplyAudioAndSave();
	void UpdatePercentText(UTextBlock* TextBlock, float Value);

	TArray<FIntPoint> Resolutions;
};
