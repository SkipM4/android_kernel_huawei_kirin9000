/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
 * Description:Implement of hisp120
 * Create: 2014-11-11
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/compiler.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <platform_include/camera/native/camera.h>
#include <platform_include/camera/native/hisp120_cfg.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-core.h>
#include <linux/pm_qos.h>
#include <clocksource/arm_arch_timer.h>
#include <asm/arch_timer.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include "cam_log.h"
#include "hisp_intf.h"
#include "platform/sensor_commom.h"
#include <linux/pm_wakeup.h>
#include <linux/mm_iommu.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <platform_include/isp/linux/hisp_mempool.h>
#include <linux/iommu.h>
#include <linux/mutex.h>
#include <cam_buf.h>
#include <securec.h>

#define HISP_MSG_LOG_MOD 100

DEFINE_MUTEX(kernel_rpmsg_service_mutex);

static struct pm_qos_request qos_request_ddr_down_record;
static int current_ddr_bandwidth = 0;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static struct wakeup_source hisp_power_wakelock;
#else
static struct wakeup_source *hisp_power_wakelock = NULL;
#endif
static struct mutex hisp_wake_lock_mutex;
static struct mutex hisp_lowpower_mode_mutex;
static struct mutex hisp_power_lock_mutex;
static struct mutex hisp_mem_lock_mutex;
static int hisp_lowpower_mode;

extern void hisp_boot_stat_dump(void);
extern int hisp_secmem_size_get(unsigned int*);

static void hisp120_deinit_isp_mem(void);
typedef enum _timestamp_state_t {
	TIMESTAMP_UNINTIAL = 0,
	TIMESTAMP_INTIAL,
} timestamp_state_t;

static timestamp_state_t s_timestamp_state;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static struct timeval s_timeval;
#else
static struct timespec64 s_timeval;
#endif
static u32 s_system_couter_rate;
static u64 s_system_counter;

enum hisp120_rpmsg_state {
	RPMSG_UNCONNECTED,
	RPMSG_CONNECTED,
	RPMSG_FAIL,
};

/*
 * These are used for distinguish the rpmsg_msg status
 * The process in hisp120_rpmsg_ept_cb are different
 * for the first receive and later.
 */
enum {
	HISP_SERV_FIRST_RECV,
	HISP_SERV_NOT_FIRST_RECV,
};

/*
 * @brief the instance for rpmsg service
 *
 * When Histar ISP is probed, this sturcture will be initialized,
 * the object is used to send/recv rpmsg and store the rpmsg data
 *
 * @end
 */
struct rpmsg_hisp120_service {
	struct rpmsg_device *rpdev;
	struct mutex send_lock;
	struct mutex recv_lock;
	struct completion *comp;
	struct sk_buff_head queue;
	wait_queue_head_t readq;
	struct rpmsg_endpoint *ept;
	u32 dst;
	int state;
	char recv_count;
};

enum hisp120_mem_pool_attr {
	MEM_POOL_ATTR_READ_WRITE_CACHE = 0,
	MEM_POOL_ATTR_READ_WRITE_SECURITY,
	MEM_POOL_ATTR_READ_WRITE_ISP_SECURITY,
	MEM_POOL_ATTR_READ_WRITE_CACHE_OFF_LINE,
	MEM_POOL_ATTR_MAX,
};

struct hisp120_mem_pool {
	void *ap_va;
	unsigned int prot;
	unsigned int ion_iova;
	unsigned int r8_iova;
	size_t size;
	size_t align_size;
	int active;
	unsigned int security_isp_mode;
	struct sg_table *sgt;
	unsigned int shared_fd;
	unsigned int is_ap_cached;
};

struct isp_mem {
	int active;
	struct dma_buf *dmabuf;
};

/*
 * @brief the instance to talk to hisp driver
 *
 * When Histar ISP is probed, this sturcture will be initialized,
 * the object is used to notify hisp driver when needed.
 *
 * @end
 */
typedef struct _tag_hisp120 {
	hisp_intf_t intf;
	hisp_notify_intf_t *notify;
	char const *name;
	atomic_t opened;
	struct platform_device *pdev; /* by used to get dts node */
	hisp_dt_data_t dt;
	struct hisp120_mem_pool mem_pool[MEM_POOL_ATTR_MAX];
	struct isp_mem mem;
} hisp120_t;

struct rpmsg_service_info {
	struct rpmsg_hisp120_service *kernel_isp_serv;
	struct completion isp_comp;
	int isp_minor;
};

extern void a7_mmu_unmap(unsigned int va, unsigned int size);

/* Store the only rpmsg_hisp120_service pointer to local static rpmsg_local */
static struct rpmsg_service_info rpmsg_local;
static bool remote_processor_up = false;
static int g_hisp_ref = 0;

#define I2HI(i) container_of(i, hisp120_t, intf)

static void hisp120_notify_rpmsg_cb(void);
char const *hisp120_get_name(hisp_intf_t *i);
static int hisp120_config(hisp_intf_t *i, void *cfg);

static int hisp120_power_on(hisp_intf_t *i);
static int hisp120_config_power_on(hisp_intf_t *i, struct hisp_cfg_data *pcfg);
static int hisp120_config_power_off(hisp_intf_t *i);
static int hisp120_power_off(hisp_intf_t *i);

static int hisp120_open(hisp_intf_t *i);
static int hisp120_close(hisp_intf_t *i);
static int hisp120_send_rpmsg(hisp_intf_t *i, hisp_msg_t *m, size_t len);
static int hisp120_recv_rpmsg(hisp_intf_t *i,
	hisp_msg_t *user_addr, size_t len);

static int hisp120_set_sec_fw_buffer(struct hisp_cfg_data *cfg);
static int hisp120_release_sec_fw_buffer(void);
static void hisp120_set_ddrfreq(int ddr_bandwidth);
static void hisp120_release_ddrfreq(void);
static void hisp120_update_ddrfreq(unsigned int ddr_bandwidth);

static int lock_ref;
void hisp120_init_timestamp(void);
void hisp120_destroy_timestamp(void);
void hisp120_set_timestamp(unsigned int *timestampH, unsigned int *timestampL);
void hisp120_handle_msg(hisp_msg_t *msg);
int hisp120_get_lowpower(void);

void hisp120_init_timestamp(void)
{
	long tv_usec;
	s_timestamp_state = TIMESTAMP_INTIAL;
	s_system_couter_rate = arch_timer_get_rate();
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	s_system_counter = arch_counter_get_cntvct();
	do_gettimeofday(&s_timeval);
	tv_usec = s_timeval.tv_usec;
#else
	s_system_counter = __arch_counter_get_cntvct();
	ktime_get_real_ts64(&s_timeval);
	tv_usec = s_timeval.tv_nsec / CAM_NANOSECOND_PER_MICROSECOND;
#endif
	cam_debug("%s state=%u system_counter=%llu rate=%u"
		" time_second=%ld time_usecond=%ld size=%lu",
		__func__,
		(unsigned int)s_timestamp_state,
		s_system_counter,
		s_system_couter_rate,
		s_timeval.tv_sec,
		tv_usec,
		sizeof(s_timeval) / sizeof(u32));
}

void hisp120_destroy_timestamp(void)
{
	int rc = 0;
	s_timestamp_state = TIMESTAMP_UNINTIAL;
	s_system_counter = 0;
	s_system_couter_rate = 0;
	rc = memset_s(&s_timeval, sizeof(struct timeval), 0x00,
		sizeof(struct timeval));
	if (rc != EOK) {
		cam_err("%s: memset_s fail rc.%d", __func__, rc);
		return;
	}
}

/* Function declaration */
/**********************************************
 * |-----pow-on------->||<----  fw-SOF ---->|
 * timeval(got)----------------->fw_timeval=?
 * system_counter(got)----------------->fw_sys_counter(got)
 *
 * fw_timeval = timeval + (fw_sys_counter - system_counter)
 *
 * With a base position(<timeval, system_counter>, we get it at same time),
 * we can calculate fw_timeval with fw syscounter
 * and deliver it to hal. Hal then gets second and microsecond
 *********************************************/
