/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sc8546.h
 *
 * sc8546 header file
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#ifndef _SC8546_H_
#define _SC8546_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/raid/pq.h>
#include <linux/bitops.h>
#include <chipset_common/hwpower/common_module/power_log.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_error_handle.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_ic.h>

#define SCP_MAX_DATA_LEN           32
#define UFCS_MAX_IRQ_LEN           2

struct sc8546_device_info {
	struct i2c_client *client;
	struct device *dev;
	struct work_struct irq_work;
	struct work_struct charge_work;
	struct notifier_block event_nb;
	struct nty_data notify_data;
	struct mutex scp_detect_lock;
	struct mutex accp_adapter_reg_lock;
	struct mutex ufcs_detect_lock;
	struct mutex ufcs_node_lock;
	struct dc_ic_ops sc_ops;
	struct dc_ic_ops lvc_ops;
	struct dc_batinfo_ops batinfo_ops;
	struct power_log_ops log_ops;
	char name[CHIP_DEV_NAME_LEN];
	int gpio_int;
	int irq_int;
	int irq_active;
	int chip_already_init;
	int device_id;
	int get_id_time;
	int i2c_recovery;
	int i2c_recovery_addr;
	bool init_finish_flag;
	bool int_notify_enable_flag;
	int dts_sw_freq;
	int dts_sw_freq_shift;
	int sense_r_actual;
	int sense_r_config;
	int dts_scp_support;
	int dts_fcp_support;
	int dts_ufcs_support;
	u8 ufcs_irq[UFCS_MAX_IRQ_LEN];
	u32 ic_role;
	u32 scp_error_flag;
	u8 scp_data[SCP_MAX_DATA_LEN];
	u8 scp_op;
	u8 scp_op_num;
	bool scp_trans_done;
	bool fcp_support;
	bool ufcs_communicating_flag;
};

struct sc8646_dump_value {
	int vbus;
	int ibus;
	int vbat;
	int ibat;
	int vusb;
	int vout;
	int temp;
};

#define SC8546_BUF_LEN                         80
#define SC8546_DBG_VAL_SIZE                    6
#define SC8546_REG_RESET                       1
#define SC8546_SWITCHCAP_DISABLE               0
#define SC8546_SOFT_INT_CHECK_TIME             500

#define SC8546_NOT_USED                        0
#define SC8546_USED                            1

/* commom para init */
#define SC8546_VBAT_OVP_TH_INIT                5000 /* mv */
#define SC8546_IBAT_OCP_TH_INIT                8300 /* ma */
#define SC8546_VBUS_OVP_TH_INIT                11500 /* mv */
#define SC8546_IBUS_OCP_TH_INIT                3000 /* ma */
#define SC8546_VAC_OVP_TH_INIT                 12000 /* mv */
#define SC8546_IBAT_REGULATION_TH_INIT         200 /* ma */
#define SC8546_VBAT_REGULATION_TH_INIT         50 /* mv */
/* lvc para init */
#define SC8546_LVC_IBUS_OCP_TH_INIT            6000 /* ma */
/* sc para init */
#define SC8546_SC_IBUS_OCP_TH_INIT             3600 /* ma */
/* default para init */
#define SC8546_DFT_VBUS_OVP_TH_INIT            6500 /* mv */
#define SC8546_DFT_VAC_OVP_TH_INIT             6000 /* mv */

/* VBAT_OVP reg=0x00 */
#define SC8546_VBAT_OVP_REG                    0x00
#define SC8546_VBAT_OVP_DIS_MASK               BIT(7)
#define SC8546_VBAT_OVP_DIS_SHIFT              7
#define SC8546_VBAT_OVP_MASK                   (BIT(5) | BIT(4) | BIT(3) | \
	BIT(2) | BIT(1) | BIT(0))
#define SC8546_VBAT_OVP_SHIFT                  0
#define SC8546_VBAT_OVP_STEP                   25 /* mv */
#define SC8546_VBAT_OVP_MAX                    5075 /* mv */
#define SC8546_VBAT_OVP_BASE                   3500 /* mv */
#define SC8546_VBAT_OVP_DEFAULT                4350 /* mv */

