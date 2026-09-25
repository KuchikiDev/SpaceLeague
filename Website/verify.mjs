import {createRequire} from 'node:module';
import {mkdir,writeFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
const require = createRequire(import.meta.url);
const {chromium} = require('C:/Users/dylan/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const browser = await chromium.launch({executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',headless:true});
const output = new URL('./qa/', import.meta.url).pathname.replace(/^\/(.:)/, '$1');
await mkdir(output,{recursive:true});
const base = process.env.ORA_TEST_URL || 'http://127.0.0.1:4173';
const report = {url:base,checks:[],errors:[]};
const page = await browser.newPage({viewport:{width:1440,height:1000},reducedMotion:'reduce'});
page.on('pageerror', error => report.errors.push(error.message));
page.on('response', response => {if(response.status() >= 400) report.errors.push(`${response.status()} ${response.url()}`);});
async function check(name, action) {await action();report.checks.push(name);console.log('PASS',name);}
try {
  await page.goto(base,{waitUntil:'networkidle'});
  await page.screenshot({path:output+'/desktop-hero.png'});
  await check('Desktop assets and web fonts load',async()=>{assert.equal(await page.evaluate(()=>document.fonts.check('600 20px OraDisplay')),true);assert.equal(await page.evaluate(()=>document.fonts.check('400 16px OraSans')),true);assert.equal(report.errors.length,0);});
  await page.locator('#univers').scrollIntoViewIfNeeded();
  await check('Atlas mouse and keyboard navigation',async()=>{await page.locator('[data-atlas="1"]').click();assert.match(await page.locator('#atlas-title').innerText(),/chacun son temps/);await page.locator('[data-atlas="1"]').press('ArrowRight');assert.equal(await page.locator('[data-atlas="2"]').getAttribute('aria-selected'),'true');assert.match(await page.locator('#atlas-text').innerText(),/restent à définir/);});
  await page.locator('[data-atlas="0"]').click();
  await page.screenshot({path:output+'/desktop-atlas.png'});
  await page.locator('#personnages').scrollIntoViewIfNeeded();
  await page.screenshot({path:output+'/desktop-characters.png'});
  await check('All four character identities, ability tabs, Escape and focus restoration',async()=>{
    for(const key of ['raijin','keplar','andaris','pandore']) {
      await page.locator(`[data-character="${key}"]`).click();
      await page.waitForFunction(()=>document.querySelector('dialog').open);
      assert.equal((await page.locator('#dialog-name').innerText()).toLowerCase(),key);
      assert.match(await page.title(),new RegExp(key,'i'));
      await page.locator('#abilities-tab').click();
      assert.equal(await page.locator('#abilities-list .ability').count(),3);
      assert.equal(await page.locator('#identity-panel').isVisible(),false);
      if(key==='keplar') await page.screenshot({path:output+'/desktop-character-detail.png'});
      await page.keyboard.press('Escape');
      await page.waitForFunction(()=>!document.querySelector('dialog').open);
      assert.equal(await page.evaluate(()=>document.activeElement.dataset.character),key);
    }
  });
  await check('Character next and browser back/forward',async()=>{await page.locator('[data-character="raijin"]').click();await page.locator('#next-character').click();assert.equal(await page.locator('#dialog-name').innerText(),'Keplar');await page.goBack();await page.waitForFunction(()=>!document.querySelector('dialog').open);await page.goForward();await page.waitForFunction(()=>document.querySelector('dialog').open);assert.equal(await page.locator('#dialog-name').innerText(),'Keplar');await page.locator('.dialog-close').click();await page.waitForFunction(()=>!document.querySelector('dialog').open);});
  await check('Full roster disclosure',async()=>{await page.locator('.roster summary').click();assert.equal(await page.locator('.roster-columns').isVisible(),true);assert.match(await page.locator('.roster-columns').innerText(),/Magnora/);});
  await check('Three story chapters with keyboard controls',async()=>{await page.locator('button[data-chapter="1"]').click();assert.match(await page.locator('#chapter-text').innerText(),/événement récent/);await page.locator('button[data-chapter="1"]').press('End');assert.match(await page.locator('#chapter-status').innerText(),/exploration/);});
  await check('Reduced motion preference persists',async()=>{assert.equal(await page.locator('.motion-toggle').getAttribute('aria-pressed'),'true');await page.locator('.motion-toggle').click();await page.locator('.motion-toggle').click();await page.reload({waitUntil:'networkidle'});assert.equal(await page.evaluate(()=>localStorage.getItem('ora-reduce-motion')),'true');});
  await check('Direct character link reload',async()=>{await page.goto(base+'/#personnage/pandore',{waitUntil:'networkidle'});assert.equal(await page.locator('dialog').evaluate(el=>el.open),true);assert.equal(await page.locator('#dialog-name').innerText(),'Pandore');await page.locator('.dialog-close').click();assert.equal(await page.locator('dialog').evaluate(el=>el.open),false);});
  for(const width of [360,390,768,1024,1920]) {
    await page.setViewportSize({width,height:900});
    await page.goto(base,{waitUntil:'networkidle'});
    await check(`No horizontal document overflow at ${width}px`,async()=>assert.ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),`overflow at ${width}`));
  }
  await page.setViewportSize({width:390,height:844});
  await page.goto(base,{waitUntil:'networkidle'});
  await page.screenshot({path:output+'/mobile-hero.png'});
  await check('Mobile menu, atlas and character dialog',async()=>{await page.locator('.menu-toggle').click();assert.equal(await page.locator('#mobile-menu').isVisible(),true);await page.locator('#mobile-menu a[href="#univers"]').click();assert.equal(await page.locator('#mobile-menu').isVisible(),false);await page.locator('[data-atlas="2"]').click();await page.screenshot({path:output+'/mobile-atlas.png'});await page.locator('[data-character="andaris"]').click();assert.equal(await page.locator('#dialog-name').innerText(),'Andaris');await page.screenshot({path:output+'/mobile-character.png'});await page.locator('#abilities-tab').click();assert.ok(await page.locator('dialog').evaluate(el=>el.scrollWidth<=el.clientWidth));await page.locator('.dialog-close').click();await page.waitForFunction(()=>!document.querySelector('dialog').open);});
  await check('200 percent text layout remains usable',async()=>{await page.addStyleTag({content:'html{font-size:200%}'});assert.ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));await page.locator('[data-character="raijin"]').click();assert.ok(await page.locator('dialog').evaluate(el=>el.scrollWidth<=el.clientWidth));await page.locator('.dialog-close').click();});
  assert.equal(report.errors.length,0,report.errors.join('\n'));
} catch(error) {report.errors.push(error.stack);throw error;} finally {await writeFile(output+'/report.json',JSON.stringify(report,null,2));await browser.close();}

