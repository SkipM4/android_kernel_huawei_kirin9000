
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "mac_vap.h"
#include "mac_ie.h"
#include "mac_frame.h"
#include "hmac_mgmt_bss_comm.h"
#include "mac_resource.h"
#include "hmac_device.h"
#include "hmac_resource.h"
#include "hmac_fsm.h"
#include "hmac_encap_frame.h"
#include "hmac_tx_amsdu.h"
#include "hmac_mgmt_ap.h"
#include "hmac_mgmt_sta.h"
#include "hmac_mgmt_join.h"
#include "hmac_blockack.h"
#include "hmac_p2p.h"
#include "securec.h"
#include "securectype.h"
#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "hmac_btcoex.h"
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_MGMT_BSS_COMM_C

#ifdef _PRE_WLAN_FEATURE_SNIFFER
#include <hwnet/ipv4/sysctl_sniffer.h>
#endif
/* 2 全局变量定义 */
oal_uint8 g_auc_avail_protocol_mode[WLAN_PROTOCOL_BUTT][WLAN_PROTOCOL_BUTT] = {
    {   WLAN_LEGACY_11A_MODE, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT,
        WLAN_LEGACY_11A_MODE, WLAN_LEGACY_11A_MODE, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT
    },
    {   WLAN_PROTOCOL_BUTT, WLAN_LEGACY_11B_MODE, WLAN_LEGACY_11B_MODE, WLAN_LEGACY_11B_MODE, WLAN_LEGACY_11B_MODE,
        WLAN_LEGACY_11B_MODE, WLAN_LEGACY_11B_MODE, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT
    },
    {   WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_LEGACY_11G_MODE, WLAN_LEGACY_11G_MODE, WLAN_LEGACY_11G_MODE,
        WLAN_LEGACY_11G_MODE, WLAN_LEGACY_11G_MODE, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_LEGACY_11G_MODE
    },
    {   WLAN_PROTOCOL_BUTT, WLAN_LEGACY_11B_MODE, WLAN_LEGACY_11G_MODE, WLAN_MIXED_ONE_11G_MODE,
        WLAN_MIXED_ONE_11G_MODE, WLAN_MIXED_ONE_11G_MODE, WLAN_MIXED_ONE_11G_MODE, WLAN_PROTOCOL_BUTT,
        WLAN_PROTOCOL_BUTT, WLAN_LEGACY_11G_MODE
    },
    {   WLAN_PROTOCOL_BUTT, WLAN_LEGACY_11B_MODE, WLAN_LEGACY_11G_MODE, WLAN_MIXED_ONE_11G_MODE,
        WLAN_MIXED_TWO_11G_MODE, WLAN_MIXED_ONE_11G_MODE, WLAN_MIXED_ONE_11G_MODE, WLAN_PROTOCOL_BUTT,
        WLAN_PROTOCOL_BUTT, WLAN_LEGACY_11G_MODE
    },
    {   WLAN_LEGACY_11A_MODE, WLAN_LEGACY_11B_MODE, WLAN_LEGACY_11G_MODE, WLAN_MIXED_ONE_11G_MODE,
        WLAN_MIXED_ONE_11G_MODE, WLAN_HT_MODE, WLAN_HT_MODE, WLAN_HT_ONLY_MODE,
        WLAN_PROTOCOL_BUTT, WLAN_HT_11G_MODE
    },
    {   WLAN_LEGACY_11A_MODE, WLAN_LEGACY_11B_MODE, WLAN_LEGACY_11G_MODE, WLAN_MIXED_ONE_11G_MODE,
        WLAN_MIXED_ONE_11G_MODE, WLAN_HT_MODE, WLAN_VHT_MODE, WLAN_HT_ONLY_MODE,
        WLAN_VHT_ONLY_MODE, WLAN_PROTOCOL_BUTT
    },
    {   WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT,
        WLAN_PROTOCOL_BUTT, WLAN_HT_ONLY_MODE, WLAN_HT_ONLY_MODE, WLAN_HT_ONLY_MODE,
        WLAN_HT_ONLY_MODE, WLAN_HT_ONLY_MODE
    },
    {   WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT,
        WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_VHT_ONLY_MODE,
        WLAN_PROTOCOL_BUTT, WLAN_VHT_ONLY_MODE, WLAN_PROTOCOL_BUTT
    },
    {   WLAN_PROTOCOL_BUTT, WLAN_PROTOCOL_BUTT, WLAN_LEGACY_11G_MODE, WLAN_LEGACY_11G_MODE,
        WLAN_LEGACY_11G_MODE, WLAN_HT_11G_MODE, WLAN_PROTOCOL_BUTT,
        WLAN_HT_ONLY_MODE, WLAN_PROTOCOL_BUTT, WLAN_HT_11G_MODE
    },
};

oal_uint32 hmac_mgmt_tx_addba_timeout(oal_void *p_arg);

#define MAC_RX_HAL_VAP_ID 14
#define MAX_MPDU_LEN_NO_VHT_CAP 3895
#define MAX_MPDU_LEN_LOW_VHT_CAP 7991
#define MAX_MPDU_LEN_HIGH_VHT_CAP 11454

/* 3 函数实现 */
#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP

oal_void hmac_rx_ba_session_decr(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_tidno)
{
    if (pst_hmac_vap->uc_rx_ba_session_num == 0) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                         "{hmac_rx_ba_session_decr::tid[%d] rx_session already zero.}", uc_tidno);
        return;
    }

    pst_hmac_vap->uc_rx_ba_session_num--;

    oam_warning_log2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                     "{hmac_rx_ba_session_decr::tid[%d] rx_session[%d] remove.}",
                     uc_tidno, pst_hmac_vap->uc_rx_ba_session_num);
}


oal_void hmac_tx_ba_session_decr(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_tidno)
{
    if (pst_hmac_vap->uc_tx_ba_session_num == 0) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                         "{hmac_tx_ba_session_decr::tid[%d] tx_session already zero.}", uc_tidno);
        return;
    }

    pst_hmac_vap->uc_tx_ba_session_num--;

    oam_warning_log2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                     "{hmac_tx_ba_session_decr::tid[%d] tx_session[%d] remove.}",
                     uc_tidno, pst_hmac_vap->uc_tx_ba_session_num);
}


OAL_STATIC OAL_INLINE oal_void hmac_tx_ba_session_incr(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_tidno)
{
    pst_hmac_vap->uc_tx_ba_session_num++;

    oam_warning_log2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                     "{hmac_tx_ba_session_incr::tid[%d] tx_session[%d] setup.}",
                     uc_tidno, pst_hmac_vap->uc_tx_ba_session_num);
}
#else

oal_void hmac_rx_ba_session_decr(mac_device_stru *pst_mac_device, oal_uint8 uc_tidno)
{
    if (pst_mac_device->uc_rx_ba_session_num == 0) {
        OAM_WARNING_LOG1(0, OAM_SF_BA,
                         "{hmac_rx_ba_session_decr::tid[%d] rx_session already zero.}", uc_tidno);
        return;
    }

    pst_mac_device->uc_rx_ba_session_num--;

    oam_warning_log2(0, OAM_SF_BA,
                     "{hmac_rx_ba_session_decr::tid[%d] rx_session[%d] remove.}",
                     uc_tidno, pst_mac_device->uc_rx_ba_session_num);
}


oal_void hmac_tx_ba_session_decr(mac_device_stru *pst_mac_device, oal_uint8 uc_tidno)
{
    if (pst_mac_device->uc_tx_ba_session_num == 0) {
        OAM_WARNING_LOG1(0, OAM_SF_BA,
                         "{hmac_tx_ba_session_decr::tid[%d] tx_session already zero.}", uc_tidno);
        return;
    }

    pst_mac_device->uc_tx_ba_session_num--;

    oam_warning_log2(0, OAM_SF_BA,
                     "{hmac_tx_ba_session_decr::tid[%d] tx_session[%d] remove.}",
                     uc_tidno, pst_mac_device->uc_tx_ba_session_num);
}


OAL_STATIC OAL_INLINE oal_void hmac_tx_ba_session_incr(mac_device_stru *pst_mac_device, oal_uint8 uc_tidno)
{
    pst_mac_device->uc_tx_ba_session_num++;

    oam_warning_log2(0, OAM_SF_BA,
                     "{hmac_tx_ba_session_incr::tid[%d] tx_session[%d] setup.}",
                     uc_tidno, pst_mac_device->uc_tx_ba_session_num);
}
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX

oal_uint16 hmac_btcoex_get_ba_size(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user)
{
    hmac_user_btcoex_stru *pst_hmac_user_btcoex;
    oal_uint16 us_ba_size = 0;

    pst_hmac_user_btcoex = &(pst_hmac_user->st_hmac_user_btcoex);
    /* 小米R1D路由器5G80M电话场景下聚合个数设为2 */
    if (MAC_IS_XIAOMI_R1D(pst_hmac_user->st_user_base_info.auc_user_mac_addr) ||
        MAC_IS_XIAOMI_R2D(pst_hmac_user->st_user_base_info.auc_user_mac_addr)) {
        if ((pst_hmac_vap->st_vap_base_info.st_channel.en_band == WLAN_BAND_5G) &&
            (pst_hmac_vap->st_vap_base_info.st_channel.en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) &&
            (pst_hmac_vap->st_vap_base_info.st_channel.en_bandwidth <= WLAN_BAND_WIDTH_80MINUSMINUS)) {
            /* 如果非电话结束场景 */
            if (pst_hmac_user_btcoex->uc_ba_size != HMAC_BA_SIZE_64) {
                us_ba_size = HMAC_BA_SIZE_2;
            }
        }
    }
    if (pst_hmac_user->st_hmac_user_btcoex.en_ba_type == BTCOEX_BA_TYPE_SIZE_1) {
        us_ba_size = HMAC_BA_SIZE_1;
    }
    return us_ba_size;
}


oal_uint32 hmac_btcoex_set_ba_size(hmac_vap_stru *pst_hmac_vap,
                                   hmac_user_stru *pst_hmac_user,
                                   hmac_ba_rx_stru *pst_addba_rsp,
                                   oal_uint16 *pus_ba_param)
{
    hmac_user_btcoex_stru *pst_hmac_user_btcoex;
    oal_uint16 us_ba_size = 0;

    pst_hmac_user_btcoex = &(pst_hmac_user->st_hmac_user_btcoex);
    if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        /* 1.黑名单用户 */
        if (pst_hmac_user_btcoex->st_hmac_btcoex_addba_req.en_ba_handle_allow == OAL_FALSE) {
            if (hmac_btcoex_get_blacklist_type(pst_hmac_user) == BTCOEX_BLACKLIST_TPYE_FIX_BASIZE) {
                *pus_ba_param |= (oal_uint16)(0x0008 << 6); /* 0x0008右左移6位为了保留pus_ba_param的第9bit位 */
            } else {
                /* 黑名单时，btcoex聚合业务处于结束状态，按照默认聚合个数恢复wifi性能 */
                *pus_ba_param |= (oal_uint16)(pst_addba_rsp->us_baw_size << 6); /* BIT6 */
            }
        } else if ((pst_hmac_user_btcoex->en_delba_btcoex_trigger == OAL_TRUE) &&
                   (pst_hmac_user_btcoex->uc_ba_size != 0)) {
            us_ba_size = hmac_btcoex_get_ba_size(pst_hmac_vap, pst_hmac_user);
            if (us_ba_size == 0) {
                us_ba_size = (oal_uint16)pst_hmac_user_btcoex->uc_ba_size;
            } else {
                us_ba_size =
                (us_ba_size > pst_hmac_user_btcoex->uc_ba_size ? pst_hmac_user_btcoex->uc_ba_size : us_ba_size);
            }
            *pus_ba_param |= (oal_uint16)(us_ba_size << 6); /* BIT6 */
        } else {
            *pus_ba_param |= (oal_uint16)(pst_addba_rsp->us_baw_size << 6); /* BIT6 */
        }
    } else {
        *pus_ba_param |= (oal_uint16)(pst_addba_rsp->us_baw_size << 6); /* BIT6 */
    }
    return OAL_SUCC;
}

oal_void hmac_btcoex_process_addba_rsp_param(
    hmac_vap_stru *pst_hmac_vap, hmac_ba_rx_stru *pst_addba_rsp, oal_uint16 *pus_param)
{
    hmac_user_stru *pst_hmac_user;
    hmac_user_btcoex_stru *pst_hmac_user_btcoex = OAL_PTR_NULL;
    hmac_device_stru *pst_hmac_device;
    hmac_device_btcoex_stru *pst_btcoex_device = OAL_PTR_NULL;

    pst_hmac_device = hmac_res_get_mac_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);;
    pst_hmac_user = mac_vap_get_hmac_user_by_addr(&(pst_hmac_vap->st_vap_base_info),
        pst_addba_rsp->puc_transmit_addr, WLAN_MAC_ADDR_LEN);
    if ((pst_hmac_user != OAL_PTR_NULL) && (pst_hmac_device != OAL_PTR_NULL)) {
        pst_hmac_user_btcoex = &(pst_hmac_user->st_hmac_user_btcoex);
        pst_btcoex_device = &pst_hmac_device->st_hmac_device_btcoex;
        if (((pst_hmac_user_btcoex->en_delba_btcoex_trigger == OAL_TRUE) &&
            (pst_hmac_user_btcoex->uc_ba_size != WLAN_AMPDU_RX_BUFFER_SIZE)) ||
            (pst_btcoex_device->st_btble_status.un_bt_status.st_bt_status.bit_bt_a2dp == OAL_TRUE)) {
            *pus_param &= (~BIT0);
        }
        hmac_btcoex_set_ba_size(pst_hmac_vap, pst_hmac_user, pst_addba_rsp, pus_param);
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{hmac_mgmt_encap_addba_rsp::user ptr null.}");
        *pus_param |= (oal_uint16)(pst_addba_rsp->us_baw_size << 6); /* BIT6 */
    }
}
#endif


