#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/param.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <asm/irq.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/pm_wakeup.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/workqueue.h>
#include <linux/bitops.h>
#include "port.h"
#include "../Platform_Linux/platform_helpers.h"
#include "hw_scp.h"
#include "core.h"
#include "../Platform_Linux/fusb3601_global.h"
#include "ana_hs_kit/ana_hs.h"
#include <chipset_common/hwpower/hardware_channel/wired_channel_switch.h>
#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger/direct_charger.h>
#endif
#include <linux/hisi/usb/chip_usb.h>
#include <linux/power/platform/charger/hisi_charger_scp.h>
#ifdef CONFIG_USB_ANALOG_HS_INTERFACE
#include <huawei_platform/audio/usb_analog_hs_interface.h>
#endif

#define FUSB3601_PWCTRL   0x1c
#define FUSB3601_AUTO_DISCH_EN 0x10
#define FUSB3601_AUTO_DISCH_DIS 0xef
#define FUSB3601_COMMAND  0x23
#define FUSB3601_DISABLE_SINK_VBUS  0x44
#define FUSB3601_DISABLE_VBUS_DET  0x22
#define FUSB3601_ENABLE_VBUS_DET  0x33
#define FUSB3601_SINK_VBUS  0x55

#define ACCP_CMD_SBRRD  0x0c
#define ACCP_CMD_SBRWR  0x0b
#define SCP_ACK_AND_NACK_MASK 0x28
#define SCP_ACK_MASK 0x20
#define FUSB3601_REG_ACCP_RT_CMD   0xa0
#define FUSB3601_REG_ACCP_RT_ADDR  0xa1
#define FUSB3601_REG_ACCP_RT_TX0 0xa2
#define FUSB3601_REG_ACCP_RT_ACK_RX  0xc0
#define FUSB3601_REG_ACCP_RT_RX0  0xc1
#define FUSB3601_REG_ACCP_RT_RX1  0xc2
#define FUSB3601_SCP_ENABLE1 0x80
#define FUSB3601_CHIP_RESET 0x40
#define FUSB3601_SCP_ENABLE1_INIT 0x80
#define FUSB3601_SCP_ENABLE2 0x81
#define FUSB3601_MST_RESET_MSK 0x02
#define FUSB3601_SCP_ENABLE2_INIT 0xa1
#define FUSB3601_DPD_DISABLE 0x81
#define FUSB3601_SCP_INT1_MASK 0x82
#define FUSB3601_SCP_INT1_MASK_INIT 0x00
#define FUSB3601_SCP_INT2_MASK 0x83
#define FUSB3601_SCP_INT2_MASK_INIT 0x00
#define FUSB3601_TIMER_SET1 0x84
#define FUSB3601_TIMER_SET1_INIT 0x00
#define FUSB3601_TIMER_SET2 0x85
#define FUSB3601_TIMER_SET2_INIT 0x00
#define FUSB3601_MUS_INTERRUPT_MASK 0xd4
#define FUSB3601_MUS_INTERRUPT_MASK_INIT 0x1b
#define FUSB3601_MUS_CONTROL1 0xd1
#define FUSB3601_DCD_TIMEOUT_DISABLE 0xef
#define FUSB3601_DEVICE_TYPE 0xd6
#define FUSB3601_SCP_EVENT_1 0x86
#define FUSB3601_SCP_EVENT1_CC_PLGIN_DPDN_PLGIN 0x60
#define FUSB3601_SCP_EVENT_2 0x87
#define FUSB3601_SCP_EVENT2_ACK 0x20
#define FUSB3601_SCP_EVENT_3 0x88
#define FUSB3601_FM_CONTROL1 0xdc
#define FUSB3601_MSM_EN_HIGH 0xfe
#define FUSB3601_MSM_EN_LOW 0xfd
#define FUSB3601_FM_CONTROL3 0xde
#define FUSB3601_FM_CONTROL4 0xdf
#define FUSB3601_DIS_VBUS_DETECTION_MSK 0xdf
#define FUSB3601_VOUT_ENABLE 0x7b
#define FUSB3601_VOUT_DISABLE 0x73
#define MUS_CONTRAL2 0xd2
#define FUSB3601_DCP_DETECT 0x40
#define FUSB3601_SCP_OR_FCP_DETECT 0x19
#define FUSB3601_SCP_B_DETECT 0x08

