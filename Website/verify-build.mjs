import assert from 'node:assert/strict';
import {access,readdir,readFile} from 'node:fs/promises';
import {gunzipSync} from 'node:zlib';
import {guides,extraCharacters} from './dist/content.js';

const root=new URL('./dist/',import.meta.url);
const shell=await readFile(new URL('index.html',root));
const routes=['univers','personnages','histoire','personnages/roster','enigmes','compte','compte/historique','compte/collection','compte/classement',...Object.keys(guides).map(path=>path.slice(1)),...['raijin','keplar','andaris','pandore',...Object.keys(extraCharacters)].map(key=>'personnages/'+key)];

for(const route of routes){
  const generated=await readFile(new URL(route+'/index.html',root));
  assert.deepEqual(generated,shell,`Route générée périmée : /${route}/`);
}

let compressed=0;
async function verifyCompressed(dir){
  for(const entry of await readdir(dir,{withFileTypes:true})){
    const path=new URL(entry.name+(entry.isDirectory()?'/':''),dir);
    if(entry.isDirectory()){await verifyCompressed(path);continue;}
    if(entry.name.endsWith('.gz')){
      await access(new URL(entry.name.slice(0,-3),dir));
      continue;
    }
    if(!/\.(html|css|js|svg|txt)$/.test(entry.name))continue;
    const [source,gzip]=await Promise.all([readFile(path),readFile(new URL(entry.name+'.gz',dir))]);
    assert.deepEqual(gunzipSync(gzip),source,`Compression périmée : ${path.pathname}`);
    compressed++;
  }
}

await verifyCompressed(root);
console.log(`PASS build cohérent : ${routes.length} routes et ${compressed} fichiers compressés vérifiés`);
