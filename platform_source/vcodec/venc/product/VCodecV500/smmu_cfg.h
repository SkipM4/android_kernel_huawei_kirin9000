/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: VCodecV520 venc smmu cfg file
 * Create: 2022-02-26
 */

#ifndef __VCODEC_VENC_SMMU_CFG_H__
#define __VCODEC_VENC_SMMU_CFG_H__

#ifdef VCODECV520
#define VCODEC_VENC_SMMU_TBU_OFFSET 0x20000
#define VCODEC_VENC_SMMU_PRE_OFFSET 0x18000

#define SMMU_PRE_SID_VALUE          9

#define SMMU_PRE_SID              0x0004
#define SMMU_PRE_SSID             0x0008
#define SMMU_PRE_SSIDV            0x000C

#endif

#endif
