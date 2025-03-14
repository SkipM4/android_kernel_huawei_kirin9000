/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include "securec.h"
#include "rnic_ondemand.h"
#include "rnic_dev_debug.h"


STATIC struct proc_dir_entry *rnic_ondemand_dir = NULL;

STATIC struct rnic_ondemand_context_s rnic_ondemand_ctx = {
	.cfg_value = {
		RNIC_DIAL_MODE_DEFAULT_VALUE,
		RNIC_IDLE_TIME_DEFAULT_VALUE,
		RNIC_EVENT_REPORT_DEFAULT_VALUE
	},
};

/*
 * rnic_ondemand_read_config - Read function of proc file.
 * @user_buf: the user space buffer to write to
 * @count: the size of the user space buffer
 * @ppos : the current position in the buffer
 * @cfg_type: config type
 *
 * On success, the number of bytes read is returned, or negative value is
 * returned on error.
 */
STATIC ssize_t rnic_ondemand_read_config(char __user *user_buf, size_t count,
					 loff_t *ppos, uint32_t cfg_type)
{
	struct rnic_ondemand_context_s *ondemand_ctx = &rnic_ondemand_ctx;
	char buf[RNIC_PROC_BUF_SIZE];
	ssize_t bytes_read;

	if (*ppos > 0) {
		RNIC_LOGE("only allow read from start, cfg_type: %u, *ppos: %llu.",
			  (int)cfg_type, *ppos);
		return 0;
	}

	(void)memset_s(buf, sizeof(buf), 0, sizeof(buf));
	scnprintf(buf, sizeof(buf), "%u", ondemand_ctx->cfg_value[cfg_type]);

	bytes_read = simple_read_from_buffer(user_buf, count, ppos, buf, strlen(buf));
	if (bytes_read < 0)
		RNIC_LOGE("input buf read error, cfg_type: %u, return: %d.",
			  (int)cfg_type, (int)bytes_read);

	return bytes_read;
}

/*
 * rnic_ondemand_write_config - Write function of proc file.
 * @user_buf: the user space buffer to write to
 * @count: the size of the user space buffer
 * @ppos : the current position in the buffer
 * @cfg_type: config type
 *
 * On success, the number of bytes written is returned, or negative value is
 * returned on error.
 */
STATIC ssize_t rnic_ondemand_write_config(const char __user *user_buf, size_t count,
					  loff_t *ppos, uint32_t cfg_type)
{
	struct rnic_ondemand_context_s *ondemand_ctx = &rnic_ondemand_ctx;
	char buf[RNIC_PROC_BUF_SIZE];
	size_t buf_size;
	ssize_t bytes_written;
	uint32_t value = 0;

	if (*ppos > 0) {
		RNIC_LOGE("only allow write from start, cfg_type: %d, *ppos: %llu.",
			  (int)cfg_type, *ppos);
		return 0;
	}

	(void)memset_s(buf, sizeof(buf), 0, sizeof(buf));
	buf_size = count < (sizeof(buf) - 1) ? count : (sizeof(buf) - 1);

	bytes_written = simple_write_to_buffer(buf, buf_size, ppos, user_buf, count);
	if (bytes_written <= 0) {
		RNIC_LOGE("input buf read error, cfg_type: %u, return: %d.",
			  (int)cfg_type, (int)bytes_written);
		return bytes_written;
	}

	if ((size_t)bytes_written != count) {
		RNIC_LOGE("input buf too long, cfg_type: %u, buf: %s.",
			  (int)cfg_type, buf);
		return -EINVAL;
	}

	if(!RNIC_ONDEMAND_ISDIGIT(buf[0])) {
		RNIC_LOGE("input buf is error, cfg_type: %u, buf: %s.",
			  (int)cfg_type, buf);
		return -EINVAL;
	}

	if (kstrtou32(buf, 10, &value)) {
		RNIC_LOGE("convert to u32 fail, cfg_type: %u, buf: %s.",
			  (int)cfg_type, buf);
		return -EINVAL;
	}

	ondemand_ctx->cfg_value[cfg_type] = value;
	if (ondemand_ctx->cfg_func[cfg_type])
		ondemand_ctx->cfg_func[cfg_type](value);

	return bytes_written;
}

