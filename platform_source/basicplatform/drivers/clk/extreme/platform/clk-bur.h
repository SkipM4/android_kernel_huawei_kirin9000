/*
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __CLK_BUR_H
#define __CLK_BUR_H
/* clk_crgctrl */
#define CLKIN_SYS		0
#define CLKIN_REF		1
#define CLK_FLL_SRC		2
#define CLK_PPLL1		3
#define CLK_PPLL2		4
#define CLK_PPLL3		5
#define CLK_PPLL4		6
#define CLK_FNPLL1		7
#define CLK_FNPLL4		8
#define CLK_SPLL		9
#define CLK_MODEM_BASE		10
#define CLK_LBINTPLL_1		11
#define CLK_PPLL_PCIE		12
#define PCLK		13
#define CLK_GATE_PPLL2		14
#define CLK_GATE_PPLL3		15
#define CLK_GATE_FNPLL4		16
#define CLK_GATE_SPLL_MEDIA		17
#define CLK_GATE_PPLL2_MEDIA		18
#define CLK_GATE_PPLL3_MEDIA		19
#define CLK_SYS_INI		20
#define CLK_DIV_SYSBUS		21
#define CLK_DIV_CFGBUS		22
#define PCLK_GATE_WD0_HIGH		23
#define CLK_GATE_WD0_HIGH		24
#define PCLK_GATE_WD0		25
#define PCLK_GATE_WD1		26
#define CLK_MUX_WD0		27
#define CODECCSSI_MUX		28
#define CLK_GATE_CODECSSI		29
#define PCLK_GATE_CODECSSI		30
#define CLK_FACTOR_TCXO		31
#define CLK_GATE_TIMER5_A		32
#define AUTODIV_SYSBUS		33
#define AUTODIV_EMMC0BUS		34
#define CLK_ATDVFS		35
#define ATCLK		36
#define TRACKCLKIN		37
#define PCLK_DBG		38
#define PCLK_DIV_DBG		39
#define TRACKCLKIN_DIV		40
#define ACLK_GATE_PERF_STAT		41
#define PCLK_GATE_PERF_STAT		42
#define CLK_DIV_PERF_STAT		43
#define CLK_PERF_DIV_GT		44
#define CLK_GATE_PERF_STAT		45
#define CLK_DIV_CSSYSDBG		46
#define CLK_GATE_CSSYSDBG		47
#define CLK_DIV_DMABUS		48
#define CLK_GATE_DMAC		49
#define CLK_GATE_DMA_IOMCU		50
#define CLK_GATE_CSSYS_ATCLK		51
#define CLK_GATE_ODT_ACPU		52
#define CLK_GATE_TCLK_ODT		53
#define CLK_GATE_TIME_STAMP_GT		54
#define CLK_DIV_TIME_STAMP		55
#define CLK_GATE_TIME_STAMP		56
#define CLK_GATE_PFA_TFT		57
#define CLK_GATE_PFA_TFT_PSAM		58
#define ACLK_GATE_DRA		59
#define CLK_GATE_DRA_REF		60
#define CLK_GATE_PFA_GT		61
#define CLK_DIV_PFA		62
#define CLK_GATE_PFA		63
#define HCLK_GATE_PFA		64
#define CLK_GATE_PFA_REF		65
#define ACLK_GATE_AXI_MEM		66
#define CLK_GATE_AXI_MEM		67
#define CLK_GATE_AXI_MEM_GS		68
#define CLK_DIV_VCODECBUS		69
#define CLK_GATE_VCODECBUS_GT		70
#define CLK_MUX_VCODECBUS		71
#define CLK_GATE_SD		72
#define CLK_DIV_MMC0BUS		73
#define CLK_MUX_SD_SYS		74
#define CLK_MUX_SD_PLL		75
#define CLK_DIV_SD		76
#define CLK_ANDGT_SD		77
#define CLK_SD_SYS		78
#define CLK_SD_SYS_GT		79
#define CLK_SDIO_PERI_GT		80
#define CLK_SDIO_PERI_DIV		81
#define CLK_SDIO_SYS		82
#define CLK_MUX_A53HPM		83
#define CLK_A53HPM_ANDGT		84
#define CLK_DIV_A53HPM		85
#define CLK_MUX_320M		86
#define CLK_320M_PLL_GT		87
#define CLK_DIV_320M		88
#define CLK_GATE_UART1		89
#define CLK_GATE_UART4		90
#define PCLK_GATE_UART1		91
#define PCLK_GATE_UART4		92
#define CLK_MUX_UARTH		93
#define CLK_DIV_UARTH		94
#define CLK_ANDGT_UARTH		95
#define CLK_GATE_UART2		96
#define CLK_GATE_UART5		97
#define PCLK_GATE_UART2		98
#define PCLK_GATE_UART5		99
#define CLK_MUX_UARTL		100
#define CLK_DIV_UARTL		101
#define CLK_ANDGT_UARTL		102
#define CLK_GATE_UART0		103
#define PCLK_GATE_UART0		104
#define CLK_MUX_UART0		105
#define CLK_DIV_UART0		106
#define CLK_ANDGT_UART0		107
#define CLK_FACTOR_UART0		108
#define CLK_UART0_DBG		109
#define CLK_GATE_I2C3		110
#define CLK_GATE_I2C4		111
#define CLK_GATE_I2C6_ACPU		112
#define CLK_GATE_I2C7		113
#define CLK_GATE_I2C9		114
#define PCLK_GATE_I2C3		115
#define PCLK_GATE_I2C4		116
#define PCLK_GATE_I2C6_ACPU		117
#define PCLK_GATE_I2C7		118
#define PCLK_GATE_I2C9		119
#define CLK_DIV_I2C		120
#define CLK_MUX_I2C		121
#define CLK_ANDGT_I3C4		122
#define CLK_DIV_I3C4		123
#define CLK_GATE_I3C4		124
#define PCLK_GATE_I3C4		125
#define CLK_GATE_SPI1		126
#define CLK_GATE_SPI4		127
#define PCLK_GATE_SPI1		128
#define PCLK_GATE_SPI4		129
#define CLK_MUX_SPI1		130
#define CLK_DIV_SPI1		131
#define CLK_ANDGT_SPI1		132
#define CLK_MUX_SPI4		133
#define CLK_DIV_SPI4		134
#define CLK_ANDGT_SPI4		135
#define CLK_USB2PHY_REF_DIV		136
#define CLKANDGT_MMC_USBDP		137
#define CLK_DIV_USBDP		138
#define CLK_GATE_MMC_USBDP		139
#define CLK_USB2_DRD_32K		140
#define CLK_USB2DRD_REF		141
#define CLK_GATE_UFSPHY_REF		142
#define CLK_GATE_UFSIO_REF		143
#define PCLK_GATE_PCTRL		144
#define CLK_ANDGT_PTP		145
#define CLK_DIV_PTP		146
#define CLK_GATE_PWM		147
#define CLK_GATE_BLPWM		148
#define CLK_SYSCNT_DIV		149
#define CLK_GATE_GPS_REF		150
#define CLK_MUX_GPS_REF		151
#define CLK_GATE_MDM2GPS0		152
#define CLK_GATE_MDM2GPS1		153
#define CLK_GATE_MDM2GPS2		154
#define CLK_GATE_AO_ASP		155
#define CLK_DIV_AO_ASP		156
#define CLK_MUX_AO_ASP		157
#define CLK_DIV_AO_ASP_GT		158
#define PCLK_GATE_DSI0		159
#define PCLK_GATE_DSI1		160
#define CLK_GATE_LDI0		161
#define CLK_GATE_LDI1		162
#define PERI_VOLT_HOLD		163
#define PERI_VOLT_MIDDLE		164
#define PERI_VOLT_LOW		165
#define EPS_VOLT_HIGH		166
#define EPS_VOLT_MIDDLE		167
#define EPS_VOLT_LOW		168
#define VENC_VOLT_HOLD		169
#define VDEC_VOLT_HOLD		170
#define EDC_VOLT_HOLD		171
#define EFUSE_VOLT_HOLD		172
#define LDI0_VOLT_HOLD		173
#define SEPLAT_VOLT_HOLD		174
#define CLK_MUX_VDEC		175
#define CLK_ANDGT_VDEC		176
#define CLK_DIV_VDEC		177
#define CLK_MUX_VENC		178
#define CLK_ANDGT_VENC		179
#define CLK_DIV_VENC		180
#define CLK_GATE_DPCTRL_16M		181
#define PCLK_GATE_DPCTRL		182
#define ACLK_GATE_DPCTRL		183
#define CLK_GT_ISP_I2C		184
#define CLK_DIV_ISP_I2C		185
#define CLK_GATE_ISP_I2C_MEDIA		186
#define CLK_GATE_ISP_SNCLK0		187
#define CLK_GATE_ISP_SNCLK1		188
#define CLK_GATE_ISP_SNCLK2		189
#define CLK_GATE_ISP_SNCLK3		190
#define CLK_ISP_SNCLK_MUX0		191
#define CLK_ISP_SNCLK_DIV0		192
#define CLK_ISP_SNCLK_MUX1		193
#define CLK_ISP_SNCLK_DIV1		194
#define CLK_ISP_SNCLK_MUX2		195
#define CLK_ISP_SNCLK_DIV2		196
#define CLK_ISP_SNCLK_MUX3		197
#define CLK_ISP_SNCLK_DIV3		198
#define CLK_ISP_SNCLK_FAC		199
#define CLK_ISP_SNCLK_ANGT		200
#define CLK_GATE_RXDPHY0_CFG		201
#define CLK_GATE_RXDPHY1_CFG		202
#define CLK_GATE_RXDPHY2_CFG		203
#define CLK_GATE_RXDPHY3_CFG		204
#define CLK_GATE_TXDPHY0_CFG		205
#define CLK_GATE_TXDPHY0_REF		206
#define CLK_GATE_TXDPHY1_CFG		207
#define CLK_GATE_TXDPHY1_REF		208
#define CLK_ANDGT_RXDPHY		209
#define CLK_FACTOR_RXDPHY		210
#define CLK_MUX_RXDPHY_CFG		211
#define PCLK_GATE_LOADMONITOR		212
#define CLK_GATE_LOADMONITOR		213
#define CLK_DIV_LOADMONITOR		214
#define CLK_GT_LOADMONITOR		215
#define PCLK_GATE_LOADMONITOR_L		216
#define CLK_GATE_LOADMONITOR_L		217
#define CLK_GATE_MEDIA_TCXO		218
#define CLK_MUX_IVP32DSP_CORE		219
#define CLK_ANDGT_IVP32DSP_CORE		220
#define CLK_DIV_IVP32DSP_CORE		221
#define CLK_GATE_FD_FUNC		222
#define CLK_UART6		223
#define CLK_GATE_I2C0		224
#define CLK_GATE_I2C1		225
#define CLK_GATE_I2C2		226
#define CLK_GATE_SPI0		227
#define CLK_FAC_180M		228
#define CLK_GATE_IOMCU_PERI0		229
#define CLK_GATE_SPI2		230
#define CLK_GATE_UART3		231
#define CLK_GATE_UART8		232
#define CLK_GATE_UART7		233
#define CLK_GNSS_ABB		234
#define CLK_PMU32KC		235
#define OSC32K		236
#define OSC19M		237
#define CLK_480M		238
#define CLK_INVALID		239
#define AUTODIV_CFGBUS		240
#define AUTODIV_DMABUS		241
#define AUTODIV_ISP_DVFS		242
#define AUTODIV_ISP		243
#define CLK_GATE_ATDIV_MMC0		244
#define CLK_GATE_ATDIV_DMA		245
#define CLK_GATE_ATDIV_CFG		246
#define CLK_GATE_ATDIV_SYS		247
#define CLK_FPGA_1P92		248
#define CLK_FPGA_2M		249
#define CLK_FPGA_10M		250
#define CLK_FPGA_19M		251
#define CLK_FPGA_20M		252
#define CLK_FPGA_24M		253
#define CLK_FPGA_26M		254
#define CLK_FPGA_27M		255
#define CLK_FPGA_32M		256
#define CLK_FPGA_40M		257
#define CLK_FPGA_48M		258
#define CLK_FPGA_50M		259
#define CLK_FPGA_57M		260
#define CLK_FPGA_60M		261
#define CLK_FPGA_64M		262
#define CLK_FPGA_80M		263
#define CLK_FPGA_100M		264
#define CLK_FPGA_160M		265

