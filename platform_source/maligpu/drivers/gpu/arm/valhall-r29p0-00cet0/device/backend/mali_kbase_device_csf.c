// SPDX-License-Identifier: GPL-2.0
/*
 *
 * (C) COPYRIGHT 2019-2020 ARM Limited. All rights reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "../mali_kbase_device_internal.h"
#include "../mali_kbase_device.h"

#include <mali_kbase_hwaccess_backend.h>
#include <mali_kbase_ctx_sched.h>
#include <mali_kbase_reset_gpu.h>
#include <csf/mali_kbase_csf.h>
#include <csf/ipa_control/mali_kbase_csf_ipa_control.h>

#ifdef CONFIG_MALI_NO_MALI
#include <mali_kbase_model_linux.h>
#endif

#include <mali_kbase.h>
#include <backend/gpu/mali_kbase_irq_internal.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <backend/gpu/mali_kbase_js_internal.h>
#include <backend/gpu/mali_kbase_clk_rate_trace_mgr.h>

static void kbase_device_csf_firmware_term(struct kbase_device *kbdev)
{
	kbase_csf_firmware_term(kbdev);
}

static int kbase_device_csf_firmware_init(struct kbase_device *kbdev)
{
	int err = kbase_csf_firmware_init(kbdev);

	if (!err) {
		unsigned long flags;

		spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
		kbdev->pm.backend.mcu_state = KBASE_MCU_ON;
		kbdev->csf.firmware_inited = true;
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	}

	/* Post firmware init, idle condition is restored. Note this is
	 * a deferral action step from the late init stage for CSF.
	 */
	kbase_pm_context_idle(kbdev);

	return err;
}

/**
 * kbase_backend_late_init - Perform any backend-specific initialization.
 * @kbdev:	Device pointer
 *
 * Return: 0 on success, or an error code on failure.
 */
static int kbase_backend_late_init(struct kbase_device *kbdev)
{
	int err;

	err = kbase_hwaccess_pm_init(kbdev);
	if (err)
		return err;

	err = kbase_reset_gpu_init(kbdev);
	if (err)
		goto fail_reset_gpu_init;

	err = kbase_hwaccess_pm_powerup(kbdev, PM_HW_ISSUES_DETECT);
	if (err)
		goto fail_pm_powerup;

	err = kbase_backend_timer_init(kbdev);
	if (err)
		goto fail_timer;

#ifdef CONFIG_MALI_DEBUG
#ifndef CONFIG_MALI_NO_MALI
	if (kbasep_common_test_interrupt_handlers(kbdev) != 0) {
		dev_err(kbdev->dev, "Interrupt assignment check failed.\n");
		err = -EINVAL;
		goto fail_interrupt_test;
	}
#endif /* !CONFIG_MALI_NO_MALI */
#endif /* CONFIG_MALI_DEBUG */

	kbase_ipa_control_init(kbdev);

	/* Initialise the metrics subsystem, it couldn't be initialized earlier
	 * due to dependency on kbase_ipa_control.
	 */
	err = kbasep_pm_metrics_init(kbdev);
	if (err)
		goto fail_pm_metrics_init;

	/* Do the initialisation of devfreq.
	 * Devfreq needs backend_timer_init() for completion of its
	 * initialisation and it also needs to catch the first callback
	 * occurrence of the runtime_suspend event for maintaining state
	 * coherence with the backend power management, hence needs to be
	 * placed before the kbase_pm_context_idle().
	 */
	err = kbase_backend_devfreq_init(kbdev);
	if (err)
		goto fail_devfreq_init;

	/* Update gpuprops with L2_FEATURES if applicable */
	err = kbase_gpuprops_update_l2_features(kbdev);
	if (err)
		goto fail_update_l2_features;

	init_waitqueue_head(&kbdev->hwaccess.backend.reset_wait);

	/* kbase_pm_context_idle is called after the boot of firmware */

	return 0;

fail_update_l2_features:
	kbase_backend_devfreq_term(kbdev);
fail_devfreq_init:
	kbasep_pm_metrics_term(kbdev);
fail_pm_metrics_init:
	kbase_ipa_control_term(kbdev);

#ifdef CONFIG_MALI_DEBUG
#ifndef CONFIG_MALI_NO_MALI
fail_interrupt_test:
#endif /* !CONFIG_MALI_NO_MALI */
#endif /* CONFIG_MALI_DEBUG */

	kbase_backend_timer_term(kbdev);
fail_timer:
	kbase_pm_context_idle(kbdev);
	kbase_hwaccess_pm_halt(kbdev);
fail_pm_powerup:
	kbase_reset_gpu_term(kbdev);
fail_reset_gpu_init:
	kbase_hwaccess_pm_term(kbdev);

	return err;
}

/**
 * kbase_backend_late_term - Perform any backend-specific termination.
 * @kbdev:	Device pointer
 */
static void kbase_backend_late_term(struct kbase_device *kbdev)
{
	kbase_backend_devfreq_term(kbdev);
	kbasep_pm_metrics_term(kbdev);
	kbase_ipa_control_term(kbdev);
	kbase_hwaccess_pm_halt(kbdev);
	kbase_reset_gpu_term(kbdev);
	kbase_hwaccess_pm_term(kbdev);
}

