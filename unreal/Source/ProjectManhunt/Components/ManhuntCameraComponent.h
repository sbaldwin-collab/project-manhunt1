#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ManhuntCameraComponent.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UCameraShakeBase;
class ULevelSequence;

/**
 * Behavioral logic for the third-person camera rig, layered on top of the
 * real USpringArmComponent + UCameraComponent that live on
 * AManhuntPlayerCharacter. Direct port of src/camera/ThirdPersonCamera.js:
 *
 *   - sprint FOV widen        -> UpdateSprintFov()
 *   - chase shake             -> PlayChaseShake() / PlayCaptureShake()
 *   - cinematic round-open    -> PlayCinematicIntro() (Level Sequence, not
 *                                hand-rolled orbit math -- see
 *                                /docs/UNREAL_MIGRATION_PLAN.md Section 7)
 *   - collision / shoulder offset / damping are configured directly on the
 *     SpringArm (bDoCollisionTest, bEnableCameraLag, socket offset) rather
 *     than reimplemented here, since UE5's SpringArm already does this
 *     correctly -- see the same section.
 */
UCLASS(ClassGroup = (Manhunt), meta = (BlueprintSpawnableComponent))
class PROJECTMANHUNT_API UManhuntCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UManhuntCameraComponent();

	void InitializeCamera(USpringArmComponent* InSpringArm, UCameraComponent* InCamera);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Call every tick with the player's current sprint state. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Camera")
	void SetSprinting(bool bInSprinting) { bIsSprinting = bInSprinting; }

	/** Fired from AManhuntHunterController::OnSpot (see /docs/HUNTER_AI_SPEC.md Section 5). */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Camera")
	void PlayChaseShake();

	/** Fired on player capture -- distinct, stronger shake than the chase shake. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Camera")
	void PlayCaptureShake();

	/** Plays the round-open cinematic. Implementation is expected to trigger
	 *  a Level Sequence (BP_RoundIntroSequence, authored in-editor) rather
	 *  than compute an orbit here -- this is a hand-off point, not a
	 *  self-contained implementation. */
	UFUNCTION(BlueprintNativeEvent, Category = "Manhunt|Camera")
	void PlayCinematicIntro();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Manhunt|Camera")
	TSubclassOf<UCameraShakeBase> ChaseShakeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Manhunt|Camera")
	TSubclassOf<UCameraShakeBase> CaptureShakeClass;

private:
	UPROPERTY()
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;

	bool bIsSprinting = false;
};