static struct mutex FUSB3601_accp_detect_lock;
static struct mutex FUSB3601_accp_adaptor_reg_lock;

static int wired_channel_status = WIRED_CHANNEL_RESTORE;

struct delayed_work m_work;
static int FUSB3601_is_support_scp(void);
#define HWLOG_TAG FUSB3601_scp
HWLOG_REGIST();

static u32 FUSB3601_scp_error_flag = 0;/*scp error flag*/

static int FUSB3601_is_support_fcp(void);
void FUSB3601_scp_initialize(void);
static int FUSB3601_scp_get_adapter_vendor_id(void);
extern int state_machine_need_resched;
static int FUSB3601_accp_adapter_reg_write(int val, int reg);
static int FUSB3601_accp_adapter_reg_read(int* val, int reg);
/****************************************************************************
  Function:     FUSB3601_fcp_stop_charge_config
  Description:  fcp stop charge config
  Input:         NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static void FUSB3601_clear_scp_event1(void)
{
	FSC_U8 reg_val1 = 0;
	int ret;

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_1,&reg_val1);
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_EVENT_1, 1, &reg_val1);
	if (!ret)
	    hwlog_info("%s:i2c error\n", __func__);
}
static int FUSB3601_fcp_stop_charge_config(void *dev_data)
{
#ifdef CONFIG_DIRECT_CHARGER
	FUSB3601_clear_scp_event1();
#endif
    return 0;
}
int FUSB3601_vout_enable(int enable)
{
	int ret;
	FSC_U8 data = 0;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	struct Port *port;

	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}
	port = &chip->port;
	if (!port) {
		pr_err("FUSB  %s - port structure is NULL!\n", __func__);
		return -1;
	}
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_FM_CONTROL3,&data);
	hwlog_info("%s:FM_CONTROL3 befor writing is : [0x%x], ret = %d\n", __func__, data,ret);
	if (enable) {
		data =FUSB3601_VOUT_ENABLE;
	} else {
		data =FUSB3601_VOUT_DISABLE;
	}
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_FM_CONTROL3, 1, &data);
	if (!ret)
		hwlog_err("i2c write failure\n");
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_FM_CONTROL3,&data);
	if (!ret)
		hwlog_err("i2c read failure\n");
	hwlog_info("%s:FM_CONTROL3 after writing is : [0x%x], ret = %d\n", __func__, data,ret);
	if (ret)
		return 0;
	else
		return -1;

}
int FUSB3601_msw_enable(int enable)
{
	int ret;
	FSC_U8 data;

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_FM_CONTROL1,&data);
	hwlog_info("%s:FM_CONTROL1 befor writing is : [0x%x], ret = %d\n", __func__, data,ret);
	if (enable) {
		data =FUSB3601_MSM_EN_HIGH;
	} else {
		data =FUSB3601_MSM_EN_LOW;
	}
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_FM_CONTROL1, 1, &data);
	if (!ret)
		hwlog_err("i2c write failure\n");
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_FM_CONTROL1,&data);
	if (!ret)
		hwlog_err("i2c read failure\n");
	hwlog_info("%s:FM_CONTROL1 after writing is : [0x%x], ret = %d\n", __func__, data,ret);
	if (ret)
		return 0;
	else
		return -1;
}

static int FUSB3601_chip_reset_nothing(void *dev_data)
{
	return 0;
}
static int FUSB3601_fcp_adapter_reset(void *dev_data)
{
	int ret = 0;
	FSC_U8 data;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_ENABLE2,&data);
	if (!ret) {
		return -1;
	}
	hwlog_info("%s\n", __func__);
	data |= FUSB3601_MST_RESET_MSK;
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE2, 1, &data);
	if (!ret) {
		return -1;
	}
	data &= ~FUSB3601_MST_RESET_MSK;
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE2, 1, &data);
	if (!ret) {
		return -1;
	}
	FUSB3601_core_redo_bc12(&chip->port);
	return 0;
}
static int FUSB3601_dcd_timout_disable(void)
{
	int ret;
	FSC_U8 data;

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_MUS_CONTROL1,&data);
	hwlog_info("%s:reg befor writing is : [0x%x], ret = %d\n", __func__, data,ret);
	data &= FUSB3601_DCD_TIMEOUT_DISABLE;
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_MUS_CONTROL1, 1, &data);
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_MUS_CONTROL1,&data);
	hwlog_info("%s:reg after writing is : [0x%x], ret = %d\n", __func__, data,ret);
	if (ret)
		return 0;
	else
		return -1;
}
static int FUSB3601_clear_event2_and_event3(void)
{
	FSC_U8 reg_val2 = 0;
	FSC_U8 reg_val3 = 0;
	int ret;

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_2,&reg_val2);
	ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_3,&reg_val3);
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_EVENT_2, 1, &reg_val2);
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_EVENT_3, 1, &reg_val3);
	if (ret)
	    return 0;
	else {
	    hwlog_info("%s:i2c error\n", __func__);
	    return -1;
	}
}
void FUSB3601_dump_register(void)
{
	FSC_U8 i = 0;
	FSC_U8 data = 0;
	int ret;
	for(i = 0x00; i <= 0x1f; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x23; i <= 0x29; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x2e; i <= 0x33; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x50; i <= 0x53; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x70; i <= 0x79; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x80; i <= 0x88; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x90; i <= 0x9d; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0xa0; i <= 0xb2; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0xc0; i <= 0xd0; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0xd1; i <= 0xe0; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0xe1; i <= 0xea; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
}
static void FUSB3601_clear_tx_buffer(void)
{
	FSC_U8 data = 0;
	int ret;

	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_TX0,1, &data);
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_ADDR,1, &data);
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_CMD,1, &data);
	(void)ret;
}
static int FUSB3601_accp_transfer_check(void)
{
	FSC_U8 reg_val2 = 0;
	FSC_U8 reg_val3 = 0;
	int i =0;
	int ret;

	do{
		usleep_range(30000,31000);
		ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_2,&reg_val2);
		hwlog_info("%s:FUSB3601_SCP_EVENT_2 = 0x%x\n", __func__,reg_val2);
		ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_3,&reg_val3);
		hwlog_info("%s:FUSB3601_SCP_EVENT_3 = 0x%x\n", __func__,reg_val3);
		if (!ret) {
		    hwlog_info("%s:read  error\n", __func__);
		    return -1;
		}
		i++;
	}while(i < ACCP_TRANSFER_POLLING_RETRY_TIMES && (reg_val2 & SCP_ACK_AND_NACK_MASK) == 0 && reg_val3 == 0);

	/*W1C for event2 and event3*/
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_EVENT_2, 1, &reg_val2);
	ret |= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_EVENT_3, 1, &reg_val3);

	if(i >= ACCP_TRANSFER_POLLING_RETRY_TIMES) {
		hwlog_info("%s : read accp interrupt time out,total time is %d ms\n",__func__,i*10);
		FUSB3601_dump_register();
		FUSB3601_clear_tx_buffer();
	}

	/*if something  changed print reg info */
	if((reg_val2 & SCP_ACK_MASK) && (reg_val3 == 0)) {
		/*succeed*/
		hwlog_info("%s:succ!\n", __func__);
		return 0;
	} else {
		hwlog_err("%s : event2=0x%x,event3=0x%x\n",__func__,reg_val2,reg_val3);
		return -1;
	}
}
/****************************************************************************
  Function:     FUSB3601_accp_adapter_reg_read
  Description:  read adapter register
  Input:        reg:register's num
                val:the value of register
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int FUSB3601_accp_adapter_reg_read(int* val, int reg)
{
	int ret;
	int i;
	FSC_U8 addr;
	FSC_U8 data;
	FSC_U8 cmd = ACCP_CMD_SBRRD;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}

	if (reg > MAX_U8 || reg < 0) {
		hwlog_err("%s: reg addr = 0x%x\n", __func__, reg);
		return -1;
	}
	addr = (FSC_U8)reg;

	mutex_lock(&FUSB3601_accp_adaptor_reg_lock);

	hwlog_info("%s: reg_addr = 0x%x\n", __func__, reg);
	for (i = 0; i< SW_FCP_RETRY_MAX_TIMES; i++) {
		/*before send cmd, clear event2 and event3*/
		ret = FUSB3601_clear_event2_and_event3();
		if (ret) {
			mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
			return -1;
		}
		ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_ADDR, 1, &addr);
		ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_CMD, 1, &cmd);
		if (!ret) {
			mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
			return -1;
		}
		if (0 == FUSB3601_accp_transfer_check()) {
			msleep(20);
			ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_REG_ACCP_RT_ACK_RX,&data);
			ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_REG_ACCP_RT_RX1,&data);
			ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_REG_ACCP_RT_RX0,&data);
			if (!ret) {
				mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
				return -1;
			} else {
				*val = data;
				mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
				hwlog_info("%s,%d,data = 0x%x\n", __func__, __LINE__,data);
				return 0;
			}
		} else {
			msleep(20);
		}
	}
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_1, &data);
	if (ret) {
		hwlog_info("%s,%d,data = 0x%x\n", __func__, __LINE__,data);
		if (data & FUSB3601_SCP_B_DETECT) {
			/* if 0x7e register read fail, can not redo bc1.2 */
			if (reg != SCP_ADP_TYPE0) {
				FUSB3601_core_redo_bc12(&chip->port);
			}
		}
	}
	mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
	return -1;
}