/* IBAT_OCP reg=0x01 */
#define SC8546_IBAT_OCP_REG                    0x01
#define SC8546_IBAT_OCP_DIS_MASK               BIT(7)
#define SC8546_IBAT_OCP_DIS_SHIFT              7
#define SC8546_IBAT_OCP_MASK                   (BIT(5) | BIT(4) | BIT(3) | \
	BIT(2) | BIT(1) | BIT(0))
#define SC8546_IBAT_OCP_SHIFT                  0
#define SC8546_IBAT_OCP_MAX                    8300 /* ma */
#define SC8546_IBAT_OCP_BASE                   2000 /* ma */
#define SC8546_IBAT_OCP_DEFAULT                5000 /* ma */
#define SC8546_IBAT_OCP_STEP                   100 /* ma */

/* VAC_OVP reg=0x02 */
#define SC8546_VAC_OVP_REG                     0x02
#define SC8546_VAC_OVP_DIS_MASK                BIT(7)
#define SC8546_VAC_OVP_DIS_SHIFT               7
#define SC8546_OVPGATE_DIS_MASK                BIT(6)
#define SC8546_OVPGATE_DIS_SHIFT               6
#define SC8546_VAC_OVP_STAT_MASK               BIT(5)
#define SC8546_VAC_OVP_STAT_SHIFT              5
#define SC8546_VAC_OVP_FLAG_MASK               BIT(4)
#define SC8546_VAC_OVP_FLAG_SHIFT              4
#define SC8546_VAC_OVP_MSK_MASK                BIT(3)
#define SC8546_VAC_OVP_MSK_SHIFT               3
#define SC8546_VAC_OVP_MASK                    (BIT(2) | BIT(1) | BIT(0))
#define SC8546_VAC_OVP_SHIFT                   0
#define SC8546_VAC_OVP_MAX                     17000 /* mv */
#define SC8546_VAC_OVP_BASE                    11000 /* mv */
#define SC8546_VAC_OVP_DEFAULT                 18000 /* actual 6.5v */
#define SC8546_VAC_OVP_STEP                    1000 /* mv */

/* VDROP_OVP reg=0x03 */
#define SC8546_VDROP_OVP_REG                   0x03
#define SC8546_VDROP_OVP_DIS_MASK              BIT(7)
#define SC8546_VDROP_OVP_DIS_SHIFT             7
#define SC8546_VDROP_OVP_STAT_MASK             BIT(4)
#define SC8546_VDROP_OVP_STAT_SHIFT            4
#define SC8546_VDROP_OVP_FLAG_MASK             BIT(3)
#define SC8546_VDROP_OVP_FLAG_SHIFT            3
#define SC8546_VDROP_OVP_MSK_MASK              BIT(2)
#define SC8546_VDROP_OVP_MSK_SHIFT             2
#define SC8546_VDROP_OVP_THLD_MASK             BIT(1)
#define SC8546_VDROP_OVP_THLD_SHIFT            1
#define SC8546_VDROP_DEGLITCH_SET_MASK         BIT(0)
#define SC8546_VDROP_DEGLITCH_SET_SHIFT        0

/* VBUS_OVP reg=0x04 */
#define SC8546_VBUS_OVP_REG                    0x04
#define SC8546_VBUS_OVP_DIS_MASK               BIT(7)
#define SC8546_VBUS_OVP_DIS_SHIFT              7
#define SC8546_VBUS_OVP_MASK                   (BIT(6) | BIT(5) | BIT(4) | \
	BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8546_VBUS_OVP_SHIFT                  0
#define SC8546_VBUS_OVP_MAX                    12350 /* mv */
#define SC8546_VBUS_OVP_BASE                   6000 /* mv */
#define SC8546_VBUS_OVP_DEFAULT                8900 /* mv */
#define SC8546_VBUS_OVP_STEP                   50 /* mv */

