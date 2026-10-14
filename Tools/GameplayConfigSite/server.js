// Site de reglage des variables de gameplay ORA.
// Lit les definitions dans GameplayVariablesSettings.h (plugin GameplayVariables)
// et les valeurs dans Config/DefaultGame.ini, puis reecrit la section du .ini a l'enregistrement.
//
// Deux modes :
// - local (defaut) : fichiers du projet sur le disque, pas de connexion.
//     node Tools/GameplayConfigSite/server.js   puis http://localhost:5178
// - Perforce (variable ORA_P4PORT definie, utilise sur le VPS) : connexion avec son compte Perforce,
//   lecture dans le depot, et "Enregistrer" soumet un changelist sous le nom de l'utilisateur.

const http = require('http');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { execFile } = require('child_process');

const PORT = Number(process.env.PORT || 5178);
const HOST = process.env.HOST || '127.0.0.1';
const SECTION = '[/Script/GameplayVariables.GameplayVariablesSettings]';
const HEADER_REL = 'Plugins/GameplayVariables/Source/GameplayVariables/Public/GameplayVariablesSettings.h';
const INI_REL = 'Config/DefaultGame.ini';

// --- mode Perforce -----------------------------------------------------------
const P4PORT = process.env.ORA_P4PORT || '';
const P4_STREAM = process.env.P4_STREAM || '//SpaceLeague/main';
const P4_PROJECT = process.env.P4_PROJECT || 'ProjetUE5/MovementORA 5.8';
const STATE_DIR = process.env.STATE_DIRECTORY || path.join(__dirname, '.state');
const SESSION_HOURS = 12;
const sessions = new Map(); // sid -> { user, ticket, expires }
const loginFailures = new Map(); // ip -> { count, until }

const depotPath = rel => `${P4_STREAM}/${P4_PROJECT}/${rel}`;

function p4(args, { user, ticket, client, input } = {}) {
  const full = ['-p', P4PORT, ...(user ? ['-u', user] : []), ...(client ? ['-c', client] : []), ...args];
  const env = { ...process.env, HOME: STATE_DIR, P4TICKETS: path.join(STATE_DIR, '.p4tickets'), P4ENVIRO: path.join(STATE_DIR, '.p4enviro'), P4CONFIG: '' };
  // Le ticket passe par l'environnement, pas par la ligne de commande (visible dans ps).
  if (ticket) env.P4PASSWD = ticket;
  return new Promise((resolve, reject) => {
    const child = execFile('p4', full, { env, maxBuffer: 16 * 1024 * 1024 }, (err, stdout, stderr) => {
      if (err) return reject(new Error((stderr || stdout || err.message).trim()));
      resolve(stdout);
    });
    if (input !== undefined) child.stdin.end(input);
  });
}

async function p4Login(user, password) {
  const out = await p4(['login', '-p'], { user, input: password + '\n' });
  const ticket = out.trim().split(/\r?\n/).pop().trim();
  if (!/^[0-9A-F]{32}$/i.test(ticket)) throw new Error('Connexion Perforce refusee');
  return ticket;
}

const clientName = user => `ora-variables-${user.replace(/[^\w.-]/g, '_')}`;

// Workspace dedie au site, par utilisateur, sur le stream main.
async function ensureClient(s) {
  const name = clientName(s.user);
  const root = path.join(STATE_DIR, 'ws', name);
  fs.mkdirSync(root, { recursive: true });
  const spec = `Client: ${name}\nOwner: ${s.user}\nHost:\nRoot: ${root}\n` +
    `Options: allwrite clobber nocompress unlocked nomodtime normdir\nLineEnd: local\nStream: ${P4_STREAM}\n` +
    `Description:\n\tWorkspace du site des variables de gameplay.\n`;
  await p4(['client', '-i'], { user: s.user, ticket: s.ticket, input: spec });
  return { name, root };
}

