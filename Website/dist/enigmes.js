// The sealed archipelago: nine islands, one puzzle each, reconnected around the ORA core.
import {puzzles,norm} from './puzzles.js';
import {sealKeys,isSolved,solvedCount,finalSolved,markSolved,markFinal,auraActive,setAura,resetProgress,finalDate} from './progress.js';

const pad=n=>String(n).padStart(2,'0');
const spot=i=>{const a=i/9*Math.PI*2-Math.PI/2;return [50+Math.cos(a)*38,50+Math.sin(a)*38];};
const rock=`<svg class="island-rock" viewBox="-120 -60 240 205" aria-hidden="true"><path class="rock-side a" d="M-105 0-63 38-24 116-75 72Z"/><path class="rock-side b" d="M-63 38 33 52 8 139-24 116Z"/><path class="rock-side c" d="M33 52 110 9 75 82 8 139Z"/><path class="rock-root" d="M-40 60q6 40-6 78M0 64q-4 44 8 80M38 58q8 34-2 66"/><path class="rock-top" d="M-105 0-48-43 60-36 110 9 33 52-63 38Z"/><path class="rock-inner" d="M-92 0-44-34 56-28 99 9 32 43-58 31Z"/></svg>`;
const current=()=>new URLSearchParams(location.search).get('sceau');
const nextOpen=key=>{const i=sealKeys.indexOf(key);for(let k=1;k<=9;k++){const next=sealKeys[(i+k)%9];if(!isSolved(next))return next;}return null;};

export function enigmaMeta(characters){
 const key=current();
 if(key==='ora')return ['Le sceau d’ORA — Énigmes | ORA','ÉNIGMES / LE SCEAU D’ORA'];
 if(puzzles[key])return [`${puzzles[key].title} — Énigmes | ORA`,`ÉNIGMES / SCEAU ${characters[key].type.toUpperCase()}`];
 return ['L’archipel scellé — Énigmes | ORA','ÉNIGMES / L’ARCHIPEL SCELLÉ'];
}

export function enigmesPage(characters,symbols){
 const key=current();
 if(key==='ora')return finalView(characters,symbols);
 if(puzzles[key])return puzzleView(key,characters,symbols);
 return archipelago(characters,symbols);
}

function archipelago(characters,symbols){
 const count=solvedCount(),done=finalSolved(),points=sealKeys.map((_,i)=>spot(i));
 const bridges=sealKeys.map((key,i)=>{const [x,y]=points[i],c=characters[key].color,live=isSolved(key);const [nx,ny]=points[(i+1)%9],link=live&&isSolved(sealKeys[(i+1)%9]);
  return `<path class="bridge ${live?'live':''}" style="--c:${c}" d="M${x} ${y}L50 50"/>${link?`<path class="bridge live ring" style="--c:${c}" d="M${x} ${y}A38 38 0 0 1 ${nx} ${ny}"/>`:''}`;}).join('');
 return `<section class="screen enigma-screen ${done?'complete':''}" aria-labelledby="screen-title"><div class="enigma-sky" aria-hidden="true"><div class="dust"></div></div>
 <div class="enigma-intro"><p class="eyebrow">04 / DÉCHIFFRER</p><h1 id="screen-title" tabindex="-1">L’archipel<br><em>scellé.</em></h1>
 <p>Neuf îlots, neuf sceaux. Chacun s’inspire du Type d’un personnage d’ORA. Brisez-les pour relier les îlots entre eux, puis éveillez le sceau central.</p>
 <div class="enigma-progress" style="--p:${count/9}"><strong>${count}</strong><span>/ 9 îlots reliés</span><div class="enigma-bar"><i></i></div></div>
 <p class="content-state">Énigmes du site, construites à partir du contenu publié. Elles ne révèlent aucun élément de lore inédit.</p>
 <div class="enigma-actions">${count<9?`<a class="account-primary" href="/enigmes/?sceau=${nextOpen(sealKeys[8])}">${count?'Reprendre':'Commencer'} l’exploration ↗</a>`:`<a class="account-primary" href="/enigmes/?sceau=ora">${done?'Revoir le sceau d’ORA':'Éveiller le sceau d’ORA'} ↗</a>`}${count||done?'<button class="text-action" id="reset-enigmas">Tout réinitialiser</button>':''}</div>
 <p class="note">Votre progression reste dans ce navigateur. Elle n’est pas liée à votre compte.</p></div>
 <div class="archipelago" style="--p:${count/9}"><svg class="archipelago-links" viewBox="0 0 100 100" aria-hidden="true">${bridges}</svg>
 <a class="archipelago-core ${count===9?'awake':''} ${done?'sealed':''}" href="/enigmes/?sceau=ora" aria-label="Le sceau d’ORA — ${done?'éveillé':count===9?'prêt à être éveillé':`encore ${9-count} îlot${9-count>1?'s':''} à relier`}"><span class="core-rings" aria-hidden="true"><i></i><i></i></span><strong aria-hidden="true">ORA</strong><small aria-hidden="true">${done?'SCEAU ÉVEILLÉ':count===9?'PRÊT':`${count} / 9`}</small></a>
 <ol class="islands">${sealKeys.map((key,i)=>{const c=characters[key],[x,y]=points[i],solved=isSolved(key);
  return `<li class="island ${solved?'solved':''}" style="--x:${x}%;--y:${y}%;--i:${i};--character-color:${c.color}"><a href="/enigmes/?sceau=${key}" aria-label="${c.name}, ${c.type} : ${puzzles[key].title} — ${solved?'sceau brisé':'sceau intact'}">${rock}<span class="island-seal" aria-hidden="true"><svg viewBox="0 0 24 24"><path d="${symbols[key]}"/></svg></span><span class="island-label" aria-hidden="true"><small>${pad(i+1)} · ${c.type}</small><strong>${c.name}</strong></span></a></li>`;}).join('')}</ol></div></section>`;
}

