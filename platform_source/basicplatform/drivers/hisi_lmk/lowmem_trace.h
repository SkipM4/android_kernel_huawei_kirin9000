/*
 * fs/proc/hisi/lmkd_dbg_trigger.c
 *
 * Copyright(C) 2004-2020 Hisilicon Technologies Co., Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM hisi_lowmem
/*lint -e773*/
#define TRACE_INCLUDE_PATH ../../platform_source/basicplatform/drivers/hisi_lmk
/*lint +e773*/
#define TRACE_INCLUDE_FILE lowmem_trace

#if !defined(_TRACE_HISI_LOWMEM_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HISI_LOWMEM_H

#include <linux/tracepoint.h>

/*lint -e548*/
TRACE_EVENT(lowmem_tune, /* [false alarm] */
	TP_PROTO(int nzones, gfp_t gfp_mask, int other_free, int other_file,
		 int tune_free, int tune_file),

	TP_ARGS(nzones, gfp_mask, other_free, other_file, tune_free,
		tune_file),

	TP_STRUCT__entry(
			__field(int, nzones)
			__field(gfp_t, gfp_mask)
			__field(int, other_free)
			__field(int, other_file)
			__field(int, tune_free)
			__field(int, tune_file)
	),

	TP_fast_assign(
			__entry->nzones = nzones;
			__entry->gfp_mask = gfp_mask;
			__entry->other_free = other_free;
			__entry->other_file = other_file;
			__entry->tune_free = tune_free;
			__entry->tune_file = tune_file;
	),

	TP_printk("%d 0x%x %d %d %d %d",
		__entry->nzones, __entry->gfp_mask, __entry->other_free,
		__entry->other_file, __entry->tune_free, __entry->tune_file)
);
/*lint +e548*/

#endif

/* This part must be outside protection */
#include <trace/define_trace.h>
