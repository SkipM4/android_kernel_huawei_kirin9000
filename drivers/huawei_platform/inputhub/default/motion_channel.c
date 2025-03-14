/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: motion channel source file
 * Author: DIVS_SENSORHUB
 * Create: 2012-05-29
 */

#include "motion_channel.h"

#include <linux/err.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include <huawei_platform/inputhub/motionhub.h>

#include "contexthub_boot.h"
#include "contexthub_ext_log.h"
#include "contexthub_recovery.h"
#include "contexthub_route.h"
#include "sensor_info.h"
#include <platform_include/smart/linux/base/ap/protocol.h>

#define USER_WRITE_BUFFER_SIZE (1 + 2 * sizeof(int))
/* include MOTIONHUB_TYPE_POPUP_CAM */
#define MAX_SUPPORTED_MOTIONS_TYPE_CNT ((MOTION_TYPE_END) + 1)
#define SUPPORTED_MOTIONS_TYPE_NODE_PATH "/sensorhub/motion"
#define SUPPORTED_MOTIONS_TYPE_PROP "supported_motions_type"
#define MOTION_SUPPORTED_FLAG 1
#define MOTION_UNSUPPORTED_FLAG 0

static bool motion_status[MOTION_TYPE_END];
static int64_t motion_ref_cnt;
static u32 g_supported_motions_type[MAX_SUPPORTED_MOTIONS_TYPE_CNT];

struct motions_cmd_map {
	int mhb_ioctl_app_cmd;
	int motion_type;
	int tag;
	enum obj_cmd cmd;
	enum obj_sub_cmd subcmd;
};

static const struct motions_cmd_map motions_cmd_map_tab[] = {
	{ MHB_IOCTL_MOTION_START, -1, TAG_MOTION,
		CMD_CMN_OPEN_REQ, SUB_CMD_NULL_REQ },
	{ MHB_IOCTL_MOTION_STOP, -1, TAG_MOTION,
		CMD_CMN_CLOSE_REQ, SUB_CMD_NULL_REQ },
	{ MHB_IOCTL_MOTION_ATTR_START, -1, TAG_MOTION,
		CMD_CMN_CONFIG_REQ, SUB_CMD_MOTION_ATTR_ENABLE_REQ },
	{ MHB_IOCTL_MOTION_ATTR_STOP, -1, TAG_MOTION,
		CMD_CMN_CONFIG_REQ, SUB_CMD_MOTION_ATTR_DISABLE_REQ },
	{ MHB_IOCTL_MOTION_INTERVAL_SET, -1, TAG_MOTION,
		CMD_CMN_INTERVAL_REQ, SUB_CMD_NULL_REQ },
};

static char *motion_type_str[] = {
	[MOTION_TYPE_START] = "start",
	[MOTION_TYPE_PICKUP] = "pickup",
	[MOTION_TYPE_FLIP] = "flip",
	[MOTION_TYPE_PROXIMITY] = "proximity",
	[MOTION_TYPE_SHAKE] = "shake",
	[MOTION_TYPE_TAP] = "tap",
	[MOTION_TYPE_TILT_LR] = "tilt_lr",
	[MOTION_TYPE_ROTATION] = "rotation",
	[MOTION_TYPE_POCKET] = "pocket",
	[MOTION_TYPE_ACTIVITY] = "activity",
	[MOTION_TYPE_TAKE_OFF] = "take_off",
	[MOTION_TYPE_EXTEND_STEP_COUNTER] = "ext_step_counter",
	[MOTION_TYPE_EXT_LOG] = "ext_log",
	[MOTION_TYPE_HEAD_DOWN] = "head_down",
	[MOTION_TYPE_PUT_DOWN] = "put_down",
	[MOTION_TYPE_REMOVE] = "remove",
	[MOTION_TYPE_FALL] = "fall",
	[MOTION_TYPE_SIDEGRIP] = "sidegrip",
	[MOTION_TYPE_FALL_DOWN] = "fall_down",
	[MOTION_TYPE_TOUCH_LINK] = "touchlink",
	[MOTION_TYPE_END] = "end",
};

