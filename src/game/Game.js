import * as THREE from 'three';
import { EffectComposer } from 'three/addons/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/addons/postprocessing/RenderPass.js';
import { UnrealBloomPass } from 'three/addons/postprocessing/UnrealBloomPass.js';
import { OutputPass } from 'three/addons/postprocessing/OutputPass.js';
import { GameConfig } from '../config/gameConfig.js';
import { World } from '../world/World.js';
import { GROUND_LAYER } from '../world/CollisionSystem.js';
import { InputManager } from './InputManager.js';
import { PlayerController } from './PlayerController.js';
import { CrewAI } from './CrewAI.js';
import { HunterAI, HunterState } from './HunterAI.js';
import { DetectionSystem } from './DetectionSystem.js';
import { RescueSystem } from './RescueSystem.js';
import { ThirdPersonCamera } from '../camera/ThirdPersonCamera.js';
import { AudioManager } from '../audio/AudioManager.js';
import { HUD } from '../ui/HUD.js';
import { distance2D } from '../utils.js';

/** Top-level orchestrator: owns the renderer/scene/camera and wires every subsystem together each frame. */
export class Game {
  constructor(container) {
    this.container = container;
    this.mobile = 'ontouchstart' in window || navigator.maxTouchPoints > 0;

    this._buildRenderer();
    this.scene = new THREE.Scene();
    this.camera = new THREE.PerspectiveCamera(GameConfig.camera.fovBase, innerWidth / innerHeight, 0.1, 200);
    this._buildComposer();

    this.world = new World(this.scene, { mobile: this.mobile });
    this.detection = new DetectionSystem(this.world.collision);
    this.input = new InputManager();
    this.hud = new HUD(this.input);
    this.input.bindTouchZones(this.hud.touchZoneElements());
    this.audio = new AudioManager();
    this.camRig = new ThirdPersonCamera(this.camera, this.world);

    this.player = new PlayerController(this.world, this.input);
    this.scene.add(this.player.mesh);

    this.hunter = new HunterAI(this.world, this.detection);
    this.scene.add(this.hunter.mesh);
    this.hunter.onSpot = () => {
      this.hud.log('HUNTER SPOTTED YOU — BREAK LINE OF SIGHT.', 'bad');
      this.camRig.addShake(0.18);
    };

    this.crew = [];
    for (let i = 0; i < GameConfig.crew.count; i++) {
      const c = new CrewAI(this.world, i);
      this.scene.add(c.mesh);
      this.crew.push(c);
    }

    this.rescue = new RescueSystem(this.world, this.crew, this.hunter, this.audio, (text, cls) => this.hud.log(text, cls));

    this.running = false;
    this.paused = false;
    this.pauseStartedAt = 0;
    this.roundStartAt = 0;
    this.lastTime = performance.now();
    this.detectionMeter = 0;
    this.best = Number(localStorage.getItem('pm_best') || 0);
    this._footstepTimer = 0;
    this._wasSeen = false;

    this.hud.showIntro();
    this.hud.onStartClick(() => this._onStart());
    this.hud.setLoading(100, 'Block ready — four blocks, one Hunter.');
    this.hud.revealStart();

    addEventListener('resize', () => this._onResize());
    requestAnimationFrame((t) => this._loop(t));

    if (typeof window !== 'undefined') window.__game = this;
  }

  _buildRenderer() {
    this.renderer = new THREE.WebGLRenderer({ antialias: true, powerPreference: 'high-performance' });
    const pr = this.mobile ? GameConfig.rendering.pixelRatioMobile : GameConfig.rendering.pixelRatioDesktop;
    this.renderer.setPixelRatio(Math.min(devicePixelRatio, pr));
    this.renderer.setSize(innerWidth, innerHeight);
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 0.85;
    this.container.appendChild(this.renderer.domElement);
  }

  /**
   * A light UnrealBloomPass so streetlamps, lit windows, and storefront/sign
   * emissives glow instead of just being flat-bright pixels — one of the
   * highest-value, lowest-risk visual upgrades for a neon/practical-lit
   * night scene. Threshold is low enough to catch the world's emissive
   * (toneMapped:false) materials — window glow, signage, puddle highlights
   * — without blooming the whole image.
   */
  _buildComposer() {
    this.composer = new EffectComposer(this.renderer);
    this.composer.setPixelRatio(this.renderer.getPixelRatio());
    this.composer.setSize(innerWidth, innerHeight);

    this.composer.addPass(new RenderPass(this.scene, this.camera));

    this.bloomPass = new UnrealBloomPass(new THREE.Vector2(innerWidth, innerHeight), 0.55, 0.4, 0.2);
    this.composer.addPass(this.bloomPass);

    this.composer.addPass(new OutputPass());
  }

  _onResize() {
    this.camera.aspect = innerWidth / innerHeight;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(innerWidth, innerHeight);
    this.composer.setSize(innerWidth, innerHeight);
    this.bloomPass.setSize(innerWidth, innerHeight);
  }

  _onStart() {
    this.audio.init();
    this.audio.resume();
    if (this.paused) {
      this.roundStartAt += performance.now() - this.pauseStartedAt;
      this.lastTime = performance.now();
      this.running = true;
      this.paused = false;
      this.hud.hideOverlay();
    } else {
      this._resetRound();
    }
  }