oal_uint16 hmac_mgmt_encap_addba_req(hmac_vap_stru *pst_vap,
                                     oal_uint8 *puc_data,
                                     dmac_ba_tx_stru *pst_tx_ba,
                                     oal_uint8 uc_tid)
{
    oal_uint16 us_index;
    oal_uint16 us_ba_param;
    if ((pst_vap == OAL_PTR_NULL) || (puc_data == OAL_PTR_NULL) || (pst_tx_ba == OAL_PTR_NULL)) {
        oam_error_log3(0, OAM_SF_BA, "{hmac_mgmt_encap_addba_req::null param.vap:%x data:%x ba:%x}", (uintptr_t)pst_vap,
                       (uintptr_t)puc_data, (uintptr_t)pst_tx_ba);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*************************************************************************/
    /* Management Frame Format */
    /* -------------------------------------------------------------------- */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS| */
    /* -------------------------------------------------------------------- */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  | */
    /* -------------------------------------------------------------------- */
    /*************************************************************************/
    /*************************************************************************/
    /* Set the fields in the frame header */
    /*************************************************************************/
    /* Frame Control Field 中只需要设置Type/Subtype值，其他设置为0 */
    mac_hdr_set_frame_control(puc_data, WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* DA is address of STA requesting association（从第4byte开始是DA地址） */
    oal_set_mac_addr(puc_data + 4, pst_tx_ba->puc_dst_addr);

    /* SA的值为dot11MACAddress的值（从第10byte开始是SA地址） */
    oal_set_mac_addr(puc_data + 10, pst_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    oal_set_mac_addr(puc_data + 16, pst_vap->st_vap_base_info.auc_bssid); /* 从第16byte开始是BSSID地址 */

    /*************************************************************************/
    /* Set the contents of the frame body */
    /*************************************************************************/
    /* 将索引指向frame body起始位置 */
    us_index = MAC_80211_FRAME_LEN;

    /* 设置Category */
    puc_data[us_index++] = MAC_ACTION_CATEGORY_BA;

    /* 设置Action */
    puc_data[us_index++] = MAC_BA_ACTION_ADDBA_REQ;

    /* 设置Dialog Token */
    puc_data[us_index++] = pst_tx_ba->uc_dialog_token;

    /*
     * 设置Block Ack Parameter set field
     * bit0 - AMSDU Allowed
     * bit1 - Immediate or Delayed block ack
     * bit2-bit5 - TID
     * bit6-bit15 -  Buffer size
 */
    us_ba_param = pst_tx_ba->en_amsdu_supp;        /* bit0 */
    us_ba_param |= (pst_tx_ba->uc_ba_policy << 1); /* bit1 */
    us_ba_param |= (uc_tid << 2);                  /* bit2 */

    us_ba_param |= (oal_uint16)(pst_tx_ba->us_baw_size << 6); /* bit6 */

    puc_data[us_index++] = (oal_uint8)(us_ba_param & 0xFF);
    puc_data[us_index++] = (oal_uint8)((us_ba_param >> 8) & 0xFF); /* 获取us_ba_param的高8位（B8-B15） */

    /* 设置BlockAck timeout */
    puc_data[us_index++] = (oal_uint8)(pst_tx_ba->us_ba_timeout & 0xFF);
    puc_data[us_index++] = (oal_uint8)((pst_tx_ba->us_ba_timeout >> 8) & 0xFF); /* 获取us_ba_timeout的高8位（B8-B15） */

    /*
     * Block ack starting sequence number字段由硬件设置
     * bit0-bit3 fragmentnumber
     * bit4-bit15: sequence number
     */
    /* us_buf_seq此处暂不填写，在dmac侧会补充填写 */
    *(oal_uint16 *)&puc_data[us_index++] = 0;
    us_index++;

    /* 返回的帧长度中不包括FCS */
    return us_index;
}


oal_uint16 hmac_mgmt_encap_addba_rsp(hmac_vap_stru *pst_vap,
                                     oal_uint8 *puc_data,
                                     hmac_ba_rx_stru *pst_addba_rsp,
                                     oal_uint8 uc_tid,
                                     oal_uint8 uc_status)
{
    oal_uint16 us_index;
    oal_uint16 us_ba_param;

    if ((pst_vap == OAL_PTR_NULL) || (puc_data == OAL_PTR_NULL) || (pst_addba_rsp == OAL_PTR_NULL)) {
        oam_error_log3(0, OAM_SF_BA, "{hmac_mgmt_encap_addba_req::null prm.vap:%x data:%x rsp:%x}", (uintptr_t)pst_vap,
                       (uintptr_t)puc_data, (uintptr_t)pst_addba_rsp);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*************************************************************************/
    /* Management Frame Format */
    /* -------------------------------------------------------------------- */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS| */
    /* -------------------------------------------------------------------- */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  | */
    /* -------------------------------------------------------------------- */
    /*************************************************************************/
    /*************************************************************************/
    /* Set the fields in the frame header */
    /*************************************************************************/
    /* All the fields of the Frame Control Field are set to zero. Only the */
    /* Type/Subtype field is set. */
    mac_hdr_set_frame_control(puc_data, WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* DA is address of STA requesting association（从第4byte开始是DA地址） */
    oal_set_mac_addr(puc_data + 4, pst_addba_rsp->puc_transmit_addr);

    /* SA is the dot11MACAddress（从第10byte开始是SA地址） */
    oal_set_mac_addr(puc_data + 10, pst_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    oal_set_mac_addr(puc_data + 16, pst_vap->st_vap_base_info.auc_bssid); /* 从第16byte开始是BSSID地址 */

    /*************************************************************************/
    /* Set the contents of the frame body */
    /*************************************************************************/
    /*************************************************************************/
    /* ADDBA Response Frame - Frame Body */
    /* --------------------------------------------------------------- */
    /* | Category | Action | Dialog | Status  | Parameters | Timeout | */
    /* --------------------------------------------------------------- */
    /* | 1        | 1      | 1      | 2       | 2          | 2       | */
    /* --------------------------------------------------------------- */
    /*************************************************************************/
    /* Initialize index and the frame data pointer */
    us_index = MAC_80211_FRAME_LEN;

    /* Action Category设置 */
    puc_data[us_index++] = MAC_ACTION_CATEGORY_BA;

    /* 特定Action种类下的action的帧类型 */
    puc_data[us_index++] = MAC_BA_ACTION_ADDBA_RSP;

    /* Dialog Token域设置，需要从req中copy过来 */
    puc_data[us_index++] = pst_addba_rsp->uc_dialog_token;

    /* 状态域设置 */
    puc_data[us_index++] = uc_status;
    puc_data[us_index++] = 0;

    /* Block Ack Parameter设置 */
    /* B0 - AMSDU Support, B1- Immediate or Delayed block ack */
    /* B2-B5 : TID, B6-B15: Buffer size */
    us_ba_param = pst_addba_rsp->en_amsdu_supp;        /* BIT0 */
    us_ba_param |= (pst_addba_rsp->uc_ba_policy << 1); /* BIT1 */
    us_ba_param |= (uc_tid << 2);                      /* BIT2 */
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    hmac_btcoex_process_addba_rsp_param(pst_vap, pst_addba_rsp, &us_ba_param);
#else
    us_ba_param |= (oal_uint16)(pst_addba_rsp->us_baw_size << 6); /* BIT6 */
#endif
    oam_warning_log2(0, OAM_SF_BA, "hmac_mgmt_encap_addba_rsp:tid[%d],ba_size[%d]", uc_tid,
                     ((us_ba_param & 0xffc0) >> 6)); /* 打印ba_size值（us_ba_param B6-B15） */
    puc_data[us_index++] = (oal_uint8)(us_ba_param & 0xFF);
    puc_data[us_index++] = (oal_uint8)((us_ba_param >> 8) & 0xFF); /* 获取us_ba_param的高8位（B8-B15） */

    puc_data[us_index++] = 0x00;
    puc_data[us_index++] = 0x00;

    /* 返回的帧长度中不包括FCS */
    return us_index;
}


oal_uint16 hmac_mgmt_encap_delba(hmac_vap_stru *pst_vap,
                                 oal_uint8 *puc_data,
                                 oal_uint8 *puc_addr,
                                 oal_uint8 uc_tid,
                                 mac_delba_initiator_enum_uint8 en_initiator,
                                 oal_uint8 reason)
{
    oal_uint16 us_index;

    if ((pst_vap == OAL_PTR_NULL) || (puc_data == OAL_PTR_NULL) || (puc_addr == OAL_PTR_NULL)) {
        oam_error_log3(0, OAM_SF_BA, "{hmac_mgmt_encap_delba::null param, %x %x %x.}", (uintptr_t)pst_vap,
                       (uintptr_t)puc_data, (uintptr_t)puc_addr);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*************************************************************************/
    /* Management Frame Format */
    /* -------------------------------------------------------------------- */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS| */
    /* -------------------------------------------------------------------- */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  | */
    /* -------------------------------------------------------------------- */
    /*************************************************************************/
    /*************************************************************************/
    /* Set the fields in the frame header */
    /*************************************************************************/
    /* All the fields of the Frame Control Field are set to zero. Only the */
    /* Type/Subtype field is set. */
    mac_hdr_set_frame_control(puc_data, WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* duration */
    puc_data[2] = 0; /* puc_data 2、3byte(duration)置零 */
    puc_data[3] = 0;

    /* DA is address of STA requesting association（从第4byte开始是DA地址） */
    oal_set_mac_addr(puc_data + 4, puc_addr);

    /* SA is the dot11MACAddress（从第10byte开始是SA地址） */
    oal_set_mac_addr(puc_data + 10, pst_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    oal_set_mac_addr(puc_data + 16, pst_vap->st_vap_base_info.auc_bssid); /* 从第16byte开始是BSSID地址 */

    /* seq control */
    puc_data[22] = 0; /* puc_data 22、23byte(seq control)置零 */
    puc_data[23] = 0;

    /*************************************************************************/
    /* Set the contents of the frame body */
    /*************************************************************************/
    /*************************************************************************/
    /* DELBA Response Frame - Frame Body */
    /* ------------------------------------------------- */
    /* | Category | Action |  Parameters | Reason code | */
    /* ------------------------------------------------- */
    /* | 1        | 1      |       2     | 2           | */
    /* ------------------------------------------------- */
    /* Parameters */
    /* ------------------------------- */
    /* | Reserved | Initiator |  TID  | */
    /* ------------------------------- */
    /* bit  |    11    |    1      |  4    | */
    /* -------------------------------- */
    /*************************************************************************/
    /* Initialize index and the frame data pointer */
    us_index = MAC_80211_FRAME_LEN;

    /* Category */
    puc_data[us_index++] = MAC_ACTION_CATEGORY_BA;

    /* Action */
    puc_data[us_index++] = MAC_BA_ACTION_DELBA;

    /* DELBA parameter set */
    /* B0 - B10 -reserved */
    /* B11 - initiator */
    /* B12-B15 - TID */
    puc_data[us_index++] = 0;
    puc_data[us_index] = (oal_uint8)(en_initiator << 3); /* initiator左移3位 */
    puc_data[us_index++] |= (oal_uint8)(uc_tid << 4); /* tid左移4位 */

    /* Reason field */
    /* Reason can be either of END_BA, QSTA_LEAVING, UNKNOWN_BA */
    puc_data[us_index++] = reason;
    puc_data[us_index++] = 0;

    /* 返回的帧长度中不包括FCS */
    return us_index;
}
oal_void hmac_fill_ba_tx_info(oal_uint8 uc_tidno, hmac_vap_stru *pst_hmac_vap,
    hmac_user_stru *pst_hmac_user, dmac_ba_tx_stru *pst_tx_ba, mac_action_mgmt_args_stru *pst_action_args)
{
    pst_hmac_vap->uc_ba_dialog_token++;
    pst_tx_ba->uc_dialog_token = pst_hmac_vap->uc_ba_dialog_token; /* 保证ba会话创建能够区分 */
    pst_tx_ba->us_baw_size = (oal_uint8)(pst_action_args->ul_arg2);
    pst_tx_ba->uc_ba_policy = (oal_uint8)(pst_action_args->ul_arg3);
    pst_tx_ba->us_ba_timeout = (oal_uint16)(pst_action_args->ul_arg4);
    pst_tx_ba->puc_dst_addr = pst_hmac_user->st_user_base_info.auc_user_mac_addr;

    /* 发端对AMPDU+AMSDU的支持 */
    pst_tx_ba->en_amsdu_supp = (oal_bool_enum_uint8)pst_hmac_vap->en_amsdu_ampdu_active;

    /*lint -e502*/
    if (pst_tx_ba->en_amsdu_supp == OAL_FALSE) {
        hmac_user_set_amsdu_not_support(pst_hmac_user, uc_tidno);
    } else {
        hmac_user_set_amsdu_support(pst_hmac_user, uc_tidno);
    }
    /*lint +e502*/
}

oal_void hmac_fill_wlan_ctx_action(dmac_ctx_action_event_stru *pst_wlan_ctx_action,
    hmac_user_stru *pst_hmac_user, dmac_ba_tx_stru *pst_tx_ba, oal_uint8 uc_tidno, oal_uint16 us_frame_len)
{
    /* 赋值要传入Dmac的信息 */
    pst_wlan_ctx_action->us_frame_len = us_frame_len;
    pst_wlan_ctx_action->uc_hdr_len = MAC_80211_FRAME_LEN;
    pst_wlan_ctx_action->en_action_category = MAC_ACTION_CATEGORY_BA;
    pst_wlan_ctx_action->uc_action = MAC_BA_ACTION_ADDBA_REQ;
    pst_wlan_ctx_action->us_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id;
    pst_wlan_ctx_action->uc_tidno = uc_tidno;
    pst_wlan_ctx_action->uc_dialog_token = pst_tx_ba->uc_dialog_token;
    pst_wlan_ctx_action->uc_ba_policy = pst_tx_ba->uc_ba_policy;
    pst_wlan_ctx_action->us_baw_size = pst_tx_ba->us_baw_size;
    pst_wlan_ctx_action->us_ba_timeout = pst_tx_ba->us_ba_timeout;
}

oal_void hmac_update_tid_info(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
    mac_device_stru *pst_device, dmac_ba_tx_stru *pst_tx_ba, oal_uint8 uc_tidno)
{
    /* 更新对应的TID信息 */
    pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status = DMAC_BA_INPROGRESS;
    pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.uc_dialog_token = pst_tx_ba->uc_dialog_token;
    pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.uc_ba_policy = pst_tx_ba->uc_ba_policy;
#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
    hmac_tx_ba_session_incr(pst_hmac_vap, uc_tidno);
#else
    hmac_tx_ba_session_incr(pst_device, uc_tidno);
#endif
}

oal_uint32 hmac_mgmt_tx_addba_req(hmac_vap_stru *pst_hmac_vap,
                                  hmac_user_stru *pst_hmac_user,
                                  mac_action_mgmt_args_stru *pst_action_args)
{
    mac_device_stru *pst_device = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL; /* 申请事件返回的内存指针 */
    oal_netbuf_stru *pst_addba_req = OAL_PTR_NULL;
    dmac_ba_tx_stru st_tx_ba;
    oal_uint8 uc_tidno;
    oal_uint16 us_frame_len;
    frw_event_stru *pst_hmac_to_dmac_ctx_event = OAL_PTR_NULL;
    dmac_tx_event_stru *pst_tx_event = OAL_PTR_NULL;
    dmac_ctx_action_event_stru st_wlan_ctx_action;
    oal_uint32 ul_ret;
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL;

    if ((pst_hmac_vap == OAL_PTR_NULL) || (pst_hmac_user == OAL_PTR_NULL) || (pst_action_args == OAL_PTR_NULL)) {
        oam_error_log3(0, OAM_SF_BA, "{hmac_mgmt_tx_addba_req::null param, %x %x %x.}", (uintptr_t)pst_hmac_vap,
                       (uintptr_t)pst_hmac_user, (uintptr_t)pst_action_args);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_BUTT) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA,
                         "{hmac_mgmt_tx_addba_req:: vap has been down/del, vap_state[%d].}", pst_mac_vap->en_vap_state);
        return OAL_FAIL;
    }

    /* 获取device结构 */
    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_tx_addba_req::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请ADDBA_REQ管理帧内存 */
    pst_addba_req = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (pst_addba_req == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_tx_addba_req::pst_addba_req null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_mem_netbuf_trace(pst_addba_req, OAL_TRUE);

    memset_s(oal_netbuf_cb(pst_addba_req), oal_netbuf_cb_size(), 0, oal_netbuf_cb_size());
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_addba_req);

    oal_netbuf_prev(pst_addba_req) = OAL_PTR_NULL;
    oal_netbuf_next(pst_addba_req) = OAL_PTR_NULL;

    uc_tidno = (oal_uint8)(pst_action_args->ul_arg1);

    /* 对tid对应的txBA会话状态加锁 */
    oal_spin_lock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
    hmac_fill_ba_tx_info(uc_tidno, pst_hmac_vap, pst_hmac_user, &st_tx_ba, pst_action_args);

    /* 调用封装管理帧接口 */
    us_frame_len = hmac_mgmt_encap_addba_req(pst_hmac_vap, oal_netbuf_data(pst_addba_req), &st_tx_ba, uc_tidno);
    memset_s((oal_uint8 *)&st_wlan_ctx_action, OAL_SIZEOF(st_wlan_ctx_action), 0, OAL_SIZEOF(st_wlan_ctx_action));
    hmac_fill_wlan_ctx_action(&st_wlan_ctx_action, pst_hmac_user, &st_tx_ba, uc_tidno, us_frame_len);

    if (memcpy_s((oal_uint8 *)(oal_netbuf_data(pst_addba_req) + us_frame_len), (WLAN_MEM_NETBUF_SIZE2 - us_frame_len),
                 (oal_uint8 *)&st_wlan_ctx_action, OAL_SIZEOF(dmac_ctx_action_event_stru)) != EOK) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "hmac_mgmt_tx_addba_req::memcpy fail!");
        oal_netbuf_free(pst_addba_req);
        oal_spin_unlock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
        return OAL_FAIL;
    }

    oal_netbuf_put(pst_addba_req, (us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru)));

    pst_tx_ctl->us_mpdu_len = us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru);

    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(dmac_tx_event_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_tx_addba_req::pst_event_mem null.}");
        oal_netbuf_free(pst_addba_req);
        oal_spin_unlock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_to_dmac_ctx_event = (frw_event_stru *)pst_event_mem->puc_data;
    frw_event_hdr_init(&(pst_hmac_to_dmac_ctx_event->st_event_hdr), FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_ACTION, OAL_SIZEOF(dmac_tx_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id, pst_mac_vap->uc_device_id, pst_mac_vap->uc_vap_id);

    pst_tx_event = (dmac_tx_event_stru *)(pst_hmac_to_dmac_ctx_event->auc_event_data);
    pst_tx_event->pst_netbuf = pst_addba_req;
    pst_tx_event->us_frame_len = us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru);

    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_tx_addba_req::dispatch_event fail[%d].}", ul_ret);
        oal_netbuf_free(pst_addba_req);
        frw_event_free_m(pst_event_mem);
        oal_spin_unlock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
        return ul_ret;
    }

    frw_event_free_m(pst_event_mem);

    /* 更新对应的TID信息 */
    hmac_update_tid_info(pst_hmac_vap, pst_hmac_user, pst_device, &st_tx_ba, uc_tidno);

    /* 启动ADDBA超时计时器 */
    frw_create_timer(&pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_addba_timer,
                     hmac_mgmt_tx_addba_timeout, WLAN_ADDBA_TIMEOUT,
                     &pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_alarm_data,
                     OAL_FALSE, OAM_MODULE_ID_HMAC, pst_device->ul_core_id);

    oal_spin_unlock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
    return OAL_SUCC;
}


oal_uint32 hmac_mgmt_tx_addba_rsp(hmac_vap_stru *pst_hmac_vap,
                                  hmac_user_stru *pst_hmac_user,
                                  hmac_ba_rx_stru *pst_ba_rx_info,
                                  oal_uint8 uc_tid,
                                  oal_uint8 uc_status)
{
    mac_device_stru *pst_device = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL; /* 申请事件返回的内存指针 */
    frw_event_stru *pst_hmac_to_dmac_ctx_event = OAL_PTR_NULL;
    dmac_tx_event_stru *pst_tx_event = OAL_PTR_NULL;
    dmac_ctx_action_event_stru st_wlan_ctx_action;
    oal_netbuf_stru *pst_addba_rsp = OAL_PTR_NULL;
    oal_uint16 us_frame_len;
    oal_uint32 ul_ret;
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL;

    if ((pst_hmac_vap == OAL_PTR_NULL) || (pst_hmac_user == OAL_PTR_NULL) || (pst_ba_rx_info == OAL_PTR_NULL)) {
        oam_error_log3(0, OAM_SF_BA, "{hmac_mgmt_tx_addba_rsp::send addba rsp failed, null param, %x %x %x.}",
                       (uintptr_t)pst_hmac_vap, (uintptr_t)pst_hmac_user, (uintptr_t)pst_ba_rx_info);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_BUTT) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA,
                         "{hmac_mgmt_tx_addba_rsp:: vap has been down/del, vap_state[%d].}", pst_mac_vap->en_vap_state);
        return OAL_FAIL;
    }

    /* 获取device结构 */
    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_tx_addba_rsp::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请ADDBA_RSP管理帧内存 */
    pst_addba_rsp = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (pst_addba_rsp == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA,
                       "{hmac_mgmt_tx_addba_rsp::send addba rsp failed, pst_addba_rsp mem alloc failed.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_mem_netbuf_trace(pst_addba_rsp, OAL_TRUE);
    memset_s(oal_netbuf_cb(pst_addba_rsp), oal_netbuf_cb_size(), 0, oal_netbuf_cb_size());

    oal_netbuf_prev(pst_addba_rsp) = OAL_PTR_NULL;
    oal_netbuf_next(pst_addba_rsp) = OAL_PTR_NULL;

    /* 填写netbuf的cb字段，共发送管理帧和发送完成接口使用 */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_addba_rsp);
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = (oal_uint8)pst_hmac_user->st_user_base_info.us_assoc_id;
    mac_set_cb_tid(pst_tx_ctl, uc_tid);
    mac_set_cb_is_amsdu(pst_tx_ctl, OAL_FALSE);

    us_frame_len = hmac_mgmt_encap_addba_rsp(pst_hmac_vap, oal_netbuf_data(pst_addba_rsp), pst_ba_rx_info, uc_tid,
                                             uc_status);
    memset_s((oal_uint8 *)&st_wlan_ctx_action, OAL_SIZEOF(st_wlan_ctx_action), 0, OAL_SIZEOF(st_wlan_ctx_action));
    st_wlan_ctx_action.en_action_category = MAC_ACTION_CATEGORY_BA;
    st_wlan_ctx_action.uc_action = MAC_BA_ACTION_ADDBA_RSP;
    st_wlan_ctx_action.uc_hdr_len = MAC_80211_FRAME_LEN;
    st_wlan_ctx_action.us_baw_size = pst_ba_rx_info->us_baw_size;
    st_wlan_ctx_action.us_frame_len = us_frame_len;
    st_wlan_ctx_action.us_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id;
    st_wlan_ctx_action.uc_tidno = uc_tid;
    st_wlan_ctx_action.uc_status = uc_status;
    st_wlan_ctx_action.us_ba_timeout = pst_ba_rx_info->us_ba_timeout;
    st_wlan_ctx_action.en_back_var = pst_ba_rx_info->en_back_var;
    st_wlan_ctx_action.uc_lut_index = pst_ba_rx_info->uc_lut_index;
    st_wlan_ctx_action.us_baw_start = pst_ba_rx_info->us_baw_start;
    st_wlan_ctx_action.uc_ba_policy = pst_ba_rx_info->uc_ba_policy;
    st_wlan_ctx_action.en_amsdu_supp = pst_ba_rx_info->en_amsdu_supp;
    if (memcpy_s((oal_uint8 *)(oal_netbuf_data(pst_addba_rsp) + us_frame_len),
                 (WLAN_MEM_NETBUF_SIZE2 - us_frame_len), (oal_uint8 *)&st_wlan_ctx_action,
                 OAL_SIZEOF(dmac_ctx_action_event_stru)) != EOK) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "hmac_mgmt_tx_addba_rsp::memcpy fail!");
        oal_netbuf_free(pst_addba_rsp);
        return OAL_FAIL;
    }

    oal_netbuf_put(pst_addba_rsp, (us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru)));

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_tx_ctl->us_mpdu_len = us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru);
#endif

    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(dmac_tx_event_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA,
                       "{hmac_mgmt_tx_addba_rsp::send addba rsp failed, pst_event_mem mem alloc failed.}");
        oal_netbuf_free(pst_addba_rsp);

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_to_dmac_ctx_event = (frw_event_stru *)pst_event_mem->puc_data;
    frw_event_hdr_init(&(pst_hmac_to_dmac_ctx_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX, DMAC_WLAN_CTX_EVENT_SUB_TYPE_ACTION,
                       OAL_SIZEOF(dmac_tx_event_stru), FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id, pst_mac_vap->uc_device_id, pst_mac_vap->uc_vap_id);

    /* 填写事件payload */
    pst_tx_event = (dmac_tx_event_stru *)(pst_hmac_to_dmac_ctx_event->auc_event_data);
    pst_tx_event->pst_netbuf = pst_addba_rsp;
    pst_tx_event->us_frame_len = us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru);

    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA,
                       "{hmac_mgmt_tx_addba_rsp::send addba rsp failed, frw_event_dispatch_event failed[%d].}", ul_ret);
        oal_netbuf_free(pst_addba_rsp);
    } else {
        pst_ba_rx_info->en_ba_status = DMAC_BA_COMPLETE;
    }

    frw_event_free_m(pst_event_mem);
    return ul_ret;
}
OAL_STATIC oal_void hmac_fill_delba_action(dmac_ctx_action_event_stru *pst_wlan_ctx_action, oal_uint16 us_frame_len,
    hmac_user_stru *pst_hmac_user, mac_action_mgmt_args_stru *pst_action_args, oal_uint8 uc_tidno)
{
    memset_s((oal_uint8 *)pst_wlan_ctx_action, OAL_SIZEOF(dmac_ctx_action_event_stru), 0,
        OAL_SIZEOF(dmac_ctx_action_event_stru));
    pst_wlan_ctx_action->us_frame_len = us_frame_len;
    pst_wlan_ctx_action->uc_hdr_len = MAC_80211_FRAME_LEN;
    pst_wlan_ctx_action->en_action_category = MAC_ACTION_CATEGORY_BA;
    pst_wlan_ctx_action->uc_action = MAC_BA_ACTION_DELBA;
    pst_wlan_ctx_action->us_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id;
    pst_wlan_ctx_action->uc_tidno = uc_tidno;
    pst_wlan_ctx_action->uc_initiator = (oal_uint8)pst_action_args->ul_arg2;
}
OAL_STATIC oal_void hmac_delba_reset(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
    mac_device_stru *pst_device, mac_delba_initiator_enum_uint8 en_initiator, oal_uint8 uc_tidno)
{
    if (en_initiator == MAC_RECIPIENT_DELBA) {
        /* 更新对应的TID信息 */
        hmac_ba_reset_rx_handle(pst_device, &pst_hmac_user->ast_tid_info[uc_tidno].pst_ba_rx_info, uc_tidno, OAL_FALSE);
    } else {
        /* 更新对应的TID信息 */
        pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status = DMAC_BA_INIT;
        pst_hmac_user->auc_ba_flag[uc_tidno] = 0;

#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
        hmac_tx_ba_session_decr(pst_hmac_vap, pst_hmac_user->ast_tid_info[uc_tidno].uc_tid_no);
#else
        hmac_tx_ba_session_decr(pst_device, pst_hmac_user->ast_tid_info[uc_tidno].uc_tid_no);
#endif
        /* 还原设置AMPDU下AMSDU的支持情况 */
        hmac_user_set_amsdu_support(pst_hmac_user, uc_tidno);
    }
}

