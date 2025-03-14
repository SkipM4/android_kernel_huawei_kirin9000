/*
 * dsp_utils.c
 *
 * misc utils driver for da combine codecdsp
 *
 * Copyright (c) 2015 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "dsp_utils.h"

#include <linux/kernel.h>
#include <linux/rtc.h>
#include <linux/vmalloc.h>
#include <linux/version.h>

#include <linux/platform_drivers/da_combine/da_combine_dsp_regs.h>
#include <linux/platform_drivers/da_combine/da_combine_utils.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <linux/platform_drivers/da_combine_dsp/da_combine_dsp_misc.h>
#include <da_combine_dsp_interface.h>
#include <rdr_audio_codec.h>
#include <rdr_audio_adapter.h>

#include "asoc_adapter.h"
#include "audio_file.h"
#include "audio_log.h"
#ifdef ENABLE_DA_COMBINE_HIFI_DEBUG
#include "om_debug.h"
#endif

#define LOG_TAG "dsp_utils"

#define WAIT_DAP_WFI_CNT 50
#define MAX_DSP_LEN 0x7d000 /* total ram(ocram,tcm) */
#define INVALID_REG_VALUE 0xFFFFFFFF
#define PARA_4BYTE_ALINED 0x3

struct dsp_utils {
	struct da_combine_dsp_config dsp_config;
	struct snd_soc_component *codec;
	struct da_combine_resmgr *resmgr;
	struct mutex pll_state_mutex;
	unsigned long long pre_hook_time;
	unsigned long long cur_hook_time;
	bool dsp_is_running;
};

static struct dsp_utils *priv_utils;

static const struct parse_log parse_codec_log[] = {
#ifdef CONFIG_HIFI_BB
	{ 0, RDR_CODECDSP_STACK_TO_MEM_SIZE, PARSER_CODEC_TRACE_SIZE,
		parse_dsp_trace },
	{ RDR_CODECDSP_STACK_TO_MEM_SIZE, RDR_CODECDSP_CPUVIEW_TO_MEM_SIZE,
		PARSER_CODEC_CPUVIEW_LOG_SIZE, parse_dsp_cpuview }
#endif
};

static int parse_ocram_log(const char *path, const char *buf)
{
	int ret;
	unsigned int i;
	unsigned int parse_size;
	unsigned int total_log_size = 0;
	char *full_text = NULL;
	char *parse_buff = NULL;

	parse_size = ARRAY_SIZE(parse_codec_log);
	if (parse_size == 0) {
		AUDIO_LOGE("no log to parse");
		return -EINVAL;
	}

	for (i = 0; i < parse_size; i++)
		total_log_size += parse_codec_log[i].parse_log_size;

	full_text = vzalloc(total_log_size);
	if (full_text == NULL) {
		AUDIO_LOGE("alloc buf failed");
		return -ENOMEM;
	}

	for (i = 0; i < parse_size; i++) {
		parse_buff = vzalloc(parse_codec_log[i].parse_log_size);
		if (parse_buff == NULL) {
			AUDIO_LOGE("error: alloc parse buff failed");
			vfree(full_text);
			return -ENOMEM;
		}

		ret = parse_codec_log[i].parse_func(buf +
			parse_codec_log[i].original_offset,
			parse_codec_log[i].original_log_size,
			parse_buff,
			parse_codec_log[i].parse_log_size,
			CODECDSP);
		if (ret == 0)
			snprintf(full_text + strlen(full_text), /* unsafe_function_ignore: snprintf */
				total_log_size - strlen(full_text),
				"\n%s\n", parse_buff);
		else
			AUDIO_LOGE("error: parse failed, i: %d", i);

		vfree(parse_buff);
	}

#ifdef CONFIG_HIFI_BB
	ret = rdr_audio_write_file(path, full_text, strlen(full_text));
	if (ret != 0)
		AUDIO_LOGE("write file fail");
#endif
	vfree(full_text);

	return ret;
}

