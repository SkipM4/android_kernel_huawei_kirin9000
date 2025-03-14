/*
 * lcd_kit_debug_hs.c
 *
 * lcdkit debug function for lcd driver of hisi platform
 *
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
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

#include "hisi_fb.h"
#include "lcd_kit_common.h"
#include "lcd_kit_dbg.h"
#include "lcd_kit_disp.h"
#include "lcd_kit_power.h"
#include "lcd_kit_parse.h"
#include <base.h>

extern struct delayed_work detect_fs_work;
extern int g_dpd_mode;

static bool dbg_panel_power_on(void);

static int dbg_fps_updt_support(int val)
{
	disp_info->fps.support = val;
	LCD_KIT_INFO("disp_info->fps.support = %d\n", disp_info->fps.support);
	return LCD_KIT_OK;
}

static int dbg_quickly_sleep_out_support(int val)
{
	disp_info->quickly_sleep_out.support = val;
	LCD_KIT_INFO("disp_info->quickly_sleep_out.support = %d\n",
		disp_info->quickly_sleep_out.support);
	return LCD_KIT_OK;
}

static int init_panel_info(struct hisi_fb_data_type *hisifd,
	struct hisi_panel_info **pinfo)
{
	hisifd = hisifd_list[PRIMARY_PANEL_IDX];
	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (pinfo == NULL) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;
	}
	*pinfo = &hisifd->panel_info;
	if (*pinfo == NULL) {
		LCD_KIT_ERR("*pinfo is null\n");
		return LCD_KIT_FAIL;
	}
	return LCD_KIT_OK;
}

static int dbg_blpwm_input_support(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->blpwm_input_disable = val;
	LCD_KIT_INFO("blpwm_input_disable = %d\n", pinfo->blpwm_input_disable);
	return LCD_KIT_OK;
}

static int dbg_dsi_upt_support(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->dsi_bit_clk_upt_support = val;
	LCD_KIT_INFO("dsi_upt_support = %d\n", pinfo->dsi_bit_clk_upt_support);
	return LCD_KIT_OK;
}

static int dbg_rgbw_support(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->rgbw_support = val;
	disp_info->rgbw.support = val;
	LCD_KIT_INFO("pinfo->rgbw_support = %d\n", pinfo->rgbw_support);
	return LCD_KIT_OK;
}

static int dbg_gamma_support(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->gamma_support = val;
	LCD_KIT_INFO("pinfo->gamma_support = %d\n", pinfo->gamma_support);
	return LCD_KIT_OK;
}

static int dbg_gmp_support(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->gmp_support = val;
	LCD_KIT_INFO("pinfo->gmp_support = %d\n", pinfo->gmp_support);
	return LCD_KIT_OK;
}

static int dbg_hiace_support(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->hiace_support = val;
	LCD_KIT_INFO("pinfo->hiace_support = %d\n", pinfo->hiace_support);
	return LCD_KIT_OK;
}

static int dbg_xcc_support(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->xcc_support = val;
	LCD_KIT_INFO("pinfo->xcc_support = %d\n", pinfo->xcc_support);
	return LCD_KIT_OK;
}

static int dbg_arsr1psharpness_support(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->arsr1p_sharpness_support = val;
	LCD_KIT_INFO("arsr1p_support = %d\n", pinfo->arsr1p_sharpness_support);
	return LCD_KIT_OK;
}

static int dbg_prefixsharptwo_d_support(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->prefix_sharpness2D_support = val;
	LCD_KIT_INFO("prefix_support=%d\n", pinfo->prefix_sharpness2D_support);
	return LCD_KIT_OK;
}

static void hisifb_frame_refresh(struct hisi_fb_data_type *hisifd)
{
#define ENVP_LEN 2
#define BUF_LEN 64
	char *envp[ENVP_LEN] = {0};
	char buf[BUF_LEN];

	if (!hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return;
	}

	snprintf(buf, sizeof(buf), "Refresh=1");
	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&(hisifd->fbi->dev->kobj), KOBJ_CHANGE, envp);
}

static int dbg_video_idle_mode_support(int val)
{
#define BACKLIGHT_DELAY 100
	struct hisi_fb_data_type *hisifd = hisifd_list[PRIMARY_PANEL_IDX];
	struct hisi_panel_info *pinfo = NULL;
	bool panel_power_on = dbg_panel_power_on();
	uint32_t bl_level_cur;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	/* lcd panel off */
	if (panel_power_on) {
		bl_level_cur = hisifd->bl_level;
		hisifb_set_backlight(hisifd, 0, false);
		ret = hisi_fb_blank_sub(FB_BLANK_POWERDOWN, hisifd->fbi);
		if (ret != 0) {
			LCD_KIT_ERR("fb%d, blank_mode(%d) failed!\n",
				hisifd->index, FB_BLANK_POWERDOWN);
			return LCD_KIT_FAIL;
		}
	}

	pinfo->video_idle_mode = val;

	/* lcd panel on */
	if (panel_power_on) {
		ret = hisi_fb_blank_sub(FB_BLANK_UNBLANK, hisifd->fbi);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, blank_mode(%d) failed!\n",
				hisifd->index, FB_BLANK_UNBLANK);
			return LCD_KIT_FAIL;
		}
		hisifb_frame_refresh(hisifd);
		msleep(BACKLIGHT_DELAY);
		bl_level_cur = bl_level_cur ? bl_level_cur : hisifd->bl_level;
		hisifb_set_backlight(hisifd, bl_level_cur, false);
	}
	LCD_KIT_INFO("pinfo->video_idle_mode = %d\n", pinfo->video_idle_mode);
	return LCD_KIT_OK;
}

