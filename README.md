# 🛡️ ESP-Watchtower

> **An autonomous, always-on Home Network Sentinel, Canary Honeypot & ISP Blame Engine for under $4.**  
> Built for the ESP8266 (NodeMCU / D1 Mini) with zero recurring subscriptions and a strict **Zero-JS Ponytail Architecture**.

---

<p align="center">
  <a href="https://github.com/prasadabhishek/esp-watchtower/raw/main/docs/assets/brag.mp4">
    <img src="docs/assets/demo.gif" alt="ESP-Watchtower 19-Second Tour" width="100%">
  </a>
</p>
<p align="center">
  🎬 <em>19-second video tour. <strong><a href="https://github.com/prasadabhishek/esp-watchtower/raw/main/docs/assets/brag.mp4">Click to watch/download full 1080p video with audio (brag.mp4)</a></strong>.</em>
</p>

---

## ⚡ Why ESP-Watchtower? (The Ponytail Philosophy)

Most home network monitoring tools are heavy, bloated, and expensive:
- They require dedicated Raspberry Pis ($60–$100), heavy Docker containers, Node.js runtimes, or monthly SaaS subscriptions.
- If a server crashes or your home network drops, the monitor goes down with it.

**ESP-Watchtower takes the opposite approach:**
It turns a **$3 ESP8266EX microcontroller** (80MHz/160MHz Tensilica L106, 80KB RAM, 4MB Flash) into a dedicated, unkillable, independent network sentinel that consumes less than 0.5 Watts.

### Key Architectural Tenets
1. **Zero JavaScript UI:** The entire 4-tab dashboard is rendered with **100% pure CSS** (`input[type=radio]:checked ~ .pane`). Zero frontend frameworks, zero npm dependencies, zero client-side DOM parsing crashes.
2. **Deterministic Memory Footprint:** Stack-allocated buffers (48-byte NTP, 102-byte WoL), static ring buffers for logs, and zero dynamic heap fragmentation. Consistently maintains **~29 KB of free heap** (~50% safety margin).
3. **Line-Rate LwIP Performance:** Native UDP/TCP non-blocking event loops running on bare-metal Arduino Core 3.1.2 with LwIP 2.1.3.

---

## 🪤 The 10 Core Capabilities

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                                ESP-WATCHTOWER ARCHITECTURE                             │
├────────────────────────────────┬───────────────────────────────────────────────────────┤
│ 1. "Who's at Fault?" Meter    │ • Diagnoses Wi-Fi (GW) vs. ISP (WAN) vs. Bufferbloat  │
│ 2. Stranger Alert (Privacy-OK) │ • Distinguishes IEEE 802 Local MACs from fixed devices│
│ 3. Passive Family Presence     │ • Tracks VIP phone arrival / departure with zero apps │
│ 4. SLA Outage Receipt Generator│ • Generates copy-paste billing credit proof for ISPs  │
│ 5. Silent Server / NAS Sentinel│ • Auto-wakes frozen servers via UDP 9 Magic Packets   │
│ 6. Triple Canary Honeypot      │ • Port 23 (Telnet), Port 8080 (Web), Port 554 (RTSP)  │
│ 7. DNS Hijacking & Bench Probe │ • RFC 1035 UDP 53 queries with RFC 5452 16-bit entropy│
│ 8. Local Micro-NTP Server      │ • RFC 5905 Stratum 2 time sync for offline smart plugs│
│ 9. Subnet Wake-on-LAN Trigger  │ • L2 broadcast magic packets with all-zero MAC guard  │
│ 10. Home Assistant MQTT Bridge │ • Native TCP 1883 telemetry export (zero cloud)       │
└────────────────────────────────┴───────────────────────────────────────────────────────┘
```

### 1. ⚖️ The "Who's at Fault?" Blame Meter (Wi-Fi vs. ISP vs. Bufferbloat)
When Zoom freezes or Netflix buffers, users don't know who to blame. Watchtower calculates statistical packet jitter across a 24-point rolling window:
$$\text{Jitter} = \frac{1}{N-1} \sum_{i=1}^{N-1} |D(i, i-1)|$$
- **🟢 Crystal Clear:** Gateway < 5 ms, WAN < 30 ms, Jitter < 10 ms.
- **🔴 ISP Failure:** Gateway nominal, WAN latency spike or 100% packet drop.
- **🟡 Bufferbloat:** Nominal gateway latency, high WAN latency inflation (>120 ms), and severe jitter (>30 ms).

### 2. 🔔 Stranger Alert with Mobile MAC Privacy Discrimination
Alerts your phone via Discord the second an unknown device connects to your Wi-Fi. It checks the IEEE 802 Locally Administered bit (`mac[0] & 0x02`):
- Modern smartphones (iOS Private Wi-Fi, Android MAC Randomization) are labeled as `[Private Mobile MAC]`, avoiding false alarms.
- Static hardware interfaces are resolved against internal OUI and NetBIOS tables.

### 3. 🏠 Arrival & Departure Presence Engine (Zero Apps)
Monitors family VIP devices (e.g. MacBook, iPhone) passively through Wi-Fi ARP state. When someone walks into the driveway and joins Wi-Fi, Watchtower records an immediate **[ARRIVAL]**. If absent for 5+ minutes, it logs **[DEPARTURE]**—without battery-draining GPS tracking apps.

### 4. 🧾 ISP Outage Receipt & Bill Credit Generator
Tracks WAN drops in LittleFS `/audit.log`. When internet cuts out, Watchtower logs exact timestamps and calculates prorated billing credit reimbursement based on your monthly subscription cost (e.g. `$80/mo` SLA credit claim).

### 5. 🐺 Silent Server / NAS Freeze Sentinel
Monitors your Synology NAS, Plex server, or homelab PC. If a VIP host fails to answer ICMP pings for 2 consecutive minutes, Watchtower automatically transmits an 802.3 Wake-on-LAN packet to revive it.

### 6. 🪤 Triple Canary Honeypot Decoys
Arms three canary traps inside your home network:
- **Door 1 (Port 23 Telnet):** Emulates an Ubuntu Linux terminal. Captures intruder IP, username, and brute-force passwords.
- **Door 2 (Port 8080 Web):** Serves a fake router gateway login screen. Logs credential stuffing attacks.
- **Door 3 (Port 554 RTSP):** Mimics an IP security camera stream to trap network scanners sniffing for video feeds.

### 7. 🔍 DNS Hijacking & Benchmark Probe (UDP 53)
Sends raw RFC 1035 UDP queries to your local router DNS vs Cloudflare (`1.1.1.1`) and Google (`8.8.8.8`). Flags upstream NXDOMAIN ad-injection and measures lookup latency using RFC 5452 cryptographic 16-bit PRNG entropy (`os_random()`).

### 8. ⏱️ Local Micro-NTP Time Server (UDP 123)
Serves RFC 5905 compliant timestamps on your local network (Stratum 2, LOCL RefID, non-zero root dispersion and delay). Provides accurate clock sync for smart bulbs, Tuya plugs, and IP cameras isolated from the public internet.

### 9. ⚡ Subnet Wake-on-LAN Trigger (UDP 9)
Broadcasts 102-byte Magic Packets (`6 × 0xFF` followed by 16 repetitions of the target MAC address) with strict zero-MAC filtering to prevent malformed packet flooding.

### 10. 📡 Home Assistant MQTT Telemetry (TCP 1883)
Publishes real-time intruder hits, WAN uptime, and active device counts directly to your local Home Assistant instance over MQTT.

---

## 🖥️ The 4-Tab Pure-CSS Dashboard

The dashboard (`http://<device-ip>/`) is split into 4 tabs rendered with zero JavaScript:

```
[ Tab 1: 📡 Live Blame Radar ] [ Tab 2: 👥 LAN Devices (28) ] [ Tab 3: 📋 Outage Receipts ] [ Tab 4: ⚙️ Canary & Settings ]
```

1. **Tab 1: Live Blame Radar:** Large visual health verdict, real-time Jitter badge, gateway vs. WAN latency gauges, 24-point ping sparkline, and an expandable hardware diagnostics drawer (`Free Heap`, `VCC`, `RSSI`).
2. **Tab 2: LAN Devices:** Live table of all discovered network devices with vendor resolution, NetBIOS hostnames, Private MAC badges, VIP toggles, and one-click WoL triggers.
3. **Tab 3: SLA Outage Receipts:** Persistent outage log with interactive dollar-value SLA credit calculator and `.csv` audit export.
4. **Tab 4: Settings & Canary Decoys:** Wi-Fi credentials manager, Discord webhook configurator, MQTT broker settings, manual DNS benchmark trigger, and canary decoy stats.

---

## 🔒 Security Hardening

- **CSRF Origin Whitelisting:** All state-changing POST endpoints (`/api/wol`, `/api/wifi`, `/api/webhook`, `/api/dns-bench`) inspect the HTTP `Origin` header. Requests from third-party websites receive an immediate `403 Forbidden`.
- **RFC 5452 DNS Resilience:** Avoids static query IDs (`0x1001`), using hardware-assisted 16-bit pseudo-random transaction IDs.
- **RFC 5905 NTP Synchronization Check:** Does not answer NTP requests until upstream SNTP has acquired valid time synchronization.
- **Flash Protection:** Limits write cycles to LittleFS via in-memory deduplication of NetBIOS names and event logs.

---

## 🛠️ Hardware Requirements

* **Microcontroller:** ESP8266EX development board (NodeMCU v2 / v3 or WeMos D1 Mini).
* **Flash:** 4MB Flash (configured as 1MB SPIFFS/LittleFS).
* **Power:** 5V Micro-USB (< 0.5W power consumption, runs indefinitely on any USB phone charger).
* **Total Cost:** **~$2.50 – $4.00 USD**.

---

## 🚀 Quickstart Guide