static void waitfor_dsp_wfi(void)
{
	int wait_cycle = WAIT_DAP_WFI_CNT;
	unsigned int state;
	unsigned int sleep_mode;
	unsigned int state_addr = priv_utils->dsp_config.msg_state_addr;

	IN_FUNCTION;

	while (wait_cycle) {
		sleep_mode = da_combine_dsp_read_reg(DA_COMBINE_DSP_DSP_STATUS0);
		if (((sleep_mode & 0x1) == 1) || (!priv_utils->dsp_is_running))
			break;

		/* wait 100 to 110us every cycle */
		usleep_range(100, 110);
		wait_cycle--;
	}

	if (wait_cycle == 0) {
		AUDIO_LOGE("wait dsp wfi timeout, dsp status0: 0x%x",
			da_combine_dsp_read_reg(DA_COMBINE_DSP_DSP_STATUS0));

		if (state_addr != 0)
			state = da_combine_dsp_read_reg(state_addr);
		else
			state = 0xff;

		AUDIO_LOGE("msg state: 0x%x", state);
	}

	OUT_FUNCTION;
}

int da_combine_save_log_file(const char *dump_ram_path, unsigned int type,
	unsigned int dump_addr, unsigned int dump_size)
{
	char *buf = NULL;
	int ret = 0;

	if (dump_size == 0 || dump_size > MAX_DSP_LEN) {
		AUDIO_LOGW("dump size err, size is %d", dump_size);
		return ret;
	}

	buf = vzalloc(dump_size);
	if (buf == NULL) {
		AUDIO_LOGE("alloc buf failed");
		return -ENOMEM;
	}
	da_combine_misc_dump_bin(dump_addr, buf, dump_size);

#ifdef ENABLE_DA_COMBINE_HIFI_DEBUG
	if (type == DUMP_TYPE_TOTAL_LOG)
		da_combine_adjust_log_sequence(buf, dump_size);
#endif

	if (type == DUMP_TYPE_PANIC_LOG) {
		ret = parse_ocram_log(dump_ram_path, buf);
		if (ret != 0) {
			AUDIO_LOGE("save ocram file fail, ret: %d", ret);
			goto exit;
		}
	} else {
#ifdef CONFIG_HIFI_BB
		ret = rdr_audio_write_file(dump_ram_path, buf, dump_size);
		if (ret)
			AUDIO_LOGE("write file fail");
#endif
	}
exit:
	vfree(buf);
	return ret;
}

void da_combine_dsp_write_reg(unsigned int reg, unsigned int val)
{
	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	(void)snd_soc_component_write_adapter(priv_utils->codec, reg, val);
}

unsigned int da_combine_dsp_read_reg(unsigned int reg)
{
	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return INVALID_REG_VALUE;
	}

	return snd_soc_component_read32(priv_utils->codec, reg);
}

void da_combine_dsp_reg_set_bit(unsigned int reg, unsigned int offset)
{
	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	(void)da_combine_update_bits(priv_utils->codec, reg,
		1 << offset, 1 << offset);
}

void da_combine_dsp_reg_clr_bit(unsigned int reg, unsigned int offset)
{
	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	(void)da_combine_update_bits(priv_utils->codec, reg, 1 << offset, 0);
}

void da_combine_dsp_reg_write_bits(unsigned int reg,
				unsigned int value,
				unsigned int mask)
{
	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	(void)da_combine_update_bits(priv_utils->codec, reg, mask, value);
}

void da_combine_memset(uint32_t dest, size_t n)
{
	size_t i;

	if (dest == 0 || n > MAX_DSP_LEN) {
		AUDIO_LOGE("param err, n is %zu", n);
		return;
	}

	if (n & PARA_4BYTE_ALINED) {
		AUDIO_LOGE("memset size: %zu is not 4 byte aligned", n);
		return;
	}

	for (i = 0; i < n; i = i + 4) {
		da_combine_dsp_write_reg(dest, 0x0);
		dest += 4;
	}
}