static int motion_ext_log_handler (const struct pkt_header *head)
{
	int offset;
	int total_len;
	size_t payload_len;
	ext_logger_req_t *pkt_ext = (ext_logger_req_t *)head;
	pedo_ext_logger_req_t *pkt_pedo = (pedo_ext_logger_req_t *)pkt_ext->data;

	/* split packet and write to route */
	hwlog_debug("%s in head tag %d cmd %d len %d handler tag %d\n",
		__func__, head->tag, head->cmd, head->length, pkt_ext->tag);
	/* extract every payload */
	offset = 0;
	total_len = pkt_ext->hd.length -
		(offsetof(ext_logger_req_t, data) - sizeof(struct pkt_header));
	while (offset < total_len) {
		payload_len = pkt_pedo->len +
			offsetof(pedo_ext_logger_req_t, data);
		hwlog_debug("%s offset %d len %lu, pointer %pK\n", __func__,
			offset, payload_len, pkt_pedo);
		if (payload_len + offset > total_len) {
			hwlog_err("%s overstacked payload_len %lu offset %d total_len %d\n",
				__func__, payload_len, offset, total_len);
			break;
		}
		inputhub_route_write(ROUTE_MOTION_PORT,
			(char *)pkt_pedo, payload_len);
		offset += (int)payload_len;
		pkt_pedo = (pedo_ext_logger_req_t *)(pkt_ext->data + offset);
	}
	return 0;
}

static void update_motion_info(enum obj_cmd cmd, motion_type_t type)
{
	if (!(type >= MOTION_TYPE_START && type < MOTION_TYPE_END))
		return;

	switch (cmd) {
	case CMD_CMN_OPEN_REQ:
		motion_status[type] = true;
		break;
	case CMD_CMN_CLOSE_REQ:
		motion_status[type] = false;
		break;
	default:
		hwlog_err("unknown cmd type in %s\n", __func__);
		break;
	}
}

static int extend_step_counter_process(bool enable)
{
	uint8_t app_config[16] = { 0 };
	interval_param_t extend_open_param = {
		.period = 20, /* default delay_ms */
		.batch_count = 1,
		.mode = AUTO_MODE,
		.reserved[0] = TYPE_EXTEND
	};

	inputhub_sensor_enable_stepcounter(enable, TYPE_EXTEND);
	if (enable) {
		app_config[0] = enable;
		app_config[1] = TYPE_EXTEND;

		inputhub_sensor_setdelay(TAG_STEP_COUNTER, &extend_open_param);
		return send_app_config_cmd(TAG_STEP_COUNTER, app_config, true);
	}
	return 0;
}

static int extend_step_counter_process_nolock(bool enable)
{
	uint8_t app_config[16] = { 0 }; /* 16: default app config length */

	app_config[0] = enable;
	app_config[1] = TYPE_EXTEND;
	/* close step counter */
	if (!enable)
		send_app_config_cmd(TAG_STEP_COUNTER, app_config, false);

	hwlog_info("%s extend_step_counter\n", enable ? "open" : "close");
	if (really_do_enable_disable(get_step_ref_cnt(), enable, TYPE_EXTEND))
		inputhub_sensor_enable_nolock(TAG_STEP_COUNTER, enable);
	if (enable) {
		interval_param_t extend_open_param = {
			.period = 20, /* default delay_ms */
			.batch_count = 1,
			.mode = AUTO_MODE,
			.reserved[0] = TYPE_EXTEND
		};
		inputhub_sensor_setdelay_nolock(TAG_STEP_COUNTER,
			&extend_open_param);
		return send_app_config_cmd(TAG_STEP_COUNTER, app_config, false);
	}
	return 0;
}

static int send_motion_cmd_internal(int tag, enum obj_cmd cmd,
	enum obj_sub_cmd subcmd, motion_type_t type, bool use_lock)
{
	uint8_t app_config[16] = { 0 };
	interval_param_t interval_param;
	bool cmd_status = false;

