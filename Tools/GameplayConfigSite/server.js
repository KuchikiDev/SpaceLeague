// Site local de reglage des variables de gameplay ORA.
// Lit les definitions dans GameplayVariablesSettings.h (plugin GameplayVariables)
// et les valeurs dans Config/DefaultGame.ini, puis reecrit la section du .ini a l'enregistrement.
// Lancement : node Tools/GameplayConfigSite/server.js   puis http://localhost:5178

const http = require('http');
const fs = require('fs');
const path = require('path');

const PORT = Number(process.env.PORT || 5178);
const SECTION = '[/Script/GameplayVariables.GameplayVariablesSettings]';

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

const PROJECT = findProject();
const HEADER = path.join(PROJECT, 'Plugins', 'GameplayVariables', 'Source', 'GameplayVariables', 'Public', 'GameplayVariablesSettings.h');
const INI = path.join(PROJECT, 'Config', 'DefaultGame.ini');

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

function parseHeader() {
  const src = fs.readFileSync(HEADER, 'utf8');
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

function readIni() {
  const text = fs.readFileSync(INI, 'utf8');
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

function save(changes) {
  const vars = Object.fromEntries(parseHeader().map(v => [v.name, v]));
  const ini = readIni();
  const values = { ...ini.values };
  for (const [name, value] of Object.entries(changes)) {
    const def = vars[name];
    if (!def || def.type === 'other') throw new Error(`Variable inconnue : ${name}`);
    if (def.type === 'color') { values[name] = String(value); continue; }
    if (def.type !== 'bool' && !Number.isFinite(Number(value))) throw new Error(`Valeur invalide pour ${name}`);
    values[name] = formatValue(value, def.type);
  }
  const body = Object.entries(values).map(([k, v]) => `${k}=${v}`);
  const lines = ini.lines.slice();
  if (ini.start < 0) {
    lines.push('', SECTION, ...body, '');
  } else {
    // Garde une ligne vide avant la section suivante, comme Unreal.
    const tail = [''];
    lines.splice(ini.start + 1, ini.end - ini.start - 1, ...body, ...tail);
  }
  const eol = fs.readFileSync(INI, 'utf8').includes('\r\n') ? '\r\n' : '\n';
  fs.writeFileSync(INI + '.bak', fs.readFileSync(INI));
  fs.writeFileSync(INI, lines.join(eol).replace(/(\r?\n){3,}/g, eol + eol));
}

const server = http.createServer((req, res) => {
  try {
    if (req.method === 'GET' && req.url === '/api/variables') {
      const payload = { project: PROJECT, ini: INI, variables: parseHeader(), values: readIni().values };
      res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
      return res.end(JSON.stringify(payload));
    }
    if (req.method === 'POST' && req.url === '/api/save') {
      let data = '';
      req.on('data', c => (data += c));
      req.on('end', () => {
        try {
          save(JSON.parse(data || '{}'));
          res.writeHead(200, { 'Content-Type': 'application/json' });
          res.end('{"ok":true}');
        } catch (e) {
          res.writeHead(400, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ ok: false, error: e.message }));
        }
      });
      return;
    }
    const file = { '/': 'index.html', '/index.html': 'index.html', '/sketches.js': 'sketches.js', '/app.js': 'app.js' }[req.url];
    if (req.method === 'GET' && file) {
      const type = file.endsWith('.js') ? 'text/javascript' : 'text/html';
      res.writeHead(200, { 'Content-Type': `${type}; charset=utf-8` });
      return res.end(fs.readFileSync(path.join(__dirname, file)));
    }
    res.writeHead(404); res.end();
  } catch (e) {
    res.writeHead(500, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end(e.message);
  }
});

server.listen(PORT, '127.0.0.1', () => {
  console.log(`Variables ORA : http://localhost:${PORT}`);
  console.log(`Fichier modifie : ${INI}`);
});