void da_combine_memcpy(uint32_t dest, uint32_t *src, size_t n)
{
	size_t i;

	if (src == NULL) {
		AUDIO_LOGE("src is null");
		return;
	}

	if (dest == 0 || n > MAX_DSP_LEN) {
		AUDIO_LOGE("param err, n is %zu", n);
		return;
	}

	if (n & PARA_4BYTE_ALINED) {
		AUDIO_LOGE("memcpy size: %zu is not 4 byte aligned", n);
		return;
	}

	for (i = 0; i < n; i = i + 4) {
		da_combine_dsp_write_reg(dest, *src);
		src++;
		dest += 4;
	}
}

static void da_combine_read_per_4byte(const unsigned int start_addr,
	unsigned int *buf, const unsigned int len)
{
	unsigned int i;

	if (buf == NULL || len == 0) {
		AUDIO_LOGE("input buf or len error, len:%u", len);
		return;
	}

	if (len & PARA_4BYTE_ALINED) {
		AUDIO_LOGE("read size: 0x%x  is not 4 byte aligned", len);
		return;
	}

	for (i = 0; i < len; i += 4)
		*buf++ = da_combine_dsp_read_reg(start_addr + i);
}

static void da_combine_read_per_1byte(const unsigned int start_addr,
	unsigned char *buf, const unsigned int len)
{
	unsigned int i;

	if (buf == NULL || len == 0) {
		AUDIO_LOGE("input buf or len error, len: %u", len);
		return;
	}

	for (i = 0; i < len; i++)
		*buf++ = da_combine_dsp_read_reg(start_addr + i);
}

void da_combine_read(const unsigned int start_addr, unsigned char *arg,
	const unsigned int len)
{
	if (arg == NULL || len == 0) {
		AUDIO_LOGE("input para error, len: %u", len);
		return;
	}
	/* start addr not in 0x20007000~0x20007xxx */
	if (start_addr < DA_COMBINE_1BYTE_SUB_START ||
		start_addr > DA_COMBINE_1BYTE_SUB_END) {
		if ((start_addr < DA_COMBINE_1BYTE_SUB_START) &&
			(start_addr + len >= DA_COMBINE_1BYTE_SUB_START)) {
			AUDIO_LOGE("range error: start: 0x%pK, len:0x%x",
				(void *)(uintptr_t)start_addr, len);
			return;
		}

		if (len & PARA_4BYTE_ALINED) {
			AUDIO_LOGE("input len error, len: %u", len);
			return;
		}

		da_combine_read_per_4byte(start_addr, (unsigned int *)arg, len);
	/* start addr in 0x20007000~0x20007xxx */
	} else {
		if (start_addr + len > DA_COMBINE_1BYTE_SUB_END) {
			AUDIO_LOGE("range error: start: 0x%pK, len: 0x%x",
				(void *)(uintptr_t)start_addr, len);
			return;
		}
		da_combine_read_per_1byte(start_addr, arg, len);
	}
}

void da_combine_write(const unsigned int start_addr, const unsigned int *buf,
	const unsigned int len)
{
	unsigned int i;

	if (buf == NULL || len == 0) {
		AUDIO_LOGE("input buf or len error, len: %u", len);
		return;
	}

	if (len & PARA_4BYTE_ALINED) {
		AUDIO_LOGE("write size:0x%x is not 4 byte aligned", len);
		return;
	}

	for (i = 0; i < len; i += 4)
		da_combine_dsp_write_reg(start_addr + i, *buf++);
}

int da_combine_check_msg_para(const struct da_combine_param_io_buf *param,
	unsigned int in_size)
{
	if (param == NULL) {
		AUDIO_LOGE("input param is null");
		return -EINVAL;
	}

	if (param->buf_in == NULL || param->buf_size_in == 0) {
		AUDIO_LOGE("input buf in or buf in size: %u error",
			param->buf_size_in);
		return -EINVAL;
	}

	if (param->buf_size_in < in_size) {
		AUDIO_LOGE("input size: %u invalid", param->buf_size_in);
		return -EINVAL;
	}

	return 0;
}