/* IBUS_OCP_UCP reg=0x05 */
#define SC8546_IBUS_OCP_UCP_REG                0x05
#define SC8546_IBUS_UCP_DIS_MASK               BIT(7)
#define SC8546_IBUS_UCP_DIS_SHIFT              7
#define SC8546_IBUS_OCP_DIS_MASK               BIT(6)
#define SC8546_IBUS_OCP_DIS_SHIFT              6
#define SC8546_IBUS_UCP_DEGLITCH_MASK          BIT(5)
#define SC8546_IBUS_UCP_DEGLITCH_SHIFT         5
#define SC8546_IBUS_UCP_DEGLITCH_5MS           1
#define SC8546_IBUS_UCP_DEGLITCH_10US          0
#define SC8546_IBUS_OCP_MASK                   (BIT(3) | BIT(2) | \
	BIT(1) | BIT(0))
#define SC8546_IBUS_OCP_SHIFT                  0
#define SC8546_IBUS_OCP_BASE                   1200 /* ma */
#define SC8546_IBUS_OCP_STEP                   300 /* ma */
#define SC8546_IBUS_OCP_MAX                    5700 /* ma */

/* CONVERTER_STATE reg=0x06 */
#define SC8546_CONVERTER_STATE_REG             0x06
#define SC8546_CONVERTER_STATE_REG_INIT        0
#define SC8546_TSHUT_FLAG_MASK                 BIT(7)
#define SC8546_TSHUT_FLAG_SHIFT                7
#define SC8546_TSHUT_STAT_MASK                 BIT(6)
#define SC8546_TSHUT_STAT_SHIFT                6
#define SC8546_VBUS_ERRORLO_STAT_MASK          BIT(5)
#define SC8546_VBUS_ERRORLO_STAT_SHIFT         5
#define SC8546_VBUS_ERRORHI_STAT_MASK          BIT(4)
#define SC8546_VBUS_ERRORHI_STAT_SHIFT         4
#define SC8546_SS_TIMEOUT_FLAG_MASK            BIT(3)
#define SC8546_SS_TIMEOUT_FLAG_SHIFT           3
#define SC8546_CP_SWITCHING_STAT_MASK          BIT(2)
#define SC8546_CP_SWITCHING_STAT_SHIFT         2
#define SC8546_REG_TIMEOUT_FLAG_MASK           BIT(1)
#define SC8546_REG_TIMEOUT_FLAG_SHIFT          1
#define SC8546_PIN_DIAG_FAIL_FLAG_MASK         BIT(0)
#define SC8546_PIN_DIAG_FAIL_FLAG_SHIFT        0

/* CTRL1 reg=0x07 */
#define SC8546_CTRL1_REG                       0x07
#define SC8546_CTRL1_REG_INIT                  0X0e
#define SC8546_CHG_CTRL_MASK                   BIT(7)
#define SC8546_CHG_CTRL_SHIFT                  7
#define SC8546_REG_RST_MASK                    BIT(6)
#define SC8546_REG_RST_SHIFT                   6
#define SC8546_FREQ_SHIFT_MASK                 (BIT(4) | BIT(3))
#define SC8546_FREQ_SHIFT_SHIFT                3
#define SC8546_SW_FREQ_MASK                    (BIT(2) | BIT(1) | BIT(0))
#define SC8546_SW_FREQ_SHIFT                   0
#define SC8546_REG_RST_ENABLE                  1
#define SC8546_CHG_CTRL_ENABLE                 1
#define SC8546_CHG_CTRL_DISABLE                0
#define SC8546_SW_FREQ_MIN                     500
#define SC8546_SW_FREQ_MAX                     850
#define SC8546_SW_FREQ_STEP                    50
#define SC8546_SW_FREQ_SHIFT_NORMAL            0
#define SC8546_SW_FREQ_SHIFT_P_P10             1 /* +10% */
#define SC8546_SW_FREQ_SHIFT_M_P10             2 /* -10% */
#define SC8546_SW_FREQ_SHIFT_MP_P10            3 /* +/-10% */