/****************************************************************************
  Function:     FUSB3601_accp_adapter_reg_write
  Description:  write value into the adapter register
  Input:        reg:register's num
                val:the value of register
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int FUSB3601_accp_adapter_reg_write(int val, int reg)
{
	int ret;
	int i;
	FSC_U8 addr;
	FSC_U8 data;
	FSC_U8 cmd = ACCP_CMD_SBRWR;

	hwlog_info("%s: reg_addr = 0x%x\n", __func__, reg);
	if (reg > MAX_U8 || reg < 0 || val > MAX_U8 || val < 0) {
		hwlog_err("%s: reg addr = 0x%x data = 0x%x\n", __func__, reg, val);
		return -1;
	}
	addr = (FSC_U8)reg;
	data = (FSC_U8)val;

	mutex_lock(&FUSB3601_accp_adaptor_reg_lock);
	for (i = 0; i< SW_FCP_RETRY_MAX_TIMES; i++) {
		/*before send cmd, clear event2 and event3*/
		ret = FUSB3601_clear_event2_and_event3();
		if (ret) {
			mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
			return -1;
		}
		ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_TX0, 1, &data);
		ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_ADDR, 1, &addr);
		ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_CMD, 1, &cmd);
		if (!ret) {
			mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
			return -1;
		}
		if (0 == FUSB3601_accp_transfer_check()) {
			msleep(20);
			ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_REG_ACCP_RT_ACK_RX,&data);
			ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_REG_ACCP_RT_RX0,&data);
			mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
			return 0;
		}
		else {
			msleep(20);
		}
	}
	mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
	return -1;
}

