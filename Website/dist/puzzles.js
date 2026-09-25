// The nine seals of the archipelago. Each puzzle borrows the documented Type of a character;
// they are puzzles of the site, not game mechanics, balance values or canonical lore.
const dist=(a,b)=>Math.hypot(a[0]-b[0],a[1]-b[1]);
const norm=s=>s.normalize('NFD').replace(/[̀-ͯ]/g,'').toUpperCase().replace(/[^A-Z]/g,'');

// Frame loop scoped to one puzzle: stops when hidden, when the step returns false, or on cleanup.
function loop(step){
 let frame=0,last=0,alive=true;
 const tick=now=>{frame=0;if(!alive||document.hidden)return;const dt=last?Math.min((now-last)/1000,.05):0;last=now;if(step(dt)===false){stop();return;}frame=requestAnimationFrame(tick);};
 const sync=()=>{cancelAnimationFrame(frame);frame=0;last=0;if(alive&&!document.hidden)frame=requestAnimationFrame(tick);};
 function stop(){alive=false;cancelAnimationFrame(frame);document.removeEventListener('visibilitychange',sync);}
 document.addEventListener('visibilitychange',sync);sync();return stop;
}
// Moves an SVG circle along a polyline at constant speed, then calls done().
function travel(ball,points,speed,done){
 const lengths=points.slice(1).map((p,i)=>dist(p,points[i])),total=lengths.reduce((a,b)=>a+b,0)||1;let travelled=0;
 return loop(dt=>{travelled=Math.min(total,travelled+dt*speed);let left=travelled,i=0;while(i<lengths.length-1&&left>lengths[i]){left-=lengths[i];i++;}
  const a=points[i],b=points[i+1]||a,k=lengths[i]?left/lengths[i]:1;ball.setAttribute('cx',a[0]+(b[0]-a[0])*k);ball.setAttribute('cy',a[1]+(b[1]-a[1])*k);
  if(travelled>=total){done?.();return false;}});
}
const polyline=points=>points.map(([x,y],i)=>`${i?'L':'M'}${x.toFixed(1)} ${y.toFixed(1)}`).join('');
const typing=e=>e.target.closest?.('input,textarea,select,button,a,summary');

