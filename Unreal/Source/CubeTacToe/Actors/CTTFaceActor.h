#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/CTTTypes.h"
#include "CTTFaceActor.generated.h"

class ACTTCellActor;

/**
 * Holds the 9 CTTCellActors for one face of the cube plus a background plane mesh.
 * BP_FaceActor implements OnFaceWon with a win-line animation.
 */
UCLASS()
class CUBETACTOE_API ACTTFaceActor : public AActor
{
	GENERATED_BODY()

public:
	ACTTFaceActor();

	virtual void BeginPlay() override;

	/** Which face of the cube this actor represents */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Face")
	EFaceIndex FaceIndex = EFaceIndex::Top;

	/** Background plane for the face */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Face")
	TObjectPtr<UStaticMeshComponent> BackgroundMesh;

	/** The 9 cell actors (row-major: [row*3 + col]) */
	UPROPERTY(BlueprintReadOnly, Category = "Face")
	TArray<TObjectPtr<ACTTCellActor>> Cells;

	/** Space between cell centers (Unreal Units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face")
	float CellSpacing = 100.f;

	/** Class to spawn for cells (assign BP_CellActor in defaults) */
	UPROPERTY(EditDefaultsOnly, Category = "Face")
	TSubclassOf<ACTTCellActor> CellClass;

	/** Update a single cell's visual state */
	UFUNCTION(BlueprintCallable, Category = "Face")
	void UpdateCellVisual(int32 CellIndex, ECellOwner NewOwner);

	/** Reset all cell visuals to empty */
	UFUNCTION(BlueprintCallable, Category = "Face")
	void ResetVisuals();

	/** Implemented in BP: play win-line animation */
	UFUNCTION(BlueprintImplementableEvent, Category = "Face")
	void OnFaceWon(ECellOwner Winner);

private:
	void SpawnCells();
};
