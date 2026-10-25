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

#include "config.h"
#include "html_ui.h"
#include "trap_ui.h"

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

// Telnet Burglar Trap State
enum TelnetState { T_DISCONNECTED, T_SENT_BANNER, T_WAIT_USER, T_WAIT_PASS };
TelnetState telnetState = T_DISCONNECTED;
String telnetUser = "";
String telnetInput = "";

// Forward declarations
void addLog(const char* type, const String& msg, const char* severity);
void triggerStrobe();
void sendWakeOnLan(const char* macStr);
void handleSerialCLI();
void saveConfig();
void loadConfig();
void dispatchWebhook(const char* type, const String& msg, const char* severity);
void dispatchRawDiscord(const String& rawContent);
void sendDailyMorningReport();
void appendPersistentLog(const char* type, const String& msg, const char* severity);

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
  
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  client->setInsecure();
  client->setBufferSizes(1024, 512);
  client->setTimeout(3000);

  HTTPClient http;
  if (http.begin(*client, cfg_webhook)) {
    http.addHeader("Content-Type", "application/json");
    
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
    } else if (strcmp(type, "TEST") == 0) {
      label = "TEST ALARM";
      emoji = "🔔";
    }

    String cleanMsg = msg;
    cleanMsg.replace("\"", "'");
    String payload = "{\"content\":\"" + emoji + " **[" + label + "]**\\n> " + cleanMsg + "\"}";

    http.POST(payload);
    http.end();
  }
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
}

void saveConfig() {
  File f = LittleFS.open("/config.json", "w");
  if (!f) return;
  JsonDocument doc;
  doc["ssid"] = cfg_ssid;
  doc["pass"] = cfg_pass;
  doc["webhook"] = cfg_webhook;
  doc["wol"] = cfg_last_wol;
  serializeJson(doc, f);
  f.close();
}

// --- Remote Wake-Up Switch ---
void sendWakeOnLan(const char* macStr) {
  uint8_t mac[6];
  int parsed = sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                      &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
  if (parsed != 6) {
    parsed = sscanf(macStr, "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx",
                    &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
  }
  if (parsed != 6) {
    addLog("WOL", String("Invalid Hardware ID: ") + macStr, "warn");
    return;
  }

  uint8_t packet[102];
  memset(packet, 0xFF, 6);
  for (int i = 0; i < 16; i++) {
    memcpy(&packet[6 + i * 6], mac, 6);
  }

  udp.beginPacket(IPAddress(255, 255, 255, 255), 9);
  udp.write(packet, sizeof(packet));
  udp.endPacket();

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
      activeTelnetClient.print("\r\nUbuntu 22.04.4 LTS (GNU/Linux 5.15.0-94-generic x86_64)\r\n\r\nlogin: ");
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
          activeTelnetClient.print("Password: ");
        } else if (telnetState == T_WAIT_PASS) {
          String pass = telnetInput;
          String ip = activeTelnetClient.remoteIP().toString();
          addLog("HONEYPOT", "Burglar trap caught intruder " + ip + " trying User: '" + telnetUser + "', Pass: '" + pass + "'", "danger");
          triggerStrobe();
          activeTelnetClient.println("\r\nLogin incorrect");
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
      cli.println("RTSP/1.0 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"IPCamera\"\r\n\r\n");
      cli.stop();
      metrics.honeypot_camera_hits++;
      addLog("HONEYPOT", "Fake Security Camera (Port 554) probed by " + ip, "danger");
      triggerStrobe();
    }
  }
}

