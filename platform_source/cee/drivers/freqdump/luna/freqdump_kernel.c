/*
 * freqdump_kernel.c
 *
 * freqdump test
 *
 * Copyright (C) 2020-2020 Huawei Technologies Co., Ltd.
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

#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <global_ddr_map.h>
#include <luna/freqdump_kernel.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <securec.h>
#include <platform_include/see/bl31_smc.h>

#ifdef CONFIG_FREQDUMP_PLATFORM
#include "loadmonitor.h"
#endif

#ifdef CONFIG_ENABLE_MIDEA_MONITOR
#include <media_monitor.h>
int g_delay_time_media;
#endif

#undef  pr_fmt
#define pr_fmt(fmt) "[freqdump]:" fmt

#define SEC_DEBUG_FS_AUTHORITY	660

/*lint -e750 -esym(750,*)*/
void __iomem *g_freqdump_virt_addr;
static phys_addr_t g_freqdump_phy_addr;
static char loadmonitor_buf[LOADMONITOR_BUFF_SIZE];
int g_loadmonitor_status;
int monitor_en_flags = -1;
char chip_type_buf[10];
unsigned int chip_type[1] = {2}; /* 0:es   1:cs    2:none */
struct dentry *freqdump_debugfs_root;
struct freqdump_data *freqdump_read_data;
static struct semaphore loadmonitor_sem, freqdump_sem;

/*lint -e749 -esym(749,*)*/
/*lint -e753 -esym(753,*)*/
#ifdef CONFIG_FREQDUMP_PLATFORM

/* cs iom7 code start */

#define MONITOR_AO_FREQ_CS              60000000

#define MONITOR_PERI_FREQ_CS		162200000

unsigned int monitor_load_bos_cs[CS_MAX_PERI_INDEX] = {0};
u64  monitor_load_bos_cs_ao[CS_MAX_AO_INDEX] = {0};
#endif
/* cs iom7 code end */

/* DPM global variable head */
void __iomem *g_loadmonitor_virt_addr;
/* DPM global variable end */
/*lint -e750 +esym(750,*)*/

int chip_type_check_err(void)
{
	if (chip_type[0] != CS) {
		pr_err("[%s]chip_type is error!\n", __func__);
		return -1;
	}
	return 0;
}
/*lint -e715*/
noinline int atfd_service_freqdump_smc(u64 _function_id,
	u64 _arg0, u64 _arg1, u64 _arg2)
{
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;
	asm volatile (
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc	#0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
}
/*lint +e715*/

void sec_freqdump_read(void)
{
	down(&freqdump_sem);
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_SVC_REG_RD, (u64)0,
		g_freqdump_phy_addr, (u64)0);
	up(&freqdump_sem);
}

#ifdef CONFIG_FREQDUMP_PLATFORM
/* cs iom7 code start */
/*lint -e679*/
int ao_loadmonitor_read(u64 *addr)
{
	struct loadmonitor_sigs sigs;
	int index = 0;  /* sensor id, in AO, from 32 to 63 */
	unsigned long i = 0;
	int ret;

	if (addr == NULL) {
		pr_err("%s: input error!\n", __func__);
		return -1;
	}

	ret = _ao_loadmonitor_read(&sigs, sizeof(sigs));
	if (ret != 0 && ret != -ENODEV) { /* -ENODEV:closed by DTS config */
		pr_err("%s: get signal cnt error! ret:%d\n", __func__, ret);
		return -1;
	}

/* ******************************************************** */
/* |loadmonitor addr                         |              */
/* |<------------peri signal data ---------->|              */
/* | 41 u32 | 41 u32 | 41 u32 |                             */
/* |ao_loadmonitor_addr                                   | */
/* |<-----------------ao signal data--------------------->| */
/* |16 u64 | 16 u64 | 16 u64 | 16 u64 |                     */
/* ******************************************************** */
	for (; i < 32; ++i) {
		if (AO_MONITOR0_IP_MASK & BIT(i)) {
			*(addr + index) = sigs.sig[i].count[DATA_INDEX_IDLE];
			*(addr + MAX_MONITOR_IP_DEVICE_AO + index) =
				sigs.sig[i].count[DATA_INDEX_BUSY_NORMAL];
			*(addr + 2 * MAX_MONITOR_IP_DEVICE_AO + index) =
				sigs.sig[i].count[DATA_INDEX_BUSY_LOW];
			*(addr + 3 * MAX_MONITOR_IP_DEVICE_AO + index) =
				sigs.sig[i].samples;
			index++;
		}
	}
	return 0;
}
/*lint +e679*/

