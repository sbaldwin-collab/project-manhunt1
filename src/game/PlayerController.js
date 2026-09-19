import { GameConfig } from '../config/gameConfig.js';
import { createCharacter } from './CharacterFactory.js';
import { clamp, lerpAngle, distance2D } from '../utils.js';
import { GROUND_LAYER } from '../world/CollisionSystem.js';

const CLIMB_DURATION = 1.5;

export class PlayerController {
  constructor(world, input) {
    this.world = world;
    this.input = input;
    this.character = createCharacter('player');
    this.mesh = this.character.root;

    this.x = 0;
    this.z = 0;
    this.y = 0; // elevation (0 = street, roofY once on a rooftop)
    this.facing = 0;
    this.radius = GameConfig.player.radius;
    this.roofLayer = null; // null on the street, otherwise the roof's collider layer id
    this.caught = false;
    this.crouched = false;
    this.sprinting = false;
    this.stamina = GameConfig.player.stamina.max;
    this.climbing = null;
    this.nearClimb = null; // access point currently in range, for the HUD prompt
    this.lastMoveMagnitude = 0;
    this._climbLockUntil = 0; // prevents an immediate up/down oscillation right at the ladder
  }

  spawn(x, z) {
    this.x = x;
    this.z = z;
    this.y = 0;
    this.roofLayer = null;
    this.caught = false;
    this.climbing = null;
    this._climbLockUntil = 0;
    this.stamina = GameConfig.player.stamina.max;
    this.mesh.position.set(x, 0, z);
  }

  get layer() {
    return this.roofLayer ?? GROUND_LAYER;
  }

  markCaught() {
    this.caught = true;
    this.character.playOneShot('capture', 700);
  }

  update(dt, now, { camYaw, detection }) {
    if (this.caught) {
      this.character.update(dt, 0);
      return;
    }

    if (this.climbing) {
      this._updateClimb(dt);
      this.character.update(dt, 0);
      return;
    }

    this.crouched = this.input.crouchActive;
    this.character.setCrouch(this.crouched);

    const move = this.input.getMoveVector();
    const wantsSprint = this.input.isSprintRequested();
    const sprint = wantsSprint && this.stamina > GameConfig.player.stamina.minToSprint && !this.crouched && move.magnitude > 0.05;
    this.sprinting = sprint;

    const speed = this.crouched
      ? GameConfig.player.crouchSpeed
      : sprint
        ? GameConfig.player.sprintSpeed
        : GameConfig.player.walkSpeed;

    const fx = -Math.sin(camYaw);
    const fz = -Math.cos(camYaw);
    const rx = Math.cos(camYaw);
    const rz = -Math.sin(camYaw);
    const dx = rx * move.x + fx * -move.z;
    const dz = rz * move.x + fz * -move.z;

    let moving = false;
    if (move.magnitude > 0.07) {
      moving = this._tryMove(dx * speed * dt, dz * speed * dt);
      if (moving) {
        const desiredFacing = Math.atan2(dx, dz);
        this.facing = lerpAngle(this.facing, desiredFacing, clamp(dt * GameConfig.player.turnLerp, 0, 1));
      }
    }
    this.lastMoveMagnitude = moving ? move.magnitude : 0;

    const st = GameConfig.player.stamina;
    if (sprint) {
      this.stamina = Math.max(0, this.stamina - st.sprintDrainPerSec * dt);
      if (Math.random() < GameConfig.player.sprintNoiseChancePerSec * dt) {
        detection.registerNoise(this.x, this.z, now, 0.6);
      }
    } else {
      const regen = move.magnitude > 0.07 ? st.regenPerSec : st.idleRegenPerSec;
      this.stamina = Math.min(st.max, this.stamina + regen * dt);
    }

    this._checkClimbAccess(now);

    this.mesh.position.set(this.x, this.y, this.z);
    this.mesh.rotation.y = this.facing;
    const locomotion = this.crouched ? 'crouch' : sprint ? 'run' : moving ? 'walk' : 'idle';
    this.character.setState(locomotion);
    this.character.update(dt, this.lastMoveMagnitude);
  }

  _tryMove(dx, dz) {
    const collision = this.world.collision;
    const layer = this.layer;
    let moved = false;
    if (!collision.isBlocked(this.x + dx, this.z, this.radius, layer, GameConfig.world.bounds)) {
      this.x += dx;
      moved = true;
    }
    if (!collision.isBlocked(this.x, this.z + dz, this.radius, layer, GameConfig.world.bounds)) {
      this.z += dz;
      moved = true;
    }
    return moved;
  }

  _checkClimbAccess(now) {
    this.nearClimb = null;
    if (now < this._climbLockUntil) return;
    for (const access of this.world.roofAccess) {
      if (this.roofLayer === null) {
        if (distance2D(this.x, this.z, access.ground.x, access.ground.z) < 1.0) {
          this.nearClimb = { access, dir: 'up' };
          this._startClimb(access, 'up', now);
          return;
        }
      } else if (this.roofLayer === access.roofId) {
        if (distance2D(this.x, this.z, access.roof.x, access.roof.z) < 1.0) {
          this.nearClimb = { access, dir: 'down' };
          this._startClimb(access, 'down', now);
          return;
        }
      }
    }
  }

  _startClimb(access, dir, now) {
    this.climbing = { access, dir, t: 0 };
    this.character.setState('climb');
    this._climbLockUntil = now + CLIMB_DURATION * 1000 + 900;
  }

  _updateClimb(dt) {
    const c = this.climbing;
    c.t += dt / CLIMB_DURATION;
    const t = Math.min(1, c.t);
    const ease = t < 0.5 ? 2 * t * t : 1 - Math.pow(-2 * t + 2, 2) / 2;
    const from = c.dir === 'up' ? c.access.ground : c.access.roof;
    const to = c.dir === 'up' ? c.access.roof : c.access.ground;
    this.x = from.x + (to.x - from.x) * ease;
    this.z = from.z + (to.z - from.z) * ease;
    const fromY = c.dir === 'up' ? 0 : c.access.roofY;
    const toY = c.dir === 'up' ? c.access.roofY : 0;
    this.y = fromY + (toY - fromY) * ease;
    this.mesh.position.set(this.x, this.y, this.z);

    if (t >= 1) {
      this.roofLayer = c.dir === 'up' ? c.access.roofId : null;
      this.climbing = null;
    }
  }

  throwDecoy(detection, now) {
    const dist = 8;
    const px = this.x - Math.sin(this.facing) * dist;
    const pz = this.z - Math.cos(this.facing) * dist;
    detection.registerNoise(px, pz, now, 1.4);
    return { x: px, z: pz };
  }
}
