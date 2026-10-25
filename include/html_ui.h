#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Home Sentinel &bull; Always-On Guardian</title>
<style>
:root {
  --bg-primary: #07090e;
  --bg-surface: #0e131f;
  --bg-surface-elevated: #141c2e;
  --border-subtle: rgba(255, 255, 255, 0.08);
  --border-active: rgba(56, 189, 248, 0.3);
  --text-primary: #f8fafc;
  --text-secondary: #94a3b8;
  --text-tertiary: #64748b;
  --cyan: #38bdf8;
  --cyan-glow: rgba(56, 189, 248, 0.15);
  --emerald: #10b981;
  --emerald-glow: rgba(16, 185, 129, 0.15);
  --rose: #f43f5e;
  --rose-glow: rgba(244, 63, 94, 0.2);
  --amber: #f59e0b;
  --amber-glow: rgba(245, 158, 11, 0.15);
  --font-sans: -apple-system, BlinkMacSystemFont, "Inter", "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  --font-mono: ui-monospace, "SF Mono", Menlo, Consolas, monospace;
  --radius-sm: 6px;
  --radius-md: 10px;
  --radius-lg: 16px;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  background-color: var(--bg-primary);
  color: var(--text-primary);
  font-family: var(--font-sans);
  line-height: 1.5;
  padding: 24px 16px;
  min-height: 100vh;
  -webkit-font-smoothing: antialiased;
}
.shell {
  max-width: 1120px;
  margin: 0 auto;
  display: flex;
  flex-direction: column;
  gap: 20px;
}

/* Header */
.header-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 16px;
  padding-bottom: 20px;
  border-bottom: 1px solid var(--border-subtle);
}
.brand-group {
  display: flex;
  align-items: center;
  gap: 14px;
}
.brand-icon {
  width: 44px;
  height: 44px;
  border-radius: var(--radius-md);
  background: linear-gradient(135deg, #0284c7 0%, #38bdf8 100%);
  display: grid;
  place-content: center;
  font-size: 22px;
  box-shadow: 0 0 20px var(--cyan-glow);
}
.brand-title {
  font-size: 20px;
  font-weight: 700;
  letter-spacing: -0.02em;
  color: #fff;
}
.brand-subtitle {
  font-size: 12px;
  font-family: var(--font-mono);
  color: var(--text-secondary);
  display: flex;
  align-items: center;
  gap: 6px;
}
.badge-tailscale {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  padding: 2px 7px;
  border-radius: 4px;
  background: rgba(56, 189, 248, 0.1);
  color: var(--cyan);
  font-size: 11px;
}
.header-actions {
  display: flex;
  align-items: center;
  gap: 12px;
}
.status-pill {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  padding: 8px 16px;
  border-radius: 9999px;
  font-size: 12px;
  font-weight: 600;
  font-family: var(--font-mono);
  background: var(--bg-surface);
  border: 1px solid var(--border-subtle);
}
.pulse-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--emerald);
  box-shadow: 0 0 10px var(--emerald);
  animation: pulse 2s infinite ease-in-out;
}
@keyframes pulse { 0%, 100% { opacity: 1; transform: scale(1); } 50% { opacity: 0.4; transform: scale(0.9); } }

/* Cards Grid */
.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
  gap: 14px;
}
.stat-card {
  background: var(--bg-surface);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-md);
  padding: 18px 20px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: 10px;
  transition: border-color 0.2s;
}
.stat-card:hover {
  border-color: var(--border-active);
}
.stat-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 11px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.06em;
  color: var(--text-tertiary);
}
.stat-header-icon {
  font-size: 14px;
  opacity: 0.8;
}
.stat-value {
  font-size: 28px;
  font-weight: 700;
  font-family: var(--font-mono);
  letter-spacing: -0.02em;
  color: #fff;
}
.stat-footer {
  font-size: 12px;
  font-family: var(--font-mono);
  color: var(--text-secondary);
  display: flex;
  align-items: center;
  gap: 8px;
  border-top: 1px solid rgba(255, 255, 255, 0.04);
  padding-top: 8px;
}
.stat-highlight {
  color: #fff;
  font-weight: 600;
}