  _resetRound() {
    const p = this.world.randomSpawnPoint(GameConfig.player.radius);
    this.player.spawn(p.x, p.z);

    this.crew.forEach((c) => {
      const q = this.world.randomSpawnPoint(c.radius);
      c.spawn(q.x, q.z);
    });

    const h = this.world.randomSpawnPoint(this.hunter.radius, 10);
    this.hunter.spawn(h.x, h.z);
    this.hunter.currentTargetRef = null;

    this.rescue.reset();
    this.detectionMeter = 0;
    this._wasSeen = false;

    this.input.crouchActive = false;
    this.input.sprintHeld = false;
    this.input.lookYaw = Math.atan2(this.player.x - this.hunter.x, this.player.z - this.hunter.z);
    this.input.lookPitch = 0.24;

    this.roundStartAt = performance.now();
    this.lastTime = this.roundStartAt;
    this.running = true;
    this.paused = false;
    this.hud.hideOverlay();
    this.hud.clearLog();
    this.hud.log('Stay out of the Hunter’s sight.', 'warn');

    this.camRig.startCinematic(GameConfig.round.cinematicIntroMs, this.input.lookYaw);
    this.hud.playCinematicLetterbox(GameConfig.round.cinematicIntroMs);
  }

  _finish(win) {
    this.running = false;
    const survived = Math.min(GameConfig.round.durationSeconds, (performance.now() - this.roundStartAt) / 1000);
    if (survived > this.best) {
      this.best = survived;
      localStorage.setItem('pm_best', String(this.best));
    }
    this.audio.play(win ? 'win' : 'lose');
    this.audio.setChasing(false);
    this.hud.showResult(win, survived, this.rescue.rescuesThisRound, this.best);
  }

  _pause() {
    if (!this.running) return;
    this.running = false;
    this.paused = true;
    this.pauseStartedAt = performance.now();
    this.audio.setChasing(false);
    this.hud.showPaused();
  }

  _loop(t) {
    const dt = Math.min(0.04, (t - this.lastTime) / 1000);
    this.lastTime = t;

    if (this.input.consumeFlag('pause')) this._pause();

    if (this.running) {
      this._update(dt, t);
    } else {
      this.world.update(dt);
    }

    this.camRig.update(dt, t, { player: this.player, input: this.input, sprinting: this.player.sprinting });
    this.audio.update(t);

    this.composer.render();
    requestAnimationFrame((tt) => this._loop(tt));
  }

  _update(dt, now) {
    const elapsed = (now - this.roundStartAt) / 1000;
    const remaining = GameConfig.round.durationSeconds - elapsed;
    if (remaining <= 0) {
      this._finish(true);
      return;
    }

    this.world.update(dt);
    this.detection.pruneNoise(now);

    this.player.update(dt, now, { camYaw: this.input.lookYaw, detection: this.detection });

    if (this.input.consumeFlag('decoy')) {
      this.player.throwDecoy(this.detection, now);
      this.hud.log('Decoy noise thrown down the block.');
      this.audio.play('decoy');
    }
    if (this.input.consumeFlag('rescue')) {
      this.rescue.tryRescue(this.player);
    }

    this.crew.forEach((c) => c.update(dt, now, this.hunter, this.detection));

    const freeCrew = this.crew.filter((c) => !c.caught);
    this.hunter.update(dt, now, { player: this.player, crew: freeCrew });

    this.rescue.captureIfTouching();

    if (!this.player.caught && this.player.roofLayer === null) {
      const dPlayer = distance2D(this.player.x, this.player.z, this.hunter.x, this.hunter.z);
      if (dPlayer < GameConfig.hunter.captureDistance) {
        this.player.markCaught();
        this.camRig.addShake(0.32);
        this.audio.play('capture');
        setTimeout(() => this._finish(false), 550);
      }
    }

    this._updateAlertMeter(dt, now);
    this._updateFootsteps(dt);

    this.hud.update({
      timeRemaining: remaining,
      stamina: this.player.stamina,
      detection: this.detectionMeter,
      chasing: this._chasingPlayer,
      freeCount: freeCrew.length,
      jailedCount: this.rescue.jailedCount(),
      rescuePromptVisible: this.rescue.jailedCount() > 0 && this.rescue.isPlayerInRange(this.player),
    });
  }

  _updateAlertMeter(dt, now) {
    const cfg = GameConfig.hunter;
    const chasingPlayer = this.hunter.state === HunterState.CHASE && this.hunter.currentTargetRef === this.player;
    this._chasingPlayer = chasingPlayer;

    let visible = false;
    if (this.player.roofLayer === null) {
      const range = cfg.viewDistance.base * (this.player.crouched ? cfg.viewDistance.crouchTargetMultiplier : 1);
      visible = this.detection.canSee(
        { x: this.hunter.x, z: this.hunter.z, y: 0, facingRad: this.hunter.facing },
        { x: this.player.x, z: this.player.z, y: this.player.y },
        { range, fovDegrees: cfg.fovDegrees, layer: GROUND_LAYER },
      );
    }

    this.detectionMeter = this.detection.updateMeter(this.detectionMeter, {
      visible,
      dt,
      riseRate: GameConfig.detection.riseRate,
      decayRate: GameConfig.detection.decayRate,
      floor: chasingPlayer ? GameConfig.detection.chaseFloor : 0,
    });

    this.audio.setChasing(chasingPlayer);
    this.audio.setHeartbeatIntensity(this.detectionMeter / 100, now);

    if (chasingPlayer && !this._wasSeen) {
      this._wasSeen = true;
    } else if (!chasingPlayer) {
      this._wasSeen = false;
    }
  }

  _updateFootsteps(dt) {
    if (this.player.climbing || this.player.caught || this.player.lastMoveMagnitude < 0.1) {
      this._footstepTimer = 0;
      return;
    }
    this._footstepTimer -= dt;
    if (this._footstepTimer <= 0) {
      this.audio.play('footstep');
      this._footstepTimer = this.player.sprinting ? 0.26 : this.player.crouched ? 0.5 : 0.38;
    }
  }
}
