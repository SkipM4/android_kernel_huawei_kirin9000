

#include "oal_net.h"
#include "oal_types.h"
#include "oam_ext_if.h"
#include "frw_task.h"
#include "mac_vap.h"
#include "mac_resource.h"
#include "hmac_vap.h"
#include "hmac_auto_adjust_freq.h"
#include "hmac_ext_if.h"
#include "hmac_blockack.h"
#include "hmac_tx_data.h"
#include "host_hal_ext_if.h"
#include "hmac_stat.h"
#include "hmac_config.h"
#include "hmac_tx_switch.h"
#include "hmac_tx_sche_switch_fsm.h"
#include "hmac_tx_switch_fsm.h"
#include "external/lpcpu_feature.h"
#include "hmac_tid_sche.h"
#include "hmac_tid_update.h"
#include "hmac_tx_complete.h"
#ifdef _PRE_WLAN_CHBA_MGMT
#include "hmac_chba_mgmt.h"
#include "hmac_chba_interface.h"
#include "hmac_chba_ps.h"
#endif
#include "external/oal_pm_qos_interface.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_AUTO_ADJUST_FREQ_C

#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
#define INVALID_PKMODE_TH (0xFFFFFFFF)
#endif
#define HMAC_TX_THROUGHPUT_OFFSET_7 7

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#ifdef WIN32
uint32_t jiffies;
#endif

freq_lock_control_stru g_freq_lock_control = { 0 };
/* Wi-Fi驱动收发负载识别数据区 */
freq_wifi_load_stru g_st_wifi_load = { 0 };
#ifdef _PRE_FEATURE_PLAT_LOCK_CPUFREQ
struct cpufreq_req g_ast_cpufreq[OAL_BUS_MAXCPU_NUM];
hisi_max_cpu_freq g_aul_cpumaxfreq[OAL_BUS_MAXCPU_NUM];
struct pm_qos_request g_st_pmqos_requset;
#endif
oal_bool_enum_uint8 g_lowpower_strategy_swtich = OAL_TRUE;
#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
oal_bool_enum_uint8 g_en_pk_mode_swtich = OAL_TRUE;

/* pk mode的判定的门限，以字节为单位 */
/*
PK模式门限基数:
{(单位Mbps)  20M     40M    80M   160M   80+80M
lagency:    {valid, valid, valid, valid, valid},   (基础协议模式没有pk mode )
HT:         {62, 123, valid, valid, valid},
VHT:        {77, 167, 329, 660, 660},
HE:         {110, 205, 460, 750, 750},
};

PK模式二级门限:
高档位门限: g_st_pk_mode_high_th_table = PK模式门限基数 * 70% *1024 *1024 /8  (单位字节)
低档位门限: g_st_pk_mode_low_th_table  = PK模式门限基数 * 30% *1024 *1024 /8  (单位字节)

 */
uint32_t g_st_pk_mode_high_th_table[WLAN_PROTOCOL_CAP_BUTT][WLAN_BW_CAP_BUTT] = {
    { INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH },
    { 45,                110,               INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH },
    { 60,                130,               280,               562,               562 },
#ifdef _PRE_WLAN_FEATURE_11AX
    { 65,                140,               300,               525,               525 }
#endif /* #ifdef _PRE_WLAN_FEATURE_11AX */

};

uint32_t g_st_pk_mode_low_th_table[WLAN_PROTOCOL_CAP_BUTT][WLAN_BW_CAP_BUTT] = {
    { INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH },
    { 30,                80,                INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH },
    { 40,                100,               240,               400,               400 },
#ifdef _PRE_WLAN_FEATURE_11AX
    { 50,                110,               250,               450,               450 }
#endif /* #ifdef _PRE_WLAN_FEATURE_11AX */
};
#endif

uint32_t g_orig_cpu_min_freq = 0;
uint32_t g_orig_cpu_max_freq = 0;
uint32_t g_orig_ddr_min_freq = 0;
uint32_t g_orig_ddr_max_freq = 0;

#if !defined(WIN32) || defined(_PRE_WINDOWS_SUPPORT)
#define HOST_NO_BA_FREQ_LEVEL_BUTT 4
/* 由定制化进行初始化 */
host_speed_freq_level_stru g_host_speed_freq_level[HOST_SPEED_FREQ_LEVEL_BUTT] = {
    /* pps门限                   CPU主频下限                     DDR主频下限 */
    { PPS_VALUE_0, CPU_MIN_FREQ_VALUE_0, DDR_MIN_FREQ_VALUE_0 },
    { PPS_VALUE_1, CPU_MIN_FREQ_VALUE_1, DDR_MIN_FREQ_VALUE_1 },
    { PPS_VALUE_2, CPU_MIN_FREQ_VALUE_2, DDR_MIN_FREQ_VALUE_2 },
    { PPS_VALUE_3, CPU_MIN_FREQ_VALUE_3, DDR_MIN_FREQ_VALUE_3 },
};
host_speed_freq_level_stru g_host_no_ba_freq_level[HOST_NO_BA_FREQ_LEVEL_BUTT] = {
    /* pps门限                        CPU主频下限                      DDR主频下限 */
    { NO_BA_PPS_VALUE_0, CPU_MIN_FREQ_VALUE_0, DDR_MIN_FREQ_VALUE_0 },
    { NO_BA_PPS_VALUE_1, CPU_MIN_FREQ_VALUE_1, DDR_MIN_FREQ_VALUE_1 },
    { NO_BA_PPS_VALUE_2, CPU_MIN_FREQ_VALUE_2, DDR_MIN_FREQ_VALUE_2 },
    { NO_BA_PPS_VALUE_3, CPU_MIN_FREQ_VALUE_2, DDR_MIN_FREQ_VALUE_2 },
};
device_speed_freq_level_stru g_device_speed_freq_level[] = {
    /* device主频类型 */
    { FREQ_IDLE },
    { FREQ_MIDIUM },
    { FREQ_HIGHER },
    { FREQ_HIGHEST },
};

struct pm_qos_request *g_pst_wifi_auto_ddr = NULL;

#else
host_speed_freq_level_stru g_host_speed_freq_level[] = {
    /* pps门限                   CPU主频下限                     DDR主频下限 */
    { PPS_VALUE_0, CPU_MIN_FREQ_VALUE_0, DDR_MIN_FREQ_VALUE_0 },
    { PPS_VALUE_1, CPU_MIN_FREQ_VALUE_1, DDR_MIN_FREQ_VALUE_1 },
    { PPS_VALUE_2, CPU_MIN_FREQ_VALUE_2, DDR_MIN_FREQ_VALUE_2 },
    { PPS_VALUE_3, CPU_MIN_FREQ_VALUE_3, DDR_MIN_FREQ_VALUE_3 },
};
device_speed_freq_level_stru g_device_speed_freq_level[] = {
    /* device主频类型 */
    { FREQ_IDLE },
    { FREQ_MIDIUM },
    { FREQ_HIGHEST },
    { FREQ_HIGHEST },
};

#define mutex_init(mux)
#define mutex_lock(mux)
#define mutex_unlock(mux)
#define spin_lock_init(mux)
#define mutex_destroy(mux)
#define spin_unlock_bh(mux)
#endif


