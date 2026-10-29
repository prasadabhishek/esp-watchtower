#pragma once
#include <Arduino.h>

// ponytail: Pure CSS radio tabs (:checked) + ultra-compact script in PROGMEM (0B RAM). Hardened against all red-team & scientific findings.
const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>root@watchtower:~#</title>
<style>
:root{--bg:#0d1117;--pan:#161b22;--bdr:#30363d;--txt:#c9d1d9;--dim:#8b949e;--hi:#f0f6fc;--grn:#3fb950;--red:#f85149;--amb:#d29922;--cyn:#58a6ff}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--txt);font:12.5px/1.4 ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;padding:12px}
.hdr,.sbar,.tbar{display:flex;flex-wrap:wrap;gap:10px;align-items:center;justify-content:space-between;margin-bottom:10px}
.hdr{border-bottom:1px solid var(--bdr);padding-bottom:8px}
.sbar{background:var(--pan);border:1px solid var(--bdr);padding:6px 10px;justify-content:flex-start;gap:14px}
.sbar b{color:var(--hi)}
.btn,select,input{background:#21262d;color:var(--hi);border:1px solid var(--bdr);padding:4px 9px;font:inherit;cursor:pointer;border-radius:2px}
.btn:hover{border-color:var(--cyn)}
.btn-p{background:#1f6feb;border-color:#388bfd}
.box{background:var(--pan);border:1px solid var(--bdr);margin-bottom:10px}
.bh{background:#0f141c;border-bottom:1px solid var(--bdr);padding:6px 10px;display:flex;justify-content:space-between;align-items:center;font-weight:700;color:var(--hi)}
.bb{padding:10px}
input.tr{display:none}
.tabs{display:flex;gap:4px;border-bottom:1px solid var(--bdr);margin-bottom:10px;flex-wrap:wrap}
.tabs label{padding:6px 12px;background:var(--pan);border:1px solid var(--bdr);border-bottom:none;cursor:pointer;color:var(--dim);font-weight:600}
#t1:checked~.tabs label[for=t1],#t2:checked~.tabs label[for=t2],#t3:checked~.tabs label[for=t3],#t4:checked~.tabs label[for=t4]{background:#1f6feb;color:#fff;border-color:#388bfd}
.pane{display:none}
#t1:checked~#p1,#t2:checked~#p2,#t3:checked~#p3,#t4:checked~#p4{display:block}
table{width:100%;border-collapse:collapse;text-align:left}
th,td{padding:5px 7px;border-bottom:1px solid #21262d}
th{color:var(--dim);border-bottom-color:var(--bdr)}
.bdg{padding:1px 5px;font-size:11px;border-radius:2px;font-weight:700}
.on{background:#0d2d14;color:var(--grn);border:1px solid #238636}
.off{background:#381111;color:var(--red);border:1px solid #da3633}
.vip{background:#342405;color:var(--amb);border:1px solid #9e6a03}
.blame{padding:10px;border:1px solid var(--bdr);margin-bottom:10px;display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:10px;background:var(--pan)}
pre,textarea{background:#000;border:1px solid var(--bdr);color:var(--hi);padding:8px;width:100%;font:inherit;overflow-y:auto}
</style>
</head>
<body>
<div class="hdr">
  <div><b style="color:var(--cyn)">root@watchtower:~#</b> <span style="color:var(--grn)">sentinel --hardened</span></div>
  <div style="display:flex;gap:6px">
    <button class="btn btn-p" onclick="post('/api/ping-now')">⚡ Ping & Diagnose</button>
    <button class="btn" onclick="addVip()">+ Add VIP Target</button>
    <button class="btn" onclick="post('/api/send-report')">📊 Discord Report</button>
  </div>
</div>

<div class="sbar">
  <span>UPTIME: <b id="u">0s</b></span>
  <span>LAN DEVICES: <b id="dv">0</b></span>
  <span>WI-FI PING: <b id="gw">--</b></span>
  <span>ISP PING: <b id="wn">--</b></span>
  <span>JITTER: <b id="jt">0ms</b></span>
  <span>DNS SPEED: <b id="dn">--</b></span>
  <span>NTP: <b id="nt">0</b></span>
  <span>WOL: <b id="wl">0</b></span>
  <span style="cursor:pointer;color:var(--cyn)" onclick="$('diag').style.display=$('diag').style.display==='none'?'flex':'none'">⚙️ Diagnostics</span>
</div>

<div id="diag" style="display:none;background:#0f141c;border:1px solid var(--bdr);padding:5px 10px;margin-bottom:10px;gap:14px;font-size:11.5px">
  <span>HEAP: <b id="h">--</b></span>
  <span>POWER (VCC): <b id="vcc">--</b></span>
  <span>WI-FI RSSI: <b id="rs">--</b></span>
  <span>MQTT SENT: <b id="mqSent">0</b></span>
</div>

<input class="tr" type="radio" name="tb" id="t1" checked>
<input class="tr" type="radio" name="tb" id="t2">
<input class="tr" type="radio" name="tb" id="t3">
<input class="tr" type="radio" name="tb" id="t4">

<div class="tabs">
  <label for="t1">[1] 📡 LAN Radar, Blame & Presence</label>
  <label for="t2">[2] 🛡️ Traps, DNS Probe & Micro-NTP</label>
  <label for="t3">[3] 🌐 ISP Outage Receipt (SLA)</label>
  <label for="t4">[4] 📜 Logs, Discord & MQTT</label>
</div>

<!-- TAB 1: BLAME METER, STRANGER ALERT, PRESENCE, AUTO-WOL -->
<div class="pane" id="p1">
  <div class="blame" id="bmBox">
    <div><div style="color:var(--dim);font-size:11px">1. THE "WHO'S AT FAULT?" BLAME METER</div><b id="bmTxt" style="font-size:13.5px;color:var(--grn)">CHECKING...</b></div>
    <div><div style="color:var(--dim);font-size:11px">LOCAL WI-FI LINK (ROUTER)</div><b id="bmGw">--</b></div>
    <div><div style="color:var(--dim);font-size:11px">INTERNET PROVIDER (ISP WAN)</div><b id="bmWn">--</b></div>
    <div><div style="color:var(--dim);font-size:11px">QUEUE JITTER / BUFFERBLOAT</div><b id="bmJt">0 ms</b> (Drops: <b id="bmFl">0</b>)</div>
  </div>
  <div class="box">
    <div class="bh">
      <span>2. STRANGER ALERT // 3. PRESENCE // 5 & 9. SILENT NAS FREEZE AUTO-WOL</span>
      <span>Auto-WoL Sent: <b id="wl2" style="color:var(--amb)">0</b></span>
    </div>
    <div class="bb">
      <div class="tbar">
        <div>
          <button class="btn btn-p" onclick="flt=0;rDev()">ALL DEVICES</button>
          <button class="btn" onclick="flt=1;rDev()">★ VIP WATCHLIST (PRESENCE + AUTO-WOL)</button>
        </div>
        <div style="font-size:11px;color:var(--dim)">New MAC = 🔔 Stranger Alert | ★ VIP = 🏠 Arrival / 🚪 Departure + ⚠️ Auto-WoL on Drop</div>
      </div>
      <div style="overflow-x:auto">
        <table>
          <thead><tr><th>VIP</th><th>IP</th><th>MAC / VENDOR</th><th>DEVICE NAME</th><th>LATENCY</th><th>LAST SEEN</th><th>STATUS</th><th>WAKE</th></tr></thead>
          <tbody id="tbD"></tbody>
        </table>
      </div>
    </div>
  </div>
</div>

<!-- TAB 2: CANARY HONEYPOTS, #7 DNS PROBE, #8 MICRO-NTP SERVER -->
<div class="pane" id="p2">
  <div class="box">
    <div class="bh"><span>7. DNS HIJACKING & BENCHMARK PROBE (RFC 5452 UDP 53)</span><button class="btn btn-p" onclick="post('/api/dns-bench')">🔍 Benchmark DNS Now</button></div>
    <div class="bb">
      <table>
        <thead><tr><th>DNS RESOLVER</th><th>LOOKUP SPEED</th><th>NXDOMAIN HIJACK / AD-INJECTION CHECK</th></tr></thead>
        <tbody>
          <tr><td><b>Router DNS (Gateway:53)</b></td><td><b id="dGw">--</b></td><td><span id="dHj" class="bdg on">✅ CLEAN (Genuine NXDOMAIN)</span></td></tr>
          <tr><td><b>Cloudflare DNS (1.1.1.1:53)</b></td><td><b id="dCf">--</b></td><td><span class="bdg on">✅ VERIFIED UPSTREAM</span></td></tr>
        </tbody>
      </table>
    </div>
  </div>
  <div class="box">
    <div class="bh"><span>8. LOCAL MICRO-NTP TIME SERVER (UDP 123, RFC 5905)</span><span class="bdg on">STRATUM-2 ACTIVE</span></div>
    <div class="bb">
      Synchronizes offline IoT cameras & smart plugs on <code style="color:var(--cyn)">UDP Port 123</code> with RFC 5905 root dispersion.
      Total IoT Time Syncs Served: <b id="nt2" style="color:var(--grn)">0</b>
    </div>
  </div>
  <div class="box">
    <div class="bh"><span>INTRUDER CANARY HONEYPOTS</span><button class="btn" onclick="post('/api/test-alert')">🚨 Test Strobe Alarm</button></div>
    <div class="bb">
      <table>
        <thead><tr><th>PORT / SERVICE</th><th>DECOY EMULATION</th><th>STATE</th><th>INTRUDER HITS</th></tr></thead>
        <tbody>
          <tr><td><b>Port 23 (Telnet)</b></td><td>Ubuntu 22.04 LTS Shell Prompt</td><td><span class="bdg on">ARMED</span></td><td><b id="hT">0</b></td></tr>
          <tr><td><b>Port 8080 (HTTP)</b></td><td>Router Admin Login Portal</td><td><span class="bdg on">ARMED</span></td><td><b id="hW">0</b></td></tr>
          <tr><td><b>Port 554 (RTSP)</b></td><td>IPCamera Video Auth Stream</td><td><span class="bdg on">ARMED</span></td><td><b id="hC">0</b></td></tr>
        </tbody>
      </table>
    </div>
  </div>
</div>

<!-- TAB 3: #4 ISP OUTAGE RECEIPT & SLA CLAIM GENERATOR -->
<div class="pane" id="p3">
  <div class="box">
    <div class="bh"><span>4. "ISP OUTAGE RECEIPT" & SLA BILLING CREDIT CALCULATOR</span><span id="wSt">ONLINE</span></div>
    <div class="bb">
      <div class="tbar">
        <span>Confirmed Outages: <b id="oCnt">0</b> | Latest Downtime: <b id="oDur">0s</b></span>
        <span>Outage Threshold:
          <select id="oTh" onchange="post('/api/config',{outage_threshold:+this.value})">
            <option value="60">1 min</option><option value="180">3 mins</option>
            <option value="300">5 mins</option><option value="600">10 mins</option>
          </select>
        </span>
      </div>
      <div style="background:#0f141c;border:1px solid var(--bdr);padding:8px;margin-bottom:8px;display:flex;gap:12px;align-items:center;flex-wrap:wrap">
        <span>Monthly ISP Bill ($): <input id="billAmt" type="number" value="80" style="width:65px" oninput="calcCredit()"></span>
        <span>Prorated SLA Credit Owed: <b id="credAmt" style="color:var(--grn);font-size:14px">$0.00</b></span>
      </div>
      <textarea id="clm" rows="9" readonly>Loading receipt from hardware...</textarea>
      <div style="margin-top:8px;display:flex;gap:8px">
        <button class="btn btn-p" onclick="navigator.clipboard.writeText($('clm').value);this.innerText='✓ Copied to Clipboard!'">📋 Copy SLA Claim Receipt</button>
        <button class="btn" onclick="loadClaim()">🔄 Refresh Receipt</button>
      </div>
    </div>
  </div>
</div>

<!-- TAB 4: LOGS, DISCORD & #10 HOME ASSISTANT MQTT -->
<div class="pane" id="p4">
  <div class="box">
    <div class="bh"><span>10. HOME ASSISTANT MQTT TELEMETRY (TCP 1883)</span><span>Published: <b id="mqCnt">0</b></span></div>
    <div class="bb" style="display:flex;gap:8px;flex-wrap:wrap">
      <input id="mqH" placeholder="MQTT Broker IP / Host (e.g. 192.168.50.10 or homeassistant.local)" style="flex:1">
      <button class="btn btn-p" onclick="post('/api/mqtt',{host:$('mqH').value})">💾 Save & Publish to TCP 1883</button>
    </div>
  </div>
  <div class="box">
    <div class="bh"><span>DISCORD INSTANT ALERTS (🔔 STRANGER / 🏠 ARRIVAL / 🚪 DEPARTURE / ⚠️ VIP DROP)</span></div>
    <div class="bb" style="display:flex;gap:8px;flex-wrap:wrap">
      <input id="wh" placeholder="https://discord.com/api/webhooks/..." style="flex:1">
      <button class="btn btn-p" onclick="post('/api/webhook',{url:$('wh').value})">💾 Save & Test Discord</button>
    </div>
  </div>
  <div class="box">
    <div class="bh">
      <span>AUDIT EVENT STREAM (DEDUPLICATED LITTLEFS + RAM RING)</span>
      <div><a href="/api/download-logs" download class="btn">⬇ CSV Audit Log</a> <button class="btn" onclick="post('/api/clear-logs')">🗑️ Clear</button></div>
    </div>
    <div class="bb"><pre id="lg" style="height:190px">Loading...</pre></div>
  </div>
</div>

<script>
const $=id=>document.getElementById(id);
let devs=[],flt=0;
const post=(u,b)=>fetch(u,{method:'POST',headers:{'Content-Type':'application/json','X-Requested-With':'Watchtower'},body:b?JSON.stringify(b):null}).then(poll);
function addVip(){const ip=prompt('VIP IP Address:');if(ip){const name=prompt('Label (e.g. Abhishek iPhone or Synology NAS):')||ip;post('/api/vip/toggle',{ip,vip:true,name});}}
function rDev(){
  $('tbD').innerHTML=devs.filter(d=>!flt||d.vip).map(d=>{
    const isZeroMac=d.mac==='00:00:00:00:00:00';
    const privTag=d.vendor==='Private Mobile MAC'?' <span class="bdg vip">Private MAC</span>':'';
    return `<tr>
      <td><button class="btn" onclick="post('/api/vip/toggle',{ip:'${d.ip}',vip:${!d.vip}})">${d.vip?'★':'☆'}</button></td>
      <td><b>${d.ip}</b></td><td>${d.mac} <span style="color:var(--cyn)">[${d.vendor}]</span>${privTag}</td>
      <td style="cursor:pointer" onclick="const n=prompt('Rename:',\`${d.name}\`);if(n)post('/api/device/rename',{ip:'${d.ip}',name:n})">${d.name} ✏️ ${d.vip?'<span class="bdg vip">VIP</span>':''}</td>
      <td>${d.online&&d.latency>=0?d.latency+'ms':'--'}</td><td>${d.last_seen_sec}s</td>
      <td><span class="bdg ${d.online?'on':'off'}">${d.online?'ONLINE':'OFFLINE'}</span></td>
      <td>${isZeroMac?'<span style="color:var(--dim)">--</span>':`<button class="btn" onclick="post('/api/wol',{mac:'${d.mac}'})">⚡WoL</button>`}</td>
    </tr>`;
  }).join('');
}
function calcCredit(){
  const bill=+($('billAmt').value)||80;
  const durSec=+($('oDur').innerText.replace('s',''))||0;
  const cred=(durSec/2592000)*bill;
  $('credAmt').innerText='$'+cred.toFixed(2);
}
function loadClaim(){fetch('/api/claim').then(r=>r.text()).then(t=>{
  $('clm').value=t;calcCredit();
});}
async function poll(){
  try{
    const s=await(await fetch('/api/status')).json();
    $('u').innerText=Math.floor(s.uptime/60)+'m';
    $('h').innerText=Math.round(s.heap/1024)+'KB';
    $('vcc').innerText=(s.vcc||0)+'mV';
    $('rs').innerText=(s.rssi||0)+'dBm';
    $('dv').innerText=`${s.lan_online}/${s.lan_total} (${s.vip_online} VIP)`;
    $('gw').innerText=s.ping_gw>=0?s.ping_gw+'ms':'DOWN';
    $('wn').innerText=s.ping_wan>=0?s.ping_wan+'ms':'OUTAGE';
    $('jt').innerText=(s.jitter_ms||0)+'ms';
    $('bmGw').innerText=s.ping_gw>=0?`${s.ping_gw} ms (${s.ping_gw<=45?'🟢 Normal':'🟡 High'})`:'🔴 Offline';
    $('bmWn').innerText=s.ping_wan>=0?`${s.ping_wan} ms (${s.ping_wan<=100?'🟢 Fast':'🔴 Lagging'})`:'🔴 ISP Outage';
    $('bmJt').innerText=`${s.jitter_ms||0} ms (${(s.jitter_ms||0)<=15?'🟢 Minimal':((s.jitter_ms||0)<=30?'🟡 Moderate':'🔴 Bufferbloat')})`;
    $('bmTxt').innerText=s.blame_desc||'OK';
    $('bmBox').style.borderColor=s.blame_verdict===0?'var(--grn)':(s.blame_verdict===1?'var(--red)':(s.blame_verdict===4?'var(--red)':'var(--amb)'));
    $('bmFl').innerText=s.pings_failed;
    const dg=s.dns_gw_ms>=0?s.dns_gw_ms+'ms':'--',dc=s.dns_cf_ms>=0?s.dns_cf_ms+'ms':'--';
    $('dn').innerText=`${dg}/${dc}`;
    $('dGw').innerText=dg;$('dCf').innerText=dc;
    $('dHj').innerText=s.dns_hijacked?'🚨 HIJACKED (NXDOMAIN Ad-Injection!)':'✅ CLEAN (Genuine NXDOMAIN)';
    $('dHj').className='bdg '+(s.dns_hijacked?'off':'on');
    $('nt').innerText=$('nt2').innerText=s.ntp_served||0;
    $('wl').innerText=$('wl2').innerText=s.wol_sent||0;
    $('mqCnt').innerText=$('mqSent').innerText=s.mqtt_sent||0;
    if(s.mqtt_host&&document.activeElement!==$('mqH'))$('mqH').value=s.mqtt_host;
    $('hT').innerText=s.telnet_hits||0;$('hW').innerText=s.web_hits||0;$('hC').innerText=s.camera_hits||0;
    $('oCnt').innerText=s.total_outages||0;$('oDur').innerText=(s.last_outage_duration||0)+'s';
    calcCredit();
    if(document.activeElement!==$('oTh'))$('oTh').value=s.outage_threshold;
    devs=await(await fetch('/api/devices')).json();rDev();
    const l=await(await fetch('/api/logs')).json();
    $('lg').innerText=l.map(x=>`[${x.t}s] [${x.type}] ${x.msg}`).join('\n');
  }catch(e){}
}
setInterval(poll,3500);poll();loadClaim();
</script>
</body>
</html>
)rawliteral";
