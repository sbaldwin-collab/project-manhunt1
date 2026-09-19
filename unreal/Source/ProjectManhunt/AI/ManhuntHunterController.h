#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ManhuntHunterController.generated.h"

class UAIPerceptionComponent;
class UManhuntDetectionComponent;
class UBehaviorTree;
class AManhuntHunterCharacter;

/**
 * Direct translation of src/game/HunterAI.js's `state` field. See
 * /docs/HUNTER_AI_SPEC.md Section 3 for the full per-state behavior spec
 * this enum and the Tick* functions below implement.
 */
UENUM(BlueprintType)
enum class EManhuntHunterState : uint8
{
	Patrol,
	Suspicious,
	Investigate,
	Chase,
	Search,
	Return,
	Guard,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHunterSpottedTarget, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHunterLostTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHunterStateChanged);

/**
 * The Hunter's brain. Direct port of src/game/HunterAI.js's per-frame
 * update() and its state transition rules, using
 * UManhuntDetectionComponent (see Components/ManhuntDetectionComponent.h)
 * for the FOV/occlusion/crouch/elevation-gated sight query and stock
 * UAIPerceptionComponent hearing for noise events.
 *
 * IMPLEMENTATION NOTE -- read before treating this as the final AI:
 * /docs/HUNTER_AI_SPEC.md specifies this state machine as a Behavior Tree +
 * Blackboard (the standard, designer-iterable UE5 approach, and the
 * recommended target architecture). Authoring an actual BT/BB is editor
 * asset work this text-only scaffold cannot produce. To keep the project
 * genuinely playable the moment it's opened in-editor -- rather than
 * shipping an empty AIController that does nothing until someone builds a
 * graph -- every state below is implemented natively in C++ (Tick-driven),
 * as a faithful, working port of HunterAI.js. This is a legitimate
 * permanent architecture choice for a project this size, not merely a
 * placeholder -- but if the team wants Behavior-Tree-level designer
 * iteration speed, treat this class's Tick*() methods as the exact logic to
 * move into native BTTaskNode/BTDecorator classes (two starter examples,
 * BTTask_FindPatrolPoint and BTTask_UpdateSuspicionMeter, exist under
 * AI/Tasks/ as templates for that migration) and OptionalBehaviorTreeAsset
 * below as the hookup point once that graph exists. Either path is valid;
 * do not run both at once.
 */
UCLASS()
class PROJECTMANHUNT_API AManhuntHunterController : public AAIController
{
	GENERATED_BODY()

public:
	AManhuntHunterController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Manhunt|AI")
	EManhuntHunterState GetCurrentState() const { return CurrentState; }

	UManhuntDetectionComponent* GetDetectionComponent() const { return Detection; }

	/** src/game/HunterAI.js `notifyCaptureEvent()` -- called by
	 *  URescueComponent whenever a crew member is captured; rolls
	 *  GuardCrewChance and may force-transition to Guard from any state. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|AI")
	void NotifyCaptureEvent();

	/** Optional target for a Behavior-Tree-based reimplementation -- see the
	 *  class header comment. Unused by the native Tick* state machine. */
	UPROPERTY(EditDefaultsOnly, Category = "Manhunt|AI|Behavior Tree (optional)")
	TObjectPtr<UBehaviorTree> OptionalBehaviorTreeAsset;

	/** /docs/HUNTER_AI_SPEC.md Section 5 events. */
	UPROPERTY(BlueprintAssignable, Category = "Manhunt|AI|Events")
	FOnHunterSpottedTarget OnHunterSpottedTarget;

	UPROPERTY(BlueprintAssignable, Category = "Manhunt|AI|Events")
	FOnHunterLostTarget OnHunterLostTarget;

	UPROPERTY(BlueprintAssignable, Category = "Manhunt|AI|Events")
	FOnHunterStateChanged OnHunterStateChanged;

protected:
	void SetState(EManhuntHunterState NewState);

	// One Tick* per row of /docs/HUNTER_AI_SPEC.md Section 3. Suspicious has
	// no dedicated function -- its meter accumulates/decays inline in
	// Tick() since it depends on whether a target is seen THIS frame, which
	// Tick() already branches on before dispatching to these.
	void TickPatrol(float DeltaTime);
	void TickInvestigate(float DeltaTime);
	void TickChase(float DeltaTime);
	void TickSearch(float DeltaTime);
	void TickReturn(float DeltaTime);
	void TickGuard(float DeltaTime);

	/** Gathers the player + free (uncaptured) crew and returns whichever is
	 *  both nearest and currently gated-detected -- src/game/HunterAI.js
	 *  `_findVisibleTarget()`. */
	AActor* FindBestVisibleTarget() const;

	void PickNewPatrolPoint();
	void MoveTowardsAtSpeed(const FVector& Location, float SpeedCm);

	UPROPERTY()
	TObjectPtr<UAIPerceptionComponent> Perception;

	UPROPERTY()
	TObjectPtr<UManhuntDetectionComponent> Detection;

	EManhuntHunterState CurrentState = EManhuntHunterState::Patrol;

	FVector CurrentTargetLocation = FVector::ZeroVector;
	FVector LastKnownLocation = FVector::ZeroVector;
	bool bHasLastKnownLocation = false;

	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentTargetRef;

	// All timers in SECONDS (UManhuntGameSettings stores state-timing
	// values in seconds, unlike the browser's millisecond fields).
	float SuspicionMeterSeconds = 0.f;
	float SearchTimerSeconds = 0.f;
	float ReturnTimerSeconds = 0.f;
	float InvestigateTimerSeconds = 0.f;
	float GuardTimerSeconds = 0.f;
};
