#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266Ping.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <time.h>

extern "C" {
#include "lwip/etharp.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
}

#include "config.h"
#include "html_ui.h"
#include "trap_ui.h"
#include "watchtower_logic.h"

ADC_MODE(ADC_VCC);

// Global State
SystemMetrics metrics;
LogEntry eventLogs[MAX_LOG_ENTRIES];
int logCount = 0;
int pingHistory[PING_HISTORY_LEN];
int pingHistoryIdx = 0;

// Servers
ESP8266WebServer server(HTTP_PORT);
ESP8266WebServer trapServer(HTTP_TRAP_PORT);
WiFiServer telnetServer(TELNET_PORT);
WiFiServer rtspServer(554);
WiFiClient activeTelnetClient;
WiFiUDP udp;
WiFiUDP ntpUdp;
WiFiEventHandler wifiDisconnectHandler;
WiFiEventHandler wifiConnectHandler;

// Timers
bool alertActive = false;
unsigned long alertStartTime = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastWatchdogRun = 0;
unsigned long lastDailyReportTime = 0;
const unsigned long TWENTY_FOUR_HOURS = 86400000UL;
bool ledState = HIGH;

// Config defaults (empty by default; configured via Web UI or Serial)
char cfg_ssid[33] = "";
char cfg_pass[64] = "";
char cfg_webhook[160] = "";
char cfg_last_wol[18] = "";
char cfg_mqtt_host[40] = "";
uint32_t cfg_outage_threshold_sec = 300; // Default 5 minutes (300 seconds)

// Notification filter preferences
bool cfg_notify_stranger = true;
bool cfg_notify_presence = true;
bool cfg_notify_outage = true;
bool cfg_notify_traps = true;
bool cfg_notify_freeze = true;

// Verified ISP Outage History (Last 8 confirmed outages)
#define MAX_OUTAGE_HISTORY 8
IspOutageRecord outageHistory[MAX_OUTAGE_HISTORY];
uint8_t outageHistoryCount = 0;

void recordOutageStart(uint32_t startSec) {
  if (outageHistoryCount < MAX_OUTAGE_HISTORY) {
    outageHistory[outageHistoryCount].id = outageHistoryCount + 1;
    outageHistory[outageHistoryCount].start_sec = startSec;
    outageHistory[outageHistoryCount].duration_sec = 0;
    outageHistory[outageHistoryCount].is_active = true;
    outageHistoryCount++;
  } else {
    for (int i = 0; i < MAX_OUTAGE_HISTORY - 1; i++) {
      outageHistory[i] = outageHistory[i + 1];
    }
    outageHistory[MAX_OUTAGE_HISTORY - 1].id = metrics.total_outages;
    outageHistory[MAX_OUTAGE_HISTORY - 1].start_sec = startSec;
    outageHistory[MAX_OUTAGE_HISTORY - 1].duration_sec = 0;
    outageHistory[MAX_OUTAGE_HISTORY - 1].is_active = true;
  }
}

void recordOutageEnd(uint32_t durationSec) {
  if (outageHistoryCount > 0) {
    outageHistory[outageHistoryCount - 1].duration_sec = durationSec;
    outageHistory[outageHistoryCount - 1].is_active = false;
  }
}

// Feature #7 (DNS Probe), #8 (Micro-NTP), #9 (WoL Watchdog), #10 (MQTT Telemetry) State
int dnsRouterMs = -1;
int dnsCloudflareMs = -1;
bool dnsHijacked = false;
unsigned long dnsQuerySentMs = 0;
uint32_t ntpServedCount = 0;
uint32_t wolSentCount = 0;
uint32_t mqttPublishedCount = 0;

// Telnet Burglar Trap State
enum TelnetState { T_DISCONNECTED, T_SENT_BANNER, T_WAIT_USER, T_WAIT_PASS };
TelnetState telnetState = T_DISCONNECTED;
String telnetUser = "";
String telnetInput = "";

// LAN Radar & VIP State
LanDevice lanDevices[MAX_LAN_DEVICES];
uint8_t lanDeviceCount = 0;
bool radarSweeperActive = true;
uint8_t radarScanHost = 1;
unsigned long lastRadarTickMs = 0;
unsigned long lastVipTickMs = 0;

// Forward declarations
void addLog(const char* type, const String& msg, const char* severity);
void triggerStrobe();
void sendWakeOnLan(const char* macStr);
void handleSerialCLI();
void saveConfig();
void saveVipList();
void loadVipList();
void handleLanRadar();
void loadConfig();
void dispatchWebhook(const char* type, const String& msg, const char* severity);
void dispatchRawDiscord(const String& rawContent);
void sendDailyMorningReport();
void appendPersistentLog(const char* type, const String& msg, const char* severity);
void runDnsBenchmarkProbe();
void publishMqttTelemetry();

// --- Outbound Discord Alert Sender ---
void dispatchRawDiscord(const String& rawContent) {
  if (strlen(cfg_webhook) == 0 || WiFi.status() != WL_CONNECTED) return;
  
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  client->setInsecure();
  client->setBufferSizes(1024, 512);
  client->setTimeout(3000);

  HTTPClient http;
  if (http.begin(*client, cfg_webhook)) {
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"content\":\"" + rawContent + "\"}";
    http.POST(payload);
    http.end();
  }
}

void dispatchWebhook(const char* type, const String& msg, const char* severity) {
  if (strlen(cfg_webhook) == 0 || WiFi.status() != WL_CONNECTED) return;
  
  // Configurable notification filtering
  if (strcmp(type, "STRANGER") == 0 && !cfg_notify_stranger) return;
  if ((strcmp(type, "PRESENCE") == 0 || strcmp(type, "PRESENCE_IN") == 0 || strcmp(type, "PRESENCE_OUT") == 0) && !cfg_notify_presence) return;
  if (strcmp(type, "OUTAGE") == 0 && !cfg_notify_outage) return;
  if (strcmp(type, "HONEYPOT") == 0 && !cfg_notify_traps) return;
  if (strcmp(type, "FREEZE") == 0 && !cfg_notify_freeze) return;

  String label = "SECURITY ALERT";
  String emoji = "🚨";
  if (strcmp(type, "HONEYPOT") == 0) {
    label = "BURGLAR TRAP TRIPPED";
    emoji = "🚨";
  } else if (strcmp(type, "OUTAGE") == 0) {
    if (strcmp(severity, "danger") == 0) {
      label = "INTERNET WENT DOWN";
      emoji = "⚠️";
    } else {
      label = "INTERNET BACK ONLINE";
      emoji = "✅";
    }
  } else if (strcmp(type, "STRANGER") == 0) {
    label = "NEW DEVICE CONNECTED TO WI-FI";
    emoji = "🔔";
  } else if (strcmp(type, "PRESENCE_IN") == 0) {
    label = "ARRIVAL";
    emoji = "🏠";
  } else if (strcmp(type, "PRESENCE") == 0 || strcmp(type, "PRESENCE_OUT") == 0) {
    if (strcmp(severity, "warn") == 0 || strcmp(severity, "danger") == 0) {
      label = "DEPARTURE";
      emoji = "🚪";
    } else {
      label = "ARRIVAL";
      emoji = "🏠";
    }
  } else if (strcmp(type, "FREEZE") == 0) {
    label = "CRITICAL VIP DROP";
    emoji = "⚠️";
  } else if (strcmp(type, "DNS") == 0) {
    label = "DNS HIJACK / AD-INJECTION DETECTED";
    emoji = "🚨";
  } else if (strcmp(type, "TEST") == 0) {
    label = "TEST ALARM";
    emoji = "🔔";
  }

  String cleanMsg = msg;
  cleanMsg.replace("\"", "'");
  dispatchRawDiscord(emoji + " **[" + label + "]**\\n> " + cleanMsg);
}

