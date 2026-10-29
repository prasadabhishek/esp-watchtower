#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>

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
inline bool evaluateOutageCondition(bool pingOk, unsigned long nowMs, unsigned long& outageStartMs,
                                   uint32_t thresholdSec, bool& isCurrentlyDown,
                                   bool& shouldAlertDown, bool& shouldAlertRestored,
                                   uint32_t& totalOutageDurationSec) {
  shouldAlertDown = false;
  shouldAlertRestored = false;
  totalOutageDurationSec = 0;

  if (!pingOk) {
    if (outageStartMs == 0) {
      outageStartMs = nowMs;
    }
    if (!isCurrentlyDown && (nowMs - outageStartMs >= thresholdSec * 1000UL)) {
      isCurrentlyDown = true;
      shouldAlertDown = true;
      return true;
    }
  } else {
    if (isCurrentlyDown) {
      totalOutageDurationSec = (uint32_t)((nowMs - outageStartMs) / 1000UL);
      isCurrentlyDown = false;
      shouldAlertRestored = true;
      outageStartMs = 0;
      return true;
    }
    outageStartMs = 0; // Reset intermittent failure
  }
  return false;
}

// --- LAN Device Structures & Radar Logic ---

struct LanDevice {
  uint8_t ip[4];
  uint8_t mac[6];
  char name[32];
  char vendor[20];
  bool is_vip;
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

inline void resolveVendor(const uint8_t mac[6], char* out, size_t maxLen) {
  if (!out || maxLen < 16) return;
  if (mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0) {
    strncpy(out, "Generic Client", maxLen);
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
