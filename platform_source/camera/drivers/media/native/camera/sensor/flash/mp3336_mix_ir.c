/*
 * mp3336_mix_ir.c
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "hw_flash.h"
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

/* MP3336 Registers define */
#define REG_CHIPID 0x00
#define REG_MODE 0x01
#define REG_PEAK_CURRENT 0x02
#define REG_FLASH_TIMER 0x03
#define REG_VBL 0x04 // Low battery voltage
#define REG_INDICATOR 0x05
#define REG_L1_FL 0x06
#define REG_L2_FL 0x07
#define REG_L1_TX 0x08
#define REG_L2_TX 0x09
#define REG_IFL_ACT 0x0A
#define REG_IFL_MIN 0x0B
#define REG_L1_TOR 0x0C
#define REG_L2_TOR 0x0D
#define REG_FAULT_INDICATION_A 0x0E
#define REG_FAULT_INDICATION_B 0x0F

#define CHIP_ID 0x38
#define CHIP_ID_MASK 0xF8
#define REG_MASK_0XFF 0xFF
#define STANDBY 0xE0
#define ASIST_MODE (0x2 << 1)
#define STR_FLASH_MODE (0x3 << 1)
#define LED2_EN (0x1 << 3)
#define LED1_EN (0x1 << 4)
#define STR_MOD (0x1 << 6)
#define LED_SD (0x1 << 3)
#define TUP_MIN 0x70

#define OVER_VOLTAGE_PROTECT (0x1 << 6)
#define OVER_TEMP_PROTECT (0x1 << 3)
#define LED_SHORT (0x1 << 4)
#define LED_OPEN 0x1

#define INVALID_GPIO 999
#define FLASH_TIMEOUT_TIME (0xF << 4) // 1600ms
#define MP3336_TORCH_MAX_CUR 318 // mA, lm3646 is 188mA
#define MP3336_FLASH_MAX_CUR 1500 // mA, ref lm3646
#define MP3336_STANDBY_MODE_CUR 0
#define MAX_LIGHTNESS_PARAM_NUM_RT 3
#define LIGHTNESS_PARAM_FD_INDEX_RT 0
#define LIGHTNESS_PARAM_MODE_INDEX_RT 1
#define LIGHTNESS_PARAM_CURRENT_INDEX_RT 2
#define MAX_FLASH_ID 0xFF // used to check flash_lightness_sotre input param

/* Internal data struct defie */
#define MAX_BRIGHTNESS_FORMMI 0x09 // MMI
#define MAX_TX_CURRENT 149 // 186mA

#define WAKE_LOCK_ENABLE 1
#define WAKE_LOCK_DISABLE 0

#define BACK_FLASH_USE_LED2 1
#define torch_current_to_level(x) ((x) * 100 / 125) // reg value = 1.25 / step
#define flash_current_to_level(x) ((x) * 100 / 784) // reg value = 7.84 / step

enum mp3336_mix_ir_pin_type_t {
	RESET = 0,
	STROBE,
	TORCH,
	MAX_PIN,
};

enum mp3336_mix_ir_pin_status_t {
	FREED = 0,
	REQUESTED,
};

struct mp3336_mix_ir_pin_t {
	unsigned int pin_id;
	enum mp3336_mix_ir_pin_status_t pin_status;
};

enum mp3336_mix_ir_current_conf {
	CURRENT_TORCH_LEVEL_MMI_BACK = 0,
	CURRENT_TORCH_LEVEL_MMI_FRONT = 1,
	CURRENT_TORCH_LEVEL_RT_BACK = 2,
	CURRENT_TORCH_LEVEL_RT_FRONT = 3,
	CURRENT_MIX_MAX = 4,
};

/* flash mode for single front or back */
enum mp3336_mix_ir_single_mode_t {
	SINGLE_STANDBY_MODE = 0x00,
	SINGLE_FLASH_MODE = 0x01,
	SINGLE_TORCH_MODE = 0x02,
};

/* flash mode for entire IC which depends on current flash mode */
/* of single front and back */
enum mp3336_mix_ir_entire_ic_mode_t {
	ENTIRE_IC_STANDBY_MODE = 0,
	ENTIRE_IC_FLASH_MODE = 1,
	ENTIRE_IC_TORCH_MODE = 2,
};

/*ir flash RT self test param*/
enum mp3336_mix_ir_selftest_param {
	IR_SELFTEST_ON_TIME_INDEX = 0,
	IR_SELFTEST_OFF_TIME_INDEX = 1,
	IR_SELFTEST_CURRENT_INDEX = 2,
	IR_SELFTEST_TIMES_INDEX = 3,
	IR_SELFTEST_MAX_INDEX = 4,
};

/* Internal data struct define */
struct hw_mp3336_mix_ir_private_data_t {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	struct wakeup_source mp3336_mix_ir_wakelock;
#else
	struct wakeup_source *mp3336_mix_ir_wakelock;
#endif
	unsigned int need_wakelock;
	/* flash control pin */
	struct mp3336_mix_ir_pin_t pin[MAX_PIN];
	/* flash electric current config */
	unsigned int ecurrent[CURRENT_MIX_MAX];
	unsigned int selftest_param[IR_SELFTEST_MAX_INDEX];
	unsigned int chipid;
	unsigned int ctrltype;
	unsigned int led_type;
	unsigned int front_cur;
	unsigned int back_cur;
	enum mp3336_mix_ir_single_mode_t front_mode;
	enum mp3336_mix_ir_single_mode_t back_mode;
	enum mp3336_mix_ir_entire_ic_mode_t entire_ic_mode;
};

/* Internal varible define */
static struct hw_mp3336_mix_ir_private_data_t g_mp3336_mix_ir_pdata;
struct hw_flash_ctrl_t g_mp3336_mix_ir_ctrl;
extern struct dsm_client *client_flash;

define_kernel_flash_mutex(mp3336_mix_ir);

/* Function define */
static int hw_mp3336_mix_ir_param_check(char *buf, unsigned long *param,
	int num_of_par);