void hisp120_set_timestamp(unsigned int *timestamp_high, unsigned int *timestamp_low)
{
	u64 fw_micro_second = 0;
	u64 fw_sys_counter = 0;
	u64 micro_second = 0;

	if (TIMESTAMP_UNINTIAL == s_timestamp_state) {
		cam_err("%s wouldn't enter this branch. ", __func__);
		hisp120_init_timestamp();
	}

	if (timestamp_high == NULL || timestamp_low == NULL) {
		cam_err("%s timestamp_high or timestamp_low is null. ", __func__);
		return;
	}

	cam_debug("%s ack_high:0x%x ack_low:0x%x", __func__,
		*timestamp_high, *timestamp_low);

	if (*timestamp_high == 0 && *timestamp_low == 0)
		return;

	fw_sys_counter = ((u64)(*timestamp_high) << 32) | (u64)(*timestamp_low);
	micro_second = (fw_sys_counter - s_system_counter) *
		CAM_MICROSECOND_PER_SECOND / s_system_couter_rate;

	/* chang nano second to micro second */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	fw_micro_second =
		(micro_second / CAM_MICROSECOND_PER_SECOND +
		s_timeval.tv_sec) * CAM_MICROSECOND_PER_SECOND +
		((micro_second % CAM_MICROSECOND_PER_SECOND) +
		s_timeval.tv_usec);
#else
	fw_micro_second =
		(micro_second / CAM_MICROSECOND_PER_SECOND +
		s_timeval.tv_sec) * CAM_MICROSECOND_PER_SECOND +
		((micro_second % CAM_MICROSECOND_PER_SECOND) +
		(s_timeval.tv_nsec / CAM_NANOSECOND_PER_MICROSECOND));
#endif
	/* 32: offset */
	*timestamp_high = (u32)(fw_micro_second >> 32 & 0xFFFFFFFF);
	*timestamp_low = (u32)(fw_micro_second & 0xFFFFFFFF);

	cam_debug("%s h:0x%x l:0x%x", __func__, *timestamp_high, *timestamp_low);
}

void hisp120_handle_msg(hisp_msg_t *msg)
{
	if (msg == NULL)
		return;
	switch (msg->api_name) {
	case REQUEST_RESPONSE:
		hisp120_set_timestamp(&(msg->u.ack_request.timestampH),
			&(msg->u.ack_request.timestampL));
		break;
	case MSG_EVENT_SENT:
		hisp120_set_timestamp(&(msg->u.event_sent.timestampH),
			&(msg->u.event_sent.timestampL));
		break;

	default:
		break;
	}
}

int hisp120_get_lowpower(void)
{
	int ret = 0;
	mutex_lock(&hisp_lowpower_mode_mutex);
	if (hisp_lowpower_mode)
		ret = 1;
	mutex_unlock(&hisp_lowpower_mode_mutex);

	return ret;
}

static hisp_vtbl_t s_vtbl_hisp120 = {
	.get_name = hisp120_get_name,
	.config = hisp120_config,
	.power_on = hisp120_power_on,
	.power_off = hisp120_power_off,
	.send_rpmsg = hisp120_send_rpmsg,
	.recv_rpmsg = hisp120_recv_rpmsg,
	.open = hisp120_open,
	.close = hisp120_close,
};

static hisp120_t s_hisp120 = {
	.intf = {.vtbl = &s_vtbl_hisp120,},
	.name = "hisp120",
};

static void hisp120_notify_rpmsg_cb(void)
{
	hisp_event_t isp_ev;
	isp_ev.kind = HISP_RPMSG_CB;
	hisp_notify_intf_rpmsg_cb(s_hisp120.notify, &isp_ev);
}


/* Function declaration */
/**********************************************
 *Save the rpmsg from isp to locally skb queue.
 *Only called by hisp120_rpmsg_ept_cb when api_name
 *is NOT POWER_REQ, will notify user space through HISP
 *********************************************/
static void hisp120_save_rpmsg_data(void *data, int len)
{
	struct rpmsg_hisp120_service *kernel_serv = rpmsg_local.kernel_isp_serv;
	struct sk_buff *skb = NULL;
	unsigned char *skbdata = NULL;
	int rc = 0;

	if (kernel_serv == NULL) {
		cam_err("%s: kernel_serv is NULL", __func__);
		return;
	}
	hisp_assert(data != NULL);
	if (data == NULL)
		return;
	hisp_assert(len > 0);

	skb = alloc_skb(len, GFP_KERNEL);
	if (!skb) {
		cam_err("%s() %d failed: alloc_skb len is %u", __func__,
			__LINE__, len);
		return;
	}

	skbdata = skb_put(skb, (u32)len);
	rc = memcpy_s((void*)skbdata, len, data, len);
	if (rc != EOK)
		cam_err("%s: memcpy_s fail rc.%d ", __func__, rc);

	/*add skb to skb queue */
	mutex_lock(&kernel_serv->recv_lock);
	skb_queue_tail(&kernel_serv->queue, skb);
	mutex_unlock(&kernel_serv->recv_lock);

	wake_up_interruptible(&kernel_serv->readq);
	hisp120_notify_rpmsg_cb();
}

/* Function declaration */
/**********************************************
 * Power up CSI/DPHY/sensor according to isp req
 * Only called by hisp120_rpmsg_ept_cb when api_name
 * is POWER_REQ, and will send a POWER_RSP to isp
 * after power request done.
 *********************************************/

static int hisp120_rpmsg_ept_cb(struct rpmsg_device *rpdev,
	void *data, int len, void *priv, u32 src)
{
	struct rpmsg_hisp120_service *kernel_serv = rpmsg_local.kernel_isp_serv;
	hisp_msg_t *msg = NULL;
	struct rpmsg_hdr *rpmsg_msg = NULL;

	hisp_recvin((void*)data);
	if (kernel_serv == NULL) {
		cam_err("func %s: kernel_serv is NULL", __func__);
		return -EINVAL;
	}
	if (data == NULL) {
		cam_err("func %s: data is NULL", __func__);
		return -EINVAL;
	}

	hisp_assert(len > 0);

	if (RPMSG_CONNECTED != kernel_serv->state) {
		hisp_assert(RPMSG_UNCONNECTED == kernel_serv->state);
		rpmsg_msg = container_of(data, struct rpmsg_hdr, data);
		cam_info("msg src.%u, msg dst.%u\n", rpmsg_msg->src,
			rpmsg_msg->dst);

		/*add instance dst and modify the instance state */
		kernel_serv->dst = rpmsg_msg->src;
		kernel_serv->state = RPMSG_CONNECTED;
	}

	msg = (hisp_msg_t*)(data);
	/* save the data and wait for hisp120_recv_rpmsg to get the data */
	hisp_recvx(data);
	hisp120_save_rpmsg_data(data, len);
	return 0;
}

char const *hisp120_get_name(hisp_intf_t *i)
{
	hisp120_t *hi = NULL;
	hisp_assert(i != NULL);
	hi = I2HI(i);
	if (hi == NULL) {
		cam_err("func %s: hi is NULL", __func__);
		return NULL;
	}
	return hi->name;
}

static int hisp120_unmap_a7isp_addr(void *cfg)
{
#ifdef CONFIG_KERNEL_CAMERA_ISP_SECURE
	struct hisp_cfg_data *pcfg = NULL;
	if (cfg == NULL) {
		("func %s: cfg is NULL", __func__);
		return -1;
	}

	pcfg = (struct hisp_cfg_data*)cfg;

	cam_info("func %s: a7 %x, size %x", __func__, pcfg->param.moduleAddr,
	pcfg->param.size);
	a7_mmu_unmap(pcfg->param.moduleAddr, pcfg->param.size);
	return 0;
#else
	return -ENODEV;
#endif
}

static int hisp120_get_a7isp_addr(void *cfg)
{
	int ret = -ENODEV;
#ifdef CONFIG_KERNEL_CAMERA_ISP_SECURE
	struct hisp_cfg_data *pcfg = NULL;
	struct scatterlist *sg = NULL;
	struct sg_table *table = NULL;
	struct dma_buf *buf = NULL;
	struct dma_buf_attachment *attach = NULL;

	if (cfg == NULL) {
		cam_err("func %s: cfg is NULL", __func__);
		return -1;
	}
	pcfg = (struct hisp_cfg_data*)cfg;

	mutex_lock(&kernel_rpmsg_service_mutex);

	ret = hisp_get_sg_table(pcfg->param.sharedFd, &(s_hisp120.pdev->dev),
		&buf,&attach,&table);
	if (ret < 0) {
		cam_err("func %s: get_sg_table failed", __func__);
		goto err_ion_client;
	}

	if (table)
		sg = table->sgl;

	pcfg->param.moduleAddr = a7_mmu_map(sg, pcfg->param.size,
		pcfg->param.prot, pcfg->param.type);

	cam_info("func %s: a8 %x", __func__, pcfg->param.moduleAddr);

	ret = 0;
	hisp_free_dma_buf(&buf, &attach, &table);
err_ion_client:
	mutex_unlock(&kernel_rpmsg_service_mutex);
#endif
	return ret;
}

