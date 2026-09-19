import * as THREE from 'three';
import { GameConfig } from '../config/gameConfig.js';
import { CollisionSystem, GROUND_LAYER } from './CollisionSystem.js';
import { CityBuilder } from './NYCBlock.js';
import { WindowInstancer } from './WindowInstancer.js';
import { asphaltTexture, sidewalkTexture } from './textures.js';

const STREET_HALF = 5.5;

function placeRow(builder, { axis, fixed, outward, dir, start, defs }) {
  let cursor = start;
  const refs = [];
  for (const d of defs) {
    const centerOffset = cursor + dir * (d.len / 2);
    let x, z, w, depth;
    if (axis === 'x') {
      x = centerOffset;
      z = fixed + outward * (d.depth / 2);
      w = d.len;
      depth = d.depth;
    } else {
      z = centerOffset;
      x = fixed + outward * (d.depth / 2);
      w = d.depth;
      depth = d.len;
    }
    const built = builder.building(x, z, w, depth, d.height, {
      color: d.color,
      roofAccess: !!d.roofAccess,
    });
    const wallRotY =
      axis === 'x' ? (outward > 0 ? Math.PI : 0) : outward > 0 ? -Math.PI / 2 : Math.PI / 2;
    // The street-facing wall sits at the row's fixed coordinate, not the building's center.
    const faceX = axis === 'x' ? x : fixed;
    const faceZ = axis === 'x' ? fixed : z;
    refs.push({ ...built, def: d, axis, fixed, outward, faceX, faceZ, wallRotY });
    cursor += dir * (d.len + (d.gap ?? 2.4));
  }
  return refs;
}

export class World {
  constructor(scene, { mobile = false } = {}) {
    this.scene = scene;
    this.mobile = mobile;
    this.collision = new CollisionSystem();
    this.windowInstancer = new WindowInstancer(scene, 1000);
    this.builder = new CityBuilder(scene, this.collision, this.windowInstancer);
    this.roofAccess = [];
    this._rain = null;
    this._rainCount = mobile ? 500 : 950;

    this._lighting();
    this._ground();
    this._blocks();
    this._jailPlaza();
    this._rainSystem();

    this.windowInstancer.finalize();
  }

  _lighting() {
    this.scene.background = new THREE.Color(0x02050a);
    this.scene.fog = new THREE.FogExp2(GameConfig.rendering.fogColor, GameConfig.rendering.fogDensity);

    this.scene.add(new THREE.HemisphereLight(GameConfig.colors.ambientSky, GameConfig.colors.ambientGround, 0.62));

    const moon = new THREE.DirectionalLight(GameConfig.colors.moon, 1.05);
    moon.position.set(-26, 38, -16);
    moon.castShadow = true;
    const smSize = GameConfig.rendering.shadowMapSize;
    moon.shadow.mapSize.set(smSize, smSize);
    const b = GameConfig.world.bounds * 0.9;
    Object.assign(moon.shadow.camera, { left: -b, right: b, top: b, bottom: -b, far: 140 });
    moon.shadow.bias = -0.0018;
    this.scene.add(moon);
    this.moon = moon;
  }