export const puzzles={
 raijin:{
  title:'Ligne directe',
  brief:'Surcharge n’autorise qu’un tir droit. Frappez quand la ligne est libre : trois tirs réussis d’affilée brisent le sceau. Un obstacle touché fait retomber les Charges.',
  hint:['/personnages/raijin/','Relire le Passif de Raijin'],
  mount(host,ui){
   host.innerHTML=`<svg class="pz-svg" viewBox="0 0 600 300" aria-hidden="true"><rect class="pz-field" x="8" y="8" width="584" height="284" rx="14"/><path class="pz-lane" d="M60 150H540"/>${[0,1,2].map(i=>`<rect class="pz-block" data-block x="${190+i*120}" y="102" width="22" height="96" rx="5"/>`).join('')}<g class="pz-goal"><ellipse cx="552" cy="150" rx="12" ry="34"/></g><circle class="pz-player" cx="60" cy="150" r="13"/><path class="pz-bolt" d=""/></svg>
   <div class="pz-controls"><button class="pz-action" data-fire>Tirer <kbd>Espace</kbd></button><span class="pz-charges" role="img" aria-label="Charges : 0 sur 3"><i></i><i></i><i></i></span></div>`;
   const blocks=[...host.querySelectorAll('[data-block]')],bolt=host.querySelector('.pz-bolt'),pips=host.querySelector('.pz-charges');
   const speeds=[1.35,1.95,2.6],phase=[0,2.1,4.2];let t=0,streak=0,ys=[150,150,150],won=false;
   const blockedAt=()=>ys.findIndex(y=>Math.abs(y-150)<54);
   const stop=loop(dt=>{t+=dt;ys=speeds.map((s,i)=>150+100*Math.sin(t*s+phase[i]));blocks.forEach((b,i)=>b.setAttribute('y',(ys[i]-48).toFixed(1)));host.dataset.clear=String(blockedAt()<0);});
   function fire(){
    if(won)return;const hit=blockedAt(),end=hit<0?540:190+hit*120;let d='M74 150';for(let x=100,s=1;x<end;x+=30,s=-s)d+=`L${x} ${150+s*(5+Math.random()*6)}`;d+=`L${end} 150`;
    bolt.setAttribute('d',d);bolt.classList.remove('flash');void bolt.getBoundingClientRect();bolt.classList.add('flash');bolt.classList.toggle('blocked',hit>=0);
    if(hit<0){streak++;ui.say(streak<3?`Ligne libre. Charge ${streak} sur 3.`:'Trois tirs droits. La Surcharge brise le sceau.');}else{streak=0;ui.say('Le tir heurte un obstacle solide. Les Charges retombent à zéro.');}
    [...pips.children].forEach((pip,i)=>pip.classList.toggle('on',i<streak));pips.setAttribute('aria-label',`Charges : ${streak} sur 3`);
    if(streak>=3){won=true;ui.done();}
   }
   host.querySelector('[data-fire]').onclick=fire;
   const key=e=>{if(e.code==='Space'&&!e.repeat&&!typing(e)){e.preventDefault();fire();}};document.addEventListener('keydown',key);
   return()=>{stop();document.removeEventListener('keydown',key);};
  }
 },
 keplar:{
  title:'Distorsion',
  brief:'Un seul tir, quatre segments. Donnez une courbure à chacun pour contourner les obstacles jusqu’au but. L’Expansion disponible n’autorise que trois courbures.',
  hint:['/personnages/keplar/','Relire Distorsion et Expansion'],
  mount(host,ui){
   const start=[48,160],L=122,T=.95,N=24,goal=[430.9,208],obstacles=[[210,175],[300,160],[330,245],[505,115]],labels={'-1':['↰','courbure à gauche'],'0':['↑','tout droit'],'1':['↱','courbure à droite']};
   let curve=[0,0,0,0],busy=false,won=false,stopBall=()=>{};
   host.innerHTML=`<svg class="pz-svg" viewBox="0 0 600 320" aria-hidden="true"><rect class="pz-field" x="18" y="18" width="564" height="284" rx="14"/>${obstacles.map(([x,y])=>`<circle class="pz-obstacle" cx="${x}" cy="${y}" r="18"/>`).join('')}<circle class="pz-target" cx="${goal[0]}" cy="${goal[1]}" r="16"/><path class="pz-trace" d=""/><circle class="pz-player" cx="${start[0]}" cy="${start[1]}" r="11"/><circle class="pz-ball" r="6" cx="${start[0]}" cy="${start[1]}"/></svg>
   <div class="pz-controls">${curve.map((_,i)=>`<button class="pz-segment" data-segment="${i}"></button>`).join('')}<span class="pz-meter" data-expansion></span><button class="pz-action" data-fire>Tirer</button></div>`;
   const trace=host.querySelector('.pz-trace'),ball=host.querySelector('.pz-ball'),meter=host.querySelector('[data-expansion]'),fireButton=host.querySelector('[data-fire]'),segments=[...host.querySelectorAll('[data-segment]')];
   function compute(){let [x,y]=start,h=0,hit=false;const points=[[x,y]];for(const k of curve){for(let i=0;i<N&&!hit;i++){h+=k*T/N;x+=L/N*Math.cos(h);y+=L/N*Math.sin(h);points.push([x,y]);hit=x<24||x>576||y<24||y>296||obstacles.some(o=>dist(o,[x,y])<24);}}return {points,hit};}
   function draw(){const {points,hit}=compute(),used=curve.filter(Boolean).length;trace.setAttribute('d',polyline(points));trace.classList.toggle('blocked',hit);
    segments.forEach((b,i)=>{const [icon,text]=labels[curve[i]];b.textContent=icon;b.setAttribute('aria-label',`Segment ${i+1} : ${text}`);});
    meter.textContent=`Expansion ${Math.max(0,3-used)} / 3`;meter.classList.toggle('empty',used>3);fireButton.disabled=busy||used>3;}
   segments.forEach((b,i)=>b.onclick=()=>{if(busy)return;curve[i]=curve[i]===0?-1:curve[i]===-1?1:0;draw();ui.say(curve.filter(Boolean).length>3?'Expansion insuffisante : une courbure de trop.':'');});
   fireButton.onclick=()=>{if(busy||won)return;busy=true;draw();const {points,hit}=compute();const reached=!hit&&dist(points.at(-1),goal)<16;
    stopBall=travel(ball,points,420,()=>{busy=false;if(reached){won=true;ui.say('La trajectoire se plie trois fois et trouve le but.');ui.done();}else{ui.say(hit?'La balle heurte un obstacle.':'La balle manque le but.');ball.setAttribute('cx',start[0]);ball.setAttribute('cy',start[1]);}draw();});};
   draw();return()=>stopBall();
  }
 },
 andaris:{
  title:'Prémonition',
  brief:'Andaris voit la trajectoire en préparation. Seul le premier segment est visible : où la balle touchera-t-elle le mur après trois rebonds ? Trois lectures justes d’affilée.',
  hint:['/jeu/mouvements/','Lire les rebonds'],
  mount(host,ui){
   const box=[30,30,570,290];let streak=0,round=null,locked=false,stopBall=()=>{};
   host.innerHTML=`<svg class="pz-svg" viewBox="0 0 600 320" aria-hidden="true"><rect class="pz-field" x="${box[0]}" y="${box[1]}" width="${box[2]-box[0]}" height="${box[3]-box[1]}" rx="4"/><g data-round></g></svg>
   <div class="pz-controls"><span class="pz-question">Point d’impact après 3 rebonds :</span>${'ABCD'.split('').map(l=>`<button class="pz-choice" data-choice="${l}">${l}</button>`).join('')}<button class="pz-action" data-next hidden>Échange suivant</button><span class="pz-charges" role="img" aria-label="Lectures justes : 0 sur 3"><i></i><i></i><i></i></span></div>`;
   const layer=host.querySelector('[data-round]'),pips=host.querySelector('.pz-charges'),next=host.querySelector('[data-next]'),choices=[...host.querySelectorAll('[data-choice]')];
   function contacts(p,d,n){const out=[];let [x,y]=p,[dx,dy]=d;for(let k=0;k<n;k++){const tx=dx>0?(box[2]-x)/dx:(box[0]-x)/dx,ty=dy>0?(box[3]-y)/dy:(box[1]-y)/dy,t=Math.min(tx,ty);x+=dx*t;y+=dy*t;if(tx<ty)dx=-dx;else dy=-dy;out.push([x,y]);}return out;}
   function generate(){for(let n=0;n<200;n++){const p=[90+Math.random()*140,80+Math.random()*160],a=(25+Math.random()*40)*Math.PI/180+Math.floor(Math.random()*4)*Math.PI/2,c=contacts(p,[Math.cos(a),Math.sin(a)],6);
     const marks=[c[3],c[2],c[4],c[5]];if(marks.some((m,i)=>marks.some((o,j)=>j>i&&dist(m,o)<75))||marks.some(m=>dist(m,c[0])<60))continue;
     const order=[0,1,2,3].sort(()=>Math.random()-.5);return {p,c,marks,order,answer:'ABCD'[order.indexOf(0)]};}return null;}
   function show(){do round=generate();while(!round);locked=false;next.hidden=true;choices.forEach(b=>{b.disabled=false;b.className='pz-choice';});
    const inward=([x,y])=>[x<=box[0]+1?x+22:x>=box[2]-1?x-22:x,y<=box[1]+1?y+22:y>=box[3]-1?y-22:y];
    layer.innerHTML=`<path class="pz-trace full" d="${polyline([round.p,...round.c.slice(0,4)])}"/><path class="pz-trace preview" d="${polyline([round.p,round.c[0]])}"/>${round.order.map((m,i)=>{const [x,y]=round.marks[m],[lx,ly]=inward(round.marks[m]);return `<g class="pz-mark" data-mark="${'ABCD'[i]}"><rect x="${x-7}" y="${y-7}" width="14" height="14" transform="rotate(45 ${x} ${y})"/><text x="${lx}" y="${ly+5}">${'ABCD'[i]}</text></g>`;}).join('')}<circle class="pz-player" cx="${round.p[0]}" cy="${round.p[1]}" r="11"/><circle class="pz-ball" r="6" cx="${round.p[0]}" cy="${round.p[1]}"/>`;
    host.dataset.answer=round.answer;}
   function choose(letter){if(locked||!round)return;locked=true;const right=letter===round.answer;choices.forEach(b=>{b.disabled=true;if(b.dataset.choice===round.answer)b.classList.add('ok');else if(b.dataset.choice===letter)b.classList.add('ko');});
    layer.querySelector('.full').classList.add('revealed');layer.querySelector(`[data-mark="${round.answer}"]`).classList.add('ok');
    stopBall=travel(layer.querySelector('.pz-ball'),[round.p,...round.c.slice(0,4)],900,()=>{streak=right?streak+1:0;[...pips.children].forEach((pip,i)=>pip.classList.toggle('on',i<streak));pips.setAttribute('aria-label',`Lectures justes : ${streak} sur 3`);
     if(streak>=3){ui.say('Trois lectures justes. L’échange n’a plus de secret.');ui.done();}else{ui.say(right?`Lecture juste (${streak} sur 3).`:`Le point d’impact était ${round.answer}. La série repart de zéro.`);next.hidden=false;next.focus({preventScroll:true});}});}
   choices.forEach(b=>b.onclick=()=>choose(b.dataset.choice));next.onclick=show;show();
   return()=>stopBall();
  }
 },
 pandore:{
  title:'Le sceau se propage',
  brief:'Chaque marque posée maudit la rune et ses voisines directes — ou lève leur malédiction. Libérez les seize runes.',
  hint:['/personnages/pandore/','Relire le Sceau de Pandore'],
  mount(host,ui){
   const start=[1,6,8,11,14],runes='◇✳⌖◈';let cells,moves=0,won=false;
   const press=i=>[i,i-4,i+4,i%4?i-1:-1,i%4<3?i+1:-1].forEach(j=>{if(j>=0&&j<16)cells[j]=!cells[j];});
   host.innerHTML=`<div class="pz-runes" role="group" aria-label="Seize runes">${Array.from({length:16},(_,i)=>`<button class="pz-rune" data-rune="${i}"><span aria-hidden="true">${runes[(i*7)%4]}</span></button>`).join('')}</div><div class="pz-controls"><span class="pz-meter" data-left></span><button class="pz-action ghost" data-reset>Recommencer</button></div>`;
   const buttons=[...host.querySelectorAll('[data-rune]')],left=host.querySelector('[data-left]');
   function draw(){const cursed=cells.filter(Boolean).length;buttons.forEach((b,i)=>{b.classList.toggle('cursed',cells[i]);b.setAttribute('aria-label',`Rune ${Math.floor(i/4)+1}-${i%4+1} : ${cells[i]?'maudite':'libre'}`);b.disabled=won;});left.textContent=`${cursed} malédiction${cursed>1?'s':''} · ${moves} marque${moves>1?'s':''}`;if(!cursed&&!won){won=true;buttons.forEach(b=>b.disabled=true);ui.say('Les seize runes sont libres.');ui.done();}}
   function reset(){cells=Array(16).fill(false);start.forEach(press);moves=0;draw();}
   buttons.forEach((b,i)=>b.onclick=()=>{if(won)return;press(i);moves++;b.classList.remove('struck');void b.offsetWidth;b.classList.add('struck');draw();});
   host.querySelector('[data-reset]').onclick=()=>{if(!won){reset();ui.say('Les malédictions d’origine reviennent.');}};reset();return()=>{};
  }
 },
 obsidia:{
  title:'Retourner la perspective',
  brief:'Un rayon d’ORA entre par la gauche. Inclinez les miroirs pour le renvoyer jusqu’au but. Un miroir ne sert à rien.',
  hint:['/personnages/obsidia/','Découvrir le Type Réflexion'],
  mount(host,ui){
   const level=['m#......','#..m..m.','..m.m.m.','.#mmm..G','#..#..#.'],C=8,R=5,D={E:[1,0],W:[-1,0],N:[0,-1],S:[0,1]},slash={E:'N',N:'E',W:'S',S:'W'},back={E:'S',S:'E',W:'N',N:'W'};
   const state={};let won=false;
   host.innerHTML=`<div class="pz-mirrors" style="--cols:${C};--rows:${R}">${level.flatMap((row,r)=>[...row].map((ch,c)=>ch==='m'?`<button class="pz-cell mirror" data-cell="${c},${r}"><i aria-hidden="true"></i></button>`:`<span class="pz-cell ${ch==='#'?'wall':ch==='G'?'goal':''}" aria-hidden="true"></span>`)).join('')}<svg class="pz-beam" viewBox="0 0 ${C} ${R}" preserveAspectRatio="none" aria-hidden="true"><path d="" vector-effect="non-scaling-stroke"/></svg><span class="pz-source" aria-hidden="true"></span></div><div class="pz-controls"><span class="pz-meter" data-beam></span></div>`;
   const mirrors=[...host.querySelectorAll('[data-cell]')],beam=host.querySelector('.pz-beam path'),meter=host.querySelector('[data-beam]');
   mirrors.forEach(b=>state[b.dataset.cell]=true);
   function trace(){let c=-1,r=2,d='E';const points=[[0,2.5]],seen=new Set();for(let i=0;i<120;i++){c+=D[d][0];r+=D[d][1];
     if(c<0||r<0||c>=C||r>=R){points.push([c+.5-D[d][0]*.5,r+.5-D[d][1]*.5]);return {points,win:false,end:'le rayon quitte l’arène'};}
     const ch=level[r][c];if(ch==='#'){points.push([c+.5-D[d][0]*.5,r+.5-D[d][1]*.5]);return {points,win:false,end:'le rayon s’écrase sur un bloc'};}
     points.push([c+.5,r+.5]);if(ch==='G')return {points,win:true};const k=c+','+r+d;if(seen.has(k))return {points,win:false,end:'le rayon tourne en boucle'};seen.add(k);
     if(ch==='m')d=state[c+','+r]?slash[d]:back[d];}return {points,win:false,end:''};}
   function draw(){const t=trace();beam.setAttribute('d',polyline(t.points));host.classList.toggle('lit',t.win);meter.textContent=t.win?'Rayon au but':`Rayon interrompu : ${t.end}`;
    mirrors.forEach(b=>{const [c,r]=b.dataset.cell.split(',').map(Number);b.classList.toggle('back',!state[b.dataset.cell]);b.setAttribute('aria-label',`Miroir colonne ${c+1}, ligne ${r+1} : incliné vers la ${state[b.dataset.cell]?'droite':'gauche'}`);b.disabled=won;});
    if(t.win&&!won){won=true;mirrors.forEach(b=>b.disabled=true);ui.say('Le rayon rebondit sur huit miroirs et atteint le but.');ui.done();}}
   mirrors.forEach(b=>b.onclick=()=>{if(won)return;state[b.dataset.cell]=!state[b.dataset.cell];draw();});draw();return()=>{};
  }
 },
 aurion:{
  title:'Révéler',
  brief:'Dans l’obscurité, des lettres attendent la lumière. Promenez la lueur — au pointeur ou aux flèches —, suivez les lettres numérotées et nommez ce que chacun possède.',
  hint:['/histoire/?chapitre=1','Relire le premier chapitre'],
  mount(host,ui){
   const letters=[['H',95,95,1],['O',455,235,2],['R',250,68,3],['I',150,238,4],['Z',545,90,5],['O',330,175,6],['N',405,92,7],['A',520,250],['E',60,200],['S',205,150],['T',300,265],['U',470,160],['L',120,40]];
   host.innerHTML=`<div class="pz-dark-room" tabindex="0" aria-label="Zone obscure. Utilisez les flèches pour déplacer la lumière."><svg class="pz-svg" viewBox="0 0 600 300" aria-hidden="true"><defs><radialGradient id="aurion-glow"><stop offset="0" stop-color="#fff"/><stop offset=".55" stop-color="#fff" stop-opacity=".8"/><stop offset="1" stop-color="#fff" stop-opacity="0"/></radialGradient><mask id="aurion-mask"><rect width="600" height="300" fill="#000"/><circle data-light r="78" cx="300" cy="150" fill="url(#aurion-glow)"/></mask></defs><rect class="pz-field" x="8" y="8" width="584" height="284" rx="14"/><g mask="url(#aurion-mask)">${letters.map(([l,x,y,n])=>`<text class="pz-letter" x="${x}" y="${y}">${l}${n?`<tspan dx="3" dy="-18">${n}</tspan>`:''}</text>`).join('')}</g><circle class="pz-halo" data-halo r="78" cx="300" cy="150"/></svg></div>
   <div class="pz-controls"><label class="pz-answer"><span>Chacun son…</span><input data-answer autocomplete="off" spellcheck="false" maxlength="20"></label><button class="pz-action" data-submit>Proposer</button></div>`;
   const room=host.querySelector('.pz-dark-room'),svg=room.querySelector('svg'),light=[host.querySelector('[data-light]'),host.querySelector('[data-halo]')],input=host.querySelector('[data-answer]');let pos=[300,150],won=false;
   const place=([x,y])=>{pos=[Math.max(0,Math.min(600,x)),Math.max(0,Math.min(300,y))];light.forEach(c=>{c.setAttribute('cx',pos[0]);c.setAttribute('cy',pos[1]);});};
   room.addEventListener('pointermove',e=>{const r=svg.getBoundingClientRect();place([(e.clientX-r.left)/r.width*600,(e.clientY-r.top)/r.height*300]);});
   room.addEventListener('keydown',e=>{const m={ArrowLeft:[-24,0],ArrowRight:[24,0],ArrowUp:[0,-24],ArrowDown:[0,24]}[e.key];if(m){e.preventDefault();place([pos[0]+m[0],pos[1]+m[1]]);}});
   const submit=()=>{if(won)return;if(norm(input.value)==='HORIZON'){won=true;input.disabled=true;room.classList.add('dawn');ui.say('« Chacun son horizon. » La lumière révèle tout l’îlot.');ui.done();}else{ui.say(input.value.trim()?'La lumière ne révèle pas ce mot.':'Proposez un mot.');input.select();}};
   host.querySelector('[data-submit]').onclick=submit;input.addEventListener('keydown',e=>{if(e.key==='Enter')submit();});return()=>{};
  }
 },
 vaalbara:{
  title:'État initial',
  brief:'Le sceau a été déplacé fragment par fragment. Faites glisser les pièces pour retrouver son état d’origine : le cercle entier et la case vide en bas à droite.',
  hint:['/personnages/vaalbara/','Découvrir le Type Origine'],
  mount(host,ui){
   const emblem=`<rect width="300" height="300" class="pz-plate"/><path class="pz-grid" d="M0 100H300M0 200H300M100 0V300M200 0V300"/><circle cx="150" cy="150" r="108" class="pz-ring"/><circle cx="150" cy="150" r="70" class="pz-ring thin"/><path class="pz-cross" d="M150 20v70m0 120v70M20 150h70m120 0h70"/><path class="pz-diamond" d="M150 112 188 150 150 188 112 150Z"/><circle cx="150" cy="150" r="9" class="pz-core"/><path class="pz-arc" d="M62 88A108 108 0 0 1 150 42"/><path class="pz-arc" d="M238 212A108 108 0 0 1 150 258"/>`;
   let board=[...'864203175'].map(Number),won=false;
   host.innerHTML=`<div class="pz-slider" role="group" aria-label="Fragments du sceau">${[1,2,3,4,5,6,7,8].map(n=>`<button class="pz-tile" data-tile="${n}"><svg viewBox="${(n-1)%3*100} ${Math.floor((n-1)/3)*100} 100 100" aria-hidden="true">${emblem}</svg><small aria-hidden="true">${n}</small></button>`).join('')}</div><div class="pz-controls"><span class="pz-meter" data-moves></span></div>`;
   const tiles=[...host.querySelectorAll('[data-tile]')],counter=host.querySelector('[data-moves]');let moves=0;
   const near=(a,b)=>Math.abs(a%3-b%3)+Math.abs(Math.floor(a/3)-Math.floor(b/3))===1;
   function draw(){const blank=board.indexOf(0);tiles.forEach(t=>{const n=Number(t.dataset.tile),at=board.indexOf(n),movable=!won&&near(at,blank);t.style.setProperty('--x',at%3);t.style.setProperty('--y',Math.floor(at/3));t.disabled=!movable&&!won;t.classList.toggle('movable',movable);t.setAttribute('aria-label',`Fragment ${n}${movable?' — déplaçable':''}`);});counter.textContent=`${moves} déplacement${moves>1?'s':''}`;host.dataset.board=board.join('');
    if(!won&&board.join('')==='123456780'){won=true;host.classList.add('whole');tiles.forEach(t=>t.disabled=true);ui.say('Le sceau retrouve son état initial.');ui.done();}}
   tiles.forEach(t=>t.onclick=()=>{const n=Number(t.dataset.tile),at=board.indexOf(n),blank=board.indexOf(0);if(won||!near(at,blank))return;[board[at],board[blank]]=[0,n];moves++;draw();const next=tiles.find(x=>x.classList.contains('movable'));if(document.activeElement===t&&t.disabled)next?.focus({preventScroll:true});});
   draw();return()=>{};
  }
 },
 chronis:{
  title:'À l’heure dite',
  brief:'Trois cadrans portent les symboles des neuf personnages. Quand la fenêtre montre, de l’extérieur vers le centre, l’Observation, la Lumière puis l’Origine, le temps s’ouvre.',
  hint:['/personnages/roster/','Associer symboles, noms et Types'],
  mount(host,ui){
   const rings=[{r:132,label:'extérieur',keys:['raijin','keplar','andaris'],pos:1},{r:96,label:'médian',keys:['aurion','pandore','obsidia'],pos:5},{r:60,label:'intérieur',keys:['chronis','magnora','vaalbara'],pos:7}],target=['andaris','aurion','vaalbara'];let won=false;
   host.innerHTML=`<div class="pz-dials"><svg class="pz-svg" viewBox="0 0 320 320" aria-hidden="true"><circle class="pz-plate-round" cx="160" cy="160" r="156"/>${rings.map((ring,i)=>`<g class="pz-dial" data-dial="${i}"><circle cx="160" cy="160" r="${ring.r}" class="pz-ring"/>${Array.from({length:9},(_,k)=>{const a=k*40*Math.PI/180;return `<path class="pz-tick" d="M${160+Math.sin(a)*(ring.r-6)} ${160-Math.cos(a)*(ring.r-6)}L${160+Math.sin(a)*(ring.r+6)} ${160-Math.cos(a)*(ring.r+6)}"/>`;}).join('')}${ring.keys.map((key,k)=>{const a=k*120*Math.PI/180;return `<g transform="translate(${160+Math.sin(a)*ring.r-13} ${160-Math.cos(a)*ring.r-13})"><g class="pz-upright"><circle cx="13" cy="13" r="15" class="pz-glyph-plate"/><path transform="translate(1 1)" d="${ui.symbols[key]}" class="pz-glyph-path" style="stroke:${ui.characters[key].color}"/></g></g>`;}).join('')}</g>`).join('')}<rect class="pz-window" x="138" y="4" width="44" height="136" rx="22"/><circle cx="160" cy="160" r="20" class="pz-core"/></svg></div>
   <div class="pz-controls pz-dial-controls">${rings.map((ring,i)=>`<span class="pz-dial-row"><span>Anneau ${ring.label}</span><button class="pz-segment" data-turn="${i}" data-step="-1" aria-label="Tourner l’anneau ${ring.label} vers la gauche">↺</button><button class="pz-segment" data-turn="${i}" data-step="1" aria-label="Tourner l’anneau ${ring.label} vers la droite">↻</button></span>`).join('')}<span class="pz-meter" data-window></span></div>`;
   const dials=[...host.querySelectorAll('[data-dial]')],shown=host.querySelector('[data-window]');
   const inWindow=ring=>ring.keys.find((_,k)=>(k*3+((ring.pos%9)+9)%9)%9===0);
   function draw(){rings.forEach((ring,i)=>dials[i].style.setProperty('--rot',`${ring.pos*40}deg`));const seen=rings.map(inWindow);shown.textContent='Fenêtre : '+seen.map(k=>k?ui.characters[k].name:'—').join(' · ');
    if(!won&&seen.every((k,i)=>k===target[i])){won=true;host.classList.add('open');host.querySelectorAll('[data-turn]').forEach(b=>b.disabled=true);ui.say('Observation, Lumière, Origine : la fenêtre s’ouvre.');ui.done();}}
   host.querySelectorAll('[data-turn]').forEach(b=>b.onclick=()=>{if(won)return;rings[Number(b.dataset.turn)].pos+=Number(b.dataset.step);draw();});draw();return()=>{};
  }
 },
 magnora:{
  title:'Polarités',
  brief:'La balle porte une charge positive. Même charge : répulsion ; charges opposées : attraction. Réglez les six aimants pour qu’elle traverse le champ et sorte par le but.',
  hint:['/personnages/magnora/','Découvrir le Type Polarité'],
  mount(host,ui){
   const sides='BTTTTB',walls=['0,0','0,1','1,0','1,4','3,0','3,1','3,2','4,4','5,3'],goal=4,X=c=>120+c*76,Y=r=>50+r*50;let pol=[1,1,1,1,1,1],busy=false,won=false,stopBall=()=>{};
   host.innerHTML=`<div class="pz-field-wrap"><svg class="pz-svg" viewBox="0 0 600 300" aria-hidden="true"><rect class="pz-field" x="8" y="8" width="584" height="284" rx="14"/>${[0,1,2,3,4].map(r=>`<path class="pz-lane faint" d="M30 ${Y(r)}H570"/>`).join('')}${walls.map(w=>{const [c,r]=w.split(',').map(Number);return `<rect class="pz-block" x="${X(c)-22}" y="${Y(r)-20}" width="44" height="40" rx="6"/>`;}).join('')}<g class="pz-goal"><ellipse cx="572" cy="${Y(goal)}" rx="10" ry="22"/></g><path class="pz-trace" d=""/><circle class="pz-player" cx="40" cy="${Y(2)}" r="11"/><circle class="pz-ball" r="7" cx="40" cy="${Y(2)}"/></svg>${[...sides].map((s,c)=>`<button class="pz-magnet ${s==='T'?'top':'bottom'}" data-magnet="${c}" style="left:${X(c)/6}%"></button>`).join('')}</div>
   <div class="pz-controls"><button class="pz-action" data-launch>Lancer la balle</button></div>`;
   const magnets=[...host.querySelectorAll('[data-magnet]')],ball=host.querySelector('.pz-ball'),trace=host.querySelector('.pz-trace'),launch=host.querySelector('[data-launch]');
   function run(){let r=2;const points=[[40,Y(2)]];for(let c=0;c<6;c++){const next=r+(sides[c]==='T'?(pol[c]?1:-1):(pol[c]?-1:1));if(next<0||next>4){points.push([X(c),next<0?14:286]);return {points,ok:false,why:'La balle est projetée hors du champ.'};}
     if(walls.includes(c+','+next)){points.push([X(c)-24,Y(r)+(Y(next)-Y(r))*.6]);return {points,ok:false,why:'La balle s’écrase sur un bloc.'};}r=next;points.push([X(c),Y(r)]);}
    points.push([572,Y(r)]);return {points,ok:r===goal,why:'La balle sort du champ loin du but.'};}
   function draw(){magnets.forEach((m,c)=>{m.textContent=pol[c]?'+':'−';m.classList.toggle('negative',!pol[c]);m.setAttribute('aria-label',`Aimant ${c+1}, ${sides[c]==='T'?'au-dessus':'au-dessous'} du couloir : polarité ${pol[c]?'positive':'négative'}`);m.disabled=busy||won;});launch.disabled=busy||won;}
   magnets.forEach((m,c)=>m.onclick=()=>{pol[c]=pol[c]?0:1;trace.setAttribute('d','');draw();});
   launch.onclick=()=>{if(busy||won)return;busy=true;draw();const result=run();trace.setAttribute('d',polyline(result.points));
    stopBall=travel(ball,result.points,360,()=>{busy=false;if(result.ok){won=true;ui.say('Attraction, répulsion : la balle glisse jusqu’au but.');ui.done();}else{ui.say(result.why);ball.setAttribute('cx',40);ball.setAttribute('cy',Y(2));}draw();});};
   draw();return()=>stopBall();
  }
 }
};
export {norm};
