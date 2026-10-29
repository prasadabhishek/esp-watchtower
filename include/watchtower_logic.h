#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>

// --- MAC Parsing & Wake-on-LAN Magic Packet ---
inline bool parseMacAddress(const char* macStr, uint8_t macOut[6]) {
  if (!macStr || strlen(macStr) < 12) return false;
  int parsed = sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                      &macOut[0], &macOut[1], &macOut[2], &macOut[3], &macOut[4], &macOut[5]);
  if (parsed != 6) {
    parsed = sscanf(macStr, "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx",
                    &macOut[0], &macOut[1], &macOut[2], &macOut[3], &macOut[4], &macOut[5]);
  }
  return (parsed == 6);
}

inline bool buildMagicPacket(const uint8_t mac[6], uint8_t packetOut[102]) {
  if (!mac || !packetOut) return false;
  memset(packetOut, 0xFF, 6);
  for (int i = 0; i < 16; i++) {
    memcpy(&packetOut[6 + i * 6], mac, 6);
  }
  return true;
}

// --- Outage & Duration Calculation ---
// --- CSRF Origin Guard ---
inline bool isAuthorizedOrigin(const char* origin, const char* localIp, const char* apIp, const char* hostname) {
  if (!origin || strlen(origin) == 0) return true; // Direct non-browser request or curl
  if (apIp && strlen(apIp) > 0 && strstr(origin, apIp)) return true;
  if (localIp && strlen(localIp) > 0 && strstr(origin, localIp)) return true;
  if (strstr(origin, "192.168.4.1")) return true;
  if (strstr(origin, "127.0.0.1")) return true;
  if (strstr(origin, "localhost")) return true;
  if (hostname && strlen(hostname) > 0 && strstr(origin, hostname)) return true;
  if (strstr(origin, ".local")) return true;
  if (strstr(origin, ".ts.net")) return true;
  return false;
}

// ponytail: Evaluates whether a confirmed ISP downtime is occurring.
// An outage is ONLY confirmed if the local Wi-Fi router is healthy (gwOk == true),
// but the WAN (outside internet) is unreachable (wanOk == false).
// If the local router is also unreachable, it is a local Wi-Fi or router reboot, NOT an ISP outage.
inline bool evaluateOutageCondition(bool gwOk, bool wanOk, unsigned long nowMs, unsigned long& outageStartMs,
                                   uint32_t thresholdSec, bool& isCurrentlyDown,
                                   bool& shouldAlertDown, bool& shouldAlertRestored,
                                   uint32_t& totalOutageDurationSec) {
  shouldAlertDown = false;
  shouldAlertRestored = false;
  totalOutageDurationSec = 0;

  // STRICT ISP DOWNTIME CONDITION:
  // Gateway/Router is answering, but WAN is dead.
  bool isSureIspDown = (gwOk && !wanOk);

  if (isSureIspDown) {
    if (outageStartMs == 0) {
      outageStartMs = nowMs;
    }
    if (!isCurrentlyDown && (nowMs - outageStartMs >= thresholdSec * 1000UL)) {
      isCurrentlyDown = true;
      shouldAlertDown = true;
      return true;
    }
  } else {
    // If it was marked as a confirmed ISP outage, and WAN has now returned
    if (isCurrentlyDown && wanOk) {
      totalOutageDurationSec = (uint32_t)((nowMs - outageStartMs) / 1000UL);
      isCurrentlyDown = false;
      shouldAlertRestored = true;
      outageStartMs = 0;
      return true;
    }
    if (!isCurrentlyDown) {
      outageStartMs = 0; // Reset intermittent failure
    }
  }
  return false;
}

struct IspOutageRecord {
  uint32_t id;
  uint32_t start_sec;
  uint32_t duration_sec;
  bool is_active;
};

