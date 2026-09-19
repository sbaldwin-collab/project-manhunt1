import * as THREE from 'three';
import { GameConfig } from '../config/gameConfig.js';
import { clamp } from '../utils.js';

/** Over-the-shoulder follow camera with wall collision, sprint FOV widen, shake and a cinematic round-intro sweep. */
export class ThirdPersonCamera {
  constructor(camera, world) {
    this.camera = camera;
    this.world = world;
    this.shake = 0;
    this.cinematicUntil = 0;
    this._cinematicDuration = 1;
    this._cinematicYaw = 0;
    this._raycaster = new THREE.Raycaster();
  }

  startCinematic(durationMs, yaw) {
    this.cinematicUntil = performance.now() + durationMs;
    this._cinematicDuration = durationMs;
    this._cinematicYaw = yaw;
  }

  addShake(amount) {
    this.shake = Math.max(this.shake, amount);
  }

  update(dt, now, { player, input, sprinting }) {
    if (!player) return;
    const cfg = GameConfig.camera;

    if (now < this.cinematicUntil) {
      this._updateCinematic(now, player);
      return;
    }

    const yaw = input.lookYaw;
    const pitch = input.lookPitch;
    const c = Math.cos(yaw);
    const s = Math.sin(yaw);

    const focus = new THREE.Vector3(player.x, player.y + 1.35, player.z);
    const desired = new THREE.Vector3(
      player.x + s * cfg.distance + cfg.shoulderOffset * c,
      player.y + cfg.heightBase + Math.sin(pitch) * cfg.pitchHeightRange,
      player.z + c * cfg.distance - cfg.shoulderOffset * s,
    );

    const allowedDist = this._collide(focus, desired);
    const dir = desired.clone().sub(focus).normalize();
    const finalPos = focus.clone().add(dir.multiplyScalar(allowedDist));

    if (this.shake > 0) {
      this.shake = Math.max(0, this.shake - dt);
      finalPos.x += (Math.random() - 0.5) * this.shake;
      finalPos.y += (Math.random() - 0.5) * this.shake * 0.5;
    }

    const lerpT = 1 - Math.pow(cfg.followLerpBase, dt);
    this.camera.position.lerp(finalPos, lerpT);

    const targetFov = sprinting ? cfg.fovSprint : cfg.fovBase;
    this.camera.fov += (targetFov - this.camera.fov) * (1 - Math.pow(0.01, dt));
    this.camera.updateProjectionMatrix();
    this.camera.lookAt(focus);
  }

  _updateCinematic(now, player) {
    const t = clamp(1 - (this.cinematicUntil - now) / this._cinematicDuration, 0, 1);
    const ang = this._cinematicYaw + (1 - t) * 0.9;
    const pos = new THREE.Vector3(player.x + Math.sin(ang) * 7.5, player.y + 3.4, player.z + Math.cos(ang) * 7.5);
    this.camera.position.lerp(pos, 0.06);
    this.camera.lookAt(player.x, player.y + 1.25, player.z);
  }

  _collide(focus, desired) {
    const dir = desired.clone().sub(focus);
    const dist = dir.length();
    if (dist < 0.001) return dist;
    dir.normalize();
    this._raycaster.set(focus, dir);
    this._raycaster.near = 0.1;
    this._raycaster.far = dist;
    const hits = this._raycaster.intersectObjects(this.world.cameraOccluders, false);
    return hits.length ? Math.max(0.8, hits[0].distance - GameConfig.camera.collisionPadding) : dist;
  }
}
