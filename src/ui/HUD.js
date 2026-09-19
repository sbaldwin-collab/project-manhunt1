function fmt(seconds) {
  const s = Math.max(0, Math.ceil(seconds));
  return `${String(Math.floor(s / 60)).padStart(2, '0')}:${String(s % 60).padStart(2, '0')}`;
}

const $ = (id) => document.getElementById(id);

/** Thin controller over the static HUD markup in index.html — no DOM is built here, only bound and updated. */
export class HUD {
  constructor(input) {
    this.input = input;
    this.el = {
      overlay: $('overlay'),
      ovTitle: $('ovTitle'),
      ovText: $('ovText'),
      stats: $('stats'),
      sTime: $('statTime'),
      sRescue: $('statRescue'),
      sBest: $('statBest'),
      startBtn: $('startBtn'),
      loadWrap: $('loadWrap'),
      loadStatus: $('loadStatus'),
      prog: $('prog'),
      timer: $('timerValue'),
      stamina: $('staminaFill'),
      detect: $('detectFill'),
      status: $('statusValue'),
      crew: $('crewValue'),
      prompt: $('rescuePrompt'),
      feed: $('feed'),
      chaseVignette: $('chaseVignette'),
      pauseBtn: $('pauseBtn'),
      crouchBtn: $('crouchBtn'),
      sprintBtn: $('sprintBtn'),
      decoyBtn: $('decoyBtn'),
      rescueBtn: $('rescueBtn'),
      joyZone: $('joyZone'),
      joyBase: $('joyBase'),
      joyNub: $('joyNub'),
      lookZone: $('lookZone'),
    };
    this._bindButtons();
  }

  _bindButtons() {
    const { crouchBtn, decoyBtn, rescueBtn, pauseBtn, sprintBtn } = this.el;
    crouchBtn.addEventListener('click', () => {
      this.input.triggerCrouchToggle();
      crouchBtn.classList.toggle('active', this.input.crouchActive);
    });
    decoyBtn.addEventListener('click', () => this.input.triggerDecoy());
    rescueBtn.addEventListener('click', () => this.input.triggerRescue());
    pauseBtn.addEventListener('click', () => this.input.triggerPause());

    sprintBtn.addEventListener('pointerdown', (e) => {
      e.preventDefault();
      this.input.setSprintHeld(true);
      sprintBtn.classList.add('active');
    });
    ['pointerup', 'pointercancel', 'pointerleave'].forEach((ev) =>
      sprintBtn.addEventListener(ev, () => {
        this.input.setSprintHeld(false);
        sprintBtn.classList.remove('active');
      }),
    );
  }

  touchZoneElements() {
    return { joyZone: this.el.joyZone, joyNub: this.el.joyNub, joyBase: this.el.joyBase, lookZone: this.el.lookZone };
  }

  onStartClick(cb) {
    this.el.startBtn.addEventListener('click', cb);
  }

  setLoading(pct, text) {
    this.el.prog.style.width = `${pct}%`;
    if (text) this.el.loadStatus.textContent = text;
  }

  revealStart() {
    this.el.startBtn.style.display = 'inline-block';
  }

  log(text, cls = '') {
    const d = document.createElement('div');
    d.className = `feed-line ${cls}`.trim();
    d.textContent = text;
    this.el.feed.prepend(d);
    while (this.el.feed.children.length > 6) this.el.feed.lastChild.remove();
  }

  clearLog() {
    this.el.feed.innerHTML = '';
  }

  hideOverlay() {
    this.el.overlay.style.display = 'none';
  }

  showPaused() {
    this.el.overlay.style.display = 'flex';
    this.el.ovTitle.innerHTML = 'PAUSED';
    this.el.ovText.textContent = 'The block is frozen. Resume when ready.';
    this.el.stats.style.display = 'none';
    this.el.loadWrap.style.display = 'none';
    this.el.startBtn.textContent = 'RESUME';
  }

  showIntro() {
    this.el.overlay.style.display = 'flex';
    this.el.ovTitle.innerHTML = 'PROJECT<br>MANHUNT';
    this.el.ovText.textContent =
      '2:07 AM. Four blocks. One Hunter. Keep moving, use cover, create distractions, and get your crew out of the lockup before time runs out.';
    this.el.stats.style.display = 'none';
    this.el.loadWrap.style.display = 'block';
    this.el.startBtn.textContent = 'ENTER THE BLOCK';
  }

  showResult(win, survivedSeconds, rescues, bestSeconds) {
    this.el.overlay.style.display = 'flex';
    this.el.ovTitle.innerHTML = win ? 'BLOCK<br>CLEARED' : 'YOU<br>WERE CAUGHT';
    this.el.ovText.textContent = win
      ? 'You survived the round. The crew lives to run another block.'
      : 'The Hunter closed the distance. Break line of sight earlier and use the alleys.';
    this.el.stats.style.display = 'flex';
    this.el.loadWrap.style.display = 'none';
    this.el.sTime.textContent = fmt(survivedSeconds);
    this.el.sRescue.textContent = rescues;
    this.el.sBest.textContent = fmt(bestSeconds);
    this.el.startBtn.textContent = 'RUN IT AGAIN';
  }

  playCinematicLetterbox(durationMs) {
    document.body.classList.add('cinematic');
    setTimeout(() => document.body.classList.remove('cinematic'), durationMs);
  }

  update({ timeRemaining, stamina, detection, chasing, freeCount, jailedCount, rescuePromptVisible }) {
    this.el.timer.textContent = fmt(timeRemaining);
    this.el.stamina.style.width = `${stamina}%`;
    this.el.detect.style.width = `${detection}%`;
    this.el.status.textContent = chasing ? 'CHASE' : detection > 40 ? 'ALERT' : 'HIDDEN';
    this.el.status.style.color = chasing ? '#ff737c' : detection > 40 ? '#f0c167' : '#77e2a5';
    this.el.crew.textContent = `${freeCount} / ${jailedCount}`;
    this.el.prompt.style.display = rescuePromptVisible ? 'block' : 'none';
    this.el.chaseVignette.style.opacity = chasing ? 0.88 : Math.max(0, detection / 250);
  }
}
