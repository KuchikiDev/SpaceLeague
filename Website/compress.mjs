import {readdir,readFile,writeFile} from 'node:fs/promises';
import {gzipSync} from 'node:zlib';
const root=new URL('./dist/',import.meta.url);
let raw=0,compressed=0,count=0;
async function walk(dir){
  for(const entry of await readdir(dir,{withFileTypes:true})){
    const path=new URL(entry.name+(entry.isDirectory()?'/':''),dir);
    if(entry.isDirectory()){await walk(path);continue;}
    if(!/\.(html|css|js|svg|txt)$/.test(entry.name))continue;
    const source=await readFile(path);
    const gzip=gzipSync(source,{level:9});
    await writeFile(new URL(entry.name+'.gz',dir),gzip);
    raw+=source.length;compressed+=gzip.length;count++;
  }
}
await walk(root);
console.log(JSON.stringify({files:count,rawBytes:raw,gzipBytes:compressed,savingPercent:Math.round((1-compressed/raw)*100)}));