### 1. Clone the Repository
```bash
git clone https://github.com/prasadabhishek/esp-watchtower.git
cd esp-watchtower
```

### 2. Run Host-Native Unit Tests
Verify all pure logic algorithms (Jitter math, MAC privacy detection, NTP response builder, WoL magic packet assembly) without physical hardware:
```bash
pio test -e native
# Tests: 16 succeeded in 0.41s
```

### 3. Flash to Physical Hardware
Connect your NodeMCU board via USB and run:
```bash
pio run -e nodemcuv2 -t upload
```

### 4. Provision Wi-Fi Credentials
On first boot (or if Wi-Fi is unconfigured), Watchtower starts a captive access point:
1. Connect to Wi-Fi: **`ESP-Watchtower`** (Password: `sentinel123`).
2. Open **`http://192.168.4.1/`** in your browser.
3. Select your home network and enter the password. The board will connect and receive a local IP (e.g. `192.168.50.191`).

### 5. Configure Discord Phone Alerts
1. In Discord: Channel Settings ➔ **Integrations** ➔ **Webhooks** ➔ **Copy Webhook URL**.
2. Go to **Tab 4 (Settings)** on your Watchtower dashboard, paste the webhook URL, and click **Save Webhook**.

---

## 🌍 Remote Access Anywhere via Tailscale

Access your Watchtower dashboard securely from outside your home without exposing ports to the internet:

### Option A: Tailscale Serve (Reverse Proxy)
Run on any Mac or Linux machine on your home LAN:
```bash
tailscale serve --bg http://<ESP_IP>:80
```
This generates a private HTTPS endpoint (e.g. `https://your-node.ts.net/`) accessible to your Tailscale devices.

### Option B: Tailscale Subnet Router
Advertise your home subnet:
```bash
tailscale up --advertise-routes=192.168.1.0/24
```
You can now navigate directly to `http://<ESP_IP>` from your phone anywhere in the world.

---

## 📡 REST API Reference

| Endpoint | Method | Security | Description |
| :--- | :--- | :--- | :--- |
| `/api/status` | `GET` | Open | Returns live telemetry (uptime, heap, ping, jitter, blame verdict, DNS, NTP). |
| `/api/logs` | `GET` | Open | Returns the in-memory rolling event log as JSON. |
| `/api/download-logs` | `GET` | Open | Downloads persistent LittleFS audit history (`/audit.log`) as a `.csv` file. |
| `/api/wol` | `POST` | CSRF Guard | Broadcasts an 802.3 Wake-on-LAN Magic Packet (`{"mac":"AA:BB:CC:DD:EE:FF"}`). |
| `/api/dns-bench` | `POST` | CSRF Guard | Triggers a fresh UDP 53 RFC 1035 query to router DNS vs. 1.1.1.1. |
| `/api/send-report` | `POST` | CSRF Guard | Dispatches a Daily Morning Report summary to Discord immediately. |
| `/api/test-alert` | `POST` | CSRF Guard | Simulates a burglar alarm trip (fires strobe + test alert). |
| `/api/wifi` | `POST` | CSRF Guard | Updates SSID and WPA2 credentials (`{"ssid":"...","pass":"..."}`). |
| `/api/webhook` | `POST` | CSRF Guard | Updates the Discord webhook URL (`{"url":"..."}`). |
| `/api/mqtt` | `POST` | CSRF Guard | Updates Home Assistant MQTT broker host (`{"host":"192.168.1.50"}`). |
| `/api/clear-logs` | `POST` | CSRF Guard | Purges flash audit records and resets counters. |

---

## 📊 Hardware Resource Utilization

| Metric | Compile-Time Usage | Available / Total | Free Headroom |
| :--- | :--- | :--- | :--- |
| **RAM (Heap / Data)** | **41,856 bytes** (51.1%) | 81,920 bytes | **28,944 bytes Free Heap** at runtime |
| **Flash Memory** | **526,279 bytes** (50.4%) | 1,044,464 bytes | **518,185 bytes Free Flash** |
| **Power Consumption** | **~0.42 Watts** (84 mA @ 5V) | Standard USB Port | Negligible annual cost (<$0.50/year) |

---

## 🧪 Unit Tests

Pure logical validation is segregated in [`watchtower_logic.h`](file:///Users/abhishekprasad/esp-watchtower/include/watchtower_logic.h) and verified with PlatformIO Native:
- `test_mac_address_parsing`
- `test_wake_on_lan_magic_packet`
- `test_outage_duration_calculation`
- `test_sla_credit_calculation`
- `test_packet_loss_calculation`
- `test_uptime_formatting`
- `test_discord_json_escaping`
- `test_ip_address_parsing`
- `test_ip_to_string`
- `test_device_table_capacity`
- `test_device_vip_status_retention`
- `test_daily_report_format`
- `test_blame_diagnosis`
- `test_rfc1035_dns_query_format`
- `test_jitter_calculation`
- `test_randomized_mac_detection`

---

## 📜 License
This project is open-source under the [MIT License](LICENSE).
