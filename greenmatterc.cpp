#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstdint>
#include <iostream>
#include <memory.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "include/current.h"

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

template <typename T, typename U> T to_words(U bytes) {
  return static_cast<T>(bytes / 4);
}

/* TODO: Fix */
uint16_t checksum(uint8_t *bytes, int len) {
  uint32_t res{};

  for (int i = 0; i < len; i += 2) {
    uint16_t v = (bytes[i] << 8) + bytes[i + 1];
    res += v;
  }

  uint16_t fifth = (res >> 16);
  // res += fifth;
  fifth = (res >> 16);
  uint16_t rres{static_cast<uint16_t>(res + fifth)};
  rres = ~rres;
  return rres;
}

struct __attribute__((packed)) icmp_extended_echo_address_object {
  uint16_t afi;
  uint8_t length;
  uint8_t reserved;
  uint32_t address; // assume ipv4
};

struct __attribute__((packed)) icmp_green_node_power_draw {
  uint32_t present;
  uint32_t idle;
};

struct __attribute__((packed)) icmp_green_object {
  union {
    struct icmp_green_node_power_draw power_draw;
  };
};

struct __attribute__((packed)) icmp_extension_object_base {
  uint16_t length;
  uint8_t class_num;
  uint8_t c_type;
};

struct __attribute__((packed)) icmp_extension_header {
  uint8_t reserved0 : 4;
  uint8_t version : 4;
  uint8_t reserved1;
  uint16_t checksum;
};

struct __attribute__((packed)) extended_time_exceeded {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint8_t unused;
  uint8_t length;
  uint16_t unused2;
  uint8_t body[128];
};

struct __attribute__((packed)) extended_echo_request {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t identifier;
  uint8_t seq;
  uint8_t l : 1;
  uint8_t reserved : 7;
};

extern int errno;

int handle_error(int err, std::string when) {
  char *err_str = strerror(err);
  std::cout << "There was an error " << when << ": " << err_str << "\n";
  return -1;
}

#define GREEN_OBJECT_CLASS 4

#define GREEN_NODE_POWER_DRAW_CTYPE 1

#define EXTENDED_ECHO 1
#define TIME_EXCEEDED 1

