/* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef DP_LINK_LAYER_INTERFACE_H
#define DP_LINK_LAYER_INTERFACE_H

#include <linux/kernel.h>

struct dp_ctrl;

void dptx_link_layer_init(struct dp_ctrl *dptx);

#endif

