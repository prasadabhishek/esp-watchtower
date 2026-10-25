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

void test_outage_duration(void) {
  uint32_t mins = 0, secs = 0;
  // Outage of 754 seconds = 12 mins, 34 seconds
  bool ok = calculateOutageDuration(1000, 1754, mins, secs);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT32(12, mins);
  TEST_ASSERT_EQUAL_UINT32(34, secs);

  // Negative / invalid outage
  TEST_ASSERT_FALSE(calculateOutageDuration(2000, 1000, mins, secs));
}

void test_packet_loss_calculation(void) {
  TEST_ASSERT_EQUAL_UINT32(0, calculatePacketLossPercent(0, 0));
  TEST_ASSERT_EQUAL_UINT32(5, calculatePacketLossPercent(100, 5));
  TEST_ASSERT_EQUAL_UINT32(50, calculatePacketLossPercent(20, 10));
  TEST_ASSERT_EQUAL_UINT32(100, calculatePacketLossPercent(10, 15));
}

void test_uptime_formatting(void) {
  TEST_ASSERT_EQUAL_STRING("00:00:00", formatUptimeString(0).c_str());
  TEST_ASSERT_EQUAL_STRING("00:01:05", formatUptimeString(65).c_str());
  TEST_ASSERT_EQUAL_STRING("01:00:00", formatUptimeString(3600).c_str());
  TEST_ASSERT_EQUAL_STRING("25:30:15", formatUptimeString(91815).c_str());
}

void test_json_escaping(void) {
  std::string input = "User 'admin' said \"hello\"\nNext line";
  std::string expected = "User 'admin' said \\\"hello\\\"\\nNext line";
  TEST_ASSERT_EQUAL_STRING(expected.c_str(), escapeJsonString(input).c_str());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_mac_parser_colon);
  RUN_TEST(test_mac_parser_hyphen);
  RUN_TEST(test_mac_parser_invalid);
  RUN_TEST(test_magic_packet_structure);
  RUN_TEST(test_outage_duration);
  RUN_TEST(test_packet_loss_calculation);
  RUN_TEST(test_uptime_formatting);
  RUN_TEST(test_json_escaping);
  return UNITY_END();
}