static int hw_mp3336_mix_ir_set_pin(struct hw_flash_ctrl_t *flash_ctrl,
	enum mp3336_mix_ir_pin_type_t  pin_type,
	int state)
{
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;
	int rc = 0;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	if (pin_type < RESET || pin_type >= MAX_PIN) {
		cam_err("%s pin type error", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;
	cam_debug("%s pin-type=%d, state=%d", __func__, pin_type, state);

	if ((pdata->pin[pin_type].pin_id != INVALID_GPIO)
	    && (pdata->pin[pin_type].pin_status == REQUESTED)) {
		rc = gpio_direction_output(pdata->pin[pin_type].pin_id, state);
		if (rc < 0)
			cam_err("%s gpio output is err rc=%d", __func__, rc);
	}

	return rc;
}

static int gpio_request_assemble(struct hw_mp3336_mix_ir_private_data_t *pdata)
{
	int i;
	int rc = 0;
	const char *gpio_name[MAX_PIN] = { "flash-reset",
		"flash-strobe",
		"flash-torch"
	};

	if (!pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	for (i = RESET; i < MAX_PIN; i++) {
		if ((pdata->pin[i].pin_id != INVALID_GPIO)
		    && (pdata->pin[i].pin_status == FREED)) {
			rc = gpio_request(pdata->pin[i].pin_id, gpio_name[i]);
			if (rc < 0) {
				cam_err("%s failed to request pin %s",
					__func__, gpio_name[i]);
				break;
			}
			pdata->pin[i].pin_status = REQUESTED;
		}
	}

	return rc;
}

static int gpio_free_assemble(struct hw_mp3336_mix_ir_private_data_t *pdata)
{
	int i;

	if (!pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	for (i = RESET; i < MAX_PIN; i++) {
		if ((pdata->pin[i].pin_id != INVALID_GPIO)
			&& (pdata->pin[i].pin_status == REQUESTED)) {
			gpio_free(pdata->pin[i].pin_id);
			pdata->pin[i].pin_status = FREED;
		}
	}

	return 0;
}

static int hw_mp3336_mix_ir_init(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;
	int rc;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	// init data
	flash_ctrl->flash_type = FLASH_MIX; // mix flash
	flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
		PINCTRL_STATE_DEFAULT);

	if (!flash_ctrl->pctrl) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	// stub:check other params which need to init
	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;
	pdata->front_cur = MP3336_STANDBY_MODE_CUR; // init current as 0
	pdata->back_cur  = MP3336_STANDBY_MODE_CUR; // init current as 0
	pdata->front_mode = SINGLE_STANDBY_MODE;
	pdata->back_mode  = SINGLE_STANDBY_MODE;
	pdata->entire_ic_mode = ENTIRE_IC_STANDBY_MODE;
	flash_ctrl->state.mode = STANDBY_MODE;
	flash_ctrl->state.data = MP3336_STANDBY_MODE_CUR; // set init value 0

	rc = gpio_request_assemble(pdata);
	if (rc < 0) {
		cam_err("%s failde to request gpio", __func__);
		goto gpio_fail_handler;
	}

	rc = hw_mp3336_mix_ir_set_pin(flash_ctrl, RESET, LOW);
	if (rc < 0) {
		cam_err("%s failed to set reset pin", __func__);
		goto gpio_fail_handler;
	}

	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		wakeup_source_init(&pdata->mp3336_mix_ir_wakelock, "mp3336_mix_ir");
#else
		pdata->mp3336_mix_ir_wakelock = wakeup_source_register(flash_ctrl->dev, "mp3336_mix_ir");
		if (!pdata->mp3336_mix_ir_wakelock) {
			rc = -EINVAL;
			cam_err("%s: wakeup source register failed", __func__);
			goto gpio_fail_handler;
		}
#endif
	}
	return rc;
gpio_fail_handler:
	gpio_free_assemble(pdata);
	return rc;
}

static int hw_mp3336_mix_ir_exit(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;

	cam_debug("%s enter", __func__);

	if (!flash_ctrl || !flash_ctrl->pdata || !flash_ctrl->func_tbl ||
		!flash_ctrl->func_tbl->flash_off) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	flash_ctrl->func_tbl->flash_off(flash_ctrl);
	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;
	gpio_free_assemble(pdata);

	flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
		PINCTRL_STATE_IDLE);

	return 0;
}

static void report_dsm(void)
{
	if (!dsm_client_ocuppy(client_flash)) {
		dsm_client_record(client_flash, "flash i2c transfer fail\n");
		dsm_client_notify(client_flash, DSM_FLASH_I2C_ERROR_NO);
		cam_warn("[I/DSM] %s dsm_client_notify",
			client_flash->client_name);
	}
}

static void check_b_and_report_dsm(unsigned char fault_val)
{
	if (fault_val & LED_OPEN) {
		if (!dsm_client_ocuppy(client_flash)) {
			dsm_client_record(client_flash,
				"flash led open fault! FlagReg2[0x%x]\n",
				fault_val);
			dsm_client_notify(client_flash,
				DSM_FLASH_OPEN_SHOTR_ERROR_NO);
			cam_warn("[I/DSM] %s dsm_client_notify",
				client_flash->client_name);
		}
	}
}
static void check_a_and_report_dsm(unsigned char fault_val)
{
	if (fault_val & OVER_VOLTAGE_PROTECT) {
		if (!dsm_client_ocuppy(client_flash)) {
			dsm_client_record(client_flash,
				"flash over voltage protect FlagReg1[0x%x]\n",
				fault_val);
			dsm_client_notify(client_flash,
				DSM_FLASH_OPEN_SHOTR_ERROR_NO);
			cam_warn("[I/DSM] %s dsm_client_notify",
				client_flash->client_name);
		}
	}
	if (fault_val & OVER_TEMP_PROTECT) {
		if (!dsm_client_ocuppy(client_flash)) {
			dsm_client_record(client_flash,
				"flash temperature is too hot FlagReg1[0x%x]\n",
				fault_val);
			dsm_client_notify(client_flash,
				DSM_FLASH_HOT_DIE_ERROR_NO);
			cam_warn("[I/DSM] %s dsm_client_notify",
				client_flash->client_name);
		}
	}
	if (fault_val & LED_SHORT) {
		if (!dsm_client_ocuppy(client_flash)) {
			dsm_client_record(client_flash,
				"flash led short fault! FlagReg2[0x%x]\n",
				fault_val);
			dsm_client_notify(client_flash,
				DSM_FLASH_OPEN_SHOTR_ERROR_NO);
			cam_warn("[I/DSM] %s dsm_client_notify",
				client_flash->client_name);
		}
	}
}

static int hw_mp3336_mix_ir_flash_mode_cfg_calculate(unsigned char *led1_val,
	unsigned char *led2_val,
	unsigned char *reg_mode,
	struct hw_mp3336_mix_ir_private_data_t *pdata)
{
	unsigned int front_level = 0;
	unsigned int back_level = 0;

	if (!led1_val || !led2_val || !reg_mode || !pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	front_level = pdata->front_cur;
	back_level = pdata->back_cur;

	if  (front_level > MP3336_FLASH_MAX_CUR)
		front_level = MP3336_FLASH_MAX_CUR;

	if  (back_level > MP3336_FLASH_MAX_CUR)
		back_level = MP3336_FLASH_MAX_CUR;

	front_level = flash_current_to_level(front_level);
	back_level = flash_current_to_level(back_level);

	if (pdata->led_type == BACK_FLASH_USE_LED2) {
		*led1_val = (unsigned char)(front_level & REG_MASK_0XFF);
		*led2_val = (unsigned char)(back_level & REG_MASK_0XFF);
	} else {
		*led1_val = (unsigned char)(back_level & REG_MASK_0XFF);
		*led2_val = (unsigned char)(front_level & REG_MASK_0XFF);
	}

	*reg_mode = (STR_FLASH_MODE | LED1_EN | LED2_EN) & (~STR_MOD);

	return 0;
}

static int hw_mp3336_mix_ir_torch_mode_cfg_calculate(unsigned char *led1_val,
	unsigned char *led2_val,
	unsigned char *reg_mode,
	struct hw_mp3336_mix_ir_private_data_t *pdata)
{
	unsigned int front_level = 0;
	unsigned int back_level = 0;

	if (!led1_val || !led2_val || !reg_mode || !pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	front_level = pdata->front_cur;
	back_level = pdata->back_cur;

	if  (front_level > MP3336_TORCH_MAX_CUR)
		front_level = MP3336_TORCH_MAX_CUR;

	if  (back_level > MP3336_TORCH_MAX_CUR)
		back_level = MP3336_TORCH_MAX_CUR;

	front_level = torch_current_to_level(front_level);
	back_level = torch_current_to_level(back_level);

	if (pdata->led_type == BACK_FLASH_USE_LED2) {
		*led1_val = (unsigned char)(front_level & REG_MASK_0XFF);
		*led2_val = (unsigned char)(back_level & REG_MASK_0XFF);
	} else {
		*led1_val = (unsigned char)(back_level & REG_MASK_0XFF);
		*led2_val = (unsigned char)(front_level & REG_MASK_0XFF);
	}

	*reg_mode = (ASIST_MODE | LED1_EN | LED2_EN) & (~STR_MOD);

	return 0;
}

static int hw_mp3336_mix_ir_flash_mode_regs_cfg(struct hw_flash_ctrl_t *flash_ctrl,
	unsigned char led1_val,
	unsigned char led2_val,
	unsigned char reg_mode)
{
	int rc = 0;
	unsigned char reg_fault_clean = 0;
	unsigned char reg_peak_cur_val = 0;
	unsigned char reg_lbv_val = 0;
	unsigned char reg_timer = 0;
	unsigned char reg_mode_debug = 0;
	unsigned char reg_indicator = 0;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_flash_i2c_client *i2c_client = NULL;
	/*
	 * (1) The IC mode conversion between flash and
	 * torch requires to enter standby
	 * mode firstly.
	 * (2) In case of flicker when the mode conversion,
	 * need to write 2 groups
	 * data continuously.
	 */
	unsigned char mode_buf[2] = { reg_mode & STANDBY, reg_mode };

	if (!flash_ctrl || !flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_write) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;

	/* clear error status */
	i2c_func->i2c_read(i2c_client,
		REG_FAULT_INDICATION_A, &reg_fault_clean);
	i2c_func->i2c_read(i2c_client,
		REG_FAULT_INDICATION_B, &reg_fault_clean);

	// current limit 2.5A
	rc += i2c_func->i2c_read(i2c_client,
		REG_PEAK_CURRENT, &reg_peak_cur_val);
	reg_peak_cur_val &= ~(0x3 << 3); // clear bit3, bit4
	reg_peak_cur_val |= (0x1 << 3); // set 2.5A(0x1)
	rc += i2c_func->i2c_write(i2c_client,
		REG_PEAK_CURRENT, reg_peak_cur_val);

	// input DC Current limit 3A
	rc += i2c_func->i2c_read(i2c_client, REG_VBL, &reg_lbv_val);
	reg_lbv_val &= 0xF0; // clear bit 0~3
	reg_lbv_val |= 0x08; // set 3A(0x8)
	rc += i2c_func->i2c_write(i2c_client, REG_VBL, reg_lbv_val);

	rc += i2c_func->i2c_read(i2c_client, REG_MODE, &reg_mode_debug);
	cam_info("%s reg_mode debug before config:0x%x", __func__,
		reg_mode_debug);

	rc += i2c_func->i2c_read(i2c_client, REG_INDICATOR, &reg_indicator);
	rc += i2c_func->i2c_write(i2c_client, REG_INDICATOR,
		(reg_indicator | TUP_MIN));
	// set flash timeout
	rc += i2c_func->i2c_read(i2c_client,  REG_FLASH_TIMER, &reg_timer);
	rc += i2c_func->i2c_write(i2c_client, REG_FLASH_TIMER,
		(reg_timer & (~LED_SD)) | FLASH_TIMEOUT_TIME);

	// set TX current
	rc += i2c_func->i2c_write(i2c_client, REG_L1_TX, MAX_TX_CURRENT);
	rc += i2c_func->i2c_write(i2c_client, REG_L2_TX, MAX_TX_CURRENT);

	// set flash levels and reg mode
	rc += i2c_func->i2c_write(i2c_client, REG_L1_FL, led1_val);
	rc += i2c_func->i2c_write(i2c_client, REG_L2_FL, led2_val);
	rc += i2c_func->i2c_write_block(i2c_client,
		REG_MODE, mode_buf, 2); // total size is 2

	cam_info("%s config flash1 = 0x%02x, flash2 = 0x%02x, reg_mode = 0x%02x",
		__func__, led1_val, led2_val, reg_mode);

	if (rc < 0)
		report_dsm();

	return rc;
}

static int hw_mp3336_mix_ir_torch_mode_regs_cfg(struct hw_flash_ctrl_t *flash_ctrl,
	unsigned char led1_val,
	unsigned char led2_val,
	unsigned char reg_mode)
{
	int rc = 0;
	unsigned char reg_fault_clean  = 0;
	unsigned char reg_peak_cur_val = 0;
	unsigned char reg_lbv_val = 0;
	unsigned char reg_mode_debug = 0;
	unsigned char reg_indicator = 0;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_flash_i2c_client *i2c_client = NULL;
	/*
	 * (1) The IC mode conversion between flash and
	 * torch requires to enter standby
	 * mode firstly.
	 * (2) In case of flicker when the mode conversion,
	 * need to write 2 groups
	 * data continuously.
	 */
	unsigned char mode_buf[2] = {reg_mode & STANDBY, reg_mode};

	if (!flash_ctrl || !flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_write) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;

	/* clear error status */
	i2c_func->i2c_read(i2c_client,
		REG_FAULT_INDICATION_A, &reg_fault_clean);
	i2c_func->i2c_read(i2c_client,
		REG_FAULT_INDICATION_B, &reg_fault_clean);

	// current limit 2.5A
	rc += i2c_func->i2c_read(i2c_client,
		REG_PEAK_CURRENT, &reg_peak_cur_val);
	reg_peak_cur_val &= ~(0x3 << 3); // clear bit3, bit4
	reg_peak_cur_val |= (0x1 << 3); // set 2.5A(0x1)
	rc += i2c_func->i2c_write(i2c_client,
		REG_PEAK_CURRENT, reg_peak_cur_val);

	// input DC Current limit 3A
	rc += i2c_func->i2c_read(i2c_client, REG_VBL, &reg_lbv_val);
	reg_lbv_val &= 0xF0; // clear bit 0~3
	reg_lbv_val |= 0x08; // set 3A(0x8)
	rc += i2c_func->i2c_write(i2c_client, REG_VBL, reg_lbv_val);

	rc += i2c_func->i2c_read(i2c_client, REG_MODE, &reg_mode_debug);
	cam_info("%s reg_mode before config:0x%x", __func__, reg_mode_debug);

	rc += i2c_func->i2c_read(i2c_client, REG_INDICATOR, &reg_indicator);
	rc += i2c_func->i2c_write(i2c_client, REG_INDICATOR,
		(reg_indicator | TUP_MIN));
	// set torch current and reg mode
	rc += i2c_func->i2c_write(i2c_client, REG_L1_TOR, led1_val);
	rc += i2c_func->i2c_write(i2c_client, REG_L2_TOR, led2_val);
	// total size is 2
	rc += i2c_func->i2c_write_block(i2c_client, REG_MODE, mode_buf, 2);

	cam_info("%s config torch1 = 0x%02x, torch2 = 0x%02x, reg_mode = 0x%02x",
		__func__, led1_val, led2_val, reg_mode);

	if (rc < 0)
		report_dsm();

	return rc;
}

static int hw_mp3336_mix_ir_action_ic_standby(struct hw_flash_ctrl_t *flash_ctrl)
{
	int rc = 0;
	unsigned char fault_val_a = 0;
	unsigned char fault_val_b = 0;
	unsigned char flash_act_cur = 0;
	unsigned char flash_min_cur = 0;
	unsigned char mode_val = 0;
	unsigned char timeout_val = 0;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;

	if (!flash_ctrl || !flash_ctrl->pdata || !flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_write) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;
	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;

	/* read back error status */
	i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_A, &fault_val_a);
	i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_B, &fault_val_b);
	check_a_and_report_dsm(fault_val_a);
	check_b_and_report_dsm(fault_val_b);
	cam_info("%s Fault and Flag Indication Reg E 0x%x Reg F 0x%x",
		__func__, fault_val_a, fault_val_b);

	rc += i2c_func->i2c_read(i2c_client, REG_IFL_ACT, &flash_act_cur);
	rc += i2c_func->i2c_read(i2c_client, REG_IFL_MIN, &flash_min_cur);
	cam_info("%s IFL ACT current 0x%x MIN Current 0x%x", __func__,
		flash_act_cur, flash_min_cur);

	rc += i2c_func->i2c_read(i2c_client,  REG_MODE, &mode_val);
	rc += i2c_func->i2c_read(i2c_client,  REG_FLASH_TIMER, &timeout_val);
	rc += i2c_func->i2c_write(i2c_client, REG_MODE,
		(mode_val & (~LED1_EN) & (~LED2_EN) & (~ASIST_MODE) & (~STR_FLASH_MODE)));
	rc += i2c_func->i2c_write(i2c_client, REG_FLASH_TIMER,
		(timeout_val & (~LED_SD)));
	if (rc < 0)
		report_dsm();

	hw_mp3336_mix_ir_set_pin(flash_ctrl, STROBE, LOW);
	hw_mp3336_mix_ir_set_pin(flash_ctrl, TORCH, LOW);

	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
		/*
		 * wake_unlock() will check
		 * whether the wakeup source is active before
		 * doing the unlock operation,
		 * so there is no dangers for the mutex being
		 * unlocked before locked.
		 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		__pm_relax(&pdata->mp3336_mix_ir_wakelock);
#else
		__pm_relax(pdata->mp3336_mix_ir_wakelock);
#endif
	}
	return rc;
}

static int hw_mp3336_mix_ir_action_ic_flash_mode(struct hw_flash_ctrl_t *flash_ctrl)
{
	int rc;
	unsigned char reg_mode = 0;
	unsigned char led1_val = 0;
	unsigned char led2_val = 0;
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}
	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;

	rc = hw_mp3336_mix_ir_flash_mode_cfg_calculate(&led1_val,
		&led2_val, &reg_mode, pdata);
	if (rc < 0) {
		cam_err("%s flash mode cfg calculate faild", __func__);
		return rc;
	}

	rc = hw_mp3336_mix_ir_flash_mode_regs_cfg(flash_ctrl,
		led1_val, led2_val, reg_mode);
	if (rc < 0) {
		cam_err("%s flash mode regs cfg faild", __func__);
		return rc;
	}

	return rc;
}

static int hw_mp3336_mix_ir_action_ic_torch_mode(struct hw_flash_ctrl_t *flash_ctrl)
{
	int rc;
	unsigned char led1_val = 0;
	unsigned char led2_val = 0;
	unsigned char reg_mode = 0;
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;

	rc = hw_mp3336_mix_ir_torch_mode_cfg_calculate(&led1_val, &led2_val,
		&reg_mode, pdata);
	if (rc < 0) {
		cam_err("%s torch mode cfg calculate faild", __func__);
		return rc;
	}

	rc = hw_mp3336_mix_ir_torch_mode_regs_cfg(flash_ctrl, led1_val,
		led2_val, reg_mode);
	if (rc < 0) {
		cam_err("%s torch mode reg cfg faild", __func__);
		return rc;
	}

	return rc;
}

static int hw_mp3336_mix_ir_update_inner_status(struct hw_flash_ctrl_t *flash_ctrl,
	struct hw_flash_cfg_data *cdata)
{
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;
	enum mp3336_mix_ir_single_mode_t inner_mode  = SINGLE_STANDBY_MODE;
	unsigned int cur = 0;

	if (!flash_ctrl || !flash_ctrl->pdata || !cdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;
	cur = (unsigned int)(cdata->data);
	// limit the max value when we calculate levels
	// in the corresponding mode

	switch (cdata->mode) {
	case STANDBY_MODE:
		inner_mode = SINGLE_STANDBY_MODE;
		/* flash_ctrl->state will be update */
		/* when ic standby mode cfg is finished */
		break;
	case FLASH_MODE:
		inner_mode = SINGLE_FLASH_MODE;
		/* flash_ctrl->state will be update here, */
		/* and may be used in thermal protect */
		flash_ctrl->state.mode = cdata->mode;
		flash_ctrl->state.data = cdata->data;
		break;
	default:
		inner_mode = SINGLE_TORCH_MODE; // set as default
		/* flash_ctrl->state will be update here, */
		/* and may be used in thermal protect */
		flash_ctrl->state.mode = cdata->mode;
		flash_ctrl->state.data = cdata->data;
		break;
	}

	if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE) {
		pdata->front_mode = inner_mode;
		pdata->front_cur  = cur;
		cam_info("%s update front mode:%d, front cur %u", __func__,
			pdata->front_mode, pdata->front_cur);
	} else {
		pdata->back_mode  = inner_mode;
		pdata->back_cur   = cur;
		cam_info("%s update back mode:%d, back cur %u",  __func__,
			pdata->back_mode, pdata->back_cur);
	}

	return 0;
}

