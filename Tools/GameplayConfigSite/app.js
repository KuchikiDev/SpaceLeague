// Interface : menus par categorie, une carte par variable avec son croquis.
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
    const res = await fetch('/api/variables');
    if (!res.ok) throw new Error(await res.text());
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

  function card(v) {
    const val = state.current[v.name];
    let control;
    if (v.type === 'bool') control = `<label class="switch"><input type="checkbox" data-n="${v.name}" ${val ? 'checked' : ''}> ${val ? 'Active' : 'Desactive'}</label>`;
    else if (v.type === 'color') control = `<input type="text" data-n="${v.name}" value="${val}">`;
    else {
      const r = range(v);
      control = `<input type="range" data-n="${v.name}" min="${r.min}" max="${Math.max(r.max, val)}" step="${r.step}" value="${val}">` +
        `<input type="number" data-n="${v.name}" step="${r.step}" value="${fmt(val)}"><span class="unit">${v.units || ''}</span>`;
    }
    return `<div class="card ${isDirty(v.name) ? 'changed' : ''}" id="c-${v.name}">
      <div class="title"><span>${v.label}</span>${isDirty(v.name) ? `<button class="reset" data-reset="${v.name}" title="Revenir a ${fmt(state.saved[v.name])}">↺ ${fmt(state.saved[v.name])}</button>` : ''}</div>
      <div class="sk">${OraSketch.render(v.name, state.current, v)}</div>
      <div class="control">${control}</div>
      ${v.tooltip ? `<div class="tip">${v.tooltip}</div>` : ''}
      <div class="code">${v.name}</div>
    </div>`;
  }

  function render() {
    renderMenu();
    const q = state.search.toLowerCase();
    const list = q ? state.vars.filter(v => (v.label + ' ' + v.name + ' ' + v.tooltip + ' ' + v.category).toLowerCase().includes(q))
      : state.vars.filter(v => sectionOf(v) === state.section);
    const groups = {};
    for (const v of list) (groups[q ? v.category.replace('|', ' › ') : subOf(v)] ||= []).push(v);
    $('#content').innerHTML = `<h2>${q ? `Recherche « ${state.search} »` : state.section}</h2>` +
      (list.length ? '' : '<p>Aucune variable.</p>') +
      Object.entries(groups).map(([g, vs]) => `${Object.keys(groups).length > 1 || g !== 'General' ? `<h3>${g}</h3>` : ''}<div class="grid">${vs.map(card).join('')}</div>`).join('');
    bind();
    updateStatus();
  }

  // Met a jour une carte sans tout redessiner (garde le focus du curseur).
  function refreshCard(name) {
    const v = state.vars.find(x => x.name === name);
    const el = document.getElementById('c-' + name);
    if (!el) return;
    el.classList.toggle('changed', isDirty(name));
    el.querySelector('.sk').innerHTML = OraSketch.render(name, state.current, v);
    el.querySelectorAll(`input[data-n="${name}"]`).forEach(i => {
      if (document.activeElement === i) return;
      if (i.type === 'checkbox') i.checked = state.current[name];
      else i.value = i.type === 'number' ? fmt(state.current[name]) : state.current[name];
    });
    // Les croquis partages (ex. saut) dependent d'autres variables : on les redessine aussi.
    document.querySelectorAll('.card').forEach(c => {
      const n = c.id.slice(2);
      if (n !== name) c.querySelector('.sk').innerHTML = OraSketch.render(n, state.current, state.vars.find(x => x.name === n));
    });
    const title = el.querySelector('.title');
    const reset = title.querySelector('.reset');
    if (isDirty(name) && !reset) title.insertAdjacentHTML('beforeend', `<button class="reset" data-reset="${name}">↺ ${fmt(state.saved[name])}</button>`), bindReset(title.querySelector('.reset'));
    if (!isDirty(name) && reset) reset.remove();
    renderMenu(); updateStatus();
  }

  function setValue(name, raw) {
    const v = state.vars.find(x => x.name === name);
    let x = raw;
    if (v.type === 'int' || v.type === 'float') { x = parseFloat(raw); if (!Number.isFinite(x)) return; x = clampVal(v, x); }
    state.current[name] = x;
    refreshCard(name);
  }

  function bindReset(b) { b.onclick = () => { state.current[b.dataset.reset] = state.saved[b.dataset.reset]; refreshCard(b.dataset.reset); }; }

  function bind() {
    document.querySelectorAll('#content input[data-n]').forEach(i => {
      if (i.type === 'checkbox') i.onchange = () => { setValue(i.dataset.n, i.checked); i.parentElement.lastChild.textContent = i.checked ? ' Active' : ' Desactive'; };
      else if (i.type === 'range') i.oninput = () => setValue(i.dataset.n, i.value);
      else i.onchange = () => setValue(i.dataset.n, i.value);
    });
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
    const res = await fetch('/api/save', { method: 'POST', body: JSON.stringify(changes) });
    const out = await res.json();
    if (!out.ok) return updateStatus('Erreur : ' + out.error);
    await load(); render();
    updateStatus('Enregistre dans DefaultGame.ini — relancer le PIE pour appliquer');
  };
  window.addEventListener('beforeunload', e => { if (dirtyNames().length) e.preventDefault(); });

  try { await load(); render(); }
  catch (e) { $('#status').textContent = 'Serveur introuvable : lancer Tools/GameplayConfigSite/Lancer.bat'; console.error(e); }
})();
