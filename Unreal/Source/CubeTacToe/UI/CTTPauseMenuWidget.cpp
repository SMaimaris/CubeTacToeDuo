#include "UI/CTTPauseMenuWidget.h"
#include "UI/CTTNeonStyle.h"
#include "Game/CTTGameMode.h"
#include "Game/CTTGameInstance.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "CubeTacToe.h"

void UCTTPauseMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	const FButtonStyle Style = FCTTNeonStyle::ButtonStyle();
	if (Btn_Continue) Btn_Continue->SetStyle(Style);
	if (Btn_MainMenu) Btn_MainMenu->SetStyle(Style);
}

void UCTTPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Continue) Btn_Continue->OnClicked.AddDynamic(this, &UCTTPauseMenuWidget::OnContinueClicked);
	if (Btn_MainMenu) Btn_MainMenu->OnClicked.AddDynamic(this, &UCTTPauseMenuWidget::OnMainMenuClicked);
}

void UCTTPauseMenuWidget::RequestContinue()
{
	ACTTGameMode* GM = GetCTTGameMode();
	if (!GM) return;
	UE_LOG(LogCTT, Log, TEXT("PauseMenuWidget: RequestContinue"));
	GM->ResumeGame();
}

void UCTTPauseMenuWidget::RequestMainMenu()
{
	UE_LOG(LogCTT, Log, TEXT("PauseMenuWidget: RequestMainMenu — travelling to menu map"));

	// Unpause before level travel so the engine isn't stuck paused
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	if (UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance()))
	{
		GI->OpenMainMenuMap();
	}
}

ACTTGameMode* UCTTPauseMenuWidget::GetCTTGameMode() const
{
	UWorld* World = GetWorld();
	return World ? World->GetAuthGameMode<ACTTGameMode>() : nullptr;
}

void UCTTPauseMenuWidget::OnContinueClicked()  { RequestContinue(); }
void UCTTPauseMenuWidget::OnMainMenuClicked()  { RequestMainMenu(); }
