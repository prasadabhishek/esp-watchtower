#include <unity.h>
#include "watchtower_logic.h"

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_mac_parser_colon(void) {
  uint8_t mac[6];
  bool ok = parseMacAddress("AA:BB:CC:11:22:33", mac);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_HEX8(0xAA, mac[0]);
  TEST_ASSERT_EQUAL_HEX8(0xBB, mac[1]);
  TEST_ASSERT_EQUAL_HEX8(0xCC, mac[2]);
  TEST_ASSERT_EQUAL_HEX8(0x11, mac[3]);
  TEST_ASSERT_EQUAL_HEX8(0x22, mac[4]);
  TEST_ASSERT_EQUAL_HEX8(0x33, mac[5]);
}

void test_mac_parser_hyphen(void) {
  uint8_t mac[6];
  bool ok = parseMacAddress("00-14-22-01-23-45", mac);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_HEX8(0x00, mac[0]);
  TEST_ASSERT_EQUAL_HEX8(0x14, mac[1]);
  TEST_ASSERT_EQUAL_HEX8(0x22, mac[2]);
  TEST_ASSERT_EQUAL_HEX8(0x01, mac[3]);
  TEST_ASSERT_EQUAL_HEX8(0x23, mac[4]);
  TEST_ASSERT_EQUAL_HEX8(0x45, mac[5]);
}

void test_mac_parser_invalid(void) {
  uint8_t mac[6];
  TEST_ASSERT_FALSE(parseMacAddress("not-a-mac", mac));
  TEST_ASSERT_FALSE(parseMacAddress("", mac));
  TEST_ASSERT_FALSE(parseMacAddress(NULL, mac));
  TEST_ASSERT_FALSE(parseMacAddress("AA:BB:CC", mac));
}

void test_magic_packet_structure(void) {
  uint8_t mac[6] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
  uint8_t packet[102];
  bool ok = buildMagicPacket(mac, packet);
  TEST_ASSERT_TRUE(ok);

  // Check sync stream: 6 bytes of 0xFF
  for (int i = 0; i < 6; i++) {
    TEST_ASSERT_EQUAL_HEX8(0xFF, packet[i]);
  }

  // Check 16 repetitions of MAC
  for (int rep = 0; rep < 16; rep++) {
    for (int b = 0; b < 6; b++) {
      TEST_ASSERT_EQUAL_HEX8(mac[b], packet[6 + rep * 6 + b]);
    }
  }
}

