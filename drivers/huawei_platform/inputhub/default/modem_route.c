/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: modem route source file
 * Author: DIVS_SENSORHUB
 * Create: 2012-05-29
 */

#include "modem_route.h"

#include <linux/device.h>
#include <linux/err.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/uaccess.h>

#include "contexthub_route.h"
#include <platform_include/smart/linux/base/ap/protocol.h>

#define MAX_RECV_MO_LEN 1023
static struct work_struct register_icc_read_work;
struct hw_ap_cp_route_t icc_route;

/* parse modem send to hub info */
static int process_modem_ap_prot(u32 channel_id, u32 len, void *context)
{
	int rc;
	int ret;
	char *modem_prot = NULL;
	struct pkt_header *mo_to_hub_data = NULL;
	struct modem_pkt_header_t *mo_info = NULL;
	size_t malloc_len;

	malloc_len = len + sizeof(struct pkt_header) + sizeof(mbox_msg_t);
	if ((malloc_len > MAX_RECV_MO_LEN) || (len == 0) ||
		!icc_route.icc_read) {
		hwlog_err("process modem info para err, len = %d\n", len);
		return 0;
	}
	if (!context)
		hwlog_debug("ignore context data null\n");
	modem_prot = kmalloc(malloc_len, GFP_ATOMIC);
	if (!modem_prot)
		return 0;

	ret = icc_route.icc_read(channel_id,
		(unsigned char *)modem_prot + sizeof(struct pkt_header), len);
	if ((u32)ret < len) {
		kfree(modem_prot);
		hwlog_info("%s size error = %d\n", __func__, len);
		return 0;
	}

	mo_info = (struct modem_pkt_header_t *)(modem_prot + sizeof(struct pkt_header));
	hwlog_info("%s get len = %d, mo_len = %d, tag = %d, cmd = %d\n",
		__func__, len, mo_info->length,
		mo_info->tag, mo_info->cmd);

	mo_to_hub_data = (struct pkt_header *)modem_prot;
	mo_to_hub_data->tag = TAG_MODEM;
	mo_to_hub_data->cmd = CMD_CMN_CONFIG_REQ;
	mo_to_hub_data->resp = NO_RESP;
	mo_to_hub_data->length = len;
	rc = send_modem_cmd_to_hub(modem_prot, len + sizeof(struct pkt_header));
	if (rc < 0)
		hwlog_err("%s, send modem info err\n", __func__);

	kfree(modem_prot);
	return 0;
}

static void register_icc_read_work_handler(struct work_struct *wk)
{
	hwlog_info("%s\n", __func__);

	if (!icc_route.icc_open) {
		hwlog_err("%s null ptr\n", __func__);
		return;
	}

	icc_route.icc_open(icc_route.channel_id, process_modem_ap_prot);
	hwlog_info("%s success!", __func__);
}

void connect_pcie_icc_sensorhub_route(struct hw_ap_cp_route_t *ap_cp_route)
{
	hwlog_info("%s\n", __func__);

	if (!ap_cp_route) {
		hwlog_err("%s null ptr\n", __func__);
		return;
	}
	memset(&icc_route, 0, sizeof(struct hw_ap_cp_route_t));
	memcpy(&icc_route, ap_cp_route, sizeof(struct hw_ap_cp_route_t));
	hwlog_info("%s, icc channel id = %d\n", __func__, icc_route.channel_id);
	INIT_WORK(&register_icc_read_work, register_icc_read_work_handler);
	schedule_work(&register_icc_read_work);
}
EXPORT_SYMBOL_GPL(connect_pcie_icc_sensorhub_route);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MOHub driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
