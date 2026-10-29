#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>root@watchtower: ~# sentinel</title>
<style>
:root {
  --bg-dark: #0d1117;
  --bg-panel: #161b22;
  --bg-subtle: #0f141c;
  --border: #30363d;
  --border-focus: #58a6ff;
  --text-main: #c9d1d9;
  --text-dim: #8b949e;
  --text-bold: #f0f6fc;
  --green: #3fb950;
  --green-bg: #0d2d14;
  --green-border: #238636;
  --red: #f85149;
  --red-bg: #381111;
  --red-border: #da3633;
  --amber: #d29922;
  --amber-bg: #342405;
  --amber-border: #9e6a03;
  --cyan: #58a6ff;
  --font-mono: ui-monospace, "SF Mono", "JetBrains Mono", Menlo, Consolas, monospace;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  background-color: var(--bg-dark);
  color: var(--text-main);
  font-family: var(--font-mono);
  font-size: 13px;
  line-height: 1.45;
  padding: 14px;
  -webkit-font-smoothing: antialiased;
}
a { color: var(--cyan); text-decoration: none; }
a:hover { text-decoration: underline; }

.cli-header {
  border-bottom: 1px solid var(--border);
  padding-bottom: 10px;
  margin-bottom: 12px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 8px;
}
.cli-prompt {
  color: var(--cyan);
  font-weight: 700;
  font-size: 14px;
}
.cli-prompt span { color: var(--text-bold); }
.cli-prompt em { color: var(--green); font-style: normal; }

.status-bar {
  background: var(--bg-panel);
  border: 1px solid var(--border);
  padding: 8px 12px;
  margin-bottom: 14px;
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
  font-size: 12px;
}
.stat-pill {
  display: flex;
  gap: 6px;
}
.stat-pill .lbl { color: var(--text-dim); }
.stat-pill .val { color: var(--text-bold); font-weight: 600; }

.grid {
  display: grid;
  grid-template-columns: 2fr 1fr;
  gap: 14px;
  margin-bottom: 14px;
}
@media (max-width: 900px) {
  .grid { grid-template-columns: 1fr; }
}

.box {
  background: var(--bg-panel);
  border: 1px solid var(--border);
  display: flex;
  flex-direction: column;
}
.box-header {
  background: var(--bg-subtle);
  border-bottom: 1px solid var(--border);
  padding: 8px 12px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-weight: 600;
  color: var(--text-bold);
  font-size: 12px;
}
.box-header .tag {
  color: var(--cyan);
  font-weight: normal;
}
.box-body {
  padding: 12px;
  flex: 1;
}