function puzzleView(key,characters,symbols){
 const p=puzzles[key],c=characters[key],i=sealKeys.indexOf(key),prev=sealKeys[(i+8)%9],next=sealKeys[(i+1)%9];
 return `<section class="screen enigma-screen puzzle-screen ${isSolved(key)?'solved':''}" style="--character-color:${c.color}" data-seal="${key}" aria-labelledby="screen-title"><div class="enigma-sky" aria-hidden="true"><div class="dust"></div></div>
 <div class="puzzle-copy"><a class="puzzle-back" href="/enigmes/"><span aria-hidden="true">←</span> L’archipel scellé</a>
 <p class="eyebrow">SCEAU ${pad(i+1)} / ${c.type.toUpperCase()} · ${c.name.toUpperCase()}</p><h1 id="screen-title" tabindex="-1">${p.title}</h1>
 <p class="puzzle-brief">${p.brief}</p><p class="puzzle-status" role="status">${isSolved(key)?'Sceau déjà brisé. Vous pouvez rejouer l’énigme.':''}</p>
 <a class="text-action" href="${p.hint[0]}">INDICE · ${p.hint[1]} <span aria-hidden="true">↗</span></a>
 <p class="content-state">Énigme inspirée du Type ${c.type} · pas une mécanique du jeu</p></div>
 <div class="puzzle-area"><div class="puzzle-stage" data-puzzle="${key}"></div>
 <div class="puzzle-success" hidden><span class="success-seal" aria-hidden="true"><svg viewBox="0 0 24 24"><path d="${symbols[key]}"/></svg></span><p class="eyebrow">SCEAU BRISÉ</p><h2>L’îlot de ${c.name} est relié.</h2><p data-success-count></p><div class="success-actions"><a class="account-primary" href="/enigmes/">Voir l’archipel</a><a class="text-action" data-success-next href="/enigmes/">Sceau suivant <span aria-hidden="true">↗</span></a></div></div></div>
 <nav class="puzzle-nav" aria-label="Autres sceaux"><a href="/enigmes/?sceau=${prev}"><span aria-hidden="true">←</span> ${characters[prev].name}</a><span>${pad(i+1)} <i>/ 09</i></span><a href="/enigmes/?sceau=${next}">${characters[next].name} <span aria-hidden="true">→</span></a></nav></section>`;
}

