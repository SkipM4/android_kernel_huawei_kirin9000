

/* 1 头文件包含 */
#include "hmac_dscr_th_opt.h"
#include "hmac_resource.h"
#include "mac_data.h"
#include "mac_frame.h"
#include "oam_ext_if.h"
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_DSCR_TH_OPT_C
/* 2 全局变量定义 */
oal_int32 g_l_large_pps_start_level = -20000;
oal_uint32 g_ul_large_pps_interval = 10000;
oal_int32 g_l_small_pps_start_level = -2000;
oal_uint32 g_ul_small_pps_interval = 1000;

oal_int32 g_al_delta_large_pps[HMAC_LARGE_DSCR_PPS_LEVEL];
oal_int32 g_al_delta_small_pps[HMAC_SMALL_DSCR_PPS_LEVEL];

oal_uint32 g_ul_large_start_th = 12;
oal_uint32 g_ul_large_interval = 3;
oal_uint32 g_ul_small_start_th = 12;
oal_uint32 g_ul_small_interval = 6;

oal_uint32 g_aul_theshold_large_dscr[HMAC_LARGE_DSCR_TH_LEVEL];
oal_uint32 g_aul_theshold_small_dscr[HMAC_SMALL_DSCR_TH_LEVEL];

/* 3 函数实现 */

OAL_STATIC oal_void hmac_calculate_dscr_th(oal_int32 l_delta_trx_large_pps, oal_int32 l_delta_trx_small_pps,
                                           oal_uint32 *pul_theshold_large_dscr, oal_uint32 *pul_theshold_small_dscr)
{
    oal_uint32 ul_index;

    /* 根据tx/rx pps差值分别计算大、小包门限 */
    if (l_delta_trx_large_pps <= g_al_delta_large_pps[0]) {
        *pul_theshold_large_dscr = g_aul_theshold_large_dscr[0];
    } else {
        for (ul_index = 0; ul_index < HMAC_LARGE_DSCR_PPS_LEVEL - 1; ul_index++) {
            if ((l_delta_trx_large_pps > g_al_delta_large_pps[ul_index]) &&
                (l_delta_trx_large_pps <= g_al_delta_large_pps[ul_index + 1])) {
                *pul_theshold_large_dscr = g_aul_theshold_large_dscr[ul_index + 1];
                break;
            }
        }

        if (ul_index == HMAC_LARGE_DSCR_PPS_LEVEL - 1) {
            *pul_theshold_large_dscr = g_aul_theshold_large_dscr[HMAC_LARGE_DSCR_TH_LEVEL - 1];
        }
    }

    if (l_delta_trx_small_pps <= g_al_delta_small_pps[0]) {
        *pul_theshold_small_dscr = g_aul_theshold_small_dscr[0];
    } else {
        for (ul_index = 0; ul_index < HMAC_SMALL_DSCR_PPS_LEVEL - 1; ul_index++) {
            if ((l_delta_trx_small_pps > g_al_delta_small_pps[ul_index]) &&
                (l_delta_trx_small_pps <= g_al_delta_small_pps[ul_index + 1])) {
                *pul_theshold_small_dscr = g_aul_theshold_small_dscr[ul_index + 1];
                break;
            }
        }

        if (ul_index == HMAC_SMALL_DSCR_PPS_LEVEL - 1) {
            *pul_theshold_small_dscr = g_aul_theshold_small_dscr[HMAC_SMALL_DSCR_TH_LEVEL - 1];
        }
    }
}


OAL_STATIC oal_bool_enum_uint8 hmac_flow_type_large_tx(hmac_dscr_th_opt_stru *pst_dscr_th_opt)
{
    /* 大包tx pps与大包rx pps比例高于一定数值即认为当前为大包发送场景 */
    return pst_dscr_th_opt->ul_tx_large_pps > pst_dscr_th_opt->ul_rx_small_pps * HMAC_TX_PPS_RATIO;
}