/* Armed Traps Row */
.section-title {
  font-size: 13px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.06em;
  color: var(--text-tertiary);
  margin-bottom: 10px;
  display: flex;
  align-items: center;
  gap: 8px;
}
.traps-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 12px;
}
.trap-tile {
  background: var(--bg-surface);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-md);
  padding: 14px 16px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}
.trap-info {
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-inline-size: 0;
}
.trap-name {
  font-size: 13px;
  font-weight: 600;
  color: #fff;
}
.trap-desc {
  font-size: 11px;
  font-family: var(--font-mono);
  color: var(--text-secondary);
}
.trap-badge {
  font-size: 10px;
  font-weight: 700;
  font-family: var(--font-mono);
  padding: 3px 8px;
  border-radius: 4px;
  background: rgba(16, 185, 129, 0.12);
  color: var(--emerald);
  border: 1px solid rgba(16, 185, 129, 0.25);
  white-space: nowrap;
}

/* Latency Chart Box */
.chart-panel {
  background: var(--bg-surface);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-md);
  padding: 20px;
}
.chart-top {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 14px;
}
.chart-stats-row {
  display: flex;
  gap: 16px;
  font-family: var(--font-mono);
  font-size: 11px;
  color: var(--text-secondary);
}
.chart-stat-chip {
  background: var(--bg-surface-elevated);
  padding: 4px 8px;
  border-radius: 4px;
  border: 1px solid var(--border-subtle);
}
svg.sparkline {
  display: block;
  inline-size: 100%;
  block-size: 85px;
  overflow: visible;
}

/* Two Column Controls */
.controls-layout {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
  gap: 16px;
}
.panel-box {
  background: var(--bg-surface);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-md);
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.panel-header {
  display: flex;
  align-items: center;
  gap: 10px;
}
.panel-header h3 {
  font-size: 14px;
  font-weight: 600;
  color: #fff;
}
.panel-header p {
  font-size: 12px;
  color: var(--text-secondary);
}
.form-field {
  display: flex;
  flex-direction: column;
  gap: 6px;
}
.form-field label {
  font-size: 11px;
  font-weight: 600;
  font-family: var(--font-mono);
  text-transform: uppercase;
  color: var(--text-tertiary);
}
.form-inline {
  display: flex;
  gap: 8px;
}
input[type="text"], input[type="password"] {
  flex: 1;
  background: var(--bg-primary);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-sm);
  padding: 9px 12px;
  color: #fff;
  font-family: var(--font-mono);
  font-size: 13px;
  outline: none;
  min-inline-size: 0;
  transition: border-color 0.15s;
}
input:focus {
  border-color: var(--cyan);
}
button {
  background: var(--bg-surface-elevated);
  color: var(--text-primary);
  border: 1px solid var(--border-subtle);
  padding: 9px 16px;
  border-radius: var(--radius-sm);
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  transition: all 0.15s ease;
}
button:hover {
  background: #1e293b;
  border-color: rgba(255, 255, 255, 0.2);
}
button.primary {
  background: #0284c7;
  border-color: #0284c7;
  color: #fff;
}
button.primary:hover {
  background: #0369a1;
  border-color: #0369a1;
}
button.danger {
  background: rgba(244, 63, 94, 0.12);
  border-color: rgba(244, 63, 94, 0.3);
  color: var(--rose);
}
button.danger:hover {
  background: rgba(244, 63, 94, 0.22);
}
.status-msg {
  font-size: 11px;
  font-family: var(--font-mono);
  color: var(--cyan);
  min-height: 14px;
}

