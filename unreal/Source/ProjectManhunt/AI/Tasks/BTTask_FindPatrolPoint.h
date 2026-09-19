#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPatrolPoint.generated.h"

/**
 * TEMPLATE / STARTER EXAMPLE -- not wired into the active Hunter AI.
 *
 * AManhuntHunterController currently implements the full state machine
 * natively in C++ (see that class's header comment for why). This native
 * BTTaskNode is provided as a working starting point for the team if/when
 * the Hunter AI is migrated to a real Behavior Tree graph for
 * designer-facing iteration, per /docs/HUNTER_AI_SPEC.md Section 3
 * (Patrol row). It finds a random reachable NavMesh point and writes it to
 * a Vector Blackboard key -- functionally equivalent to
 * AManhuntHunterController::PickNewPatrolPoint().
 */
UCLASS()
class PROJECTMANHUNT_API UBTTask_FindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindPatrolPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetLocationKey;

	/** Matches AManhuntHunterController::PickNewPatrolPoint()'s placeholder
	 *  radius -- replace with pre-placed patrol point Actors per
	 *  /docs/NYC_LEVEL_DESIGN.md once the level exists. */
	UPROPERTY(EditAnywhere, Category = "Manhunt", meta = (ClampMin = "0.0"))
	float SearchRadiusCm = 4000.f;
};