static int FUSB3601_scp_adapter_reg_read(int* val, int reg)
{
	int ret;

	if (FUSB3601_scp_error_flag) {
		hwlog_err("%s : scp timeout happened ,do not read reg = %d \n",__func__,reg);
		return -1;
	}
	ret = FUSB3601_accp_adapter_reg_read(val, reg);
	if (ret) {
		hwlog_err("%s : error reg = %d \n",__func__,reg);
		FUSB3601_scp_error_flag = 1;
		return -1;
	}
	return 0;
}

static int FUSB3601_scp_adapter_reg_write(int val, int reg)
{
	int ret;

	if (FUSB3601_scp_error_flag) {
		hwlog_err("%s : scp timeout happened ,do not write reg = %d \n",__func__,reg);
		return -1;
	}
	ret = FUSB3601_accp_adapter_reg_write(val, reg);
	if (ret) {
		hwlog_err("%s : error reg = %d \n",__func__,reg);
		FUSB3601_scp_error_flag = 1;
		return -1;
	}
	return 0;
}

/****************************************************************************
  Function:     acp_adapter_detect
  Description:  detect accp adapter
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: other fail
                1:fcp adapter but detect fail
***************************************************************************/
static int FUSB3601_accp_adapter_detect(void *dev_data)
{
	int ret;
	int i;
	FSC_U8 data;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_DEVICE_TYPE,&data);
	if (ret) {
		if (data & FUSB3601_DCP_DETECT) {
			/*if DCP is detected by superswitch, no need to redo bc1.2
			 go to scp detection directly*/
			hwlog_info("%s, DEVICE_TYPE = 0x%x, DCP detected by superswitch\n", __func__,data);
		} else {
			/*if no dcp is detected, then redo bc1.2 and return fail ,
			 for the next loop 30 seconds later, we will read DEVICE_TYPE
			 register again* */
			//redo_bc12();
			FUSB3601_core_redo_bc12(&chip->port);
			hwlog_info("%s,DEVICE_TYPE = 0x%x\n", __func__,data);
			return 1;
		}
	} else {
		hwlog_info("%s,%d\n", __func__, __LINE__);
		return -1;
	}
	for (i = 0; i < ACCP_DETECT_MAX_COUT; i++)
	{
		ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_1, &data);
		if (!ret) {
			return -1;
		}
		hwlog_info("%s,%d,data = 0x%x\n", __func__, __LINE__,data);
		if (data & FUSB3601_SCP_OR_FCP_DETECT) {
			msleep(150);
			return 0;
		}
		msleep(ACCP_POLL_TIME);
	}
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_1, &data);
	if (ret) {
		if (data == FUSB3601_SCP_EVENT1_CC_PLGIN_DPDN_PLGIN) {
			ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_2, &data);
			if (ret) {
				if (data & FUSB3601_SCP_EVENT2_ACK) {
					data = 0x00;
					ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE1, 1, &data);
					if (ret) {
						return -1;
					}
				}
			}
		}
	}
	FUSB3601_core_redo_bc12_limited(&chip->port);
	return -1;
}

