import { clamp } from '../utils.js';
import { GameConfig } from '../config/gameConfig.js';

/**
 * Single source of truth for player input: keyboard, the on-screen touch
 * joystick, the touch look-zone, and desktop right-drag mouse look. HUD
 * button elements (crouch/sprint/decoy/rescue/pause) call the trigger*()
 * methods directly so keyboard and touch always funnel through the same
 * state instead of duplicating logic.
 */
export class InputManager {
  constructor() {
    this.keys = {};
    this.joy = { x: 0, y: 0 };
    this.lookYaw = 0;
    this.lookPitch = 0.24;
    this.sprintHeld = false;
    this.crouchActive = false;

    this._flags = { decoy: false, rescue: false, pause: false, interact: false };

    this._joyState = { active: false, id: null, cx: 0, cy: 0 };
    this._lookState = { active: false, id: null, x: 0, y: 0 };
    this._mouseLook = false;

    this._bindKeyboard();
  }

  _bindKeyboard() {
    addEventListener('keydown', (e) => {
      const k = e.key.toLowerCase();
      const repeat = !!this.keys[k];
      this.keys[k] = true;
      if (repeat) return;
      if (k === 'escape') this._flags.pause = true;
      if (k === 'c') this.crouchActive = !this.crouchActive;
      if (k === 'f') this._flags.decoy = true;
      if (k === 'e') {
        this._flags.rescue = true;
        this._flags.interact = true;
      }
    });
    addEventListener('keyup', (e) => {
      this.keys[e.key.toLowerCase()] = false;
    });
  }

  bindTouchZones({ joyZone, joyNub, joyBase, lookZone }) {
    if (joyZone) {
      const moveKnob = (x, y) => {
        const s = this._joyState;
        let dx = x - s.cx;
        let dy = y - s.cy;
        const max = 36;
        const d = Math.hypot(dx, dy);
        if (d > max) {
          dx = (dx / d) * max;
          dy = (dy / d) * max;
        }
        this.joy.x = dx / max;
        this.joy.y = dy / max;
        if (joyNub) joyNub.style.transform = `translate(${dx}px,${dy}px)`;
      };
      joyZone.addEventListener('pointerdown', (e) => {
        this._joyState.active = true;
        this._joyState.id = e.pointerId;
        this._joyState.cx = e.clientX;
        this._joyState.cy = e.clientY;
        if (joyBase) {
          joyBase.style.left = `${e.clientX - 50}px`;
          joyBase.style.top = `${e.clientY - 50}px`;
          joyBase.style.display = 'block';
        }
        joyZone.setPointerCapture(e.pointerId);
        moveKnob(e.clientX, e.clientY);
      });
      joyZone.addEventListener('pointermove', (e) => {
        if (this._joyState.active && e.pointerId === this._joyState.id) moveKnob(e.clientX, e.clientY);
      });
      const endJoy = (e) => {
        if (e.pointerId !== this._joyState.id) return;
        this._joyState.active = false;
        this.joy.x = 0;
        this.joy.y = 0;
        if (joyBase) joyBase.style.display = 'none';
        if (joyNub) joyNub.style.transform = '';
      };
      joyZone.addEventListener('pointerup', endJoy);
      joyZone.addEventListener('pointercancel', endJoy);
    }

    if (lookZone) {
      lookZone.addEventListener('pointerdown', (e) => {
        this._lookState.active = true;
        this._lookState.id = e.pointerId;
        this._lookState.x = e.clientX;
        this._lookState.y = e.clientY;
        lookZone.setPointerCapture(e.pointerId);
      });
      lookZone.addEventListener('pointermove', (e) => {
        if (!this._lookState.active || e.pointerId !== this._lookState.id) return;
        this._applyLook(e.clientX - this._lookState.x, e.clientY - this._lookState.y, GameConfig.camera.lookSensitivityTouch);
        this._lookState.x = e.clientX;
        this._lookState.y = e.clientY;
      });
      const endLook = (e) => {
        if (e.pointerId === this._lookState.id) this._lookState.active = false;
      };
      lookZone.addEventListener('pointerup', endLook);
      lookZone.addEventListener('pointercancel', endLook);
    }

    addEventListener('mousedown', (e) => {
      if (e.button === 2) {
        this._mouseLook = true;
        this._lookState.x = e.clientX;
        this._lookState.y = e.clientY;
      }
    });
    addEventListener('mousemove', (e) => {
      if (!this._mouseLook) return;
      this._applyLook(e.clientX - this._lookState.x, e.clientY - this._lookState.y, GameConfig.camera.lookSensitivityMouse);
      this._lookState.x = e.clientX;
      this._lookState.y = e.clientY;
    });
    addEventListener('mouseup', (e) => {
      if (e.button === 2) this._mouseLook = false;
    });
    addEventListener('contextmenu', (e) => e.preventDefault());
  }

  _applyLook(dx, dy, sensitivity) {
    this.lookYaw -= dx * sensitivity;
    this.lookPitch = clamp(this.lookPitch - dy * sensitivity * 0.7, 0.04, 0.64);
  }

  getMoveVector() {
    const k = this.keys;
    let mx = (k.d || k.arrowright ? 1 : 0) - (k.a || k.arrowleft ? 1 : 0) + this.joy.x;
    let mz = (k.s || k.arrowdown ? 1 : 0) - (k.w || k.arrowup ? 1 : 0) + this.joy.y;
    const len = Math.hypot(mx, mz);
    if (len > 1) {
      mx /= len;
      mz /= len;
    }
    return { x: mx, z: mz, magnitude: Math.min(1, len) };
  }

  isSprintRequested() {
    return this.keys.shift || this.sprintHeld;
  }

  consumeFlag(name) {
    if (this._flags[name]) {
      this._flags[name] = false;
      return true;
    }
    return false;
  }

  setSprintHeld(v) {
    this.sprintHeld = v;
  }

  triggerCrouchToggle() {
    this.crouchActive = !this.crouchActive;
  }

  triggerDecoy() {
    this._flags.decoy = true;
  }

  triggerRescue() {
    this._flags.rescue = true;
  }

  triggerPause() {
    this._flags.pause = true;
  }
}
