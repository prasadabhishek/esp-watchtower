# ESP-Watchtower

**Turn a $3 microcontroller into a 24/7 guard and truth-teller for your home Wi-Fi.**

When your internet slows down, is it your Wi-Fi router or your internet provider? When a strange device connects to your network at 2 AM, how do you know? And when your server or desktop goes to sleep while you're away, how do you wake it back up?

**ESP-Watchtower** is a plug-and-forget hardware sentinel that plugs into any USB port, draws less than half a watt of power, and watches over your home network around the clock—with no monthly fees, no cloud accounts, and no apps to install.

---

<p align="center">
  <a href="docs/assets/brag.mp4">
    <img src="docs/assets/demo.gif" alt="ESP-Watchtower Overview" width="100%">
  </a>
</p>
<p align="center">
  <em>18-second overview. <strong><a href="docs/assets/brag.mp4">Watch full 1080p walkthrough (brag.mp4)</a></strong>.</em>
</p>

---

## How ESP-Watchtower Helps You

### 1. Stop Guessing Why the Internet Is Slow ("Who's at Fault?" Blame Meter)
Instead of rebooting your router blindly or sitting on hold with customer support, Watchtower continuously tests your home router and the outside internet separately so you know the exact culprit in one glance:
- **Your Home Wi-Fi:** Tells you if you're too far from the router or if local Wi-Fi interference is slowing you down.
- **Your Internet Provider (ISP):** Tells you when your Wi-Fi inside the house is fine, but your provider's line outside is failing.
- **Traffic Jams (Lag Spikes):** Spots when another device in the house is hogging bandwidth with a massive download or backup.

### 2. Catch Fake Websites & ISP Ad-Injection
Some internet providers and compromised routers secretly intercept mistyped website addresses to show you sponsored ads or tracking pages instead of a normal error page. Watchtower tests your router's address lookups against independent servers and warns you immediately if your traffic is being redirected or tampered with.

### 3. Early-Warning Burglar Traps for Your Wi-Fi
If a piece of malware, a compromised smart TV, or an intruder gets onto your Wi-Fi, the first thing they do is scan your network looking for computers, routers, or security cameras to break into. Watchtower sets up three harmless decoy "tripwires" on your network:
- **Decoy Computer Login**
- **Decoy Router Admin Page**
- **Decoy Security Camera Feed**

Normal devices never touch these decoys. If anything tries to log into or probe them, Watchtower sounds an instant alert on your phone and tells you exactly which device on your network did it.

### 4. Proof When Your Internet Provider Goes Down
Internet providers often deny outages unless you have timestamps. Watchtower automatically logs **confirmed ISP outages**—and *only* when your home Wi-Fi router is still up and healthy while the outside internet is dead. Unplugging your router or restarting your Wi-Fi won't create false alarms. When you call support for a bill credit, you can download a clean, timestamped outage receipt in one click.

### 5. Know Who Is Home & Spot Strangers Instantly
Watchtower keeps a live roster of every phone, laptop, TV, and smart plug on your Wi-Fi:
- **Stranger Alerts:** Get a notification on your phone the moment a brand-new, unrecognized device joins your Wi-Fi.
- **Family Arrival & Departure:** Optionally toggle arrival/departure tracking on specific devices so you know when family members get home or leave—without installing battery-draining location apps on their phones.
- **Remote Wake-Up & Server Recovery:** Turn on sleeping desktop PCs from your browser with one click, or mark a home media server as a starred VIP so Watchtower automatically wakes it back up if it freezes or drops offline.

### 6. Keep Smart Home Devices Working Offline
Many smart plugs, lightbulbs, and security cameras lose their schedules if they can't reach the internet to check the time. Watchtower acts as a local master clock on your home network so your smart home routines keep running on time even during an internet blackout, and connects directly with **Home Assistant** for local dashboards.

### 7. Custom Phone Alerts via Discord
Choose exactly which notifications you want sent to your phone—and mute the rest:
- 🔔 New unknown devices joining your Wi-Fi
- 🏠 Selected family devices arriving or leaving
- ⚠️ Confirmed internet provider outages and restorations
- 🚨 Someone tripping a burglar decoy trap
- 🐺 A critical home server going offline

---

## Quick Start (Works in 2 Minutes)

### What You Need
- Any **ESP8266** board (such as a **NodeMCU v2** or **Wemos D1 Mini**, ~$3–$4 online)
- A Micro-USB or USB-C cable
- [PlatformIO](https://platformio.org/install/cli) installed on your computer

### 1. Plug In & Flash
Clone this repository and upload it to your board with a single command:

```bash
git clone https://github.com/prasadabhishek/esp-watchtower.git
cd esp-watchtower
pio run -e nodemcuv2 -t upload
```

### 2. Connect to Your Wi-Fi
1. On your phone or laptop, connect to the setup Wi-Fi network **`ESP-Watchtower`** (Password: `sentinel123`) and open **`http://192.168.4.1/`** in your browser.
2. Scroll to **Section 8 (Network Settings)**, type your home 2.4 GHz Wi-Fi name and password, and click **Connect to Wi-Fi**.
3. *(Optional)* Or set your Wi-Fi directly over the USB cable from your terminal:
   ```bash
   echo "wifi YourWiFiName YourWiFiPassword" > /dev/cu.usbserial-0001
   ```
4. Open your Watchtower dashboard at **`http://watchtower.local/`** (or the IP address shown on screen) from any device at home.

---

## Running Self-Tests

To run the automated test suite on your computer before flashing:

```bash
pio test -e native
```

---

## License

MIT License — free for personal, home, and educational use.