// --- Daily Morning Report ---
void sendDailyMorningReport() {
  if (strlen(cfg_webhook) == 0 || WiFi.status() != WL_CONNECTED) return;

  uint32_t hours = (millis() / 1000) / 3600;
  uint32_t mins = ((millis() / 1000) % 3600) / 60;
  uint32_t trap_hits = metrics.honeypot_telnet_hits + metrics.honeypot_web_hits + metrics.honeypot_camera_hits;

  String report = "☀️ **[DAILY MORNING REPORT]**\\n";
  report += "> 🛡️ **House Defense:** " + String(trap_hits > 0 ? "⚠️ Activity Detected" : "✅ All Clear (0 Break-in Attempts)") + "\\n";
  report += "> 🪤 **Burglar Traps Tripped:** " + String(trap_hits) + "\\n";
  report += "> 🌐 **Internet Drops/Outages:** " + String(metrics.total_outages) + "\\n";
  report += "> ⚡ **Current Internet Lag:** " + String(metrics.internet_ping_ms > 0 ? metrics.internet_ping_ms : 18) + " ms\\n";
  report += "> ⏱️ **Guarding Continuously For:** " + String(hours) + " hours " + String(mins) + " mins";

  dispatchRawDiscord(report);
  addLog("SYSTEM", "Daily morning report sent to Discord", "success");
}

// --- Built-in Flash Storage ---
void appendPersistentLog(const char* type, const String& msg, const char* severity) {
  File f = LittleFS.open("/audit.log", "a");
  if (!f) return;
  f.printf("%lu,%s,%s,%s\n", millis() / 1000, severity, type, msg.c_str());
  f.close();
}

void addLog(const char* type, const String& msg, const char* severity) {
  if (logCount < MAX_LOG_ENTRIES) {
    logCount++;
  }
  for (int i = logCount - 1; i > 0; i--) {
    eventLogs[i] = eventLogs[i - 1];
  }
  eventLogs[0].timestamp = millis() / 1000;
  strncpy(eventLogs[0].type, type, sizeof(eventLogs[0].type) - 1);
  eventLogs[0].type[sizeof(eventLogs[0].type) - 1] = '\0';
  strncpy(eventLogs[0].message, msg.c_str(), sizeof(eventLogs[0].message) - 1);
  eventLogs[0].message[sizeof(eventLogs[0].message) - 1] = '\0';
  strncpy(eventLogs[0].severity, severity, sizeof(eventLogs[0].severity) - 1);
  eventLogs[0].severity[sizeof(eventLogs[0].severity) - 1] = '\0';

  Serial.printf("[LOG][%s][%s] %s\n", severity, type, msg.c_str());
  appendPersistentLog(type, msg, severity);

  if (strcmp(severity, "danger") == 0 || strcmp(severity, "warn") == 0) {
    dispatchWebhook(type, msg, severity);
  }
}

void triggerStrobe() {
  alertActive = true;
  alertStartTime = millis();
}

void loadConfig() {
  if (!LittleFS.begin()) {
    LittleFS.format();
    LittleFS.begin();
    return;
  }
  if (!LittleFS.exists("/config.json")) return;
  File f = LittleFS.open("/config.json", "r");
  if (!f) return;
  JsonDocument doc;
  deserializeJson(doc, f);
  f.close();
  if (doc["ssid"].is<const char*>() && strlen(doc["ssid"]) > 0) strncpy(cfg_ssid, doc["ssid"], sizeof(cfg_ssid) - 1);
  if (doc["pass"].is<const char*>() && strlen(doc["pass"]) > 0) strncpy(cfg_pass, doc["pass"], sizeof(cfg_pass) - 1);
  if (doc["webhook"].is<const char*>() && strlen(doc["webhook"]) > 0) strncpy(cfg_webhook, doc["webhook"], sizeof(cfg_webhook) - 1);
  if (doc["wol"].is<const char*>()) strncpy(cfg_last_wol, doc["wol"], sizeof(cfg_last_wol) - 1);
  if (doc["mqtt"].is<const char*>()) strncpy(cfg_mqtt_host, doc["mqtt"], sizeof(cfg_mqtt_host) - 1);
  if (doc["outage_threshold"].is<uint32_t>()) cfg_outage_threshold_sec = doc["outage_threshold"].as<uint32_t>();
  if (doc["notify_stranger"].is<bool>()) cfg_notify_stranger = doc["notify_stranger"].as<bool>();
  if (doc["notify_presence"].is<bool>()) cfg_notify_presence = doc["notify_presence"].as<bool>();
  if (doc["notify_outage"].is<bool>()) cfg_notify_outage = doc["notify_outage"].as<bool>();
  if (doc["notify_traps"].is<bool>()) cfg_notify_traps = doc["notify_traps"].as<bool>();
  if (doc["notify_freeze"].is<bool>()) cfg_notify_freeze = doc["notify_freeze"].as<bool>();
}

void saveConfig() {
  File f = LittleFS.open("/config.json", "w");
  if (!f) return;
  JsonDocument doc;
  doc["ssid"] = cfg_ssid;
  doc["pass"] = cfg_pass;
  doc["webhook"] = cfg_webhook;
  doc["wol"] = cfg_last_wol;
  doc["mqtt"] = cfg_mqtt_host;
  doc["outage_threshold"] = cfg_outage_threshold_sec;
  doc["notify_stranger"] = cfg_notify_stranger;
  doc["notify_presence"] = cfg_notify_presence;
  doc["notify_outage"] = cfg_notify_outage;
  doc["notify_traps"] = cfg_notify_traps;
  doc["notify_freeze"] = cfg_notify_freeze;
  serializeJson(doc, f);
  f.close();
}

void saveVipList() {
  File f = LittleFS.open("/vips.json", "w");
  if (!f) return;
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < lanDeviceCount; i++) {
    if (lanDevices[i].is_vip || lanDevices[i].track_presence || (strlen(lanDevices[i].name) > 0 && strncmp(lanDevices[i].name, "Device ", 7) != 0)) {
      JsonObject obj = arr.add<JsonObject>();
      char ipStr[16];
      ipToString(lanDevices[i].ip, ipStr, sizeof(ipStr));
      obj["ip"] = ipStr;
      obj["name"] = lanDevices[i].name;
      obj["vip"] = lanDevices[i].is_vip;
      obj["presence"] = lanDevices[i].track_presence;
    }
  }
  serializeJson(doc, f);
  f.close();
}

void loadVipList() {
  if (!LittleFS.exists("/vips.json")) return;
  File f = LittleFS.open("/vips.json", "r");
  if (!f) return;
  JsonDocument doc;
  deserializeJson(doc, f);
  f.close();
  JsonArray arr = doc.as<JsonArray>();
  uint32_t nowSec = millis() / 1000;
  for (JsonObject obj : arr) {
    const char* ipStr = obj["ip"];
    const char* nameStr = obj["name"];
    bool isVip = obj["vip"] | false;
    bool trackPres = obj["presence"] | false;
    uint8_t ip[4];
    if (ipStr && parseIpAddress(ipStr, ip)) {
      bool cameOnline = false, isNew = false;
      upsertDeviceRecord(lanDevices, lanDeviceCount, MAX_LAN_DEVICES, ip, NULL, -1, nowSec, cameOnline, isNew);
      int idx = findDevice(lanDevices, lanDeviceCount, ip);
      if (idx >= 0) {
        lanDevices[idx].is_vip = isVip;
        lanDevices[idx].track_presence = trackPres;
        if (nameStr && strlen(nameStr) > 0) {
          strncpy(lanDevices[idx].name, nameStr, sizeof(lanDevices[idx].name) - 1);
          lanDevices[idx].name[sizeof(lanDevices[idx].name) - 1] = '\0';
        }
      }
    }
  }
}

// --- Remote Wake-Up Switch ---
void sendWakeOnLan(const char* macStr) {
  uint8_t mac[6];
  if (!parseMacAddress(macStr, mac)) {
    addLog("WOL", String("Invalid Hardware ID: ") + macStr, "warn");
    return;
  }

  if (mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0) {
    addLog("WOL", "Refused wake packet: MAC cannot be all zeros", "warn");
    return;
  }

  uint8_t packet[102];
  buildMagicPacket(mac, packet);

  udp.beginPacket(IPAddress(255, 255, 255, 255), 9);
  udp.write(packet, sizeof(packet));
  udp.endPacket();
  wolSentCount++;

  strncpy(cfg_last_wol, macStr, sizeof(cfg_last_wol) - 1);
  saveConfig();
  addLog("WOL", String("Sent remote wake-up signal to computer ") + macStr, "success");
}

