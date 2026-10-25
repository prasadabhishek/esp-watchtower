#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <string>

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
inline bool calculateOutageDuration(int64_t prev_epoch, int64_t now_epoch, uint32_t& out_mins, uint32_t& out_secs) {
  if (now_epoch <= prev_epoch) return false;
  int64_t diff = now_epoch - prev_epoch;
  out_mins = (uint32_t)(diff / 60);
  out_secs = (uint32_t)(diff % 60);
  return true;
}

// --- Packet Loss Metric ---
inline uint32_t calculatePacketLossPercent(uint32_t total, uint32_t failed) {
  if (total == 0) return 0;
  if (failed >= total) return 100;
  return (failed * 100) / total;
}

// --- Time String Formatter ---
inline std::string formatUptimeString(uint32_t totalSeconds) {
  uint32_t hours = totalSeconds / 3600;
  uint32_t mins = (totalSeconds % 3600) / 60;
  uint32_t secs = totalSeconds % 60;
  char buf[32];
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hours, mins, secs);
  return std::string(buf);
}

// --- Discord Payload Sanitizer ---
inline std::string escapeJsonString(const std::string& input) {
  std::string output;
  for (char c : input) {
    if (c == '"') {
      output += "\\\"";
    } else if (c == '\\') {
      output += "\\\\";
    } else if (c == '\n') {
      output += "\\n";
    } else if (c == '\r') {
      output += "\\r";
    } else {
      output += c;
    }
  }
  return output;
}