/* Activity Log Panel */
.log-panel {
  background: var(--bg-surface);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-md);
  padding: 20px;
}
.log-panel-top {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 14px;
  flex-wrap: wrap;
  gap: 10px;
}
.log-list {
  max-height: 320px;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding-right: 4px;
}
.log-item {
  background: var(--bg-primary);
  border: 1px solid rgba(255, 255, 255, 0.04);
  border-radius: var(--radius-sm);
  padding: 10px 14px;
  display: flex;
  align-items: flex-start;
  gap: 12px;
  font-family: var(--font-mono);
  font-size: 12px;
  border-left: 3px solid var(--cyan);
}
.log-item.danger {
  border-left-color: var(--rose);
  background: rgba(244, 63, 94, 0.04);
}
.log-item.warn {
  border-left-color: var(--amber);
  background: rgba(245, 158, 11, 0.04);
}
.log-item.success {
  border-left-color: var(--emerald);
  background: rgba(16, 185, 129, 0.04);
}
.log-time {
  color: var(--text-tertiary);
  font-size: 11px;
  min-width: 60px;
}
.log-badge {
  padding: 2px 7px;
  border-radius: 4px;
  font-size: 10px;
  font-weight: 700;
  background: var(--bg-surface-elevated);
  color: var(--cyan);
}
.log-badge.danger {
  background: rgba(244, 63, 94, 0.2);
  color: var(--rose);
}
.log-badge.warn {
  background: rgba(245, 158, 11, 0.2);
  color: var(--amber);
}
.log-badge.success {
  background: rgba(16, 185, 129, 0.2);
  color: var(--emerald);
}
.log-msg {
  flex: 1;
  word-break: break-all;
  color: var(--text-primary);
}