uint8_t hmac_set_auto_freq_mod(oal_freq_lock_enum uc_freq_enable)
{
    g_freq_lock_control.uc_lock_mod = uc_freq_enable;

    /* 设置device是否使能 */
    if (uc_freq_enable == FREQ_LOCK_ENABLE) {
        /* 初始频率为idle */
        g_freq_lock_control.uc_curr_lock_level = FREQ_IDLE;
        /* 期望频率为idle */
        g_freq_lock_control.uc_req_lock_level = FREQ_IDLE;
        /* WIFI打开时,定时器参数更新 */
        g_freq_lock_control.pre_jiffies = jiffies;
    } else {
        /* 关闭WIFI时,将CPU频率调整为默认值 */
        mutex_lock(&g_freq_lock_control.st_lock_freq_mtx);
        mutex_unlock(&g_freq_lock_control.st_lock_freq_mtx);

        g_freq_lock_control.uc_curr_lock_level = FREQ_BUTT;
        oam_warning_log2(0, OAM_SF_ANY, "{hmac_set_auto_freq_mod: freq,min[%d]max[%d]!}",
                         g_orig_cpu_min_freq, g_orig_cpu_max_freq);
    }

    return OAL_SUCC;
}

void hmac_adjust_set_freq(void)
{
    uint8_t uc_req_lock_level = g_freq_lock_control.uc_req_lock_level;

    /* 相同则不需要配置系统文件 */
    if (uc_req_lock_level == g_freq_lock_control.uc_curr_lock_level) {
        return;
    }

    oam_warning_log2(0, OAM_SF_PWR, "{hmac_adjust_set_freq: freq [%d]to[%d]}",
                     g_freq_lock_control.uc_curr_lock_level, uc_req_lock_level);

    g_freq_lock_control.uc_curr_lock_level = uc_req_lock_level;
}

uint8_t hmac_get_freq_level(uint32_t speed)  // 根据吞吐率计算目标主频等级
{
    uint8_t level_idx = 0;

    if (hmac_is_device_ba_setup()) {
        if (speed <= g_host_speed_freq_level[HOST_SPEED_FREQ_LEVEL_1].speed_level) {
            level_idx = HMAC_FREQ_LEVEL_0;
        } else if ((speed > g_host_speed_freq_level[HOST_SPEED_FREQ_LEVEL_1].speed_level) &&
                   (speed <= g_host_speed_freq_level[HOST_SPEED_FREQ_LEVEL_2].speed_level)) {
            level_idx = HMAC_FREQ_LEVEL_1;
        } else if ((speed > g_host_speed_freq_level[HOST_SPEED_FREQ_LEVEL_2].speed_level) &&
                   (speed <= g_host_speed_freq_level[HOST_SPEED_FREQ_LEVEL_3].speed_level)) {
            level_idx = HMAC_FREQ_LEVEL_2;
        } else {
            level_idx = HMAC_FREQ_LEVEL_3;
        }
    } else {
#ifndef WIN32
        if (speed <= g_host_no_ba_freq_level[HOST_SPEED_FREQ_LEVEL_1].speed_level) {
            level_idx = HMAC_FREQ_LEVEL_0;
        } else if ((speed > g_host_no_ba_freq_level[HOST_SPEED_FREQ_LEVEL_1].speed_level) &&
                   (speed <= g_host_no_ba_freq_level[HOST_SPEED_FREQ_LEVEL_2].speed_level)) {
            level_idx = HMAC_FREQ_LEVEL_1;
        } else if ((speed > g_host_no_ba_freq_level[HOST_SPEED_FREQ_LEVEL_2].speed_level) &&
                   (speed <= g_host_no_ba_freq_level[HOST_SPEED_FREQ_LEVEL_3].speed_level)) {
            level_idx = HMAC_FREQ_LEVEL_2;
        } else {
            level_idx = HMAC_FREQ_LEVEL_3;
        }
#endif
    }
    return level_idx;
}


OAL_STATIC void hmac_pps_calc(void)
{
    uint32_t cur_jiffies;
    uint32_t sdio_dur_ms;  // 时间
    uint32_t trx_total;
    uint32_t tx_packets;
    uint32_t rx_packets;
    uint32_t host_ring_rx_packets;
    hmac_device_stat_stru *hmac_device_stats = hmac_stat_get_device_stats();

    tx_packets = (uint32_t)oal_atomic_read(&hmac_device_stats->tx_packets);
    rx_packets = (uint32_t)oal_atomic_read(&hmac_device_stats->rx_packets);
    host_ring_rx_packets = (uint32_t)oal_atomic_read(&hmac_device_stats->host_ring_rx_packets);
    oal_atomic_set(&hmac_device_stats->tx_packets, 0);
    oal_atomic_set(&hmac_device_stats->rx_packets, 0);
    oal_atomic_set(&hmac_device_stats->host_ring_rx_packets, 0);
    hmac_device_stats->total_tx_packets += tx_packets;
    hmac_device_stats->total_rx_packets += rx_packets;

    cur_jiffies = jiffies;  // jiffies是Linux内核中的一个全局变量，用来记录自系统启动以来产生的节拍的总数
    sdio_dur_ms = oal_jiffies_to_msecs(cur_jiffies - g_freq_lock_control.pre_jiffies);
    g_freq_lock_control.pre_jiffies = cur_jiffies;

    /* 如果当前统计时间不足定时器周期的一半,会导致统计PPS值偏大返回 */
    if ((sdio_dur_ms == 0) || (sdio_dur_ms < (WLAN_FREQ_TIMER_PERIOD >> 1))) {
        return;
    }

    // 计算PPS
    trx_total = tx_packets + rx_packets;
    g_freq_lock_control.total_sdio_pps = (trx_total * HMAC_FREQ_S_TRANS_TO_MS) / sdio_dur_ms;
    g_freq_lock_control.tx_pps = (tx_packets * HMAC_FREQ_S_TRANS_TO_MS) / sdio_dur_ms;
    g_freq_lock_control.rx_pps = (rx_packets * HMAC_FREQ_S_TRANS_TO_MS) / sdio_dur_ms;
    g_freq_lock_control.host_rx_pps = (host_ring_rx_packets * HMAC_FREQ_S_TRANS_TO_MS) / sdio_dur_ms;
}


void hmac_adjust_freq(void)
{
    uint8_t uc_req_lock_level;  // 目标主频等级

    /* host调频是否使能 */
    if (g_freq_lock_control.uc_lock_mod == FREQ_LOCK_DISABLE) {
        return;
    }

    g_freq_lock_control.uc_req_lock_level = hmac_get_freq_level(g_freq_lock_control.total_sdio_pps);

    mutex_lock(&g_freq_lock_control.st_lock_freq_mtx);
    uc_req_lock_level = g_freq_lock_control.uc_req_lock_level;
    if (uc_req_lock_level == g_freq_lock_control.uc_curr_lock_level) { // 目标主频和当前主频相同，不需要调频
        g_freq_lock_control.adjust_count = 0;
        mutex_unlock(&g_freq_lock_control.st_lock_freq_mtx);
        return;
    }

    if (uc_req_lock_level < g_freq_lock_control.uc_curr_lock_level) {
        /* 连续MAX_DEGRADE_FREQ_TIME_THRESHOLD后才降频，保证性能 */
        g_freq_lock_control.adjust_count++;

        if (g_freq_lock_control.total_sdio_pps) { // 当前数据流量不为零
            // 有包时连续100个周期都需要降频才降频
            if (g_freq_lock_control.adjust_count >= MAX_DEGRADE_FREQ_COUNT_THRESHOLD_SUCCESSIVE_10) {
                g_freq_lock_control.adjust_count = 0;
                /* 在这里进行主频值的调整 */
                hmac_adjust_set_freq();
            }
        } else { // 没有包时连续3个周期都需要降频再降频
            if (g_freq_lock_control.adjust_count >= MAX_DEGRADE_FREQ_COUNT_THRESHOLD_SUCCESSIVE_3) {
                g_freq_lock_control.adjust_count = 0;
                hmac_adjust_set_freq();
            }
        }
    } else {
        /* 升频不等待，立即执行保证性能 */
        g_freq_lock_control.adjust_count = 0;
        hmac_adjust_set_freq();
    }

    mutex_unlock(&g_freq_lock_control.st_lock_freq_mtx);
}


