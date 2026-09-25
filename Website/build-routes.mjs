import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {guides,extraCharacters} from './dist/content.js';
const root=new URL('./dist/',import.meta.url);const html=await readFile(new URL('index.html',root),'utf8');
const routes=['univers','personnages','histoire','personnages/roster','enigmes','compte','compte/historique','compte/collection','compte/classement',...Object.keys(guides).map(p=>p.slice(1)),...['raijin','keplar','andaris','pandore',...Object.keys(extraCharacters)].map(k=>'personnages/'+k)];
for(const route of routes){await mkdir(new URL(route+'/',root),{recursive:true});await writeFile(new URL(route+'/index.html',root),html);}console.log(routes.length+' destination pages generated.');
