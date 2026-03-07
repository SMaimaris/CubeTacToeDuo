#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/CTTTypes.h"
#include "CTTCellActor.generated.h"

/**
 * Leaf node representing one cell on a face.
 * No game logic — just a clickable target.
 * BP_CellActor implements OnOwnerChanged to swap mesh/material.
 */
UCLASS()
class CUBETACTOE_API ACTTCellActor : public AActor
{
	GENERATED_BODY()

public:
	ACTTCellActor();

	/** Set by CTTFaceActor at spawn time */
	UPROPERTY(BlueprintReadOnly, Category = "Cell")
	FCellCoord CellCoord;

	/** The mesh that players click on; collision responds to ECC_GameTraceChannel1 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 3-D marker shown when the cell is claimed (X or O mesh pops in here) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	/** Static mesh to assign when the cell is claimed by Player X */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cell|Meshes")
	TObjectPtr<UStaticMesh> XMarkerMesh;

	/** Static mesh to assign when the cell is claimed by Player O */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cell|Meshes")
	TObjectPtr<UStaticMesh> OMarkerMesh;

	/** Uniform scale the X marker animates to at the end of the pop-in */
	UPROPERTY(EditDefaultsOnly, Category = "Cell|Meshes")
	float XMarkerScale = 1.0f;

	/** Uniform scale the O marker animates to at the end of the pop-in */
	UPROPERTY(EditDefaultsOnly, Category = "Cell|Meshes")
	float OMarkerScale = 1.0f;

	/** Sound played at the cell's location when Player X claims it */
	UPROPERTY(EditDefaultsOnly, Category = "Cell|Sounds")
	TObjectPtr<USoundBase> XPlacementSound;

	/** Sound played at the cell's location when Player O claims it */
	UPROPERTY(EditDefaultsOnly, Category = "Cell|Sounds")
	TObjectPtr<USoundBase> OPlacementSound;

	/** Returns this cell's board coordinate */
	UFUNCTION(BlueprintCallable, Category = "Cell")
	FCellCoord GetCellCoord() const { return CellCoord; }

	/** Returns who currently owns this cell (None = empty) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cell")
	ECellOwner GetCellOwner() const { return CurrentOwner; }

	/** Called by FaceActor when the board state changes */
	UFUNCTION(BlueprintCallable, Category = "Cell")
	void SetCellOwner(ECellOwner NewOwner);

	/** Force-clear hover highlight (called on board reset) */
	void ClearHover();

	/**
	 * Called when the cell's owner changes. Default C++ implementation plays an elastic
	 * scale pop-in on MarkerMesh. BP can override (call Parent to keep the animation).
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Cell")
	void OnOwnerChanged(ECellOwner NewOwner);
	virtual void OnOwnerChanged_Implementation(ECellOwner NewOwner);

	/** Implemented in BP: show/hide hover highlight */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cell")
	void OnHoverChanged(bool bNewHovered);

	virtual void Tick(float DeltaTime) override;
	virtual void NotifyActorBeginCursorOver() override;
	virtual void NotifyActorEndCursorOver() override;

private:
	UPROPERTY()
	ECellOwner CurrentOwner = ECellOwner::None;

	bool  bHovered          = false;
	float PopInElapsed      = 0.f;
	float PopInTargetScale  = 1.0f;
	static constexpr float PopInDuration = 0.35f;

	/** Elastic ease-out: overshoots 1 then settles — gives the "pop" feel. */
	static float EaseOutElastic(float T);
};