void hmac_wifi_auto_freq_ctrl_init(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (g_freq_lock_control.en_is_inited != OAL_TRUE) {
        mutex_init(&g_freq_lock_control.st_lock_freq_mtx);
        mutex_lock(&g_freq_lock_control.st_lock_freq_mtx);

        oam_warning_log4(0, OAM_SF_ANY,
            "{hmac_wifi_auto_freq_ctrl_init: orig_freq:: cpu_min[%d],cpu_max[%d],ddr_min[%d],ddr_max[%d]}",
            g_orig_cpu_min_freq, g_orig_cpu_max_freq, g_orig_ddr_min_freq, g_orig_ddr_max_freq);

        g_freq_lock_control.en_is_inited = OAL_TRUE;

        mutex_unlock(&g_freq_lock_control.st_lock_freq_mtx);
    }
#endif
}

void hmac_wifi_auto_freq_ctrl_deinit(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    mutex_lock(&g_freq_lock_control.st_lock_freq_mtx);

    if (g_freq_lock_control.en_is_inited == OAL_TRUE) {
    } else {
        oam_warning_log0(0, OAM_SF_ANY, "{hw_wifi_freq_ctrl_destroy freq lock has already been released!}");
    }

    mutex_unlock(&g_freq_lock_control.st_lock_freq_mtx);
    mutex_destroy(&g_freq_lock_control.st_lock_freq_mtx);

    g_freq_lock_control.en_is_inited = OAL_FALSE;
#endif
}

oal_bool_enum_uint8 hmac_wifi_rx_is_busy(void)
{
    return g_st_wifi_load.en_wifi_rx_busy;
}

#ifdef _PRE_FEATURE_PLAT_LOCK_CPUFREQ
#define HMAC_MAX_CPU_FREQ 2516000 // kHZ
OAL_STATIC uint32_t hmac_get_max_cpu_freq(uint8_t cpu_id)
{
#ifdef CONFIG_HI110X_SOFT_AP_LIMIT_CPU_FREQ
    if (g_freq_lock_control.limit_cpu_freq) {
        return oal_min(g_aul_cpumaxfreq[cpu_id].max_cpu_freq, HMAC_MAX_CPU_FREQ);
    }
#endif
    return g_aul_cpumaxfreq[cpu_id].max_cpu_freq;
}

OAL_STATIC OAL_INLINE void hmac_lock_max_cpu_freq(void)
{
    uint8_t uc_cpuid_loop;
    /* 所有核都锁定最高频率 */
    for (uc_cpuid_loop = 0; uc_cpuid_loop < OAL_BUS_MAXCPU_NUM; uc_cpuid_loop++) {
        /* 未获取到正确的cpu频率则不设置 */
        if (g_aul_cpumaxfreq[uc_cpuid_loop].valid != OAL_TRUE) {
            continue;
        }

        external_cpufreq_update_req(&g_ast_cpufreq[uc_cpuid_loop], hmac_get_max_cpu_freq(uc_cpuid_loop));
    }
}

OAL_STATIC OAL_INLINE void hmac_unlock_max_cpu_freq(void)
{
    uint8_t uc_cpuid_loop;
    for (uc_cpuid_loop = 0; uc_cpuid_loop < OAL_BUS_MAXCPU_NUM; uc_cpuid_loop++) {
        /* 未获取到正确的cpu频率则不设置 */
        if (g_aul_cpumaxfreq[uc_cpuid_loop].valid != OAL_TRUE) {
            continue;
        }

        external_cpufreq_update_req(&g_ast_cpufreq[uc_cpuid_loop], 0);
    }
}
#endif

OAL_STATIC OAL_INLINE void hmac_lock_cpu_freq_high_throughput_proc(void)
{
#ifdef _PRE_FEATURE_PLAT_LOCK_CPUFREQ
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (g_freq_lock_control.uc_lock_max_cpu_freq == OAL_TRUE) {
        /* 当前还存在锁频后频率会掉下来，并且后面也无法锁到最高频率，需要继续定位。后续需要调整锁频时间。
         * 以及确认是否每次需要重新req。 */
        core_ctl_set_boost(20 * WLAN_FREQ_TIMER_PERIOD * WLAN_THROUGHPUT_STA_PERIOD); // 20是锁频时间为单位时间的20倍
        hmac_lock_max_cpu_freq();
    }
#endif
#endif
}


void hmac_set_cpu_freq(uint8_t uc_req_freq_state)
{
#if defined(_PRE_PRODUCT_HI1620S_KUNPENG) || defined(_PRE_WINDOWS_SUPPORT)
    if (uc_req_freq_state == g_freq_lock_control.uc_cur_cpu_freq_state) {
        return;
    }

    g_freq_lock_control.uc_cur_cpu_freq_state = uc_req_freq_state;
#else
    if (uc_req_freq_state == g_freq_lock_control.uc_cur_cpu_freq_state) {
        if (uc_req_freq_state == WLAN_CPU_FREQ_SUPER) {
            /* sdio锁频后频率会掉下来，需要每次都执行锁频 */
            hmac_lock_cpu_freq_high_throughput_proc();
        }
        return;
    }

    g_freq_lock_control.uc_cur_cpu_freq_state = uc_req_freq_state;
    oam_warning_log1(0, OAM_SF_ANY, "{hmac_set_cpu_freq: cur state:%d! 1: super 0: idle}", uc_req_freq_state);

#ifdef _PRE_FEATURE_PLAT_LOCK_CPUFREQ
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (g_freq_lock_control.uc_lock_max_cpu_freq == OAL_TRUE) {
        if (uc_req_freq_state == WLAN_CPU_FREQ_SUPER) {
            /* 所有核都锁定最高频率 */
            hmac_lock_max_cpu_freq();
        } else {
            hmac_unlock_max_cpu_freq();
        }
    }

    if (g_freq_lock_control.uc_lock_dma_latency == OAL_TRUE) {
        if (uc_req_freq_state == WLAN_CPU_FREQ_SUPER) {
            /* 修改DMA latency,避免cpu进入过深的idle state */
            oal_pm_qos_update_request(&g_st_pmqos_requset, g_freq_lock_control.dma_latency_value);
        } else {
            oal_pm_qos_update_request(&g_st_pmqos_requset, PM_QOS_DEFAULT_VALUE);
        }
    }
#endif
#endif
#endif
}

#if defined(CONFIG_ARCH_HISI) && defined(CONFIG_NR_CPUS)
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
void hmac_userctl_bindcpu_rxdata(uint8_t thread_cpu, struct cpumask thread_cpu_mask)
{
    hmac_rxdata_thread_stru *rxdata_thread = hmac_get_rxdata_thread_addr();

    if (rxdata_thread->pst_rxdata_thread) {
        set_cpus_allowed_ptr(rxdata_thread->pst_rxdata_thread, &thread_cpu_mask);
        oam_warning_log1(0, OAM_SF_ANY, "{hmac_userctl_bindcpu_rxdata::hisi_rxdata bind CPU[0x%x]}", thread_cpu);
    }
}


static void hmac_bindcpu_txthread(const struct cpumask *thread_cpu_mask)
{
    hmac_tid_schedule_stru *tid_sched = hmac_get_tid_schedule_list();
    hmac_tx_comp_stru *tx_comp = hmac_get_tx_comp_mgmt();

    if (tid_sched->tid_schedule_thread) {
        set_cpus_allowed_ptr(tid_sched->tid_schedule_thread, thread_cpu_mask);
    }
    if (tx_comp->tx_comp_thread) {
        set_cpus_allowed_ptr(tx_comp->tx_comp_thread, thread_cpu_mask);
    }
}