void test_ip_helpers(void) {
  uint8_t ip[4];
  bool ok = parseIpAddress("192.168.50.119", ip);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT8(192, ip[0]);
  TEST_ASSERT_EQUAL_UINT8(168, ip[1]);
  TEST_ASSERT_EQUAL_UINT8(50, ip[2]);
  TEST_ASSERT_EQUAL_UINT8(119, ip[3]);

  char buf[16];
  ipToString(ip, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("192.168.50.119", buf);

  TEST_ASSERT_FALSE(parseIpAddress("999.1.1.1", ip));
  TEST_ASSERT_FALSE(parseIpAddress("invalid", ip));
}

void test_lan_device_upsert_and_state(void) {
  LanDevice list[10];
  uint8_t count = 0;
  uint8_t ip1[4] = {192, 168, 50, 119};
  uint8_t mac1[6] = {0xb0, 0xde, 0x28, 0x30, 0xc1, 0x6a};
  bool cameOnline = false, isNew = false;

  // Insert device 1
  bool ok = upsertDeviceRecord(list, count, 10, ip1, mac1, 5, 100, cameOnline, isNew);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT8(1, count);
  TEST_ASSERT_TRUE(isNew);
  TEST_ASSERT_TRUE(cameOnline);
  TEST_ASSERT_TRUE(list[0].is_online);
  TEST_ASSERT_EQUAL_STRING("Apple Inc.", list[0].vendor);

  // Mark missed sweeps until offline (threshold = 3)
  bool wentOffline = false;
  markDeviceSweepResult(list, count, ip1, false, -1, 150, 3, wentOffline, cameOnline);
  TEST_ASSERT_FALSE(wentOffline);
  TEST_ASSERT_TRUE(list[0].is_online);

  markDeviceSweepResult(list, count, ip1, false, -1, 200, 3, wentOffline, cameOnline);
  TEST_ASSERT_FALSE(wentOffline);

  markDeviceSweepResult(list, count, ip1, false, -1, 250, 3, wentOffline, cameOnline);
  TEST_ASSERT_TRUE(wentOffline);
  TEST_ASSERT_FALSE(list[0].is_online);

  // Comes back online
  markDeviceSweepResult(list, count, ip1, true, 4, 300, 3, wentOffline, cameOnline);
  TEST_ASSERT_TRUE(cameOnline);
  TEST_ASSERT_TRUE(list[0].is_online);
  TEST_ASSERT_EQUAL_INT(4, list[0].latency_ms);
}

void test_vip_tagging(void) {
  LanDevice list[5];
  uint8_t count = 0;
  uint8_t ip[4] = {192, 168, 50, 85};
  uint8_t mac[6] = {0x00, 0x24, 0xe4, 0x8a, 0x86, 0x2a};
  bool cameOnline = false, isNew = false;

  upsertDeviceRecord(list, count, 5, ip, mac, 2, 50, cameOnline, isNew);
  TEST_ASSERT_EQUAL_STRING("Synology NAS", list[0].vendor);
  TEST_ASSERT_FALSE(list[0].is_vip);

  // Toggle VIP with custom name
  bool ok = toggleVipStatus(list, count, ip, true, "Home Storage NAS");
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_TRUE(list[0].is_vip);
  TEST_ASSERT_EQUAL_STRING("Home Storage NAS", list[0].name);
}

void test_outage_configurable_threshold(void) {
  unsigned long outageStartMs = 0;
  uint32_t thresholdSec = 300; // 5 minutes = 300,000 ms
  bool isCurrentlyDown = false;
  bool shouldAlertDown = false;
  bool shouldAlertRestored = false;
  uint32_t totalDurationSec = 0;

  // Intermittent failure at t=10s: should NOT alert
  evaluateOutageCondition(false, 10000, outageStartMs, thresholdSec, isCurrentlyDown, shouldAlertDown, shouldAlertRestored, totalDurationSec);
  TEST_ASSERT_FALSE(shouldAlertDown);
  TEST_ASSERT_FALSE(isCurrentlyDown);

  // Recovery at t=20s: resets intermittent start
  evaluateOutageCondition(true, 20000, outageStartMs, thresholdSec, isCurrentlyDown, shouldAlertDown, shouldAlertRestored, totalDurationSec);
  TEST_ASSERT_EQUAL_UINT32(0, outageStartMs);
  TEST_ASSERT_FALSE(isCurrentlyDown);

  // Real outage begins at t=30s
  evaluateOutageCondition(false, 30000, outageStartMs, thresholdSec, isCurrentlyDown, shouldAlertDown, shouldAlertRestored, totalDurationSec);
  TEST_ASSERT_EQUAL_UINT32(30000, outageStartMs);
  TEST_ASSERT_FALSE(shouldAlertDown);

  // t=200s (under 300s): still no alert
  evaluateOutageCondition(false, 200000, outageStartMs, thresholdSec, isCurrentlyDown, shouldAlertDown, shouldAlertRestored, totalDurationSec);
  TEST_ASSERT_FALSE(shouldAlertDown);
  TEST_ASSERT_FALSE(isCurrentlyDown);

  // t=330s (threshold reached 300s since t=30s): ALERTS DOWN
  evaluateOutageCondition(false, 330000, outageStartMs, thresholdSec, isCurrentlyDown, shouldAlertDown, shouldAlertRestored, totalDurationSec);
  TEST_ASSERT_TRUE(shouldAlertDown);
  TEST_ASSERT_TRUE(isCurrentlyDown);

  // t=400s (still down): does not re-alert down
  evaluateOutageCondition(false, 400000, outageStartMs, thresholdSec, isCurrentlyDown, shouldAlertDown, shouldAlertRestored, totalDurationSec);
  TEST_ASSERT_FALSE(shouldAlertDown);
  TEST_ASSERT_TRUE(isCurrentlyDown);

  // t=630s: Internet RESTORED (total downtime 600 seconds = 10 minutes)
  evaluateOutageCondition(true, 630000, outageStartMs, thresholdSec, isCurrentlyDown, shouldAlertDown, shouldAlertRestored, totalDurationSec);
  TEST_ASSERT_TRUE(shouldAlertRestored);
  TEST_ASSERT_FALSE(isCurrentlyDown);
  TEST_ASSERT_EQUAL_UINT32(600, totalDurationSec);
}

void test_netbios_parser(void) {
  uint8_t sample[80];
  memset(sample, 0, sizeof(sample));
  sample[56] = 1; // 1 name
  memcpy(&sample[57], "APOLLOSYNOLOGY ", 15);
  sample[72] = 0x00; // Workstation Service

  char name[20];
  bool ok = parseNetbiosName(sample, sizeof(sample), name, sizeof(name));
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("APOLLOSYNOLOGY", name);

  // Invalid length
  TEST_ASSERT_FALSE(parseNetbiosName(sample, 50, name, sizeof(name)));
  // Zero names
  sample[56] = 0;
  TEST_ASSERT_FALSE(parseNetbiosName(sample, sizeof(sample), name, sizeof(name)));
}

void test_blame_meter(void) {
  // Normal
  TEST_ASSERT_EQUAL_INT(BLAME_OK, diagnoseNetworkLag(2, 25, 0));
  // ISP Down
  TEST_ASSERT_EQUAL_INT(BLAME_ISP_DOWN, diagnoseNetworkLag(2, -1, 0));
  TEST_ASSERT_EQUAL_INT(BLAME_ISP_DOWN, diagnoseNetworkLag(2, 25, 1));
  // ISP Lag
  TEST_ASSERT_EQUAL_INT(BLAME_ISP_LAG, diagnoseNetworkLag(5, 180, 0));
  // Router / Wi-Fi Congestion
  TEST_ASSERT_EQUAL_INT(BLAME_ROUTER_LAG, diagnoseNetworkLag(85, 95, 0));
  // Both Offline
  TEST_ASSERT_EQUAL_INT(BLAME_OFFLINE, diagnoseNetworkLag(-1, -1, 0));
}

void test_outage_claim_receipt(void) {
  char buf[512];
  bool ok = formatOutageClaimReceipt(buf, sizeof(buf), "Comcast Xfinity", "ACC-998822", 3, 720, 15, 2, -1);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_NOT_NULL(strstr(buf, "Comcast Xfinity"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "ACC-998822"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "Recorded Outages: 3"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "Total Failed Checks: 15"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "Latest Outage Duration: 12m 0s"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "Gateway RTT: 2 ms"));
}

void test_dns_probe_packets(void) {
  uint8_t pkt[64];
  size_t len = buildDnsQueryPacket("example.com", 0x1337, pkt, sizeof(pkt));
  TEST_ASSERT_EQUAL_UINT32(29, len);
  TEST_ASSERT_EQUAL_HEX8(0x13, pkt[0]);
  TEST_ASSERT_EQUAL_HEX8(0x37, pkt[1]);
  TEST_ASSERT_EQUAL_HEX8(7, pkt[12]); // "example" length 7

  // Simulate valid NXDOMAIN response (RCODE = 3, ANCOUNT = 0)
  uint8_t resp[12] = {0x13, 0x37, 0x81, 0x83, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t rcode = 0;
  uint16_t answers = 99;
  TEST_ASSERT_TRUE(parseDnsResponse(resp, 12, 0x1337, rcode, answers));
  TEST_ASSERT_EQUAL_UINT8(3, rcode);
  TEST_ASSERT_EQUAL_UINT16(0, answers);
}

void test_micro_ntp_server(void) {
  uint8_t req[48];
  memset(req, 0, 48);
  req[0] = 0x23; // VN=4, Mode=3 (Client)
  req[2] = 6;    // Poll
  req[40] = 0xAA; req[47] = 0x55; // Client transmit timestamp

  uint8_t resp[48];
  TEST_ASSERT_TRUE(buildNtpResponse(req, 1700000000UL, resp));
  TEST_ASSERT_EQUAL_HEX8(0x24, resp[0]); // VN=4, Mode=4 (Server)
  TEST_ASSERT_EQUAL_UINT8(2, resp[1]);   // Stratum 2
  TEST_ASSERT_EQUAL_HEX8(0xAA, resp[24]); // Origin timestamp copied from client transmit
  TEST_ASSERT_EQUAL_HEX8(0x55, resp[31]);
}

void test_mqtt_telemetry_packets(void) {
  uint8_t buf[128];
  size_t cLen = buildMqttConnectPacket("watchtower", buf, sizeof(buf));
  TEST_ASSERT_EQUAL_UINT32(24, cLen);
  TEST_ASSERT_EQUAL_HEX8(0x10, buf[0]); // CONNECT

  size_t pLen = buildMqttPublishPacket("watchtower/status", "{\"online\":26}", buf, sizeof(buf));
  TEST_ASSERT_TRUE(pLen > 20);
  TEST_ASSERT_EQUAL_HEX8(0x30, buf[0]); // PUBLISH
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_mac_parser_colon);
  RUN_TEST(test_mac_parser_hyphen);
  RUN_TEST(test_mac_parser_invalid);
  RUN_TEST(test_magic_packet_structure);
  RUN_TEST(test_ip_helpers);
  RUN_TEST(test_lan_device_upsert_and_state);
  RUN_TEST(test_vip_tagging);
  RUN_TEST(test_outage_configurable_threshold);
  RUN_TEST(test_netbios_parser);
  RUN_TEST(test_blame_meter);
  RUN_TEST(test_outage_claim_receipt);
  RUN_TEST(test_dns_probe_packets);
  RUN_TEST(test_micro_ntp_server);
  RUN_TEST(test_mqtt_telemetry_packets);
  return UNITY_END();
}
