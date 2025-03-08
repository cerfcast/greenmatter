#ifndef _GREEN_H
#define _GREEN_H

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstdint>
#include <memory.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

struct iphdr make_fake_packet();

template <typename T, typename U> T to_words(U bytes) {
  return static_cast<T>(bytes / 4);
}

/* TODO: Fix */
uint16_t checksum(uint8_t *bytes, int len);

struct __attribute__((packed)) icmp_extended_echo_address_object_wire {
  uint16_t afi;
  uint8_t length;
  uint8_t reserved;
  uint32_t address; // assume ipv4
};

struct __attribute__((packed)) icmp_green_node_power_draw_wire {
  uint32_t present;
  uint32_t idle;
};

struct __attribute__((packed)) icmp_green_object_wire {
  union {
    struct icmp_green_node_power_draw_wire power_draw;
  };
};

struct __attribute__((packed)) icmp_extension_object_base_wire {
  uint16_t length;
  uint8_t class_num;
  uint8_t c_type;
};

struct __attribute__((packed)) icmp_extension_header_wire {
  uint8_t reserved0 : 4;
  uint8_t version : 4;
  uint8_t reserved1;
  uint16_t checksum;
};

struct __attribute__((packed)) extended_time_exceeded_wire {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint8_t unused;
  uint8_t length;
  uint16_t unused2;
  uint8_t body[128];
};

struct __attribute__((packed)) extended_echo_request_wire {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t identifier;
  uint8_t seq;
  uint8_t l : 1;
  uint8_t reserved : 7;
};

struct icmp_extended_echo_address_object {
  uint16_t afi;
  uint8_t length;
  uint8_t reserved;
  uint32_t address; // assume ipv4
};

struct  icmp_green_node_power_draw {
  uint32_t present;
  uint32_t idle;
};

struct icmp_green_object {
  union {
    struct icmp_green_node_power_draw power_draw;
  };
};

struct icmp_extension_object_base {
  uint16_t length;
  uint8_t class_num;
  uint8_t c_type;
};

struct icmp_extension_header {
  uint8_t reserved0 : 4;
  uint8_t version : 4;
  uint8_t reserved1;
  uint16_t checksum;
};

struct extended_time_exceeded {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint8_t unused;
  uint8_t length;
  uint16_t unused2;
  uint8_t body[128];
};

struct extended_echo_request {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t identifier;
  uint8_t seq;
  uint8_t l : 1;
  uint8_t reserved : 7;
};

class extended_echo_request_builder {
    public:
        static extended_echo_request from_bytes(uint8_t *bytes) {
            struct extended_echo_request_wire *wire{reinterpret_cast<struct extended_echo_request_wire*>(bytes)};
            struct extended_echo_request result{};

            result.checksum = wire->checksum;
            result.type = wire->type;
            result.code = wire->code;
            result.identifier = wire->identifier;
            result.l = wire->l;
            result.reserved = wire->reserved;

            return result;
        }
        static size_t into_bytes(struct extended_echo_request *eer, uint8_t *bytes) {
            struct extended_echo_request_wire *wire{reinterpret_cast<extended_echo_request_wire*>(bytes)};

            wire->checksum = eer->checksum;
            wire->type = eer->type;
            wire->code = eer->code;
            wire->identifier = eer->identifier;
            wire->l = eer->l;
            wire->reserved = eer->reserved;
            wire->seq = eer->seq;

            return sizeof(struct extended_echo_request_wire);
        }
};

extern int errno;

int handle_error(int err, std::string when);

#define GREEN_OBJECT_CLASS 4

#define GREEN_NODE_POWER_DRAW_CTYPE 1

#define EXTENDED_ECHO 1
#define TIME_EXCEEDED 1


void checksum_test_odd();

#endif