static int dbg_cmd_type(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->type = val;
	LCD_KIT_INFO("pinfo->type = %d\n", pinfo->type);
	return LCD_KIT_OK;
}

static int dbg_pxl_clk(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->pxl_clk_rate = val * 1000000UL;
	LCD_KIT_INFO("pinfo->pxl_clk_rate = %llu\n", pinfo->pxl_clk_rate);
	return LCD_KIT_OK;
}

static int dbg_pxl_clk_div(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->pxl_clk_rate_div = val;
	LCD_KIT_INFO("pinfo->pxl_clk_rate_div = %d\n", pinfo->pxl_clk_rate_div);
	return LCD_KIT_OK;
}

static int dbg_vsync_ctrl_type(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->vsync_ctrl_type = val;
	LCD_KIT_INFO("pinfo->vsync_ctrl_type = %d\n", pinfo->vsync_ctrl_type);
	return LCD_KIT_OK;
}

static int dbg_hback_porch(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->ldi.h_back_porch = val;
	LCD_KIT_INFO("pinfo->ldi.h_back_porch = %d\n", pinfo->ldi.h_back_porch);
	return LCD_KIT_OK;
}

static int dbg_hfront_porch(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->ldi.h_front_porch = val;
	LCD_KIT_INFO("ldi.h_front_porch = %d\n", pinfo->ldi.h_front_porch);
	return LCD_KIT_OK;
}

static int dbg_hpulse_width(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->ldi.h_pulse_width = val;
	LCD_KIT_INFO("ldi.h_pulse_width = %d\n", pinfo->ldi.h_pulse_width);
	return LCD_KIT_OK;
}

static int dbg_vback_porch(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->ldi.v_back_porch = val;
	LCD_KIT_INFO("pinfo->ldi.v_back_porch = %d\n", pinfo->ldi.v_back_porch);
	return LCD_KIT_OK;
}

static int dbg_vfront_porch(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->ldi.v_front_porch = val;
	LCD_KIT_INFO("pinfo->ldi.v_front_porch=%d\n", pinfo->ldi.v_front_porch);
	return LCD_KIT_OK;
}

static int dbg_vpulse_width(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->ldi.v_pulse_width = val;
	LCD_KIT_INFO("pinfo->ldi.v_pulse_width=%d\n", pinfo->ldi.v_pulse_width);
	return LCD_KIT_OK;
}