/* CTRL2 reg=0x08 */
#define SC8546_CTRL2_REG                       0x08
#define SC8546_CTRL2_REG_INTI                  0x08
#define SC8546_SS_TIMEOUT_MASK                 (BIT(7) | BIT(6) | BIT(5))
#define SC8546_SS_TIMEOUT_SHIFT                5
#define SC8546_REG_TIMEOUT_DIS_MASK            BIT(4)
#define SC8546_REG_TIMEOUT_DIS_SHIFT           4
#define SC8546_VOUT_OVP_DIS_MASK               BIT(3)
#define SC8546_VOUT_OVP_DIS_SHIFT              3
#define SC8546_SET_IBAT_SNS_RES_MASK           BIT(2)
#define SC8546_SET_IBAT_SNS_RES_SHIFT          2
#define SC8546_VBUS_PD_EN_MASK                 BIT(1)
#define SC8546_VBUS_PD_EN_SHIFT                1
#define SC8546_VAC_PD_EN_MASK                  BIT(0)
#define SC8546_VAC_PD_EN_SHIFT                 0
#define SC8546_IBAT_SNS_RES_5MOHM              0
#define SC8546_IBAT_SNS_RES_2MOHM              1

/* CTRL3 reg=0x09 */
#define SC8546_CTRL3_REG                       0x09
#define SC8546_CTRL3_REG_INTI                  0x10
#define SC8546_CHG_MODE_MASK                   BIT(7)
#define SC8546_CHG_MODE_SHIFT                  7
#define SC8546_POR_FLAG_MASK                   BIT(6)
#define SC8546_POR_FLAG_SHIFT                  6
#define SC8546_IBUS_UCP_RISE_FLAG_MASK         BIT(5)
#define SC8546_IBUS_UCP_RISE_FLAG_SHIFT        5
#define SC8546_IBUS_UCP_RISE_MSK_MASK          BIT(4)
#define SC8546_IBUS_UCP_RISE_MSK_SHIFT         4
#define SC8546_WTD_TIMEOUT_FLAG_MASK           BIT(3)
#define SC8546_WTD_TIMEOUT_FLAG_SHIFT          3
#define SC8546_WTD_TIMEOUT_MASK                (BIT(2) | BIT(1) | BIT(0))
#define SC8546_WTD_TIMEOUT_SHIFT               0
#define SC8546_CHG_MODE_CHGPUMP                0
#define SC8546_CHG_MODE_BYPASS                 1
#define SC8546_CHG_MODE_BUCK                   2
#define SC8546_WTD_CONFIG_TIMING_DIS           0
#define SC8546_WTD_CONFIG_TIMING_200MS         200
#define SC8546_WTD_CONFIG_TIMING_500MS         500
#define SC8546_WTD_CONFIG_TIMING_1000MS        1000
#define SC8546_WTD_CONFIG_TIMING_5000MS        5000
#define SC8546_WTD_CONFIG_TIMING_30000MS       30000
#define SC8546_WTD_SET_30000MS                 5
#define SC8546_WTD_SET_5000MS                  4
#define SC8546_WTD_SET_1000MS                  3
#define SC8546_WTD_SET_500MS                   2
#define SC8546_WTD_SET_200MS                   1

/* VBAT_REGULATION reg=0x0A */
#define SC8546_VBATREG_REG                     0x0A
#define SC8546_VBATREG_EN_MASK                 BIT(7)
#define SC8546_VBATREG_EN_SHIFT                7
#define SC8546_VBATREG_ACTIVE_STAT_MASK        BIT(4)
#define SC8546_VBATREG_ACTIVE_STAT_SHIFT       4
#define SC8546_VBATREG_ACTIVE_FLAG_MASK        BIT(3)
#define SC8546_VBATREG_ACTIVE_FLAG_SHIFT       3
#define SC8546_VBATREG_ACTIVE_MSK_MASK         BIT(2)
#define SC8546_VBATREG_ACTIVE_MSK_SHIFT        2
#define SC8546_VBATREG_BELOW_OVP_MASK          (BIT(1) | BIT(0))
#define SC8546_VBATREG_BELOW_OVP_SHIFT         0
#define SC8546_VBATREG_BELOW_OVP_BASE          50 /* mv */
#define SC8546_VBATREG_BELOW_OVP_STEP          50 /* mv */
#define SC8546_VBATREG_BELOW_OVP_MAX           200 /* mv */
#define SC8546_VBATREG_EN                      1

