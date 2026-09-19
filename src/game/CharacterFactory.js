import * as THREE from 'three';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { GameConfig } from '../config/gameConfig.js';
import { damp } from '../utils.js';

const SKIN_TONES = [0xe0aa83, 0xc98f66, 0x8b5a3c, 0x6b4530];
const EYE_COLOR = 0x1c140f;

function mat(color, roughness = 0.78, metalness = 0.03, extra = {}) {
  return new THREE.MeshStandardMaterial({ color, roughness, metalness, ...extra });
}

/**
 * Procedurally built, rig-animated civilian/streetwear character. This is a
 * deliberate stand-in for a real skinned character model: every mesh hangs
 * off a small hierarchy of pivot Groups (hip -> spine -> head, hip -> legs,
 * spine -> arms) so the locomotion cycle below reads as a believable human
 * gait rather than a static mannequin, and so the same animation driver can
 * later target bones on an imported GLTF rig with minimal changes (see
 * loadExternalCharacter at the bottom of this file).
 */
export class Character {
  constructor(role, opts = {}) {
    this.role = role;
    this.opts = opts;
    this.root = new THREE.Group();
    this.state = 'idle';
    this.phase = Math.random() * 10;
    this.baseHipY = role === 'hunter' ? 0.96 : 0.92;
    this.crouchAmount = 0; // 0 = standing, 1 = fully crouched (animated toward target)
    this.crouchTarget = 0;
    this.oneShot = null;
    this._build();
  }

  _build() {
    const accent = this.opts.accent ?? 0x888888;
    const skin = SKIN_TONES[Math.floor(Math.random() * SKIN_TONES.length)];
    const isHunter = this.role === 'hunter';
    const jacketMat = mat(accent, 0.72, 0.06);
    const pantsMat = mat(isHunter ? 0x14151a : 0x1b2433, 0.85, 0.04);
    const skinMat = mat(skin, 0.55, 0.0);
    const shoeMat = mat(0x0a0c10, 0.75, 0.08);

    this.hip = new THREE.Group();
    this.hip.position.y = this.baseHipY;
    this.root.add(this.hip);

    // Pelvis block bridges the legs into the torso instead of leaving a gap
    // between the leg capsules and the spine's base.
    const pelvis = new THREE.Mesh(
      new RoundedBoxGeometry((isHunter ? 0.66 : 0.58) * 0.78, 0.16, 0.28, 3, 0.045),
      pantsMat,
    );
    pelvis.position.y = 0.08;
    pelvis.castShadow = true;
    pelvis.receiveShadow = true;
    this.hip.add(pelvis);

    this.legL = this._limb(pantsMat, 0.1, 0.82, shoeMat, 'foot');
    this.legR = this._limb(pantsMat, 0.1, 0.82, shoeMat, 'foot');
    this.legL.pivot.position.set(-0.14, 0, 0);
    this.legR.pivot.position.set(0.14, 0, 0);
    this.hip.add(this.legL.pivot, this.legR.pivot);

    this.spine = new THREE.Group();
    this.spine.position.y = 0.02;
    this.hip.add(this.spine);

    const torsoWidth = isHunter ? 0.66 : 0.58;

    // Torso is built from a narrower waist and a wider chest (rounded, not
    // boxy) instead of one flat slab -- the taper is what reads as a human
    // ribcage-to-hip silhouette rather than a crate.
    const waist = new THREE.Mesh(
      new RoundedBoxGeometry(torsoWidth * 0.84, 0.26, 0.29, 3, 0.05),
      pantsMat,
    );
    waist.position.y = 0.17;
    waist.castShadow = true;
    waist.receiveShadow = true;
    this.spine.add(waist);

    const chest = new THREE.Mesh(
      new RoundedBoxGeometry(torsoWidth, 0.34, 0.33, 3, 0.06),
      jacketMat,
    );
    chest.position.y = 0.47;
    chest.castShadow = true;
    chest.receiveShadow = true;
    this.spine.add(chest);
    this.chestY = 0.47;
    this._chestDetail(accent, isHunter);

    // Shoulder caps round out the chest-to-arm join.
    for (const side of [-1, 1]) {
      const shoulder = new THREE.Mesh(new THREE.SphereGeometry(0.1, 12, 10), jacketMat);
      shoulder.position.set(side * torsoWidth * 0.46, 0.6, 0);
      shoulder.castShadow = true;
      this.spine.add(shoulder);
    }

    const neck = new THREE.Mesh(new THREE.CylinderGeometry(0.075, 0.085, 0.11, 10), skinMat);
    neck.position.y = 0.68;
    this.spine.add(neck);

    this.head = new THREE.Group();
    this.head.position.y = 0.81;
    const skull = new THREE.Mesh(new THREE.SphereGeometry(0.145, 22, 18), skinMat);
    skull.scale.set(0.94, 1.08, 1.0);
    skull.castShadow = true;
    this.head.add(skull);
    this._face(skin);
    this._headwear(accent, isHunter);
    this.spine.add(this.head);

    this.armL = this._limb(jacketMat, 0.075, 0.56, skinMat, 'hand');
    this.armR = this._limb(jacketMat, 0.075, 0.56, skinMat, 'hand');
    this.armL.pivot.position.set(-torsoWidth * 0.46, 0.6, 0);
    this.armR.pivot.position.set(torsoWidth * 0.46, 0.6, 0);
    this.spine.add(this.armL.pivot, this.armR.pivot);

    if (isHunter) this._flashlight();

    this.root.scale.setScalar(this.opts.scale ?? 1);
  }