static int dbg_mipi_burst_mode(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.burst_mode = val;
	LCD_KIT_INFO("pinfo->mipi.burst_mode = %d\n", pinfo->mipi.burst_mode);
	return LCD_KIT_OK;
}

static int dbg_mipi_max_tx_esc_clk(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.max_tx_esc_clk = val;
	LCD_KIT_INFO("mipi.max_tx_esc_clk = %d\n", pinfo->mipi.max_tx_esc_clk);
	return LCD_KIT_OK;
}

static int dbg_mipi_dsi_bit_clk(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.dsi_bit_clk_upt = val;
	LCD_KIT_INFO("pinfo->mipi.dsi_bit_clk = %d\n", pinfo->mipi.dsi_bit_clk);
	return LCD_KIT_OK;
}

static int dbg_mipi_dsi_bit_clk_a(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.dsi_bit_clk_val1 = val;
	LCD_KIT_INFO("dsi_bit_clk_val1 = %d\n", pinfo->mipi.dsi_bit_clk_val1);
	return LCD_KIT_OK;
}

static int dbg_mipi_dsi_bit_clk_b(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.dsi_bit_clk_val2 = val;
	LCD_KIT_INFO("dsi_bit_clk_val2 = %d\n", pinfo->mipi.dsi_bit_clk_val2);
	return LCD_KIT_OK;
}

static int dbg_mipi_dsi_bit_clk_c(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.dsi_bit_clk_val3 = val;
	LCD_KIT_INFO("dsi_bit_clk_val3 = %d\n", pinfo->mipi.dsi_bit_clk_val3);
	return LCD_KIT_OK;
}

static int dbg_mipi_dsi_bit_clk_d(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.dsi_bit_clk_val4 = val;
	LCD_KIT_INFO("dsi_bit_clk_val4 = %d\n", pinfo->mipi.dsi_bit_clk_val4);
	return LCD_KIT_OK;
}

static int dbg_mipi_dsi_bit_clk_e(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.dsi_bit_clk_val5 = val;
	LCD_KIT_INFO("dsi_bit_clk_val5 = %d\n", pinfo->mipi.dsi_bit_clk_val5);
	return LCD_KIT_OK;
}

static int dbg_mipi_noncontinue_enable(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.non_continue_en = val;
	LCD_KIT_INFO("non_continue_en = %d\n", pinfo->mipi.non_continue_en);
	return LCD_KIT_OK;
}

static int dbg_mipi_rg_vcm_adjust(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.rg_vrefsel_vcm_adjust = val;
	LCD_KIT_INFO("rg_vcm_adjust = %d\n", pinfo->mipi.rg_vrefsel_vcm_adjust);
	return LCD_KIT_OK;
}

static int dbg_mipi_clk_post_adjust(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.clk_post_adjust = val;
	LCD_KIT_INFO("pinfo->mipi.clk_post_adjust = %d\n", pinfo->mipi.clk_post_adjust);
	return LCD_KIT_OK;
}

static int dbg_mipi_clk_pre_adjust(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.clk_pre_adjust = val;
	LCD_KIT_INFO("clk_pre_adjust = %d\n", pinfo->mipi.clk_pre_adjust);
	return LCD_KIT_OK;
}

static int dbg_mipi_clk_ths_prepare_adjust(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.clk_t_hs_prepare_adjust = val;
	LCD_KIT_INFO("clk_ths_pre=%d\n", pinfo->mipi.clk_t_hs_prepare_adjust);
	return LCD_KIT_OK;
}

static int dbg_mipi_clk_tlpx_adjust(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.clk_t_lpx_adjust = val;
	LCD_KIT_INFO("clk_tlpx_adjust = %d\n", pinfo->mipi.clk_t_lpx_adjust);
	return LCD_KIT_OK;
}

static int dbg_mipi_clk_ths_trail_adjust(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.clk_t_hs_trial_adjust = val;
	LCD_KIT_INFO("clk_t_hs_trial=%d\n", pinfo->mipi.clk_t_hs_trial_adjust);
	return LCD_KIT_OK;
}

