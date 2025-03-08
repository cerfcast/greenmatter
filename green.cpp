#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cstdint>
#include <iostream>
#include <memory.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "include/green.h"

struct iphdr make_fake_packet() {
  struct iphdr fake_iphdr {};

  fake_iphdr.ihl = 5;
  fake_iphdr.version = 4;
  fake_iphdr.ihl = 5;
  fake_iphdr.tot_len = htons(40);
  fake_iphdr.frag_off = 0;
  fake_iphdr.ttl = 61;
  fake_iphdr.protocol = IPPROTO_ICMP;

  return fake_iphdr;
}

uint16_t checksum(uint8_t *bytes, int len) {
  uint32_t res{};

  for (int i{0}; i + 1 < len; i+=2) {
    res += ((bytes[i] << 8) + bytes[i + 1]);
  }

  // If there is a lonely byte left, handle it.
  if (len % 2) {
    // Sensitive to the endianness of the host machine.
    res += (bytes[len-1]<<8);
  }

  // Incorporate carry-out end around.
  res = (res >> 16) + (res & 0xffff);           // Worst case: 0xffff + 0xffff
  uint16_t csum = (res >> 16) + (res & 0xffff); // Worst case: 0x1 + 0xfffe
  // ... but no more after that.                // Worst case: 0xffff

  return ~csum;
}

int handle_error(int err, std::string when) {
  char *err_str = strerror(err);
  std::cout << "There was an error " << when << ": " << err_str << "\n";
  return -1;
}

void checksum_test_even() {

}

void checksum_test_odd() {
  uint8_t bytes[5]{0x11, 0x22, 0x33, 0x44, 0x55};
  auto result = checksum(bytes, 5);
  std::cout << std::hex << "result: " << result << "\n";
}