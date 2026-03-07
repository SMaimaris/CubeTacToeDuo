#include "Game/CTTMainMenuGameMode.h"
#include "Game/CTTGameInstance.h"
#include "UI/CTTMainMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "CubeTacToe.h"

void ACTTMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Restart music (previous component was destroyed with the old world)
	if (UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance()))
	{
		GI->StartMusic(GI->MainMenuMusic);
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	if (MainMenuWidgetClass)
	{
		MainMenuWidget = CreateWidget<UCTTMainMenuWidget>(PC, MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport(0);

			PC->bShowMouseCursor = true;
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);

			UE_LOG(LogCTT, Log, TEXT("MainMenuGameMode: Main menu widget created and shown"));
		}
	}
	else
	{
		UE_LOG(LogCTT, Warning, TEXT("MainMenuGameMode: No MainMenuWidgetClass set!"));
	}
}