static int hw_mp3336_mix_ir_get_work_mode_strategy(struct hw_flash_ctrl_t *flash_ctrl,
	enum mp3336_mix_ir_entire_ic_mode_t *ic_work_mode)
{
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;
	unsigned int front_mode = SINGLE_STANDBY_MODE;
	unsigned int back_mode  = SINGLE_STANDBY_MODE;

	if (!flash_ctrl || !flash_ctrl->pdata || !ic_work_mode) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;
	front_mode = pdata->front_mode;
	back_mode  = pdata->back_mode;
	*ic_work_mode = ENTIRE_IC_STANDBY_MODE; // set as default

	if ((front_mode | back_mode) == SINGLE_STANDBY_MODE)
		*ic_work_mode = ENTIRE_IC_STANDBY_MODE;
	else if (((front_mode | back_mode) & SINGLE_FLASH_MODE) ==
		SINGLE_FLASH_MODE)
		*ic_work_mode = ENTIRE_IC_FLASH_MODE;
	else if (((front_mode | back_mode) & SINGLE_TORCH_MODE) ==
		SINGLE_TORCH_MODE)
		*ic_work_mode = ENTIRE_IC_TORCH_MODE;

	// if both front and back current is 0, set ic standby mode
	if (pdata->front_cur == MP3336_STANDBY_MODE_CUR &&
		pdata->back_cur == MP3336_STANDBY_MODE_CUR)
		*ic_work_mode = ENTIRE_IC_STANDBY_MODE;