/**********************************************************
*  Function:       FUSB3601_is_support_fcp
*  Discription:    check whether support fcp
*  Parameters:     NA
*  return value:   0:support
                  -1:not support
**********************************************************/
static int FUSB3601_is_support_fcp(void)
{
	return 0;
}

#ifdef CONFIG_DIRECT_CHARGER
static int FUSB3601_is_support_scp(void)
{
    return 0;
}

static int FUSB3601_scp_init(void *dev_data)
{
	int ret;
	FSC_U8 data;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	struct Port* port;

	FUSB3601_scp_error_flag = 0;
	if (!chip) {
		hwlog_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}
	port = &chip->port;
	if (!port) {
		hwlog_err("FUSB  %s - port structure is NULL!\n", __func__);
		return -1;
	}
	FUSB3601_PDDisable(port);
	port->registers_.AlertMskH.M_VBUS_ALRM_LO= 0;
	FUSB3601_WriteRegisters(port, regALERTMSKH, 1);
	FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);
	if (get_dpd_enable()) {
#ifdef CONFIG_USB_ANALOG_HS_INTERFACE
		ret = FUSB3601_scp_get_adapter_vendor_id();
		if (IWATT_ADAPTER == ret || WELTREND_ADAPTER == ret || ID0X32_ADAPTER == ret) {
			usb_analog_hs_plug_in_out_handle(DIRECT_CHARGE_IN);
			ana_hs_plug_handle(DIRECT_CHARGE_IN);
			hwlog_info("%s :  config rd on Dm for IWATT\n ",__func__);
		}
#endif
		data = FUSB3601_SCP_ENABLE2_INIT;
		ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE2, 1, &data);
		FUSB3601_set_vbus_detach(port, VBUS_DETACH_DISABLE);
		FUSB3601_ReadRegister(port, regFM_CONTROL4);
	}

	hwlog_info("%s\n", __func__);
	return 0;
}
static int FUSB3601_scp_exit(void *dev_data)
{
	int ret;
	FSC_U8 data;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	struct Port* port;
	if (!chip) {
		hwlog_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}
	port = &chip->port;
	if (!port) {
		hwlog_err("FUSB  %s - port structure is NULL!\n", __func__);
		return -1;
	}

	FUSB3601_vout_enable(1);
	if (get_dpd_enable()) {
#ifdef CONFIG_USB_ANALOG_HS_INTERFACE
		hwlog_info("%s :  disable rd on Dm\n ",__func__);
		usb_analog_hs_plug_in_out_handle(DIRECT_CHARGE_OUT);
		ana_hs_plug_handle(DIRECT_CHARGE_OUT);
#endif
		data = FUSB3601_DPD_DISABLE;
		ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE2, 1, &data);
		hwlog_info("%s: ret = %d\n", __func__, ret);
		FUSB3601_set_vbus_detach(port, VBUS_DETACH_ENABLE);
		FUSB3601_ReadRegister(port, regFM_CONTROL4);
		state_machine_need_resched = 1;
		queue_work(chip->highpri_wq,&chip->sm_worker);
	}

	hwlog_info("%s\n", __func__);
	FUSB3601_scp_error_flag = 0;
	return 0;
}
static int FUSB3601_scp_get_adapter_vendor_id(void)
{
	int val = 0;
	int ret;

	ret = FUSB3601_scp_adapter_reg_read(&val, SCP_PCHIP_ID);
	if(ret)
	{
		hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
		return -1;
	}
	hwlog_info("[%s]val is 0x%x \n", __func__, val);
	switch (val)
	{
		case VENDOR_ID_RICHTEK:
			hwlog_info("[%s]adapter is richtek \n", __func__);
			return RICHTEK_ADAPTER;
		case VENDOR_ID_IWATT:
			hwlog_info("[%s]adapter is iwatt \n", __func__);
			return IWATT_ADAPTER;
		case VENDOR_ID_WELTREND:
			hwlog_info("[%s]adapter is weltrend \n", __func__);
			return WELTREND_ADAPTER;
		case VENDOR_ID_0X32:
			hwlog_info("[%s]adapter id is 0x32 \n", __func__);
			return ID0X32_ADAPTER;
		default:
			hwlog_info("[%s]this adaptor vendor id is not found!\n", __func__);
			return val;
	}
}