int peri_monitor_clk_init(const char *clk_name, unsigned int clk_freq)
{
	int ret;
	struct clk *monitor_clk = NULL;

	if (clk_name == NULL) {
		pr_err("[%s]input error!.\n", __func__);
		return -1;
	}

	monitor_clk = clk_get(NULL, clk_name);
	if (IS_ERR(monitor_clk)) {
		pr_err("[%s]peri clk init error!.\n", __func__);
		return -1;
	}

	/* set frequency */
	ret = clk_set_rate(monitor_clk, clk_freq);
	if (ret < 0) {
		pr_err("[%s]peri clk set rate error!.\n", __func__);
		clk_put(monitor_clk);
		return -1;
	}
	/* enable clk_loadmonitor */
	ret = clk_prepare_enable(monitor_clk);
	if (ret) {
		clk_put(monitor_clk);
		pr_err("[%s]peri clk enable error!.\n", __func__);
		return -1;
	}

	clk_put(monitor_clk);

	return 0;
}

int peri_monitor_clk_disable(const char *clk_name)
{
	struct clk *monitor_clk = NULL;

	if (clk_name == NULL) {
		pr_err("[%s]input error!.\n", __func__);
		return -1;
	}

	monitor_clk = clk_get(NULL, clk_name);
	if (IS_ERR(monitor_clk)) {
		pr_err("[%s]peri clk disable error!.\n", __func__);
		return -1;
	}

	/* disable clk_loadmonitor */
	clk_disable_unprepare(monitor_clk);
	clk_put(monitor_clk);

	return 0;
}

int all_peri_clk_init(void)
{
	int ret;

	ret = peri_monitor_clk_init("clk_loadmonitor", MONITOR_PERI_FREQ_CS);
	if (ret != 0) {
		pr_err("[%s] peri_monitor0_clk_init error.\n", __func__);
		return ret;
	}
	ret = peri_monitor_clk_init("clk_loadmonitor_l", MONITOR_PERI_FREQ_CS);
	if (ret != 0) {
		pr_err("[%s] peri_monitor1_clk_init error.\n", __func__);
		return ret;
	}
	pr_err("[%s] success. \n", __func__);
	return 0;

}
/* ****************************** cs iom7 code end **************************** */
void sec_loadmonitor_switch_enable(unsigned int value_peri,
	unsigned int value_ao, unsigned int en_flags)
{
	int ret;

	if (chip_type_check_err()) {
		pr_err("[%s]chip_type is error!\n", __func__);
		return;
	}

	if (chip_type[0] == CS) { /* cs */
		if (en_flags == PERI_MONITOR_EN) {
			ret = all_peri_clk_init();
			if (ret != 0)
				return;
			pr_err("[%s] cs monitor clk init success en_flags:%d.\n",
				__func__, en_flags);
			(void)atfd_service_freqdump_smc(
				(u64)FREQDUMP_LOADMONITOR_SVC_ENABLE,
				(u64)value_peri, (u64)value_ao, (u64)en_flags);
			pr_err("[%s] end loadmonitor enable.\n", __func__);
		} else if (en_flags == AO_MONITOR_EN) {
			ao_loadmonitor_enable(value_ao, MONITOR_AO_FREQ_CS);
		} else {
			ret = all_peri_clk_init();
			if (ret != 0)
				return;

			pr_err("[%s] cs monitor clk init success en_flags:%d.\n",
				__func__, en_flags);
			(void)atfd_service_freqdump_smc(
				(u64)FREQDUMP_LOADMONITOR_SVC_ENABLE,
				(u64)value_peri, (u64)value_peri, (u64)en_flags);
			ao_loadmonitor_enable(value_ao, MONITOR_AO_FREQ_CS);
		}
	}
	pr_err("[%s] success. en_flags:%u\n", __func__, en_flags);
	return;
}

