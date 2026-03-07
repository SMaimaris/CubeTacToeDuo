#include "Game/CTTPlayerController.h"
#include "Game/CTTGameMode.h"
#include "Actors/CTTCellActor.h"
#include "Actors/CTTBoardActor.h"
#include "Actors/CTTOrbitCameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "CubeTacToe.h"

// Face local outward normals — matches EFaceIndex order in CTTTypes.h:
//   Top(0)=(0,0,1)  Front(1)=(1,0,0)  Right(2)=(0,1,0)
//   Back(3)=(-1,0,0) Left(4)=(0,-1,0) Bottom(5)=(0,0,-1)
static const FVector GFaceLocalNormals[6] =
{
	FVector( 0,  0,  1), // Top
	FVector( 1,  0,  0), // Front
	FVector( 0,  1,  0), // Right
	FVector(-1,  0,  0), // Back
	FVector( 0, -1,  0), // Left
	FVector( 0,  0, -1), // Bottom
};

// Canonical "up" for each face in cube local space —
// what direction should appear as "up on screen" when that face faces the camera.
static const FVector GFaceLocalUp[6] =
{
	FVector( 1,  0,  0), // Top:    screen-up = toward Front
	FVector( 0,  0,  1), // Front:  screen-up = world Z
	FVector( 0,  0,  1), // Right:  screen-up = world Z
	FVector( 0,  0,  1), // Back:   screen-up = world Z
	FVector( 0,  0,  1), // Left:   screen-up = world Z
	FVector( 1,  0,  0), // Bottom: screen-up = toward Front
};

// Adjacency table [face][dir]: dir 0=W 1=S 2=A 3=D
//         W   S   A   D
static const int32 GFaceAdjacency[6][4] =
{
	{ 3,  1,  4,  2 }, // Top(0):    W=Back,  S=Front,  A=Left,  D=Right
	{ 0,  5,  4,  2 }, // Front(1):  W=Top,   S=Bottom, A=Left,  D=Right
	{ 0,  5,  1,  3 }, // Right(2):  W=Top,   S=Bottom, A=Front, D=Back
	{ 5,  0,  2,  4 }, // Back(3):   W=Bottom, S=Top,    A=Right, D=Left
	{ 0,  5,  3,  1 }, // Left(4):   W=Top,   S=Bottom, A=Back,  D=Front
	{ 1,  3,  4,  2 }, // Bottom(5): W=Front, S=Back,   A=Left,  D=Right
};

// ---------------------------------------------------------------------------

ACTTPlayerController::ACTTPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	PrimaryActorTick.bCanEverTick = true;
}

void ACTTPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogCTT, Log, TEXT("PlayerController: Input mapping context registered"));
			}
			else
			{
				UE_LOG(LogCTT, Warning, TEXT("PlayerController: DefaultMappingContext is null — input won't work"));
			}
		}
	}

	CachedBoardActor = Cast<ACTTBoardActor>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ACTTBoardActor::StaticClass()));
	if (!CachedBoardActor)
	{
		UE_LOG(LogCTT, Warning, TEXT("PlayerController: ACTTBoardActor not found"));
	}
}

void ACTTPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bSmoothingBoard && CachedBoardActor)
	{
		BoardRotElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(BoardRotElapsed / FaceNavDuration, 0.f, 1.f);
		const float T     = Alpha * Alpha * (3.f - 2.f * Alpha); // smoothstep

		CachedBoardActor->SetActorRotation(FQuat::Slerp(BoardRotStart, BoardRotTarget, T));

		if (Alpha >= 1.f)
		{
			CachedBoardActor->SetActorRotation(BoardRotTarget);
			bSmoothingBoard = false;
		}
	}
}

void ACTTPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (IA_Click)
		{
			EIC->BindAction(IA_Click, ETriggerEvent::Triggered, this,
			                &ACTTPlayerController::HandleClick);
		}
		if (IA_RMB)
		{
			EIC->BindAction(IA_RMB, ETriggerEvent::Started, this,
			                &ACTTPlayerController::HandleRMBPressed);
			EIC->BindAction(IA_RMB, ETriggerEvent::Completed, this,
			                &ACTTPlayerController::HandleRMBReleased);
		}
		if (IA_RotateCamera)
		{
			EIC->BindAction(IA_RotateCamera, ETriggerEvent::Triggered, this,
			                &ACTTPlayerController::HandleCameraRotate);
		}
		if (IA_NavigateFace)
		{
			EIC->BindAction(IA_NavigateFace, ETriggerEvent::Started, this,
			                &ACTTPlayerController::HandleNavigateFace);
		}
		if (IA_ZoomCamera)
		{
			EIC->BindAction(IA_ZoomCamera, ETriggerEvent::Triggered, this,
			                &ACTTPlayerController::HandleCameraZoom);
		}
		if (IA_Pause)
		{
			EIC->BindAction(IA_Pause, ETriggerEvent::Started, this,
			                &ACTTPlayerController::HandlePause);
		}
	}
}

// ---------------------------------------------------------------------------

void ACTTPlayerController::HandleClick(const FInputActionValue& Value)
{
	ACTTGameMode* GM = GetCTTGameMode();
	if (!GM)
	{
		UE_LOG(LogCTT, Warning, TEXT("HandleClick: no CTTGameMode found"));
		return;
	}

	FVector WorldLocation, WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		UE_LOG(LogCTT, Warning, TEXT("HandleClick: DeprojectMousePositionToWorld failed"));
		return;
	}

	FHitResult Hit;
	const FVector TraceEnd = WorldLocation + WorldDirection * 50000.f;
	FCollisionQueryParams Params;
	Params.bTraceComplex = false;

	if (GetWorld()->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, ECC_GameTraceChannel1, Params))
	{
		if (ACTTCellActor* Cell = Cast<ACTTCellActor>(Hit.GetActor()))
		{
			FCellCoord Coord = Cell->GetCellCoord();
			UE_LOG(LogCTT, Log, TEXT("HandleClick: hit CellActor — Face=%d Row=%d Col=%d"),
			       static_cast<int32>(Coord.FaceIndex), Coord.Row, Coord.Col);
			GM->AttemptMove(Coord);
		}
		else
		{
			UE_LOG(LogCTT, Log, TEXT("HandleClick: raycast hit '%s' (not a CellActor)"),
			       *Hit.GetActor()->GetName());
		}
	}
	else
	{
		UE_LOG(LogCTT, Log, TEXT("HandleClick: raycast missed (no Cell channel hit)"));
	}
}

void ACTTPlayerController::HandleRMBPressed(const FInputActionValue& Value)
{
	bRMBHeld = true;
}

void ACTTPlayerController::HandleRMBReleased(const FInputActionValue& Value)
{
	bRMBHeld = false;
}

void ACTTPlayerController::HandleCameraRotate(const FInputActionValue& Value)
{
	if (!CachedBoardActor || !bRMBHeld) return;

	// RMB drag cancels any ongoing face-navigation smooth rotation
	bSmoothingBoard = false;

	const FVector2D Delta = Value.Get<FVector2D>();
	if (Delta.IsNearlyZero()) return;

	FRotator ViewRot;
	FVector  ViewLoc;
	GetPlayerViewPoint(ViewLoc, ViewRot);

	// Camera-relative axes for intuitive rotation
	const FVector RightVec = FRotationMatrix(FRotator(0.f, ViewRot.Yaw, 0.f)).GetScaledAxis(EAxis::Y);

	// Combine yaw (around world up) and pitch (around camera right) into a single rotation
	const FQuat YawQ   = FQuat(FVector::UpVector, FMath::DegreesToRadians(-Delta.X * CubeRotationSensitivity));
	const FQuat PitchQ = FQuat(RightVec,          FMath::DegreesToRadians(Delta.Y * CubeRotationSensitivity));
	const FQuat Combined = (PitchQ * YawQ).GetNormalized();

	CachedBoardActor->SetActorRotation((Combined * CachedBoardActor->GetActorQuat()).GetNormalized());
}

