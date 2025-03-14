# MD5: b8e47c81308b72eb3433bc2e60077a22
CFG_ENABLE_BUILD_VARS := YES
CFG_FEATURE_UPGRADE_TL := YES
CFG_PRODUCT_NAME                      :="baltimore"
CFG_PRODUCT_CFG_CHIP_SOLUTION_NAME    :="Balong"
CFG_CCPU_OS                           := RTOSCK_SMP
CFG_HCC_VERSION                       := 7.3
CFG_PLATFORM := baltimore
CFG_PLATFORM_HISI_BALONG :=baltimore
CFG_CONFIG_VERSION_STUB             :=NO
CFG_HW_VERSION_STUB                 :=0x34F5FC01
CFG_BSP_CONFIG_EDA := NO
ifeq ($(CFG_EMU_TYPE_ESL),FEATURE_ON)
CFG_CONFIG_UART_ESL := YES
CFG_FEATURE_SVLSOCKET := NO
CFG_BURN_ESL := YES
CFG_BSP_CONFIG_EMU_MCORE_DTB := NO
CFG_CONFIG_USE_TIMER_STAMP:=YES
CFG_CONFIG_NRBBP := YES
CFG_CONFIG_BALONG_L2CACHE := NO
CFG_CONFIG_BALONG_MODEM_RESET_CTRL := NO
CFG_CONFIG_VERSION_ESL_EMU := YES
else
CFG_CONFIG_BALONG_L2CACHE := YES
CFG_EMU_EDMA_LOAD_HAC := NO
CFG_MINI_XLOADER:=NO
CFG_MINI_HIBOOT:=NO
CFG_BSP_CONFIG_EMU_NO_USB := NO
CFG_BSP_CONFIG_EMU_NO_NV := NO
CFG_CONFIG_BBP := YES
CFG_CONFIG_NRBBP := YES
CFG_BSP_CONFIG_EMU_MCORE_DTB := NO
CFG_FEATURE_CPU_PRIVATE_SLICE := YES
CFG_FEATURE_CPU_PRIVATE_SLICE_HRT := YES
endif
CFG_BSP_CONFIG_EMU := NO
CFG_BSP_CONFIG_HI3650 := YES
CFG_BSP_CONFIG_PHONE_TYPE := YES
CFG_BSP_CONFIG_HI3660 := YES
CFG_BSP_CONFIG_BOARD_ASIC := YES
CFG_FULLSTACK_NR_LLRAM_ADDR  := DDR_FULLSTACK_MEM_ADDR
CFG_FULLSTACK_HAC_LLRAM_ADDR := (FULLSTACK_NR_LLRAM_ADDR + NRCCPU_LLRAM_BASE_SIZE)
CFG_FULLSTACK_WFI_ADDR       := (FULLSTACK_HAC_LLRAM_ADDR + HAC_LLRAM_SIZE)
CFG_FULLSTACK_WFI_SIZE       := 0x10000
CFG_BSP_CONFIG_BOARD_SFT := NO
CFG_BSP_HAS_SEC_FEATURE     := NO
CFG_CONFIG_MALLOC_UNIFIED := YES
CFG_CONFIG_SMART_SYSTEM_MODEM := NO
CFG_CONFIG_HIBOOT_DEBUG := NO
CFG_CONFIG_OF := YES
CFG_FEATURE_BALONG_MODEM_ATE := NO
CFG_CONFIG_RINGBUFFER_STUB := NO
CFG_CONFIG_CONSOLE_STUB := NO
CFG_CCPU_CORE_NUM := 2
CFG_CCPU_LLRAM_BASE_ADDR := 0xE0800000
CFG_CCPU_LLRAM_BASE_SIZE := 0x30000
CFG_CCPU_SRAM_SIZE  := 0x3000
CFG_LL2_LLRAM_BASE_ADDR        := ((CCPU_LLRAM_BASE_ADDR) + (CCPU_SRAM_SIZE))
CFG_LL2_LLRAM_SIZE             := 0x28000
CFG_CONFIG_CCPU_HAS_LLRAM := YES
CFG_CCPU_LLRAM_ADDR := (LL2_LLRAM_BASE_ADDR  + LL2_LLRAM_SIZE)
CFG_CCPU_LLRAM_SIZE := (CCPU_LLRAM_BASE_SIZE - CCPU_SRAM_SIZE - LL2_LLRAM_SIZE )
CFG_HI_SRAM_MEM_ADDR            := CCPU_LLRAM_BASE_ADDR
CFG_HI_SRAM_SIZE                := CCPU_SRAM_SIZE
CFG_DRV_SRAM_ADDR               := (HI_SRAM_MEM_ADDR)
CFG_DRV_SRAM_SIZE               := 0x2000
CFG_CPHY_SRAM_ADDR              := ((DRV_SRAM_ADDR) + (DRV_SRAM_SIZE))
CFG_CPHY_SRAM_SIZE              := 0x800
CFG_CPHY_LPC_SRAM_ADDR          := ( CPHY_SRAM_ADDR )
CFG_CPHY_LPC_SRAM_SIZE          := 0x40
CFG_CPHY_1X_DATA_MBX_SRAM_ADDR  := ( (CPHY_LPC_SRAM_ADDR) + (CPHY_LPC_SRAM_SIZE) )
CFG_CPHY_1X_DATA_MBX_SRAM_SIZE  := 0x40
CFG_CPHY_HRPD_DATA_MBX_SRAM_ADDR:= ( (CPHY_1X_DATA_MBX_SRAM_ADDR) + (CPHY_1X_DATA_MBX_SRAM_SIZE) )
CFG_CPHY_HRPD_DATA_MBX_SRAM_SIZE:= 0x40
CFG_GPHY_SRAM_ADDR              := ((CPHY_SRAM_ADDR) + (CPHY_SRAM_SIZE))
CFG_GPHY_SRAM_SIZE              := 0x40
CFG_CONFIG_CCPU_HAS_TCM := YES
CFG_CCPU_ITCM_ADDR := 0x0
CFG_CCPU_ITCM_SIZE := 0x8000
CFG_CCPU_ITCM_SIZE_CFG := (0x6u<<0x2)
CFG_CCPU_DTCM_ADDR := (CCPU_ITCM_ADDR + CCPU_ITCM_SIZE)
CFG_CCPU_DTCM_SIZE := 0x4000
CFG_CCPU_DTCM_SIZE_CFG := (0x5u<<0x2)
CFG_DIGITAL_POWER_MONITOR := YES
CFG_CONFIG_HAS_HAC := YES
CFG_HAC_OS := RTOSCK_SMP
CFG_HAC_COMPILER := HCC
CFG_CONFIG_HAC_ARCH := Cortex-R8
CFG_HAC_CORE_NUM := 2
CFG_HAC_TEXT_START_ADDR := HAC_LLRAM_ADDR
CFG_HAC_LLRAM_ADDR := 0xF4C00000
CFG_HAC_LLRAM_SIZE := 0x200000
CFG_HAC_LLRAM_UNCACHED_SIZE := 0x128000
CFG_HAC_LLRAM_CACHED_SIZE := (HAC_LLRAM_SIZE - HAC_LLRAM_UNCACHED_SIZE)
CFG_CONFIG_HAC_HAS_TCM := YES
CFG_HAC_ITCM_SIZE := 0x8000
CFG_HAC_ITCM_SIZE_CFG := (0x6u<<0x2)
CFG_HAC_DTCM_SIZE := 0x20000
CFG_HAC_DTCM_SIZE_CFG := (0x8u<<0x2)
CFG_HAC_TCM_START_ADDR := 0xF5800000
CFG_HAC_ITCM0_ADDR := HAC_TCM_START_ADDR
CFG_HAC_ITCM1_ADDR := (HAC_ITCM0_ADDR + HAC_ITCM_SIZE)
CFG_HAC_DTCM_ADDR := 0xF5820000
CFG_HAC_DTCM0_ADDR := HAC_DTCM_ADDR
CFG_HAC_DTCM1_ADDR := HAC_DTCM_ADDR
CFG_HAC_THUMB_OPTIMIZE := YES
CFG_NRCCPU_TEMP_CACHE_ADDR := 0x1F000000
CFG_NRCCPU_TEMP_CACHE_SIZE := 0x00200000
CFG_NRCCPU_TEMP_UNCACHE_ADDR := (NRCCPU_TEMP_CACHE_ADDR+NRCCPU_TEMP_CACHE_SIZE)
CFG_NRCCPU_TEMP_UNCACHE_SIZE := 0x00200000
CFG_CONFIG_HAS_NRCCPU := YES
CFG_NRCCPU_OS := RTOSCK_SMP
CFG_NRCCPU_COMPILER := HCC
CFG_CONFIG_NRCCPU_ARCH := Cortex-R8
CFG_NRCCPU_CORE_NUM := 2
CFG_CONFIG_NRCCPU_HAS_LLRAM := YES
CFG_NRCCPU_LLRAM_BASE_ADDR := 0xF5400000
CFG_NRCCPU_LLRAM_BASE_SIZE := 0x00C0000
CFG_NRCCPU_SRAM_SIZE  := 0x3000
CFG_NRCCPU_LLRAM_ADDR := (NRCCPU_LLRAM_BASE_ADDR + NRCCPU_SRAM_SIZE)
CFG_NRCCPU_LLRAM_SIZE := (NRCCPU_LLRAM_BASE_SIZE - NRCCPU_SRAM_SIZE)
CFG_CONFIG_NRCCPU_HAS_TCM := YES
CFG_NRCCPU_ITCM_ADDR := 0x0
CFG_NRCCPU_ITCM_SIZE := 0x4000
CFG_NRCCPU_ITCM_SIZE_CFG := (0x5u<<0x2)
CFG_NRCCPU_DTCM_ADDR := 0x10000
CFG_NRCCPU_DTCM_SIZE := 0x10000
CFG_NRCCPU_DTCM_SIZE_CFG := (0x7u<<0x2)
CFG_MPU_ASLR_RESERVE            :=  0x300000
CFG_MPU_ALGN_RESERVE            :=  0x100000
CFG_DDR_MCORE_RELOC_SIZE := 0x10000
CFG_CONFIG_NVIM := YES
CFG_CONFIG_NR_NVIM:=YES
CFG_FEATURE_NV_FLASH_ON := NO
CFG_FEATURE_NV_EMMC_ON := YES
CFG_FEATURE_NV_LFILE_ON := NO
CFG_FEATURE_NV_RFILE_ON := NO
CFG_FEATURE_NV_NO_MODEMNVM_SYS := YES
CFG_FEATURE_NV_SEC_ON := YES
CFG_FEATURE_DIS_HMAC_CHECK := NO
CFG_EFUSE_HUK_BIT_START := 1995
CFG_FEATURE_TLPHY_MAILBOX :=YES
CFG_CONFIG_MAILBOX_TYPE := YES
CFG_CONFIG_HIFI_MAILBOX:=NO
CFG_FEATURE_CPHY_MAILBOX     := NO
CFG_ENABLE_BUILD_OM := YES
CFG_CONFIG_DUMP_LOG_ESCAPE_FIQ := YES
CFG_FEATURE_OM_PHONE := NO
CFG_ENABLE_BUILD_SYSVIEW := NO
CFG_ENABLE_BUILD_CPUVIEW := NO
CFG_ENABLE_BUILD_MEMVIEW := NO
CFG_ENABLE_AMON_SOC := NO
CFG_ENABLE_AMON_MDM := YES
CFG_ENABLE_BUILD_UTRACE := NO
CFG_ENABLE_BUILD_HAC_DUMP := YES
CFG_ENABLE_BUILD_NRRDR := YES
CFG_ENABLE_BUILD_DUMP_MDM_LPM3 := YES
CFG_ENABLE_BUILD_SOCP := YES
CFG_CONFIG_DIAG_SYSTEM := YES
CFG_CONFIG_DIAG_NETLINK := YES
CFG_ENABLE_BUILD_PRINT := YES
CFG_CONFIG_DIAG_NRM := YES
CFG_CONFIG_FILEBROSWER := NO
CFG_CONFIG_HIMS_NV_PROC := YES
CFG_DIAG_SYSTEM_5G := YES
CFG_SOCP_V300 := YES
CFG_CONFIG_DEFLATE := NO
CFG_CONFIG_APPLOG := NO
CFG_CONFIG_DIAG_FRAME := YES
CFG_FEATURE_SOCP_MEM_RESERVED			:= FEATURE_ON
CFG_FEATURE_HDS_PRINTLOG := FEATURE_ON
CFG_FEATURE_HDS_TRANSLOG := FEATURE_ON
CFG_FEATURE_SRE_PRINT_SLICE := FEATURE_ON
CFG_FEATURE_SRE_PRINT_RTC := FEATURE_OFF
CFG_FEATURE_NOC_PARSE_ES := NO
CFG_CONFIG_MID:=YES
CFG_CONFIG_NOC := YES
CFG_CONFIG_NOC_AP := NO
CFG_CONFIG_MODEM_FULL_DUMP := YES
CFG_CONFIG_MODEM_MINI_DUMP := YES
CFG_CONFIG_MODEM_DUMP_BLACKLIST := NO
CFG_CONFIG_MODEM_SOCP_3_0 := YES
CFG_DIAG_VERSION_ENG := YES
CFG_CONFIG_BUS_ERR_NR:=YES
CFG_CONFIG_BUS_ERR_LR:=YES
CFG_CONFIG_BUS_ERR_AP:=NO
CFG_CONFIG_PDLOCK:=NO
CFG_CONFIG_PDLOCK_RENEW:=YES
CFG_CONFIG_PDLOCK_AP:=NO
CFG_CONFIG_M3_APB_SPI := NO
CFG_FEATURE_SAMPLE_LTE_CHAN 			:= FEATURE_OFF
CFG_FEATURE_SCI_PROTOL_T1               := FEATURE_OFF
CFG_FEATURE_SCI_ESIM                    := FEATURE_OFF
CFG_CONFIG_SCI_DECP                     := YES
CFG_CONFIG_SC := NO
CFG_CONFIG_DSP := NO
CFG_CONFIG_NRDSP := YES
CFG_CONFIG_NRDSP_FULL_DUMP := YES
CFG_CONFIG_NRDSP_MINI_DUMP := NO
CFG_CONFIG_CBBE   := YES
CFG_MODEM_MEM_REPAIR := YES
CFG_CONFIG_WATCHPOINT := YES
CFG_ENABLE_BUILD_PRINT := YES
CFG_CONFIG_DSPDVS := YES
CFG_FEATURE_DSP_DFS := FEATURE_ON
CFG_CONFIG_DSP_STRESS   := NO
CFG_FEATURE_GUBBP_HANDSHAKE                     := FEATURE_ON
CFG_FEATURE_GUDRX_NEWVERSION                    := FEATURE_ON
CFG_FEATURE_BOSTON_ONLY_FEATURE                 := FEATURE_OFF
CFG_FEATURE_DSP_PM_SEPARATE_MODE     := FEATURE_ON
CFG_FEATURE_CSDR_COMBINE     := FEATURE_ON
CFG_FEATURE_GUC_TRANSPLANT     := FEATURE_OFF
CFG_CONFIG_DFS_DDR := NO
CFG_CONFIG_DFS_NOC := NO
CFG_SUPPORT_PARA_CFG := NO
CFG_FEATURE_TCM_RETENTION     := FEATURE_ON
CFG_FEATURE_TCM_PART_RETENTION    := FEATURE_OFF
CFG_CONFIG_NRCPM := YES
CFG_CONFIG_NEWVER_RFIC := YES
CFG_CONFIG_NR_NEWMAILBOX := YES
CFG_CONFIG_BBIC_LOWPOWER := YES
CFG_CONFIG_NR_LTE_INTEROPER := YES
CFG_CONFIG_LIGHTSLEEP_POWERDOWN := YES
CFG_MEMREPAIR_ACTRL_CONFIG_BASE_ADDR := 0xFA894000
CFG_FEATURE_MULTI_CHANNEL			:= FEATURE_OFF
CFG_FEATURE_CHR_OM  := FEATURE_ON
CFG_FEATURE_CHR_NR := YES
CFG_CONFIG_GPIO_PL061 := YES
CFG_CONFIG_TRNG_SEED := YES
CFG_FEATURE_ANTEN_DETECT := YES
CFG_CONFIG_ANTEN := YES
CFG_CONFIG_ONOFF := YES
CFG_CONFIG_MLOADER_COLD_PATCH := YES
CFG_CONFIG_LOAD_SEC_IMAGE := YES
CFG_CONFIG_COMPRESS_CCORE_IMAGE := YES
CFG_CONFIG_COMPRESS_DTB_IMAGE := NO
CFG_CONFIG_MODEM_DTB_LOAD_IN_KERNEL := YES
CFG_CONFIG_ENABLE_DTO := YES
CFG_CONFIG_IS_DTB_VERIFY := YES
CFG_CONFIG_MODEM_BALONG_ASLR:=NO
CFG_CONFIG_MODEM_BALONG_ASLR_CACHE_ON:=NO
CFG_CONFIG_MODEM_ASLR_DEBUG:=NO
CFG_CONFIG_MLOADER := YES
CFG_FEATURE_DELAY_MODEM_INIT                    := FEATURE_ON
CFG_CONFIG_MODEM_PINTRL := YES
CFG_CONFIG_MODEM_PINTRL_KERNEL := YES
CFG_CONFIG_ADC := YES
CFG_CONFIG_EFUSE := YES
CFG_CONFIG_EFUSE_NR := YES
CFG_CONFIG_RTC_ON_SOC := YES
CFG_CONFIG_TEE_DECOUPLE := YES
ifeq ($(CFG_MODEM_SANITIZER),FEATURE_ON)
CFG_STACK_CANARY_COMPILE := NO
else
CFG_STACK_CANARY_COMPILE := YES
endif
CFG_CONFIG_RFILE_ON := YES
CFG_CONFIG_RFILE_USER := YES
CFG_FEATURE_THERMAL := YES
CFG_MODEM_FW_PARTITION_SIZE := 138936320
CFG_CONFIG_CCPU_FIQ_SMP := YES
CFG_OS_K3V3_USE_LPM3_API           :=NO
CFG_BSP_ICC_MCHANNEL_USE_LPM3TCM := YES
CFG_CONFIG_HIBOOT_UART_NUM := 0
CFG_MCORE_MPU_ENABLE := YES
CFG_CONFIG_MODULE_VIC := NO
CFG_CONFIG_AT_UART := NO
CFG_CONFIG_MEM_DEBUG := YES
CFG_CONFIG_BALONG_MODEM_RESET := YES
dts:=true
CFG_DTS_STATIC_MEM_SIZE := 0X5000
CFG_CONFIG_PLL_UNLOCK := YES
CFG_CONFIG_AVS := YES
CFG_CONFIG_AVS_TEST := YES
CFG_CONFIG_CPUFREQ := YES
CFG_CPUFREQ_DEBUG_INTERFACE:=YES
CFG_CONFIG_BUSFREQ := YES
CFG_CONFIG_IDLEFREQ := YES
CFG_CONFIG_CCORE_REGULATOR := YES
CFG_CONFIG_CCORE_WDT := YES
CFG_CONFIG_ACORE_WDT := NO
CFG_CONFIG_WDT_HAC_BUILD := YES
CFG_CONFIG_WDT_NR_BUILD := YES
CFG_CONFIG_PMIC_FPGA := NO
CFG_CONFIG_NRCCPU_PMU := YES
CFG_CONFIG_PMIC_SPMI := YES
CFG_CONFIG_PMU_NEW := YES
CFG_CONFIG_PMU_VERION := 6563
CFG_CONFIG_PMIC_SPMI_PROTOCOL := YES
CFG_CONFIG_NR_DCXO_NV := YES
ifeq ($(CFG_EMU_TYPE_ESL),FEATURE_ON)
CFG_CONFIG_PMU := NO
else
CFG_CONFIG_PMU := YES
endif
CFG_CONFIG_PMCTRL_MODEM := YES
CFG_CONFIG_NRCCPU_PM := YES
CFG_CONFIG_BALONG_CCLK := YES
CFG_CONFIG_BALONG_CCLK_DEBUG := YES
CFG_CONFIG_BALONG_CCLK_ATUOGATE := YES
CFG_CONFIG_PM_SCTRL := YES
CFG_CONFIG_CCORE_CPU_IDLE := YES
CFG_CONFIG_HAC_CPU_IDLE := YES
CFG_CONFIG_CCORE_PM := YES
CFG_CONFIG_CCORE_NOTIFIER := YES
CFG_CONFIG_MODULE_IPC := YES
CFG_CONFIG_IPCM_USE_FPGA_VIC := NO
CFG_CONFIG_MODULE_TIMER := YES
CFG_CONFIG_MODULE_HAC_SLICE := YES
CFG_CONFIG_CCORE_CPU := YES
CFG_CONFIG_HAS_CCORE_WAKELOCK := YES
CFG_CONFIG_HAS_HAC_WAKELOCK := YES
CFG_CONFIG_CCORE_BALONG_PM := YES
CFG_CONFIG_MODULE_HAC_PM := YES
CFG_CONFIG_LRCCPU_PM_DEBUG := YES
CFG_CONFIG_NRCCPU_PM_DEBUG := YES
CFG_CONFIG_BALONG_EDMA := YES
CFG_CONFIG_EDMA_DEBUG := YES
CFG_CONFIG_PWC_MNTN_CCORE := NO
CFG_CONFIG_HWADP := YES
CFG_CONFIG_SYSCTRL := YES
CFG_CONFIG_SYSBUS := NO
CFG_CONFIG_SYSBUS_NRCCPU := NO
CFG_CONFIG_SYSBUS_HAC := NO
CFG_CONFIG_BALONG_HPM_TEMP := NO
CFG_CONFIG_MEM := YES
CFG_CONFIG_TCXO_BALONG := NO
CFG_CONFIG_BALONG_DPM := NO
CFG_CONFIG_USE_TIMER_STAMP:=NO
CFG_CONFIG_MODULE_BUSSTRESS := NO
CFG_CONFIG_RSR_ACC := YES
CFG_CONFIG_RSRACC_DEBUG := YES
CFG_CONFIG_CCPU_HOTPLUG := NO
CFG_CONFIG_MPERF  := NO
CFG_CONFIG_PERF_STAT_1 := NO
CFG_CONFIG_AXIMEM_BALONG := NO
CFG_CONFIG_AXIMEM_CLK_RST := YES
CFG_CONFIG_NXDEP_MPU:=NO
CFG_CONFIG_NXDEP_MPU_NEW := YES
CFG_CONFIG_PM_OM := YES
CFG_CONFIG_PM_OM_NR := YES
CFG_CONFIG_PM_OM_DEBUG := YES
CFG_CONFIG_IPF    := NO
CFG_CONFIG_PSAM   := NO
CFG_CONFIG_CIPHER := NO
CFG_CONFIG_EIPF   := YES
CFG_CONFIG_WAN	  := YES
CFG_CONFIG_NEW_PLATFORM := YES
CFG_CONFIG_CIPHER_NEW := YES
CFG_CONFIG_CIPHER_EN_DC := YES
CFG_CONFIG_ECIPHER := YES
CFG_CONFIG_MAA_BALONG := YES
CFG_CONFIG_MAA_LR := YES
CFG_CONFIG_MAA_NR := YES
CFG_CONFIG_ESPE   := YES
CFG_CONFIG_BALONG_ESPE := YES
CFG_CONFIG_ESPE_PHONE_SOC := YES
CFG_CONFIG_MAA_LL1C := YES
CFG_CONFIG_MAA_V2 := YES
CFG_MAA_ASIC := YES
CFG_CONFIG_IPF_VESION  := 2
CFG_CONFIG_IPF_ADQ_LEN := 5
CFG_CONFIG_IPF_PROPERTY_MBB := NO
CFG_CONFIG_USB_DWC3_VBUS_DISCONNECT:=NO
CFG_USB3_SYNOPSYS_PHY:=NO
CFG_CONFIG_USB_FORCE_HIGHSPEED:=NO
CFG_FEATURE_HISOCKET := FEATURE_OFF
CFG_CONFIG_BALONG_TRANS_REPORT := YES
CFG_CONFIG_HMI := YES
CFG_CONFIG_HMI_ICC := YES
CFG_CONFIG_HMI_DEBUG := YES
CFG_ENABLE_TEST_CODE := NO
CFG_CONFIG_LLT_MDRV := NO
CFG_CONFIG_ECDC := NO
CFG_CONFIG_DYNMEM_REPORT := YES
CFG_CONFIG_ICC := YES
CFG_CONFIG_NRICC := YES
CFG_CONFIG_ICC_DEBUG := YES
CFG_CONFIG_SEC_ICC := YES
CFG_CONFIG_EICC_V210 := YES
ifeq ($(CFG_EMU_TYPE_EMU),FEATURE_ON)
CFG_CONFIG_EICC_PHY_EMU := YES
endif
CFG_NPHY_EICC_ENABLE       := YES
CFG_CONFIG_CSHELL := YES
CFG_CONFIG_NR_CSHELL := YES
CFG_CONFIG_UART_SHELL := YES
CFG_CONFIG_OS_INCLUDE_SHELL := YES
CFG_CONFIG_SHELL_SYMBOL_REG := NO
CFG_CONFIG_IPC_MSG := YES
CFG_CONFIG_IPC_MSG_AO_DISC := YES
CFG_CONFIG_IPC_MSG_V230 := NO
CFG_CONFIG_BALONG_IPC_MSG_V230 := NO
CFG_CONFIG_HW_SPINLOCK := YES
CFG_HI_NRSRAM_MEM_ADDR                  := NRCCPU_LLRAM_BASE_ADDR
CFG_HI_NRSRAM_SIZE                		:= NRCCPU_SRAM_SIZE
CFG_DRV_NRSRAM_ADDR               		:= (HI_NRSRAM_MEM_ADDR)
CFG_DRV_NRSRAM_SIZE						:= 0x2000
CFG_LPMCU_DRAM_WINDOW := 0x20000000
CFG_DDR_MEM_ADDR       		:= 0xA0000000
CFG_DDR_MEM_SIZE       		:= 0x15700000
CFG_DDR_APP_ACP_ADDR := 0
CFG_DDR_APP_ACP_SIZE := 0
CFG_DDR_MDM_ACP_ADDR := 0
CFG_DDR_MDM_ACP_SIZE := 0
CFG_DDR_MCORE_SIZE        := 0x8900000
CFG_DDR_MCORE_NR_SIZE     := 0x3100000
CFG_DDR_MCORE_DTS_SIZE    := 0x180000
CFG_DDR_GU_SIZE           := 0x40000
CFG_DDR_TLPHY_IMAGE_SIZE  := 0x800000
CFG_DDR_NRPHY_IMAGE_SIZE  := 0x900000
CFG_DDR_LPHY_SDR_SIZE     := 0xC80000
CFG_DDR_LCS_SIZE          := 0x280000
CFG_DDR_SDR_NR_SIZE       := 0x1600000
CFG_DDR_CBBE_IMAGE_SIZE   := 0x0
CFG_DDR_HARQ_UP_SIZE      := 0xC0000
CFG_DDR_HARQ_NRUL_SIZE    := 0x2C0000
CFG_DDR_RFIC_SUB6G_IMAGE_SIZE   := 0x138000
CFG_DDR_RFIC_HF_IMAGE_SIZE      := 0x0c8000
CFG_DDR_RFIC_DUMP_SIZE    := 0x100000
CFG_DDR_SEC_SHARED_SIZE   := 0x100000
CFG_DDR_PDE_IMAGE_SIZE    := 0x300000
CFG_DDR_MAA_MDM_SIZE      := 0xD00000
CFG_DDR_FULLSTACK_MEM_SIZE := 0x500000
CFG_DDR_LPMCU_IMAGE_SIZE  := 0x40000
CFG_DDR_EMU_HAC_LOAD_SIZE         := 0x200000
CFG_DDR_MTD_MEM_SIZE      := 0x0
CFG_DDR_MCORE_UNCACHE_SIZE:=0x03000000
CFG_DDR_MCORE_NR_UNCACHE_SIZE:=0x01000000
CFG_DDR_MCORE_ADDR        := $(CFG_DDR_MEM_ADDR)
CFG_DDR_MCORE_NR_ADDR     := $(CFG_DDR_MCORE_ADDR)+$(CFG_DDR_MCORE_SIZE)
CFG_DDR_MCORE_DTS_ADDR    := $(CFG_DDR_MCORE_NR_ADDR)+$(CFG_DDR_MCORE_NR_SIZE)
CFG_DDR_GU_ADDR           := $(CFG_DDR_MCORE_DTS_ADDR)+$(CFG_DDR_MCORE_DTS_SIZE)
CFG_DDR_TLPHY_IMAGE_ADDR  := $(CFG_DDR_GU_ADDR)+$(CFG_DDR_GU_SIZE)
CFG_DDR_NRPHY_IMAGE_ADDR  := $(CFG_DDR_TLPHY_IMAGE_ADDR)+$(CFG_DDR_TLPHY_IMAGE_SIZE)
CFG_DDR_LPHY_SDR_ADDR     := $(CFG_DDR_NRPHY_IMAGE_ADDR)+$(CFG_DDR_NRPHY_IMAGE_SIZE)
CFG_DDR_LCS_ADDR          := $(CFG_DDR_LPHY_SDR_ADDR)+$(CFG_DDR_LPHY_SDR_SIZE)
CFG_DDR_SDR_NR_ADDR       := $(CFG_DDR_LCS_ADDR)+$(CFG_DDR_LCS_SIZE)
CFG_DDR_CBBE_IMAGE_ADDR   := $(CFG_DDR_SDR_NR_ADDR)+$(CFG_DDR_SDR_NR_SIZE)
CFG_DDR_HARQ_UP_ADDR      := $(CFG_DDR_CBBE_IMAGE_ADDR)+$(CFG_DDR_CBBE_IMAGE_SIZE)
CFG_DDR_HARQ_NRUL_ADDR    := $(CFG_DDR_HARQ_UP_ADDR)+$(CFG_DDR_HARQ_UP_SIZE)
CFG_DDR_RFIC_SUB6G_IMAGE_ADDR := $(CFG_DDR_HARQ_NRUL_ADDR)+$(CFG_DDR_HARQ_NRUL_SIZE)
CFG_DDR_RFIC_HF_IMAGE_ADDR    := $(CFG_DDR_RFIC_SUB6G_IMAGE_ADDR)+$(CFG_DDR_RFIC_SUB6G_IMAGE_SIZE)
CFG_DDR_RFIC_DUMP_ADDR    := $(CFG_DDR_RFIC_HF_IMAGE_ADDR)+$(CFG_DDR_RFIC_HF_IMAGE_SIZE)
CFG_DDR_SEC_SHARED_ADDR   := $(CFG_DDR_RFIC_DUMP_ADDR)+$(CFG_DDR_RFIC_DUMP_SIZE)
CFG_DDR_PDE_IMAGE_ADDR    := $(CFG_DDR_SEC_SHARED_ADDR)+$(CFG_DDR_SEC_SHARED_SIZE)
CFG_DDR_MAA_MDM_ADDR      := $(CFG_DDR_PDE_IMAGE_ADDR)+$(CFG_DDR_PDE_IMAGE_SIZE)
CFG_DDR_EMU_HAC_LOAD_ADDR := $(CFG_DDR_MAA_MDM_ADDR)+$(CFG_DDR_MAA_MDM_SIZE)
CFG_DDR_MTD_MEM_ADDR      := $(CFG_DDR_EMU_HAC_LOAD_ADDR)+$(CFG_DDR_EMU_HAC_LOAD_SIZE)
CFG_DDR_FULLSTACK_MEM_ADDR := $(CFG_DDR_MTD_MEM_ADDR)+$(CFG_DDR_MTD_MEM_SIZE)
CFG_DDR_LPMCU_IMAGE_ADDR  := $(CFG_DDR_FULLSTACK_MEM_ADDR)+$(CFG_DDR_FULLSTACK_MEM_SIZE)
CFG_DDR_LRCCPU_DTS_SIZE := 0x40000
CFG_DDR_LPMCU_DTS_SIZE := 0x18000
CFG_DDR_NRCCPU_DTS_SIZE := 0x40000
CFG_DDR_L2CPU_DTS_SIZE := 0x18000
CFG_DDR_MDTS_TOTAL_SIZE := $(CFG_DDR_MCORE_DTS_SIZE)
CFG_DDR_LRCCPU_DTS_ADDR := $(CFG_DDR_MCORE_DTS_ADDR)
CFG_DDR_LPMCU_DTS_ADDR := $(CFG_DDR_LRCCPU_DTS_ADDR) + $(CFG_DDR_LRCCPU_DTS_SIZE)
CFG_DDR_NRCCPU_DTS_ADDR := $(CFG_DDR_LPMCU_DTS_ADDR) + $(CFG_DDR_LPMCU_DTS_SIZE)
CFG_DDR_L2CPU_DTS_ADDR := $(CFG_DDR_NRCCPU_DTS_ADDR) + $(CFG_DDR_NRCCPU_DTS_SIZE)
ifeq ($(CFG_CONFIG_ENABLE_DTO),YES)
CFG_DDR_DTB_OVL_SIZE := 0x2000
CFG_DDR_LRCCPU_DTBO_SIZE := $(CFG_DDR_DTB_OVL_SIZE)
CFG_DDR_LPMCU_DTBO_SIZE := $(CFG_DDR_DTB_OVL_SIZE)
CFG_DDR_NRCCPU_DTBO_SIZE := $(CFG_DDR_DTB_OVL_SIZE)
CFG_DDR_L2CPU_DTBO_SIZE := $(CFG_DDR_DTB_OVL_SIZE)
CFG_DDR_IMAGE_DTBO_SIZE := 0xC8000
CFG_DDR_MDTS_COMMON_SIZE := $(CFG_DDR_LRCCPU_DTS_SIZE) + $(CFG_DDR_LPMCU_DTS_SIZE) + $(CFG_DDR_NRCCPU_DTS_SIZE) + $(CFG_DDR_L2CPU_DTS_SIZE) + $(CFG_DDR_LRCCPU_DTBO_SIZE) + $(CFG_DDR_LPMCU_DTBO_SIZE) + $(CFG_DDR_NRCCPU_DTBO_SIZE) +$(CFG_DDR_L2CPU_DTBO_SIZE)
CFG_DDR_MDTS_OVERLAY_SIZE := $(CFG_DDR_IMAGE_DTBO_SIZE)
CFG_DDR_LRCCPU_DTBO_ADDR := $(CFG_DDR_L2CPU_DTS_ADDR) + $(CFG_DDR_L2CPU_DTS_SIZE)
CFG_DDR_LPMCU_DTBO_ADDR := $(CFG_DDR_LRCCPU_DTBO_ADDR) + $(CFG_DDR_LRCCPU_DTBO_SIZE)
CFG_DDR_NRCCPU_DTBO_ADDR := $(CFG_DDR_LPMCU_DTBO_ADDR) + $(CFG_DDR_LPMCU_DTBO_SIZE)
CFG_DDR_L2CPU_DTBO_ADDR := $(CFG_DDR_NRCCPU_DTBO_ADDR) + $(CFG_DDR_NRCCPU_DTBO_SIZE)
CFG_DDR_IMAGE_DTBO_ADDR := $(CFG_DDR_L2CPU_DTBO_ADDR) + $(CFG_DDR_L2CPU_DTBO_SIZE)
endif
CFG_MEM_ADJUST_INTERCEPT := YES
CFG_MCORE_TEXT_START_ADDR := $(CFG_DDR_MCORE_ADDR)
CFG_HIBOOT_DDR_ENTRY := $(CFG_DDR_HIBOOT_ADDR)
CFG_PRODUCT_CFG_KERNEL_ENTRY := $(CFG_DDR_ACORE_ADDR)+0x80000-0x8000
CFG_PRODUCT_KERNEL_PARAMS_PHYS := $(CFG_DDR_ACORE_ADDR)+0x100
CFG_ONCHIP_HIBOOT_ADDR                := $(CFG_MCORE_TEXT_START_ADDR)+0x100000-0x1000
CFG_DDR_MCORE_UNCACHE_ADDR:=0x24800000
CFG_DDR_MCORE_NR_UNCACHE_ADDR:=((DDR_MCORE_UNCACHE_ADDR) + (DDR_MCORE_UNCACHE_SIZE))
CFG_MODEM_SANITIZER_ADDR_OFFSET:=0xBDE0000
CFG_MODEM_SANITIZER_NR_ADDR_OFFSET:=0xBE00000
CFG_MDM_SANTIZIER_MEM_SIZE:=0x4000000
CFG_MDM_SANTIZIER_NR_MEM_SIZE:=0x800000
CFG_MDM_SANTIZIER_MEM_ADDR:=0x20000000
CFG_MDM_SANTIZIER_NR_MEM_ADDR:=( (MDM_SANTIZIER_MEM_ADDR) + (MDM_SANTIZIER_MEM_SIZE ))
CFG_DDR_UPA_ADDR                := DDR_GU_ADDR
CFG_DDR_UPA_SIZE                := 0x00024000
CFG_DDR_CQI_ADDR                := ((DDR_UPA_ADDR) + (DDR_UPA_SIZE))
CFG_DDR_CQI_SIZE                := 0x00003400
CFG_DDR_ZSP_UP_ADDR             := ((DDR_CQI_ADDR) + (DDR_CQI_SIZE))
CFG_DDR_ZSP_UP_SIZE             := 0x00008000
CFG_DDR_ZSP_UP_1_ADDR           := ((DDR_ZSP_UP_ADDR) + (DDR_ZSP_UP_SIZE))
CFG_DDR_ZSP_UP_1_SIZE           := 0x00008000
CFG_SHM_SEC_SIZE          		:=(DDR_SEC_SHARED_SIZE)
CFG_SHM_SIZE_PROTECT_BARRIER    :=(0x1000)
CFG_SHM_SIZE_PARAM_CFG          :=(0x4000)
CFG_SHM_SIZE_SEC_ICC            :=(0x20000)
CFG_SHM_SIZE_SEC_RESERVED       :=(0xb000)
CFG_SHM_SIZE_SEC_MDMA9_PM_BOOT  :=(0x2000)
CFG_SHM_SIZE_SEC_CERT			:=(0)
CFG_SHM_SIZE_SEC_DUMP			:=(0x24000)
CFG_SHM_SIZE_SEC_MALLOC         :=(0x80000)
CFG_MODEM_SHARED_DDR_BASE := (0x1E000000)
CFG_MODEM_NSRO_SHARED_MEM_BASE := (MODEM_SHARED_DDR_BASE)
CFG_DDR_NSRO_SHARED_MEM_SIZE := 0xC00000
CFG_DDR_SHA_NV_SIZE       := 0xA00000
CFG_DDR_SHA_SEC_MEM_SIZE  := ((DDR_NSRO_SHARED_MEM_SIZE) - (DDR_SHA_NV_SIZE))
CFG_DDR_SHA_MEM_SIZE      := 0x100000
CFG_DDR_MCORE_NR_SHARED_MEM_SIZE     := 0x100000
CFG_DDR_SHARED_MEM_SIZE   := $(CFG_DDR_SHA_MEM_SIZE)+$(CFG_DDR_NSRO_SHARED_MEM_SIZE)
CFG_DDR_MNTN_SIZE         := 0x300000
CFG_DDR_SHARED_MEM_ADDR   :=  ( MODEM_NSRO_SHARED_MEM_BASE )
CFG_DDR_SHAREDNR_MEM_ADDR :=  ((DDR_SHARED_MEM_ADDR      ) + (DDR_SHARED_MEM_SIZE      ))
CFG_DDR_MNTN_ADDR         :=  ( (DDR_SHAREDNR_MEM_ADDR      ) + (DDR_MCORE_NR_SHARED_MEM_SIZE  ) )
CFG_MODEM_SOCP_DDR_BASE  := (0xB1280000)
CFG_DDR_SOCP_SIZE         := 0x8000000
CFG_DDR_HIFI_SIZE         := 0x0
CFG_DDR_SOCP_ADDR         :=  ( MODEM_SOCP_DDR_BASE)
CFG_DDR_HIFI_ADDR         :=  ( (DDR_SOCP_ADDR            ) + (DDR_SOCP_SIZE            ) )
CFG_NV_MBN_MAX_SIZE                 := 0x20000
CFG_NV_DDR_SIZE                     := (DDR_SHA_NV_SIZE)
CFG_NV_COMM_BIN_FILE_MAX_SIZE       := 0x9DFC00
CFG_SHA_SEC_ICC_DDR_SIZE            := (0x50000)
CFG_SHM_SIZE_NR_PM                  := (NRCCPU_LLRAM_BASE_SIZE)
CFG_SHM_SIZE_HAC_PM                 := (0x80000)
CFG_SHM_SIZE_PM_SCTRL               := (0x8000)
CFG_SHM_SIZE_NSRO_RSV               := (DDR_NSRO_SHARED_MEM_SIZE - DDR_SHA_NV_SIZE - SHA_SEC_ICC_DDR_SIZE - SHM_SIZE_NR_PM - SHM_SIZE_HAC_PM - SHM_SIZE_PM_SCTRL)
CFG_DDR_HIFI_MBX_ADDR               := ((DDR_SHARED_MEM_ADDR) + (DDR_NSRO_SHARED_MEM_SIZE))
CFG_DDR_HIFI_MBX_SIZE               := (0X9800)
CFG_SHM_SIZE_HIFI_MBX               :=(DDR_HIFI_MBX_SIZE)
CFG_SHM_SIZE_HIFI                   :=(0)
CFG_SHM_SIZE_TLPHY                  :=(12*1024)
CFG_SHM_SIZE_TEMPERATURE            :=(1*1024)
CFG_SHM_SIZE_DDM_LOAD               :=(0)
CFG_SHM_SIZE_MEM_APPA9_PM_BOOT      :=(0)
CFG_SHM_SIZE_MEM_MDMA9_PM_BOOT      :=(0)
CFG_SHM_SIZE_TENCILICA_MULT_BAND    :=(0)
CFG_SHM_SIZE_ICC                    :=(0x79800)
CFG_SHM_SIZE_IPF                    :=(0)
CFG_SHM_SIZE_PSAM                   :=(0)
CFG_SHM_SIZE_WAN                    :=(0)
CFG_SHM_SIZE_NV_UNSAFE              :=(0x400)
CFG_SHM_SIZE_NV                     :=(NV_DDR_SIZE)
CFG_SHM_SIZE_M3_MNTN                :=(0)
CFG_SHM_SIZE_TIMESTAMP              :=(1*1024)
CFG_SHM_SIZE_IOS                    :=(6*1024)
CFG_SHM_SIZE_RESTORE_AXI            :=(CCPU_LLRAM_BASE_SIZE)
CFG_SHM_SIZE_POWER_MONITOR          :=(0x800)
CFG_SHM_SIZE_PMU                    :=(3*1024)
CFG_SHM_SIZE_PTABLE                 :=(0)
CFG_SHM_SIZE_CCORE_RESET            :=(0x400)
CFG_SHM_SIZE_PM_OM                  :=(256*1024)
CFG_SHM_SIZE_M3PM                   :=(0x1000)
CFG_SHM_SIZE_SLICE_MEM              :=(0x1000)
CFG_SHM_SIZE_OSA_LOG                :=(0)
CFG_SHM_SIZE_WAS_LOG                :=(0)
CFG_SHM_SIZE_SRAM_BAK               :=(0)
CFG_SHM_SIZE_SRAM_TO_DDR            :=(0)
CFG_SHM_SIZE_M3RSRACC_BD            :=(0)
CFG_SHM_SIZE_SIM_MEMORY             :=(0)
CFG_SHM_SIZE_PRODUCT_MEM            :=(0)
CFG_SHM_SIZE_SHA_SEC_ICC            :=(SHA_SEC_ICC_DDR_SIZE)
CFG_NRSHM_SIZE_ICC            :=(0xC0000)
CFG_FEATURE_NV_PARTRION_MULTIPLEX               := FEATURE_ON
CFG_AXIMEM_BASE_ADDR := (0xfe380000)
CFG_AXIMEM_MAX_LEN := (128*1024)
CFG_AXIMEM_CACHE_MAX_LEN := (4*1024)
CFG_AXIMEM_UNCACHE_MAX_LEN := (AXIMEM_MAX_LEN - AXIMEM_CACHE_MAX_LEN)
CFG_CONFIG_BALONG_CORESIGHT := YES