/* Footer */
footer {
  text-align: center;
  font-size: 12px;
  font-family: var(--font-mono);
  color: var(--text-tertiary);
  padding-top: 10px;
}
</style>
</head>
<body>
<div class="shell">
  <!-- Top Navigation Header -->
  <header class="header-bar">
    <div class="brand-group">
      <div class="brand-icon">&#x1F6E1;&#xFE0F;</div>
      <div>
        <h1 class="brand-title">HOME SECURITY SENTINEL</h1>
        <div class="brand-subtitle">
          <span>ESP8266 DEFENSE NODE</span>
          <span>&bull;</span>
          <span class="badge-tailscale">&#x1F517; Tailscale Ready</span>
        </div>
      </div>
    </div>
    <div class="header-actions">
      <div class="status-pill" id="global-status">
        <span class="pulse-dot" id="status-dot"></span>
        <span id="status-text">HOUSE IS SECURE</span>
      </div>
    </div>
  </header>

  <!-- 4 Top KPI Metric Cards -->
  <div class="stats-grid">
    <div class="stat-card">
      <div class="stat-header">
        <span>Defense Status</span>
        <span class="stat-header-icon">&#x1F512;</span>
      </div>
      <div class="stat-value" id="threat-state" style="color:var(--emerald)">ALL CLEAR</div>
      <div class="stat-footer">
        <span>Trap Probes: <span class="stat-highlight" id="val-honeypot">0</span></span>
        <span>&bull;</span>
        <span>Sabotage: <span class="stat-highlight" id="val-deauth">0</span></span>
      </div>
    </div>

    <div class="stat-card">
      <div class="stat-header">
        <span>Internet Health</span>
        <span class="stat-header-icon">&#x1F310;</span>
      </div>
      <div class="stat-value" id="val-ping-wan">-- ms</div>
      <div class="stat-footer">
        <span>Router: <span class="stat-highlight" id="val-ping-gw">-- ms</span></span>
        <span>&bull;</span>
        <span>Drops: <span class="stat-highlight" id="val-loss">0%</span></span>
      </div>
    </div>

    <div class="stat-card">
      <div class="stat-header">
        <span>Wi-Fi & Perimeter</span>
        <span class="stat-header-icon">&#x1F4F6;</span>
      </div>
      <div class="stat-value" id="val-rssi">-- dBm</div>
      <div class="stat-footer">
        <span id="val-ssid">Connecting...</span>
        <span>&bull;</span>
        <span>Phones: <span class="stat-highlight" id="val-phones">0</span></span>
      </div>
    </div>

    <div class="stat-card">
      <div class="stat-header">
        <span>Device Hardware</span>
        <span class="stat-header-icon">&#x2699;&#xFE0F;</span>
      </div>
      <div class="stat-value" id="val-uptime">00:00:00</div>
      <div class="stat-footer">
        <span>Memory: <span class="stat-highlight" id="val-heap">-- KB</span></span>
        <span>&bull;</span>
        <span>Power: <span class="stat-highlight" id="val-vcc">-- V</span></span>
      </div>
    </div>
  </div>

  <!-- Active Burglar Traps Overview -->
  <div>
    <div class="section-title">&#x1F6E1;&#xFE0F; External Traps &amp; Internal Misbehavior Defenses</div>
    <div class="traps-grid">
      <div class="trap-tile">
        <div class="trap-info">
          <div class="trap-name">Fake Computer (Telnet)</div>
          <div class="trap-desc">Port 23 &bull; Catches Terminal Scanners</div>
        </div>
        <div class="trap-badge">ARMED</div>
      </div>
      <div class="trap-tile">
        <div class="trap-info">
          <div class="trap-name">Fake Router Portal</div>
          <div class="trap-desc">Port 8080 &bull; Catches Web Attackers</div>
        </div>
        <div class="trap-badge">ARMED</div>
      </div>
      <div class="trap-tile">
        <div class="trap-info">
          <div class="trap-name">Fake Security Camera</div>
          <div class="trap-desc">Port 554 &bull; Catches Video Sniffers</div>
        </div>
        <div class="trap-badge">ARMED</div>
      </div>
      <div class="trap-tile">
        <div class="trap-info">
          <div class="trap-name">Wi-Fi Sabotage Detector</div>
          <div class="trap-desc">RF Sniffer &bull; Catches Jamming</div>
        </div>
        <div class="trap-badge">ARMED</div>
      </div>
      <div class="trap-tile">
        <div class="trap-info">
          <div class="trap-name">Chatterbox / Spam Alarm</div>
          <div class="trap-desc">Monitors Abnormal Broadcast Floods</div>
        </div>
        <div class="trap-badge" id="badge-spam">MONITORING</div>
      </div>
      <div class="trap-tile">
        <div class="trap-info">
          <div class="trap-name">Mystery Device Sweeper</div>
          <div class="trap-desc">Alerts on Unknown Devices Joining</div>
        </div>
        <div class="trap-badge" id="badge-devices">SCANNING</div>
      </div>
      <div class="trap-tile">
        <div class="trap-info">
          <div class="trap-name">Router Imposter Guard</div>
          <div class="trap-desc">Blocks Traffic Stealing &amp; Hijacks</div>
        </div>
        <div class="trap-badge">ARMED</div>
      </div>
      <div class="trap-tile">
        <div class="trap-info">
          <div class="trap-name">Website Lookup Guard</div>
          <div class="trap-desc">Verifies DNS Against Poisoning</div>
        </div>
        <div class="trap-badge">ARMED</div>
      </div>
    </div>
  </div>

  <!-- Latency Trend Chart Box -->
  <div class="chart-panel">
    <div class="chart-top">
      <div>
        <div class="section-title" style="margin-bottom:2px;">&#x1F4C8; Internet Speed & Latency Trend</div>
        <div style="font-size:12px; color:var(--text-secondary); font-family:var(--font-mono)">Rolling 24 samples (1.1.1.1 Target)</div>
      </div>
      <div class="chart-stats-row">
        <span class="chart-stat-chip">Target: Cloudflare (1.1.1.1)</span>
      </div>
    </div>
    <svg class="sparkline" id="ping-chart" viewBox="0 0 500 85" preserveAspectRatio="none">
      <path id="sparkline-fill" fill="rgba(56, 189, 248, 0.08)" d=""></path>
      <path id="sparkline-path" fill="none" stroke="#38bdf8" stroke-width="2.5" stroke-linecap="round" d=""></path>
    </svg>
  </div>

  <!-- Control Panels (2 Columns) -->
  <div class="controls-layout">
    <!-- Quick Security Tools -->
    <div class="panel-box">
      <div class="panel-header">
        <div>
          <h3>&#x1F4A1; Quick Actions & Tools</h3>
          <p>Test the burglar alarm or turn on computers remotely.</p>
        </div>
      </div>
      
      <div class="form-field">
        <label>Frozen Gadget Watchdog (Camera / Server IP)</label>
        <div class="form-inline">
          <input type="text" id="watch-ip" placeholder="Target IP to monitor (e.g. 192.168.1.50)" />
          <button class="primary" onclick="saveWatchedIP()">Watch Device</button>
        </div>
        <div class="status-msg" id="watch-status"></div>
      </div>

      <div class="form-field">
        <label>Remote Wake-Up Switch</label>
        <div class="form-inline">
          <input type="text" id="wol-mac" placeholder="Computer Hardware ID (e.g. AA:BB:CC:DD:EE:FF)" />
          <button class="primary" onclick="sendWoL()">Wake Host</button>
        </div>
        <div class="status-msg" id="wol-status"></div>
      </div>

      <div class="form-field">
        <label>Alarm Verification</label>
        <div class="form-inline">
          <button onclick="testAlert()">Simulate Burglar Trip</button>
          <button onclick="sendReportNow()">Send Morning Report Now</button>
        </div>
        <div class="status-msg" id="test-status"></div>
      </div>
    </div>

    <!-- Configuration & Network Settings -->
    <div class="panel-box">
      <div class="panel-header">
        <div>
          <h3>&#x2699;&#xFE0F; Network & Alert Settings</h3>
          <p>Update Discord alert destination or switch Wi-Fi.</p>
        </div>
      </div>

      <div class="form-field">
        <label>Discord Alert Feed Link</label>
        <div class="form-inline">
          <input type="text" id="webhook-url" placeholder="Paste Discord webhook link" />
          <button onclick="saveWebhook()">Save Link</button>
        </div>
        <div class="status-msg" id="webhook-status"></div>
      </div>

      <div class="form-field">
        <label>Switch Protected Wi-Fi</label>
        <div class="form-inline">
          <input type="text" id="wifi-ssid" placeholder="Wi-Fi Name" />
          <input type="password" id="wifi-pass" placeholder="Password" />
          <button class="primary" onclick="saveWiFi()">Connect</button>
        </div>
        <div class="status-msg" id="wifi-status"></div>
      </div>
    </div>
  </div>

  <!-- Activity Audit Log -->
  <div class="log-panel">
    <div class="log-panel-top">
      <div>
        <div class="section-title" style="margin-bottom:2px;">&#x1F4DC; Security Activity Log</div>
        <div style="font-size:12px; color:var(--text-secondary); font-family:var(--font-mono)">Saved to built-in memory &amp; surviving reboots</div>
      </div>
      <div style="display:flex; gap:8px;">
        <a href="/api/download-logs" target="_blank" style="text-decoration:none;"><button>&#x2B07;&#xFE0F; Download Records (CSV)</button></a>
        <button onclick="fetchLogs()">&#x1F504; Refresh</button>
        <button class="danger" onclick="clearLogs()">&#x1F5D1;&#xFE0F; Clear</button>
      </div>
    </div>
    <div class="log-list" id="logs-container">
      <div class="log-item info">
        <span class="log-time">00:00:00</span>
        <span class="log-badge">SYSTEM</span>
        <span class="log-msg">Sentinel active and guarding home network.</span>
      </div>
    </div>
  </div>

  <footer>
    Home Sentinel Node &bull; IP: <span id="foot-ip">--</span> &bull; Hardware ID: <span id="foot-mac">--</span>
  </footer>
