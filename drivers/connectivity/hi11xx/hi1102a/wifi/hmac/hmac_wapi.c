

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_types.h"
#include "mac_frame.h"
#include "hmac_wapi_sms4.h"
#include "hmac_wapi_wpi.h"
#include "hmac_wapi.h"
#include "mac_data.h"
#include "securec.h"
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_WAPI_C

#ifdef _PRE_WLAN_FEATURE_WAPI

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
OAL_CONST oal_uint8 g_auc_wapi_pn_init[WAPI_PN_LEN] = {
    0x36, 0x5c, 0x36, 0x5c, 0x36, 0x5c, 0x36,
    0x5c, 0x36, 0x5c, 0x36, 0x5c, 0x36, 0x5c, 0x36, 0x5c
};

OAL_CONST oal_uint8 g_auc_wapi_oui[WAPI_IE_OUI_SIZE] = { 0x00, 0x14, 0x72 };
#define OAL_SEQ_CTL_LEN 2 /* 序列控制字段长度 */

/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_bool_enum_uint8 hmac_wapi_is_qos(mac_ieee80211_frame_stru *pst_mac_hdr)
{
    if ((pst_mac_hdr->st_frame_control.bit_type == WLAN_DATA_BASICTYPE)
        && (WLAN_QOS_DATA & pst_mac_hdr->st_frame_control.bit_sub_type)) {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


oal_void hmac_wapi_calc_mic_data(
    mac_ieee80211_frame_stru *pst_mac_hdr, oal_uint8 uc_mac_hdr_len, oal_uint8 uc_keyidx,
    oal_uint8 *puc_payload, oal_uint16 us_pdu_len, oal_uint8 *puc_mic, oal_uint16 us_mic_len)
{
    oal_uint8 us_is_qos;
    oal_uint8 *puc_mic_oringin = OAL_PTR_NULL;
    oal_int32 l_ret;

    memset_s(puc_mic, us_mic_len, 0, us_mic_len);

    puc_mic_oringin = puc_mic;

    /* frame control */
    l_ret = memcpy_s(puc_mic, us_mic_len,
                     (oal_uint8 *)&(pst_mac_hdr->st_frame_control),
                     OAL_SIZEOF(pst_mac_hdr->st_frame_control));
    puc_mic[0] &= ~(BIT4 | BIT5 | BIT6); /* sub type */
    puc_mic[1] &= ~(BIT3 | BIT4 | BIT5); /* retry, pwr Mgmt, more data */
    puc_mic[1] |= BIT6;

    puc_mic += OAL_SIZEOF(pst_mac_hdr->st_frame_control);

    /* addr1 */
    mac_get_address1((oal_uint8 *)pst_mac_hdr, puc_mic, WLAN_MAC_ADDR_LEN);
    puc_mic += OAL_MAC_ADDR_LEN;

    /* addr2 */
    mac_get_address2((oal_uint8 *)pst_mac_hdr, puc_mic, WLAN_MAC_ADDR_LEN);
    puc_mic += OAL_MAC_ADDR_LEN;

    /* 序列控制 */
    memset_s(puc_mic, OAL_SEQ_CTL_LEN, 0, OAL_SEQ_CTL_LEN);
    puc_mic[0] = (oal_uint8)(pst_mac_hdr->bit_frag_num);
    puc_mic += 2; /* 跳过序列控制（2byte） */

    /* addr3 */
    mac_get_address3((oal_uint8 *)pst_mac_hdr, puc_mic, WLAN_MAC_ADDR_LEN);
    puc_mic += OAL_MAC_ADDR_LEN;

    /* 跳过addr4 */
    puc_mic += OAL_MAC_ADDR_LEN;

    /* qos ctrl */
    us_is_qos = hmac_wapi_is_qos(pst_mac_hdr);
    if (us_is_qos == OAL_TRUE) {
        mac_get_qos_ctrl((oal_uint8 *)pst_mac_hdr, puc_mic);
        puc_mic += MAC_QOS_CTL_LEN;
    }

    /* keyidx + reserve总共2个字节 */
    *puc_mic = uc_keyidx;
    puc_mic += 2; /* 跳过keyidx + reserve（2byte） */

    /* 填充pdulen 协议写明大端字节序 */
    *puc_mic = (oal_uint8)((us_pdu_len & 0xff00) >> 8); /* 获取us_pdu_len的8-15位（高8位） */
    *(puc_mic + 1) = (oal_uint8)(us_pdu_len & 0x00ff);
    puc_mic += 2; /* 跳过填充pdulen 协议写明大端字节序的2byte */

    /************填充第2部分*******************/
    puc_mic_oringin += (hmac_wapi_is_qos(pst_mac_hdr) == OAL_TRUE) ? SMS4_MIC_PART1_QOS_LEN : SMS4_MIC_PART1_NO_QOS_LEN;
    l_ret += memcpy_s(puc_mic_oringin, us_pdu_len, puc_payload, us_pdu_len);
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_wapi_calc_mic_data::memcpy fail!");
        return;
    }

    return;
}


oal_uint8 *hmac_wapi_mic_alloc(oal_uint8 is_qos, oal_uint16 us_pdu_len, oal_uint16 *pus_mic_len)
{
    oal_uint16 us_mic_part1_len;
    oal_uint16 us_mic_part2_len;
    oal_uint16 us_mic_len;

    us_mic_part1_len = (is_qos == OAL_TRUE) ? SMS4_MIC_PART1_QOS_LEN : SMS4_MIC_PART1_NO_QOS_LEN;

    /* 按照协议，补齐不足位，16字节对齐 */
    us_mic_part2_len = padding(us_pdu_len, SMS4_PADDING_LEN);

    us_mic_len = us_mic_part1_len + us_mic_part2_len;

    *pus_mic_len = us_mic_len;

    return oal_mem_alloc_m(OAL_MEM_POOL_ID_LOCAL, us_mic_len, OAL_TRUE);
}


oal_void hmac_wapi_mic_free(oal_uint8 *puc_mic)
{
    if (puc_mic != OAL_PTR_NULL) {
        oal_mem_free_m(puc_mic, OAL_TRUE);
        return;
    }

    return;
}


oal_uint8 hmac_wapi_is_keyidx_valid(hmac_wapi_stru *pst_wapi, oal_uint8 uc_keyidx_rx)
{
    if ((pst_wapi->uc_keyidx != uc_keyidx_rx)
        && (pst_wapi->uc_keyupdate_flg != OAL_TRUE)) {
        oam_warning_log3(0, OAM_SF_ANY, "{hmac_wapi_is_keyidx_valid::keyidx==%u, uc_keyidx_rx==%u, update==%u.}",
                         pst_wapi->uc_keyidx, uc_keyidx_rx, pst_wapi->uc_keyupdate_flg);
        wapi_rx_idx_update_err(pst_wapi);
        return OAL_FALSE;
    }

    pst_wapi->uc_keyupdate_flg = OAL_FALSE; /* 更新完成取消标志 */

    /* key没有启用 */
    if (pst_wapi->ast_wapi_key[uc_keyidx_rx].uc_key_en != OAL_TRUE) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_wapi_is_keyidx_valid::keyen==%u.}",
                         pst_wapi->ast_wapi_key[uc_keyidx_rx].uc_key_en);
        wapi_rx_idx_update_err(pst_wapi);
        return OAL_FALSE;
    }

    return OAL_TRUE;
}


