#include "Game/CTTGameState.h"
#include "Game/CTTBoardState.h"

ACTTGameState::ACTTGameState()
{
}

void ACTTGameState::BeginPlay()
{
	Super::BeginPlay();
	BoardState = NewObject<UCTTBoardState>(this, TEXT("BoardState"));
}
