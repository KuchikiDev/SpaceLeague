// Suspended terrain drawn in code; visual exploration, not canonical geography.
import {animationLoop} from './animation-loop.js';
export function createAmbient(host,isReduced){
 const canvas=document.createElement('canvas');canvas.className='ambient-canvas';canvas.setAttribute('aria-hidden','true');const atlas=host.classList.contains('atlas-screen');const surface=atlas?host.querySelector('.map-surface'):host;surface.prepend(canvas);
 const ctx=canvas.getContext('2d');let width=0,height=0,time=0;
 const color=getComputedStyle(host).getPropertyValue('--character-color').trim()||'#a5dad1';
 const ro=new ResizeObserver(()=>{const b=surface.getBoundingClientRect();width=b.width;height=b.height;const d=Math.min(devicePixelRatio,1.5);canvas.width=width*d;canvas.height=height*d;ctx.setTransform(d,0,0,d,0,0);draw(time);});ro.observe(surface);
 function poly(points,fill,stroke){ctx.beginPath();points.forEach(([x,y],i)=>i?ctx.lineTo(x,y):ctx.moveTo(x,y));ctx.closePath();if(fill){ctx.fillStyle=fill;ctx.fill();}if(stroke){ctx.strokeStyle=stroke;ctx.lineWidth=1;ctx.stroke();}}
 function island(x,y,s,t,arena=false){ctx.save();ctx.translate(x,y+Math.sin(t*.6+x)*7);ctx.scale(s,s);
 const top=[[-210,0],[-95,-87],[119,-71],[221,18],[66,104],[-126,76]];
 poly([[-210,0],[-126,76],[-48,232],[-150,143]],'#152e36','#35505a');poly([[-126,76],[66,104],[17,278],[-48,232]],'#1b3940','#45636a');poly([[66,104],[221,18],[150,164],[17,278]],'#102730','#36535f');
 poly([[-126,76],[-25,124],[-48,232]],'#29434a');poly([[66,104],[150,164],[17,278]],'#19343e');poly([[-210,0],[-95,-87],[-106,-32],[-150,143]],'#213c43');
 poly(top,'#304e4d','#90afa0');poly([[-192,0],[-90,-72],[114,-56],[203,19],[64,87],[-120,62]],'#405c54','#728f7a');
 if(arena){ctx.save();ctx.transform(.92,.16,-.68,.44,0,2);poly([[-115,-85],[115,-85],[115,85],[-115,85]],'#152e35',color);ctx.strokeStyle=color;ctx.globalAlpha=.9;ctx.strokeRect(-104,-74,208,148);ctx.beginPath();ctx.moveTo(0,-74);ctx.lineTo(0,74);ctx.stroke();ctx.beginPath();ctx.ellipse(0,0,29,29,0,0,Math.PI*2);ctx.stroke();ctx.strokeRect(-104,-32,25,64);ctx.strokeRect(79,-32,25,64);ctx.shadowColor=color;ctx.shadowBlur=14;ctx.fillStyle='#dfc491';ctx.fillRect(-118,-25,5,50);ctx.fillRect(113,-25,5,50);ctx.restore();
 // The ball never stops: it rebounds inside the arena, projected onto the same tilted plane, with a short trail.
 const tri=v=>Math.abs(((v%2)+2)%2-1)*2-1;for(let k=6;k>=0;k--){const tt=t-k*.04,bx=tri(tt*.31)*96,by=tri(tt*.47+.3)*66;ctx.globalAlpha=k?.32-k*.04:1;ctx.fillStyle=k?color:'#fff3d3';ctx.shadowColor=color;ctx.shadowBlur=k?0:18;ctx.beginPath();ctx.arc(.92*bx-.68*by,.16*bx+.44*by+2,k?4.6-k*.5:5.5,0,Math.PI*2);ctx.fill();}ctx.shadowBlur=0;ctx.globalAlpha=1;}
 for(let i=0;i<3;i++){const px=-90+i*87;ctx.strokeStyle='#a5e3da';ctx.globalAlpha=.2;ctx.beginPath();ctx.moveTo(px,107);ctx.quadraticCurveTo(px+Math.sin(t+i)*12,225,px-15,310+Math.sin(t+i)*14);ctx.stroke();}ctx.globalAlpha=1;ctx.restore();}
 function draw(t){ctx.clearRect(0,0,width,height);const mobile=width<761,cx=width*(mobile?.67:.75),cy=mobile?230:height*.49,s=Math.min(width*(mobile?.0015:.00135),height*.00135);
 const glow=ctx.createRadialGradient(cx,cy,10,cx,cy,500*s);glow.addColorStop(0,'#486c6238');glow.addColorStop(1,'#10212b00');ctx.fillStyle=glow;ctx.fillRect(0,0,width,height);
 if(atlas){const positions=width<761?[[.27,.48],[.69,.35],[.7,.77]]:[[.40,.36],[.72,.27],[.74,.66]];positions.forEach(([px,py],i)=>{const x=width*px,y=height*py;island(x,y,Math.min(width/1100,height/850)*[.62,.72,.58][i],t,i===1);const node=host.querySelector('.node-'+i);node.style.left=x+'px';node.style.top=(y+Math.sin(t*.6+x)*7)+'px';});}else{ctx.globalAlpha=.4;island(cx-225*s,cy-165*s,.33*s,t+2);island(cx+230*s,cy-115*s,.43*s,t+4);ctx.globalAlpha=1;island(cx,cy,s,t,true);
 ctx.globalAlpha=.6;island(cx-265*s,cy+163*s,.23*s,t+1);island(cx+270*s,cy+215*s,.19*s,t+5);ctx.globalAlpha=1;}
 for(let i=0;i<35;i++){const x=(i*173.7+t*(5+i%4))%(width+100)-50,y=(i*91.3)%height;ctx.globalAlpha=.15+.2*Math.sin(t*.6+i)**2;ctx.fillStyle=color;ctx.fillRect(x,y,2,2);}
 for(let i=0;i<6;i++){ctx.globalAlpha=.08;ctx.strokeStyle='#c3dfcf';ctx.beginPath();const y=cy+(i-2)*65*s;const x=cx+Math.sin(t*.16+i)*40;ctx.ellipse(x,y,(240+i*21)*s,12*s,-.08,0,Math.PI*2);ctx.stroke();}ctx.globalAlpha=1;
 }
 const stop=animationLoop(dt=>{time+=dt;draw(time);},isReduced);
 return()=>{stop();ro.disconnect();canvas.remove();};
}