	return 0;
}

static int hw_mp3336_mix_ir_update_work_mode_dispatch(struct hw_flash_ctrl_t *flash_ctrl,
	enum mp3336_mix_ir_entire_ic_mode_t ic_work_mode)
{
	int rc = 0;
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;

	switch (ic_work_mode) {
	case ENTIRE_IC_STANDBY_MODE:
		if (pdata->entire_ic_mode != ENTIRE_IC_STANDBY_MODE) {
			// only when current mode is not in standby,
			// action the standby mode
			rc = hw_mp3336_mix_ir_action_ic_standby(flash_ctrl);
			pdata->entire_ic_mode  = ENTIRE_IC_STANDBY_MODE;
			flash_ctrl->state.mode = STANDBY_MODE;
			flash_ctrl->state.data = MP3336_STANDBY_MODE_CUR;
		}
		break;
	case ENTIRE_IC_FLASH_MODE:
		rc = hw_mp3336_mix_ir_action_ic_flash_mode(flash_ctrl);
		pdata->entire_ic_mode = ENTIRE_IC_FLASH_MODE;
		break;
	case ENTIRE_IC_TORCH_MODE:
		rc = hw_mp3336_mix_ir_action_ic_torch_mode(flash_ctrl);
		pdata->entire_ic_mode = ENTIRE_IC_TORCH_MODE;
		break;
	default:
		cam_err("%s invalid ic work mode", __func__);
		rc = -EINVAL;
		break;
	}

