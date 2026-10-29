#pragma once
#include <Arduino.h>

#define FIRMWARE_VERSION "v1.0.0"
#define DEFAULT_AP_SSID  "ESP-Watchtower"
#define DEFAULT_AP_PASS  "sentinel123"
#define HOSTNAME         "watchtower"

#define TELNET_PORT      23
#define HTTP_TRAP_PORT   8080
#define HTTP_PORT        80

#define MAX_LOG_ENTRIES  60
#define PING_HISTORY_LEN 24
#define MAX_LAN_DEVICES  32

struct LogEntry {
  uint32_t timestamp; // Seconds since boot
  char type[16];      // HONEYPOT, DEAUTH, OUTAGE, DNS, WOL, SYSTEM
  char message[100];
  char severity[8];   // info, warn, danger, success
};

struct SystemMetrics {
  uint32_t uptime_sec;
  uint32_t free_heap;
  float vcc_volts;
  int8_t wifi_rssi;
  uint8_t wifi_channel;
  bool wifi_connected;
  char local_ip[16];
  char ap_ip[16];
  char mac_addr[18];
  
  // Watchdog
  int gateway_ping_ms;
  int internet_ping_ms;
  uint32_t total_pings;
  uint32_t failed_pings;
  uint32_t total_outages;
  uint32_t last_outage_duration;
  bool is_internet_down;
  
  // Radio Sniffer
  uint32_t total_deauth_packets;
  uint32_t total_disassoc_packets;
  uint32_t deauth_attacks_detected;
  uint32_t total_smartphone_probes;
  uint32_t broadcast_packets_per_sec;
  uint32_t spam_warnings_count;
  
  // Internal Devices & Watchdog
  uint8_t active_devices_count;
  bool watched_device_online;
  char watched_device_ip[16];
  bool dns_tampered;
  
  // Honeypot
  uint32_t honeypot_telnet_hits;
  uint32_t honeypot_web_hits;
  uint32_t honeypot_camera_hits;
  char last_attacker_ip[16];
  char last_attacker_time[16];
};
