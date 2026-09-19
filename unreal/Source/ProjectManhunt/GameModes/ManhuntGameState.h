#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ManhuntGameState.generated.h"

class URescueComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnManhuntRoundEnded, bool, bWasWin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnManhuntRoundStarted);

/**
 * Round state: the timer, win/lose, and ownership of URescueComponent.
 * Direct port of the round-level bookkeeping in src/game/Game.js
 * (`roundStartAt`, `running`, `_finish()`) -- the browser build kept this in
 * one orchestrator object; UE5 idiomatically splits it into GameMode (rules
 * / transitions, server-authoritative) and GameState (replicated state,
 * HUD-readable) -- see /docs/UNREAL_MIGRATION_PLAN.md's Game.js row.
 */
UCLASS()
class PROJECTMANHUNT_API AManhuntGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AManhuntGameState();

	UFUNCTION(BlueprintPure, Category = "Manhunt|Round")
	float GetTimeRemainingSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Manhunt|Round")
	bool IsRoundActive() const { return bRoundActive; }

	URescueComponent* GetRescueComponent() const { return RescueComponent; }

	UPROPERTY(BlueprintAssignable, Category = "Manhunt|Round")
	FOnManhuntRoundStarted OnRoundStarted;

	UPROPERTY(BlueprintAssignable, Category = "Manhunt|Round")
	FOnManhuntRoundEnded OnRoundEnded;

	// -- Called by AManhuntGameMode only; public for that call site, not
	// intended as general-purpose API (would be `friend class
	// AManhuntGameMode` in a stricter pass, kept simple for this scaffold). --

	void BeginRound();
	void EndRound(bool bWasWin);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Round")
	bool bRoundActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Round")
	float RoundElapsedSeconds = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Round")
	int32 RescuesThisRound = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manhunt|Round")
	TObjectPtr<URescueComponent> RescueComponent;

	friend class AManhuntGameMode;
};
