#include "Actors/CTTFaceActor.h"
#include "Actors/CTTCellActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

ACTTFaceActor::ACTTFaceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BackgroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackgroundMesh"));
	SetRootComponent(BackgroundMesh);
	BackgroundMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Default to CTTCellActor; blueprint can override with BP_CellActor
	CellClass = ACTTCellActor::StaticClass();
}

void ACTTFaceActor::BeginPlay()
{
	Super::BeginPlay();
	SpawnCells();
}

void ACTTFaceActor::SpawnCells()
{
	if (!CellClass) return;

	Cells.Empty();
	Cells.Reserve(9);

	// 3×3 grid in local space, centered at actor origin
	// Row 0 = top (+Y), Col 0 = left (-X) in face local space
	for (int32 Row = 0; Row < 3; ++Row)
	{
		for (int32 Col = 0; Col < 3; ++Col)
		{
			// Offset: centre row/col = 0, outer = ±CellSpacing
			float LocalX = (Col - 1) * CellSpacing;
			float LocalY = (1 - Row) * CellSpacing; // row 0 at +Y

			// Small +Z offset so cells are always in front of the BackgroundMesh,
			// preventing z-fighting (especially visible on the coplanar centre cell).
			FVector LocalOffset(LocalX, LocalY, 0.01f);
			FVector WorldPos = GetActorTransform().TransformPosition(LocalOffset);
			FRotator WorldRot = GetActorRotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride =
			    ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			ACTTCellActor* Cell = GetWorld()->SpawnActor<ACTTCellActor>(
			    CellClass, WorldPos, WorldRot, SpawnParams);

			if (Cell)
			{
				Cell->CellCoord.FaceIndex = FaceIndex;
				Cell->CellCoord.Row       = Row;
				Cell->CellCoord.Col       = Col;
				Cell->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
				Cells.Add(Cell);
			}
		}
	}
}

void ACTTFaceActor::UpdateCellVisual(int32 CellIndex, ECellOwner NewOwner)
{
	if (Cells.IsValidIndex(CellIndex) && Cells[CellIndex])
	{
		Cells[CellIndex]->SetCellOwner(NewOwner);
	}
}

void ACTTFaceActor::ResetVisuals()
{
	for (TObjectPtr<ACTTCellActor>& Cell : Cells)
	{
		if (Cell)
		{
			Cell->ClearHover();
			Cell->SetCellOwner(ECellOwner::None);
		}
	}
}