// --- sources ----------------------------------------------------------------
function findProject() {
  if (process.env.ORA_PROJECT) return process.env.ORA_PROJECT;
  let dir = __dirname;
  for (;;) {
    const candidate = path.join(dir, 'ProjetUE5', 'MovementORA 5.8');
    if (fs.existsSync(path.join(candidate, 'MovementORA.uproject'))) return candidate;
    const parent = path.dirname(dir);
    if (parent === dir) throw new Error('Projet introuvable : definir ORA_PROJECT vers "ProjetUE5/MovementORA 5.8".');
    dir = parent;
  }
}

const PROJECT = P4PORT ? null : findProject();
const localPath = rel => path.join(PROJECT, ...rel.split('/'));

async function readSource(rel, s) {
  if (!P4PORT) return fs.readFileSync(localPath(rel), 'utf8');
  return p4(['print', '-q', depotPath(rel)], { user: s.user, ticket: s.ticket });
}

// Variables sans Category/DisplayName dans le header : rangement et nom lisible pour le site.
const FALLBACK = {
  MatchStartSeconds: ['Match|Duree', 'Duree du match'],
};
const FALLBACK_LABELS = {
  CurvedShotScaleMultiplier: 'Taille de la balle en tir enroule',
  CurvedShotAimTraceDistance: 'Distance de visee',
  CurvedShotInputChangeSpeed: 'Reactivite de la courbe au stick',
  CurvedShotCollisionSampleCount: 'Points de test de collision',
  CurvedShotPostObstacleOpacityMultiplier: 'Opacite apres un obstacle',
  CurvedShotVisualStartOffset: 'Decalage du debut du trace',
  CurvedShotInitialStraightDistance: 'Distance en ligne droite au depart',
  CurvedShotInitialStraightDistanceRatio: 'Part en ligne droite au depart',
};

function parseMeta(text) {
  const meta = {};
  const re = /(\w+)\s*=\s*("((?:[^"\\]|\\.)*)"|[^,)]+)/g;
  let m;
  while ((m = re.exec(text))) meta[m[1]] = m[3] !== undefined ? m[3] : m[2].trim();
  return meta;
}