	return rc;
}

static int hw_mp3336_mix_ir_update_work_mode(struct hw_flash_ctrl_t *flash_ctrl,
	struct hw_flash_cfg_data *cdata)
{
	int rc;
	enum mp3336_mix_ir_entire_ic_mode_t ic_work_mode =
		ENTIRE_IC_STANDBY_MODE;

	if (!cdata || !flash_ctrl) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	rc = hw_mp3336_mix_ir_update_inner_status(flash_ctrl, cdata);
	if (rc < 0) {
		cam_err("%s update inner status failed", __func__);
		goto work_mode_err;
	}

	rc = hw_mp3336_mix_ir_get_work_mode_strategy(flash_ctrl, &ic_work_mode);
	if (rc < 0) {
		cam_err("%s get work mode strategy failed", __func__);
		goto work_mode_err;
	}

	rc = hw_mp3336_mix_ir_update_work_mode_dispatch(flash_ctrl,
		ic_work_mode);
	if (rc < 0) {
		cam_err("%s update work mode dispatch failed", __func__);
		goto work_mode_err;
	}

	return rc;

work_mode_err:
	// error handler may need to add here
	return rc;
}

static int hw_mp3336_mix_ir_on(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;
	struct hw_flash_cfg_data *cdata = NULL;
	int rc;

	if (!data || !flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	cdata = (struct hw_flash_cfg_data *)data;
	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;

	cam_info("%s mode=%d, cur=%d", __func__, cdata->mode, cdata->data);

	mutex_lock(flash_ctrl->hw_flash_mutex);

	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
		/*
		 * if the wake lock source has already been active,
		 * repeat call of
		 * 'wake_lock' will just add the event_count
		 * of the wake lock source,
		 * and will not cause evil effects.
		 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		__pm_stay_awake(&pdata->mp3336_mix_ir_wakelock);
#else
		__pm_stay_awake(pdata->mp3336_mix_ir_wakelock);
#endif
	}
	rc = hw_mp3336_mix_ir_update_work_mode(flash_ctrl, cdata);

	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return rc;
}

static int hw_mp3336_mix_ir_off(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_flash_cfg_data cdata = {0};
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;
	int rc;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	cam_debug("%s enter", __func__);

	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;
	cdata.mode = SINGLE_STANDBY_MODE;
	cdata.data = MP3336_STANDBY_MODE_CUR; // set standby current as 0

	mutex_lock(flash_ctrl->hw_flash_mutex);
	rc = hw_mp3336_mix_ir_update_work_mode(flash_ctrl, &cdata);
	mutex_unlock(flash_ctrl->hw_flash_mutex);

	cam_info("%s end", __func__);

	return rc;
}

static int hw_mp3336_mix_ir_get_dt_data(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;
	struct device_node *node = NULL;
	int i;
	int rc = -EINVAL;
	unsigned int pin_tmp[MAX_PIN] = {0};

	if (!flash_ctrl || !flash_ctrl->pdata || !flash_ctrl->dev ||
		!flash_ctrl->dev->of_node) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;
	node = flash_ctrl->dev->of_node;

	rc = of_property_read_u32_array(node, "vendor,flash-pin",
		pin_tmp, (unsigned long)MAX_PIN);
	if (rc < 0) {
		cam_err("%s get dt flash-pin failed line %d",
			__func__, __LINE__);
		/* if failed, init gpio num as invalid */
		for (i = 0; i < MAX_PIN; i++) {
			pdata->pin[i].pin_id = INVALID_GPIO;
			pdata->pin[i].pin_status = FREED;
		}
		return rc;
	}
	for (i = 0; i < MAX_PIN; i++) {
		pdata->pin[i].pin_id = pin_tmp[i];
		pdata->pin[i].pin_status = FREED;
		cam_info("%s pin[%d]=%u", __func__,
			i, pdata->pin[i].pin_id);
	}

	rc = of_property_read_u32(node, "vendor,flash-chipid", &pdata->chipid);
	cam_info("%s vendor,flash-chipid 0x%x, rc %d", __func__, pdata->chipid, rc);
	if (rc < 0) {
		cam_err("%s failed %d", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(node, "vendor,flash-ctrltype",
		&pdata->ctrltype);
	cam_info("%s vendor,ctrltype 0x%x, rc %d", __func__,
		pdata->ctrltype, rc);
	if (rc < 0) {
		cam_err("%s failed %d", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(node, "vendor,led-type",
		&pdata->led_type);
	cam_info("%s vendor,led-type %d, rc %d\n", __func__,
		pdata->led_type, rc);
	if (rc < 0) {
		cam_err("%s read led-type failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(node, "vendor,need-wakelock",
		(u32 *)&pdata->need_wakelock);
	cam_info("%s vendor,need-wakelock %d, rc %d\n", __func__,
		pdata->need_wakelock, rc);
	if (rc < 0) {
		pdata->need_wakelock = WAKE_LOCK_DISABLE;
		cam_err("%s failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32_array(node, "vendor,flash-current",
		pdata->ecurrent, CURRENT_MIX_MAX);
	if (rc < 0) {
		cam_err("%s read flash-current failed line %d\n",
			__func__, __LINE__);
		return rc;
	}
	for (i = 0; i < CURRENT_MIX_MAX; i++)
		cam_info("%s ecurrent[%d]=%d\n", __func__, i,
			pdata->ecurrent[i]);

	rc = of_property_read_u32_array(node, "vendor,selftest-param",
		pdata->selftest_param, IR_SELFTEST_MAX_INDEX);
	if (rc < 0) {
		cam_err("%s read selftest-param failed line %d\n",
			__func__, __LINE__);
		return rc;
	}
	for (i = 0; i < IR_SELFTEST_MAX_INDEX; i++)
		cam_info("%s selftest_param[%d]=%d\n", __func__, i,
			pdata->selftest_param[i]);

	return rc;
}

static ssize_t hw_mp3336_mix_ir_flash_lightness_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int rc;

	if (!buf) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	// PAGE_SIZE = 4096
	rc = scnprintf(buf, PAGE_SIZE,
		"front_mode=%d, data=%d; back_mode=%d, data=%d\n",
		g_mp3336_mix_ir_pdata.front_mode,
		g_mp3336_mix_ir_pdata.front_cur,
		g_mp3336_mix_ir_pdata.back_mode,
		g_mp3336_mix_ir_pdata.back_cur);

	return rc;
}

static ssize_t hw_mp3336_mix_ir_flash_lightness_show_f(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int rc;

	if (!buf) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	// show for ir selftest in RT, PAGE_SIZE = 4096
	rc = scnprintf(buf, PAGE_SIZE, "%d %d %d %d",
		g_mp3336_mix_ir_pdata.selftest_param[IR_SELFTEST_ON_TIME_INDEX],
		g_mp3336_mix_ir_pdata.selftest_param[IR_SELFTEST_OFF_TIME_INDEX],
		g_mp3336_mix_ir_pdata.selftest_param[IR_SELFTEST_CURRENT_INDEX],
		g_mp3336_mix_ir_pdata.selftest_param[IR_SELFTEST_TIMES_INDEX]);

	return rc;
}

static int hw_mp3336_mix_ir_param_check(char *buf, unsigned long *param,
	int num_of_par)
{
	char *token = NULL;
	unsigned int base = 0;
	int cnt;

	if (!buf || !param) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	token = strsep(&buf, " ");

	for (cnt = 0; cnt < num_of_par; cnt++) {
		if (token) {
			// add for check token[1] has valid element
			if (strlen(token) < 2) {
				base = 10; // decimal
			} else {
				// format 0x**
				if ((token[1] == 'x') || (token[1] == 'X'))
					base = 16; // hex
				else
					base = 10; // decimal
			}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
			if (strict_strtoul(token, base, &param[cnt]) != 0)
#else
			if (kstrtoul(token, base, &param[cnt]) != 0)
#endif
				return -EINVAL;
			token = strsep(&buf, " ");
		} else {
			cam_info("%s the %d param is null\n", __func__,
				num_of_par);
		}
	}

	return 0;
}

static ssize_t hw_mp3336_mix_ir_flash_lightness_store_imp(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hw_flash_cfg_data cdata = {0};
	unsigned long param[MAX_LIGHTNESS_PARAM_NUM_RT] = {0};
	int rc;
	int flash_id = 0;

	if (!buf) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	rc = hw_mp3336_mix_ir_param_check((char *)buf, param,
		MAX_LIGHTNESS_PARAM_NUM_RT);
	if (rc < 0) {
		cam_err("%s failed to check param", __func__);
		return rc;
	}

	flash_id = (int)param[LIGHTNESS_PARAM_FD_INDEX_RT]; // 0 - flash id
	cdata.mode = (int)param[LIGHTNESS_PARAM_MODE_INDEX_RT]; // 1 - mode
	// 2 - current
	cdata.data = (int)param[LIGHTNESS_PARAM_CURRENT_INDEX_RT];
	cam_info("%s flash_id=%d, cdata.mode=%d, cdata.data=%d",
		__func__, flash_id, cdata.mode, cdata.data);

	if ((cdata.mode < STANDBY_MODE) || (cdata.mode > MAX_MODE)) {
		cam_err("%s out of bounds: %d", __func__, cdata.mode);
		return -EINVAL;
	}
	// 0~1500 mA
	if ((cdata.data < 0) || (cdata.data > MP3336_FLASH_MAX_CUR)) {
		cam_err("%s out of bounds: %d", __func__, cdata.data);
		return -EINVAL;
	}
	// 0~0xFF, pass from HAL, but not use juse print
	if ((flash_id < 0) || (flash_id > MAX_FLASH_ID)) {
		cam_err("%s out of bounds: %d", __func__, flash_id);
		return -EINVAL;
	}

	if (cdata.mode == STANDBY_MODE) {
		rc = hw_mp3336_mix_ir_off(&g_mp3336_mix_ir_ctrl);
		if (rc < 0) {
			cam_err("%s flash off error", __func__);
			return rc;
		}
	} else if (cdata.mode == TORCH_MODE) {
		if (g_mp3336_mix_ir_ctrl.mix_pos == HWFLASH_POSITION_FORE)
			cdata.data =
				g_mp3336_mix_ir_pdata.ecurrent[CURRENT_TORCH_LEVEL_RT_FRONT];
		else
			cdata.data =
				g_mp3336_mix_ir_pdata.ecurrent[CURRENT_TORCH_LEVEL_RT_BACK];

		cam_info("%s mode=%d, max_current=%d", __func__,
			cdata.mode, cdata.data);

		rc = hw_mp3336_mix_ir_on(&g_mp3336_mix_ir_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s flash on error", __func__);
			return rc;
		}
	} else if (cdata.mode == FLASH_MODE) {
		cam_info("%s mode=%d, current=%d", __func__,
			cdata.mode, cdata.data);
		rc = hw_mp3336_mix_ir_on(&g_mp3336_mix_ir_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s flash on error", __func__);
			return rc;
		}
	} else {
		cam_err("%s wrong mode=%d", __func__, cdata.mode);
		return -EINVAL;
	}

	return count;
}

static ssize_t hw_mp3336_mix_ir_flash_lightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	// call back flash
	g_mp3336_mix_ir_ctrl.mix_pos = HWFLASH_POSITION_REAR;
	return hw_mp3336_mix_ir_flash_lightness_store_imp(dev, attr,
		buf, count);
}

static ssize_t hw_mp3336_mix_ir_flash_lightness_f_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	// call front flash
	g_mp3336_mix_ir_ctrl.mix_pos = HWFLASH_POSITION_FORE;
	return hw_mp3336_mix_ir_flash_lightness_store_imp(dev, attr,
		buf, count);
}

static ssize_t hw_mp3336_mix_ir_flash_mask_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	if (!buf) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}
	rc = scnprintf(buf, PAGE_SIZE, "flash_mask_disabled=%d\n",
		g_mp3336_mix_ir_ctrl.flash_mask_enable);

	return rc;
}

static ssize_t hw_mp3336_mix_ir_flash_mask_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (!buf) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}
	if (buf[0] == '0') // char '0' for mask disable
		g_mp3336_mix_ir_ctrl.flash_mask_enable = false;
	else
		g_mp3336_mix_ir_ctrl.flash_mask_enable = true;
	cam_info("%s flash_mask_enable=%d", __func__,
		g_mp3336_mix_ir_ctrl.flash_mask_enable);
	return (ssize_t)count;
}

