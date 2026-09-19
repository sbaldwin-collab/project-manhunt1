#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ManhuntDetectableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UManhuntDetectableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by any character UManhuntDetectionComponent can be asked to
 * sense (player, crew). Lets the detection gating logic in
 * UManhuntDetectionComponent query crouch state and vertical layer without
 * casting to a concrete character class -- see /docs/HUNTER_AI_SPEC.md
 * Section 4 for the rules this feeds (crouch view-distance multiplier,
 * rooftop elevation gate).
 */
class PROJECTMANHUNT_API IManhuntDetectableInterface
{
	GENERATED_BODY()

public:
	/** Mirrors the `crouched` flag read by DetectionSystem.js candidates. */
	UFUNCTION(BlueprintNativeEvent, Category = "Manhunt|Detection")
	bool IsCrouchingForDetection() const;

	/** True while on a rooftop layer, mirroring PlayerController.js's
	 *  `roofLayer !== null`. Used as a fast-path in addition to the raw
	 *  elevation (Z) check, in case a rooftop and the street below it are
	 *  ever close enough in Z to otherwise confuse the gate. */
	UFUNCTION(BlueprintNativeEvent, Category = "Manhunt|Detection")
	bool IsOnRooftopLayer() const;
};
