// Croquis des variables de gameplay.
// Chaque croquis est une fonction (V, name) -> contenu SVG (viewBox 0 0 320 180),
// ou V contient toutes les valeurs courantes : le dessin suit les reglages en direct.
// Couleurs : classe "hi" = ce que la variable change, "soft" = decor, "acc" = repere secondaire.

(function () {
  const W = 320, H = 180;
  const clamp = (x, a, b) => Math.max(a, Math.min(b, x));
  const f = (x, d = 0) => Number(x).toFixed(d);
  const n = (V, k, d = 0) => (V[k] === undefined || V[k] === null || isNaN(V[k]) ? d : Number(V[k]));
  const lerp = (a, b, t) => a + (b - a) * t;

  // --- primitives ---------------------------------------------------------
  const line = (x1, y1, x2, y2, c = '') => `<line x1="${f(x1,1)}" y1="${f(y1,1)}" x2="${f(x2,1)}" y2="${f(y2,1)}" class="${c}"/>`;
  const arrow = (x1, y1, x2, y2, c = '') => `<line x1="${f(x1,1)}" y1="${f(y1,1)}" x2="${f(x2,1)}" y2="${f(y2,1)}" class="${c}" marker-end="url(#ah${c.includes('hi') ? 'h' : c.includes('acc') ? 'a' : ''})"/>`;
  const pathD = (d, c = '', extra = '') => `<path d="${d}" class="${c}" ${extra}/>`;
  const circle = (x, y, r, c = '') => `<circle cx="${f(x,1)}" cy="${f(y,1)}" r="${f(Math.max(r,0.5),1)}" class="${c}"/>`;
  const rect = (x, y, w, h, c = '') => `<rect x="${f(x,1)}" y="${f(y,1)}" width="${f(Math.max(w,0),1)}" height="${f(Math.max(h,0),1)}" rx="3" class="${c}"/>`;
  const text = (x, y, s, c = '', anchor = 'middle') => `<text x="${f(x,1)}" y="${f(y,1)}" text-anchor="${anchor}" class="${c}">${s}</text>`;
  const ground = (y = 160) => line(10, y, 310, y, 'soft') + Array.from({ length: 15 }, (_, i) => line(18 + i * 20, y, 12 + i * 20, y + 6, 'soft thin')).join('');
  // Bonhomme vu de profil, pieds a (x, y).
  const guy = (x, y, s = 1, c = '') => `<g class="${c}" transform="translate(${f(x,1)} ${f(y,1)}) scale(${s})">` +
    `<circle cx="0" cy="-30" r="5"/><line x1="0" y1="-25" x2="0" y2="-11"/><line x1="0" y1="-11" x2="-5" y2="0"/><line x1="0" y1="-11" x2="5" y2="0"/>` +
    `<line x1="0" y1="-21" x2="-6" y2="-14"/><line x1="0" y1="-21" x2="6" y2="-16"/></g>`;
  // Joueur vu de dessus (cercle + nez).
  const top = (x, y, ang = 0, c = '') => `<g class="${c}" transform="translate(${f(x,1)} ${f(y,1)}) rotate(${f(ang,1)})"><circle r="7"/><line x1="0" y1="0" x2="12" y2="0"/></g>`;
  const ball = (x, y, r = 5, c = 'ball') => circle(x, y, r, c);
  const goal = (x, y, s = 1) => `<g class="soft" transform="translate(${x} ${y}) scale(${s})"><rect x="-4" y="-34" width="8" height="34" rx="1"/><line x1="-4" y1="-34" x2="10" y2="-26"/><line x1="10" y1="-26" x2="10" y2="0"/></g>`;
  const label = (x, y, s, c = 'hi') => text(x, y, s, `lbl ${c}`);

  // Graphique simple : axes + courbe sur [0,1] -> points.
  function graph(points, xl, yl, extra = '') {
    const X0 = 40, Y0 = 150, GW = 260, GH = 120;
    const d = points.map(([x, y], i) => `${i ? 'L' : 'M'}${f(X0 + x * GW, 1)},${f(Y0 - y * GH, 1)}`).join(' ');
    return arrow(X0, Y0, X0 + GW + 10, Y0, 'soft') + arrow(X0, Y0, X0, Y0 - GH - 10, 'soft') +
      text(X0 + GW + 6, Y0 + 14, xl, 'small', 'end') + text(X0 - 4, Y0 - GH - 14, yl, 'small', 'start') +
      pathD(d, 'hi fat') + extra;
  }
  const gx = x => 40 + x * 260, gy = y => 150 - y * 120;

  // Frise : segments [{len, label, cls}] sur une ligne de temps.
  function timeline(segs, title = '') {
    // Largeur mini par segment pour que les libelles restent lisibles.
    const raw = segs.reduce((s, x) => s + Math.max(x.len, 0), 0) || 1;
    const lens = segs.map(x => Math.max(x.len, raw * 0.3));
    const total = lens.reduce((a, b) => a + b, 0);
    let x = 20, out = title ? text(160, 30, title, 'small') : '';
    for (const [i, s] of segs.entries()) {
      const w = 280 * lens[i] / total;
      out += rect(x, 70, w - 2, 40, s.cls || 'soft') + text(x + w / 2, 95, s.label, s.cls && s.cls.includes('hi') ? 'lbl hi' : 'small');
      if (s.sub) out += text(x + w / 2, 128, s.sub, 'small');
      x += w;
    }
    return out + arrow(20, 145, 305, 145, 'soft') + text(305, 160, 'temps', 'small', 'end');
  }

  // Jauge generique pour une valeur sans croquis dedie.
  function gauge(V, name, def) {
    const v = n(V, name);
    const lo = def && def.min != null ? def.min : 0;
    const hi = def && def.max != null ? def.max : Math.max(1, Math.abs(v) * 2);
    const t = clamp((v - lo) / (hi - lo || 1), 0, 1);
    return rect(30, 80, 260, 20, 'soft') + rect(30, 80, 260 * t, 20, 'hi fill') + text(160, 130, `${f(v, 2)}`, 'lbl hi');
  }

  // --- simulations simples --------------------------------------------------
  // Trajectoire de saut (cm, s) avec gravite de montee/descente separees (g Unreal = 980 cm/s2).
  function jumpArc(V, vx) {
    const v0 = n(V, 'JumpVelocity', 4800), gr = 980 * n(V, 'JumpRiseGravityScale', 12), gf = 980 * n(V, 'FallGravityScale', 9);
    const tUp = v0 / gr, h = v0 * v0 / (2 * gr), tDown = Math.sqrt(2 * h / gf);
    return { tUp, h, tDown, dist: vx * (tUp + tDown) };
  }

  // --- croquis -------------------------------------------------------------
  const S = {};

  S.jump = (V, name) => {
    const vx = n(V, 'BaseWalkSpeed', 2800);
    const j = jumpArc(V, vx);
    const count = Math.max(1, Math.round(n(V, 'MaxJumpCount', 2)));
    const sx = 200 / Math.max(j.dist * count, 1), sy = 110 / Math.max(j.h * count, 1);
    const s = Math.min(sx, sy);
    let out = ground(), x0 = 30, y0 = 160, pts = [];
    // Chaque saut suivant part en l'air, a mi-hauteur de la descente du precedent.
    const gr = 980 * n(V, 'JumpRiseGravityScale', 12), gf = 980 * n(V, 'FallGravityScale', 9);
    let xs = 0, zs = 0;
    for (let k = 0; k < count; k++) {
      const last = k === count - 1;
      const tEnd = last ? j.tUp + Math.sqrt(2 * (j.h + zs) / gf) : j.tUp + Math.sqrt(j.h / gf);
      for (let i = 0; i <= 24; i++) {
        const t = tEnd * i / 24;
        const z = t <= j.tUp ? zs + j.h - 0.5 * gr * (j.tUp - t) ** 2 : zs + j.h - 0.5 * gf * (t - j.tUp) ** 2;
        pts.push([x0 + (xs + vx * t) * s, y0 - Math.max(z, 0) * s]);
      }
      xs += vx * tEnd; zs += j.h / 2;
    }
    const d = pts.map((p, i) => `${i ? 'L' : 'M'}${f(p[0],1)},${f(p[1],1)}`).join(' ');
    out += pathD(d, name === 'MaxJumpCount' || name === 'JumpVelocity' ? 'hi fat dash' : 'ink dash');
    out += guy(x0, y0, 0.9);
    const peak = pts.reduce((a, p) => (p[1] < a[1] ? p : a), pts[0]);
    out += line(peak[0], peak[1], peak[0], y0, 'acc thin') + text(peak[0] + 6, (peak[1] + y0) / 2, `${f(j.h / 100, 1)} m`, 'small', 'start');
    if (name === 'JumpRiseGravityScale') out += label(90, 40, `montee : ${f(j.tUp, 2)} s`);
    else if (name === 'FallGravityScale') out += label(230, 40, `descente : ${f(j.tDown, 2)} s`);
    else if (name === 'MaxJumpCount') out += label(240, 40, `${count} saut${count > 1 ? 's' : ''}`);
    else out += label(240, 40, `hauteur ${f(j.h / 100, 1)} m`);
    return out;
  };

  S.coyote = V => {
    const t = n(V, 'CoyoteTimeSeconds', 0.12);
    const w = clamp(t * 400, 6, 140);
    return line(10, 120, 140, 120, 'soft') + line(140, 120, 140, 170, 'soft') + line(140, 170, 310, 170, 'soft') +
      guy(110, 120, 0.8, 'soft') + rect(140, 75, w, 45, 'hi fill') + guy(140 + w - 6, 120, 0.8, 'hi') +
      arrow(140 + w - 6, 85, 140 + w + 20, 55, 'hi') + text(140 + w / 2, 140, `${f(t * 1000)} ms`, 'lbl hi') +
      text(140, 40, 'on peut encore sauter apres avoir quitte le bord', 'small');
  };

  S.speeds = (V, name) => {
    const walk = n(V, 'BaseWalkSpeed'), sprint = n(V, 'SprintWalkSpeed'), wall = n(V, 'WallRunSpeed');
    const m = Math.max(walk, sprint, wall, 1);
    const row = (y, v, l, k) => text(20, y + 14, l, 'small', 'start') + rect(95, y, 200 * v / m, 18, name === k ? 'hi fill' : 'soft fill') +
      text(100 + 200 * v / m, y + 14, `${f(v / 100, 1)} m/s`, 'small', 'start');
    return row(40, walk, 'marche', 'BaseWalkSpeed') + row(80, sprint, 'sprint', 'SprintWalkSpeed') + row(120, wall, 'wall run', 'WallRunSpeed');
  };

  S.accel = (V, name) => {
    const vmax = n(V, 'SprintWalkSpeed', 4000), a = Math.max(n(V, 'MaxAcceleration', 9000), 1), b = Math.max(n(V, 'BrakingDeceleration', 2048), 1);
    const tA = vmax / a, tB = vmax / b, hold = 0.6, T = tA + hold + tB;
    const pts = [[0, 0], [tA / T, 0.9], [(tA + hold) / T, 0.9], [1, 0]];
    return graph(pts, 'temps', 'vitesse') +
      (name === 'MaxAcceleration' ? label(gx(tA / T / 2) + 10, gy(0.45), `${f(tA, 2)} s`) : label(gx((tA + hold + tB / 2) / T) + 14, gy(0.45), `${f(tB, 2)} s pour s'arreter`)) +
      text(gx(tA / T / 2), 168, 'appui', 'small') + text(gx((tA + hold + tB / 2) / T), 168, 'relache', 'small');
  };

  S.decay = (V, name) => {
    const rate = Math.max(n(V, name, 1000), 1);
    const v0 = 9000, keep = n(V, 'SprintWalkSpeed', 4000);
    const t = (v0 - keep) / rate;
    const pts = [[0, 1], [clamp(t / 3, 0, 1), keep / v0], [1, keep / v0]];
    return graph(pts, 'temps', 'vitesse') + line(gx(0), gy(keep / v0), gx(1), gy(keep / v0), 'acc thin dash') +
      text(gx(1), gy(keep / v0) - 6, 'vitesse normale', 'small', 'end') + label(gx(0.3), gy(0.9), `l'elan en trop se perd en ${f(t, 1)} s`);
  };

  S.airControl = V => {
    const c = n(V, 'AirControl', 1);
    const bend = clamp(c * 50, 0, 120);
    return top(40, 130, -20) + arrow(52, 126, 160, 90, 'soft dash') + text(170, 85, 'sans controle', 'small', 'start') +
      pathD(`M52,126 Q140,100 ${f(150 + bend * 0.4)},${f(110 - bend)}`, 'hi fat') +
      text(40, 160, 'en l\'air, on tourne avec le stick :', 'small', 'start') + label(250, 40, `x${f(c, 2)}`);
  };

  S.airFriction = V => {
    const fr = n(V, 'AirMomentumFriction', 0.15);
    const len = clamp(260 * Math.exp(-fr * 2), 30, 260);
    return pathD(`M30,140 Q${30 + len / 2},40 ${30 + len},140`, 'hi fat dash') + guy(30, 150, 0.7) +
      pathD('M30,140 Q170,20 300,140', 'soft dash') + text(300, 160, 'sans frottement', 'small', 'end') + label(160, 170, `frottement ${f(fr, 2)}`);
  };

  S.airTurn = (V, name) => {
    const keep = n(V, 'AirTurnSpeedKeep', 0.9), rate = n(V, 'AirTurnRate', 720);
    const r = clamp(9000 / Math.max(rate, 1), 12, 60);
    return top(60, 100, 0, 'soft') + arrow(70, 100, 200, 100, 'soft') + pathD(`M200,100 A${f(r)},${f(r)} 0 0 1 200,${f(100 - 2 * r)}`, name === 'AirTurnRate' ? 'hi fat' : 'ink') +
      arrow(200, 100 - 2 * r, 200 - 140 * keep, 100 - 2 * r, name === 'AirTurnSpeedKeep' ? 'hi fat' : 'ink') +
      text(200 - 70 * keep, 90 - 2 * r, name === 'AirTurnSpeedKeep' ? `${f(keep * 100)} % de la vitesse gardee` : `${f(rate)} deg/s`, 'lbl hi') +
      text(160, 165, 'demi-tour en l\'air (vu de dessus)', 'small');
  };

  // Mur vu de dessus : le mur est la ligne du haut.
  const wallTop = () => rect(20, 30, 280, 10, 'soft fill') + text(160, 24, 'mur', 'small');

  S.wallRun = (V, name) => {
    const keep = n(V, 'WallRunEntrySpeedKeep', 0.9);
    const hiK = k => (name === k ? 'hi fat' : 'ink');
    return wallTop() + top(50, 140, -45) + pathD('M58,132 Q90,60 120,52', hiK('WallRunEntrySpeedKeep')) +
      arrow(120, 52, 120 + 170 * clamp(n(V, 'WallRunSpeed') / 8000, 0.2, 1), 52, hiK('WallRunSpeed')) +
      text(200, 75, name === 'WallRunEntrySpeedKeep' ? `garde ${f(keep * 100)} % de l'elan` : name === 'WallRunMomentumDecay' ? `l'elan en trop fond de ${f(n(V, name))}/s` : `${f(n(V, 'WallRunSpeed') / 100, 1)} m/s le long du mur`, 'lbl hi') +
      text(160, 165, 'course sur le mur (vu de dessus)', 'small');
  };

  // Vu de dessus : mur en haut, joueur colle au mur ; "dos au mur" = direction qui s'en eloigne.
  // Le secteur colore = directions du regard couvertes par l'angle (mesure depuis "dos au mur").
  S.wallAngle = (V, name) => {
    const a = clamp(n(V, name, 20), 0, 180);
    const px = 160, py = 46, R = 85, h = a * Math.PI / 180;
    const ex = s => px + s * Math.sin(h) * R, ey = py + Math.cos(h) * R;
    const sector = a >= 179.5 ? `M${px - R},${py} A${R},${R} 0 0 0 ${px + R},${py} Z`
      : `M${px},${py} L${f(ex(-1))},${f(ey)} A${R},${R} 0 ${a > 90 ? 1 : 0} 0 ${f(ex(1))},${f(ey)} Z`;
    const msg = name === 'WallDashDetachLookAngle'
      ? 'regard dans la zone + dash = on quitte le mur'
      : 'zone ou la camera peut regarder (au-dela : ramenee)';
    return wallTop() + pathD(sector, 'hi dashfill') + line(px, py, px, py + R + 6, 'acc thin dash') + text(px + 4, py + R + 6, 'dos au mur', 'small', 'start') +
      top(px, py, 90) + label(275, 100, `${f(a)}°`) + text(160, 176, msg, 'small');
  };

  S.wallTimes = (V, name) => timeline([
    { len: n(V, 'WallSlideMaxDuration', 5), label: 'sur le mur', sub: `max ${f(n(V, 'WallSlideMaxDuration', 5), 1)} s`, cls: name === 'WallSlideMaxDuration' ? 'hi fill' : 'soft' },
    { len: Math.max(n(V, 'WallLeaveGraceSeconds', 0.35) * 4, 0.6), label: 'decolle', sub: `saut ok ${f(n(V, 'WallLeaveGraceSeconds'), 2)} s`, cls: name === 'WallLeaveGraceSeconds' ? 'hi fill' : 'soft' },
    { len: Math.max(n(V, 'WallReattachSameWallSeconds', 0.8) * 3, 0.8), label: 'mur bloque', sub: `${f(n(V, 'WallReattachSameWallSeconds'), 2)} s`, cls: name === 'WallReattachSameWallSeconds' ? 'hi fill' : 'soft' },
  ], 'le mur, dans le temps');

  S.wallDash = (V, name) => {
    const boost = n(V, 'WallDashSpeedBoost'), min = n(V, 'WallDashMinSpeed');
    return wallTop() + top(80, 52, 0) + arrow(90, 56, 160, 140, 'ink') +
      arrow(160, 140, 160 + 120 * clamp(boost / 8000, 0.1, 1), 140 + 0, name === 'WallDashSpeedBoost' ? 'hi fat' : 'ink') +
      text(200, 128, name === 'WallDashSpeedBoost' ? `+${f(boost / 100)} m/s` : `au moins ${f(min / 100)} m/s`, 'lbl hi') +
      text(160, 172, 'dash pour quitter le mur', 'small');
  };

  S.wallCamera = (V, name) => {
    const s = n(V, name);
    return wallTop() + top(160, 120, 0) + pathD('M160,120 L220,60 M160,120 L250,100', 'soft') + pathD('M200,80 Q225,85 230,105', 'hi fat') +
      arrow(220, 95, 232, 108, 'hi') + text(160, 165, name.includes('Strength') ? `la camera tourne a ${f(s * 100)} % vers la course` : name === 'WallLookPushBackSpeed' ? `la camera est repoussee du mur a ${f(s)} deg/s` : `la camera suit la course a ${f(s)} deg/s`, 'lbl hi');
  };

  S.dash = (V, name) => {
    const imp = n(V, 'DashImpulse', 8000), dur = n(V, 'DashDurationSeconds', 0.2), cd = n(V, 'DashCooldownSeconds', 0.2);
    if (name === 'DashImpulse') {
      const L = clamp(imp * dur / 25, 20, 230);
      return ground() + guy(40, 160, 0.8, 'soft') + guy(40 + L, 160, 0.8) + arrow(50, 130, 40 + L - 6, 130, 'hi fat') +
        text(40 + L / 2, 120, `~${f(imp * dur / 100, 1)} m en un dash`, 'lbl hi');
    }
    return timeline([
      { len: dur, label: 'dash', sub: `${f(dur, 2)} s`, cls: name === 'DashDurationSeconds' ? 'hi fill' : 'soft' },
      { len: cd, label: 'recharge', sub: `${f(cd, 2)} s`, cls: name === 'DashCooldownSeconds' ? 'hi fill' : 'soft' },
      { len: Math.max(dur, 0.1), label: 'dash', cls: 'soft' },
    ], 'un dash, puis on attend le cooldown');
  };

  // Balle --------------------------------------------------------------------
  S.ballRadius = (V, name) => {
    const r = n(V, name, 500);
    const rr = clamp(r / 12, 8, 75);
    return top(160, 95) + circle(160, 95, rr, 'hi dashfill') + ball(160 + rr * 0.7, 95 - rr * 0.7) + ball(160 + rr + 25, 95 + 20, 5, 'soft') +
      text(160, 172, name === 'BallControlRadius' ? `la balle est attrapee a moins de ${f(r / 100, 1)} m` : `${f(r / 100, 1)} m`, 'lbl hi');
  };

  S.ballHit = (V, name) => {
    const m = n(V, 'BallPlayerHitMargin', 30), mv = n(V, 'BallPlayerHitVerticalMargin', 120);
    const sx = clamp(m / 3, 2, 40), sy = clamp(mv / 6, 2, 40);
    return ground(150) + rect(140 - sx, 70 - sy, 40 + 2 * sx, 80 + sy, 'hi dashfill') + guy(160, 150, 2.4) + ball(230, 90, 7) + arrow(290, 90, 240, 90, 'soft') +
      text(160, 172, name === 'BallPlayerHitMargin' ? `+${f(m)} cm autour du joueur` : `+${f(mv)} cm en hauteur`, 'lbl hi');
  };

  S.ballScale = (V, name) => {
    const s = n(V, name, 2);
    return ball(80, 95, 8, 'ball') + text(80, 130, 'normale', 'small') + arrow(110, 95, 150, 95, 'soft') + ball(220, 95, clamp(8 * s, 2, 70), 'ball hi') +
      text(220, 172, `x${f(s, 1)}`, 'lbl hi');
  };

  S.orbit = (V, name) => {
    const r = clamp(n(V, 'BallOrbitRadius', 450) / 8, 10, 70), drop = clamp(n(V, 'BallOrbitDropBelowCamera', 30) / 2, 0, 40);
    return guy(80, 160, 2) + pathD(`M80,98 L230,${f(80)} M80,98 L230,${f(130)}`, 'soft') + text(232, 76, 'vue camera', 'small', 'start') +
      `<ellipse cx="80" cy="${f(98 + drop)}" rx="${f(r)}" ry="${f(r * 0.3)}" class="${name === 'BallOrbitRadius' ? 'hi fat' : 'ink'}"/>` + ball(80 + r, 98 + drop, 6) +
      (name === 'BallOrbitDropBelowCamera' ? line(80 + r + 14, 98, 80 + r + 14, 98 + drop, 'hi fat') : '') +
      text(200, 172, name === 'BallOrbitRadius' ? `la balle tourne a ${f(n(V, name) / 100, 1)} m` : name === 'BallOrbitDropBelowCamera' ? `${f(n(V, name))} cm sous la camera` : `compensation FOV ${f(n(V, name), 2)}`, 'lbl hi');
  };

  S.recapture = V => timeline([
    { len: 0.3, label: 'tir', cls: 'soft' },
    { len: n(V, 'BallRecaptureDelayAfterShot', 0.35), label: 'pas de reprise', sub: `${f(n(V, 'BallRecaptureDelayAfterShot'), 2)} s`, cls: 'hi fill' },
    { len: 0.5, label: 'reprise ok', cls: 'soft' },
  ], 'apres un tir, on ne peut pas reprendre sa propre balle tout de suite');

  S.pass = V => {
    const p = n(V, 'BasePassPower', 1600);
    const L = clamp(p / 12, 20, 240);
    return top(40, 95) + ball(55, 95) + arrow(62, 95, 62 + L, 95, 'hi fat') + top(80 + L, 95, 180, 'soft') + label(60 + L / 2, 80, `${f(p / 100, 1)} m/s`);
  };

  S.camp = V => timeline([
    { len: n(V, 'BallHoldGraceSeconds', 5), label: 'garde la balle', sub: `${f(n(V, 'BallHoldGraceSeconds'))} s tranquille`, cls: 'soft' },
    { len: n(V, 'BallHoldCountdownSeconds', 5), label: '5..4..3..', sub: `${f(n(V, 'BallHoldCountdownSeconds'))} s d'alerte`, cls: 'hi fill' },
    { len: 1, label: 'balle perdue', cls: 'soft' },
  ], 'anti-stagnation : trop longtemps avec la balle');

  S.curve = (V, name) => {
    const hd = clamp(n(V, 'CurvedShotHorizontalDistance', 560) / 15, 0, 90), vd = clamp(n(V, 'CurvedShotVerticalDistance', 560) / 15, 0, 60);
    const straight = clamp(n(V, 'CurvedShotInitialStraightDistance', 220) / 6, 0, 80);
    const k = name === 'CurvedShotVerticalDistance' ? 'v' : 'h';
    const d = k === 'v'
      ? `M40,120 L${40 + straight},120 Q160,${f(120 - vd * 2)} 280,100`
      : `M40,120 L${40 + straight},120 Q160,${f(120 - hd * 1.2)} 280,100`;
    return top(30, 120) + ball(40, 120) + goal(290, 120, 1.2) + line(40, 120, 280, 100, 'soft dash') + pathD(d, 'hi fat') +
      (name === 'CurvedShotInitialStraightDistance' || name === 'CurvedShotInitialStraightDistanceRatio' ? line(40, 135, 40 + straight, 135, 'hi fat') + text(40, 150, 'part tout droit', 'small', 'start') : '') +
      text(160, 172, name === 'CurvedShotHorizontalDistance' ? `courbe gauche/droite ${f(n(V, name) / 100, 1)} m` : name === 'CurvedShotVerticalDistance' ? `courbe haut/bas ${f(n(V, name) / 100, 1)} m` : 'tir enroule : la balle contourne', 'lbl hi');
  };

  S.curveSpeed = (V, name) => {
    const v = n(V, name);
    return top(30, 120) + pathD('M40,120 Q160,30 280,100', 'ink dash') + ball(160, 63) + arrow(166, 62, 166 + clamp(v / 150, 10, 120), 64, 'hi fat') + goal(290, 120, 1.2) +
      text(160, 160, `${f(v / 100, 1)} m/s`, 'lbl hi');
  };

  S.closeRange = V => {
    const r = n(V, 'CurvedShotCloseRangeReduction', 0.45);
    return top(30, 70) + goal(150, 70) + pathD('M40,70 Q95,30 145,62', 'hi fat') + text(95, 95, 'but proche', 'small') +
      top(30, 150) + goal(300, 150) + pathD('M40,150 Q170,80 295,142', 'ink') + text(170, 172, 'but loin', 'small') +
      label(240, 50, `courbe proche x${f(1 - r, 2)}`);
  };

  S.curveDisplay = (V, name) => {
    const v = n(V, name);
    const msg = {
      CurvedShotMinDisplayDistance: `courbe cachee a moins de ${f(v / 100, 1)} m`,
      CurvedShotGoalLockMargin: `le but se verrouille a ${f(v)} cm pres`,
      CurvedShotVisualStartOffset: `le trace commence a ${f(v)} cm de la balle`,
      CurvedShotAimTraceDistance: `visee jusqu'a ${f(v / 100)} m`,
      CurvedShotPostObstacleOpacityMultiplier: `apres un obstacle : opacite x${f(v, 2)}`,
      CurvedShotCollisionSampleCount: `${f(v)} points testes sur la courbe`,
      CurvedShotScaleMultiplier: `taille de la balle x${f(v, 1)}`,
      CurvedShotInputChangeSpeed: `la courbe suit le stick a x${f(v, 1)}`,
    }[name] || f(v, 2);
    let pts = '';
    if (name === 'CurvedShotCollisionSampleCount') for (let i = 0; i <= clamp(v, 1, 40); i++) { const t = i / clamp(v, 1, 40); pts += circle((1 - t) ** 2 * 40 + 2 * (1 - t) * t * 160 + t * t * 280, (1 - t) ** 2 * 120 + 2 * (1 - t) * t * 30 + t * t * 100, 2.5, 'hi'); }
    const obs = name === 'CurvedShotPostObstacleOpacityMultiplier' ? rect(150, 50, 14, 40, 'soft fill') + pathD('M160,63 Q220,75 280,100', 'hi fat', `opacity="${clamp(v, 0.05, 1)}"`) : '';
    return top(30, 120) + ball(40, 120) + goal(290, 120, 1.2) + pathD(name === 'CurvedShotPostObstacleOpacityMultiplier' ? 'M40,120 Q100,60 156,63' : 'M40,120 Q160,30 280,100', 'ink dash') + pts + obs + text(160, 165, msg, 'lbl hi');
  };

  // Grappin ------------------------------------------------------------------
  S.grappleRange = (V, name) => {
    const r = clamp(n(V, name, 2200) / 40, 10, 140);
    return top(40, 150) + `<path d="M40,150 m${f(-r)},0 a${f(r)},${f(r)} 0 0 1 ${f(2 * r)},0" class="hi dashfill"/>` + circle(40 + r * 0.6, 150 - r * 0.6, 4, 'acc') +
      line(40, 150, 40 + r * 0.6, 150 - r * 0.6, 'ink') + text(200, 40, name === 'GrappleMinTargetDistance' ? `cible ignoree a moins de ${f(n(V, name) / 100, 1)} m` : `portee ${f(n(V, name) / 100, 1)} m`, 'lbl hi');
  };

  S.grappleLaunch = (V, name) => {
    const vmin = n(V, 'GrappleLaunchMinSpeed', 4200), vmax = n(V, 'GrappleLaunchMaxSpeed', 10500);
    const start = n(V, 'GrappleLaunchDistanceStart', 120), range = Math.max(n(V, 'GrappleLaunchDistanceRange', 2400), 1), ex = n(V, 'GrappleLaunchDistanceExponent', 0.85);
    const D = Math.max(start + range * 1.3, 1), top_ = Math.max(vmax, vmin, 1) * 1.1;
    const pts = [];
    for (let i = 0; i <= 40; i++) {
      const d = D * i / 40, t = clamp((d - start) / range, 0, 1);
      pts.push([i / 40, lerp(vmin, vmax, Math.pow(t, Math.max(ex, 0.01))) / top_]);
    }
    return graph(pts, 'distance a la cible', 'vitesse de lancement',
      line(gx(start / D), gy(0), gx(start / D), gy(1), name === 'GrappleLaunchDistanceStart' ? 'hi thin dash' : 'soft thin dash') +
      line(gx((start + range) / D), gy(0), gx((start + range) / D), gy(1), name === 'GrappleLaunchDistanceRange' ? 'hi thin dash' : 'soft thin dash') +
      text(gx(0.02), gy(vmin / top_) - 6, `proche ${f(vmin / 100)} m/s`, 'small', 'start') + text(gx(1), gy(vmax / top_) - 6, `loin ${f(vmax / 100)} m/s`, 'small', 'end'));
  };

  S.grappleSimple = (V, name) => {
    const hf = n(V, 'GrappleSimpleHorizontalForce', 1), vf = n(V, 'GrappleSimpleHeightForce', 1);
    return ground() + guy(40, 160, 0.8) + circle(250, 40, 5, 'acc') + line(40, 130, 250, 40, 'ink thin dash') +
      arrow(40, 130, 40 + 90 * clamp(hf, 0.1, 2), 130, name.includes('Horizontal') ? 'hi fat' : 'ink') +
      arrow(40, 130, 40, 130 - 60 * clamp(vf, 0.1, 2), name.includes('Height') ? 'hi fat' : 'ink') +
      text(190, 120, `${{ GrappleSimpleHorizontalForce: 'poussee vers l\'avant', GrappleSimpleHeightForce: 'poussee vers le haut', GrappleSimpleDistancePower: 'effet de la distance', GrappleSimpleHeightDifferencePower: 'effet de l\'ecart de hauteur', GrappleSimpleHorizontalDifferencePower: 'effet de l\'ecart horizontal' }[name] || ''} : ${f(n(V, name), 2)}`, 'lbl hi');
  };

  S.grappleSwing = (V, name) => {
    const sag = clamp(n(V, 'GrappleSwingSagRatio', 0.3) * 120, 0, 90);
    const maxSag = clamp(n(V, 'GrappleSwingMaxSag', 800) / 10, 0, 90);
    const s = Math.min(sag, maxSag);
    return circle(160, 30, 5, 'acc') + pathD(`M40,60 Q160,${f(60 + s * 2)} 280,60`, 'hi fat dash') + line(40, 60, 160, 30, 'soft thin') + guy(40, 74, 0.6) +
      line(40, 60, 280, 60, 'soft thin dash') +
      text(160, 172, { GrappleSwingSagRatio: `creux du balancier : ${f(n(V, name) * 100)} % de la distance`, GrappleSwingMaxSag: `creux plafonne a ${f(n(V, name) / 100, 1)} m`, GrappleSwingSpeedBoost: `+${f(n(V, name) * 100)} % de vitesse au creux`, GrappleSwingMaxHeightBelowTarget: `balancier si la cible est a moins de ${f(n(V, name) / 100, 1)} m au-dessus` }[name], 'lbl hi');
  };

  S.grappleTimes = (V, name) => timeline([
    { len: n(V, 'GrapplePullBlendTime', 0.12), label: 'montee', sub: `${f(n(V, 'GrapplePullBlendTime'), 2)} s`, cls: name === 'GrapplePullBlendTime' ? 'hi fill' : 'soft' },
    { len: n(V, 'GrappleDurationSeconds', 1.2), label: 'traction', sub: `${f(n(V, 'GrappleDurationSeconds'), 2)} s`, cls: name === 'GrappleDurationSeconds' ? 'hi fill' : 'soft' },
    { len: n(V, 'GrappleReleaseDelay', 0.2), label: 'lache', sub: `${f(n(V, 'GrappleReleaseDelay'), 2)} s`, cls: name === 'GrappleReleaseDelay' ? 'hi fill' : 'soft' },
    { len: n(V, 'GrappleRestartDelay', 1.2), label: 'relance', sub: `${f(n(V, 'GrappleRestartDelay'), 2)} s`, cls: name === 'GrappleRestartDelay' ? 'hi fill' : 'soft' },
  ], 'un grappin, du tir a la relance');

  S.grappleSpeed = (V, name) => {
    const v = n(V, name);
    return ground() + guy(60, 160, 0.8) + circle(270, 40, 5, 'acc') + line(60, 135, 270, 40, 'ink') +
      arrow(70, 128, 70 + clamp(v / 100, 10, 160), 128 - clamp(v / 100, 10, 160) * 0.45, 'hi fat') +
      text(170, 172, `${{ GrappleVelocityMax: 'vitesse max', GrappleVelocityMin: 'vitesse min', GrappleSwingMaxSpeed: 'vitesse max pendant le grappin', GrappleFarLaunchExtraUpSpeed: 'bonus vers le haut (cible loin)', GrappleSurfaceAimLaunchSpeed: 'vitesse en visant une surface', GrappleArrivalUpBoost: 'petit saut a l\'arrivee' }[name] || ''} ${f(v / 100, 1)} m/s`, 'lbl hi');
  };

  S.grappleMult = (V, name) => {
    const v = n(V, name);
    return arrow(40, 70, 160, 70, 'ink') + text(100, 60, 'avant', 'small') + arrow(40, 120, 40 + 120 * clamp(v, 0, 2.2), 120, 'hi fat') +
      text(100, 145, 'apres', 'small') + text(220, 100, { GrappleLaunchCarryBoost: `elan garde x${f(v, 2)}`, GrappleReleaseVelocityBoost: `au lacher x${f(v, 2)}`, GrappleArrivalSpeedKeep: `relance : ${f(v * 100)} % garde` }[name] || `x${f(v, 2)}`, 'lbl hi', 'start');
  };

  S.grappleDetach = (V, name) => {
    const v = n(V, name);
    const d = clamp((name === 'GrappleEarlyDetachLeadTime' ? v * 2000 : v) / 4, 4, 120);
    return circle(280, 90, 6, 'acc') + line(40, 90, 280, 90, 'ink') + guy(280 - d, 105, 0.6) +
      line(280 - d, 125, 280, 125, 'hi fat') + text(280 - d / 2, 140, name === 'GrappleEarlyDetachLeadTime' ? `${f(v * 1000)} ms avant` : `${f(v)} cm`, 'lbl hi') +
      text(140, 40, 'le grappin se coupe avant d\'arriver sur la cible', 'small');
  };

  S.grappleAim = (V, name) => {
    const v = n(V, name);
    const e = name === 'GrappleAimHitboxExpansion' ? clamp(v / 8, 0, 50) : name === 'GrappleSurfaceAimDeadZone' ? clamp(v * 200, 2, 70) : 10;
    return rect(140, 60, 40, 40, 'soft fill') + rect(140 - e, 60 - e, 40 + 2 * e, 40 + 2 * e, 'hi dashfill') + circle(160, 80, 3, 'acc') +
      text(160, 172, { GrappleAimHitboxExpansion: `la cible compte ${f(v)} cm plus large`, GrappleSurfaceAimDeadZone: `zone morte au centre ${f(v * 100)} %`, GrappleTraceStartOffset: `la detection part ${f(v)} cm devant` }[name] || f(v, 2), 'lbl hi');
  };

  S.grappleFarUp = (V, name) => {
    const s = n(V, 'GrappleFarUpBoostStart', 1100), r = Math.max(n(V, 'GrappleFarUpBoostRange', 5000), 1);
    const D = s + r * 1.3;
    return graph([[0, 0], [s / D, 0], [(s + r) / D, 0.9], [1, 0.9]], 'distance', 'bonus vers le haut',
      line(gx(name === 'GrappleFarUpBoostStart' ? s / D : (s + r) / D), gy(0), gx(name === 'GrappleFarUpBoostStart' ? s / D : (s + r) / D), gy(1), 'hi thin dash'));
  };

  S.rope = (V, name) => {
    const v = n(V, name);
    const slack = name === 'GrappleCableSlack' ? clamp(v, 0, 60) : 10;
    return guy(40, 150, 0.8) + circle(280, 40, 5, 'acc') + pathD(`M46,128 Q${160},${f(84 + slack * 1.5)} 280,40`, name === 'GrappleCableSlack' ? 'hi fat' : 'ink') +
      text(160, 172, { GrappleMinCableLength: `corde visible a partir de ${f(v)} cm`, GrappleRopeDisplayDuration: `corde affichee ${f(v, 2)} s`, GrappleCableSlack: `mou de la corde au depart : ${f(v)}` }[name], 'lbl hi');
  };

  S.aimDot = (V, name) => {
    const on = !!V.bShowAimDot && V.bShowAimDot !== 'False';
    const s = clamp(n(V, 'AimDotSize', 5), 1, 30);
    return rect(60, 30, 200, 120, 'soft') + (on ? circle(160, 90, s, 'hi fill') : text(160, 95, 'point cache', 'small')) +
      text(160, 172, name === 'bShowAimDot' ? (on ? 'point de visee affiche' : 'point de visee masque') : `taille ${f(s, 1)}`, 'lbl hi');
  };

  // Camera -------------------------------------------------------------------
  S.fov = (V, name) => {
    const a = clamp(n(V, name, 100), 20, 170);
    const h = a / 2 * Math.PI / 180, L = 75;
    const other = name === 'RunFOV' ? n(V, 'SprintFOV') : n(V, 'RunFOV');
    const ho = clamp(other, 20, 170) / 2 * Math.PI / 180;
    return top(80, 85, 0) + pathD(`M80,85 L${f(80 + Math.cos(ho) * L)},${f(85 - Math.sin(ho) * L)} M80,85 L${f(80 + Math.cos(ho) * L)},${f(85 + Math.sin(ho) * L)}`, 'soft dash') +
      pathD(`M80,85 L${f(80 + Math.cos(h) * L)},${f(85 - Math.sin(h) * L)} A${L},${L} 0 0 1 ${f(80 + Math.cos(h) * L)},${f(85 + Math.sin(h) * L)} Z`, 'hi dashfill') +
      text(250, 85, `${f(a)}°`, 'lbl hi') + text(160, 172, name === 'SprintFOV' ? 'champ de vision en sprint (pointille : en course)' : 'champ de vision en course (pointille : en sprint)', 'small');
  };

  S.camShake = (V, name) => {
    const v = n(V, name);
    if (name === 'CameraRollDegrees') {
      return `<g transform="rotate(${f(-clamp(v, 0, 30))} 160 90)">${rect(60, 40, 200, 100, 'hi')}${line(60, 100, 260, 100, 'soft')}</g>` + text(160, 172, `l'image penche de ${f(v, 1)}° en strafe`, 'lbl hi');
    }
    const amp = clamp(v * 8, 0, 40);
    let d = 'M20,90';
    for (let x = 20; x <= 300; x += 5) d += ` L${x},${f(90 + Math.sin((x - 20) / 18) * amp)}`;
    return pathD(d, 'hi fat') + line(20, 90, 300, 90, 'soft dash') + text(160, 172, `la camera monte/descend a chaque pas (${f(v, 1)})`, 'lbl hi');
  };

  S.camLag = (V, name) => {
    const v = n(V, name);
    const d = clamp(v / 3, 0, 100);
    return top(220, 95) + `<g class="soft">${rect(200 - d, 85, 18, 14)}</g>` + `<g class="hi">${rect(200 - d, 85, 18, 14)}</g>` + line(218 - d, 92, 213, 95, 'hi dash') +
      arrow(240, 95, 300, 95, 'ink') + text(160, 172, name === 'CameraLagMaxDistance' ? `la camera peut trainer jusqu'a ${f(v)} cm` : name === 'FOVInterpSpeed' ? `le FOV change a la vitesse ${f(v, 1)}` : `lissage ${f(v, 1)}`, 'lbl hi');
  };

  S.speedFx = (V, name) => {
    const s = n(V, 'SpeedEffectsStartSpeed', 2900), fl = n(V, 'SpeedEffectsFullSpeed', 5400), mx = Math.max(fl * 1.3, n(V, 'SpeedFOVOverSpeedMaxSpeed', 8000));
    return graph([[0, 0], [s / mx, 0], [fl / mx, 0.8], [1, 0.8]], 'vitesse', 'effets (vent, FOV)',
      line(gx(s / mx), gy(0), gx(s / mx), gy(1), name === 'SpeedEffectsStartSpeed' ? 'hi thin dash' : 'soft thin dash') +
      line(gx(fl / mx), gy(0), gx(fl / mx), gy(1), name === 'SpeedEffectsFullSpeed' ? 'hi thin dash' : 'soft thin dash') +
      text(gx(s / mx), 168, `${f(s / 100)} m/s`, 'small') + text(gx(fl / mx), 168, `${f(fl / 100)} m/s`, 'small'));
  };

  S.speedLines = (V, name) => {
    const on = V.bShowSpeedLines !== false && V.bShowSpeedLines !== 'False';
    const k = clamp(n(V, 'SpeedLinesIntensity', 1), 0, 2);
    let l = '';
    if (on) for (let i = 0; i < 14; i++) { const a = i / 14 * Math.PI * 2; l += line(160 + Math.cos(a) * 50, 90 + Math.sin(a) * 35, 160 + Math.cos(a) * (50 + 60 * k), 90 + Math.sin(a) * (35 + 45 * k), 'hi'); }
    return rect(40, 20, 240, 140, 'soft') + l + text(160, 95, on ? '' : 'pas de trainees', 'small') + text(160, 175, on ? `trainees de vent x${f(k, 2)}` : 'desactive', 'lbl hi');
  };

  S.overSpeed = (V, name) => {
    const b = n(V, 'SpeedFOVOverSpeedBoost', 5), m = n(V, 'SpeedFOVOverSpeedMaxSpeed', 8000), fl = n(V, 'SpeedEffectsFullSpeed', 5400);
    const mx = m * 1.2;
    return graph([[0, 0], [fl / mx, 0], [m / mx, 0.8], [1, 0.8]], 'vitesse', 'FOV en plus', text(gx(m / mx), gy(0.8) - 8, `+${f(b)}°`, 'lbl hi'));
  };

  S.landing = (V, name) => {
    const d = clamp(n(V, 'LandingDipMaxDistance', 12) * 2, 0, 60);
    return ground() + guy(160, 160, 1.4) + rect(140, 50 + d, 40, 26, 'hi') + rect(140, 50, 40, 26, 'soft') + arrow(200, 50, 200, 50 + d, 'hi') +
      text(160, 30, name === 'LandingDipMaxDistance' ? `la camera s'enfonce de ${f(n(V, name))} cm max` : `tassement complet en tombant a ${f(n(V, name) / 100)} m/s`, 'lbl hi');
  };

  // Match / terrain ----------------------------------------------------------
  S.intro = (V, name) => timeline([
    { len: V.bEnablePreMatchIntro === false || V.bEnablePreMatchIntro === 'False' ? 1.5 : n(V, 'PreMatchIntroDurationSeconds', 5), label: V.bEnablePreMatchIntro === false || V.bEnablePreMatchIntro === 'False' ? 'pas d intro' : 'vue aerienne', sub: `${f(n(V, 'PreMatchIntroDurationSeconds'), 1)} s`, cls: name !== 'PreMatchCountdownSeconds' ? 'hi fill' : 'soft' },
    { len: n(V, 'PreMatchCountdownSeconds', 3), label: Array.from({ length: Math.max(0, n(V, 'PreMatchCountdownSeconds', 3)) }, (_, i) => n(V, 'PreMatchCountdownSeconds', 3) - i).join('-') || 'go', cls: name === 'PreMatchCountdownSeconds' ? 'hi fill' : 'soft' },
    { len: 6, label: 'match', sub: `${f(n(V, 'MatchStartSeconds') / 60, 1)} min`, cls: name === 'MatchStartSeconds' ? 'hi fill' : 'soft' },
  ], 'debut de match');

  S.players = V => {
    const k = clamp(Math.round(n(V, 'RequiredNetworkPlayersToStart', 4)), 1, 12);
    let out = '';
    for (let i = 0; i < k; i++) out += guy(160 - (k - 1) * 14 + i * 28, 130, 0.8, 'hi');
    return out + text(160, 165, `le match attend ${k} joueurs`, 'lbl hi');
  };

  S.prison = (V, name) => {
    const k = clamp(Math.round(n(V, 'PrisonCompletePlayerCount', 2)), 1, 6);
    let out = rect(150, 50, 140, 100, 'soft') + text(220, 44, 'prison', 'small');
    for (let i = 0; i < k; i++) out += guy(170 + i * 22, 140, 0.7, 'hi');
    for (let i = 0; i < 6; i++) out += line(160 + i * 25, 50, 160 + i * 25, 150, 'soft thin');
    const msg = {
      PrisonDurationSeconds: `${f(n(V, name))} s en prison`,
      PrisonCompletePlayerCount: `${k} prisonniers = prison complete`,
      PrisonCompletePoints: `prison complete : +${f(n(V, name))} pts`,
      PrisonCompleteReleaseDelaySeconds: `liberation apres ${f(n(V, name), 1)} s`,
    }[name];
    return out + guy(60, 150, 0.9) + text(70, 90, msg, 'lbl hi');
  };

  S.spawnHeight = (V, name) => {
    const o = n(V, 'ObstacleMaxSpawnHeight', 5000), p = n(V, 'PillarMaxSpawnHeight', 5000), m = Math.max(o, p, 1);
    return ground() + rect(60, 160 - 130 * o / m, 60, 130 * o / m, name === 'ObstacleMaxSpawnHeight' ? 'hi dashfill' : 'soft') + text(90, 175, 'obstacles', 'small') +
      rect(200, 160 - 130 * p / m, 30, 130 * p / m, name === 'PillarMaxSpawnHeight' ? 'hi dashfill' : 'soft') + text(215, 175, 'piliers', 'small') +
      label(160, 20, `${f(n(V, name) / 100)} m max`);
  };

  S.timedObstacle = (V, name) => {
    if (name === 'TimedObstacleScale' || name === 'GrappleObstacleRespawnDelay' || name === 'GrappleConsumedFadeDuration') {
      if (name === 'TimedObstacleScale') { const s = clamp(n(V, name), 0.2, 6); return ground() + rect(60, 140, 20, 20, 'soft') + rect(200 - 10 * s, 160 - 20 * s, 20 * s, 20 * s, 'hi') + text(160, 30, `x${f(s, 1)}`, 'lbl hi'); }
      return timeline([
        { len: 0.4, label: 'accroche', cls: 'soft' },
        { len: Math.max(n(V, 'GrappleConsumedFadeDuration', 0.1), 0.05), label: 'disparait', sub: `${f(n(V, 'GrappleConsumedFadeDuration'), 2)} s`, cls: name === 'GrappleConsumedFadeDuration' ? 'hi fill' : 'soft' },
        { len: Math.max(n(V, 'GrappleObstacleRespawnDelay', 0), 0.05), label: 'revient', sub: `${f(n(V, 'GrappleObstacleRespawnDelay'), 2)} s`, cls: name === 'GrappleObstacleRespawnDelay' ? 'hi fill' : 'soft' },
      ], 'obstacle utilise par un grappin');
    }
    return timeline([
      { len: n(V, 'TimedObstacleStateDurationSeconds', 10), label: 'visible', sub: `${f(n(V, 'TimedObstacleStateDurationSeconds'), 1)} s`, cls: name === 'TimedObstacleStateDurationSeconds' ? 'hi fill' : 'soft' },
      { len: Math.max(n(V, 'TimedObstacleHiddenDurationSeconds', 3), 0.3), label: 'cache', sub: `${f(n(V, 'TimedObstacleHiddenDurationSeconds'), 1)} s`, cls: name === 'TimedObstacleHiddenDurationSeconds' ? 'hi fill' : 'soft' },
      { len: n(V, 'TimedObstacleStateDurationSeconds', 10), label: 'visible', cls: 'soft' },
    ], 'obstacle temporaire');
  };

  // --- association variable -> croquis ---------------------------------------
  const MAP = {
    bEnablePreMatchIntro: 'intro', PreMatchIntroDurationSeconds: 'intro', PreMatchCountdownSeconds: 'intro', MatchStartSeconds: 'intro',
    RequiredNetworkPlayersToStart: 'players',
    PrisonDurationSeconds: 'prison', PrisonCompletePlayerCount: 'prison', PrisonCompletePoints: 'prison', PrisonCompleteReleaseDelaySeconds: 'prison',
    ObstacleMaxSpawnHeight: 'spawnHeight', PillarMaxSpawnHeight: 'spawnHeight',
    TimedObstacleStateDurationSeconds: 'timedObstacle', TimedObstacleHiddenDurationSeconds: 'timedObstacle', TimedObstacleScale: 'timedObstacle',
    GrappleConsumedFadeDuration: 'timedObstacle', GrappleObstacleRespawnDelay: 'timedObstacle',
    BaseWalkSpeed: 'speeds', SprintWalkSpeed: 'speeds', WallRunSpeed: 'wallRun',
    MaxAcceleration: 'accel', BrakingDeceleration: 'accel',
    AirControl: 'airControl', AirMomentumFriction: 'airFriction', GroundMomentumDecay: 'decay', WallRunMomentumDecay: 'wallRun',
    AirTurnRate: 'airTurn', AirTurnSpeedKeep: 'airTurn',
    WallRunEntrySpeedKeep: 'wallRun', WallLookPushBackSpeed: 'wallCamera', WallSlideLookAngleLimit: 'wallAngle', WallDashDetachLookAngle: 'wallAngle',
    WallDashMinSpeed: 'wallDash', WallDashSpeedBoost: 'wallDash', WallLeaveGraceSeconds: 'wallTimes', WallReattachSameWallSeconds: 'wallTimes', WallSlideMaxDuration: 'wallTimes',
    WallRunCameraTurnSpeed: 'wallCamera', WallRunCameraTurnStrength: 'wallCamera',
    DashImpulse: 'dash', DashDurationSeconds: 'dash', DashCooldownSeconds: 'dash',
    MaxJumpCount: 'jump', JumpVelocity: 'jump', JumpRiseGravityScale: 'jump', FallGravityScale: 'jump', CoyoteTimeSeconds: 'coyote',
    BallControlRadius: 'ballRadius', BallPlayerHitMargin: 'ballHit', BallPlayerHitVerticalMargin: 'ballHit', BallScaleAfterShot: 'ballScale',
    BallOrbitRadius: 'orbit', BallOrbitDropBelowCamera: 'orbit', BallOrbitFOVCompensation: 'orbit', BallRecaptureDelayAfterShot: 'recapture',
    BasePassPower: 'pass', BallHoldGraceSeconds: 'camp', BallHoldCountdownSeconds: 'camp',
    CurvedShotOrbitBallSpeed: 'curveSpeed', CurvedShotSplineSpeed: 'curveSpeed', CurvedShotMaxSplineSpeed: 'curveSpeed', CurvedShotCapturedSpeedMultiplier: 'curveSpeed',
    CurvedShotHorizontalDistance: 'curve', CurvedShotVerticalDistance: 'curve', CurvedShotInitialStraightDistance: 'curve', CurvedShotInitialStraightDistanceRatio: 'curve',
    CurvedShotCloseRangeReduction: 'closeRange',
    GrappleRange: 'grappleRange', GrappleMinTargetDistance: 'grappleRange',
    GrappleDurationSeconds: 'grappleTimes', GrapplePullBlendTime: 'grappleTimes', GrappleReleaseDelay: 'grappleTimes', GrappleRestartDelay: 'grappleTimes',
    GrappleSimpleHorizontalForce: 'grappleSimple', GrappleSimpleHeightForce: 'grappleSimple', GrappleSimpleDistancePower: 'grappleSimple', GrappleSimpleHeightDifferencePower: 'grappleSimple', GrappleSimpleHorizontalDifferencePower: 'grappleSimple',
    GrappleVelocityMax: 'grappleSpeed', GrappleVelocityMin: 'grappleSpeed', GrappleSwingMaxSpeed: 'grappleSpeed', GrappleFarLaunchExtraUpSpeed: 'grappleSpeed', GrappleSurfaceAimLaunchSpeed: 'grappleSpeed', GrappleArrivalUpBoost: 'grappleSpeed',
    GrappleLaunchMinSpeed: 'grappleLaunch', GrappleLaunchMaxSpeed: 'grappleLaunch', GrappleLaunchDistanceStart: 'grappleLaunch', GrappleLaunchDistanceRange: 'grappleLaunch', GrappleLaunchDistanceExponent: 'grappleLaunch',
    GrappleLaunchCarryBoost: 'grappleMult', GrappleReleaseVelocityBoost: 'grappleMult', GrappleArrivalSpeedKeep: 'grappleMult',
    GrappleFarUpBoostStart: 'grappleFarUp', GrappleFarUpBoostRange: 'grappleFarUp',
    GrappleAutoDetachBuffer: 'grappleDetach', GrappleAutoReleaseDistance: 'grappleDetach', GrappleEarlyDetachLeadTime: 'grappleDetach',
    GrappleAimHitboxExpansion: 'grappleAim', GrappleSurfaceAimDeadZone: 'grappleAim', GrappleTraceStartOffset: 'grappleAim',
    GrappleSwingSagRatio: 'grappleSwing', GrappleSwingMaxSag: 'grappleSwing', GrappleSwingSpeedBoost: 'grappleSwing', GrappleSwingMaxHeightBelowTarget: 'grappleSwing',
    bShowAimDot: 'aimDot', AimDotSize: 'aimDot',
    GrappleMinCableLength: 'rope', GrappleRopeDisplayDuration: 'rope', GrappleCableSlack: 'rope',
    RunFOV: 'fov', SprintFOV: 'fov', FOVInterpSpeed: 'camLag', CameraBobAmplitude: 'camShake', CameraRollDegrees: 'camShake',
    WallCameraNormalSmoothing: 'camLag', CameraLagMaxDistance: 'camLag',
    SpeedEffectsStartSpeed: 'speedFx', SpeedEffectsFullSpeed: 'speedFx', SpeedEffectsInterpSpeed: 'camLag',
    SpeedFOVOverSpeedBoost: 'overSpeed', SpeedFOVOverSpeedMaxSpeed: 'overSpeed',
    bShowSpeedLines: 'speedLines', SpeedLinesIntensity: 'speedLines',
    LandingDipMaxDistance: 'landing', LandingDipFullFallSpeed: 'landing',
  };

  const DEFS = `<defs>
    <marker id="ah" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" class="mk"/></marker>
    <marker id="ahh" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" class="mk hi"/></marker>
    <marker id="aha" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" class="mk acc"/></marker>
  </defs>`;

  // CurvedShot* restants (affichage, collisions...) -> croquis generique de courbe.
  function sketchFor(name) {
    if (MAP[name]) return MAP[name];
    if (name.startsWith('CurvedShot')) return 'curveDisplay';
    return null;
  }

  // Variables reglees ensemble : une seule carte, un seul croquis (min/max, debut/fin, etc.).
  const GROUPS = [
    ['Debut de match', ['bEnablePreMatchIntro', 'PreMatchIntroDurationSeconds', 'PreMatchCountdownSeconds']],
    ['Prison', ['PrisonDurationSeconds', 'PrisonCompletePlayerCount', 'PrisonCompletePoints', 'PrisonCompleteReleaseDelaySeconds']],
    ['Hauteur de spawn', ['ObstacleMaxSpawnHeight', 'PillarMaxSpawnHeight']],
    ['Obstacle temporaire : cycle', ['TimedObstacleStateDurationSeconds', 'TimedObstacleHiddenDurationSeconds']],
    ['Vitesses au sol', ['BaseWalkSpeed', 'SprintWalkSpeed']],
    ['Acceleration et freinage', ['MaxAcceleration', 'BrakingDeceleration']],
    ['Demi-tour en l\'air', ['AirTurnRate', 'AirTurnSpeedKeep']],
    ['Course sur le mur', ['WallRunSpeed', 'WallRunEntrySpeedKeep', 'WallRunMomentumDecay']],
    ['Le mur dans le temps', ['WallSlideMaxDuration', 'WallLeaveGraceSeconds', 'WallReattachSameWallSeconds']],
    ['Dash pour quitter le mur', ['WallDashMinSpeed', 'WallDashSpeedBoost']],
    ['Camera vers la course', ['WallRunCameraTurnSpeed', 'WallRunCameraTurnStrength']],
    ['Dash', ['DashImpulse', 'DashDurationSeconds', 'DashCooldownSeconds']],
    ['Saut', ['JumpVelocity', 'MaxJumpCount', 'JumpRiseGravityScale', 'FallGravityScale']],
    ['Zone de touche joueur', ['BallPlayerHitMargin', 'BallPlayerHitVerticalMargin']],
    ['Orbite de la balle', ['BallOrbitRadius', 'BallOrbitDropBelowCamera', 'BallOrbitFOVCompensation']],
    ['Anti-stagnation', ['BallHoldGraceSeconds', 'BallHoldCountdownSeconds']],
    ['Forme de la courbe', ['CurvedShotHorizontalDistance', 'CurvedShotVerticalDistance']],
    ['Vitesse sur la courbe (min / max)', ['CurvedShotSplineSpeed', 'CurvedShotMaxSplineSpeed', 'CurvedShotCapturedSpeedMultiplier']],
    ['Depart en ligne droite', ['CurvedShotInitialStraightDistance', 'CurvedShotInitialStraightDistanceRatio']],
    ['Portee (min / max)', ['GrappleRange', 'GrappleMinTargetDistance']],
    ['Le grappin dans le temps', ['GrapplePullBlendTime', 'GrappleDurationSeconds', 'GrappleReleaseDelay', 'GrappleRestartDelay']],
    ['Grappin simple : forces', ['GrappleSimpleHorizontalForce', 'GrappleSimpleHeightForce', 'GrappleSimpleDistancePower', 'GrappleSimpleHeightDifferencePower', 'GrappleSimpleHorizontalDifferencePower']],
    ['Vitesse du grappin (min / max)', ['GrappleVelocityMin', 'GrappleVelocityMax', 'GrappleSwingMaxSpeed']],
    ['Vitesse de lancement selon la distance', ['GrappleLaunchMinSpeed', 'GrappleLaunchMaxSpeed', 'GrappleLaunchDistanceStart', 'GrappleLaunchDistanceRange', 'GrappleLaunchDistanceExponent']],
    ['Boost vertical (cible loin)', ['GrappleFarUpBoostStart', 'GrappleFarUpBoostRange', 'GrappleFarLaunchExtraUpSpeed']],
    ['Elan garde', ['GrappleLaunchCarryBoost', 'GrappleReleaseVelocityBoost', 'GrappleArrivalSpeedKeep', 'GrappleArrivalUpBoost']],
    ['Coupure automatique', ['GrappleAutoReleaseDistance', 'GrappleAutoDetachBuffer', 'GrappleEarlyDetachLeadTime']],
    ['Visee', ['GrappleAimHitboxExpansion', 'GrappleSurfaceAimDeadZone', 'GrappleSurfaceAimLaunchSpeed', 'GrappleTraceStartOffset']],
    ['Balancier', ['GrappleSwingSagRatio', 'GrappleSwingMaxSag', 'GrappleSwingSpeedBoost', 'GrappleSwingMaxHeightBelowTarget']],
    ['Point de visee', ['bShowAimDot', 'AimDotSize']],
    ['Corde', ['GrappleMinCableLength', 'GrappleRopeDisplayDuration', 'GrappleCableSlack']],
    ['Obstacle grappine', ['GrappleConsumedFadeDuration', 'GrappleObstacleRespawnDelay']],
    ['Champ de vision', ['RunFOV', 'SprintFOV', 'FOVInterpSpeed']],
    ['Secousses', ['CameraBobAmplitude', 'CameraRollDegrees']],
    ['Effets de vitesse (debut / max)', ['SpeedEffectsStartSpeed', 'SpeedEffectsFullSpeed', 'SpeedEffectsInterpSpeed']],
    ['FOV a tres grande vitesse', ['SpeedFOVOverSpeedBoost', 'SpeedFOVOverSpeedMaxSpeed']],
    ['Trainees de vent', ['bShowSpeedLines', 'SpeedLinesIntensity']],
    ['Atterrissage', ['LandingDipMaxDistance', 'LandingDipFullFallSpeed']],
  ];

  // --- croquis 3D : image capturee dans Unreal + dessin projete par-dessus ----------------------
  // Les images et cameras viennent de capture/capture_sketches.py (img/*.png, img/cameras.json).
  // Les positions du monde (cm, axes Unreal) doivent rester celles de la scene du script de capture.
  const SCENE = {
    wall: { y: -90, x0: -900, x1: 2100, h: 800 }, // face avant du mur
    target: [1800, 0, 900],                        // cible du grappin
    ball: [80, -110, 160],                         // balle en orbite
  };
  let cameras = null;
  const IW = 1280, IH = 720;

  // Projection perspective d'un point du monde (cm) vers les pixels de l'image capturee.
  function project(cam, p) {
    const r = Math.PI / 180, cp = Math.cos(cam.pitch * r), sp = Math.sin(cam.pitch * r), cy = Math.cos(cam.yaw * r), sy = Math.sin(cam.yaw * r);
    const fwd = [cp * cy, cp * sy, sp], right = [-sy, cy, 0], up = [-sp * cy, -sp * sy, cp];
    const d = [p[0] - cam.loc[0], p[1] - cam.loc[1], p[2] - cam.loc[2]];
    const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    const z = dot(d, fwd), focal = (cam.w / 2) / Math.tan(cam.fov * r / 2);
    if (z < 20) return null; // derriere la camera
    return [cam.w / 2 + dot(d, right) * focal / z, cam.h / 2 - dot(d, up) * focal / z];
  }
  // Trace coupe la ou il passe derriere la camera.
  const P3 = (cam, pts, c = 'hi fat', extra = '') => {
    let d = '', pen = false;
    for (const p of pts) {
      const q = project(cam, p);
      if (!q) { pen = false; continue; }
      d += `${pen ? 'L' : 'M'}${f(q[0], 1)},${f(q[1], 1)} `;
      pen = true;
    }
    return d ? pathD(d.trim(), c, extra) : '';
  };
  const A3 = (cam, a, b, c = 'hi fat') => { const p = project(cam, a), q = project(cam, b); return p && q ? arrow(p[0], p[1], q[0], q[1], c) : ''; };
  const T3 = (cam, p, s, c = 'lbl hi', anchor = 'middle', dy = 0) => { const q = project(cam, p); return q ? text(q[0], q[1] + dy, s, c, anchor) : ''; };
  const ring = (c, rad, z, n = 72) => Array.from({ length: n + 1 }, (_, i) => [c[0] + Math.cos(i / n * 2 * Math.PI) * rad, c[1] + Math.sin(i / n * 2 * Math.PI) * rad, z]);
  const tag = (x, y, s) => rect(x - 6, y - 64, s.length * 34 + 36, 86, "tag") + text(x + 12, y, s, "lbl hi", "start");

  const S3 = {};

  // Saut : vraie trajectoire calculee (vitesse de saut, gravites, sauts en l'air), vue de profil.
  S3.jump = (V, name, cam) => {
    const vx = n(V, 'BaseWalkSpeed', 2800), v0 = n(V, 'JumpVelocity', 4800);
    const gr = 980 * Math.max(n(V, 'JumpRiseGravityScale', 12), 0.01), gf = 980 * Math.max(n(V, 'FallGravityScale', 9), 0.01);
    const count = clamp(Math.round(n(V, 'MaxJumpCount', 2)), 1, 5);
    const tUp = v0 / gr, h = v0 * v0 / (2 * gr);
    let out = '', x = 0, z = 0, peak = null, pts = [[0, 0, 0]];
    for (let k = 0; k < count; k++) {
      const last = k === count - 1;
      // Saut suivant declenche a mi-descente du precedent.
      const tDown = last ? Math.sqrt(2 * (z + h) / gf) : Math.sqrt(h / gf);
      for (let i = 1; i <= 40; i++) {
        const t = (tUp + tDown) * i / 40;
        const zz = t <= tUp ? z + v0 * t - 0.5 * gr * t * t : z + h - 0.5 * gf * (t - tUp) ** 2;
        pts.push([x + vx * t, 0, Math.max(zz, 0)]);
      }
      if (!peak || z + h > peak[2]) peak = [x + vx * tUp, 0, z + h];
      x += vx * (tUp + tDown); z = last ? 0 : z + h / 2;
    }
    out += P3(cam, pts, 'hi fat dash');
    const mid = project(cam, [peak[0], 0, peak[2] / 2]) || [0, 0];
    out += P3(cam, [[peak[0], 0, 0], peak], 'acc') + text(mid[0] + 16, mid[1], `${f(peak[2] / 100, 1)} m`, 'lbl acc', 'start');
    const msg = { JumpRiseGravityScale: `montee en ${f(tUp, 2)} s`, FallGravityScale: `descente rapide x${f(n(V, 'FallGravityScale'), 1)}`, MaxJumpCount: `${count} saut${count > 1 ? 's' : ''}` }[name] || `hauteur ${f(h / 100, 1)} m`;
    return out + tag(40, 80, msg);
  };

  // Mur : trajectoire d'arrivee puis course le long du mur (1 seconde de course).
  S3.wall = (V, name, cam) => {
    const keep = clamp(n(V, 'WallRunEntrySpeedKeep', 0.9), 0, 1), speed = n(V, 'WallRunSpeed', 5500);
    const y = SCENE.wall.y + 50, z = 110;
    const L = clamp(speed * 0.35, 200, 2400);
    const entry = [];
    for (let i = 0; i <= 20; i++) { const t = i / 20; entry.push([-700 + 700 * t, 500 * (1 - t) ** 2 + y * (1 - (1 - t) ** 2), z]); }
    let out = P3(cam, entry, name === 'WallRunEntrySpeedKeep' ? 'hi fat' : 'ink fat') + A3(cam, [0, y, z], [L, y, z], name === 'WallRunEntrySpeedKeep' ? 'ink fat' : 'hi fat');
    const msg = { WallRunEntrySpeedKeep: `garde ${f(keep * 100)} % de l'elan`, WallRunMomentumDecay: `l'elan en trop fond de ${f(n(V, name) / 100, 1)} m/s par s` }[name] || `${f(speed / 100, 1)} m/s le long du mur`;
    return out + T3(cam, [L * 0.6, y, z + 120], '0,35 s de course', 'small', 'middle') + tag(40, 80, msg);
  };

  // Grappin : portee au sol, distance mini, corde vers la cible.
  S3.grapple = (V, name, cam) => {
    const range = n(V, 'GrappleRange', 2200), min = n(V, 'GrappleMinTargetDistance', 425);
    const tgt = SCENE.target, dist = Math.hypot(tgt[0], tgt[1], tgt[2] - 150), inRange = dist <= range;
    let out = P3(cam, ring([0, 0], range, 2), name === 'GrappleRange' ? 'hi fat dash' : 'ink dash') + P3(cam, ring([0, 0], min, 2, 48), name === 'GrappleMinTargetDistance' ? 'hi fat' : 'acc');
    out += P3(cam, [[20, 0, 150], tgt], inRange ? 'acc fat' : 'soft dash');
    const msg = name === 'GrappleMinTargetDistance' ? `cibles ignorees a moins de ${f(min / 100, 1)} m` : `portee ${f(range / 100, 1)} m : cible ${inRange ? 'atteignable' : 'trop loin'} (${f(dist / 100, 1)} m)`;
    return out + tag(40, 80, msg);
  };

  // Balle : rayon de controle au sol + orbite de la balle autour du joueur.
  S3.ball = (V, name, cam) => {
    const ctrl = n(V, 'BallControlRadius', 520), orb = n(V, 'BallOrbitRadius', 450), drop = n(V, 'BallOrbitDropBelowCamera', 30);
    const zOrb = 160 - drop * 0.5;
    let out = P3(cam, ring([0, 0], ctrl, 2), name === 'BallControlRadius' ? 'hi fat dash' : 'soft dash');
    out += P3(cam, ring([0, 0], Math.min(orb, 600) * 0.3, zOrb), name.startsWith('BallOrbit') ? 'hi fat' : 'ink');
    const msg = { BallControlRadius: `balle attrapee a moins de ${f(ctrl / 100, 1)} m` , BallOrbitRadius: `orbite a ${f(orb / 100, 1)} m (a l'ecran)`, BallOrbitDropBelowCamera: `${f(drop)} cm sous la camera`, BallOrbitFOVCompensation: `compensation FOV ${f(n(V, name), 2)}` }[name] || '';
    return out + tag(40, 80, msg);
  };

  // Variables qui utilisent un croquis 3D (cle de croquis 2D -> image).
  const IMAGE_FOR = { jump: 'jump', wallRun: 'wall', grappleRange: 'grapple', ballRadius: 'ball', orbit: 'ball' };
  const image3d = name => { const k = sketchFor(name); const img = IMAGE_FOR[k]; return img && cameras && cameras[img] ? img : null; };

  window.OraSketch = {
    groups: GROUPS,
    has: name => !!sketchFor(name),
    async loadCameras() {
      try { cameras = await fetch('img/cameras.json').then(r => (r.ok ? r.json() : null)); } catch (e) { cameras = null; }
    },
    // Dessine dans el : image 3D + calque si disponible (l'image n'est pas rechargee a chaque reglage), sinon croquis 2D.
    paint(el, name, values, def) {
      const img = image3d(name);
      if (!img) { el.innerHTML = this.render(name, values, def); return; }
      let body;
      try { body = S3[img](values, name, cameras[img]); } catch (e) { body = ''; console.error(e); }
      const svg = `<svg viewBox="0 0 ${IW} ${IH}" class="sketch s3d" role="img" aria-label="Croquis 3D : ${name}">${DEFS}${body}</svg>`;
      const cur = el.querySelector('img.bg');
      if (cur && cur.dataset.img === img) el.querySelector('svg').outerHTML = svg;
      else el.innerHTML = `<div class="sk3d"><img class="bg" data-img="${img}" src="img/${img}.png" alt="">${svg}</div>`;
    },
    render(name, values, def) {
      const key = sketchFor(name);
      let body;
      try { body = key ? S[key](values, name) : gauge(values, name, def); }
      catch (e) { body = text(160, 90, 'croquis indisponible', 'small'); }
      return `<svg viewBox="0 0 ${W} ${H}" class="sketch" role="img" aria-label="Croquis : ${name}">${DEFS}${body}</svg>`;
    },
  };
})();