int all_peri_clk_disable(void)
{
	int ret;

	ret = peri_monitor_clk_disable("clk_loadmonitor");
	if (ret != 0) {
		pr_err("[%s] peri_monitor0_clk_disable error.\n", __func__);
		return;
	}
	ret = peri_monitor_clk_disable("clk_loadmonitor_l");
	if (ret != 0) {
		pr_err("[%s] peri_monitor1_clk_disable error.\n", __func__);
		return;
	}
	pr_err("[%s] all_clk_disable success.\n", __func__);
}

void sec_loadmonitor_switch_disable(void)
{
	int ret;

	(void)atfd_service_freqdump_smc((u64)FREQDUMP_LOADMONITOR_SVC_DISABLE,
		(u64)0, (u64)0, (u64)0);

	if (chip_type[0] == CS) { /* cs */
		if ((monitor_en_flags == PERI_MONITOR_EN) ||
			(monitor_en_flags == ALL_MONITOR_EN)) {
			ret = all_peri_clk_disable();
			if (ret != 0)
				return;
		}
		ao_loadmonitor_disable();
	}
	return;
}

void sec_loadmonitor_data_read(unsigned int enable_flags)
{
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_LOADMONITOR_SVC_REG_RD,
		(u64)enable_flags, g_freqdump_phy_addr, (u64)0);
}

void sec_chip_type_read(void)
{
	int ret;

	(void)atfd_service_freqdump_smc(
		(u64)FREQDUMP_LOADMONITOR_SVC_ENABLE_READ,
		(u64)0, g_freqdump_phy_addr, (u64)0);
	memcpy_fromio(chip_type, (void *)g_freqdump_virt_addr + SHARED_MEMORY_SIZE -
		CS_MAX_PERI_INDEX * 4 - 1, sizeof(chip_type));
	ret = snprintf_s(chip_type_buf, sizeof(chip_type_buf),
		sizeof(chip_type_buf) - 1, "%u", chip_type[0]);
	if (ret == -1)
		pr_err("[%s] snprintf_s is err.\n", __func__);
}

#ifdef CONFIG_DFX_DEBUG_FS
void hisi_sec_adc_set_param(void)
{
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_ADC_SVC_SET_PARAM,
		(u64)0, (u64)0, (u64)0);
}

void hisi_sec_adc_disable(void)
{
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_ADC_SVC_DISABLE,
		(u64)0, (u64)0, (u64)0);
}
#endif
#endif

#ifdef CONFIG_DFX_DEBUG_FS
#ifdef CONFIG_NPU_PM_DEBUG
/* data[6][2] used to store NPU module freq & voltage info          */
/* name[6][10] used to store NPU module name, max length is 9bytes  */
extern int get_npu_freq_info(void *data, int size, char (*name)[10],
	int size_name);
#endif
/*lint -e715*/
static int freqdump_node_dump(struct seq_file *s, void *p)
{
	int ret;

	sec_freqdump_read();
	ret = memset_s(freqdump_read_data, sizeof(struct freqdump_data),
		       0, sizeof(struct freqdump_data));
	if (ret != EOK) {
		pr_err("[%s] memset_s is err.\n", __func__);
		return -ENOMEM;
	}
	memcpy_fromio(freqdump_read_data, (void *)g_freqdump_virt_addr,
		      sizeof(struct freqdump_data));

#ifdef CONFIG_NPU_PM_DEBUG
	/* Add the NPU freqinfo at the end of data struct */
	if (get_npu_freq_info(freqdump_read_data->npu,
		sizeof(freqdump_read_data->npu), freqdump_read_data->npu_name,
		sizeof(freqdump_read_data->npu_name)))
		pr_err("%s Failed to get NPU freqdump info!\n", __func__);
#endif

	seq_write(s, (const struct freqdump_data *)freqdump_read_data,
		  sizeof(struct freqdump_data));

	return OK;
}
/*lint +e715*/