</div>

<script>
function formatTime(sec) {
  let h = Math.floor(sec / 3600);
  let m = Math.floor((sec % 3600) / 60);
  let s = sec % 60;
  return [h,m,s].map(v => v < 10 ? '0' + v : v).join(':');
}

function updateSparkline(history) {
  if (!history || history.length < 2) return;
  const maxVal = Math.max(...history, 50);
  const minVal = 0;
  const width = 500;
  const height = 85;
  const step = width / (history.length - 1);
  
  let points = history.map((val, idx) => {
    let clamped = Math.max(0, val);
    let y = height - ((clamped - minVal) / (maxVal - minVal) * (height - 14) + 7);
    return `${idx * step},${y}`;
  });

  const pathStr = "M " + points.join(" L ");
  document.getElementById("sparkline-path").setAttribute("d", pathStr);
  
  const fillStr = pathStr + ` L ${(history.length-1)*step},${height} L 0,${height} Z`;
  document.getElementById("sparkline-fill").setAttribute("d", fillStr);
}

async function fetchStatus() {
  try {
    const res = await fetch('/api/status');
    if (!res.ok) return;
    const data = await res.json();
    
    document.getElementById("val-uptime").textContent = formatTime(data.uptime);
    document.getElementById("val-heap").textContent = Math.round(data.heap / 1024) + " KB";
    document.getElementById("val-vcc").textContent = (data.vcc / 1000).toFixed(2) + " V";
    document.getElementById("val-rssi").textContent = data.rssi + " dBm";
    document.getElementById("val-ssid").textContent = data.ssid || "Initial Setup";
    document.getElementById("val-ping-wan").textContent = (data.ping_wan >= 0 ? data.ping_wan + " ms" : "OFFLINE");
    document.getElementById("val-ping-gw").textContent = (data.ping_gw >= 0 ? data.ping_gw + " ms" : "ERR");
    document.getElementById("val-deauth").textContent = data.deauth_count;
    document.getElementById("val-honeypot").textContent = data.honeypot_hits;
    if (document.getElementById("val-phones")) {
      document.getElementById("val-phones").textContent = data.phones || 0;
    }
    if (data.active_devices !== undefined && document.getElementById("badge-devices")) {
      document.getElementById("badge-devices").textContent = data.active_devices + " FOUND";
    }
    if (data.spam_alert && document.getElementById("badge-spam")) {
      document.getElementById("badge-spam").textContent = "SPAM DETECTED";
      document.getElementById("badge-spam").style.color = "var(--rose)";
    }
    if (data.watched_ip && document.getElementById("watch-ip") && !document.getElementById("watch-ip").value) {
      document.getElementById("watch-ip").value = data.watched_ip;
    }
    document.getElementById("foot-ip").textContent = data.ip;
    document.getElementById("foot-mac").textContent = data.mac;

    const loss = data.pings_total > 0 ? Math.round((data.pings_failed / data.pings_total) * 100) : 0;
    document.getElementById("val-loss").textContent = loss + "%";

    const threatState = document.getElementById("threat-state");
    const statusText = document.getElementById("status-text");
    const statusDot = document.getElementById("status-dot");
    
    if (data.attack_alert) {
      threatState.textContent = "TRAP TRIPPED!";
      threatState.style.color = "var(--rose)";
      statusText.textContent = "BURGLAR TRAP TRIGGERED";
      statusDot.style.background = "var(--rose)";
      statusDot.style.boxShadow = "0 0 12px var(--rose)";
    } else if (data.wan_down) {
      threatState.textContent = "INTERNET DOWN";
      threatState.style.color = "var(--amber)";
      statusText.textContent = "INTERNET OFFLINE";
      statusDot.style.background = "var(--amber)";
      statusDot.style.boxShadow = "0 0 12px var(--amber)";
    } else {
      threatState.textContent = "ALL CLEAR";
      threatState.style.color = "var(--emerald)";
      statusText.textContent = "HOUSE IS SECURE";
      statusDot.style.background = "var(--emerald)";
      statusDot.style.boxShadow = "0 0 10px var(--emerald)";
    }

    if (data.ping_history) {
      updateSparkline(data.ping_history);
    }
  } catch (e) {
    console.error("Status fetch error", e);
  }
}