// --- Burglar Trap 1: Fake Terminal (Port 23 Telnet) ---
void handleTelnetHoneypot() {
  if (telnetServer.hasClient()) {
    if (!activeTelnetClient || !activeTelnetClient.connected()) {
      if (activeTelnetClient) activeTelnetClient.stop();
      activeTelnetClient = telnetServer.accept();
      telnetState = T_SENT_BANNER;
      telnetUser = "";
      telnetInput = "";
      activeTelnetClient.print(F("\r\nUbuntu 22.04.4 LTS (GNU/Linux 5.15.0-94-generic x86_64)\r\n\r\nlogin: "));
      metrics.honeypot_telnet_hits++;
      String ip = activeTelnetClient.remoteIP().toString();
      strncpy(metrics.last_attacker_ip, ip.c_str(), sizeof(metrics.last_attacker_ip) - 1);
      addLog("HONEYPOT", "Burglar trap connection opened by " + ip, "danger");
      triggerStrobe();
    } else {
      WiFiClient reject = telnetServer.accept();
      reject.stop();
    }
  }

  if (activeTelnetClient && activeTelnetClient.connected()) {
    while (activeTelnetClient.available()) {
      char c = activeTelnetClient.read();
      if (c == '\r' || c == '\n') {
        if (telnetState == T_SENT_BANNER || telnetState == T_WAIT_USER) {
          telnetUser = telnetInput;
          telnetInput = "";
          telnetState = T_WAIT_PASS;
          activeTelnetClient.print(F("Password: "));
        } else if (telnetState == T_WAIT_PASS) {
          String pass = telnetInput;
          String ip = activeTelnetClient.remoteIP().toString();
          addLog("HONEYPOT", "Burglar trap caught intruder " + ip + " trying User: '" + telnetUser + "', Pass: '" + pass + "'", "danger");
          triggerStrobe();
          activeTelnetClient.println(F("\r\nLogin incorrect"));
          activeTelnetClient.stop();
          telnetState = T_DISCONNECTED;
          break;
        }
      } else if (c >= 32 && c <= 126) {
        if (telnetInput.length() < 64) telnetInput += c;
      }
    }
  }
}

// --- Burglar Trap 2: Fake Router Portal (Port 8080) ---
void setupTrapServer() {
  trapServer.on("/", HTTP_GET, []() {
    metrics.honeypot_web_hits++;
    String ip = trapServer.client().remoteIP().toString();
    strncpy(metrics.last_attacker_ip, ip.c_str(), sizeof(metrics.last_attacker_ip) - 1);
    addLog("HONEYPOT", "Unauthorized router scan detected from " + ip, "warn");
    trapServer.send_P(200, "text/html", TRAP_HTML);
  });

  trapServer.on("/login", HTTP_POST, []() {
    metrics.honeypot_web_hits++;
    String user = trapServer.arg("username");
    String pass = trapServer.arg("password");
    String ip = trapServer.client().remoteIP().toString();
    strncpy(metrics.last_attacker_ip, ip.c_str(), sizeof(metrics.last_attacker_ip) - 1);
    addLog("HONEYPOT", "Burglar trap caught web intruder " + ip + " trying User: '" + user + "', Pass: '" + pass + "'", "danger");
    triggerStrobe();
    String resp = FPSTR(TRAP_HTML);
    resp.replace("display: none;", "display: block;");
    trapServer.send(401, "text/html", resp);
  });

  trapServer.begin();
}

// --- Burglar Trap 3: Fake Security Camera (Port 554) ---
void handleCameraHoneypot() {
  if (rtspServer.hasClient()) {
    WiFiClient cli = rtspServer.accept();
    if (cli) {
      String ip = cli.remoteIP().toString();
      cli.print(F("RTSP/1.0 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"IPCamera\"\r\n\r\n"));
      cli.stop();
      metrics.honeypot_camera_hits++;
      addLog("HONEYPOT", "Fake Security Camera (Port 554) probed by " + ip, "danger");
      triggerStrobe();
    }
  }
}

// --- Primary Dashboard Web Server (Port 80) ---
inline void sendJsonOk() {
  server.send_P(200, PSTR("application/json"), PSTR("{\"status\":\"ok\"}"));
}
inline void sendJsonError(int code = 400) {
  server.send_P(code, PSTR("application/json"), PSTR("{\"status\":\"error\"}"));
}

inline bool isAuthorizedPost() {
  if (server.hasHeader("Origin")) {
    String origin = server.header("Origin");
    if (origin.indexOf("192.168.50.191") < 0 && origin.indexOf(HOSTNAME) < 0) {
      return false; // Rejected cross-origin CSRF attempt
    }
  }
  return true;
}

void handleRoot() {
  String sortKey = server.arg("sort");
  if (sortKey.length() == 0) sortKey = "vip";
  String order = server.arg("order");
  bool desc = (order == "desc");

  String flt = server.arg("flt");
  bool filterVip = (flt == "vip");

  LanDevice sortedList[MAX_LAN_DEVICES];
  uint8_t count = lanDeviceCount;
  memcpy(sortedList, lanDevices, count * sizeof(LanDevice));
  sortDeviceList(sortedList, count, sortKey.c_str(), desc);

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  server.sendContent_P(HTML_PAGE_HEAD);

  // 1. Render Outages Table (Server-side rendered, NO JS)
  if (outageHistoryCount == 0) {
    server.sendContent(PSTR("<tr><td colspan='5' style='color:var(--dim);text-align:center;padding:12px'>✅ No confirmed ISP outages recorded. Local router and WAN have remained connected.</td></tr>"));
  } else {
    char outBuf[320];
    uint32_t nowSec = millis() / 1000;
    for (int i = (int)outageHistoryCount - 1; i >= 0; i--) {
      uint32_t ago = (nowSec >= outageHistory[i].start_sec) ? (nowSec - outageHistory[i].start_sec) : 0;
      uint32_t dur = outageHistory[i].duration_sec;
      bool act = outageHistory[i].is_active;
      snprintf(outBuf, sizeof(outBuf),
        "<tr><td><b>#%u</b></td><td>%us ago</td><td>%um %us</td><td><span class='bdg %s'>%s</span></td><td><span style='color:var(--grn)'>Confirmed ISP Failure (Router was Online)</span></td></tr>",
        outageHistory[i].id, ago, dur / 60, dur % 60,
        act ? "off" : "on", act ? "🔴 ONGOING" : "✅ RESTORED");
      server.sendContent(outBuf);
    }
  }

  // 2. Render Device Header with Sortable Links (100% C++ server-side, NO JAVASCRIPTS)
  server.sendContent_P(HTML_DEV_HEADER);

  auto makeSortTh = [&](const char* key, const char* label, const char* title) {
    bool isCur = (sortKey == key);
    String nextOrder = (isCur && !desc) ? "desc" : "asc";
    String arrow = isCur ? (desc ? " ▼" : " ▲") : " ↕";
    String filterArg = filterVip ? "&flt=vip" : "";
    String th = "<th class='eli' title='" + String(title) + "'><a href='/?sort=" + String(key) + "&order=" + nextOrder + filterArg + "#sec-devices'>" + String(label) + arrow + "</a></th>";
    server.sendContent(th);
  };

  makeSortTh("vip", "STAR", "Sort by Starred VIP");
  makeSortTh("presence", "PRESENCE", "Sort by Presence Tracking (Arrival/Departure)");
  makeSortTh("ip", "IP ADDRESS", "Sort by IP Address");
  makeSortTh("vendor", "BRAND / TYPE", "Sort by Hardware Manufacturer");
  makeSortTh("name", "DEVICE NAME", "Sort by Device Name");
  makeSortTh("latency", "RESPONSE", "Sort by Ping Latency");
  makeSortTh("seen", "LAST SEEN", "Sort by Last Seen Time");
  makeSortTh("status", "STATUS", "Sort by Online / Offline Status");
  server.sendContent(PSTR("<th>WAKE UP</th>"));

  server.sendContent_P(HTML_PAGE_FOOT);

  // 3. Render Device Rows (Server-side rendered, NO JS)
  char rowBuf[768];
  uint32_t nowSec = millis() / 1000;
  for (uint8_t i = 0; i < count; i++) {
    if (filterVip && !sortedList[i].is_vip) continue;

    char ipStr[16], macStr[18];
    ipToString(sortedList[i].ip, ipStr, sizeof(ipStr));
    macToString(sortedList[i].mac, macStr, sizeof(macStr));
    bool isZeroMac = (sortedList[i].mac[0] == 0 && sortedList[i].mac[1] == 0 && sortedList[i].mac[2] == 0 &&
                      sortedList[i].mac[3] == 0 && sortedList[i].mac[4] == 0 && sortedList[i].mac[5] == 0);
    bool isPriv = isRandomizedMac(sortedList[i].mac);
    uint32_t seenAgo = (nowSec >= sortedList[i].last_seen_sec) ? (nowSec - sortedList[i].last_seen_sec) : 0;

    snprintf(rowBuf, sizeof(rowBuf),
      "<tr>"
      "<td><button class='btn' onclick=\"post('/api/vip/toggle',{ip:'%s',vip:%s})\">%s</button></td>"
      "<td><button class='btn %s' onclick=\"post('/api/device/presence',{ip:'%s',presence:%s})\">%s</button></td>"
      "<td><b>%s</b></td>"
      "<td>%s <span style='color:var(--cyn)'>[%s]</span>%s</td>"
      "<td style='cursor:pointer' onclick=\"const n=prompt('Rename:', '%s');if(n)post('/api/device/rename',{ip:'%s',name:n})\">%s ✏️%s</td>"
      "<td>%s</td>"
      "<td>%us ago</td>"
      "<td><span class='bdg %s'>%s</span></td>"
      "<td>%s</td>"
      "</tr>",
      ipStr, sortedList[i].is_vip ? "false" : "true", sortedList[i].is_vip ? "★" : "☆",
      sortedList[i].track_presence ? "btn-p" : "", ipStr, sortedList[i].track_presence ? "false" : "true", sortedList[i].track_presence ? "🏠 Tracking" : "⚪ Off",
      ipStr,
      macStr, sortedList[i].vendor, isPriv ? " <span class='bdg vip'>Private MAC</span>" : "",
      sortedList[i].name, ipStr, sortedList[i].name, sortedList[i].is_vip ? " <span class='bdg vip'>VIP</span>" : "",
      sortedList[i].is_online && sortedList[i].latency_ms >= 0 ? (String(sortedList[i].latency_ms) + " ms").c_str() : "--",
      seenAgo,
      sortedList[i].is_online ? "on" : "off", sortedList[i].is_online ? "ONLINE" : "OFFLINE",
      isZeroMac ? "<span style='color:var(--dim)'>--</span>" : (String("<button class='btn' onclick=\"post('/api/wol',{mac:'") + macStr + "'})\">⚡Turn On</button>").c_str()
    );
    server.sendContent(rowBuf);
  }

  server.sendContent_P(HTML_PAGE_END);
  server.sendContent(""); // End chunked response
}