int main() {

  // options
  bool to_broadcast{true};
  char broadcast_target[]{"224.0.0.252"};
  //char broadcast_target[]{"255.255.255.255"};

  //char non_broadcast_target[]{"127.0.0.1"};
  char non_broadcast_target[]{"192.168.124.76"};

  //char source_ip[]{"255.255.255.255"};
  //char source_ip[]{"224.0.0.252"};
  char source_ip[]{"192.168.124.1"};

  int raws = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
  if (raws < 0) {
    return handle_error(errno, "opening a raw socket");
  }
#if EXTENDED_ECHO
  auto icmp_type{ICMP_EXT_ECHO};
#define ICMP_MSG_BODY_LEN                                                      \
  (sizeof(struct extended_echo_request) +                                      \
   sizeof(struct icmp_extension_header) +                                      \
   sizeof(struct icmp_extension_object_base) +                                 \
   sizeof(struct icmp_extended_echo_address_object))
#else
  auto icmp_type{ICMP_TIME_EXCEEDED};
#define ICMP_MSG_BODY_LEN                                                      \
  (sizeof(struct extended_time_exceeded) +                                     \
   sizeof(struct icmp_extension_header) +                                      \
   sizeof(struct icmp_extension_object_base) +                                 \
   sizeof(struct icmp_green_object))
#endif

  char pkt_data[sizeof(struct iphdr) + ICMP_MSG_BODY_LEN] = {
      0,
  };

  struct sockaddr_in target {};
  struct sockaddr_in source {};

  target.sin_family = AF_INET;
  source.sin_family = AF_INET;

  auto inet_aton_r2 = inet_aton(source_ip, &source.sin_addr);
	(void)inet_aton_r2;

  if (to_broadcast) {
    uint32_t broadcast_val{1};
    int setsockopt_res = setsockopt(raws, SOL_SOCKET, SO_BROADCAST,
                                    &broadcast_val, sizeof(uint32_t));
    if (setsockopt_res < 0) {
      return handle_error(errno, "configuring port to send to broadcast");
    }
    auto inet_aton_r0 = inet_aton(broadcast_target, &target.sin_addr);
		(void)inet_aton_r0;
  } else {
    auto inet_aton_r1 = inet_aton(non_broadcast_target, &target.sin_addr);
		(void)inet_aton_r1;
  }

  struct iphdr *iph = (struct iphdr *)pkt_data;

  iph->version = 4;
  iph->ihl = 5;
  iph->frag_off = 0;
  iph->ttl = 64;
  iph->protocol = IPPROTO_ICMP;
  memcpy((char *)&iph->daddr, &target.sin_addr, sizeof(uint32_t));
  memcpy((char *)&iph->saddr, &source.sin_addr, sizeof(uint32_t));

#if EXTENDED_ECHO
  struct extended_echo_request *hdr =
      (struct extended_echo_request *)(pkt_data + sizeof(struct iphdr));

  hdr->type = icmp_type;

  hdr->identifier = htons(0x1234);
  hdr->seq = htons(0x5678);
  hdr->checksum = 0;
  hdr->reserved = 3;
  hdr->l = 1;
  struct icmp_extension_header *ext_hdr =
      (struct icmp_extension_header *)(pkt_data + sizeof(struct iphdr) +
                                       sizeof(struct extended_echo_request));
  ext_hdr->version = 2;
  ext_hdr->reserved0 = 0;
  ext_hdr->reserved1 = 0;

  struct icmp_extension_object_base *ext_base =
      (struct icmp_extension_object_base *)(pkt_data + sizeof(struct iphdr) +
                                            sizeof(
                                                struct extended_echo_request) +
                                            sizeof(
                                                struct icmp_extension_header));
  ext_base->class_num = 3;
  ext_base->c_type = 3;
  ext_base->length = htons(sizeof(struct icmp_extended_echo_address_object) +
                           2 * sizeof(uint16_t));

  struct icmp_extended_echo_address_object *ext_address =
      (struct icmp_extended_echo_address_object
           *)(pkt_data + sizeof(struct iphdr) +
              sizeof(struct extended_echo_request) +
              sizeof(struct icmp_extension_header) +
              sizeof(struct icmp_extension_object_base));

  ext_address->afi = htons(1);
  ext_address->length = 4;
  inet_aton("127.0.0.1", (struct in_addr *)&ext_address->address);
#else
  struct extended_time_exceeded *hdr =
      (struct extended_time_exceeded *)(pkt_data + sizeof(struct iphdr));

  hdr->type = icmp_type;
  hdr->checksum = 0;
  hdr->code = 0;
  hdr->length = to_words<uint8_t>(sizeof(hdr->body));

  struct iphdr fake_iphdr {
    make_fake_packet()
  };
  memcpy(&hdr->body, &fake_iphdr, sizeof(struct iphdr));

  struct icmp_extension_header *ext_hdr =
      (struct icmp_extension_header *)(pkt_data + sizeof(struct iphdr) +
                                       sizeof(struct extended_time_exceeded));
  ext_hdr->version = 2;
  // Set to 0 here because we are overwriting data that was previously set.
  ext_hdr->reserved0 = 0;
  ext_hdr->reserved1 = 0;

  struct icmp_extension_object_base *ext_base =
      (struct icmp_extension_object_base *)(pkt_data + sizeof(struct iphdr) +
                                            sizeof(
                                                struct extended_time_exceeded) +
                                            sizeof(
                                                struct icmp_extension_header));
  ext_base->class_num = GREEN_OBJECT_CLASS;
  ext_base->c_type = GREEN_NODE_POWER_DRAW_CTYPE;
  ext_base->length =
      htons(sizeof(struct icmp_green_object) + sizeof(ext_base->class_num) +
            sizeof(ext_base->c_type));

  struct icmp_green_object *green_object =
      (struct icmp_green_object *)(pkt_data + sizeof(struct iphdr) +
                                   sizeof(struct extended_time_exceeded) +
                                   sizeof(struct icmp_extension_header) +
                                   sizeof(struct icmp_extension_object_base));
  green_object->power_draw.present = htonl(72);
  green_object->power_draw.idle = htonl(72);

#endif

  hdr->checksum = htons(checksum((uint8_t *)hdr, ICMP_MSG_BODY_LEN));

  auto sendto_result = sendto(raws, pkt_data, sizeof(pkt_data), 0,
                              (sockaddr *)&target, sizeof(sockaddr_in));

  if (sendto_result < 0) {
    handle_error(errno, "sending icmp packet");
  }

  std::cout << "Success!\n";

  auto result = currentCurrent();

  std::cout << "Result: " << result << "\n";

  return 0;
}