static int buffer_is_invalid(int share_fd, unsigned int req_addr,
	unsigned int req_size)
{
	int ret = 0;
	struct iommu_format fmt = {0};

	ret = cam_buf_map_iommu(share_fd, &fmt);
	if (ret < 0) {
		cam_err("%s: fail to map iommu", __func__);
		return ret;
	}

	if (req_addr != fmt.iova || req_size > fmt.size) {
		cam_err("%s: req_iova:%#x, req_size:%u", __func__,
			req_addr, req_size);
		cam_err("%s:real_iova:%#llx, real_size:%llu", __func__,
			fmt.iova, fmt.size);
		ret = -ERANGE;
	}
	cam_buf_unmap_iommu(share_fd, &fmt);

	return ret;
}

static int find_suitable_mem_pool(struct hisp_cfg_data *pcfg)
{
	int ipool;
	if (pcfg->param.type == MAP_TYPE_RAW2YUV) {
		ipool = MEM_POOL_ATTR_READ_WRITE_CACHE_OFF_LINE;
	} else if (pcfg->param.type == MAP_TYPE_STATIC_ISP_SEC) {
		ipool = MEM_POOL_ATTR_READ_WRITE_ISP_SECURITY;
	} else {
		for (ipool = 0; ipool < MEM_POOL_ATTR_MAX; ipool++) {
			if (s_hisp120.mem_pool[ipool].prot ==
				pcfg->param.prot) {
				break;
			}
		}

		if (ipool >= MEM_POOL_ATTR_MAX) {
			cam_err("func %s: no pool hit for prot:%d",
				__func__, pcfg->param.prot);
			return -EINVAL;
		}
	}
	return ipool;
}

static int hisp120_init_r8isp_memory_pool(void *cfg)
{
	int ipool;
	uint32_t r8va;
	struct hisp_cfg_data *pcfg = NULL;
	struct sg_table *sgt = NULL;
	enum maptype enm_map_type;
	int ret;

	if (cfg == NULL) {
		cam_err("func %s: cfg is NULL", __func__);
		return -1;
	}

	pcfg = (struct hisp_cfg_data*)cfg;
	cam_info("%s: pool cfg vaddr=0x%pK, iova=0x%x, size=0x%x", __func__,
		pcfg->param.vaddr,
		pcfg->param.iova,
		pcfg->param.size);
	cam_info("%s: type=%d, prot=0x%x align=0x%zd sec=0x%x", __func__,
		pcfg->param.type,
		pcfg->param.prot,
		pcfg->param.pool_align_size,
		pcfg->param.security_isp_mode);

	if ((!pcfg->param.security_isp_mode) &&
		buffer_is_invalid(pcfg->param.sharedFd, pcfg->param.iova,
		pcfg->param.size)) {
		cam_err("check buffer fail");
		return -EINVAL;
	}

	/* find suitable mem pool */
	ipool = find_suitable_mem_pool(pcfg);
	if (ipool < 0)
		return ipool;

	if (enm_map_type != MAP_TYPE_DYNAMIC_SEC) {
		if (ipool == MEM_POOL_ATTR_READ_WRITE_CACHE_OFF_LINE) {
			enm_map_type = MAP_TYPE_RAW2YUV;
		} else {
			enm_map_type = pcfg->param.security_isp_mode ?
				MAP_TYPE_STATIC_SEC : MAP_TYPE_DYNAMIC;
		}
	}

	/* take care of putting sgtable. */
	sgt = cam_buf_get_sgtable(pcfg->param.sharedFd);
	if (IS_ERR_OR_NULL(sgt)) {
		cam_err("%s: fail to get sgtable", __func__);
		return -ENOENT;
	}

	mutex_lock(&kernel_rpmsg_service_mutex);

	if (get_secmem_type() == 1 && (pcfg->param.type == MAP_TYPE_DYNAMIC_SEC ||
				pcfg->param.type == MAP_TYPE_STATIC_SEC ||
				pcfg->param.type == MAP_TYPE_STATIC_ISP_SEC)) {
		ret = hisp_secmem_ca_map((unsigned int)ipool,
				pcfg->param.sharedFd, pcfg->param.size);
		if (ret != 0) {
			cam_err("%s: hisp_secmem_ca_map failed", __func__);
			mutex_unlock(&kernel_rpmsg_service_mutex);
			cam_buf_put_sgtable(sgt);
			return -ENOMEM;
		}
	}

	r8va = hisp_mem_map_setup(sgt->sgl, pcfg->param.iova, pcfg->param.size,
		pcfg->param.prot, (unsigned int)ipool, enm_map_type,
		(unsigned int)(pcfg->param.pool_align_size));
	if (!r8va) {
		cam_err("%s: hisp_mem_map_setup failed", __func__);
		mutex_unlock(&kernel_rpmsg_service_mutex);
		cam_buf_put_sgtable(sgt);
		return -ENOMEM;
	}

	/* hold sg_table things, release at deinit. */
	s_hisp120.mem_pool[ipool].sgt = sgt;
	s_hisp120.mem_pool[ipool].r8_iova = r8va;
	s_hisp120.mem_pool[ipool].ap_va = pcfg->param.vaddr;
	s_hisp120.mem_pool[ipool].ion_iova = pcfg->param.iova;
	s_hisp120.mem_pool[ipool].size = pcfg->param.size;
	s_hisp120.mem_pool[ipool].align_size = pcfg->param.pool_align_size;
	s_hisp120.mem_pool[ipool].security_isp_mode =
		pcfg->param.security_isp_mode;

	s_hisp120.mem_pool[ipool].shared_fd = pcfg->param.sharedFd;
	s_hisp120.mem_pool[ipool].is_ap_cached = pcfg->param.isApCached;

	/*
	 * ion iova isn't equal r8 iova, security or unsecurity, align etc
	 * return r8 iova to daemon, and send to r8 later
	 */
	pcfg->param.iova = s_hisp120.mem_pool[ipool].r8_iova;
	s_hisp120.mem_pool[ipool].active = 1;

	cam_info("func %s: r8_iova_pool_base=0x%x", __func__,
		s_hisp120.mem_pool[ipool].r8_iova);
	mutex_unlock(&kernel_rpmsg_service_mutex);
	return 0;
}

static int hisp120_deinit_r8isp_memory_pool(void *cfg)
{
	int ipool;
	int rc;
	struct hisp_cfg_data *pcfg = NULL;

	if (cfg == NULL) {
		cam_err("func %s: cfg is NULL", __func__);
		return -EINVAL;
	}

	ipool = find_suitable_mem_pool(cfg);
	if (ipool < 0)
		return ipool;

	mutex_lock(&kernel_rpmsg_service_mutex);
	if (s_hisp120.mem_pool[ipool].active) {
		s_hisp120.mem_pool[ipool].active = 0;
		hisp_mem_pool_destroy((unsigned int)ipool);

		pcfg = (struct hisp_cfg_data *)cfg;
		if (get_secmem_type() == 1 && (pcfg->param.type == MAP_TYPE_DYNAMIC_SEC ||
					pcfg->param.type == MAP_TYPE_STATIC_SEC ||
					pcfg->param.type == MAP_TYPE_STATIC_ISP_SEC)) {
			rc = hisp_secmem_ca_unmap((unsigned int)ipool);
			if (rc != 0)
				cam_err("%s: hisp_secmem_ca_unmap failed",
						__func__);
		}
		/* release sg_table things. */
		cam_buf_put_sgtable(s_hisp120.mem_pool[ipool].sgt);
	}
	rc = memset_s(&(s_hisp120.mem_pool[ipool]),
		sizeof(struct hisp120_mem_pool),
		0, sizeof(struct hisp120_mem_pool));
	if (rc != 0)
		cam_warn("%s: fail for memset_s mem_pool", __func__);
	mutex_unlock(&kernel_rpmsg_service_mutex);

	return 0;
}

/*
 * handle daemon carsh
 * miss ispmanager poweroff
 * miss memory pool deinit
 */