void da_combine_misc_dump_bin(const unsigned int addr, char *buf,
	const size_t len)
{
	int ret;

	if (buf == NULL) {
		AUDIO_LOGE("buf is null");
		return;
	}

	if (addr == 0) {
		AUDIO_LOGE("addr is 0");
		return;
	}

	ret = da_combine_request_low_pll_resource(LOW_FREQ_SCENE_DUMP);
	if (ret != 0) {
		AUDIO_LOGE("dump bin request pll failed");
		return;
	}

	da_combine_read(addr, (unsigned char *)buf, len);
	da_combine_release_low_pll_resource(LOW_FREQ_SCENE_DUMP);
}

void da_combine_dsp_set_pll(bool enable)
{
	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	/* set pll */
	if (enable)
		da_combine_resmgr_request_pll(priv_utils->resmgr, PLL_HIGH);
	else
		da_combine_resmgr_release_pll(priv_utils->resmgr, PLL_HIGH);
}

void da_combine_dsp_set_low_pll(bool enable)
{
	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	/* set mad pll */
	if (enable)
		da_combine_resmgr_request_pll(priv_utils->resmgr, PLL_LOW);
	else
		da_combine_resmgr_release_pll(priv_utils->resmgr, PLL_LOW);
}

void da_combine_run_dsp(void)
{
	IN_FUNCTION;

	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	AUDIO_LOGI("state_mutex lock");
	da_combine_dsp_state_lock();

	if (!priv_utils->dsp_is_running) {
		if (priv_utils->dsp_config.dsp_ops.dsp_power_ctrl)
			priv_utils->dsp_config.dsp_ops.dsp_power_ctrl(true);

		if (priv_utils->dsp_config.dsp_ops.ram2axi)
			priv_utils->dsp_config.dsp_ops.ram2axi(true);

		if (priv_utils->dsp_config.dsp_ops.clk_enable)
			priv_utils->dsp_config.dsp_ops.clk_enable(true);

		if (priv_utils->dsp_config.dsp_ops.wtd_enable)
			priv_utils->dsp_config.dsp_ops.wtd_enable(true);

		priv_utils->dsp_is_running = true;
	}

	da_combine_dsp_state_unlock();
	AUDIO_LOGI("state mutex unlock");

	OUT_FUNCTION;
}

void da_combine_reset_dsp(void)
{
	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	/* reset ir path */
	if (priv_utils->dsp_config.dsp_ops.ir_path_clean)
		priv_utils->dsp_config.dsp_ops.ir_path_clean();

	if (priv_utils->dsp_config.dsp_ops.dsp_power_ctrl)
		priv_utils->dsp_config.dsp_ops.dsp_power_ctrl(true);

	if (priv_utils->dsp_config.dsp_ops.runstall)
		priv_utils->dsp_config.dsp_ops.runstall(false);

	if (priv_utils->dsp_config.dsp_ops.deinit)
		priv_utils->dsp_config.dsp_ops.deinit();

	if (priv_utils->dsp_config.dsp_ops.init)
		priv_utils->dsp_config.dsp_ops.init();
}