static int FUSB3601_scp_adaptor_reset(void *dev_data)
{
	return 0;
}
static unsigned int FUSB3601_get_charger_type(void)
{
	enum chip_charger_type charger_type = CHARGER_TYPE_NONE;
	FSC_U8 data;
	int ret;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return charger_type;
	}

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_DEVICE_TYPE,&data);
	if (ret) {
		if (data & FUSB3601_DCP_DETECT) {
			hwlog_info("%s, DEVICE_TYPE = 0x%x, DCP detected by superswitch\n", __func__,data);
			charger_type = CHARGER_TYPE_DCP;
		} else {
			hwlog_info("%s,DEVICE_TYPE = 0x%x\n", __func__,data);
			FUSB3601_core_redo_bc12(&chip->port);
		}
	} else {
		hwlog_info("%s,error!\n", __func__);
	}

    return charger_type;
}

#endif

struct charge_switch_ops FUSB3601_switch_ops = {
	.get_charger_type = FUSB3601_get_charger_type,
};

static int fusb3601_fcp_reg_read_block(int reg, int *val, int num, void *dev_data)
{
	int ret;
	int i;
	int data = 0;

	if (!val) {
		hwlog_err("val is null\n");
		return -1;
	}

	for (i = 0; i < num; i++) {
		ret = FUSB3601_accp_adapter_reg_read(&data, reg + i);
		if (ret) {
			hwlog_err("fcp read failed(reg=0x%x)\n", reg + i);
			return -1;
		}

		val[i] = data;
	}

	return 0;
}