static int read_format_cs_loadmonitor_data(void)
{
	int pos = 0;
	int buf_size = sizeof(loadmonitor_buf);
	int i, ret;
	char *monitor_addr = (char *)g_freqdump_virt_addr + SHARED_MEMORY_SIZE -
		CS_MAX_PERI_INDEX * 4;

	if ((monitor_en_flags == PERI_MONITOR_EN) ||
		(monitor_en_flags == ALL_MONITOR_EN)) {
		#ifdef CONFIG_ENABLE_MIDEA_MONITOR
		media_monitor_read(g_delay_time_media);
		#else
		sec_loadmonitor_data_read((unsigned int)monitor_en_flags);
		#endif
		memcpy_fromio(monitor_load_bos_cs, (void *)(monitor_addr),
		CS_MAX_PERI_INDEX * 4);
	}
	if ((monitor_en_flags == AO_MONITOR_EN) ||
		(monitor_en_flags == ALL_MONITOR_EN)) {
		if (ao_loadmonitor_read(monitor_load_bos_cs_ao)) {
			pr_err("%s : ao_loadmonitor_read error!\n", __func__);
			return -EINVAL;
		}
	}

	for (i = 0; i < CS_MAX_PERI_INDEX && pos < buf_size; i++) {
		ret = snprintf_s(loadmonitor_buf + pos, buf_size - pos,
				 buf_size - pos - 1, "%u|",
				 (u32)monitor_load_bos_cs[i]);
		if (ret == -1) {
			pr_err("%s :print peri monitor data to buff fail!\n",
			       __func__);
			return -EINVAL;
		}
		pos += ret;
	}

	for (i = 0; i < CS_MAX_AO_INDEX && pos < buf_size; i++) {
		ret = snprintf_s(loadmonitor_buf + pos, buf_size - pos,
				 buf_size - pos - 1, "%llu|",
				 (u64)monitor_load_bos_cs_ao[i]);
		if (ret == -1) {
			pr_err("%s :print ao monitor data to buff fail!\n",
			       __func__);
			return -EINVAL;
		}
		pos += ret;
	}

	return OK;
}

/*lint -e715*/
static int loadmonitor_node_dump(struct seq_file *s, void *p)
{

	down(&loadmonitor_sem);
	if (!g_loadmonitor_status) {
		up(&loadmonitor_sem);
		return -EINVAL;
	}

	if ((monitor_en_flags != PERI_MONITOR_EN) &&
		(monitor_en_flags != AO_MONITOR_EN) &&
		(monitor_en_flags != ALL_MONITOR_EN)) {
		pr_err("%s loadmonitor node dump error!\n", __func__);
		up(&loadmonitor_sem);
		return -EINVAL;
	}

	if (chip_type_check_err()) {
		pr_err("[%s]chip_type is error!\n", __func__);
		up(&loadmonitor_sem);
		return -EINVAL;
	}

	if (chip_type[0] == CS) {
		if (read_format_cs_loadmonitor_data() == -EINVAL) {
			up(&loadmonitor_sem);
			return -EINVAL;
		}
	}
	seq_printf(s, "%s\n", loadmonitor_buf);

	up(&loadmonitor_sem);
	return OK;
}
/*lint +e715*/




/*lint -e713 -e715*/
/* set adc params, only NO 1 is effective */
static ssize_t adc_set_param_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char buf[20] = {0};

	down(&loadmonitor_sem);

	if (ubuf != NULL) {
		if (copy_from_user(buf, ubuf, min_t(size_t, sizeof(buf) - 1, cnt))) {
			pr_err("[%s]can not copy from user\n", __func__);
			up(&loadmonitor_sem);
			return -EINVAL;
		}
	} else {
		pr_err("ubuf is NULL\n");
		up(&loadmonitor_sem);
		return -EINVAL;
	}

	if (buf[0] == '1')
		hisi_sec_adc_set_param();
	else
		pr_err("[%s]only 1 can set\n", __func__);

	up(&loadmonitor_sem);
	return cnt;
}

