/*
 * Huawei Touchscreen Driver
 *
 * Copyright (c) 2012-2050 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __HUAWEI_TS_KIT_API_H_
#define __HUAWEI_TS_KIT_API_H_

#include <linux/version.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/completion.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <huawei_platform/log/hw_log.h>
#if defined(CONFIG_HUAWEI_DSM)
extern struct dsm_client *ts_dclient;
#endif

void ts_proc_bottom_half(struct ts_cmd_node *in_cmd,
	struct ts_cmd_node *out_cmd);
void ts_algo_calibrate(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
void ts_finger_pen_algo_calibrate(struct ts_cmd_node *in_cmd,
	struct ts_cmd_node *out_cmd);
void ts_report_input(struct ts_cmd_node *in_cmd);
void ts_report_fingers_pen(struct ts_cmd_node *in_cmd,
	struct ts_cmd_node *out_cmd);
void ts_report_finger_pen_directly(struct ts_cmd_node *in_cmd);
void ts_report_pen(struct ts_cmd_node *in_cmd);
int ts_power_control(int irq_id,
	struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
void ts_palm_report(struct ts_cmd_node *in_cmd);
void ts_report_key_event(struct ts_cmd_node *in_cmd);
int ts_fw_update_boot(struct ts_cmd_node *in_cmd);
int ts_fw_update_sd(struct ts_cmd_node *in_cmd);
void ts_start_wd_timer(struct ts_kit_platform_data *cd);
void ts_stop_wd_timer(struct ts_kit_platform_data *cd);
bool ts_cmd_need_process(struct ts_cmd_node *cmd);
int ts_kit_power_control_notify(enum lcd_kit_ts_pm_type pm_type, int timeout);
int ts_kit_power_notify_callback(struct notifier_block *self,
	unsigned long notify_pm_type, void *data);
int ts_read_debug_data(struct ts_cmd_node *in_cmd,
	struct ts_cmd_node *out_cmd, struct ts_cmd_sync *sync);
int ts_read_rawdata(struct ts_cmd_node *in_cmd,
	struct ts_cmd_node *out_cmd, struct ts_cmd_sync *sync);
int ts_get_chip_info(struct ts_cmd_node *in_cmd);
int ts_set_info_flag(struct ts_cmd_node *in_cmd);
int ts_calibrate(struct ts_cmd_node *in_cmd, struct ts_cmd_sync *sync);
int ts_calibrate_wakeup_gesture(struct ts_cmd_node *in_cmd,
	struct ts_cmd_sync *sync);
int ts_dsm_debug(struct ts_cmd_node *in_cmd);
int ts_glove_switch(struct ts_cmd_node *in_cmd);
int ts_get_capacitance_test_type(struct ts_cmd_node *in_cmd,
	struct ts_cmd_sync *sync);
int ts_palm_switch(struct ts_cmd_node *in_cmd, struct ts_cmd_sync *sync);
int ts_hand_detect(struct ts_cmd_node *in_cmd);
int ts_force_reset(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
int ts_int_err_process(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
int ts_err_process(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
int ts_check_status(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
int ts_wakeup_gesture_enable_switch(struct ts_cmd_node *in_cmd);
int ts_holster_switch(struct ts_cmd_node *in_cmd);
int ts_roi_switch(struct ts_cmd_node *in_cmd);
int ts_chip_regs_operate(struct ts_cmd_node *in_cmd, struct ts_cmd_sync *sync);
int ts_test_cmd(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
int ts_send_holster_cmd(void);
int ts_touch_window(struct ts_cmd_node *in_cmd);
int ts_send_roi_cmd(enum ts_action_status read_write_type, int timeout);
void ts_kit_charger_switch(struct ts_cmd_node *in_cmd);
int ts_chip_detect(struct ts_cmd_node *in_cmd);
int ts_oem_info_switch(struct ts_cmd_node *in_cmd, struct ts_cmd_sync *sync);
int ts_gamma_info_switch(struct ts_cmd_node *in_cmd, struct ts_cmd_sync *sync);
int ts_get_calibration_info(struct ts_cmd_node *in_cmd,
	struct ts_cmd_node *out_cmd, struct ts_cmd_sync *sync);
int ts_read_calibration_data(struct ts_cmd_node *in_cmd,
	struct ts_cmd_node *out_cmd, struct ts_cmd_sync *sync);
int ts_oemdata_type_check_legal(u8 type, u8 len);
void ts_work_after_input(void);
void dump_fingers_info_debug(struct ts_finger *fingers,
	unsigned int len);
unsigned int ts_is_factory(void);
#if (defined CONFIG_HUAWEI_TS_KIT_UDP)
int ts_kit_fb_notifier_call(struct notifier_block *nb, unsigned long action, void *data);
#endif
#endif
