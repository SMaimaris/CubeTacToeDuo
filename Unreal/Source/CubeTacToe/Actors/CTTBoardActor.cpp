#include "Actors/CTTBoardActor.h"
#include "Actors/CTTFaceActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

ACTTBoardActor::ACTTBoardActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	SetRootComponent(CubeMesh);
	CubeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FaceClass = ACTTFaceActor::StaticClass();
}

void ACTTBoardActor::BeginPlay()
{
	Super::BeginPlay();
	SpawnFaces();
}

void ACTTBoardActor::SpawnFaces()
{
	if (!FaceClass) return;

	// Offsets and rotations for each face, in local space of the board actor.
	// EFaceIndex order: Top(0), Front(1), Right(2), Back(3), Left(4), Bottom(5)
	struct FFacePlacement
	{
		FVector  Offset;
		FRotator Rotation;
	};

	// Cells lie in the face actor's local XY plane (Z = 0), so each face's
	// local Z axis must point outward from the cube centre.
	//
	//   Pitch -90  → local Z becomes +X world  (Front)
	//   Pitch +90  → local Z becomes -X world  (Back)
	//   Roll  +90  → local Z becomes +Y world  (Right)
	//   Roll  -90  → local Z becomes -Y world  (Left)
	//   Pitch 180  → local Z becomes -Z world  (Bottom)
	const FFacePlacement Placements[6] =
	{
		// Top    — local Z = +Z world (identity)
		{ FVector(0,            0,           FaceOffset), FRotator(  0, 0,   0) },
		// Front  — local Z = +X world (Pitch -90)
		{ FVector( FaceOffset,  0,           0         ), FRotator(-90, 0,   0) },
		// Right  — local Z = +Y world (Roll +90)
		{ FVector(0,            FaceOffset,  0         ), FRotator(  0, 0,  90) },
		// Back   — local Z = -X world (Pitch +90)
		{ FVector(-FaceOffset,  0,           0         ), FRotator( 90, 0,   0) },
		// Left   — local Z = -Y world (Roll -90)
		{ FVector(0,           -FaceOffset,  0         ), FRotator(  0, 0, -90) },
		// Bottom — local Z = -Z world (Pitch 180)
		{ FVector(0,            0,          -FaceOffset), FRotator(180, 0,   0) },
	};

	Faces.Empty();
	Faces.Reserve(6);

	for (int32 i = 0; i < 6; ++i)
	{
		FVector WorldOffset = GetActorTransform().TransformPosition(Placements[i].Offset);
		FRotator WorldRot   = (GetActorQuat() * Placements[i].Rotation.Quaternion()).Rotator();
		FTransform SpawnTransform(WorldRot, WorldOffset);

		// Deferred spawn: set FaceIndex BEFORE BeginPlay fires so SpawnCells()
		// reads the correct face index when assigning cell coords.
		ACTTFaceActor* Face = GetWorld()->SpawnActorDeferred<ACTTFaceActor>(
		    FaceClass, SpawnTransform, this, nullptr,
		    ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (Face)
		{
			Face->FaceIndex = static_cast<EFaceIndex>(i);
			Face->FinishSpawning(SpawnTransform);
			Face->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			Faces.Add(Face);
		}
	}
}

void ACTTBoardActor::ApplyMoveVisual(FCellCoord Coord, ECellOwner Player)
{
	const int32 FaceIdx = static_cast<int32>(Coord.FaceIndex);
	if (Faces.IsValidIndex(FaceIdx) && Faces[FaceIdx])
	{
		Faces[FaceIdx]->UpdateCellVisual(Coord.CellIndex(), Player);
	}
}

void ACTTBoardActor::ShowFaceWin(EFaceIndex Face, ECellOwner Winner)
{
	const int32 FaceIdx = static_cast<int32>(Face);
	if (Faces.IsValidIndex(FaceIdx) && Faces[FaceIdx])
	{
		Faces[FaceIdx]->OnFaceWon(Winner);
	}
}

void ACTTBoardActor::ResetVisuals()
{
	for (TObjectPtr<ACTTFaceActor>& Face : Faces)
	{
		if (Face) Face->ResetVisuals();
	}
	OnBoardReset();
}
