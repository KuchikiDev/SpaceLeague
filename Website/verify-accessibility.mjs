import assert from 'node:assert/strict';
import {createRequire} from 'node:module';
import {guides,extraCharacters} from './dist/content.js';

const require=createRequire(import.meta.url);
const {chromium}=require('C:/Users/dylan/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const base=process.env.ORA_TEST_URL||'http://127.0.0.1:4173';
const routes=['/','/univers/','/personnages/','/histoire/','/personnages/roster/','/enigmes/','/enigmes/?sceau=chronis','/enigmes/?sceau=aurion','/enigmes/?sceau=ora','/compte/','/compte/historique/','/compte/collection/','/compte/classement/',...Object.keys(guides).map(path=>path+'/'),...['raijin','keplar','andaris','pandore',...Object.keys(extraCharacters)].map(key=>`/personnages/${key}/`)];
const browser=await chromium.launch({executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',headless:true});
const page=await browser.newPage({viewport:{width:1280,height:900},reducedMotion:'reduce'});
const errors=[];
const hrefs=new Set();
page.on('pageerror',error=>errors.push(error.message));
page.on('response',response=>{if(response.status()>=400&&!response.url().includes('/account-api/'))errors.push(`${response.status()} ${response.url()}`);});

try{
  for(const route of routes){
    await page.goto(base+route,{waitUntil:'networkidle'});
    await page.locator('#screen-title').waitFor();
    const issues=await page.evaluate(()=>{
      const problems=[];
      const ids=[...document.querySelectorAll('[id]')].map(node=>node.id);
      const duplicates=[...new Set(ids.filter((id,index)=>ids.indexOf(id)!==index))];
      if(duplicates.length)problems.push(`identifiants dupliqués : ${duplicates.join(', ')}`);
      for(const node of document.querySelectorAll('[aria-controls],[aria-labelledby]')){
        for(const attr of ['aria-controls','aria-labelledby']){
          const value=node.getAttribute(attr);
          if(!value)continue;
          for(const id of value.split(/\s+/))if(!document.getElementById(id))problems.push(`${attr} cible absente : ${id}`);
        }
      }
      for(const list of document.querySelectorAll('[role="tablist"]')){
        const selected=[...list.querySelectorAll('[role="tab"]')].filter(tab=>tab.getAttribute('aria-selected')==='true');
        if(selected.length!==1)problems.push(`tablist avec ${selected.length} onglet sélectionné`);
      }
      for(const node of document.querySelectorAll('button,a[href]')){
        if(node.closest('[hidden]')||node.getAttribute('aria-hidden')==='true')continue;
        const name=(node.getAttribute('aria-label')||node.textContent||'').trim();
        if(!name)problems.push(`${node.tagName.toLowerCase()} sans nom accessible`);
      }
      if(document.querySelectorAll('main h1').length!==1)problems.push(`${document.querySelectorAll('main h1').length} titre h1 dans main`);
      return problems;
    });
    assert.deepEqual(issues,[],`${route}: ${issues.join('; ')}`);
    for(const href of await page.evaluate(()=>[...document.querySelectorAll('a[href^="/"]')].map(link=>new URL(link.href).pathname)))hrefs.add(href);
  }
  for(const href of hrefs){
    const response=await fetch(base+href,{redirect:'manual'});
    assert.ok(response.status<400,`Lien interne cassé : ${href} (${response.status})`);
  }
  assert.deepEqual(errors,[]);
  console.log(`PASS accessibilité structurelle : ${routes.length} routes, relations ARIA, noms accessibles et liens internes`);
}finally{
  await browser.close();
}
