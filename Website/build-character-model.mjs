import {copyFile,mkdir,rm} from 'node:fs/promises';
import {build} from 'esbuild';

await mkdir('dist/vendor',{recursive:true});
await copyFile('node_modules/three/LICENSE','dist/vendor/THREE-LICENSE.txt');
await Promise.all(['character-model.js','character-model.js.gz','character-three.js','character-three.js.gz'].map(file=>rm('dist/'+file,{force:true})));
await build({
  entryPoints:{'character-model':'src/character-model.mjs'},
  outdir:'dist',
  bundle:true,
  splitting:true,
  format:'esm',
  minify:true,
  target:['es2022'],
  entryNames:'[name]',
  chunkNames:'character-three',
  legalComments:'none'
});
console.log('Modèle 3D et sous-ensemble Three.js générés.');
