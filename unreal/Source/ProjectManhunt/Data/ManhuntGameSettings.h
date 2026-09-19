#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ManhuntGameSettings.generated.h"

class USoundBase;

/**
 * Central tuning asset for PROJECT MANHUNT, exposed under
 * Project Settings > Game > Manhunt Game Settings.
 *
 * This is a direct, value-for-value port of src/config/gameConfig.js in the
 * browser reference build (see /docs/UNREAL_MIGRATION_PLAN.md Section 5).
 * Every field below cites the JS field it replaces so the two stay in sync;
 * if a browser-build value changes, mirror it here (and vice versa) rather
 * than letting the two implementations drift apart.
 *
 * Distances are in Unreal's native centimeters; the browser build's meter
 * values are multiplied by 100 here. See /docs/NYC_LEVEL_DESIGN.md Section 2
 * for the full unit-conversion discussion.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Manhunt Game Settings"))
class PROJECTMANHUNT_API UManhuntGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UManhuntGameSettings();

	static const UManhuntGameSettings* Get();

	// ---- round (gameConfig.round) --------------------------------------

	/** gameConfig.round.durationSeconds */
	UPROPERTY(EditDefaultsOnly, config, Category = "Round")
	float RoundDurationSeconds = 90.f;

	/** gameConfig.round.cinematicIntroMs */
	UPROPERTY(EditDefaultsOnly, config, Category = "Round")
	float CinematicIntroSeconds = 2.6f;

	// ---- world (gameConfig.world) ---------------------------------------

	/** gameConfig.world.bounds (58m -> 5800cm). Advisory only in UE5 -- the
	 *  level's actual collision/NavMesh extents are authoritative. */
	UPROPERTY(EditDefaultsOnly, config, Category = "World")
	float WorldBoundsCm = 5800.f;

	/** gameConfig.world.jail.{x,z} -- authored as a level Actor transform in
	 *  practice (AJailVolume), kept here only as the documented reference
	 *  starting position from the browser layout. */
	UPROPERTY(EditDefaultsOnly, config, Category = "World")
	FVector2D JailLocationCm = FVector2D(900.f, 900.f);

	/** gameConfig.world.jail.radius */
	UPROPERTY(EditDefaultsOnly, config, Category = "World")
	float JailRadiusCm = 360.f;

	// ---- player (gameConfig.player) --------------------------------------

	/** gameConfig.player.walkSpeed (cm/s) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Player")
	float PlayerWalkSpeed = 360.f;

	/** gameConfig.player.sprintSpeed (cm/s) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Player")
	float PlayerSprintSpeed = 630.f;

	/** gameConfig.player.crouchSpeed (cm/s) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Player")
	float PlayerCrouchSpeed = 175.f;

	/** gameConfig.player.turnLerp -- retuned in UE5 via turn-in-place
	 *  animation (see /docs/ANIMATION_SPEC.md) rather than a raw rotation
	 *  interpolation rate; kept as a fallback for non-animated rotation. */
	UPROPERTY(EditDefaultsOnly, config, Category = "Player")
	float PlayerTurnLerpRate = 10.f;

	/** gameConfig.player.stamina.max */
	UPROPERTY(EditDefaultsOnly, config, Category = "Player|Stamina")
	float StaminaMax = 100.f;

	/** gameConfig.player.stamina.sprintDrainPerSec */
	UPROPERTY(EditDefaultsOnly, config, Category = "Player|Stamina")
	float StaminaSprintDrainPerSec = 24.f;

	/** gameConfig.player.stamina.regenPerSec (while moving, not sprinting) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Player|Stamina")
	float StaminaRegenPerSec = 14.f;

	/** gameConfig.player.stamina.idleRegenPerSec */
	UPROPERTY(EditDefaultsOnly, config, Category = "Player|Stamina")
	float StaminaIdleRegenPerSec = 20.f;

	/** gameConfig.player.stamina.minToSprint */
	UPROPERTY(EditDefaultsOnly, config, Category = "Player|Stamina")
	float StaminaMinToSprint = 3.f;

	/** gameConfig.player.sprintNoiseChancePerSec -- probability per second,
	 *  while sprinting, of emitting an AI-audible noise event. Per
	 *  /docs/ANIMATION_SPEC.md Section 8, prefer driving this from the
	 *  footstep Anim Notify once real locomotion animation exists rather
	 *  than a flat per-second roll. */
	UPROPERTY(EditDefaultsOnly, config, Category = "Player", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SprintNoiseChancePerSec = 0.065f;

	/** gameConfig.player.radius (cm) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Player")
	float PlayerCapsuleRadiusCm = 42.f;

	// ---- crew (gameConfig.crew) -------------------------------------------

	/** gameConfig.crew.count */
	UPROPERTY(EditDefaultsOnly, config, Category = "Crew")
	int32 CrewCount = 3;

	/** gameConfig.crew.wanderSpeed (cm/s) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Crew")
	float CrewWanderSpeed = 125.f;

	/** gameConfig.crew.fleeSpeed (cm/s) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Crew")
	float CrewFleeSpeed = 360.f;

	/** gameConfig.crew.fleeTriggerDistance (cm) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Crew")
	float CrewFleeTriggerDistanceCm = 950.f;

	/** gameConfig.crew.radius (cm) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Crew")
	float CrewCapsuleRadiusCm = 42.f;

	// ---- hunter (gameConfig.hunter) ----------------------------------------

	/** gameConfig.hunter.radius (cm) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter")
	float HunterCapsuleRadiusCm = 46.f;

	/** gameConfig.hunter.speeds.patrol (cm/s) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Speeds")
	float HunterSpeedPatrol = 180.f;

	/** gameConfig.hunter.speeds.suspicious (cm/s) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Speeds")
	float HunterSpeedSuspicious = 140.f;

	/** gameConfig.hunter.speeds.investigate (cm/s) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Speeds")
	float HunterSpeedInvestigate = 270.f;

	/** gameConfig.hunter.speeds.search (cm/s) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Speeds")
	float HunterSpeedSearch = 300.f;

	/** gameConfig.hunter.speeds.chase (cm/s) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Speeds")
	float HunterSpeedChase = 490.f;

	/** gameConfig.hunter.fovDegrees -- full cone angle; AIPerceptionComponent
	 *  wants the HALF angle (see /docs/HUNTER_AI_SPEC.md Section 4). */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Detection")
	float HunterFovDegrees = 100.f;

	/** gameConfig.hunter.viewDistance.base (cm) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Detection")
	float HunterViewDistanceBaseCm = 1500.f;

	/** gameConfig.hunter.viewDistance.crouchTargetMultiplier -- applied to
	 *  the effective sight range when the SENSED ACTOR is crouched. */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Detection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CrouchTargetViewMultiplier = 0.58f;

	/** gameConfig.hunter.viewDistance.patrolMultiplier -- applied to the
	 *  effective sight range while the Hunter itself is in Patrol/Return. */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Detection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PatrolAlertnessViewMultiplier = 0.72f;

	/** gameConfig.hunter.hearingRadius (cm) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Detection")
	float HunterHearingRadiusCm = 2000.f;

	/** gameConfig.hunter.captureDistance (cm) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Detection")
	float CaptureDistanceCm = 95.f;

	/** Elevation gate: a target more than this far above/below the Hunter
	 *  (e.g. on a rooftop) can never be sensed, matching the browser's
	 *  canSee() short-circuit. See /docs/HUNTER_AI_SPEC.md Section 4 --
	 *  re-derive against real story heights once the modular kit is final. */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|Detection")
	float ElevationDetectionGateCm = 320.f;

	/** gameConfig.hunter.suspiciousToSpotMs (seconds here, not ms) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|State Timings")
	float SuspiciousToChaseSeconds = 0.55f;

	/** gameConfig.hunter.searchDurationMs (seconds) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|State Timings")
	float SearchDurationSeconds = 4.5f;

	/** gameConfig.hunter.returnToPatrolGraceMs (seconds) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|State Timings")
	float ReturnToPatrolGraceSeconds = 1.2f;

	/** Investigate state's total look-around budget once arrived at the
	 *  noise location (browser: 2200ms total, with a 3x-accelerated timer
	 *  once within arrival radius). */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|State Timings")
	float InvestigateTotalSeconds = 2.2f;

	/** gameConfig.hunter.guardCrewChance */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|State Timings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GuardCrewChance = 0.35f;

	/** Fixed guard-state duration (browser: hardcoded 9000ms in HunterAI.js,
	 *  not in gameConfig.js -- surfaced here as a tunable). */
	UPROPERTY(EditDefaultsOnly, config, Category = "Hunter|State Timings")
	float GuardDurationSeconds = 9.f;

	// ---- detection meter (gameConfig.detection) -- HUD-facing only ---------

	/** gameConfig.detection.riseRate (per second, 0-100 scale) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Detection Meter (HUD)")
	float DetectionMeterRiseRate = 130.f;

	/** gameConfig.detection.decayRate (per second, 0-100 scale) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Detection Meter (HUD)")
	float DetectionMeterDecayRate = 85.f;

	/** gameConfig.detection.chaseFloor */
	UPROPERTY(EditDefaultsOnly, config, Category = "Detection Meter (HUD)")
	float DetectionMeterChaseFloor = 78.f;

	// ---- camera (gameConfig.camera) ----------------------------------------

	/** gameConfig.camera.distance (cm) -- SpringArm TargetArmLength */
	UPROPERTY(EditDefaultsOnly, config, Category = "Camera")
	float CameraArmLengthCm = 460.f;

	/** gameConfig.camera.shoulderOffset (cm) -- SpringArm socket offset X (or
	 *  Y depending on convention chosen in ManhuntPlayerCharacter) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Camera")
	float CameraShoulderOffsetCm = 68.f;

	/** gameConfig.camera.fovBase (degrees) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Camera")
	float CameraFovBase = 56.f;

	/** gameConfig.camera.fovSprint (degrees) */
	UPROPERTY(EditDefaultsOnly, config, Category = "Camera")
	float CameraFovSprint = 62.f;

	/** How quickly FOV interpolates between base/sprint (higher = faster). */
	UPROPERTY(EditDefaultsOnly, config, Category = "Camera")
	float CameraFovInterpSpeed = 6.f;

	// ---- audio (gameConfig.audio) ------------------------------------------

	/** gameConfig.audio.masterGain */
	UPROPERTY(EditDefaultsOnly, config, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AudioMasterGain = 0.5f;

	/**
	 * Named one-shot/loop cue library, keyed the same way
	 * src/audio/AudioManager.js's play() switch and _startLoop() calls are
	 * named (Footstep, Decoy, Interact, Capture, Rescue, Siren, Subway, Win,
	 * Lose, Rain, CityAmbience, Chase, Heartbeat). Assign MetaSound or Sound
	 * Cue assets per key in Project Settings once real audio exists -- see
	 * /docs/ASSET_MANIFEST.md Section 10. UManhuntAudioManager (see
	 * Audio/ManhuntAudioManager.h) reads this map; nothing else in the
	 * codebase should reference an audio asset directly.
	 */
	UPROPERTY(EditDefaultsOnly, config, Category = "Audio")
	TMap<FName, TSoftObjectPtr<USoundBase>> AudioCues;
};
