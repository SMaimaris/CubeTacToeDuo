#include "Game/CTTSaveGame.h"

void UCTTUserSettings::SetMasterVolume(float InVolume)
{
	MasterVolume = FMath::Clamp(InVolume, 0.f, 1.f);
}

void UCTTUserSettings::SetSFXVolume(float InVolume)
{
	SFXVolume = FMath::Clamp(InVolume, 0.f, 1.f);
}

void UCTTUserSettings::SetMusicVolume(float InVolume)
{
	MusicVolume = FMath::Clamp(InVolume, 0.f, 1.f);
}

void UCTTUserSettings::SetMuteAll(bool bInMute)
{
	bMuteAll = bInMute;
}

float UCTTUserSettings::GetEffectiveMusicVolume() const
{
	return bMuteAll ? 0.f : MasterVolume * MusicVolume;
}

float UCTTUserSettings::GetEffectiveSFXVolume() const
{
	return bMuteAll ? 0.f : MasterVolume * SFXVolume;
}

UCTTUserSettings* UCTTUserSettings::GetCTTUserSettings()
{
	return Cast<UCTTUserSettings>(UGameUserSettings::GetGameUserSettings());
}