oal_void hmac_mgmt_tx_delba(
    hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, mac_action_mgmt_args_stru *pst_action_args)
{
    mac_device_stru *pst_device = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL; /* 申请事件返回的内存指针 */
    oal_netbuf_stru *pst_delba = OAL_PTR_NULL;
    oal_uint16 us_frame_len;
    frw_event_stru *pst_hmac_to_dmac_ctx_event = OAL_PTR_NULL;
    dmac_tx_event_stru *pst_tx_event = OAL_PTR_NULL;
    dmac_ctx_action_event_stru st_wlan_ctx_action;
    mac_delba_initiator_enum_uint8 en_initiator;
    oal_uint32 ul_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL;
#endif
    oal_uint8 uc_tidno;

    if ((pst_hmac_vap == OAL_PTR_NULL) || (pst_hmac_user == OAL_PTR_NULL) || (pst_action_args == OAL_PTR_NULL)) {
        oam_error_log3(0, OAM_SF_BA, "{hmac_mgmt_tx_delba::null param, %x %x %x.}",
                       (uintptr_t)pst_hmac_vap, (uintptr_t)pst_hmac_user, (uintptr_t)pst_action_args);
        return;
    }

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_BUTT) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{vap down/del,state[%d]}", pst_mac_vap->en_vap_state);
        return;
    }

    /* 获取device结构 */
    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_tx_delba::pst_device null.}");
        return;
    }
    en_initiator = (oal_uint8)pst_action_args->ul_arg2;
    uc_tidno = (oal_uint8)(pst_action_args->ul_arg1);
    /* 对tid对应的tx BA会话状态加锁 */
    oal_spin_lock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
    if (en_initiator == MAC_ORIGINATOR_DELBA) {
        if (pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status == DMAC_BA_INIT) {
            oal_spin_unlock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
            return;
        }
    }
    /* 申请DEL_BA管理帧内存 */
    pst_delba = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (pst_delba == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_tx_delba::pst_delba null.}");
        oal_spin_unlock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
        return;
    }

    oal_mem_netbuf_trace(pst_delba, OAL_TRUE);

    memset_s(oal_netbuf_cb(pst_delba), oal_netbuf_cb_size(), 0, oal_netbuf_cb_size());
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_delba);
#endif

    oal_netbuf_prev(pst_delba) = OAL_PTR_NULL;
    oal_netbuf_next(pst_delba) = OAL_PTR_NULL;

    en_initiator = (oal_uint8)pst_action_args->ul_arg2;

    /* 调用封装管理帧接口 */
    us_frame_len = hmac_mgmt_encap_delba(pst_hmac_vap, (oal_uint8 *)oal_netbuf_header(pst_delba),
                                         pst_action_args->puc_arg5, uc_tidno,
                                         en_initiator, (oal_uint8)pst_action_args->ul_arg3);
    hmac_fill_delba_action(&st_wlan_ctx_action, us_frame_len, pst_hmac_user, pst_action_args, uc_tidno);

    if (memcpy_s((oal_uint8 *)(oal_netbuf_data(pst_delba) + us_frame_len), (WLAN_MEM_NETBUF_SIZE2 - us_frame_len),
                 (oal_uint8 *)&st_wlan_ctx_action, OAL_SIZEOF(dmac_ctx_action_event_stru)) != EOK) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "hmac_mgmt_tx_delba::memcpy fail!");
        /* 释放管理帧内存到netbuf内存池 */
        oal_netbuf_free(pst_delba);
        /* 对tid对应的tx BA会话状态解锁 */
        oal_spin_unlock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
        return;
    }

    oal_netbuf_put(pst_delba, (us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru)));

#ifdef _PRE_WLAN_1102A_CHR
    hmac_chr_set_del_ba_info(uc_tidno, (oal_uint16)pst_action_args->ul_arg3);
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_tx_ctl->us_mpdu_len = us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru);
#endif

    /* 抛事件，到DMAC模块发送 */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(dmac_tx_event_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_tx_delba::pst_event_mem null.}");
        /* 释放管理帧内存到netbuf内存池 */
        oal_netbuf_free(pst_delba);
        oal_spin_unlock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
        return;
    }

    /* 获得事件指针 */
    pst_hmac_to_dmac_ctx_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* 填写事件头 */
    frw_event_hdr_init(&(pst_hmac_to_dmac_ctx_event->st_event_hdr), FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_ACTION, OAL_SIZEOF(dmac_tx_event_stru), FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id, pst_mac_vap->uc_device_id, pst_mac_vap->uc_vap_id);

    /* 填写事件payload */
    pst_tx_event = (dmac_tx_event_stru *)(pst_hmac_to_dmac_ctx_event->auc_event_data);
    pst_tx_event->pst_netbuf = pst_delba;
    pst_tx_event->us_frame_len = us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru);

    /* 分发 */
    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_tx_delba::dispatch_event failed[%d].}", ul_ret);
        oal_netbuf_free(pst_delba);
        frw_event_free_m(pst_event_mem);
        oal_spin_unlock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
        return;
    }

    frw_event_free_m(pst_event_mem);

    hmac_delba_reset(pst_hmac_vap, pst_hmac_user, pst_device, en_initiator, uc_tidno);
    oal_spin_unlock_bh(&(pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.st_ba_status_lock));
    return;
}
OAL_STATIC oal_void hmac_reorder_queue_init(hmac_ba_rx_stru *pst_ba_rx_stru)
{
    oal_uint8 uc_reorder_index;
    for (uc_reorder_index = 0; uc_reorder_index < WLAN_AMPDU_RX_BUFFER_SIZE; uc_reorder_index++) {
        pst_ba_rx_stru->ast_re_order_list[uc_reorder_index].in_use = 0;
        pst_ba_rx_stru->ast_re_order_list[uc_reorder_index].us_seq_num = 0;
        pst_ba_rx_stru->ast_re_order_list[uc_reorder_index].uc_num_buf = 0;

        pst_ba_rx_stru->ast_re_order_list[uc_reorder_index].en_tcp_ack_filtered[0] = OAL_FALSE;
        pst_ba_rx_stru->ast_re_order_list[uc_reorder_index].en_tcp_ack_filtered[1] = OAL_FALSE;

        oal_netbuf_list_head_init(&(pst_ba_rx_stru->ast_re_order_list[uc_reorder_index].st_netbuf_head));
    }
}
OAL_STATIC oal_void hmac_ba_window_init(hmac_ba_rx_stru *pst_ba_rx_stru, oal_uint8 *puc_payload)
{
    /* puc_payload 7byte的高4位与 8byte的低4位拼接成8bit的baw_start */
    pst_ba_rx_stru->us_baw_start = (puc_payload[7] >> 4) | (puc_payload[8] << 4);
    pst_ba_rx_stru->us_baw_size = (puc_payload[3] & 0xC0) >> 6; /* payload的3 byte高2位与其4 byte的低6位拼接成baw_size */
    pst_ba_rx_stru->us_baw_size |= (puc_payload[4] << 2);
    if ((pst_ba_rx_stru->us_baw_size == 0) || (pst_ba_rx_stru->us_baw_size > WLAN_AMPDU_RX_BUFFER_SIZE)) {
        pst_ba_rx_stru->us_baw_size = WLAN_AMPDU_RX_BUFFER_SIZE;
    }

    if (pst_ba_rx_stru->us_baw_size == 1) {
        pst_ba_rx_stru->us_baw_size = WLAN_AMPDU_RX_BUFFER_SIZE;
    }

    pst_ba_rx_stru->us_baw_end = DMAC_BA_SEQ_ADD(pst_ba_rx_stru->us_baw_start, (pst_ba_rx_stru->us_baw_size - 1));
    pst_ba_rx_stru->us_baw_tail = DMAC_BA_SEQNO_SUB(pst_ba_rx_stru->us_baw_start, 1);
    pst_ba_rx_stru->us_baw_head = DMAC_BA_SEQNO_SUB(pst_ba_rx_stru->us_baw_start, HMAC_BA_BMP_SIZE);
    pst_ba_rx_stru->uc_mpdu_cnt = 0;
    pst_ba_rx_stru->en_is_ba = OAL_TRUE;  // Ba session is processing
}

OAL_STATIC oal_void hmac_ba_timer_init(hmac_user_stru *pst_hmac_user, mac_vap_stru *pst_mac_vap, oal_uint8 uc_tid,
    hmac_ba_rx_stru *pst_ba_rx_stru)
{
    pst_ba_rx_stru->st_alarm_data.pst_ba = pst_ba_rx_stru;
    pst_ba_rx_stru->st_alarm_data.us_mac_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id;
    pst_ba_rx_stru->st_alarm_data.uc_vap_id = pst_mac_vap->uc_vap_id;
    pst_ba_rx_stru->st_alarm_data.uc_tid = uc_tid;
    pst_ba_rx_stru->st_alarm_data.us_timeout_times = 0;
    pst_ba_rx_stru->st_alarm_data.en_direction = MAC_RECIPIENT_DELBA;
    pst_ba_rx_stru->en_timer_triggered = OAL_FALSE;
}
OAL_STATIC oal_void hmac_ba_session_init(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
    hmac_ba_rx_stru *pst_ba_rx_stru, oal_uint8 *puc_payload)
{
    /* puc_payload 5byte为低8位与6byte为高8位拼接16bit的ba_timeout */
    pst_ba_rx_stru->us_ba_timeout = puc_payload[5] | (puc_payload[6] << 8);
    pst_ba_rx_stru->en_amsdu_supp = (pst_hmac_vap->en_rx_ampduplusamsdu_active ? OAL_TRUE : OAL_FALSE);
    pst_ba_rx_stru->en_back_var = MAC_BACK_COMPRESSED;
    pst_ba_rx_stru->puc_transmit_addr = pst_hmac_user->st_user_base_info.auc_user_mac_addr;
    pst_ba_rx_stru->uc_ba_policy = (puc_payload[3] & 0x02) >> 1; /* puc_payload[3]的第1bit表示为_ba_policy */
}

