import * as THREE from 'three';
import { GameConfig } from '../config/gameConfig.js';
import { createCharacter } from './CharacterFactory.js';
import { lerpAngle, distance2D, clamp } from '../utils.js';
import { GROUND_LAYER } from '../world/CollisionSystem.js';

// The Hunter's full behaviour state machine. States map directly onto the
// brief: Patrol, Suspicious, Investigate (sound), Spot (the brief instant a
// target crosses into full detection, folded into the Suspicious->Chase
// transition below), Chase, Lose sight, Search last known position, Return
// to patrol, Guard captured crew.
export const HunterState = Object.freeze({
  PATROL: 'patrol',
  SUSPICIOUS: 'suspicious',
  INVESTIGATE: 'investigate',
  CHASE: 'chase',
  SEARCH: 'search',
  RETURN: 'return',
  GUARD: 'guard',
});

export class HunterAI {
  constructor(world, detection) {
    this.world = world;
    this.detection = detection;
    this.character = createCharacter('hunter');
    this.mesh = this.character.root;
    this.x = 0;
    this.z = 0;
    this.facing = 0;
    this.radius = GameConfig.hunter.radius;

    this.state = HunterState.PATROL;
    this.targetX = 0;
    this.targetZ = 0;
    this.lastSeen = null;
    this.currentTargetRef = null;
    this.suspiciousMeter = 0; // ms
    this.searchTimer = 0;
    this.returnTimer = 0;
    this.investigateTimer = 0;
    this.guardTimer = 0;

    this.onSpot = null; // callback(target)
    this.onLoseSight = null;
    this.onStateChange = null;
  }

  spawn(x, z) {
    this.x = x;
    this.z = z;
    this.mesh.position.set(x, 0, z);
    this._pickPatrolTarget();
  }

  notifyCaptureEvent() {
    if (Math.random() < GameConfig.hunter.guardCrewChance) {
      this._setState(HunterState.GUARD);
      this.guardTimer = 0;
    }
  }

  _setState(next) {
    if (this.state === next) return;
    this.state = next;
    if (this.onStateChange) this.onStateChange(next);
  }

  _pickPatrolTarget() {
    const p = this.world.randomSpawnPoint(this.radius, 0);
    this.targetX = p.x;
    this.targetZ = p.z;
  }

  _findVisibleTarget(candidates) {
    const cfg = GameConfig.hunter;
    const vd = cfg.viewDistance;
    const patrolFalloff = this.state === HunterState.PATROL || this.state === HunterState.RETURN ? vd.patrolMultiplier : 1;
    let best = null;
    let bestDist = Infinity;
    for (const c of candidates) {
      if (c.caught) continue;
      const crouchFalloff = c.crouched ? vd.crouchTargetMultiplier : 1;
      const range = vd.base * patrolFalloff * crouchFalloff;
      const observer = { x: this.x, z: this.z, y: 0, facingRad: this.facing };
      // Rooftops sit well outside the elevation tolerance in canSee(), so a
      // player who has climbed up is automatically out of the Hunter's
      // vision without needing a separate roof-aware AI (documented limitation).
      const target = { x: c.x, z: c.z, y: c.y ?? 0 };
      if (this.detection.canSee(observer, target, { range, fovDegrees: cfg.fovDegrees, layer: GROUND_LAYER })) {
        const d = distance2D(this.x, this.z, c.x, c.z);
        if (d < bestDist) {
          bestDist = d;
          best = c;
        }
      }
    }
    return best;
  }

