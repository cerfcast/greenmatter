#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "include/current.h"
#include "include/green.h"
#include "sockfilter.h"

#if EXTENDED_ECHO
#define ICMP_MSG_BODY_LEN                                                      \
  (sizeof(struct extended_echo_request_wire) +                                 \
   sizeof(struct icmp_extension_header_wire) +                                 \
   sizeof(struct icmp_extension_object_base_wire) +                            \
   sizeof(struct icmp_extended_echo_address_object_wire))
#else
  auto icmp_type{ICMP_TIME_EXCEEDED};
#define ICMP_MSG_BODY_LEN                                                      \
  (sizeof(struct extended_time_exceeded) +                                     \
   sizeof(struct icmp_extension_header) +                                      \
   sizeof(struct icmp_extension_object_base) +                                 \
   sizeof(struct icmp_green_object))
#endif

bool green_processor(const struct so_event *evt, size_t, void *) {
  const size_t PACKET_SIZE{ICMP_MSG_BODY_LEN};
  (void)PACKET_SIZE;

  std::cout << "I am in the packet processor.\n";

  const uint8_t *captured_icmp_data{evt->data};
  const extended_echo_request_wire *eer{reinterpret_cast<const extended_echo_request_wire*>(captured_icmp_data)};
  (void)eer;
  captured_icmp_data += sizeof(extended_echo_request_wire);

  const icmp_extension_header_wire *ext_hdr{reinterpret_cast<const icmp_extension_header_wire*>(captured_icmp_data)};
  (void)ext_hdr;
  captured_icmp_data += sizeof(icmp_extension_header_wire);

  const icmp_extension_object_base_wire *ext_base{reinterpret_cast<const icmp_extension_object_base_wire*>(captured_icmp_data)};
  (void)ext_base;
  captured_icmp_data += sizeof(icmp_extension_object_base_wire);

  const icmp_extended_echo_address_object_wire *ext_addr{reinterpret_cast<const icmp_extended_echo_address_object_wire*>(captured_icmp_data)};

  auto destinationId{static_cast<uint32_t>(ext_addr->address>>8)};
  auto endpointId{static_cast<uint32_t>(ext_addr->address & 0xFF)};
  std::cout << "Using the specified destination Id (" << std::hex << destinationId << ") ";
  std::cout << "and endpoint id (" << std::hex << endpointId << ") to query for power usage (via Matter) ...\n";

    auto result = currentCurrent(destinationId, endpointId);
    std::cout << "Result: " << result << "\n";
  return true;
}

int main(int argc, char **argv) {

  // A very, very lazy hack: _any_ cli argument will trigger
  // the tool to be a responder.
  if (argc > 1) {
    filter(green_processor);
    return 0;
  }

  // Invoking this tool with no cli arguments will launch
  // the tool as a client.

  // Note: All options are configurable only at the time
  // the program is built. Of course, that will change. But ...

  char target_ip[]{"10.63.1.63"};
  char source_ip[]{"192.168.124.1"};

  int raws = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
  if (raws < 0) {
    return handle_error(errno, "opening a raw socket");
  }

  uint8_t pkt_data[sizeof(struct iphdr) + ICMP_MSG_BODY_LEN] = {
      0,
  };

  pkt_data[sizeof(struct iphdr) + ICMP_MSG_BODY_LEN - 1] = 0xff;
  struct sockaddr_in target {};
  struct sockaddr_in source {};
  struct sockaddr_in localhost {};

  target.sin_family = AF_INET;
  source.sin_family = AF_INET;
  localhost.sin_family = AF_INET;

  auto inet_aton_r0 =
      inet_aton("127.0.0.1", (struct in_addr *)&localhost.sin_addr);
  (void)inet_aton_r0;

  auto inet_aton_r1 = inet_aton(target_ip, &target.sin_addr);
  (void)inet_aton_r1;

  auto inet_aton_r2 = inet_aton(source_ip, &source.sin_addr);
  (void)inet_aton_r2;

  // Configure the IP header.
  struct iphdr *iph = (struct iphdr *)pkt_data;
  iph->version = 4;
  iph->ihl = 5;
  iph->frag_off = 0;
  iph->ttl = 64;
  iph->protocol = IPPROTO_ICMP;
  memcpy((char *)&iph->daddr, &target.sin_addr, sizeof(uint32_t));
  memcpy((char *)&iph->saddr, &source.sin_addr, sizeof(uint32_t));

  // Use memcpy_target as an always-up-to-date pointer to where
  // to write the next data.
  uint8_t *memcpy_target{pkt_data + sizeof(struct iphdr)};
#if EXTENDED_ECHO
  // Craft the base of the extended echo request ICMP packet.
  struct extended_echo_request eer {};
  eer.type = ICMP_EXT_ECHO;
  eer.identifier = htons(0x1234);
  eer.seq = 56;
  eer.checksum = 0;
  // Note: Intentionally setting this field to a non-zero value
  // to make the packets "stand out" in Wireshark.
  eer.reserved = 3;
  eer.l = 1;
  memcpy_target +=
      extended_echo_request_builder::into_bytes(&eer, memcpy_target);

  // Build the extension header.
  struct icmp_extension_header_wire ext_hdr {};
  ext_hdr.version = 2;
  ext_hdr.reserved0 = 0;
  ext_hdr.reserved1 = 0;
  memcpy(memcpy_target, &ext_hdr, sizeof(struct icmp_extension_header_wire));
  memcpy_target += sizeof(struct icmp_extension_header_wire);

  // Build the base of an extension object.
  struct icmp_extension_object_base_wire ext_base {};
  ext_base.class_num = 3;
  ext_base.c_type = 3;
  ext_base.length = htons(
      sizeof(struct icmp_extended_echo_address_object_wire) +
      2 * sizeof(uint16_t)); // The 2 uint16_ts here are because the length
                             // includes the space needed for the class number and type.
  memcpy(memcpy_target, &ext_base,
         sizeof(struct icmp_extension_object_base_wire));
  memcpy_target += sizeof(struct icmp_extension_object_base_wire);

  // Finally, we get to do some fun work!
  struct icmp_extended_echo_address_object_wire ext_address {};
  ext_address.afi = htons(1); // Indicate that address object is IPv4.
  ext_address.length = 4;
  ext_address.address = 0x0f02; // A request for the active current for the 
                                // matter node with descriminator 0xf on the
                                // fabric with the endpoint 1.
  memcpy(memcpy_target, &ext_address,
         sizeof(struct icmp_extended_echo_address_object_wire));
  memcpy_target += sizeof(struct icmp_extended_echo_address_object_wire);

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

  struct extended_echo_request_wire *final_hdr{
      reinterpret_cast<struct extended_echo_request_wire *>(pkt_data +
                                                            sizeof(iphdr))};
  final_hdr->checksum =
      htons(checksum((uint8_t *)final_hdr, ICMP_MSG_BODY_LEN));

  auto sendto_result = sendto(raws, pkt_data, sizeof(pkt_data), 0,
                              (sockaddr *)&target, sizeof(sockaddr_in));

  if (sendto_result < 0) {
    handle_error(errno, "sending icmp packet");
  }

  std::cout << "Success!\n";

  /*
  */
  return 0;
}