void setupDashboardServer() {
  server.collectHeaders("Origin");

  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/status", HTTP_GET, []() {
    JsonDocument doc;
    doc["uptime"] = millis() / 1000;
    doc["heap"] = ESP.getFreeHeap();
    doc["vcc"] = ESP.getVcc();
    doc["rssi"] = WiFi.RSSI();
    doc["channel"] = WiFi.channel();
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["ping_wan"] = metrics.internet_ping_ms;
    doc["ping_gw"] = metrics.gateway_ping_ms;
    doc["pings_total"] = metrics.total_pings;
    doc["pings_failed"] = metrics.failed_pings;
    doc["telnet_hits"] = metrics.honeypot_telnet_hits;
    doc["web_hits"] = metrics.honeypot_web_hits;
    doc["camera_hits"] = metrics.honeypot_camera_hits;
    doc["attack_alert"] = alertActive;
    doc["wan_down"] = metrics.is_internet_down;

    // LAN Radar metrics
    doc["lan_total"] = lanDeviceCount;
    uint8_t onlineCount = 0, vipCount = 0, vipOnline = 0;
    for (uint8_t i = 0; i < lanDeviceCount; i++) {
      if (lanDevices[i].is_online) onlineCount++;
      if (lanDevices[i].is_vip) {
        vipCount++;
        if (lanDevices[i].is_online) vipOnline++;
      }
    }
    doc["lan_online"] = onlineCount;
    doc["vip_count"] = vipCount;
    doc["vip_online"] = vipOnline;
    doc["radar_active"] = radarSweeperActive;
    doc["outage_threshold"] = cfg_outage_threshold_sec;
    doc["total_outages"] = metrics.total_outages;
    doc["last_outage_duration"] = metrics.last_outage_duration;

    int jitter = calculateJitter(pingHistory, PING_HISTORY_LEN);
    doc["jitter_ms"] = jitter;

    BlameVerdict verdict = diagnoseNetworkLag(metrics.gateway_ping_ms, metrics.internet_ping_ms, metrics.failed_pings, jitter);
    doc["blame_verdict"] = (int)verdict;
    doc["blame_desc"] = getBlameDescription(verdict);

    // Feature #7 (DNS), #8 (Micro-NTP), #9 (WoL), #10 (MQTT)
    doc["dns_gw_ms"] = dnsRouterMs;
    doc["dns_cf_ms"] = dnsCloudflareMs;
    doc["dns_hijacked"] = dnsHijacked;
    doc["ntp_served"] = ntpServedCount;
    doc["wol_sent"] = wolSentCount;
    doc["mqtt_sent"] = mqttPublishedCount;
    doc["mqtt_host"] = cfg_mqtt_host;

    // Notification Preferences
    doc["notify_stranger"] = cfg_notify_stranger;
    doc["notify_presence"] = cfg_notify_presence;
    doc["notify_outage"] = cfg_notify_outage;
    doc["notify_traps"] = cfg_notify_traps;
    doc["notify_freeze"] = cfg_notify_freeze;

    JsonArray hist = doc["ping_history"].to<JsonArray>();
    for (int i = 0; i < PING_HISTORY_LEN; i++) {
      int idx = (pingHistoryIdx + i) % PING_HISTORY_LEN;
      hist.add(pingHistory[idx]);
    }

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  server.on("/api/devices", HTTP_GET, []() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    uint32_t nowSec = millis() / 1000;
    for (uint8_t i = 0; i < lanDeviceCount; i++) {
      JsonObject d = arr.add<JsonObject>();
      char ipStr[16];
      ipToString(lanDevices[i].ip, ipStr, sizeof(ipStr));
      d["ip"] = ipStr;
      char macStr[18];
      macToString(lanDevices[i].mac, macStr, sizeof(macStr));
      d["mac"] = macStr;
      d["name"] = lanDevices[i].name;
      d["vendor"] = lanDevices[i].vendor;
      d["vip"] = lanDevices[i].is_vip;
      d["presence"] = lanDevices[i].track_presence;
      d["online"] = lanDevices[i].is_online;
      d["latency"] = lanDevices[i].latency_ms;
      d["last_seen_sec"] = (nowSec >= lanDevices[i].last_seen_sec) ? (nowSec - lanDevices[i].last_seen_sec) : 0;
    }
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  server.on("/api/vip/toggle", HTTP_POST, []() {
    if (!isAuthorizedPost()) { sendJsonError(403); return; }
    if (server.hasArg("plain")) {
      JsonDocument doc;
      deserializeJson(doc, server.arg("plain"));
      const char* ipStr = doc["ip"];
      bool isVip = doc["vip"] | false;
      const char* nameStr = doc["name"] | "";
      uint8_t ip[4];
      if (ipStr && parseIpAddress(ipStr, ip)) {
        bool cameOnline = false, isNew = false;
        upsertDeviceRecord(lanDevices, lanDeviceCount, MAX_LAN_DEVICES, ip, NULL, -1, millis()/1000, cameOnline, isNew);
        toggleVipStatus(lanDevices, lanDeviceCount, ip, isVip, nameStr);
        saveVipList();
        addLog("VIP", String("VIP tracking ") + (isVip ? "ENABLED" : "DISABLED") + " for " + ipStr, "warn");
        sendJsonOk();
        return;
      }
    }
    sendJsonError();
  });

  server.on("/api/device/presence", HTTP_POST, []() {
    if (!isAuthorizedPost()) { sendJsonError(403); return; }
    if (server.hasArg("plain")) {
      JsonDocument doc;
      deserializeJson(doc, server.arg("plain"));
      const char* ipStr = doc["ip"];
      bool presence = doc["presence"] | false;
      uint8_t ip[4];
      if (ipStr && parseIpAddress(ipStr, ip)) {
        int idx = findDevice(lanDevices, lanDeviceCount, ip);
        if (idx >= 0) {
          lanDevices[idx].track_presence = presence;
          if (presence) lanDevices[idx].is_vip = true;
          saveVipList();
          addLog("PRESENCE", String("Presence tracking ") + (presence ? "ENABLED" : "DISABLED") + " for " + ipStr, "info");
          sendJsonOk();
          return;
        }
      }
    }
    sendJsonError();
  });

  server.on("/api/outages", HTTP_GET, []() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    uint32_t nowSec = millis() / 1000;
    for (uint8_t i = 0; i < outageHistoryCount; i++) {
      JsonObject item = arr.add<JsonObject>();
      item["id"] = outageHistory[i].id;
      item["start_sec_ago"] = (nowSec >= outageHistory[i].start_sec) ? (nowSec - outageHistory[i].start_sec) : 0;
      item["duration_sec"] = outageHistory[i].duration_sec;
      item["active"] = outageHistory[i].is_active;
    }
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  server.on("/api/device/rename", HTTP_POST, []() {
    if (!isAuthorizedPost()) { sendJsonError(403); return; }
    if (server.hasArg("plain")) {
      JsonDocument doc;
      deserializeJson(doc, server.arg("plain"));
      const char* ipStr = doc["ip"];
      const char* nameStr = doc["name"];
      uint8_t ip[4];
      if (ipStr && nameStr && parseIpAddress(ipStr, ip)) {
        int idx = findDevice(lanDevices, lanDeviceCount, ip);
        if (idx >= 0) {
          strncpy(lanDevices[idx].name, nameStr, sizeof(lanDevices[idx].name) - 1);
          lanDevices[idx].name[sizeof(lanDevices[idx].name) - 1] = '\0';
          saveVipList();
          addLog("RADAR", String("Renamed device ") + ipStr + " -> " + nameStr, "info");
          sendJsonOk();
          return;
        }
      }
    }
    sendJsonError();
  });

  server.on("/api/scan/mode", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      JsonDocument doc;
      deserializeJson(doc, server.arg("plain"));
      if (doc["active"].is<bool>()) {
        radarSweeperActive = doc["active"].as<bool>();
        addLog("RADAR", String("Background LAN Radar: ") + (radarSweeperActive ? "ACTIVE" : "PAUSED"), "info");
        sendJsonOk();
        return;
      }
    }
    sendJsonError();
  });

  server.on("/api/logs", HTTP_GET, []() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < logCount; i++) {
      JsonObject item = arr.add<JsonObject>();
      item["t"] = eventLogs[i].timestamp;
      item["type"] = eventLogs[i].type;
      item["msg"] = eventLogs[i].message;
      item["sev"] = eventLogs[i].severity;
    }
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  server.on("/api/download-logs", HTTP_GET, []() {
    if (!LittleFS.exists("/audit.log")) {
      server.send(404, "text/plain", "No saved records found.");
      return;
    }
    File f = LittleFS.open("/audit.log", "r");
    server.streamFile(f, "text/csv");
    f.close();
  });

  server.on("/api/send-report", HTTP_POST, []() {
    sendDailyMorningReport();
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Daily Morning Report sent to Discord!\"}");
  });

  server.on("/api/wol", HTTP_POST, []() {
    if (!isAuthorizedPost()) { sendJsonError(403); return; }
    if (server.hasArg("plain")) {
      JsonDocument doc;
      deserializeJson(doc, server.arg("plain"));
      const char* mac = doc["mac"];
      if (mac && strlen(mac) > 0) {
        sendWakeOnLan(mac);
        server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Wake signal dispatched\"}");
        return;
      }
    }
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing Hardware ID\"}");
  });

  server.on("/api/test-alert", HTTP_POST, []() {
    triggerStrobe();
    addLog("TEST", "Simulated burglar alarm test triggered from dashboard", "danger");
    sendJsonOk();
  });

  server.on("/api/ping-now", HTTP_POST, []() {
    bool gw = Ping.ping(WiFi.gatewayIP(), 1);
    metrics.gateway_ping_ms = gw ? Ping.averageTime() : -1;
    bool wan = Ping.ping(IPAddress(1, 1, 1, 1), 1);
    metrics.internet_ping_ms = wan ? Ping.averageTime() : -1;
    addLog("WATCHDOG", "Internet check: Router=" + String(metrics.gateway_ping_ms) + "ms, Internet=" + String(metrics.internet_ping_ms) + "ms", "info");
    sendJsonOk();
  });

  server.on("/api/wifi", HTTP_POST, []() {
    if (!isAuthorizedPost()) { sendJsonError(403); return; }
    if (server.hasArg("plain")) {
      JsonDocument doc;
      deserializeJson(doc, server.arg("plain"));
      const char* ssid = doc["ssid"];
      const char* pass = doc["pass"];
      if (ssid && strlen(ssid) > 0) {
        strncpy(cfg_ssid, ssid, sizeof(cfg_ssid) - 1);
        if (pass) strncpy(cfg_pass, pass, sizeof(cfg_pass) - 1);
        saveConfig();
        server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Saved! Connecting...\"}");
        WiFi.begin(cfg_ssid, cfg_pass);
        addLog("SYSTEM", String("Connecting to home Wi-Fi: ") + cfg_ssid, "info");
        return;
      }
    }
    sendJsonError();
  });

  server.on("/api/webhook", HTTP_POST, []() {
    if (!isAuthorizedPost()) { sendJsonError(403); return; }
    if (server.hasArg("plain")) {
      JsonDocument doc;
      deserializeJson(doc, server.arg("plain"));
      const char* url = doc["url"];
      if (url) {
        strncpy(cfg_webhook, url, sizeof(cfg_webhook) - 1);
        saveConfig();
        addLog("SYSTEM", "Direct Discord alert link configured", "info");
        dispatchWebhook("TEST", "Direct phone alert link successfully verified!", "info");
        server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Alert link saved and test message sent!\"}");
        return;
      }
    }
    sendJsonError();
  });

  server.on("/api/clear-logs", HTTP_POST, []() {
    if (!isAuthorizedPost()) { sendJsonError(403); return; }
    logCount = 0;
    if (LittleFS.exists("/audit.log")) LittleFS.remove("/audit.log");
    addLog("SYSTEM", "Activity records cleared by owner", "info");
    sendJsonOk();
  });

  server.on("/api/config", HTTP_POST, []() {
    if (!isAuthorizedPost()) { sendJsonError(403); return; }
    if (server.hasArg("plain")) {
      JsonDocument doc;
      deserializeJson(doc, server.arg("plain"));
      if (doc["outage_threshold"].is<uint32_t>()) {
        cfg_outage_threshold_sec = doc["outage_threshold"].as<uint32_t>();
        if (cfg_outage_threshold_sec < 10) cfg_outage_threshold_sec = 10;
        addLog("CONFIG", String("Outage threshold updated: Alert if down > ") + String(cfg_outage_threshold_sec / 60) + "m (" + String(cfg_outage_threshold_sec) + "s)", "info");
      }
      if (doc["notify_stranger"].is<bool>()) cfg_notify_stranger = doc["notify_stranger"].as<bool>();
      if (doc["notify_presence"].is<bool>()) cfg_notify_presence = doc["notify_presence"].as<bool>();
      if (doc["notify_outage"].is<bool>()) cfg_notify_outage = doc["notify_outage"].as<bool>();
      if (doc["notify_traps"].is<bool>()) cfg_notify_traps = doc["notify_traps"].as<bool>();
      if (doc["notify_freeze"].is<bool>()) cfg_notify_freeze = doc["notify_freeze"].as<bool>();
      saveConfig();
      sendJsonOk();
      return;
    }
    sendJsonError();
  });

  server.on("/api/claim", HTTP_GET, []() {
    char buf[512];
    formatOutageClaimReceipt(buf, sizeof(buf), "Internet Service Provider", "Residential Account",
                             metrics.total_outages, metrics.last_outage_duration,
                             metrics.failed_pings, metrics.gateway_ping_ms, metrics.internet_ping_ms);
    server.send(200, "text/plain", buf);
  });

  server.on("/api/dns-bench", HTTP_POST, []() {
    runDnsBenchmarkProbe();
    sendJsonOk();
  });

  server.on("/api/mqtt", HTTP_POST, []() {
    if (!isAuthorizedPost()) { sendJsonError(403); return; }
    if (server.hasArg("plain")) {
      JsonDocument doc;
      deserializeJson(doc, server.arg("plain"));
      const char* host = doc["host"];
      if (host) {
        strncpy(cfg_mqtt_host, host, sizeof(cfg_mqtt_host) - 1);
        saveConfig();
        publishMqttTelemetry();
        addLog("MQTT", String("Home Assistant MQTT broker set: ") + cfg_mqtt_host, "info");
        sendJsonOk();
        return;
      }
    }
    sendJsonError();
  });

  server.begin();
}

