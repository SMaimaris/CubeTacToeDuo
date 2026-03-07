#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "CTTOrbitCameraComponent.generated.h"

/**
 * Arc-ball (orbit) camera that always looks at the world origin (where
 * ACTTBoardActor sits). Attach to a BP_CameraRig pawn.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CUBETACTOE_API UCTTOrbitCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	UCTTOrbitCameraComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	/** Current orbit radius (distance from target) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float Radius = 1200.f;

	/** Horizontal angle around the target (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float Yaw = 45.f;

	/** Vertical angle above the horizontal plane (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float Pitch = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float MinRadius = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float MaxRadius = 2500.f;

	/** Degrees per input unit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float RotationSensitivity = 0.4f;

	/** UU per input unit (scroll wheel) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float ZoomSensitivity = 80.f;

	/** Yaw by Delta * RotationSensitivity degrees */
	UFUNCTION(BlueprintCallable, Category = "Orbit")
	void AddYaw(float Delta);

	/** Pitch by Delta * RotationSensitivity degrees, clamped to ±80° */
	UFUNCTION(BlueprintCallable, Category = "Orbit")
	void AddPitch(float Delta);

	/** Zoom by Delta * ZoomSensitivity UU */
	UFUNCTION(BlueprintCallable, Category = "Orbit")
	void AddZoom(float Delta);

	/**
	 * Smoothly animate Yaw and Pitch to the given angles over InDuration seconds.
	 * Yaw takes the shortest angular path; Pitch is linearly interpolated with smoothstep easing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Orbit")
	void SmoothRotateTo(float InTargetYaw, float InTargetPitch, float InDuration);

	/** True while a SmoothRotateTo animation is in progress. */
	UFUNCTION(BlueprintPure, Category = "Orbit")
	bool IsAnimatingCamera() const { return bIsAnimating; }

	/** World-space target point the camera orbits around */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	FVector Target = FVector::ZeroVector;

private:
	bool  bIsAnimating   = false;
	float AnimStartYaw   = 0.f;
	float AnimStartPitch = 0.f;
	float AnimYawDelta   = 0.f; // shortest-path delta, computed once on start
	float AnimTargetPitch = 0.f;
	float AnimDuration   = 1.f;
	float AnimElapsed    = 0.f;
};