static int fusb3601_fcp_reg_write_block(int reg, const int *val, int num, void *dev_data)
{
	int ret;
	int i;

	if (!val) {
		hwlog_err("val is null\n");
		return -1;
	}

	for (i = 0; i < num; i++) {
		ret = FUSB3601_accp_adapter_reg_write(val[i], reg + i);
		if (ret) {
			hwlog_err("fcp write failed(reg=0x%x)\n", reg + i);
			return -1;
		}
	}

	return 0;
}

static struct hwfcp_ops fusb3601_fcp_protocol_ops = {
	.chip_name = "fusb3601",
	.reg_read = fusb3601_fcp_reg_read_block,
	.reg_write = fusb3601_fcp_reg_write_block,
	.detect_adapter = FUSB3601_accp_adapter_detect,
	.soft_reset_master = FUSB3601_chip_reset_nothing,
	.soft_reset_slave = FUSB3601_fcp_adapter_reset,
	.get_master_status = NULL,
	.stop_charging_config = FUSB3601_fcp_stop_charge_config,
	.is_accp_charger_type = NULL,
};

static int fusb3601_scp_reg_read_block(int reg, int *val, int num, void *dev_data)
{
	int ret;
	int i;
	int data = 0;

	if (!val) {
		hwlog_err("val is null\n");
		return -1;
	}

	FUSB3601_scp_error_flag = 0;

	for (i = 0; i < num; i++) {
		ret = FUSB3601_scp_adapter_reg_read(&data, reg + i);
		if (ret) {
			hwlog_err("scp read failed(reg=0x%x)\n", reg + i);
			return -1;
		}

		val[i] = data;
	}

	return 0;
}

static int fusb3601_scp_reg_write_block(int reg, const int *val, int num, void *dev_data)
{
	int ret;
	int i;

	if (!val) {
		hwlog_err("val is null\n");
		return -1;
	}

	FUSB3601_scp_error_flag = 0;

	for (i = 0; i < num; i++) {
		ret = FUSB3601_scp_adapter_reg_write(val[i], reg + i);
		if (ret) {
			hwlog_err("scp write failed(reg=0x%x)\n", reg + i);
			return -1;
		}
	}

	return 0;
}

static struct hwscp_ops fusb3601_scp_protocol_ops = {
	.chip_name = "fusb3601",
	.reg_read = fusb3601_scp_reg_read_block,
	.reg_write = fusb3601_scp_reg_write_block,
	.detect_adapter = FUSB3601_accp_adapter_detect,
	.soft_reset_master = FUSB3601_chip_reset_nothing,
	.soft_reset_slave = FUSB3601_scp_adaptor_reset,
	.pre_init = FUSB3601_scp_init,
	.pre_exit = FUSB3601_scp_exit,
};