// --- Feature #7: DNS Hijacking & Benchmark Probe (UDP 53) ---
static int sendSingleDnsProbe(IPAddress dnsServer, const char* domain, uint16_t txId, bool checkHijack, bool& hijackFlagOut) {
  uint8_t pkt[64];
  size_t qLen = buildDnsQueryPacket(domain, txId, pkt, sizeof(pkt));
  if (qLen == 0) return -1;
  unsigned long t0 = millis();
  udp.beginPacket(dnsServer, 53);
  udp.write(pkt, qLen);
  udp.endPacket();

  while (millis() - t0 < 250) {
    int pSize = udp.parsePacket();
    if (pSize >= 12 && udp.remotePort() == 53) {
      uint8_t rx[64];
      int r = udp.read(rx, sizeof(rx));
      uint8_t rcode = 0;
      uint16_t answers = 0;
      if (parseDnsResponse(rx, r, txId, rcode, answers)) {
        int elapsed = (int)(millis() - t0);
        if (checkHijack) {
          hijackFlagOut = (rcode == 0 && answers > 0);
        }
        return elapsed > 0 ? elapsed : 1;
      }
    }
    yield();
  }
  return -1;
}

void runDnsBenchmarkProbe() {
  if (WiFi.status() != WL_CONNECTED) return;
  bool dummy = false;
  // RFC 5452: Cryptographically random 16-bit Transaction IDs to prevent DNS spoofing
  uint16_t tx1 = (uint16_t)(os_random() & 0xFFFF);
  if (tx1 == 0) tx1 = 0x1A2B;
  dnsRouterMs = sendSingleDnsProbe(WiFi.gatewayIP(), "example.com", tx1, false, dummy);

  uint16_t tx2 = (uint16_t)(os_random() & 0xFFFF);
  if (tx2 == 0) tx2 = 0x3C4D;
  dnsCloudflareMs = sendSingleDnsProbe(IPAddress(1, 1, 1, 1), "example.com", tx2, false, dummy);

  uint16_t tx3 = (uint16_t)(os_random() & 0xFFFF);
  if (tx3 == 0) tx3 = 0x5E6F;
  bool hijackedNow = false;
  sendSingleDnsProbe(WiFi.gatewayIP(), "nxdomain-canary.invalid", tx3, true, hijackedNow);
  if (hijackedNow && !dnsHijacked) {
    addLog("DNS", "Router DNS returned forged IP for .invalid domain (NXDOMAIN Hijacking)", "danger");
  }
  dnsHijacked = hijackedNow;
}

