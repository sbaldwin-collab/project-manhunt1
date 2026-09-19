// Axis-aligned collision + line-of-sight queries shared by movement, the
// Hunter's vision cone and the third-person camera. Colliders are tagged
// with a "layer" (ground level vs a specific rooftop id) so line-of-sight
// and movement never leak between the street and a rooftop.

export const GROUND_LAYER = 'ground';

export class CollisionSystem {
  constructor() {
    /** @type {{minX:number,maxX:number,minZ:number,maxZ:number,layer:string}[]} */
    this.boxes = [];
    /** @type {import('three').Object3D[]} */
    this.cameraMeshes = [];
  }

  addBox(x, z, w, d, layer = GROUND_LAYER) {
    const box = {
      minX: x - w / 2,
      maxX: x + w / 2,
      minZ: z - d / 2,
      maxZ: z + d / 2,
      layer,
    };
    this.boxes.push(box);
    return box;
  }

  addCameraOccluder(mesh) {
    this.cameraMeshes.push(mesh);
  }

  boxesForLayer(layer) {
    return this.boxes.filter((b) => b.layer === layer);
  }

  isBlocked(x, z, radius, layer = GROUND_LAYER, bounds = 58) {
    if (Math.abs(x) > bounds || Math.abs(z) > bounds) return true;
    for (const box of this.boxes) {
      if (box.layer !== layer) continue;
      if (
        x + radius > box.minX &&
        x - radius < box.maxX &&
        z + radius > box.minZ &&
        z - radius < box.maxZ
      ) {
        return true;
      }
    }
    return false;
  }

  /** Liang-Barsky segment/AABB intersection, true if the segment a->b hits any box on the layer. */
  segmentBlocked(ax, az, bx, bz, layer = GROUND_LAYER) {
    const dx = bx - ax;
    const dz = bz - az;
    for (const box of this.boxes) {
      if (box.layer !== layer) continue;
      let t0 = 0;
      let t1 = 1;
      let accept = true;
      const clips = [
        [-dx, ax - box.minX],
        [dx, box.maxX - ax],
        [-dz, az - box.minZ],
        [dz, box.maxZ - az],
      ];
      for (const [p, q] of clips) {
        if (p === 0) {
          if (q < 0) {
            accept = false;
            break;
          }
        } else {
          const r = q / p;
          if (p < 0) {
            if (r > t1) {
              accept = false;
              break;
            } else if (r > t0) t0 = r;
          } else {
            if (r < t0) {
              accept = false;
              break;
            } else if (r < t1) t1 = r;
          }
        }
      }
      if (accept && t0 <= t1) return true;
    }
    return false;
  }

  hasLineOfSight(ax, az, bx, bz, layer = GROUND_LAYER) {
    return !this.segmentBlocked(ax, az, bx, bz, layer);
  }
}
