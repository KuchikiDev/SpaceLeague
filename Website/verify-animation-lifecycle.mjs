import {createRequire} from 'node:module';
import assert from 'node:assert/strict';
const require=createRequire(import.meta.url);
const {chromium}=require('C:/Users/dylan/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const browser=await chromium.launch({executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',args:['--enable-unsafe-swiftshader']});
const page=await browser.newPage({reducedMotion:'reduce'});
await page.addInitScript(()=>{
  window.frameCallbacks=0;
  const raf=window.requestAnimationFrame.bind(window);
  window.requestAnimationFrame=cb=>raf(t=>{window.frameCallbacks++;cb(t);});
});
const base=process.env.ORA_TEST_URL||'http://127.0.0.1:4173';
async function frames(){
  const initial=await page.evaluate(()=>window.frameCallbacks);
  await page.waitForTimeout(400);
  return await page.evaluate(()=>window.frameCallbacks)-initial;
}
try{
  for(const route of ['/','/personnages/keplar/']){
    await page.goto(base+route,{waitUntil:'networkidle'});
    if(route!=='/')await page.locator('[data-state=ready]').waitFor();
    assert.equal(await frames(),0,route+' reduced motion schedules no callbacks');
    await page.locator('#motion').click();
    assert.ok(await frames()>0,route+' resumes animation');
    await page.locator('#motion').click();
    assert.equal(await frames(),0,route+' pause cancels callbacks');
    await page.locator('#motion').click();
    await page.evaluate(()=>{Object.defineProperty(document,'hidden',{configurable:true,value:true});document.dispatchEvent(new Event('visibilitychange'));});
    assert.equal(await frames(),0,route+' hidden visibility cancels callbacks');
    await page.evaluate(()=>{delete document.hidden;document.dispatchEvent(new Event('visibilitychange'));});
    assert.ok(await frames()>0,route+' visible resumes');
    await page.locator('#motion').click();
  }
  await page.locator('a[href="/jeu/"]').first().click();
  await page.waitForTimeout(200);
  assert.equal(await frames(),0,'old character loop disposed on navigation');
  console.log('PASS canvas and WebGL lifecycle: zero callbacks reduced/paused/hidden/disposed; animation resumes');
}finally{await browser.close();}
