#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JailVolume.generated.h"

class UStaticMeshComponent;

/**
 * Marks the jail/lockup plaza location in the level -- direct equivalent of
 * gameConfig.js `world.jail` (the browser build had no level Actor, just a
 * coordinate + radius). Place exactly one of these per
 * /docs/NYC_LEVEL_DESIGN.md's jail plaza location; URescueComponent finds it
 * via UGameplayStatics::GetActorOfClass at BeginPlay.
 *
 * The cage mesh/collision here is a placeholder box -- replace with the
 * real jail cage art (see /docs/ASSET_MANIFEST.md) once available; the
 * Radius property is what gameplay code actually reads.
 */
UCLASS()
class PROJECTMANHUNT_API AJailVolume : public AActor
{
	GENERATED_BODY()

public:
	AJailVolume();

	UFUNCTION(BlueprintPure, Category = "Manhunt|Jail")
	float GetRadius() const { return Radius; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Jail")
	TObjectPtr<UStaticMeshComponent> PlaceholderCageMesh;

	/** gameConfig.world.jail.radius (360cm / 3.6m default -- see
	 *  UManhuntGameSettings::JailRadiusCm, which this should be kept in
	 *  sync with once the real jail plaza is built). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Manhunt|Jail")
	float Radius = 360.f;
};
