#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, NewStamina, float, MaxStamina);

/**
 * Direct port of the stamina logic in src/game/PlayerController.js (the
 * `stamina` field and its update() drain/regen branches). Drives whether
 * the player is allowed to sprint and feeds the HUD stamina bar.
 *
 * See /docs/UNREAL_MIGRATION_PLAN.md Section 5 for the source values (all
 * pulled from UManhuntGameSettings, which itself mirrors gameConfig.js).
 */
UCLASS(ClassGroup = (Manhunt), meta = (BlueprintSpawnableComponent))
class PROJECTMANHUNT_API UStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStaminaComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Mirrors PlayerController.js: `sprint = wantsSprint && stamina > minToSprint && !crouched && moving`. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Stamina")
	bool CanSprint() const;

	/** Call every tick with the character's current locomotion intent; drives drain/regen. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Stamina")
	void UpdateLocomotionState(bool bWantsSprint, bool bIsCrouched, bool bIsMoving);

	UFUNCTION(BlueprintPure, Category = "Manhunt|Stamina")
	float GetStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "Manhunt|Stamina")
	float GetStaminaFraction() const;

	UPROPERTY(BlueprintAssignable, Category = "Manhunt|Stamina")
	FOnStaminaChanged OnStaminaChanged;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Stamina")
	float CurrentStamina = 100.f;

	bool bWantsSprintCached = false;
	bool bIsCrouchedCached = false;
	bool bIsMovingCached = false;

	/** True once CanSprint() actually granted sprint last tick -- distinct from bWantsSprintCached,
	 *  matching the browser's `sprint` (granted) vs `wantsSprint`/`keys.shift` (requested) split. */
	bool bIsSprintingCached = false;
};
