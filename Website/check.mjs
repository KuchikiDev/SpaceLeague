import {readdir} from 'node:fs/promises';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const root=new URL('./',import.meta.url);
for(const dir of [root,new URL('src/',root),new URL('dist/',root)])for(const entry of await readdir(dir)){
  if(!/\.m?js$/.test(entry))continue;
  const result=spawnSync(process.execPath,['--check',fileURLToPath(new URL(entry,dir))],{stdio:'inherit'});
  if(result.status!==0)process.exit(result.status||1);
}
console.log('PASS JavaScript syntax: source modules and build/verification tools');
