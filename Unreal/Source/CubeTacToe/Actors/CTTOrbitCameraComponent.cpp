#include "Actors/CTTOrbitCameraComponent.h"

UCTTOrbitCameraComponent::UCTTOrbitCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCTTOrbitCameraComponent::SmoothRotateTo(float InTargetYaw, float InTargetPitch,
                                               float InDuration)
{
	AnimStartYaw    = Yaw;
	AnimStartPitch  = Pitch;
	AnimYawDelta    = FMath::FindDeltaAngleDegrees(Yaw, InTargetYaw); // shortest path
	AnimTargetPitch = InTargetPitch;
	AnimDuration    = FMath::Max(InDuration, 0.01f);
	AnimElapsed     = 0.f;
	bIsAnimating    = true;
}

void UCTTOrbitCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Advance smooth camera animation
	if (bIsAnimating)
	{
		AnimElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(AnimElapsed / AnimDuration, 0.f, 1.f);
		const float T     = Alpha * Alpha * (3.f - 2.f * Alpha); // smoothstep ease

		Yaw   = AnimStartYaw + AnimYawDelta * T;
		Pitch = FMath::Lerp(AnimStartPitch, AnimTargetPitch, T);

		if (Alpha >= 1.f)
		{
			Yaw   = AnimStartYaw + AnimYawDelta; // exact final value
			Pitch = AnimTargetPitch;
			bIsAnimating = false;
		}
	}

	// Compute camera world position from spherical coordinates
	const float PitchRad = FMath::DegreesToRadians(Pitch);
	const float YawRad   = FMath::DegreesToRadians(Yaw);

	const float CosP = FMath::Cos(PitchRad);
	FVector CamPos = Target + FVector(
		Radius * CosP * FMath::Cos(YawRad),
		Radius * CosP * FMath::Sin(YawRad),
		Radius * FMath::Sin(PitchRad)
	);

	// Move the owning actor / component to this world position
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(CamPos);
	}
	else
	{
		SetWorldLocation(CamPos);
	}

	// Always look at the target
	FVector LookDir = (Target - CamPos).GetSafeNormal();
	SetWorldRotation(LookDir.Rotation());
}

void UCTTOrbitCameraComponent::AddYaw(float Delta)
{
	Yaw += Delta * RotationSensitivity;
}

void UCTTOrbitCameraComponent::AddPitch(float Delta)
{
	Pitch = FMath::Clamp(Pitch + Delta * RotationSensitivity, -80.f, 80.f);
}

void UCTTOrbitCameraComponent::AddZoom(float Delta)
{
	Radius = FMath::Clamp(Radius + Delta * ZoomSensitivity, MinRadius, MaxRadius);
}