/* clk_hsdt_crg */
#define CLK_GATE_PCIEPLL		0
#define CLK_GATE_PCIEPHY_REF		1
#define PCLK_GATE_PCIE_SYS		2
#define PCLK_GATE_PCIE_PHY		3
#define ACLK_GATE_PCIE		4
#define CLK_GATE_HSDT_TBU		5
#define CLK_GATE_HSDT_TCU		6
#define CLK_GATE_PCIE0_MCU		7
#define CLK_GATE_PCIE0_MCU_BUS		8
#define CLK_GATE_PCIE0_MCU_19P2		9
#define CLK_GATE_PCIE0_MCU_32K		10
#define CLK_GATE_SDIO		11
#define CLK_MUX_SDIO_SYS		12
#define CLK_DIV_SDIO		13
#define CLK_ANDGT_SDIO		14
#define CLK_SDIO_SYS_GT		15
#define HCLK_GATE_SDIO		16

/* clk_mmc0_crg */
#define HCLK_GATE_SD		0
#define CLKANDGT_USB2PHY_REF		1
#define CLK_MUX_USB19D2PHY		2
#define CLK_USB2PHY_REF		3
#define CLK_ULPI_REF		4
#define PCLK_HISI_USB20PHY		5
#define PCLK_USB20_SYSCTRL		6
#define HCLK_USB2DRD		7