oal_bool_enum_uint8 hmac_wapi_is_pn_odd_ucast(oal_uint8 *puc_pn)
{
    return (oal_bool_enum_uint8)(((*puc_pn & BIT0) == 0) ? OAL_FALSE : OAL_TRUE);
}


oal_uint8 hmac_wapi_is_pn_bigger(oal_uint8 *puc_pn, oal_uint8 *puc_pn_rx)
{
    oal_uint8 uc_pnidx;

    for (uc_pnidx = SMS4_PN_LEN - 1; uc_pnidx > 0; uc_pnidx--) {
        if ((puc_pn[uc_pnidx] != puc_pn_rx[uc_pnidx])) {
            if (puc_pn[uc_pnidx] > puc_pn_rx[uc_pnidx]) {
                oam_warning_log2(0, OAM_SF_ANY, "{hmac_wapi_is_pn_bigger::err! puc_pn==%u, puc_pn_rx==%u.}",
                                 puc_pn[uc_pnidx], puc_pn_rx[uc_pnidx]);
                return OAL_FALSE;
            }

            return OAL_TRUE;
        }
    }

    return OAL_TRUE;
}


oal_uint8 hmac_wapi_is_pn_odd_bcast(oal_uint8 *puc_pn)
{
    return OAL_TRUE;
}


oal_void hmac_wapi_pn_update(oal_uint8 *puc_pn, oal_uint8 uc_inc)
{
    oal_uint32 ul_loop;
    oal_uint32 ul_loop_num;
    oal_uint32 ul_overlow; /* 进位 */
    oal_uint32 *pul_pn;

    pul_pn = (oal_uint32 *)puc_pn;
    ul_loop_num = WAPI_PN_LEN / OAL_SIZEOF(oal_uint32);
    ul_overlow = uc_inc;

    for (ul_loop = 0; ul_loop < ul_loop_num; ul_loop++) {
        if (*pul_pn > (*pul_pn + ul_overlow)) {
            *pul_pn += ul_overlow;
            ul_overlow = 1; /* 溢出高位加1 */
        } else {
            *pul_pn += ul_overlow;
            break;
        }
        pul_pn++;
    }

    return;
}


