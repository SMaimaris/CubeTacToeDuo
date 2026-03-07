#include "UI/CTTSettingsWidget.h"
#include "UI/CTTNeonStyle.h"
#include "Game/CTTGameInstance.h"
#include "Game/CTTSaveGame.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameUserSettings.h"
#include "RHI.h"
#include "CubeTacToe.h"

// ---- Helpers for styling sliders ----

static void StyleSlider(USlider* Slider)
{
	if (!Slider) return;
	Slider->SetSliderBarColor(FCTTNeonStyle::ElectricBlue());
	Slider->SetSliderHandleColor(FCTTNeonStyle::Phlox());
}

// ---- Widget overrides ----

void UCTTSettingsWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Btn_Back) Btn_Back->SetStyle(FCTTNeonStyle::ButtonStyle());

	StyleSlider(Slider_MasterVolume);
	StyleSlider(Slider_SFXVolume);
	StyleSlider(Slider_MusicVolume);
	if (Text_MasterPercent)
		Text_MasterPercent->SetColorAndOpacity(FCTTNeonStyle::TyrianPurple());
	if (Text_SFXPercent)
		Text_SFXPercent->SetColorAndOpacity(FCTTNeonStyle::TyrianPurple());
	if (Text_MusicPercent)
		Text_MusicPercent->SetColorAndOpacity(FCTTNeonStyle::TyrianPurple());

	if (Combo_Resolution)
	{
		Combo_Resolution->SetWidgetStyle(FCTTNeonStyle::ComboBoxStyle());
		Combo_Resolution->SetItemStyle(FCTTNeonStyle::ComboItemStyle());
	}
}

void UCTTSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UCTTUserSettings* Settings = UCTTUserSettings::GetCTTUserSettings();

	// ---- Back button ----
	if (Btn_Back)
	{
		Btn_Back->OnClicked.RemoveDynamic(this, &UCTTSettingsWidget::OnBackClicked);
		Btn_Back->OnClicked.AddDynamic(this, &UCTTSettingsWidget::OnBackClicked);
	}

	// ---- Master Volume ----
	if (Slider_MasterVolume)
	{
		Slider_MasterVolume->OnValueChanged.RemoveDynamic(this, &UCTTSettingsWidget::OnMasterVolumeChanged);
		const float MasterVal = Settings ? Settings->GetMasterVolume() : 1.f;
		Slider_MasterVolume->SetValue(MasterVal);
		UpdatePercentText(Text_MasterPercent, MasterVal);
		Slider_MasterVolume->OnValueChanged.AddDynamic(this, &UCTTSettingsWidget::OnMasterVolumeChanged);
	}

	// ---- SFX Volume ----
	if (Slider_SFXVolume)
	{
		Slider_SFXVolume->OnValueChanged.RemoveDynamic(this, &UCTTSettingsWidget::OnSFXVolumeChanged);
		const float SFXVal = Settings ? Settings->GetSFXVolume() : 1.f;
		Slider_SFXVolume->SetValue(SFXVal);
		UpdatePercentText(Text_SFXPercent, SFXVal);
		Slider_SFXVolume->OnValueChanged.AddDynamic(this, &UCTTSettingsWidget::OnSFXVolumeChanged);
	}

	// ---- Music Volume ----
	if (Slider_MusicVolume)
	{
		Slider_MusicVolume->OnValueChanged.RemoveDynamic(this, &UCTTSettingsWidget::OnMusicVolumeChanged);
		const float MusicVal = Settings ? Settings->GetMusicVolume() : 1.f;
		Slider_MusicVolume->SetValue(MusicVal);
		UpdatePercentText(Text_MusicPercent, MusicVal);
		Slider_MusicVolume->OnValueChanged.AddDynamic(this, &UCTTSettingsWidget::OnMusicVolumeChanged);
	}

	// ---- Mute All ----
	if (Check_MuteAll)
	{
		Check_MuteAll->OnCheckStateChanged.RemoveDynamic(this, &UCTTSettingsWidget::OnMuteAllChanged);
		Check_MuteAll->SetIsChecked(Settings ? Settings->GetMuteAll() : false);
		Check_MuteAll->OnCheckStateChanged.AddDynamic(this, &UCTTSettingsWidget::OnMuteAllChanged);
	}

	// ---- Resolution ----
	if (Combo_Resolution)
	{
		Combo_Resolution->OnSelectionChanged.RemoveDynamic(this, &UCTTSettingsWidget::OnResolutionChanged);
		Combo_Resolution->OnGenerateWidgetEvent.BindDynamic(this, &UCTTSettingsWidget::MakeWhiteComboText);
		PopulateResolutions();
		Combo_Resolution->OnSelectionChanged.AddDynamic(this, &UCTTSettingsWidget::OnResolutionChanged);
	}

	// ---- VSync ----
	if (Check_VSync)
	{
		Check_VSync->OnCheckStateChanged.RemoveDynamic(this, &UCTTSettingsWidget::OnVSyncChanged);
		Check_VSync->SetIsChecked(Settings ? Settings->IsVSyncEnabled() : false);
		Check_VSync->OnCheckStateChanged.AddDynamic(this, &UCTTSettingsWidget::OnVSyncChanged);
	}

	// ---- Fullscreen ----
	if (Check_Fullscreen)
	{
		Check_Fullscreen->OnCheckStateChanged.RemoveDynamic(this, &UCTTSettingsWidget::OnFullscreenChanged);
		const bool bFullscreen = Settings
			? (Settings->GetFullscreenMode() == EWindowMode::Fullscreen ||
			   Settings->GetFullscreenMode() == EWindowMode::WindowedFullscreen)
			: false;
		Check_Fullscreen->SetIsChecked(bFullscreen);
		Check_Fullscreen->OnCheckStateChanged.AddDynamic(this, &UCTTSettingsWidget::OnFullscreenChanged);
	}

	UE_LOG(LogCTT, Log, TEXT("SettingsWidget: Constructed"));
}