  _face(skin) {
    const eyeMat = mat(EYE_COLOR, 0.25, 0.1);
    for (const side of [-1, 1]) {
      const eye = new THREE.Mesh(new THREE.SphereGeometry(0.016, 8, 8), eyeMat);
      eye.position.set(side * 0.052, 0.02, 0.132);
      this.head.add(eye);
    }
    const nose = new THREE.Mesh(new THREE.ConeGeometry(0.018, 0.045, 8), mat(skin, 0.6, 0));
    nose.rotation.x = Math.PI / 2;
    nose.position.set(0, -0.015, 0.142);
    this.head.add(nose);
  }

  _limb(mainMat, radius, length, tipMat, tipShape = 'default') {
    const pivot = new THREE.Group();
    const mesh = new THREE.Mesh(new THREE.CapsuleGeometry(radius, length * 0.6, 6, 10), mainMat);
    mesh.position.y = -length / 2;
    mesh.castShadow = true;
    pivot.add(mesh);
    const tip = new THREE.Mesh(new THREE.SphereGeometry(radius * 0.95, 10, 8), tipMat);
    tip.position.y = -length;
    if (tipShape === 'hand') {
      tip.scale.set(1.0, 0.72, 1.25);
    } else if (tipShape === 'foot') {
      tip.scale.set(1.2, 0.55, 1.85);
      tip.position.z = 0.045;
    }
    tip.castShadow = true;
    pivot.add(tip);
    return { pivot, mesh };
  }

  _chestDetail(accent, isHunter) {
    const chestY = this.chestY;
    if (isHunter) {
      const vest = new THREE.Mesh(
        new RoundedBoxGeometry(0.5, 0.3, 0.1, 2, 0.03),
        mat(0x14151a, 0.7, 0.15),
      );
      vest.position.set(0, chestY, -0.19);
      this.spine.add(vest);
      const badge = new THREE.Mesh(new THREE.CircleGeometry(0.045, 10), mat(0xd8b34a, 0.4, 0.7));
      badge.position.set(0.15, chestY + 0.1, -0.245);
      this.spine.add(badge);
      return;
    }
    const pack = new THREE.Mesh(new RoundedBoxGeometry(0.36, 0.32, 0.16, 2, 0.035), mat(0x1a2028, 0.82, 0.04));
    pack.position.set(0, chestY, -0.22);
    this.spine.add(pack);
    const stripe = new THREE.Mesh(new THREE.BoxGeometry(0.38, 0.06, 0.04), mat(accent, 0.6, 0.1));
    stripe.position.set(0, chestY + 0.1, -0.25);
    this.spine.add(stripe);
  }

  _headwear(accent, isHunter) {
    const style = this.opts.hat ?? (isHunter ? 'cap' : 'none');
    if (style === 'hood') {
      const hood = new THREE.Mesh(
        new THREE.SphereGeometry(0.175, 12, 8, 0, Math.PI * 2, 0, Math.PI * 0.6),
        mat(accent, 0.8, 0.03),
      );
      hood.position.y = 0.04;
      hood.rotation.x = -0.1;
      this.head.add(hood);
    } else if (style === 'beanie') {
      const beanie = new THREE.Mesh(
        new THREE.SphereGeometry(0.155, 12, 8, 0, Math.PI * 2, 0, Math.PI * 0.55),
        mat(0x2c2c30, 0.85, 0.02),
      );
      beanie.position.y = 0.03;
      this.head.add(beanie);
    } else if (style === 'cap') {
      const capMat = mat(isHunter ? 0x101114 : accent, 0.65, 0.1);
      const crown = new THREE.Mesh(new THREE.SphereGeometry(0.15, 12, 8, 0, Math.PI * 2, 0, Math.PI * 0.5), capMat);
      crown.position.y = 0.045;
      this.head.add(crown);
      const brim = new THREE.Mesh(new THREE.CylinderGeometry(0.16, 0.16, 0.02, 12, 1, false, 0, Math.PI), capMat);
      brim.rotation.x = Math.PI / 2;
      brim.position.set(0, 0.02, -0.13);
      this.head.add(brim);
    }
  }

  _flashlight() {
    const handle = new THREE.Mesh(
      new THREE.CylinderGeometry(0.035, 0.035, 0.22, 8),
      mat(0x1c1d20, 0.4, 0.5),
    );
    handle.rotation.x = Math.PI / 2;
    handle.position.set(0, -0.56, 0.05);
    this.armR.pivot.add(handle);
    this.flashlightTip = new THREE.Object3D();
    this.flashlightTip.position.set(0, -0.56, 0.18);
    this.armR.pivot.add(this.flashlightTip);
  }

  setState(state) {
    this.state = state;
  }

