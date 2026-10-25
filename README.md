# 🛡️ ESP-Watchtower

> **An autonomous, always-on Home Network Sentinel & Canary Honeypot for under $4.**  
> Built for the ESP8266 (NodeMCU / D1 Mini) with zero recurring subscriptions.

---

## ⚡ What is ESP-Watchtower?

ESP-Watchtower transforms a cheap **$3 ESP8266 microcontroller** into a dedicated, 24/7 hardware sentinel for your home network. It guards your local network from intrusions, watches your internet connection health, alerts you to power outages, and notifies you immediately on Discord whenever suspicious activity occurs.

```
                       ┌────────────────────────────────────────────────────────┐
                       │               ESP-WATCHTOWER SENTINEL                  │
                       ├────────────────────────────────────────────────────────┤
[Burglar Traps]        │ • Door 1: Fake Computer Terminal (Telnet Port 23)      │
(Canary Decoys)        │ • Door 2: Fake Router Admin Gateway (HTTP Port 8080)   │
                       │ • Door 3: Fake IP Security Camera (RTSP Port 554)      │
                       ├────────────────────────────────────────────────────────┤
[Network Health]       │ • Continuous Internet Ping & Latency Sparkline         │
(Reliability Tracker)  │ • Outage & Dropdown Duration Logger                    │
                       │ • Gateway / Router Latency Tracker                     │
                       ├────────────────────────────────────────────────────────┤
[Home Ops & Alerts]    │ • Remote Wake-Up Switch (Wake-on-LAN over Tailscale)   │
                       │ • Instant Discord Push Alerts on Intrusion/Outage      │
                       │ • Automated 24-Hour Daily Morning Report               │
                       │ • Persistent Built-in Flash Logs (Survives Reboots)    │
                       ├────────────────────────────────────────────────────────┤
[Access Anywhere]      │ • Modern, Responsive Dark-Mode Web Dashboard           │
                       │ • Tailscale Ready (Access securely from anywhere)       │
                       └────────────────────────────────────────────────────────┘
```

---

## 🪤 Features in Everyday Terms

* **🚪 3 Armed Burglar Traps (Honeypots):**
  * **Door 1 (Port 23):** Emulates an Ubuntu Linux terminal. Captures intruder IP, username, and brute-force passwords.
  * **Door 2 (Port 8080):** Serves a realistic router gateway login screen. Logs credential stuffing attacks.
  * **Door 3 (Port 554):** Mimics an IP security camera stream to catch network scanners sniffing for video feeds.
* **🌐 Internet Health Tracker:** Continuously pings your router and `1.1.1.1` to track latency, packet loss, and exact outage durations.
* **⚡ Power Outage Alert:** Detects when home electricity cuts out and returns, notifying you of the exact downtime duration.
* **💡 Remote Wake-Up Switch:** Send a Wake-on-LAN magic packet from your phone through the dashboard to turn on sleeping PCs or home servers remotely.
* **📊 Daily Morning Report:** Automatically sends a 24-hour summary of all network activity and threats to your Discord channel every morning.
* **🔒 Accessible Anywhere via Tailscale:** Securely access the full dashboard from outside your home without opening router ports or paying for static IPs.
* **💾 Built-in Memory:** All logs are permanently appended to internal LittleFS flash storage (`/audit.log`) so no evidence is lost during power loss.

---

## 🛠️ Hardware Requirements

* **ESP8266 Development Board:**
  * NodeMCU v2 / v3 or WeMos D1 Mini (with 4MB flash & CP2102/CH340 USB chip).
  * Cost: **~$2.50 – $4.00 USD** (₹250 – ₹350 INR).
* **Micro-USB Cable & Power Supply:** Any 5V standard phone charger or USB port (consumes less than 0.5 Watts).

---

## 🚀 Quickstart Guide

### 1. Clone the Repository
```bash
git clone https://github.com/prasadabhishek/esp-watchtower.git
cd esp-watchtower
```

### 2. Build & Flash with PlatformIO
Connect your ESP8266 board via USB and run:
```bash
# Compile and flash firmware onto the board
pio run -t upload
```

### 3. Connect to Home Wi-Fi
1. Once flashed, the board creates a temporary setup Wi-Fi network:
   * **Network Name:** `ESP-Watchtower`
   * **Password:** `sentinel123`
2. Connect to it on your phone or laptop, and open **`http://192.168.4.1`** in your browser.
3. In the **Connect to Home Wi-Fi** section, enter your home Wi-Fi name and password, then click **Connect**.
4. The board will connect to your home Wi-Fi and receive an IP address on your local network (e.g. `192.168.1.150`).

### 4. Enable Discord Phone Alerts
1. In Discord: Open channel settings ➔ **Integrations** ➔ **Webhooks** ➔ **New Webhook** ➔ **Copy Webhook URL**.
2. Open your Watchtower dashboard (`http://<device-ip>`), paste your webhook link into the **Direct Phone Alerts** box, and click **Save Link**.
3. You will immediately receive a verification message on Discord!

---

## 🌍 Remote Access via Tailscale

To view your Sentinel dashboard from anywhere in the world without exposing ports to the public internet:

### Option A: Tailscale Serve (Reverse Proxy)
Run this on any Mac, Linux machine, or Raspberry Pi on your home network running Tailscale:
```bash
tailscale serve --bg http://<ESP_LOCAL_IP>:80
```
This gives you a private HTTPS URL (e.g. `https://your-node.ts.net/`) accessible exclusively to your devices on Tailscale.

### Option B: Tailscale Subnet Router
Advertise your home subnet from your local Tailscale machine:
```bash
tailscale up --advertise-routes=192.168.1.0/24
```
You can now directly open `http://<ESP_LOCAL_IP>` from your phone or laptop anywhere in the world.

---

## 📡 REST API Reference

| Endpoint | Method | Description |
| :--- | :--- | :--- |
| `/api/status` | `GET` | Returns live telemetry (uptime, ping, RSSI, trap hits, memory). |
| `/api/logs` | `GET` | Returns the rolling in-memory security audit log as JSON. |
| `/api/download-logs` | `GET` | Downloads the persistent Flash audit log (`/audit.log`) as a `.csv` file. |
| `/api/send-report` | `POST` | Immediately dispatches a Daily Morning Report to Discord. |
| `/api/test-alert` | `POST` | Simulates a burglar trap trip (flashes LED strobe + sends alert). |
| `/api/wol` | `POST` | Broadcasts an 802.3 Wake-on-LAN Magic Packet (`{"mac":"AA:BB:CC:DD:EE:FF"}`). |
| `/api/wifi` | `POST` | Updates target Wi-Fi credentials (`{"ssid":"...","pass":"..."}`). |
| `/api/webhook` | `POST` | Saves a new Discord alert webhook URL (`{"url":"..."}`). |
| `/api/clear-logs` | `POST` | Clears stored flash audit logs and resets memory buffers. |

---

## 🧪 Running Unit Tests

ESP-Watchtower includes a host-native unit test suite testing pure logic (MAC parsing, Wake-on-LAN Magic Packet generation, outage duration math, packet loss formulas, uptime formatting, Discord JSON payload escaping) without requiring physical hardware:

```bash
# Run host-native unit tests via PlatformIO
pio test -e native
```

---

## 📜 License
This project is open-source under the [MIT License](LICENSE).