void da_combine_stop_dsp(void)
{
	IN_FUNCTION;

	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	waitfor_dsp_wfi();

	AUDIO_LOGI("state mutex lock");
	da_combine_dsp_state_lock();

	if (priv_utils->dsp_is_running) {
		if (priv_utils->dsp_config.dsp_ops.wtd_enable)
			priv_utils->dsp_config.dsp_ops.wtd_enable(false);

		if (priv_utils->dsp_config.dsp_ops.clk_enable)
			priv_utils->dsp_config.dsp_ops.clk_enable(false);

		if (priv_utils->dsp_config.dsp_ops.ram2axi)
			priv_utils->dsp_config.dsp_ops.ram2axi(false);
		if (priv_utils->dsp_config.dsp_ops.dsp_power_ctrl)
			priv_utils->dsp_config.dsp_ops.dsp_power_ctrl(false);

		priv_utils->dsp_is_running = false;
	}

	da_combine_dsp_state_unlock();
	AUDIO_LOGI("state mutex unlock");

	OUT_FUNCTION;
}

void da_combine_pause_dsp(void)
{
	IN_FUNCTION;

	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	waitfor_dsp_wfi();

	if (priv_utils->dsp_config.dsp_ops.wtd_enable)
		priv_utils->dsp_config.dsp_ops.wtd_enable(false);

	if (priv_utils->dsp_config.dsp_ops.clk_enable)
		priv_utils->dsp_config.dsp_ops.clk_enable(false);

	OUT_FUNCTION;
}

void da_combine_resume_dsp(enum pll_state state)
{
	IN_FUNCTION;

	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	if (priv_utils->dsp_config.dsp_ops.clk_enable)
		priv_utils->dsp_config.dsp_ops.clk_enable(true);

	if (priv_utils->dsp_config.dsp_ops.wtd_enable)
		priv_utils->dsp_config.dsp_ops.wtd_enable(true);

	OUT_FUNCTION;
}

bool da_combine_get_dsp_state(void)
{
	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return false;
	}

	return priv_utils->dsp_is_running;
}

int da_combine_request_pll_resource(unsigned int scene_id)
{
	IN_FUNCTION;

	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return -EPERM;
	}

	AUDIO_LOGI("sid[0x%x]hifreq_status[0x%x]", scene_id,
		da_combine_get_high_freq_status());

	if (scene_id >= HIGH_FREQ_SCENE_BUTT) {
		AUDIO_LOGE("unknow scene for pll: %u", scene_id);
		return -EPERM;
	}

	mutex_lock(&priv_utils->pll_state_mutex);
	if ((da_combine_get_high_freq_status() & (1 << scene_id)) != 0) {
		AUDIO_LOGW("scene: %u is alread started", scene_id);
		mutex_unlock(&priv_utils->pll_state_mutex);
		return -EAGAIN;
	}

	if (da_combine_get_high_freq_status() == 0) {
		da_combine_dsp_set_pll(true);
		da_combine_run_dsp();

		AUDIO_LOGI("current pll state: %u, new pll state: %u",
			da_combine_dsp_read_reg(priv_utils->dsp_config.cmd1_addr), da_combine_get_pll_state());
		da_combine_dsp_write_reg(priv_utils->dsp_config.cmd1_addr,
			da_combine_get_pll_state());
		da_combine_dsp_write_reg(priv_utils->dsp_config.cmd2_addr,
			(unsigned int)priv_utils->dsp_config.low_pll_state);
	}

	da_combine_set_high_freq_status(scene_id, true);
	mutex_unlock(&priv_utils->pll_state_mutex);

	OUT_FUNCTION;

	return 0;
}

void da_combine_release_pll_resource(unsigned int scene_id)
{
	IN_FUNCTION;

	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	if (scene_id >= HIGH_FREQ_SCENE_BUTT) {
		AUDIO_LOGE("unknow scene for pll: %u", scene_id);
		return;
	}

	mutex_lock(&priv_utils->pll_state_mutex);
	if ((da_combine_get_high_freq_status() & (1 << scene_id)) == 0) {
		AUDIO_LOGW("scene: %u is not started", scene_id);
		mutex_unlock(&priv_utils->pll_state_mutex);
		return;
	}

	da_combine_set_high_freq_status(scene_id, false);

	if (da_combine_get_high_freq_status() == 0) {
		if (da_combine_get_low_freq_status() == 0)
			da_combine_stop_dsp();

		da_combine_dsp_set_pll(false);
	}
	mutex_unlock(&priv_utils->pll_state_mutex);

	OUT_FUNCTION;
}

