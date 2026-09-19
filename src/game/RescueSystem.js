import { GameConfig } from '../config/gameConfig.js';
import { distance2D } from '../utils.js';

/** Owns the jail: capturing free-roaming crew, arranging them behind bars, and releasing them on rescue. */
export class RescueSystem {
  constructor(world, crew, hunter, audio, onLog) {
    this.world = world;
    this.crew = crew;
    this.hunter = hunter;
    this.audio = audio;
    this.onLog = onLog;
    this.rescuesThisRound = 0;
  }

  reset() {
    this.rescuesThisRound = 0;
  }

  captureIfTouching() {
    const captureDist = GameConfig.hunter.captureDistance;
    let captured = false;
    for (const c of this.crew) {
      if (c.caught) continue;
      if (distance2D(c.x, c.z, this.hunter.x, this.hunter.z) < captureDist) {
        c.caught = true;
        c.character.playOneShot('capture', 700);
        this.hunter.notifyCaptureEvent();
        this.audio?.play('capture');
        this.onLog?.('Crew member taken to the lockup.', 'bad');
        captured = true;
      }
    }
    if (captured) this._layoutJail();
  }

  _layoutJail() {
    const jailed = this.crew.filter((c) => c.caught);
    const jail = GameConfig.world.jail;
    jailed.forEach((c, i) => {
      const ang = (i / Math.max(1, jailed.length)) * Math.PI * 2;
      c.x = jail.x + Math.cos(ang) * 1.3;
      c.z = jail.z + Math.sin(ang) * 1.3;
      c.mesh.position.set(c.x, 0, c.z);
      c.mesh.visible = true;
      c.character.setState('idle');
    });
  }

  isPlayerInRange(player) {
    const jail = GameConfig.world.jail;
    return distance2D(player.x, player.z, jail.x, jail.z) < jail.radius + 1.6;
  }

  jailedCount() {
    return this.crew.filter((c) => c.caught).length;
  }

  freeCount() {
    return this.crew.length - this.jailedCount();
  }

  tryRescue(player) {
    if (!this.isPlayerInRange(player)) return false;
    const jailed = this.crew.filter((c) => c.caught);
    if (!jailed.length) return false;
    const jail = GameConfig.world.jail;
    jailed.forEach((c, i) => {
      const ang = (i * Math.PI * 2) / Math.max(1, jailed.length);
      c.markRescued(jail.x + Math.cos(ang) * (jail.radius + 1.4), jail.z + Math.sin(ang) * (jail.radius + 1.4));
    });
    this.rescuesThisRound += jailed.length;
    this.audio?.play('rescue');
    this.onLog?.('LOCKUP OPEN — crew released.', 'good');
    return true;
  }
}