static int hisp120_deinit_r8isp_memory_pool_force(void)
{
	int ipool = 0;
	int rc;
	cam_warn("func %s", __func__);

	mutex_lock(&kernel_rpmsg_service_mutex);
	for (ipool = 0; ipool < MEM_POOL_ATTR_MAX; ipool++) {
		if (s_hisp120.mem_pool[ipool].active) {
			cam_warn("%s: force deiniting pool:%d",
				__func__, ipool);
			s_hisp120.mem_pool[ipool].active = 0;
			hisp_mem_pool_destroy((unsigned int)ipool);
			cam_buf_put_sgtable(s_hisp120.mem_pool[ipool].sgt);
		}
		rc = memset_s(&(s_hisp120.mem_pool[ipool]),
			sizeof(struct hisp120_mem_pool),
			0, sizeof(struct hisp120_mem_pool));
		if (rc != 0)
			cam_warn("%s: fail for memset_s mem_pool",
				__func__);
	}

	mutex_unlock(&kernel_rpmsg_service_mutex);

	return 0;
}

static int hisp120_alloc_r8isp_addr(void *cfg)
{
	int ipool;
	unsigned int r8_iova;
	size_t  offset;
	struct hisp_cfg_data *pcfg = NULL;
	int rc = 0;
	bool secure_mode = false;

	if (cfg == NULL) {
		cam_err("func %s: cfg is NULL", __func__);
		return -1;
	}

	mutex_lock(&kernel_rpmsg_service_mutex);

	pcfg = (struct hisp_cfg_data*)cfg;

	/*
	 * handle static memory
	 * just return r8 reserved iova address == map only
	 */
	if (pcfg->param.type == MAP_TYPE_STATIC) {
#ifndef CONFIG_KERNEL_CAMERA_ISP_SECURE
		mutex_unlock(&kernel_rpmsg_service_mutex);
		return -ENODEV;
#else
		cam_info("func %s static", __func__);
		pcfg->param.iova = a7_mmu_map(NULL, pcfg->param.size,
			pcfg->param.prot, pcfg->param.type);
		mutex_unlock(&kernel_rpmsg_service_mutex);
		return 0;
#endif
	}

	/* handle dynamic carveout alloc */
	if (pcfg->param.type == MAP_TYPE_DYNAMIC_CARVEOUT) {
		cam_debug("func %s dynamic carveout", __func__);
		pcfg->param.iova =
			hisp_mem_pool_alloc_carveout(pcfg->param.size,
				pcfg->param.type);
		mutex_unlock(&kernel_rpmsg_service_mutex);
		return 0;
	}

	for (ipool = 0; ipool < MEM_POOL_ATTR_MAX; ipool++) {
		if (s_hisp120.mem_pool[ipool].security_isp_mode) {
			secure_mode = true;
			break;
		}
	}

	/* hanlde dynamic memory alloc */
	ipool = find_suitable_mem_pool(pcfg);
	if (ipool < 0) {
		rc = -EINVAL;
		goto alloc_err;
	}

	r8_iova = (unsigned int)hisp_mem_pool_alloc_iova(pcfg->param.size,
		(unsigned int)ipool);
	if (!r8_iova) {
		cam_err("func %s: hisp_mem_pool_alloc_iova error", __func__);
		rc = -ENOMEM;
		goto alloc_err;
	}

	/*
	 * offset calculator
	 * security mode, pool base is r8_iova, is security address, not align
	 * normal mode, pool base is ion_iova, is normal address, align by isp.
	 */
	if (pcfg->param.type == MAP_TYPE_RAW2YUV) {
		offset = r8_iova - s_hisp120.mem_pool[ipool].r8_iova;
	} else {
#ifdef HISP130_USE_V3
		offset = r8_iova - s_hisp120.mem_pool[ipool].r8_iova;
#else
		if (secure_mode)
			offset = r8_iova - s_hisp120.mem_pool[ipool].r8_iova;
		else
			offset = r8_iova - s_hisp120.mem_pool[ipool].ion_iova;
#endif
	}

	if (offset > s_hisp120.mem_pool[ipool].size) {
		cam_err("func %s: r8_iova invalid", __func__);
		rc = -EFAULT;
		goto alloc_err;
	}

#ifdef HISP130_USE_V3
	pcfg->param.vaddr = (void *)(
		((unsigned char *)s_hisp120.mem_pool[ipool].ap_va) + offset);
	pcfg->param.ispCoreIova = s_hisp120.mem_pool[ipool].ion_iova + offset;
	pcfg->param.iova = r8_iova;
	pcfg->param.offset_in_pool = offset;
	pcfg->param.sharedFd = s_hisp120.mem_pool[ipool].shared_fd;
	pcfg->param.isApCached = s_hisp120.mem_pool[ipool].is_ap_cached;
#else
	pcfg->param.vaddr =
		(void*)(((unsigned char*)s_hisp120.mem_pool[ipool].ap_va) +
			offset);
	pcfg->param.iova = r8_iova;
	pcfg->param.offset_in_pool = offset;
#endif
	mutex_unlock(&kernel_rpmsg_service_mutex);
	return 0;

alloc_err:
	mutex_unlock(&kernel_rpmsg_service_mutex);
	return rc;
}

static int hisp120_free_r8isp_addr(void *cfg)
{
	int rc = 0;
	int ipool;
	struct hisp_cfg_data *pcfg = NULL;

	if (cfg == NULL) {
		cam_err("func %s: cfg is NULL", __func__);
		return -1;
	}

	mutex_lock(&kernel_rpmsg_service_mutex);
	pcfg = (struct hisp_cfg_data*)cfg;
	/* handle static memory, unmap only */
	if (pcfg->param.type == MAP_TYPE_STATIC) {
#ifndef CONFIG_KERNEL_CAMERA_ISP_SECURE
		mutex_unlock(&kernel_rpmsg_service_mutex);
		return -ENODEV;
#else
		cam_info("func %s static", __func__);
		a7_mmu_unmap(pcfg->param.iova, pcfg->param.size);
		mutex_unlock(&kernel_rpmsg_service_mutex);
		return 0;
#endif
	}

	/* handle dynamic carveout free */
	if (pcfg->param.type == MAP_TYPE_DYNAMIC_CARVEOUT) {
		cam_debug("func %s dynamic carveout", __func__);
		rc = hisp_mem_pool_free_carveout(pcfg->param.iova,
			pcfg->param.size);
	if (rc)
		cam_err("func %s: hisp_mem_pool_free_carveout error",
			__func__);
		mutex_unlock(&kernel_rpmsg_service_mutex);
		return 0;
	}

	/* hanlde dynamic memory alloc */
	ipool = find_suitable_mem_pool(pcfg);
	if (ipool < 0) {
		rc = -EFAULT;
		goto free_err;
	}

	rc = (int)hisp_mem_pool_free_iova((unsigned int)ipool, pcfg->param.iova,
		pcfg->param.size);
	if (rc) {
		cam_err("func %s: hisp_mem_pool_free_iova error", __func__);
		rc = -EFAULT;
		goto free_err;
	}

	mutex_unlock(&kernel_rpmsg_service_mutex);
	return 0;

free_err:
	mutex_unlock(&kernel_rpmsg_service_mutex);
	return rc;
}

static int hisp120_mem_pool_pre_init(void)
{
	int ipool = 0;
	int prot = 0;
	int rc;

	for (ipool = 0; ipool < MEM_POOL_ATTR_MAX; ipool++) {
		rc = memset_s(&(s_hisp120.mem_pool[ipool]),
			sizeof(struct hisp120_mem_pool),
			0, sizeof(struct hisp120_mem_pool));
		if (rc != 0)
			cam_warn("%s: fail for memset_s mem_pool",
				__func__);

		switch (ipool) {
		case MEM_POOL_ATTR_READ_WRITE_CACHE:
		case MEM_POOL_ATTR_READ_WRITE_CACHE_OFF_LINE:
			prot = IOMMU_READ | IOMMU_WRITE | IOMMU_CACHE;
			break;
		case MEM_POOL_ATTR_READ_WRITE_SECURITY:
		case MEM_POOL_ATTR_READ_WRITE_ISP_SECURITY:
			prot = IOMMU_READ | IOMMU_WRITE | IOMMU_CACHE |
				IOMMU_SEC;
			break;
		default:
			prot = -1;
			break;
		}

		cam_debug("%s ipool %d prot 0x%x", __func__, ipool, prot);

		if (prot < 0) {
			cam_err("%s unkown ipool %d prot 0x%x",
				__func__, ipool, prot);
			return -EINVAL;
		}

		s_hisp120.mem_pool[ipool].prot = (unsigned int)prot;
	}

	return 0;
}