	if (get_stop_auto_motion() == 1) {
		hwlog_info("%s stop_auto_motion: %d", __func__,
			get_stop_auto_motion());
		return 0;
	}

	app_config[0] = type;
	app_config[1] = cmd;
	memset(&interval_param, 0, sizeof(interval_param));

	if ((type == MOTIONHUB_TYPE_HW_STEP_COUNTER) &&
		(cmd == CMD_CMN_OPEN_REQ || cmd == CMD_CMN_CLOSE_REQ)) {
		cmd_status = (cmd == CMD_CMN_OPEN_REQ);
		if (use_lock)
			return extend_step_counter_process(cmd_status);
		else
			return extend_step_counter_process_nolock(cmd_status);
	}

	if (cmd == CMD_CMN_OPEN_REQ) {
		/* send open motion cmd when open first sub type */
		if (really_do_enable_disable(&motion_ref_cnt, true, type)) {
			if (use_lock) {
				inputhub_sensor_enable(tag, true);
				inputhub_sensor_setdelay(tag, &interval_param);
			} else {
				inputhub_sensor_enable_nolock(tag, true);
				inputhub_sensor_setdelay_nolock(tag,
					&interval_param);
			}
			hwlog_info("send_motion_cmd open cmd:%d motion: %s",
				cmd, motion_type_str[type]);
		}
		/* send config cmd to open motion type */
		send_app_config_cmd(TAG_MOTION, app_config, use_lock);
		hwlog_info("send_motion_cmd config cmd:%d motion: %s motion_ref_cnt= %x",
			cmd, motion_type_str[type], motion_ref_cnt);
	} else if (cmd == CMD_CMN_CLOSE_REQ) {
		/* send config cmd to close motion type */
		send_app_config_cmd(TAG_MOTION, app_config, use_lock);
		hwlog_info("send_motion_cmd config cmd:%d motion: %s motion_ref_cnt= %x",
			cmd, motion_type_str[type], motion_ref_cnt);

		/* send close motion cmd when all sub type closed */
		if (really_do_enable_disable(&motion_ref_cnt, false, type)) {
			if (use_lock)
				inputhub_sensor_enable(tag, false);
			else
				inputhub_sensor_enable_nolock(tag, false);
			hwlog_info("send_motion_cmd close cmd:%d motion: %s",
				cmd, motion_type_str[type]);
		}
	} else {
		hwlog_err("send_motion_cmd unknown cmd!\n");
		return -EINVAL;
	}
	return 0;
}

static int send_motion_cmd(unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	int argvalue = 0;
	int i;
	int loop_num;

	if (get_flag_for_sensor_test())
		return 0;

	loop_num = sizeof(motions_cmd_map_tab) /
		sizeof(motions_cmd_map_tab[0]);
	for (i = 0; i < loop_num; i++) {
		if (motions_cmd_map_tab[i].mhb_ioctl_app_cmd == cmd)
			break;
	}

	if (i == loop_num) {
		hwlog_err("%s unknown cmd %d in parse_motion_cmd\n",
			__func__, cmd);
		return -EFAULT;
	}

	if (copy_from_user(&argvalue, argp, sizeof(argvalue)))
		return -EFAULT;

	if (!(argvalue >= MOTION_TYPE_START && argvalue < MOTION_TYPE_END)) {
		hwlog_err("error motion type %d in %s\n", argvalue, __func__);
		return -EINVAL;
	}
	update_motion_info(motions_cmd_map_tab[i].cmd, argvalue);

	return send_motion_cmd_internal(motions_cmd_map_tab[i].tag,
		motions_cmd_map_tab[i].cmd, motions_cmd_map_tab[i].subcmd,
		argvalue, true);
}

void enable_motions_when_recovery_iom3(void)
{
	motion_type_t type;

	motion_ref_cnt = 0;
	/* to send open motion cmd when open first type */
	for (type = MOTION_TYPE_START; type < MOTION_TYPE_END; type++) {
		if (motion_status[type]) {
			hwlog_info("motion state %d in %s\n", type, __func__);
			send_motion_cmd_internal(TAG_MOTION, CMD_CMN_OPEN_REQ,
				SUB_CMD_NULL_REQ, type, false);
		}
	}
}