static void hw_mp3336_mix_ir_torch_brightness_set_imp(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	int rc = 0;
	struct hw_flash_cfg_data cdata = {0};
	unsigned int led_bright = brightness;

	cam_info("%s brightness= %d", __func__, brightness);

	if (led_bright == LED_OFF) {
		rc = hw_mp3336_mix_ir_off(&g_mp3336_mix_ir_ctrl);
		if (rc < 0) {
			cam_err("%s pmu_led off error", __func__);
			return;
		}
	} else {
		cdata.mode = TORCH_MODE;
		if (g_mp3336_mix_ir_ctrl.mix_pos == HWFLASH_POSITION_FORE)
			cdata.data = g_mp3336_mix_ir_pdata.ecurrent[CURRENT_TORCH_LEVEL_MMI_FRONT];
		else
			cdata.data = g_mp3336_mix_ir_pdata.ecurrent[CURRENT_TORCH_LEVEL_MMI_BACK];

		cam_info("%s brightness=0x%x, mode=%d, data=%d", __func__,
			brightness, cdata.mode, cdata.data);
		rc = hw_mp3336_mix_ir_on(&g_mp3336_mix_ir_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s flash on error", __func__);
			return;
		}
	}
}

static void hw_mp3336_mix_ir_torch_brightness_set(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	g_mp3336_mix_ir_ctrl.mix_pos = HWFLASH_POSITION_REAR;
	hw_mp3336_mix_ir_torch_brightness_set_imp(cdev, brightness);
}