static int dbg_mipi_clk_ths_exit_adjust(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.clk_t_hs_exit_adjust = val;
	LCD_KIT_INFO("clk_t_hs_exit = %d\n", pinfo->mipi.clk_t_hs_exit_adjust);
	return LCD_KIT_OK;
}

static int dbg_mipi_clk_ths_zero_adjust(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.clk_t_hs_zero_adjust = val;
	LCD_KIT_INFO("clk_t_hs_zero = %d\n", pinfo->mipi.clk_t_hs_zero_adjust);
	return LCD_KIT_OK;
}

static int dbg_mipi_lp11_flag(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.lp11_flag = val;
	LCD_KIT_INFO("pinfo->mipi.lp11_flag = %d\n", pinfo->mipi.lp11_flag);
	return LCD_KIT_OK;
}

static int dbg_mipi_phy_update(int val)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->mipi.phy_m_n_count_update = val;
	LCD_KIT_INFO("phy_update = %d\n", pinfo->mipi.phy_m_n_count_update);
	return LCD_KIT_OK;
}

static int dbg_rgbw_bl_max(int val)
{
	disp_info->rgbw.rgbw_bl_max = val;
	LCD_KIT_INFO("rgbw_bl_max = %d\n", disp_info->rgbw.rgbw_bl_max);
	return LCD_KIT_OK;
}

static int dbg_rgbw_set_mode1(char *buf, int len)
{
	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	lcd_kit_dbg_parse_cmd(&disp_info->rgbw.mode1_cmds, buf, len);
	return LCD_KIT_OK;
}

static int dbg_rgbw_set_mode2(char *buf, int len)
{
	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	lcd_kit_dbg_parse_cmd(&disp_info->rgbw.mode2_cmds, buf, len);
	return LCD_KIT_OK;
}

static int dbg_rgbw_set_mode3(char *buf, int len)
{
	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	lcd_kit_dbg_parse_cmd(&disp_info->rgbw.mode3_cmds, buf, len);
	return LCD_KIT_OK;
}

static int dbg_rgbw_set_mode4(char *buf, int len)
{
	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	lcd_kit_dbg_parse_cmd(&disp_info->rgbw.mode4_cmds, buf, len);
	return LCD_KIT_OK;
}

static int dbg_rgbw_backlight_cmd(char *buf, int len)
{
	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	lcd_kit_dbg_parse_cmd(&disp_info->rgbw.backlight_cmds, buf, len);
	return LCD_KIT_OK;
}

static int dbg_rgbw_pixel_gainlimit_cmd(char *buf, int len)
{
	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	lcd_kit_dbg_parse_cmd(&disp_info->rgbw.pixel_gain_limit_cmds, buf, len);
	return LCD_KIT_OK;
}

static int dbg_barcode_2d_cmd(char *buf, int len)
{
	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	lcd_kit_dbg_parse_cmd(&disp_info->oeminfo.barcode_2d.cmds, buf, len);
	return LCD_KIT_OK;
}

static int dbg_brightness_color_cmd(char *buf, int len)
{
	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	lcd_kit_dbg_parse_cmd(&disp_info->oeminfo.brightness_color_uniform.brightness_color_cmds,
		buf, len);
	return LCD_KIT_OK;
}

static int dbg_set_voltage(void)
{
	int ret;

	ret = lcd_kit_dbg_set_voltage();
	return ret;
}

static int dbg_dsi_cmds_rx(uint8_t *out, int out_len,
	struct lcd_kit_dsi_panel_cmds *cmds)
{
	int ret;
	uint8_t dsi1_value[LCD_KIT_ITEM_NAME_MAX] = { 0 };

	struct hisi_fb_data_type *hisifd = hisifd_list[PRIMARY_PANEL_IDX];

	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (!out || !cmds) {
		LCD_KIT_ERR("out is null or cmds is null\n");
		return LCD_KIT_FAIL;
	}

