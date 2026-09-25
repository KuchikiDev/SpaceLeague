// Puzzle progress stays in this browser only; it is never sent to the account API.
const KEY='ora-archipel';
export const sealKeys=['raijin','keplar','andaris','pandore','obsidia','aurion','vaalbara','chronis','magnora'];
function load(){try{const value=JSON.parse(localStorage.getItem(KEY)||'{}');return value&&typeof value==='object'?value:{};}catch{return {};}}
let state=load();
function save(){try{localStorage.setItem(KEY,JSON.stringify(state));}catch{}document.documentElement.classList.toggle('aura',auraActive());document.dispatchEvent(new CustomEvent('ora-progress'));}
export const isSolved=key=>Boolean(state.solved?.[key]);
export const solvedCount=()=>sealKeys.filter(isSolved).length;
export const finalSolved=()=>Boolean(state.final);
export const auraActive=()=>finalSolved()&&state.aura!==false;
export function markSolved(key){if(isSolved(key))return;state={...state,solved:{...state.solved,[key]:Date.now()}};save();}
export function markFinal(){state={...state,final:Date.now(),aura:true};save();}
export function setAura(on){state={...state,aura:Boolean(on)};save();}
export function resetProgress(){state={};save();}
export function finalDate(){return state.final?new Date(state.final):null;}
document.documentElement.classList.toggle('aura',auraActive());