oal_netbuf_stru *hmac_wapi_netbuff_tx_handle(hmac_wapi_stru *pst_wapi, oal_netbuf_stru *pst_buf)
{
    oal_uint32 ul_ret;
    oal_netbuf_stru *pst_netbuf_tmp = OAL_PTR_NULL;  /* 指向需要释放的netbuff */
    oal_netbuf_stru *pst_netbuf_prev = OAL_PTR_NULL; /* 指向已经加密的netbuff */
    oal_netbuf_stru *pst_buf_first = OAL_PTR_NULL;   /* 指向还未加密的netbuff */

    /* 不加密wai帧 */
    if (mac_get_data_type_from_80211(pst_buf, MAC_80211_QOS_HTC_4ADDR_FRAME_LEN) == MAC_DATA_WAPI) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_wapi_netbuff_tx_handle::wai, dont encrypt!.}");
        return pst_buf;
    }

    ul_ret = pst_wapi->wapi_encrypt(pst_wapi, pst_buf);
    if (ul_ret != OAL_SUCC) {
        hmac_free_netbuf_list(pst_buf);
        return OAL_PTR_NULL;
    }

    pst_netbuf_tmp = pst_buf;

    /* 使netbuff指针指向下一个需要加密的分片帧 */
    pst_netbuf_prev = oal_netbuf_next(pst_buf);
    if (pst_netbuf_prev == OAL_PTR_NULL) {
        // print TODO
        return OAL_PTR_NULL;
    }
    pst_buf_first = pst_netbuf_prev;
    pst_buf = oal_netbuf_next(pst_netbuf_prev);

    oal_netbuf_free(pst_netbuf_tmp);

    while (pst_buf != OAL_PTR_NULL) {
        ul_ret = pst_wapi->wapi_encrypt(pst_wapi, pst_buf);
        if (ul_ret != OAL_SUCC) {
            hmac_free_netbuf_list(pst_buf_first);
            return OAL_PTR_NULL;
        }
        oal_netbuf_next(pst_netbuf_prev) = oal_netbuf_next(pst_buf);
        pst_netbuf_tmp = pst_buf;
        pst_netbuf_prev = oal_netbuf_next(pst_buf);
        if (pst_netbuf_prev == OAL_PTR_NULL) {
            // print TODO
            return OAL_PTR_NULL;
        }
        pst_buf = oal_netbuf_next(pst_netbuf_prev);

        oal_netbuf_free(pst_netbuf_tmp);
    }
    return pst_buf_first;
}


oal_netbuf_stru *hmac_wapi_netbuff_rx_handle(hmac_wapi_stru *pst_wapi, oal_netbuf_stru *pst_buf)
{
    oal_uint32 ul_ret;
    oal_netbuf_stru *pst_netbuf_tmp = OAL_PTR_NULL; /* 指向需要释放的netbuff */

    /* 非加密帧，不进行解密 */
    if (!((oal_netbuf_data(pst_buf))[1] & 0x40)) {
        return pst_buf;
    }

    ul_ret = pst_wapi->wapi_decrypt(pst_wapi, pst_buf);
    if (ul_ret != OAL_SUCC) {
        return OAL_PTR_NULL;
    }

    pst_netbuf_tmp = pst_buf;
    pst_buf = oal_netbuf_next(pst_buf);
    oal_netbuf_free(pst_netbuf_tmp);

    return pst_buf;
}


oal_uint32 hmac_wapi_add_key(hmac_wapi_stru *pst_wapi, oal_uint8 uc_key_index, oal_uint8 *puc_key)
{
    hmac_wapi_key_stru *pst_key;
    oal_int32 l_ret;

    wapi_is_port_valid(pst_wapi) = OAL_TRUE;
    pst_wapi->uc_keyidx = uc_key_index;
    pst_wapi->uc_keyupdate_flg = OAL_TRUE;
    // pst_wapi->ast_wapi_key[uc_key_index].ul_wpi_ek_len = WAPI_KEY_LEN;
    pst_key = &(pst_wapi->ast_wapi_key[uc_key_index]);

    l_ret = memcpy_s(pst_key->auc_wpi_ek, WAPI_KEY_LEN, puc_key, WAPI_KEY_LEN);
    l_ret += memcpy_s(pst_key->auc_wpi_ck, WAPI_KEY_LEN, puc_key + WAPI_KEY_LEN, WAPI_KEY_LEN);
    pst_key->uc_key_en = OAL_TRUE;

    /* 重置PN */
    l_ret += memcpy_s(pst_key->auc_pn_rx, WAPI_PN_LEN, g_auc_wapi_pn_init, WAPI_PN_LEN);
    l_ret += memcpy_s(pst_key->auc_pn_tx, WAPI_PN_LEN, g_auc_wapi_pn_init, WAPI_PN_LEN);
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_wapi_add_key::memcpy fail!");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_uint8 hmac_wapi_is_wpihdr_valid(hmac_wapi_stru *pst_wapi, oal_uint8 *puc_wapi_hdr)
{
    oal_uint8 uc_keyidx_rx;
    oal_uint8 *puc_pn_rx = OAL_PTR_NULL;

    uc_keyidx_rx = *puc_wapi_hdr;

    if (hmac_wapi_is_keyidx_valid(pst_wapi, uc_keyidx_rx) != OAL_TRUE) {
        return OAL_FALSE;
    }

    puc_pn_rx = puc_wapi_hdr + SMS4_KEY_IDX + SMS4_WAPI_HDR_RESERVE;
    if (pst_wapi->wapi_is_pn_odd(puc_pn_rx) != OAL_TRUE) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_wapi_is_wpihdr_valid::wapi_is_pn_odd==false.}");
        wapi_rx_pn_odd_err(pst_wapi, puc_pn_rx);
        return OAL_FALSE;
    }

    /* 此处为重放检测，实际测试中非常影响性能，暂时屏蔽 */
    return OAL_TRUE;
}


