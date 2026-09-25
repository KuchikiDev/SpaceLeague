// Signature motion: the ORA ball carries each navigation, marks the active destination and follows the pointer.
// Everything here is decorative (aria-hidden) and stands down with the site's motion toggle.
const SPOT='.portal,.roster-card,.island a,.player-metrics article,.fragment-panel,.home-enigma,.puzzle-stage,.guide-visual,.account-empty,.certificate';

export function createMotion(isReduced){
 const layer=document.createElement('div');layer.className='trajectory-layer';layer.setAttribute('aria-hidden','true');
 layer.innerHTML='<svg><path class="trajectory-glow"/><path class="trajectory-trail"/></svg><i class="trajectory-ball"></i><i class="trajectory-impact"></i>';
 const cursor=document.createElement('div');cursor.className='ora-cursor';cursor.setAttribute('aria-hidden','true');
 document.body.append(layer,cursor);
 const svg=layer.querySelector('svg'),paths=[...svg.querySelectorAll('path')],ball=layer.querySelector('.trajectory-ball'),impact=layer.querySelector('.trajectory-impact');
 let animations=[];

 // A shot from the pointer that rebounds on the edges of the screen, like the ball on the arena walls.
 function route(x,y){
  const w=innerWidth,h=innerHeight,m=16;x=Math.min(w-m,Math.max(m,x));y=Math.min(h-m,Math.max(m,y));
  // Always a diagonal shot towards the centre, so the rebounds read clearly.
  const t=(25+Math.random()*30)*Math.PI/180,sx=Math.abs(w/2-x)<40?(Math.random()<.5?-1:1):Math.sign(w/2-x),sy=Math.abs(h*.55-y)<40?-1:Math.sign(h*.55-y);
  let dx=sx*Math.cos(t),dy=sy*Math.sin(t),left=Math.max(w,h)*1.15;const points=[[x,y]];
  for(let k=0;k<5&&left>0;k++){const tx=dx>0?(w-m-x)/dx:(m-x)/dx,ty=dy>0?(h-m-y)/dy:(m-y)/dy,t=Math.min(tx,ty,left);x+=dx*t;y+=dy*t;left-=t;points.push([x,y]);if(t===tx)dx=-dx;else if(t===ty)dy=-dy;}
  return points;
 }
 function launch(x,y){
  if(isReduced())return;animations.forEach(a=>a.cancel());
  const points=route(x,y),lengths=points.slice(1).map((p,i)=>Math.hypot(p[0]-points[i][0],p[1]-points[i][1])),total=lengths.reduce((s,l)=>s+l,0)||1;
  const d=points.map(([px,py],i)=>`${i?'L':'M'}${px.toFixed(1)} ${py.toFixed(1)}`).join('');svg.setAttribute('viewBox',`0 0 ${innerWidth} ${innerHeight}`);
  paths.forEach(p=>{p.setAttribute('d',d);p.style.strokeDasharray=total;});
  let run=0;const frames=points.map(([px,py],i)=>{if(i)run+=lengths[i-1];return {transform:`translate(${px}px,${py}px)`,offset:run/total};});
  const [ex,ey]=points.at(-1),timing={duration:640,easing:'cubic-bezier(.25,.75,.3,1)'};
  animations=[
   ...paths.map(p=>p.animate([{strokeDashoffset:total,opacity:1},{strokeDashoffset:0,opacity:1,offset:.7},{strokeDashoffset:0,opacity:0}],{...timing,duration:1100})),
   ball.animate(frames.map(f=>({...f,opacity:1})),timing),
   ball.animate([{opacity:1},{opacity:0}],{duration:260,delay:620,fill:'forwards'}),
   impact.animate([{transform:`translate(${ex}px,${ey}px) scale(.2)`,opacity:.9},{transform:`translate(${ex}px,${ey}px) scale(2.4)`,opacity:0}],{duration:620,delay:560,easing:'cubic-bezier(.2,.7,.3,1)',fill:'backwards'})
  ];
 }
 function burst(x,y,color='#e9ad7d'){
  if(isReduced())return;
  for(let i=0;i<12;i++){const spark=document.createElement('i');spark.className='trajectory-spark';spark.style.setProperty('--c',color);layer.append(spark);
   const a=i/12*Math.PI*2+Math.random()*.3,r=90+Math.random()*120;
   spark.animate([{transform:`translate(${x}px,${y}px) scale(1.2)`,opacity:1},{transform:`translate(${x+Math.cos(a)*r}px,${y+Math.sin(a)*r}px) scale(.2)`,opacity:0}],{duration:900+Math.random()*400,easing:'cubic-bezier(.15,.75,.3,1)'}).onfinish=()=>spark.remove();}
  impact.animate([{transform:`translate(${x}px,${y}px) scale(.3)`,opacity:1},{transform:`translate(${x}px,${y}px) scale(4)`,opacity:0}],{duration:900,easing:'cubic-bezier(.2,.7,.3,1)'});
 }
 // The header ball rests on the centre line on the crossroads and travels to the current destination.
 function placeBall(){
  const nav=document.querySelector('.shell-nav');if(!nav)return;const box=nav.getBoundingClientRect();if(!box.width)return;
  const active=nav.querySelector('a[aria-current]'),target=(active||nav.querySelector('.nav-midline')).getBoundingClientRect();
  nav.style.setProperty('--ball-x',`${(target.left-box.left+target.width/2).toFixed(1)}px`);nav.dataset.camp=active?active.closest('[data-camp]').dataset.camp:'mid';
 }
 addEventListener('resize',placeBall);document.fonts?.ready.then(placeBall);

 // Pointer companion: a small ball that trails the cursor and opens into an orbit over interactive elements.
 let target=[0,0],pos=[0,0],frame=0,hovering=false;
 const tick=()=>{frame=0;if(isReduced()||document.hidden)return;pos=pos.map((v,i)=>v+(target[i]-v)*.24);cursor.style.transform=`translate(${pos[0].toFixed(1)}px,${pos[1].toFixed(1)}px)`;if(Math.abs(target[0]-pos[0])+Math.abs(target[1]-pos[1])>.4)frame=requestAnimationFrame(tick);};
 addEventListener('pointermove',e=>{
  const card=e.target.closest?.(SPOT);if(card){const r=card.getBoundingClientRect();card.style.setProperty('--px',`${e.clientX-r.left}px`);card.style.setProperty('--py',`${e.clientY-r.top}px`);}
  if(e.pointerType!=='mouse'||isReduced())return;
  if(!cursor.classList.contains('on')){pos=[e.clientX,e.clientY];cursor.classList.add('on');}
  target=[e.clientX,e.clientY];const over=Boolean(e.target.closest?.('a,button,summary,label,input,[role=tab],[tabindex="0"]'));if(over!==hovering){hovering=over;cursor.classList.toggle('orbiting',over);}
  if(!frame)frame=requestAnimationFrame(tick);
 },{passive:true});
 addEventListener('pointerdown',e=>{if(e.pointerType!=='mouse'||isReduced())return;cursor.classList.remove('control');void cursor.offsetWidth;cursor.classList.add('control');});
 document.documentElement.addEventListener('mouseleave',()=>cursor.classList.remove('on'));
 document.addEventListener('ora-motion-change',()=>{if(isReduced()){cancelAnimationFrame(frame);frame=0;cursor.classList.remove('on');animations.forEach(a=>a.cancel());}});
 return {launch,burst,placeBall};
}
