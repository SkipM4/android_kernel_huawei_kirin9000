/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HISI_DP_UTILS_H
#define HISI_DP_UTILS_H

#include "hisi_connector_utils.h"

struct hisi_connector_device *hisi_dp_create_platform_device(struct hisi_panel_device *panel_data,
		struct hisi_panel_ops *panel_ops);


#endif /* HISI_DP_UTILS_H */