static const struct kbase_device_init dev_init[] = {
#ifdef CONFIG_MALI_NO_MALI
	{kbase_gpu_device_create, kbase_gpu_device_destroy,
			"Dummy model initialization failed"},
#else
	{assign_irqs, NULL,
			"IRQ search failed"},
	{registers_map, registers_unmap,
			"Register map failed"},
#endif
	{power_control_init, power_control_term,
			"Power control initialization failed"},
	{kbase_device_io_history_init, kbase_device_io_history_term,
			"Register access history initialization failed"},
	{kbase_device_early_init, kbase_device_early_term,
			"Early device initialization failed"},
	{kbase_device_populate_max_freq, NULL,
			"Populating max frequency failed"},
	{kbase_device_misc_init, kbase_device_misc_term,
			"Miscellaneous device initialization failed"},
	{kbase_ctx_sched_init, kbase_ctx_sched_term,
			"Context scheduler initialization failed"},
	{kbase_mem_init, kbase_mem_term,
			"Memory subsystem initialization failed"},
	{kbase_csf_protected_memory_init, kbase_csf_protected_memory_term,
			"Protected memory allocator initialization failed"},
	{kbase_device_coherency_init, NULL,
			"Device coherency init failed"},
	{kbase_protected_mode_init, kbase_protected_mode_term,
			"Protected mode subsystem initialization failed"},
	{kbase_device_list_init, kbase_device_list_term,
			"Device list setup failed"},
	{kbase_device_timeline_init, kbase_device_timeline_term,
			"Timeline stream initialization failed"},
	{kbase_clk_rate_trace_manager_init,
			kbase_clk_rate_trace_manager_term,
			"Clock rate trace manager initialization failed"},
	{kbase_device_hwcnt_backend_csf_if_init,
			kbase_device_hwcnt_backend_csf_if_term,
			"GPU hwcnt backend CSF interface creation failed"},
	{kbase_device_hwcnt_backend_csf_init,
			kbase_device_hwcnt_backend_csf_term,
			"GPU hwcnt backend creation failed"},
	{kbase_device_hwcnt_context_init, kbase_device_hwcnt_context_term,
			"GPU hwcnt context initialization failed"},
	{kbase_backend_late_init, kbase_backend_late_term,
			"Late backend initialization failed"},
	{kbase_device_csf_firmware_init, kbase_device_csf_firmware_term,
			"Firmware initialization failed"},
	{kbase_device_hwcnt_backend_csf_metadata_init,
			kbase_device_hwcnt_backend_csf_metadata_term,
			"GPU hwcnt backend metadata creation failed"},
	{kbase_device_hwcnt_virtualizer_init,
			kbase_device_hwcnt_virtualizer_term,
			"GPU hwcnt virtualizer initialization failed"},
	{kbase_device_vinstr_init, kbase_device_vinstr_term,
			"Virtual instrumentation initialization failed"},
#ifdef MALI_KBASE_BUILD
	{kbase_device_debugfs_init, kbase_device_debugfs_term,
			"DebugFS initialization failed"},
	/* Sysfs init needs to happen before registering the device with
	 * misc_register(), otherwise it causes a race condition between
	 * registering the device and a uevent event being generated for
	 * userspace, causing udev rules to run which might expect certain
	 * sysfs attributes present. As a result of the race condition
	 * we avoid, some Mali sysfs entries may have appeared to udev
	 * to not exist.
	 * For more information, see
	 * https://www.kernel.org/doc/Documentation/driver-model/device.txt, the
	 * paragraph that starts with "Word of warning", currently the
	 * second-last paragraph.
	 */
	{kbase_sysfs_init, kbase_sysfs_term, "SysFS group creation failed"},
	{kbase_device_misc_register, kbase_device_misc_deregister,
			"Misc device registration failed"},
#ifdef CONFIG_MALI_BUSLOG
	{buslog_init, buslog_term, "Bus log client registration failed"},
#endif
	{kbase_gpuprops_populate_user_buffer, kbase_gpuprops_free_user_buffer,
			"GPU property population failed"},
#endif
};

static void kbase_device_term_partial(struct kbase_device *kbdev,
		unsigned int i)
{
	while (i-- > 0) {
		if (dev_init[i].term)
			dev_init[i].term(kbdev);
	}
}

void kbase_device_term(struct kbase_device *kbdev)
{
	kbdev->csf.mali_file_inode = NULL;
	kbase_device_term_partial(kbdev, ARRAY_SIZE(dev_init));
	kbase_mem_halt(kbdev);
}

int kbase_device_init(struct kbase_device *kbdev)
{
	int err = 0;
	unsigned int i = 0;

	dev_err(kbdev->dev, "Kernel DDK version %s", MALI_RELEASE_NAME);

	kbase_device_id_init(kbdev);
	kbase_disjoint_init(kbdev);

	for (i = 0; i < ARRAY_SIZE(dev_init); i++) {
		err = dev_init[i].init(kbdev);
		if (err) {
			dev_err(kbdev->dev, "%s error = %d\n",
						dev_init[i].err_mes, err);
			kbase_device_term_partial(kbdev, i);
			break;
		}
	}

	return err;
}