.btn {
  background: #21262d;
  color: var(--text-bold);
  border: 1px solid var(--border);
  padding: 4px 10px;
  font-family: var(--font-mono);
  font-size: 12px;
  cursor: pointer;
  border-radius: 2px;
  display: inline-flex;
  align-items: center;
  gap: 5px;
}
.btn:hover { background: #30363d; border-color: var(--text-dim); }
.btn-primary { background: #1f6feb; border-color: #388bfd; color: #fff; }
.btn-primary:hover { background: #388bfd; }
.btn-danger { background: var(--red-bg); border-color: var(--red-border); color: var(--red); }
.btn-amber { background: var(--amber-bg); border-color: var(--amber-border); color: var(--amber); }

.toolbar {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 12px;
  align-items: center;
}

.tag-badge {
  display: inline-block;
  padding: 2px 6px;
  font-size: 11px;
  font-weight: 600;
  border-radius: 2px;
}
.badge-online { background: var(--green-bg); color: var(--green); border: 1px solid var(--green-border); }
.badge-offline { background: var(--red-bg); color: var(--red); border: 1px solid var(--red-border); }
.badge-vip { background: var(--amber-bg); color: var(--amber); border: 1px solid var(--amber-border); }

table.linux-table {
  width: 100%;
  border-collapse: collapse;
  text-align: left;
  font-size: 12px;
}
table.linux-table th {
  border-bottom: 1px solid var(--border);
  color: var(--text-dim);
  padding: 6px 8px;
  font-weight: 600;
}
table.linux-table td {
  border-bottom: 1px solid #21262d;
  padding: 6px 8px;
}
table.linux-table tr:hover {
  background: rgba(255, 255, 255, 0.02);
}

.star-btn {
  background: none;
  border: none;
  font-size: 14px;
  cursor: pointer;
  padding: 0 4px;
  color: var(--text-dim);
}
.star-btn.active { color: var(--amber); }

.log-stream {
  background: #000;
  border: 1px solid var(--border);
  padding: 8px;
  height: 220px;
  overflow-y: auto;
  font-size: 11.5px;
  line-height: 1.4;
}
.log-line { margin-bottom: 3px; word-break: break-all; }
.log-t { color: var(--text-dim); }
.log-type { font-weight: 700; margin-right: 6px; }
.log-type.danger { color: var(--red); }
.log-type.warn { color: var(--amber); }
.log-type.info { color: var(--cyan); }
.log-type.success { color: var(--green); }

.trap-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px;
  border-bottom: 1px solid var(--border);
  font-size: 12px;
}
.trap-row:last-child { border-bottom: none; }

.sparkline-box {
  background: #000;
  border: 1px solid var(--border);
  padding: 8px;
  height: 60px;
  display: flex;
  align-items: flex-end;
  gap: 3px;
  margin-top: 8px;
}
.spark-bar {
  flex: 1;
  background: var(--cyan);
  min-height: 2px;
}

.modal {
  display: none;
  position: fixed;
  top: 0; left: 0; width: 100%; height: 100%;
  background: rgba(0,0,0,0.7);
  align-items: center;
  justify-content: center;
  z-index: 1000;
}
.modal.open { display: flex; }
.modal-box {
  background: var(--bg-panel);
  border: 1px solid var(--border);
  width: 90%;
  max-width: 440px;
  padding: 16px;
}
.modal-header {
  font-weight: 700;
  color: var(--text-bold);
  margin-bottom: 12px;
  border-bottom: 1px solid var(--border);
  padding-bottom: 8px;
}
.input-text {
  width: 100%;
  background: #000;
  border: 1px solid var(--border);
  color: var(--text-bold);
  padding: 6px 8px;
  font-family: var(--font-mono);
  font-size: 12px;
  margin-bottom: 10px;
}
.input-text:focus { outline: none; border-color: var(--cyan); }
</style>
</head>
<body>

<div class="cli-header">
  <div class="cli-prompt">root@watchtower:<span>~#</span> <em>sentinel --radar=full+vip</em></div>
  <div>
    <button class="btn btn-primary" onclick="triggerPingNow()">⚡ Ping Check</button>
    <button class="btn" onclick="openAddVipModal()">+ Add VIP Target</button>
    <button class="btn" onclick="sendDailyReport()">📊 Dispatch Report</button>
  </div>
</div>

<div class="status-bar" id="statusBar">
  <div class="stat-pill"><span class="lbl">UPTIME:</span><span class="val" id="sbUptime">00:00:00</span></div>
  <div class="stat-pill"><span class="lbl">HEAP:</span><span class="val" id="sbHeap">-- KB</span></div>
  <div class="stat-pill"><span class="lbl">SUBNET:</span><span class="val" id="sbSubnet">192.168.50.0/24</span></div>
  <div class="stat-pill"><span class="lbl">LAN DEVICES:</span><span class="val" id="sbDevices">0 ONLINE (0 VIP)</span></div>
  <div class="stat-pill"><span class="lbl">WAN (1.1.1.1):</span><span class="val" id="sbWan">-- ms</span></div>
  <div class="stat-pill"><span class="lbl">ROUTER:</span><span class="val" id="sbGw">-- ms</span></div>
  <div class="stat-pill"><span class="lbl">WIFI:</span><span class="val" id="sbRssi">-- dBm</span></div>
</div>

<div class="grid">
  <!-- Window 1: LAN Radar & Device Table -->
  <div class="box">
    <div class="box-header">
      <span>[01: LAN RADAR // SUBNET SWEEPER & VIP SENTINEL]</span>
      <span class="tag" id="radarStatusTag">AUTO-SCANNER: ACTIVE</span>
    </div>
    <div class="box-body">
      <div class="toolbar">
        <button class="btn btn-primary" id="btnFilterAll" onclick="setDeviceFilter('all')">ALL DEVICES (<span id="countAll">0</span>)</button>
        <button class="btn btn-amber" id="btnFilterVip" onclick="setDeviceFilter('vip')">★ VIP WATCHLIST (<span id="countVip">0</span>)</button>
        <button class="btn" id="btnToggleScan" onclick="toggleScanner()">SWEEPER: ACTIVE</button>
        <button class="btn" onclick="refreshDevices()">🔄 REFRESH</button>
      </div>

      <div style="overflow-x: auto;">
        <table class="linux-table">
          <thead>
            <tr>
              <th style="width: 30px;">VIP</th>
              <th>IP ADDRESS</th>
              <th>MAC / VENDOR</th>
              <th>DEVICE NAME</th>
              <th>LATENCY</th>
              <th>LAST SEEN</th>
              <th>STATUS</th>
            </tr>
          </thead>
          <tbody id="deviceTableBody">
            <tr><td colspan="7" style="text-align: center; color: var(--text-dim); padding: 16px;">Scanning subnet for active clients...</td></tr>
          </tbody>
        </table>
      </div>
    </div>
  </div>

  <!-- Column 2: Canary Traps & Latency -->
  <div style="display: flex; flex-direction: column; gap: 14px;">
    <!-- Burglar Canary Traps -->
    <div class="box">
      <div class="box-header">
        <span>[02: CANARY TRAPS // HONEYPOTS]</span>
        <span class="tag">3 ARMED</span>
      </div>
      <div class="box-body" style="padding: 0;">
        <div class="trap-row">
          <div>
            <div style="font-weight: 600; color: var(--text-bold);">Port 23: Telnet Terminal Decoy</div>
            <div style="font-size: 11px; color: var(--text-dim);">Traps brute-force shell logins</div>
          </div>
          <div><span class="tag-badge badge-online">ARMED</span> <span id="hitsTelnet" style="color: var(--amber); margin-left: 4px;">0 hits</span></div>
        </div>
        <div class="trap-row">
          <div>
            <div style="font-weight: 600; color: var(--text-bold);">Port 8080: Gateway Admin Trap</div>
            <div style="font-size: 11px; color: var(--text-dim);">Traps web credential stuffing</div>
          </div>
          <div><span class="tag-badge badge-online">ARMED</span> <span id="hitsWeb" style="color: var(--amber); margin-left: 4px;">0 hits</span></div>
        </div>
        <div class="trap-row">
          <div>
            <div style="font-weight: 600; color: var(--text-bold);">Port 554: RTSP Security Cam Decoy</div>
            <div style="font-size: 11px; color: var(--text-dim);">Traps video stream recon scanners</div>
          </div>
          <div><span class="tag-badge badge-online">ARMED</span> <span id="hitsCam" style="color: var(--amber); margin-left: 4px;">0 hits</span></div>
        </div>
      </div>
    </div>

    <!-- Latency Watchdog -->
    <div class="box">
      <div class="box-header">
        <span>[03: WAN HEALTH // 1.1.1.1 LATENCY]</span>
        <span class="tag" id="wanStatusTag">ONLINE</span>
      </div>
      <div class="box-body">
        <div style="display: flex; justify-content: space-between; font-size: 12px; margin-bottom: 6px;">
          <span>Current Lag: <strong id="wanCurrentLag">-- ms</strong></span>
          <span>Failures: <strong id="wanFailures">0</strong></span>
        </div>
        <div class="sparkline-box" id="sparkline"></div>
        <div style="margin-top: 10px; padding-top: 8px; border-top: 1px solid var(--border); font-size: 11.5px; display: flex; justify-content: space-between; align-items: center;">
          <span style="color: var(--text-dim);">ALERT IF DOWN >:</span>
          <select id="selOutageThreshold" onchange="updateOutageThreshold(this.value)" style="background: #000; border: 1px solid var(--border); color: var(--text-bold); font-family: var(--font-mono); font-size: 11px; padding: 2px 6px;">
            <option value="60">1 min (Strict)</option>
            <option value="180">3 mins</option>
            <option value="300">5 mins (Default)</option>
            <option value="600">10 mins</option>
            <option value="900">15 mins (Quiet)</option>
          </select>
        </div>
      </div>
    </div>
  </div>
</div>

<!-- Window 4: System Audit Log -->
<div class="box">
  <div class="box-header">
    <span>[04: AUDIT STREAM // EVENT LOGS]</span>
    <div>
      <a href="/api/download-logs" download class="btn" style="padding: 2px 8px; font-size: 11px;">⬇ CSV Backup</a>
    </div>
  </div>
  <div class="box-body" style="padding: 0;">
    <div class="log-stream" id="logStream">
      <div class="log-line"><span class="log-t">[00:00:00]</span> <span class="log-type info">[SYSTEM]</span> Sentinel engine initialized.</div>
    </div>
  </div>
</div>

<!-- Modal: Add VIP Target -->
<div class="modal" id="vipModal">
  <div class="modal-box">
    <div class="modal-header">[+ ADD VIP TARGET TO WATCHLIST]</div>
    <div style="font-size: 12px; color: var(--text-dim); margin-bottom: 10px;">
      VIP devices are priority-scanned every 15s with instant Discord alerts on disconnect.
    </div>
    <label style="font-size: 11px; color: var(--text-dim);">IP ADDRESS</label>
    <input type="text" id="vipInputIp" class="input-text" placeholder="192.168.50.119">
    <label style="font-size: 11px; color: var(--text-dim);">CUSTOM NAME / LABEL</label>
    <input type="text" id="vipInputName" class="input-text" placeholder="e.g. MacBook Pro M3">
    <div style="display: flex; justify-content: flex-end; gap: 8px;">
      <button class="btn" onclick="closeAddVipModal()">Cancel</button>
      <button class="btn btn-primary" onclick="submitAddVip()">Save Target</button>
    </div>
  </div>
</div>

<script>
let currentFilter = 'all';
let scannerActive = true;

function formatUptime(s) {
  const h = String(Math.floor(s / 3600)).padStart(2, '0');
  const m = String(Math.floor((s % 3600) / 60)).padStart(2, '0');
  const sec = String(s % 60).padStart(2, '0');
  return `${h}:${m}:${sec}`;
}

async function fetchStatus() {
  try {
    const res = await fetch('/api/status');
    const d = await res.json();
    document.getElementById('sbUptime').innerText = formatUptime(d.uptime || 0);
    document.getElementById('sbHeap').innerText = Math.round((d.heap || 0) / 1024) + ' KB';
    document.getElementById('sbWan').innerText = (d.ping_wan >= 0 ? d.ping_wan + ' ms' : 'OUTAGE');
    document.getElementById('sbGw').innerText = (d.ping_gw >= 0 ? d.ping_gw + ' ms' : 'DOWN');
    document.getElementById('sbRssi').innerText = (d.rssi || 0) + ' dBm';
    document.getElementById('wanCurrentLag').innerText = (d.ping_wan >= 0 ? d.ping_wan + ' ms' : 'DOWN');
    document.getElementById('wanFailures').innerText = d.pings_failed || 0;
    document.getElementById('hitsTelnet').innerText = (d.honeypot_telnet_hits || 0) + ' hits';
    document.getElementById('hitsWeb').innerText = (d.honeypot_web_hits || 0) + ' hits';
    document.getElementById('hitsCam').innerText = (d.honeypot_camera_hits || 0) + ' hits';
    if (d.outage_threshold) {
      const sel = document.getElementById('selOutageThreshold');
      if (sel && document.activeElement !== sel) sel.value = d.outage_threshold;
    }

    const wanTag = document.getElementById('wanStatusTag');
    if (d.wan_down) {
      wanTag.innerText = 'DISCONNECTED';
      wanTag.style.color = 'var(--red)';
    } else {
      wanTag.innerText = 'ONLINE';
      wanTag.style.color = 'var(--green)';
    }

    if (d.ping_history) {
      const box = document.getElementById('sparkline');
      box.innerHTML = '';
      const maxVal = Math.max(...d.ping_history.map(v => (v > 0 ? v : 100)), 40);
      d.ping_history.forEach(val => {
        const bar = document.createElement('div');
        bar.className = 'spark-bar';
        if (val < 0) {
          bar.style.height = '100%';
          bar.style.background = 'var(--red)';
        } else {
          const pct = Math.max(4, Math.min(100, (val / maxVal) * 100));
          bar.style.height = pct + '%';
          bar.style.background = (val > 80 ? 'var(--amber)' : 'var(--cyan)');
        }
        box.appendChild(bar);
      });
    }
  } catch(e) {}
}

async function refreshDevices() {
  try {
    const res = await fetch('/api/devices');
    const devices = await res.json();
    const tbody = document.getElementById('deviceTableBody');
    tbody.innerHTML = '';

    let onlineCount = 0;
    let vipCount = 0;
    let vipOnline = 0;

    devices.forEach(dev => {
      if (dev.online) onlineCount++;
      if (dev.vip) {
        vipCount++;
        if (dev.online) vipOnline++;
      }
    });

    document.getElementById('countAll').innerText = devices.length;
    document.getElementById('countVip').innerText = vipCount;
    document.getElementById('sbDevices').innerText = `${onlineCount} ONLINE (${vipOnline} VIP)`;

    const filtered = (currentFilter === 'vip') ? devices.filter(d => d.vip) : devices;

    if (filtered.length === 0) {
      tbody.innerHTML = `<tr><td colspan="7" style="text-align: center; color: var(--text-dim); padding: 16px;">No ${currentFilter === 'vip' ? 'VIP targets' : 'devices'} recorded yet.</td></tr>`;
      return;
    }

    filtered.forEach(dev => {
      const tr = document.createElement('tr');
      const starClass = dev.vip ? 'star-btn active' : 'star-btn';
      const starIcon = dev.vip ? '★' : '☆';
      const statusBadge = dev.online 
        ? '<span class="tag-badge badge-online">ONLINE</span>' 
        : '<span class="tag-badge badge-offline">OFFLINE</span>';
      const vipBadge = dev.vip ? ' <span class="tag-badge badge-vip">VIP</span>' : '';
      const latencyStr = dev.online ? (dev.latency >= 0 ? dev.latency + ' ms' : '< 1 ms') : '--';

      tr.innerHTML = `
        <td><button class="${starClass}" onclick="toggleVip('${dev.ip}', ${!dev.vip})">${starIcon}</button></td>
        <td style="font-weight: 600; color: var(--text-bold);">${dev.ip}</td>
        <td><span style="color: var(--text-dim);">${dev.mac || '00:00:00:00:00:00'}</span> <em style="color: var(--cyan); font-style: normal;">[${dev.vendor || 'Client'}]</em></td>
        <td><span style="cursor:pointer;border-bottom:1px dotted var(--text-dim);" title="Click to rename" onclick="renameDevice('${dev.ip}', '${(dev.name||'').replace(/'/g, "\\'")}')">${dev.name || 'Unnamed'}</span> <span style="font-size:10px;cursor:pointer;opacity:0.6;" onclick="renameDevice('${dev.ip}', '${(dev.name||'').replace(/'/g, "\\'")}')">✏️</span>${vipBadge}</td>
        <td>${latencyStr}</td>
        <td style="color: var(--text-dim);">${dev.last_seen_sec ? dev.last_seen_sec + 's ago' : 'just now'}</td>
        <td>${statusBadge}</td>
      `;
      tbody.appendChild(tr);
    });
  } catch(e) {}
}

async function fetchLogs() {
  try {
    const res = await fetch('/api/logs');
    const logs = await res.json();
    const box = document.getElementById('logStream');
    box.innerHTML = '';
    logs.forEach(item => {
      const line = document.createElement('div');
      line.className = 'log-line';
      const t = formatUptime(item.t || 0);
      const sevClass = (item.sev === 'danger' || item.sev === 'warn' || item.sev === 'info' || item.sev === 'success') ? item.sev : 'info';
      line.innerHTML = `<span class="log-t">[${t}]</span> <span class="log-type ${sevClass}">[${item.type}]</span> ${item.msg}`;
      box.appendChild(line);
    });
  } catch(e) {}
}

function setDeviceFilter(filter) {
  currentFilter = filter;
  if (filter === 'vip') {
    document.getElementById('btnFilterVip').className = 'btn btn-primary';
    document.getElementById('btnFilterAll').className = 'btn';
  } else {
    document.getElementById('btnFilterAll').className = 'btn btn-primary';
    document.getElementById('btnFilterVip').className = 'btn btn-amber';
  }
  refreshDevices();
}

async function toggleVip(ip, newVipState) {
  await fetch('/api/vip/toggle', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ ip: ip, vip: newVipState })
  });
  refreshDevices();
}

async function renameDevice(ip, curName) {
  const n = prompt(`Rename device (${ip}):`, curName);
  if (n !== null && n.trim() !== '' && n.trim() !== curName) {
    await fetch('/api/device/rename', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ip: ip, name: n.trim() })
    });
    refreshDevices();
  }
}

