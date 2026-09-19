import * as THREE from 'three';
import {
  brickTexture,
  fenceTexture,
  signTexture,
  graffitiTexture,
} from './textures.js';
import { GROUND_LAYER } from './CollisionSystem.js';

const BRICK_PALETTE = ['#5a3b32', '#4a3d3a', '#3f4750', '#5c4a2e', '#463a44'];

function material(color, roughness = 0.85, metalness = 0.04, extra = {}) {
  return new THREE.MeshStandardMaterial({ color, roughness, metalness, ...extra });
}

/**
 * CityBuilder is a toolkit of reusable NYC prop/building factories. It knows
 * nothing about the overall city layout — World.js decides where things go
 * and calls these methods. Every method that creates a walkable obstruction
 * registers an AABB with the shared CollisionSystem so movement, the
 * Hunter's line-of-sight and the camera's collision raycast all agree on
 * where solid geometry is.
 */
export class CityBuilder {
  constructor(scene, collision, windowInstancer) {
    this.scene = scene;
    this.collision = collision;
    this.windows = windowInstancer;
    this.roofs = [];
    this.roofAccessPoints = [];
    this._buildingId = 0;
  }

  _add(mesh, { shadow = true, occluder = true } = {}) {
    if (shadow) {
      mesh.castShadow = true;
      mesh.receiveShadow = true;
    }
    this.scene.add(mesh);
    if (occluder) this.collision.addCameraOccluder(mesh);
    return mesh;
  }