function parseHeader(src) {
  const vars = [];
  const re = /UPROPERTY\(([\s\S]*?)\)\s*\n\s*([\w<>:]+)\s+(\w+)\s*=\s*([^;]+);/g;
  let m;
  while ((m = re.exec(src))) {
    const spec = m[1];
    if (!/\bConfig\b/.test(spec)) continue;
    const metaText = (spec.match(/meta\s*=\s*\(([\s\S]*)$/) || [, ''])[1];
    const meta = parseMeta(metaText);
    const fallback = FALLBACK[m[3]] || (m[3].startsWith('CurvedShot') ? ['Balle - Tir enroule|Avance'] : ['Divers']);
    const cat = (spec.match(/Category\s*=\s*"([^"]+)"/) || [, fallback[0]])[1];
    if (!meta.DisplayName && fallback[1]) meta.DisplayName = fallback[1];
    if (!meta.DisplayName && FALLBACK_LABELS[m[3]]) meta.DisplayName = FALLBACK_LABELS[m[3]];
    const cppType = m[2];
    const type = cppType === 'bool' ? 'bool' : cppType === 'int32' ? 'int' : cppType === 'float' ? 'float'
      : cppType === 'FLinearColor' ? 'color' : 'other';
    vars.push({
      name: m[3], type, category: cat,
      label: meta.DisplayName || m[3], tooltip: meta.ToolTip || '',
      units: meta.Units || '',
      min: num(meta.UIMin ?? meta.ClampMin), max: num(meta.UIMax ?? meta.ClampMax),
      clampMin: num(meta.ClampMin), clampMax: num(meta.ClampMax),
      defaultRaw: m[4].trim(),
    });
  }
  return vars;
}

function num(v) { const n = parseFloat(v); return Number.isFinite(n) ? n : null; }

function parseIni(text) {
  const lines = text.split(/\r?\n/);
  const start = lines.findIndex(l => l.trim() === SECTION);
  const values = {};
  if (start < 0) return { lines, start, end: start, values };
  let end = start + 1;
  while (end < lines.length && !lines[end].trim().startsWith('[')) {
    const kv = lines[end].match(/^([^=;]+)=(.*)$/);
    if (kv) values[kv[1].trim()] = kv[2].trim();
    end++;
  }
  return { lines, start, end, values };
}

function formatValue(v, type) {
  if (type === 'bool') return v ? 'True' : 'False';
  if (type === 'int') return String(Math.round(Number(v)));
  if (type === 'float') return Number(v).toFixed(6);
  return String(v);
}

// Applique les changements sur le texte du .ini ; ne touche que les cles modifiees.
function applyChanges(iniText, headerText, changes) {
  const vars = Object.fromEntries(parseHeader(headerText).map(v => [v.name, v]));
  const ini = parseIni(iniText);
  const values = { ...ini.values };
  for (const [name, value] of Object.entries(changes)) {
    const def = vars[name];
    if (!def || def.type === 'other') throw new Error(`Variable inconnue : ${name}`);
    if (def.type === 'color') { values[name] = String(value).replace(/[\r\n]/g, ''); continue; }
    if (def.type !== 'bool' && !Number.isFinite(Number(value))) throw new Error(`Valeur invalide pour ${name}`);
    values[name] = formatValue(value, def.type);
  }
  const body = Object.entries(values).map(([k, v]) => `${k}=${v}`);
  const lines = ini.lines.slice();
  if (ini.start < 0) lines.push('', SECTION, ...body, '');
  else lines.splice(ini.start + 1, ini.end - ini.start - 1, ...body, ''); // ligne vide avant la section suivante, comme Unreal
  const eol = iniText.includes('\r\n') ? '\r\n' : '\n';
  return lines.join(eol).replace(/(\r?\n){3,}/g, eol + eol);
}

function describe(changes, before) {
  return Object.entries(changes).map(([k, v]) => `${k}: ${before[k] ?? '(defaut)'} -> ${v}`).join('\n');
}

async function save(changes, s) {
  if (!Object.keys(changes).length) return {};
  if (!P4PORT) {
    const iniPath = localPath(INI_REL);
    const iniText = fs.readFileSync(iniPath, 'utf8');
    fs.writeFileSync(iniPath + '.bak', iniText);
    fs.writeFileSync(iniPath, applyChanges(iniText, fs.readFileSync(localPath(HEADER_REL), 'utf8'), changes));
    return {};
  }
  const auth = { user: s.user, ticket: s.ticket };
  const ws = await ensureClient(s);
  const c = { ...auth, client: ws.name };
  const file = depotPath(INI_REL);
  const local = path.join(ws.root, ...P4_PROJECT.split('/'), ...INI_REL.split('/'));
  await p4(['revert', '-k', '//...'], c).catch(() => {});
  await p4(['sync', '-f', file], c);
  await p4(['edit', file], c);
  try {
    const iniText = fs.readFileSync(local, 'utf8');
    const before = parseIni(iniText).values;
    fs.writeFileSync(local, applyChanges(iniText, await readSource(HEADER_REL, s), changes));
    const desc = `chore: variables de gameplay (site)\n\n${describe(changes, before)}\n\nModifie depuis le site des variables par ${s.user}.`;
    const out = await p4(['submit', '-d', desc, file], c);
    const cl = (out.match(/Change (\d+) submitted/) || [])[1];
    return { change: cl };
  } catch (e) {
    await p4(['revert', file], c).catch(() => {});
    throw e;
  }
}

// --- HTTP -------------------------------------------------------------------
function cookies(req) {
  return Object.fromEntries((req.headers.cookie || '').split(';').map(c => c.trim().split('=')).filter(p => p[0]).map(([k, ...v]) => [k, decodeURIComponent(v.join('='))]));
}

function session(req) {
  if (!P4PORT) return { user: 'local' };
  const s = sessions.get(cookies(req).ora_vars);
  if (!s || s.expires < Date.now()) return null;
  return s;
}

const clientIp = req => (req.headers['x-real-ip'] || req.socket.remoteAddress || '').toString();

function readBody(req) {
  return new Promise((resolve, reject) => {
    let data = '';
    req.on('data', c => { data += c; if (data.length > 1e6) req.destroy(); });
    req.on('end', () => { try { resolve(JSON.parse(data || '{}')); } catch (e) { reject(new Error('JSON invalide')); } });
  });
}

function json(res, code, obj, headers = {}) {
  res.writeHead(code, { 'Content-Type': 'application/json; charset=utf-8', 'Cache-Control': 'no-store', ...headers });
  res.end(JSON.stringify(obj));
}

const STATIC = { '/': 'index.html', '/index.html': 'index.html', '/sketches.js': 'sketches.js', '/app.js': 'app.js' };

const server = http.createServer(async (req, res) => {
  const url = req.url.split('?')[0];
  try {
    if (req.method === 'GET' && STATIC[url]) {
      const file = STATIC[url];
      res.writeHead(200, { 'Content-Type': `${file.endsWith('.js') ? 'text/javascript' : 'text/html'}; charset=utf-8`, 'Cache-Control': 'no-cache' });
      return res.end(fs.readFileSync(path.join(__dirname, file)));
    }

    if (url === '/api/me' && req.method === 'GET') {
      const s = session(req);
      return json(res, 200, { mode: P4PORT ? 'perforce' : 'local', user: s ? s.user : null });
    }

    if (url === '/api/login' && req.method === 'POST' && P4PORT) {
      const ip = clientIp(req);
      const f = loginFailures.get(ip);
      if (f && f.count >= 5 && f.until > Date.now()) return json(res, 429, { ok: false, error: 'Trop d\'essais, reessayer dans quelques minutes.' });
      const { user, password } = await readBody(req);
      if (!user || !password || !/^[\w.@-]{1,64}$/.test(user)) return json(res, 400, { ok: false, error: 'Identifiant ou mot de passe manquant' });
      try {
        const ticket = await p4Login(user, String(password));
        loginFailures.delete(ip);
        const sid = crypto.randomBytes(32).toString('hex');
        sessions.set(sid, { user, ticket, expires: Date.now() + SESSION_HOURS * 3600e3 });
        return json(res, 200, { ok: true, user }, { 'Set-Cookie': `ora_vars=${sid}; HttpOnly; Secure; SameSite=Strict; Path=/; Max-Age=${SESSION_HOURS * 3600}` });
      } catch (e) {
        const cur = loginFailures.get(ip) || { count: 0 };
        loginFailures.set(ip, { count: cur.count + 1, until: Date.now() + 5 * 60e3 });
        return json(res, 401, { ok: false, error: 'Connexion Perforce refusee' });
      }
    }

    if (url === '/api/logout' && req.method === 'POST') {
      sessions.delete(cookies(req).ora_vars);
      return json(res, 200, { ok: true }, { 'Set-Cookie': 'ora_vars=; HttpOnly; Secure; SameSite=Strict; Path=/; Max-Age=0' });
    }

    const s = session(req);
    if (url.startsWith('/api/') && !s) return json(res, 401, { ok: false, error: 'Connexion requise' });

    if (url === '/api/variables' && req.method === 'GET') {
      const [header, ini] = await Promise.all([readSource(HEADER_REL, s), readSource(INI_REL, s)]);
      return json(res, 200, { mode: P4PORT ? 'perforce' : 'local', variables: parseHeader(header), values: parseIni(ini).values });
    }

    if (url === '/api/save' && req.method === 'POST') {
      try {
        const out = await save(await readBody(req), s);
        return json(res, 200, { ok: true, ...out });
      } catch (e) {
        if (/session|password|ticket/i.test(e.message)) sessions.delete(cookies(req).ora_vars);
        return json(res, 400, { ok: false, error: e.message });
      }
    }

    res.writeHead(404); res.end();
  } catch (e) {
    if (P4PORT && /session|password invalid|ticket/i.test(e.message)) {
      sessions.delete(cookies(req).ora_vars);
      return json(res, 401, { ok: false, error: 'Session Perforce expiree, se reconnecter' });
    }
    json(res, 500, { ok: false, error: e.message });
  }
});

server.listen(PORT, HOST, () => {
  console.log(`Variables ORA : http://${HOST}:${PORT}`);
  console.log(P4PORT ? `Mode Perforce : ${P4PORT} ${P4_STREAM}/${P4_PROJECT}` : `Mode local : ${PROJECT}`);
});