static int hisp120_mem_pool_later_deinit(void)
{
	int ipool = 0;
	int rc;
	cam_debug("%s", __func__);

	for (ipool = 0; ipool < MEM_POOL_ATTR_MAX; ipool++) {
		if (ipool == MEM_POOL_ATTR_READ_WRITE_CACHE_OFF_LINE)
			continue;
		if (s_hisp120.mem_pool[ipool].active) {
			cam_warn("%s: force deiniting pool:%d",
				__func__, ipool);
			s_hisp120.mem_pool[ipool].active = 0;
			hisp_mem_pool_destroy((unsigned int)ipool);
			cam_buf_put_sgtable(s_hisp120.mem_pool[ipool].sgt);
		}
		rc = memset_s(&s_hisp120.mem_pool[ipool],
			sizeof(struct hisp120_mem_pool),
			0, sizeof(struct hisp120_mem_pool));
		if (rc != 0)
			cam_warn("%s: fail for memset_s mem_pool",
				__func__);
	}

	return 0;
}

static int hisp120_phy_csi_connect(void *pdata)
{
	int rc = 0;
	msg_req_connect_camera_t *req_connect = NULL;

	if (pdata == NULL) {
		cam_err("%s: data is NULL", __func__);
		return -1;
	}
	req_connect = (msg_req_connect_camera_t*)(pdata);

	rc = hisp_phy_csi_connect(
		(struct hisp_phy_info_t*)&(req_connect->phy_info),
		req_connect->csi_index);
	if (rc != 0)
		cam_err("%s: phy csi connect fail:%d", __func__, rc);
	return rc;
}

static int hisp120_config_power_on(hisp_intf_t *i, struct hisp_cfg_data *pcfg)
{
	int rc = 0;
	mutex_lock(&hisp_lowpower_mode_mutex);
	hisp_lowpower_mode = pcfg->mode;
	mutex_unlock(&hisp_lowpower_mode_mutex);

	mutex_lock(&hisp_power_lock_mutex);
	cam_info("%s, enter cam_secmem_type = %d", __func__, get_secmem_type());
	if (remote_processor_up) {
		rc = hisp120_power_off(i);
		if (rc != 0) {
			mutex_unlock(&hisp_power_lock_mutex);
			cam_info("%s hisp120_power_off failed ", __func__);
			return rc;
		}
		hisp120_deinit_r8isp_memory_pool_force();
	}

	if (pcfg->isSecure == 0)
		hisp_set_boot_mode(NONSEC_CASE);
	else if (pcfg->isSecure == 1)
		hisp_set_boot_mode(SEC_CASE);
	else
		cam_info("%s invalid mode ", __func__);
	cam_notice("%s power on the hisp120. ", __func__);

	rc = hisp120_power_on(i);
	mutex_unlock(&hisp_power_lock_mutex);
	return rc;
}

static int hisp120_config_power_off(hisp_intf_t *i)
{
	int rc = 0;
	mutex_lock(&hisp_power_lock_mutex);
	mutex_lock(&hisp_lowpower_mode_mutex);
	if (hisp_lowpower_mode != 0)
		hisp_lowpower_mode = 0;
	mutex_unlock(&hisp_lowpower_mode_mutex);
	if (remote_processor_up) {
		cam_notice("%s power off the hisp120", __func__);
		rc = hisp120_power_off(i);
	}
	mutex_unlock(&hisp_power_lock_mutex);
	return rc;
}

static int hisp120_config(hisp_intf_t *i, void *cfg)
{
	int rc = 0;
	hisp120_t *hi = NULL;
	struct hisp_cfg_data *pcfg = NULL;

	hisp_assert(i != NULL);
	if (cfg == NULL) {
		cam_err("func %s: cfg is NULL", __func__);
		return -1;
	}
	pcfg = (struct hisp_cfg_data*)cfg;
	hi = I2HI(i);
	hisp_assert(hi != NULL);

	switch (pcfg->cfgtype) {
	case HISP_CONFIG_POWER_ON:
		rc = hisp120_config_power_on(i, pcfg);
		break;
	case HISP_CONFIG_POWER_OFF:
		rc = hisp120_config_power_off(i);
		break;
	case HISP_CONFIG_GET_MAP_ADDR:
		rc = hisp120_get_a7isp_addr(cfg);
		cam_notice("%s get a7 map address 0x%x",
			__func__, pcfg->param.moduleAddr);
		break;
	case HISP_CONFIG_UNMAP_ADDR:
		cam_notice("%s unmap a7 address from isp atf", __func__);
		rc = hisp120_unmap_a7isp_addr(cfg);
		break;

	case HISP_CONFIG_INIT_MEMORY_POOL:
		rc = hisp120_init_r8isp_memory_pool(cfg);
		break;

	case HISP_CONFIG_DEINIT_MEMORY_POOL:
		rc = hisp120_deinit_r8isp_memory_pool(cfg);
		break;

	case HISP_CONFIG_ALLOC_MEM:
		rc = hisp120_alloc_r8isp_addr(cfg);
		break;

	case HISP_CONFIG_FREE_MEM:
		rc = hisp120_free_r8isp_addr(cfg);
		break;
	case HISP_CONFIG_PROC_TIMEOUT:
		cam_info("%s message_id.0x%x", __func__, pcfg->cfgdata[0]);
		hisp_dump_rpmsg_with_id(pcfg->cfgdata[0]);
		break;
	case HISP_CONFIG_GET_SEC_ISPFW_SIZE:
		if (get_secmem_type() == 1)
			rc = hisp_secboot_memsize_get_from_type(pcfg->secMemType, &pcfg->buf_size);
		else
			rc = hisp_secmem_size_get(&pcfg->buf_size);
		break;
	case HISP_CONFIG_SET_SEC_ISPFW_BUFFER:
		rc = hisp120_set_sec_fw_buffer(cfg);
		break;
	case HISP_CONFIG_RELEASE_SEC_ISPFW_BUFFER:
		rc = hisp120_release_sec_fw_buffer();
		break;
	case HISP_CONFIG_PHY_CSI_CONNECT:
		rc = hisp120_phy_csi_connect((void *)(pcfg->cfgdata));
		break;
	case HISP_CONFIG_SECBOOT_PREPARE:
		rc = hisp_secboot_prepare();
		break;
	case HISP_CONFIG_SECBOOT_UNPREPARE:
		rc = 0;
		break;
	default:
		cam_err("%s: unsupported cmd:%#x", __func__, pcfg->cfgtype);
		break;
	}

	if (rc < 0)
		cam_err("%s: cmd:%#x fail, rc:%u",
			__func__, pcfg->cfgtype, rc);
	return rc;
}

static int hisp120_open(hisp_intf_t *i)
{
	cam_info("%s hisp120 device open", __func__);

	mutex_lock(&hisp_power_lock_mutex);
	g_hisp_ref++;
	mutex_unlock(&hisp_power_lock_mutex);
	return 0;
}

static int hisp120_close(hisp_intf_t *i)
{
	int rc = 0;
	cam_info("%s hisp120 device close", __func__);
	mutex_lock(&hisp_power_lock_mutex);

	if (g_hisp_ref)
		g_hisp_ref--;

	if ((g_hisp_ref == 0) && remote_processor_up) {
		cam_warn("%s hisp120 is still on power-on state, power off it",
			__func__);

		rc = hisp120_power_off(i);
		if (rc != 0)
			cam_err("failed to hisp120 power off");
		hisp120_deinit_r8isp_memory_pool_force();
	}
	if (g_hisp_ref == 0)
		hisp120_deinit_isp_mem();

	mutex_unlock(&hisp_power_lock_mutex);
	return rc;
}

