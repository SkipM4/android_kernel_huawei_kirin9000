/*
 * Copyright (C) 2004-2020 Jens Axboe <axboe@suse.de> SUSE Labs
 *
 * Discription: blk_scsi_kern.c - block layer implementation of the sg v4 interface
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License version 2.  See the file "COPYING" in the main directory of this
 *  archive for more details.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/blkdev.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/jiffies.h>
#include <linux/percpu.h>
#include <linux/uio.h>
#include <linux/idr.h>
#include <linux/bsg.h>
#include <linux/slab.h>

#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_driver.h>
#include <scsi/sg.h>
#include <scsi/scsi_request.h>
#include <linux/types.h>
#include <linux/version.h>
#if defined(CONFIG_MAS_ORDER_PRESERVE) || defined(CONFIG_MAS_UNISTORE_PRESERVE)
#include "mas_blk.h"
#endif

struct blk_scsi_device {
	struct request_queue *queue;
	spinlock_t lock;
	struct list_head busy_list;
	struct list_head done_list;
	struct hlist_node dev_list;
	atomic_t ref_count;
	int queued_cmds;
	int done_cmds;
	wait_queue_head_t wq_done;
	wait_queue_head_t wq_free;
	char name[20];
	int max_queue;
	unsigned long flags;
};

struct blk_scsi_kern_time {
	u64 req_get_done;
	u64 req_exec_start;
	u64 req_exec_end;
	bool is_current_io;
	unsigned int source_id;
};

#define MAX_TIME_RECORD_COUNT 5
#define RPMB_TIMEOUT_TIME_BLK_EXEC 800000000
static struct blk_scsi_kern_time blk_scsi_cur_time[MAX_TIME_RECORD_COUNT];
enum time_record_type {
	REQ_GET_DONE,
	REQ_EXEC_START,
	REQ_EXEC_END,
};

void blk_scsi_kern_time_stamp_dump(void)
{
	int i;

	pr_err("[%s]: start\n", __func__);
	for (i = 0; i < MAX_TIME_RECORD_COUNT; i++)
		pr_err("req_get_done %llu, req_exec_start %llu, req_exec_end %llu, is_current_io %d, source_id %u\n",
		       blk_scsi_cur_time[i].req_get_done,
		       blk_scsi_cur_time[i].req_exec_start,
		       blk_scsi_cur_time[i].req_exec_end,
		       blk_scsi_cur_time[i].is_current_io,
		       blk_scsi_cur_time[i].source_id);
}

static void blk_scsi_kern_time_record(enum time_record_type type, unsigned int source_id)
{
	static unsigned int i = 0;

	blk_scsi_cur_time[i].source_id = source_id;
	switch (type) {
	case REQ_GET_DONE:
#ifdef CONFIG_DFX_TIME
		blk_scsi_cur_time[i].req_get_done = dfx_getcurtime();
#else
		blk_scsi_cur_time[i].req_get_done = local_clock();
#endif
		break;
	case REQ_EXEC_START:
#ifdef CONFIG_DFX_TIME
		blk_scsi_cur_time[i].req_exec_start = dfx_getcurtime();
#else
		blk_scsi_cur_time[i].req_exec_start = local_clock();
#endif
		break;
	case REQ_EXEC_END:
#ifdef CONFIG_DFX_TIME
		blk_scsi_cur_time[i].req_exec_end = dfx_getcurtime();
#else
		blk_scsi_cur_time[i].req_exec_end = local_clock();
#endif
		blk_scsi_cur_time[i].is_current_io = false;

		/* record the abnormal request takes more than 800 ms */
		if ((blk_scsi_cur_time[i].req_exec_end -
		     blk_scsi_cur_time[i].req_get_done) >
		    RPMB_TIMEOUT_TIME_BLK_EXEC)
			i++;

		if (i >= MAX_TIME_RECORD_COUNT)
			i = 0;
		blk_scsi_cur_time[i].is_current_io = true;
		break;
	}
}