oal_uint32 hmac_mgmt_rx_addba_req(hmac_vap_stru *pst_hmac_vap,
                                  hmac_user_stru *pst_hmac_user,
                                  oal_uint8 *puc_payload,
                                  oal_uint32 frame_body_len)
{
    mac_device_stru *pst_device = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_uint8 uc_tid;
    oal_uint8 uc_status;
    hmac_ba_rx_stru *pst_ba_rx_stru = OAL_PTR_NULL;
    oal_uint32 ul_ret;

    if ((pst_hmac_vap == OAL_PTR_NULL) || (pst_hmac_user == OAL_PTR_NULL) || (puc_payload == OAL_PTR_NULL)) {
        oam_error_log3(0, OAM_SF_BA, "{hmac_mgmt_rx_addba_req::addba req receive failed, null param, 0x%x 0x%x 0x%x.}",
                       (uintptr_t)pst_hmac_vap, (uintptr_t)pst_hmac_user, (uintptr_t)puc_payload);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (frame_body_len < MAC_ADDBA_REQ_FRAME_BODY_LEN) {
        OAM_WARNING_LOG1(0, OAM_SF_BA, "{frame_body_len[%d] < MAC_ADDBA_REQ_FRAME_BODY_LEN.}", frame_body_len);
        return OAL_FAIL;
    }

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);

    /* 11n以上能力才可接收ampdu */
    if ((!(pst_mac_vap->en_protocol >= WLAN_HT_MODE)) ||
        (!(pst_hmac_user->st_user_base_info.en_protocol_mode >= WLAN_HT_MODE))) {
        oam_warning_log2(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_rx_addba_req::ampdu is not supprot or not open,\
                         vap protocol mode = %d, user protocol mode = %d.}",
                         pst_mac_vap->en_protocol, pst_hmac_user->st_user_base_info.en_protocol_mode);
        return OAL_SUCC;
    }

    /* 获取device结构 */
    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_rx_addba_req::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /******************************************************************/
    /* ADDBA Request Frame - Frame Body */
    /* --------------------------------------------------------------- */
    /* | Category | Action | Dialog token | Parameters | Timeout | SSN     | */
    /* --------------------------------------------------------------- */
    /* | 1        | 1      | 1      | 2          | 2       | 2       | */
    /* --------------------------------------------------------------- */
    /******************************************************************/
    uc_tid = (puc_payload[3] & 0x3C) >> 2; /* 获取puc_payload[3]的B2-B5表示tid */
    if (uc_tid >= WLAN_TID_MAX_NUM) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA,
                         "{hmac_mgmt_rx_addba_req::addba req receive failed, tid %d overflow.}", uc_tid);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    if (pst_hmac_user->ast_tid_info[uc_tid].pst_ba_rx_info != OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA,
                         "{hmac_mgmt_rx_addba_req::addba req received, but tid [%d] already set up.}", uc_tid);
        hmac_ba_reset_rx_handle(pst_device, &pst_hmac_user->ast_tid_info[uc_tid].pst_ba_rx_info, uc_tid, OAL_FALSE);
    }

    pst_hmac_user->ast_tid_info[uc_tid].pst_ba_rx_info =
    (hmac_ba_rx_stru *)oal_mem_alloc_m(OAL_MEM_POOL_ID_LOCAL, (oal_uint16)OAL_SIZEOF(hmac_ba_rx_stru), OAL_TRUE);
    if (pst_hmac_user->ast_tid_info[uc_tid].pst_ba_rx_info == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA,
                         "{hmac_mgmt_rx_addba_req::addba req receive failed, rx_hdl mem alloc failed.tid[%d]}", uc_tid);

        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_ba_rx_stru = pst_hmac_user->ast_tid_info[uc_tid].pst_ba_rx_info;
    pst_ba_rx_stru->en_ba_status = DMAC_BA_INIT;
    pst_ba_rx_stru->uc_dialog_token = puc_payload[2]; /* puc_payload第2byte是Dialog token */

    /* 初始化reorder队列 */
    hmac_reorder_queue_init(pst_ba_rx_stru);

    /* 初始化接收窗口 */
    hmac_ba_window_init(pst_ba_rx_stru, puc_payload);

    /* 初始化定时器资源 */
    hmac_ba_timer_init(pst_hmac_user, pst_mac_vap, uc_tid, pst_ba_rx_stru);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_spin_lock_init(&pst_ba_rx_stru->st_ba_lock);
#endif

    /* Ba会话参数初始化 */
    hmac_ba_session_init(pst_hmac_vap, pst_hmac_user, pst_ba_rx_stru, puc_payload);

#ifndef _PRE_PROFILING_MODE
    /* profiling测试中，接收端不删除ba */
    frw_create_timer(&(pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer),
                     hmac_ba_timeout_fn,
                     pst_hmac_vap->us_rx_timeout[WLAN_WME_TID_TO_AC(uc_tid)],
                     &(pst_ba_rx_stru->st_alarm_data),
                     OAL_FALSE,
                     OAM_MODULE_ID_HMAC,
                     pst_device->ul_core_id);
#endif
#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
    pst_hmac_vap->uc_rx_ba_session_num++;
#else
    pst_device->uc_rx_ba_session_num++;
#endif

    /* 判断建立能否成功 */
    uc_status = hmac_mgmt_check_set_rx_ba_ok(pst_hmac_vap, pst_hmac_user, pst_ba_rx_stru, pst_device);
    if (uc_status == MAC_SUCCESSFUL_STATUSCODE) {
        pst_hmac_user->ast_tid_info[uc_tid].pst_ba_rx_info->en_ba_status = DMAC_BA_INPROGRESS;
    }

    ul_ret = hmac_mgmt_tx_addba_rsp(pst_hmac_vap, pst_hmac_user, pst_ba_rx_stru, uc_tid, uc_status);

    oam_warning_log4(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_rx_addba_req::process addba req receive\
        and send addba rsp, uc_tid[%d], uc_status[%d], baw_start[%d], baw_size[%d]}\r\n",
                     uc_tid, uc_status, pst_ba_rx_stru->us_baw_start, pst_ba_rx_stru->us_baw_size);

    if ((uc_status != MAC_SUCCESSFUL_STATUSCODE) || (ul_ret != OAL_SUCC)) {
        /* pst_hmac_user->ast_tid_info[uc_tid].pst_ba_rx_info修改为在函数中置空，与其他
                调用一致 */
        hmac_ba_reset_rx_handle(pst_device, &pst_hmac_user->ast_tid_info[uc_tid].pst_ba_rx_info, uc_tid, OAL_FALSE);
        pst_ba_rx_stru = OAL_PTR_NULL;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_void hmac_token_or_policy_unmatch(mac_vap_stru *pst_mac_vap, hmac_tid_stru *pst_tid,
    oal_uint8 uc_dialog_token, oal_uint8 uc_ba_policy, oal_uint8 uc_tidno)
{
OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA,
                 "{hmac_mgmt_rx_addba_rsp::addba rsp tid[%d],status SUCC,but token/policy wr}", uc_tidno);
oam_warning_log4(pst_mac_vap->uc_vap_id, OAM_SF_BA,
                 "{hmac_mgmt_rx_addba_rsp::rsp policy[%d],req policy[%d], rsp dialog[%d], req dialog[%d]}",
                 uc_ba_policy, pst_tid->st_ba_tx_info.uc_ba_policy, uc_dialog_token,
                 pst_tid->st_ba_tx_info.uc_dialog_token);
}
OAL_STATIC oal_void hmac_delba_for_wrong_state(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
    mac_vap_stru *pst_mac_vap, mac_action_mgmt_args_stru *pst_action_args, oal_uint8 uc_tidno)
{
    pst_action_args->uc_category = MAC_ACTION_CATEGORY_BA;
    pst_action_args->uc_action = MAC_BA_ACTION_DELBA;
    pst_action_args->ul_arg1 = uc_tidno;                                            /* 该数据帧对应的TID号 */
    pst_action_args->ul_arg2 = MAC_ORIGINATOR_DELBA;                               /* DELBA中，触发删除BA会话的发起端 */
    pst_action_args->ul_arg3 = MAC_UNSPEC_QOS_REASON;                               /* DELBA中代表删除reason */
    pst_action_args->puc_arg5 = pst_hmac_user->st_user_base_info.auc_user_mac_addr; /* DELBA中代表目的地址 */

    hmac_mgmt_tx_delba(pst_hmac_vap, pst_hmac_user, pst_action_args);
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA,
                     "{hmac_mgmt_rx_addba_rsp::addba rsp is received when ba status is BA_INIT.tid[%d]}", uc_tidno);
}

OAL_STATIC oal_void hmac_addba_resp_success(oal_uint8 *puc_payload, hmac_tid_stru *pst_tid,
    dmac_ctx_action_event_stru *pst_rx_addba_rsp_event)
{
    oal_uint16 us_baw_size;
    oal_uint8 uc_ampdu_max_num;
    /* 只有状态为成功时，才有必要将这些信息传递给dmac */
    pst_rx_addba_rsp_event->uc_ba_policy = ((puc_payload[5] & 0x02) >> 1); /* uc_ba_policy是puc_payload[5]的1bit */
    /* puc_payload 7byte为低8位与puc_payload 8byte为高8位拼成16bit的ba_timeout */
    pst_rx_addba_rsp_event->us_ba_timeout = puc_payload[7] | (puc_payload[8] << 8);
    pst_rx_addba_rsp_event->en_amsdu_supp = puc_payload[5] & BIT0;  /* en_amsdu_supp是puc_payload[5]的第0bit */
    /* puc_payload 5byte保留高2位与puc_payload 6byte保留低低6位拼接成baw_size */
    us_baw_size = (oal_uint16)(((puc_payload[5] & 0xC0) >> 6) | (puc_payload[6] << 2));
    if ((us_baw_size == 0) || (us_baw_size > WLAN_AMPDU_TX_MAX_BUF_SIZE)) {
        us_baw_size = WLAN_AMPDU_TX_MAX_BUF_SIZE;
    }

    uc_ampdu_max_num = (oal_uint8)us_baw_size / WLAN_AMPDU_TX_SCHD_STRATEGY;

    pst_rx_addba_rsp_event->uc_ampdu_max_num = oal_max(uc_ampdu_max_num, 1);
    pst_rx_addba_rsp_event->us_baw_size = us_baw_size;

    /* 设置hmac模块对应的BA句柄的信息 */
    pst_tid->st_ba_tx_info.en_ba_status = DMAC_BA_COMPLETE;
    pst_tid->st_ba_tx_info.uc_addba_attemps = 0;
}