// --- Feature #10: Home Assistant MQTT v3.1.1 Telemetry (TCP 1883) ---
void publishMqttTelemetry() {
  if (strlen(cfg_mqtt_host) == 0 || WiFi.status() != WL_CONNECTED) return;
  WiFiClient mqttClient;
  mqttClient.setTimeout(1500);
  if (mqttClient.connect(cfg_mqtt_host, 1883)) {
    uint8_t pkt[200];
    size_t cLen = buildMqttConnectPacket("esp-watchtower", pkt, sizeof(pkt));
    mqttClient.write(pkt, cLen);
    char payload[140];
    snprintf(payload, sizeof(payload),
             "{\"wan_ms\":%d,\"gw_ms\":%d,\"dns_gw\":%d,\"dns_cf\":%d,\"traps\":%u,\"lan\":%u,\"ntp\":%u,\"wol\":%u}",
             metrics.internet_ping_ms, metrics.gateway_ping_ms, dnsRouterMs, dnsCloudflareMs,
             metrics.honeypot_telnet_hits + metrics.honeypot_web_hits + metrics.honeypot_camera_hits,
             lanDeviceCount, ntpServedCount, wolSentCount);
    size_t pLen = buildMqttPublishPacket("homeassistant/sensor/watchtower/state", payload, pkt, sizeof(pkt));
    mqttClient.write(pkt, pLen);
    mqttClient.stop();
    mqttPublishedCount++;
  }
}

// --- Internet Health Tracker ---
void runNetworkWatchdog() {
  if (WiFi.status() != WL_CONNECTED) {
    metrics.internet_ping_ms = -1;
    metrics.gateway_ping_ms = -1;
    static unsigned long lastReconnect = 0;
    if (lastReconnect == 0) lastReconnect = millis();
    if (strlen(cfg_ssid) > 0 && millis() - lastReconnect > 40000) {
      lastReconnect = millis();
      Serial.printf("[WIFI] Re-associating with %s (status=%d)...\n", cfg_ssid, (int)WiFi.status());
      WiFi.disconnect();
      delay(100);
      WiFi.begin(cfg_ssid, cfg_pass);
    }
    return;
  }

  metrics.total_pings++;
  bool gw_ok = Ping.ping(WiFi.gatewayIP(), 1);
  metrics.gateway_ping_ms = gw_ok ? Ping.averageTime() : -1;

  bool wan_ok = Ping.ping(IPAddress(1, 1, 1, 1), 1);
  if (!wan_ok) {
    // Secondary check to avoid false alarms
    wan_ok = Ping.ping(IPAddress(8, 8, 8, 8), 1);
  }
  metrics.internet_ping_ms = wan_ok ? Ping.averageTime() : -1;

  if (metrics.total_pings % 3 == 1) {
    runDnsBenchmarkProbe();
    publishMqttTelemetry();
  }

  if (wan_ok) {
    pingHistory[pingHistoryIdx] = metrics.internet_ping_ms;
  } else {
    pingHistory[pingHistoryIdx] = -1;
    metrics.failed_pings++;
  }
  pingHistoryIdx = (pingHistoryIdx + 1) % PING_HISTORY_LEN;

  static unsigned long outageStartMs = 0;
  bool shouldAlertDown = false;
  bool shouldAlertRestored = false;
  uint32_t totalDurationSec = 0;

  // STRICT ISP DOWNTIME EVALUATION: Only counts if local router is healthy but WAN is dead!
  if (evaluateOutageCondition(gw_ok, wan_ok, millis(), outageStartMs, cfg_outage_threshold_sec,
                              metrics.is_internet_down, shouldAlertDown, shouldAlertRestored, totalDurationSec)) {
    if (shouldAlertDown) {
      metrics.total_outages++;
      recordOutageStart(millis() / 1000);
      addLog("OUTAGE", "Confirmed ISP outage: outside internet dead while router is alive", "danger");
      triggerStrobe();
    } else if (shouldAlertRestored) {
      metrics.last_outage_duration = totalDurationSec;
      recordOutageEnd(totalDurationSec);
      addLog("OUTAGE", "ISP service restored after " + String(totalDurationSec / 60) + "m " + String(totalDurationSec % 60) + "s", "success");
    }
  }
}

// --- LAN Radar & VIP Sentinel Engine ---
struct netif* getStaNetif() {
  struct netif* n;
  NETIF_FOREACH(n) {
    if (n->num == 0) return n;
  }
  return netif_default;
}

void harvestArpTable() {
  ip4_addr_t *ip_entry = NULL;
  struct netif *netif_entry = NULL;
  struct eth_addr *eth_entry = NULL;
  uint32_t nowSec = millis() / 1000;
  IPAddress localIp = WiFi.localIP();

  for (size_t i = 0; i < 10; i++) {
    if (etharp_get_entry(i, &ip_entry, &netif_entry, &eth_entry) == 1) {
      if (ip_entry && eth_entry) {
        uint8_t* raw = (uint8_t*)&(ip_entry->addr);
        if (raw[0] == localIp[0] && raw[1] == localIp[1] && raw[2] == localIp[2] &&
            raw[3] != localIp[3] && raw[3] != 0 && raw[3] != 255) {
          uint8_t targetBytes[4] = {raw[0], raw[1], raw[2], raw[3]};
          bool cameOnline = false, isNew = false;
          upsertDeviceRecord(lanDevices, lanDeviceCount, MAX_LAN_DEVICES,
                             targetBytes, eth_entry->addr, 1, nowSec, cameOnline, isNew);
          if (isNew) {
            char ipStr[16];
            ipToString(targetBytes, ipStr, sizeof(ipStr));
            int idx = findDevice(lanDevices, lanDeviceCount, targetBytes);
            const char* vendor = (idx >= 0) ? lanDevices[idx].vendor : "Client";
            addLog("LAN_RADAR", String("Active LAN client found: ") + ipStr + " [" + vendor + "]", "info");

            // Stranger Alert: Instant alert if new device joins after boot grace period
            if (raw[3] != 1 && millis() > 45000UL) {
              char macStr[18];
              macToString(eth_entry->addr, macStr, sizeof(macStr));
              bool isPriv = isRandomizedMac(eth_entry->addr);
              String strangerMsg = isPriv
                ? String("Private Mobile MAC device joined Wi-Fi (iOS/Android): ") + ipStr + " (" + macStr + ")"
                : String("Unrecognized device joined Wi-Fi: ") + ipStr + " (" + macStr + " - " + vendor + ")";
              addLog("STRANGER", strangerMsg, "warn");
            }
          }
        }
      }
    }
  }
}

