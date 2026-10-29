# ESP-Watchtower

An autonomous, always-on home network sentinel, canary decoy engine, and diagnostic monitor running on the ESP8266 microcontroller ($3–$4 hardware). Designed with zero cloud subscriptions, zero external telemetry, and a strict **zero-JavaScript** server-rendered architecture.

---

<p align="center">
  <a href="docs/assets/brag.mp4">
    <img src="docs/assets/demo.gif" alt="ESP-Watchtower Overview" width="100%">
  </a>
</p>
<p align="center">
  <em>18-second video walkthrough. <strong><a href="docs/assets/brag.mp4">Download / View 1080p Video (brag.mp4)</a></strong>.</em>
</p>

---

## System Architecture

Most home network monitors require dedicated single-board computers (Raspberry Pi), container runtimes, or monthly SaaS subscriptions. When power cuts out or your server crashes, the monitor fails alongside the network.

ESP-Watchtower runs on bare metal on an **ESP8266EX** (80/160 MHz Tensilica L106, 80 KB RAM, 4 MB Flash) drawing less than 0.5 Watts:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        ESP-WATCHTOWER ARCHITECTURE                     │
├────────────────────────┬───────────────────────────────────────────────┤
│ Diagnostic Engine      │ • Latency & jitter isolation (Gateway vs WAN) │
│                        │ • RFC 5452 DNS hijacking & benchmark probe    │
│                        │ • Confirmed ISP downtime logging (LittleFS)   │
├────────────────────────┼───────────────────────────────────────────────┤
│ Intrusion Decoys       │ • Port 23 Telnet honeypot (Linux login decoy) │
│                        │ • Port 8080 HTTP honeypot (Router admin trap) │
│                        │ • Port 554 RTSP honeypot (Camera decoy)       │
├────────────────────────┼───────────────────────────────────────────────┤
│ LAN Discovery & Radar  │ • L2 ARP table harvesting + NetBIOS queries   │
│                        │ • Passive family presence tracking            │
│                        │ • Mobile MAC randomization detection          │
├────────────────────────┼───────────────────────────────────────────────┤
│ Local Infrastructure   │ • RFC 5905 Micro-NTP server (UDP 123)         │
│                        │ • Subnet Wake-on-LAN dispatcher (UDP 9)       │
│                        │ • Home Assistant MQTT publisher (TCP 1883)    │
│                        │ • Configurable Discord webhook notifications  │
└────────────────────────┴───────────────────────────────────────────────┘
```

### Core Design Principles

1. **Zero Client-Side JavaScript:** The executive dashboard is 100% server-rendered using chunked HTTP streaming and pure CSS. Device table column sorting (`/?sort=...&order=...`) executes entirely on-chip in C++.
2. **Deterministic Memory Model:** Static buffers (48-byte NTP, 102-byte WoL), circular logging in LittleFS flash, and stack allocation preserve **~30 KB free heap RAM** (~50% safety margin) with zero heap fragmentation.
3. **Confirmed-Only Downtime Logic:** An ISP outage is logged *strictly* when the local default gateway responds to ICMP ping but upstream WAN endpoints fail (`gw_ok == true && wan_ok == false`). Local Wi-Fi drops or router reboots do not trigger false ISP fault records.

---

## Feature Capabilities

### 1. Network Latency & Bufferbloat Isolation
Measures latency to the local default gateway and public WAN resolvers (`1.1.1.1`, `8.8.8.8`) while tracking packet jitter across a 24-point rolling window using the RFC 3550 statistical algorithm:
$$J = \frac{1}{N-1} \sum_{i=1}^{N-1} |D(i, i-1)|$$
- **Crystal Clear:** Gateway < 10 ms, WAN < 35 ms, Jitter < 15 ms.
- **Upstream ISP Outage:** Gateway responsive, WAN 100% loss.
- **Wi-Fi Congestion / Weak Signal:** Gateway latency elevated (>45 ms).
- **Bufferbloat / Bandwidth Saturation:** Low gateway latency, high WAN latency inflation (>100 ms), high jitter (>30 ms).

### 2. Triple Canary Burglar Decoys
Binds unauthenticated decoy listeners on standard ports to detect unauthorized internal network reconnaissance:
- **Port 23 (Telnet):** Emulates an Ubuntu Linux terminal. Logs source IP, username, and password attempts.
- **Port 8080 (HTTP):** Emulates a router web administration portal. Traps credential-stuffing bots.
- **Port 554 (RTSP):** Mimics an IP security camera stream to flag port scanners sniffing for video feeds.

### 3. Confirmed ISP Outage Logger
Records confirmed upstream downtimes into persistent LittleFS flash storage. Each record includes event ID, start timestamp, outage duration, and verification flags. The minimum duration threshold is configurable (1, 3, 5, or 10 minutes) to filter transient packet loss.

### 4. Connected Devices Radar & Passive Presence
Harvests the L2 ARP cache and queries NetBIOS names (UDP 137) to track every active client on the subnet:
- **Presence Tracking:** Tracks arrival and departure events for starred family devices without battery-draining client apps. Presence notifications can be enabled or disabled individually per device.
- **Mobile Privacy Detection:** Inspects the IEEE 802 Locally Administered bit (`mac[0] & 0x02`) to distinguish randomized mobile addresses (iOS Private Wi-Fi, Android MAC randomization) from physical hardware NICs.
- **Server-Side Sorting:** Table headers allow instant sorting by IP, Name, Vendor, Latency, Seen Time, and Presence with zero JavaScript.

### 5. DNS Hijacking & Benchmark Probe (RFC 5452 UDP 53)
Dispatches raw DNS queries directly to the local router resolver versus Cloudflare (`1.1.1.1:53`). Utilizes 16-bit PRNG transaction IDs (`os_random()`) to test for cache poisoning vulnerability and inspects response flags to flag NXDOMAIN ad-injection or search redirection.

### 6. Local Micro-NTP Stratum-2 Time Server (RFC 5905 UDP 123)
Listens on UDP 123 and serves RFC 5905 compliant timestamps (`LOCL` RefID, Stratum 2, valid root delay and dispersion). Keeps offline smart plugs, Tuya devices, and IP cameras synchronized even during an internet blackout.

### 7. Remote Wake-on-LAN Dispatcher (UDP 9)
Constructs and broadcasts 102-byte Magic Packets across the subnet. Includes input validation and all-zero MAC guards to prevent packet corruption.

### 8. Home Assistant MQTT Bridge (TCP 1883)
Publishes real-time telemetry (WAN latency, gateway latency, DNS metrics, intruder trap counts, active device counts) to `homeassistant/sensor/watchtower/state` every 30 seconds.

### 9. Configurable Discord Webhook Alerts
Supports instant Discord webhook push notifications with granular preference toggles:
- 🔔 Stranger Alert (New uncataloged devices joining Wi-Fi)
- 🏠 Family Arrival & Departure
- ⚠️ Confirmed ISP Outages
- 🚨 Burglar Trap Decoys Tripped
- 🐺 Critical Server Freeze / Auto-WoL Dispatched

### 10. Dual-Mode Provisioning (SoftAP + Station)
Operates simultaneously in `WIFI_AP_STA` mode. If station connection fails or credentials change, the recovery hotspot `ESP-Watchtower` (Password: `sentinel123`) remains accessible at `http://192.168.4.1/` with a dedicated Wi-Fi configuration card. Credentials can also be configured directly via serial CLI (`wifi <SSID> <PASS>`).

