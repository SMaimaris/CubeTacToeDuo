#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CTTMainMenuGameMode.generated.h"

class UCTTMainMenuWidget;

/**
 * Lightweight GameMode for the main menu level.
 * Creates and shows the main menu widget on BeginPlay.
 * Set this (or a BP child) as the GameMode Override on Lvl_MainMenu.
 */
UCLASS()
class CUBETACTOE_API ACTTMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** Widget class for the main menu. Set in a BP child (e.g. BP_MainMenuGameMode). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UCTTMainMenuWidget> MainMenuWidgetClass;

	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UCTTMainMenuWidget> MainMenuWidget;
};