// ---- Callbacks ----

void UCTTSettingsWidget::OnBackClicked()
{
	UE_LOG(LogCTT, Log, TEXT("SettingsWidget: Back"));
	RemoveFromParent();
}

void UCTTSettingsWidget::OnMasterVolumeChanged(float Value)
{
	if (UCTTUserSettings* S = UCTTUserSettings::GetCTTUserSettings())
	{
		S->SetMasterVolume(Value);
	}
	UpdatePercentText(Text_MasterPercent, Value);
	ApplyAudioAndSave();
}

void UCTTSettingsWidget::OnSFXVolumeChanged(float Value)
{
	if (UCTTUserSettings* S = UCTTUserSettings::GetCTTUserSettings())
	{
		S->SetSFXVolume(Value);
	}
	UpdatePercentText(Text_SFXPercent, Value);
	ApplyAudioAndSave();
}

void UCTTSettingsWidget::OnMusicVolumeChanged(float Value)
{
	if (UCTTUserSettings* S = UCTTUserSettings::GetCTTUserSettings())
	{
		S->SetMusicVolume(Value);
	}
	UpdatePercentText(Text_MusicPercent, Value);
	ApplyAudioAndSave();
}

void UCTTSettingsWidget::OnMuteAllChanged(bool bIsChecked)
{
	if (UCTTUserSettings* S = UCTTUserSettings::GetCTTUserSettings())
	{
		S->SetMuteAll(bIsChecked);
	}
	ApplyAudioAndSave();
}

void UCTTSettingsWidget::OnResolutionChanged(FString SelectedItem, ESelectInfo::Type /*SelectionType*/)
{
	int32 SepIndex;
	if (!SelectedItem.FindChar(TEXT('x'), SepIndex)) return;

	const int32 Width  = FCString::Atoi(*SelectedItem.Left(SepIndex));
	const int32 Height = FCString::Atoi(*SelectedItem.Mid(SepIndex + 1));
	if (Width <= 0 || Height <= 0) return;

	if (UCTTUserSettings* S = UCTTUserSettings::GetCTTUserSettings())
	{
		S->SetScreenResolution(FIntPoint(Width, Height));
		S->ApplyResolutionSettings(false);
		S->SaveSettings();
	}
	UE_LOG(LogCTT, Log, TEXT("SettingsWidget: Resolution → %dx%d"), Width, Height);
}