  update(dt, now, { player, crew, roundElapsedMs }) {
    const cfg = GameConfig.hunter;
    const candidates = [player, ...crew];
    const spotted = this.state !== HunterState.GUARD ? this._findVisibleTarget(candidates) : null;

    if (spotted) {
      this.lastSeen = { x: spotted.x, z: spotted.z };
      this.targetX = spotted.x;
      this.targetZ = spotted.z;
      this.currentTargetRef = spotted;

      if (this.state === HunterState.CHASE) {
        this.searchTimer = 0;
      } else {
        this.suspiciousMeter += dt * 1000;
        this._setState(HunterState.SUSPICIOUS);
        if (this.suspiciousMeter >= cfg.suspiciousToSpotMs) {
          this._setState(HunterState.CHASE);
          this.suspiciousMeter = 0;
          if (this.onSpot) this.onSpot(spotted);
        }
      }
    } else {
      if (this.state === HunterState.SUSPICIOUS) {
        this.suspiciousMeter -= dt * 1400;
        if (this.suspiciousMeter <= 0) {
          this.suspiciousMeter = 0;
          this._setState(HunterState.PATROL);
        }
      } else if (this.state === HunterState.CHASE) {
        this._setState(HunterState.SEARCH);
        this.searchTimer = 0;
        if (this.lastSeen) {
          this.targetX = this.lastSeen.x;
          this.targetZ = this.lastSeen.z;
        }
        if (this.onLoseSight) this.onLoseSight();
      } else if (this.state === HunterState.SEARCH) {
        this.searchTimer += dt * 1000;
        const reachedLastKnown = this.lastSeen && distance2D(this.x, this.z, this.lastSeen.x, this.lastSeen.z) < 1.3;
        if (reachedLastKnown || this.searchTimer >= cfg.searchDurationMs) {
          this._setState(HunterState.RETURN);
          this.returnTimer = 0;
          this._pickPatrolTarget();
        }
      } else if (this.state === HunterState.RETURN) {
        this.returnTimer += dt * 1000;
        if (this.returnTimer >= cfg.returnToPatrolGraceMs) {
          this._setState(HunterState.PATROL);
        }
      } else if (this.state === HunterState.GUARD) {
        this.guardTimer += dt * 1000;
        const jail = GameConfig.world.jail;
        const orbit = this.guardTimer / 1000;
        this.targetX = jail.x + Math.cos(orbit * 0.4) * (jail.radius + 1.6);
        this.targetZ = jail.z + Math.sin(orbit * 0.4) * (jail.radius + 1.6);
        if (this.guardTimer >= 9000) {
          this._setState(HunterState.RETURN);
          this.returnTimer = 0;
          this._pickPatrolTarget();
        }
      } else if (this.state === HunterState.INVESTIGATE) {
        this.investigateTimer += dt * 1000;
        const reached = distance2D(this.x, this.z, this.targetX, this.targetZ) < 1.2;
        if (reached) this.investigateTimer += dt * 1000 * 2; // look around briskly once arrived
        if (this.investigateTimer >= 2200) {
          this._setState(HunterState.RETURN);
          this.returnTimer = 0;
          this._pickPatrolTarget();
        }
      } else {
        // PATROL: listen for noise, otherwise wander.
        const ping = this.detection.latestNoiseWithin(this.x, this.z, cfg.hearingRadius);
        if (ping) {
          this._setState(HunterState.INVESTIGATE);
          this.investigateTimer = 0;
          this.targetX = ping.x;
          this.targetZ = ping.z;
        } else if (distance2D(this.x, this.z, this.targetX, this.targetZ) < 1.1) {
          this._pickPatrolTarget();
        }
      }
    }

    this._move(dt);
    this._sync(dt);
  }

  _move(dt) {
    const cfg = GameConfig.hunter;
    const speed = {
      [HunterState.PATROL]: cfg.speeds.patrol,
      [HunterState.SUSPICIOUS]: cfg.speeds.suspicious,
      [HunterState.INVESTIGATE]: cfg.speeds.investigate,
      [HunterState.SEARCH]: cfg.speeds.search,
      [HunterState.CHASE]: cfg.speeds.chase,
      [HunterState.RETURN]: cfg.speeds.patrol,
      [HunterState.GUARD]: cfg.speeds.suspicious * 0.7,
    }[this.state];

    const dx = this.targetX - this.x;
    const dz = this.targetZ - this.z;
    const dist = Math.hypot(dx, dz) || 1;
    const stepX = (dx / dist) * speed * dt;
    const stepZ = (dz / dist) * speed * dt;

    const collision = this.world.collision;
    if (dist > 0.05) {
      if (!collision.isBlocked(this.x + stepX, this.z, this.radius, GROUND_LAYER, GameConfig.world.bounds)) {
        this.x += stepX;
      }
      if (!collision.isBlocked(this.x, this.z + stepZ, this.radius, GROUND_LAYER, GameConfig.world.bounds)) {
        this.z += stepZ;
      }
      const desiredFacing = Math.atan2(dx, dz);
      this.facing = lerpAngle(this.facing, desiredFacing, clamp(dt * 8, 0, 1));
    }
    this._lastSpeedRatio = clamp(Math.hypot(stepX, stepZ) / Math.max(0.0001, speed * dt), 0, 1);
  }

  _sync(dt) {
    this.mesh.position.set(this.x, 0, this.z);
    this.mesh.rotation.y = this.facing;
    const moving = this._lastSpeedRatio > 0.05;
    this.character.setState(this.state === HunterState.CHASE ? 'run' : moving ? 'walk' : 'idle');
    this.character.update(dt, this._lastSpeedRatio);
  }

  get isChasing() {
    return this.state === HunterState.CHASE;
  }
}