/* IBAT_REGULATION reg=0x0B */
#define SC8546_IBATREG_REG                     0x0B
#define SC8546_IBATREG_EN_MASK                 BIT(7)
#define SC8546_IBATREG_EN_SHIFT                7
#define SC8546_IBATREG_ACTIVE_STAT_MASK        BIT(4)
#define SC8546_IBATREG_ACTIVE_STAT_SHIFT       4
#define SC8546_IBATREG_ACTIVE_FLAG_MASK        BIT(3)
#define SC8546_IBATREG_ACTIVE_FLAG_SHIFT       3
#define SC8546_IBATREG_ACTIVE_MSK_MASK         BIT(2)
#define SC8546_IBATREG_ACTIVE_MSK_SHIFT        2
#define SC8546_IBATREG_BELOW_OCP_MASK          (BIT(1) | BIT(0))
#define SC8546_IBATREG_BELOW_OCP_SHIFT         0

#define SC8546_IBATREG_BELOW_OCP_BASE          200 /* ma */
#define SC8546_IBATREG_BELOW_OCP_STEP          100 /* ma */
#define SC8546_IBATREG_BELOW_OCP_MAX           500 /* ma */
#define SC8546_IBATREG_EN                      1

/* CTRL4 reg=0x0C */
#define SC8546_CTRL4_REG                       0x0C
#define SC8546_VBUS_UVLO_MAS_MASK              BIT(3)
#define SC8546_VBUS_UVLO_MAS_SHIFT             3
#define SC8546_VBUS_UVLO_FLAG_MASK             BIT(2)
#define SC8546_VBUS_UVLO_FLAG_SHIFT            2
#define SC8546_FORVE_VAC_MASK                  BIT(1)
#define SC8546_FORVE_VAC_SHIFT                 1
#define SC8546_ACDRV_HI_EN_MASK                BIT(0)
#define SC8546_ACDRV_HI_EN_SHIFT               0

/* PMID2OUT_OVP_UVP reg=0x0D */
#define SC8546_PMID2OUT_OVP_UVP_REG            0x0D
#define SC8546_PMID2OUT_UVP_MASK               (BIT(7) | BIT(6))
#define SC8546_PMID2OUT_UVP_SHIFT              6
#define SC8546_PMID2OUT_OVP_MASK               (BIT(5) | BIT(4))
#define SC8546_PMID2OUT_OVP_SHIFT              4
#define SC8546_PMID2OUT_UVP_FLAG_MASK          BIT(3)
#define SC8546_PMID2OUT_UVP_FLAG_SHIFT         3
#define SC8546_PMID2OUT_OVP_FLAG_MASK          BIT(2)
#define SC8546_PMID2OUT_OVP_FLAG_SHIFT         2
#define SC8546_PMID2OUT_UVP_STAT_MASK          BIT(1)
#define SC8546_PMID2OUT_UVP_STAT_SHIFT         1
#define SC8546_PMID2OUT_OVP_STAT_MASK          BIT(0)
#define SC8546_PMID2OUT_OVP_STAT_SHIFT         0

/* INT_STAT reg=0x0E */
#define SC8546_INT_STAT_REG                    0x0E
#define SC8546_VOUT_OVP_STAT_MASK              BIT(7)
#define SC8546_VOUT_OVP_STAT_SHIFT             7
#define SC8546_VBAT_OVP_STAT_MASK              BIT(6)
#define SC8546_VBAT_OVP_STAT_SHIFT             6
#define SC8546_IBAT_OCP_STAT_MASK              BIT(5)
#define SC8546_IBAT_OCP_STAT_SHIFT             5
#define SC8546_VBUS_OVP_STAT_MASK              BIT(4)
#define SC8546_VBUS_OVP_STAT_SHIFT             4
#define SC8546_IBUS_OCP_STAT_MASK              BIT(3)
#define SC8546_IBUS_OCP_STAT_SHIFT             3
#define SC8546_IBUS_UCP_FALL_STAT_MASK         BIT(2)
#define SC8546_IBUS_UCP_FALL_STAT_SHIFT        2
#define SC8546_ADAPTER_INSERT_STAT_MASK        BIT(1)
#define SC8546_ADAPTER_INSERT_STAT_SHIFT       1
#define SC8546_VBAT_INSERT_STAT_MASK           BIT(0)
#define SC8546_VBAT_INSERT_STAT_SHIFT          0

