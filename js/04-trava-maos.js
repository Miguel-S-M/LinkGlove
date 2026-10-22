// Trava: só as mãos de quem travou
// ===== Trava de mãos: depois de travar, só as mãos de quem travou entram (tamanho da mão; com rosto, tamanho da mão em relação ao rosto) =====
let LOCK = null, LOCKREQ = null;
const handPx = (lm,w,h) => Math.max(Math.hypot((lm[9].x-lm[0].x)*w,(lm[9].y-lm[0].y)*h), Math.hypot((lm[5].x-lm[17].x)*w,(lm[5].y-lm[17].y)*h));
const medOf = a => { const q=[...a].sort((x,y)=>x-y); return q[q.length>>1]; };
function lockUi(msg){
  document.querySelectorAll('.btnLock').forEach(b=>{ b.disabled=!!(LOCK||LOCKREQ); });
  document.querySelectorAll('.btnUnlock').forEach(b=>{ b.disabled=!(LOCK||LOCKREQ); });
  if (msg!==undefined) lockTxt(msg);
}
function lockTxt(t){ if (t!==lockTxt.last){ lockTxt.last=t; document.querySelectorAll('.lockSt').forEach(e=>e.textContent=t); } }
function lockFilter(raw,w,h,now){
  if (LOCKREQ){
    if (now>LOCKREQ.until){ LOCKREQ=null; if(hands) hands.setOptions({maxNumHands:2}); lockUi('Não consegui travar: não vi as suas mãos. Ligue a câmera e deixe só você na imagem.'); return raw; }
    if (raw.length){
      const F=faceFresh();
      raw.forEach(lm=>{ const px=handPx(lm,w,h); LOCKREQ.abs.push(px/h); if(F) LOCKREQ.rat.push(px/(F.w*w)); });
      LOCKREQ.n++;
      if (LOCKREQ.n>=15){
        const useRat = LOCKREQ.rat.length>=10 && F;
        LOCK = {abs:medOf(LOCKREQ.abs), ratio:useRat?medOf(LOCKREQ.rat):null, face:useRat?{cx:F.cx,cy:F.cy}:null, ignT:0};
        LOCKREQ=null; lockUi(); lockStatus(now);
      } else lockTxt('Travando... mantenha as mãos na imagem ('+LOCKREQ.n+'/15)');
    }
    return raw;
  }
  if (!LOCK) return raw;
  const F=faceFresh(); let ign=0;
  const keep = raw.filter(lm=>{
    const px=handPx(lm,w,h); let ok;
    if (LOCK.ratio && F){ const r=px/(F.w*w); ok = r>=LOCK.ratio*0.7 && r<=LOCK.ratio*1.3; }   // mão tem de estar à mesma distância do rosto travado
    else { const a=px/h; ok = a>=LOCK.abs*0.65 && a<=LOCK.abs*1.5; }
    if (!ok) ign++; return ok;
  });
  if (ign) LOCK.ignT=now;
  lockStatus(now);
  return keep;
}
function lockStatus(now){
  if (!LOCK) return;
  lockTxt('🔒 Travado nas suas mãos ('+(LOCK.ratio?'tamanho da mão em relação ao rosto':'tamanho da mão')+')'+(now-LOCK.ignT<1000?' · ignorando outra mão':''));
}
document.querySelectorAll('.btnLock').forEach(b=>b.onclick = () => {
  if (!camOn){ lockUi('Ligue a câmera antes de travar.'); return; }
  if (!FACEON){ FACEON=true; FACEFAIL=false; try{ localStorage.setItem('linkglove_rosto','1') }catch(e){} faceUi(); }   // o rosto deixa a trava bem mais precisa
  if (hands) hands.setOptions({maxNumHands:4});   // enxerga mais mãos para não perder as suas se houver outras pessoas
  LOCKREQ = {until:performance.now()+6000, n:0, abs:[], rat:[]};
  lockUi('Travando... mostre as suas mãos, com só você na imagem.');
});
document.querySelectorAll('.btnUnlock').forEach(b=>b.onclick = () => {
  LOCK=null; LOCKREQ=null; if (hands) hands.setOptions({maxNumHands:2});
  lockUi('Trava solta: qualquer mão volta a ser lida.');
});
lockUi();

