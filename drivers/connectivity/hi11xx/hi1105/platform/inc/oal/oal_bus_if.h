

#ifndef __OAL_BUS_IF_H__
#define __OAL_BUS_IF_H__

/* 其他头文件包含 */
#include "oal_mem.h"
#include "oal_schedule.h"
#include "oal_util.h"

/* 宏定义 */
#define OAL_MAX_BAR_NUM 6 /* 每个设备最大BAR数量 */

#define OAL_1151_IRQ_NAME_BUFF_SIZE 4

/* 总线设备，DEVICE级 */
typedef struct {
    uint32_t ul_device_id; /* 设备号,如0x1151 */
    void *p_dev;         /* 指向设备的指针 */
    uint32_t ul_irq_num;   /* 设备中断号 */
    uint8_t uc_chip_id;    /* PCIe号，如PCIe0 */
    uint8_t uc_bar_num;    /* BAR数量 */
    uint8_t auc_resv[2];
    uint32_t aul_mem_start[1];
    oal_irq_dev_stru st_irq_info;
} oal_bus_dev_stru;

/* 总线设备，CHIP级 */
typedef struct {
    uint8_t uc_chip_id;    /* PCIe号，如PCIe0 */
    uint8_t uc_device_num; /* 该芯片中device个数，应该小于WLAN_DEVICE_MAX_NUM_PER_CHIP */
    uint8_t auc_resv[2];
    oal_pci_dev_stru *pst_pci_device; /* 指向所属PCI设备的指针 */
    void *p_pci_dbi_base;         /* 指向PCI设备DBI接口的指针，用于配置PCIE内部寄存器 */
    uint32_t aul_mem_start[1];

    /* device信息 */
    oal_bus_dev_stru st_bus_dev[WLAN_DEVICE_MAX_NUM_PER_CHIP];
} oal_bus_chip_stru;

/* 函数声明 */
extern uint8_t oal_bus_get_chip_num(void);
extern uint32_t oal_bus_inc_chip_num(void);
extern void oal_bus_init_chip_num(void);
#endif /* end of oal_bus_if.h */