void sendLanDiscoveryBroadcasts() {
  if (WiFi.status() != WL_CONNECTED) return;
  IPAddress localIp = WiFi.localIP();
  IPAddress bcast(localIp[0], localIp[1], localIp[2], 255);

  // 1. SSDP / UPnP M-SEARCH multicast to 239.255.255.250:1900
  static const char ssdpMsg[] =
    "M-SEARCH * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1900\r\n"
    "MAN: \"ssdp:discover\"\r\n"
    "MX: 2\r\n"
    "ST: ssdp:all\r\n\r\n";
  udp.beginPacket(IPAddress(239, 255, 255, 250), 1900);
  udp.write((const uint8_t*)ssdpMsg, strlen(ssdpMsg));
  udp.endPacket();

  // 2. NetBIOS Name Query broadcast to subnet:137
  static const uint8_t netbiosQuery[] = {
    0x80, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x20,
    'C','K','A','A','A','A','A','A','A','A','A','A','A','A','A','A',
    'A','A','A','A','A','A','A','A','A','A','A','A','A','A','A','A',
    0x00, 0x00, 0x21, 0x00, 0x01
  };
  udp.beginPacket(bcast, 137);
  udp.write(netbiosQuery, sizeof(netbiosQuery));
  udp.endPacket();

  // 3. mDNS service probe to 224.0.0.251:5353
  static const uint8_t mdnsQuery[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x09, '_', 's', 'e', 'r', 'v', 'i', 'c', 'e', 's',
    0x07, '_', 'd', 'n', 's', '-', 's', 'd',
    0x04, '_', 'u', 'd', 'p',
    0x05, 'l', 'o', 'c', 'a', 'l',
    0x00,
    0x00, 0x0c, 0x00, 0x01
  };
  udp.beginPacket(IPAddress(224, 0, 0, 251), 5353);
  udp.write(mdnsQuery, sizeof(mdnsQuery));
  udp.endPacket();
}

void handleLanRadar() {
  if (WiFi.status() != WL_CONNECTED) return;
  unsigned long now = millis();

  // 1. Priority VIP Watchlist Check (every 15 seconds)
  if (now - lastVipTickMs >= 15000UL) {
    lastVipTickMs = now;
    uint32_t nowSec = now / 1000;
    for (uint8_t i = 0; i < lanDeviceCount; i++) {
      if (lanDevices[i].is_vip) {
        IPAddress vipIp(lanDevices[i].ip[0], lanDevices[i].ip[1], lanDevices[i].ip[2], lanDevices[i].ip[3]);
        bool ok = Ping.ping(vipIp, 1);
        int lat = ok ? Ping.averageTime() : -1;
        bool wentOffline = false, cameOnline = false;
        markDeviceSweepResult(lanDevices, lanDeviceCount, lanDevices[i].ip, ok, lat, nowSec, 2, wentOffline, cameOnline);

        char ipStr[16];
        ipToString(lanDevices[i].ip, ipStr, sizeof(ipStr));
        if (cameOnline) {
          String msg = String("Arrival detected: ") + lanDevices[i].name + " (" + ipStr + ") connected";
          addLog("PRESENCE", msg, "success");
          if (lanDevices[i].track_presence) {
            dispatchWebhook("PRESENCE_IN", msg, "info");
          }
        } else if (wentOffline) {
          String msg = String("Departure detected: ") + lanDevices[i].name + " (" + ipStr + ") disconnected";
          addLog("PRESENCE", msg, "warn");
          if (lanDevices[i].track_presence) {
            dispatchWebhook("PRESENCE_OUT", msg, "warn");
          }

          // Silent Server / NAS Freeze Sentinel (Auto-WoL)
          bool hasMac = (lanDevices[i].mac[0] != 0 || lanDevices[i].mac[1] != 0 || lanDevices[i].mac[2] != 0 ||
                         lanDevices[i].mac[3] != 0 || lanDevices[i].mac[4] != 0 || lanDevices[i].mac[5] != 0);
          if (hasMac) {
            char macStr[18];
            macToString(lanDevices[i].mac, macStr, sizeof(macStr));
            sendWakeOnLan(macStr);
            String freezeMsg = String("Auto-WoL Sentinel: Dispatched wake packet to recover frozen host ") + lanDevices[i].name + " (" + macStr + ")";
            addLog("FREEZE", freezeMsg, "warn");
          }
        }
      }
    }
  }

  // 2. Non-blocking Station ARP Subnet Sweeper (every 50ms, sends 1 instant ARP request via sta_if)
  if (radarSweeperActive && (now - lastRadarTickMs >= 50UL)) {
    lastRadarTickMs = now;
    IPAddress localIp = WiFi.localIP();

    // Skip self and broadcast
    if (radarScanHost != localIp[3] && radarScanHost != 255) {
      ip4_addr_t target_ip4;
      IP4_ADDR(&target_ip4, localIp[0], localIp[1], localIp[2], radarScanHost);
      struct netif* sta = getStaNetif();
      if (sta) {
        etharp_request(sta, &target_ip4);
      }
    }

    // Continuously harvest all entries learned by LwIP
    harvestArpTable();

    radarScanHost++;
    if (radarScanHost > 254) {
      radarScanHost = 1;
      // Broadcast discovery burst every full sweep cycle (~12s)
      sendLanDiscoveryBroadcasts();
      harvestArpTable();
    }
  }

  // 3. Low-frequency NetBIOS Hostname Prober (queries 1 active LAN device every 3 seconds)
  // ponytail: NetBIOS names only cover Windows/Samba/NAS devices; upgrade path: lightweight mDNS PTR instance reader if Apple names need zero-config discovery
  static unsigned long lastNbProbeMs = 0;
  static uint8_t nextNbDevIdx = 0;
  if (lanDeviceCount > 0 && (now - lastNbProbeMs >= 3000UL)) {
    lastNbProbeMs = now;
    if (nextNbDevIdx >= lanDeviceCount) nextNbDevIdx = 0;
    
    // Only query if device is online and doesn't already have a human hostname
    if (lanDevices[nextNbDevIdx].is_online &&
        (strncmp(lanDevices[nextNbDevIdx].name, "Device ", 7) == 0 ||
         strcmp(lanDevices[nextNbDevIdx].name, lanDevices[nextNbDevIdx].vendor) == 0)) {
      IPAddress targetIp(lanDevices[nextNbDevIdx].ip[0], lanDevices[nextNbDevIdx].ip[1],
                         lanDevices[nextNbDevIdx].ip[2], lanDevices[nextNbDevIdx].ip[3]);
      static const uint8_t nbQuery[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x20,
        'C','K','A','A','A','A','A','A','A','A','A','A','A','A','A','A',
        'A','A','A','A','A','A','A','A','A','A','A','A','A','A','A','A',
        0x00, 0x00, 0x21, 0x00, 0x01
      };
      udp.beginPacket(targetIp, 137);
      udp.write(nbQuery, sizeof(nbQuery));
      udp.endPacket();
    }
    nextNbDevIdx++;
  }
}

