#pragma once
// Osmonauts – OpenOsmosis Controller · web interface (ESP32), served from flash
// (c) 2026 Kai Mommsen & Elias Dörre · SPDX-License-Identifier: CC-BY-NC-SA-4.0

static const char OSMO_PAGE[] = R"rawliteral(<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Osmonauts Controller</title>
<style>
:root{--g:#1D9E75;--r:#E24B4A;--b:#3B8BD4;--t:#1f2933;--m:#6b7785;--bg:#f3f5f7}
*{box-sizing:border-box}
body{margin:0;font-family:Helvetica,Arial,sans-serif;background:var(--bg);color:var(--t)}
header{background:#172a40;color:#fff;padding:14px 18px}
header h1{margin:0;font-size:20px}
header div{font-size:12px;opacity:.75;margin-top:2px}
main{max-width:760px;margin:0 auto;padding:12px;display:grid;gap:12px}
.card{background:#fff;border-radius:12px;padding:14px 16px;box-shadow:0 1px 4px rgba(0,0,0,.08)}
.card h2{font-size:14px;margin:0 0 10px;color:var(--m);text-transform:uppercase;letter-spacing:.5px}
#step{font-size:22px;font-weight:700}
#mode{font-size:13px;color:var(--m)}
.bar{height:8px;background:#e4e7eb;border-radius:4px;overflow:hidden;margin:10px 0 4px}
.bar div{height:100%;width:0;background:var(--g);transition:width .5s}
.row{display:flex;justify-content:space-between;font-size:13px;color:var(--m)}
.valves{display:grid;grid-template-columns:repeat(4,1fr);gap:8px;margin-top:12px}
.v{border:1px solid #dde1e5;border-radius:8px;padding:8px;text-align:center;font-size:12px}
.v b{display:block;font-size:15px}
.v.on{border-color:var(--r);background:#fdeeee}
.v.on b{color:var(--r)}
.btns{display:flex;flex-wrap:wrap;gap:8px}
button{font:inherit;font-size:14px;border:1px solid #c9ced3;background:#fff;border-radius:8px;padding:9px 12px;cursor:pointer}
button:hover{border-color:var(--g)}
button.stop{background:var(--r);border-color:var(--r);color:#fff;font-weight:700}
button.pri{background:var(--g);border-color:var(--g);color:#fff}
button:disabled{opacity:.45;cursor:default}
.kv{display:grid;grid-template-columns:1fr auto;gap:4px 12px;font-size:14px}
.kv span:nth-child(odd){color:var(--m)}
.note{font-size:12px;color:var(--m);margin-top:8px;line-height:1.5}
.alarm{background:#fdeeee;border:2px solid var(--r)}
#msg{font-size:13px;min-height:18px;color:var(--g)}
label{display:flex;gap:8px;align-items:flex-start;font-size:14px;margin:6px 0}
input[type=password]{font:inherit;padding:7px;border:1px solid #c9ced3;border-radius:8px;width:100%}
footer{font-size:11px;color:var(--m);text-align:center;padding:10px 0 20px}
footer a{color:var(--m)}
</style>
</head>
<body>
<header><h1>Osmonauts – OpenOsmosis Controller</h1><div id="dev">…</div></header>
<main>
  <section class="card" id="statuscard">
    <h2>Status</h2>
    <div id="step">…</div>
    <div id="mode"></div>
    <div class="bar"><div id="pbar"></div></div>
    <div class="row"><span id="elapsed"></span><span id="left"></span></div>
    <div class="valves">
      <div class="v" id="v0"><b>V1</b>Zulauf</div>
      <div class="v" id="v1"><b>V2</b>Zapfen</div>
      <div class="v" id="v2"><b>V3</b>Standwasser</div>
      <div class="v" id="v3"><b>V4</b>Spülung</div>
    </div>
  </section>

  <section class="card">
    <h2>Bedienung</h2>
    <div class="btns">
      <button class="stop" onclick="cmd('stop')">STOPP</button>
      <button onclick="cmd('flush','Jetzt eine Spülung (Membran + Standwasser + Nachspülung) starten?')">Jetzt spülen</button>
      <button onclick="cmd('filter','Filterwechsel-Spülung starten? Dauer ca. 30 Minuten, das Wasser läuft in den Abfluss.')">Filterwechsel</button>
      <button onclick="cmd('disinfect','Desinfektion starten? Du musst zweimal H2O2 einspritzen und einen Behälter unter den Zapfhahn stellen.')">Desinfektion</button>
    </div>
    <div id="msg"></div>
    <div class="note">Wasser zapfen geht aus Sicherheitsgründen nur am Gerät (Taster 1). Der Aquastop arbeitet unabhängig vom WLAN.</div>
  </section>

  <section class="card">
    <h2>Anlage</h2>
    <div class="kv">
      <span>Aquastop-Sensor</span><span id="aq">–</span>
      <span>Nächste Auto-Spülung</span><span id="nextflush">–</span>
      <span>Nachspülung</span><span id="post">–</span>
      <span>Laufzeit seit Start</span><span id="uptime">–</span>
      <span>WLAN</span><span id="wifi">–</span>
    </div>
  </section>

  <section class="card">
    <h2>Statistik (nur auf diesem Gerät)</h2>
    <div class="kv" id="stats"></div>
  </section>

  <section class="card">
    <h2>Firmware</h2>
    <div class="kv">
      <span>Installiert</span><span id="fw">–</span>
      <span>Auf GitHub</span><span id="fwnew">–</span>
    </div>
    <div class="btns" style="margin-top:10px">
      <button onclick="cmd('check')">Nach Update suchen</button>
      <button class="pri" id="inst" onclick="cmd('install','Update jetzt installieren? Der Controller startet danach neu.')" disabled>Update installieren</button>
    </div>
    <div class="note">Oder eine Firmware-Datei (.bin) hochladen – nur im Leerlauf:</div>
    <input type="file" id="file" accept=".bin">
    <button onclick="upload()">Hochladen</button>
    <div class="bar"><div id="ubar"></div></div>
    <div id="umsg" class="note"></div>
  </section>

  <section class="card">
    <h2>Einstellungen</h2>
    <label><input type="checkbox" id="upd"> <span>Einmal täglich nach Updates suchen<br><small class="note">Dabei wird eine kleine Datei von GitHub geladen. GitHub zählt die Abrufe – so sehen die Entwickler, wie viele Osmonauts aktiv sind. Es werden keine Geräte- oder Nutzungsdaten übertragen; GitHub sieht wie bei jedem Abruf die IP-Adresse deines Anschlusses.</small></span></label>
    <label><input type="checkbox" id="auto"> <span>Updates automatisch im Leerlauf installieren</span></label>
    <label style="display:block">Neues Passwort für diese Seite (mind. 8 Zeichen, leer = unverändert)<input type="password" id="pw" autocomplete="new-password"></label>
    <div class="btns" style="margin-top:8px">
      <button class="pri" onclick="save()">Speichern</button>
      <button onclick="cmd('reboot','Controller neu starten? Alle Ventile werden geschlossen.')">Neustart</button>
      <button onclick="cmd('wifireset','WLAN-Zugangsdaten löschen? Danach startet der Einrichtungs-Hotspot.')">WLAN zurücksetzen</button>
    </div>
  </section>
</main>
<footer>Osmonauts – OpenOsmosis Controller · Kai Mommsen &amp; Elias Dörre · CC BY-NC-SA 4.0 · <a href="https://github.com/MoManufaktur/Osmonauts-Controller" target="_blank">GitHub</a></footer>
<script>
const $=id=>document.getElementById(id);
const fmt=s=>{s=Math.max(0,Math.round(s));const h=Math.floor(s/3600),m=Math.floor(s%3600/60),x=s%60;return h?`${h} h ${m} min`:m?`${m} min ${x} s`:`${x} s`};
const MODES=['Leerlauf','Wasser zapfen','Auto-Spülung','Nachspülung','Filterwechsel','Desinfektion','AQUASTOP'];
const STATS=[['taps','Zapfvorgänge'],['postFlushes','Nachspülungen'],['autoFlushes','Auto-Spülungen'],['filterChanges','Filterwechsel'],['disinfections','Desinfektionen'],['aquastops','Aquastop-Auslösungen'],['stops','Stopps'],['boots','Starts']];
let first=true;
async function poll(){
  try{
    const d=await (await fetch('/api/status',{cache:'no-store'})).json();
    $('dev').textContent=`${d.variant} · Firmware ${d.version} · ${d.ip}`;
    $('step').textContent=d.step; $('mode').textContent=MODES[d.mode]||'';
    $('statuscard').classList.toggle('alarm',d.mode==6);
    const p=d.total>0?Math.min(1,d.elapsed/d.total):0;
    $('pbar').style.width=(p*100)+'%';
    $('elapsed').textContent=d.total>0?'seit '+fmt(d.elapsed):'';
    $('left').textContent=d.total>0?'noch '+fmt(d.total-d.elapsed):'';
    for(let i=0;i<4;i++)$('v'+i).classList.toggle('on',!!(d.relais&(1<<i)));
    $('aq').textContent=d.aq.enabled?`${d.aq.value} (Schwelle ${d.aq.threshold}${d.aq.count?', nass '+d.aq.count+'×':''})`:'nicht aktiv';
    $('nextflush').textContent=fmt(d.nextFlush);
    $('post').textContent=d.postPending?'in '+fmt(d.postIn):'–';
    $('uptime').textContent=fmt(d.uptime);
    $('wifi').textContent=`${d.ssid} (${d.rssi} dBm)`;
    $('stats').innerHTML=STATS.map(([k,l])=>`<span>${l}</span><span>${d.stats[k]}</span>`).join('');
    $('fw').textContent=d.version;
    const u=d.update;
    $('fwnew').textContent=u.busy?'… '+(u.progress?u.progress+' %':'prüfe'):u.error?u.error:u.available?`${u.latest} verfügbar`:u.checked?`${u.latest||d.version} – aktuell`:'noch nicht geprüft';
    $('inst').disabled=!u.available||u.busy||d.mode!=0;
    if(d.message)$('msg').textContent=d.message;
    if(first){$('upd').checked=d.settings.upd;$('auto').checked=d.settings.auto;first=false;}
  }catch(e){$('msg').textContent='Keine Verbindung zum Controller …';}
}
async function post(url,data){const r=await fetch(url,{method:'POST',body:new URLSearchParams(data)});return r.text();}
async function cmd(c,q){if(q&&!confirm(q))return;$('msg').textContent=await post('/api/cmd',{c});poll();}
async function save(){const pw=$('pw').value;if(pw&&pw.length<8){$('msg').textContent='Passwort zu kurz (mind. 8 Zeichen)';return;}
  $('msg').textContent=await post('/api/settings',{upd:$('upd').checked?1:0,auto:$('auto').checked?1:0,pw});$('pw').value='';}
function upload(){const f=$('file').files[0];if(!f){$('umsg').textContent='Bitte zuerst eine .bin-Datei auswählen.';return;}
  if(!confirm('Firmware '+f.name+' hochladen und installieren?'))return;
  const x=new XMLHttpRequest(),fd=new FormData();fd.append('firmware',f,f.name);
  x.upload.onprogress=e=>{if(e.lengthComputable)$('ubar').style.width=(e.loaded/e.total*100)+'%';};
  x.onload=()=>{$('umsg').textContent=x.responseText;};x.onerror=()=>{$('umsg').textContent='Upload fehlgeschlagen';};
  x.open('POST','/api/update/upload');x.send(fd);$('umsg').textContent='Lade hoch …';}
poll();setInterval(poll,1500);
</script>
</body>
</html>
)rawliteral";
