#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateSuspicion.generated.h"

/**
 * TEMPLATE / STARTER EXAMPLE -- not wired into the active Hunter AI.
 * See the note in BTTask_FindPatrolPoint.h and
 * AManhuntHunterController's header comment.
 *
 * Ticks the Suspicious-state meter described in
 * /docs/HUNTER_AI_SPEC.md Section 3: rises while a target is currently
 * gated-detected (see UManhuntDetectionComponent), decays faster than it
 * rises when not, and is read by a BT Decorator gating the Suspicious ->
 * Chase transition once it crosses UManhuntGameSettings::SuspiciousToChaseSeconds.
 */
UCLASS()
class PROJECTMANHUNT_API UBTService_UpdateSuspicion : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateSuspicion();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector SuspicionMeterKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** Browser value: decay is roughly 1.4x the rise rate. */
	UPROPERTY(EditAnywhere, Category = "Manhunt", meta = (ClampMin = "0.0"))
	float DecayRateMultiplier = 1.4f;
};