void UCTTSettingsWidget::OnVSyncChanged(bool bIsChecked)
{
	if (UCTTUserSettings* S = UCTTUserSettings::GetCTTUserSettings())
	{
		S->SetVSyncEnabled(bIsChecked);
		S->ApplySettings(false);
		S->SaveSettings();
	}
	UE_LOG(LogCTT, Log, TEXT("SettingsWidget: VSync → %s"), bIsChecked ? TEXT("ON") : TEXT("OFF"));
}

void UCTTSettingsWidget::OnFullscreenChanged(bool bIsChecked)
{
	if (UCTTUserSettings* S = UCTTUserSettings::GetCTTUserSettings())
	{
		S->SetFullscreenMode(bIsChecked ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
		S->ApplyResolutionSettings(false);
		S->SaveSettings();
	}
	UE_LOG(LogCTT, Log, TEXT("SettingsWidget: Fullscreen → %s"), bIsChecked ? TEXT("ON") : TEXT("OFF"));
}

// ---- Helpers ----

void UCTTSettingsWidget::ApplyAudioAndSave()
{
	if (UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance()))
	{
		GI->ApplyVolumeSettings();
	}
	if (UCTTUserSettings* S = UCTTUserSettings::GetCTTUserSettings())
	{
		S->SaveSettings();
	}
}

void UCTTSettingsWidget::PopulateResolutions()
{
	Combo_Resolution->ClearOptions();
	Resolutions.Empty();

	FScreenResolutionArray ScreenResolutions;
	if (RHIGetAvailableResolutions(ScreenResolutions, true))
	{
		for (const FScreenResolutionRHI& Res : ScreenResolutions)
		{
			if (Res.Width >= 1280)
			{
				Resolutions.AddUnique(FIntPoint(Res.Width, Res.Height));
			}
		}
	}

	if (Resolutions.IsEmpty())
	{
		Resolutions.Add(FIntPoint(1280, 720));
		Resolutions.Add(FIntPoint(1600, 900));
		Resolutions.Add(FIntPoint(1920, 1080));
		Resolutions.Add(FIntPoint(2560, 1440));
		Resolutions.Add(FIntPoint(3840, 2160));
	}

	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return A.X < B.X || (A.X == B.X && A.Y < B.Y);
	});

	FIntPoint CurrentRes(1920, 1080);
	if (UGameUserSettings* S = UGameUserSettings::GetGameUserSettings())
	{
		CurrentRes = S->GetScreenResolution();
	}

	FString CurrentResStr;
	for (const FIntPoint& Res : Resolutions)
	{
		const FString Label = FString::Printf(TEXT("%dx%d"), Res.X, Res.Y);
		Combo_Resolution->AddOption(Label);
		if (Res == CurrentRes)
		{
			CurrentResStr = Label;
		}
	}

	if (CurrentResStr.IsEmpty())
	{
		CurrentResStr = FString::Printf(TEXT("%dx%d"), CurrentRes.X, CurrentRes.Y);
		Combo_Resolution->AddOption(CurrentResStr);
	}

	Combo_Resolution->SetSelectedOption(CurrentResStr);
}

UWidget* UCTTSettingsWidget::MakeWhiteComboText(FString Item)
{
	UTextBlock* Text = NewObject<UTextBlock>(this);
	Text->SetText(FText::FromString(Item));
	Text->SetColorAndOpacity(FLinearColor::White);
	return Text;
}

void UCTTSettingsWidget::UpdatePercentText(UTextBlock* TextBlock, float Value)
{
	if (!TextBlock) return;
	const int32 Percent = FMath::RoundToInt(Value * 100.f);
	TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), Percent)));
}
