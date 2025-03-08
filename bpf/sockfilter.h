// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2022 Jacky Yin */
#ifndef __SOCKFILTER_H
#define __SOCKFILTER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <linux/types.h>

struct so_event {
	__be32 src_addr;
	__be32 dst_addr;
	union {
		__be32 ports;
		__be16 port16[2];
	};
	__u32 ip_proto;
	__u32 pkt_type;
	__u32 ifindex;
	uint8_t data[25]; // TODO: Make customizable.
};

#ifdef __cplusplus
extern "C" {
#endif
	typedef bool (*packet_processor_t)(const struct so_event *, size_t, void *);
	int filter(packet_processor_t);
#ifdef __cplusplus
}
#endif

#endif /* __SOCKFILTER_H */