#if !defined(_PRE_PRODUCT_HI1620S_KUNPENG) && !defined(_PRE_WINDOWS_SUPPORT)

static void hmac_bindcpu_rxthread(const struct cpumask *thread_cpu_mask, uint8_t uc_cpu_id)
{
    hmac_rxdata_thread_stru *rxdata_thread = hmac_get_rxdata_thread_addr();
    if (rxdata_thread->pst_rxdata_thread == NULL) {
        return;
    }

    rxdata_thread->uc_allowed_cpus = 0;
    if (set_cpus_allowed_ptr(rxdata_thread->pst_rxdata_thread, thread_cpu_mask) != EOK) {
        oam_warning_log2(0, OAM_SF_ANY, "{hmac_bindcpu_rxthread::cpumask fast[0x%x], cpu[0x%x]}",
            cpumask_bits(thread_cpu_mask)[0], uc_cpu_id);
        return;
    }
    /* 用于NAPI polling */
    rxdata_thread->uc_allowed_cpus = uc_cpu_id;
}


void hmac_rx_frw_task_bind_cpu(struct cpumask *cpus)
{
    oal_task_stru *frw_task_thread = get_frw_task();
    if (frw_task_thread == NULL) {
        return;
    }

    set_cpus_allowed_ptr(frw_task_thread, cpus);
}


OAL_STATIC void hmac_bind_wifi_thread(uint8_t uc_cpu_id)
{
    struct cpumask fast_cpus, all_cpus;
    external_get_fast_cpus(&fast_cpus);
    cpumask_setall(&all_cpus);

    if (uc_cpu_id >= WLAN_IRQ_AFFINITY_BUSY_CPU) {
        cpumask_clear_cpu(cpumask_first(&fast_cpus), &fast_cpus);

        hmac_bindcpu_rxthread(&fast_cpus, uc_cpu_id);
        hmac_bindcpu_txthread(&fast_cpus);
        /* hisi frw线程绑核 */
        wlan_chip_frw_task_bind_cpu(&fast_cpus);
    } else {
        /* CPU0上中断多, WIFI线程不放在CPU0 */
        cpumask_clear_cpu(cpumask_first(&all_cpus), &all_cpus);

        hmac_bindcpu_rxthread(&all_cpus, uc_cpu_id);
        hmac_bindcpu_txthread(&all_cpus);
        /* hisi frw线程绑核 */
        wlan_chip_frw_task_bind_cpu(&all_cpus);
    }

    oam_warning_log3(0, OAM_SF_ANY, "{hmac_bind_wifi_thread::set id %d cpumask fast[0x%x], all[0x%x]}",
        uc_cpu_id, cpumask_bits(&fast_cpus)[0], cpumask_bits(&all_cpus)[0]);
}


OAL_STATIC uint8_t hmac_wifi_thread_bind_check(uint8_t *thread_bind, uint8_t *irq_bind)
{
    struct cpumask fast_cpus, slow_cpus, all_cpus;
    external_get_slow_cpus(&slow_cpus);
    external_get_fast_cpus(&fast_cpus);
    cpumask_setall(&all_cpus);
    oam_warning_log3(0, OAM_SF_ANY, "{hmac_wifi_thread_bind_check::cpumask fast[0x%x], slow[0x%x], all[0x%x]}",
        cpumask_bits(&fast_cpus)[0], cpumask_bits(&slow_cpus)[0], cpumask_bits(&all_cpus)[0]);
    *thread_bind = 0;
    *irq_bind = 0;

    /* 策略1：不大于4核 */
    if (CONFIG_NR_CPUS <= OAL_BUS_HPCPU_NUM) {
        return HOST_CPU_BIND_STRATEGY_LT_4CPU;
    }
    /* 策略2：只有小核 */
    if (cpumask_empty(&fast_cpus) || cpumask_empty(&slow_cpus) || cpumask_equal(&fast_cpus, &slow_cpus)) {
        return HOST_CPU_BIND_STRATEGY_SMALL_CPU;
    }
    /* 策略3：只有1个中大核, 绑PCIE中断 */
    if (cpumask_weight(&fast_cpus) == 1) {
        *irq_bind = 1;
        return HOST_CPU_BIND_STRATEGY_1_BIG_CPU;
    }
    /* 策略4：只有2个中大核 */
    if (cpumask_weight(&fast_cpus) == 2) {
        *thread_bind = 1;
        return HOST_CPU_BIND_STRATEGY_2_BIG_CPU;
    }
    /* 策略5：3个及以上中大核 */
    if (cpumask_weight(&fast_cpus) >= 3) {
        *irq_bind = 1;
        *thread_bind = 1;
        return HOST_CPU_BIND_STRATEGY_GT_4CPU;
    }
    return HOST_CPU_BIND_STRATEGY_BUTT;
}
#endif
#endif
#endif


OAL_STATIC void hmac_auto_set_irq(uint8_t uc_cpu_id)
{
#if !defined(_PRE_PRODUCT_HI1620S_KUNPENG) && !defined(_PRE_WINDOWS_SUPPORT)
#if defined(CONFIG_ARCH_HISI) && defined(CONFIG_NR_CPUS)
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
    uint8_t thread_bind, irq_bind, ret;
#endif
#endif
#endif

    if (uc_cpu_id == g_freq_lock_control.uc_cur_irq_cpu) {
        return;
    }

    g_freq_lock_control.uc_cur_irq_cpu = uc_cpu_id;
    /* 定制化关闭 */
    if (g_freq_lock_control.en_irq_affinity == OAL_FALSE) {
        return;
    }

#if !defined(_PRE_PRODUCT_HI1620S_KUNPENG) && !defined(_PRE_WINDOWS_SUPPORT)
#if defined(CONFIG_ARCH_HISI) && defined(CONFIG_NR_CPUS)
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
    /* if need to bind */
    ret = hmac_wifi_thread_bind_check(&thread_bind, &irq_bind);
    oam_warning_log3(0, OAM_SF_ANY, "{hmac_wifi_thread_bind_check::ret[%d] thread[%d] irq[%d]}",
        ret, thread_bind, irq_bind);
    if ((thread_bind == 0) && (irq_bind == 0)) {
        return;
    }
    /* WIFI bus thread process */
    hi110x_bind_hcc_thread(((uc_cpu_id >= WLAN_IRQ_AFFINITY_BUSY_CPU) ? 1 : 0), thread_bind, irq_bind);
    /* WIFI thread process */
    if (thread_bind) {
        hmac_bind_wifi_thread(uc_cpu_id);
    }
#endif
#endif
#endif
}

#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
static mac_cfg_pk_mode_stru g_st_wifi_pk_mode_status = { 0 }; /* PK mode模式 */
OAL_STATIC OAL_INLINE void hmac_update_pk_mode_curr_protocol_cap(mac_user_stru *pst_mac_user)
{
    switch (pst_mac_user->en_protocol_mode) {
        case WLAN_LEGACY_11A_MODE:
        case WLAN_LEGACY_11B_MODE:
        case WLAN_LEGACY_11G_MODE:
        case WLAN_MIXED_ONE_11G_MODE:
        case WLAN_MIXED_TWO_11G_MODE:
            g_st_wifi_pk_mode_status.en_curr_protocol_cap = WLAN_PROTOCOL_CAP_LEGACY;
            break;
        case WLAN_HT_MODE:
        case WLAN_HT_ONLY_MODE:
        case WLAN_HT_11G_MODE:
            g_st_wifi_pk_mode_status.en_curr_protocol_cap = WLAN_PROTOCOL_CAP_HT;
            break;
        case WLAN_VHT_MODE:
        case WLAN_VHT_ONLY_MODE:
            g_st_wifi_pk_mode_status.en_curr_protocol_cap = WLAN_PROTOCOL_CAP_VHT;
            break;
#ifdef _PRE_WLAN_FEATURE_11AX
        case WLAN_HE_MODE:
            if (g_wlan_spec_cfg->feature_11ax_is_open) {
                g_st_wifi_pk_mode_status.en_curr_protocol_cap = WLAN_PROTOCOL_CAP_HE;
                break;
            }
            g_st_wifi_pk_mode_status.en_curr_protocol_cap = WLAN_PROTOCOL_CAP_BUTT;
            break;
#endif
        case WLAN_PROTOCOL_BUTT:
        default:
            g_st_wifi_pk_mode_status.en_curr_protocol_cap = WLAN_PROTOCOL_CAP_BUTT;
            break;
    }
}


