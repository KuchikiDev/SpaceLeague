// Interface : menus par categorie, une carte par groupe de variables avec son croquis.
(async function () {
  const $ = s => document.querySelector(s);
  const state = { vars: [], saved: {}, current: {}, section: null, search: '' };

  // Menu principal : la partie avant "|" de la categorie Unreal.
  const sectionOf = v => v.category.split('|')[0].trim();
  const subOf = v => (v.category.split('|')[1] || '').trim() || 'General';

  function parseValue(def, raw) {
    if (raw === undefined) raw = def.defaultRaw;
    if (def.type === 'bool') return /^true$/i.test(String(raw).trim());
    if (def.type === 'int' || def.type === 'float') return parseFloat(String(raw).replace(/f$/, ''));
    return String(raw);
  }

  async function load() {
    const res = await fetch('api/variables');
    if (res.status === 401) { showLogin('Session expiree, se reconnecter.'); throw new Error('connexion requise'); }
    if (!res.ok) throw new Error((await res.json().catch(() => ({}))).error || res.statusText);
    const data = await res.json();
    state.vars = data.variables.filter(v => v.type !== 'other');
    state.saved = {};
    for (const v of state.vars) state.saved[v.name] = parseValue(v, data.values[v.name]);
    state.current = { ...state.saved };
    state.ini = data.ini;
    if (!state.section) state.section = sectionOf(state.vars[0]);
  }

  const isDirty = name => state.current[name] !== state.saved[name];
  const dirtyNames = () => state.vars.filter(v => isDirty(v.name)).map(v => v.name);

  // Bornes du curseur : meta UI si presente, sinon autour de la valeur.
  function range(v) {
    const val = Math.abs(Number(state.saved[v.name]) || 0);
    const min = v.min != null ? v.min : 0;
    let max = v.max != null ? v.max : val > 0 ? val * 3 : 1;
    if (v.max == null && v.units === 'deg' || /Angle|FOV$|Degrees/.test(v.name)) max = v.max ?? Math.max(180, val);
    const step = v.type === 'int' ? 1 : (max - min) <= 2 ? 0.01 : (max - min) <= 20 ? 0.05 : (max - min) <= 200 ? 1 : 10;
    return { min, max, step };
  }

  function clampVal(v, x) {
    if (v.clampMin != null) x = Math.max(v.clampMin, x);
    if (v.clampMax != null) x = Math.min(v.clampMax, x);
    return v.type === 'int' ? Math.round(x) : x;
  }

  function renderMenu() {
    const sections = [...new Set(state.vars.map(sectionOf))];
    $('#menu').innerHTML = sections.map(s => {
      const list = state.vars.filter(v => sectionOf(v) === s);
      const dirty = list.filter(v => isDirty(v.name)).length;
      return `<a href="#" data-s="${s}" class="${s === state.section && !state.search ? 'active' : ''}">${s}<span class="count">${dirty ? `<span class="dirty">● ${dirty}</span>` : list.length}</span></a>`;
    }).join('');
    $('#menu').querySelectorAll('a').forEach(a => a.onclick = e => {
      e.preventDefault(); state.section = a.dataset.s; state.search = ''; $('#search').value = ''; render(); window.scrollTo(0, 0);
    });
  }

  function fmt(x) { return Number.isInteger(x) ? String(x) : String(+Number(x).toFixed(4)); }

  // Groupes : nom de variable -> { title, names }. Une carte par groupe, une ligne de reglage par variable.
  const groupOf = {};
  for (const [title, names] of OraSketch.groups) for (const n of names) groupOf[n] = { title, names };
  const byName = name => state.vars.find(x => x.name === name);
  state.focus = {}; // id de carte -> variable mise en evidence dans le croquis

  // Construit les cartes d'une liste de variables : un groupe apparait a la place de son premier membre.
  function cardsFor(list) {
    const done = new Set(), cards = [];
    for (const v of list) {
      if (done.has(v.name)) continue;
      const g = groupOf[v.name];
      const names = g ? g.names.filter(n => byName(n) && sectionOf(byName(n)) === sectionOf(v)) : [v.name];
      names.forEach(n => done.add(n));
      cards.push({ id: names[0], title: g && names.length > 1 ? g.title : v.label, names });
    }
    return cards;
  }

  function control(v) {
    const val = state.current[v.name];
    if (v.type === 'bool') return `<label class="switch"><input type="checkbox" data-n="${v.name}" ${val ? 'checked' : ''}><span>${val ? 'Active' : 'Desactive'}</span></label>`;
    if (v.type === 'color') return `<input type="text" data-n="${v.name}" value="${val}">`;
    const r = range(v);
    return `<input type="range" data-n="${v.name}" min="${r.min}" max="${Math.max(r.max, val)}" step="${r.step}" value="${val}">` +
      `<input type="number" data-n="${v.name}" step="${r.step}" value="${fmt(val)}"><span class="unit">${v.units || ''}</span>`;
  }

  const resetBtn = n => isDirty(n) ? `<button class="reset" data-reset="${n}" title="Revenir a ${fmt(state.saved[n])}">↺ ${fmt(state.saved[n])}</button>` : '';

  function row(v, multi) {
    return `<div class="row ${isDirty(v.name) ? 'changed' : ''}" data-row="${v.name}">
      ${multi ? `<div class="rlabel"><span>${v.label}</span><span class="rst">${resetBtn(v.name)}</span></div>` : ''}
      <div class="control">${control(v)}</div>
      ${v.tooltip ? `<div class="tip">${v.tooltip}</div>` : ''}
      <div class="code">${v.name}</div>
    </div>`;
  }

  function card(c) {
    const multi = c.names.length > 1;
    const focus = state.focus[c.id] || c.names[0];
    const dirty = c.names.some(isDirty);
    return `<div class="card ${dirty ? 'changed' : ''} ${multi ? 'multi' : ''}" id="c-${c.id}" data-card="${c.id}">
      <div class="title"><span>${c.title}</span>${multi ? `<span class="count">${c.names.length} reglages</span>` : `<span class="rst">${resetBtn(c.id)}</span>`}</div>
      <div class="sk">${OraSketch.render(focus, state.current, byName(focus))}</div>
      <div class="rows">${c.names.map(n => row(byName(n), multi)).join('')}</div>
    </div>`;
  }

  function render() {
    renderMenu();
    const q = state.search.toLowerCase();
    const list = q ? state.vars.filter(v => (v.label + ' ' + v.name + ' ' + v.tooltip + ' ' + v.category + ' ' + (groupOf[v.name] ? groupOf[v.name].title : '')).toLowerCase().includes(q))
      : state.vars.filter(v => sectionOf(v) === state.section);
    const subs = {};
    for (const c of cardsFor(list)) { const v = byName(c.id); (subs[q ? v.category.replace('|', ' › ') : subOf(v)] ||= []).push(c); }
    $('#content').innerHTML = `<h2>${q ? `Recherche « ${state.search} »` : state.section}</h2>` +
      (list.length ? '' : '<p>Aucune variable.</p>') +
      Object.entries(subs).map(([g, cs]) => `${Object.keys(subs).length > 1 || g !== 'General' ? `<h3>${g}</h3>` : ''}<div class="grid">${cs.map(card).join('')}</div>`).join('');
    bind();
    updateStatus();
  }

  function drawSketch(cardEl) {
    const id = cardEl.dataset.card;
    const focus = state.focus[id] || id;
    cardEl.querySelector('.sk').innerHTML = OraSketch.render(focus, state.current, byName(focus));
    cardEl.querySelectorAll('.row').forEach(r => r.classList.toggle('focus', cardEl.classList.contains('multi') && r.dataset.row === focus));
  }

  // Met a jour sans tout redessiner (garde le focus du curseur).
  function refresh(name) {
    const rowEl = document.querySelector(`[data-row="${name}"]`);
    if (!rowEl) return;
    const cardEl = rowEl.closest('.card');
    rowEl.classList.toggle('changed', isDirty(name));
    rowEl.querySelectorAll(`input[data-n="${name}"]`).forEach(i => {
      if (document.activeElement === i) return;
      if (i.type === 'checkbox') { i.checked = state.current[name]; i.nextElementSibling.textContent = i.checked ? 'Active' : 'Desactive'; }
      else i.value = i.type === 'number' ? fmt(state.current[name]) : state.current[name];
    });
    const rst = cardEl.classList.contains('multi') ? rowEl.querySelector('.rst') : cardEl.querySelector('.title .rst');
    rst.innerHTML = resetBtn(name);
    rst.querySelectorAll('.reset').forEach(bindReset);
    cardEl.classList.toggle('changed', [...cardEl.querySelectorAll('[data-row]')].some(r => isDirty(r.dataset.row)));
    // Les croquis dependent souvent d'autres variables (ex. saut) : on les redessine tous.
    document.querySelectorAll('.card').forEach(drawSketch);
    renderMenu(); updateStatus();
  }

  function setValue(name, raw) {
    const v = byName(name);
    let x = raw;
    if (v.type === 'int' || v.type === 'float') { x = parseFloat(raw); if (!Number.isFinite(x)) return; x = clampVal(v, x); }
    state.current[name] = x;
    refresh(name);
  }

  function setFocus(name) {
    const cardEl = document.querySelector(`[data-row="${name}"]`).closest('.card');
    if (state.focus[cardEl.dataset.card] === name) return;
    state.focus[cardEl.dataset.card] = name;
    drawSketch(cardEl);
  }

  function bindReset(b) { b.onclick = () => { state.current[b.dataset.reset] = state.saved[b.dataset.reset]; refresh(b.dataset.reset); }; }

  function bind() {
    document.querySelectorAll('#content input[data-n]').forEach(i => {
      if (i.type === 'checkbox') i.onchange = () => setValue(i.dataset.n, i.checked);
      else if (i.type === 'range') i.oninput = () => setValue(i.dataset.n, i.value);
      else i.onchange = () => setValue(i.dataset.n, i.value);
    });
    // Le croquis montre la ligne survolee ou en cours de reglage.
    document.querySelectorAll('.card.multi .row').forEach(r => {
      r.onmouseenter = () => setFocus(r.dataset.row);
      r.onfocusin = () => setFocus(r.dataset.row);
    });
    document.querySelectorAll('.card').forEach(drawSketch);
    document.querySelectorAll('.reset').forEach(bindReset);
  }

  function updateStatus(msg) {
    const d = dirtyNames().length;
    $('#save').disabled = $('#discard').disabled = !d;
    $('#status').textContent = msg || (d ? `${d} modification${d > 1 ? 's' : ''} non enregistree${d > 1 ? 's' : ''}` : 'Tout est enregistre');
  }

  $('#search').oninput = e => { state.search = e.target.value.trim(); render(); };
  $('#discard').onclick = () => { state.current = { ...state.saved }; render(); };
  $('#save').onclick = async () => {
    const changes = Object.fromEntries(dirtyNames().map(n => [n, state.current[n]]));
    if (state.mode === 'perforce' && !confirm(`Soumettre ${Object.keys(changes).length} modification(s) dans Perforce (//SpaceLeague/main) ?`)) return;
    $('#save').disabled = true;
    updateStatus('Envoi…');
    const res = await fetch('api/save', { method: 'POST', body: JSON.stringify(changes) });
    const out = await res.json();
    if (res.status === 401) return showLogin('Session expiree, se reconnecter.');
    if (!out.ok) { updateStatus('Erreur : ' + out.error); $('#save').disabled = false; return; }
    await load(); render();
    updateStatus(out.change ? `Soumis dans Perforce : changelist ${out.change} — faire un sync puis relancer le PIE`
      : 'Enregistre dans DefaultGame.ini — relancer le PIE pour appliquer');
  };
  window.addEventListener('beforeunload', e => { if (dirtyNames().length) e.preventDefault(); });

  // Connexion Perforce (mode serveur).
  function showLogin(msg) {
    $('#login').hidden = false;
    $('#login-msg').textContent = msg || '';
    $('#login-user').focus();
  }
  $('#login-form').onsubmit = async e => {
    e.preventDefault();
    $('#login-msg').textContent = 'Connexion…';
    const res = await fetch('api/login', { method: 'POST', body: JSON.stringify({ user: $('#login-user').value.trim(), password: $('#login-pass').value }) });
    const out = await res.json();
    $('#login-pass').value = '';
    if (!out.ok) return ($('#login-msg').textContent = out.error);
    $('#login').hidden = true;
    start(out.user);
  };
  $('#logout').onclick = async () => { await fetch('api/logout', { method: 'POST' }); location.reload(); };

  async function start(user) {
    if (state.mode === 'perforce') { $('#who').textContent = user; $('#logout').hidden = false; }
    try { await load(); render(); }
    catch (e) { $('#status').textContent = 'Chargement impossible : ' + e.message; console.error(e); }
  }

  try {
    const me = await fetch('api/me').then(r => r.json());
    state.mode = me.mode;
    if (me.mode === 'perforce' && !me.user) showLogin();
    else start(me.user);
  } catch (e) { $('#status').textContent = 'Serveur introuvable : lancer Tools/GameplayConfigSite/Lancer.bat'; console.error(e); }
})();