function finalView(characters,symbols){
 const count=solvedCount(),done=finalSolved(),missing=sealKeys.filter(k=>!isSolved(k));
 const body=done?certificate():count<9?`<div class="final-locked"><p>Le cœur de l’archipel ne répond pas encore. ${missing.length} îlot${missing.length>1?'s restent':' reste'} à relier :</p><ul>${missing.map(k=>`<li><a href="/enigmes/?sceau=${k}" style="--character-color:${characters[k].color}"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="${symbols[k]}"/></svg>${characters[k].name} · ${puzzles[k].title}</a></li>`).join('')}</ul></div>`
  :`<div class="final-question"><p>Les neuf îlots sont reliés. Une dernière question garde le cœur : <strong>de quel mot le nom d’ORA s’inspire-t-il ?</strong></p><div class="pz-controls"><label class="pz-answer"><span>Le nom d’ORA s’inspire de…</span><input id="final-answer" autocomplete="off" spellcheck="false" maxlength="20"></label><button class="pz-action" id="final-submit">Éveiller le sceau</button></div><a class="text-action" href="/univers/ora/">INDICE · L’énergie ORA <span aria-hidden="true">↗</span></a></div>`;
 return `<section class="screen enigma-screen final-screen ${done?'complete':''}" aria-labelledby="screen-title"><div class="enigma-sky" aria-hidden="true"><div class="dust"></div></div>
 <div class="puzzle-copy"><a class="puzzle-back" href="/enigmes/"><span aria-hidden="true">←</span> L’archipel scellé</a><p class="eyebrow">SCEAU CENTRAL / ${count} SUR 9 ÎLOTS</p><h1 id="screen-title" tabindex="-1">Le sceau<br><em>d’ORA.</em></h1><p class="puzzle-status" role="status"></p>${body}</div>
 <div class="final-emblem" aria-hidden="true" style="--p:${count/9}">${sealKeys.map((k,i)=>`<i style="--i:${i};--character-color:${characters[k].color}" class="${isSolved(k)?'on':''}"></i>`).join('')}<span>ORA</span></div></section>`;
}

function certificate(){
 const date=finalDate();
 return `<div class="certificate"><p class="eyebrow">ARCHIPEL RELIÉ</p><h2>Les neuf horizons se rejoignent.</h2><p>Vous avez brisé les neuf sceaux et éveillé le cœur de l’archipel${date?` le ${date.toLocaleDateString('fr-FR',{day:'numeric',month:'long',year:'numeric'})}`:''}.</p><p class="note">Récompense du site : le mode Aura illumine l’interface d’ORA dans ce navigateur.</p><button class="pz-action" id="aura-toggle" aria-pressed="${auraActive()}">Mode Aura : ${auraActive()?'activé':'désactivé'}</button></div>`;
}

export function wireEnigmes(characters,symbols,effects,refresh){
 const screen=document.querySelector('.enigma-screen');if(!screen)return()=>{};
 const reset=document.querySelector('#reset-enigmas');
 if(reset)reset.onclick=()=>{if(reset.dataset.confirm){resetProgress();refresh();return;}reset.dataset.confirm='1';reset.textContent='Confirmer : effacer la progression';};
 const status=screen.querySelector('.puzzle-status'),say=text=>{if(status)status.textContent=text;};
 const center=el=>{const r=el.getBoundingClientRect();return [r.left+r.width/2,r.top+r.height/2];};
 if(screen.classList.contains('final-screen')){
  const input=screen.querySelector('#final-answer'),toggle=()=>{const b=screen.querySelector('#aura-toggle');if(b)b.onclick=()=>{setAura(!auraActive());b.setAttribute('aria-pressed',String(auraActive()));b.textContent=`Mode Aura : ${auraActive()?'activé':'désactivé'}`;};};
  if(input){const submit=()=>{if(norm(input.value)==='AURA'){markFinal();screen.classList.add('complete');screen.querySelector('.final-question').outerHTML=certificate();toggle();say('Le sceau d’ORA s’éveille.');effects.burst(...center(screen.querySelector('.final-emblem')),'#f6dcaa');}else{say(input.value.trim()?'Le cœur reste silencieux.':'Proposez un mot.');input.select();}};
   screen.querySelector('#final-submit').onclick=submit;input.addEventListener('keydown',e=>{if(e.key==='Enter')submit();});}
  toggle();return()=>{};
 }
 const stage=screen.querySelector('.puzzle-stage');if(!stage)return()=>{};
 const key=stage.dataset.puzzle,success=screen.querySelector('.puzzle-success');
 const done=()=>{const first=!isSolved(key);markSolved(key);screen.classList.add('solved');const count=solvedCount(),next=nextOpen(key);
  success.querySelector('[data-success-count]').textContent=count===9?'Les neuf îlots sont reliés. Le sceau d’ORA attend.':`${count} îlot${count>1?'s':''} relié${count>1?'s':''} sur 9.`;
  const link=success.querySelector('[data-success-next]');link.href=count===9?'/enigmes/?sceau=ora':`/enigmes/?sceau=${next}`;link.firstChild.textContent=count===9?'Éveiller le sceau d’ORA ':`Sceau suivant · ${characters[next].name} `;
  setTimeout(()=>{if(!success.isConnected)return;success.hidden=false;if(first||count===9)success.querySelector('a').focus({preventScroll:true});},650);effects.burst(...center(stage),characters[key].color);};
 return puzzles[key].mount(stage,{say,done,symbols,characters});
}
