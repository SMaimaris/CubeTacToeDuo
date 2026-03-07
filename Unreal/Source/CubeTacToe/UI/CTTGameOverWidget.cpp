#include "UI/CTTGameOverWidget.h"
#include "UI/CTTNeonStyle.h"
#include "Game/CTTGameMode.h"
#include "Game/CTTGameInstance.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "CubeTacToe.h"

void UCTTGameOverWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	const FButtonStyle Style = FCTTNeonStyle::ButtonStyle();
	if (Btn_PlayAgain) Btn_PlayAgain->SetStyle(Style);
	if (Btn_MainMenu)  Btn_MainMenu->SetStyle(Style);
}

void UCTTGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_PlayAgain) Btn_PlayAgain->OnClicked.AddDynamic(this, &UCTTGameOverWidget::OnPlayAgainClicked);
	if (Btn_MainMenu)  Btn_MainMenu->OnClicked.AddDynamic(this, &UCTTGameOverWidget::OnMainMenuClicked);
}

void UCTTGameOverWidget::ShowResult_Implementation(ECellOwner Winner, bool bIsDraw)
{
	if (!Text_Result) return;

	FString ResultStr;
	if (bIsDraw)
	{
		ResultStr = TEXT("Draw!");
		Text_Result->SetColorAndOpacity(FSlateColor(FCTTNeonStyle::Phlox()));
	}
	else if (Winner == ECellOwner::PlayerX)
	{
		ResultStr = TEXT("X Wins!");
		Text_Result->SetColorAndOpacity(FSlateColor(FCTTNeonStyle::PlayerX()));
	}
	else
	{
		ResultStr = TEXT("O Wins!");
		Text_Result->SetColorAndOpacity(FSlateColor(FCTTNeonStyle::PlayerO()));
	}

	Text_Result->SetText(FText::FromString(ResultStr));
	UE_LOG(LogCTT, Log, TEXT("GameOverWidget: ShowResult — %s"), *ResultStr);
}

void UCTTGameOverWidget::RequestPlayAgain()
{
	ACTTGameMode* GM = GetCTTGameMode();
	if (!GM) return;
	UE_LOG(LogCTT, Log, TEXT("GameOverWidget: RequestPlayAgain"));
	GM->StartNewGame();
}

void UCTTGameOverWidget::RequestMainMenu()
{
	UE_LOG(LogCTT, Log, TEXT("GameOverWidget: RequestMainMenu — travelling to menu map"));
	if (UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance()))
	{
		GI->OpenMainMenuMap();
	}
}

ACTTGameMode* UCTTGameOverWidget::GetCTTGameMode() const
{
	UWorld* World = GetWorld();
	return World ? World->GetAuthGameMode<ACTTGameMode>() : nullptr;
}

void UCTTGameOverWidget::OnPlayAgainClicked()  { RequestPlayAgain(); }
void UCTTGameOverWidget::OnMainMenuClicked()   { RequestMainMenu(); }