async function toggleScanner() {
  scannerActive = !scannerActive;
  const btn = document.getElementById('btnToggleScan');
  btn.innerText = scannerActive ? 'SWEEPER: ACTIVE' : 'SWEEPER: PAUSED';
  document.getElementById('radarStatusTag').innerText = scannerActive ? 'AUTO-SCANNER: ACTIVE' : 'AUTO-SCANNER: PAUSED';
  await fetch('/api/scan/mode', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ active: scannerActive })
  });
}

function openAddVipModal() {
  document.getElementById('vipModal').className = 'modal open';
}
function closeAddVipModal() {
  document.getElementById('vipModal').className = 'modal';
}
async function submitAddVip() {
  const ip = document.getElementById('vipInputIp').value.trim();
  const name = document.getElementById('vipInputName').value.trim();
  if (!ip) return;
  await fetch('/api/vip/add', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ ip: ip, name: name || ('Target ' + ip) })
  });
  closeAddVipModal();
  document.getElementById('vipInputIp').value = '';
  document.getElementById('vipInputName').value = '';
  refreshDevices();
}

async function triggerPingNow() {
  await fetch('/api/ping-now', { method: 'POST' });
  fetchStatus();
  fetchLogs();
}

async function sendDailyReport() {
  await fetch('/api/send-report', { method: 'POST' });
  fetchLogs();
}

async function updateOutageThreshold(val) {
  await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ outage_threshold: parseInt(val) })
  });
  fetchLogs();
}

setInterval(fetchStatus, 3000);
setInterval(refreshDevices, 4000);
setInterval(fetchLogs, 5000);

fetchStatus();
refreshDevices();
fetchLogs();
</script>
</body>
</html>
)rawliteral";