static int blk_kern_fill_sgv4_hdr_rq(struct request_queue *q,
					  struct request *rq,
					  struct sg_io_v4 *hdr)
{
	struct scsi_request *req = scsi_req(rq);

	if (hdr->request_len > BLK_MAX_CDB) {
		req->cmd = kzalloc((size_t)hdr->request_len, GFP_KERNEL);
		if (!req->cmd)
			return -ENOMEM;
	}

	memcpy((void *)req->cmd, /* unsafe_function_ignore: memcpy */
	       (void *)(uintptr_t)hdr->request,
	       (unsigned long)hdr->request_len);

	/*
	 * fill in request structure
	 */
	req->cmd_len = (unsigned short)hdr->request_len;

	rq->timeout = (unsigned int)msecs_to_jiffies(hdr->timeout);
	if (!rq->timeout)
		rq->timeout = q->sg_timeout;
	if (!rq->timeout)
		rq->timeout = BLK_DEFAULT_SG_TIMEOUT;
	if (rq->timeout < BLK_MIN_SG_TIMEOUT)
		rq->timeout = BLK_MIN_SG_TIMEOUT;

	return 0;
}

/*
 * Check if sg_io_v4 from kernel is allowed and valid
 */
static int blk_kern_validate_sgv4_hdr(struct sg_io_v4 *hdr,
					   int *op)
{
	int ret = 0;

	if (hdr->guard != 'Q')
		return -EINVAL;

	switch (hdr->protocol) {
	case BSG_PROTOCOL_SCSI:
		switch (hdr->subprotocol) {
		case BSG_SUB_PROTOCOL_SCSI_CMD:
		case BSG_SUB_PROTOCOL_SCSI_TRANSPORT:
			break;
		default:
			ret = -EINVAL;
		}
		break;
	default:
		ret = -EINVAL;
	}

	*op = hdr->dout_xfer_len ? REQ_OP_SCSI_OUT : REQ_OP_SCSI_IN;
	return ret;
}

/*
 * map sg_io_v4 to a request.
 */
