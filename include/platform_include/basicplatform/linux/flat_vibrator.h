/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2011-2021. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_FLAT_VIBRATOR_H
#define _LINUX_FLAT_VIBRATOR_H

#define FLAT_VIBRATOR  "vibrator"

#define	PERIOD			(0x20) /* period 4ms, dirct current enable */

#define   SET_MODE		(0x01) /* direct mode */
#define	ISET_POWER		(0xF0)  /* power */

#define	TIMEOUT_MIN 	(35)

#define VIBRATOR_OFF 0
struct flat_vph_pwr_vol_vib_iset {
	int vph_voltage;
	int vreg_value;
};

struct flat_vibrator_platform_data {
	u8 low_freq;
	u8 low_power;
	u8 mode;
	u8 high_freq;
	u8 high_power;
};

#endif

