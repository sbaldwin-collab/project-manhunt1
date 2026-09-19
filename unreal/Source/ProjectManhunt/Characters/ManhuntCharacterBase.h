#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/ManhuntDetectableInterface.h"
#include "ManhuntCharacterBase.generated.h"

/**
 * High-level locomotion state, read by the Animation Blueprint's state
 * machine (see /docs/ANIMATION_SPEC.md Section 5) and by gameplay code that
 * needs to know "is this character currently in a scripted full-body
 * action." Direct expansion of the state names CharacterFactory.js used in
 * the browser build (idle/walk/run/crouch/climb), split out per
 * /docs/ANIMATION_SPEC.md Section 3 (Jog added as a new mid-speed tier;
 * Sprint replaces the browser's single "run" state).
 */
UENUM(BlueprintType)
enum class EManhuntLocomotionState : uint8
{
	Idle,
	Walk,
	Jog,
	Sprint,
	Crouch,
	CrouchWalk,
	Climbing,
};

/**
 * Shared base for the player, Hunter, and crew characters. Owns the
 * locomotion-state bookkeeping the Animation Blueprint reads and implements
 * IManhuntDetectableInterface so UManhuntDetectionComponent can query crouch
 * state and rooftop layer without knowing which concrete subclass it's
 * looking at.
 *
 * Corresponds to no single browser file -- it factors out what
 * PlayerController.js, CrewAI.js, and HunterAI.js all duplicated (x/z/facing
 * bookkeeping, a `crouched` flag, a `roofLayer` marker) into one shared root,
 * the way ACharacter + a shared base class is idiomatic in UE5 where the
 * browser build had no such inheritance available to it.
 */
UCLASS(Abstract)
class PROJECTMANHUNT_API AManhuntCharacterBase : public ACharacter, public IManhuntDetectableInterface
{
	GENERATED_BODY()

public:
	AManhuntCharacterBase();

	UFUNCTION(BlueprintCallable, Category = "Manhunt|Locomotion")
	virtual void SetLocomotionState(EManhuntLocomotionState NewState);

	UFUNCTION(BlueprintPure, Category = "Manhunt|Locomotion")
	EManhuntLocomotionState GetLocomotionState() const { return CurrentLocomotionState; }

	/** NAME_None on the street; a specific roof identifier once climbed.
	 *  Mirrors PlayerController.js's `roofLayer` (null vs. a roof id string)
	 *  and is the same identifier space as the level's roof collision
	 *  volumes / NavMesh areas -- see /docs/HUNTER_AI_SPEC.md Section 6. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Traversal")
	virtual void SetRoofLayer(FName NewRoofLayer) { CurrentRoofLayer = NewRoofLayer; }

	UFUNCTION(BlueprintPure, Category = "Manhunt|Traversal")
	bool IsOnRooftop() const { return CurrentRoofLayer != NAME_None; }

	// -- IManhuntDetectableInterface --
	virtual bool IsCrouchingForDetection_Implementation() const;
	virtual bool IsOnRooftopLayer_Implementation() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Locomotion")
	EManhuntLocomotionState CurrentLocomotionState = EManhuntLocomotionState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Traversal")
	FName CurrentRoofLayer = NAME_None;
};
