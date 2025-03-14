/* Copyright (c) Huawei Technologies Co., Ltd. 2013-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef DPU_FB_DEBUG_H
#define DPU_FB_DEBUG_H

#include <linux/console.h>
#include <linux/uaccess.h>
#include <linux/leds.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/raid/pq.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/backlight.h>
#include <linux/pwm.h>
#include <linux/pm_runtime.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/memblock.h>

#include <linux/spi/spi.h>

#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/gpio.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/pinctrl/consumer.h>
#include <linux/file.h>
#include <linux/dma-buf.h>
#include <linux/genalloc.h>
#include <linux/mm_iommu.h>
#if defined(CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#if defined(CONFIG_HUAWEI_DSM)
#define VACTIVE0_TIMEOUT_EXPIRE_COUNT 6
#define UNDERFLOW_EXPIRE_COUNT 6
#define UNDERFLOW_INTERVAL 1000
#define OFFLINE_COMPOSE_TIMEOUT_EXPECT_COUNT 6
#endif
#define ERR_RETURN_DUMP (0x12345678)

/* DSS debug tag func */
#ifdef CONFIG_DPU_FB_ENG_DBG
#define DDTF(tag, func)    \
	do {                    \
		if (tag) func();     \
	} while (0)
#else
#define DDTF(tag, func)
#endif
/******************************************************************************
 * FUNCTIONS PROTOTYPES
 */
extern int g_debug_ldi_underflow;
extern int g_debug_ldi_underflow_clear;
extern int g_dss_perf_debug;
extern uint32_t g_dsi_pipe_switch_connector;
extern uint32_t g_dss_dfr_debug;
extern int g_debug_panel_mode_switch;
extern int g_debug_mmu_error;
extern int g_debug_underflow_error;
extern int g_debug_set_reg_val;
extern int g_debug_online_vsync;
extern int g_debug_ovl_online_composer;
extern int g_debug_ovl_online_composer_hold;
extern int g_debug_ovl_online_composer_return;
extern uint32_t g_debug_ovl_online_composer_timediff;
extern uint32_t g_debug_ovl_online_composer_time_threshold;

extern int g_debug_ovl_offline_composer;
extern int g_debug_ovl_block_composer;
extern int g_debug_ovl_offline_composer_hold;
extern uint32_t g_debug_ovl_offline_composer_timediff;
extern int g_debug_ovl_offline_composer_time_threshold;
extern int g_debug_ovl_offline_block_num;
extern int g_enable_ovl_async_composer;
extern int g_debug_ovl_mediacommon_composer;
extern int g_debug_ovl_online_wb_count;

extern int g_debug_ovl_cmdlist;
extern int g_dump_cmdlist_content;
extern int g_enable_ovl_cmdlist_online;
extern int g_enable_video_idle_l3cache;
extern int g_smmu_global_bypass;
extern int g_enable_ovl_cmdlist_offline;
extern int g_rdma_stretch_threshold;
extern int g_enable_dirty_region_updt;
extern int g_enable_alsc;
extern int g_debug_dirty_region_updt;
extern int g_enable_mmbuf_debug;
extern int g_ldi_data_gate_en;
extern int g_debug_ovl_credit_step;
extern int g_debug_layerbuf_sync;
extern int g_debug_fence_timeline;
extern int g_enable_dss_idle;
extern int g_dss_effect_sharpness1D_en;
extern int g_dss_effect_sharpness2D_en;
extern int g_dss_effect_acm_ce_en;
extern int g_debug_dump_mmbuf;
extern int g_debug_dump_iova;
extern int g_debug_online_play_bypass;
extern int g_debug_dpp_cmdlist_dump;
extern int g_debug_dpp_cmdlist_type;
extern int g_debug_dpp_cmdlist_debug;
extern int g_debug_effect_hihdr;
extern int g_debug_mipi_phy_config;
extern uint32_t g_mmbuf_addr_test;
extern uint32_t g_dump_sensorhub_aod_hwlock;
extern uint32_t g_dss_min_bandwidth_inbusbusy;
extern uint32_t g_debug_wait_asy_vactive0_thr;
extern uint32_t g_debug_enable_asynchronous_play;

extern uint32_t g_err_status;
extern int g_debug_enable_lcd_sleep_in;
extern uint32_t g_underflow_count;

#if defined(CONFIG_HUAWEI_DSM)
extern struct dsm_client *lcd_dclient;
#endif

#if defined(CONFIG_POWER_DUBAI_RGB_STATS)
extern int g_debug_rgb_stats_enable;
#endif

void dpu_underflow_dump_cmdlist(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req_prev, dss_overlay_t *pov_req_prev_prev);

void dpufb_debug_register(struct platform_device *pdev);
void dpufb_debug_unregister(struct platform_device *pdev);

#if defined(CONFIG_HUAWEI_DSM)
void dss_underflow_debug_func(struct work_struct *work);
#endif
#endif /* DPU_FB_DEBUG_H */
