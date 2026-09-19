#pragma once

#include "CoreMinimal.h"
#include "Characters/ManhuntCharacterBase.h"
#include "ManhuntPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaminaComponent;
class UManhuntCameraComponent;
class UMotionWarpingComponent;
class AManhuntClimbVolume;

/**
 * The player. Direct port of src/game/PlayerController.js + the camera
 * wiring from src/camera/ThirdPersonCamera.js.
 *
 * NOTE for Phase 1 bring-up (see /docs/UNREAL_MIGRATION_PLAN.md Section 9):
 * input is bound via the classic Axis/Action system (see
 * SetupPlayerInputComponent below and Config/DefaultInput.ini) rather than
 * Enhanced Input, specifically so this scaffold is playable immediately
 * after opening the project, with zero Input Action/Mapping Context
 * binary assets required. EnhancedInput is already a module dependency
 * (see ProjectManhunt.Build.cs) -- migrating these bindings to Enhanced
 * Input Actions is a recommended, low-risk Phase 1 task once the team is
 * ready to author those assets in-editor.
 */
UCLASS()
class PROJECTMANHUNT_API AManhuntPlayerCharacter : public AManhuntCharacterBase
{
	GENERATED_BODY()

public:
	AManhuntPlayerCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "Manhunt|Player")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintPure, Category = "Manhunt|Player")
	bool IsCrouchToggled() const { return bWantsCrouch; }

	/** Called by AManhuntClimbVolume when the player enters a ladder-up or
	 *  ladder-down trigger. Mirrors PlayerController.js's
	 *  _checkClimbAccess()/_startClimb() -- see that file and
	 *  /docs/ANIMATION_SPEC.md Section 6 (root motion + Motion Warping). */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Traversal")
	void BeginClimb(AManhuntClimbVolume* ClimbVolume, bool bClimbingUp);

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void OnSprintPressed() { bWantsSprint = true; }
	void OnSprintReleased() { bWantsSprint = false; }
	void OnCrouchToggle() { bWantsCrouch = !bWantsCrouch; }
	void OnDecoyPressed();
	void OnRescuePressed();
	void OnPausePressed();

	void UpdateLocomotionAndSpeed(float DeltaTime);

	/** src/game/PlayerController.js `throwDecoy()`. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Player")
	void ThrowDecoy();

	/** src/game/RescueSystem.js `tryRescue()`, invoked here. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Player")
	void TryRescue();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Camera")
	TObjectPtr<UManhuntCameraComponent> CameraBehavior;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Player")
	TObjectPtr<UStaminaComponent> Stamina;

	/** Aligns the vault/climb/mantle montages to exact level contact points
	 *  (the AManhuntClimbVolume's ground/roof transforms) -- see
	 *  /docs/ANIMATION_SPEC.md Section 6. Requires the MotionWarping plugin
	 *  (already enabled in ProjectManhunt.uproject). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Traversal")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	bool bWantsSprint = false;
	bool bWantsCrouch = false;
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Manhunt|Traversal")
	bool bIsClimbing = false;
};
