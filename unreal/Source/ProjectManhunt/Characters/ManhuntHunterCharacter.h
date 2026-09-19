#pragma once

#include "CoreMinimal.h"
#include "Characters/ManhuntCharacterBase.h"
#include "ManhuntHunterCharacter.generated.h"

class USpotLightComponent;

/**
 * The Hunter's pawn. Movement speed and all decision-making live in
 * AManhuntHunterController (see AI/ManhuntHunterController.h) -- this class
 * only owns the physical body and the flashlight prop, matching how
 * src/game/HunterAI.js owned both the state machine AND the mesh/light in
 * one object in the browser build, but split here along Unreal's normal
 * Pawn/Controller boundary.
 */
UCLASS()
class PROJECTMANHUNT_API AManhuntHunterCharacter : public AManhuntCharacterBase
{
	GENERATED_BODY()

public:
	AManhuntHunterCharacter();

	/** Applies one of UManhuntGameSettings' per-state Hunter speeds
	 *  (Patrol/Suspicious/Investigate/Search/Chase) -- called by
	 *  AManhuntHunterController's Behavior Tree tasks on state entry. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Hunter")
	void SetMovementSpeed(float NewSpeedCm);

protected:
	/** src/game/HunterAI.js `attachHunterLight()`. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Hunter")
	TObjectPtr<USpotLightComponent> Flashlight;
};
