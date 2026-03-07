#include "Game/CTTGameInstance.h"
#include "Game/CTTSaveGame.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "CubeTacToe.h"

void UCTTGameInstance::OnStart()
{
	Super::OnStart();
	ApplyVolumeSettings();
}

void UCTTGameInstance::Shutdown()
{
	// MusicAC is world-owned and destroyed automatically. Just clear the weak ref.
	MusicAC = nullptr;
	Super::Shutdown();
}

void UCTTGameInstance::RecordResult(ECellOwner Winner)
{
	switch (Winner)
	{
	case ECellOwner::PlayerX: ++ScoreX; break;
	case ECellOwner::PlayerO: ++ScoreO; break;
	default:                  ++Draws;  break;
	}
}

void UCTTGameInstance::ResetScores()
{
	ScoreX = 0;
	ScoreO = 0;
	Draws  = 0;
}

void UCTTGameInstance::ApplyVolumeSettings()
{
	if (!MasterSoundMix) return;

	UCTTUserSettings* Settings = UCTTUserSettings::GetCTTUserSettings();
	if (!Settings) return;

	const float Master = Settings->GetMasterVolume();
	const bool  bMute  = Settings->GetMuteAll();

	if (MusicSoundClass)
	{
		const float Vol = bMute ? 0.f : Master * Settings->GetMusicVolume();
		UGameplayStatics::SetSoundMixClassOverride(this, MasterSoundMix,
			MusicSoundClass, Vol, 1.f, 0.f);
	}

	if (SFXSoundClass)
	{
		const float Vol = bMute ? 0.f : Master * Settings->GetSFXVolume();
		UGameplayStatics::SetSoundMixClassOverride(this, MasterSoundMix,
			SFXSoundClass, Vol, 1.f, 0.f);
	}

	UGameplayStatics::PushSoundMixModifier(this, MasterSoundMix);
}

void UCTTGameInstance::StartMusic(USoundBase* Music)
{
	if (!Music) return;
	if (!GetWorld()) return;

	// Stop whatever is currently playing.
	if (IsValid(MusicAC))
	{
		MusicAC->Stop();
		MusicAC = nullptr;
	}

	MusicAC = UGameplayStatics::SpawnSound2D(this, Music, 1.f, 1.f, 0.f,
		nullptr, false, false);
	if (MusicAC)
	{
		MusicAC->bIsUISound = true;
		if (MusicSoundClass)
		{
			MusicAC->SoundClassOverride = MusicSoundClass;
		}
		UE_LOG(LogCTT, Log, TEXT("GameInstance: Music started — %s"), *Music->GetName());
	}
}

void UCTTGameInstance::OpenGameplayMap()
{
	const FString MapPath = GameplayMap.GetLongPackageName();
	UE_LOG(LogCTT, Log, TEXT("GameInstance: Opening gameplay map — %s"), *MapPath);
	UGameplayStatics::OpenLevel(this, *MapPath);
}

void UCTTGameInstance::OpenMainMenuMap()
{
	const FString MapPath = MainMenuMap.GetLongPackageName();
	UE_LOG(LogCTT, Log, TEXT("GameInstance: Opening main menu map — %s"), *MapPath);
	UGameplayStatics::OpenLevel(this, *MapPath);
}
