/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2021. All rights reserved.
 * Description: This file describe GPU IPA headers
 * Create: 2014-2-24
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#ifndef GPU_IPA_CTX_H
#define GPU_IPA_CTX_H

struct kbase_ipa_context;

/**
 * kbase_dynipa_init - initialize the kbase ipa core
 * @kbdev:      kbase device
 *
 * Return:      pointer to the IPA context or NULL on failure
 */
struct kbase_ipa_context *kbase_dynipa_init(struct kbase_device *kbdev);

/**
 * kbase_dynipa_term - terminate the kbase ipa core
 * @ctx:        pointer to the IPA context
 */
void kbase_dynipa_term(struct kbase_ipa_context *ctx);

/**
 * kbase_ipa_dynamic_core_power - calculate power
 * @ctx:        pointer to the IPA context
 * @err:        0 on success, negative on failure
 *
 * Return:      returns power consumption as mw @ 1GHz @ 1V
 */
u32 kbase_ipa_dynamic_core_power(struct kbase_ipa_context *ctx, int *err);

/**
 * kbase_ipa_dynamic_bound_measure - calculate bound measure
 * @ctx:        pointer to the IPA context
 * @err:        0 on success, negative on failure
 *
 * Return:      returns GPU bound measurement value
 */
u32 kbase_ipa_dynamic_bound_measure(struct kbase_ipa_context *ctx, int *err);

/**
 * kbase_ipa_dynamic_bound_detect - detect GPU bound event
 * @ctx:        pointer to the IPA context
 * @err:        0 on success, negative on failure
 *
 * Return:      returns GPU bound event is true or false
 */
bool kbase_ipa_dynamic_bound_detect(struct kbase_ipa_context *ctx, int *err,
	unsigned long cur_freq, unsigned long load, bool ipa_enable);

/**
 * mali_kbase_devfreq_detect_bound - detect GPU bound main entry
 * @kbdev:      pointer to kbase device
 * @cur_freq:   current frequency
 * @btime:      gpu busy time
 *
 * Return:      void
 */
void mali_kbase_devfreq_detect_bound(struct kbase_device *kbdev,
	unsigned long cur_freq, unsigned long btime);

/**
 * mali_kbase_devfreq_detect_bound_worker - detect GPU bound worker
 * @work:       pointer to work_struct
 *
 * Return:      void
 */
void mali_kbase_devfreq_detect_bound_worker(struct work_struct *work);
#ifdef CONFIG_MALI_LAST_BUFFER
/**
 * mali_kbase_enable_lb - Enable GPU last buffer
 * @kbdev:        pointer to the kbase_device
 * @enable:        1 on enable, 0 on disable
 *
 * Return:      none
 */
void mali_kbase_enable_lb(struct kbase_device *kbdev, unsigned int enable);
#endif
#endif

