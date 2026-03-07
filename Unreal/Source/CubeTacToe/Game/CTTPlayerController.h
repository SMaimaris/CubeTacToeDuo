#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "CTTPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ACTTGameMode;
class ACTTBoardActor;

/**
 * Wires Enhanced Input events to world raycasts and GameMode calls.
 * The Blueprint child (BP_PlayerController) assigns the IMC and IA assets.
 *
 * Camera is static.
 * RMB drag    → free-rotates the cube.
 * WASD        → navigates between cube faces (cube smooth-rotates to face the camera).
 */
UCLASS()
class CUBETACTOE_API ACTTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACTTPlayerController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;

	// ---- Input Assets (assigned in BP_PlayerController defaults) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Click;

	/** RMB held — gates cube rotation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_RMB;

	/** Mouse delta — rotates the cube (only while RMB held) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_RotateCamera;

	/** WASD — navigate between cube faces (one step per key press) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_NavigateFace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_ZoomCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Pause;

	/** Mouse sensitivity for RMB cube rotation (degrees per mouse delta unit) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	float CubeRotationSensitivity = 0.4f;

	/** Duration of the smooth face-navigation rotation (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	float FaceNavDuration = 0.35f;

	/** Which face is currently "active" (camera-facing). BlueprintReadOnly for UI. */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation")
	int32 ActiveFaceIndex = 1; // start at Front

	/** Rotate the cube to show the given face index (0-5). Called from BP for AI moves. */
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void NavigateToFace(int32 FaceIndex);

private:
	/** LMB click → raycast → AttemptMove */
	void HandleClick(const FInputActionValue& Value);

	/** RMB pressed/released */
	void HandleRMBPressed(const FInputActionValue& Value);
	void HandleRMBReleased(const FInputActionValue& Value);

	/** Mouse delta → rotate the cube (only while RMB held) */
	void HandleCameraRotate(const FInputActionValue& Value);

	/** WASD → step to the adjacent face and smooth-rotate cube to face camera */
	void HandleNavigateFace(const FInputActionValue& Value);

	/** Scroll wheel → zoom camera */
	void HandleCameraZoom(const FInputActionValue& Value);

	/** ESC → toggle pause */
	void HandlePause(const FInputActionValue& Value);

	/** Convenience: get the CTTGameMode */
	ACTTGameMode* GetCTTGameMode() const;

	/** Cached reference to the board actor — set in BeginPlay */
	TObjectPtr<ACTTBoardActor> CachedBoardActor;

	bool  bRMBHeld           = false;

	// ---- Smooth board rotation (for face navigation) ----
	bool  bSmoothingBoard    = false;
	FQuat BoardRotStart;
	FQuat BoardRotTarget;
	float BoardRotElapsed    = 0.f;
};