void hmac_update_pk_mode_info(uint8_t *puc_vap_id)
{
    uint8_t uc_vap_id;
    mac_vap_stru *pst_mac_vap = NULL;
    mac_user_stru *pst_mac_user = NULL;
    oal_dlist_head_stru *pst_entry = NULL;
    oal_dlist_head_stru *pst_dlist_tmp = NULL;

    g_st_wifi_pk_mode_status.en_curr_bw_cap = WLAN_BW_CAP_BUTT;
    g_st_wifi_pk_mode_status.en_curr_protocol_cap = WLAN_PROTOCOL_CAP_BUTT;
    g_st_wifi_pk_mode_status.en_curr_num_spatial_stream = WLAN_SINGLE_NSS;

    for (uc_vap_id = 0; uc_vap_id < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id++) {
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(uc_vap_id);
        if (pst_mac_vap == NULL) {
            oam_error_log1(0, OAM_SF_ANY, "{hmac_update_pk_mode_info::pst_mac_vap(%d) is null.}", uc_vap_id);
            continue;
        }

        if (!IS_LEGACY_STA(pst_mac_vap)) {
            continue;
        }

        oal_dlist_search_for_each_safe(pst_entry, pst_dlist_tmp, &(pst_mac_vap->st_mac_user_list_head)) {
            pst_mac_user = oal_dlist_get_entry(pst_entry, mac_user_stru, st_user_dlist);
            if (pst_mac_user == NULL) {
                oam_error_log0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_update_pk_mode_info::pst_user_tmp null.}");
                continue;
            }

            /* 从用户的信息中更新目前带宽和协议模式 */
            /* 此设计目前仅适用支持一个LEGACY_STA且LEGACY_STA仅一个业务用户，若规格有修改此函数需要重构 */
            g_st_wifi_pk_mode_status.en_curr_bw_cap = pst_mac_user->en_bandwidth_cap;
            g_st_wifi_pk_mode_status.en_curr_num_spatial_stream = pst_mac_user->en_avail_num_spatial_stream;
            /* 嵌套深度优化封装 */
            hmac_update_pk_mode_curr_protocol_cap(pst_mac_user);

            *puc_vap_id = uc_vap_id;
        }
    }

    return;
}

OAL_STATIC oal_bool_enum_uint8 hmac_update_pk_mode_status_by_th(
    wlan_protocol_cap_enum_uint8 en_cur_protocol_cap, uint32_t txrx_throughput)
{
    uint32_t pk_mode_th;
    oal_bool_enum_uint8 en_curr_pk_mode;

    if (g_st_wifi_pk_mode_status.en_is_pk_mode == OAL_TRUE) {
        pk_mode_th = (g_st_pk_mode_low_th_table[en_cur_protocol_cap][g_st_wifi_pk_mode_status.en_curr_bw_cap]) *
            (1 + g_st_wifi_pk_mode_status.en_curr_num_spatial_stream);
        en_curr_pk_mode = (txrx_throughput < pk_mode_th) ? OAL_FALSE : OAL_TRUE;
    } else {
        pk_mode_th = (g_st_pk_mode_high_th_table[en_cur_protocol_cap][g_st_wifi_pk_mode_status.en_curr_bw_cap]) *
            (1 + g_st_wifi_pk_mode_status.en_curr_num_spatial_stream);
        en_curr_pk_mode = (txrx_throughput > pk_mode_th) ? OAL_TRUE : OAL_FALSE;
    }

    return en_curr_pk_mode;
}
OAL_STATIC oal_bool_enum_uint8 hmac_pk_mode_is_not_supp(mac_vap_stru *mac_vap)
{
    if ((mac_vap->en_protocol != WLAN_VHT_MODE) &&
        (mac_vap->en_protocol != WLAN_VHT_ONLY_MODE)) {
        if (g_wlan_spec_cfg->feature_11ax_is_open) {
#ifdef _PRE_WLAN_FEATURE_11AX
            if (mac_vap->en_protocol != WLAN_HE_MODE) {
                return OAL_TRUE;
            }
#endif
        } else {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}
void hmac_update_fill_pk_mode_info(mac_cfg_pk_mode_stru *pk_mode_info)
{
    pk_mode_info->rx_bytes = g_st_wifi_pk_mode_status.rx_bytes;
    pk_mode_info->tx_bytes = g_st_wifi_pk_mode_status.tx_bytes;
    pk_mode_info->dur_time = g_st_wifi_pk_mode_status.dur_time;
    pk_mode_info->en_is_pk_mode = g_st_wifi_pk_mode_status.en_is_pk_mode;
    pk_mode_info->en_curr_bw_cap = g_st_wifi_pk_mode_status.en_curr_bw_cap;
    pk_mode_info->en_curr_protocol_cap = g_st_wifi_pk_mode_status.en_curr_protocol_cap;
    pk_mode_info->en_curr_num_spatial_stream = g_st_wifi_pk_mode_status.en_curr_num_spatial_stream;
}


OAL_STATIC oal_bool_enum_uint8 hmac_update_pk_mode_hisi(mac_vap_stru *mac_vap)
{
    hmac_user_stru *hmac_user = NULL;

    if (mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        return OAL_FALSE;
    }

    hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(mac_vap->us_assoc_vap_id);
    if (hmac_user == NULL) {
        oam_warning_log0(mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_update_pk_mode_hisi::get_mac_user null!}");
        return OAL_FALSE;
    }

    return hmac_user->user_hisi;
}

static void hmac_update_pk_mode_set_status(oal_bool_enum_uint8 curr_pk_mode,
    uint32_t tx_throughput, uint32_t rx_throughput, uint32_t dur_time)
{
    g_st_wifi_pk_mode_status.en_is_pk_mode = curr_pk_mode;
    g_st_wifi_pk_mode_status.rx_bytes = rx_throughput;
    g_st_wifi_pk_mode_status.tx_bytes = tx_throughput;
    g_st_wifi_pk_mode_status.dur_time = dur_time;
}


void hmac_update_pk_mode(uint32_t tx_throughput,
    uint32_t rx_throughput,  uint32_t dur_time)
{
    oal_bool_enum_uint8 en_curr_pk_mode;
    uint8_t uc_vap_id;
    mac_cfg_pk_mode_stru st_pk_mode_info;
    mac_vap_stru *pst_mac_vap = NULL;
    uint32_t ret, txrx_throughput;
    mac_device_stru *pst_mac_device = NULL;
    wlan_protocol_cap_enum_uint8 en_cur_protocol_cap;

    if (g_en_pk_mode_swtich == OAL_FALSE) {
        return;
    }

    pst_mac_device = mac_res_get_dev(0);
    /* 如果非单VAP,则不开启硬件聚合 */
    if (mac_device_calc_up_vap_num(pst_mac_device) != 1) {
        return;
    }

    /* 更新相关的信息 */
    hmac_update_pk_mode_info(&uc_vap_id);

    /* 非有效模式，不进行吞吐计算 */
    if ((g_st_wifi_pk_mode_status.en_curr_bw_cap == WLAN_BW_CAP_BUTT) ||
        (g_st_wifi_pk_mode_status.en_curr_protocol_cap == WLAN_PROTOCOL_CAP_BUTT)) {
        return;
    }

    /* 计算吞吐率: rxtx吞吐之和 / 统计的时间间隔(单位s) */
    txrx_throughput = tx_throughput + rx_throughput;

    en_cur_protocol_cap = g_st_wifi_pk_mode_status.en_curr_protocol_cap;

    /* 依据门限值判定pk mode状态 */
    en_curr_pk_mode = hmac_update_pk_mode_status_by_th(en_cur_protocol_cap, txrx_throughput);
    /* PK mode状态未变化，不更新数值 */
    if (en_curr_pk_mode == g_st_wifi_pk_mode_status.en_is_pk_mode) {
        return;
    }

    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(uc_vap_id);
    if (pst_mac_vap == NULL) {
        oam_error_log1(0, OAM_SF_ANY, "{hmac_update_pk_mode::pst_mac_vap(%d) is null.}", uc_vap_id);
        return;
    }
    /* 避免ddut Hi1103/1105时调整edca参数 */
    if (hmac_pk_mode_is_not_supp(pst_mac_vap) || (hmac_update_pk_mode_hisi(pst_mac_vap) == OAL_TRUE)) {
        return;
    }
    oam_warning_log4(0, OAM_SF_ANY,
                     "{hmac_update_pk_mode: pk_mode changed (%d) -> (%d), tx_throughput[%d], rx_throughput[%d]!}",
                     g_st_wifi_pk_mode_status.en_is_pk_mode, en_curr_pk_mode, tx_throughput, rx_throughput);
    hmac_update_pk_mode_set_status(en_curr_pk_mode, tx_throughput, rx_throughput, dur_time);
    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    hmac_update_fill_pk_mode_info(&st_pk_mode_info);

    ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SYNC_PK_MODE,
        sizeof(mac_cfg_pk_mode_stru), (uint8_t *)(&st_pk_mode_info));
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_update_pk_mode::hmac_config_send_event failed[%d].}", ret);
    }

    return;
}
#endif


