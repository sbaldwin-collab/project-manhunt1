#pragma once

#include "CoreMinimal.h"
#include "Characters/ManhuntCharacterBase.h"
#include "ManhuntCrewCharacter.generated.h"

/**
 * A crew member. Direct port of src/game/CrewAI.js. Movement decisions live
 * in AManhuntCrewController (a lightweight AIController with no Behavior
 * Tree -- wander/flee is two states, not worth a BT graph -- see
 * /docs/UNREAL_MIGRATION_PLAN.md's CrewAI.js mapping row).
 */
UCLASS()
class PROJECTMANHUNT_API AManhuntCrewCharacter : public AManhuntCharacterBase
{
	GENERATED_BODY()

public:
	AManhuntCrewCharacter();

	/** src/game/CrewAI.js `caught` flag. Gates AI ticking and blocks capture
	 *  re-triggering; set by URescueComponent on capture/rescue. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Crew")
	void SetCaptured(bool bInCaptured);

	UFUNCTION(BlueprintPure, Category = "Manhunt|Crew")
	bool IsCaptured() const { return bIsCaptured; }

	UFUNCTION(BlueprintCallable, Category = "Manhunt|Crew")
	void SetMovementSpeed(float NewSpeedCm);

	/** Crew index (0-2), used to pick a distinct clothing/headwear variant
	 *  -- see /docs/ASSET_MANIFEST.md Section 2 (crew outfits) and the
	 *  browser build's CharacterFactory.js `createCharacter('crew', index)`. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Manhunt|Crew")
	int32 CrewIndex = 0;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Crew")
	bool bIsCaptured = false;
};