/* clk_sctrl */
#define CLK_DIV_AOBUS		0
#define CLK_GATE_TIMER5_B		1
#define CLK_MUX_TIMER5_A		2
#define CLK_MUX_TIMER5_B		3
#define CLK_GATE_TIMER5		4
#define CLK_PCIE_AUX_ANDGT		5
#define CLK_PCIE_AUX_DIV		6
#define CLK_PCIE_AUX_MUX		7
#define CLK_GATE_PCIEAUX		8
#define CLK_ANDGT_IOPERI		9
#define CLK_DIV_IOPERI		10
#define CLK_MUX_IOPERI		11
#define CLK_GATE_SPI		12
#define PCLK_GATE_SPI		13
#define CLK_GATE_SPI5		14
#define PCLK_GATE_SPI5		15
#define CLK_DIV_SPI5		16
#define CLK_ANDGT_SPI5		17
#define CLK_MUX_SPI5		18
#define PCLK_GATE_RTC		19
#define PCLK_GATE_RTC1		20
#define PCLK_GATE_SYSCNT		21
#define CLK_GATE_SYSCNT		22
#define CLKMUX_SYSCNT		23
#define CLK_ASP_BACKUP		24
#define CLKGT_ASP_CODEC		25
#define CLKDIV_ASP_CODEC		26
#define CLK_MUX_ASP_CODEC		27
#define CLK_ASP_CODEC		28
#define CLK_GATE_ASP_SUBSYS		29
#define CLK_MUX_ASP_PLL		30
#define CLK_AO_ASP_MUX		31
#define CLK_GATE_ASP_TCXO		32
#define PCLK_GATE_AO_LOADMONITOR		33
#define CLK_GATE_AO_LOADMONITOR		34
#define CLK_DIV_AO_LOADMONITOR		35
#define CLK_GT_AO_LOADMONITOR		36
#define CLK_SW_AO_LOADMONITOR		37

