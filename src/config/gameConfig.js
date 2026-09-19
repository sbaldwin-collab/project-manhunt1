// Central tuning file for PROJECT MANHUNT. Every gameplay number a designer
// would want to iterate on lives here so no other module hardcodes constants.

export const GameConfig = {
  round: {
    durationSeconds: 90,
    cinematicIntroMs: 2600,
  },

  world: {
    // Half-extent of the playable NYC block cluster.
    bounds: 58,
    jail: { x: 9, z: 9, radius: 3.6 },
  },

  player: {
    walkSpeed: 3.6,
    sprintSpeed: 6.3,
    crouchSpeed: 1.75,
    turnLerp: 10,
    stamina: {
      max: 100,
      sprintDrainPerSec: 24,
      regenPerSec: 14,
      idleRegenPerSec: 20,
      minToSprint: 3,
    },
    sprintNoiseChancePerSec: 6.5,
    radius: 0.42,
  },

  crew: {
    count: 3,
    wanderSpeed: 1.25,
    fleeSpeed: 3.6,
    fleeTriggerDistance: 9.5,
    radius: 0.42,
  },

  hunter: {
    radius: 0.46,
    speeds: {
      patrol: 1.8,
      suspicious: 1.4,
      investigate: 2.7,
      search: 3.0,
      chase: 4.9,
    },
    fovDegrees: 100,
    viewDistance: {
      base: 15,
      crouchTargetMultiplier: 0.58,
      patrolMultiplier: 0.72,
    },
    hearingRadius: 20,
    captureDistance: 0.95,
    // Time (ms) spent fully alert before locking to chase after first spotting.
    suspiciousToSpotMs: 550,
    // How long the hunter keeps pursuing a lost target's last known position.
    searchDurationMs: 4500,
    // After search expires and nothing found, settle time before returning to patrol.
    returnToPatrolGraceMs: 1200,
    guardCrewChance: 0.35,
  },

  detection: {
    // Detection meter (0-100) rises when in FOV+LOS, decays otherwise.
    riseRate: 130,
    decayRate: 85,
    chaseFloor: 78,
  },

  camera: {
    distance: 4.6,
    shoulderOffset: 0.68,
    heightBase: 2.15,
    pitchHeightRange: 2.1,
    collisionPadding: 0.35,
    fovBase: 56,
    fovSprint: 62,
    followLerpBase: 0.0018,
    lookSensitivityTouch: 0.006,
    lookSensitivityMouse: 0.0045,
  },

  rendering: {
    pixelRatioMobile: 1.5,
    pixelRatioDesktop: 2,
    shadowMapSize: 1024,
    fogColor: 0x0a121c,
    fogDensity: 0.019,
  },

  colors: {
    moon: 0x9bbdff,
    ambientSky: 0x263d5c,
    ambientGround: 0x05070a,
    sodium: 0xffa85f,
    playerAccent: 0x2f78c4,
    hunterAccent: 0x7a2430,
    crewAccents: [0x2c6046, 0x5e4269, 0x3d5f78],
  },

  audio: {
    masterGain: 0.5,
    // Set a URL here (e.g. 'assets/audio/rain.mp3') to use a real asset;
    // leave null to fall back to the built-in procedural synthesis.
    assets: {
      footstep: null,
      rain: null,
      trafficDistant: null,
      siren: null,
      subwayRumble: null,
      cityAmbience: null,
      chaseCue: null,
      heartbeat: null,
      capture: null,
      rescue: null,
      interact: null,
    },
  },

  // Toggle to true once real character models exist under assets/characters/.
  characters: {
    useExternalModels: false,
    manifest: {
      player: 'assets/characters/player.glb',
      hunter: 'assets/characters/hunter.glb',
      crew: 'assets/characters/crew.glb',
    },
  },
};
