import https from 'node:https';
import assert from 'node:assert/strict';
import {gunzipSync} from 'node:zlib';
import {mkdir,writeFile} from 'node:fs/promises';
const base=process.env.ORA_TEST_URL||'https://poc.oragame.eu';
function get(path,encoding){return new Promise((resolve,reject)=>{
  const req=https.get(base+path,{headers:{'Accept-Encoding':encoding}},res=>{
    const chunks=[];res.on('data',x=>chunks.push(x));res.on('end',()=>resolve({status:res.statusCode,headers:res.headers,body:Buffer.concat(chunks)}));
  });req.setTimeout(15000,()=>req.destroy(new Error('timeout')));req.on('error',reject);
});}
const results=[];
for(const path of ['/','/app.js','/animation-loop.js','/character-model.js','/character-three.js','/style.css','/vendor/THREE-LICENSE.txt']){
  const raw=await get(path,'identity'),gzip=await get(path,'gzip');
  assert.equal(raw.status,200);assert.equal(gzip.status,200);
  assert.equal(gzip.headers['content-encoding'],'gzip',path);
  assert.deepEqual(gunzipSync(gzip.body),raw.body,path+' decompresses to identical content');
  assert.ok(gzip.headers['content-security-policy']);
  // The CDN may normalize Vary; compression negotiation is verified by both bodies.
  results.push({path,rawBytes:raw.body.length,gzipBytes:gzip.body.length});
}
const account=await get('/account-api/session','gzip');
assert.match(account.headers['cache-control'],/no-store/);
assert.equal(account.headers['content-encoding'],undefined);
await mkdir('qa/maintenance',{recursive:true});
await writeFile('qa/maintenance/compression.json',JSON.stringify({base,results},null,2));
console.log('PASS public gzip equivalence, security headers, and uncompressed no-store account response');
console.log(JSON.stringify(results));