/*
 * rnic_dial_mode_read - Read function of ondemand proc file.
 * @file: file handle
 * @user_buf: the user space buffer to write to
 * @count: the size of the user space buffer
 * @ppos : the current position in the buffer
 *
 * On success, the number of bytes read is returned, or negative value is
 * returned on error.
 */
STATIC ssize_t rnic_dial_mode_read(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	uint32_t cfg_type = RNIC_CFG_TYPE_DIAL_MODE;

	return rnic_ondemand_read_config(user_buf, count, ppos, cfg_type);
}

/*
 * rnic_dial_mode_write - Write function of ondemand proc file.
 * @file: file handle
 * @user_buf: the user space buffer to write to
 * @count: the size of the user space buffer
 * @ppos : the current position in the buffer
 *
 * On success, the number of bytes written is returned, or negative value is
 * returned on error.
 */
STATIC ssize_t rnic_dial_mode_write(struct file *file, const char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	uint32_t cfg_type = RNIC_CFG_TYPE_DIAL_MODE;

	return rnic_ondemand_write_config(user_buf, count, ppos, cfg_type);
}

/*
 * rnic_idle_time_read - Read function of idle time proc file.
 * @file: file handle
 * @user_buf: the user space buffer to write to
 * @count: the size of the user space buffer
 * @ppos : the current position in the buffer
 *
 * On success, the number of bytes read is returned, or negative value is
 * returned on error.
 */
STATIC ssize_t rnic_idle_time_read(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	uint32_t cfg_type = RNIC_CFG_TYPE_IDLE_TIME;

	return rnic_ondemand_read_config(user_buf, count, ppos, cfg_type);
}

/*
 * rnic_idle_time_write - Write function of idle time proc file.
 * @file: file handle
 * @user_buf: the user space buffer to write to
 * @count: the size of the user space buffer
 * @ppos : the current position in the buffer
 *
 * On success, the number of bytes written is returned, or negative value is
 * returned on error.
 */
STATIC ssize_t rnic_idle_time_write(struct file *file, const char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	uint32_t cfg_type = RNIC_CFG_TYPE_IDLE_TIME;

	return rnic_ondemand_write_config(user_buf, count, ppos, cfg_type);
}

/*
 * rnic_event_report_read - Read function of dial event report proc file.
 * @file: file handle
 * @user_buf: the user space buffer to write to
 * @count: the size of the user space buffer
 * @ppos : the current position in the buffer
 *
 * On success, the number of bytes read is returned, or negative value is
 * returned on error.
 */
STATIC ssize_t rnic_event_report_read(struct file *file, char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	uint32_t cfg_type = RNIC_CFG_TYPE_EVENT_REPORT;

	return rnic_ondemand_read_config(user_buf, count, ppos, cfg_type);
}

/*
 * rnic_idle_time_write - Write function of dial event report proc file.
 * @file: file handle
 * @user_buf: the user space buffer to write to
 * @count: the size of the user space buffer
 * @ppos : the current position in the buffer
 *
 * On success, the number of bytes written is returned, or negative value is
 * returned on error.
 */
