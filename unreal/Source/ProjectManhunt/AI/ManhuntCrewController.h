#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ManhuntCrewController.generated.h"

class AManhuntHunterCharacter;

/**
 * Lightweight controller for a single crew member: wander when the Hunter
 * can't see them, flee directly away from the Hunter when it can. Direct
 * port of src/game/CrewAI.js `update()`. No Behavior Tree -- two states
 * don't warrant one; see /docs/UNREAL_MIGRATION_PLAN.md's CrewAI.js row.
 */
UCLASS()
class PROJECTMANHUNT_API AManhuntCrewController : public AAIController
{
	GENERATED_BODY()

public:
	AManhuntCrewController();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;

	void TickWander(float DeltaTime);
	void TickFlee(float DeltaTime, const FVector& HunterLocation);

	/** Finds the single Hunter in the level and checks whether ITS detection
	 *  component currently senses this crew member -- see
	 *  /docs/HUNTER_AI_SPEC.md Section "translate CrewAI.js": crew reuse the
	 *  Hunter's own AIPerception-driven query rather than duplicating it. */
	bool IsSeenByHunter(FVector& OutHunterLocation) const;

	UPROPERTY()
	TObjectPtr<AManhuntHunterCharacter> CachedHunter;

	FVector WanderTarget = FVector::ZeroVector;
	bool bHasWanderTarget = false;
};
