// Small procedural texture library. The vertical slice ships without a
// texture-artist pass, so every surface below is generated on a <canvas> at
// runtime. Swapping any of these for a authored PBR texture set later only
// requires changing the loader call in NYCBlock.js — nothing else in the
// codebase references canvas generation directly.

import * as THREE from 'three';

function canvas(w, h) {
  const c = document.createElement('canvas');
  c.width = w;
  c.height = h;
  return c;
}

function tex(c, repeatX = 1, repeatY = 1) {
  const t = new THREE.CanvasTexture(c);
  t.wrapS = THREE.RepeatWrapping;
  t.wrapT = THREE.RepeatWrapping;
  t.repeat.set(repeatX, repeatY);
  t.anisotropy = 4;
  return t;
}

export function brickTexture(baseColor = '#5a3b32', mortar = '#2c2320') {
  const c = canvas(128, 128);
  const ctx = c.getContext('2d');
  ctx.fillStyle = mortar;
  ctx.fillRect(0, 0, 128, 128);
  const rows = 8;
  const rh = 128 / rows;
  for (let r = 0; r < rows; r++) {
    const offset = r % 2 === 0 ? 0 : 16;
    for (let x = -16; x < 128; x += 32) {
      const shade = 0.85 + Math.random() * 0.3;
      ctx.fillStyle = shadeColor(baseColor, shade);
      ctx.fillRect(x + offset + 1, r * rh + 1, 30, rh - 2);
    }
  }
  return tex(c, 3, 3);
}

function shadeColor(hex, mult) {
  const c = new THREE.Color(hex);
  c.multiplyScalar(mult);
  return `#${c.getHexString()}`;
}

export function asphaltTexture() {
  const c = canvas(256, 256);
  const ctx = c.getContext('2d');
  ctx.fillStyle = '#15171a';
  ctx.fillRect(0, 0, 256, 256);
  for (let i = 0; i < 2200; i++) {
    const x = Math.random() * 256;
    const y = Math.random() * 256;
    const g = 10 + Math.random() * 22;
    ctx.fillStyle = `rgba(${g + 8},${g + 8},${g + 10},${Math.random() * 0.4})`;
    ctx.fillRect(x, y, 1.4, 1.4);
  }
  // a few cracks
  ctx.strokeStyle = 'rgba(0,0,0,0.4)';
  ctx.lineWidth = 1;
  for (let i = 0; i < 6; i++) {
    ctx.beginPath();
    let x = Math.random() * 256;
    let y = Math.random() * 256;
    ctx.moveTo(x, y);
    for (let s = 0; s < 6; s++) {
      x += (Math.random() - 0.5) * 40;
      y += (Math.random() - 0.5) * 40;
      ctx.lineTo(x, y);
    }
    ctx.stroke();
  }
  return tex(c, 18, 18);
}

export function sidewalkTexture() {
  const c = canvas(256, 256);
  const ctx = c.getContext('2d');
  ctx.fillStyle = '#54585d';
  ctx.fillRect(0, 0, 256, 256);
  ctx.strokeStyle = 'rgba(0,0,0,0.35)';
  ctx.lineWidth = 2;
  for (let x = 0; x <= 256; x += 64) {
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, 256);
    ctx.stroke();
  }
  for (let i = 0; i < 900; i++) {
    ctx.fillStyle = `rgba(0,0,0,${Math.random() * 0.08})`;
    ctx.fillRect(Math.random() * 256, Math.random() * 256, 2, 2);
  }
  return tex(c, 10, 10);
}

export function fenceTexture() {
  const c = canvas(128, 128);
  const ctx = c.getContext('2d');
  ctx.clearRect(0, 0, 128, 128);
  ctx.strokeStyle = 'rgba(190,196,204,0.85)';
  ctx.lineWidth = 2;
  const step = 16;
  for (let x = -128; x < 256; x += step) {
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x + 128, 128);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(x + 128, 0);
    ctx.lineTo(x, 128);
    ctx.stroke();
  }
  const t = new THREE.CanvasTexture(c);
  t.wrapS = THREE.RepeatWrapping;
  t.wrapT = THREE.RepeatWrapping;
  return t;
}

export function signTexture(text, bg = '#101317', fg = '#e7dcc3') {
  const c = canvas(512, 128);
  const ctx = c.getContext('2d');
  ctx.fillStyle = bg;
  ctx.fillRect(0, 0, 512, 128);
  ctx.fillStyle = fg;
  ctx.font = 'bold 64px Arial Narrow, sans-serif';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.fillText(text.toUpperCase(), 256, 68);
  return new THREE.CanvasTexture(c);
}

export function graffitiTexture(seed = 0) {
  const c = canvas(256, 256);
  const ctx = c.getContext('2d');
  ctx.clearRect(0, 0, 256, 256);
  const palette = ['#e0475e', '#f0c150', '#4fb0e0', '#8be08a', '#e0e0e0'];
  const rand = mulberry32(seed + 1);
  for (let i = 0; i < 4; i++) {
    ctx.strokeStyle = palette[Math.floor(rand() * palette.length)];
    ctx.lineWidth = 6 + rand() * 10;
    ctx.globalAlpha = 0.55 + rand() * 0.35;
    ctx.beginPath();
    let x = rand() * 256;
    let y = 60 + rand() * 140;
    ctx.moveTo(x, y);
    for (let s = 0; s < 5; s++) {
      x += (rand() - 0.5) * 90;
      y += (rand() - 0.5) * 50;
      ctx.lineTo(x, y);
    }
    ctx.stroke();
  }
  ctx.globalAlpha = 1;
  const t = new THREE.CanvasTexture(c);
  t.transparent = true;
  return t;
}

export function windowGridTexture(cols, rows, litRatio = 0.4) {
  const cw = 32;
  const ch = 32;
  const c = canvas(cw * cols, ch * rows);
  const ctx = c.getContext('2d');
  ctx.fillStyle = '#141a22';
  ctx.fillRect(0, 0, c.width, c.height);
  for (let r = 0; r < rows; r++) {
    for (let cx = 0; cx < cols; cx++) {
      const lit = Math.random() < litRatio;
      const warm = Math.random() < 0.55;
      ctx.fillStyle = lit
        ? warm
          ? '#ffcf8a'
          : '#bcd6ff'
        : '#232c37';
      ctx.fillRect(cx * cw + 3, r * ch + 3, cw - 6, ch - 6);
      if (lit) {
        ctx.fillStyle = 'rgba(0,0,0,0.18)';
        ctx.fillRect(cx * cw + 3, r * ch + 3, cw - 6, 4);
      }
    }
  }
  return new THREE.CanvasTexture(c);
}

function mulberry32(a) {
  return function () {
    let t = (a += 0x6d2b79f5);
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

export { mulberry32 };