/* INT_FLAG reg=0x0F */
#define SC8546_INT_FLAG_REG                    0x0F
#define SC8546_VOUT_OVP_FLAG_MASK              BIT(7)
#define SC8546_VOUT_OVP_FLAG_SHIFT             7
#define SC8546_VBAT_OVP_FLAG_MASK              BIT(6)
#define SC8546_VBAT_OVP_FLAG_SHIFT             6
#define SC8546_IBAT_OCP_FLAG_MASK              BIT(5)
#define SC8546_IBAT_OCP_FLAG_SHIFT             5
#define SC8546_VBUS_OVP_FLAG_MASK              BIT(4)
#define SC8546_VBUS_OVP_FLAG_SHIFT             4
#define SC8546_IBUS_OCP_FLAG_MASK              BIT(3)
#define SC8546_IBUS_OCP_FLAG_SHIFT             3
#define SC8546_IBUS_UCP_FALL_FLAG_MASK         BIT(2)
#define SC8546_IBUS_UCP_FALL_FLAG_SHIFT        2
#define SC8546_ADAPTER_INSERT_FLAG_MASK        BIT(1)
#define SC8546_ADAPTER_INSERT_FLAG_SHIFT       1
#define SC8546_VBAT_INSERT_FLAG_MASK           BIT(0)
#define SC8546_VBAT_INSERT_FLAG_SHIFT          0

/* INT_MASK reg=0x10 */
#define SC8546_INT_MASK_REG                    0x10
#define SC8546_INT_MASK_REG_INIT               0xFF
#define SC8546_VOUT_OVP_MSK_MASK               BIT(7)
#define SC8546_VOUT_OVP_MSK_SHIFT              7
#define SC8546_VBAT_OVP_MSK_MASK               BIT(6)
#define SC8546_VBAT_OVP_MSK_SHIFT              6
#define SC8546_IBAT_OCP_MSK_MASK               BIT(5)
#define SC8546_IBAT_OCP_MSK_SHIFT              5
#define SC8546_VBUS_OVP_MSK_MASK               BIT(4)
#define SC8546_VBUS_OVP_MSK_SHIFT              4
#define SC8546_IBUS_OCP_MSK_MASK               BIT(3)
#define SC8546_IBUS_OCP_MSK_SHIFT              3
#define SC8546_IBUS_UCP_FALL_MSK_MASK          BIT(2)
#define SC8546_IBUS_UCP_FALL_MSK_SHIFT         2
#define SC8546_ADAPTER_INSERT_MSK_MASK         BIT(1)
#define SC8546_ADAPTER_INSERT_MSK_SHIFT        1
#define SC8546_VBAT_INSERT_MSK_MASK            BIT(0)
#define SC8546_VBAT_INSERT_MSK_SHIFT           0

/* ADCCTRL reg=0x11 */
#define SC8546_ADCCTRL_REG                     0x11
#define SC8546_ADCCTRL_REG_INIT                0
#define SC8546_ADC_EN_MASK                     BIT(7)
#define SC8546_ADC_EN_SHIFT                    7
#define SC8546_ADC_RATE_MASK                   BIT(6)
#define SC8546_ADC_RATE_SHIFT                  6
#define SC8546_ADC_DONE_STAT_MASK              BIT(2)
#define SC8546_ADC_DONE_STAT_SHIFT             2
#define SC8546_ADC_DONE_FLAG_MASK              BIT(1)
#define SC8546_ADC_DONE_FLAG_SHIFT             1
#define SC8546_ADC_DONE_MSK_MASK               BIT(0)
#define SC8546_ADC_DONE_MSK_SHIFT              0