OAL_STATIC void hmac_calc_throughput_cpu_freq(
    uint32_t tx_throughput_mbps, uint32_t rx_throughput_mbps)
{
    uint16_t us_cpu_freq_high = WLAN_CPU_FREQ_THROUGHPUT_THRESHOLD_HIGH;
    uint16_t us_cpu_freq_low = WLAN_CPU_FREQ_THROUGHPUT_THRESHOLD_LOW;

    /* 每秒吞吐量门限 */
    if ((g_freq_lock_control.us_lock_cpu_th_high) && (g_freq_lock_control.us_lock_cpu_th_low)) {
        us_cpu_freq_high = g_freq_lock_control.us_lock_cpu_th_high;
        us_cpu_freq_low = g_freq_lock_control.us_lock_cpu_th_low;
    }
#ifdef _PRE_WLAN_CHBA_MGMT
    /* 存在CHBA vap时，降低锁频门限为600M */
    hmac_chba_set_freq_lock_th(&us_cpu_freq_high, &us_cpu_freq_low);
#endif
    /* 高于800M时切超频，低于500M恢复 */
    if ((tx_throughput_mbps > us_cpu_freq_high) ||
        (rx_throughput_mbps > us_cpu_freq_high)) {
        g_freq_lock_control.uc_req_super_cpu_freq = WLAN_CPU_FREQ_SUPER;
    } else if ((tx_throughput_mbps < us_cpu_freq_low) &&
               (rx_throughput_mbps < us_cpu_freq_low)) {
        g_freq_lock_control.uc_req_super_cpu_freq = WLAN_CPU_FREQ_IDLE;
    }

    hmac_set_cpu_freq(g_freq_lock_control.uc_req_super_cpu_freq);
}

void hmac_get_perform_limit_value(uint32_t *limit_throughput_high, uint32_t *limit_throughput_low,
    uint32_t *limit_pps_high, uint32_t *limit_pps_low)
{
    /* 每秒吞吐量门限 */
    if ((g_freq_lock_control.us_throughput_irq_high) && (g_freq_lock_control.us_throughput_irq_low)) {
        *limit_throughput_high = g_freq_lock_control.us_throughput_irq_high;
        *limit_throughput_low = g_freq_lock_control.us_throughput_irq_low;
    }

    /* 每秒PPS门限 */
    if ((g_freq_lock_control.irq_pps_high) && (g_freq_lock_control.irq_pps_low)) {
        *limit_pps_high = g_freq_lock_control.irq_pps_high;
        *limit_pps_low = g_freq_lock_control.irq_pps_low;
    }
}

void hmac_calc_throughput_cpu_mask(void)
{
#ifdef _PRE_WLAN_FEATURE_LOW_LATENCY_SWITCHING
    if (g_freq_lock_control.uc_req_irq_cpu == WLAN_IRQ_AFFINITY_IDLE_CPU) {
        /*
        * Auto-adjusting code detected low network load but instead of
        * switching to small cores immediately, check first if the
        * low-latency configuration is set.
        */
        if (g_freq_lock_control.en_low_latency_bindcpu == OAL_TRUE) {
            g_freq_lock_control.uc_req_irq_cpu = WLAN_IRQ_AFFINITY_BUSY_CPU;
        }
    }
#endif

    hmac_auto_set_irq(g_freq_lock_control.uc_req_irq_cpu);
}


void hmac_perform_calc_throughput(uint32_t tx_throughput_mbps, uint32_t rx_throughput_mbps, uint32_t trx_pps)
{
    uint32_t limit_throughput_high = WLAN_IRQ_THROUGHPUT_THRESHOLD_HIGH;
    uint32_t limit_throughput_low = WLAN_IRQ_THROUGHPUT_THRESHOLD_LOW;
    uint32_t limit_pps_high = WLAN_IRQ_PPS_THRESHOLD_HIGH;
    uint32_t limit_pps_low = WLAN_IRQ_PPS_THRESHOLD_LOW;

    hmac_get_perform_limit_value(&limit_throughput_high, &limit_throughput_low,
                                 &limit_pps_high, &limit_pps_low);
    /* 高于200M时切大核，低于150M切小核 */
    if ((tx_throughput_mbps > limit_throughput_high) ||
        (rx_throughput_mbps > limit_throughput_high) || (trx_pps >= limit_pps_high)) {
        g_freq_lock_control.uc_req_irq_cpu = WLAN_IRQ_AFFINITY_BUSY_CPU;
    } else if ((tx_throughput_mbps < limit_throughput_low) &&
               (rx_throughput_mbps < limit_throughput_low) && (trx_pps < limit_pps_low)) {
        g_freq_lock_control.uc_req_irq_cpu = WLAN_IRQ_AFFINITY_IDLE_CPU;
    }

    if (g_freq_lock_control.uc_req_irq_cpu != g_freq_lock_control.uc_cur_irq_cpu) {
        oam_warning_log4(0, OAM_SF_ANY,
            "{hmac_perform_calc_throughput:throughput rx = %d , tx = %d, high_th = %d,low_th = %d!}",
            rx_throughput_mbps, tx_throughput_mbps, limit_throughput_high, limit_throughput_low);
        oam_warning_log3(0, OAM_SF_ANY, "{hmac_perform_calc_throughput: rx pps = %d , tx pps = %d, trx pps = %d,!}",
            g_freq_lock_control.rx_pps, g_freq_lock_control.tx_pps, trx_pps);
        oam_warning_log4(0, OAM_SF_ANY,
            "{hmac_perform_calc_throughput: req cpu id[%d],cur cpu id[%d],pps_high = %d, pps low = %d!}",
            g_freq_lock_control.uc_req_irq_cpu, g_freq_lock_control.uc_cur_irq_cpu,
            limit_pps_high, limit_pps_low);
    }

    /* 自适应绑核 */
    hmac_calc_throughput_cpu_mask();
    /* 自适应调频 */
    hmac_calc_throughput_cpu_freq(tx_throughput_mbps, rx_throughput_mbps);
}