void disable_motions_when_sysreboot(void)
{
	motion_type_t type;

	for (type = MOTION_TYPE_START; type < MOTION_TYPE_END; type++) {
		if (motion_status[type]) {
			hwlog_info("motion state %d in %s\n", type, __func__);
			send_motion_cmd_internal(TAG_MOTION, CMD_CMN_CLOSE_REQ,
				SUB_CMD_NULL_REQ, type, false);
		}
	}
}

static void read_supported_motions_type_from_dts(void)
{
	struct device_node *np = NULL;
	int supported_motions_count;
	int ret;

	memset(g_supported_motions_type, 0, sizeof(g_supported_motions_type));

	np = of_find_node_by_path(SUPPORTED_MOTIONS_TYPE_NODE_PATH);
	if (!np) {
		hwlog_info("%s, motion node not exist!\n", __func__);
		return;
	}

	supported_motions_count = of_property_count_u32_elems(np,
		SUPPORTED_MOTIONS_TYPE_PROP);
	if (supported_motions_count < 0) {
		hwlog_info("%s, no valid value exist!\n", __func__);
		return;
	}
	if (supported_motions_count > MAX_SUPPORTED_MOTIONS_TYPE_CNT) {
		hwlog_info("%s, buffer is not large enough!\n",
			__func__);
		return;
	}
	ret = of_property_read_u32_array(np, SUPPORTED_MOTIONS_TYPE_PROP,
		g_supported_motions_type, supported_motions_count);
	if (ret != 0 && ret != -ENODATA)
		hwlog_info("%s, read supported motions prop fail\n", __func__);
}

static bool is_motion_supported(u32 motion_type)
{
	int i;

	for (i = 0; i < MAX_SUPPORTED_MOTIONS_TYPE_CNT; i++) {
		if (motion_type == g_supported_motions_type[i])
			return true;
	}
	return false;
}

static int motion_support_query(unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	int motion_type = MOTIONHUB_TYPE_POPUP_CAM;
	int supported = MOTION_SUPPORTED_FLAG;
	int unsupported = MOTION_UNSUPPORTED_FLAG;

	if (copy_from_user(&motion_type, argp, sizeof(motion_type))) {
		hwlog_err("%s, copy motion type fail\n", __func__);
		return -EFAULT;
	}
	if (is_motion_supported(motion_type)) {
		if (copy_to_user(argp, (void *)&supported,
			sizeof(supported)) != 0) {
			hwlog_err("%s, supported copy_to_user error\n",
				__func__);
			return -EFAULT;
		}
	} else {
		if (copy_to_user(argp, (void *)&unsupported,
			sizeof(unsupported)) != 0) {
			hwlog_err("%s, unsupported copy_to_user error\n",
				__func__);
			return -EFAULT;
		}
	}
	return 0;
}

/* read /dev/motionhub */
static ssize_t mhb_read(struct file *file, char __user *buf, size_t count,
			loff_t *pos)
{
	return inputhub_route_read(ROUTE_MOTION_PORT, buf, count);
}

/* write to /dev/motionhub, do nothing now */
static ssize_t mhb_write(struct file *file, const char __user *data,
			 size_t len, loff_t *ppos)
{
	char user_data[USER_WRITE_BUFFER_SIZE] = { 0 };
	char motion_type;

	if (len != USER_WRITE_BUFFER_SIZE) {
		hwlog_err("%s length is invalid\n", __func__);
		return len;
	}

	if (copy_from_user(user_data, data, len)) {
		hwlog_err("%s copy_from_user failed\n", __func__);
		return len;
	}

	motion_type = user_data[0];
	if (motion_type == MOTIONHUB_TYPE_POPUP_CAM) {
		if (inputhub_route_write(ROUTE_MOTION_PORT,
			user_data, len) == 0)
			hwlog_err("%s route_write failed\n", __func__);
	}
	return len;
}

