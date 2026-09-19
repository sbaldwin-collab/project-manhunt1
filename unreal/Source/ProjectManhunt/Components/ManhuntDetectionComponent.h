#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIStimulus.h"
#include "ManhuntDetectionComponent.generated.h"

class UAIPerceptionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnManhuntTargetDetectionChanged, AActor*, Target, bool, bIsDetected);

/**
 * Wraps a UAIPerceptionComponent (Sight + Hearing) with the detection rules
 * proven in the browser build's src/game/DetectionSystem.js that have no
 * stock AIPerception equivalent:
 *
 *   - a target's crouch state shrinks the effective sight range against
 *     THEM specifically (not a global range change);
 *   - the Hunter's own Patrol/Return alertness shrinks its own sight range;
 *   - a hard elevation gate makes a rooftop target unsensable from the
 *     street regardless of line of sight.
 *
 * Occlusion/line-of-sight itself is intentionally NOT reimplemented here --
 * stock AIPerception sight tracing already does that correctly against
 * level geometry (see /docs/HUNTER_AI_SPEC.md Section 4), so this component
 * only ever narrows what AIPerception already reported, never widens it.
 *
 * Attach to AManhuntHunterController (see Header comment there for wiring).
 */
UCLASS(ClassGroup = (Manhunt), meta = (BlueprintSpawnableComponent))
class PROJECTMANHUNT_API UManhuntDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UManhuntDetectionComponent();

	/** Call once from the owning AIController after its UAIPerceptionComponent exists. */
	void InitializeDetection(UAIPerceptionComponent* InPerception);

	/** Call from the Hunter's state machine whenever entering/leaving Patrol or Return. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Detection")
	void SetPatrolAlertnessReduced(bool bReduced) { bPatrolAlertnessReduced = bReduced; }

	UFUNCTION(BlueprintPure, Category = "Manhunt|Detection")
	bool IsTargetCurrentlyDetected(AActor* Target) const;

	/** Nearest currently-detected candidate, or nullptr -- mirrors
	 *  HunterAI.js's `_findVisibleTarget()`, which picks the closest of the
	 *  player + free crew that currently passes canSee(). */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Detection")
	AActor* FindNearestDetectedActor(const TArray<AActor*>& Candidates) const;

	UPROPERTY(BlueprintAssignable, Category = "Manhunt|Detection")
	FOnManhuntTargetDetectionChanged OnTargetDetectionChanged;

	/** Returns true (once) if a hearing stimulus has arrived since the last
	 *  call and writes its world location to OutLocation, then clears the
	 *  pending flag -- mirrors consuming a single entry from
	 *  DetectionSystem.js's `noisePings` array, one event at a time, rather
	 *  than polling a live list. Used by AManhuntHunterController's Patrol
	 *  state (see /docs/HUNTER_AI_SPEC.md Section 3, "Investigate"). */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Detection")
	bool ConsumePendingNoiseEvent(FVector& OutLocation);

private:
	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** Applies the crouch-target and patrol-alertness multipliers to the
	 *  configured base sight radius. */
	float GetEffectiveSightRangeCmFor(const AActor* Target) const;

	/** src/game/DetectionSystem.js: `Math.abs(oy - ty) > 3.2` short-circuit. */
	bool PassesElevationGate(const AActor* Target) const;

	UPROPERTY()
	TObjectPtr<UAIPerceptionComponent> Perception;

	bool bPatrolAlertnessReduced = false;

	UPROPERTY()
	TMap<TObjectPtr<AActor>, bool> GatedDetectionState;

	bool bHasPendingNoiseEvent = false;
	FVector PendingNoiseLocation = FVector::ZeroVector;
};
