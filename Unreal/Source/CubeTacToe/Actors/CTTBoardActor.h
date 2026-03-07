#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/CTTTypes.h"
#include "CTTBoardActor.generated.h"

class ACTTFaceActor;

/**
 * Root world object. Holds the 6 CTTFaceActors and the central cube mesh.
 * BP_BoardActor implements OnBoardReset and OnGameOverVisual (Niagara effects).
 */
UCLASS()
class CUBETACTOE_API ACTTBoardActor : public AActor
{
	GENERATED_BODY()

public:
	ACTTBoardActor();

	virtual void BeginPlay() override;

	/** Central cube body mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Board")
	TObjectPtr<UStaticMeshComponent> CubeMesh;

	/** The 6 face actors indexed by EFaceIndex */
	UPROPERTY(BlueprintReadOnly, Category = "Board")
	TArray<TObjectPtr<ACTTFaceActor>> Faces;

	/** Half-extent of the cube. Face actors are placed at this offset from center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
	float FaceOffset = 150.f;

	/** Class to spawn for faces (assign BP_FaceActor in defaults) */
	UPROPERTY(EditDefaultsOnly, Category = "Board")
	TSubclassOf<ACTTFaceActor> FaceClass;

	/** Apply a move visually: update the relevant cell actor */
	UFUNCTION(BlueprintCallable, Category = "Board")
	void ApplyMoveVisual(FCellCoord Coord, ECellOwner Player);

	/** Trigger the win animation on a face */
	UFUNCTION(BlueprintCallable, Category = "Board")
	void ShowFaceWin(EFaceIndex Face, ECellOwner Winner);

	/** Reset all face visuals to empty */
	UFUNCTION(BlueprintCallable, Category = "Board")
	void ResetVisuals();

	/** Implemented in BP: e.g., reset particle effects */
	UFUNCTION(BlueprintImplementableEvent, Category = "Board")
	void OnBoardReset();

	/** Implemented in BP: celebration Niagara effect */
	UFUNCTION(BlueprintImplementableEvent, Category = "Board")
	void OnGameOverVisual(ECellOwner Winner);

private:
	void SpawnFaces();
};