async function fetchLogs() {
  try {
    const res = await fetch('/api/logs');
    if (!res.ok) return;
    const logs = await res.json();
    const container = document.getElementById("logs-container");
    container.innerHTML = "";
    
    if (logs.length === 0) {
      container.innerHTML = '<div style="color:var(--text-tertiary); padding:12px; font-size:12px;">No suspicious activity recorded. All quiet.</div>';
      return;
    }

    logs.forEach(log => {
      const item = document.createElement("div");
      item.className = "log-item " + (log.sev || "info");
      
      const timeSpan = document.createElement("span");
      timeSpan.className = "log-time";
      timeSpan.textContent = formatTime(log.t);
      
      const badgeSpan = document.createElement("span");
      badgeSpan.className = "log-badge " + (log.sev || "info");
      
      let badgeLabel = log.type;
      if (log.type === "HONEYPOT") badgeLabel = "TRIPWIRE";
      if (log.type === "DEAUTH") badgeLabel = "SABOTAGE";
      if (log.type === "OUTAGE") badgeLabel = "INTERNET";
      if (log.type === "WOL") badgeLabel = "WAKE-UP";
      badgeSpan.textContent = badgeLabel;
      
      const msgSpan = document.createElement("span");
      msgSpan.className = "log-msg";
      msgSpan.textContent = log.msg;
      
      item.appendChild(timeSpan);
      item.appendChild(badgeSpan);
      item.appendChild(msgSpan);
      container.appendChild(item);
    });
  } catch (e) {
    console.error("Logs fetch error", e);
  }
}

