

#ifndef __SDT_DRV_H__
#define __SDT_DRV_H__

/* 其他头文件包含 */
#include "oal_ext_if.h"
#include "oal_workqueue.h"
#include "oam_ext_if.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_SDT_DRV_H
/* 宏定义 */
/* memory */
#if (_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT) || \
    (_PRE_TARGET_PRODUCT_TYPE_CPE == _PRE_CONFIG_TARGET_PRODUCT)
#define NETLINK_TEST 23 /* E5平台定为23 */
#else
#ifdef  NETLINK_WIFI_SDT_HISI
#define NETLINK_TEST                        NETLINK_WIFI_SDT_HISI
#else
#define NETLINK_TEST                        28   /* 适配5610 ATP工程，防止协议号冲突，归一修改为28 */
#endif
#endif
#define DATA_BUF_LEN 2048

/* head and end */
#define SDT_DRV_PKT_START_FLG        0x7e
#define SDT_DRV_PKT_END_FLG          0x7e
#define sdt_drv_get_low_byte(_data)  ((oal_uint8)((_data) & 0xff))
#define sdt_drv_get_high_byte(_data) ((oal_uint8)(((_data) & 0xff00) >> 8))
#define SDT_DRV_PKT_TAIL_LEN         1

#define SDT_DEV_NAME_INDEX           8

#define MAX_NUM 256

#define MAX_QUEUE_COUNT 20
#define MAX_CO_SIZE     896
#define MAX_NLMSG_LEN   1024

#define SDT_DRV_REPORT_NO_CONNECT_FREQUENCE 500

#define SDT_DRV_PKT_RECORD_MAX_NUM 100

/* 枚举定义 */
enum OM_NETLINK_MSG_TYPE_ENUM {
    NETLINK_MSG_HELLO = 0, /* netlink connect hello */
    NETLINK_MSG_SDTCMD,    /* std with device */
    NETLINK_MSG_SDTCMD_OPS /* device to SDT need encapsulation */
};

/* STRUCT定义 */
typedef struct {
    oal_uint32 ul_cmd;
    oal_uint32 ul_len;
} sdt_drv_netlink_msg_hdr_stru;

/* SDT驱动传给PC端的数据头结构，一共8个字节 */
typedef struct {
    oal_uint8 uc_data_start_flg;          /* 数据开始的标志，同时也是数据结束的标志 */
    oam_data_type_enum_uint8 en_msg_type; /* 数据类型(LOG,EVENT,OTA等) */
    oal_uint8 uc_prim_id;                 /* 通讯原语 */
    oal_uint8 uc_resv[1];                 /* 保留 */
    oal_uint8 uc_data_len_low_byte;       /* 数据长度的低8比特 */
    oal_uint8 uc_data_len_high_byte;      /* 数据长度的高8比特 */
    oal_uint8 uc_sequence_num_low_byte;   /* 序列号低8比特 */
    oal_uint8 uc_sequence_num_high_byte;  /* 序列号高8比特 */
} sdt_drv_pkt_hdr_stru;

/* SDT DRV侧的全局管理结构 */
typedef struct {
    oal_work_stru rx_wifi_work;
    oal_netbuf_head_stru rx_wifi_dbg_seq;
    oal_sock_stru *pst_nlsk;
    oal_uint8 *puc_data;
    oal_uint32 ul_usepid;
    oal_uint16 us_sn_num;
    oal_uint8 auc_resv[2];
    oal_spin_lock_stru st_spin_lock;
    oal_atomic ul_unconnect_cnt; /* 统计进入send函数时netlink没有连接的次数 */
} sdt_drv_mng_stru;

/* 函数声明 */
extern oal_int32 sdt_drv_main_init(oal_void);
extern oal_void sdt_drv_main_exit(oal_void);

#endif /* end of Sdt_drv.h */