OAL_STATIC oal_uint16 hmac_get_large_dscr_limit(hmac_dscr_th_opt_stru *pst_dscr_th_opt)
{
    pst_dscr_th_opt->en_flow_type_tx = hmac_flow_type_large_tx(pst_dscr_th_opt);

    /* 发送场景限制接收描述符数量, 其他场景(接收/混合)不限制 */
    return pst_dscr_th_opt->en_flow_type_tx ? HMAC_LARGE_DSCR_TH_LIMIT_TX : HMAC_LARGE_DSCR_TH_LIMIT_OTHER;
}


OAL_STATIC oal_void hmac_rx_dscr_base_th_update(hmac_dscr_th_opt_stru *pst_dscr_th_opt)
{
    oal_uint32 ul_index;
    oal_uint32 ul_base_th;
    oal_bool_enum_uint8 en_need_update = OAL_FALSE;

    /* 接收amsdu多于一定数量时额外分配大包描述符 */
    if (pst_dscr_th_opt->ul_rx_large_amsdu_pps > HMAC_DSCR_TH_UPDATE_AMSDU_TH) {
        /* Rx amsdu pps每增加150多分配1个大包描述符, 上限为HMAC_LARGE_DSCR_TH_LIMIT_HIGH */
        ul_base_th = oal_min(g_ul_large_start_th + pst_dscr_th_opt->ul_rx_large_amsdu_pps / 150,
                             hmac_get_large_dscr_limit(pst_dscr_th_opt));
    } else {
        ul_base_th = g_ul_large_start_th;
    }

    /* 与当前基准值相差3以上才更新, 避免更新过于频繁 */
    if (oal_absolute_sub(pst_dscr_th_opt->ul_base_th, ul_base_th) >= 3) {
        pst_dscr_th_opt->ul_base_th = ul_base_th;
        en_need_update = OAL_TRUE;
    }

    if (en_need_update) {
        for (ul_index = 0; ul_index < HMAC_LARGE_DSCR_TH_LEVEL; ul_index++) {
            g_aul_theshold_large_dscr[ul_index] = ul_base_th + g_ul_large_interval * ul_index;
        }

        oam_warning_log2(0, OAM_SF_RX, "{hmac_rx_dscr_base_th_update::large dscr th[0] = %d, flow type[%d](0:rx/1:tx)}",
                         g_aul_theshold_large_dscr[0], pst_dscr_th_opt->en_flow_type_tx);
    }
}


OAL_STATIC oal_bool_enum_uint8 hmac_flow_type_need_update(hmac_dscr_th_opt_stru *pst_dscr_th_opt)
{
    oal_uint32 ul_threshold_large_dscr;
    oal_uint32 ul_threshold_small_dscr;
    oal_int32  l_delta_trx_large_pps = (oal_int32)(pst_dscr_th_opt->ul_rx_large_pps - pst_dscr_th_opt->ul_tx_large_pps);
    oal_int32  l_delta_trx_small_pps = (oal_int32)(pst_dscr_th_opt->ul_rx_small_pps - pst_dscr_th_opt->ul_tx_small_pps);

    hmac_rx_dscr_base_th_update(pst_dscr_th_opt);

    hmac_calculate_dscr_th(l_delta_trx_large_pps, l_delta_trx_small_pps, &ul_threshold_large_dscr,
                           &ul_threshold_small_dscr);

    if ((ul_threshold_large_dscr == pst_dscr_th_opt->ul_large_queue_th) &&
        (ul_threshold_small_dscr == pst_dscr_th_opt->ul_small_queue_th)) {
        return OAL_FALSE;
    }

    pst_dscr_th_opt->ul_large_queue_th = ul_threshold_large_dscr;
    pst_dscr_th_opt->ul_small_queue_th = ul_threshold_small_dscr;

    return OAL_TRUE;
}