	if (is_dual_mipi_panel(hisifd)) {
		ret = lcd_kit_dual_dsi_cmds_rx(hisifd, out, dsi1_value, LCD_KIT_ITEM_NAME_MAX - 1, cmds);
		if (ret != LCD_KIT_OK) {
			LCD_KIT_INFO("dual_mipi_rx fail\n");
			return ret;
		}
		strncat(out, dsi1_value, strlen(dsi1_value));
		return ret;
	}
	return lcd_kit_dsi_cmds_rx(hisifd, out, out_len, cmds);
}

static bool dbg_panel_power_on(void)
{
	struct hisi_fb_data_type *hisifd = hisifd_list[PRIMARY_PANEL_IDX];
	bool panel_power_on = false;

	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null\n");
		return false;
	}
	down(&hisifd->blank_sem);
	panel_power_on = hisifd->panel_power_on;
	up(&hisifd->blank_sem);
	return panel_power_on;
}

static int dbg_esd_check_func(void)
{
	struct hisi_fb_data_type *hisifd = hisifd_list[PRIMARY_PANEL_IDX];
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	ret = init_panel_info(hisifd, &pinfo);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_INFO("init_panel_info fail\n");
		return ret;
	}
	pinfo->esd_enable = 1;
	if (hisifd->esd_register)
		hisifd->esd_register(hisifd->pdev);
	return LCD_KIT_OK;
}

struct lcd_kit_dbg_ops hisi_dbg_ops = {
	.fps_updt_support = dbg_fps_updt_support,
	.quickly_sleep_out_support = dbg_quickly_sleep_out_support,
	.blpwm_input_support = dbg_blpwm_input_support,
	.dsi_upt_support = dbg_dsi_upt_support,
	.rgbw_support = dbg_rgbw_support,
	.gamma_support = dbg_gamma_support,
	.gmp_support = dbg_gmp_support,
	.hiace_support = dbg_hiace_support,
	.xcc_support = dbg_xcc_support,
	.arsr1psharpness_support = dbg_arsr1psharpness_support,
	.prefixsharptwo_d_support = dbg_prefixsharptwo_d_support,
	.video_idle_mode_support = dbg_video_idle_mode_support,
	.cmd_type = dbg_cmd_type,
	.pxl_clk = dbg_pxl_clk,
	.pxl_clk_div = dbg_pxl_clk_div,
	.vsync_ctrl_type = dbg_vsync_ctrl_type,
	.hback_porch = dbg_hback_porch,
	.hfront_porch = dbg_hfront_porch,
	.hpulse_width = dbg_hpulse_width,
	.vback_porch = dbg_vback_porch,
	.vfront_porch = dbg_vfront_porch,
	.vpulse_width = dbg_vpulse_width,
	.mipi_burst_mode = dbg_mipi_burst_mode,
	.mipi_max_tx_esc_clk = dbg_mipi_max_tx_esc_clk,
	.mipi_dsi_bit_clk = dbg_mipi_dsi_bit_clk,
	.mipi_dsi_bit_clk_a = dbg_mipi_dsi_bit_clk_a,
	.mipi_dsi_bit_clk_b = dbg_mipi_dsi_bit_clk_b,
	.mipi_dsi_bit_clk_c = dbg_mipi_dsi_bit_clk_c,
	.mipi_dsi_bit_clk_d = dbg_mipi_dsi_bit_clk_d,
	.mipi_dsi_bit_clk_e = dbg_mipi_dsi_bit_clk_e,
	.mipi_noncontinue_enable = dbg_mipi_noncontinue_enable,
	.mipi_rg_vcm_adjust = dbg_mipi_rg_vcm_adjust,
	.mipi_clk_post_adjust = dbg_mipi_clk_post_adjust,
	.mipi_clk_pre_adjust = dbg_mipi_clk_pre_adjust,
	.mipi_clk_ths_prepare_adjust = dbg_mipi_clk_ths_prepare_adjust,
	.mipi_clk_tlpx_adjust = dbg_mipi_clk_tlpx_adjust,
	.mipi_clk_ths_trail_adjust = dbg_mipi_clk_ths_trail_adjust,
	.mipi_clk_ths_exit_adjust = dbg_mipi_clk_ths_exit_adjust,
	.mipi_clk_ths_zero_adjust = dbg_mipi_clk_ths_zero_adjust,
	.mipi_lp11_flag = dbg_mipi_lp11_flag,
	.mipi_phy_update = dbg_mipi_phy_update,
	.rgbw_bl_max = dbg_rgbw_bl_max,
	.rgbw_set_mode1 = dbg_rgbw_set_mode1,
	.rgbw_set_mode2 = dbg_rgbw_set_mode2,
	.rgbw_set_mode3 = dbg_rgbw_set_mode3,
	.rgbw_set_mode4 = dbg_rgbw_set_mode4,
	.rgbw_backlight_cmd = dbg_rgbw_backlight_cmd,
	.rgbw_pixel_gainlimit_cmd = dbg_rgbw_pixel_gainlimit_cmd,
	.barcode_2d_cmd = dbg_barcode_2d_cmd,
	.brightness_color_cmd = dbg_brightness_color_cmd,
	.set_voltage = dbg_set_voltage,
	.dbg_mipi_rx = dbg_dsi_cmds_rx,
	.panel_power_on = dbg_panel_power_on,
	.esd_check_func = dbg_esd_check_func,
};