OAL_STATIC oal_bool_enum_uint8 hmac_adjust_throughput_stat_txrx_bytes(uint32_t dur_time)
{
    /* 小于一个周期,不统计 */
    if (dur_time < WLAN_FREQ_TIMER_PERIOD - 10) { // 10是当前定时器周期需要减去10ms为一个周期
        return OAL_FALSE;
    }

    /* 超过一定时常则不统计,排除定时器异常 */
    if (dur_time > ((WLAN_FREQ_TIMER_PERIOD * WLAN_THROUGHPUT_STA_PERIOD) << BIT_OFFSET_2)) {
        hmac_device_stat_stru *hmac_device_stats = hmac_stat_get_device_stats();
        g_freq_lock_control.pre_time = jiffies;
        g_freq_lock_control.uc_timer_cycles = 0;

        hmac_device_stats->total_rx_bytes += (uint64_t)oal_atomic_read(&hmac_device_stats->rx_bytes);
        hmac_device_stats->total_tx_bytes += (uint64_t)oal_atomic_read(&hmac_device_stats->tx_bytes);
        oal_atomic_set(&hmac_device_stats->rx_bytes, 0);
        oal_atomic_set(&hmac_device_stats->tx_bytes, 0);

        return OAL_FALSE;
    }

    return OAL_TRUE;
}

static void hmac_tx_sched_lowpower_lowdelay_strategy(uint32_t tx_pps)
{
    uint8_t lowpower = OAL_FALSE;
    mac_vap_stru *cfg_vap = mac_res_get_mac_vap(0);
    hmac_psm_tx_switch_event_enum ring_state = hmac_psm_tx_switch_throughput_to_event(tx_pps);
    if (ring_state == PSM_TX_H2D_SWITCH) {
        lowpower = OAL_TRUE;
    }
    if (g_lowpower_strategy_swtich == lowpower) {
        return;
    }
    g_lowpower_strategy_swtich = lowpower;
    hmac_config_send_event(cfg_vap, WLAN_CFGID_LOW_DELAY_STRATEGY, sizeof(uint8_t), (uint8_t *)(&lowpower));
}

static void hmac_update_pps_switch(uint32_t tx_throughput_mbps, uint32_t rx_throughput_mbps, uint32_t dur_time)
{
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    /* AMSDU+AMPDU聚合切换入口 */
    hmac_tx_amsdu_ampdu_switch(tx_throughput_mbps, rx_throughput_mbps);
#endif
    hmac_tx_small_amsdu_switch(rx_throughput_mbps, g_freq_lock_control.tx_pps);

#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
    hmac_tx_tcp_ack_buf_switch(rx_throughput_mbps);
#endif

    hmac_tx_sched_lowpower_lowdelay_strategy(g_freq_lock_control.tx_pps + g_freq_lock_control.host_rx_pps);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (hmac_tx_keep_host_switch() == OAL_FALSE) {
        /* tx切换使用chip级pps, 因为如果rx_pps不为0，说明rx已经切到host, 这时tx再切到device 对低功耗并没有意义 */
        hmac_tx_pps_switch(g_freq_lock_control.tx_pps + g_freq_lock_control.host_rx_pps);
    }
    hmac_tx_sche_switch_process(tx_throughput_mbps);
#endif

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    /* 聚合软硬件切换入口,每个周期探测一次 */
    hmac_tx_ampdu_switch(tx_throughput_mbps);
#endif
#ifdef _PRE_WLAN_TCP_OPT
    hmac_tcp_ack_filter_switch(rx_throughput_mbps);
#endif

    /* 根据吞吐量，判断是否需要bypass 外置LNA */
    hmac_rx_dyn_bypass_extlna_switch(tx_throughput_mbps, rx_throughput_mbps);

    /* 中断动态调整 */
    hmac_perform_calc_throughput(tx_throughput_mbps, rx_throughput_mbps, g_freq_lock_control.total_sdio_pps);
#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
    /* 更新pk mode */
    hmac_update_pk_mode(tx_throughput_mbps, rx_throughput_mbps, dur_time);
#endif
}


void hmac_adjust_throughput(void)
{
    uint32_t tx_throughput, rx_throughput, dur_time;
    uint32_t tx_throughput_mbps = 0;
    uint32_t rx_throughput_mbps = 0;
    hmac_device_stat_stru *hmac_device_stats = hmac_stat_get_device_stats();

    /* 循环次数统计 */
    g_freq_lock_control.uc_timer_cycles++;

    dur_time = oal_jiffies_to_msecs(jiffies - g_freq_lock_control.pre_time);
    if (!hmac_adjust_throughput_stat_txrx_bytes(dur_time)) {
        return;
    }

    /* 总字节数 */
    rx_throughput = (uint32_t)oal_atomic_read(&hmac_device_stats->rx_bytes);
    tx_throughput = (uint32_t)oal_atomic_read(&hmac_device_stats->tx_bytes);
    if (dur_time != 0) {
        tx_throughput_mbps = (tx_throughput >> HMAC_TX_THROUGHPUT_OFFSET_7) / dur_time;
        rx_throughput_mbps = (rx_throughput >> HMAC_TX_THROUGHPUT_OFFSET_7) / dur_time;
    }

    g_st_wifi_load.en_wifi_rx_busy = (rx_throughput_mbps <= WLAN_THROUGHPUT_LOAD_LOW) ? OAL_FALSE : OAL_TRUE;
    if (g_freq_lock_control.uc_trx_stat_log_en == OAL_TRUE) {
        oam_warning_log4(0, 0, "{hmac_adjust_throughput::rx[%d],tx[%d],smooth[%d],dur_time[%d]ms!}", rx_throughput_mbps,
            tx_throughput_mbps, mac_vap_get_tcp_ack_buf_switch()->us_tcp_ack_smooth_throughput, dur_time);
    }

    hmac_update_pps_switch(tx_throughput_mbps, rx_throughput_mbps, dur_time);

    /* irq 2s 探测一次 */
    if (g_freq_lock_control.uc_timer_cycles < WLAN_THROUGHPUT_STA_PERIOD) {
        return;
    }
    g_freq_lock_control.uc_timer_cycles = 0;

    /* 2s周期清零一次 */
    hmac_device_stats->total_rx_bytes += (uint64_t)oal_atomic_read(&hmac_device_stats->rx_bytes);
    hmac_device_stats->total_tx_bytes += (uint64_t)oal_atomic_read(&hmac_device_stats->tx_bytes);
    oal_atomic_set(&hmac_device_stats->rx_bytes, 0);
    oal_atomic_set(&hmac_device_stats->tx_bytes, 0);

    g_freq_lock_control.pre_time = jiffies;
}


OAL_STATIC uint32_t hmac_wlan_freq_wdg_timeout(void *p_arg)
{
    /* 计算pps */
    hmac_pps_calc();

    /* 吞吐统计 */
    hmac_adjust_throughput();
#ifdef _PRE_WLAN_CHBA_MGMT
    hmac_chba_update_throughput();
#endif
    return OAL_SUCC;
}