  /** A full building: brick box, punched window grid, optional rooftop detail set. */
  building(x, z, w, d, h, opts = {}) {
    const id = `roof:${this._buildingId++}`;
    const colorHex = opts.color || BRICK_PALETTE[Math.floor(Math.random() * BRICK_PALETTE.length)];
    const brick = brickTexture(colorHex);
    const mat = material(0xffffff, 0.88, 0.03, { map: brick });
    const box = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), mat);
    box.position.set(x, h / 2, z);
    this._add(box);

    this.collision.addBox(x, z, w, d, GROUND_LAYER);

    this._facadeWindows(x, z, w, d, h);

    const roofAccess = !!opts.roofAccess;
    const building = { id, x, z, w, d, h, roofAccess };
    if (roofAccess) {
      this.roofs.push({ id, minX: x - w / 2, maxX: x + w / 2, minZ: z - d / 2, maxZ: z + d / 2, y: h });
      this._roofDetails(building);
    }
    return building;
  }

  _facadeWindows(x, z, w, d, h) {
    const rows = Math.max(2, Math.floor((h - 1.6) / 2.3));
    const colsX = Math.max(2, Math.floor(w / 2.6));
    const colsZ = Math.max(2, Math.floor(d / 2.6));
    const faces = [
      { axis: 'x', count: colsX, span: w, normalZ: 1, fixedZ: z + d / 2 + 0.02, rotY: 0 },
      { axis: 'x', count: colsX, span: w, normalZ: -1, fixedZ: z - d / 2 - 0.02, rotY: Math.PI },
      { axis: 'z', count: colsZ, span: d, normalX: 1, fixedX: x + w / 2 + 0.02, rotY: -Math.PI / 2 },
      { axis: 'z', count: colsZ, span: d, normalX: -1, fixedX: x - w / 2 - 0.02, rotY: Math.PI / 2 },
    ];
    for (const f of faces) {
      const step = f.span / (f.count + 1);
      for (let r = 0; r < rows; r++) {
        const y = 2.0 + r * 2.3;
        if (y > h - 1) continue;
        for (let c = 0; c < f.count; c++) {
          if (Math.random() < 0.12) continue;
          const lit = Math.random() < 0.42;
          const warm = Math.random() < 0.6;
          if (f.axis === 'x') {
            const wx = x - w / 2 + step * (c + 1);
            this.windows.add(wx, y, f.fixedZ, f.rotY, lit, warm);
          } else {
            const wz = z - d / 2 + step * (c + 1);
            this.windows.add(f.fixedX, y, wz, f.rotY, lit, warm);
          }
        }
      }
    }
  }

  _roofDetails(b) {
    const parapet = material(0x2c2f34, 0.8, 0.05);
    const top = material(0x22262b, 0.95, 0.02);
    const cap = this._add(
      new THREE.Mesh(new THREE.BoxGeometry(b.w, 0.25, b.d), top),
      { occluder: false },
    );
    cap.position.set(b.x, b.h + 0.12, b.z);

    const wallH = 0.9;
    const segs = [
      [b.x, b.h + wallH / 2, b.z - b.d / 2, b.w, wallH, 0.15],
      [b.x, b.h + wallH / 2, b.z + b.d / 2, b.w, wallH, 0.15],
      [b.x - b.w / 2, b.h + wallH / 2, b.z, 0.15, wallH, b.d],
      [b.x + b.w / 2, b.h + wallH / 2, b.z, 0.15, wallH, b.d],
    ];
    for (const [sx, sy, sz, sw, sh, sd] of segs) {
      const wall = new THREE.Mesh(new THREE.BoxGeometry(sw, sh, sd), parapet);
      wall.position.set(sx, sy, sz);
      this._add(wall, { occluder: false });
      this.collision.addBox(sx, sz, sw + 0.3, sd + 0.3, b.id);
    }

    // Water tank
    const tankMat = material(0x3b2a1c, 0.75, 0.1);
    const tank = new THREE.Group();
    const drum = new THREE.Mesh(new THREE.CylinderGeometry(1.1, 1.1, 1.7, 12), tankMat);
    drum.position.y = b.h + 2.6;
    tank.add(drum);
    const roofCone = new THREE.Mesh(new THREE.ConeGeometry(1.25, 0.8, 12), material(0x2a2018, 0.7, 0.1));
    roofCone.position.y = b.h + 3.85;
    tank.add(roofCone);
    for (const [lx, lz] of [[-0.8, -0.8], [0.8, -0.8], [-0.8, 0.8], [0.8, 0.8]]) {
      const leg = new THREE.Mesh(new THREE.CylinderGeometry(0.05, 0.05, 1.75, 6), material(0x1b1b1b, 0.5, 0.6));
      leg.position.set(lx, b.h + 0.9, lz);
      tank.add(leg);
    }
    tank.position.set(b.x - b.w / 4, 0, b.z - b.d / 4);
    this._add(tank, { occluder: false });
    this.collision.addBox(b.x - b.w / 4, b.z - b.d / 4, 2.4, 2.4, b.id);

    // HVAC unit
    const hvac = new THREE.Mesh(new THREE.BoxGeometry(1.6, 0.9, 1.2), material(0x5a5f66, 0.6, 0.35));
    hvac.position.set(b.x + b.w / 4, b.h + 0.45, b.z + b.d / 4);
    this._add(hvac, { occluder: false });
    this.collision.addBox(b.x + b.w / 4, b.z + b.d / 4, 1.8, 1.4, b.id);

    // Chimney
    const chimney = new THREE.Mesh(new THREE.BoxGeometry(0.6, 1.6, 0.6), material(0x35302c, 0.9, 0.02));
    chimney.position.set(b.x + b.w / 3.2, b.h + 0.8, b.z - b.d / 3.2);
    this._add(chimney, { occluder: false });
  }

  /** Fire escape zig-zagging up a facade; also usable as a rooftop access ladder. */
  fireEscape(x, z, w, levels, faceSign) {
    const metal = material(0x272b2e, 0.46, 0.76);
    for (let i = 0; i < levels; i++) {
      const y = 3.0 + i * 3;
      const platform = new THREE.Mesh(new THREE.BoxGeometry(w, 0.09, 1.15), metal);
      platform.position.set(x, y, z);
      this._add(platform, { occluder: false });
      for (const s of [-1, 1]) {
        const rail = new THREE.Mesh(new THREE.BoxGeometry(w, 0.05, 0.05), metal);
        rail.position.set(x, y + 0.62, z + s * 0.52);
        this._add(rail, { occluder: false });
      }
      if (i < levels - 1) {
        const stair = new THREE.Mesh(new THREE.BoxGeometry(0.12, 3.4, 0.12), metal);
        stair.position.set(x + w * 0.28, y + 1.45, z);
        stair.rotation.z = 0.42 * (faceSign < 0 ? -1 : 1);
        this._add(stair, { occluder: false });
      }
    }
  }

  /** rotY is the outward-facing normal direction of the storefront wall (0 = +Z, PI = -Z, PI/2 = +X, -PI/2 = -X). */
  storefront(x, z, w, rotY, label, kind = 'deli') {
    const nx = Math.sin(rotY);
    const nz = Math.cos(rotY);
    const glassMat = new THREE.MeshStandardMaterial({
      color: 0x9fd0e6,
      roughness: 0.1,
      metalness: 0.2,
      emissive: 0x2a4a55,
      emissiveIntensity: kind === 'laundromat' ? 1.1 : 0.55,
      transparent: true,
      opacity: 0.85,
    });

    const glass = new THREE.Mesh(new THREE.PlaneGeometry(w * 0.72, 2.0), glassMat);
    glass.position.set(x + nx * 0.03, 1.35, z + nz * 0.03);
    glass.rotation.y = rotY;
    this._add(glass, { shadow: false, occluder: false });

    // Metal roll-down gate (partially open feel via horizontal slats)
    const gateGroup = new THREE.Group();
    const slatMat = material(0x1c2024, 0.6, 0.5);
    for (let i = 0; i < 6; i++) {
      const slat = new THREE.Mesh(new THREE.BoxGeometry(w * 0.78, 0.09, 0.04), slatMat);
      slat.position.y = 0.35 + i * 0.11;
      gateGroup.add(slat);
    }
    gateGroup.position.set(x + nx * 0.05, 0, z + nz * 0.05);
    gateGroup.rotation.y = rotY;
    gateGroup.visible = kind !== 'open';
    this._add(gateGroup, { shadow: false, occluder: false });

    // Awning
    const awning = new THREE.Mesh(
      new THREE.BoxGeometry(w * 0.85, 0.08, 0.9),
      material(kind === 'laundromat' ? 0x2c6f8f : 0x7a2430, 0.6, 0.05),
    );
    awning.position.set(x + nx * 0.5, 2.55, z + nz * 0.5);
    awning.rotation.y = rotY;
    this._add(awning, { occluder: false });

    // Sign
    const signTex = signTexture(label);
    const sign = new THREE.Mesh(
      new THREE.PlaneGeometry(w * 0.7, 0.55),
      new THREE.MeshBasicMaterial({ map: signTex, toneMapped: false }),
    );
    sign.position.set(x + nx * 0.3, 2.95, z + nz * 0.3);
    sign.rotation.y = rotY;
    this._add(sign, { shadow: false, occluder: false });

    const signLight = new THREE.PointLight(0xffe4b0, 3.2, 5, 2);
    signLight.position.set(x + nx * 0.6, 2.9, z + nz * 0.6);
    this.scene.add(signLight);
  }

  streetLamp(x, z) {
    const pole = new THREE.Mesh(
      new THREE.CylinderGeometry(0.07, 0.1, 5.3, 8),
      material(0x181b20, 0.55, 0.6),
    );
    pole.position.set(x, 2.65, z);
    this._add(pole);

    const bulb = new THREE.Mesh(
      new THREE.SphereGeometry(0.16, 10, 8),
      new THREE.MeshStandardMaterial({ color: 0xffdfb1, emissive: 0xffa84c, emissiveIntensity: 2.2 }),
    );
    bulb.position.set(x, 5.05, z);
    this._add(bulb, { shadow: false, occluder: false });

    const light = new THREE.PointLight(0xffa85f, 14, 13, 2);
    light.position.set(x, 4.9, z);
    this.scene.add(light);
  }

  dumpster(x, z, rot = 0) {
    const g = new THREE.Group();
    const body = new THREE.Mesh(new THREE.BoxGeometry(2.1, 1.25, 1.25), material(0x21443a, 0.72, 0.18));
    body.position.y = 0.66;
    g.add(body);
    const lid = new THREE.Mesh(new THREE.BoxGeometry(2.2, 0.12, 1.28), material(0x163a2f, 0.55, 0.25));
    lid.position.y = 1.34;
    lid.rotation.z = -0.08;
    g.add(lid);
    g.position.set(x, 0, z);
    g.rotation.y = rot;
    this._add(g);
    this.collision.addBox(x, z, 2.3, 1.4, GROUND_LAYER);
  }

  car(x, z, color, rot = 0) {
    const g = new THREE.Group();
    const body = new THREE.Mesh(new THREE.BoxGeometry(3.6, 0.72, 1.68), material(color, 0.4, 0.35));
    body.position.y = 0.62;
    g.add(body);
    const cab = new THREE.Mesh(new THREE.BoxGeometry(1.9, 0.72, 1.5), material(0x19232d, 0.2, 0.13));
    cab.position.set(0.25, 1.27, 0);
    g.add(cab);
    for (const sx of [-1.18, 1.18]) {
      for (const sz of [-0.82, 0.82]) {
        const wheel = new THREE.Mesh(new THREE.CylinderGeometry(0.32, 0.32, 0.18, 12), material(0x08090b, 0.95, 0.03));
        wheel.rotation.x = Math.PI / 2;
        wheel.position.set(sx, 0.35, sz);
        g.add(wheel);
      }
    }
    g.position.set(x, 0, z);
    g.rotation.y = rot;
    this._add(g);
    this.collision.addBox(x, z, rot === 0 ? 3.7 : 1.8, rot === 0 ? 1.8 : 3.7, GROUND_LAYER);
  }

  hydrant(x, z) {
    const g = new THREE.Group();
    const red = material(0x6d1c20, 0.66, 0.3);
    const body = new THREE.Mesh(new THREE.CylinderGeometry(0.25, 0.3, 0.8, 10), red);
    body.position.y = 0.4;
    g.add(body);
    const cap = new THREE.Mesh(new THREE.SphereGeometry(0.27, 10, 7), red);
    cap.position.y = 0.82;
    g.add(cap);
    g.position.set(x, 0, z);
    this._add(g);
  }

  trashBag(x, z, s = 0.5) {
    const bag = new THREE.Mesh(new THREE.IcosahedronGeometry(s, 1), material(0x0a0c0e, 0.9, 0.05));
    bag.position.set(x, s * 0.6, z);
    bag.scale.y = 1.2;
    this._add(bag);
  }

  manhole(x, z) {
    const disc = new THREE.Mesh(new THREE.CylinderGeometry(0.55, 0.55, 0.03, 16), material(0x14161a, 0.7, 0.4));
    disc.position.set(x, 0.016, z);
    this._add(disc, { shadow: false });
  }

  puddle(x, z, r = 1.4) {
    const mat = new THREE.MeshStandardMaterial({
      color: 0x0c1420,
      roughness: 0.05,
      metalness: 0.65,
      transparent: true,
      opacity: 0.75,
    });
    const p = new THREE.Mesh(new THREE.CircleGeometry(r, 20), mat);
    p.rotation.x = -Math.PI / 2;
    p.position.set(x, 0.014, z);
    this._add(p, { shadow: false, occluder: false });
  }

  graffiti(x, z, w, h, rotY, seed) {
    const tex = graffitiTexture(seed);
    const g = new THREE.Mesh(
      new THREE.PlaneGeometry(w, h),
      new THREE.MeshBasicMaterial({ map: tex, transparent: true, toneMapped: false }),
    );
    g.position.set(x, h / 2 + 0.3, z);
    g.rotation.y = rotY;
    this._add(g, { shadow: false, occluder: false });
  }

  scaffolding(x, z, w, h, rotY = 0) {
    const g = new THREE.Group();
    const pipe = material(0x8a6a2c, 0.5, 0.6);
    const poles = [
      [-w / 2, -0.5],
      [w / 2, -0.5],
      [-w / 2, 0.5],
      [w / 2, 0.5],
    ];
    for (const [px, pz] of poles) {
      const pole = new THREE.Mesh(new THREE.CylinderGeometry(0.06, 0.06, h, 6), pipe);
      pole.position.set(px, h / 2, pz);
      g.add(pole);
    }
    for (let level = 0; level < Math.floor(h / 2); level++) {
      const y = 1.2 + level * 2;
      const plank = new THREE.Mesh(new THREE.BoxGeometry(w, 0.08, 1), material(0x8a6432, 0.85, 0.02));
      plank.position.set(0, y, 0);
      g.add(plank);
      for (const side of [-1, 1]) {
        const rail = new THREE.Mesh(new THREE.BoxGeometry(w, 0.04, 0.04), pipe);
        rail.position.set(0, y + 0.5, side * 0.5);
        g.add(rail);
      }
    }
    g.position.set(x, 0, z);
    g.rotation.y = rotY;
    this._add(g);
    const cw = rotY === 0 ? w + 0.4 : 1.4;
    const cd = rotY === 0 ? 1.4 : w + 0.4;
    this.collision.addBox(x, z, cw, cd, GROUND_LAYER);
  }

  chainLinkFence(x, z, length, rotY, height = 1.9) {
    const tex = fenceTexture();
    tex.repeat.set(length / 1.6, height / 1.6);
    const mat = new THREE.MeshStandardMaterial({
      map: tex,
      transparent: true,
      alphaTest: 0.2,
      color: 0xaab2bb,
      roughness: 0.6,
      metalness: 0.3,
      side: THREE.DoubleSide,
    });
    const fence = new THREE.Mesh(new THREE.PlaneGeometry(length, height), mat);
    fence.position.set(x, height / 2, z);
    fence.rotation.y = rotY;
    this._add(fence, { shadow: false });
    const w = rotY === 0 ? length : 0.15;
    const d = rotY === 0 ? 0.15 : length;
    this.collision.addBox(x, z, w, d, GROUND_LAYER);
  }

  roadMark(x, z, w, d, rotY = 0) {
    const mark = new THREE.Mesh(
      new THREE.PlaneGeometry(w, d),
      new THREE.MeshBasicMaterial({ color: 0xd6d3bb, transparent: true, opacity: 0.4 }),
    );
    mark.rotation.x = -Math.PI / 2;
    mark.rotation.z = rotY;
    mark.position.set(x, 0.012, z);
    this._add(mark, { shadow: false, occluder: false });
  }
}