// --- Network Diagnosis: Blame Meter & Queueing Theory Jitter ---
// ponytail: Categorizes latency/loss and queue oscillation into plain English diagnosis
enum BlameVerdict {
  BLAME_OK = 0,          // Both router and WAN latency normal
  BLAME_ISP_DOWN = 1,    // Router reachable, but WAN unreachable
  BLAME_ISP_LAG = 2,     // Router fast (<40ms), but WAN lagging (>120ms)
  BLAME_ROUTER_LAG = 3,  // Local Wi-Fi / Router congested (>45ms)
  BLAME_BUFFERBLOAT = 4, // Severe queue oscillation / bufferbloat (Jitter > 30ms)
  BLAME_OFFLINE = 5      // Disconnected from Wi-Fi
};

inline int calculateJitter(const int history[], int len) {
  if (!history || len < 2) return 0;
  int sumDiff = 0;
  int validPairs = 0;
  int lastValid = -1;
  for (int i = 0; i < len; i++) {
    if (history[i] >= 0) {
      if (lastValid >= 0) {
        sumDiff += (history[i] > lastValid) ? (history[i] - lastValid) : (lastValid - history[i]);
        validPairs++;
      }
      lastValid = history[i];
    }
  }
  return (validPairs > 0) ? (sumDiff / validPairs) : 0;
}

inline BlameVerdict diagnoseNetworkLag(int gwPingMs, int wanPingMs, int failedPings, int jitterMs = 0) {
  if (gwPingMs < 0 && wanPingMs < 0) return BLAME_OFFLINE;
  if (wanPingMs < 0 || failedPings > 0) return BLAME_ISP_DOWN;
  if (gwPingMs > 45) return BLAME_ROUTER_LAG;
  if (jitterMs > 30) return BLAME_BUFFERBLOAT;
  if (wanPingMs > 120) return BLAME_ISP_LAG;
  return BLAME_OK;
}

inline const char* getBlameDescription(BlameVerdict verdict) {
  switch (verdict) {
    case BLAME_OK: return "ALL SYSTEMS FAST (Wi-Fi & ISP Optimal)";
    case BLAME_ISP_DOWN: return "ISP OUTAGE DETECTED (Local Wi-Fi is fine)";
    case BLAME_ISP_LAG: return "ISP SLOWDOWN (ISP Lagging, not Wi-Fi)";
    case BLAME_ROUTER_LAG: return "LOCAL WI-FI CONGESTION (Router/RF Congested)";
    case BLAME_BUFFERBLOAT: return "BUFFERBLOAT / JITTER SPIKE (Queue Instability)";
    case BLAME_OFFLINE: return "DISCONNECTED (No Wi-Fi Link)";
    default: return "CHECKING NETWORK...";
  }
}

// --- Verified ISP Downtime Formatter ---
inline bool formatDowntimeLogEntry(char* out, size_t maxLen, uint32_t outageId, uint32_t startSecAgo, uint32_t durationSec, bool isActive) {
  if (!out || maxLen < 120) return false;
  uint32_t mins = durationSec / 60;
  uint32_t secs = durationSec % 60;
  if (isActive) {
    snprintf(out, maxLen, "[Outage #%u] Started %us ago | Status: ONGOING DOWNTIME | Cause: Confirmed ISP Failure (Local Router Online)",
             outageId, startSecAgo);
  } else {
    snprintf(out, maxLen, "[Outage #%u] %um %us downtime | Status: RESTORED | Cause: Confirmed ISP Failure (Local Router Online)",
             outageId, mins, secs);
  }
  return true;
}

// --- ISP Outage Claim Receipt Formatter ---
inline bool formatOutageClaimReceipt(char* out, size_t maxLen,
                                     const char* ispName, const char* accountId,
                                     uint32_t totalOutages, uint32_t durationSec,
                                     uint32_t failedPings, int gwPingMs, int wanPingMs) {
  if (!out || maxLen < 200) return false;
  const char* isp = (ispName && strlen(ispName) > 0) ? ispName : "[ISP Provider]";
  const char* acc = (accountId && strlen(accountId) > 0) ? accountId : "[Account/Circuit #]";
  
  uint32_t mins = durationSec / 60;
  uint32_t secs = durationSec % 60;

  snprintf(out, maxLen,
    "=== ISP SERVICE OUTAGE CLAIM RECEIPT ===\n"
    "ISP: %s | Account: %s\n"
    "Diagnostic Probe: ESP8266 Watchtower Sentinel\n"
    "Gateway RTT: %d ms | WAN 1.1.1.1 RTT: %d ms\n"
    "Recorded Outages: %u | Total Failed Checks: %u\n"
    "Latest Outage Duration: %um %us\n"
    "Diagnosis: Gateway reachable; upstream ISP connection interrupted.\n"
    "Request: Billing credit for downtime per SLA terms.\n"
    "=========================================",
    isp, acc, gwPingMs, wanPingMs, totalOutages, failedPings, mins, secs);
  return true;
}