void hmac_freq_timer_init(void)
{
    hmac_device_stat_stru *hmac_device_stats = NULL;
    /* 如果这个定时器已经注册成功，则不能再次被注册，不然后发生错误！ */
    if (g_freq_lock_control.hmac_freq_timer.en_is_registerd == OAL_TRUE) {
        return;
    }

    /* 重启定时器后，清空之前的统计 */
    g_freq_lock_control.pre_time = jiffies;
    hmac_device_stats = hmac_stat_get_device_stats();
#ifdef _PRE_WLAN_CHBA_MGMT
    /* 重启定时器后, 清空chba之前的统计 */
    hmac_chba_throughput_statis_init();
#endif

    hmac_device_stats->total_rx_packets += (uint32_t)oal_atomic_read(&hmac_device_stats->rx_packets);
    hmac_device_stats->total_rx_bytes += (uint64_t)oal_atomic_read(&hmac_device_stats->rx_bytes);
    hmac_device_stats->total_tx_packets += (uint32_t)oal_atomic_read(&hmac_device_stats->tx_packets);
    hmac_device_stats->total_tx_bytes += (uint64_t)oal_atomic_read(&hmac_device_stats->tx_bytes);
    oal_atomic_set(&hmac_device_stats->rx_packets, 0);
    oal_atomic_set(&hmac_device_stats->rx_bytes, 0);
    oal_atomic_set(&hmac_device_stats->tx_packets, 0);
    oal_atomic_set(&hmac_device_stats->tx_bytes, 0);
    oal_atomic_set(&hmac_device_stats->host_ring_rx_packets, 0);
    frw_timer_create_timer_m(&g_freq_lock_control.hmac_freq_timer,  // pst_timeout
                             hmac_wlan_freq_wdg_timeout,                                   // p_timeout_func
                             WLAN_FREQ_TIMER_PERIOD,                                       // timeout
                             NULL,                                                 // p_timeout_arg
                             OAL_TRUE,                                                     // en_is_periodic
                             OAM_MODULE_ID_HMAC, 0);                                       // en_module_id && core_id
}


void hmac_freq_timer_deinit(void)
{
    /* 定时器没有注册，就不用删除了，不然会发生错误！ */
    if (g_freq_lock_control.hmac_freq_timer.en_is_registerd == OAL_FALSE) {
        return;
    }
    /* 非低时延业务,说明业务流量比较低, 才恢复绑核 */
    g_freq_lock_control.uc_req_irq_cpu = WLAN_IRQ_AFFINITY_IDLE_CPU;
    hmac_calc_throughput_cpu_mask();

    hmac_set_cpu_freq(WLAN_CPU_FREQ_IDLE);
    frw_timer_immediate_destroy_timer_m(&g_freq_lock_control.hmac_freq_timer);
}

void hmac_wifi_pm_state_notify(oal_bool_enum_uint8 en_wake_up)
{
    if (en_wake_up == OAL_TRUE) {
        /* WIFI唤醒,启动吞吐量统计定时器 */
        hmac_freq_timer_init();
        hmac_tid_update_timer_init();
    } else {
        /* WIFI睡眠,关闭吞吐量统计定时器 */
        hmac_freq_timer_deinit();
        hmac_tid_update_timer_deinit();
    }
}

void hmac_userctl_bindcpu_get_cmd(uint32_t cmd)
{
}
#ifdef _PRE_WLAN_FEATURE_LOW_LATENCY_SWITCHING
/*
 * This function works like `hmac_userctl_bindcpu_get_cmd` but it is called for
 * low-latency binding reasons
 */
static void hmac_low_latency_bindcpu(uint32_t cmd)
{
    uint8_t irq_cmd;
    uint8_t thread_cmd;

    /* IRQ binding is encoded in upper bits, thread binding in lower */
    irq_cmd = (uint8_t)((uint16_t)cmd >> 8); /* 右移8位获取高位硬中断命令 */
    thread_cmd = (uint8_t)((uint16_t)cmd & 0xff);
    /* IRQ affinity must be set to a single CPU core */
    if (irq_cmd_is_onecpu(irq_cmd)) {
        oam_warning_log1(0, OAM_SF_ANY, "{hmac_low_latency_bindcpu::"
                         "irq_cmd[0x%x] is not single core}",
                         irq_cmd);
        return;
    }

    if (!irq_cmd && !thread_cmd) {
        /*
         * This branch is used to return back to auto-adjusting based on PPS
         * packet load and throughput estimations
         */
        g_freq_lock_control.uc_low_latency_irqbind    = 0;
        g_freq_lock_control.uc_low_latency_threadbind = 0;
        g_freq_lock_control.en_low_latency_bindcpu = OAL_FALSE;
    } else {
        /* This branch is used to set low-latency binding */
        g_freq_lock_control.uc_low_latency_irqbind    = irq_cmd;
        g_freq_lock_control.uc_low_latency_threadbind = thread_cmd;
        g_freq_lock_control.en_low_latency_bindcpu    = OAL_TRUE;
    }
}

void hmac_low_latency_bindcpu_fast(void)
{
    uint8_t irq_cmd;
    uint8_t thread_cmd;
    uint32_t command;

    /* set IRQ affinity to special CPU core */
    irq_cmd = 1 << WLAN_IRQ_AFFINITY_BUSY_CPU;

    /*
     * Note: bind driver threads to middle CPU cores (4-5) to achieve lower
     * latency with slightly increased power consumption but check the
     * possibility of scenarios which require both low latency and high
     * throughput. In these scenarios we should consider binding to fast cores
     * here.
     * latency with slightly increased power consumption.
     */
    thread_cmd = 0x30; /* 0x30表示cpu中核4-5 */

    /* construct bit command */
    command = (irq_cmd << 8) | thread_cmd; /* 右移8位获取高位硬中断命令 */
    hmac_low_latency_bindcpu(command);
}

void hmac_low_latency_bindcpu_default(void)
{
    hmac_low_latency_bindcpu(0);
}

#endif

void hmac_wifi_state_notify(oal_bool_enum_uint8 en_wifi_on)
{
    if (en_wifi_on == OAL_TRUE) {
        hal_host_intr_regs_init(HAL_DEVICE_ID_MASTER);
        if (hmac_host_ring_tx_enabled() == OAL_TRUE) {
            hal_host_ring_tx_init(HAL_DEVICE_ID_MASTER);
        }
#ifdef _PRE_WLAN_FEATURE_FTM
        hal_host_ftm_reg_init(HAL_DEVICE_ID_MASTER);
#endif
        /* WIFI上电,启动吞吐量统计定时器,打开调频功能 */
        hmac_freq_timer_init();
        hmac_tid_update_timer_init();
#ifdef _PRE_WLAN_FEATURE_LOW_LATENCY_SWITCHING
        hmac_low_latency_wifi_enable();
#endif
    } else {
#ifdef _PRE_WLAN_FEATURE_LOW_LATENCY_SWITCHING
        hmac_low_latency_wifi_disable();
#endif
        frw_event_process_all_event_when_wlan_close();
        /* WIFI下电,关闭吞吐量统计定时器,关闭调频功能 */
        hmac_freq_timer_deinit();
        hmac_tid_update_timer_deinit();
#ifdef _PRE_CONFIG_HISIFASTPOWER_DOWN_STATE
        /* WIFI下电, 停止正在进行的wifi业务 */
        hmac_config_change_wifi_state_for_power_down();
#endif
        hal_host_device_exit(0);
        hal_host_device_exit(1);
    }
}

#ifdef CONFIG_HI110X_SOFT_AP_LIMIT_CPU_FREQ
void hmac_cpu_freq_upper_limit_switch(oal_bool_enum_uint8 limit)
{
    g_freq_lock_control.limit_cpu_freq = limit;
    oam_warning_log1(0, OAM_SF_ANY, "{hmac_cpu_freq_upper_limit_switch::limit[%d]}", limit);
}
#endif
