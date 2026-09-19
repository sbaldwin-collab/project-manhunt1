#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ManhuntAudioManager.generated.h"

class UAudioComponent;

/**
 * Direct port of src/audio/AudioManager.js's public API (Play/loop
 * start-stop/heartbeat/chase-gain), reimplemented against real sound assets
 * instead of runtime Web Audio synthesis -- see /docs/ASSET_MANIFEST.md
 * Section 10 and /docs/UNREAL_MIGRATION_PLAN.md's AudioManager.js row.
 *
 * A UGameInstanceSubsystem so any system (character, AI controller, HUD)
 * can reach it via `GetGameInstance()->GetSubsystem<UManhuntAudioManager>()`
 * without a manual singleton or Actor reference-passing, replacing the
 * browser build's single `audio` object threaded through Game.js's
 * constructor.
 *
 * All cue assets are resolved by name through
 * UManhuntGameSettings::AudioCues (soft references, loaded on first use) --
 * nothing in this class hardcodes an asset reference, so swapping
 * placeholder cues for final audio never touches code, matching the
 * browser build's own "swap a config value, not the implementation" design
 * for src/config/gameConfig.js's `audio.assets`.
 */
UCLASS()
class PROJECTMANHUNT_API UManhuntAudioManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Non-spatial one-shot (UI/HUD-adjacent cues: capture/rescue/interact/win/lose stingers). */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Audio")
	void Play(FName CueName);

	/** Spatial one-shot at a world location (footsteps, decoys). */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Audio")
	void PlayAtLocation(FName CueName, FVector Location);

	/** src/audio/AudioManager.js `setChasing()` -- fades the chase-tension
	 *  loop's gain in/out rather than hard start/stop. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Audio")
	void SetChasing(bool bActive);

	/** src/audio/AudioManager.js `setHeartbeatIntensity()` -- intensity 0..1,
	 *  typically driven by the HUD detection meter. Triggers a one-shot
	 *  heartbeat cue at an interval that shortens with intensity, matching
	 *  the browser's timer-gated beep pattern. */
	UFUNCTION(BlueprintCallable, Category = "Manhunt|Audio")
	void SetHeartbeatIntensity(float Intensity01);

	UFUNCTION(BlueprintCallable, Category = "Manhunt|Audio")
	void SetMasterVolume(float Volume01);

private:
	UAudioComponent* GetOrStartLoop(FName CueName);
	class USoundBase* ResolveCue(FName CueName) const;

	UPROPERTY()
	TMap<FName, TObjectPtr<UAudioComponent>> ActiveLoops;

	float LastHeartbeatTime = -1.f;
};
