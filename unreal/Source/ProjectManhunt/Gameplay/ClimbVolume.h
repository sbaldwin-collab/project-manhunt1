#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClimbVolume.generated.h"

class USphereComponent;
class UArrowComponent;

/**
 * A single fire-escape traversal point: a ground-level trigger and a
 * matching rooftop-level trigger, plus the roof layer identifier the player
 * enters/leaves on climb. Direct actor equivalent of one entry in
 * src/world/World.js's `roofAccess` array
 * (`{ roofId, ground: {x,z}, roof: {x,z}, roofY }`) -- placed by hand in the
 * level rather than computed procedurally, per /docs/NYC_LEVEL_DESIGN.md
 * (two of these total, one per roof-access building).
 *
 * Only the player uses these (see AManhuntPlayerCharacter::BeginClimb) --
 * the Hunter never climbs, matching /docs/HUNTER_AI_SPEC.md Section 6.
 */
UCLASS()
class PROJECTMANHUNT_API AManhuntClimbVolume : public AActor
{
	GENERATED_BODY()

public:
	AManhuntClimbVolume();

	/** Matches AManhuntCharacterBase::SetRoofLayer's identifier space --
	 *  give each of the level's two climb volumes a distinct value
	 *  (e.g. "Roof_NW_FireEscape", "Roof_SW_FireEscape"). */
	UFUNCTION(BlueprintPure, Category = "Manhunt|Traversal")
	FName GetRoofLayerId() const { return RoofLayerId; }

	UFUNCTION(BlueprintPure, Category = "Manhunt|Traversal")
	FTransform GetGroundTransform() const;

	UFUNCTION(BlueprintPure, Category = "Manhunt|Traversal")
	FTransform GetRoofTransform() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnGroundTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRoofTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Manhunt|Traversal")
	FName RoofLayerId = TEXT("Roof_Unset");

	/** Placed at street level, near the fire escape's base ladder. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Traversal")
	TObjectPtr<USceneComponent> GroundPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Traversal")
	TObjectPtr<USphereComponent> GroundTrigger;

	/** Placed on the rooftop, at the top of the fire escape. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Traversal")
	TObjectPtr<USceneComponent> RoofPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Traversal")
	TObjectPtr<USphereComponent> RoofTrigger;
};