void ACTTPlayerController::NavigateToFace(int32 FaceIndex)
{
	if (!CachedBoardActor) return;
	if (FaceIndex < 0 || FaceIndex >= 6) return;

	ActiveFaceIndex = FaceIndex;

	FRotator ViewRot;
	FVector  ViewLoc;
	GetPlayerViewPoint(ViewLoc, ViewRot);

	const FVector CubeOrigin  = CachedBoardActor->GetActorLocation();
	const FVector ToCameraDir = (ViewLoc - CubeOrigin).GetSafeNormal();
	const FVector LocalN      = GFaceLocalNormals[ActiveFaceIndex];
	const FVector LocalU      = GFaceLocalUp[ActiveFaceIndex];

	// Step 1: align face normal with camera direction
	const FQuat Q1 = FQuat::FindBetweenNormals(LocalN, ToCameraDir);

	// Step 2: correct roll so face "up" aligns with world up
	const FVector RotatedUp     = Q1.RotateVector(LocalU);
	const FVector WorldUp       = FVector::UpVector;
	const FVector ProjRotatedUp = (RotatedUp - (RotatedUp | ToCameraDir) * ToCameraDir).GetSafeNormal();
	const FVector ProjWorldUp   = (WorldUp   - (WorldUp   | ToCameraDir) * ToCameraDir).GetSafeNormal();

	FQuat Q2 = FQuat::Identity;
	if (!ProjRotatedUp.IsNearlyZero(0.01f) && !ProjWorldUp.IsNearlyZero(0.01f))
	{
		Q2 = FQuat::FindBetweenNormals(ProjRotatedUp, ProjWorldUp);
	}

	BoardRotStart   = CachedBoardActor->GetActorQuat();
	BoardRotTarget  = (Q2 * Q1).GetNormalized();
	BoardRotElapsed = 0.f;
	bSmoothingBoard = true;

	UE_LOG(LogCTT, Log, TEXT("NavigateToFace → %d"), ActiveFaceIndex);
}

void ACTTPlayerController::HandleNavigateFace(const FInputActionValue& Value)
{
	if (!CachedBoardActor) return;
	if (bSmoothingBoard) return; // don't interrupt an ongoing navigation

	const FVector2D Input = Value.Get<FVector2D>();

	// Map WASD to adjacency direction: 0=W, 1=S, 2=A, 3=D
	int32 Dir = -1;
	if      (Input.Y >  0.5f) Dir = 0; // W
	else if (Input.Y < -0.5f) Dir = 1; // S
	else if (Input.X < -0.5f) Dir = 2; // A
	else if (Input.X >  0.5f) Dir = 3; // D
	else return;

	const int32 TargetFace = GFaceAdjacency[ActiveFaceIndex][Dir];
	NavigateToFace(TargetFace);
}

void ACTTPlayerController::HandleCameraZoom(const FInputActionValue& Value)
{
	APawn* PossessedPawn = GetPawn();
	if (!PossessedPawn) return;

	if (UCTTOrbitCameraComponent* Orbit =
	        PossessedPawn->FindComponentByClass<UCTTOrbitCameraComponent>())
	{
		float Delta = Value.Get<float>();
		Orbit->AddZoom(-Delta);
	}
}

void ACTTPlayerController::HandlePause(const FInputActionValue& Value)
{
	ACTTGameMode* GM = GetCTTGameMode();
	if (GM)
	{
		GM->TogglePause();
	}
}

bool ACTTPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	// Handle ESC directly so it works even while the game is paused
	// (Enhanced Input stops ticking when paused)
	if (Params.Key == EKeys::Escape && Params.Event == IE_Pressed)
	{
		ACTTGameMode* GM = GetCTTGameMode();
		if (GM && (GM->GamePhase == EGamePhase::Playing || GM->GamePhase == EGamePhase::Paused))
		{
			GM->TogglePause();
			return true;
		}
	}
	return Super::InputKey(Params);
}


ACTTGameMode* ACTTPlayerController::GetCTTGameMode() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<ACTTGameMode>() : nullptr;
}