#ifdef CONFIG_SUPERSWITCH_FSC
static int FUSB3601_chsw_set_wired_channel(int channel_type, int flag)
{
	int ret = 0;

	if ((channel_type != WIRED_CHANNEL_BUCK) && (channel_type != WIRED_CHANNEL_ALL))
		return 0;

	if (WIRED_CHANNEL_CUTOFF == flag) {
		hwlog_info("%s set fusb3601 en disable\n", __func__);
		ret = FUSB3601_vout_enable(0);
	} else {
		hwlog_info("%s set fusb3601 en enable\n", __func__);
		ret = FUSB3601_vout_enable(1);
	}
	if (!ret)
		wired_channel_status = flag;
	return ret;
}
static int FUSB3601_chsw_get_wired_channel(int channel_type)
{
	if ((channel_type != WIRED_CHANNEL_BUCK) && (channel_type != WIRED_CHANNEL_ALL))
		return WIRED_CHANNEL_RESTORE;

	return wired_channel_status;
}
static struct wired_chsw_device_ops chsw_ops = {
	.get_wired_channel = FUSB3601_chsw_get_wired_channel,
	.set_wired_channel = FUSB3601_chsw_set_wired_channel,
};
static int FUSB3601_wired_chsw_ops_register(void)
{
	int ret = 0;
	struct fusb3601_chip* chip = fusb3601_GetChip();

	if (chip->use_super_switch_cutoff_wired_channel) {
		ret = wired_chsw_ops_register(&chsw_ops);
		if (ret) {
			hwlog_err("%s register fusb3601 switch ops failed!\n", __func__);
			return -1;
		}
		hwlog_info("%s fusb3601 switch ops register success\n", __func__);
	}
	hwlog_info("%s++\n", __func__);

	return ret;
}
#endif

void FUSB3601_scp_initialize(void)
{
	int ret;
	FSC_U8 data;

	data = FUSB3601_SCP_ENABLE1_INIT;
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE1, 1, &data);
	data = FUSB3601_SCP_INT1_MASK_INIT;
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_INT1_MASK, 1, &data);
	data = FUSB3601_SCP_INT2_MASK_INIT;
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_INT2_MASK, 1, &data);
	data = FUSB3601_TIMER_SET1_INIT;
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_TIMER_SET1, 1, &data);
	data = FUSB3601_TIMER_SET2_INIT;
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_TIMER_SET2, 1, &data);

	ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_MUS_INTERRUPT_MASK,&data);
	if (ret) {
	    data &= FUSB3601_MUS_INTERRUPT_MASK_INIT;
	    ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_MUS_INTERRUPT_MASK, 1, &data);
	}
	if (!ret) {
	    hwlog_err(" %s:%d error!\n", __func__,__LINE__);
	}
	ret = FUSB3601_dcd_timout_disable();
	if (ret) {
	    hwlog_err(" %s:%d error!\n", __func__,__LINE__);
	}
}

void FUSB3601_charge_register_callback(void)
{
    hwlog_info(" %s++!\n", __func__);
    mutex_init(&FUSB3601_accp_detect_lock);
    mutex_init(&FUSB3601_accp_adaptor_reg_lock);

    if(0 == charge_switch_ops_register(&FUSB3601_switch_ops))
    {
        hwlog_info(" charge switch ops register success!\n");
    }
#ifdef CONFIG_SUPERSWITCH_FSC
	FUSB3601_wired_chsw_ops_register();
#endif

	if (FUSB3601_is_support_fcp() == 0)
		hwfcp_ops_register(&fusb3601_fcp_protocol_ops);

#ifdef CONFIG_DIRECT_CHARGER
	if (FUSB3601_is_support_scp() == 0)
		hwscp_ops_register(&fusb3601_scp_protocol_ops);
#endif /* CONFIG_DIRECT_CHARGER */

	hwlog_info(" %s--!\n", __func__);
}
