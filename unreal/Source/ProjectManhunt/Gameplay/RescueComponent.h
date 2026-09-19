#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RescueComponent.generated.h"

class AManhuntCrewCharacter;
class AJailVolume;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnManhuntCrewCaptured, AManhuntCrewCharacter*, Crew);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnManhuntRescuePerformed, int32, CrewFreedCount);

/**
 * Owns the jail: capturing crew, arranging them behind bars, and releasing
 * them on rescue. Direct port of src/game/RescueSystem.js. Lives on
 * AManhuntGameState (see that class) so it's a single, HUD-queryable
 * source of truth, matching how RescueSystem.js was constructed once by
 * Game.js and shared between the Hunter, the player, and the HUD.
 */
UCLASS(ClassGroup = (Manhunt), meta = (BlueprintSpawnableComponent))
class PROJECTMANHUNT_API URescueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URescueComponent();

	virtual void BeginPlay() override;

	/** Called by each AManhuntCrewCharacter on its own BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Rescue")
	void RegisterCrewMember(AManhuntCrewCharacter* Crew);

	/** src/game/RescueSystem.js `captureIfTouching()`, called per-crew from
	 *  AManhuntGameMode's capture-distance check. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Rescue")
	void CaptureCrew(AManhuntCrewCharacter* Crew);

	/** src/game/RescueSystem.js `tryRescue()`. Returns true if a rescue
	 *  occurred (player was in range AND at least one crew was jailed). */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Rescue")
	bool TryRescue(APawn* PlayerPawn);

	UFUNCTION(BlueprintPure, Category = "Manhunt|Rescue")
	bool IsPlayerInRescueRange(const APawn* PlayerPawn) const;

	UFUNCTION(BlueprintPure, Category = "Manhunt|Rescue")
	int32 GetJailedCount() const;

	UFUNCTION(BlueprintPure, Category = "Manhunt|Rescue")
	int32 GetFreeCount() const;

	void ResetForNewRound();

	UPROPERTY(BlueprintAssignable, Category = "Manhunt|Rescue")
	FOnManhuntCrewCaptured OnCrewCaptured;

	UPROPERTY(BlueprintAssignable, Category = "Manhunt|Rescue")
	FOnManhuntRescuePerformed OnRescuePerformed;

private:
	void ArrangeCapturedCrewInCage();
	FVector GetJailLocation() const;
	float GetJailRadius() const;

	UPROPERTY()
	TArray<TObjectPtr<AManhuntCrewCharacter>> AllCrew;

	UPROPERTY()
	TObjectPtr<AJailVolume> CachedJailVolume;
};
