// Small shared math helpers used across gameplay, AI and camera code.

export function clamp(v, min, max) {
  return Math.max(min, Math.min(max, v));
}

export function lerp(a, b, t) {
  return a + (b - a) * t;
}

/** Frame-rate independent exponential smoothing factor for lerp(). */
export function damp(dt, halfLifeSeconds) {
  return 1 - Math.pow(2, -dt / Math.max(1e-4, halfLifeSeconds));
}

function wrapAngle(a) {
  while (a > Math.PI) a -= Math.PI * 2;
  while (a < -Math.PI) a += Math.PI * 2;
  return a;
}

export function lerpAngle(a, b, t) {
  const diff = wrapAngle(b - a);
  return a + diff * t;
}

export function distance2D(ax, az, bx, bz) {
  return Math.hypot(ax - bx, az - bz);
}

export function randRange(min, max) {
  return min + Math.random() * (max - min);
}