async function sendWoL() {
  const mac = document.getElementById("wol-mac").value.trim();
  const status = document.getElementById("wol-status");
  if (!mac) { status.textContent = "Please enter a Hardware ID (MAC)"; return; }
  status.textContent = "Sending wake signal...";
  try {
    const res = await fetch('/api/wol', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({mac: mac})
    });
    const d = await res.json();
    status.textContent = d.message || "Wake signal dispatched!";
    fetchLogs();
  } catch (e) {
    status.textContent = "Error sending signal";
  }
}

async function testAlert() {
  const status = document.getElementById("test-status");
  status.textContent = "Triggering simulated alarm...";
  try {
    await fetch('/api/test-alert', {method: 'POST'});
    status.textContent = "Alarm tested! Strobe activated & Discord alert sent.";
    fetchStatus();
    fetchLogs();
  } catch (e) {
    status.textContent = "Failed to trigger test";
  }
}

async function sendReportNow() {
  const status = document.getElementById("test-status");
  status.textContent = "Sending Daily Morning Report...";
  try {
    await fetch('/api/send-report', {method: 'POST'});
    status.textContent = "Daily Morning Report dispatched to Discord!";
    fetchLogs();
  } catch (e) {
    status.textContent = "Failed to send report";
  }
}

async function saveWatchedIP() {
  const ip = document.getElementById("watch-ip").value.trim();
  const status = document.getElementById("watch-status");
  if (!ip) { status.textContent = "Please enter an IP address"; return; }
  status.textContent = "Setting watched device...";
  try {
    const res = await fetch('/api/watch-ip', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({ip: ip})
    });
    const d = await res.json();
    status.textContent = d.message || "Device watchdog activated!";
    fetchStatus();
    fetchLogs();
  } catch (e) {
    status.textContent = "Error setting watchdog";
  }
}

async function saveWiFi() {
  const ssid = document.getElementById("wifi-ssid").value.trim();
  const pass = document.getElementById("wifi-pass").value;
  const status = document.getElementById("wifi-status");
  if (!ssid) { status.textContent = "Wi-Fi Name cannot be empty"; return; }
  status.textContent = "Connecting to new Wi-Fi...";
  try {
    const res = await fetch('/api/wifi', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({ssid: ssid, pass: pass})
    });
    const d = await res.json();
    status.textContent = d.message || "Connecting...";
  } catch (e) {
    status.textContent = "Failed to send Wi-Fi settings";
  }
}

async function saveWebhook() {
  const url = document.getElementById("webhook-url").value.trim();
  const status = document.getElementById("webhook-status");
  status.textContent = "Saving alert link...";
  try {
    const res = await fetch('/api/webhook', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({url: url})
    });
    const d = await res.json();
    status.textContent = d.message || "Link saved!";
  } catch (e) {
    status.textContent = "Error saving link";
  }
}

async function clearLogs() {
  await fetch('/api/clear-logs', {method: 'POST'});
  fetchLogs();
}

setInterval(fetchStatus, 3000);
fetchStatus();
fetchLogs();
</script>
</body>
</html>
)rawliteral";