int da_combine_request_low_pll_resource(unsigned int scene_id)
{
	IN_FUNCTION;

	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return -EINVAL;
	}

	if (scene_id >= LOW_FREQ_SCENE_BUTT) {
		AUDIO_LOGE("unknow scene for mad pll: %u", scene_id);
		return -EINVAL;
	}

	mutex_lock(&priv_utils->pll_state_mutex);
	if ((da_combine_get_low_freq_status() & (1 << scene_id)) != 0) {
		AUDIO_LOGW("scene: %u is alread started", scene_id);
		mutex_unlock(&priv_utils->pll_state_mutex);
		return -EAGAIN;
	}

	if (da_combine_get_low_freq_status() == 0) {
		da_combine_dsp_set_low_pll(true);
		da_combine_run_dsp();

		AUDIO_LOGI("current pll state: %u, new pll state: %u",
			da_combine_dsp_read_reg(priv_utils->dsp_config.cmd1_addr), da_combine_get_pll_state());
		da_combine_dsp_write_reg(priv_utils->dsp_config.cmd1_addr,
			da_combine_get_pll_state());
		da_combine_dsp_write_reg(priv_utils->dsp_config.cmd2_addr,
			(unsigned int)priv_utils->dsp_config.low_pll_state);
	}

	da_combine_set_low_freq_status(scene_id, true);

	AUDIO_LOGI("low scene: 0x%x", da_combine_get_low_freq_status());
	mutex_unlock(&priv_utils->pll_state_mutex);

	OUT_FUNCTION;

	return 0;
}

void da_combine_release_low_pll_resource(unsigned int scene_id)
{
	IN_FUNCTION;

	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	if (scene_id >= LOW_FREQ_SCENE_BUTT) {
		AUDIO_LOGE("unknow scene for low pll: %u", scene_id);
		return;
	}

	mutex_lock(&priv_utils->pll_state_mutex);
	if ((da_combine_get_low_freq_status() & (1 << scene_id)) == 0) {
		AUDIO_LOGW("scene: %u is not started", scene_id);
		mutex_unlock(&priv_utils->pll_state_mutex);
		return;
	}

	da_combine_set_low_freq_status(scene_id, false);

	if (da_combine_get_low_freq_status() == 0) {
		if (da_combine_get_high_freq_status() == 0)
			da_combine_stop_dsp();

		da_combine_dsp_set_low_pll(false);
	}
	mutex_unlock(&priv_utils->pll_state_mutex);

	OUT_FUNCTION;
}

void da_combine_release_requested_pll(void)
{
	int i;

	for (i = 0; i < HIGH_FREQ_SCENE_BUTT; i++)
		da_combine_release_pll_resource(i);

	for (i = 0; i < LOW_FREQ_SCENE_BUTT; i++)
		da_combine_release_low_pll_resource(i);
}

int da_combine_dsp_utils_init(const struct da_combine_dsp_config *config,
	struct snd_soc_component *codec, struct da_combine_resmgr *resmgr)
{
	priv_utils = kzalloc(sizeof(*priv_utils), GFP_KERNEL);
	if (priv_utils == NULL) {
		AUDIO_LOGE("failed to kzalloc priv utils");
		return -ENOMEM;
	}

	memcpy(&priv_utils->dsp_config, config, sizeof(*config));

	priv_utils->codec = codec;
	priv_utils->resmgr = resmgr;

	mutex_init(&priv_utils->pll_state_mutex);

	return 0;
}

void da_combine_dsp_utils_deinit(void)
{
	if (priv_utils == NULL) {
		AUDIO_LOGE("priv utils is null");
		return;
	}

	mutex_destroy(&priv_utils->pll_state_mutex);

	kfree(priv_utils);

	priv_utils = NULL;
}