/* ADC_FN_DISABLE reg=0x12 */
#define SC8546_ADC_FN_DIS_REG                  0x12
#define SC8546_ADC_FN_DIS_REG_INIT             0x02
#define SC8546_VBUS_ADC_DIS_MASK               BIT(6)
#define SC8546_VBUS_ADC_DIS_SHIFT              6
#define SC8546_VAC_ADC_DIS_MASK                BIT(5)
#define SC8546_VAC_ADC_DIS_SHIFT               5
#define SC8546_VOUT_ADC_DIS_MASK               BIT(4)
#define SC8546_VOUT_ADC_DIS_SHIFT              4
#define SC8546_VBAT_ADC_DIS_MASK               BIT(3)
#define SC8546_VBAT_ADC_DIS_SHIFT              3
#define SC8546_IBAT_ADC_DIS_MASK               BIT(2)
#define SC8546_IBAT_ADC_DIS_SHIFT              2
#define SC8546_IBUS_ADC_DIS_MASK               BIT(1)
#define SC8546_IBUS_ADC_DIS_SHIFT              1
#define SC8546_TDIE_ADC_DIS_MASK               BIT(0)
#define SC8546_TDIE_ADC_DIS_SHIFT              0

/*
 * IBUS_ADC1 reg=0x13 IBUS=ibus_adc[3:0]*1.875ma
 * IBUS_ADC0 reg=0x14
 */
#define SC8546_IBUS_ADC1_REG                   0x13
#define SC8546_IBUS_ADC0_REG                   0x14
#define SC8546_IBUS_ADC_STEP                   1875 /* ua */

/*
 * VBUS_ADC1 reg=0x15 VBUS=vbus_adc[3:0]*3.75mv
 * VBUS_ADC0 reg=0x16
 */
#define SC8546_VBUS_ADC1_REG                   0x15
#define SC8546_VBUS_ADC0_REG                   0x16
#define SC8546_VBUS_ADC_STEP                   3750 /* uv */

/*
 * VAC_ADC1 reg=0x17 VAC=vac_adc[3:0]*5mv
 * VAC_ADC0 reg=0x18
 */
#define SC8546_VAC_ADC1_REG                    0x17
#define SC8546_VAC_ADC0_REG                    0x18
#define SC8546_VAC_ADC_STEP                    5 /* mv */

/*
 * VOUT_ADC1 reg=0x19 VOUT=vout_adc[3:0]*1.25mv
 * VOUT_ADC0 reg=0x1A
 */
#define SC8546_VOUT_ADC1_REG                   0x19
#define SC8546_VOUT_ADC0_REG                   0x1A
#define SC8546_VOUT_ADC_STEP                   1250 /* uv */

/*
 * VBAT_ADC1 reg=0x1B VBAT=vbat_adc[3:0]*1.25mv
 * VBAT_ADC0 reg=0x1C
 */
#define SC8546_VBAT_ADC1_REG                   0x1B
#define SC8546_VBAT_ADC0_REG                   0x1C
#define SC8546_VBAT_ADC_STEP                   1250 /* uv */

/*
 * IBAT_ADC1 reg=0x1D IBAT=ibat_adc[3:0]*3.125ma
 * IBAT_ADC0 reg=0x1E
 */
#define SC8546_IBAT_ADC1_REG                   0x1D
#define SC8546_IBAT_ADC0_REG                   0x1E
#define SC8546_IBAT_ADC_STEP                   3125 /* uma */

/*
 * TDIE_ADC1 reg=0x1F TDIE=tdie_adc[0]*0.5c
 * TDIE_ADC0 reg=0x20
 */
#define SC8546_TDIE_ADC1_REG                   0x1F
#define SC8546_TDIE_ADC0_REG                   0x20
#define SC8546_TDIE_ADC_STEP                   5 /* 0.5c */

/* DEVICE_ID reg=0x36 */
#define SC8546_DEVICE_ID_REG                   0x36
#define SC8546_DEVICE_ID_VALUE                 0x67