static int hisp120_power_on(hisp_intf_t *i)
{
	int rc = 0;
	bool rproc_enabled = false;
	bool hi_opened = false;
	hisp120_t *hi = NULL;
	unsigned long current_jiffies = jiffies;
	uint32_t timeout = hw_is_fpga_board() ? 600000 : 15000;

	struct rpmsg_hisp120_service *kernel_serv = NULL;
	struct rpmsg_channel_info chinfo = {
		.src = RPMSG_ADDR_ANY,
	};
	if (i == NULL)
		return -1;
	hi = I2HI(i);

	cam_info("%s enter ....\n", __func__);

	mutex_lock(&hisp_wake_lock_mutex);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	if (!hisp_power_wakelock.active) {
		__pm_stay_awake(&hisp_power_wakelock);
		cam_info("%s hisp power on enter, wake lock\n", __func__);
	}
#else
	if (!hisp_power_wakelock->active) {
		__pm_stay_awake(hisp_power_wakelock);
		cam_info("%s hisp power on enter, wake lock", __func__);
	}
#endif
	mutex_unlock(&hisp_wake_lock_mutex);

	mutex_lock(&kernel_rpmsg_service_mutex);
	if (!atomic_read((&hi->opened))) {
		if (!hw_is_fpga_board()) {
			if (!IS_ERR(hi->dt.pinctrl_default)) {
				rc = pinctrl_select_state(hi->dt.pinctrl,
					hi->dt.pinctrl_default);
				if (rc != 0)
					goto FAILED_RET;
			}
		}

		hisp_rpmsgrefs_reset();
		rc = hisp_rproc_enable();
		if (rc != 0)
			goto FAILED_RET;
		rproc_enabled = true;

		rc = wait_for_completion_timeout(&rpmsg_local.isp_comp,
			msecs_to_jiffies(timeout));
		if (rc == 0) {
			rc = -ETIME;
			hisp_boot_stat_dump();
			goto FAILED_RET;
		} else {
			cam_info("%s() %d after wait completion, rc = %d!\n",
				__func__, __LINE__, rc);
			rc = 0;
		}

		atomic_inc(&hi->opened);
		hi_opened = true;
	} else {
		cam_notice("%s isp has been opened.\n", __func__);
	}
	remote_processor_up = true;
	kernel_serv = rpmsg_local.kernel_isp_serv;
	if (!kernel_serv) {
		rc = -ENODEV;
		goto FAILED_RET;
	}

	/*assign a new, unique, local address and associate instance with it */

	kernel_serv->ept = rpmsg_create_ept(kernel_serv->rpdev,
		hisp120_rpmsg_ept_cb, kernel_serv, chinfo);

	if (!kernel_serv->ept) {
		kernel_serv->state = RPMSG_FAIL;
		rc = -ENOMEM;
		goto FAILED_RET;
	}
	cam_info("%s() %d kernel_serv->rpdev:src.%d, dst.%d\n",
		__func__, __LINE__,
		kernel_serv->rpdev->src, kernel_serv->rpdev->dst);
	kernel_serv->state = RPMSG_CONNECTED;

	/* set the instance recv_count */
	kernel_serv->recv_count = HISP_SERV_FIRST_RECV;

	hisp120_init_timestamp();

	if (hisp120_mem_pool_pre_init()) {
		cam_err("failed to pre init mem pool \n");
		rc = -ENOMEM;
		goto FAILED_RET;
	}

	mutex_unlock(&kernel_rpmsg_service_mutex);
	cam_info("%s exit ,power on time:%d....\n", __func__,
		jiffies_to_msecs(jiffies - current_jiffies) );
	return rc;

FAILED_RET:
	if (hi_opened)
		atomic_dec(&hi->opened);

	if (rproc_enabled) {
		hisp_rproc_disable();
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		rproc_set_sync_flag(true);
#endif
	}

	hisp120_mem_pool_later_deinit();
	remote_processor_up = false;

	mutex_unlock(&kernel_rpmsg_service_mutex);

	mutex_lock(&hisp_wake_lock_mutex);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	if (hisp_power_wakelock.active) {
		__pm_relax(&hisp_power_wakelock);
		cam_err("%s hisp power on failed, wake unlock ", __func__);
	}
#else
	if (hisp_power_wakelock->active) {
		__pm_relax(hisp_power_wakelock);
		cam_err("%s hisp power on failed, wake unlock", __func__);
	}
#endif
	mutex_unlock(&hisp_wake_lock_mutex);
	return rc;
}

static int hisp120_power_off(hisp_intf_t *i)
{
	int rc = 0;
	hisp120_t *hi = NULL;
	unsigned long current_jiffies = jiffies;
	struct rpmsg_hisp120_service *kernel_serv = NULL;
	if (i == NULL)
		return -1;
	hi = I2HI(i);

	cam_info("%s enter .... ", __func__);

	/* check the remote processor boot flow */
	if (remote_processor_up == false) {
		rc = -EPERM;
		goto RET;
	}

	kernel_serv = rpmsg_local.kernel_isp_serv;
	if (!kernel_serv) {
		rc = -ENODEV;
		goto RET;
	}

	if (kernel_serv->state == RPMSG_FAIL) {
		rc = -EFAULT;
		goto RET;
	}

	mutex_lock(&kernel_rpmsg_service_mutex);

	if (!kernel_serv->ept) {
		rc = -ENODEV;
		goto UNLOCK_RET;
	}
	rpmsg_destroy_ept(kernel_serv->ept);
	kernel_serv->ept = NULL;

	kernel_serv->state = RPMSG_UNCONNECTED;
	kernel_serv->recv_count = HISP_SERV_FIRST_RECV;

	if (atomic_read((&hi->opened))) {
		hisp_rproc_disable();
		if (!hw_is_fpga_board()) {
			if (!IS_ERR(hi->dt.pinctrl_idle)) {
				rc = pinctrl_select_state(hi->dt.pinctrl,
					hi->dt.pinctrl_idle);
				if (rc != 0) {
					//Empty.
				}
			}
		}

		remote_processor_up = false;
		atomic_dec(&hi->opened);
	} else {
		cam_notice("%s isp hasn't been opened. ", __func__);
	}

	hisp120_destroy_timestamp();
UNLOCK_RET:
	hisp120_mem_pool_later_deinit();

	mutex_unlock(&kernel_rpmsg_service_mutex);
RET:
	cam_info("%s exit ,power 0ff time:%d.... ", __func__,
		jiffies_to_msecs(jiffies - current_jiffies) );

	mutex_lock(&hisp_wake_lock_mutex);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	if (hisp_power_wakelock.active) {
		__pm_relax(&hisp_power_wakelock);
		cam_info("%s hisp power off exit, wake unlock ", __func__);
	}
#else
	if (hisp_power_wakelock->active) {
		__pm_relax(hisp_power_wakelock);
		cam_info("%s hisp power off exit, wake unlock", __func__);
	}
#endif
	mutex_unlock(&hisp_wake_lock_mutex);
	return rc;
}

static void hisp120_rpmsg_remove(struct rpmsg_device *rpdev)
{
	struct rpmsg_hisp120_service *kernel_serv = dev_get_drvdata(&rpdev->dev);
	if (!rpdev) {
		cam_err("func %s: rpdev is NULL", __func__);
		return;
	}

	cam_info("%s enter .... ", __func__);

	if (kernel_serv == NULL) {
		cam_err("%s: kernel_serv == NULL", __func__);
		return;
	}

	mutex_destroy(&kernel_serv->send_lock);
	mutex_destroy(&kernel_serv->recv_lock);

	kfree(kernel_serv);
	rpmsg_local.kernel_isp_serv = NULL;
	cam_notice("rpmsg hisi driver is removed ");
}

static int hisp120_rpmsg_driver_cb(struct rpmsg_device *rpdev,
	void *data, int len, void *priv, u32 src)
{
	cam_info("%s enter .... ", __func__);
	cam_warn("%s() %d uhm, unexpected message", __func__,
		__LINE__);

	print_hex_dump(KERN_DEBUG, __func__, DUMP_PREFIX_NONE, 16, 1,
		data, len, true);
	return 0;
}

