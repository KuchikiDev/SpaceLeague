import assert from 'node:assert/strict';
import {createRequire} from 'node:module';
import {mkdir} from 'node:fs/promises';

const require=createRequire(import.meta.url);
const {chromium}=require('C:/Users/dylan/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const base=process.env.ORA_TEST_URL||'http://127.0.0.1:4173';
const browser=await chromium.launch({executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',headless:true});
const page=await browser.newPage({viewport:{width:1440,height:900},reducedMotion:'no-preference'});
const errors=[];
page.on('pageerror',error=>errors.push(error.message));
page.on('response',response=>{if(response.status()>=400&&!response.url().includes('/account-api/'))errors.push(`${response.status()} ${response.url()}`);});
await mkdir('qa/enigmes',{recursive:true});

const open=async key=>{await page.goto(`${base}/enigmes/?sceau=${key}`,{waitUntil:'networkidle'});await page.locator('.puzzle-stage').waitFor();};
const solved=async key=>{await page.locator('.puzzle-success:not([hidden])').waitFor({timeout:15000});assert.ok(await page.evaluate(k=>Boolean(JSON.parse(localStorage.getItem('ora-archipel')).solved[k]),key),`progression absente : ${key}`);await page.screenshot({path:`qa/enigmes/${key}.png`});};
const click=async(selector,times=1)=>{for(let i=0;i<times;i++)await page.locator(selector).click();};

try{
  await page.goto(base+'/enigmes/',{waitUntil:'networkidle'});
  assert.equal(await page.locator('.island').count(),9);
  assert.equal(await page.locator('.bridge.live').count(),0);
  assert.equal(await page.locator('.seal-count').first().innerText(),'0/9');
  await page.screenshot({path:'qa/enigmes/archipel-vide.png'});

  // Raijin: fire only when the line is clear, three times in a row.
  await open('raijin');
  for(let shot=0;shot<3;shot++){
    await page.evaluate(()=>new Promise(resolve=>{const stage=document.querySelector('.puzzle-stage');const wait=()=>stage.dataset.clear==='true'?(stage.querySelector('[data-fire]').click(),resolve()):requestAnimationFrame(wait);wait();}));
    await page.waitForTimeout(150);
  }
  await solved('raijin');

  // Keplar: left, right, right, straight — and a fourth curve is refused.
  await open('keplar');
  await click('[data-segment="0"]');await click('[data-segment="1"]',2);await click('[data-segment="2"]',2);await click('[data-segment="3"]');
  assert.equal(await page.locator('[data-fire]').isDisabled(),true,'Expansion should forbid four curves');
  await click('[data-segment="3"]',2);
  await click('[data-fire]');await solved('keplar');

  // Andaris: three right readings in a row.
  await open('andaris');
  for(let round=0;round<3;round++){
    const answer=await page.locator('.puzzle-stage').getAttribute('data-answer');
    await click(`[data-choice="${answer}"]`);
    if(round<2){await page.locator('[data-next]:not([hidden])').waitFor();await click('[data-next]');}
  }
  await solved('andaris');

  // Pandore: pressing the original marks again lifts every curse.
  await open('pandore');
  for(const rune of [1,6,8,11,14])await click(`[data-rune="${rune}"]`);
  await solved('pandore');

  // Obsidia: four mirrors to lean left.
  await open('obsidia');
  assert.match(await page.locator('[data-beam]').innerText(),/interrompu/);
  for(const cell of ['2,2','2,3','6,1','4,3'])await click(`[data-cell="${cell}"]`);
  await solved('obsidia');

  // Aurion: a wrong word first, then the horizon.
  await open('aurion');
  await page.locator('[data-answer]').fill('lumiere');await click('[data-submit]');
  assert.match(await page.locator('.puzzle-status').innerText(),/ne révèle pas/);
  await page.locator('[data-answer]').fill('Horizon');await page.locator('[data-answer]').press('Enter');
  await solved('aurion');

  // Vaalbara: shortest solution computed from the board the page shows.
  await open('vaalbara');
  const path=await page.evaluate(()=>{
    const start=document.querySelector('.puzzle-stage').dataset.board,goal='123456780',near=i=>[i-3,i+3,i%3?i-1:-1,i%3<2?i+1:-1].filter(j=>j>=0&&j<9);
    const prev=new Map([[start,null]]),queue=[start];
    for(let head=0;head<queue.length;head++){const s=queue[head];if(s===goal)break;const b=s.indexOf('0');for(const j of near(b)){const a=[...s];[a[b],a[j]]=[a[j],a[b]];const t=a.join('');if(!prev.has(t)){prev.set(t,[s,s[j]]);queue.push(t);}}}
    const moves=[];for(let s=goal;prev.get(s);s=prev.get(s)[0])moves.unshift(prev.get(s)[1]);return moves;
  });
  assert.ok(path.length>=10,'the shuffle should not be trivial');
  for(const tile of path)await click(`[data-tile="${tile}"]`);
  await solved('vaalbara');

  // Chronis: Observation, Lumière, Origine in the window.
  await open('chronis');
  await click('[data-turn="0"][data-step="1"]',2);await click('[data-turn="1"][data-step="1"]',4);await click('[data-turn="2"][data-step="-1"]',4);
  assert.match(await page.locator('[data-window]').innerText(),/Andaris · Aurion · Vaalbara/);
  await solved('chronis');

  // Magnora: a failed launch, then the right polarities.
  await open('magnora');
  await click('[data-launch]');await page.waitForFunction(()=>/bloc|hors|loin/.test(document.querySelector('.puzzle-status').textContent));
  for(const magnet of [0,1,4,5])await click(`[data-magnet="${magnet}"]`);
  await click('[data-launch]');await solved('magnora');

  // The archipelago is reconnected and the ORA seal can be awakened.
  await page.goto(base+'/enigmes/',{waitUntil:'networkidle'});
  assert.equal(await page.locator('.seal-count').first().innerText(),'9/9');
  assert.equal(await page.locator('.island.solved').count(),9);
  assert.equal(await page.locator('.bridge.live').count(),18);
  assert.equal(await page.locator('.archipelago-core.awake').count(),1);
  await page.screenshot({path:'qa/enigmes/archipel-relie.png'});
  await page.locator('.archipelago-core').click();
  await page.locator('#final-answer').fill('énergie');await click('#final-submit');
  assert.match(await page.locator('.puzzle-status').innerText(),/silencieux/);
  await page.locator('#final-answer').fill('aura');await click('#final-submit');
  await page.locator('.certificate').waitFor();
  assert.equal(await page.evaluate(()=>document.documentElement.classList.contains('aura')),true);
  await page.screenshot({path:'qa/enigmes/sceau-ora.png'});
  await click('#aura-toggle');
  assert.equal(await page.evaluate(()=>document.documentElement.classList.contains('aura')),false);
  await click('#aura-toggle');

  // Progress survives a reload and can be erased after confirmation.
  await page.reload({waitUntil:'networkidle'});
  assert.equal(await page.locator('.certificate').count(),1);
  await page.goto(base+'/enigmes/',{waitUntil:'networkidle'});
  await click('#reset-enigmas');assert.match(await page.locator('#reset-enigmas').innerText(),/Confirmer/);
  await click('#reset-enigmas');
  assert.equal(await page.locator('.island.solved').count(),0);
  assert.equal(await page.evaluate(()=>document.documentElement.classList.contains('aura')),false);

  // Locked core and mobile layouts.
  await page.goto(base+'/enigmes/?sceau=ora',{waitUntil:'networkidle'});
  assert.equal(await page.locator('.final-locked li').count(),9);
  await page.setViewportSize({width:390,height:844});
  for(const route of ['/','/enigmes/','/enigmes/?sceau=obsidia','/enigmes/?sceau=magnora','/enigmes/?sceau=chronis']){
    await page.goto(base+route,{waitUntil:'networkidle'});
    assert.ok(await page.evaluate(()=>document.querySelector('#app').scrollWidth<=innerWidth),`débordement mobile ${route}`);
  }
  await page.screenshot({path:'qa/enigmes/mobile-chronis.png',fullPage:true});
  await page.goto(base+'/enigmes/',{waitUntil:'networkidle'});await page.screenshot({path:'qa/enigmes/mobile-archipel.png',fullPage:true});

  assert.deepEqual(errors,[]);
  console.log('PASS archipel : neuf énigmes résolues, sceau d’ORA, mode Aura, persistance, réinitialisation et mobile');
}finally{
  await browser.close();
}