oal_uint32 hmac_mgmt_rx_addba_rsp(hmac_vap_stru *pst_hmac_vap,
                                  hmac_user_stru *pst_hmac_user,
                                  oal_uint8 *puc_payload,
                                  oal_uint32 frame_body_len)
{
    mac_device_stru *pst_mac_device = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL; /* 申请事件返回的内存指针 */
    frw_event_stru *pst_hmac_to_dmac_crx_sync = OAL_PTR_NULL;
    dmac_ctx_action_event_stru *pst_rx_addba_rsp_event = OAL_PTR_NULL;
    oal_uint8 uc_tidno;
    hmac_tid_stru *pst_tid = OAL_PTR_NULL;
    mac_action_mgmt_args_stru st_action_args;
    oal_uint8 uc_dialog_token;
    oal_uint8 uc_ba_status;
    oal_uint8 uc_ba_policy;

    if ((pst_hmac_vap == OAL_PTR_NULL) || (pst_hmac_user == OAL_PTR_NULL) || (puc_payload == OAL_PTR_NULL)) {
        oam_error_log3(0, OAM_SF_BA,
                       "{hmac_mgmt_rx_addba_rsp::null param, %x %x %x.}",
                       (uintptr_t)pst_hmac_vap, (uintptr_t)pst_hmac_user, (uintptr_t)puc_payload);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (frame_body_len < MAC_ADDBA_RSP_FRAME_BODY_LEN) {
        OAM_WARNING_LOG1(0, OAM_SF_BA, "{frame_body_len[%d] < MAC_ADDBA_REQ_FRAME_BODY_LEN.}", frame_body_len);
        return OAL_FAIL;
    }

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);

    /* 获取device结构 */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (oal_unlikely(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_rx_addba_rsp::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /******************************************************************/
    /* ADDBA Response Frame - Frame Body */
    /* --------------------------------------------------------------- */
    /* | Category | Action | Dialog token | Status  | Parameters | Timeout | */
    /* --------------------------------------------------------------- */
    /* | 1        | 1      | 1      | 2       | 2          | 2       | */
    /* --------------------------------------------------------------- */
    /******************************************************************/
    uc_tidno = (puc_payload[5] & 0x3C) >> 2; /* 保留puc_payload[5]的2-5bit，右移2位，表示tidno */
    /* 协议支持tid为0~15,02只支持tid0~7 */
    if (uc_tidno >= WLAN_TID_MAX_NUM) {
        /* 对于tid > 7的resp直接忽略 */
        oam_warning_log3(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_rx_addba_rsp::tid[%d]token[%d]state[%d]",
                         uc_tidno, puc_payload[2], puc_payload[3]); /* puc_payload第2、3byte为参数输出打印 */
        return OAL_SUCC;
    }

    uc_dialog_token = puc_payload[2]; /* puc_payload第2byte是Dialog token */
    uc_ba_status = puc_payload[3]; /* puc_payload第3byte是Status */
    uc_ba_policy = ((puc_payload[5] & 0x02) >> 1); /* ba_policy为puc_payload[5]第1bit */
    pst_tid = &(pst_hmac_user->ast_tid_info[uc_tidno]);

    /* 对tid对应的tx BA会话状态加锁 */
    oal_spin_lock_bh(&(pst_tid->st_ba_tx_info.st_ba_status_lock));

    /* BA状态成功，但token、policy不匹配，需要删除聚合 */
    
    if ((pst_tid->st_ba_tx_info.en_ba_status == DMAC_BA_INPROGRESS) && (uc_ba_status == MAC_SUCCESSFUL_STATUSCODE)) {
        if ((uc_dialog_token != pst_tid->st_ba_tx_info.uc_dialog_token) ||
            (uc_ba_policy != pst_tid->st_ba_tx_info.uc_ba_policy)) {
            /* 对tid对应的tx BA会话状态解锁 */
            oal_spin_unlock_bh(&(pst_tid->st_ba_tx_info.st_ba_status_lock));
            hmac_token_or_policy_unmatch(pst_mac_vap, pst_tid, uc_dialog_token, uc_ba_policy, uc_tidno);
            return OAL_SUCC;
        }
    }

    if (pst_tid->st_ba_tx_info.en_ba_status == DMAC_BA_INIT) {
        /* 释放锁一定要放在调用hmac_mgmt_tx_delba前面,因为hmac_mgmt_tx_delba函数内也使用该锁,否则会导致死锁 */
        oal_spin_unlock_bh(&(pst_tid->st_ba_tx_info.st_ba_status_lock));
        /* 发送DELBA帧 */
        hmac_delba_for_wrong_state(pst_hmac_vap, pst_hmac_user, pst_mac_vap, &st_action_args, uc_tidno);
        return OAL_SUCC;
    }

    if (pst_tid->st_ba_tx_info.en_ba_status == DMAC_BA_COMPLETE) {
        oam_warning_log2(pst_mac_vap->uc_vap_id, OAM_SF_BA,
            "{hmac_mgmt_rx_addba_rsp::addba rsp is received whne ba status is\
            DMAC_BA_COMPLETE or uc_dialog_token wrong.tid[%d], status[%d], rsp dialog[%d], req dialog[%d]}",
                         uc_tidno, pst_tid->st_ba_tx_info.en_ba_status);
        oal_spin_unlock_bh(&(pst_tid->st_ba_tx_info.st_ba_status_lock));
        return OAL_SUCC;
    }

    /* 停止计时器 */
    frw_immediate_destroy_timer(&pst_tid->st_ba_tx_info.st_addba_timer);

    /* 抛事件到DMAC处理 */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(dmac_ctx_action_event_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_rx_addba_rsp::pst_event_mem null.}");
        oal_spin_unlock_bh(&(pst_tid->st_ba_tx_info.st_ba_status_lock));
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获得事件指针 */
    pst_hmac_to_dmac_crx_sync = (frw_event_stru *)pst_event_mem->puc_data;

    /* 填写事件头 */
    frw_event_hdr_init(&(pst_hmac_to_dmac_crx_sync->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX, DMAC_WLAN_CTX_EVENT_SUB_TYPE_BA_SYNC,
                       OAL_SIZEOF(dmac_ctx_action_event_stru), FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    /* 获取帧体信息，由于DMAC的同步，填写事件payload */
    pst_rx_addba_rsp_event = (dmac_ctx_action_event_stru *)(pst_hmac_to_dmac_crx_sync->auc_event_data);
    pst_rx_addba_rsp_event->en_action_category = MAC_ACTION_CATEGORY_BA;
    pst_rx_addba_rsp_event->uc_action = MAC_BA_ACTION_ADDBA_RSP;
    pst_rx_addba_rsp_event->us_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id;
    pst_rx_addba_rsp_event->uc_status = puc_payload[3]; /* puc_payload第3byte是Status */
    pst_rx_addba_rsp_event->uc_tidno = uc_tidno;
    pst_rx_addba_rsp_event->uc_dialog_token = puc_payload[2]; /* puc_payload第2byte是 Dialog token */

    if (pst_rx_addba_rsp_event->uc_status != MAC_SUCCESSFUL_STATUSCODE) {
        /* 重置HMAC模块信息 */
        pst_tid->st_ba_tx_info.en_ba_status = DMAC_BA_INIT;

#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
        hmac_tx_ba_session_decr(pst_hmac_vap, uc_tidno);
#else
        hmac_tx_ba_session_decr(pst_mac_device, uc_tidno);
#endif
    } else {
        hmac_addba_resp_success(puc_payload, pst_tid, pst_rx_addba_rsp_event);
        /*lint -e502*/
        if (pst_rx_addba_rsp_event->en_amsdu_supp && pst_hmac_vap->en_amsdu_ampdu_active) {
            hmac_user_set_amsdu_support(pst_hmac_user, uc_tidno);
        } else {
            hmac_user_set_amsdu_not_support(pst_hmac_user, uc_tidno);
        }
        /*lint +e502*/
    }

    /* 分发 */
    frw_event_dispatch_event(pst_event_mem);

    /* 释放事件内存 */
    frw_event_free_m(pst_event_mem);
    oal_spin_unlock_bh(&(pst_tid->st_ba_tx_info.st_ba_status_lock));
    return OAL_SUCC;
}


oal_uint32 hmac_mgmt_rx_delba(hmac_vap_stru *pst_hmac_vap,
                              hmac_user_stru *pst_hmac_user,
                              oal_uint8 *puc_payload,
                              oal_uint32 frame_body_len)
{
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL; /* 申请事件返回的内存指针 */
    frw_event_stru *pst_hmac_to_dmac_crx_sync = OAL_PTR_NULL;
    dmac_ctx_action_event_stru *pst_wlan_crx_action = OAL_PTR_NULL;
    mac_device_stru *pst_device = OAL_PTR_NULL;
    hmac_tid_stru *pst_tid = OAL_PTR_NULL;
    oal_uint8 uc_tid;
    oal_uint8 uc_initiator;
    oal_uint16 us_reason;

    if (oal_unlikely((pst_hmac_vap == OAL_PTR_NULL) || (pst_hmac_user == OAL_PTR_NULL) ||
                     (puc_payload == OAL_PTR_NULL))) {
        oam_error_log3(0, OAM_SF_BA,
                       "{hmac_mgmt_rx_delba::null param, %x %x %x.}",
                       (uintptr_t)pst_hmac_vap, (uintptr_t)pst_hmac_user, (uintptr_t)puc_payload);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (frame_body_len < MAC_ADDBA_DEL_FRAME_BODY_LEN) {
        OAM_WARNING_LOG1(0, OAM_SF_BA, "{frame_body_len[%d] < MAC_ADDBA_DEL_FRAME_BODY_LEN.}", frame_body_len);
        return OAL_FAIL;
    }

    /* 获取device结构 */
    pst_device = mac_res_get_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (oal_unlikely(pst_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_mgmt_rx_delba::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /************************************************/
    /* DELBA Response Frame - Frame Body */
    /* -------------------------------------------- */
    /* | Category | Action | Parameters | Reason  | */
    /* -------------------------------------------- */
    /* | 1        | 1      | 2          | 2       | */
    /* -------------------------------------------- */
    /************************************************/
    uc_tid = (puc_payload[3] & 0xF0) >> 4; /* puc_payload 3byte的高4位为tid */
    uc_initiator = (puc_payload[3] & 0x08) >> 3; /* puc_payload 3byte的第3bit为initiator */
    /* puc_payload 4byte为低8位与5byte左移8位为高8位拼接成16位的Reason */
    us_reason = (puc_payload[4] & 0xFF) | ((puc_payload[5] << 8) & 0xFF00);

    /* tid保护，避免数组越界 */
    if (uc_tid >= WLAN_TID_MAX_NUM) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                         "{hmac_mgmt_rx_delba::delba receive failed, tid %d overflow.}", uc_tid);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    pst_tid = &(pst_hmac_user->ast_tid_info[uc_tid]);

    /* 对tid对应的tx BA会话状态加锁 */
    oal_spin_lock_bh(&(pst_tid->st_ba_tx_info.st_ba_status_lock));

    oam_warning_log3(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                     "{hmac_mgmt_rx_delba::receive delba from peer sta, tid[%d], uc_initiator[%d], reason[%d].}",
                     uc_tid, uc_initiator, us_reason);

    /* 重置BA发送会话 */
    if (uc_initiator == MAC_RECIPIENT_DELBA) {
        if (pst_tid->st_ba_tx_info.en_ba_status == DMAC_BA_INIT) {
            oal_spin_unlock_bh(&(pst_tid->st_ba_tx_info.st_ba_status_lock));
            return OAL_SUCC;
        }

        pst_hmac_user->auc_ba_flag[uc_tid] = 0;

        /* 还原设置AMPDU下AMSDU的支持情况 */
        hmac_user_set_amsdu_support(pst_hmac_user, uc_tid);

#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
        hmac_tx_ba_session_decr(pst_hmac_vap, pst_hmac_user->ast_tid_info[uc_tid].uc_tid_no);
#else
        hmac_tx_ba_session_decr(pst_device, pst_hmac_user->ast_tid_info[uc_tid].uc_tid_no);
#endif
    } else { /* 重置BA接收会话 */
        hmac_ba_reset_rx_handle(pst_device, &pst_hmac_user->ast_tid_info[uc_tid].pst_ba_rx_info, uc_tid, OAL_FALSE);
    }

    /* 抛事件到DMAC处理 */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(dmac_ctx_action_event_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                       "{hmac_mgmt_rx_delba::pst_event_mem null.}");
        oal_spin_unlock_bh(&(pst_tid->st_ba_tx_info.st_ba_status_lock));
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获得事件指针 */
    pst_hmac_to_dmac_crx_sync = (frw_event_stru *)pst_event_mem->puc_data;

    /* 填写事件头 */
    frw_event_hdr_init(&(pst_hmac_to_dmac_crx_sync->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_BA_SYNC,
                       OAL_SIZEOF(dmac_ctx_action_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    /* 填写事件payload */
    pst_wlan_crx_action = (dmac_ctx_action_event_stru *)(pst_hmac_to_dmac_crx_sync->auc_event_data);
    pst_wlan_crx_action->en_action_category = MAC_ACTION_CATEGORY_BA;
    pst_wlan_crx_action->uc_action = MAC_BA_ACTION_DELBA;
    pst_wlan_crx_action->us_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id;

    pst_wlan_crx_action->uc_tidno = uc_tid;
    pst_wlan_crx_action->uc_initiator = uc_initiator;

    /* 分发 */
    frw_event_dispatch_event(pst_event_mem);

    /* 释放事件内存 */
    frw_event_free_m(pst_event_mem);

    /* DELBA事件先处理再改状态,防止addba req先处理 */
    if (uc_initiator == MAC_RECIPIENT_DELBA) {
        pst_tid->st_ba_tx_info.en_ba_status = DMAC_BA_INIT;
    }
    oal_spin_unlock_bh(&(pst_tid->st_ba_tx_info.st_ba_status_lock));
    return OAL_SUCC;
}


oal_uint32 hmac_mgmt_tx_addba_timeout(oal_void *p_arg)
{
    hmac_vap_stru *pst_vap = OAL_PTR_NULL; /* vap指针 */
    oal_uint8 *puc_da = OAL_PTR_NULL;      /* 保存用户目的地址的指针 */
    hmac_user_stru *pst_hmac_user = OAL_PTR_NULL;
    mac_action_mgmt_args_stru st_action_args;
    dmac_ba_alarm_stru *pst_alarm_data = OAL_PTR_NULL;

    if (p_arg == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_BA, "{hmac_mgmt_tx_addba_timeout::p_arg null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_alarm_data = (dmac_ba_alarm_stru *)p_arg;

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_alarm_data->us_mac_user_idx);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_BA,
                         "{hmac_mgmt_tx_addba_timeout::pst_hmac_user[%d] null.}",
                         pst_alarm_data->us_mac_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    puc_da = pst_hmac_user->st_user_base_info.auc_user_mac_addr;

    pst_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_alarm_data->uc_vap_id);
    if (oal_unlikely(pst_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_BA,
                       "{hmac_mgmt_tx_addba_timeout::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 生成DELBA帧 */
    st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
    st_action_args.uc_action = MAC_BA_ACTION_DELBA;
    st_action_args.ul_arg1 = pst_alarm_data->uc_tid; /* 该数据帧对应的TID号 */
    st_action_args.ul_arg2 = MAC_ORIGINATOR_DELBA;   /* DELBA中，触发删除BA会话的发起端 */
    st_action_args.ul_arg3 = MAC_QSTA_TIMEOUT;       /* DELBA中代表删除reason */
    st_action_args.puc_arg5 = puc_da;                /* DELBA中代表目的地址 */

    hmac_mgmt_tx_delba(pst_vap, pst_hmac_user, &st_action_args);

    return OAL_SUCC;
}


oal_uint32 hmac_mgmt_tx_ampdu_start(hmac_vap_stru *pst_hmac_vap,
                                    hmac_user_stru *pst_hmac_user,
                                    mac_priv_req_args_stru *pst_priv_req)
{
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL; /* 申请事件返回的内存指针 */
    frw_event_stru *pst_crx_priv_req_event = OAL_PTR_NULL;
    mac_priv_req_args_stru *pst_rx_ampdu_start_event = OAL_PTR_NULL;
    oal_uint8 uc_tidno;
    hmac_tid_stru *pst_tid = OAL_PTR_NULL;
    oal_uint32 ul_ret;

    if ((pst_hmac_vap == OAL_PTR_NULL) || (pst_hmac_user == OAL_PTR_NULL) || (pst_priv_req == OAL_PTR_NULL)) {
        oam_error_log3(0, OAM_SF_AMPDU,
                       "{hmac_mgmt_tx_ampdu_start::param null, %x %x %x.}",
                       (uintptr_t)pst_hmac_vap, (uintptr_t)pst_hmac_user, (uintptr_t)pst_priv_req);
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_tidno = pst_priv_req->uc_arg1;
    pst_tid = &(pst_hmac_user->ast_tid_info[uc_tidno]);

    /* AMPDU为NORMAL ACK时，对应的BA会话没有建立，则返回 */
    if (pst_priv_req->uc_arg3 == WLAN_TX_NORMAL_ACK) {
        if (pst_tid->st_ba_tx_info.en_ba_status == DMAC_BA_INIT) {
            oam_info_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU,
                          "{hmac_mgmt_tx_ampdu_start::normal ack but ba session is not build.}");
            return OAL_SUCC;
        }
    }

    /* 抛事件到DMAC处理 */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(mac_priv_req_args_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU,
                       "{hmac_mgmt_tx_ampdu_start::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获得事件指针 */
    pst_crx_priv_req_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* 填写事件头 */
    frw_event_hdr_init(&(pst_crx_priv_req_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_PRIV_REQ,
                       OAL_SIZEOF(mac_priv_req_args_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    /* 获取设置AMPDU的参数，到dmac进行设置 */
    pst_rx_ampdu_start_event = (mac_priv_req_args_stru *)(pst_crx_priv_req_event->auc_event_data);
    pst_rx_ampdu_start_event->uc_type = MAC_A_MPDU_START;
    pst_rx_ampdu_start_event->uc_arg1 = pst_priv_req->uc_arg1;
    pst_rx_ampdu_start_event->uc_arg2 = pst_priv_req->uc_arg2;
    pst_rx_ampdu_start_event->uc_arg3 = pst_priv_req->uc_arg3;
    pst_rx_ampdu_start_event->us_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id; /* 保存的是资源池的索引 */

    /* 分发 */
    ul_ret = frw_event_dispatch_event(pst_event_mem);

    /* 释放事件内存 */
    frw_event_free_m(pst_event_mem);

    return ul_ret;
}


oal_uint32 hmac_mgmt_tx_ampdu_end(hmac_vap_stru *pst_hmac_vap,
                                  hmac_user_stru *pst_hmac_user,
                                  mac_priv_req_args_stru *pst_priv_req)
{
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL; /* 申请事件返回的内存指针 */
    frw_event_stru *pst_crx_priv_req_event = OAL_PTR_NULL;
    mac_priv_req_args_stru *pst_rx_ampdu_end_event = OAL_PTR_NULL;
    oal_uint32 ul_ret;

    if ((pst_hmac_vap == OAL_PTR_NULL) || (pst_hmac_user == OAL_PTR_NULL) || (pst_priv_req == OAL_PTR_NULL)) {
        oam_error_log3(0, OAM_SF_AMPDU,
                       "{hmac_mgmt_tx_ampdu_end::param null, %x %x %x.}",
                       (uintptr_t)pst_hmac_vap, (uintptr_t)pst_hmac_user, (uintptr_t)pst_priv_req);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 抛事件到DMAC处理 */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(mac_priv_req_args_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU,
                       "{hmac_mgmt_tx_ampdu_end::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获得事件指针 */
    pst_crx_priv_req_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* 填写事件头 */
    frw_event_hdr_init(&(pst_crx_priv_req_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_PRIV_REQ,
                       OAL_SIZEOF(mac_priv_req_args_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    /* 获取设置AMPDU的参数，到dmac进行设置 */
    pst_rx_ampdu_end_event = (mac_priv_req_args_stru *)(pst_crx_priv_req_event->auc_event_data);
    pst_rx_ampdu_end_event->uc_type = MAC_A_MPDU_END;                                   /* 类型 */
    pst_rx_ampdu_end_event->uc_arg1 = pst_priv_req->uc_arg1;                            /* tid no */
    pst_rx_ampdu_end_event->us_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id; /* 保存的是资源池的索引 */

    /* 分发 */
    ul_ret = frw_event_dispatch_event(pst_event_mem);

    /* 释放事件内存 */
    frw_event_free_m(pst_event_mem);

    return ul_ret;
}


oal_uint32 hmac_tx_mgmt_send_event(mac_vap_stru *pst_vap,
                                   oal_netbuf_stru *pst_mgmt_frame, oal_uint16 us_frame_len)
{
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL;
    frw_event_stru *pst_event = OAL_PTR_NULL;
    oal_uint32 ul_return;
    dmac_tx_event_stru *pst_ctx_stru = OAL_PTR_NULL;

    if ((pst_vap == OAL_PTR_NULL) || (pst_mgmt_frame == OAL_PTR_NULL)) {
        oam_error_log2(0, OAM_SF_SCAN,
                       "{hmac_tx_mgmt_send_event::param null, %x %x.}",
                       (uintptr_t)pst_vap, (uintptr_t)pst_mgmt_frame);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 抛事件给DMAC,让DMAC完成配置VAP创建 */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(dmac_tx_event_stru));
    if (oal_unlikely(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_SCAN,
                       "{hmac_tx_mgmt_send_event::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* 填写事件头 */
    frw_event_hdr_init(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_MGMT,
                       OAL_SIZEOF(dmac_tx_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_vap->uc_chip_id,
                       pst_vap->uc_device_id,
                       pst_vap->uc_vap_id);

    pst_ctx_stru = (dmac_tx_event_stru *)pst_event->auc_event_data;
    pst_ctx_stru->pst_netbuf = pst_mgmt_frame;
    pst_ctx_stru->us_frame_len = us_frame_len;

#ifdef _PRE_WLAN_FEATURE_SNIFFER
    proc_sniffer_write_file (NULL, 0, (oal_uint8 *)(oal_netbuf_header(pst_mgmt_frame)), us_frame_len, 1);
#endif
    ul_return = frw_event_dispatch_event(pst_event_mem);
    if (ul_return != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_tx_mgmt_send_event::frw_event_dispatch_event failed[%d].}", ul_return);
        frw_event_free_m(pst_event_mem);
        return ul_return;
    }

    /* 释放事件 */
    frw_event_free_m(pst_event_mem);

    return OAL_SUCC;
}


oal_uint32 hmac_mgmt_reset_psm(mac_vap_stru *pst_vap, oal_uint16 us_user_id)
{
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL;
    frw_event_stru *pst_event = OAL_PTR_NULL;
    oal_uint16 *pus_user_id = OAL_PTR_NULL;
    hmac_user_stru *pst_hmac_user = OAL_PTR_NULL;

    if (pst_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "{hmac_mgmt_reset_psm::pst_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 在这里直接做重置的一些操作，不需要再次同步 */
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_id);
    if (oal_unlikely(pst_hmac_user == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_PWR,
                       "{hmac_mgmt_reset_psm::pst_hmac_user[%d] null.}", us_user_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(oal_uint16));
    if (oal_unlikely(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_PWR, "{hmac_mgmt_reset_psm::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* 填写事件头 */
    frw_event_hdr_init(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_RESET_PSM,
                       OAL_SIZEOF(oal_uint16),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_vap->uc_chip_id,
                       pst_vap->uc_device_id,
                       pst_vap->uc_vap_id);

    pus_user_id = (oal_uint16 *)pst_event->auc_event_data;

    *pus_user_id = us_user_id;

    frw_event_dispatch_event(pst_event_mem);

    frw_event_free_m(pst_event_mem);

    return OAL_SUCC;
}

oal_void hmac_mgmt_send_deauth_frame(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_da, oal_uint8 da_len,
    oal_uint16 us_err_code, oal_bool_enum_uint8 en_is_protected)
{
    oal_uint16 us_deauth_len;
    oal_netbuf_stru *pst_deauth = OAL_PTR_NULL;
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL;
    oal_uint32 ul_ret;

    if ((pst_mac_vap == OAL_PTR_NULL) || (puc_da == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_AUTH, "{hmac_mgmt_send_deauth_frame::param null.}");
        return;
    }

    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_BUTT) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_AUTH,
                         "{hmac_mgmt_send_deauth_frame::vap has been down/del,state[%d].}", pst_mac_vap->en_vap_state);
        return;
    }
    pst_deauth = (oal_netbuf_stru *)oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2,
                                                         OAL_NETBUF_PRIORITY_MID);

    oal_mem_netbuf_trace(pst_deauth, OAL_TRUE);

    if (pst_deauth == OAL_PTR_NULL) {
        /* Reserved Memory pool tried for high priority deauth frames */
        pst_deauth = (oal_netbuf_stru *)oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2,
                                                             OAL_NETBUF_PRIORITY_MID);
        if (pst_deauth == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "{hmac_mgmt_send_deauth_frame::pst_deauth null.}");
            return;
        }
    }

    memset_s(oal_netbuf_cb(pst_deauth), oal_netbuf_cb_size(), 0, oal_netbuf_cb_size());

    us_deauth_len = hmac_mgmt_encap_deauth(pst_mac_vap, (oal_uint8 *)oal_netbuf_header(pst_deauth), puc_da,
                                           us_err_code);
    if (us_deauth_len == 0) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "{hmac_mgmt_send_deauth_frame:: us_deauth_len = 0.}");
        oal_netbuf_free(pst_deauth);
        return;
    }
    oal_netbuf_put(pst_deauth, us_deauth_len);

    /* MIB variables related to deauthentication are updated */
    mac_mib_set_DeauthenticateReason(pst_mac_vap, us_err_code);
    mac_mib_set_DeauthenticateStation(pst_mac_vap, puc_da);

    /* 增加发送去认证帧时的维测信息 */
    oam_warning_log4(pst_mac_vap->uc_vap_id, OAM_SF_AUTH,
                     "{hmac_mgmt_send_deauth_frame:: send deauth frame to %2x:XX:XX:XX:%2x:%2x, status code[%d]}",
                     puc_da[0], puc_da[4], puc_da[5], us_err_code); /* puc_da第0、4、5byte为参数输出打印 */

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_deauth); /* 获取cb结构体 */
    pst_tx_ctl->us_mpdu_len = us_deauth_len;                   /* dmac发送需要的mpdu长度 */
    mac_vap_set_cb_tx_user_idx(pst_mac_vap, pst_tx_ctl, puc_da);

    /* Buffer this frame in the Memory Queue for transmission */
    ul_ret = hmac_tx_mgmt_send_event(pst_mac_vap, pst_deauth, us_deauth_len);
    if (ul_ret != OAL_SUCC) {
        oal_netbuf_free(pst_deauth);

        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_AUTH,
                         "{hmac_mgmt_send_deauth_frame::hmac_tx_mgmt_send_event failed[%d].}", ul_ret);
    }
}

oal_uint32 hmac_config_send_deauth(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_da)
{
    hmac_user_stru *pst_hmac_user = OAL_PTR_NULL;

    if (oal_unlikely((pst_mac_vap == OAL_PTR_NULL) || (puc_da == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_AUTH, "{hmac_config_send_deauth::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_user = mac_vap_get_hmac_user_by_addr(pst_mac_vap, puc_da, WLAN_MAC_ADDR_LEN);
    if (pst_hmac_user == OAL_PTR_NULL) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "{hmac_config_send_deauth::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_hmac_user->st_user_base_info.en_user_asoc_state != MAC_USER_STATE_ASSOC) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "{hmac_config_send_deauth::the user is unassociated.}");
        return OAL_FAIL;
    }

    /* 发去认证帧 */
    hmac_mgmt_send_deauth_frame(pst_mac_vap, puc_da, WLAN_MAC_ADDR_LEN, MAC_AUTH_NOT_VALID, OAL_FALSE);

    /* 删除用户 */
    hmac_user_del(pst_mac_vap, pst_hmac_user);

    return OAL_SUCC;
}


oal_void hmac_mgmt_send_disassoc_frame(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_da, oal_uint16 us_err_code,
                                       oal_bool_enum_uint8 en_is_protected)
{
    oal_uint16 us_disassoc_len;
    oal_netbuf_stru *pst_disassoc = OAL_PTR_NULL;
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL;
    oal_uint32 ul_ret;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint32 ul_pedding_data = 0;
#endif

    if ((pst_mac_vap == OAL_PTR_NULL) || (puc_da == OAL_PTR_NULL)) {
        oam_error_log2(0, OAM_SF_ASSOC,
                       "{hmac_mgmt_send_disassoc_frame::param null,%x %x.}", (uintptr_t)pst_mac_vap, (uintptr_t)puc_da);
        return;
    }

    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_BUTT) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_mgmt_send_disassoc_frame:: vap has been down/del, vap_state[%d].}",
                         pst_mac_vap->en_vap_state);
        return;
    }
    pst_disassoc = (oal_netbuf_stru *)oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2,
                                                           OAL_NETBUF_PRIORITY_MID);
    if (pst_disassoc == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_mgmt_send_disassoc_frame::pst_disassoc null.}");
        return;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    hmac_config_scan_abort(pst_mac_vap, OAL_SIZEOF(oal_uint32), (oal_uint8 *)&ul_pedding_data);
#endif

    oal_mem_netbuf_trace(pst_disassoc, OAL_TRUE);

    memset_s(oal_netbuf_cb(pst_disassoc), oal_netbuf_cb_size(), 0, oal_netbuf_cb_size());

    us_disassoc_len = hmac_mgmt_encap_disassoc(pst_mac_vap, (oal_uint8 *)oal_netbuf_header(pst_disassoc), puc_da,
                                               us_err_code);
    if (us_disassoc_len == 0) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_mgmt_send_disassoc_frame:: us_disassoc_len = 0.}");
        oal_netbuf_free(pst_disassoc);
        return;
    }

    mac_mib_set_DisassocReason(pst_mac_vap, us_err_code);
    mac_mib_set_DisassocStation(pst_mac_vap, puc_da);

    /* 增加发送去关联帧时的维测信息 */
    oam_warning_log4(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                     "{hmac_mgmt_send_disassoc_frame::Because of err_code[%d],\
                     send disassoc frame to dest addr, da[%2x:xx:xx:xx:%2x:%2x].}",
                     us_err_code, puc_da[0], puc_da[4], puc_da[5]); /* puc_da第0、4、5byte为参数输出打印 */

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_disassoc);
    pst_tx_ctl->us_mpdu_len = us_disassoc_len;
    mac_vap_set_cb_tx_user_idx(pst_mac_vap, pst_tx_ctl, puc_da);

    oal_netbuf_put(pst_disassoc, us_disassoc_len);

    /* 加入发送队列 */
    ul_ret = hmac_tx_mgmt_send_event(pst_mac_vap, pst_disassoc, us_disassoc_len);
    if (ul_ret != OAL_SUCC) {
        oal_netbuf_free(pst_disassoc);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_mgmt_send_disassoc_frame:: failed[%d].}", ul_ret);
    }
}


oal_void hmac_mgmt_update_assoc_user_qos_table(oal_uint8 *puc_payload,
                                               oal_uint16 us_msg_len,
                                               oal_uint16 us_info_elem_offset,
                                               hmac_user_stru *pst_hmac_user)
{
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL;
    frw_event_stru *pst_event = OAL_PTR_NULL;
    dmac_ctx_asoc_set_reg_stru st_asoc_set_reg_param = { 0 };
    oal_uint8 *puc_ie = OAL_PTR_NULL;

    /* 如果关联用户之前就是wmm使能的，什么都不用做，直接返回 */
    if (pst_hmac_user->st_user_base_info.st_cap_info.bit_qos == OAL_TRUE) {
        oam_info_log0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ASSOC,
                      "{hmac_mgmt_update_assoc_user_qos_table::assoc user is wmm cap already.}");
        return;
    }

    mac_user_set_qos(&pst_hmac_user->st_user_base_info, OAL_FALSE);

    if (us_info_elem_offset < us_msg_len) {
        us_msg_len -= us_info_elem_offset;
        puc_payload += us_info_elem_offset;

        puc_ie = mac_get_wmm_ie(puc_payload, us_msg_len);
        if (puc_ie != OAL_PTR_NULL) {
            mac_user_set_qos(&pst_hmac_user->st_user_base_info, OAL_TRUE);
        } else {
            /* 如果关联用户之前就是没有携带wmm ie, 再查找HT CAP能力 */
            puc_ie = mac_find_ie(MAC_EID_HT_CAP, puc_payload, us_msg_len);
            if (puc_ie != OAL_PTR_NULL) {
                mac_user_set_qos(&pst_hmac_user->st_user_base_info, OAL_TRUE);
            }
        }
    }

    /* 如果关联用户到现在仍然不是wmm使能的，什么也不做，直接返回 */
    if (pst_hmac_user->st_user_base_info.st_cap_info.bit_qos == OAL_FALSE) {
        return;
    }

    /* 当关联用户从不支持wmm到支持wmm转换时，抛事件到DMAC 写寄存器 */
    /* 申请事件内存 */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(dmac_ctx_asoc_set_reg_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_mgmt_update_assoc_user_qos_table::event alloc null[%d].}",
                       OAL_SIZEOF(dmac_ctx_asoc_set_reg_stru));
        return;
    }

    st_asoc_set_reg_param.uc_user_index = pst_hmac_user->st_user_base_info.us_assoc_id;

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    frw_event_hdr_init(&(pst_event->st_event_hdr), FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_ASOC_WRITE_REG, OAL_SIZEOF(dmac_ctx_asoc_set_reg_stru),
                       FRW_EVENT_PIPELINE_STAGE_1, pst_hmac_user->st_user_base_info.uc_chip_id,
                       pst_hmac_user->st_user_base_info.uc_device_id, pst_hmac_user->st_user_base_info.uc_vap_id);

    /* 拷贝参数 */
    if (memcpy_s(pst_event->auc_event_data, OAL_SIZEOF(dmac_ctx_asoc_set_reg_stru),
                 (oal_void *)&st_asoc_set_reg_param, OAL_SIZEOF(dmac_ctx_asoc_set_reg_stru)) != EOK) {
        OAM_ERROR_LOG0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ASSOC,
                       "hmac_mgmt_update_assoc_user_qos_table::memcpy fail!");
        frw_event_free_m(pst_event_mem);
        return;
    }

    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
    frw_event_free_m(pst_event_mem);
}


#ifdef _PRE_WLAN_FEATURE_TXBF
oal_void hmac_mgmt_update_11ntxbf_cap(oal_uint8 *puc_payload,
                                      hmac_user_stru *pst_hmac_user)
{
    mac_11ntxbf_vendor_ie_stru *pst_vendor_ie;
    if (puc_payload == OAL_PTR_NULL) {
        return;
    }

    /* 检测到vendor ie */
    pst_vendor_ie = (mac_11ntxbf_vendor_ie_stru *)puc_payload;
    if (pst_vendor_ie->uc_len < (OAL_SIZEOF(mac_11ntxbf_vendor_ie_stru) - MAC_IE_HDR_LEN)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_mgmt_update_11ntxbf_cap:invalid vendor ie len[%d]",
                         pst_vendor_ie->uc_len);
        return;
    }

    pst_hmac_user->st_user_base_info.st_cap_info.bit_11ntxbf = pst_vendor_ie->st_11ntxbf.bit_11ntxbf;

    return;
}
#endif /* #ifdef _PRE_WLAN_FEATURE_TXBF */


oal_uint32 hmac_check_bss_cap_info(oal_uint16 us_cap_info, mac_vap_stru *pst_mac_vap)
{
    hmac_vap_stru *pst_hmac_vap;
    oal_uint32 ul_ret;
    wlan_mib_desired_bsstype_enum_uint8 en_bss_type;

    /* 获取CAP INFO里BSS TYPE */
    en_bss_type = mac_get_bss_type(us_cap_info);

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_check_bss_cap_info::pst_hmac_vap null.}");
        return OAL_FAIL;
    }

    /* 比较BSS TYPE是否一致 不一致，如果是STA仍然发起入网，增加兼容性，其它模式则返回不支持 */
    if (en_bss_type != pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11DesiredBSSType) {
        oam_warning_log2(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_check_bss_cap_info::vap_bss_type[%d] is different from asoc_user_bss_type[%d].}",
                         pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11DesiredBSSType, en_bss_type);
    }

    if (pst_hmac_vap->en_wps_active == OAL_TRUE) {
        return OAL_TRUE;
    }

    /* 比较CAP INFO中privacy位，检查是否加密，加密不一致，返回失败 */
    ul_ret = mac_check_mac_privacy(us_cap_info, (oal_uint8 *)pst_mac_vap);
    if (ul_ret != OAL_TRUE) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_check_bss_cap_info::mac privacy capabilities is different.}");
    }

    return OAL_TRUE;
}