/* set adc disable, only NO 1 is effective */
static ssize_t adc_disable(struct file *filp, const char __user *ubuf,
		size_t cnt, loff_t *ppos)
{
	char buf[20] = {0};

	down(&loadmonitor_sem);

	if (ubuf != NULL) {
		if (copy_from_user(buf, ubuf, min_t(size_t, sizeof(buf) - 1, cnt))) {
			pr_err("adc_disable can not copy from user\n");
			up(&loadmonitor_sem);
			return -EINVAL;
		}
	} else {
		pr_err("ubuf is NULL\n");
		up(&loadmonitor_sem);
		return -EINVAL;
	}

	if (buf[0] == '1')
		hisi_sec_adc_disable();
	else
		pr_err("[%s] only 1 can set\n", __func__);

	up(&loadmonitor_sem);
	return cnt;
}


static ssize_t loadmonitor_enable_write(struct file *filp,
		const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char buf[45] = {0};
	int ret;
	unsigned int delay_time_peri, delay_time_ao;
	int enable_flags;

	pr_err("[%s] down!\n", __func__);
	down(&loadmonitor_sem);
	if (ubuf != NULL) {
		if (copy_from_user(buf, ubuf, min_t(size_t, sizeof(buf) - 1, cnt))) {
			pr_err("loadmonitor_switch_write can not copy from user\n");
			up(&loadmonitor_sem);
			return -EINVAL;
		}
	} else {
		pr_err("ubuf is NULL\n");
		up(&loadmonitor_sem);
		return -EINVAL;
	}
	// cppcheck-suppress *
	ret = sscanf_s(buf, "%d,%d,%d", &delay_time_peri, &delay_time_ao,
				&enable_flags);
	if ((ret < 0) || ((delay_time_peri == 0) && (delay_time_ao == 0))) {
		pr_err("%s interpreting data error or the delay time should not be 0!\n", __func__);
		up(&loadmonitor_sem);
		return (ssize_t)cnt;
	}
	pr_err("[%s] g_delay_time_media:%d\n", __func__, MONITOR_PERI_FREQ_CS *
		g_media_info[INFO_MONITOR_FREQ]);
#ifdef CONFIG_ENABLE_MIDEA_MONITOR
	g_delay_time_media = delay_time_peri / MONITOR_PERI_FREQ_CS *
		g_media_info[INFO_MONITOR_FREQ];
#endif
	if ((enable_flags != PERI_MONITOR_EN) && (enable_flags != AO_MONITOR_EN) &&
		(enable_flags != ALL_MONITOR_EN)) {
		pr_err("%s loadmonitor can not be enable!\n", __func__);
		up(&loadmonitor_sem);
		return (ssize_t)cnt;
	} else {
		monitor_en_flags = enable_flags;
		pr_err("[%s] enable:delay_time_peri:%d, delay_time_ao:%d, enable_flags:%d!\n",
			__func__, delay_time_peri, delay_time_ao, enable_flags);
		sec_loadmonitor_switch_enable(delay_time_peri, delay_time_ao,
			(unsigned int)enable_flags);
	}
	g_loadmonitor_status = 1;
	up(&loadmonitor_sem);
	return (ssize_t)cnt;
}

static int chip_type_node_dump(struct seq_file *s, void *p)
{
	sec_chip_type_read();
	seq_printf(s, "%s\n", chip_type_buf);

	return OK;
}

static int chip_type_node_open(struct inode *inode, struct file *file)
{
	return single_open(file, chip_type_node_dump, NULL);
}

