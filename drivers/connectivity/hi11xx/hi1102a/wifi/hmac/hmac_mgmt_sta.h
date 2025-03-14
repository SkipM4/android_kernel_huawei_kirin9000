

#ifndef __HMAC_MGMT_STA_H__
#define __HMAC_MGMT_STA_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "hmac_vap.h"
#include "hmac_mgmt_bss_comm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_MGMT_STA_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HMAC_WMM_QOS_PARAMS_HDR_LEN        8
#define HMAC_WMM_QOSINFO_AND_RESV_LEN      2
#define HMAC_WMM_AC_PARAMS_RECORD_LEN      4
/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
/*****************************************************************************
  5 消息头定义
*****************************************************************************/
/*****************************************************************************
  6 消息定义
*****************************************************************************/
/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
/* 加入请求参数 */
typedef struct {
    mac_bss_dscr_stru   st_bss_dscr;            /* 要加入的bss网络 */
    oal_uint16          us_join_timeout;        /* 加入超时 */
    oal_uint16          us_probe_delay;
}hmac_join_req_stru;

/* 认证请求参数 */
typedef struct {
    oal_uint16                  us_timeout;
    oal_uint8                   auc_resv[2];
}hmac_auth_req_stru;

/* 关联请求参数 */
typedef struct {
    oal_uint16                  us_assoc_timeout;
    oal_uint8                   auc_resv[2];
}hmac_asoc_req_stru;

/* 加入结果 */
typedef struct {
    hmac_mgmt_status_enum_uint8 en_result_code;
    oal_uint8                   auc_resv[3];
}hmac_join_rsp_stru;

/* 认证结果 */
typedef struct {
    oal_uint8                   auc_peer_sta_addr[WLAN_MAC_ADDR_LEN];   /* mesh下peer station的地址 */
    oal_uint16                  us_status_code;         /* 认证结果 */
}hmac_auth_rsp_stru;

/* 去关联原因 */
typedef struct {
    hmac_mgmt_status_enum_uint8  en_disasoc_reason_code;
    oal_uint8                    auc_resv[3];
}hmac_disasoc_rsp_stru;

typedef struct {
    hmac_mgmt_status_enum_uint8  en_result_code;
    oal_uint8                    auc_resv[3];
}hmac_ap_start_rsp_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/
/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32  hmac_sta_wait_asoc_rx(hmac_vap_stru *pst_sta, oal_void *pst_msg);
extern oal_uint32  hmac_sta_auth_timeout(hmac_vap_stru *pst_hmac_sta, oal_void *p_param);
extern oal_uint32  hmac_sta_up_rx_mgmt(hmac_vap_stru *pst_hmac_vap_sta, oal_void *p_param);
extern oal_uint32  hmac_sta_wait_asoc_timeout(hmac_vap_stru *pst_hmac_sta, oal_void *p_param);
extern oal_void    hmac_sta_handle_disassoc_rsp(hmac_vap_stru *pst_hmac_vap, oal_uint16 us_disasoc_reason_code);
#if defined(_PRE_WLAN_FEATURE_HS20) || defined(_PRE_WLAN_FEATURE_P2P)
extern oal_uint32  hmac_sta_not_up_rx_mgmt(hmac_vap_stru *pst_hmac_vap_sta, oal_void *p_param);
#endif
extern wlan_channel_bandwidth_enum_uint8 hmac_sta_get_band(
    wlan_bw_cap_enum_uint8 en_dev_cap, wlan_channel_bandwidth_enum_uint8 en_bss_cap);
oal_void hmac_sta_update_wmm_info(hmac_vap_stru *pst_hmac_vap, mac_user_stru *pst_mac_user, oal_uint8 *puc_wmm_ie);
extern oal_uint32  hmac_sta_up_update_edca_params_machw(
    hmac_vap_stru  *pst_hmac_sta, mac_wmm_set_param_type_enum_uint8 en_wmm_set_param_type);
extern oal_uint32  hmac_sta_set_txopps_partial_aid(mac_vap_stru *pst_mac_vap);
extern oal_void hmac_sta_up_update_edca_params(
                    oal_uint8               *puc_payload,
                    oal_uint16               us_msg_len,
                    oal_uint16               us_info_elem_offset,
                    hmac_vap_stru           *pst_hmac_sta,
                    oal_uint8                uc_frame_sub_type,
                    hmac_user_stru          *pst_hmac_user);
extern oal_uint32 hmac_process_assoc_rsp(
    hmac_vap_stru *pst_hmac_sta, hmac_user_stru *pst_hmac_user,
    oal_uint8 *puc_mac_hdr, oal_uint8 *puc_payload, oal_uint16 us_msg_len);
extern oal_uint8 *hmac_sta_find_ie_in_probe_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_eid, oal_uint16 *pus_index);
extern oal_bool_enum_uint8 hmac_is_ht_mcs_set_valid(oal_uint8 *puc_ht_cap_ie, wlan_channel_band_enum_uint8  en_band);
extern oal_uint32 hmac_get_frame_body_len(oal_netbuf_stru *net_buf);

void hmac_sta_wait_asoc_rx_cfg_rsp_para(hmac_asoc_rsp_stru *asoc_rsp, uint8_t *mac_hdr,
    uint16_t msg_len, hmac_user_stru *hmac_user_ap);
void hmac_process_assoc_rsp_set_user_para(hmac_user_stru *hmac_user,
    hmac_vap_stru *hmac_sta, uint32_t *change);
oal_uint32 hmac_update_vht_opern_ie_sta(mac_vap_stru *pst_mac_vap,
                                        hmac_user_stru *pst_hmac_user,
                                        oal_uint8 *puc_payload,
                                        oal_uint16 us_msg_idx);
oal_void hmac_ie_proc_assoc_user_legacy_rate(
    oal_uint8 *puc_payload, oal_uint16 us_offset, oal_uint16 us_rx_len, hmac_user_stru *pst_hmac_user);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_mgmt_sta.h */