/* clk_iomcu_crgctrl */
#define CLK_I2C1_GATE_IOMCU		0

/* clk_media1_crg */
#define CLK_MUX_VIVOBUS		0
#define CLK_GATE_VIVOBUS_ANDGT		1
#define CLK_DIV_VIVOBUS		2
#define CLK_GATE_VIVOBUS		3
#define PCLK_GATE_ISP_NOC_SUBSYS		4
#define ACLK_GATE_ISP_NOC_SUBSYS		5
#define PCLK_GATE_DISP_NOC_SUBSYS		6
#define ACLK_GATE_DISP_NOC_SUBSYS		7
#define PCLK_GATE_DSS		8
#define ACLK_GATE_DSS		9
#define CLK_GATE_EDC0FREQ		10
#define CLK_DIV_EDC0		11
#define CLK_ANDGT_EDC0		12
#define CLK_MUX_EDC0		13
#define ACLK_GATE_ISP		14
#define CLK_MUX_ISPI2C		15
#define CLK_GATE_ISPI2C		16
#define CLK_GATE_ISP_SYS		17
#define CLK_ANDGT_ISP_I3C		18
#define CLK_DIV_ISP_I3C		19
#define CLK_GATE_ISP_I3C		20
#define CLK_MUX_ISPCPU		21
#define CLK_ANDGT_ISPCPU		22
#define CLK_DIV_ISPCPU		23
#define CLK_GATE_ISPCPU		24
#define CLK_MUX_ISPFUNC		25
#define CLK_ANDGT_ISPFUNC		26
#define CLK_DIV_ISPFUNC		27
#define CLK_GATE_ISPFUNCFREQ		28
#define CLK_MUX_ISPFUNC2		29
#define CLK_ANDGT_ISPFUNC2		30
#define CLK_DIV_ISPFUNC2		31
#define CLK_GATE_ISPFUNC2FREQ		32
#define CLK_MUX_ISPFUNC3		33
#define CLK_ANDGT_ISPFUNC3		34
#define CLK_DIV_ISPFUNC3		35
#define CLK_GATE_ISPFUNC3FREQ		36
#define PCLK_GATE_MEDIA1_LM		37
#define CLK_GATE_LOADMONITOR_MEDIA1		38
#define CLK_JPG_FUNC_MUX		39
#define CLK_ANDGT_JPG_FUNC		40
#define CLK_DIV_JPG_FUNC		41
#define CLK_GATE_JPG_FUNCFREQ		42
#define ACLK_GATE_JPG		43
#define PCLK_GATE_JPG		44
#define ACLK_GATE_NOC_ISP		45
#define PCLK_GATE_NOC_ISP		46
#define CLK_FDAI_FUNC_MUX		47
#define CLK_ANDGT_FDAI_FUNC		48
#define CLK_DIV_FDAI_FUNC		49
#define CLK_GATE_FDAI_FUNCFREQ		50
#define ACLK_GATE_ASC		51
#define CLK_GATE_DSS_AXI_MM		52
#define CLK_GATE_MMBUF		53
#define PCLK_GATE_MMBUF		54
#define CLK_SW_MMBUF		55
#define ACLK_DIV_MMBUF		56
#define CLK_MMBUF_PLL_ANDGT		57
#define PCLK_DIV_MMBUF		58
#define PCLK_MMBUF_ANDGT		59
#define CLK_GATE_ATDIV_VIVO		60
#define CLK_GATE_ATDIV_ISPCPU		61

