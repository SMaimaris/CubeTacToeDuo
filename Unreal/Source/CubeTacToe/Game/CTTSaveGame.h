#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "CTTSaveGame.generated.h"

/**
 * Extends GameUserSettings with game-specific audio settings.
 * Automatically saved/loaded to GameUserSettings.ini.
 * Set in DefaultEngine.ini:
 *   [/Script/Engine.Engine]
 *   GameUserSettingsClassName=/Script/CubeTacToe.CTTUserSettings
 */
UCLASS()
class CUBETACTOE_API UCTTUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	// ---- Audio ----

	UPROPERTY(Config)
	float MasterVolume = 0.2f;

	UPROPERTY(Config)
	float SFXVolume = 1.f;

	UPROPERTY(Config)
	float MusicVolume = 1.f;

	UPROPERTY(Config)
	bool bMuteAll = false;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float InVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float InVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float InVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMuteAll(bool bInMute);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetSFXVolume() const { return SFXVolume; }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	bool GetMuteAll() const { return bMuteAll; }

	/** Effective music volume = MasterVolume * MusicVolume (0 if muted). */
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetEffectiveMusicVolume() const;

	/** Effective SFX volume = MasterVolume * SFXVolume (0 if muted). */
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetEffectiveSFXVolume() const;

	// ---- Convenience ----

	UFUNCTION(BlueprintCallable, Category = "Settings", meta = (DisplayName = "Get CTT User Settings"))
	static UCTTUserSettings* GetCTTUserSettings();
};