static void hw_mp3336_mix_ir_torch_brightness_f_set(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	g_mp3336_mix_ir_ctrl.mix_pos = HWFLASH_POSITION_FORE;
	hw_mp3336_mix_ir_torch_brightness_set_imp(cdev, brightness);
}

// for RT
static struct device_attribute g_mp3336_mix_ir_flash_lightness =
	__ATTR(flash_lightness, 0660, hw_mp3336_mix_ir_flash_lightness_show,
	hw_mp3336_mix_ir_flash_lightness_store); // 660:-wr-wr---

static struct device_attribute g_mp3336_mix_ir_flash_lightness_f =
	__ATTR(flash_lightness_f, 0660, hw_mp3336_mix_ir_flash_lightness_show_f,
	hw_mp3336_mix_ir_flash_lightness_f_store); // 660:-wr-wr---

static struct device_attribute g_mp3336_mix_ir_flash_mask =
	__ATTR(flash_mask, 0660, hw_mp3336_mix_ir_flash_mask_show,
	hw_mp3336_mix_ir_flash_mask_store); // 660:-wr-wr---

extern int register_camerafs_attr(struct device_attribute *attr);
static int hw_mp3336_mix_ir_register_attribute(struct hw_flash_ctrl_t *flash_ctrl,
	struct device *dev)
{
	int rc;

	if (!flash_ctrl || !dev) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	register_camerafs_attr(&g_mp3336_mix_ir_flash_lightness);
	register_camerafs_attr(&g_mp3336_mix_ir_flash_lightness_f);