static int hisp120_send_rpmsg(hisp_intf_t *i, hisp_msg_t *from_user, size_t len)
{
	int rc = 0;
	hisp120_t *hi = NULL;
	struct rpmsg_hisp120_service *kernel_serv = NULL;
	hisp_msg_t *msg = from_user;
	hisp_assert(i != NULL);
	hisp_assert(from_user != NULL);
	hi = I2HI(i);

	cam_debug("%s enter. api_name(0x%x) ", __func__, msg->api_name);

	if (msg->message_id % HISP_MSG_LOG_MOD == 0)
		cam_info("%s: api_name:%#x, message_id:%#x ", __func__,
				msg->api_name, msg->message_id);

	kernel_serv = rpmsg_local.kernel_isp_serv;
	if (!kernel_serv) {
		cam_err("%s() %d failed: kernel_serv does not exist",
			__func__, __LINE__);
		rc = -ENODEV;
		goto RET;
	}

	if (!kernel_serv->ept) {
		cam_err("%s() %d failed:kernel_serv->ept does not exist",
			__func__, __LINE__);
		rc = -ENODEV;
		goto RET;
	}

	mutex_lock(&kernel_serv->send_lock);
	/*if the msg is the first msg, let's treat it special */
	if (RPMSG_CONNECTED != kernel_serv->state) {
		if (!kernel_serv->rpdev) {
			cam_err("%s() %d failed:kernel_serv->rpdev does not exist", __func__,
				__LINE__);
			rc = -ENODEV;
			goto UNLOCK_RET;
		}
		hisp_sendin(msg);
		rc = rpmsg_send_offchannel(kernel_serv->ept,
			kernel_serv->ept->addr,
			kernel_serv->rpdev->dst, (void*)msg,
			len);
		if (rc)
			cam_err("%s() %d failed: first rpmsg_send_offchannel ret is %d",
				__func__, __LINE__, rc);
		goto UNLOCK_RET;
	}
	hisp_sendin(msg);
	rc = rpmsg_send_offchannel(kernel_serv->ept, kernel_serv->ept->addr,
		kernel_serv->dst, (void*)msg, len);
	if (rc) {
		cam_err("%s() %d failed: rpmsg_send_offchannel ret is %d",
			__func__, __LINE__, rc);
		goto UNLOCK_RET;
	}
UNLOCK_RET:
	mutex_unlock(&kernel_serv->send_lock);
RET:
	return rc;
}

static int hisp120_recv_rpmsg(hisp_intf_t *i, hisp_msg_t *user_addr, size_t len)
{
	int rc = len;
	hisp120_t *hi = NULL;
	struct rpmsg_hisp120_service *kernel_serv = NULL;
	struct sk_buff *skb = NULL;
	hisp_msg_t *msg = NULL;
	hisp_assert(i != NULL);
	if (user_addr == NULL) {
		cam_err("func %s: user_addr is NULL", __func__);
		return -1;
	}
	hi = I2HI(i);

	cam_debug("%s enter. ", __func__);

	kernel_serv = rpmsg_local.kernel_isp_serv;
	if (!kernel_serv) {
		cam_err("%s() %d failed: kernel_serv does not exist",
			__func__, __LINE__);
		rc = -ENODEV;
		goto RET;
	}

	if (kernel_serv->recv_count == HISP_SERV_FIRST_RECV)
		kernel_serv->recv_count = HISP_SERV_NOT_FIRST_RECV;

	if (mutex_lock_interruptible(&kernel_serv->recv_lock)) {
		cam_err("%s() %d failed: mutex_lock_interruptible",
			__func__, __LINE__);
		rc = -ERESTARTSYS;
		goto RET;
	}

	if (kernel_serv->state != RPMSG_CONNECTED) {
		cam_err("%s() %d kernel_serv->state != RPMSG_CONNECTED",
			__func__, __LINE__);
		rc = -ENOTCONN;
		goto UNLOCK_RET;
	}

	/* nothing to read ? */
	/* check if skb_queue is NULL ? */
	if (skb_queue_empty(&kernel_serv->queue)) {
		mutex_unlock(&kernel_serv->recv_lock);
		cam_err("%s() %d skb_queue is empty", __func__, __LINE__);

		/* otherwise block, and wait for data */
		if (wait_event_interruptible_timeout(kernel_serv->readq,
			(!skb_queue_empty
			(&kernel_serv->queue) || kernel_serv->state == RPMSG_FAIL),
			HISP_WAIT_TIMEOUT)) {
			cam_err("%s() %d kernel_serv->state = %d", __func__,
			__LINE__, kernel_serv->state);
			rc = -ERESTARTSYS;
			goto RET;
		}

		if (mutex_lock_interruptible(&kernel_serv->recv_lock)) {
			cam_err("%s() %d failed: mutex_lock_interruptible",
				__func__, __LINE__);
			rc = -ERESTARTSYS;
			goto RET;
		}
	}

	if (kernel_serv->state == RPMSG_FAIL) {
		cam_err("%s() %d state = RPMSG_FAIL", __func__, __LINE__);
		rc = -ENXIO;
		goto UNLOCK_RET;
	}

	skb = skb_dequeue(&kernel_serv->queue);
	if (!skb) {
		cam_err("%s() %d skb is NULL", __func__, __LINE__);
		rc = -EIO;
		goto UNLOCK_RET;
	}

	rc = min((unsigned int)len, skb->len);
	msg = (hisp_msg_t*) (skb->data);
	hisp_recvdone((void*)msg);
	if (msg->api_name == ISP_CPU_POWER_OFF_RESPONSE)
		hisp_rpmsgrefs_dump();
	cam_debug("%s: api_name(0x%x) ", __func__, msg->api_name);

	if (msg->message_id % HISP_MSG_LOG_MOD == 0)
		cam_info("%s: api_name:%#x, message_id:%#x ", __func__,
				msg->api_name, msg->message_id);

	hisp120_handle_msg(msg);
	if (memcpy_s(user_addr, sizeof(hisp_msg_t), msg, rc) != EOK) {
		rc = -EFAULT;
		cam_err("Fail: %s()%d ret = %d ", __func__, __LINE__, rc);
	}
	kfree_skb(skb);

UNLOCK_RET:
	mutex_unlock(&kernel_serv->recv_lock);
RET:
	return rc;
}

int hisp120_set_sec_fw_buffer(struct hisp_cfg_data *cfg)
{
	int rc;
	mutex_lock(&hisp_mem_lock_mutex);
	rc = hisp_set_sec_fw_buffer(cfg);
	if(rc < 0)
		cam_err("%s: fail, rc:%d", __func__, rc);

	if (s_hisp120.mem.active) {
		s_hisp120.mem.active = 0;
		dma_buf_put(s_hisp120.mem.dmabuf);
		cam_warn("%s() %d free isp boot buffer", __func__, __LINE__);
	}

	s_hisp120.mem.dmabuf = dma_buf_get(cfg->share_fd);
	if (IS_ERR_OR_NULL(s_hisp120.mem.dmabuf)) {
		cam_err("Fail: dma buffer error");
		mutex_unlock(&hisp_mem_lock_mutex);
		return -EFAULT;
	}
	s_hisp120.mem.active = 1;
	mutex_unlock(&hisp_mem_lock_mutex);
	return rc;
}

int hisp120_release_sec_fw_buffer(void)
{
	int rc;
	mutex_lock(&hisp_mem_lock_mutex);
	rc = hisp_release_sec_fw_buffer();
	if (rc < 0)
		cam_err("%s: fail, rc:%d", __func__, rc);

	if (s_hisp120.mem.active) {
		s_hisp120.mem.active = 0;
		dma_buf_put(s_hisp120.mem.dmabuf);
	}
	memset_s(&(s_hisp120.mem), sizeof(struct isp_mem),
		0, sizeof(struct isp_mem));
	mutex_unlock(&hisp_mem_lock_mutex);
	return rc;
}

static void hisp120_deinit_isp_mem(void)
{
	cam_info("func %s", __func__);
	mutex_lock(&hisp_mem_lock_mutex);
	if (s_hisp120.mem.active) {
		cam_err("sec isp ex,put dmabuf");
		s_hisp120.mem.active = 0;
		dma_buf_put(s_hisp120.mem.dmabuf);
	}

	memset_s(&(s_hisp120.mem),
		sizeof(struct isp_mem), 0, sizeof(struct isp_mem));
	mutex_unlock(&hisp_mem_lock_mutex);

	return ;
}

static void hisp120_set_ddrfreq(int ddr_bandwidth)
{
	cam_info("%s enter, ddr_bandwidth:%d ", __func__, ddr_bandwidth);
}

static void hisp120_release_ddrfreq(void)
{
	cam_info("%s enter ", __func__);
	if (current_ddr_bandwidth == 0)
		return;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	pm_qos_remove_request(&qos_request_ddr_down_record);
#else
	cpu_latency_qos_remove_request(&qos_request_ddr_down_record);
#endif
	current_ddr_bandwidth = 0;
}