int lcd_kit_dbg_init(void)
{
	int ret;

	ret = lcd_kit_debug_register(&hisi_dbg_ops);
	return ret;
}

static void dpd_power_init(struct device_node *np)
{
	/* vci */
	if (power_hdl->lcd_vci.buf == NULL)
		lcd_kit_parse_array_data(np, "lcd-kit,lcd-vci",
			&power_hdl->lcd_vci);
	/* iovcc */
	if (power_hdl->lcd_iovcc.buf == NULL)
		lcd_kit_parse_array_data(np, "lcd-kit,lcd-iovcc",
			&power_hdl->lcd_iovcc);
	/* vsp */
	if (power_hdl->lcd_vsp.buf == NULL)
		lcd_kit_parse_array_data(np, "lcd-kit,lcd-vsp",
			&power_hdl->lcd_vsp);
	/* vsn */
	if (power_hdl->lcd_vsn.buf == NULL)
		lcd_kit_parse_array_data(np, "lcd-kit,lcd-vsn",
			&power_hdl->lcd_vsn);
	/* backlight */
	if (power_hdl->lcd_backlight.buf == NULL)
		lcd_kit_parse_array_data(np, "lcd-kit,lcd-backlight",
			&power_hdl->lcd_backlight);
	/* vdd */
	if (power_hdl->lcd_vdd.buf == NULL)
		lcd_kit_parse_array_data(np, "lcd-kit,lcd-vdd",
			&power_hdl->lcd_vdd);
}

int dpd_init(struct platform_device *pdev)
{
	static bool is_first = true;
	struct device_node *np = NULL;

	if (!pdev) {
		LCD_KIT_ERR("pdev is null\n");
		return LCD_KIT_FAIL;
	}
	if (!g_dpd_mode)
		return LCD_KIT_OK;
	np = pdev->dev.of_node;
	if (!np) {
		LCD_KIT_ERR("NOT FOUND device node\n");
		return LCD_KIT_FAIL;
	}
	if (is_first) {
		is_first = false;
		dpd_power_init(np);
		lcd_kit_power_init(pdev);
		/* register lcd ops */
		lcd_kit_ops_register(&g_lcd_ops);
		INIT_DELAYED_WORK(&detect_fs_work, detect_fs_wq_handler);
		/* delay 500ms schedule work */
		schedule_delayed_work(&detect_fs_work, msecs_to_jiffies(500));
	}
	if (file_sys_is_ready()) {
		LCD_KIT_INFO("sysfs is not ready!\n");
		return LCD_KIT_FAIL;
	}
	return LCD_KIT_OK;
}