static struct request *blk_kern_map_hdr(struct blk_scsi_device *bd,
					struct sg_io_v4 *hdr,
					unsigned int source_id)
{
	struct request_queue *q = bd->queue;
	struct request *rq = NULL;
	int ret, op;
	unsigned int dxfer_len;
	void *dxferp = NULL;
	struct bsg_class_device *bcd = &q->bsg_dev;

	/* if the LLD has been removed then the bsg_unregister_queue will
	 * eventually be called and the class_dev was freed, so we can no
	 * longer use this request_queue. Return no such address.
	 */
	if (!bcd->class_dev)
		return ERR_PTR(-ENXIO);

	ret = blk_kern_validate_sgv4_hdr(hdr, &op);
	if (ret)
		return ERR_PTR(ret);

	/*
	 * map scatter-gather elements separately and string them to request
	 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	rq = blk_get_request(q, op, 0);
#else
	rq = blk_get_request(q, op, GFP_KERNEL);
#endif
	if (IS_ERR(rq))
		return rq;

	blk_scsi_kern_time_record(REQ_GET_DONE, source_id);
	ret = blk_kern_fill_sgv4_hdr_rq(q, rq, hdr);
	if (ret)
		goto out;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
	if (op == REQ_OP_SCSI_OUT && hdr->din_xfer_len) {
		if (!test_bit(QUEUE_FLAG_BIDI, &q->queue_flags)) {
			ret = -EOPNOTSUPP;
			goto out;
		}
	}
#endif
	if (hdr->dout_xfer_len) {
		dxfer_len = hdr->dout_xfer_len;
		dxferp = (void *)(uintptr_t)hdr->dout_xferp;
	} else if (hdr->din_xfer_len) {
		dxfer_len = hdr->din_xfer_len;
		dxferp = (void *)(uintptr_t)hdr->din_xferp;
	} else {
		dxfer_len = 0;
	}

	if (dxfer_len) {
		ret = blk_rq_map_kern(q, rq, dxferp, dxfer_len, GFP_KERNEL);
		if (ret)
			goto out;
	}

	return rq;
out:
	scsi_req_free_cmd(scsi_req(rq));
	blk_put_request(rq);
	return ERR_PTR(ret);
}

static int blk_kern_complete_hdr_rq(struct request *rq,
					 struct sg_io_v4 *hdr)
{
	struct scsi_request *req = scsi_req(rq);
	int ret = 0;

	/*
	 * fill in all the output members
	 */
	hdr->device_status = (unsigned int)req->result & 0xff;
	hdr->transport_status = host_byte((unsigned int)(req->result));
	hdr->driver_status = driver_byte((unsigned int)(req->result));
	hdr->info = 0;
	if (hdr->device_status || hdr->transport_status || hdr->driver_status)
		hdr->info |= SG_INFO_CHECK;
	hdr->response_len = 0;

	if (req->sense_len && hdr->response) {
		unsigned int len = min_t(unsigned int, hdr->max_response_len,
				req->sense_len);

		memcpy((void *)(uintptr_t)hdr->response, /* unsafe_function_ignore: memcpy */
		       req->sense,
		       (unsigned long)len);
		hdr->response_len = len;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	if (rq_data_dir(rq) == READ) {
#else
	if (rq->next_rq) {
		hdr->dout_resid = (__s32)req->resid_len;
		hdr->din_resid = (__s32)scsi_req(rq->next_rq)->resid_len;
		blk_put_request(rq->next_rq);
	} else if (rq_data_dir(rq) == READ) {
#endif
		hdr->din_resid = (__s32)req->resid_len;
	} else {
		hdr->dout_resid = (__s32)req->resid_len;
	}

	/*
	 * If the request generated a negative error number, return it
	 * (providing we aren't already returning an error); if it's
	 * just a protocol response (i.e. non negative), that gets
	 * processed above.
	 */
	if (!ret && req->result < 0)
		ret = req->result;

	scsi_req_free_cmd(req);
	blk_put_request(rq);

	return ret;
}

long blk_scsi_kern_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg,
			 bool need_order, unsigned int source_id)
{
	int *uarg = (int *)(uintptr_t)arg;
	int ret;
	struct request *rq = NULL;
	struct sg_io_v4 hdr;
	int at_head;
	struct blk_scsi_device *bd = NULL;
	struct file *pfile = fget(fd);
	if (pfile == NULL)
		return -EFAULT;
	bd = (struct blk_scsi_device *)pfile->private_data;
	switch (cmd) {
	case SG_IO:
		memcpy(&hdr, uarg, sizeof(hdr)); /* unsafe_function_ignore: memcpy */
		rq = blk_kern_map_hdr(bd, &hdr, source_id);
		if (IS_ERR(rq))
			return PTR_ERR(rq);
#if defined(CONFIG_MAS_ORDER_PRESERVE) || defined(CONFIG_MAS_UNISTORE_PRESERVE)
		if (need_order)
			blk_req_set_make_req_nr(rq);
#endif
		at_head = (0 == (hdr.flags & BSG_FLAG_Q_AT_TAIL));
		blk_scsi_kern_time_record(REQ_EXEC_START, source_id);
		blk_execute_rq(bd->queue, NULL, rq, at_head);
		blk_scsi_kern_time_record(REQ_EXEC_END, source_id);
		ret = blk_kern_complete_hdr_rq(rq, &hdr);

		memcpy(uarg, &hdr, sizeof(hdr)); /* unsafe_function_ignore: memcpy */

		break;
	/*
	* block device ioctls
	 */
	default:
		ret = -ENOTTY;
	}
	fput(pfile);
	return ret;
}
