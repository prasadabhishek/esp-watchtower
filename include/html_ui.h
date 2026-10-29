#pragma once
#include <Arduino.h>

// ponytail: Pure CSS + Server-Rendered HTML in PROGMEM (0B Heap RAM).
// Sortable table headers with NO JAVASCRIPT (100% C++ server-side sorting).
// All major features front-and-center: Blame Meter, DNS Guard, Canary Traps, Verified Outages, Device List, NTP/MQTT & Configurable Alerts.

const char HTML_PAGE_HEAD[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>root@watchtower:~#</title>
<style>
:root{--bg:#0d1117;--pan:#161b22;--bdr:#30363d;--txt:#c9d1d9;--dim:#8b949e;--hi:#f0f6fc;--grn:#3fb950;--red:#f85149;--amb:#d29922;--cyn:#58a6ff}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--txt);font:12.5px/1.4 ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;padding:12px}
a{color:var(--cyn);text-decoration:none}
a:hover{text-decoration:underline}
.hdr,.sbar,.tbar{display:flex;flex-wrap:wrap;gap:10px;align-items:center;justify-content:space-between;margin-bottom:10px}
.hdr{border-bottom:1px solid var(--bdr);padding-bottom:8px}
.sbar{background:var(--pan);border:1px solid var(--bdr);padding:6px 10px;justify-content:flex-start;gap:14px}
.sbar b{color:var(--hi)}
.nav{display:flex;gap:6px;flex-wrap:wrap;margin-bottom:12px;padding:6px;background:var(--pan);border:1px solid var(--bdr);font-size:11.5px}
.nav a{padding:3px 8px;background:#21262d;border:1px solid var(--bdr);color:var(--hi);border-radius:2px}
.nav a:hover{border-color:var(--cyn)}
.btn,select,input{background:#21262d;color:var(--hi);border:1px solid var(--bdr);padding:4px 9px;font:inherit;cursor:pointer;border-radius:2px}
.btn:hover{border-color:var(--cyn)}
.btn-p{background:#1f6feb;border-color:#388bfd}
.box{background:var(--pan);border:1px solid var(--bdr);margin-bottom:12px}
.bh{background:#0f141c;border-bottom:1px solid var(--bdr);padding:7px 10px;display:flex;justify-content:space-between;align-items:center;font-weight:700;color:var(--hi)}
.bb{padding:10px}
.grid2{display:grid;grid-template-columns:repeat(auto-fit,minmax(340px,1fr));gap:12px;margin-bottom:12px}
table{width:100%;border-collapse:collapse;text-align:left}
th,td{padding:6px 8px;border-bottom:1px solid #21262d}
th{color:var(--dim);border-bottom-color:var(--bdr);background:#0f141c}
th a{color:var(--hi);display:block}
th a:hover{color:var(--cyn)}
.bdg{padding:1px 6px;font-size:11px;border-radius:2px;font-weight:700;display:inline-block}
.on{background:#0d2d14;color:var(--grn);border:1px solid #238636}
.off{background:#381111;color:var(--red);border:1px solid #da3633}
.vip{background:#342405;color:var(--amb);border:1px solid #9e6a03}
.blame{padding:12px;border:1px solid var(--bdr);margin-bottom:12px;display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:12px;background:var(--pan)}
pre,textarea{background:#000;border:1px solid var(--bdr);color:var(--hi);padding:8px;width:100%;font:inherit;overflow-y:auto}
.eli{cursor:help;border-bottom:1px dotted var(--dim)}
</style>
</head>
<body>
<div class="hdr">
  <div><b style="color:var(--cyn)">root@watchtower:~#</b> <span style="color:var(--grn)">sentinel --all-systems-online</span></div>
  <div style="display:flex;gap:6px">
    <button class="btn btn-p" onclick="post('/api/ping-now')" title="Test your internet and router right now">⚡ Quick Speed Test</button>
    <button class="btn" onclick="addVip()" title="Add an important family phone, laptop, or server to watch">+ Star Family Device</button>
    <button class="btn" onclick="post('/api/send-report')" title="Send today's summary to your Discord phone app">📊 Discord Daily Recap</button>
  </div>
</div>

<div class="sbar">
  <span class="eli" title="How long this little watchtower has been guarding your network without restarting">UPTIME: <b id="u">0s</b></span>
  <span class="eli" title="Total phones, laptops, TVs, and smart devices found on your home Wi-Fi">CONNECTED DEVICES: <b id="dv">0</b></span>
  <span class="eli" title="Speed between your devices and your Wi-Fi router. Under 10ms is excellent!">WI-FI: <b id="gw">--</b></span>
  <span class="eli" title="Speed between your house and the outside internet. Under 30ms is great!">INTERNET: <b id="wn">--</b></span>
  <span class="eli" title="Internet lag spikes or traffic jams (Jitter). Under 15ms is smooth for gaming & Zoom!">LAG SPIKES: <b id="jt">0ms</b></span>
  <span class="eli" title="How fast websites open when you click them (DNS lookup speed)">WEB LOOKUP: <b id="dn">--</b></span>
  <span class="eli" title="Offline smart plugs and cameras that got the official time today">CLOCK SYNCS: <b id="nt">0</b></span>
  <span class="eli" title="Times this sentinel woke up a sleeping computer (Wake-on-LAN)">WAKE SIGNALS: <b id="wl">0</b></span>
  <span class="eli" style="color:var(--cyn)" title="Click to inspect internal chip voltage, available RAM, and Wi-Fi signal" onclick="$('diag').style.display=$('diag').style.display==='none'?'flex':'none'">⚙️ Chip Health</span>
</div>

<div id="diag" style="display:none;background:#0f141c;border:1px solid var(--bdr);padding:5px 10px;margin-bottom:10px;gap:14px;font-size:11.5px">
  <span class="eli" title="Free working memory on the ESP8266 chip (Technical: Heap RAM)">FREE RAM: <b id="h">--</b></span>
  <span class="eli" title="Electric voltage powering the board (Technical: Internal VCC)">POWER VOLTAGE: <b id="vcc">--</b></span>
  <span class="eli" title="Wi-Fi antenna reception strength in decibels (Technical: RSSI)">WI-FI STRENGTH: <b id="rs">--</b></span>
  <span class="eli" title="Total smart home alerts sent (Technical: MQTT packets to TCP 1883)">SMART HOME UPDATES: <b id="mqSent">0</b></span>
</div>

<div class="nav">
  <b>JUMP TO:</b>
  <a href="#sec-blame">1. Who's Ruining Internet?</a>
  <a href="#sec-dns">2. Fake Website Detector</a>
  <a href="#sec-traps">3. Canary Burglar Traps</a>
  <a href="#sec-outages">4. Verified Downtimes</a>
  <a href="#sec-devices">5. Connected Devices</a>
  <a href="#sec-ntp">6. Master Clock</a>
  <a href="#sec-mqtt">7. Smart Home Link</a>
  <a href="#sec-settings">8. Settings & Phone Alerts</a>
  <a href="#sec-logs">9. Full Event Logs</a>
</div>

<!-- SECTION 1: BLAME METER -->
<div class="blame" id="sec-blame">
  <div>
    <div style="color:var(--dim);font-size:11px"><span class="eli" title="Technical: Compares local router round-trip latency against Cloudflare 1.1.1.1 and RFC 3550 statistical jitter to tell you who to blame for slow internet">1. WHO'S RUINING MY INTERNET? (BLAME METER)</span></div>
    <b id="bmTxt" style="font-size:13.5px;color:var(--grn)">CHECKING...</b>
  </div>
  <div>
    <div style="color:var(--dim);font-size:11px"><span class="eli" title="Technical: Local Gateway ICMP ping. High numbers mean your phone is too far from the router or Wi-Fi is congested">YOUR HOME WI-FI (ROUTER)</span></div>
    <b id="bmGw">--</b>
  </div>
  <div>
    <div style="color:var(--dim);font-size:11px"><span class="eli" title="Technical: WAN ICMP latency to Cloudflare 1.1.1.1. High numbers mean Comcast/Spectrum/AT&T is having issues outside">OUTSIDE INTERNET (PROVIDER)</span></div>
    <b id="bmWn">--</b>
  </div>
  <div>
    <div style="color:var(--dim);font-size:11px"><span class="eli" title="Technical: RFC 3550 packet jitter & bufferbloat. Caused when someone in the house is downloading huge files and hogging bandwidth">TRAFFIC JAMS & LAG SPIKES</span></div>
    <b id="bmJt">0 ms</b> (Failed: <b id="bmFl">0</b>)
  </div>
</div>

<!-- SECTION 2 & 3: DNS GUARD & CANARY TRAPS (TWO-COLUMN GRID) -->
<div class="grid2">
  <div class="box" id="sec-dns">
    <div class="bh">
      <span class="eli" title="Technical: RFC 1035 UDP 53 DNS benchmark with RFC 5452 16-bit random entropy. Checks if your router or ISP secretly intercepts nonexistent web domains to inject ads or tracking pages">2. FAKE WEBSITE & AD-REDIRECT DETECTOR</span>
      <button class="btn btn-p" onclick="post('/api/dns-bench')" title="Test if your provider redirects misspelled websites or injects ads">🔍 Test Web Lookups</button>
    </div>
    <div class="bb">
      <table>
        <thead>
          <tr>
            <th class="eli" title="The address book server translating names like google.com into numbers">WEB ADDRESS BOOK</th>
            <th class="eli" title="Time taken to look up a website address">SPEED</th>
            <th class="eli" title="Checks if nonexistent domains return fake ad pages instead of real NXDOMAIN">SECRET AD-INJECTION CHECK</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td><b class="eli" title="Technical: Your home router's local DNS resolver on Port 53">Home Router (Gateway)</b></td>
            <td><b id="dGw">--</b></td>
            <td><span id="dHj" class="bdg on">✅ CLEAN (No Secret Ads)</span></td>
          </tr>
          <tr>
            <td><b class="eli" title="Technical: Cloudflare public secure DNS on 1.1.1.1:53">Cloudflare Secure (1.1.1.1)</b></td>
            <td><b id="dCf">--</b></td>
            <td><span class="bdg on">✅ VERIFIED CLEAN</span></td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>

  <div class="box" id="sec-traps">
    <div class="bh">
      <span class="eli" title="Technical: Honeypot decoys on Ports 23, 8080, and 554. Emulates vulnerable computers, routers, and cameras to trip an instant alarm if an intruder or virus scans your network">3. FAKE BURGLAR TRAPS (CANARY DECOYS)</span>
      <button class="btn" onclick="post('/api/test-alert')" title="Simulate what happens when an intruder touches a trap">🚨 Test Alarm</button>
    </div>
    <div class="bb">
      <table>
        <thead>
          <tr>
            <th class="eli" title="The fake door opened on your network for hackers to bump into">FAKE TRAP DOOR</th>
            <th class="eli" title="What the trap pretends to be to fool attackers">PRETENDS TO BE</th>
            <th class="eli" title="Whether the trap is armed and waiting for intruders">GUARD STATE</th>
            <th class="eli" title="Number of times someone tried sneaking into this trap">TRIPPED</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td><b class="eli" title="Technical: Port 23 Telnet emulation">Door 1: Linux Terminal</b></td>
            <td>Ubuntu 22.04 LTS Login</td>
            <td><span class="bdg on">ARMED</span></td>
            <td><b id="hT">0</b></td>
          </tr>
          <tr>
            <td><b class="eli" title="Technical: Port 8080 Web HTTP trap">Door 2: Router Admin</b></td>
            <td>Router Control Panel</td>
            <td><span class="bdg on">ARMED</span></td>
            <td><b id="hW">0</b></td>
          </tr>
          <tr>
            <td><b class="eli" title="Technical: Port 554 RTSP camera stream">Door 3: Security Camera</b></td>
            <td>IP Camera Video Stream</td>
            <td><span class="bdg on">ARMED</span></td>
            <td><b id="hC">0</b></td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</div>

<!-- SECTION 4: VERIFIED INTERNET DOWNTIMES -->
<div class="box" id="sec-outages">
  <div class="bh">
    <span class="eli" title="Technical: Autonomous LittleFS WAN downtime auditor. Only logs an outage when the local Wi-Fi router is 100% healthy but outside internet is dead">4. VERIFIED INTERNET DOWNTIMES (CONFIRMED ISP OUTAGES ONLY)</span>
    <span id="wSt" class="bdg on">ONLINE</span>
  </div>
  <div class="bb">
    <div class="tbar">
      <span>Confirmed Outages: <b id="oCnt">0</b> | Latest Downtime: <b id="oDur">0s</b></span>
      <span>
        <span class="eli" title="Ignore brief hiccups shorter than this time">Count as Outage after:</span>
        <select id="oTh" onchange="post('/api/config',{outage_threshold:+this.value})">
          <option value="60">1 min</option><option value="180">3 mins</option>
          <option value="300">5 mins</option><option value="600">10 mins</option>
        </select>
      </span>
    </div>
    <div style="overflow-x:auto">
      <table>
        <thead>
          <tr>
            <th class="eli" title="Sequential event number">EVENT</th>
            <th class="eli" title="When the outage started">STARTED</th>
            <th class="eli" title="Total duration of lost internet connection">DURATION</th>
            <th class="eli" title="Current status of the internet connection">STATUS</th>
            <th class="eli" title="Proof that this was an upstream ISP failure, not local Wi-Fi">VERIFIED CAUSE</th>
          </tr>
        </thead>
        <tbody>
)rawliteral";

const char HTML_DEV_HEADER[] PROGMEM = R"rawliteral(
        </tbody>
      </table>
    </div>
  </div>
</div>

<!-- SECTION 5: CONNECTED DEVICES TABLE (SORTABLE HEADERS, NO JS!) -->
<div class="box" id="sec-devices">
  <div class="bh">
    <span class="eli" title="Technical: L2 ARP + NetBIOS active radar. Alerts your phone when unknown devices connect, tracks family arrivals/departures, and auto-wakes frozen servers via Wake-on-LAN">5. CONNECTED DEVICES & FAMILY WATCHLIST</span>
    <span class="eli" title="Technical: 802.3 Wake-on-LAN packets dispatched over UDP port 9 to revive sleeping computers">Auto-Wakes Sent: <b id="wl2" style="color:var(--amb)">0</b></span>
  </div>
  <div class="bb">
    <div class="tbar">
      <div>
        <a href="/?sort=ip&order=asc#sec-devices" class="btn btn-p">ALL DEVICES</a>
        <a href="/?sort=vip&order=desc&flt=vip#sec-devices" class="btn" title="Filter to starred family devices and critical computers">★ STARRED FAMILY ONLY</a>
      </div>
      <div style="font-size:11px;color:var(--dim)">
        <span>Click any column header below to sort</span>
      </div>
    </div>
    <div style="overflow-x:auto">
      <table>
        <thead>
          <tr>
)rawliteral";

const char HTML_PAGE_FOOT[] PROGMEM = R"rawliteral(
          </tr>
        </thead>
        <tbody id="tbD">
)rawliteral";

const char HTML_PAGE_END[] PROGMEM = R"rawliteral(
        </tbody>
      </table>
    </div>
  </div>
</div>

<!-- SECTION 6 & 7: SMART MASTER CLOCK & SMART HOME -->
<div class="grid2">
  <div class="box" id="sec-ntp">
    <div class="bh">
      <span class="eli" title="Technical: RFC 5905 Stratum-2 Micro-NTP Server on UDP Port 123. Keeps smart plugs, Tuya bulbs, and security cameras synced to exact time even when internet is unplugged">6. MASTER CLOCK FOR SMART DEVICES</span>
      <span class="bdg on">STRATUM-2 ACTIVE</span>
    </div>
    <div class="bb">
      Synchronizes offline smart cameras & plugs on your local network (<span class="eli" title="Standard Network Time Protocol port">UDP Port 123</span>) so smart home schedules keep working even if the internet goes down.  
      Total Smart Devices Synced: <b id="nt2" style="color:var(--grn)">0</b>
    </div>
  </div>

  <div class="box" id="sec-mqtt">
    <div class="bh">
      <span class="eli" title="Technical: RFC 3986 / TCP 1883 MQTT telemetry publisher. Streams intruder alarms, WAN uptime, and device count straight to Home Assistant without third-party cloud">7. SMART HOME CONNECTOR (HOME ASSISTANT)</span>
      <span>Published Updates: <b id="mqCnt">0</b></span>
    </div>
    <div class="bb" style="display:flex;gap:8px;flex-wrap:wrap">
      <input id="mqH" placeholder="Home Assistant IP (e.g. 192.168.50.10 or homeassistant.local)" style="flex:1" title="IP address of your local Home Assistant server">
      <button class="btn btn-p" onclick="post('/api/mqtt',{host:$('mqH').value})" title="Save and start broadcasting sensor telemetry">💾 Connect Smart Home</button>
    </div>
  </div>
</div>

<!-- SECTION 8: WI-FI & DISCORD PHONE ALERTS -->
<div class="box" id="sec-settings">
  <div class="bh">
    <span class="eli" title="Configure your home Wi-Fi credentials, alert webhooks, and notification rules">8. NETWORK SETTINGS & NOTIFICATION PREFERENCES</span>
  </div>
  <div class="bb">
    <!-- WI-FI SETUP CARD -->
    <div style="background:#0f141c;border:1px solid var(--bdr);padding:10px;margin-bottom:12px">
      <div style="color:var(--hi);font-weight:700;margin-bottom:6px">📶 Home Wi-Fi Network Setup</div>
      <form method="POST" action="/api/wifi" onsubmit="event.preventDefault();saveWifi();" style="display:flex;gap:8px;flex-wrap:wrap">
        <input id="wfSsid" name="ssid" placeholder="Wi-Fi Name (SSID, e.g. Sasta_Internet)" style="flex:1" title="Your Wi-Fi SSID" required>
        <input id="wfPass" name="pass" type="password" placeholder="Wi-Fi Password" style="flex:1" title="Your Wi-Fi Password">
        <button type="submit" class="btn btn-p" title="Save and connect to Wi-Fi">💾 Connect to Wi-Fi</button>
      </form>
      <div id="wfMsg" style="font-size:11.5px;color:var(--dim);margin-top:6px">Current Network: <b id="wfCur">Connecting...</b></div>
    </div>

    <!-- DISCORD ALERTS -->
    <div style="color:var(--hi);font-weight:700;margin-bottom:6px">🔔 Discord Phone Alerts Webhook</div>
    <div style="display:flex;gap:8px;flex-wrap:wrap;margin-bottom:12px">
      <input id="wh" placeholder="https://discord.com/api/webhooks/..." style="flex:1" title="Paste Discord webhook URL here">
      <button class="btn btn-p" onclick="post('/api/webhook',{url:$('wh').value})" title="Save webhook">💾 Save Alert Link</button>
    </div>
    <div style="color:var(--dim);font-size:11px;margin-bottom:6px">NOTIFICATION FILTERS:</div>
    <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:8px;background:#0f141c;border:1px solid var(--bdr);padding:10px">
      <label><input type="checkbox" id="chkStr" onchange="saveNotifyConfig()"> 🔔 Stranger Alert (New Devices)</label>
      <label><input type="checkbox" id="chkPres" onchange="saveNotifyConfig()"> 🏠 Family Arrival & Departure</label>
      <label><input type="checkbox" id="chkOut" onchange="saveNotifyConfig()"> ⚠️ Confirmed ISP Outages</label>
      <label><input type="checkbox" id="chkTrp" onchange="saveNotifyConfig()"> 🚨 Burglar Trap Decoys</label>
      <label><input type="checkbox" id="chkFrz" onchange="saveNotifyConfig()"> 🐺 Critical Server Freeze / Auto-WoL</label>
    </div>
  </div>
</div>

<!-- SECTION 9: AUDIT EVENT LOGS -->
<div class="box" id="sec-logs">
  <div class="bh">
    <span class="eli" title="Technical: Deduplicated LittleFS persistent flash storage and circular RAM ring buffer. Records all network events safely even if power cuts out">9. FULL EVENT HISTORY (WHO CAME, LEFT & OUTAGES)</span>
    <div>
      <a href="/api/download-logs" download class="btn" title="Download spreadsheet of all recorded events">⬇ Download Spreadsheet (.CSV)</a> 
      <button class="btn" onclick="post('/api/clear-logs')" title="Wipe event history">🗑️ Clear History</button>
    </div>
  </div>
  <div class="bb"><pre id="lg" style="height:190px">Loading event records...</pre></div>
</div>

<script>
const $=id=>document.getElementById(id);
const post=(u,b)=>fetch(u,{method:'POST',headers:{'Content-Type':'application/json','X-Requested-With':'Watchtower'},body:b?JSON.stringify(b):null}).then(poll);
function addVip(){const ip=prompt('Enter Device IP Address (e.g. 192.168.50.15):');if(ip){const name=prompt('Friendly Nickname (e.g. Dad iPhone, Mom Laptop, Living Room TV):')||ip;post('/api/vip/toggle',{ip,vip:true,name});}}
function saveNotifyConfig(){
  post('/api/config',{
    notify_stranger:$('chkStr').checked,
    notify_presence:$('chkPres').checked,
    notify_outage:$('chkOut').checked,
    notify_traps:$('chkTrp').checked,
    notify_freeze:$('chkFrz').checked
  });
}
function saveWifi(){
  const ssid=$('wfSsid').value.trim(), pass=$('wfPass').value;
  if(!ssid){ alert('Please enter Wi-Fi Name (SSID)'); return; }
  $('wfMsg').innerText='Saving credentials & connecting to '+ssid+'...';
  post('/api/wifi',{ssid,pass}).then(()=>{
    $('wfMsg').innerText='Saved! Connecting to '+ssid+'... (check status bar above)';
  });
}
async function poll(){
  try{
    const s=await(await fetch('/api/status')).json();
    $('u').innerText=Math.floor(s.uptime/60)+'m';
    $('h').innerText=Math.round(s.heap/1024)+'KB';
    $('vcc').innerText=(s.vcc||0)+'mV';
    $('rs').innerText=(s.rssi||0)+'dBm';
    $('dv').innerText=`${s.lan_online}/${s.lan_total} (${s.vip_online} Starred)`;
    $('gw').innerText=s.ping_gw>=0?s.ping_gw+'ms':'DOWN';
    $('wn').innerText=s.ping_wan>=0?s.ping_wan+'ms':'OUTAGE';
    $('jt').innerText=(s.jitter_ms||0)+'ms';
    $('bmGw').innerText=s.ping_gw>=0?`${s.ping_gw} ms (${s.ping_gw<=45?'🟢 Excellent':'🟡 Slow'})`:'🔴 Wi-Fi Down';
    $('bmWn').innerText=s.ping_wan>=0?`${s.ping_wan} ms (${s.ping_wan<=100?'🟢 Fast':'🔴 Lagging'})`:'🔴 ISP Outage';
    $('bmJt').innerText=`${s.jitter_ms||0} ms (${(s.jitter_ms||0)<=15?'🟢 Smooth':((s.jitter_ms||0)<=30?'🟡 Minor Lag':'🔴 Traffic Jam')})`;
    $('bmTxt').innerText=s.blame_desc||'OK';
    $('bmBox').style.borderColor=s.blame_verdict===0?'var(--grn)':(s.blame_verdict===1?'var(--red)':(s.blame_verdict===4?'var(--red)':'var(--amb)'));
    $('bmFl').innerText=s.pings_failed;
    const dg=s.dns_gw_ms>=0?s.dns_gw_ms+'ms':'--',dc=s.dns_cf_ms>=0?s.dns_cf_ms+'ms':'--';
    $('dn').innerText=`${dg}/${dc}`;
    $('dGw').innerText=dg;$('dCf').innerText=dc;
    $('dHj').innerText=s.dns_hijacked?'🚨 SECRET ADS DETECTED!':'✅ CLEAN (No Secret Ads)';
    $('dHj').className='bdg '+(s.dns_hijacked?'off':'on');
    $('nt').innerText=$('nt2').innerText=s.ntp_served||0;
    $('wl').innerText=$('wl2').innerText=s.wol_sent||0;
    $('mqCnt').innerText=$('mqSent').innerText=s.mqtt_sent||0;
    if(s.mqtt_host&&document.activeElement!==$('mqH'))$('mqH').value=s.mqtt_host;
    $('hT').innerText=s.telnet_hits||0;$('hW').innerText=s.web_hits||0;$('hC').innerText=s.camera_hits||0;
    $('oCnt').innerText=s.total_outages||0;$('oDur').innerText=(s.last_outage_duration||0)+'s';
    if(document.activeElement!==$('oTh'))$('oTh').value=s.outage_threshold;
    if(s.notify_stranger!==undefined)$('chkStr').checked=s.notify_stranger;
    if(s.notify_presence!==undefined)$('chkPres').checked=s.notify_presence;
    if(s.notify_outage!==undefined)$('chkOut').checked=s.notify_outage;
    if(s.notify_traps!==undefined)$('chkTrp').checked=s.notify_traps;
    if(s.notify_freeze!==undefined)$('chkFrz').checked=s.notify_freeze;
    if(s.ssid){
      if(document.activeElement!==$('wfSsid') && !$('wfSsid').value) $('wfSsid').value=s.ssid;
      $('wfCur').innerText=s.ssid + (s.ip ? ' (Online: '+s.ip+')' : ' (Status: Connecting / Checking Password...)');
    }
    const l=await(await fetch('/api/logs')).json();
    $('lg').innerText=l.map(x=>`[${x.t}s] [${x.type}] ${x.msg}`).join('\n');
  }catch(e){}
}
setInterval(poll,4000);poll();
</script>
</body>
</html>
)rawliteral";
