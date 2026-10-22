// Service worker do LinkGlove: deixa o site funcionar sem internet.
// Ao mudar arquivos do site, aumente VERSION (e o ?v= no index.html) para o navegador baixar a versão nova.
const VERSION = '1';
const APP = 'linkglove-app-' + VERSION;      // arquivos do próprio site (trocados a cada versão)
const LIB = 'linkglove-lib-v1';              // bibliotecas e modelos de visão do CDN (guardados entre versões)
const APP_FILES = [
 "index.html",
 "manifest.webmanifest",
 "icons/icon-192.png",
 "icons/icon-512.png",
 "icons/icon-maskable-512.png",
 "style.css?v=1",
 "favicon.webp?v=1",
 "js/01-base-tradutor.js?v=1",
 "js/02-nucleo-ia.js?v=1",
 "js/03-rosto.js?v=1",
 "js/04-trava-maos.js?v=1",
 "js/05-aprender-jogos.js?v=1",
 "js/06-classificador.js?v=1",
 "js/07-suavizacao.js?v=1",
 "js/08-sinais-camera.js?v=1",
 "js/09-abas-treino.js?v=1",
 "js/10-voz-jogos.js?v=1",
 "js/11-progresso.js?v=1",
 "js/12-precisao-calibracao.js?v=1",
 "js/13-utilidades.js?v=1",
 "js/14-boxe.js?v=1",
 "js/15-minigames.js?v=1",
 "js/16-online-boxe.js?v=1",
 "js/17-online-rap.js?v=1",
 "js/18-online-corrida.js?v=1",
 "js/19-trilha-licoes.js?v=1",
 "js/20-offline.js?v=1",
 "js/imagens.js?v=1",
 "assets/arte/animado.webp?v=1",
 "assets/arte/capa_casa.webp?v=1",
 "assets/arte/capa_corrida.webp?v=1",
 "assets/arte/capa_rap.webp?v=1",
 "assets/arte/confiante.webp?v=1",
 "assets/arte/conquista.webp?v=1",
 "assets/arte/dica.webp?v=1",
 "assets/arte/erro.webp?v=1",
 "assets/arte/feliz.webp?v=1",
 "assets/arte/link_big.webp?v=1",
 "assets/arte/link_cross.webp?v=1",
 "assets/arte/link_esq.webp?v=1",
 "assets/arte/link_gancho.webp?v=1",
 "assets/arte/link_idle.webp?v=1",
 "assets/arte/link_jab.webp?v=1",
 "assets/arte/link_mascote.webp?v=1",
 "assets/arte/link_upper.webp?v=1",
 "assets/arte/logo.webp?v=1",
 "assets/arte/null_big.webp?v=1",
 "assets/arte/null_cross.webp?v=1",
 "assets/arte/null_esq.webp?v=1",
 "assets/arte/null_gancho.webp?v=1",
 "assets/arte/null_idle.webp?v=1",
 "assets/arte/null_jab.webp?v=1",
 "assets/arte/null_upper.webp?v=1",
 "assets/arte/pensando.webp?v=1",
 "assets/arte/surpreso.webp?v=1",
 "assets/arte/triste.webp?v=1",
 "assets/letras/a.webp?v=1",
 "assets/letras/b.webp?v=1",
 "assets/letras/c.webp?v=1",
 "assets/letras/cedilha.webp?v=1",
 "assets/letras/d.webp?v=1",
 "assets/letras/e.webp?v=1",
 "assets/letras/f.webp?v=1",
 "assets/letras/g.webp?v=1",
 "assets/letras/h.webp?v=1",
 "assets/letras/i.webp?v=1",
 "assets/letras/j.webp?v=1",
 "assets/letras/k.webp?v=1",
 "assets/letras/l.webp?v=1",
 "assets/letras/m.webp?v=1",
 "assets/letras/n.webp?v=1",
 "assets/letras/o.webp?v=1",
 "assets/letras/p.webp?v=1",
 "assets/letras/q.webp?v=1",
 "assets/letras/r.webp?v=1",
 "assets/letras/s.webp?v=1",
 "assets/letras/t.webp?v=1",
 "assets/letras/u.webp?v=1",
 "assets/letras/v.webp?v=1",
 "assets/letras/w.webp?v=1",
 "assets/letras/x.webp?v=1",
 "assets/letras/y.webp?v=1",
 "assets/letras/z.webp?v=1"
];
const CDN_FILES = [
 "https://cdn.jsdelivr.net/npm/@mediapipe/camera_utils@0.3.1675466862/camera_utils.js",
 "https://cdn.jsdelivr.net/npm/@mediapipe/hands@0.4.1675469240/hands.js",
 "https://cdn.jsdelivr.net/npm/@mediapipe/face_detection@0.4.1646425229/face_detection.js",
 "https://cdn.jsdelivr.net/npm/@mediapipe/face_mesh@0.4.1633559619/face_mesh.js",
 "https://cdn.jsdelivr.net/npm/peerjs@1.5.4/dist/peerjs.min.js"
];

self.addEventListener('install', e => {
  e.waitUntil((async () => {
    const app = await caches.open(APP);
    await Promise.allSettled(APP_FILES.map(u => app.add(u)));
    const lib = await caches.open(LIB);
    await Promise.allSettled(CDN_FILES.map(u => lib.add(new Request(u, {mode:'cors', credentials:'omit'}))));
    await self.skipWaiting();
  })());
});

self.addEventListener('activate', e => {
  e.waitUntil((async () => {
    for (const k of await caches.keys()) if (k.startsWith('linkglove-app-') && k !== APP) await caches.delete(k);
    await self.clients.claim();
  })());
});

async function cacheFirst(req, name){
  const c = await caches.open(name);
  const hit = await c.match(req, {ignoreVary:true});
  if (hit) return hit;
  try{
    const res = await fetch(req);
    if (res && res.ok) c.put(req, res.clone());
    return res;
  }catch(err){ return Response.error(); }
}

async function networkFirst(req){            // a página principal: tenta a versão nova, cai para a salva
  const c = await caches.open(APP);
  try{
    const res = await fetch(req);
    if (res && res.ok) c.put(req, res.clone());
    return res;
  }catch(err){
    return (await c.match(req, {ignoreSearch:true})) || (await c.match(new URL('index.html', location.href).href)) || Response.error();
  }
}

self.addEventListener('fetch', e => {
  const r = e.request;
  if (r.method !== 'GET') return;
  const u = new URL(r.url);
  if (u.origin === location.origin){
    if (r.mode === 'navigate' || u.pathname.endsWith('/') || u.pathname.endsWith('/index.html')) e.respondWith(networkFirst(r));
    else if (!u.pathname.endsWith('/sw.js')) e.respondWith(cacheFirst(r, APP));
  } else if (u.hostname === 'cdn.jsdelivr.net'){
    e.respondWith(cacheFirst(r, LIB));        // bibliotecas e modelos do MediaPipe/PeerJS: ficam salvos depois do primeiro uso
  }
});