oal_void hmac_set_user_protocol_mode(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user)
{
    mac_user_ht_hdl_stru *pst_mac_ht_hdl;
    mac_vht_hdl_stru *pst_mac_vht_hdl;
    mac_user_stru *pst_mac_user;

    /* 获取HT和VHT结构体指针 */
    pst_mac_user = &pst_hmac_user->st_user_base_info;
    pst_mac_vht_hdl = &(pst_mac_user->st_vht_hdl);
    pst_mac_ht_hdl = &(pst_mac_user->st_ht_hdl);

    if (pst_mac_vht_hdl->en_vht_capable == OAL_TRUE) {
        mac_user_set_protocol_mode(pst_mac_user, WLAN_VHT_MODE);
    } else if (pst_mac_ht_hdl->en_ht_capable == OAL_TRUE) {
        mac_user_set_protocol_mode(pst_mac_user, WLAN_HT_MODE);
    } else {
        if (pst_mac_vap->st_channel.en_band == WLAN_BAND_5G) { /* 判断是否是5G */
            mac_user_set_protocol_mode(pst_mac_user, WLAN_LEGACY_11A_MODE);
        } else {
            if (hmac_is_support_11grate(pst_hmac_user->st_op_rates.auc_rs_rates,
                                        pst_hmac_user->st_op_rates.uc_rs_nrates) == OAL_TRUE) {
                mac_user_set_protocol_mode(pst_mac_user, WLAN_LEGACY_11G_MODE);
                if (hmac_is_support_11brate(pst_hmac_user->st_op_rates.auc_rs_rates,
                                            pst_hmac_user->st_op_rates.uc_rs_nrates) == OAL_TRUE) {
                    mac_user_set_protocol_mode(pst_mac_user, WLAN_MIXED_ONE_11G_MODE);
                }
            } else if (hmac_is_support_11brate(pst_hmac_user->st_op_rates.auc_rs_rates,
                                               pst_hmac_user->st_op_rates.uc_rs_nrates) == OAL_TRUE) {
                mac_user_set_protocol_mode(pst_mac_user, WLAN_LEGACY_11B_MODE);
            } else {
                oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_set_user_protocol_mode::set protl fail.}");
            }
        }
    }

    /* 兼容性问题：思科AP 2.4G（11b）和5G(11a)共存时发送的assoc rsp帧携带的速率分别是11g和11b，导致STA创建用户时通知算法失败，
    Autorate失效，DBAC情况下，DBAC无法启动已工作的VAP状态无法恢复的问题 临时方案，建议针对对端速率异常的情况统一分析优化 */
    if (((pst_mac_user->en_protocol_mode == WLAN_LEGACY_11B_MODE) &&
         (pst_mac_vap->en_protocol == WLAN_LEGACY_11A_MODE)) ||
        ((pst_mac_user->en_protocol_mode == WLAN_LEGACY_11G_MODE) &&
         (pst_mac_vap->en_protocol == WLAN_LEGACY_11B_MODE))) {
        mac_user_set_protocol_mode(pst_mac_user, pst_mac_vap->en_protocol);
        if (memcpy_s((oal_void *)&(pst_hmac_user->st_op_rates), OAL_SIZEOF(pst_hmac_user->st_op_rates),
                     (oal_void *)&(pst_mac_vap->st_curr_sup_rates.st_rate), OAL_SIZEOF(mac_rate_stru)) != EOK) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "hmac_set_user_protocol_mode::memcpy fail!");
            return;
        }
    }
}

oal_void hmac_user_init_rates(hmac_user_stru *pst_hmac_user)
{
    memset_s((oal_uint8 *)(&pst_hmac_user->st_op_rates), OAL_SIZEOF(pst_hmac_user->st_op_rates),
             0, OAL_SIZEOF(pst_hmac_user->st_op_rates));
}

oal_uint8 hmac_add_user_rates(hmac_user_stru *pst_hmac_user, oal_uint8 uc_rates_cnt, oal_uint8 *puc_rates)
{
    if (pst_hmac_user->st_op_rates.uc_rs_nrates + uc_rates_cnt > WLAN_MAX_SUPP_RATES) {
        uc_rates_cnt = WLAN_MAX_SUPP_RATES - pst_hmac_user->st_op_rates.uc_rs_nrates;
    }

    if (memcpy_s(&(pst_hmac_user->st_op_rates.auc_rs_rates[pst_hmac_user->st_op_rates.uc_rs_nrates]),
                 WLAN_MAX_SUPP_RATES - pst_hmac_user->st_op_rates.uc_rs_nrates, puc_rates, uc_rates_cnt) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_add_user_rates::memcpy fail!");
        return uc_rates_cnt;
    }

    pst_hmac_user->st_op_rates.uc_rs_nrates += uc_rates_cnt;

    return uc_rates_cnt;
}
#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY

oal_uint32 hmac_mgmt_rx_opmode_notify_frame(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf)
{
    mac_opmode_notify_stru *pst_opmode_notify = OAL_PTR_NULL;
    oal_uint8 uc_mgmt_frm_type;
    oal_uint8 *puc_frame_payload;
    mac_ieee80211_frame_stru *pst_mac_header;
    frw_event_mem_stru *pst_event_mem;
    frw_event_stru *pst_event;
    oal_uint8 uc_power_save;
    oal_uint16 *pus_user_id;
    mac_user_stru *pst_mac_user;
    dmac_rx_ctl_stru *pst_rx_ctrl;
    oal_uint16 us_user_idx = 0;
    oal_uint8 auc_sta_addr[WLAN_MAC_ADDR_LEN];
    oal_uint8 *puc_data;
    oal_uint32 ul_ret;

    if ((pst_hmac_vap == OAL_PTR_NULL) || (pst_netbuf == OAL_PTR_NULL)) {
        oam_error_log2(0, OAM_SF_ANY,
                       "{hmac_mgmt_rx_opmode_notify_frame::pst_hmac_vap = [%x], pst_netbuf = [%x]!}\r\n",
                       (uintptr_t)pst_hmac_vap, (uintptr_t)pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if ((mac_mib_get_VHTOptionImplemented(&pst_hmac_vap->st_vap_base_info) == OAL_FALSE) ||
        (mac_mib_get_OperatingModeNotificationImplemented(&pst_hmac_vap->st_vap_base_info) == OAL_FALSE)) {
        return OAL_SUCC;
    }

    pst_rx_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    mac_get_address2((oal_uint8 *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr, auc_sta_addr, WLAN_MAC_ADDR_LEN);

    ul_ret = mac_vap_find_user_by_macaddr(&(pst_hmac_vap->st_vap_base_info),
        auc_sta_addr, WLAN_MAC_ADDR_LEN, &us_user_idx);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SMPS,
                         "{hmac_mgmt_rx_opmode_notify_frame::mac_vap_find_user_by_macaddr failed[%d].}", ul_ret);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_user = (mac_user_stru *)mac_res_get_mac_user(us_user_idx);
    if (pst_mac_user == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SMPS,
                       "{hmac_mgmt_rx_opmode_notify_frame::pst_mac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取帧体指针 */
    puc_data = (oal_uint8 *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;

    uc_mgmt_frm_type = mac_get_frame_sub_type(puc_data);

    /* 是否需要处理Power Management bit位 */
    pst_mac_header = (mac_ieee80211_frame_stru *)puc_data;
    uc_power_save = (oal_uint8)pst_mac_header->st_frame_control.bit_power_mgmt;

    /* 如果节能位开启(bit_power_mgmt == 1),抛事件到DMAC，处理用户节能信息 */
    if ((uc_power_save == OAL_TRUE) && (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP)) {
        /* 申请事件内存 */
        pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(oal_uint16));
        if (pst_event_mem == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_mgmt_rx_opmode_notify_frame::frw_event_alloc fail");
            return OAL_FAIL;
        }

        /* 填写事件 */
        pst_event = (frw_event_stru *)pst_event_mem->puc_data;

        frw_event_hdr_init(&(pst_event->st_event_hdr),
                           FRW_EVENT_TYPE_WLAN_CTX,
                           DMAC_WLAN_CTX_EVENT_SUB_TYPE_PSM_OPMODE_NOTIFY,
                           OAL_SIZEOF(oal_uint16),
                           FRW_EVENT_PIPELINE_STAGE_1,
                           pst_mac_user->uc_chip_id,
                           pst_mac_user->uc_device_id,
                           pst_mac_user->uc_vap_id);

        pus_user_id = (oal_uint16 *)pst_event->auc_event_data;

        *pus_user_id = us_user_idx;

        /* 分发事件 */
        frw_event_dispatch_event(pst_event_mem);
        frw_event_free_m(pst_event_mem);
    }

    /****************************************************/
    /* OperatingModeNotification Frame - Frame Body */
    /* ------------------------------------------------- */
    /* |   Category   |   Action   |   OperatingMode   | */
    /* ------------------------------------------------- */
    /* |   1          |   1        |   1               | */
    /* ------------------------------------------------- */
    /****************************************************/
    /* 获取payload的指针 */
    puc_frame_payload = (oal_uint8 *)puc_data + MAC_80211_FRAME_LEN;
    pst_opmode_notify = (mac_opmode_notify_stru *)(puc_frame_payload + MAC_ACTION_OFFSET_ACTION + 1);

    ul_ret = mac_ie_proc_opmode_field(&(pst_hmac_vap->st_vap_base_info), pst_mac_user, pst_opmode_notify);
    if (oal_unlikely(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_user->uc_vap_id, OAM_SF_CFG,
                         "{hmac_mgmt_rx_opmode_notify_frame::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* opmode息同步dmac */
    ul_ret = hmac_config_update_opmode_event(&(pst_hmac_vap->st_vap_base_info), pst_mac_user, uc_mgmt_frm_type);
    if (oal_unlikely(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_user->uc_vap_id, OAM_SF_CFG,
                         "{hmac_mgmt_rx_opmode_notify_frame::hmac_config_send_event failed[%d].}", ul_ret);
    }
    return ul_ret;
}
#endif

oal_void hmac_send_mgmt_to_host(hmac_vap_stru *pst_hmac_vap,
                                oal_netbuf_stru *puc_buf,
                                oal_uint16 us_len,
                                oal_int l_freq)
{
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL;
    frw_event_stru *pst_event = OAL_PTR_NULL;
    hmac_rx_mgmt_event_stru *pst_mgmt_frame = OAL_PTR_NULL;
    mac_rx_ctl_stru *pst_rx_info = OAL_PTR_NULL;
    mac_ieee80211_frame_stru *pst_frame_hdr = OAL_PTR_NULL;
    oal_uint8 uc_hal_vap_id;
    oal_uint8 *pst_mgmt_data = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    oal_int32 l_ret;
    oal_uint32 ret;

    mac_device_stru *pst_mac_device = mac_res_get_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        oam_warning_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_send_mgmt_to_host::dev null}");
        return;
    }

    /* 抛关联一个新的sta完成事件到WAL */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(hmac_rx_mgmt_event_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_send_mgmt_to_host::event null.}");
        return;
    }

    pst_rx_info = (mac_rx_ctl_stru *)oal_netbuf_cb(puc_buf);

    pst_mgmt_data = (oal_uint8 *)oal_memalloc(us_len);
    if (pst_mgmt_data == OAL_PTR_NULL) {
        frw_event_free_m(pst_event_mem);
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_send_mgmt_to_host::data null}");
        return;
    }
    l_ret = memcpy_s(pst_mgmt_data, us_len, (oal_uint8 *)pst_rx_info->pul_mac_hdr_start_addr, us_len);

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    frw_event_hdr_init(&(pst_event->st_event_hdr), FRW_EVENT_TYPE_HOST_CTX, HMAC_HOST_CTX_EVENT_SUB_TYPE_RX_MGMT,
                       OAL_SIZEOF(hmac_rx_mgmt_event_stru), FRW_EVENT_PIPELINE_STAGE_0, pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id, pst_mac_vap->uc_vap_id);

    /* 填写上报管理帧数据 */
    pst_mgmt_frame = (hmac_rx_mgmt_event_stru *)(pst_event->auc_event_data);
    pst_mgmt_frame->puc_buf = (oal_uint8 *)pst_mgmt_data;
    pst_mgmt_frame->us_len = us_len;
    pst_mgmt_frame->l_freq = l_freq;
    oal_netbuf_len(puc_buf) = us_len;
    l_ret += memcpy_s(pst_mgmt_frame->ac_name, OAL_IF_NAME_SIZE, pst_hmac_vap->pst_net_device->name, OAL_IF_NAME_SIZE);
    pst_frame_hdr = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(pst_rx_info);
    if (!IS_LEGACY_VAP(pst_mac_vap)) {
        /* 仅针对P2P设备做处理。P2P vap 存在一个vap 对应多个hal_vap 情况，非P2P vap 不存在一个vap 对应多个hal_vap 情况 */
        /* 对比接收到的管理帧vap_id 是否和vap 中hal_vap_id 相同 */
        /* 从管理帧cb字段中的hal vap id 的相应信息查找对应的net dev 指针 */
        uc_hal_vap_id = MAC_GET_RX_CB_HAL_VAP_IDX((mac_rx_ctl_stru *)oal_netbuf_cb(puc_buf));
        if (oal_compare_mac_addr(pst_frame_hdr->auc_address1,
                                 pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID) == 0) {
            /* 第二个net dev槽 */
            l_ret += memcpy_s(pst_mgmt_frame->ac_name, OAL_IF_NAME_SIZE,
                              pst_hmac_vap->pst_p2p0_net_device->name, OAL_IF_NAME_SIZE);
        } else if (oal_compare_mac_addr(pst_frame_hdr->auc_address1,
                                        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID) == 0) {
            l_ret += memcpy_s(pst_mgmt_frame->ac_name, OAL_IF_NAME_SIZE,
                              pst_hmac_vap->pst_net_device->name, OAL_IF_NAME_SIZE);
        } else if ((uc_hal_vap_id == MAC_RX_HAL_VAP_ID) &&
                   (pst_frame_hdr->st_frame_control.bit_sub_type == WLAN_PROBE_REQ) &&
                   (IS_P2P_CL(pst_mac_vap) || IS_P2P_DEV(pst_mac_vap))) {
            frw_event_free_m(pst_event_mem);
            oal_free(pst_mgmt_data);
            return;
        } else {
            frw_event_free_m(pst_event_mem);
            oal_free(pst_mgmt_data);
            return;
        }
    }
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "hmac_send_mgmt_to_host::memcpy fail!");
        frw_event_free_m(pst_event_mem);
        oal_free(pst_mgmt_data);
        return;
    }

    /* 分发事件 */
    ret = frw_event_dispatch_event(pst_event_mem);
    if (ret != OAL_SUCC) {
        oal_free(pst_mgmt_data);
    }

    frw_event_free_m(pst_event_mem);
}


OAL_STATIC oal_void hmac_wpas_tx_mgmt_update_cb(mac_vap_stru *pst_mac_vap, oal_netbuf_stru *pst_netbuf)
{
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL;
    oal_uint8  auc_dest_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint8 *puc_dest_mac_addr = auc_dest_mac_addr;
    oal_uint16 us_user_idx = MAC_INVALID_USER_ID;
    oal_uint32 ul_ret;

    mac_get_address1((oal_uint8 *)oal_netbuf_header(pst_netbuf), puc_dest_mac_addr, WLAN_MAC_ADDR_LEN);
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_tx_ctl->uc_netbuf_num = 1;

    /* 管理帧可能发给已关联的用户，也可能发给未关联的用户。已关联的用户可以找到，未关联的用户找不到。不用判断返回值 */
    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, puc_dest_mac_addr, WLAN_MAC_ADDR_LEN, &us_user_idx);
    if (ul_ret == OAL_SUCC) {
        pst_tx_ctl->us_tx_user_idx = us_user_idx;
    }

    return;
}