oal_uint32 hmac_wapi_decrypt(hmac_wapi_stru *pst_wapi, oal_netbuf_stru *pst_netbuf)
{
    oal_netbuf_stru *pst_netbuff_des = NULL;
    oal_uint8 *puc_key_ek = NULL;
    oal_uint8 *puc_key_ck = NULL;
    oal_uint8 uc_key_index;
    oal_uint16 us_pdu_len;
    oal_uint8 auc_calc_mic[SMS4_MIC_LEN];
    oal_uint32 ul_wapi_result;

    mac_ieee80211_frame_stru *pst_mac_hdr = OAL_PTR_NULL;
    oal_uint8 *puc_netbuff = OAL_PTR_NULL;
    oal_uint16 us_netbuff_len;
    oal_uint8 *puc_wapi_hdr = OAL_PTR_NULL;
    oal_uint8 uc_mac_hdr_len;
    oal_uint8 *puc_pn = OAL_PTR_NULL;
    oal_uint8 *puc_pdu = OAL_PTR_NULL;
    oal_uint8 *puc_mic_data = OAL_PTR_NULL; /* 按照协议，构造mic所需要的数据，见 wapi实施指南 图51 */
    oal_uint8 *puc_mic = OAL_PTR_NULL;
    oal_uint16 us_mic_len;
    mac_rx_ctl_stru *pst_rx_ctl = OAL_PTR_NULL;
    mac_rx_ctl_stru *pst_rx_ctl_in = OAL_PTR_NULL;

    wapi_rx_port_valid(pst_wapi);
    /************ 1. 解密前的数据准备,获取各头指针和内容长度 ************/
    puc_netbuff = oal_netbuf_data(pst_netbuf);  // for ut,del temprarily

    us_netbuff_len = (oal_uint16)oal_netbuf_len(pst_netbuf);

    /* 获取mac头 */
    pst_mac_hdr = (mac_ieee80211_frame_stru *)puc_netbuff;

    /* wapi的数据帧一般为QOS帧 */
    pst_rx_ctl_in = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    uc_mac_hdr_len = pst_rx_ctl_in->uc_mac_header_len;
    puc_wapi_hdr = (oal_uint8 *)pst_mac_hdr + uc_mac_hdr_len;
    puc_pn = puc_wapi_hdr + SMS4_KEY_IDX + SMS4_WAPI_HDR_RESERVE;
    puc_pdu = puc_pn + SMS4_PN_LEN;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_wpi_decrypt::uc_mac_hdr_len %u!.}", uc_mac_hdr_len);

    if (us_netbuff_len < (oal_uint16)(uc_mac_hdr_len + HMAC_WAPI_HDR_LEN + SMS4_MIC_LEN)) {
        oam_error_log2(0, OAM_SF_ANY, "{hmac_wpi_decrypt::ul_netbuff_len %u, machdr len %u err!.}",
                       us_netbuff_len, uc_mac_hdr_len);
        oal_netbuf_free(pst_netbuf);
        wapi_rx_netbuf_len_err(pst_wapi);
        return OAL_FAIL;
    }
    us_pdu_len = us_netbuff_len - uc_mac_hdr_len - HMAC_WAPI_HDR_LEN - SMS4_MIC_LEN;

    uc_key_index = *puc_wapi_hdr;

    if (uc_key_index >= HMAC_WAPI_MAX_KEYID) {
        wapi_rx_idx_err(pst_wapi);
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_wpi_decrypt::uc_key_index %u err!.}", uc_key_index);
        oal_netbuf_free(pst_netbuf);
        return OAL_FAIL;
    }

    if (hmac_wapi_is_wpihdr_valid(pst_wapi, puc_wapi_hdr) != OAL_TRUE) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_wpi_decrypt::hmac_wapi_is_wpihdr_valid err!.}");
        oal_netbuf_free(pst_netbuf);
        return OAL_FAIL;
    }

    /************ 2. 准备新的netbuff,用来存放解密后的数据, 填写cb字段 ************/
    pst_netbuff_des = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (pst_netbuff_des == NULL) {
        wapi_rx_memalloc_err(pst_wapi);
        oal_netbuf_free(pst_netbuf);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 先拷贝mac头 */
    oal_netbuf_init(pst_netbuff_des, uc_mac_hdr_len);
    oal_netbuf_copydata(pst_netbuf, 0, oal_netbuf_data(pst_netbuff_des), uc_mac_hdr_len);

    /* 拷贝cb */
    pst_rx_ctl = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuff_des);
    if (memcpy_s(pst_rx_ctl, MAC_TX_CTL_SIZE, oal_netbuf_cb(pst_netbuf), MAC_TX_CTL_SIZE) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_wpi_decrypt::memcpy fail!");
        oal_netbuf_free(pst_netbuff_des);
        oal_netbuf_free(pst_netbuf);
        return OAL_FAIL;
    }

    /************ 3. 解密前的密钥准备和PN准备 ************/
    puc_key_ek = pst_wapi->ast_wapi_key[uc_key_index].auc_wpi_ek;
    hmac_wpi_swap_pn(puc_pn, SMS4_PN_LEN);

    /******************** 4. 解密**************************/
    ul_wapi_result = hmac_wpi_decrypt(puc_pn, puc_pdu,
                                      (us_pdu_len + SMS4_MIC_LEN), /* 需要解密的数据长度 */
                                      puc_key_ek, (oal_netbuf_data(pst_netbuff_des) + uc_mac_hdr_len));
    if (ul_wapi_result != OAL_SUCC) {
        oal_netbuf_free(pst_netbuff_des);
        /* 返回之前注意入参netbuff是否在外面被释放 */
        oal_netbuf_free(pst_netbuf);
        return OAL_ERR_CODE_WAPI_DECRYPT_FAIL;
    }

    /* mic作为校验数，不需要put */
    oal_netbuf_put(pst_netbuff_des, us_pdu_len);

    /************ 5. 计算mic，并进行校验 ************/
    puc_mic_data = hmac_wapi_mic_alloc(hmac_wapi_is_qos(pst_mac_hdr), us_pdu_len, &us_mic_len);
    if (puc_mic_data == OAL_PTR_NULL) {
        wapi_rx_memalloc_err(pst_wapi);
        oal_netbuf_free(pst_netbuff_des);
        /* 注意netbuff后续是否有释放处理 */
        oal_netbuf_free(pst_netbuf);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 计算mic预备数据 */
    hmac_wapi_calc_mic_data(pst_mac_hdr, uc_mac_hdr_len, uc_key_index,
                            oal_netbuf_data(pst_netbuff_des) + uc_mac_hdr_len, us_pdu_len, puc_mic_data, us_mic_len);
    puc_key_ck = pst_wapi->ast_wapi_key[uc_key_index].auc_wpi_ck;
    ul_wapi_result = hmac_wpi_pmac(puc_pn, SMS4_PN_LEN * sizeof(oal_uint8), puc_mic_data,
        (us_mic_len >> 4), puc_key_ck, auc_calc_mic, OAL_SIZEOF(auc_calc_mic)); /* mic_len 取高4位 */

    /* 计算完mic后，释放mic data */
    hmac_wapi_mic_free(puc_mic_data);
    if (ul_wapi_result != OAL_SUCC) {
        oal_netbuf_free(pst_netbuff_des);
        oal_netbuf_free(pst_netbuf);
        return OAL_ERR_CODE_WAPI_MIC_CALC_FAIL;
    }

    puc_mic = oal_netbuf_data(pst_netbuff_des) + uc_mac_hdr_len + us_pdu_len;
    if (oal_memcmp(puc_mic, auc_calc_mic, SMS4_MIC_LEN) != 0) {  /* 比较MIC */
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_wpi_decrypt::mic check fail!.}");
        wapi_rx_mic_err(pst_wapi);

        oal_netbuf_free(pst_netbuff_des);
        oal_netbuf_free(pst_netbuf);
        return OAL_ERR_CODE_WAPI_MIC_CMP_FAIL;
    }

    /* 返回前清protected */
    (oal_netbuf_data(pst_netbuff_des))[1] &= ~0x40;

    /* 填写cb */
    pst_rx_ctl->pul_mac_hdr_start_addr = (oal_uint32 *)oal_netbuf_header(pst_netbuff_des);
    pst_rx_ctl->uc_mac_header_len = uc_mac_hdr_len;
    pst_rx_ctl->us_frame_len = (oal_uint16)oal_netbuf_len(pst_netbuff_des);

    oal_netbuf_next(pst_netbuff_des) = oal_netbuf_next(pst_netbuf);
    oal_netbuf_next(pst_netbuf) = pst_netbuff_des;

    hmac_wapi_pn_update(pst_wapi->ast_wapi_key[pst_wapi->uc_keyidx].auc_pn_rx, pst_wapi->uc_pn_inc);

    oam_warning_log0(0, OAM_SF_ANY, "{hmac_wpi_decrypt::OK!.}");
    wapi_rx_decrypt_ok(pst_wapi);
    return OAL_SUCC;
}