  _ground() {
    const bounds = GameConfig.world.bounds;
    const size = bounds * 2 + 20;

    const sidewalkMat = new THREE.MeshStandardMaterial({
      map: sidewalkTexture(),
      roughness: 0.95,
      metalness: 0.02,
    });
    const ground = new THREE.Mesh(new THREE.PlaneGeometry(size, size), sidewalkMat);
    ground.rotation.x = -Math.PI / 2;
    ground.receiveShadow = true;
    this.scene.add(ground);

    const asphaltMat = new THREE.MeshStandardMaterial({
      map: asphaltTexture(),
      roughness: 0.6,
      metalness: 0.08,
    });

    const avenue = new THREE.Mesh(new THREE.PlaneGeometry(STREET_HALF * 2, size), asphaltMat);
    avenue.rotation.x = -Math.PI / 2;
    avenue.position.y = 0.01;
    avenue.receiveShadow = true;
    this.scene.add(avenue);

    const crossStreet = new THREE.Mesh(new THREE.PlaneGeometry(size, STREET_HALF * 2), asphaltMat);
    crossStreet.rotation.x = -Math.PI / 2;
    crossStreet.position.y = 0.01;
    crossStreet.receiveShadow = true;
    this.scene.add(crossStreet);

    // Intersection crosswalks
    for (let x = -8; x <= 8; x += 2.3) this.builder.roadMark(x, -8.5, 1.4, 0.2);
    for (let x = -8; x <= 8; x += 2.3) this.builder.roadMark(x, 8.5, 1.4, 0.2);
    for (let z = -8; z <= 8; z += 2.3) this.builder.roadMark(-8.5, z, 0.2, 1.4);
    for (let z = -8; z <= 8; z += 2.3) this.builder.roadMark(8.5, z, 0.2, 1.4);

    // Yellow lane dashes along the avenue
    for (let z = -bounds; z <= bounds; z += 4.5) {
      if (Math.abs(z) < 10) continue;
      this.builder.roadMark(0, z, 0.22, 1.6);
    }

    // Manholes and puddles scattered near the crossing
    this.builder.manhole(0, 0);
    this.builder.manhole(-2.5, 14);
    this.builder.puddle(3, -12, 1.6);
    this.builder.puddle(-16, 3, 1.3);
    this.builder.puddle(18, 22, 1.5);
    this.builder.puddle(-22, -18, 1.4);
  }

  _blocks() {
    const quadrants = [
      { ox: -1, oz: -1, startOffset: 3.2 },
      { ox: 1, oz: -1, startOffset: 3.2 },
      { ox: -1, oz: 1, startOffset: 3.2 },
      { ox: 1, oz: 1, startOffset: 9.5 }, // reserved corner for jail plaza
    ];

    const layouts = [
      {
        // NW
        rowA: [
          { len: 12, depth: 14, height: 17, color: '#4a3d3a', roofAccess: true, gap: 1.8 },
          { len: 10, depth: 12, height: 11, color: '#3f4750', gap: 4.2, storefront: { label: 'DELI', kind: 'deli' } },
          { len: 9, depth: 13, height: 20, color: '#5a3b32' },
        ],
        rowB: [
          { len: 11, depth: 13, height: 13, color: '#463a44', gap: 2.0 },
          { len: 9, depth: 12, height: 10, color: '#5c4a2e' },
        ],
      },
      {
        // NE
        rowA: [
          { len: 10, depth: 12, height: 12, color: '#3f4750', gap: 1.7 },
          { len: 12, depth: 14, height: 18, color: '#4a3d3a' },
        ],
        rowB: [
          { len: 9, depth: 12, height: 10, color: '#5c4a2e', gap: 3.8, storefront: { label: 'LAUNDROMAT', kind: 'laundromat' } },
          { len: 11, depth: 13, height: 15, color: '#463a44', gap: 1.8 },
          { len: 8, depth: 12, height: 11, color: '#3f4750' },
        ],
      },
      {
        // SW
        rowA: [
          { len: 9, depth: 12, height: 11, color: '#5a3b32', gap: 1.8, roofAccess: true },
          { len: 12, depth: 13, height: 16, color: '#463a44', gap: 3.6, storefront: { label: 'BODEGA', kind: 'deli' } },
        ],
        rowB: [
          { len: 10, depth: 12, height: 12, color: '#3f4750', gap: 2.0 },
          { len: 9, depth: 13, height: 19, color: '#5a3b32' },
        ],
      },
      {
        // SE (buildings start further from corner to leave jail plaza room)
        rowA: [
          { len: 11, depth: 13, height: 14, color: '#463a44', gap: 1.8 },
          { len: 9, depth: 12, height: 10, color: '#5c4a2e' },
        ],
        rowB: [
          { len: 10, depth: 12, height: 12, color: '#3f4750', gap: 1.8 },
          { len: 11, depth: 13, height: 16, color: '#4a3d3a' },
        ],
      },
    ];

    this.blockRows = [];

    const SIDEWALK_DEPTH = 3.4;

    quadrants.forEach((q, i) => {
      const layout = layouts[i];
      const cornerX = q.ox * STREET_HALF;
      const cornerZ = q.oz * STREET_HALF;

      const rowA = placeRow(this.builder, {
        axis: 'x',
        fixed: cornerZ + q.oz * SIDEWALK_DEPTH,
        outward: q.oz,
        dir: q.ox,
        start: cornerX + q.ox * q.startOffset,
        defs: layout.rowA,
      });
      const rowB = placeRow(this.builder, {
        axis: 'z',
        fixed: cornerX + q.ox * SIDEWALK_DEPTH,
        outward: q.ox,
        dir: q.oz,
        start: cornerZ + q.oz * q.startOffset,
        defs: layout.rowB,
      });

      for (const ref of [...rowA, ...rowB]) {
        if (ref.def.storefront) {
          this.builder.storefront(ref.faceX, ref.faceZ, Math.min(ref.def.len, 7), ref.wallRotY, ref.def.storefront.label, ref.def.storefront.kind);
        }
        if (ref.def.roofAccess && ref.axis === 'x') {
          const wallOut = -ref.outward;
          this.builder.fireEscape(ref.faceX - ref.def.len * 0.22, ref.faceZ, 5.2, Math.max(2, Math.floor(ref.def.height / 3)), wallOut);
          this.roofAccess.push({
            roofId: ref.id,
            ground: { x: ref.faceX - ref.def.len * 0.22, z: ref.faceZ - ref.outward * 1.1 },
            roof: { x: ref.faceX - ref.def.len * 0.22, z: ref.faceZ + ref.outward * 1.1 },
            roofY: ref.h,
          });
        }
      }

      this._streetProps(q, cornerX, cornerZ);
      this.blockRows.push({ q, rowA, rowB });
    });

    // Fenced vacant lot tucked in the NW block interior
    this._fencedLot(-38, -38, 12, 10);

    // Rear-alley clutter in the other three block interiors
    const ne = this._quadrantInterior(quadrants[1]);
    this.builder.dumpster(ne.x, ne.z, 0.2);
    this.builder.trashBag(ne.x + 1.6, ne.z - 1.2, 0.45);

    const sw = this._quadrantInterior(quadrants[2]);
    this.builder.dumpster(sw.x, sw.z, -0.15);
    this.builder.trashBag(sw.x - 1.5, sw.z + 1.3, 0.4);

    const se = this._quadrantInterior(quadrants[3]);
    this.builder.dumpster(se.x, se.z, 0.3);
    this.builder.trashBag(se.x + 1.4, se.z + 1.4, 0.42);

    // Scaffolding in the alley behind an SE building
    this.builder.scaffolding(20, 22.7, 6, 6);

    // Graffiti tags in a couple of alley gaps
    this.builder.graffiti(-15.2, -9, 3.2, 1.8, Math.PI / 2, 3);
    this.builder.graffiti(11.8, 20, 3, 1.6, -Math.PI / 2, 9);
  }

  _fencedLot(cx, cz, w, d) {
    this.builder.chainLinkFence(cx - w / 2, cz, d, Math.PI / 2);
    this.builder.chainLinkFence(cx + w / 2, cz, d, Math.PI / 2);
    this.builder.chainLinkFence(cx, cz - d / 2, w, 0);
    this.builder.chainLinkFence(cx, cz + d / 2, w, 0);
    this.builder.trashBag(cx - 2, cz - 2, 0.45);
    this.builder.trashBag(cx + 1.5, cz + 1, 0.4);
    this.builder.dumpster(cx, cz, 0.3);
  }

  _streetProps(q, cornerX, cornerZ) {
    const lampA = { x: cornerX + q.ox * 15, z: cornerZ + q.oz * 2.3 };
    const lampB = { x: cornerX + q.ox * 2.3, z: cornerZ + q.oz * 15 };
    this.builder.streetLamp(lampA.x, lampA.z);
    this.builder.streetLamp(lampB.x, lampB.z);

    this.builder.hydrant(cornerX + q.ox * 8, cornerZ + q.oz * 1.8);
    this.builder.car(cornerX + q.ox * 22, cornerZ + q.oz * 2.6, [0x26384e, 0x4b2d32, 0x313840, 0x2e4030][Math.floor(Math.random() * 4)], q.oz === 0 ? 0 : Math.PI / 2 * q.oz);
    this.builder.car(cornerX + q.ox * 9.5, cornerZ + q.oz * 2.4, [0x26384e, 0x4b2d32, 0x313840, 0x2e4030][Math.floor(Math.random() * 4)], q.oz === 0 ? 0 : Math.PI / 2 * q.oz);
  }