  setCrouch(isCrouched) {
    this.crouchTarget = isCrouched ? 1 : 0;
  }

  /** Plays a short interrupt animation (capture / rescue) then returns to locomotion. */
  playOneShot(name, durationMs) {
    this.oneShot = { name, t: 0, duration: durationMs / 1000 };
  }

  update(dt, moveRatio = 0) {
    this.crouchAmount += (this.crouchTarget - this.crouchAmount) * damp(dt, 0.12);
    this.hip.position.y = this.baseHipY - this.crouchAmount * 0.22;

    if (this.oneShot) {
      this.oneShot.t += dt;
      const t = Math.min(1, this.oneShot.t / this.oneShot.duration);
      this._applyOneShot(this.oneShot.name, t);
      if (t >= 1) this.oneShot = null;
      return;
    }

    this._applyLocomotion(dt, moveRatio);
  }

  _applyLocomotion(dt, moveRatio) {
    const state = this.state;
    let freq = 0;
    let stride = 0;
    let lean = 0;
    if (state === 'walk') {
      freq = 5.2;
      stride = 0.45;
    } else if (state === 'run') {
      freq = 8.4;
      stride = 0.78;
      lean = 0.16;
    } else if (state === 'crouch') {
      freq = 4.4;
      stride = 0.3;
      lean = 0.22;
    } else if (state === 'climb') {
      this._applyClimb(dt);
      return;
    }

    const moving = state === 'walk' || state === 'run' || (state === 'crouch' && moveRatio > 0.05);
    if (moving) {
      this.phase += dt * freq;
    } else {
      this.phase += dt * 1.3;
      stride = 0;
    }

    const legAngleL = Math.sin(this.phase) * stride;
    const legAngleR = -legAngleL;
    this.legL.pivot.rotation.x = legAngleL;
    this.legR.pivot.rotation.x = legAngleR;
    this.armL.pivot.rotation.x = legAngleR * 0.85;
    this.armR.pivot.rotation.x = legAngleL * 0.85;
    this.armL.pivot.rotation.z = 0.06;
    this.armR.pivot.rotation.z = -0.06;

    const bob = moving ? Math.abs(Math.sin(this.phase * 2)) * 0.035 : Math.sin(this.phase) * 0.008;
    this.spine.position.y = 0.02 + bob;
    this.spine.rotation.x = -lean * (moving ? 1 : 0.15);
  }

  _applyClimb(dt) {
    this.phase += dt * 4.2;
    const l = Math.sin(this.phase);
    this.armL.pivot.rotation.x = -1.7 + l * 0.5;
    this.armR.pivot.rotation.x = -1.7 - l * 0.5;
    this.legL.pivot.rotation.x = -0.4 - l * 0.35;
    this.legR.pivot.rotation.x = -0.4 + l * 0.35;
    this.spine.rotation.x = -0.28;
  }

  _applyOneShot(name, t) {
    const ease = 1 - Math.pow(1 - t, 3);
    if (name === 'capture') {
      this.spine.rotation.x = -ease * 1.05;
      this.armL.pivot.rotation.x = -ease * 1.4;
      this.armR.pivot.rotation.x = -ease * 1.4;
      this.armL.pivot.rotation.z = ease * 0.8;
      this.armR.pivot.rotation.z = -ease * 0.8;
      this.hip.position.y = this.baseHipY - ease * 0.32;
    } else if (name === 'rescue') {
      const bounce = Math.sin(t * Math.PI) * 0.12;
      this.armL.pivot.rotation.x = -2.5 * ease;
      this.armR.pivot.rotation.x = -2.5 * ease;
      this.spine.rotation.x = -0.1 * ease;
      this.hip.position.y = this.baseHipY + bounce;
    }
  }
}

export function createCharacter(role, index = 0) {
  const c = GameConfig.colors;
  let accent = c.playerAccent;
  let hat = 'none';
  if (role === 'hunter') {
    accent = c.hunterAccent;
    hat = 'cap';
  } else if (role === 'crew') {
    accent = c.crewAccents[index % c.crewAccents.length];
    hat = ['hood', 'beanie', 'cap'][index % 3];
  } else if (role === 'player') {
    hat = 'beanie';
  }
  return new Character(role, { accent, hat, scale: role === 'hunter' ? 1.03 : 1 });
}

/**
 * Optional hook for dropping in a production character pipeline. When
 * GameConfig.characters.useExternalModels is true this attempts to load a
 * GLTF from the manifest; the vertical slice ships with it disabled because
 * no licensed/authored character assets exist in this repo yet (see
 * README "Current limitations"). Returns null on failure so callers fall
 * back to the procedural Character above.
 */
export async function loadExternalCharacter(role) {
  if (!GameConfig.characters.useExternalModels) return null;
  const url = GameConfig.characters.manifest[role];
  if (!url) return null;
  try {
    const { GLTFLoader } = await import('three/addons/loaders/GLTFLoader.js');
    const loader = new GLTFLoader();
    const gltf = await loader.loadAsync(url);
    return gltf.scene;
  } catch (err) {
    console.warn(`[CharacterFactory] External model for "${role}" failed to load, using procedural character.`, err);
    return null;
  }
}