---

## Hardware Specifications & Memory Budget

| Metric | Measured Value | Allocation Budget | Utilization |
|---|---|---|---|
| **Flash ROM** | 543,439 Bytes | 1,044,464 Bytes | **52.0%** |
| **RAM (Heap)** | 43,808 Bytes | 81,920 Bytes | **53.5%** |
| **Free Heap** | ~32,000 Bytes | — | Stable (0% leak) |
| **Power Consumption** | ~70–90 mA @ 3.3V | — | **< 0.35 Watts** |
| **Hardware Target** | NodeMCU v2 / ESP-12E / D1 Mini | ESP8266EX @ 80 MHz | — |

---

## Getting Started

### 1. Prerequisites
- [PlatformIO Core (CLI)](https://platformio.org/install/cli) or PlatformIO IDE
- A standard ESP8266 development board (NodeMCU v2 or Wemos D1 Mini)
- Micro-USB cable

### 2. Build & Flash
Clone the repository and upload firmware via PlatformIO:

```bash
git clone https://github.com/prasadabhishek/esp-watchtower.git
cd esp-watchtower

# Run native test suite (18 unit tests)
pio test -e native

# Compile and flash to connected board
pio run -e nodemcuv2 -t upload
```

### 3. Initial Configuration

1. Connect to the setup Wi-Fi hotspot broadcast by the device:
   - **SSID:** `ESP-Watchtower`
   - **Password:** `sentinel123`
2. Open `http://192.168.4.1/` in your browser.
3. In the **Network Settings** section:
   - Enter your home Wi-Fi SSID and Password, then click **Connect to Wi-Fi**.
   - Paste your Discord Webhook URL and toggle your notification preferences.
4. Once connected, the device acquires a local IP via DHCP (e.g. `192.168.50.191`) and serves the full executive dashboard on your home LAN.

Alternatively, configure credentials directly via the USB serial interface (115200 baud):
```bash
wifi "YourHomeSSID" "YourPassword"
webhook "https://discord.com/api/webhooks/..."
status
```

---

## Automated Test Verification

The core logic is isolated from hardware APIs in [`include/watchtower_logic.h`](include/watchtower_logic.h) and verified through 18 native unit tests covering MAC parsing, Wake-on-LAN frame layout, Blame Meter algorithms, RFC 3550 jitter calculation, NetBIOS decoding, server-side table sorting, and outage threshold logic:

```bash
$ pio test -e native

Processing * in native environment
--------------------------------------------------------------------------------
test/test_main.cpp:364: test_mac_parser_colon          [PASSED]
test/test_main.cpp:365: test_mac_parser_hyphen         [PASSED]
test/test_main.cpp:366: test_mac_parser_invalid        [PASSED]
test/test_main.cpp:367: test_magic_packet_structure    [PASSED]
test/test_main.cpp:368: test_ip_helpers                [PASSED]
test/test_main.cpp:369: test_lan_device_upsert_state   [PASSED]
test/test_main.cpp:370: test_vip_tagging               [PASSED]
test/test_main.cpp:371: test_outage_configurable_thresh[PASSED]
test/test_main.cpp:372: test_netbios_parser            [PASSED]
test/test_main.cpp:373: test_blame_meter               [PASSED]
test/test_main.cpp:374: test_jitter_calculation        [PASSED]
test/test_main.cpp:375: test_randomized_mac_detection  [PASSED]
test/test_main.cpp:376: test_outage_claim_receipt      [PASSED]
test/test_main.cpp:377: test_dns_probe_packets         [PASSED]
test/test_main.cpp:378: test_micro_ntp_server          [PASSED]
test/test_main.cpp:379: test_mqtt_telemetry_packets    [PASSED]
test/test_main.cpp:380: test_device_presence_config    [PASSED]
test/test_main.cpp:381: test_server_side_device_sorting[PASSED]
--------------------- native:* [PASSED] Took 0.42 seconds ---------------------
================= 18 test cases: 18 succeeded in 00:00:00.416 =================
```

---

## License

MIT License. Open source and free for home and personal use.
