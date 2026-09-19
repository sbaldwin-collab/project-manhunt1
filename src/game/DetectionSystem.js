import { clamp } from '../utils.js';

/**
 * Pure vision/hearing queries shared by the Hunter AI and the HUD alert
 * meter. Kept separate from HunterAI so the "can X see Y" math is testable
 * and reusable (e.g. crew members use the same FOV+LOS check to decide
 * whether to flee).
 */
export class DetectionSystem {
  constructor(collision) {
    this.collision = collision;
    this.noisePings = [];
  }

  registerNoise(x, z, now, strength = 1) {
    this.noisePings.push({ x, z, t: now, strength });
    if (this.noisePings.length > 24) this.noisePings.shift();
  }

  pruneNoise(now, maxAgeMs = 5200) {
    this.noisePings = this.noisePings.filter((p) => now - p.t < maxAgeMs);
  }

  latestNoiseWithin(x, z, radius) {
    for (let i = this.noisePings.length - 1; i >= 0; i--) {
      const p = this.noisePings[i];
      if (Math.hypot(p.x - x, p.z - z) <= radius) return p;
    }
    return null;
  }

  /**
   * observer/target: { x, z, y=0, facingRad? }
   * opts: { range, fovDegrees=360, layer='ground' }
   */
  canSee(observer, target, opts) {
    const { range, fovDegrees = 360, layer = 'ground' } = opts;
    const dx = target.x - observer.x;
    const dz = target.z - observer.z;
    const dist = Math.hypot(dx, dz);
    if (dist > range) return false;

    const oy = observer.y ?? 0;
    const ty = target.y ?? 0;
    if (Math.abs(oy - ty) > 3.2) return false;

    if (fovDegrees < 359) {
      const fx = Math.sin(observer.facingRad ?? 0);
      const fz = Math.cos(observer.facingRad ?? 0);
      const dot = dist > 0.001 ? (fx * dx + fz * dz) / dist : 1;
      const cosHalf = Math.cos(((fovDegrees / 2) * Math.PI) / 180);
      if (dot < cosHalf) return false;
    }

    return this.collision.hasLineOfSight(observer.x, observer.z, target.x, target.z, layer);
  }

  /** Smoothly raises/lowers a 0-100 alert meter toward a target visibility state. */
  updateMeter(current, { visible, dt, riseRate, decayRate, floor = 0 }) {
    const next = visible ? current + riseRate * dt : current - decayRate * dt;
    return clamp(Math.max(next, floor), 0, 100);
  }
}