  /** Open ground far from any building row — used for rear-alley clutter (dumpsters, trash bags, lots). */
  _quadrantInterior(q) {
    return { x: q.ox * 42, z: q.oz * 42 };
  }

  _jailPlaza() {
    const { x, z, radius } = GameConfig.world.jail;
    const floor = new THREE.Mesh(
      new THREE.CircleGeometry(radius, 36),
      new THREE.MeshStandardMaterial({ color: 0x211a0b, roughness: 0.75, metalness: 0.04 }),
    );
    floor.rotation.x = -Math.PI / 2;
    floor.position.set(x, 0.025, z);
    floor.receiveShadow = true;
    this.scene.add(floor);

    for (let i = 0; i < 20; i++) {
      const a = (i / 20) * Math.PI * 2;
      if (a > 0.25 && a < 0.75) continue; // gap = entrance
      const bar = new THREE.Mesh(
        new THREE.CylinderGeometry(0.045, 0.045, 2.8, 7),
        new THREE.MeshStandardMaterial({ color: 0x565a61, roughness: 0.35, metalness: 0.75 }),
      );
      bar.position.set(x + Math.cos(a) * radius, 1.4, z + Math.sin(a) * radius);
      bar.castShadow = true;
      this.scene.add(bar);
    }

    const jailLight = new THREE.PointLight(0xffb34d, 16, 12, 2);
    jailLight.position.set(x, 3.7, z);
    this.scene.add(jailLight);
  }

  _rainSystem() {
    const geo = new THREE.BufferGeometry();
    const count = this._rainCount;
    const positions = new Float32Array(count * 3);
    const bounds = GameConfig.world.bounds;
    for (let i = 0; i < count; i++) {
      positions[i * 3] = (Math.random() * 2 - 1) * bounds;
      positions[i * 3 + 1] = Math.random() * 24 + 2;
      positions[i * 3 + 2] = (Math.random() * 2 - 1) * bounds;
    }
    geo.setAttribute('position', new THREE.BufferAttribute(positions, 3));
    this._rain = new THREE.Points(
      geo,
      new THREE.PointsMaterial({ color: 0xa7c6dc, size: 0.035, transparent: true, opacity: 0.5 }),
    );
    this.scene.add(this._rain);
  }

  update(dt) {
    const arr = this._rain.geometry.attributes.position.array;
    const bounds = GameConfig.world.bounds;
    for (let i = 0; i < this._rainCount; i++) {
      arr[i * 3 + 1] -= dt * 15;
      arr[i * 3] -= dt * 2.2;
      if (arr[i * 3 + 1] < 0.1) {
        arr[i * 3 + 1] = Math.random() * 20 + 10;
        arr[i * 3] = (Math.random() * 2 - 1) * bounds;
        arr[i * 3 + 2] = (Math.random() * 2 - 1) * bounds;
      }
    }
    this._rain.geometry.attributes.position.needsUpdate = true;
  }

  get roofs() {
    return this.builder.roofs;
  }

  get cameraOccluders() {
    return this.collision.cameraMeshes;
  }

  randomSpawnPoint(radius = 0.5, minDistFromJail = 7) {
    const bounds = GameConfig.world.bounds - 4;
    const jail = GameConfig.world.jail;
    for (let i = 0; i < 400; i++) {
      const x = (Math.random() * 2 - 1) * bounds;
      const z = (Math.random() * 2 - 1) * bounds;
      if (this.collision.isBlocked(x, z, radius, GROUND_LAYER, GameConfig.world.bounds)) continue;
      if (Math.hypot(x - jail.x, z - jail.z) < minDistFromJail) continue;
      return { x, z };
    }
    return { x: 15, z: 15 };
  }
}