STATIC ssize_t rnic_event_report_write(struct file *file, const char __user *user_buf,
				       size_t count, loff_t *ppos)
{
	uint32_t cfg_type = RNIC_CFG_TYPE_EVENT_REPORT;

	return rnic_ondemand_write_config(user_buf, count, ppos, cfg_type);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
STATIC const struct proc_ops rnic_dial_mode_ops = {
	.proc_read	= rnic_dial_mode_read,
	.proc_write	= rnic_dial_mode_write,
};

STATIC const struct proc_ops rnic_idle_time_ops = {
	.proc_read	= rnic_idle_time_read,
	.proc_write	= rnic_idle_time_write,
};

STATIC const struct proc_ops rnic_event_report_ops = {
	.proc_read	= rnic_event_report_read,
	.proc_write	= rnic_event_report_write,
};
#else
STATIC const struct file_operations rnic_dial_mode_ops = {
	.owner	= THIS_MODULE,
	.read	= rnic_dial_mode_read,
	.write	= rnic_dial_mode_write,
};

STATIC const struct file_operations rnic_idle_time_ops = {
	.owner	= THIS_MODULE,
	.read	= rnic_idle_time_read,
	.write	= rnic_idle_time_write,
};

STATIC const struct file_operations rnic_event_report_ops = {
	.owner	= THIS_MODULE,
	.read	= rnic_event_report_read,
	.write	= rnic_event_report_write,
};
#endif

/*
 * rnic_register_ondemand_ops - Register config rx callback function.
 * @ops: operation callback structure
 *
 * Return 0 on success, negative on failure.
 */
int rnic_register_ondemand_ops(struct rnic_ondemand_cb_ops_s *ops)
{
	struct rnic_ondemand_context_s *ondemand_ctx = &rnic_ondemand_ctx;

	if (ops == NULL) {
		RNIC_LOGE("register ops fail.");
		return -EINVAL;
	}

	ondemand_ctx->cfg_func[RNIC_CFG_TYPE_DIAL_MODE] = ops->dial_mode_cfg_func;
	ondemand_ctx->cfg_func[RNIC_CFG_TYPE_IDLE_TIME] = ops->idle_time_cfg_func;
	ondemand_ctx->cfg_func[RNIC_CFG_TYPE_EVENT_REPORT] = ops->event_report_cfg_func;
	RNIC_LOGH("register ops succ");

	return 0;
}

/*
 * rnic_create_ondemand_procfs - Create ondemand proc file.
 *
 * Return 0 on success, negative on failure.
 */
/*lint -save -e801*/
int rnic_create_ondemand_procfs(void)
{
	struct proc_dir_entry *proc_entry = NULL;

	rnic_ondemand_dir = proc_mkdir(RNIC_PROC_FILE_PATH, NULL);
	if (rnic_ondemand_dir == NULL) {
		RNIC_LOGE("create dial dir fail.");
		goto err_file_path;
	}

	proc_entry = proc_create(RNIC_PROC_FILE_ONDEMAND, 0770,
				 rnic_ondemand_dir, &rnic_dial_mode_ops);
	if (proc_entry == NULL) {
		RNIC_LOGE("create ondemand proc file fail.");
		goto err_create_dial_mode;
	}

	proc_entry = proc_create(RNIC_PROC_FILE_IDEL_TIME, 0770,
				 rnic_ondemand_dir, &rnic_idle_time_ops);
	if (proc_entry == NULL) {
		RNIC_LOGE("create idle_timeout proc file fail.");
		goto err_create_idle_time;
	}

	proc_entry = proc_create(RNIC_PROC_FILE_EVENT_REPORT, 0770,
				 rnic_ondemand_dir, &rnic_event_report_ops);
	if (proc_entry == NULL) {
		RNIC_LOGE("create dial_event_report proc file fail.");
		goto err_create_event_report;
	}

	RNIC_LOGH("succ.");

	return 0;

err_create_event_report:
	remove_proc_entry(RNIC_PROC_FILE_IDEL_TIME, rnic_ondemand_dir);
err_create_idle_time:
	remove_proc_entry(RNIC_PROC_FILE_ONDEMAND, rnic_ondemand_dir);
err_create_dial_mode:
	remove_proc_entry(RNIC_PROC_FILE_PATH, NULL);
	rnic_ondemand_dir = NULL;
err_file_path:
	return -ENOMEM;
}
/*lint -restore +e801*/

