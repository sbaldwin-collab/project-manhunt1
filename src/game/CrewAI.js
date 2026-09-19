import { GameConfig } from '../config/gameConfig.js';
import { createCharacter } from './CharacterFactory.js';
import { lerpAngle, clamp, distance2D } from '../utils.js';
import { GROUND_LAYER } from '../world/CollisionSystem.js';

export class CrewAI {
  constructor(world, index) {
    this.world = world;
    this.index = index;
    this.character = createCharacter('crew', index);
    this.mesh = this.character.root;

    this.x = 0;
    this.z = 0;
    this.facing = 0;
    this.radius = GameConfig.crew.radius;
    this.caught = false;
    this.crouched = false; // crew members never crouch; kept for the HunterAI candidate interface
    this.roofLayer = null; // crew never leave street level in this slice
    this.targetX = 0;
    this.targetZ = 0;
    this.lastMoveMagnitude = 0;
  }

  spawn(x, z) {
    this.x = x;
    this.z = z;
    this.caught = false;
    this.mesh.visible = true;
    this.mesh.position.set(x, 0, z);
    this._pickWanderTarget();
  }

  _pickWanderTarget() {
    const p = this.world.randomSpawnPoint(this.radius, 0);
    this.targetX = p.x;
    this.targetZ = p.z;
  }

  markRescued(x, z) {
    this.caught = false;
    this.mesh.visible = true;
    this.x = x;
    this.z = z;
    this.mesh.position.set(x, 0, z);
    this.character.playOneShot('rescue', 800);
    this._pickWanderTarget();
  }

  update(dt, now, hunter, detection) {
    if (this.caught) return;

    const cfg = GameConfig.crew;
    const dHunter = distance2D(this.x, this.z, hunter.x, hunter.z);
    const hunterVisible =
      dHunter < cfg.fleeTriggerDistance &&
      detection.canSee({ x: hunter.x, z: hunter.z }, { x: this.x, z: this.z }, { range: cfg.fleeTriggerDistance, fovDegrees: 359, layer: GROUND_LAYER });

    let speed;
    let moving = true;
    if (hunterVisible) {
      const vx = (this.x - hunter.x) / (dHunter || 1);
      const vz = (this.z - hunter.z) / (dHunter || 1);
      moving = this._tryMove(vx * cfg.fleeSpeed * dt, vz * cfg.fleeSpeed * dt);
      speed = cfg.fleeSpeed;
      if (moving) this.facing = lerpAngle(this.facing, Math.atan2(vx, vz), clamp(dt * 10, 0, 1));
    } else {
      if (distance2D(this.x, this.z, this.targetX, this.targetZ) < 1.1) this._pickWanderTarget();
      const dx = this.targetX - this.x;
      const dz = this.targetZ - this.z;
      const d = Math.hypot(dx, dz) || 1;
      moving = this._tryMove((dx / d) * cfg.wanderSpeed * dt, (dz / d) * cfg.wanderSpeed * dt);
      speed = cfg.wanderSpeed;
      if (moving) this.facing = lerpAngle(this.facing, Math.atan2(dx, dz), clamp(dt * 6, 0, 1));
    }

    this.lastMoveMagnitude = moving ? 1 : 0;
    this.mesh.position.set(this.x, 0, this.z);
    this.mesh.rotation.y = this.facing;
    this.character.setState(hunterVisible ? 'run' : moving ? 'walk' : 'idle');
    this.character.update(dt, this.lastMoveMagnitude);
  }

  _tryMove(dx, dz) {
    const collision = this.world.collision;
    let moved = false;
    if (!collision.isBlocked(this.x + dx, this.z, this.radius, GROUND_LAYER, GameConfig.world.bounds)) {
      this.x += dx;
      moved = true;
    }
    if (!collision.isBlocked(this.x, this.z + dz, this.radius, GROUND_LAYER, GameConfig.world.bounds)) {
      this.z += dz;
      moved = true;
    }
    return moved;
  }
}