/* DPDM_CTRL1 reg=0x21 */
#define SC8546_DPDM_CTRL1_REG                  0x21
#define SC8546_DM_500K_PD_EN_MASK              BIT(7)
#define SC8546_DM_500K_PD_EN_SHIFT             7
#define SC8546_DP_500K_PD_EN_MASK              BIT(6)
#define SC8546_DP_500K_PD_EN_SHIFT             6
#define SC8546_DM_20K_PD_EN_MASK               BIT(5)
#define SC8546_DM_20K_PD_EN_SHIFT              5
#define SC8546_DP_20K_PD_EN_MASK               BIT(4)
#define SC8546_DP_20K_PD_EN_SHIFT              4
#define SC8546_DM_SINK_EN_MASK                 BIT(3)
#define SC8546_DM_SINK_EN_SHIFT                3
#define SC8546_DP_SINK_EN_MASK                 BIT(2)
#define SC8546_DP_SINK_EN_SHIFT                2
#define SC8546_DP_SRC_10UA_MASK                BIT(1)
#define SC8546_DP_SRC_10UA_SHIFT               1
#define SC8546_DPDM_EN_MASK                    BIT(0)
#define SC8546_DPDM_EN_SHIFT                   0

/* SCP_CTRL reg=0x25 */
#define SC8546_SCP_CTRL_REG                    0x25
#define SC8546_SCP_EN_MASK                     BIT(7)
#define SC8546_SCP_EN_SHIFT                    7
#define SC8546_SCP_SOFT_RST_MASK               BIT(6)
#define SC8546_SCP_SOFT_RST_SHIFT              6
#define SC8546_TX_CRC_DIS_MASK                 BIT(5)
#define SC8546_TX_CRC_DIS_SHIFT                5
#define SC8546_DM_3P3_EN_MASK                  BIT(4)
#define SC8546_DM_3P3_EN_SHIFT                 4
#define SC8546_CLR_TX_FIFO_MASK                BIT(3)
#define SC8546_CLR_TX_FIFO_SHIFT               3
#define SC8546_CLR_RX_FIFO_MASK                BIT(2)
#define SC8546_CLR_RX_FIFO_SHIFT               2
#define SC8546_SND_RST_TRANS_MASK              BIT(1)
#define SC8546_SND_RST_TRANS_SHIFT             1
#define SC8546_SND_START_TRANS_MASK            BIT(0)
#define SC8546_SND_START_TRANS_SHIFT           0
#define SC8546_SCP_ENABLE                      1
#define SC8546_SCP_DISABLE                     0
#define SC8546_SCP_TRANS_RESET                 1

/* COMPATY_PROTOCOL reg=0x3D */
#define SC8546_COMPATY_PROTOCOL_REG            0x3D
#define SC8546_SCP_PROTOCOL_VAL                0x09
#define SC8546_UFCS_PROTOCOL_VAL               0x2A

/* CTL1 reg=0x40 */
#define SC8546_UFCS_CTL1_REG                   0x40
#define SC8546_UFCS_CTL1_EN_PROTOCOL_MASK      BIT(7)
#define SC8546_UFCS_CTL1_EN_PROTOCOL_SHIFT     7
#define SC8546_UFCS_CTL1_EN_HANDSHAKE_MASK     BIT(5)
#define SC8546_UFCS_CTL1_EN_HANDSHAKE_SHIFT    5
#define SC8546_UFCS_CTL1_BAUD_RATE_MASK        (BIT(4) | BIT(3))
#define SC8546_UFCS_CTL1_BAUD_RATE_SHIFT       3
#define SC8546_UFCS_CTL1_SEND_MASK             BIT(2)
#define SC8546_UFCS_CTL1_SEND_SHIFT            2
#define SC8546_UFCS_CTL1_CABLE_HARDRESET_MASK  BIT(1)
#define SC8546_UFCS_CTL1_CABLE_HARDRESET_SHIFT 1
#define SC8546_UFCS_CTL1_SOUR_HARDRESET_MASK   BIT(0)
#define SC8546_UFCS_CTL1_SOUR_HARDRESET_SHIFT  0

int sc8546_config_vac_ovp_th_mv(struct sc8546_device_info *di, int ovp_th);
int sc8546_config_vbus_ovp_th_mv(struct sc8546_device_info *di, int ovp_th);
int sc8546_get_vbus_mv(int *vbus, void *dev_data);

#endif /* _SC8546_H_ */
