#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Types/CTTTypes.h"
#include "CTTGameInstance.generated.h"

class UAudioComponent;
class USoundMix;
class USoundClass;

/**
 * Persists game settings and session scores across level travel.
 * Set this as the GameInstance class in Project Settings → Maps & Modes.
 */
UCLASS()
class CUBETACTOE_API UCTTGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void OnStart() override;
	virtual void Shutdown() override;

	// ---- Settings (written by main menu, read by gameplay GameMode) ----

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	EGameMode PendingGameMode = EGameMode::VsAI;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	EWinCondition PendingWinCondition = EWinCondition::WinAnyFace;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	ECellOwner PendingHumanPlayer = ECellOwner::PlayerX;

	// ---- Session scores (survive level travel) ----

	UPROPERTY(BlueprintReadOnly, Category = "Scores")
	int32 ScoreX = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Scores")
	int32 ScoreO = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Scores")
	int32 Draws = 0;

	UFUNCTION(BlueprintCallable, Category = "Scores")
	void RecordResult(ECellOwner Winner);

	UFUNCTION(BlueprintCallable, Category = "Scores")
	void ResetScores();

	// ---- Audio ----

	/** Music for the main menu level. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> MainMenuMusic;

	/** Music for the gameplay level. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> GameplayMusic;

	/** Sound mix used to control per-class volume. Create a blank USoundMix asset and assign here. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundMix> MasterSoundMix;

	/** Sound class for background music. Assign to the music asset and here. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundClass> MusicSoundClass;

	/** Sound class for SFX (placement sounds, etc.). Assign to SFX assets and here. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundClass> SFXSoundClass;

	/** Apply the current volume settings via Sound Mix. */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void ApplyVolumeSettings();

	/** Start the given music track. Stops any currently playing music first. */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StartMusic(USoundBase* Music);

	// ---- Map paths ----

	/** Path to the gameplay level. Override in BP if needed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Maps")
	FSoftObjectPath GameplayMap = FSoftObjectPath(TEXT("/Game/Maps/Lvl_Default.Lvl_Default"));

	/** Path to the main menu level. Override in BP if needed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Maps")
	FSoftObjectPath MainMenuMap = FSoftObjectPath(TEXT("/Game/Maps/Lvl_MainMenu.Lvl_MainMenu"));

	/** Open the gameplay level. */
	UFUNCTION(BlueprintCallable, Category = "Maps")
	void OpenGameplayMap();

	/** Open the main menu level. */
	UFUNCTION(BlueprintCallable, Category = "Maps")
	void OpenMainMenuMap();

private:
	/** World-spawned music component for the current level. Not persisted across travel. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicAC;
};