// --- LAN Device Structures & Radar Logic ---

struct LanDevice {
  uint8_t ip[4];
  uint8_t mac[6];
  char name[32];
  char vendor[20];
  bool is_vip;
  bool track_presence; // true if this specific device should trigger arrival/departure notifications
  bool is_online;
  uint32_t first_seen_sec;
  uint32_t last_seen_sec;
  int latency_ms;
  uint8_t missed_sweeps;
};

inline bool compareIp(const uint8_t a[4], const uint8_t b[4]) {
  return (a && b && memcmp(a, b, 4) == 0);
}

inline void ipToString(const uint8_t ip[4], char* out, size_t maxLen) {
  if (!out || maxLen < 16) return;
  snprintf(out, maxLen, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

inline bool parseIpAddress(const char* str, uint8_t ipOut[4]) {
  if (!str || !ipOut) return false;
  unsigned int b0, b1, b2, b3;
  if (sscanf(str, "%u.%u.%u.%u", &b0, &b1, &b2, &b3) == 4) {
    if (b0 <= 255 && b1 <= 255 && b2 <= 255 && b3 <= 255) {
      ipOut[0] = (uint8_t)b0;
      ipOut[1] = (uint8_t)b1;
      ipOut[2] = (uint8_t)b2;
      ipOut[3] = (uint8_t)b3;
      return true;
    }
  }
  return false;
}

inline void macToString(const uint8_t mac[6], char* out, size_t maxLen) {
  if (!out || maxLen < 18) return;
  snprintf(out, maxLen, "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

inline bool isRandomizedMac(const uint8_t mac[6]) {
  if (!mac) return false;
  // IEEE 802 Locally Administered Bit (Bit 1 of Byte 0)
  return ((mac[0] & 0x02) != 0);
}

inline void resolveVendor(const uint8_t mac[6], char* out, size_t maxLen) {
  if (!out || maxLen < 16) return;
  if (mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0) {
    strncpy(out, "Generic Client", maxLen);
    return;
  }
  
  // IEEE 802 Locally Administered Bit -> iOS / Android / Windows Private MAC
  if ((mac[0] & 0x02) != 0) {
    strncpy(out, "Private Mobile MAC", maxLen);
    out[maxLen - 1] = '\0';
    return;
  }

  // Apple
  if ((mac[0] == 0xb0 && mac[1] == 0xde && mac[2] == 0x28) ||
      (mac[0] == 0xa4 && mac[1] == 0x83 && mac[2] == 0xe7) ||
      (mac[0] == 0xf0 && mac[1] == 0x18 && mac[2] == 0x98) ||
      (mac[0] == 0x64 && mac[1] == 0xbb && mac[2] == 0x1e) ||
      (mac[0] == 0xaa && mac[1] == 0x57 && mac[2] == 0x9c)) {
    strncpy(out, "Apple Inc.", maxLen);
  }
  // Synology
  else if ((mac[0] == 0x00 && mac[1] == 0x24 && mac[2] == 0xe4) ||
           (mac[0] == 0x00 && mac[1] == 0x11 && mac[2] == 0x32) ||
           (mac[0] == 0x90 && mac[1] == 0x09 && mac[2] == 0xd0)) {
    strncpy(out, "Synology NAS", maxLen);
  }
  // Espressif
  else if ((mac[0] == 0x84 && mac[1] == 0xcc && mac[2] == 0xa8) ||
           (mac[0] == 0x24 && mac[1] == 0x6f && mac[2] == 0x28) ||
           (mac[0] == 0x48 && mac[1] == 0x55 && mac[2] == 0x19)) {
    strncpy(out, "Espressif IoT", maxLen);
  }
  // Asus / Router
  else if (mac[0] == 0x04 && mac[1] == 0x42 && mac[2] == 0x1a) {
    strncpy(out, "Asus Router", maxLen);
  }
  // Google / Nest
  else if (mac[0] == 0x40 && mac[1] == 0xae && mac[2] == 0x30) {
    strncpy(out, "Google Nest", maxLen);
  }
  // Amazon Echo / Alexa
  else if ((mac[0] == 0x80 && mac[1] == 0x48 && mac[2] == 0x2c) ||
           (mac[0] == 0x74 && mac[1] == 0xec && mac[2] == 0xb2) ||
           (mac[0] == 0x88 && mac[1] == 0x71 && mac[2] == 0xe5) ||
           (mac[0] == 0x74 && mac[1] == 0xd6 && mac[2] == 0x37) ||
           (mac[0] == 0x58 && mac[1] == 0xe4 && mac[2] == 0x88) ||
           (mac[0] == 0x14 && mac[1] == 0x0a && mac[2] == 0xc5)) {
    strncpy(out, "Amazon Alexa", maxLen);
  }
  // Roborock
  else if (mac[0] == 0x24 && mac[1] == 0x9e && mac[2] == 0x7d) {
    strncpy(out, "Roborock Vacuum", maxLen);
  }
  // TP-Link
  else if (mac[0] == 0xcc && mac[1] == 0x32 && mac[2] == 0xe5) {
    strncpy(out, "TP-Link Smart Plug", maxLen);
  }
  // Canon
  else if (mac[0] == 0x74 && mac[1] == 0x38 && mac[2] == 0xb7) {
    strncpy(out, "Canon Printer", maxLen);
  }
  // Vizio SmartCast
  else if ((mac[0] == 0xa4 && mac[1] == 0x8d && mac[2] == 0x3b) ||
           (mac[0] == 0x2c && mac[1] == 0x64 && mac[2] == 0x1f)) {
    strncpy(out, "Vizio SmartCast", maxLen);
  }
  // LG Smart TV
  else if (mac[0] == 0x4c && mac[1] == 0xba && mac[2] == 0xd7) {
    strncpy(out, "LG Smart TV", maxLen);
  }
  // Smart Appliance (AzureWave)
  else if (mac[0] == 0x14 && mac[1] == 0x13 && mac[2] == 0x33) {
    strncpy(out, "Smart Appliance", maxLen);
  }
  // Raspberry Pi
  else if ((mac[0] == 0xb8 && mac[1] == 0x27 && mac[2] == 0xeb) ||
           (mac[0] == 0xdc && mac[1] == 0xa6 && mac[2] == 0x32)) {
    strncpy(out, "Raspberry Pi", maxLen);
  }
  else {
    strncpy(out, "LAN Client", maxLen);
  }
  out[maxLen - 1] = '\0';
}

// ponytail: NetBIOS names only cover Windows/Samba/NAS devices; upgrade path: lightweight mDNS PTR instance reader if Apple names need zero-config discovery
inline bool parseNetbiosName(const uint8_t* payload, size_t len, char* nameOut, size_t maxLen) {
  if (!payload || len < 74 || !nameOut || maxLen < 2) return false;
  uint8_t numNames = payload[56];
  if (numNames == 0 || numNames > 20) return false;
  char raw[16];
  memcpy(raw, &payload[57], 15);
  raw[15] = '\0';
  for (int i = 14; i >= 0; i--) {
    if (raw[i] == ' ' || raw[i] == 0) raw[i] = '\0';
    else break;
  }
  if (strlen(raw) == 0) return false;
  strncpy(nameOut, raw, maxLen);
  nameOut[maxLen - 1] = '\0';
  return true;
}

inline int findDevice(const LanDevice* list, uint8_t count, const uint8_t ip[4]) {
  if (!list) return -1;
  for (uint8_t i = 0; i < count; i++) {
    if (compareIp(list[i].ip, ip)) return i;
  }
  return -1;
}

inline bool upsertDeviceRecord(LanDevice* list, uint8_t& count, uint8_t maxCap,
                               const uint8_t ip[4], const uint8_t mac[6], int latency,
                               uint32_t nowSec, bool& cameOnline, bool& isNew) {
  cameOnline = false;
  isNew = false;
  if (!list || !ip) return false;

  int idx = findDevice(list, count, ip);
  if (idx >= 0) {
    if (!list[idx].is_online) {
      cameOnline = true;
      list[idx].is_online = true;
    }
    list[idx].last_seen_sec = nowSec;
    list[idx].latency_ms = latency;
    list[idx].missed_sweeps = 0;
    if (mac) {
      memcpy(list[idx].mac, mac, 6);
      resolveVendor(mac, list[idx].vendor, sizeof(list[idx].vendor));
    }
    return true;
  }

  // Insert new
  if (count >= maxCap) {
    // Evict oldest non-VIP offline device
    int evictIdx = -1;
    uint32_t oldestTime = 0xFFFFFFFF;
    for (uint8_t i = 0; i < count; i++) {
      if (!list[i].is_vip && !list[i].is_online && list[i].last_seen_sec < oldestTime) {
        oldestTime = list[i].last_seen_sec;
        evictIdx = i;
      }
    }
    if (evictIdx >= 0) {
      idx = evictIdx;
    } else {
      return false; // Table full of VIPs or active devices
    }
  } else {
    idx = count++;
  }

  memcpy(list[idx].ip, ip, 4);
  if (mac) {
    memcpy(list[idx].mac, mac, 6);
    resolveVendor(mac, list[idx].vendor, sizeof(list[idx].vendor));
  } else {
    memset(list[idx].mac, 0, 6);
    strncpy(list[idx].vendor, "LAN Device", sizeof(list[idx].vendor));
  }
  char ipBuf[16];
  ipToString(ip, ipBuf, sizeof(ipBuf));
  if (strcmp(list[idx].vendor, "LAN Client") != 0 && strcmp(list[idx].vendor, "LAN Device") != 0 && strcmp(list[idx].vendor, "Generic Client") != 0) {
    snprintf(list[idx].name, sizeof(list[idx].name), "%s", list[idx].vendor);
  } else {
    snprintf(list[idx].name, sizeof(list[idx].name), "Device %s", ipBuf);
  }
  list[idx].is_vip = false;
  list[idx].track_presence = false;
  list[idx].is_online = true;
  list[idx].first_seen_sec = nowSec;
  list[idx].last_seen_sec = nowSec;
  list[idx].latency_ms = latency;
  list[idx].missed_sweeps = 0;
  isNew = true;
  cameOnline = true;
  return true;
}

inline bool markDeviceSweepResult(LanDevice* list, uint8_t count, const uint8_t ip[4],
                                  bool responded, int latency, uint32_t nowSec,
                                  uint8_t maxMisses, bool& wentOffline, bool& cameOnline) {
  wentOffline = false;
  cameOnline = false;
  int idx = findDevice(list, count, ip);
  if (idx < 0) return false;

  if (responded) {
    if (!list[idx].is_online) {
      cameOnline = true;
      list[idx].is_online = true;
    }
    list[idx].last_seen_sec = nowSec;
    list[idx].latency_ms = latency;
    list[idx].missed_sweeps = 0;
  } else {
    list[idx].missed_sweeps++;
    if (list[idx].is_online && list[idx].missed_sweeps >= maxMisses) {
      list[idx].is_online = false;
      wentOffline = true;
    }
  }
  return true;
}

inline bool toggleVipStatus(LanDevice* list, uint8_t count, const uint8_t ip[4], bool isVip, const char* customName) {
  int idx = findDevice(list, count, ip);
  if (idx < 0) return false;
  list[idx].is_vip = isVip;
  if (customName && strlen(customName) > 0) {
    strncpy(list[idx].name, customName, sizeof(list[idx].name) - 1);
    list[idx].name[sizeof(list[idx].name) - 1] = '\0';
  }
  return true;
}

// ponytail: Pure server-side table sorting in C++ (Zero JavaScript)
inline void sortDeviceList(LanDevice* list, uint8_t count, const char* sortKey, bool desc) {
  if (!list || count < 2) return;
  const char* key = (sortKey && strlen(sortKey) > 0) ? sortKey : "ip";

  std::sort(list, list + count, [key, desc](const LanDevice& a, const LanDevice& b) {
    int cmp = 0;
    if (strcmp(key, "name") == 0) {
      cmp = strcasecmp(a.name, b.name);
    } else if (strcmp(key, "vendor") == 0) {
      cmp = strcasecmp(a.vendor, b.vendor);
    } else if (strcmp(key, "latency") == 0) {
      if (a.is_online != b.is_online) {
        return desc ? (!a.is_online < !b.is_online) : (a.is_online > b.is_online);
      }
      cmp = a.latency_ms - b.latency_ms;
    } else if (strcmp(key, "seen") == 0) {
      cmp = (a.last_seen_sec > b.last_seen_sec) ? 1 : (a.last_seen_sec < b.last_seen_sec ? -1 : 0);
    } else if (strcmp(key, "status") == 0) {
      cmp = (int)a.is_online - (int)b.is_online;
    } else if (strcmp(key, "vip") == 0) {
      cmp = (int)a.is_vip - (int)b.is_vip;
    } else if (strcmp(key, "presence") == 0) {
      cmp = (int)a.track_presence - (int)b.track_presence;
    } else { // default: "ip"
      uint32_t ipA = ((uint32_t)a.ip[0] << 24) | ((uint32_t)a.ip[1] << 16) | ((uint32_t)a.ip[2] << 8) | a.ip[3];
      uint32_t ipB = ((uint32_t)b.ip[0] << 24) | ((uint32_t)b.ip[1] << 16) | ((uint32_t)b.ip[2] << 8) | b.ip[3];
      cmp = (ipA < ipB) ? -1 : (ipA > ipB ? 1 : 0);
    }
    return desc ? (cmp > 0) : (cmp < 0);
  });
}

// --- Feature #7: DNS Hijacking & Benchmark Probe (RFC 1035 UDP 53) ---
inline size_t buildDnsQueryPacket(const char* domain, uint16_t txId, uint8_t* out, size_t maxLen) {
  if (!domain || !out || maxLen < 64) return 0;
  memset(out, 0, 12);
  out[0] = (uint8_t)(txId >> 8);
  out[1] = (uint8_t)(txId & 0xFF);
  out[2] = 0x01; // Recursion Desired
  out[5] = 0x01; // QDCOUNT = 1

  size_t pos = 12;
  const char* p = domain;
  while (*p) {
    const char* dot = strchr(p, '.');
    size_t labelLen = dot ? (size_t)(dot - p) : strlen(p);
    if (labelLen == 0 || labelLen > 63 || pos + labelLen + 6 >= maxLen) return 0;
    out[pos++] = (uint8_t)labelLen;
    memcpy(&out[pos], p, labelLen);
    pos += labelLen;
    if (!dot) break;
    p = dot + 1;
  }
  out[pos++] = 0x00; // Root terminator
  out[pos++] = 0x00; out[pos++] = 0x01; // QTYPE = A (1)
  out[pos++] = 0x00; out[pos++] = 0x01; // QCLASS = IN (1)
  return pos;
}

inline bool parseDnsResponse(const uint8_t* buf, size_t len, uint16_t expectedTxId, uint8_t& rcodeOut, uint16_t& answerCountOut) {
  if (!buf || len < 12) return false;
  uint16_t rxId = ((uint16_t)buf[0] << 8) | buf[1];
  if (rxId != expectedTxId) return false;
  if ((buf[2] & 0x80) == 0) return false; // Must be QR=1 (response)
  rcodeOut = buf[3] & 0x0F;
  answerCountOut = ((uint16_t)buf[6] << 8) | buf[7];
  return true;
}

// --- Feature #8: Local Micro-NTP Time Server (RFC 5905 UDP 123) ---
inline bool buildNtpResponse(const uint8_t req[48], uint32_t unixTimeSec, uint8_t resp[48], bool isClockSynced = true) {
  if (!req || !resp || unixTimeSec < 1000000000UL) return false;
  memset(resp, 0, 48);
  resp[0] = isClockSynced ? 0x24 : 0xE4; // LI = 0 (Normal) or LI = 3 (Alarm/Unsynced), VN = 4, Mode = 4 (Server)
  resp[1] = isClockSynced ? 2 : 16;     // Stratum 2 (Secondary) or 16 (Unsynchronized)
  resp[2] = req[2];                     // Copy client Poll interval
  resp[3] = 0xEC;                       // Precision -20 (~1 microsecond)
  // Root Delay ~16ms (0x0000.0400 in 16.16 fixed-point)
  resp[6] = 0x01; resp[7] = 0x00;
  // Root Dispersion ~15ms (0x0000.03E8 in 16.16 fixed-point)
  resp[10] = 0x00; resp[11] = 0xF0;
  // Reference ID: "LOCL"
  resp[12] = 'L'; resp[13] = 'O'; resp[14] = 'C'; resp[15] = 'L';

  // Copy client Transmit Timestamp (bytes 40..47) -> Origin Timestamp (bytes 24..31)
  memcpy(&resp[24], &req[40], 8);

  // Convert Unix epoch (1970) to NTP epoch (1900): +2208988800UL
  uint32_t ntpSec = unixTimeSec + 2208988800UL;
  uint8_t ts[4] = {
    (uint8_t)(ntpSec >> 24),
    (uint8_t)(ntpSec >> 16),
    (uint8_t)(ntpSec >> 8),
    (uint8_t)(ntpSec & 0xFF)
  };
  // Set Reference (16..19), Receive (32..35), and Transmit (40..43) timestamps
  memcpy(&resp[16], ts, 4);
  memcpy(&resp[32], ts, 4);
  memcpy(&resp[40], ts, 4);
  return true;
}

// --- Feature #10: Home Assistant MQTT v3.1.1 Telemetry (TCP 1883, Zero External Libs) ---
inline size_t buildMqttConnectPacket(const char* clientId, uint8_t* out, size_t maxLen) {
  if (!clientId || !out) return 0;
  size_t idLen = strlen(clientId);
  size_t remLen = 10 + 2 + idLen;
  if (remLen > 127 || maxLen < remLen + 2) return 0;
  out[0] = 0x10; // CONNECT
  out[1] = (uint8_t)remLen;
  // Protocol Name "MQTT" (level 4 = v3.1.1)
  out[2] = 0x00; out[3] = 0x04;
  out[4] = 'M'; out[5] = 'Q'; out[6] = 'T'; out[7] = 'T';
  out[8] = 0x04; // Level 3.1.1
  out[9] = 0x02; // Clean Session
  out[10] = 0x00; out[11] = 60; // Keepalive 60s
  out[12] = (uint8_t)(idLen >> 8);
  out[13] = (uint8_t)(idLen & 0xFF);
  memcpy(&out[14], clientId, idLen);
  return remLen + 2;
}

inline size_t buildMqttPublishPacket(const char* topic, const char* payload, uint8_t* out, size_t maxLen) {
  if (!topic || !payload || !out) return 0;
  size_t tLen = strlen(topic);
  size_t pLen = strlen(payload);
  size_t remLen = 2 + tLen + pLen;
  if (remLen > 250 || maxLen < remLen + 4) return 0;
  size_t idx = 0;
  out[idx++] = 0x30; // PUBLISH (QoS 0)
  if (remLen < 128) {
    out[idx++] = (uint8_t)remLen;
  } else {
    out[idx++] = (uint8_t)((remLen & 0x7F) | 0x80);
    out[idx++] = (uint8_t)(remLen >> 7);
  }
  out[idx++] = (uint8_t)(tLen >> 8);
  out[idx++] = (uint8_t)(tLen & 0xFF);
  memcpy(&out[idx], topic, tLen);
  idx += tLen;
  memcpy(&out[idx], payload, pLen);
  idx += pLen;
  return idx;
}
