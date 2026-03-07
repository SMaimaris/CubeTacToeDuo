#include "Actors/CTTCellActor.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/CTTGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "CubeTacToe.h"

ACTTCellActor::ACTTCellActor()
{
	PrimaryActorTick.bCanEverTick       = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only enabled during pop-in

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	// Cells respond only to the dedicated Cell trace channel (ECC_GameTraceChannel1)
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Required for BeginCursorOver / EndCursorOver events to fire in Blueprint.
	// Also needs the mesh to respond to ECC_Visibility for the engine's mouse trace.
	bGenerateOverlapEventsDuringLevelStreaming = false;
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Marker mesh: sits on top of the cell face, hidden until the cell is claimed
	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(RootComponent);
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetVisibility(false);
	// Small Z offset so the marker rests on the cell surface rather than clipping into it
	MarkerMesh->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
}

void ACTTCellActor::SetCellOwner(ECellOwner NewOwner)
{
	if (CurrentOwner == NewOwner) return;
	UE_LOG(LogCTT, Log, TEXT("CellActor [Face=%d Row=%d Col=%d]: %s → %s"),
	       static_cast<int32>(CellCoord.FaceIndex), CellCoord.Row, CellCoord.Col,
	       *UEnum::GetValueAsString(CurrentOwner),
	       *UEnum::GetValueAsString(NewOwner));
	if (NewOwner != ECellOwner::None)
	{
		ClearHover(); // clear while owner is still None so BP Switch routes to the None pin

		// Assign the appropriate static mesh so BP_CellActor can animate it in OnOwnerChanged
		UStaticMesh* Mesh = (NewOwner == ECellOwner::PlayerX) ? XMarkerMesh.Get() : OMarkerMesh.Get();
		MarkerMesh->SetStaticMesh(Mesh);
		MarkerMesh->SetRelativeScale3D(FVector::ZeroVector); // start collapsed; scales to 1 during pop-in
		MarkerMesh->SetRelativeRotation(
			NewOwner == ECellOwner::PlayerX ? FRotator(0.f, -45.f, 0.f) : FRotator::ZeroRotator);
		PopInTargetScale = (NewOwner == ECellOwner::PlayerX) ? XMarkerScale : OMarkerScale;
		MarkerMesh->SetVisibility(true);
	}
	else
	{
		// Reset: hide the marker
		MarkerMesh->SetVisibility(false);
		MarkerMesh->SetStaticMesh(nullptr);
	}

	CurrentOwner = NewOwner;
	OnOwnerChanged(NewOwner);
}

void ACTTCellActor::ClearHover()
{
	if (bHovered)
	{
		bHovered = false;
		ReceiveActorEndCursorOver(); // fires BP "Event EndCursorOver"
	}
}

void ACTTCellActor::NotifyActorBeginCursorOver()
{
	bHovered = true;
	if (CurrentOwner == ECellOwner::None)
	{
		Super::NotifyActorBeginCursorOver(); // fires BP "Event BeginCursorOver" only for empty cells
	}
}

void ACTTCellActor::NotifyActorEndCursorOver()
{
	bHovered = false;
	Super::NotifyActorEndCursorOver(); // fires BP "Event EndCursorOver"
}

// ============================================================
//  Pop-in animation
// ============================================================

float ACTTCellActor::EaseOutElastic(float T)
{
	// Standard elastic ease-out: one overshoot above 1, then settles.
	// f(0)=0, f(1)=1, peaks ~1.15 around t≈0.8.
	if (T <= 0.f) return 0.f;
	if (T >= 1.f) return 1.f;
	const float C4 = (2.f * PI) / 3.f;
	return FMath::Pow(2.f, -10.f * T) * FMath::Sin((T * 10.f - 0.75f) * C4) + 1.f;
}

void ACTTCellActor::OnOwnerChanged_Implementation(ECellOwner NewOwner)
{
	if (NewOwner == ECellOwner::None) return;

	// Play placement sound at the cell's world location
	USoundBase* Sound = (NewOwner == ECellOwner::PlayerX) ? XPlacementSound.Get() : OPlacementSound.Get();
	if (Sound)
	{
		UAudioComponent* AC = UGameplayStatics::SpawnSoundAtLocation(this, Sound, GetActorLocation());
		if (AC)
		{
			if (UCTTGameInstance* GI = Cast<UCTTGameInstance>(GetGameInstance()))
			{
				AC->SoundClassOverride = GI->SFXSoundClass;
			}
		}
	}

	// Kick off the pop-in animation via Tick
	PopInElapsed = 0.f;
	SetActorTickEnabled(true);
}

void ACTTCellActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PopInElapsed += DeltaTime;
	const float T     = FMath::Clamp(PopInElapsed / PopInDuration, 0.f, 1.f);
	const float Scale = EaseOutElastic(T) * PopInTargetScale;
	MarkerMesh->SetRelativeScale3D(FVector(Scale));

	if (T >= 1.f)
	{
		SetActorTickEnabled(false);
	}
}
