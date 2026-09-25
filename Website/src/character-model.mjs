// Procedural 3D studies for the POC, not final character designs or game assets.
import {animationLoop} from '../dist/animation-loop.js';
export function createCharacterModel(host,key,color,isReduced){
 let disposed=false,cleanup=()=>{};
 import('./three-subset.mjs').then(T=>{if(disposed)return;try{cleanup=mount(T,host,key,color,isReduced);}catch{host.dataset.state='unavailable';host.querySelector('.model-status').textContent='Aperçu 3D indisponible sur ce navigateur.';}}).catch(()=>{if(!disposed){host.dataset.state='unavailable';host.querySelector('.model-status').textContent='Aperçu 3D indisponible. Rechargez la page pour réessayer.';}});
 return ()=>{disposed=true;cleanup();};
}
function mount(T,host,key,color,isReduced){
 const canvas=host.querySelector('canvas'),renderer=new T.WebGLRenderer({canvas,alpha:true,antialias:true,powerPreference:'low-power'});
 renderer.setPixelRatio(Math.min(devicePixelRatio,1.5));renderer.setClearColor(0,0);renderer.outputColorSpace=T.SRGBColorSpace;renderer.toneMapping=T.ACESFilmicToneMapping;renderer.toneMappingExposure=1.55;
 const scene=new T.Scene(),camera=new T.PerspectiveCamera(32,1,.1,50);camera.position.set(0,2.3,8.4);camera.lookAt(0,1.65,0);
 scene.add(new T.HemisphereLight(0xd9f1ec,0x263440,3));const keylight=new T.DirectionalLight(0xffeed2,4);keylight.position.set(-3,6,5);scene.add(keylight);const rim=new T.DirectionalLight(color,5);rim.position.set(3,3,-3);scene.add(rim);
 const materials=[],geometries=[];const mat=(c,metal=.55,rough=.4,emission=false)=>{const m=new T.MeshStandardMaterial({color:c,metalness:metal,roughness:rough,flatShading:true,...(emission?{emissive:c,emissiveIntensity:1}: {})});materials.push(m);return m;};
 const dark=mat('#263940'),armor=mat('#9caeaa',.65),light=mat('#d5d0bb'),accent=mat(color,.6,.3),glow=mat(color,.1,.3,true),black=mat('#071119');
 const figure=new T.Group(),body=new T.Group();scene.add(figure);figure.add(body);figure.rotation.y=-.23;
 const mesh=(geometry,material,pos,parent=body,scale)=>{geometries.push(geometry);const m=new T.Mesh(geometry,material);m.position.set(...pos);if(scale)m.scale.set(...scale);parent.add(m);return m;};
 const box=(w,h,d,x,y,z,m=armor,parent=body)=>mesh(new T.BoxGeometry(w,h,d),m,[x,y,z],parent);
 const sphere=(r,x,y,z,m=dark,parent=body,scale)=>mesh(new T.IcosahedronGeometry(r,1),m,[x,y,z],parent,scale);
 const cone=(r1,r2,h,x,y,z,m=armor,parent=body,n=6)=>mesh(new T.CylinderGeometry(r1,r2,h,n),m,[x,y,z],parent);
 const rod=(a,b,r,m=armor,parent=body)=>{const v=new T.Vector3(...b).sub(new T.Vector3(...a));const o=mesh(new T.CylinderGeometry(r,r*.82,v.length(),6),m,new T.Vector3(...a).add(new T.Vector3(...b)).multiplyScalar(.5).toArray(),parent);o.quaternion.setFromUnitVectors(new T.Vector3(0,1,0),v.normalize());return o;};
 const ring=(r,x,y,z,m=glow,parent=body)=>mesh(new T.TorusGeometry(r,.013,5,64),m,[x,y,z],parent);
 const broad=['pandore','obsidia','aurion'].includes(key),slender=['andaris','chronis'].includes(key),w=broad?.67:slender?.43:.52;
 // Armoured athlete proportions, articulated limbs and an individual silhouette.
 cone(w*.82,w*.52,.68,0,2.03,0,armor);cone(.24,.31,.36,0,1.53,0,dark);box(w*.8,.16,.4,0,1.43,.015,accent);
 const chest=mesh(new T.OctahedronGeometry(.2),accent,[0,2.12,.26]);chest.scale.set(.9,1.35,.35);
 [-1,1].forEach(s=>{const hip=[s*.19,1.39,0],knee=[s*.24,.85,s===1?.06:-.07],ankle=[s*.3,.26,s===1?.13:-.07];sphere(.15,...hip);rod(hip,knee,.14,dark);cone(.145,.115,.44,s*.22,1.11,0,armor);sphere(.12,...knee,accent);rod(knee,ankle,.1,dark);cone(.105,.145,.37,s*.28,.48,ankle[2],armor);box(.22,.18,.42,s*.3,.14,ankle[2]+.11,black);box(.235,.05,.3,s*.3,.21,ankle[2]+.08,accent);});
 const arms=[];[-1,1].forEach(s=>{const arm=new T.Group();arm.position.set(s*w*.78,2.23,0);body.add(arm);arms.push(arm);sphere(broad?.23:.17,0,0,0,armor,arm,[1.2,.9,1]);rod([0,-.1,0],[s*.13,-.46,.03],.095,dark,arm);cone(.115,.09,.26,s*.075,-.25,.025,armor,arm);sphere(.095,s*.13,-.46,.03,accent,arm);rod([s*.13,-.46,.03],[s*.17,-.8,.14],.095,armor,arm);sphere(.11,s*.17,-.83,.15,dark,arm,[.8,1.15,.8]);arm.rotation.z=s*(key==='raijin'?.13:key==='keplar'?-.3:.04);});
 cone(.095,.12,.17,0,2.48,0,dark);const head=new T.Group();head.position.y=2.72;body.add(head);sphere(.23,0,0,0,light,head,[.82,1.18,.83]);box(.31,.13,.11,0,.015,.175,black,head);box(.25,.025,.12,0,.04,.183,glow,head);cone(.19,.22,.12,0,.19,0,dark,head);
 const effects=new T.Group();body.add(effects);
 if(key==='raijin'){[-1,1].forEach(s=>{rod([s*.14,2.9,0],[s*.28,3.25,-.1],.025,accent);for(let i=0;i<3;i++){const fin=box(.06,.24+i*.05,.26,s*(w*.8+i*.065),2.28+i*.03,-.06,accent);fin.rotation.z=-s*.5;}});const bolt=[[-.06,0,0],[.07,.16,0],[0,.16,0],[.12,.36,0]];for(let i=1;i<bolt.length;i++)rod(bolt[i-1].map((v,j)=>v+[-.78,1.45,.25][j]),bolt[i].map((v,j)=>v+[-.78,1.45,.25][j]),.02,glow,effects);}
 if(key==='keplar'){cone(.31,.2,.19,0,2.49,-.05,accent);const mantle=cone(.36,.56,.85,0,1.76,-.15,dark);mantle.scale.z=.65;for(let i=0;i<3;i++){const r=ring(.48+i*.13,.67,1.75,.22,accent,effects);r.rotation.set(.45+i*.65,.4+i*.6,0);}sphere(.11,.67,1.75,.22,glow,effects);}
 if(key==='andaris'){const hood=cone(.24,.32,.4,0,2.78,-.075,dark);hood.scale.z=.9;box(.29,.16,.12,0,2.77,.2,black);sphere(.07,.13,2.79,.27,glow);const tail=cone(.31,.51,.78,0,1.57,-.19,dark);tail.scale.z=.6;box(.15,.38,.2,-.51,1.89,.1,accent);const reticle=ring(.18,.61,2.65,.08,glow,effects);reticle.rotation.y=.3;}
 if(key==='pandore'){cone(.31,.67,1.24,0,.99,-.04,dark);cone(.35,.47,.5,0,1.83,-.08,accent);[-1,1].forEach(s=>{cone(.01,.09,.34,s*.2,3.02,-.035,accent);sphere(.08,s*.63,1.3,.2,glow,effects);});const seal=ring(.2,0,1.25,.47,accent,effects);seal.rotation.z=.5;}
 if(key==='obsidia'){[-1,1].forEach(s=>{const p=mesh(new T.OctahedronGeometry(.31),armor,[s*.51,2.27,0]);p.scale.set(1,1.2,.6);const shard=mesh(new T.OctahedronGeometry(.34),accent,[s*.77,1.9,.07],effects);shard.scale.set(.35,1.6,.6);});mesh(new T.OctahedronGeometry(.25),armor,[0,2.75,.08]).scale.set(.9,1.3,.75);}
 if(key==='aurion'){const halo=ring(.4,0,2.96,-.2,accent);halo.rotation.x=.2;[-1,1].forEach(s=>{for(let i=0;i<3;i++){const fin=box(.1,.57-i*.1,.12,s*(.53+i*.12),2.4-i*.05,-.1,light);fin.rotation.z=-s*.45;}});cone(.27,.43,.8,0,1.2,-.14,light);}
 if(key==='vaalbara'){cone(.3,.65,1.1,0,1.08,0,dark);cone(.31,.39,.16,0,2.52,0,accent);[-1,1].forEach(s=>{mesh(new T.TetrahedronGeometry(.23),accent,[s*.68,1.84,0],effects);});mesh(new T.TetrahedronGeometry(.15),glow,[0,3.12,0],effects);}
 if(key==='chronis'){cone(.27,.46,.87,0,1.2,-.07,dark);const dial=ring(.42,0,2.2,-.28,accent);dial.rotation.y=.35;for(let i=0;i<12;i++){const a=i*Math.PI/6;box(.025,.06,.025,Math.sin(a)*.43,2.2+Math.cos(a)*.43,-.28,glow);}const hourglass=new T.Group();hourglass.position.set(.66,1.65,.2);effects.add(hourglass);cone(.16,0,.2,0,.1,0,accent,hourglass);cone(0,.16,.2,0,-.1,0,accent,hourglass);}
 if(key==='magnora'){[-1,1].forEach(s=>{const r=ring(.22,s*.66,1.68,.12,accent,effects);r.rotation.y=Math.PI/2;box(.19,.42,.25,s*.51,1.71,.09,dark);box(.21,.09,.27,s*.51,1.5,.09,accent);});cone(.23,.29,.15,0,2.92,0,accent);}
 const stage=mesh(new T.CylinderGeometry(.91,1,.07,64),mat('#162a31',.7),[0,.025,0],scene);const stageRing=ring(.92,0,.07,0,accent,scene);stageRing.rotation.x=Math.PI/2;
 const motes=[];for(let i=0;i<14;i++){const a=i*2.399;const m=sphere(.013,Math.sin(a)*1.1,.5+(i%7)*.39,Math.cos(a)*.7,glow,figure);motes.push(m);}
 let angle=-.23,dragging=false,lastX=0,time=0;
 const render=()=>renderer.render(scene,camera);const resize=()=>{const r=canvas.getBoundingClientRect();if(!r.width||!r.height)return;renderer.setSize(r.width,r.height,false);camera.aspect=r.width/r.height;camera.position.z=camera.aspect<.65?9.6:7.8;camera.updateProjectionMatrix();render();};const ro=new ResizeObserver(resize);ro.observe(host);
 const down=e=>{dragging=true;lastX=e.clientX;canvas.setPointerCapture(e.pointerId);};const move=e=>{if(!dragging)return;angle+=(e.clientX-lastX)*.008;lastX=e.clientX;figure.rotation.y=angle;render();};const up=()=>dragging=false;const keys=e=>{if(e.key==='ArrowLeft'||e.key==='ArrowRight'){e.preventDefault();angle+=e.key==='ArrowLeft'?-.2:.2;figure.rotation.y=angle;render();}};const reset=()=>{angle=-.23;figure.rotation.y=angle;render();};
 canvas.addEventListener('pointerdown',down);canvas.addEventListener('pointermove',move);canvas.addEventListener('pointerup',up);canvas.addEventListener('pointercancel',up);canvas.addEventListener('keydown',keys);host.querySelector('[data-model-reset]').onclick=reset;
 const stop=animationLoop(dt=>{time+=dt;body.position.y=Math.sin(time*1.25)*.019;head.rotation.y=Math.sin(time*.4)*.07;figure.rotation.y=angle+(!dragging?Math.sin(time*.35)*.09:0);effects.rotation.y=Math.sin(time*.6)*.09;motes.forEach((m,i)=>m.position.y+=Math.sin(time+i)*.0009);render();},isReduced);
 resize();host.dataset.state='ready';host.querySelector('.model-status').textContent='Étude 3D · apparence à valider';
 return ()=>{stop();ro.disconnect();canvas.removeEventListener('pointerdown',down);canvas.removeEventListener('pointermove',move);canvas.removeEventListener('pointerup',up);canvas.removeEventListener('pointercancel',up);canvas.removeEventListener('keydown',keys);geometries.forEach(g=>g.dispose());materials.forEach(m=>m.dispose());renderer.dispose();renderer.forceContextLoss();};
}


