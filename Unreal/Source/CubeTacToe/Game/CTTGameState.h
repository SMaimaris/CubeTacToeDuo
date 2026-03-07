#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Types/CTTTypes.h"
#include "CTTGameState.generated.h"

class UCTTBoardState;

/**
 * Owns the authoritative UCTTBoardState for the current game.
 * Session scores have moved to UCTTGameInstance (survives level travel).
 */
UCLASS()
class CUBETACTOE_API ACTTGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ACTTGameState();

	virtual void BeginPlay() override;

	/** The authoritative board. GameMode holds a non-owning pointer to this. */
	UPROPERTY(BlueprintReadOnly, Category = "Board")
	TObjectPtr<UCTTBoardState> BoardState;
};