oal_uint32 hmac_wapi_encrypt(hmac_wapi_stru *pst_wapi, oal_netbuf_stru *pst_netbuf)
{
    mac_ieee80211_frame_stru *pst_mac_hdr = ((mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf))->pst_frame_header;
    oal_uint8 uc_hdr_len = ((mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf))->uc_frame_header_length;
    oal_uint8 *puc_mic_data = OAL_PTR_NULL;
    oal_uint16 us_pdu_len = ((mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf))->us_mpdu_len;
    oal_uint16 us_mic_len;
    oal_uint8 uc_key_index;
    oal_uint8 *puc_key_ck = OAL_PTR_NULL;
    oal_uint8 *puc_key_ek = OAL_PTR_NULL;
    oal_uint8 auc_mic[SMS4_MIC_LEN];
    oal_uint8 auc_pn_swap[SMS4_PN_LEN]; /* 保存变换后的pn,用来计算mic和加密 */
    oal_netbuf_stru *pst_netbuff_des = OAL_PTR_NULL;
    oal_uint8 *puc_wapi_hdr = OAL_PTR_NULL;
    oal_uint8 *puc_datain = OAL_PTR_NULL;
    oal_uint32 ul_result;
    int32_t l_ret;
    oal_uint8 *puc_payload = oal_netbuf_data(pst_netbuf) + MAC_80211_QOS_HTC_4ADDR_FRAME_LEN;

    /* 设置加密位 注意，mac头涉及加密，所以需要在最开始设置 */
    mac_set_protectedframe((oal_uint8 *)pst_mac_hdr);
    oam_warning_log2(0, 0, "uc_hdr_len %u! bit_more==%u", uc_hdr_len, pst_mac_hdr->st_frame_control.bit_more_frag);
    /************ 2. 计算mic,wapi的数据帧一般为QOS帧  ************/
    puc_mic_data = hmac_wapi_mic_alloc(hmac_wapi_is_qos(pst_mac_hdr), us_pdu_len, &us_mic_len);
    if (puc_mic_data == OAL_PTR_NULL) {
        wapi_tx_memalloc_err(pst_wapi);
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wapi_encrypt::hmac_wapi_mic_alloc err!");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 计算mic预备数据 */
    uc_key_index = pst_wapi->uc_keyidx;
    hmac_wapi_calc_mic_data(pst_mac_hdr, uc_hdr_len, uc_key_index, puc_payload, us_pdu_len, puc_mic_data, us_mic_len);
    puc_key_ck = pst_wapi->ast_wapi_key[uc_key_index].auc_wpi_ck;
    l_ret = memcpy_s(auc_pn_swap, SMS4_PN_LEN, pst_wapi->ast_wapi_key[uc_key_index].auc_pn_tx, SMS4_PN_LEN);

    hmac_wpi_swap_pn(auc_pn_swap, SMS4_PN_LEN);
    /* 计算mic（us_mic_len取高4位） */
    ul_result = hmac_wpi_pmac(auc_pn_swap, SMS4_PN_LEN * sizeof(oal_uint8), puc_mic_data, (us_mic_len >> 4), puc_key_ck,
        auc_mic, OAL_SIZEOF(auc_mic));

    hmac_wapi_mic_free(puc_mic_data);
    if (ul_result == OAL_FAIL) {
        wapi_tx_mic_err(pst_wapi);
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wapi_encrypt::hmac_wpi_pmac mic calc err!");
        return OAL_ERR_CODE_WAPI_MIC_CALC_FAIL;
    }

    /************ 3. 准备新的netbuff,用来存放加密后的数据, 填写cb,并准备加密前的数据 ************/
    pst_netbuff_des = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (pst_netbuff_des == OAL_PTR_NULL) {
        wapi_tx_memalloc_err(pst_wapi);
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wapi_encrypt::pst_netbuff_des alloc err!");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 填写cb */
    l_ret += memcpy_s(oal_netbuf_cb(pst_netbuff_des), MAC_TX_CTL_SIZE, oal_netbuf_cb(pst_netbuf), MAC_TX_CTL_SIZE);

    /* 先拷贝mac头,为了后续hcc处理，此处填写最大的空间 */
    oal_netbuf_init(pst_netbuff_des, MAC_80211_QOS_HTC_4ADDR_FRAME_LEN);
    l_ret += memcpy_s(oal_netbuf_data(pst_netbuff_des), uc_hdr_len, pst_mac_hdr, uc_hdr_len);

    puc_datain = oal_mem_alloc_m(OAL_MEM_POOL_ID_LOCAL, us_pdu_len + SMS4_MIC_LEN, OAL_TRUE);
    if (puc_datain == OAL_PTR_NULL) {
        oal_netbuf_free(pst_netbuff_des);
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wapi_encrypt::puc_datain alloc err!");
        wapi_tx_memalloc_err(pst_wapi);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    l_ret += memcpy_s(puc_datain, us_pdu_len + SMS4_MIC_LEN, puc_payload, us_pdu_len);
    /* 拷贝mic */
    l_ret += memcpy_s(puc_datain + us_pdu_len, SMS4_MIC_LEN, auc_mic, SMS4_MIC_LEN);

    puc_key_ek = pst_wapi->ast_wapi_key[uc_key_index].auc_wpi_ek;
    /************************ 4. 加密 ************************/
    ul_result = hmac_wpi_encrypt(auc_pn_swap, puc_datain, us_pdu_len + SMS4_MIC_LEN, puc_key_ek, HMAC_WAPI_HDR_LEN +
        oal_netbuf_data(pst_netbuff_des) + MAC_80211_QOS_HTC_4ADDR_FRAME_LEN);

    oal_mem_free_m(puc_datain, OAL_TRUE);
    if (ul_result != OAL_SUCC) {
        oal_netbuf_free(pst_netbuff_des);
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_wapi_encrypt::hmac_wpi_encrypt err==%u!", ul_result);
        return OAL_ERR_CODE_WAPI_ENRYPT_FAIL;
    }
    /* 此处put完之后，netbuff的len为mac头+pdulen+sms4+wapi的长度 */
    oal_netbuf_put(pst_netbuff_des, us_pdu_len + SMS4_MIC_LEN + HMAC_WAPI_HDR_LEN);

    /***************** 5. 填写wapi头 *****************/
    puc_wapi_hdr = oal_netbuf_data(pst_netbuff_des) + MAC_80211_QOS_HTC_4ADDR_FRAME_LEN;

    /* 填写WPI头 -- keyIndex */
    *(puc_wapi_hdr) = uc_key_index;
    /* 保留位清零 */
    *(puc_wapi_hdr + SMS4_KEY_IDX) = 0;
    /* 填写PN */
    l_ret += memcpy_s((puc_wapi_hdr + SMS4_KEY_IDX + SMS4_WAPI_HDR_RESERVE), SMS4_PN_LEN,
                      pst_wapi->ast_wapi_key[uc_key_index].auc_pn_tx, SMS4_PN_LEN);
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_wapi_encrypt::memcpy fail!");
        oal_netbuf_free(pst_netbuff_des);
        return OAL_FAIL;
    }

    /* 再次填写cb */
    ((mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuff_des))->pst_frame_header =
        (mac_ieee80211_frame_stru *)oal_netbuf_data(pst_netbuff_des);

    /* 不包括mac hdr */
    ((mac_tx_ctl_stru *)oal_netbuf_cb (pst_netbuff_des))->us_mpdu_len =
        (oal_uint16)(HMAC_WAPI_HDR_LEN + us_pdu_len + SMS4_MIC_LEN);

    oal_netbuf_next(pst_netbuff_des) = oal_netbuf_next(pst_netbuf);
    oal_netbuf_next(pst_netbuf) = pst_netbuff_des;

    /* 更新pn */
    hmac_wapi_pn_update(pst_wapi->ast_wapi_key[pst_wapi->uc_keyidx].auc_pn_tx, pst_wapi->uc_pn_inc);
    oam_warning_log0(0, OAM_SF_ANY, "{hmac_wapi_encrypt::hmac_wpi_encrypt OK!");

    wapi_tx_encrypt_ok(pst_wapi);
    return OAL_SUCC;
}


oal_void hmac_wapi_deinit(hmac_wapi_stru *pst_wapi)
{
    memset_s(pst_wapi, OAL_SIZEOF(hmac_wapi_stru), 0, OAL_SIZEOF(hmac_wapi_stru));

    return;
}


oal_uint32 hmac_wapi_reset_port(hmac_wapi_stru *pst_wapi)
{
    pst_wapi->uc_port_valid = OAL_FALSE;
    return OAL_SUCC;
}


oal_void hmac_wapi_init(hmac_wapi_stru *pst_wapi, oal_uint8 uc_pairwise)
{
    oal_uint32 ul_loop;

    hmac_wapi_deinit(pst_wapi);

    if (uc_pairwise == OAL_TRUE) {
        pst_wapi->uc_pn_inc = WAPI_UCAST_INC;
        pst_wapi->wapi_is_pn_odd = hmac_wapi_is_pn_odd_ucast;
    } else {
        pst_wapi->uc_pn_inc = WAPI_BCAST_INC;
        pst_wapi->wapi_is_pn_odd = hmac_wapi_is_pn_odd_bcast;
    }

    for (ul_loop = 0; ul_loop < HMAC_WAPI_MAX_KEYID; ul_loop++) {
        pst_wapi->ast_wapi_key[ul_loop].uc_key_en = OAL_FALSE;
    }

    pst_wapi->uc_port_valid = OAL_FALSE;
    pst_wapi->wapi_decrypt = hmac_wapi_decrypt;
    pst_wapi->wapi_encrypt = hmac_wapi_encrypt;
    pst_wapi->wapi_netbuff_txhandle = hmac_wapi_netbuff_tx_handle;
    pst_wapi->wapi_netbuff_rxhandle = hmac_wapi_netbuff_rx_handle;
    return;
}


#ifdef _PRE_WAPI_DEBUG

oal_void hmac_wapi_dump_frame(oal_uint8 *puc_info, oal_uint8 *puc_data, oal_uint32 ul_len)
{
    oal_uint32 ul_loop;
    OAL_IO_PRINT("%s: \r\n", puc_info);
    for (ul_loop = 0; ul_loop < ul_len; ul_loop += 4) { /* 每4个byte打印 */
        OAL_IO_PRINT("%2x ", ul_loop / 4); /* 每4个byte打印 */
        OAL_IO_PRINT("%2x %2x %2x %2x \r\n", puc_data[ul_loop], puc_data[ul_loop + 1],
                     puc_data[ul_loop + 2], puc_data[ul_loop + 3]); /* puc_data[ul_loop]偏移2、3byte为参数输出打印 */
    }
}


oal_void hmac_wapi_display_usr_info(hmac_user_stru *pst_hmac_user)
{
    oal_uint32 ul_loop = 0;
    hmac_wapi_stru *pst_wapi = OAL_PTR_NULL;
    hmac_wapi_key_stru *pst_key = OAL_PTR_NULL;
    hmac_wapi_debug *pst_debug = OAL_PTR_NULL;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wapi port is %u!", wapi_is_port_valid(&pst_hmac_user->st_wapi));
    if (wapi_is_port_valid(&pst_hmac_user->st_wapi) != OAL_TRUE) {
        OAL_IO_PRINT("Err! wapi port is not valid!\n");

        return;
    }

    pst_wapi = &(pst_hmac_user->st_wapi);
    oam_warning_log0(0, OAM_SF_ANY, "keyidx\tupdate\t\tpn_inc\t\n");
    oam_warning_log3(0, OAM_SF_ANY, "%u\t%08x%04x\t\n",
                     pst_wapi->uc_keyidx,
                     pst_wapi->uc_keyupdate_flg,
                     pst_wapi->uc_pn_inc);

    for (ul_loop = 0; ul_loop < HMAC_WAPI_MAX_KEYID; ul_loop++) {
        pst_key = &pst_wapi->ast_wapi_key[ul_loop];
        /*lint -e64*/
        hmac_wapi_dump_frame("ek :", pst_key->auc_wpi_ek, WAPI_KEY_LEN);
        hmac_wapi_dump_frame("ck :", pst_key->auc_wpi_ck, WAPI_KEY_LEN);
        hmac_wapi_dump_frame("pn_local_rx :", pst_key->auc_pn_rx, WAPI_PN_LEN);
        hmac_wapi_dump_frame("pn_local_tx :", pst_key->auc_pn_tx, WAPI_PN_LEN);
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "key_en: %u\n", pst_key->uc_key_en);
        /*lint +e64*/
    }

    pst_debug = &pst_wapi->st_debug;
    oam_warning_log0(0, OAM_SF_ANY, "TX DEBUG INFO:");
    /*lint -e64*/
    hmac_wapi_dump_frame("pn_rx :", pst_debug->aucrx_pn, WAPI_PN_LEN);
    /*lint +e64*/
    oam_warning_log4(0, OAM_SF_ANY, "tx_drop==%u, tx_wai==%u, tx_port_valid==%u, tx_memalloc_fail==%u",
                     pst_debug->ultx_ucast_drop,
                     pst_debug->ultx_wai,
                     pst_debug->ultx_port_valid,
                     pst_debug->ultx_memalloc_err);
    oam_warning_log3(0, OAM_SF_ANY, "tx_mic_calc_fail==%u, tx_encrypt_ok==%u, tx_memalloc_err==%u",
                     pst_debug->ultx_mic_calc_fail,
                     // pst_debug->ultx_drop_wai,
                     pst_debug->ultx_encrypt_ok,
                     pst_debug->ultx_memalloc_err);

    oam_warning_log0(0, OAM_SF_ANY, "RX DEBUG INFO:");
    oam_warning_log4(0, OAM_SF_ANY, "rx_port_valid==%u, rx_idx_err==%u, rx_netbuff_len_err==%u, rx_idx_update_err==%u",
                     pst_debug->ulrx_port_valid,
                     pst_debug->ulrx_idx_err,
                     pst_debug->ulrx_netbuff_len_err,
                     pst_debug->ulrx_idx_update_err);

    oam_warning_log4(0, OAM_SF_ANY, "rx_key_en_err==%u, rx_pn_odd_err==%u, rx_pn_replay_err==%u, rx_decrypt_ok==%u",
                     pst_debug->ulrx_key_en_err,
                     pst_debug->ulrx_pn_odd_err,
                     pst_debug->ulrx_pn_replay_err,
                     pst_debug->ulrx_decrypt_ok);
    return;
}


oal_uint32 hmac_wapi_display_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_usr_idx)
{
    hmac_user_stru *pst_hmac_user = OAL_PTR_NULL;
    hmac_user_stru *pst_hmac_multi_user;

    pst_hmac_multi_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_mac_vap->us_multi_user_idx);
    if (pst_hmac_multi_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "Err! multi usr %u does not exist!",
                         pst_mac_vap->us_multi_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "*****************multi usr info start****************");
    hmac_wapi_display_usr_info(pst_hmac_multi_user);
    oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "*****************multi usr info end****************");

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_usr_idx);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "Err! ucast usr %u does not exist!", us_usr_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "*****************ucast usr info start****************");
    hmac_wapi_display_usr_info(pst_hmac_user);
    oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "*****************ucast usr info end****************");

    return OAL_SUCC;
}
#endif
#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */


