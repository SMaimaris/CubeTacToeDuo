#include "UI/CTTMainMenuWidget.h"
#include "UI/CTTNeonStyle.h"
#include "UI/CTTSettingsWidget.h"
#include "Game/CTTGameInstance.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "CubeTacToe.h"

// ---- Option string constants ----

const FString UCTTMainMenuWidget::OpponentLocal  = TEXT("Local Two Player");
const FString UCTTMainMenuWidget::OpponentAI     = TEXT("Vs AI");
const FString UCTTMainMenuWidget::WinAnyFace     = TEXT("Win Any Face");
const FString UCTTMainMenuWidget::WinMostFaces   = TEXT("Win Most Faces");
const FString UCTTMainMenuWidget::PlayFirst      = TEXT("Play First (X)");
const FString UCTTMainMenuWidget::PlaySecond     = TEXT("Play Second (O)");

// ---- UUserWidget overrides ----

void UCTTMainMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Btn_Start)    Btn_Start->SetStyle(FCTTNeonStyle::ButtonStyle());
	if (Btn_Quit)     Btn_Quit->SetStyle(FCTTNeonStyle::ButtonStyle());
	if (Btn_Settings) Btn_Settings->SetStyle(FCTTNeonStyle::ButtonStyle());

	const FComboBoxStyle ComboStyle = FCTTNeonStyle::ComboBoxStyle();
	const FTableRowStyle ItemStyle  = FCTTNeonStyle::ComboItemStyle();

	for (UComboBoxString* Combo : { Combo_Opponent, Combo_WinCondition, Combo_PlayerOrder })
	{
		if (!Combo) continue;
		Combo->SetWidgetStyle(ComboStyle);
		Combo->SetItemStyle(ItemStyle);
	}
}

void UCTTMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Wire Start button (safe to call repeatedly — AddUniqueDynamic guards duplicates)
	if (Btn_Start)
	{
		Btn_Start->OnClicked.RemoveDynamic(this, &UCTTMainMenuWidget::OnStartClicked);
		Btn_Start->OnClicked.AddDynamic(this, &UCTTMainMenuWidget::OnStartClicked);
	}

	// Wire Quit button
	if (Btn_Quit)
	{
		Btn_Quit->OnClicked.RemoveDynamic(this, &UCTTMainMenuWidget::OnQuitClicked);
		Btn_Quit->OnClicked.AddDynamic(this, &UCTTMainMenuWidget::OnQuitClicked);
	}

	// Wire Settings button
	if (Btn_Settings)
	{
		Btn_Settings->OnClicked.RemoveDynamic(this, &UCTTMainMenuWidget::OnSettingsClicked);
		Btn_Settings->OnClicked.AddDynamic(this, &UCTTMainMenuWidget::OnSettingsClicked);
	}

	// Bind custom widget generator so combo text is always white.
	for (UComboBoxString* Combo : { Combo_Opponent, Combo_WinCondition, Combo_PlayerOrder })
	{
		if (Combo)
		{
			Combo->OnGenerateWidgetEvent.BindDynamic(this, &UCTTMainMenuWidget::MakeWhiteComboText);
		}
	}

	// Populate and wire Opponent combo
	// RemoveDynamic first so SetSelectedOption doesn't fire stale handlers
	if (Combo_Opponent)
	{
		Combo_Opponent->OnSelectionChanged.RemoveDynamic(this, &UCTTMainMenuWidget::OnOpponentChanged);
		Combo_Opponent->ClearOptions();
		Combo_Opponent->AddOption(OpponentLocal);
		Combo_Opponent->AddOption(OpponentAI);
		Combo_Opponent->SetSelectedOption(
			PendingGameMode == EGameMode::VsAI ? OpponentAI : OpponentLocal);
		Combo_Opponent->OnSelectionChanged.AddDynamic(
			this, &UCTTMainMenuWidget::OnOpponentChanged);
	}

	// Populate and wire Win Condition combo
	if (Combo_WinCondition)
	{
		Combo_WinCondition->OnSelectionChanged.RemoveDynamic(this, &UCTTMainMenuWidget::OnWinConditionChanged);
		Combo_WinCondition->ClearOptions();
		Combo_WinCondition->AddOption(WinAnyFace);
		Combo_WinCondition->AddOption(WinMostFaces);
		Combo_WinCondition->SetSelectedOption(
			PendingWinCondition == EWinCondition::WinMostFaces ? WinMostFaces : WinAnyFace);
		Combo_WinCondition->OnSelectionChanged.AddDynamic(
			this, &UCTTMainMenuWidget::OnWinConditionChanged);
	}

	// Populate and wire Player Order combo
	if (Combo_PlayerOrder)
	{
		Combo_PlayerOrder->OnSelectionChanged.RemoveDynamic(this, &UCTTMainMenuWidget::OnPlayerOrderChanged);
		Combo_PlayerOrder->ClearOptions();
		Combo_PlayerOrder->AddOption(PlayFirst);
		Combo_PlayerOrder->AddOption(PlaySecond);
		Combo_PlayerOrder->SetSelectedOption(
			PendingHumanPlayer == ECellOwner::PlayerO ? PlaySecond : PlayFirst);
		Combo_PlayerOrder->OnSelectionChanged.AddDynamic(
			this, &UCTTMainMenuWidget::OnPlayerOrderChanged);
	}

	UE_LOG(LogCTT, Log, TEXT("MainMenuWidget: Constructed — defaults Mode=%s WinCondition=%s"),
	       *UEnum::GetValueAsString(PendingGameMode), *UEnum::GetValueAsString(PendingWinCondition));
}

// ---- Public API ----

void UCTTMainMenuWidget::RequestStartGame(EGameMode NewGameMode, EWinCondition NewWinCondition)
{
	UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->PendingGameMode     = NewGameMode;
	GI->PendingWinCondition = NewWinCondition;
	GI->PendingHumanPlayer  = PendingHumanPlayer;

	UE_LOG(LogCTT, Log, TEXT("MainMenuWidget: StartGame — Mode=%s WinCondition=%s, travelling to gameplay map"),
	       *UEnum::GetValueAsString(NewGameMode), *UEnum::GetValueAsString(NewWinCondition));

	GI->OpenGameplayMap();
}

void UCTTMainMenuWidget::RequestResetScoresAndStart(EGameMode NewGameMode, EWinCondition NewWinCondition)
{
	if (UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance()))
	{
		GI->ResetScores();
		UE_LOG(LogCTT, Log, TEXT("MainMenuWidget: Scores reset"));
	}

	RequestStartGame(NewGameMode, NewWinCondition);
}

// ---- Private callbacks ----

void UCTTMainMenuWidget::OnStartClicked()
{
	RequestStartGame(PendingGameMode, PendingWinCondition);
}

void UCTTMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

void UCTTMainMenuWidget::OnSettingsClicked()
{
	if (!SettingsWidgetClass) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	UCTTSettingsWidget* SettingsWidget = CreateWidget<UCTTSettingsWidget>(PC, SettingsWidgetClass);
	if (SettingsWidget)
	{
		SettingsWidget->AddToViewport(5);
		UE_LOG(LogCTT, Log, TEXT("MainMenuWidget: Settings overlay opened"));
	}
}

UWidget* UCTTMainMenuWidget::MakeWhiteComboText(FString Item)
{
	UTextBlock* Text = NewObject<UTextBlock>(this);
	Text->SetText(FText::FromString(Item));
	Text->SetColorAndOpacity(FLinearColor::White);
	return Text;
}

void UCTTMainMenuWidget::OnOpponentChanged(FString SelectedItem, ESelectInfo::Type /*SelectionType*/)
{
	SetPendingGameMode(SelectedItem == OpponentAI ? EGameMode::VsAI : EGameMode::LocalTwoPlayer);
}

void UCTTMainMenuWidget::OnWinConditionChanged(FString SelectedItem, ESelectInfo::Type /*SelectionType*/)
{
	SetPendingWinCondition(SelectedItem == WinMostFaces ? EWinCondition::WinMostFaces : EWinCondition::WinAnyFace);
}

void UCTTMainMenuWidget::OnPlayerOrderChanged(FString SelectedItem, ESelectInfo::Type /*SelectionType*/)
{
	PendingHumanPlayer = (SelectedItem == PlaySecond) ? ECellOwner::PlayerO : ECellOwner::PlayerX;
	UE_LOG(LogCTT, Log, TEXT("MainMenuWidget: PendingHumanPlayer → %s"),
	       *UEnum::GetValueAsString(PendingHumanPlayer));
	OnSelectionChanged(PendingGameMode, PendingWinCondition);
}

void UCTTMainMenuWidget::SetPendingGameMode(EGameMode NewMode)
{
	PendingGameMode = NewMode;
	UE_LOG(LogCTT, Log, TEXT("MainMenuWidget: PendingGameMode → %s"), *UEnum::GetValueAsString(NewMode));
	OnSelectionChanged(PendingGameMode, PendingWinCondition);
}

void UCTTMainMenuWidget::SetPendingWinCondition(EWinCondition NewCondition)
{
	PendingWinCondition = NewCondition;
	UE_LOG(LogCTT, Log, TEXT("MainMenuWidget: PendingWinCondition → %s"), *UEnum::GetValueAsString(NewCondition));
	OnSelectionChanged(PendingGameMode, PendingWinCondition);
}
