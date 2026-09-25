import {spawn} from 'node:child_process';

const checks=[
  'verify-navigation.mjs',
  'verify-v3.mjs',
  'verify-v6.mjs',
  'verify-diagrams.mjs',
  'verify-characters-3d.mjs',
  'verify-model-motion.mjs',
  'verify-account.mjs',
  'verify-animation-lifecycle.mjs',
  'verify-character-names.mjs',
  'verify-accessibility.mjs',
  'verify-enigmes.mjs'
];
const preview=spawn(process.execPath,['serve.mjs'],{stdio:['ignore','pipe','inherit']});

function run(script){
  return new Promise((resolve,reject)=>{
    const child=spawn(process.execPath,[script],{stdio:'inherit'});
    child.once('error',reject);
    child.once('exit',code=>code===0?resolve():reject(new Error(`${script} a échoué (${code})`)));
  });
}

try{
  await new Promise((resolve,reject)=>{
    const timeout=setTimeout(()=>reject(new Error('Le serveur local ne répond pas.')),10_000);
    preview.once('error',reject);
    preview.once('exit',code=>reject(new Error(`Le serveur local s’est arrêté (${code}).`)));
    preview.stdout.on('data',chunk=>{
      process.stdout.write(chunk);
      if(chunk.toString().includes('ORA preview:')){clearTimeout(timeout);resolve();}
    });
  });
  for(const check of checks)await run(check);
  console.log(`PASS suite locale complète : ${checks.length} contrôles`);
}finally{
  preview.kill();
}
