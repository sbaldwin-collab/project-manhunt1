import { GameConfig } from '../config/gameConfig.js';

/**
 * All game audio is synthesized at runtime with the Web Audio API so the
 * vertical slice ships with zero binary audio assets. Every cue name in
 * GameConfig.audio.assets can instead point at a real file; wiring that up
 * only requires teaching play()/_startLoop() to prefer a decoded
 * AudioBuffer over the procedural path below — nothing in the rest of the
 * game references synthesis directly, it only calls play()/setX() by name.
 */
export class AudioManager {
  constructor() {
    this.ctx = null;
    this.master = null;
    this._loops = {};
    this._heartbeatTimer = 0;
    this._nextAmbientEventAt = 0;
  }

  init() {
    if (this.ctx) return;
    const Ctx = window.AudioContext || window.webkitAudioContext;
    if (!Ctx) return;
    this.ctx = new Ctx();
    this.master = this.ctx.createGain();
    this.master.gain.value = GameConfig.audio.masterGain;
    this.master.connect(this.ctx.destination);

    this._startLoop('rain', 'rain');
    this._startLoop('cityAmbience', 'ambience');
    this._startLoop('chase', 'chase');
    this._nextAmbientEventAt = performance.now() + 6000;
  }

  resume() {
    if (this.ctx && this.ctx.state === 'suspended') this.ctx.resume();
  }

  setMasterVolume(v) {
    if (this.master) this.master.gain.value = v;
  }

  _noiseBuffer(dur) {
    const n = Math.max(1, Math.floor(this.ctx.sampleRate * dur));
    const buf = this.ctx.createBuffer(1, n, this.ctx.sampleRate);
    const d = buf.getChannelData(0);
    for (let i = 0; i < n; i++) d[i] = Math.random() * 2 - 1;
    return buf;
  }

  _tone(freq, dur, gain = 0.05, type = 'sine') {
    if (!this.ctx) return;
    const o = this.ctx.createOscillator();
    const g = this.ctx.createGain();
    o.type = type;
    const t0 = this.ctx.currentTime;
    o.frequency.setValueAtTime(freq, t0);
    g.gain.setValueAtTime(gain, t0);
    g.gain.exponentialRampToValueAtTime(0.0001, t0 + dur);
    o.connect(g).connect(this.master);
    o.start(t0);
    o.stop(t0 + dur + 0.02);
  }

  _noiseBurst(dur = 0.08, gain = 0.05, freq = 1200) {
    if (!this.ctx) return;
    const src = this.ctx.createBufferSource();
    src.buffer = this._noiseBuffer(dur);
    const filt = this.ctx.createBiquadFilter();
    filt.type = 'lowpass';
    filt.frequency.value = freq;
    const g = this.ctx.createGain();
    const t0 = this.ctx.currentTime;
    g.gain.setValueAtTime(gain, t0);
    g.gain.exponentialRampToValueAtTime(0.0001, t0 + dur);
    src.connect(filt).connect(g).connect(this.master);
    src.start(t0);
  }

  _startLoop(name, type) {
    if (!this.ctx || this._loops[name]) return;
    const src = this.ctx.createBufferSource();
    src.buffer = this._noiseBuffer(2);
    src.loop = true;
    const filt = this.ctx.createBiquadFilter();
    const g = this.ctx.createGain();
    if (type === 'rain') {
      filt.type = 'highpass';
      filt.frequency.value = 1700;
      g.gain.value = 0.045;
    } else if (type === 'ambience') {
      filt.type = 'lowpass';
      filt.frequency.value = 350;
      g.gain.value = 0.03;
    } else if (type === 'chase') {
      filt.type = 'bandpass';
      filt.frequency.value = 210;
      filt.Q.value = 0.9;
      g.gain.value = 0;
    }
    src.connect(filt).connect(g).connect(this.master);
    src.start();
    this._loops[name] = { src, filt, gain: g };
  }

  play(name) {
    if (!this.ctx) return;
    switch (name) {
      case 'footstep':
        this._noiseBurst(0.045, 0.03, 1500);
        break;
      case 'interact':
        this._tone(520, 0.08, 0.05, 'sine');
        break;
      case 'decoy':
        this._noiseBurst(0.12, 0.05, 900);
        break;
      case 'capture':
        this._tone(140, 0.24, 0.07, 'sawtooth');
        break;
      case 'rescue':
        this._tone(420, 0.16, 0.055, 'sine');
        setTimeout(() => this._tone(660, 0.16, 0.05, 'sine'), 110);
        break;
      case 'siren':
        this._siren();
        break;
      case 'subway':
        this._subwayRumble();
        break;
      case 'win':
        this._tone(392, 0.2, 0.06, 'sine');
        setTimeout(() => this._tone(523, 0.28, 0.06, 'sine'), 160);
        break;
      case 'lose':
        this._tone(196, 0.35, 0.07, 'sawtooth');
        break;
      default:
        break;
    }
  }

  _siren() {
    if (!this.ctx) return;
    const dur = 2.4;
    const o = this.ctx.createOscillator();
    const g = this.ctx.createGain();
    o.type = 'sine';
    const t0 = this.ctx.currentTime;
    [500, 780, 500, 780, 500].forEach((f, i) => o.frequency.linearRampToValueAtTime(f, t0 + i * 0.6));
    g.gain.setValueAtTime(0.0001, t0);
    g.gain.linearRampToValueAtTime(0.024, t0 + 0.3);
    g.gain.linearRampToValueAtTime(0.0001, t0 + dur);
    o.connect(g).connect(this.master);
    o.start(t0);
    o.stop(t0 + dur + 0.05);
  }

  _subwayRumble() {
    if (!this.ctx) return;
    const dur = 2.6;
    const src = this.ctx.createBufferSource();
    src.buffer = this._noiseBuffer(dur);
    const filt = this.ctx.createBiquadFilter();
    filt.type = 'lowpass';
    filt.frequency.value = 110;
    const g = this.ctx.createGain();
    const t0 = this.ctx.currentTime;
    g.gain.setValueAtTime(0.0001, t0);
    g.gain.linearRampToValueAtTime(0.045, t0 + 0.8);
    g.gain.linearRampToValueAtTime(0.0001, t0 + dur);
    src.connect(filt).connect(g).connect(this.master);
    src.start(t0);
  }

  setChasing(active) {
    const loop = this._loops.chase;
    if (loop && this.ctx) loop.gain.gain.setTargetAtTime(active ? 0.045 : 0, this.ctx.currentTime, 0.4);
  }

  /** intensity: 0..1, typically driven by the detection meter. now: performance.now() ms. */
  setHeartbeatIntensity(intensity, now) {
    if (!this.ctx || intensity <= 0.02) return;
    const interval = 680 - intensity * 380;
    if (now - this._heartbeatTimer > interval) {
      this._tone(56, 0.11, 0.04 + intensity * 0.05, 'sine');
      setTimeout(() => this._tone(52, 0.09, 0.025 + intensity * 0.03, 'sine'), 120);
      this._heartbeatTimer = now;
    }
  }

  /** Call once per frame with performance.now(); occasionally layers a siren or subway rumble for city atmosphere. */
  update(now) {
    if (!this.ctx) return;
    if (now > this._nextAmbientEventAt) {
      this._nextAmbientEventAt = now + 9000 + Math.random() * 15000;
      this.play(Math.random() < 0.55 ? 'siren' : 'subway');
    }
  }
}
