# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_PM_DEVFREQ)	+= devfreq.o
obj-$(CONFIG_PM_DEVFREQ_EVENT)	+= devfreq-event.o
obj-$(CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND)	+= governor_simpleondemand.o
obj-$(CONFIG_DEVFREQ_GOV_PERFORMANCE)	+= governor_performance.o
obj-$(CONFIG_DEVFREQ_GOV_POWERSAVE)	+= governor_powersave.o
obj-$(CONFIG_DEVFREQ_GOV_USERSPACE)	+= governor_userspace.o
obj-$(CONFIG_DEVFREQ_GOV_PASSIVE)	+= governor_passive.o

# DEVFREQ Drivers
obj-$(CONFIG_ARM_EXYNOS_BUS_DEVFREQ)	+= exynos-bus.o
obj-$(CONFIG_ARM_IMX_BUS_DEVFREQ)	+= imx-bus.o
obj-$(CONFIG_ARM_IMX8M_DDRC_DEVFREQ)	+= imx8m-ddrc.o
obj-$(CONFIG_ARM_RK3399_DMC_DEVFREQ)	+= rk3399_dmc.o
obj-$(CONFIG_ARM_TEGRA_DEVFREQ)		+= tegra30-devfreq.o
obj-$(CONFIG_ARM_TEGRA20_DEVFREQ)	+= tegra20-devfreq.o

# DEVFREQ Event Drivers
obj-$(CONFIG_PM_DEVFREQ_EVENT)		+= event/

# DEVFREQ Lowpower Drivers
obj-$(CONFIG_GPU_DEVFREQ) += gpu_devfreq/
obj-y += npu_devfreq/
obj-$(CONFIG_L3C_DEVFREQ)      	   += l3c_devfreq/
obj-y += ddr_devfreq/
obj-y += mem_lat/