/*lint -e713 -e715*/
static ssize_t loadmonitor_disable_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char buf[20] = {0};

	down(&loadmonitor_sem);

	if (g_loadmonitor_status == 0) {
		up(&loadmonitor_sem);
		return -EINVAL;
	}
	if (ubuf != NULL) {
		if (copy_from_user(buf, ubuf, min_t(size_t, sizeof(buf) - 1, cnt))) {
			pr_err("loadmonitor_switch_write can not copy from user\n");
			up(&loadmonitor_sem);
			return -EINVAL;
		}
	} else {
		pr_err("[%s] ubuf is NULL!\n", __func__);
		up(&loadmonitor_sem);
		return -EINVAL;
	}

	if (buf[0] == '1')
		sec_loadmonitor_switch_disable();
	g_loadmonitor_status = 0;
	monitor_en_flags = -1;
	up(&loadmonitor_sem);
	return cnt;
}

static int loadmonitor_debugfs_node_open(struct inode *inode,
	struct file *file)
{
	return single_open(file, loadmonitor_node_dump, NULL);
}

static int freqdump_debugfs_node_open(struct inode *inode,
	struct file *file)
{
	return single_open(file, freqdump_node_dump, NULL);
}

/*lint +e713 +e715*/
/*lint -e64 -e785*/
static const struct file_operations freqdump_fops = {
	.owner		= THIS_MODULE,
	.open      = freqdump_debugfs_node_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release
};

static const struct file_operations loadmonitor_dump_fops = {
	.owner		= THIS_MODULE,
	.open	= loadmonitor_debugfs_node_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release
};


#ifdef CONFIG_FREQDUMP_PLATFORM
static const struct file_operations adc_set_param_fops = {
	.owner		= THIS_MODULE,
	.write      = adc_set_param_write,
};

static const struct file_operations adc_disable_fops = {
	.owner		= THIS_MODULE,
	.write      = adc_disable,
};

static const struct file_operations loadmonitor_enable_fops = {
	.owner		= THIS_MODULE,
	.write      = loadmonitor_enable_write,
	.open       = chip_type_node_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release
};
#else
static const struct file_operations loadmonitor_enable_fops = {
	.owner		= THIS_MODULE,
	.write      = loadmonitor_enable_write,
};
#endif
static const struct file_operations loadmonitor_disable_fops = {
	.owner		= THIS_MODULE,
	.write      = loadmonitor_disable_write,
};
/*lint +e64 +e785*/
#endif

/*lint -e124 -e747 -e776 -e801 -e835 -e838 -e845 -e679*/
static int freqdump_debugfs_init_node(void)
{
#ifdef CONFIG_DFX_DEBUG_FS
	struct dentry *freqdump_flie = NULL;
	struct dentry *loadmonitor_file = NULL;
	struct dentry *loadmonitor_enable_file = NULL;
	struct dentry *loadmonitor_disable_file = NULL;
#ifdef CONFIG_FREQDUMP_PLATFORM
	struct dentry *adc_set_param_file = NULL;
	struct dentry *adc_disable_file = NULL;
#endif
#endif

#ifdef CONFIG_DFX_DEBUG_FS
	freqdump_debugfs_root = debugfs_create_dir("freqdump", NULL);
	if (!freqdump_debugfs_root)
		goto Dir_Fail;
	freqdump_flie = debugfs_create_file("freqdump_data", 0660, freqdump_debugfs_root, NULL, &freqdump_fops);
	if (!freqdump_flie)
		goto File_Fail;
	loadmonitor_file = debugfs_create_file("loadmonitor_data", 0660, freqdump_debugfs_root, NULL, &loadmonitor_dump_fops);
	if (!loadmonitor_file)
		goto File_Fail;
#ifdef CONFIG_FREQDUMP_PLATFORM
	adc_set_param_file = debugfs_create_file("adc_param_set", 0660, freqdump_debugfs_root, NULL, &adc_set_param_fops);
	if (!adc_set_param_file)
		goto File_Fail;

	adc_disable_file = debugfs_create_file("adc_disable", 0660, freqdump_debugfs_root, NULL, &adc_disable_fops);
	if (!adc_disable_file)
		goto File_Fail;
#endif
	loadmonitor_enable_file = debugfs_create_file("loadmonitor_enable", 0660, freqdump_debugfs_root, NULL, &loadmonitor_enable_fops);
	if (!loadmonitor_enable_file)
		goto File_Fail;
	loadmonitor_disable_file = debugfs_create_file("loadmonitor_disable", 0660, freqdump_debugfs_root, NULL, &loadmonitor_disable_fops);
	if (!loadmonitor_disable_file)
		goto File_Fail;
#endif
	return OK;

#ifdef CONFIG_DFX_DEBUG_FS
File_Fail:
	debugfs_remove_recursive(freqdump_debugfs_root);
Dir_Fail:
	iounmap(g_freqdump_virt_addr);
	return -ENOENT;
#endif
}