// --- Primary Dashboard Web Server (Port 80) ---
void setupDashboardServer() {
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

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
    doc["deauth_count"] = 0;
    doc["honeypot_hits"] = metrics.honeypot_telnet_hits + metrics.honeypot_web_hits + metrics.honeypot_camera_hits;
    doc["phones"] = 0;
    doc["camera_hits"] = metrics.honeypot_camera_hits;
    doc["active_devices"] = 1;
    doc["watched_ip"] = "";
    doc["watched_online"] = true;
    doc["spam_alert"] = false;
    doc["attack_alert"] = alertActive;
    doc["wan_down"] = metrics.is_internet_down;

    JsonArray hist = doc["ping_history"].to<JsonArray>();
    for (int i = 0; i < PING_HISTORY_LEN; i++) {
      int idx = (pingHistoryIdx + i) % PING_HISTORY_LEN;
      hist.add(pingHistory[idx]);
    }

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
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
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/ping-now", HTTP_POST, []() {
    bool gw = Ping.ping(WiFi.gatewayIP(), 1);
    metrics.gateway_ping_ms = gw ? Ping.averageTime() : -1;
    bool wan = Ping.ping(IPAddress(1, 1, 1, 1), 1);
    metrics.internet_ping_ms = wan ? Ping.averageTime() : -1;
    addLog("WATCHDOG", "Internet check: Router=" + String(metrics.gateway_ping_ms) + "ms, Internet=" + String(metrics.internet_ping_ms) + "ms", "info");
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/wifi", HTTP_POST, []() {
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
    server.send(400, "application/json", "{\"status\":\"error\"}");
  });

  server.on("/api/webhook", HTTP_POST, []() {
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
    server.send(400, "application/json", "{\"status\":\"error\"}");
  });

  server.on("/api/clear-logs", HTTP_POST, []() {
    logCount = 0;
    if (LittleFS.exists("/audit.log")) LittleFS.remove("/audit.log");
    addLog("SYSTEM", "Activity records cleared by owner", "info");
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.begin();
}

// --- Internet Health Tracker ---
void runNetworkWatchdog() {
  if (WiFi.status() != WL_CONNECTED) {
    metrics.internet_ping_ms = -1;
    metrics.gateway_ping_ms = -1;
    return;
  }

  metrics.total_pings++;
  bool gw_ok = Ping.ping(WiFi.gatewayIP(), 1);
  metrics.gateway_ping_ms = gw_ok ? Ping.averageTime() : -1;

  bool wan_ok = Ping.ping(IPAddress(1, 1, 1, 1), 1);
  metrics.internet_ping_ms = wan_ok ? Ping.averageTime() : -1;

  if (wan_ok) {
    pingHistory[pingHistoryIdx] = metrics.internet_ping_ms;
  } else {
    pingHistory[pingHistoryIdx] = -1;
    metrics.failed_pings++;
  }
  pingHistoryIdx = (pingHistoryIdx + 1) % PING_HISTORY_LEN;

  if (!wan_ok && !metrics.is_internet_down) {
    metrics.is_internet_down = true;
    metrics.total_outages++;
    addLog("OUTAGE", "Internet connection was dropped (unreachable)", "danger");
    triggerStrobe();
  } else if (wan_ok && metrics.is_internet_down) {
    metrics.is_internet_down = false;
    addLog("OUTAGE", "Internet connection has been restored", "success");
  }
}

// --- Setup ---
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);
  delay(500);

  loadConfig();

  for (int i = 0; i < PING_HISTORY_LEN; i++) pingHistory[i] = 15;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASS);

  if (strlen(cfg_ssid) > 0) {
    WiFi.begin(cfg_ssid, cfg_pass);
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

  addLog("SYSTEM", "Sentinel active and guarding home network", "info");
  addLog("HONEYPOT", "Burglar traps armed (Door 1: Telnet 23, Door 2: Router 8080, Door 3: Camera 554)", "info");

  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}

// --- Main Loop ---
void loop() {
  server.handleClient();
  trapServer.handleClient();
  handleTelnetHoneypot();
  handleCameraHoneypot();
  MDNS.update();
  handleSerialCLI();

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
    Serial.println("\n=== Home Sentinel Commands ===");
    Serial.println("status                 - Print device health and defense status");
    Serial.println("report                 - Send Daily Morning Report to Discord right now");
    Serial.println("alert                  - Simulate burglar alarm test");
    Serial.println("reboot                 - Restart Sentinel");
    Serial.println("==============================\n");
  } else if (line.equalsIgnoreCase("status")) {
    Serial.printf("Guarding for: %lu s | Free Memory: %u bytes | Power: %u mV\n",
                  millis() / 1000, ESP.getFreeHeap(), ESP.getVcc());
    Serial.printf("Wi-Fi: %s (%s) | Internet Lag: %d ms | Router Lag: %d ms\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(),
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