oal_uint32 hmac_wpas_mgmt_tx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_netbuf_stru *pst_netbuf_mgmt_tx = OAL_PTR_NULL;
    oal_uint32 ul_ret;
    mac_device_stru *pst_mac_device = OAL_PTR_NULL;

    if (puc_param == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_netbuf_mgmt_tx = *((oal_netbuf_stru **)puc_param);
    if (pst_netbuf_mgmt_tx == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_mac_vap == OAL_PTR_NULL) {
        oal_netbuf_free(pst_netbuf_mgmt_tx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        oal_netbuf_free(pst_netbuf_mgmt_tx);

        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                       "{hmac_wpas_mgmt_tx::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_wpas_tx_mgmt_update_cb(pst_mac_vap, pst_netbuf_mgmt_tx);

    /* Buffer this frame in the Memory Queue for transmission */
    ul_ret = hmac_tx_mgmt_send_event(pst_mac_vap, pst_netbuf_mgmt_tx, oal_netbuf_len(pst_netbuf_mgmt_tx));
    if (ul_ret != OAL_SUCC) {
        oal_netbuf_free(pst_netbuf_mgmt_tx);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P, "{hmac_wpas_mgmt_tx::hmac_tx_mgmt_send_event failed[%d].}",
                         ul_ret);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_void hmac_rx_mgmt_send_to_host(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf)
{
    mac_rx_ctl_stru *pst_rx_info;
    oal_int l_freq;

    pst_rx_info = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0))
    l_freq = oal_ieee80211_channel_to_frequency(pst_rx_info->uc_channel_number,
                                                /* 判断信道号是否大于14，大于则返回5G带宽，否则返回2G带宽 */
                                                (pst_rx_info->uc_channel_number > 14) ?
                                                NL80211_BAND_5GHZ : NL80211_BAND_2GHZ);
#else
    l_freq = oal_ieee80211_channel_to_frequency(pst_rx_info->uc_channel_number,
                                                /* 判断信道号是否大于14，大于则返回5G带宽，否则返回2G带宽 */
                                                (pst_rx_info->uc_channel_number > 14) ?
                                                IEEE80211_BAND_5GHZ : IEEE80211_BAND_2GHZ);
#endif
    hmac_send_mgmt_to_host(pst_hmac_vap, pst_netbuf, pst_rx_info->us_frame_len, l_freq);
}


oal_uint32 hmac_mgmt_tx_event_status(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_crx_mgmt_tx_status_stru *pst_mgmt_tx_status_param = OAL_PTR_NULL;
    dmac_crx_mgmt_tx_status_stru *pst_mgmt_tx_status_param_2wal = OAL_PTR_NULL;
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL;
    frw_event_stru *pst_event = OAL_PTR_NULL;

    oal_uint32 ul_ret;

    if (puc_param == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{hmac_mgmt_tx_event_status::puc_param is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mgmt_tx_status_param = (dmac_crx_mgmt_tx_status_stru *)puc_param;

    oam_warning_log3(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                     "{hmac_mgmt_tx_event_status::dmac tx mgmt status report.useridx[%d],tx status[%d], frame_id[%d]}",
                     pst_mgmt_tx_status_param->uc_user_idx,
                     pst_mgmt_tx_status_param->uc_dscr_status,
                     pst_mgmt_tx_status_param->mgmt_frame_id);

    /* 抛扫描完成事件到WAL */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(dmac_crx_mgmt_tx_status_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_P2P, "{hmac_mgmt_tx_event_status::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    frw_event_hdr_init(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_MGMT_TX_STATUS,
                       OAL_SIZEOF(dmac_crx_mgmt_tx_status_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    pst_mgmt_tx_status_param_2wal = (dmac_crx_mgmt_tx_status_stru *)(pst_event->auc_event_data);
    pst_mgmt_tx_status_param_2wal->uc_dscr_status = pst_mgmt_tx_status_param->uc_dscr_status;
    pst_mgmt_tx_status_param_2wal->mgmt_frame_id = pst_mgmt_tx_status_param->mgmt_frame_id;

    /* 分发事件 */
    ul_ret = frw_event_dispatch_event(pst_event_mem);
    frw_event_free_m(pst_event_mem);

    return ul_ret;
}


oal_void hmac_vap_set_user_avail_rates(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user)
{
    mac_user_stru *pst_mac_user;
    mac_curr_rateset_stru *pst_mac_vap_rate;
    mac_rate_stru *pst_mac_user_rate;
    mac_rate_stru st_avail_op_rates;
    oal_uint8 uc_mac_vap_rate_num;
    oal_uint8 uc_mac_user_rate_num;
    oal_uint8 uc_vap_rate_idx;
    oal_uint8 uc_user_rate_idx;
    oal_uint8 uc_user_avail_rate_idx = 0;

    /* 获取VAP和USER速率的结构体指针 */
    pst_mac_user = &(pst_hmac_user->st_user_base_info);
    pst_mac_vap_rate = &(pst_mac_vap->st_curr_sup_rates);
    pst_mac_user_rate = &(pst_hmac_user->st_op_rates);
    memset_s ((oal_uint8 *)(&st_avail_op_rates), OAL_SIZEOF(mac_rate_stru), 0, OAL_SIZEOF(mac_rate_stru));

    uc_mac_vap_rate_num = pst_mac_vap_rate->st_rate.uc_rs_nrates;
    uc_mac_user_rate_num = pst_mac_user_rate->uc_rs_nrates;

    for (uc_vap_rate_idx = 0; uc_vap_rate_idx < uc_mac_vap_rate_num; uc_vap_rate_idx++) {
        for (uc_user_rate_idx = 0; uc_user_rate_idx < uc_mac_user_rate_num; uc_user_rate_idx++) {
            if ((pst_mac_vap_rate->st_rate.ast_rs_rates[uc_vap_rate_idx].uc_mac_rate > 0) &&
                ((pst_mac_vap_rate->st_rate.ast_rs_rates[uc_vap_rate_idx].uc_mac_rate & 0x7F) ==
                 (pst_mac_user_rate->auc_rs_rates[uc_user_rate_idx] & 0x7F))) {
                st_avail_op_rates.auc_rs_rates[uc_user_avail_rate_idx] =
                    (pst_mac_user_rate->auc_rs_rates[uc_user_rate_idx] & 0x7F);
                uc_user_avail_rate_idx++;
                st_avail_op_rates.uc_rs_nrates++;
            }
        }
    }

    mac_user_set_avail_op_rates(pst_mac_user, st_avail_op_rates.uc_rs_nrates, st_avail_op_rates.auc_rs_rates);
    if (st_avail_op_rates.uc_rs_nrates == 0) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_vap_set_user_avail_rates::avail_op_rates_num=0.}");
    }
}

oal_uint32 hmac_proc_ht_cap_ie(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, oal_uint8 *puc_ht_cap_ie)
{
    oal_uint8 uc_mcs_bmp_index;
    oal_uint16 us_offset;
    mac_user_ht_hdl_stru *pst_ht_hdl = OAL_PTR_NULL;
    oal_uint16 us_tmp_info_elem;
    oal_uint16 us_tmp_txbf_low;
    oal_uint16 us_ht_cap_info;
    oal_uint32 ul_tmp_txbf_elem;

    if ((pst_mac_vap == OAL_PTR_NULL) || (pst_mac_user == OAL_PTR_NULL) || (puc_ht_cap_ie == OAL_PTR_NULL)) {
        oam_warning_log3(0, OAM_SF_ROAM,
                         "{hmac_proc_ht_cap_ie::PARAM NULL! vap=[0x%X],user=[0x%X],cap_ie=[0x%X].}",
                         (uintptr_t)pst_mac_vap, (uintptr_t)pst_mac_user, (uintptr_t)puc_ht_cap_ie);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 至少支持11n才进行后续的处理 */
    if (mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap) == OAL_FALSE) {
        return OAL_SUCC;
    }

    mac_user_set_ht_capable(pst_mac_user, OAL_TRUE);

    us_offset = 0;
    pst_ht_hdl = &pst_mac_user->st_ht_hdl;

    /* 带有 HT Capability Element 的 AP，标示它具有HT capable. */
    pst_ht_hdl->en_ht_capable = OAL_TRUE;

    us_offset += MAC_IE_HDR_LEN;

    /********************************************/
    /* 解析 HT Capabilities Info Field */
    /********************************************/
    us_ht_cap_info = oal_make_word16(puc_ht_cap_ie[us_offset], puc_ht_cap_ie[us_offset + 1]);

    /* 检查STA所支持的LDPC编码能力 B0，0:不支持，1:支持 */
    pst_ht_hdl->bit_ldpc_coding_cap = (us_ht_cap_info & BIT0);

    /* 提取AP所支持的带宽能力 */
    pst_ht_hdl->bit_supported_channel_width = ((us_ht_cap_info & BIT1) >> 1);

    /* 检查空间复用节能模式 B2~B3 */
    mac_ie_proc_sm_power_save_field (pst_mac_user, (oal_uint8)((us_ht_cap_info & (BIT2 | BIT3)) >> 2));

    /* 提取AP支持Greenfield情况 B4 */
    pst_ht_hdl->bit_ht_green_field = ((us_ht_cap_info & BIT4) >> 4);

    /* 提取AP支持20MHz Short-GI情况 B5 */
    pst_ht_hdl->bit_short_gi_20mhz = ((us_ht_cap_info & BIT5) >> 5);

    /* 提取AP支持40MHz Short-GI情况 B6 */
    pst_ht_hdl->bit_short_gi_40mhz = ((us_ht_cap_info & BIT6) >> 6);

    /* 提取AP支持STBC PPDU情况 B8~B9 */
    pst_ht_hdl->bit_rx_stbc = (oal_uint8)((us_ht_cap_info & (BIT9 | BIT8)) >> 8);

    /* 提取AP 40M上DSSS/CCK的支持情况 B12 */
    pst_ht_hdl->bit_dsss_cck_mode_40mhz = ((us_ht_cap_info & BIT12) >> 12);

    /* 提取AP L-SIG TXOP 保护的支持情况 B15 */
    pst_ht_hdl->bit_lsig_txop_protection = ((us_ht_cap_info & BIT15) >> 15);

    us_offset += MAC_HT_CAPINFO_LEN;

    /********************************************/
    /* 解析 A-MPDU Parameters Field */
    /********************************************/
    /* 提取 Maximum Rx A-MPDU factor (B1 - B0) */
    pst_ht_hdl->uc_max_rx_ampdu_factor = (puc_ht_cap_ie[us_offset] & 0x03);

    /* 提取 Minmum Rx A-MPDU factor (B3 - B2) */
    pst_ht_hdl->uc_min_mpdu_start_spacing = (puc_ht_cap_ie[us_offset] >> 2) & 0x07;

    us_offset += MAC_HT_AMPDU_PARAMS_LEN;

    /********************************************/
    /* 解析 Supported MCS Set Field */
    /********************************************/
    for (uc_mcs_bmp_index = 0; uc_mcs_bmp_index < WLAN_HT_MCS_BITMASK_LEN; uc_mcs_bmp_index++) {
        pst_ht_hdl->uc_rx_mcs_bitmask[uc_mcs_bmp_index] =
           (*(oal_uint8 *)(puc_ht_cap_ie + us_offset + uc_mcs_bmp_index));
    }

    pst_ht_hdl->uc_rx_mcs_bitmask[WLAN_HT_MCS_BITMASK_LEN - 1] &= 0x1F;

    us_offset += MAC_HT_SUP_MCS_SET_LEN;

    /********************************************/
    /* 解析 HT Extended Capabilities Info Field */
    /********************************************/
    us_ht_cap_info = oal_make_word16(puc_ht_cap_ie[us_offset], puc_ht_cap_ie[us_offset + 1]);

    /* 提取 HTC support Information（uc_htc_support是us_ht_cap_info的第10bit位） */
    pst_ht_hdl->uc_htc_support = ((us_ht_cap_info & BIT10) >> 10);

    us_offset += MAC_HT_EXT_CAP_LEN;

    /********************************************/
    /* 解析 Tx Beamforming Field */
    /********************************************/
    us_tmp_info_elem = oal_make_word16(puc_ht_cap_ie[us_offset], puc_ht_cap_ie[us_offset + 1]);
    /* 第2byte和第3byte拼接为txbf字段低16bit */
    us_tmp_txbf_low = oal_make_word16(puc_ht_cap_ie[us_offset + 2], puc_ht_cap_ie[us_offset + 3]);
    ul_tmp_txbf_elem = oal_make_word32(us_tmp_info_elem, us_tmp_txbf_low);
    pst_ht_hdl->bit_imbf_receive_cap = (ul_tmp_txbf_elem & BIT0);
    pst_ht_hdl->bit_receive_staggered_sounding_cap = ((ul_tmp_txbf_elem & BIT1) >> 1);
    /* bit_transmit_staggered_sounding_cap是第2bit */
    pst_ht_hdl->bit_transmit_staggered_sounding_cap = ((ul_tmp_txbf_elem & BIT2) >> 2);
    pst_ht_hdl->bit_receive_ndp_cap = ((ul_tmp_txbf_elem & BIT3) >> 3); /* bit_receive_ndp_cap是第3bit */
    pst_ht_hdl->bit_transmit_ndp_cap = ((ul_tmp_txbf_elem & BIT4) >> 4); /* bit_transmit_ndp_cap是第4bit */
    pst_ht_hdl->bit_imbf_cap = ((ul_tmp_txbf_elem & BIT5) >> 5); /* bit_imbf_cap是第5bit */
    pst_ht_hdl->bit_calibration = ((ul_tmp_txbf_elem & 0x000000C0) >> 6); /* bit_calibration是第6bit */
    pst_ht_hdl->bit_exp_csi_txbf_cap = ((ul_tmp_txbf_elem & BIT8) >> 8); /* bit_exp_csi_txbf_cap是第8bit */
    pst_ht_hdl->bit_exp_noncomp_txbf_cap = ((ul_tmp_txbf_elem & BIT9) >> 9); /* bit_exp_noncomp_txbf_cap是第9bit */
    pst_ht_hdl->bit_exp_comp_txbf_cap = ((ul_tmp_txbf_elem & BIT10) >> 10); /* bit_exp_comp_txbf_cap是第10bit */
    pst_ht_hdl->bit_exp_csi_feedback = ((ul_tmp_txbf_elem & 0x00001800) >> 11); /* bit_exp_csi_feedback是第11bit */
    /* bit_exp_noncomp_feedback是第13bit */
    pst_ht_hdl->bit_exp_noncomp_feedback = ((ul_tmp_txbf_elem & 0x00006000) >> 13);
    pst_ht_hdl->bit_exp_comp_feedback = ((ul_tmp_txbf_elem & 0x0001C000) >> 15); /* bit_exp_comp_feedback是第15bit */
    pst_ht_hdl->bit_min_grouping = ((ul_tmp_txbf_elem & 0x00060000) >> 17); /* bit_min_grouping是第17bit */
    pst_ht_hdl->bit_csi_bfer_ant_number = ((ul_tmp_txbf_elem & 0x001C0000) >> 19); /* bit_csi_bfer_ant_number是第19bit */
    /* bit_noncomp_bfer_ant_number是第21bit */
    pst_ht_hdl->bit_noncomp_bfer_ant_number = ((ul_tmp_txbf_elem & 0x00600000) >> 21);
    /* bit_comp_bfer_ant_number是第23bit */
    pst_ht_hdl->bit_comp_bfer_ant_number = ((ul_tmp_txbf_elem & 0x01C00000) >> 23);
    pst_ht_hdl->bit_csi_bfee_max_rows = ((ul_tmp_txbf_elem & 0x06000000) >> 25); /* bit_csi_bfee_max_rows是第25bit */
    pst_ht_hdl->bit_channel_est_cap = ((ul_tmp_txbf_elem & 0x18000000) >> 27); /* bit_channel_est_cap是第27bit */

    return OAL_SUCC;
}


oal_void hmac_get_max_mpdu_len_by_vht_cap_ie(
    mac_vht_hdl_stru *pst_mac_vht_hdl, oal_uint32 ul_vht_cap_field)
{
    pst_mac_vht_hdl->bit_max_mpdu_length = (ul_vht_cap_field & (BIT1 | BIT0));
    if (pst_mac_vht_hdl->bit_max_mpdu_length == 0) {
        pst_mac_vht_hdl->us_max_mpdu_length = MAX_MPDU_LEN_NO_VHT_CAP;
    } else if (pst_mac_vht_hdl->bit_max_mpdu_length == 1) {
        pst_mac_vht_hdl->us_max_mpdu_length = MAX_MPDU_LEN_LOW_VHT_CAP;
    /* 判断max_mpdu_length是否为2，是则返回MAX_MPDU_LEN_HIGH_VHT_CAP值11454 */
    } else if (pst_mac_vht_hdl->bit_max_mpdu_length == 2) {
        pst_mac_vht_hdl->us_max_mpdu_length = MAX_MPDU_LEN_HIGH_VHT_CAP;
    }
}


oal_uint32 hmac_proc_vht_cap_ie(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user,
                                oal_uint8 *puc_vht_cap_ie)
{
    mac_vht_hdl_stru *pst_mac_vht_hdl = OAL_PTR_NULL;
    mac_vht_hdl_stru st_mac_vht_hdl;
    mac_user_stru *pst_mac_user = OAL_PTR_NULL;
    oal_uint16 us_vht_cap_filed_low;
    oal_uint16 us_vht_cap_filed_high;
    oal_uint32 ul_vht_cap_field;
    oal_uint16 us_rx_mcs_map;
    oal_uint16 us_tx_mcs_map;
    oal_uint16 us_rx_highest_supp_loggi_data;
    oal_uint16 us_tx_highest_supp_loggi_data;
    oal_uint16 us_msg_idx = 0;
    oal_int32 l_ret;

    /* 解析vht cap IE */
    if ((pst_mac_vap == OAL_PTR_NULL) || (pst_hmac_user == OAL_PTR_NULL) || (puc_vht_cap_ie == OAL_PTR_NULL)) {
        oam_error_log3(0, OAM_SF_ANY, "{hmac_proc_vht_cap_ie::paranull,mac_vap[0x%x],hmac_user[0x%x],vht_cap_ie[0x%x]}",
                       (uintptr_t)pst_mac_vap, (uintptr_t)pst_hmac_user, (uintptr_t)puc_vht_cap_ie);

        return OAL_ERR_CODE_PTR_NULL;
    }

    if (puc_vht_cap_ie[1] < MAC_VHT_CAP_IE_LEN) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_proc_vht_cap_ie::invalid vht cap ie len[%d].}", puc_vht_cap_ie[1]);
        return OAL_FAIL;
    }

    pst_mac_user = &pst_hmac_user->st_user_base_info;

    /* 支持11ac，才进行后续的处理 */
    if (mac_mib_get_VHTOptionImplemented(pst_mac_vap) == OAL_FALSE) {
        return OAL_SUCC;
    }

    pst_mac_vht_hdl = &st_mac_vht_hdl;
    mac_user_get_vht_hdl(pst_mac_user, pst_mac_vht_hdl);

    /* 进入此函数代表user支持11ac */
    pst_mac_vht_hdl->en_vht_capable = OAL_TRUE;

#ifdef _PRE_WLAN_FEATURE_11AC2G
    /* 定制化实现如果不支持11ac2g模式，则关掉vht cap */
    if ((pst_mac_vap->st_cap_flag.bit_11ac2g == OAL_FALSE) && (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G)) {
        pst_mac_vht_hdl->en_vht_capable = OAL_FALSE;
    }
#endif

    us_msg_idx += MAC_IE_HDR_LEN;

    /* 解析VHT capablities info field */
    us_vht_cap_filed_low = oal_make_word16(puc_vht_cap_ie[us_msg_idx], puc_vht_cap_ie[us_msg_idx + 1]);
    /* 第2byte和第3byte拼接为vht_cap_filed字段高16bit */
    us_vht_cap_filed_high = oal_make_word16(puc_vht_cap_ie[us_msg_idx + 2], puc_vht_cap_ie[us_msg_idx + 3]);
    ul_vht_cap_field = oal_make_word32(us_vht_cap_filed_low, us_vht_cap_filed_high);

    /* 解析max_mpdu_length 参见11ac协议 Table 8-183u */
    hmac_get_max_mpdu_len_by_vht_cap_ie(pst_mac_vht_hdl, ul_vht_cap_field);
    /* 解析supported_channel_width */
    pst_mac_vht_hdl->bit_supported_channel_width = (ul_vht_cap_field & (BIT3 | BIT2));

    /* 解析rx_ldpc */
    pst_mac_vht_hdl->bit_rx_ldpc = ((ul_vht_cap_field & BIT4) >> 4); /* bit_rx_ldpc是第4bit */

    /* 解析short_gi_80mhz和short_gi_160mhz支持情况 */
    pst_mac_vht_hdl->bit_short_gi_80mhz = ((ul_vht_cap_field & BIT5) >> 5); /* bit_short_gi_80mhz是第5bit */
    pst_mac_vht_hdl->bit_short_gi_80mhz &=
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTShortGIOptionIn80Implemented;

    pst_mac_vht_hdl->bit_short_gi_160mhz = ((ul_vht_cap_field & BIT6) >> 6); /* bit_short_gi_160mhz是第6bit */
    pst_mac_vht_hdl->bit_short_gi_160mhz &=
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTShortGIOptionIn160and80p80Implemented;

    /* 解析tx_stbc 和rx_stbc */
    pst_mac_vht_hdl->bit_tx_stbc = ((ul_vht_cap_field & BIT7) >> 7); /* bit_tx_stbc是第7bit */
    /* bit_rx_stbc是8bit、9bit、10bit拼接成 */
    pst_mac_vht_hdl->bit_rx_stbc = ((ul_vht_cap_field & (BIT10 | BIT9 | BIT8)) >> 8);

    /* 解析su_beamformer_cap和su_beamformee_cap */
    pst_mac_vht_hdl->bit_su_beamformer_cap = ((ul_vht_cap_field & BIT11) >> 11); /* bit_su_beamformer_cap是第11bit */
    pst_mac_vht_hdl->bit_su_beamformee_cap = ((ul_vht_cap_field & BIT12) >> 12); /* bit_su_beamformee_cap是第12bit */

    /* 解析num_bf_ant_supported（bit_num_bf_ant_supported是13bit、14bit、15bit拼接成） */
    pst_mac_vht_hdl->bit_num_bf_ant_supported = ((ul_vht_cap_field & (BIT15 | BIT14 | BIT13)) >> 13);

    pst_mac_user->uc_avail_bf_num_spatial_stream = pst_mac_vht_hdl->bit_num_bf_ant_supported;

    /* 解析num_sounding_dim（bit_num_sounding_dim是16bit、17bit、18bit拼接成） */
    pst_mac_vht_hdl->bit_num_sounding_dim = ((ul_vht_cap_field & (BIT18 | BIT17 | BIT16)) >> 16);

    /* 解析mu_beamformer_cap和mu_beamformee_cap */
    pst_mac_vht_hdl->bit_mu_beamformer_cap = ((ul_vht_cap_field & BIT19) >> 19); /* bit_mu_beamformer_cap是第19bit */
    pst_mac_vht_hdl->bit_mu_beamformee_cap = ((ul_vht_cap_field & BIT20) >> 20); /* bit_mu_beamformee_cap是第20bit */

    /* 解析vht_txop_ps（bit_vht_txop_ps是第21bit） */
    pst_mac_vht_hdl->bit_vht_txop_ps = ((ul_vht_cap_field & BIT21) >> 21);
    if (pst_mac_vht_hdl->bit_vht_txop_ps) {
        pst_mac_vap->st_cap_flag.bit_txop_ps = 0x1;
    }

    /* 解析htc_vht_capable（bit_htc_vht_capable是第22bit） */
    pst_mac_vht_hdl->bit_htc_vht_capable = ((ul_vht_cap_field & BIT22) >> 22);

    /* 解析max_ampdu_len_exp（bit_max_ampdu_len_exp是23bit、24bit、25bit拼接成） */
    pst_mac_vht_hdl->bit_max_ampdu_len_exp = ((ul_vht_cap_field & (BIT25 | BIT24 | BIT23)) >> 23);

    /* 解析vht_link_adaptation（bit_vht_link_adaptation是26bit与27bit拼接成） */
    pst_mac_vht_hdl->bit_vht_link_adaptation = ((ul_vht_cap_field & (BIT27 | BIT26)) >> 26);

    /* 解析rx_ant_pattern（bit_rx_ant_pattern是第28bit） */
    pst_mac_vht_hdl->bit_rx_ant_pattern = ((ul_vht_cap_field & BIT28) >> 28);

    /* 解析tx_ant_pattern（bit_tx_ant_pattern是第29bit） */
    pst_mac_vht_hdl->bit_tx_ant_pattern = ((ul_vht_cap_field & BIT29) >> 29);

    us_msg_idx += MAC_VHT_CAP_INFO_FIELD_LEN;

    /* 解析VHT Supported MCS Set field */
    us_rx_mcs_map = oal_make_word16(puc_vht_cap_ie[us_msg_idx], puc_vht_cap_ie[us_msg_idx + 1]);
    l_ret = memcpy_s(&(pst_mac_vht_hdl->st_rx_max_mcs_map), OAL_SIZEOF(mac_rx_max_mcs_map_stru),
        &us_rx_mcs_map, OAL_SIZEOF(mac_rx_max_mcs_map_stru));

    us_msg_idx += MAC_VHT_CAP_RX_MCS_MAP_FIELD_LEN;

    /* 解析rx_highest_supp_logGi_data */
    us_rx_highest_supp_loggi_data = oal_make_word16(puc_vht_cap_ie[us_msg_idx], puc_vht_cap_ie[us_msg_idx + 1]);
    pst_mac_vht_hdl->bit_rx_highest_rate = us_rx_highest_supp_loggi_data & (0x1FFF);

    us_msg_idx += MAC_VHT_CAP_RX_HIGHEST_DATA_FIELD_LEN;

    /* 解析tx_mcs_map */
    us_tx_mcs_map = oal_make_word16(puc_vht_cap_ie[us_msg_idx], puc_vht_cap_ie[us_msg_idx + 1]);
    l_ret += memcpy_s(&(pst_mac_vht_hdl->st_tx_max_mcs_map), OAL_SIZEOF(mac_tx_max_mcs_map_stru),
        &us_tx_mcs_map, OAL_SIZEOF(mac_tx_max_mcs_map_stru));
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_proc_vht_cap_ie::memcpy fail!");
        return OAL_FAIL;
    }

    us_msg_idx += MAC_VHT_CAP_TX_MCS_MAP_FIELD_LEN;

    /* 解析tx_highest_supp_logGi_data */
    us_tx_highest_supp_loggi_data = oal_make_word16(puc_vht_cap_ie[us_msg_idx], puc_vht_cap_ie[us_msg_idx + 1]);
    pst_mac_vht_hdl->bit_tx_highest_rate = us_tx_highest_supp_loggi_data & (0x1FFF);

    mac_user_set_vht_hdl(pst_mac_user, pst_mac_vht_hdl);
    return OAL_SUCC;
}
oal_module_symbol(hmac_config_send_deauth);
oal_module_symbol(hmac_wpas_mgmt_tx);