/* clk_media2_crg */
#define CLK_GATE_VCODECBUS		0
#define CLK_GATE_VDECFREQ		1
#define PCLK_GATE_VDEC		2
#define ACLK_GATE_VDEC		3
#define CLK_GATE_VENCFREQ		4
#define PCLK_GATE_VENC		5
#define ACLK_GATE_VENC		6
#define PCLK_GATE_MEDIA2_LM		7
#define CLK_GATE_LOADMONITOR_MEDIA2		8
#define CLK_GATE_IVP32DSP_TCXO		9
#define CLK_GATE_IVP32DSP_COREFREQ		10
#define CLK_GATE_AUTODIV_VCODECBUS		11
#define CLK_GATE_ATDIV_VDEC		12
#define CLK_GATE_ATDIV_VENC		13

/* clk_pctrl */
#define CLK_USB2PHY_REF_MUX		0

/* clk_xfreqclk */
#define CLK_CLUSTER0		0
#define CLK_CLUSTER1		1
#define CLK_G3D		2
#define CLK_DDRC_FREQ		3
#define CLK_DDRC_MAX		4
#define CLK_DDRC_MIN		5
#define CLK_L1BUS_MIN		6

/* clk_pmuctrl */
#define CLK_GATE_ABB_192		0
#define CLK_PMU32KA		1
#define CLK_PMU32KB		2
#define CLK_PMUAUDIOCLK		3
#define CLK_GATE_NFC		4

/* clk_interactive */
#define CLK_SPLL_VOTE		0

#define CLK_GATE_EDC0		0
#define CLK_GATE_VDEC		1
#define CLK_GATE_VENC		2
#define CLK_GATE_ISPFUNC		3
#define CLK_GATE_JPG_FUNC		4
#define CLK_GATE_FDAI_FUNC		5
#define CLK_GATE_IVP32DSP_CORE		6
#define CLK_GATE_ISPFUNC2		7
#define CLK_GATE_ISPFUNC3		8
#define CLK_GATE_HIFACE		9
#define CLK_GATE_NPU_DVFS		10


#endif	/* __CLK_BUR_H */
