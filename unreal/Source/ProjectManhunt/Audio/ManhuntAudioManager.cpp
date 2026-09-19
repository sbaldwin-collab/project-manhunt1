#include "Audio/ManhuntAudioManager.h"
#include "Data/ManhuntGameSettings.h"
#include "ProjectManhunt.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void UManhuntAudioManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// The two continuous ambience beds (rain, city ambience) and the chase
	// tension loop start quietly-but-immediately, matching
	// src/audio/AudioManager.js `init()`'s `_startLoop('rain', ...)` /
	// `_startLoop('cityAmbience', ...)` / `_startLoop('chase', ...)` calls.
	GetOrStartLoop(TEXT("Rain"));
	GetOrStartLoop(TEXT("CityAmbience"));
	if (UAudioComponent* ChaseLoop = GetOrStartLoop(TEXT("Chase")))
	{
		ChaseLoop->SetVolumeMultiplier(0.f); // silent until SetChasing(true)
	}
}

void UManhuntAudioManager::Deinitialize()
{
	for (auto& Pair : ActiveLoops)
	{
		if (Pair.Value)
		{
			Pair.Value->Stop();
		}
	}
	ActiveLoops.Empty();

	Super::Deinitialize();
}

USoundBase* UManhuntAudioManager::ResolveCue(FName CueName) const
{
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	if (const TSoftObjectPtr<USoundBase>* SoftCue = Settings->AudioCues.Find(CueName))
	{
		// Synchronous load for scaffold simplicity. A production pass should
		// prefer UAssetManager-driven async loading / preloading the small
		// set of always-needed cues (footstep, heartbeat) at level start so
		// Play() never stalls the game thread on a first-use disk hit.
		return SoftCue->LoadSynchronous();
	}

	UE_LOG(LogManhunt, Verbose, TEXT("UManhuntAudioManager: no cue mapped for \"%s\" in UManhuntGameSettings::AudioCues -- silently ignored (see /docs/ASSET_MANIFEST.md Section 10)."), *CueName.ToString());
	return nullptr;
}

void UManhuntAudioManager::Play(FName CueName)
{
	if (USoundBase* Sound = ResolveCue(CueName))
	{
		const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
		UGameplayStatics::PlaySound2D(this, Sound, Settings->AudioMasterGain);
	}
}

void UManhuntAudioManager::PlayAtLocation(FName CueName, FVector Location)
{
	if (USoundBase* Sound = ResolveCue(CueName))
	{
		const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, Settings->AudioMasterGain);
	}
}

UAudioComponent* UManhuntAudioManager::GetOrStartLoop(FName CueName)
{
	if (TObjectPtr<UAudioComponent>* Existing = ActiveLoops.Find(CueName))
	{
		return *Existing;
	}

	USoundBase* Sound = ResolveCue(CueName);
	if (!Sound)
	{
		return nullptr;
	}

	UAudioComponent* Loop = UGameplayStatics::SpawnSound2D(this, Sound, UManhuntGameSettings::Get()->AudioMasterGain, 1.f, 0.f, nullptr, /*bPersistAcrossLevelTransition=*/true);
	ActiveLoops.Add(CueName, Loop);
	return Loop;
}

void UManhuntAudioManager::SetChasing(bool bActive)
{
	if (UAudioComponent* ChaseLoop = GetOrStartLoop(TEXT("Chase")))
	{
		// src/audio/AudioManager.js used a ~0.4s exponential approach via
		// setTargetAtTime; FInterpTo-per-tick from a Tickable owner is the
		// equivalent here. This scaffold sets it immediately for simplicity
		// -- wire up a smoothed transition once this class has a tick path
		// (e.g. driven from AManhuntGameMode::Tick, which already knows
		// AManhuntHunterController::GetCurrentState()).
		ChaseLoop->SetVolumeMultiplier(bActive ? 1.f : 0.f);
	}
}

void UManhuntAudioManager::SetHeartbeatIntensity(float Intensity01)
{
	if (Intensity01 <= 0.02f)
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	const double IntervalSeconds = 0.68 - Intensity01 * 0.38; // browser: 680ms - intensity*380ms

	if (LastHeartbeatTime < 0.0 || (Now - LastHeartbeatTime) > IntervalSeconds)
	{
		Play(TEXT("Heartbeat"));
		LastHeartbeatTime = Now;
	}
}

void UManhuntAudioManager::SetMasterVolume(float Volume01)
{
	for (auto& Pair : ActiveLoops)
	{
		if (Pair.Value)
		{
			Pair.Value->SetVolumeMultiplier(Volume01);
		}
	}
	// Note: this does not affect the gain of one-shot Play()/PlayAtLocation()
	// calls already in flight -- route both through a dedicated Sound Mix /
	// Sound Class (MetaSounds-compatible) for a real master-volume control
	// once the audio pass begins, rather than per-call gain arguments.
}