// --- Setup ---
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);
  delay(500);

  loadConfig();
  loadVipList();

  for (int i = 0; i < PING_HISTORY_LEN; i++) pingHistory[i] = 15;

  wifiDisconnectHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& evt) {
    Serial.printf("[WIFI EVENT] Disconnected! Reason: %d, SSID: %s\n", evt.reason, evt.ssid.c_str());
  });
  wifiConnectHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& evt) {
    Serial.printf("[WIFI EVENT] Connected! IP: %s, GW: %s\n", evt.ip.toString().c_str(), evt.gw.toString().c_str());
  });

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASS);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);

  if (strlen(cfg_ssid) > 0) {
    Serial.printf("[WIFI] Connecting to %s (pass len %zu)...\n", cfg_ssid, strlen(cfg_pass));
    WiFi.begin(cfg_ssid, cfg_pass);
    unsigned long startConn = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startConn < 15000)) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
      sendLanDiscoveryBroadcasts();
    } else {
      Serial.printf("[WIFI] Initial attempt status: %d, SoftAP active at %s\n", (int)WiFi.status(), WiFi.softAPIP().toString().c_str());
    }
  }

  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", HTTP_PORT);
  }

  setupDashboardServer();
  setupTrapServer();
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  rtspServer.begin();
  rtspServer.setNoDelay(true);
  udp.begin(9);
  ntpUdp.begin(123);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  addLog("SYSTEM", "Sentinel active and guarding home network", "info");
  addLog("HONEYPOT", "Burglar traps armed (Door 1: Telnet 23, Door 2: Router 8080, Door 3: Camera 554)", "info");

  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  lastWatchdogRun = millis();
}

// --- Main Loop ---
void loop() {
  server.handleClient();
  trapServer.handleClient();
  handleTelnetHoneypot();
  handleCameraHoneypot();
  handleLanRadar();
  MDNS.update();
  handleSerialCLI();

  // Feature #8: Local Micro-NTP Time Server (UDP 123, RFC 5905)
  int ntpSize = ntpUdp.parsePacket();
  if (ntpSize >= 48) {
    uint8_t req[48], resp[48];
    ntpUdp.read(req, 48);
    time_t nowUnix = time(nullptr);
    bool isSynced = (nowUnix > 1000000000L);
    uint32_t unixSec = isSynced ? (uint32_t)nowUnix : (1730160000UL + (millis() / 1000));
    if (buildNtpResponse(req, unixSec, resp, isSynced)) {
      ntpUdp.beginPacket(ntpUdp.remoteIP(), ntpUdp.remotePort());
      ntpUdp.write(resp, 48);
      ntpUdp.endPacket();
      ntpServedCount++;
    }
  }

  // Drain any incoming UDP packets (SSDP / mDNS / NetBIOS / WoL) and harvest learned hosts
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    if (udp.remotePort() == 137 && packetSize >= 74) {
      uint8_t nbBuf[128];
      int r = udp.read(nbBuf, sizeof(nbBuf));
      char nbName[20];
      if (parseNetbiosName(nbBuf, r, nbName, sizeof(nbName))) {
        IPAddress remoteIp = udp.remoteIP();
        uint8_t rip[4] = {remoteIp[0], remoteIp[1], remoteIp[2], remoteIp[3]};
        int idx = findDevice(lanDevices, lanDeviceCount, rip);
        if (idx >= 0) {
          // SecOps fix: Deduplicate NetBIOS discovery so it only logs when hostname actually updates
          if (strcmp(lanDevices[idx].name, nbName) != 0) {
            strncpy(lanDevices[idx].name, nbName, sizeof(lanDevices[idx].name) - 1);
            lanDevices[idx].name[sizeof(lanDevices[idx].name) - 1] = '\0';
            addLog("RADAR", String("NetBIOS hostname found: ") + remoteIp.toString() + " -> " + nbName, "info");
            saveVipList();
          }
        }
      }
    }
    harvestArpTable();
    udp.flush();
  }

  unsigned long now = millis();

  // Check internet health every 10 seconds
  if (now - lastWatchdogRun >= 10000) {
    lastWatchdogRun = now;
    runNetworkWatchdog();
  }

  // Send Daily Morning Report every 24 hours
  if (now - lastDailyReportTime >= TWENTY_FOUR_HOURS) {
    lastDailyReportTime = now;
    sendDailyMorningReport();
  }

  // Boot notification once Wi-Fi connects
  static bool bootNotified = false;
  if (!bootNotified && WiFi.status() == WL_CONNECTED) {
    bootNotified = true;
    dispatchRawDiscord("⚡ **[SENTINEL RESTARTED & READY]**\\n> Device booted up, reconnected to Wi-Fi, and re-armed all home burglar traps.");
  }

  // Strobe management
  if (alertActive) {
    if (now - alertStartTime > 3500) {
      alertActive = false;
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      static unsigned long lastStrobe = 0;
      if (now - lastStrobe >= 80) {
        lastStrobe = now;
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
      }
    }
  } else {
    if (now - lastHeartbeat >= 2500) {
      lastHeartbeat = now;
      digitalWrite(LED_BUILTIN, LOW);
    } else if (now - lastHeartbeat >= 50) {
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }

  yield();
}

// --- Serial CLI ---
void handleSerialCLI() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  if (line.equalsIgnoreCase("help")) {
    Serial.println(F("\n=== Home Sentinel Commands ==="));
    Serial.println(F("status                 - Print device health and defense status"));
    Serial.println(F("report                 - Send Daily Morning Report to Discord right now"));
    Serial.println(F("alert                  - Simulate burglar alarm test"));
    Serial.println(F("reboot                 - Restart Sentinel"));
    Serial.println(F("printconfig            - Inspect active configuration"));
    Serial.println(F("wifi <ssid> <pass>     - Configure Wi-Fi credentials"));
    Serial.println(F("reconnect              - Trigger Wi-Fi reconnection"));
    Serial.println(F("==============================\n"));
  } else if (line.equalsIgnoreCase("reconnect")) {
    Serial.println("Triggering Wi-Fi reconnection...");
    WiFi.disconnect();
    delay(200);
    WiFi.begin(cfg_ssid, cfg_pass);
  } else if (line.equalsIgnoreCase("diag")) {
    Serial.printf("Mode: %d | Channel: %d | Status: %d | SSID: %s | IP: %s\n",
                  (int)WiFi.getMode(), (int)WiFi.channel(), (int)WiFi.status(),
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  } else if (line.equalsIgnoreCase("scan")) {
    int n = WiFi.scanNetworks();
    Serial.printf("Found %d networks:\n", n);
    for (int i = 0; i < n; i++) {
      Serial.printf("  %s (%d dBm, ch %d, enc %d)\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i), WiFi.encryptionType(i));
    }
  } else if (line.equalsIgnoreCase("printconfig")) {
    Serial.printf("Config: SSID='%s' Pass='%s' WebhookLen=%zu IP=%s Status=%d\n",
                  cfg_ssid, cfg_pass, strlen(cfg_webhook), WiFi.localIP().toString().c_str(), (int)WiFi.status());
  } else if (line.startsWith("wifi ")) {
    int firstSpace = line.indexOf(' ');
    int secondSpace = line.indexOf(' ', firstSpace + 1);
    if (secondSpace > firstSpace) {
      String s = line.substring(firstSpace + 1, secondSpace);
      String p = line.substring(secondSpace + 1);
      s.trim();
      p.trim();
      strncpy(cfg_ssid, s.c_str(), sizeof(cfg_ssid) - 1);
      strncpy(cfg_pass, p.c_str(), sizeof(cfg_pass) - 1);
      saveConfig();
      Serial.printf("Config saved. Connecting to %s...\n", cfg_ssid);
      WiFi.begin(cfg_ssid, cfg_pass);
    }
  } else if (line.startsWith("webhook ")) {
    String url = line.substring(8);
    url.trim();
    strncpy(cfg_webhook, url.c_str(), sizeof(cfg_webhook) - 1);
    saveConfig();
    Serial.printf("Direct alert webhook saved (len=%zu)\n", strlen(cfg_webhook));
  } else if (line.equalsIgnoreCase("status")) {
    Serial.printf("Guarding for: %lu s | Free Memory: %u bytes | Power: %u mV\n",
                  millis() / 1000, ESP.getFreeHeap(), ESP.getVcc());
    Serial.printf("Wi-Fi: %s (%s) [status=%d] | Internet Lag: %d ms | Router Lag: %d ms\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), (int)WiFi.status(),
                  metrics.internet_ping_ms, metrics.gateway_ping_ms);
  } else if (line.equalsIgnoreCase("report")) {
    sendDailyMorningReport();
  } else if (line.equalsIgnoreCase("alert")) {
    triggerStrobe();
    addLog("TEST", "Simulated burglar alarm test from serial", "danger");
  } else if (line.equalsIgnoreCase("reboot")) {
    ESP.restart();
  }
}