OAL_STATIC oal_uint32 hmac_send_dscr_th_update_event(hmac_device_stru *pst_hmac_device)
{
    mac_h2d_dscr_th_stru st_dscr_th;
    oal_uint32           ul_ret;
    mac_vap_stru        *pst_mac_vap = mac_res_get_mac_vap(0);

    if (pst_mac_vap == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_dscr_th.ul_large_queue_th = pst_hmac_device->st_dscr_th_opt.ul_large_queue_th;
    st_dscr_th.ul_small_queue_th = pst_hmac_device->st_dscr_th_opt.ul_small_queue_th;

    /***************************************************************************
        抛事件到DMAC层, 同步RX描述符门限到device侧
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_DSCR_TH, OAL_SIZEOF(mac_h2d_dscr_th_stru),
                                    (oal_uint8 *)&st_dscr_th);
    if (oal_unlikely(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_send_dscr_th_update_event::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


OAL_STATIC oal_void hmac_rx_dscr_th_update(hmac_device_stru *pst_hmac_device)
{
    if (hmac_flow_type_need_update(&pst_hmac_device->st_dscr_th_opt)) {
        hmac_send_dscr_th_update_event(pst_hmac_device);
    }
}


OAL_STATIC oal_void hmac_flow_statistics_update(hmac_device_stru *pst_hmac_device, oal_uint32 ul_tx_large_pps,
                                                oal_uint32 ul_rx_large_pps,
                                                oal_uint32 ul_tx_small_pps, oal_uint32 ul_rx_small_pps,
                                                oal_uint32 ul_rx_large_amsdu_pps)
{
    pst_hmac_device->st_dscr_th_opt.ul_tx_large_pps = ul_tx_large_pps;
    pst_hmac_device->st_dscr_th_opt.ul_rx_large_pps = ul_rx_large_pps;
    pst_hmac_device->st_dscr_th_opt.ul_tx_small_pps = ul_tx_small_pps;
    pst_hmac_device->st_dscr_th_opt.ul_rx_small_pps = ul_rx_small_pps;
    pst_hmac_device->st_dscr_th_opt.ul_rx_large_amsdu_pps = ul_rx_large_amsdu_pps;
}


oal_void hmac_rx_dscr_th_opt(oal_uint32 ul_tx_large_pps, oal_uint32 ul_rx_large_pps, oal_uint32 ul_tx_small_pps,
                             oal_uint32 ul_rx_small_pps, oal_uint32 ul_rx_large_amsdu_pps)
{
    hmac_device_stru *pst_hmac_device = (hmac_device_stru *)hmac_res_get_mac_dev(0);

    if (oal_unlikely(pst_hmac_device == OAL_PTR_NULL)) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_rx_dscr_th_opt::pst_hmac_device is null}");
        return;
    }

    /* 将当前流量统计值更新至hmac device中的描述符门限优化结构体 */
    hmac_flow_statistics_update(pst_hmac_device, ul_tx_large_pps, ul_rx_large_pps,
                                ul_tx_small_pps, ul_rx_small_pps, ul_rx_large_amsdu_pps);

    /* 若需要下发门限, 则抛事件至device */
    hmac_rx_dscr_th_update(pst_hmac_device);
}


oal_void hmac_rx_dscr_th_init(hmac_device_stru *pst_hmac_device)
{
    oal_uint32 ul_index;

    for (ul_index = 0; ul_index < HMAC_LARGE_DSCR_TH_LEVEL; ul_index++) {
        g_aul_theshold_large_dscr[ul_index] = g_ul_large_start_th + g_ul_large_interval * ul_index;
    }

    for (ul_index = 0; ul_index < HMAC_SMALL_DSCR_TH_LEVEL; ul_index++) {
        g_aul_theshold_small_dscr[ul_index] = g_ul_small_start_th + g_ul_small_interval * ul_index;
    }

    for (ul_index = 0; ul_index < HMAC_LARGE_DSCR_PPS_LEVEL; ul_index++) {
        g_al_delta_large_pps[ul_index] = g_l_large_pps_start_level + (oal_int32)(g_ul_large_pps_interval * ul_index);
    }

    for (ul_index = 0; ul_index < HMAC_SMALL_DSCR_PPS_LEVEL; ul_index++) {
        g_al_delta_small_pps[ul_index] = g_l_small_pps_start_level + (oal_int32)(g_ul_small_pps_interval * ul_index);
    }

    oam_warning_log4(0, 0, "{hmac_rx_dscr_th_init::large start[%d], interval[%d], small start[%d], interval[%d]}",
        g_aul_theshold_large_dscr[0], g_ul_large_interval, g_aul_theshold_small_dscr[0], g_ul_small_interval);
}


oal_void hmac_rx_dscr_th_deinit(hmac_device_stru *pst_hmac_device)
{
}