static int freqdump_shared_memory_init(void)
{
	struct device_node *np = NULL;
	u32 data[2] = { 0 };
	u32 ret;

	np = of_find_compatible_node(NULL, NULL, "platform,freqdump");
	if (!np) {
		pr_err("%s: dts[%s] node not found\n", __func__, "platform,freqdump");
		return -ENODEV;
	}
	ret = of_property_read_u32_array(np, "reg", &data[0], (size_t)2);
	if (ret) {
		pr_err("[%s] get hisilicon,freqdump attribute failed.\n", __func__);
		return -ENODEV;
	}

	g_freqdump_phy_addr = (ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE +
		data[0]);
	g_freqdump_virt_addr = ioremap(g_freqdump_phy_addr, (size_t)data[1]);
	memset_io(g_freqdump_virt_addr, 0, (size_t)data[1]);
	if (g_freqdump_virt_addr == NULL) {
		pr_err("freqdump ioremap failed!\n");
		return -ENOMEM;
	}

	return OK;
}

static int __init freqdump_debugfs_init(void)
{
	int ret;

	sema_init(&loadmonitor_sem, 1);
	sema_init(&freqdump_sem, 1);

	ret = memset_s(loadmonitor_buf, sizeof(loadmonitor_buf), 0,
		sizeof(loadmonitor_buf));
	if (ret != EOK) {
		pr_err("[%s] memset_s is err!\n", __func__);
		return -ENOMEM;
	}
	g_loadmonitor_status = 0;

	if (freqdump_shared_memory_init() != OK) {
		pr_err("freqdump shared memory init fail!\n");
		return -ENOMEM;
	}

#ifndef CONFIG_FREQDUMP_PLATFORM
	g_loadmonitor_virt_addr = g_freqdump_virt_addr + sizeof(struct freqdump_data);
#else
	g_loadmonitor_virt_addr = g_freqdump_virt_addr + SHARED_MEMORY_SIZE -
		CS_MAX_PERI_INDEX * 4;
#endif
	if (freqdump_debugfs_init_node() != OK) {
		pr_err("freqdump debugfs init failed!\n");
		return -ENOENT;
	}

#ifdef CONFIG_FREQDUMP_PLATFORM
	freqdump_read_data = kzalloc(sizeof(struct freqdump_data), GFP_KERNEL);
#else
	freqdump_read_data = kzalloc(sizeof(struct freqdump_data), GFP_KERNEL);
#endif
	if (!freqdump_read_data) {
		pr_err("freqdump alloc mem failed!\n");
		debugfs_remove_recursive(freqdump_debugfs_root);
		iounmap(g_freqdump_virt_addr);
		return -ENOMEM;
	}

	return OK;

}
/*lint +e747 +e776 +e801 +e835 +e838 +e845 +e679*/

static void __exit freqdump_debugfs_exit(void)
{
	debugfs_remove_recursive(freqdump_debugfs_root);
	kfree(freqdump_read_data);
	freqdump_read_data = NULL;
	iounmap(g_freqdump_virt_addr);
	pr_err("freqdump removed!\n");
}
/*lint -e528 -esym(528,*)*/
module_init(freqdump_debugfs_init);
module_exit(freqdump_debugfs_exit);
/*lint -e528 +esym(528,*)*/
