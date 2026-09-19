#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ManhuntGameMode.generated.h"

class AManhuntHunterCharacter;
class AManhuntPlayerCharacter;

/**
 * Round rules and transitions -- direct port of the orchestration half of
 * src/game/Game.js (the round timer, win/lose, and the raw capture-distance
 * check that ends the round regardless of the Hunter's AI state). See
 * /docs/UNREAL_MIGRATION_PLAN.md's Game.js row: GameMode owns rules,
 * AManhuntGameState (see that class) owns the state the HUD reads.
 *
 * IMPORTANT, ported deliberately from the browser build: capture is a raw
 * distance check run every frame here, independent of
 * AManhuntHunterController's current state -- see src/game/Game.js
 * `_update()`, which captures the player even if the Hunter's AI is
 * technically "Investigating" a decoy at the moment it closes the distance.
 * Do not gate this check on CurrentState == Chase.
 */
UCLASS()
class PROJECTMANHUNT_API AManhuntGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AManhuntGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Manhunt|Round")
	void RestartRound();

protected:
	void CheckCaptures();
	void EndRound(bool bWasWin);

	UFUNCTION()
	void HandleHunterSpottedTarget(AActor* Target);

	UFUNCTION()
	void HandleHunterLostTarget();

	UPROPERTY()
	TObjectPtr<AManhuntHunterCharacter> CachedHunter;

	UPROPERTY()
	TObjectPtr<AManhuntPlayerCharacter> CachedPlayer;

	bool bCaptureResolvedThisRound = false;
};