static void hisp120_update_ddrfreq(unsigned int ddr_bandwidth)
{
	cam_info("%s enter, ddr_bandwidth:%u ", __func__, ddr_bandwidth);
	if (!atomic_read(&s_hisp120.opened)) {
		cam_info("%s ,cam is not opened,so u can not set ddr bandwidth ", __func__);
		return;
	}

	if (current_ddr_bandwidth == 0) {
		hisp120_set_ddrfreq(ddr_bandwidth);
	} else if (current_ddr_bandwidth > 0) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		pm_qos_update_request(&qos_request_ddr_down_record,
			ddr_bandwidth);
#else
		cpu_latency_qos_update_request(&qos_request_ddr_down_record,
			ddr_bandwidth);
#endif
		current_ddr_bandwidth = ddr_bandwidth;
	} else {
		cam_err("%s,current_ddr_bandwidth is invalid ", __func__);
	}
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static ssize_t hisp_ddr_freq_ctrl_show(struct device *dev,
	struct device_attribute *attr,char *buf)
{
	cam_info("enter %s,current_ddr_bandwidth:%d ", __func__,
		current_ddr_bandwidth);

	return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%d ",
		current_ddr_bandwidth);
}

static ssize_t hisp_ddr_freq_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ddr_bandwidth = 0;
	if (buf == NULL) {
		cam_err("%s,input buffer is invalid ", __func__);
		return -EINVAL;
	}

	ddr_bandwidth = simple_strtol(buf, NULL, 10);
	cam_info("%s enter, ddr_bandwidth:%d ", __func__, ddr_bandwidth);

	if (ddr_bandwidth < 0) {
		cam_err("%s, ddr_bandwidth is invalid ", __func__);
		return -EINVAL;
	} else if (ddr_bandwidth == 0) {
		hisp120_release_ddrfreq();
	} else if (ddr_bandwidth > 0) {
		hisp120_update_ddrfreq(ddr_bandwidth);
	}

	return count;
}
#pragma GCC diagnostic pop

static int32_t hisp120_rpmsg_probe(struct rpmsg_device *rpdev)
{
	int32_t ret = 0;
	struct rpmsg_hisp120_service *kernel_serv = NULL;
	cam_info("%s enter ", __func__);

	if (rpmsg_local.kernel_isp_serv != NULL) {
		cam_notice("%s kernel_serv is already up", __func__);
		goto SERVER_UP;
	}

	kernel_serv = kzalloc(sizeof(*kernel_serv), GFP_KERNEL);
	if (!kernel_serv) {
		cam_err("%s() %d kzalloc failed", __func__, __LINE__);
		ret = -ENOMEM;
		goto ERROR_RET;
	}
	mutex_init(&kernel_serv->send_lock);
	mutex_init(&kernel_serv->recv_lock);
	skb_queue_head_init(&kernel_serv->queue);
	init_waitqueue_head(&kernel_serv->readq);
	kernel_serv->ept = NULL;
	kernel_serv->comp = &rpmsg_local.isp_comp;

	rpmsg_local.kernel_isp_serv = kernel_serv;
SERVER_UP:
	if (kernel_serv == NULL) {
		cam_err("func %s: kernel_serv is NULL", __func__);
		return -1;
	}
	kernel_serv->rpdev = rpdev;
	kernel_serv->state = RPMSG_UNCONNECTED;
	dev_set_drvdata(&rpdev->dev, kernel_serv);

	complete(kernel_serv->comp);

	cam_info("new HISI connection srv channel: %u -> %u",
		rpdev->src, rpdev->dst);
ERROR_RET:
	return ret;
}

static struct rpmsg_device_id rpmsg_hisp120_id_table[] = {
	{.name = "rpmsg-hisi"},
	{},
};

MODULE_DEVICE_TABLE(platform, rpmsg_hisp120_id_table);

static const struct of_device_id s_hisp120_dt_match[] = {
	{
	 .compatible = "vendor,chip_isp120",
	 .data = &s_hisp120.intf,
	},
	{},
};

MODULE_DEVICE_TABLE(of, s_hisp120_dt_match);
static struct rpmsg_driver rpmsg_hisp120_driver = {
	.drv.name = KBUILD_MODNAME,
	.drv.owner = THIS_MODULE,
	.id_table = rpmsg_hisp120_id_table,
	.probe = hisp120_rpmsg_probe,
	.callback = hisp120_rpmsg_driver_cb,
	.remove = hisp120_rpmsg_remove,
};

#ifdef CONFIG_DFX_DEBUG_FS
static struct device_attribute hisp_ddr_freq_ctrl_attr =
	__ATTR(ddr_freq_ctrl, 0660,
	hisp_ddr_freq_ctrl_show, hisp_ddr_freq_store);
#endif /* CONFIG_DFX_DEBUG_FS */

static int32_t hisp120_platform_probe(struct platform_device *pdev)
{
	int32_t ret = 0;

	cam_info("%s: enter", __func__);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	wakeup_source_init(&hisp_power_wakelock, "hisp_power_wakelock");
#else
	hisp_power_wakelock = wakeup_source_register(&pdev->dev, "hisp_power_wakelock");
	if (!hisp_power_wakelock) {
		cam_err("%s: wakeup source register failed", __func__);
		return -1;
	}
#endif
	mutex_init(&hisp_wake_lock_mutex);
	mutex_init(&hisp_lowpower_mode_mutex);
	mutex_init(&hisp_power_lock_mutex);
	mutex_init(&hisp_mem_lock_mutex);
	ret = hisp_get_dt_data(pdev, &s_hisp120.dt);
	if (ret < 0) {
		cam_err("%s: get dt failed", __func__);
		goto error;
	}

	init_completion(&rpmsg_local.isp_comp);
	ret = hisp_register(pdev, &s_hisp120.intf, &s_hisp120.notify);
	if (ret == 0) {
		atomic_set(&s_hisp120.opened, 0);
	} else {
		cam_err("%s() %d hisp_register failed with ret %d",
			__func__, __LINE__, ret);
		goto error;
	}

	rpmsg_local.kernel_isp_serv = NULL;

	ret = register_rpmsg_driver(&rpmsg_hisp120_driver);
	if (ret != 0) {
		cam_err("%s() %d register_rpmsg_driver failed with ret %d",
			__func__, __LINE__, ret);
		goto error;
	}
	s_hisp120.pdev = pdev;

	memset_s(&(s_hisp120.mem), sizeof(struct isp_mem), 0, sizeof(struct isp_mem));
#ifdef CONFIG_DFX_DEBUG_FS
	ret = device_create_file(&pdev->dev, &hisp_ddr_freq_ctrl_attr);
	if (ret < 0) {
		cam_err("%s failed to creat hisp ddr freq ctrl attribute",
			__func__);
		unregister_rpmsg_driver(&rpmsg_hisp120_driver);
		hisp_unregister(s_hisp120.pdev);
		goto error;
	}
#endif
	return 0;

error:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	wakeup_source_trash(&hisp_power_wakelock);
#else
	wakeup_source_unregister(hisp_power_wakelock);
	hisp_power_wakelock = NULL;
#endif
	mutex_destroy(&hisp_wake_lock_mutex);
	mutex_destroy(&hisp_lowpower_mode_mutex);
	mutex_destroy(&hisp_power_lock_mutex);
	mutex_destroy(&hisp_mem_lock_mutex);
	cam_notice("%s exit with ret = %d ", __func__, ret);
	return ret;
}

static struct platform_driver s_hisp120_driver = {
	.probe = hisp120_platform_probe,
	.driver = {
		.name = "vendor,chip_isp120",
		.owner = THIS_MODULE,
		.of_match_table = s_hisp120_dt_match,
	},
};

static int __init hisp120_init_module(void)
{
	cam_notice("%s enter ", __func__);
	return platform_driver_register(&s_hisp120_driver);
}

static void __exit hisp120_exit_module(void)
{
	cam_notice("%s enter ", __func__);
	hisp_unregister(s_hisp120.pdev);
	platform_driver_unregister(&s_hisp120_driver);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	wakeup_source_trash(&hisp_power_wakelock);
#else
	wakeup_source_unregister(hisp_power_wakelock);
	hisp_power_wakelock = NULL;
#endif
	mutex_destroy(&hisp_wake_lock_mutex);
	mutex_destroy(&hisp_lowpower_mode_mutex);
}

module_init(hisp120_init_module);
module_exit(hisp120_exit_module);
MODULE_DESCRIPTION("hisp120 driver");
MODULE_LICENSE("GPL v2");
