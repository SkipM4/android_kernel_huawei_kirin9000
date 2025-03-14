/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: contexthub ext log header file
 * Author: DIVS_SENSORHUB
 * Create: 2012-05-29
 */

#ifndef __IOMCU_EXT_LOG_H
#define __IOMCU_EXT_LOG_H

#include <linux/types.h>
#include <linux/spinlock.h>

#include <platform_include/smart/linux/base/ap/protocol.h>

struct inputhub_ext_notifier_node {
	struct list_head entry;
	int tag;
	int (*notify)(const struct pkt_header *data);
};

struct inputhub_ext_log_notifier {
	struct list_head head;
	spinlock_t lock;
};

typedef struct {
	struct pkt_header hd;
	uint8_t tag;
	uint8_t data[];
} ext_logger_req_t;

typedef struct {
	uint8_t type;
	uint16_t len;
	uint8_t data[];
} pedo_ext_logger_req_t;

int inputhub_ext_log_init(void);
int is_inputhub_ext_log_notify(const struct pkt_header *head);
int inputhub_ext_log_register_handler(int tag,
		int (*notify)(const struct pkt_header *head));

#endif