	flash_ctrl->cdev_torch.name = "torch";
	flash_ctrl->cdev_torch.max_brightness =
		(enum led_brightness)MAX_BRIGHTNESS_FORMMI;
	flash_ctrl->cdev_torch.brightness_set =
		hw_mp3336_mix_ir_torch_brightness_set;
	rc = led_classdev_register((struct device *)dev,
		&flash_ctrl->cdev_torch);
	if (rc < 0) {
		cam_err("%s failed to register torch classdev", __func__);
		goto err_out;
	}

	flash_ctrl->cdev_torch1.name = "torch_front";
	flash_ctrl->cdev_torch1.max_brightness
		= (enum led_brightness)MAX_BRIGHTNESS_FORMMI;
	flash_ctrl->cdev_torch1.brightness_set =
		hw_mp3336_mix_ir_torch_brightness_f_set;
	rc = led_classdev_register((struct device *)dev,
		&flash_ctrl->cdev_torch1);
	if (rc < 0) {
		cam_err("%s failed to register torch_front classdev", __func__);
		goto err_create_torch_front_file;
	}

	rc = device_create_file(dev, &g_mp3336_mix_ir_flash_lightness);
	if (rc < 0) {
		cam_err("%s failed to creat flash_lightness attribute",
			__func__);
		goto err_create_flash_lightness_file;
	}

	rc = device_create_file(dev, &g_mp3336_mix_ir_flash_lightness_f);
	if (rc < 0) {
		cam_err("%s failed to creat flash_f_lightness attribute",
			__func__);
		goto err_create_flash_f_lightness_file;
	}

	rc = device_create_file(dev, &g_mp3336_mix_ir_flash_mask);
	if (rc < 0) {
		cam_err("%s failed to creat flash_mask attribute", __func__);
		goto err_create_flash_mask_file;
	}

	return 0;

err_create_flash_mask_file:
	device_remove_file(dev, &g_mp3336_mix_ir_flash_lightness_f);
err_create_flash_f_lightness_file:
	device_remove_file(dev, &g_mp3336_mix_ir_flash_lightness);
err_create_flash_lightness_file:
	led_classdev_unregister(&flash_ctrl->cdev_torch1);
err_create_torch_front_file:
	led_classdev_unregister(&flash_ctrl->cdev_torch);
err_out:
	return rc;
}

static int hw_mp3336_mix_ir_match(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_mp3336_mix_ir_private_data_t *pdata = NULL;
	unsigned char id = 0;

	if (!flash_ctrl || !flash_ctrl->pdata ||
		!flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;
	pdata = (struct hw_mp3336_mix_ir_private_data_t *)flash_ctrl->pdata;

	/*
	 * Enable mp3336_mix_ir switch to standby current is 10ua,
	 * if match id success, reset pin will always be enabled.
	 */
	hw_mp3336_mix_ir_set_pin(flash_ctrl, RESET, HIGH);
	i2c_func->i2c_read(i2c_client, REG_CHIPID, &id);
	cam_info("%s 0x%x", __func__, id);
	id = id & CHIP_ID_MASK;
	if (id != CHIP_ID) {
		cam_err("%s match error, 0x%x != 0x%x",
			__func__, id, CHIP_ID);

		/*
		 * Enable mp3336_mix_ir switch to shutdown when matchid fail,
		 * current is 1.3ua
		 */
		hw_mp3336_mix_ir_set_pin(flash_ctrl, RESET, LOW);
		return -EINVAL;
	}

	return 0;
}

static int hw_mp3336_mix_ir_remove(struct i2c_client *client)
{
	cam_debug("%s enter", __func__);
	if (!client) {
		cam_err("%s client is NULL", __func__);
		return -EINVAL;
	}
	g_mp3336_mix_ir_ctrl.func_tbl->flash_exit(&g_mp3336_mix_ir_ctrl);

	client->adapter = NULL;
	return 0;
}

static const struct i2c_device_id g_mp3336_mix_ir_id[] = {
	{ "mp3336_mix_ir", (unsigned long) &g_mp3336_mix_ir_ctrl },
	{}
};

static const struct of_device_id g_mp3336_mix_ir_dt_match[] = {
	{ .compatible = "vendor,mp3336_mix_ir" },
	{}
};
MODULE_DEVICE_TABLE(of, mp3336_mix_ir_dt_match);

static struct i2c_driver g_mp3336_mix_ir_i2c_driver = {
	.probe  = hw_flash_i2c_probe,
	.remove = hw_mp3336_mix_ir_remove,
	.id_table = g_mp3336_mix_ir_id,
	.driver = {
		.name = "hw_mp3336_mix_ir",
		.of_match_table = g_mp3336_mix_ir_dt_match,
	},
};

static int __init hw_mp3336_mix_ir_module_init(void)
{
	cam_info("%s erter", __func__);
	return i2c_add_driver(&g_mp3336_mix_ir_i2c_driver);
}

static void __exit hw_mp3336_mix_ir_module_exit(void)
{
	cam_info("%s enter", __func__);
	i2c_del_driver(&g_mp3336_mix_ir_i2c_driver);
}

static struct hw_flash_i2c_client g_mp3336_mix_ir_i2c_client;

static struct hw_flash_fn_t g_mp3336_mix_ir_func_tbl = {
	.flash_config = hw_flash_config,
	.flash_init = hw_mp3336_mix_ir_init,
	.flash_exit = hw_mp3336_mix_ir_exit,
	.flash_on = hw_mp3336_mix_ir_on,
	.flash_off = hw_mp3336_mix_ir_off,
	.flash_match = hw_mp3336_mix_ir_match,
	.flash_get_dt_data = hw_mp3336_mix_ir_get_dt_data,
	.flash_register_attribute = hw_mp3336_mix_ir_register_attribute,
};

struct hw_flash_ctrl_t g_mp3336_mix_ir_ctrl = {
	.flash_i2c_client = &g_mp3336_mix_ir_i2c_client,
	.func_tbl = &g_mp3336_mix_ir_func_tbl,
	.hw_flash_mutex = &flash_mut_mp3336_mix_ir,
	.pdata = (void *)&g_mp3336_mix_ir_pdata,
	.flash_mask_enable = true,
	.state = {
		.mode = STANDBY_MODE,
	},
};


module_init(hw_mp3336_mix_ir_module_init);
module_exit(hw_mp3336_mix_ir_module_exit);
MODULE_DESCRIPTION("MP3336 FLASH");
MODULE_LICENSE("GPL v2");