/*
 * Description:   ioctrl function to /dev/motionhub, do open, close motion,
 *                or set interval and attribute to motion
 * Return:        result of ioctrl command, 0 successed, -ENOTTY failed
 */
static long mhb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case MHB_IOCTL_MOTION_START:
	case MHB_IOCTL_MOTION_STOP:
	case MHB_IOCTL_MOTION_ATTR_START:
	case MHB_IOCTL_MOTION_ATTR_STOP:
		break;
	case MHB_IOCTL_MOTION_SUPPORT_QUERY:
		return motion_support_query(arg);
	default:
		hwlog_err("%s unknown cmd : %d\n", __func__, cmd);
		return -ENOTTY;
	}
	return send_motion_cmd(cmd, arg);
}

/* open to /dev/motionhub, do nothing now */
static int mhb_open(struct inode *inode, struct file *file)
{
	hwlog_info("%s ok!\n", __func__);
	return 0;
}

/* releaseto /dev/motionhub, do nothing now */
static int mhb_release(struct inode *inode, struct file *file)
{
	hwlog_info("%s ok!\n", __func__);
	return 0;
}

static int motion_recovery_notifier(struct notifier_block *nb,
	unsigned long foo, void *bar)
{
	/* prevent access the emmc now: */
	hwlog_info("%s (%lu) +\n", __func__, foo);
	switch (foo) {
	case IOM3_RECOVERY_IDLE:
	case IOM3_RECOVERY_START:
	case IOM3_RECOVERY_MINISYS:
	case IOM3_RECOVERY_3RD_DOING:
	case IOM3_RECOVERY_FAILED:
		break;
	case IOM3_RECOVERY_DOING:
		save_step_count();
		enable_motions_when_recovery_iom3();
		break;
	default:
		hwlog_err("%s -unknow state %ld\n", __func__, foo);
		break;
	}
	hwlog_info("%s -\n", __func__);
	return 0;
}

static struct notifier_block motion_recovery_notify = {
	.notifier_call = motion_recovery_notifier,
	.priority = -1,
};

/* file_operations to motion */
static const struct file_operations mhb_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = mhb_read,
	.write = mhb_write,
	.unlocked_ioctl = mhb_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mhb_ioctl,
#endif
	.open = mhb_open,
	.release = mhb_release,
};

/* miscdevice to motion */
static struct miscdevice motionhub_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "motionhub",
	.fops = &mhb_fops,
};

/*
 * Description:   apply kernel buffer, register motionhub_miscdev
 * Return:        result of function, 0 successed, else false
 */
static int __init motionhub_init(void)
{
	int ret;

	if (is_sensorhub_disabled())
		return -1;

	if (!get_sensor_mcu_mode()) {
		hwlog_err("mcu boot fail,motionhub_init exit\n");
		return -1;
	}

	hwlog_info("%s start\n", __func__);

	ret = inputhub_route_open(ROUTE_MOTION_PORT);
	if (ret != 0) {
		hwlog_err("cannot open inputhub route err=%d\n", ret);
		goto OUT;
	}

	ret = misc_register(&motionhub_miscdev);
	if (ret != 0) {
		hwlog_err("cannot register miscdev err=%d\n", ret);
		goto CLOSE;
	}
	ret = inputhub_ext_log_register_handler(TAG_MOTION,
		motion_ext_log_handler);
	if (ret != 0) {
		hwlog_err("cannot register ext_log err=%d\n", ret);
		goto CLOSE;
	}

	register_iom3_recovery_notifier(&motion_recovery_notify);
	read_supported_motions_type_from_dts();
	hwlog_info("%s ok\n", __func__);
	goto OUT;
CLOSE:
	inputhub_route_close(ROUTE_MOTION_PORT);
OUT:
	return ret;
}

/*
 * Description:   release kernel buffer, deregister motionhub_miscdev
 */
static void __exit motionhub_exit(void)
{
	inputhub_route_close(ROUTE_MOTION_PORT);
	misc_deregister(&motionhub_miscdev);
	hwlog_info("exit %s\n", __func__);
}

late_initcall_sync(motionhub_init);
module_exit(motionhub_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MotionHub driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");