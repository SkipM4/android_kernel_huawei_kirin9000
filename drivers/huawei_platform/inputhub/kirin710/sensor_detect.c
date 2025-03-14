/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
 * Description: some functions of sensorhub power
 * Author: DIVS_SENSORHUB
 * Create: 2012-05-29
 */
#include "sensor_detect.h"

#include <linux/delay.h>
#include <linux/err.h>
#include <platform_include/basicplatform/linux/hw_cmdline_parse.h>
#include <linux/module.h>
#include <linux/mtd/hisi_nve_interface.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pm_wakeup.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/types.h>

#include <huawei_platform/inputhub/sensorhub.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include <securec.h>

#include "als_detect.h"
#include "cap_prox_detect.h"
#include "contexthub_boot.h"
#include "contexthub_debug.h"
#include "contexthub_pm.h"
#include "contexthub_route.h"
#include "handpress_detect.h"
#include "huawei_thp_attr.h"
#include "motion_detect.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "sensor_config.h"
#include "sensor_sysfs.h"
#ifdef CONFIG_CONTEXTHUB_SHMEM
#include "shmem.h"
#endif
#include "tof_detect.h"
#include "vibrator_detect.h"

#define ADAPT_SENSOR_LIST_NUM           20
#define TP_REPLACE_PS        1
#define GPIO_STAT_HIGH       1
#define GPIO_STAT_LOW        0
#define RESET_SHORT_SLEEP    5
#define RESET_LONG_SLEEP     10
#define FPC_NAME_LEN         3
#define SYNA_NAME_LEN        4
#define GOODIX_NAME_LEN      6
#define SILEAD_NAME_LEN      6
#define QFP_NAME_LEN         3
#define EGIS_NAME_LEN        4
#define GOODIX_G_NAME_LEN    15
#define FP_SPI_NUM           2
#define RET_FAIL             (-1)
#define RET_SUCC             0
#define IMG_START_SLEEP      1

#define GOODIX_SENSOR_SLEEP          100
#define GOODIX_WRITE_CMD             0xF0
#define GOODIX_IDLE_MODE             0xC0
#define GOODIX_IDLE_MODE_TX_LEN      1
#define GOODIX_WRITE_CMD_TX_LEN      3
#define GOODIX_WRITE_WAKEUP_CMD_TX_LEN  7
#define GOODIX_G2_CHIP_ID_ADDR_HIGH  0x43
#define GOODIX_G2_CHIP_ID_ADDR_LOW   0x04
#define GOODIX_G3_CHIP_ID_ADDR_HIGH  0x00
#define GOODIX_G3_CHIP_ID_ADDR_LOW   0x00
#define BYTE_MASK                    0xFF
#define BYTE_SHIFT                   8
#define WORD_LEN_HIGH                0x00
#define WORD_LEN_LOW                 0x01
#define DEVICE_ID_LEN                4
#define AW_DETECT_LEN                2

#define PS_DEV_COUNT_MAX             1
#define PS_DEVICE_ID_0               0
#define TOF_DEV_COUNT_MAX            2

static int g_sensor_tof_flag;
static int g_hifi_supported;
static struct sensor_redetect_state s_redetect_state;
static struct wakeup_source *sensor_rd;
static struct work_struct redetect_work;
static const char *str_soft_para = "softiron_parameter";
static char g_dyn_buf[MAX_PKT_LENGTH] = { 0 };
static pkt_sys_dynload_req_t *dyn_req = (pkt_sys_dynload_req_t *)g_dyn_buf;
struct sleeve_detect_pare sleeve_detect_paremeter[MAX_PHONE_COLOR_NUM] = {{0,0},};
struct sensorlist_info sensorlist_info[SENSOR_MAX];
static u32 tof_replace_ps_flag = 0;
static u32 replace_ps_type = 0;
static uint8_t g_gyro_cali_way;
static uint8_t g_acc_cali_way;
int mag_threshold_for_als_calibrate = 0;
int gyro_detect_flag = 0;
int ps_support_mode = 0;
int ps_external_ir_calibrate_flag = 0;
struct regulator *ps_external_ir_vdd = NULL;
static uint8_t register_add_len = 1;

int akm_cal_algo;
uint8_t gyro_position;
static uint16_t sensorlist[SENSOR_LIST_NUM];
extern int gyro_range;

static int get_combo_bus_tag(const char *bus, uint8_t *tag);
static int support_hall_hishow = 0;
/* hall_num_1: Hall is supported by default. */
static int g_hall_number = 1;

/*lint -e785*/
struct gyro_platform_data gyro_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.poll_interval = 10, /* inteval 10 ms */
	.position = 1, /* gyro position for OIS */
	.axis_map_x = 1,
	.axis_map_y = 0,
	.axis_map_z = 2, /* index 2 */
	.negate_x = 1,
	.negate_y = 0,
	.negate_z = 1,
	.gpio_int1 = 210, /* gpio 210 */
	.gpio_int2 = 0,
	.gpio_int2_sh = 0,
	.fac_fix_offset_y = 100, /* 100 times than real value */
	.still_calibrate_threshold = 5, /* 5 dps */
	.calibrate_way = 0,
	.calibrate_thredhold = 572, /* 572 dps */
	.gyro_range = 2000, /* 2000 dps */
};

struct g_sensor_platform_data gsensor_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.poll_interval = 10, /* interval 10 ms */
	.axis_map_x = 1,
	.axis_map_y = 0,
	.axis_map_z = 2, /* index 2 */
	.negate_x = 0,
	.negate_y = 1,
	.negate_z = 0,
	.used_int_pin = 2, /* used int pin 2 */
	.gpio_int1 = 208, /* gpio int1 208 */
	.gpio_int2 = 0,
	.gpio_int2_sh = 0,
	.device_type = 0,
	.calibrate_style = 0,
	.calibrate_way = 0,
	.x_calibrate_thredhold = 250, /* 250 mg */
	.y_calibrate_thredhold = 250, /* 250 mg */
	.z_calibrate_thredhold = 320, /* 320 mg */
	.wakeup_duration = 0x60, /* default set up 3 duration */
};

struct compass_platform_data mag_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.poll_interval = 10, /* interval 10 ms */
	.outbit = 0,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2, /* index 2 */
	.soft_filter = 0,
	.calibrate_method = 1,
	.charger_trigger = 0,
};

struct ps_platform_data ps_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.min_proximity_value = 750, /* min proximity value 750 */
	.pwindows_value = 100, /* pwindows value 100 */
	.pwave_value = 60, /* pwave value 60 */
	.threshold_value = 70, /* threshold value 70 */
	.rdata_under_sun = 5500, /* rdata under sun 5500 */
	.ps_pulse_count = 5, /* ps pulse count 5 */
	.gpio_int1 = 206, /* gpio int1 206 */
	.persistent = 0x33,
	.ptime = 0xFF,
	.poll_interval = 250, /* poll interval 250 */
	.init_time = 100, /* init time 100 */
	.ps_oily_threshold = 2, /* ps oily threshold 2 */
	.use_oily_judge = 0,
	.ps_calib_20cm_threshold = 120, /* ps calib 20cm threshold 120 */
	.ps_calib_5cm_threshold = 200, /* ps calib 5cm threshold 200 */
	.ps_calib_3cm_threshold = 250, /* ps calib 3cm threshold 250 */
	.wtime = 100, /* 100 ms */
	.pulse_len = 8, /* 8 us */
	.pgain = 4, /* pgain 4 */
	.led_current = 0x40, /* mA */
	.led_limited_curr = 0xC0,
	.pd_current = 0x03,
	.prox_avg = 2, /* ps average contrl 2 */
	.offset_max = 200, /* offset max 200 */
	.offset_min = 50, /* offset min 50 */
	.oily_max_near_pdata = 230, /* oily max near pdata 230 */
	.max_oily_add_pdata = 50, /* max oily add pdata 50 */
	.max_near_pdata_loop = 4, /* max near pdata loop 4 */
	.oily_count_size = 12, /* oily count size 12 */
	.ps_tp_threshold = 0,
};

static struct ps_device_info g_ps_dev_info[PS_DEV_COUNT_MAX];
static struct tof_device_info g_tof_dev_info[TOF_DEV_COUNT_MAX] = {};

struct ps_extend_platform_data ps_extend_data = {
	.external_ir_mode_flag = 0,
	.external_ir_avg_algo = 0,
	.external_ir_calibrate_noise_max = 100, /* noise max 100 */
	.external_ir_calibrate_noise_min = 3, /* noise min 3 */
	.external_ir_calibrate_far_threshold_max = 800, /* far max 800 */
	.external_ir_calibrate_far_threshold_min = 10, /* far min 10 */
	.external_ir_calibrate_near_threshold_max = 1500, /* near max 1500 */
	.external_ir_calibrate_near_threshold_min = 20, /* near min 20 */
	.external_ir_calibrate_pwindows_max = 800, /* pwindows max 800 */
	.external_ir_calibrate_pwindows_min = 3, /* pwindows min 3 */
	.external_ir_calibrate_pwave_max = 1500, /* pwave max 1500 */
	.external_ir_calibrate_pwave_min = 5, /* pwave min 5 */
	.min_proximity_value = 850, /* min proximity value 850 */
	.pwindows_value = 5, /* pwindows value 5 */
	.pwave_value = 20, /* pwave value 20 */
	.threshold_value = 20, /* threshold value 20 */
	.calibrate_noise = 30, /* calibrate noise 30 */
};

struct ps_external_ir_parameter ps_external_ir_param = {
	.external_ir = 0,
	.internal_ir_min_proximity_value = 750, /* min proximity value 750 */
	.external_ir_min_proximity_value = 850, /* min proximity value 850 */
	.internal_ir_pwindows_value = 75, /* pwindows value 75 */
	.external_ir_pwindows_value = 300, /* pwindows value 300 */
	.internal_ir_pwave_value = 10, /* pwave value 10 */
	.external_ir_pwave_value = 55, /* pwave value 55 */
	.internal_ir_threshold_value = 35, /* threshold value 35 */
	.external_ir_threshold_value = 60, /* threshold value 60 */
	.external_ir_calibrate_noise = 30, /* calibrate noise 30 */
	.external_ir_enable_gpio = 67, /* enable gpio 67 */
	.external_ir_powermode = 0,
	.external_ir_pwindows_ratio = 1000, /* pwindows ratio 1000 */
	.external_ir_pwave_ratio = 1000, /* pwave ratio 1000 */
};

struct airpress_platform_data airpress_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.poll_interval = 1000, /* interval 1000 ms */
};

static struct tof_platform_data tof_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
};

static struct tof_platform_data_ext tof1_data_ext = {
	.cfg = DEF_SENSOR_COM_SETTING,
};

static struct humiture_platform_data humiture_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.poll_interval = 100, /* interval 100 ms */
};

struct cap_prox_platform_data cap_prox_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.init_reg_val = {
		0x00010005, 0x00020529, 0x000300cc, 0x00040001,
		0x00050F55,
		0x00069905, 0x000700e8, 0x00080200, 0x00090000,
		0x000a000C, 0x00798000,
		0x000b9905, 0x000c00e8, 0x000d0200, 0x000e0000,
		0x000f000C, 0x007a8000
	},
	.poll_interval = 200, /* interval 200 ms */
};

static struct gps_4774_platform_data gps_4774_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.poll_interval = 50, /* interval 50 ms */
	.gpio1_gps_cmd_ap = 200, /* gpio1 gps cmd ap 200 */
	.gpio1_gps_cmd_sh = 230, /* gpio1 gps cmd sh 230 */
	.gpio2_gps_ready_ap = 213, /* gpio2 gps ready ap 213 */
	.gpio2_gps_ready_sh = 242, /* gpio2 gps ready sh 242 */
	.gpio3_wakeup_gps_ap = 214, /* gpio3 wakeup gps ap 214 */
	.gpio3_wakeup_gps_sh = 243, /* gpio3 wakeup gps sh 243 */
};

static struct fingerprint_platform_data fingerprint_data = {
	.cfg = {
		.bus_type = TAG_SPI,
		.bus_num = 2, /* bus num 2 */
		.disable_sample_thread = 1,
		{ .ctrl = { .data = 218 } }, /* ctrl data 218 */
	},
	.reg = 0xFC,
	.chip_id = 0x021b,
	.product_id = 9, /* product id 9 */
	.gpio_irq = 207, /* gpio irq 207 */
	.gpio_irq_sh = 236, /* gpio irq sh 236 */
	.gpio_reset = 149, /* gpio reset 149 */
	.gpio_reset_sh = 1013, /* gpio reset sh 1013 */
	.gpio_cs = 218, /* gpio cs 218 */
	.poll_interval = 50, /* poll interval 50 ms */
};

static struct fingerprint_platform_data fingerprint_ud_data = {
	.cfg = {
		.bus_type = TAG_SPI,
		.bus_num = 2, /* bus num 2 */
		.disable_sample_thread = 1,
		{ .ctrl = { .data = 218 } }, /* ctrl data 218 */
	},
	.reg = 0xF1,
	.chip_id = 0x1204,
	.product_id = 35, /* product id 35 */
	.gpio_irq = 147, /* gpio irq 147 */
	.gpio_irq_sh = 1020, /* gpio irq sh 1020 */
	.gpio_reset = 211, /* gpio reset 211 */
	.gpio_cs = 216, /* gpio cs 216 */
	.poll_interval = 50, /* poll interval 50 ms */
	.tp_hover_support = 0,
};

static struct tp_ud_platform_data tp_ud_data = {
	.cfg = {
		.bus_type = TAG_I3C,
		.bus_num = 0,
		.disable_sample_thread = 1,
		{ .i2c_address = 0x70 },
	},
	.gpio_irq = 186, /* gpio irq 186 */
	.gpio_irq_sh = 1001, /* gpio irq sh 1001 */
	.gpio_cs = 231, /* gpio cs 231 */
	.gpio_irq_pull_up_status = 0,
	.pressure_support = 0,
	.anti_forgery_support = 0,
	.spi_max_speed_hz = 10000000, /* spi max speed hz 10000000 */
	.spi_mode = 0,
	.ic_type = 0,
	.hover_enable = 0,
	.i2c_max_speed_hz = 0,
	.aod_display_support = 0,
};

static struct key_platform_data key_data = {
	.cfg = {
		.bus_type = TAG_I2C,
		.bus_num = 0,
		.disable_sample_thread = 0,
		{ .i2c_address = 0x27 },
	},
	.i2c_address_bootloader = 0x28,
	.poll_interval = 30, /* poll interval 30 ms */
};
struct magn_bracket_platform_data magn_bracket_data = {
	.cfg = DEF_SENSOR_COM_SETTING, /* donot use, just give it a value */
	.mag_x_change_lower = 0,
	.mag_x_change_upper = 0,
	.mag_y_change_lower = 0,
	.mag_y_change_upper = 0,
	.mag_z_change_lower = 0,
	.mag_z_change_upper = 0,
};
struct aod_platform_data aod_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.feature_set = { 0, },
};
struct rpc_platform_data rpc_data = {
	.table = { 0 },
	.mask = { 0 },
	.default_value = 0,
};
struct sar_sensor_detect semtech_sar_detect = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.detect_flag = 0,
	.chip_id = 0,
};

struct sar_sensor_detect aw9610_sar_detect = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.detect_flag = 0,
	.chip_id = 0,
};

struct sar_sensor_detect adi_sar_detect = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.detect_flag = 0,
	.chip_id = 0,
};

struct sar_sensor_detect cypress_sar_detect = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.detect_flag = 0,
	.chip_id = 0,
};

struct sar_sensor_detect g_abov_sar_detect = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.detect_flag = 0,
	.chip_id = 0,
};

struct sensor_detect_manager sensor_manager[SENSOR_MAX] = {
	{ "acc", ACC, DET_INIT, TAG_ACCEL, &gsensor_data, sizeof(gsensor_data) },
	{ "mag", MAG, DET_INIT, TAG_MAG, &mag_data, sizeof(mag_data) },
	{ "gyro", GYRO, DET_INIT, TAG_GYRO, &gyro_data, sizeof(gyro_data) },
	{ "als", ALS, DET_INIT, TAG_ALS, NULL, 0 },
	{ "ps", PS, DET_INIT, TAG_PS, &ps_data, sizeof(ps_data) },
	{ "airpress", AIRPRESS, DET_INIT, TAG_PRESSURE, &airpress_data,
		sizeof(airpress_data) },
	{ "handpress", HANDPRESS, DET_INIT, TAG_HANDPRESS, NULL, 0 },
	{ "cap_prox", CAP_PROX, DET_INIT, TAG_CAP_PROX, NULL, 0 },
	{ "gps_4774_i2c", GPS_4774_I2C, DET_INIT, TAG_GPS_4774_I2C, &gps_4774_data,
		sizeof(gps_4774_data) },
	{ "fingerprint", FINGERPRINT, DET_INIT, TAG_FP, &fingerprint_data,
		sizeof(fingerprint_data) },
	{ "key", KEY, DET_INIT, TAG_KEY, &key_data, sizeof(key_data) },
	{ "hw_magn_bracket", MAGN_BRACKET, DET_INIT, TAG_MAGN_BRACKET, &magn_bracket_data,
		sizeof(magn_bracket_data) },
	{ "rpc", RPC, DET_INIT, TAG_RPC, &rpc_data, sizeof(rpc_data) },
	{ "vibrator", VIBRATOR, DET_INIT, TAG_VIBRATOR, NULL, 0 },
	{ "fingerprint_ud", FINGERPRINT_UD, DET_INIT, TAG_FP_UD, &fingerprint_ud_data,
		sizeof(fingerprint_ud_data) },
	{ "tof", TOF, DET_INIT, TAG_TOF, &tof_data, sizeof(tof_data) },
	{ "tp_ud", TP_UD, DET_INIT, TAG_TP, &tp_ud_data, sizeof(tp_ud_data) },
	{ "sh_aod", SH_AOD, DET_INIT, TAG_AOD, &aod_data, sizeof(aod_data) },
	{ "motion", MOTION, DET_INIT, TAG_MOTION, NULL, 0 },
	{ "humiture", HUMITURE, DET_INIT, TAG_HUMITURE, &humiture_data, sizeof(humiture_data) },
	{ "tof1", TOF1, DET_INIT, TAG_TOF1, &tof1_data_ext, sizeof(tof1_data_ext) },
};

static const struct app_link_info app_link_info_gyro[] = {
	{ SENSORHUB_TYPE_ACCELEROMETER, TAG_ACCEL, 1, {TAG_ACCEL} },
	{ SENSORHUB_TYPE_LIGHT, TAG_ALS, 1, {TAG_ALS} },
	{ SENSORHUB_TYPE_PROXIMITY, TAG_PS, 1, {TAG_PS} },
	{ SENSORHUB_TYPE_GYROSCOPE, TAG_GYRO, 1, {TAG_GYRO} },
	{ SENSORHUB_TYPE_GRAVITY, TAG_GRAVITY, 3,
		{ TAG_ACCEL, TAG_GYRO, TAG_MAG } },
	{ SENSORHUB_TYPE_MAGNETIC, TAG_MAG, 2, { TAG_GYRO, TAG_MAG, } },
	{ SENSORHUB_TYPE_LINEARACCELERATE, TAG_LINEAR_ACCEL, 3,
		{ TAG_ACCEL, TAG_GYRO, TAG_MAG } },
	{ SENSORHUB_TYPE_ORIENTATION, TAG_ORIENTATION, 3,
		{ TAG_ACCEL, TAG_GYRO, TAG_MAG } },
	{ SENSORHUB_TYPE_ROTATEVECTOR, TAG_ROTATION_VECTORS, 3,
		{ TAG_ACCEL, TAG_GYRO, TAG_MAG } },
	{ SENSORHUB_TYPE_PRESSURE, TAG_PRESSURE, 1, {TAG_PRESSURE} },
	{ SENSORHUB_TYPE_HALL, TAG_HALL, 0, {0} },
	{ SENSORHUB_TYPE_MAGNETIC_FIELD_UNCALIBRATED, TAG_MAG_UNCALIBRATED, 2,
		{ TAG_MAG, TAG_GYRO } },
	{ SENSORHUB_TYPE_GAME_ROTATION_VECTOR, TAG_GAME_RV, 2,
		{ TAG_ACCEL, TAG_GYRO } },
	{ SENSORHUB_TYPE_GYROSCOPE_UNCALIBRATED, TAG_GYRO_UNCALIBRATED, 1,
		{TAG_GYRO} },
	{ SENSORHUB_TYPE_SIGNIFICANT_MOTION, TAG_SIGNIFICANT_MOTION, 1,
		{TAG_ACCEL} },
	{ SENSORHUB_TYPE_STEP_DETECTOR, TAG_STEP_DETECTOR, 1, {TAG_ACCEL} },
	{ SENSORHUB_TYPE_STEP_COUNTER, TAG_STEP_COUNTER, 1, {TAG_ACCEL} },
	{ SENSORHUB_TYPE_GEOMAGNETIC_ROTATION_VECTOR, TAG_GEOMAGNETIC_RV,
		3, { TAG_ACCEL, TAG_GYRO, TAG_MAG } },
	{ SENSORHUB_TYPE_HANDPRESS, TAG_HANDPRESS, 1, {TAG_HANDPRESS} },
	{ SENSORHUB_TYPE_CAP_PROX, TAG_CAP_PROX, 1, {TAG_CAP_PROX} },
	{ SENSORHUB_TYPE_PHONECALL, TAG_PHONECALL, 2, { TAG_ACCEL, TAG_PS } },
	{ SENSORHUB_TYPE_MAGN_BRACKET, TAG_MAGN_BRACKET, 2,
		{ TAG_ACCEL, TAG_MAG } },
	{ SENSORHUB_TYPE_HINGE, TAG_HINGE, 1, {TAG_ACCEL} },
	{ SENSORHUB_TYPE_META_DATA, TAG_FLUSH_META, 0, {0} },
	{ SENSORHUB_TYPE_RPC, TAG_RPC, 3,
		{ TAG_ACCEL, TAG_GYRO, TAG_CAP_PROX } },
	{ SENSORHUB_TYPE_AGT, TAG_AGT, 0, {0} },
	{ SENSORHUB_TYPE_COLOR, TAG_COLOR, 0, {0} },
	{ SENSORHUB_TYPE_ACCELEROMETER_UNCALIBRATED, TAG_ACCEL_UNCALIBRATED, 1,
		{TAG_ACCEL} },
	{ SENSORHUB_TYPE_TOF, TAG_TOF, 1, {TAG_TOF} },
	{ SENSORHUB_TYPE_DROP, TAG_DROP, 1, {TAG_ACCEL} },
	{ SENSORHUB_TYPE_RELATIVE_HUMIDITY, TAG_HUMITURE, 1, {TAG_HUMITURE} },
	{ SENSORHUB_TYPE_TOF1, TAG_TOF1, 1, {TAG_TOF1} },
};

static const struct app_link_info app_link_info_no_gyro[] = {
	{ SENSORHUB_TYPE_ACCELEROMETER, TAG_ACCEL, 1, {TAG_ACCEL} },
	{ SENSORHUB_TYPE_LIGHT, TAG_ALS, 1, {TAG_ALS} },
	{ SENSORHUB_TYPE_PROXIMITY, TAG_PS, 1, {TAG_PS} },
	{ SENSORHUB_TYPE_GYROSCOPE, TAG_GYRO, 2, { TAG_MAG, TAG_ACCEL } },
	{ SENSORHUB_TYPE_GRAVITY, TAG_GRAVITY, 2, { TAG_ACCEL, TAG_MAG } },
	{ SENSORHUB_TYPE_MAGNETIC, TAG_MAG, 1, {TAG_MAG} },
	{ SENSORHUB_TYPE_LINEARACCELERATE, TAG_LINEAR_ACCEL, 2,
		{ TAG_ACCEL, TAG_MAG } },
	{ SENSORHUB_TYPE_ORIENTATION, TAG_ORIENTATION, 2,
		{ TAG_ACCEL, TAG_MAG } },
	{ SENSORHUB_TYPE_ROTATEVECTOR, TAG_ROTATION_VECTORS, 2,
		{ TAG_ACCEL, TAG_MAG } },
	{ SENSORHUB_TYPE_PRESSURE, TAG_PRESSURE, 1, {TAG_PRESSURE} },
	{ SENSORHUB_TYPE_HALL, TAG_HALL, 0, {0} },
	{ SENSORHUB_TYPE_MAGNETIC_FIELD_UNCALIBRATED, TAG_MAG_UNCALIBRATED, 1,
		{TAG_MAG} },
	{ SENSORHUB_TYPE_GAME_ROTATION_VECTOR, TAG_GAME_RV, 2,
		{ TAG_ACCEL, TAG_MAG } },
	{ SENSORHUB_TYPE_GYROSCOPE_UNCALIBRATED, TAG_GYRO_UNCALIBRATED, 0, {0} },
	{ SENSORHUB_TYPE_SIGNIFICANT_MOTION, TAG_SIGNIFICANT_MOTION, 1, {TAG_ACCEL} },
	{ SENSORHUB_TYPE_STEP_DETECTOR, TAG_STEP_DETECTOR, 1, {TAG_ACCEL} },
	{ SENSORHUB_TYPE_STEP_COUNTER, TAG_STEP_COUNTER, 1, {TAG_ACCEL} },
	{ SENSORHUB_TYPE_GEOMAGNETIC_ROTATION_VECTOR, TAG_GEOMAGNETIC_RV, 2,
		{ TAG_ACCEL, TAG_MAG } },
	{ SENSORHUB_TYPE_HANDPRESS, TAG_HANDPRESS, 1, {TAG_HANDPRESS} },
	{ SENSORHUB_TYPE_CAP_PROX, TAG_CAP_PROX, 1, {TAG_CAP_PROX} },
	{ SENSORHUB_TYPE_PHONECALL, TAG_PHONECALL, 2, { TAG_ACCEL, TAG_PS } },
	{ SENSORHUB_TYPE_MAGN_BRACKET, TAG_MAGN_BRACKET, 2, { TAG_ACCEL, TAG_MAG } },
	{ SENSORHUB_TYPE_HINGE, TAG_HINGE, 1, {TAG_ACCEL} },
	{ SENSORHUB_TYPE_META_DATA, TAG_FLUSH_META, 0, {0} },
	{ SENSORHUB_TYPE_RPC, TAG_RPC, 2, { TAG_ACCEL, TAG_CAP_PROX } },
	{ SENSORHUB_TYPE_AGT, TAG_AGT, 0, {0} },
	{ SENSORHUB_TYPE_COLOR, TAG_COLOR, 0, {0} },
	{ SENSORHUB_TYPE_ACCELEROMETER_UNCALIBRATED, TAG_ACCEL_UNCALIBRATED, 1,
		{TAG_ACCEL} },
	{ SENSORHUB_TYPE_TOF, TAG_TOF, 1, {TAG_TOF} },
	{ SENSORHUB_TYPE_DROP, TAG_DROP, 1, {TAG_ACCEL} },
	{ SENSORHUB_TYPE_RELATIVE_HUMIDITY, TAG_HUMITURE, 1, {TAG_HUMITURE} },
	{ SENSORHUB_TYPE_TOF1, TAG_TOF1, 1, {TAG_TOF1} },
};

struct sensor_detect_manager *get_sensor_manager(void)
{
	return sensor_manager;
}

struct ps_device_info *ps_get_device_info(int32_t tag)
{
	if (tag == TAG_PS)
		return (&(g_ps_dev_info[PS_DEVICE_ID_0]));

	hwlog_info("%s error, please check tag %d\n", __func__, tag);
	return NULL;
}

struct tof_device_info *tof_get_device_info(int32_t tag)
{
	if (tag == TAG_TOF)
		return (&g_tof_dev_info[0]);
	else if (tag == TAG_TOF1)
		return (&g_tof_dev_info[1]);
	hwlog_err("[%s-%d]:error, please check tag %d\n", __func__, __LINE__, tag);
	return NULL;
}

int get_hall_number(void)
{
	return g_hall_number;
}

int get_support_hall_hishow(void)
{
	return support_hall_hishow;
}

int get_sensor_tof_flag(void)
{
	return g_sensor_tof_flag;
}

int get_hifi_supported(void)
{
	return g_hifi_supported;
}

uint8_t get_gyro_cali_way(void)
{
	return g_gyro_cali_way;
}

uint8_t get_acc_cali_way(void)
{
	return g_acc_cali_way;
}

uint16_t *get_sensorlist(void)
{
	return sensorlist;
}

void add_sensor_list_info_id(uint16_t id)
{
	sensorlist[++sensorlist[0]] = id;
}

struct sensorlist_info *get_sensorlist_info_by_index(enum sensor_detect_list index)
{
	return &sensorlist_info[index];
}

struct sleeve_detect_pare *get_sleeve_detect_parameter(void)
{
	return sleeve_detect_paremeter;
}

/* get app attach sensor info */
const struct app_link_info *get_app_link_info(int type)
{
	size_t i, size;
	const struct app_link_info *app_info = NULL;

	if (gyro_detect_flag) {
		app_info = app_link_info_gyro;
		size = sizeof(app_link_info_gyro) / sizeof(struct app_link_info);
	} else {
		app_info = app_link_info_no_gyro;
		size = sizeof(app_link_info_no_gyro) / sizeof(struct app_link_info);
	}

	for (i = 0; i < size; i++) {
		if (type == app_info[i].hal_sensor_type &&
			app_info[i].used_sensor_cnt > 0 &&
			app_info[i].used_sensor_cnt <= SENSORHUB_TAG_NUM_MAX)
			return &app_info[i];
	}

	return NULL;
}

enum sensor_detect_list get_id_by_sensor_tag(int tag)
{
	int i;

	for (i = 0; i < SENSOR_MAX; i++)
		if (sensor_manager[i].tag == tag)
			break;

	return i;
}

void read_sensorlist_info(struct device_node *dn, int sensor)
{
	int temp = 0;
	char *chip_info = NULL;

	if (of_property_read_string(dn, "sensorlist_name", (const char **)&chip_info) >= 0) {
		strncpy(sensorlist_info[sensor].name, chip_info, MAX_CHIP_INFO_LEN - 1);
		sensorlist_info[sensor].name[MAX_CHIP_INFO_LEN - 1] = '\0';
		hwlog_info("sensor chip info name %s\n", chip_info);
		hwlog_info("sensor SENSOR_DETECT_LIST %d get name %s\n", sensor, sensorlist_info[sensor].name);
	} else {
		sensorlist_info[sensor].name[0] = '\0';
	}
	if (of_property_read_string(dn, "vendor", (const char **)&chip_info) == 0) {
		strncpy(sensorlist_info[sensor].vendor, chip_info, MAX_CHIP_INFO_LEN - 1);
		sensorlist_info[sensor].vendor[MAX_CHIP_INFO_LEN - 1] = '\0';
		hwlog_info("sensor SENSOR_DETECT_LIST %d get vendor %s\n", sensor, sensorlist_info[sensor].vendor);
	} else {
		sensorlist_info[sensor].vendor[0] = '\0';
	}
	if (of_property_read_u32(dn, "version", &temp) == 0) {
		sensorlist_info[sensor].version = temp;
		hwlog_info("sensor SENSOR_DETECT_LIST %d get version %d\n", sensor, temp);
	} else {
		sensorlist_info[sensor].version = -1;
	}
	if (of_property_read_u32(dn, "maxRange", &temp) == 0)
		sensorlist_info[sensor].max_range = temp;
	else
		sensorlist_info[sensor].max_range = -1;

	if (of_property_read_u32(dn, "resolution", &temp) == 0)
		sensorlist_info[sensor].resolution = temp;
	else
		sensorlist_info[sensor].resolution = -1;

	if (of_property_read_u32(dn, "power", &temp) == 0)
		sensorlist_info[sensor].power = temp;
	else
		sensorlist_info[sensor].power = -1;

	if (of_property_read_u32(dn, "minDelay", &temp) == 0)
		sensorlist_info[sensor].min_delay = temp;
	else
		sensorlist_info[sensor].min_delay = -1;

	if (of_property_read_u32(dn, "fifoReservedEventCount", &temp) == 0)
		sensorlist_info[sensor].fifo_reserved_event_count = temp;
	else
		sensorlist_info[sensor].fifo_reserved_event_count = -1;

	if (of_property_read_u32(dn, "fifoMaxEventCount", &temp) == 0)
		sensorlist_info[sensor].fifo_max_event_count = temp;
	else
		sensorlist_info[sensor].fifo_max_event_count = -1;

	if (of_property_read_u32(dn, "maxDelay", &temp) == 0)
		sensorlist_info[sensor].max_delay = temp;
	else
		sensorlist_info[sensor].max_delay = -1;

	if (of_property_read_u32(dn, "flags", &temp) == 0)
		sensorlist_info[sensor].flags = temp;
	else
		sensorlist_info[sensor].flags = -1;
}

void read_chip_info(struct device_node *dn, enum sensor_detect_list sname)
{
	char *chip_info = NULL;
	int ret;

	ret = of_property_read_string(dn, "compatible", (const char **)&chip_info);
	if (ret) {
		hwlog_err("%s:read name_id:%d info fail\n", __func__, sname);
	} else {
		if (strncpy_s(get_sensor_chip_info_address(sname),
			MAX_CHIP_INFO_LEN, chip_info,
			MAX_CHIP_INFO_LEN - 1) != EOK) {
			hwlog_err("%s strncpy_s fail\n", __func__);
			return;
		}
	}
}

void read_dyn_file_list(uint16_t fileid)
{
	dyn_req->file_list[dyn_req->file_count] = fileid;
	dyn_req->file_count++;
}

static void read_acc_data_from_dts(struct device_node *dn)
{
	int temp = 0;

	read_chip_info(dn, ACC);
	if (!of_property_read_u32(dn, "used_int_pin", &temp))
		gsensor_data.used_int_pin = (uint8_t)temp;

	temp = of_get_named_gpio(dn, "gpio_int1", 0);
	if (temp >= 0)
		gsensor_data.gpio_int1 = (GPIO_NUM_TYPE)temp;

	temp = of_get_named_gpio(dn, "gpio_int2", 0);
	if (temp >= 0)
		gsensor_data.gpio_int2 = (GPIO_NUM_TYPE)temp;

	if (!of_property_read_u32(dn, "gpio_int2_sh", &temp))
		gsensor_data.gpio_int2_sh = (GPIO_NUM_TYPE)temp;

	if (!of_property_read_u32(dn, "poll_interval", &temp))
		gsensor_data.poll_interval = (uint16_t)temp;

	if (!of_property_read_u32(dn, "calibrate_style", &temp))
		gsensor_data.calibrate_style = (uint8_t)temp;

	if (!of_property_read_u32(dn, "axis_map_x", &temp))
		gsensor_data.axis_map_x = (uint8_t)temp;

	if (!of_property_read_u32(dn, "axis_map_y", &temp))
		gsensor_data.axis_map_y = (uint8_t)temp;

	if (!of_property_read_u32(dn, "axis_map_z", &temp))
		gsensor_data.axis_map_z = (uint8_t)temp;

	if (!of_property_read_u32(dn, "negate_x", &temp))
		gsensor_data.negate_x = (uint8_t)temp;

	if (!of_property_read_u32(dn, "negate_y", &temp))
		gsensor_data.negate_y = (uint8_t)temp;

	if (!of_property_read_u32(dn, "negate_z", &temp))
		gsensor_data.negate_z = (uint8_t)temp;

	if (!of_property_read_u32(dn, "device_type", &temp))
		gsensor_data.device_type = (uint8_t)temp;

	if (!of_property_read_u32(dn, "x_calibrate_thredhold", &temp))
		gsensor_data.x_calibrate_thredhold = (uint16_t)temp;

	if (!of_property_read_u32(dn, "y_calibrate_thredhold", &temp))
		gsensor_data.y_calibrate_thredhold = (uint16_t)temp;

	if (!of_property_read_u32(dn, "z_calibrate_thredhold", &temp))
		gsensor_data.z_calibrate_thredhold = (uint16_t)temp;

	/* i2c_address should be set when detect success! not here */
	if (!of_property_read_u32(dn, "file_id", &temp))
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;
	dyn_req->file_count++;

	if (!of_property_read_u32(dn, "sensor_list_info_id", &temp))
		add_sensor_list_info_id((uint16_t)temp);

	if (!of_property_read_u32(dn, "calibrate_way", &temp)) {
		gsensor_data.calibrate_way = (uint8_t)temp;
		g_acc_cali_way = (uint8_t)temp;
	}

	if (!of_property_read_u32(dn, "wakeup_duration", &temp))
		gsensor_data.wakeup_duration = (uint8_t)temp;

	read_sensorlist_info(dn, ACC);
}

static void read_mag_data_from_dts(struct device_node *dn)
{
	int temp = 0;

	read_chip_info(dn, MAG);
	if (!of_property_read_u32(dn, "poll_interval", &temp))
		mag_data.poll_interval = (uint16_t)temp;

	if (!of_property_read_u32(dn, "axis_map_x", &temp))
		mag_data.axis_map_x = (uint8_t)temp;

	if (!of_property_read_u32(dn, "axis_map_y", &temp))
		mag_data.axis_map_y = (uint8_t)temp;

	if (!of_property_read_u32(dn, "axis_map_z", &temp))
		mag_data.axis_map_z = (uint8_t)temp;

	if (!of_property_read_u32(dn, "negate_x", &temp))
		mag_data.negate_x = (uint8_t)temp;

	if (!of_property_read_u32(dn, "negate_y", &temp))
		mag_data.negate_y = (uint8_t)temp;

	if (!of_property_read_u32(dn, "negate_z", &temp))
		mag_data.negate_z = (uint8_t)temp;

	if (!of_property_read_u32(dn, "outbit", &temp))
		mag_data.outbit = (uint8_t)temp;

	if (!of_property_read_u32(dn, "softfilter", &temp))
		mag_data.soft_filter = (uint8_t)temp;

	if (!of_property_read_u32(dn, "calibrate_method", &temp))
		mag_data.calibrate_method = (uint8_t)temp;

	if (!of_property_read_u32(dn, "charger_trigger", &temp))
		mag_data.charger_trigger = (uint8_t)temp;

	if (!of_property_read_u32(dn, "threshold_for_als_calibrate", &temp))
		mag_threshold_for_als_calibrate = temp;

	if (of_property_read_u32(dn, "akm_cal_algo", &temp)) {
		akm_cal_algo = 0;
	} else {
		if (temp == 1)
			akm_cal_algo = 1;
		else
			akm_cal_algo = 0;
	}

	/* i2c_address should be set when detect success! not here */

	if (of_property_read_u32(dn, "file_id", &temp))
		hwlog_err("%s:read mag file_id fail\n", __func__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;
	dyn_req->file_count++;

	if (of_property_read_u32(dn, "sensor_list_info_id", &temp))
		hwlog_err("%s:read mag sensor_list_info_id fail\n", __func__);
	else
		add_sensor_list_info_id((uint16_t)temp);

	temp = of_get_named_gpio(dn, "gpio_reset", 0);
	if (temp < 0)
		hwlog_err("%s:read gpio_rst fail\n", __func__);
	else
		mag_data.gpio_rst = (GPIO_NUM_TYPE)temp;
	read_sensorlist_info(dn, MAG);
}

static void read_gyro_data_from_dts(struct device_node *dn)
{
	int temp = 0;

	read_chip_info(dn, GYRO);

	if (of_property_read_u32(dn, "poll_interval", &temp))
		hwlog_err("%s:read mag poll_interval fail\n", __func__);
	else
		gyro_data.poll_interval = (uint16_t)temp;

	if (of_property_read_u32(dn, "position", &temp)) {
		gyro_position = gyro_data.position;
		hwlog_err("%s:read gyro position fail,use default position %d\n",
			__func__, gyro_position);
	} else {
		gyro_data.position = (uint8_t)temp;
		gyro_position = gyro_data.position;
		hwlog_info("%s:read gyro position suc position %d\n", __func__, gyro_position);
	}

	if (of_property_read_u32(dn, "axis_map_x", &temp))
		hwlog_err("%s:read gyro axis_map_x fail\n", __func__);
	else
		gyro_data.axis_map_x = (uint8_t)temp;

	if (of_property_read_u32(dn, "axis_map_y", &temp))
		hwlog_err("%s:read gyro axis_map_y fail\n", __func__);
	else
		gyro_data.axis_map_y = (uint8_t)temp;

	if (of_property_read_u32(dn, "axis_map_z", &temp))
		hwlog_err("%s:read gyro axis_map_z fail\n", __func__);
	else
		gyro_data.axis_map_z = (uint8_t)temp;

	if (of_property_read_u32(dn, "negate_x", &temp))
		hwlog_err("%s:read gyro negate_x fail\n", __func__);
	else
		gyro_data.negate_x = (uint8_t)temp;

	if (of_property_read_u32(dn, "negate_y", &temp))
		hwlog_err("%s:read gyro negate_y fail\n", __func__);
	else
		gyro_data.negate_y = (uint8_t)temp;

	if (of_property_read_u32(dn, "negate_z", &temp))
		hwlog_err("%s:read gyro negate_z fail\n", __func__);
	else
		gyro_data.negate_z = (uint8_t)temp;

	if (of_property_read_u32(dn, "still_calibrate_threshold", &temp))
		hwlog_err("%s:read gyro still_calibrate_threshold fail\n", __func__);
	else
		gyro_data.still_calibrate_threshold = (uint8_t)temp;

	if (of_property_read_u32(dn, "file_id", &temp))
		hwlog_err("%s:read gyro file_id fail\n", __func__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;
	dyn_req->file_count++;

	if (of_property_read_u32(dn, "sensor_list_info_id", &temp))
		hwlog_err("%s:read gyro sensor_list_info_id fail\n", __func__);
	else
		add_sensor_list_info_id((uint16_t)temp);

	temp = of_get_named_gpio(dn, "gpio_int1", 0);
	if (temp < 0)
		hwlog_err("%s:read gpio_int1 fail\n", __func__);
	else
		gyro_data.gpio_int1 = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "fac_fix_offset_Y", &temp)) {
		hwlog_info("%s:read fac_fix_offset_Y fail\n", __func__);
	} else {
		gyro_data.fac_fix_offset_y = (uint8_t)temp;
		hwlog_debug("%s:read acc fac_fix_offset_Y %d\n", __func__, temp);
	}
	if (of_property_read_u32(dn, "calibrate_way", &temp)) {
		hwlog_err("%s:read gyro calibrate_way fail\n", __func__);
	} else {
		gyro_data.calibrate_way = (uint8_t)temp;
		g_gyro_cali_way = (uint8_t)temp;
	}

	if (of_property_read_u32(dn, "calibrate_thredhold", &temp)) {
		hwlog_info("%s:read calibrate_thredhold fail\n", __func__);
	} else {
		gyro_data.calibrate_thredhold = (uint16_t)temp;
		hwlog_debug("%s:read gyro calibrate_thredhold %d\n", __func__, temp);
	}

	if (of_property_read_u32(dn, "gyro_range", &temp)) {
		hwlog_debug("%s:read gyro_range fail\n", __func__);
	} else {
		gyro_data.gyro_range = (uint16_t)temp;
		gyro_range = gyro_data.gyro_range;
		hwlog_info("%s:read gyro gyro_range %d\n", __func__, temp);
	}
	read_sensorlist_info(dn, GYRO);
}

static void read_ps_external_ir_calibrate_threshold(struct device_node *dn)
{
	int temp = 0;

	if (of_property_read_u32(dn, "external_ir_calibrate_noise_max", &temp))
		hwlog_err("%s:read external_ir_calibrate_noise_max fail\n", __func__);
	else
		ps_extend_data.external_ir_calibrate_noise_max = temp;

	if (of_property_read_u32(dn, "external_ir_calibrate_noise_min", &temp))
		hwlog_err("%s:read external_ir_calibrate_noise_min fail\n", __func__);
	else
		ps_extend_data.external_ir_calibrate_noise_min = temp;

	if (of_property_read_u32(dn, "external_ir_calibrate_far_threshold_max", &temp))
		hwlog_err("%s:read external_ir_calibrate_far_threshold_max fail\n", __func__);
	else
		ps_extend_data.external_ir_calibrate_far_threshold_max = temp;

	if (of_property_read_u32(dn, "external_ir_calibrate_far_threshold_min", &temp))
		hwlog_err("%s:read external_ir_calibrate_far_threshold_min fail\n", __func__);
	else
		ps_extend_data.external_ir_calibrate_far_threshold_min = temp;

	if (of_property_read_u32(dn, "external_ir_calibrate_near_threshold_max", &temp))
		hwlog_err("%s:read external_ir_calibrate_near_threshold_max fail\n", __func__);
	else
		ps_extend_data.external_ir_calibrate_near_threshold_max = temp;

	if (of_property_read_u32(dn, "external_ir_calibrate_near_threshold_min", &temp))
		hwlog_err("%s:read external_ir_calibrate_near_threshold_min fail\n", __func__);
	else
		ps_extend_data.external_ir_calibrate_near_threshold_min = temp;
}

static void read_ps_external_ir_pwindow_pwave(struct device_node *dn)
{
	int temp = 0;

	if (of_property_read_u32(dn, "external_ir_calibrate_pwindows_max", &temp))
		hwlog_err("%s:read external_ir_calibrate_pwindows_max fail\n", __func__);
	else
		ps_extend_data.external_ir_calibrate_pwindows_max = temp;

	if (of_property_read_u32(dn, "external_ir_calibrate_pwindows_min", &temp))
		hwlog_err("%s:read external_ir_calibrate_pwindows_min fail\n", __func__);
	else
		ps_extend_data.external_ir_calibrate_pwindows_min = temp;

	if (of_property_read_u32(dn, "external_ir_calibrate_pwave_max", &temp))
		hwlog_err("%s:read external_ir_calibrate_pwave_max fail\n", __func__);
	else
		ps_extend_data.external_ir_calibrate_pwave_max = temp;

	if (of_property_read_u32(dn, "external_ir_calibrate_pwave_min", &temp))
		hwlog_err("%s:read external_ir_calibrate_pwave_min fail\n", __func__);
	else
		ps_extend_data.external_ir_calibrate_pwave_min = temp;
}

static void read_ps_external_ir_para_from_dts(struct device_node *dn)
{
	int temp = 0;

	if (of_property_read_u32(dn, "external_ir_powermode", &temp))
		hwlog_err("%s:read powermode fail\n", __func__);
	else
		ps_external_ir_param.external_ir_powermode = temp;

	if (of_property_read_u32(dn, "external_ir_min_proximity_value", &temp))
		hwlog_err("%s:read mag min_proximity_value fail\n", __func__);
	else
		ps_external_ir_param.external_ir_min_proximity_value = temp;

	if (of_property_read_u32(dn, "external_ir_pwindows_value", &temp))
		hwlog_err("%s:read pwindows_value fail\n", __func__);
	else
		ps_external_ir_param.external_ir_pwindows_value = temp;

	if (of_property_read_u32(dn, "external_ir_pwave_value", &temp))
		hwlog_err("%s:read pwave_value fail\n", __func__);
	else
		ps_external_ir_param.external_ir_pwave_value = temp;

	if (of_property_read_u32(dn, "external_ir_pwindows_ratio", &temp))
		hwlog_err("%s:read pwindows_ratio fail\n", __func__);
	else
		ps_external_ir_param.external_ir_pwindows_ratio = temp;

	if (of_property_read_u32(dn, "external_ir_pwave_ratio", &temp))
		hwlog_err("%s:read pwave_ratio fail\n", __func__);
	else
		ps_external_ir_param.external_ir_pwave_ratio = temp;

	if (of_property_read_u32(dn, "external_ir_threshold_value", &temp))
		hwlog_err("%s:read threshold_value fail\n", __func__);
	else
		ps_external_ir_param.external_ir_threshold_value = temp;

	if (of_property_read_u32(dn, "external_ir_enable_gpio", &temp))
		hwlog_err("%s:read threshold_value fail\n", __func__);
	else
		ps_external_ir_param.external_ir_enable_gpio = temp;

	if (of_property_read_u32(dn, "external_ir_avg_algo", &temp))
		hwlog_err("%s:read external_ir_avg_algo fail\n", __func__);
	else
		ps_extend_data.external_ir_avg_algo = temp;

	read_ps_external_ir_calibrate_threshold(dn);
	read_ps_external_ir_pwindow_pwave(dn);
}

static void read_ps_chip_info(void)
{
	struct ps_device_info *dev_info = &(g_ps_dev_info[PS_DEVICE_ID_0]);
	char *chip_info = get_sensor_chip_info_address(PS);

	if (!strncmp(chip_info, "huawei,txc-pa224",
		sizeof("huawei,txc-pa224")))
		dev_info->chip_type = PS_CHIP_PA224;
	else if (!strncmp(chip_info, "huawei,ams-tmd2620",
		sizeof("huawei,ams-tmd2620")))
		dev_info->chip_type = PS_CHIP_TMD2620;
	else if (!strncmp(chip_info, "huawei,avago-apds9110",
		sizeof("huawei,avago-apds9110")))
		dev_info->chip_type = PS_CHIP_APDS9110;
	else if (!strncmp(chip_info, "huawei,ams-tmd3725",
		sizeof("huawei,ams-tmd3725")))
		dev_info->chip_type = PS_CHIP_TMD3725;
	else if (!strncmp(chip_info, "huawei,liteon-ltr582",
		sizeof("huawei,liteon-ltr582")))
		dev_info->chip_type = PS_CHIP_LTR582;
	else if (!strncmp(chip_info, "huawei,liteon-ltr2568",
		sizeof("huawei,liteon-ltr2568")))
		dev_info->chip_type = PS_CHIP_LTR2568;
	else if (!strncmp(chip_info, "huawei,avago_apds9999",
		sizeof("huawei,avago_apds9999")))
		dev_info->chip_type = PS_CHIP_APDS9999;
	else if (!strncmp(chip_info, "huawei,ams_tmd3702",
		sizeof("huawei,ams_tmd3702")))
		dev_info->chip_type = PS_CHIP_TMD3702;
	else if (!strncmp(chip_info, "huawei,vishay-vcnl36658",
		sizeof("huawei,vishay-vcnl36658")))
		dev_info->chip_type = PS_CHIP_VCNL36658;
	else if (!strncmp(chip_info, "proximity,ps_s001006",
		sizeof("proximity,ps_s001006")))
		dev_info->chip_type = PS_CHIP_TMD2755;
	else
		return;

	hwlog_debug("%s:ps i2c_address suc\n", __func__);
}

static void read_ps_data_from_dts(struct device_node *dn,
	struct sensor_detect_manager *sm)
{
	int temp = 0;
	const char *ps_chip_info = "proximity-tp";
	const char *ps_vendor = "huawei";

	if (!of_property_read_u32(dn, "sensor_list_info_id", &temp))
		add_sensor_list_info_id((uint16_t)temp);

	if (tof_replace_ps_flag != 0) {
		hwlog_info("tof_replace_ps_flag is true skip read ps dts %u\n",
			tof_replace_ps_flag);
		return;
	}
	if (replace_ps_type == TP_REPLACE_PS) {
		strncpy(sensorlist_info[PS].name, ps_chip_info,
			(MAX_CHIP_INFO_LEN - 1));
		sensorlist_info[PS].name[MAX_CHIP_INFO_LEN - 1] = '\0';
		strncpy(sensorlist_info[PS].vendor, ps_vendor,
			(MAX_CHIP_INFO_LEN - 1));
		sensorlist_info[PS].vendor[MAX_CHIP_INFO_LEN - 1] = '\0';
		hwlog_info("tp replace ps, name is %s vendor is %s\n",
			sensorlist_info[PS].name, sensorlist_info[PS].vendor);
		return;
	}

	read_chip_info(dn, PS);
	read_ps_chip_info();

	temp = of_get_named_gpio(dn, "gpio_int1", 0);
	if (temp >= 0)
		ps_data.gpio_int1 = (GPIO_NUM_TYPE)temp;

	if (!of_property_read_u32(dn, "min_proximity_value", &temp))
		ps_data.min_proximity_value = temp;

	if (!of_property_read_u32(dn, "pwindows_value", &temp))
		ps_data.pwindows_value = temp;

	if (!of_property_read_u32(dn, "pwave_value", &temp))
		ps_data.pwave_value = temp;

	if (!of_property_read_u32(dn, "threshold_value", &temp))
		ps_data.threshold_value = temp;

	if (of_property_read_u32(dn, "ps_support_mode", &temp)) {
		ps_support_mode = 0;
		hwlog_err("%s:read ps_support_mode fail\n", __func__);
	} else {
		ps_support_mode = temp;
	}

	if (of_property_read_u32(dn, "external_ir", &temp)) {
		hwlog_err("%s:read mag min_proximity_value fail\n", __func__);
	} else if (temp == 1) {
		ps_external_ir_param.external_ir = temp;
		hwlog_err("%s:external_ir set value\n", __func__);
		ps_external_ir_calibrate_flag = 1;
		read_ps_external_ir_para_from_dts(dn);
	}

	if (!of_property_read_u32(dn, "rdata_under_sun", &temp))
		ps_data.rdata_under_sun = temp;

	if (!of_property_read_u32(dn, "ps_pulse_count", &temp))
		ps_data.ps_pulse_count = (uint8_t)temp;

	if (!of_property_read_u32(dn, "persistent", &temp))
		ps_data.persistent = (uint8_t)temp;

	if (!of_property_read_u32(dn, "ptime", &temp))
		ps_data.ptime = (uint8_t)temp;

	if (!of_property_read_u32(dn, "p_on", &temp))
		ps_data.p_on = (uint8_t)temp;

	if (!of_property_read_u32(dn, "poll_interval", &temp))
		ps_data.poll_interval = (uint16_t)temp;

	if (!of_property_read_u32(dn, "use_oily_judge", &temp))
		ps_data.use_oily_judge = (uint16_t)temp;

	if (!of_property_read_u32(dn, "init_time", &temp))
		ps_data.init_time = (uint16_t)temp;

	if (!of_property_read_u32(dn, "ps_oily_threshold", &temp))
		ps_data.ps_oily_threshold = (uint8_t)temp;

	if (!of_property_read_u32(dn, "ps_calib_20cm_threshold", &temp))
		ps_data.ps_calib_20cm_threshold = (uint16_t)temp;

	if (!of_property_read_u32(dn, "ps_calib_5cm_threshold", &temp))
		ps_data.ps_calib_5cm_threshold = (uint16_t)temp;

	if (!of_property_read_u32(dn, "ps_calib_3cm_threshold", &temp))
		ps_data.ps_calib_3cm_threshold = (uint16_t)temp;

	if (!of_property_read_u32(dn, "wtime", &temp))
		ps_data.wtime = (uint8_t)temp;

	if (!of_property_read_u32(dn, "led_current", &temp))
		ps_data.led_current = (uint8_t)temp;

	if (!of_property_read_u32(dn, "led_limited_curr", &temp))
		ps_data.led_limited_curr = (uint8_t)temp;

	if (!of_property_read_u32(dn, "pd_current", &temp))
		ps_data.pd_current = (uint8_t)temp;

	if (!of_property_read_u32(dn, "pulse_len", &temp))
		ps_data.pulse_len = (uint8_t)temp;

	if (!of_property_read_u32(dn, "pgain", &temp))
		ps_data.pgain = (uint8_t)temp;

	if (!of_property_read_u32(dn, "prox_avg", &temp))
		ps_data.prox_avg = (uint8_t)temp;

	if (!of_property_read_u32(dn, "offset_max", &temp))
		ps_data.offset_max = (uint8_t)temp;

	if (!of_property_read_u32(dn, "offset_min", &temp))
		ps_data.offset_min = (uint8_t)temp;

	if (!of_property_read_u32(dn, "oily_max_near_pdata", &temp))
		ps_data.oily_max_near_pdata = (uint16_t)temp;

	if (!of_property_read_u32(dn, "max_oily_add_pdata", &temp))
		ps_data.max_oily_add_pdata = (uint16_t)temp;

	if (!of_property_read_u32(dn, "max_near_pdata_loop", &temp))
		ps_data.max_near_pdata_loop = (uint8_t)temp;

	if (!of_property_read_u32(dn, "oily_count_size", &temp))
		ps_data.oily_count_size = (uint8_t)temp;

	if (!of_property_read_u32(dn, "ps_tp_threshold", &temp))
		ps_data.ps_tp_threshold = (uint16_t)temp;

	if (!of_property_read_u32(dn, "ps_phone_type", &temp))
		ps_data.ps_phone_type = (uint8_t)temp;

	if (!of_property_read_u32(dn, "ps_phone_version", &temp))
		ps_data.ps_phone_version = (uint8_t)temp;

	if (!of_property_read_u32(dn, "file_id", &temp))
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;
	dyn_req->file_count++;

	read_sensorlist_info(dn, PS);
	select_ps_para(sm);
}

void read_tof_chip_info(enum sensor_detect_list name)
{
	struct tof_device_info *dev_info = NULL;
	char *chip_info = get_sensor_chip_info_address(name);

	if (name == TOF) {
		dev_info = &g_tof_dev_info[0];
	} else if (name == TOF1) {
		dev_info = &g_tof_dev_info[1];
	} else {
		hwlog_err("[%s-%d]:name is not valied name:%d\n", __func__, __LINE__, name);
		return;
	}

	if (strncmp(chip_info, "vi5300,tof1", sizeof("vi5300,tof1")) == 0) {
		dev_info->tof_first_start_flag = 0;
		dev_info->chip_type = TOF_CHIP_VI5300;
		dev_info->tof_enable_flag = 1;
	} else if (strncmp(chip_info, "vi5300,tof", sizeof("vi5300,tof")) == 0) {
		dev_info->tof_first_start_flag = 0;
		dev_info->chip_type = TOF_CHIP_VI5300;
		dev_info->tof_enable_flag = 1;
	} else if (strncmp(chip_info, "vl53l3,tof1", sizeof("vl53l3,tof1")) == 0) {
		dev_info->tof_first_start_flag = 0;
		dev_info->chip_type = TOF_CHIP_VL53L3;
		dev_info->tof_enable_flag = 1;
	} else if (strncmp(chip_info, "vl53l3,tof", sizeof("vl53l3,tof")) == 0) {
		dev_info->tof_first_start_flag = 0;
		dev_info->chip_type = TOF_CHIP_VL53L3;
		dev_info->tof_enable_flag = 1;
	} else {
		hwlog_info("[%s-%d]:tof:%d not find target device info\n",
			__func__, __LINE__, name);
		return;
	}
	hwlog_info("[%s-%d]:tof:%d succ\n", __func__, __LINE__, name);
}

static void read_tof_data_from_dts(struct device_node *dn)
{
	int temp = 0;

	read_chip_info(dn, TOF);
	read_tof_chip_info(TOF);
	if (!strncmp(get_sensor_chip_info_address(TOF), "huawei,ams_tmf8701",
		sizeof("huawei,ams_tmf8701"))) {
		g_sensor_tof_flag = 1;
		hwlog_info("%s:ams_tmf8701 i2c_address suc,%d\n", __func__, temp);
	}
	if (!strncmp(get_sensor_chip_info_address(TOF), "huawei,sharp_gp2ap02",
		sizeof("huawei,sharp_gp2ap02"))) {
		g_sensor_tof_flag = 1;
		hwlog_info("%s:sharp_gp2ap02 i2c_address suc,%d\n", __func__, temp);
	}
	if (!strncmp(get_sensor_chip_info_address(TOF), "vi5300,tof",
		sizeof("vi5300,tof"))) {
		g_sensor_tof_flag = 1;
		hwlog_info("%s:vi5300,tof i2c_address suc,%d\n", __func__, temp);
	}
	if (!strncmp(get_sensor_chip_info_address(TOF), "vl53l3,tof",
		sizeof("vl53l3,tof"))) {
		g_sensor_tof_flag = 1;
		hwlog_info("%s:vl53l3,tof i2c_address suc,%d\n", __func__, temp);
	}

	if (of_property_read_u32(dn, "file_id", &temp))
		hwlog_err("%s:read tof file_id fail\n", __func__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;

	dyn_req->file_count++;

	if (of_property_read_u32(dn, "sensor_list_info_id", &temp))
		hwlog_err("%s:read tof sensor_list_info_id fail\n", __func__);
	else
		add_sensor_list_info_id((uint16_t)temp);
	read_sensorlist_info(dn, TOF);
}

static void read_humiture_data_from_dts(struct device_node *dn)
{
	uint32_t temp = 0;
	hwlog_info("[%s-%d]:enter humiture data from dts\n", __func__, __LINE__);
	read_chip_info(dn, HUMITURE);
	if (!strncmp(get_sensor_chip_info_address(HUMITURE), "huawei,cht8305",
		sizeof("huawei,cht8305")))
		hwlog_info("%s, humiture cht8305 i2c_address success\n", __func__);

	if (of_property_read_u32(dn, "file_id", &temp))
		hwlog_err("%s:read humiture cht8305 file_id fail\n", __func__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;

	dyn_req->file_count++;

	if (of_property_read_u32(dn, "sensor_list_info_id", &temp))
		hwlog_err("%s:read humiture cht8305 sensor_list_info_id fail\n", __func__);
	else
		add_sensor_list_info_id((uint16_t)temp);
	read_sensorlist_info(dn, HUMITURE);
}

static void read_airpress_data_from_dts(struct device_node *dn)
{
	int temp = 0;

	read_chip_info(dn, AIRPRESS);

	if (of_property_read_u32(dn, "poll_interval", &temp))
		hwlog_err("%s:read poll_interval fail\n", __func__);
	else
		airpress_data.poll_interval = (uint16_t)temp;

	if (of_property_read_u32(dn, "reg", &temp))
		hwlog_err("%s:read airpress reg fail\n", __func__);
	else
		airpress_data.cfg.i2c_address = (uint8_t)temp;

	if (of_property_read_u32(dn, "file_id", &temp))
		hwlog_err("%s:read airpress file_id fail\n", __func__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;
	dyn_req->file_count++;

	if (of_property_read_u32(dn, "sensor_list_info_id", &temp))
		hwlog_err("%s:read ps sensor_list_info_id fail\n", __func__);
	else
		add_sensor_list_info_id((uint16_t)temp);
	read_sensorlist_info(dn, AIRPRESS);
}

static void read_gps_4774_i2c_data_from_dts(struct device_node *dn)
{
	int temp = 0;

	read_chip_info(dn, GPS_4774_I2C);

	if (of_property_read_u32(dn, "gpio1_gps_cmd_ap", &temp))
		hwlog_err("%s:read gpio1_gps_cmd_ap fail\n", __func__);
	else
		gps_4774_data.gpio1_gps_cmd_ap = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio1_gps_cmd_sh", &temp))
		hwlog_err("%s:read gpio1_gps_cmd_sh fail\n", __func__);
	else
		gps_4774_data.gpio1_gps_cmd_sh = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio2_gps_ready_ap", &temp))
		hwlog_err("%s:read gpio2_gps_ready_ap fail\n", __func__);
	else
		gps_4774_data.gpio2_gps_ready_ap = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio2_gps_ready_sh", &temp))
		hwlog_err("%s:read gpio2_gps_ready_sh fail\n", __func__);
	else
		gps_4774_data.gpio2_gps_ready_sh = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio3_wakeup_gps_ap", &temp))
		hwlog_err("%s:read gpio3_wakeup_gps_ap fail\n", __func__);
	else
		gps_4774_data.gpio3_wakeup_gps_ap = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio3_wakeup_gps_sh", &temp))
		hwlog_err("%s:read gpio3_wakeup_gps_sh fail\n", __func__);
	else
		gps_4774_data.gpio3_wakeup_gps_sh = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "file_id", &temp))
		hwlog_err("%s:read gps_4774_i2c file_id fail\n", __func__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;

	dyn_req->file_count++;

	hwlog_err("gps 4774 i2c file id is %d\n", temp);
	if (of_property_read_u32(dn, "sensor_list_info_id", &temp))
		hwlog_err("%s:read gps 4774 sensor_list_info_id fail\n", __func__);
	else
		add_sensor_list_info_id((uint16_t)temp);
}

static void read_fingerprint_from_dts(struct device_node *dn)
{
	int temp = 0;

	read_chip_info(dn, FINGERPRINT);

	if (of_property_read_u32(dn, "file_id", &temp))
		hwlog_err("%s:read fingerprint file_id fail\n", __func__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;
	dyn_req->file_count++;
	hwlog_err("fingerprint file id is %d\n", temp);

	if (of_property_read_u32(dn, "chip_id_register", &temp))
		hwlog_err("%s:read chip_id_register fail\n", __func__);
	else
		fingerprint_data.reg = (uint16_t)temp;

	if (of_property_read_u32(dn, "chip_id_value", &temp))
		hwlog_err("%s:read chip_id_value fail\n", __func__);
	else
		fingerprint_data.chip_id = (uint16_t)temp;

	if (of_property_read_u32(dn, "product_id_value", &temp))
		hwlog_err("%s:read product_id_value fail\n", __func__);
	else
		fingerprint_data.product_id = (uint16_t)temp;

	if (of_property_read_u32(dn, "gpio_irq", &temp))
		hwlog_err("%s:read gpio_irq fail\n", __func__);
	else
		fingerprint_data.gpio_irq = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio_irq_sh", &temp))
		hwlog_err("%s:read gpio_irq_sh fail\n", __func__);
	else
		fingerprint_data.gpio_irq_sh = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio_reset", &temp))
		hwlog_err("%s:read gpio_reset fail\n", __func__);
	else
		fingerprint_data.gpio_reset = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio_reset_sh", &temp))
		hwlog_err("%s:read gpio_reset_sh fail\n", __func__);
	else
		fingerprint_data.gpio_reset_sh = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio_cs", &temp))
		hwlog_err("%s:read gpio_cs fail\n", __func__);
	else
		fingerprint_data.gpio_cs = (GPIO_NUM_TYPE)temp;
}

static void read_fingerprint_ud_from_dts(struct device_node *dn)
{
	int temp = 0;

	read_chip_info(dn, FINGERPRINT_UD);

	if (of_property_read_u32(dn, "file_id", &temp))
		hwlog_err("%s:read fingerprint file_id fail\n", __func__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;
	dyn_req->file_count++;
	hwlog_err("fingerprint file id is %d\n", temp);

	if (of_property_read_u32(dn, "chip_id_register", &temp))
		hwlog_err("%s:read chip_id_register fail\n", __func__);
	else
		fingerprint_ud_data.reg = (uint16_t)temp;

	if (of_property_read_u32(dn, "chip_id_value", &temp))
		hwlog_err("%s:read chip_id_value fail\n", __func__);
	else
		fingerprint_ud_data.chip_id = (uint16_t)temp;

	if (of_property_read_u32(dn, "product_id_value", &temp))
		hwlog_err("%s:read product_id_value fail\n", __func__);
	else
		fingerprint_ud_data.product_id = (uint16_t)temp;

	if (of_property_read_u32(dn, "gpio_irq", &temp))
		hwlog_err("%s:read gpio_irq fail\n", __func__);
	else
		fingerprint_ud_data.gpio_irq = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio_irq_sh", &temp))
		hwlog_err("%s:read gpio_irq_sh fail\n", __func__);
	else
		fingerprint_ud_data.gpio_irq_sh = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio_reset", &temp))
		hwlog_err("%s:read gpio_reset fail\n", __func__);
	else
		fingerprint_ud_data.gpio_reset = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio_cs", &temp))
		hwlog_err("%s:read gpio_cs fail\n", __func__);
	else
		fingerprint_ud_data.gpio_cs = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "tp_hover_support", &temp))
		hwlog_warn("%s:read tp_hover_support fail\n", __func__);
	else
		fingerprint_ud_data.tp_hover_support = (uint16_t)temp;
}

static void read_key_i2c_data_from_dts(struct device_node *dn)
{
	read_chip_info(dn, KEY);
	key_data.cfg.i2c_address = 0x27;
	dyn_req->file_list[dyn_req->file_count] = 59; /* key file_id 59 */
	dyn_req->file_count++;

	hwlog_info("key read dts\n");
}

static void read_aod_data_from_dts(struct device_node *dn)
{
	int i;
	int len = of_property_count_u32_elems(dn, "feature");

	read_chip_info(dn, SH_AOD);

	if (len <= 0) {
		hwlog_warn("%s:count u32 data fail\n", __func__);
		return;
	}

	if (of_property_read_u32_array(dn, "feature", aod_data.feature_set, len))
		hwlog_err("%s:read chip_id_value fail\n", __func__);

	for (i = 0; i < len; i++)
		hwlog_info("aod_data.feature_set[%d]= 0x%x\n",
			i, aod_data.feature_set[i]);
}
static void read_magn_bracket_data_from_dts(struct device_node *dn)
{
	int temp = 0;
	u32 wia[6] = {0};
	struct property *prop = NULL;
	unsigned int len;

	read_chip_info(dn, MAGN_BRACKET);

	prop = of_find_property(dn, "mag_axis_change", NULL);
	if (!prop) {
		hwlog_err("%s! prop is NULL!\n", __func__);
		return;
	}
	if (!prop->value) {
		hwlog_err("%s! prop->value is NULL!\n", __func__);
		return;
	}
	len = prop->length / 4; /* 4: to word */
	if (of_property_read_u32_array(dn, "mag_axis_change", wia, len)) {
		hwlog_err("%s:read mag_axis_change from dts fail!\n", __func__);
		return;
	}

	magn_bracket_data.mag_x_change_lower = (int)wia[0];
	magn_bracket_data.mag_x_change_upper = (int)wia[1];
	magn_bracket_data.mag_y_change_lower = (int)wia[2];
	magn_bracket_data.mag_y_change_upper = (int)wia[3];
	magn_bracket_data.mag_z_change_lower = (int)wia[4];
	magn_bracket_data.mag_z_change_upper = (int)wia[5];

	hwlog_info("%s: %d, %d, %d, %d, %d, %d\n", __func__,
		magn_bracket_data.mag_x_change_lower, magn_bracket_data.mag_x_change_upper,
		magn_bracket_data.mag_y_change_lower, magn_bracket_data.mag_y_change_upper,
		magn_bracket_data.mag_z_change_lower, magn_bracket_data.mag_z_change_upper);

	if (of_property_read_u32(dn, "file_id", &temp))
		hwlog_err("%s:read magn_bracket file_id fail\n", __func__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;
	dyn_req->file_count++;

	if (of_property_read_u32(dn, "sensor_list_info_id", &temp))
		hwlog_err("%s:read magn_bracket sensor_list_info_id fail\n", __func__);
	else
		add_sensor_list_info_id((uint16_t)temp);

	read_sensorlist_info(dn, MAGN_BRACKET);
}

static int read_rpc_data_from_dts(struct device_node *dn)
{
	int t = 0;
	int *temp = &t;
	unsigned int i;
	u32 wia[32] = { 0 }; /* count 32 */
	struct property *prop = NULL;
	unsigned int len;

	memset(&rpc_data, 0, sizeof(rpc_data));
	read_chip_info(dn, RPC);
	prop = of_find_property(dn, "table", NULL);
	if (!prop) {
		hwlog_err("%s! prop is NULL!\n", __func__);
		return -1;
	}

	len = (u32)(prop->length) / sizeof(u32);
	if (of_property_read_u32_array(dn, "table", wia, len)) {
		hwlog_err("%s:read rpc_table from dts fail!\n", __func__);
		return -1;
	}
	for (i = 0; i < len; i++)
		rpc_data.table[i] = (u16)wia[i];

	memset(wia, 0, sizeof(wia));
	prop = of_find_property(dn, "mask", NULL);
	if (!prop) {
		hwlog_err("%s! prop is NULL!\n", __func__);
		return -1;
	}

	len = (u32)(prop->length) / sizeof(u32);
	if (of_property_read_u32_array(dn, "mask", wia, len)) {
		hwlog_err("%s:read rpc_mask from dts fail!\n", __func__);
		return -1;
	}
	for (i = 0; i < len; i++)
		rpc_data.mask[i] = (u16)wia[i];
	if (of_property_read_u32(dn, "file_id", (u32 *)temp))
		hwlog_err("%s:read rpc file_id fail\n", __func__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)t;

	dyn_req->file_count++;

	if (of_property_read_u32(dn, "sensor_list_info_id", (u32 *)temp))
		hwlog_err("%s:read rpc sensor_list_info_id fail\n", __func__);
	else
		add_sensor_list_info_id((uint16_t)t);

	if (of_property_read_u32(dn, "default_value", (u32 *)temp))
		hwlog_err("%s:read default_value fail\n", __func__);
	else
		rpc_data.default_value = (uint16_t)t;

	read_sensorlist_info(dn, RPC);
	return 0;
}
static int get_adapt_file_id_for_dyn_load(void)
{
	u32 wia[ADAPT_SENSOR_LIST_NUM] = {0};
	struct property *prop = NULL;
	unsigned int i;
	unsigned int len;
	struct device_node *sensorhub_node = NULL;
	const char *name = "adapt_file_id";
	char *step_count_ty = NULL;

	sensorhub_node = of_find_compatible_node(NULL, NULL, "huawei,sensorhub");
	if (!sensorhub_node) {
		hwlog_err("%s, can't find node sensorhub\n", __func__);
		return -1;
	}
	prop = of_find_property(sensorhub_node, name, NULL);
	if (!prop) {
		hwlog_err("%s! prop is NULL!\n", __func__);
		return -EINVAL;
	}
	if (!prop->value) {
		hwlog_err("%s! prop->value is NULL!\n", __func__);
		return -ENODATA;
	}
	len = prop->length / 4; /* 4: to word */
	if (of_property_read_u32_array(sensorhub_node, name, wia, len)) {
		hwlog_err("%s:read adapt_file_id from dts fail!\n", name);
		return -1;
	}
	for (i = 0; i < len; i++) {
		dyn_req->file_list[dyn_req->file_count] = wia[i];
		dyn_req->file_count++;
	}
	/* find hifi supported or not */
	if (of_property_read_u32(sensorhub_node, "hifi_support", &i) == 0) {
		if (i == 1) {
			g_hifi_supported = 1;
			hwlog_info("sensor get hifi support %d\n", i);
		}
	}
	if (of_property_read_string(sensorhub_node, "docom_step_counter",
		(const char **)&step_count_ty) == 0) {
		if (!strcmp("enabled", step_count_ty)) {
			g_p_config_on_ddr->reserved = 1;
			hwlog_info("%s:docom_step_counter status is %s\n",
				__func__, step_count_ty);
		}
	}
	if (of_property_read_u32(sensorhub_node, "is_support_hall_hishow", &i) == 0) {
		if (i == 1) {
			support_hall_hishow = 1;
			hwlog_info("sensor get support_hall_hishow: %d\n", support_hall_hishow);
		}
	}
	return 0;
}

static int get_hall_config_from_dts(void)
{
	unsigned int i = 0;
	struct device_node *sensorhub_node = NULL;

	sensorhub_node = of_find_compatible_node(NULL, NULL, "huawei,sensorhub");
	if (!sensorhub_node) {
		hwlog_err("%s, can't find node sensorhub\n", __func__);
		return -1; /* get huawei sensorhub node fail */
	}

	/* find number of the hall sensor */
	if (!of_property_read_u32(sensorhub_node, "hall_number", &i)) {
		g_hall_number = i;
		hwlog_info("sensor get hall number %d\n", g_hall_number);
	}

	return 0;
}

static void swap1(uint16_t *left, uint16_t *right)
{
	uint16_t temp;

	temp = *left;
	*left = *right;
	*right = temp;
}

/* delete the repeated file id by map table */
static uint8_t check_file_list(uint8_t file_count, uint16_t *file_list)
{
	int i;
	int j;
	int k;

	if ((file_count == 0) || (file_list == NULL)) {
		hwlog_err("%s, val invalid\n", __func__);
		return 0;
	}

	for (i = 0; i < file_count; i++) {
		for (j = i + 1; j < file_count; j++) {
			if (file_list[i] != file_list[j])
				continue;
			file_count -= 1;
			for (k = j; k < file_count; k++)
				file_list[k] = file_list[k + 1];
			j -= 1;
		}
	}

	for (i = 0; i < file_count; i++) {
		for (j = 1; j < file_count - i; j++) {
			if (file_list[j - 1] > file_list[j])
				swap1(&file_list[j - 1], &file_list[j]);
		}
	}
	return file_count;
}

static int get_adapt_sensor_list_id(void)
{
	u32 wia[ADAPT_SENSOR_LIST_NUM] = {0};
	struct property *prop = NULL;
	unsigned int i;
	unsigned int len;
	struct device_node *sensorhub_node = NULL;
	const char *name = "adapt_sensor_list_id";

	sensorhub_node = of_find_compatible_node(NULL, NULL, "huawei,sensorhub");
	if (!sensorhub_node) {
		hwlog_err("%s, can't find node sensorhub\n", __func__);
		return -1;
	}
	prop = of_find_property(sensorhub_node, name, NULL);
	if (!prop) {
		hwlog_err("%s! prop is NULL!\n", __func__);
		return -EINVAL;
	}
	if (!prop->value) {
		hwlog_err("%s! prop->value is NULL!\n", __func__);
		return -ENODATA;
	}
	len = prop->length / 4; /* 4: to word */
	if (of_property_read_u32_array(sensorhub_node, name, wia, len)) {
		hwlog_err("%s:read adapt_sensor_list_id from dts fail!\n", name);
		return -1;
	}
	for (i = 0; i < len; i++)
		add_sensor_list_info_id((uint16_t)wia[i]);

	return 0;
}

int send_fileid_to_mcu(void)
{
	struct write_info pkg_ap;
	struct read_info pkg_mcu;
	int ret;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));
	dyn_req->end = 1;
	pkg_ap.tag = TAG_SYS;
	pkg_ap.cmd = CMD_SYS_DYNLOAD_REQ;
	pkg_ap.wr_buf = &(dyn_req->end);
	pkg_ap.wr_len = dyn_req->file_count * sizeof(dyn_req->file_list[0])
		+ sizeof(dyn_req->end) + sizeof(dyn_req->file_count);

	if ((get_iom3_state() == IOM3_ST_RECOVERY) ||
		(get_iomcu_power_state() == ST_SLEEP))
		ret = write_customize_cmd(&pkg_ap, NULL, false);
	else
		ret = write_customize_cmd(&pkg_ap, &pkg_mcu, true);

	if (ret) {
		hwlog_err("send file id to mcu fail,ret %d\n", ret);
		return -1;
	}
	if (pkg_mcu.errno != 0) {
		hwlog_err("file id set fail\n");
		return -1;
	}

	return 0;
}

static int get_adapt_id_and_send(void)
{
	int ret;
	int i;

	ret = get_adapt_file_id_for_dyn_load();
	if (ret < 0)
		hwlog_err("get_adapt_file_id_for_dyn_load() failed!\n");

	hwlog_info("get file id number = %d\n", dyn_req->file_count);

	ret =  get_hall_config_from_dts();
	if (ret < 0)
		hwlog_err("get_hall_config_from_dts() failed!\n");

	ret = get_adapt_sensor_list_id();
	if (ret < 0)
		hwlog_err("get_adapt_sensor_list_id() failed!\n");

	sensorlist[0] = check_file_list(sensorlist[0], &sensorlist[1]);
	if (sensorlist[0] > 0) {
		hwlog_info("sensorhub after check, get sensor list id number = %d, list id: ", sensorlist[0]);
		for (i = 0; i < sensorlist[0]; i++)
			hwlog_info("--%d", sensorlist[i + 1]);
		hwlog_info("\n");
	} else {
		hwlog_err("%s list num = 0, not send file_id to muc\n", __func__);
		return -EINVAL;
	}
	dyn_req->file_count = check_file_list(dyn_req->file_count, dyn_req->file_list);

	if (dyn_req->file_count) {
		hwlog_info("sensorhub after check, get dynload file id number = %d, fild id", dyn_req->file_count);
		for (i = 0; i < dyn_req->file_count; i++)
			hwlog_info("--%d", dyn_req->file_list[i]);

		hwlog_info("\n");
		return send_fileid_to_mcu();
	}
	hwlog_err("%s file_count = 0, not send file_id to mcu\n", __func__);
	return -EINVAL;
}

static int get_key_chip_type(struct device_node *dn)
{
	int ret;
	int ctype = 0;

	ret = of_property_read_u32(dn, "chip_type", &ctype);
	if (ret < 0)
		hwlog_err("read chip type err. ret:%d\n", ret);
	return ctype;
}

static int key_sensor_detect(struct device_node *dn)
{
	int ret;
	int reg = 0;
	int bootloader_reg = 0;
	uint8_t values[12] = {0};
	int chip_id_register;

	ret = of_property_read_u32(dn, "reg", &reg);
	if (ret < 0) {
		hwlog_err("read reg err. ret:%d\n", ret);
		return -1;
	}
	ret = of_property_read_u32(dn, "reg_bootloader", &bootloader_reg);
	if (ret < 0) {
		hwlog_err("read reg_bootloader err. ret:%d\n", ret);
		return -1;
	}
	hwlog_info("[%s] debug key reg:%d, btld reg:%d\n", __func__, reg, bootloader_reg);
	msleep(50); /* sleep 50 ms */
	ret = mcu_i2c_rw(0, (uint8_t)bootloader_reg, NULL, 0, values,
		(uint32_t)(sizeof(values) / sizeof(values[0])));
	if (ret < 0) {
		hwlog_info("[%s][28] %d %d %d %d %d %d %d %d\n", __func__,
			values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7]);
		msleep(10); /* sleep 10 ms */
		chip_id_register = 0x46;
		ret = mcu_i2c_rw(0, (uint8_t)reg, (uint8_t*)&chip_id_register,
			1, values, 2); /* 2 bytes */
		if (ret < 0) {
			hwlog_err("i2c 27 read err\n");
			return -1;
		}
	}

	hwlog_info("[%s][28] %d %d %d %d %d %d %d %d\n", __func__,
		values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7]);
	return 0;
}

static int inputhub_key_sensor_detect(struct device_node *dn,
	struct sensor_detect_manager *sm, int index)
{
	int ret;
	int chip_type;
	struct sensor_detect_manager *p = sm + index;
	struct sensor_combo_cfg cfg = { 0 };

	chip_type = get_key_chip_type(dn);
	if (chip_type)
		return key_sensor_detect(dn);

	ret = _device_detect(dn, index, &cfg);
	if (!ret) {
		if (memcpy_s((void *)p->spara, sizeof(struct sensor_combo_cfg),
			(void *)&cfg, sizeof(struct sensor_combo_cfg)) != EOK)
			ret = RET_FAIL;
	}
	return ret;
}

#define FINGERPRINT_SENSOR_DETECT_SUCCESS 10
#define FINGERPRINT_WRITE_CMD_LEN 7
#define FINGERPRINT_READ_CONTENT_LEN 2

static int fingerprint_get_gpio_config(struct device_node *dn,
	GPIO_NUM_TYPE *gpio_reset, GPIO_NUM_TYPE *gpio_cs,
	GPIO_NUM_TYPE *gpio_irq)
{
	int temp = 0;
	int ret = RET_SUCC;

	if (of_property_read_u32(dn, "gpio_reset", &temp)) {
		hwlog_err("%s:read gpio_reset fail\n", __func__);
		ret = RET_FAIL;
	} else {
		*gpio_reset = (GPIO_NUM_TYPE)temp;
	}

	if (of_property_read_u32(dn, "gpio_cs", &temp)) {
		hwlog_err("%s:read gpio_cs fail\n", __func__);
		ret = RET_FAIL;
	} else {
		*gpio_cs = (GPIO_NUM_TYPE)temp;
	}

	if (of_property_read_u32(dn, "gpio_irq", &temp)) {
		hwlog_err("%s:read gpio_irq fail\n", __func__);
		ret = RET_FAIL;
	} else {
		*gpio_irq = (GPIO_NUM_TYPE)temp;
	}

	return ret;
}

static void fingerprint_gpio_reset(GPIO_NUM_TYPE gpio_reset,
	unsigned int first_sleep, unsigned int second_sleep)
{
	gpio_direction_output(gpio_reset, GPIO_STAT_HIGH);
	msleep(first_sleep);
	gpio_direction_output(gpio_reset, GPIO_STAT_LOW);
	msleep(second_sleep);
	gpio_direction_output(gpio_reset, GPIO_STAT_HIGH);
}

static void set_fpc_config(GPIO_NUM_TYPE gpio_reset)
{
	gpio_direction_output(gpio_reset, GPIO_STAT_HIGH);
	msleep(RESET_LONG_SLEEP);
}

static void set_syna_config(GPIO_NUM_TYPE gpio_reset, GPIO_NUM_TYPE gpio_cs)
{
	union spi_ctrl ctrl;
	uint8_t tx[2] = {0}; /* 2 : tx register length */
	uint32_t tx_len;

	fingerprint_gpio_reset(gpio_reset, RESET_LONG_SLEEP, RESET_LONG_SLEEP);

	/* send 2 byte 0xF0 cmd to software reset sensor */
	ctrl.data = gpio_cs;
	tx[0] = 0xF0;
	tx[1] = 0xF0;
	tx_len = 2; /* 2 bytes */
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);
	msleep(100); /* 100 : sleep time after soft reset */
}

static void set_goodix_config(GPIO_NUM_TYPE gpio_reset, GPIO_NUM_TYPE gpio_cs)
{
	uint8_t tx[FINGERPRINT_WRITE_CMD_LEN] = {0};
	uint8_t rx[FINGERPRINT_READ_CONTENT_LEN] = {0};
	uint32_t tx_len;
	uint32_t rx_len;
	union spi_ctrl ctrl;

	fingerprint_gpio_reset(gpio_reset, RESET_LONG_SLEEP, RESET_LONG_SLEEP);

	/* set sensor to idle mode, cmd is 0xC0, lenth is 1 */
	ctrl.data = gpio_cs;
	tx[0] = 0xC0;
	tx_len = 1;
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);

	/* write cmd 0xF0 & address 0x0126, length is 3 */
	tx[0] = 0xF0;
	tx[1] = 0x01;
	tx[2] = 0x26;
	tx_len = 3; /* 3 bytes */
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);

	/* read cmd 0xF1, cmd length is 1, rx length is 2 */
	tx[0] = 0xF1;
	tx_len = 1;
	rx_len = 2; /* 2 bytes */
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, rx, rx_len);

	/* write cmd 0xF0 & address 0x0124 and 0x0001 */
	/* clear irq, tx length is 7 */
	if ((rx[0] != 0x00) || (rx[1] != 0x00)) {
		tx[0] = 0xF0;
		tx[1] = 0x01;
		tx[2] = 0x24;
		tx[3] = 0x00;
		tx[4] = 0x01;
		tx[5] = rx[0];
		tx[6] = rx[1];
		tx_len = 7; /* 7 bytes */
		mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);
	}

	/* set sensor to idle mode, cmd is 0xC0, lenth is 1 */
	tx[0] = 0xC0;
	tx_len = 1;
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);

	/* write cmd 0xF0 & address 0x0000, length is 3 */
	tx[0] = 0xF0;
	tx[1] = 0x00;
	tx[2] = 0x00;
	tx_len = 3; /* 3 bytes */
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);
}

static void set_silead_config(GPIO_NUM_TYPE gpio_reset)
{
	fingerprint_gpio_reset(gpio_reset, RESET_LONG_SLEEP, RESET_LONG_SLEEP);
	msleep(RESET_SHORT_SLEEP);
}

static int fingerprint_sensor_detect(struct device_node *dn, int index,
	struct sensor_combo_cfg *cfg)
{
	int ret;
	int irq_value = 0;
	int reset_value = 0;
	char *sensor_vendor = NULL;
	GPIO_NUM_TYPE gpio_reset = 0;
	GPIO_NUM_TYPE gpio_cs = 0;
	GPIO_NUM_TYPE gpio_irq = 0;

	if (fingerprint_get_gpio_config(dn, &gpio_reset,
		&gpio_cs, &gpio_irq) != 0)
		hwlog_err("%s:read fingerprint gpio fail\n", __func__);

	if (sensor_manager[index].detect_result == DET_FAIL) {
		reset_value = gpio_get_value(gpio_reset);
		irq_value = gpio_get_value(gpio_irq);
	}

	ret = of_property_read_string(dn, "compatible",
		(const char **)&sensor_vendor);
	if (!ret) {
		if (!strncmp(sensor_vendor, "fpc", FPC_NAME_LEN)) {
			set_fpc_config(gpio_reset);
		} else if (!strncmp(sensor_vendor, "syna", SYNA_NAME_LEN)) {
			set_syna_config(gpio_reset, gpio_cs);
		} else if (!strncmp(sensor_vendor, "goodix", GOODIX_NAME_LEN)) {
			set_goodix_config(gpio_reset, gpio_cs);
		} else if (!strncmp(sensor_vendor, "silead", SILEAD_NAME_LEN)) {
			set_silead_config(gpio_reset);
		} else if (!strncmp(sensor_vendor, "qfp", QFP_NAME_LEN)) {
			if (_device_detect(dn, index, cfg))
				hwlog_info("%s: qfp err\n", __func__);
			hwlog_info("%s: fingerprint device %s detect bypass\n",
				__func__, sensor_vendor);
			return FINGERPRINT_SENSOR_DETECT_SUCCESS;
		}
		hwlog_info("%s: fingerprint device %s\n", __func__,
			sensor_vendor);
	} else {
		hwlog_err("%s: get sensor_vendor err\n", __func__);
		ret = RET_FAIL;
	}

	if (sensor_manager[index].detect_result == DET_FAIL) {
		if (irq_value == GPIO_STAT_HIGH) {
			gpio_direction_output(gpio_reset, reset_value);
			gpio_direction_output(gpio_irq, irq_value);
		}
		hwlog_info("%s: irq_value = %d, reset_value = %d\n",
			__func__, irq_value, reset_value);
	}

	return ret;
}

static int inputhub_fingerprint_sensor_detect(struct device_node *dn,
	struct sensor_detect_manager *sm, int index)
{
	int ret;
	struct sensor_detect_manager *p = NULL;
	struct sensor_combo_cfg cfg = { 0 };

	if (!dn || !sm)
		return -1;

	p = sm + index;
	ret = fingerprint_sensor_detect(dn, index, &cfg);
	if (ret == FINGERPRINT_SENSOR_DETECT_SUCCESS) {
		ret = 0;
		if (memcpy_s((void *)p->spara, sizeof(struct sensor_combo_cfg),
			(void *)&cfg, sizeof(struct sensor_combo_cfg)) != EOK)
			ret = RET_FAIL;
		return ret;
	} else if (ret) {
		return ret;
	}

	ret = _device_detect(dn, index, &cfg);
	if (!ret) {
		if (memcpy_s((void *)p->spara, sizeof(struct sensor_combo_cfg),
			(void *)&cfg, sizeof(struct sensor_combo_cfg)) != EOK)
			ret = RET_FAIL;
	}
	return ret;
}

static void set_goodix_ud_config(GPIO_NUM_TYPE gpio_reset,
	GPIO_NUM_TYPE gpio_cs)
{
	uint8_t tx[3] = {0}; /* 3 : tx register length */
	uint32_t tx_len;
	union spi_ctrl ctrl;

	fingerprint_gpio_reset(gpio_reset, RESET_LONG_SLEEP, RESET_LONG_SLEEP);

	ctrl.data = gpio_cs;

	/* set sensor to idle mode, cmd is 0xC0, lenth is 1 */
	tx[0] = 0xC0;
	tx_len = 1;
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);
	msleep(100); /* 100 : sleep 100ms after set sensor to idel */

	/* write cmd 0xF0 & address 0x0004, length is 3 */
	tx[0] = 0xF0;
	tx[1] = 0x00;
	tx[2] = 0x04;
	tx_len = 3; /* 3 bytes */
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);
}

static void set_goodix_udg2_config(GPIO_NUM_TYPE gpio_reset,
	GPIO_NUM_TYPE gpio_cs)
{
	uint8_t tx[3] = {0}; /* 3 : tx register length */
	uint32_t tx_len;
	union spi_ctrl ctrl;

	fingerprint_gpio_reset(gpio_reset, RESET_LONG_SLEEP, RESET_LONG_SLEEP);

	msleep(RESET_LONG_SLEEP);
	ctrl.data = gpio_cs;

	tx[0] = GOODIX_IDLE_MODE;
	tx_len = GOODIX_IDLE_MODE_TX_LEN;
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);

	msleep(GOODIX_SENSOR_SLEEP);
	tx[0] = GOODIX_WRITE_CMD;
	tx[1] = GOODIX_G2_CHIP_ID_ADDR_HIGH;
	tx[2] = GOODIX_G2_CHIP_ID_ADDR_LOW;
	tx_len = GOODIX_WRITE_CMD_TX_LEN;
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);
}

static void set_goodix_udg3_config(GPIO_NUM_TYPE gpio_reset,
	GPIO_NUM_TYPE gpio_cs)
{
	uint8_t tx[GOODIX_WRITE_WAKEUP_CMD_TX_LEN] = {0};
	uint32_t i;
	uint32_t count = 0;
	const uint8_t sleep_num = 3;
	union spi_ctrl ctrl;
	uint16_t wakeup_addr[] = {
		0xE500, 0x00E0, 0xE600, 0x00E0, 0x00E2, 0x00E2, 0x00E0
	};
	uint16_t wakeup_val[] = {
		0x0000, 0x0150, 0x0000, 0x0010, 0x00A8, 0x0028, 0x0000
	};

	fingerprint_gpio_reset(gpio_reset, RESET_LONG_SLEEP, RESET_LONG_SLEEP);
	msleep(RESET_LONG_SLEEP);

	for (i = 0; i < GOODIX_WRITE_WAKEUP_CMD_TX_LEN; i++) {
		tx[count] = GOODIX_WRITE_CMD;
		tx[++count] = ((wakeup_addr[i] >> BYTE_SHIFT) & BYTE_MASK);
		tx[++count] = (wakeup_addr[i] & BYTE_MASK);
		tx[++count] = WORD_LEN_HIGH;
		tx[++count] = WORD_LEN_LOW;
		tx[++count] = ((wakeup_val[i] >> BYTE_SHIFT) & BYTE_MASK);
		tx[++count] = (wakeup_val[i] & BYTE_MASK);
		count = 0;
		gpio_direction_output(gpio_cs, GPIO_STAT_LOW);
		mcu_spi_rw(FP_SPI_NUM, ctrl, tx, GOODIX_WRITE_WAKEUP_CMD_TX_LEN, NULL, 0);
		gpio_direction_output(gpio_cs, GPIO_STAT_HIGH);
		if (i < sleep_num)
			msleep(IMG_START_SLEEP);
	}

	msleep(GOODIX_SENSOR_SLEEP);
	tx[0] = GOODIX_WRITE_CMD;
	tx[1] = GOODIX_G3_CHIP_ID_ADDR_HIGH;
	tx[2] = GOODIX_G3_CHIP_ID_ADDR_LOW;
	gpio_direction_output(gpio_cs, GPIO_STAT_LOW);
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, GOODIX_WRITE_CMD_TX_LEN, NULL, 0);
	gpio_direction_output(gpio_cs, GPIO_STAT_HIGH);
}

static void set_egis_ud_config(GPIO_NUM_TYPE gpio_reset)
{
	fingerprint_gpio_reset(gpio_reset, RESET_SHORT_SLEEP, RESET_LONG_SLEEP);
	msleep(RESET_LONG_SLEEP);
}

static int fingerprint_ud_sensor_detect(struct device_node *dn, int index,
	struct sensor_combo_cfg *cfg)
{
	int ret;
	int irq_value = 0;
	int reset_value = 0;
	char *sensor_vendor = NULL;
	GPIO_NUM_TYPE gpio_reset = 0;
	GPIO_NUM_TYPE gpio_cs = 0;
	GPIO_NUM_TYPE gpio_irq = 0;

	if (fingerprint_get_gpio_config(dn, &gpio_reset,
		&gpio_cs, &gpio_irq) != 0)
		hwlog_err("%s:read fingerprint gpio fail\n", __func__);

	if (sensor_manager[index].detect_result == DET_FAIL) {
		reset_value = gpio_get_value(gpio_reset);
		irq_value = gpio_get_value(gpio_irq);
	}

	ret = of_property_read_string(dn,
		"compatible", (const char **)&sensor_vendor);
	if (!ret) {
		if (!strncmp(sensor_vendor, "goodix,goodixG2",
			GOODIX_G_NAME_LEN)) {
			set_goodix_udg2_config(gpio_reset, gpio_cs);
		} else if (!strncmp(sensor_vendor, "goodix,goodixG3",
			GOODIX_G_NAME_LEN)) {
			set_goodix_udg3_config(gpio_reset, gpio_cs);
		} else if (!strncmp(sensor_vendor, "goodix", GOODIX_NAME_LEN)) {
			set_goodix_ud_config(gpio_reset, gpio_cs);
		} else if (!strncmp(sensor_vendor, "qfp", QFP_NAME_LEN)) {
			if (_device_detect(dn, index, cfg))
				hwlog_info("%s: qfp err\n", __func__);
			hwlog_info("%s: fingerprint device %s detect bypass\n",
				__func__, sensor_vendor);
			return FINGERPRINT_SENSOR_DETECT_SUCCESS;
		} else if (!strncmp(sensor_vendor, "silead", SILEAD_NAME_LEN)) {
			set_silead_config(gpio_reset);
		} else if (!strncmp(sensor_vendor, "egis", EGIS_NAME_LEN)) {
			set_egis_ud_config(gpio_reset);
		}
		hwlog_info("%s: fingerprint device %s\n",
			__func__, sensor_vendor);
	} else {
		hwlog_err("%s: get sensor_vendor err\n", __func__);
		ret = RET_FAIL;
	}

	if (sensor_manager[index].detect_result == DET_FAIL) {
		if (irq_value == GPIO_STAT_HIGH) {
			gpio_direction_output(gpio_reset, reset_value);
			gpio_direction_output(gpio_irq, irq_value);
		}
		hwlog_info("%s: irq_value = %d, reset_value = %d\n",
			__func__, irq_value, reset_value);
	}

	return ret;
}

static int inputhub_fingerprint_ud_sensor_detect(struct device_node *dn,
	struct sensor_detect_manager *sm, int index)
{
	int ret;
	struct sensor_detect_manager *p = NULL;
	struct sensor_combo_cfg cfg = { 0 };

	if (!dn || !sm)
		return -1;

	p = sm + index;
	ret = fingerprint_ud_sensor_detect(dn, index, &cfg);
	if (ret == FINGERPRINT_SENSOR_DETECT_SUCCESS) {
		ret = 0;
		if (memcpy_s((void *)p->spara, sizeof(struct sensor_combo_cfg),
			(void *)&cfg, sizeof(struct sensor_combo_cfg)) != EOK)
			ret = RET_FAIL;
		return ret;
	} else if (ret) {
		return ret;
	}

	ret = _device_detect(dn, index, &cfg);
	if (!ret) {
		if (memcpy_s((void *)p->spara, sizeof(struct sensor_combo_cfg),
			(void *)&cfg, sizeof(struct sensor_combo_cfg)) != EOK)
			ret = RET_FAIL;
	}
	return ret;
}

static int ps_sensor_detect(struct device_node *dn,
	struct sensor_detect_manager *sm, int index)
{
	int ret;
	struct sensor_detect_manager *p = NULL;
	struct sensor_combo_cfg cfg = { 0 };
#ifdef CONFIG_HUAWEI_THP
	t_ap_sensor_ops_record *record = get_all_ap_sensor_operations();
#endif

	ret = of_property_read_u32(dn, "replace_by_tof", &tof_replace_ps_flag);
	if (tof_replace_ps_flag) {
		hwlog_info("get replace_by_tof flag %d, skip detect\n",
			tof_replace_ps_flag);
		return ret;
	}
	ret = of_property_read_u32(dn, "replace_by_who", &replace_ps_type);
	if (ret) {
		hwlog_info("get replace_by_who failed, use defalut value\n");
		replace_ps_type = 0;
	} else {
		hwlog_info("get replace_by_who successful, replace_ps_type %d\n", replace_ps_type);
	}
	if (replace_ps_type == TP_REPLACE_PS) {
		hwlog_info("get replace_by_who flag %d, skip detect\n", replace_ps_type);
#ifdef CONFIG_HUAWEI_THP
		record[TAG_PS].work_on_ap = true;
		record[TAG_PS].ops.enable = thp_set_prox_switch_status;
		ret = is_tp_detected();
		return ret;
#endif
	}

	ret = _device_detect(dn, index, &cfg);
	if (!ret) {
		p = sm + index;
		if (memcpy_s((void *)p->spara, sizeof(struct sensor_combo_cfg),
			(void *)&cfg, sizeof(struct sensor_combo_cfg)) != EOK)
			ret = RET_FAIL;
	}
	return ret;
}

__weak const char *thp_get_vendor_name(void)
{
	return NULL;
}

__weak const int get_thp_unregister_ic_num(void)
{
	return 0;
}

static int tp_ud_sensor_detect(struct device_node *dn)
{
	int ret;
	const char *configed_vendor_name = NULL;
	const char *attached_vendor_name = NULL;
	const char *bus_type = NULL;
	uint32_t temp = 0;
	int retry_count = 400;
	int unregister_ic_num;

	ret = of_property_read_string(dn, "vendor_name", &configed_vendor_name);
	if (ret) {
		hwlog_err("%s:ic name not configed\n", __func__);
		return ret;
	}

	ret = of_property_read_string(dn, "bus_type", &bus_type);
	if (ret) {
		hwlog_err("%s:ic name not configed\n", __func__);
		return ret;
	}
	ret = get_combo_bus_tag(bus_type, (uint8_t *)&temp);
	if (ret) {
		hwlog_err("%s:bus_typeis invalid\n", __func__);
		return ret;
	}
	tp_ud_data.cfg.bus_type = (uint8_t)temp;
	ret = of_property_read_u32(dn, "bus_number", &temp);
	if (ret) {
		hwlog_err("%s:bus_number not configed\n", __func__);
		return ret;
	}
	tp_ud_data.cfg.bus_num = (uint8_t)temp;

	if (tp_ud_data.cfg.bus_type == TAG_I2C ||
		tp_ud_data.cfg.bus_type == TAG_I3C) {
		ret = of_property_read_u32(dn, "i2c_address", &temp);
		if (ret) {
			hwlog_err("%s:i2c_address not configed\n", __func__);
			return ret;
		}
		tp_ud_data.cfg.i2c_address = temp;
	}

	while (retry_count) {
		unregister_ic_num = get_thp_unregister_ic_num();
		if (!unregister_ic_num) {
			hwlog_info("%s:unregister ic num:%d\n", __func__, unregister_ic_num);
			break;
		}
		retry_count--;
		mdelay(5);
	}
	hwlog_info("%s:unregister ic num:%d\n", __func__, unregister_ic_num);

	attached_vendor_name = thp_get_vendor_name();
	if (!attached_vendor_name) {
		hwlog_err("%s:no attached ic\n", __func__);
		return -ENODEV;
	}

	if (!strcmp(configed_vendor_name, attached_vendor_name)) {
		hwlog_info("%s:tp detect succ, ic:%s\n", __func__, configed_vendor_name);
		return 0;
	}

	return -ENODEV;
}

static void parse_tp_ud_algo_conf(void)
{
	uint32_t temp = 0;
	struct device_node *dn = NULL;

	dn = of_find_compatible_node(NULL, NULL, "up_tp,algo_config");
	if (!dn) {
		hwlog_err("%s:no config, skip\n", __func__);
		memset(&tp_ud_data.algo_conf, 0, sizeof(tp_ud_data.algo_conf));
		return;
	}

	if (of_property_read_u32(dn, "move_area_x_min", &temp))
		hwlog_err("%s:read move_area_x_min fail\n", __func__);
	else
		tp_ud_data.algo_conf.move_area_x_min = (uint16_t)temp;
	if (of_property_read_u32(dn, "move_area_x_max", &temp))
		hwlog_err("%s:read move_area_x_max fail\n", __func__);
	else
		tp_ud_data.algo_conf.move_area_x_max = (uint16_t)temp;

	if (of_property_read_u32(dn, "move_area_y_min", &temp))
		hwlog_err("%s:read move_area_y_min fail\n", __func__);
	else
		tp_ud_data.algo_conf.move_area_y_min = (uint16_t)temp;

	if (of_property_read_u32(dn, "move_area_y_max", &temp))
		hwlog_err("%s:read move_area_y_max fail\n", __func__);
	else
		tp_ud_data.algo_conf.move_area_y_max = (uint16_t)temp;

	if (of_property_read_u32(dn, "finger_area_x_min", &temp))
		hwlog_err("%s:read finger_area_x_min fail\n", __func__);
	else
		tp_ud_data.algo_conf.finger_area_x_min = (uint16_t)temp;

	if (of_property_read_u32(dn, "finger_area_x_max", &temp))
		hwlog_err("%s:read finger_area_x_max fail\n", __func__);
	else
		tp_ud_data.algo_conf.finger_area_x_max = (uint16_t)temp;

	if (of_property_read_u32(dn, "finger_area_y_min", &temp))
		hwlog_err("%s:read finger_area_y_min fail\n", __func__);
	else
		tp_ud_data.algo_conf.finger_area_y_min = (uint16_t)temp;

	if (of_property_read_u32(dn, "finger_area_y_max", &temp))
		hwlog_err("%s:read finger_area_y_max fail\n", __func__);
	else
		tp_ud_data.algo_conf.finger_area_y_max = (uint16_t)temp;

	if (of_property_read_u32(dn, "coor_scale", &temp))
		hwlog_err("%s:read coor_scale fail\n", __func__);
	else
		tp_ud_data.algo_conf.coor_scale = (uint16_t)temp;
}

static void read_tp_ud_from_dts(struct device_node *dn)
{
	uint32_t temp = 0;

	read_chip_info(dn, TP_UD);
	parse_tp_ud_algo_conf();

	if (of_property_read_u32(dn, "file_id", &temp))
		hwlog_err("%s:read tp ud file_id fail\n", __func__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;

	dyn_req->file_count++;
	hwlog_info("tp ud file id is %u\n", temp);

	if (of_property_read_u32(dn, "spi_max_speed_hz", &temp))
		hwlog_err("%s:read spi_max_speed_hz fail\n", __func__);
	else
		tp_ud_data.spi_max_speed_hz = temp;

	if (of_property_read_u32(dn, "spi_mode", &temp))
		hwlog_err("%s:read spi_mode fail\n", __func__);
	else
		tp_ud_data.spi_mode = temp;

	if (of_property_read_u32(dn, "gpio_irq", &temp))
		hwlog_err("%s:read gpio_irq fail\n", __func__);
	else
		tp_ud_data.gpio_irq = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio_irq_sh", &temp))
		hwlog_err("%s:read gpio_irq_sh fail\n", __func__);
	else
		tp_ud_data.gpio_irq_sh = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio_cs", &temp))
		hwlog_err("%s:read gpio_cs fail\n", __func__);
	else
		tp_ud_data.gpio_cs = (GPIO_NUM_TYPE)temp;

	if (of_property_read_u32(dn, "gpio_irq_pull_up_status", &temp)) {
		hwlog_err("%s:read gpio_irq_flag fail\n", __func__);
		tp_ud_data.gpio_irq_pull_up_status = 0;
	} else {
		tp_ud_data.gpio_irq_pull_up_status = (uint16_t)temp;
		hwlog_err("%s:read gpio_irq_pull_up_status %u\n", __func__,
			tp_ud_data.gpio_irq_pull_up_status);
	}

	if (of_property_read_u32(dn, "pressure_support", &temp))
		hwlog_err("%s:read pressure_support fail\n", __func__);
	else
		tp_ud_data.pressure_support = (uint16_t)temp;

	if (of_property_read_u32(dn, "anti_forgery_support", &temp))
		hwlog_err("%s:read anti_forgery_support fail\n", __func__);
	else
		tp_ud_data.anti_forgery_support = (uint16_t)temp;

	if (of_property_read_u32(dn, "ic_type", &temp))
		hwlog_err("%s:read low_power_addr fail\n", __func__);
	else
		tp_ud_data.ic_type = temp;

	if (of_property_read_u32(dn, "hover_enable", &temp))
		hwlog_err("%s:read event_info_addr fail use default\n", __func__);
	else
		tp_ud_data.hover_enable = temp;

	if (of_property_read_u32(dn, "i2c_max_speed_hz", &temp))
		hwlog_err("%s:read i2c_max_speed_hz fail\n", __func__);
	else
		tp_ud_data.i2c_max_speed_hz = temp;
	if (of_property_read_u32(dn, "fw_power_config_reg", &temp)) {
		hwlog_err("%s:read fw_power_config_reg not config\n", __func__);
		tp_ud_data.fw_power_config_reg = 0;
	} else {
		tp_ud_data.fw_power_config_reg = (uint16_t)temp;
		hwlog_err("%s:read fw_power_config_reg = %u\n", __func__,
			tp_ud_data.fw_power_config_reg);
	}

	if (of_property_read_u32(dn, "fw_touch_data_reg", &temp)) {
		hwlog_err("%s:read fw_touch_data_reg not config\n", __func__);
		tp_ud_data.fw_touch_data_reg = 0;
	} else {
		tp_ud_data.fw_touch_data_reg = (uint16_t)temp;
		hwlog_err("%s:read fw_touch_data_reg = %u\n", __func__,
			tp_ud_data.fw_touch_data_reg);
	}

	if (of_property_read_u32(dn, "fw_touch_command_reg", &temp)) {
		hwlog_err("%s:read fw_touch_command_reg not config\n", __func__);
		tp_ud_data.fw_touch_command_reg = 0;
	} else {
		tp_ud_data.fw_touch_command_reg = (uint16_t)temp;
		hwlog_err("%s:read fw_touch_command_reg = %u\n",
			__func__, tp_ud_data.fw_touch_command_reg);
	}

	if (of_property_read_u32(dn, "fw_addr_3", &temp)) {
		hwlog_err("%s:read fw_addr_3 not config\n", __func__);
		tp_ud_data.fw_addr_3 = 0;
	} else {
		tp_ud_data.fw_addr_3 = (uint16_t)temp;
		hwlog_err("%s:read fw_addr_3 = %u\n", __func__, tp_ud_data.fw_addr_3);
	}

	if (of_property_read_u32(dn, "fw_addr_4", &temp)) {
		hwlog_err("%s:read fw_addr_4 not config\n", __func__);
		tp_ud_data.fw_addr_4 = 0;
	} else {
		tp_ud_data.fw_addr_4 = (uint16_t)temp;
		hwlog_err("%s:read fw_addr_4 = %u\n", __func__, tp_ud_data.fw_addr_4);
	}

	if (of_property_read_u32(dn, "fw_addr_5", &temp)) {
		hwlog_err("%s:read fw_addr_5 not config\n", __func__);
		tp_ud_data.fw_addr_5 = 0;
	} else {
		tp_ud_data.fw_addr_5 = (uint16_t)temp;
		hwlog_err("%s:read fw_addr_5 = %u\n", __func__, tp_ud_data.fw_addr_5);
	}

	if (of_property_read_u32(dn, "fw_addr_6", &temp)) {
		hwlog_err("%s:read fw_addr_6 not config\n", __func__);
		tp_ud_data.fw_addr_6 = 0;
	} else {
		tp_ud_data.fw_addr_6 = (uint16_t)temp;
		hwlog_err("%s:read fw_addr_6 = %u\n", __func__, tp_ud_data.fw_addr_6);
	}

	if (of_property_read_u32(dn, "fw_addr_7", &temp)) {
		hwlog_err("%s:read fw_addr_7 not config\n", __func__);
		tp_ud_data.fw_addr_7 = 0;
	} else {
		tp_ud_data.fw_addr_7 = (uint16_t)temp;
		hwlog_err("%s:read fw_addr_7 = %u\n", __func__, tp_ud_data.fw_addr_7);
	}
	if (of_property_read_u32(dn, "tp_sensorhub_irq_flag", &temp)) {
		hwlog_err("%s:read tp_sensorhub_irq_flag not config\n", __func__);
		tp_ud_data.tp_sensorhub_irq_flag = 0;
	} else {
		tp_ud_data.tp_sensorhub_irq_flag = (uint16_t)temp;
		hwlog_err("%s:read tp_sensorhub_irq_flag = %u\n",
			__func__, tp_ud_data.tp_sensorhub_irq_flag);
	}
	if (of_property_read_u32(dn, "tp_sensor_spi_sync_cs_low_delay_us", &temp)) {
		hwlog_err("%s:read tp_sensor_spi_sync_cs_low_delay_us not config\n",
			__func__);
		tp_ud_data.tp_sensor_spi_sync_cs_low_delay_us = 0;
	} else {
		tp_ud_data.tp_sensor_spi_sync_cs_low_delay_us = (uint16_t)temp;
		hwlog_err("%s:read tp_sensor_spi_sync_cs_low_delay_us = %u\n",
			__func__, tp_ud_data.tp_sensor_spi_sync_cs_low_delay_us);
	}
	if (of_property_read_u32(dn, "touch_report_restore_support", &temp)) {
		hwlog_info("%s: read touch_report_restore_support not config\n",
			__func__);
		tp_ud_data.touch_report_restore_support = 0;
	} else {
		tp_ud_data.touch_report_restore_support = (uint16_t)temp;
		hwlog_info("%s: read touch_report_restore_support = %u\n",
			__func__, tp_ud_data.touch_report_restore_support);
	}

	if (of_property_read_u32(dn, "soft_reset_support", &temp)) {
		hwlog_info("%s:read soft_reset_support not config\n", __func__);
		tp_ud_data.soft_reset_support = 0; /* default value */
	} else {
		tp_ud_data.soft_reset_support = (uint16_t)temp;
		hwlog_info("%s:read soft_reset_support = %u\n",
			__func__, tp_ud_data.soft_reset_support);
	}

	if (of_property_read_u32(dn, "aod_display_support", &temp)) {
		hwlog_info("%s:read aod_display_support not config\n", __func__);
		tp_ud_data.aod_display_support = 0; /* default value */
	} else {
		tp_ud_data.aod_display_support = (uint16_t)temp;
		hwlog_info("%s:read aod_display_support = %u\n",
			__func__, tp_ud_data.aod_display_support);
	}
}

int detect_disable_sample_task_prop(struct device_node *dn, uint32_t *value)
{
	int ret;

	ret = of_property_read_u32(dn, "disable_sample_task", value);
	if (ret)
		return -1;

	return 0;
}

static int get_combo_bus_tag(const char *bus, uint8_t *tag)
{
	enum obj_tag tag_tmp = TAG_END;

	if (!strcmp(bus, "i2c"))
		tag_tmp = TAG_I2C;
	else if (!strcmp(bus, "spi"))
		tag_tmp = TAG_SPI;

	if (tag_tmp == TAG_END)
		return -1;

	*tag = (uint8_t)tag_tmp;
	return 0;
}

static int get_combo_prop(struct device_node *dn, struct detect_word *p_det_wd)
{
	int ret;
	struct property *prop = NULL;
	const char *bus_type = NULL;
	uint32_t u32_temp;

	/* combo_bus_type */
	ret = of_property_read_string(dn, "combo_bus_type", &bus_type);
	if (ret) {
		hwlog_err("%s: get bus_type err!\n", __func__);
		return ret;
	}
	if (get_combo_bus_tag(bus_type, &p_det_wd->cfg.bus_type)) {
		hwlog_err("%s: bus_type %s err!\n", __func__, bus_type);
		return -1;
	}

	/* combo_bus_num */
	ret = of_property_read_u32(dn, "combo_bus_num", &u32_temp);
	if (ret) {
		hwlog_err("%s: get combo_data err!\n", __func__);
		return ret;
	}
	p_det_wd->cfg.bus_num = (uint8_t)u32_temp;

	/* combo_data */
	ret = of_property_read_u32(dn, "combo_data", &u32_temp);
	if (ret) {
		hwlog_err("%s: get combo_data err!\n", __func__);
		return ret;
	}
	p_det_wd->cfg.data = u32_temp;

	/* combo_tx */
	prop = of_find_property(dn, "combo_tx", NULL);
	if (!prop) {
		hwlog_err("%s: get combo_tx err!\n", __func__);
		return -1;
	}
	p_det_wd->tx_len = (uint32_t)prop->length;
	if (p_det_wd->tx_len > sizeof(p_det_wd->tx)) {
		hwlog_err("%s: get combo_tx_len %d too big!\n", __func__, p_det_wd->tx_len);
		return -1;
	}
	of_property_read_u8_array(dn, "combo_tx", p_det_wd->tx, (size_t)prop->length);

	/* combo_rx_mask */
	prop = of_find_property(dn, "combo_rx_mask", NULL);
	if (!prop) {
		hwlog_err("%s: get combo_rx_mask err!\n", __func__);
		return -1;
	}
	p_det_wd->rx_len = (uint32_t)prop->length;
	if (p_det_wd->rx_len > sizeof(p_det_wd->rx_msk)) {
		hwlog_err("%s: get rx_len %d too big!\n", __func__, p_det_wd->rx_len);
		return -1;
	}
	of_property_read_u8_array(dn, "combo_rx_mask", p_det_wd->rx_msk, (size_t)prop->length);

	/* combo_rx_exp */
	prop = of_find_property(dn, "combo_rx_exp", NULL);
	if (!prop) {
		hwlog_err("%s: get combo_rx_exp err!\n", __func__);
		return -1;
	}
	prop->length = (uint32_t)prop->length;
	if ((ssize_t)prop->length > sizeof(p_det_wd->rx_exp) || ((uint32_t)prop->length % p_det_wd->rx_len)) {
		hwlog_err("%s: rx_exp_len %d not available!\n", __func__, prop->length);
		return -1;
	}
	p_det_wd->exp_n = (uint32_t)prop->length / p_det_wd->rx_len;
	of_property_read_u8_array(dn, "combo_rx_exp", p_det_wd->rx_exp, (size_t)prop->length);

	return 0;
}

static int i2c_detect_sensors(char *device_name, int i2c_address,
			      int i2c_bus_num, int register_add,
			      u32 *wia, int len)
{
	int i;
	int ret;
	uint32_t device_id = 0;
	uint8_t detect_device_id[DEVICE_ID_LEN] = { 0 };

	ret = mcu_i2c_rw((uint8_t)i2c_bus_num, (uint8_t)i2c_address,
			 (uint8_t *)&register_add, AW_DETECT_LEN,
			 &detect_device_id, DEVICE_ID_LEN);
	if (ret) {
		hwlog_err("%s:detect_i2c_device:send i2c read cmd to mcu fail,ret=%d\n", device_name, ret);
		return -1;
	}

	ret = memcpy_s(&device_id, sizeof(device_id), detect_device_id, DEVICE_ID_LEN);
	if (ret != EOK) {
		hwlog_err("%s:memcpy_s fail,ret %d\n", __func__, ret);
		return -1;
	}

	for (i = 0; i < len; i++) {
		if (device_id == wia[i]) {
			hwlog_info("%s:i2c detect  suc!chip_value:0x%x\n", device_name, device_id);
			return 0;
		}
	}

	hwlog_info("%s:i2c detect fail,chip_value:0x%x,len:%d\n", device_name, device_id, len);
	return -1;
}

static int detect_i2c_device(struct device_node *dn, char *device_name)
{
	int i;
	int ret;
	int i2c_address = 0;
	int i2c_bus_num = 0;
	int register_add = 0;
	int len;
	u32 wia[10] = { 0 };
	uint8_t detected_device_id;
	uint8_t i2c_detect_long_type = 0;
	struct property *prop = NULL;

	if (of_property_read_u32(dn, "bus_number", &i2c_bus_num) ||
		of_property_read_u32(dn, "reg", &i2c_address) ||
		of_property_read_u32(dn, "chip_id_register", &register_add)) {
		hwlog_err("%s:read i2c bus_num %d or bus addr %x or chip_id_reg %x fail\n",
			device_name, i2c_bus_num, i2c_address, register_add);
		return -1;
	}

	prop = of_find_property(dn, "chip_id_value", NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;
	len = prop->length / 4; /* 4: to word */

	if (of_property_read_u32_array(dn, "chip_id_value", wia, len)) {
		hwlog_err("%s:read chip_id_value id0 %x from dts fail len %d\n", device_name, wia[0], len);
		return -1;
	}

	hwlog_info("%s:i2c %d slave addr 0x%x chip_id_reg 0x%x chipid value 0x%x 0x%x 0x%x 0x%x\n",
		device_name, i2c_bus_num, i2c_address, register_add, wia[0], wia[1], wia[2], wia[3]);

	of_property_read_u8(dn, "i2c_detect_long_type", &i2c_detect_long_type);
	hwlog_info("%s:read i2c_detect_long_type:0x%x\n", device_name, i2c_detect_long_type);

	if (i2c_detect_long_type) {
		ret = i2c_detect_sensors(device_name, i2c_address,
					 i2c_bus_num, register_add,
					 wia, len);
		if (ret)
			return -1;
		else
			return 0;
	}

	(void)of_property_read_u8(dn, "chip_id_register_len", &register_add_len);
	hwlog_info("%s:read chip_id_register_len:0x%x\n", device_name, register_add_len);

	ret = mcu_i2c_rw((uint8_t)i2c_bus_num, (uint8_t)i2c_address,
		(uint8_t *)&register_add, register_add_len, &detected_device_id, 1);
	if (ret) {
		hwlog_err("%s:%s:send i2c read cmd to mcu fail,ret %d\n",
			__func__, device_name, ret);
		return -1;
	}
	register_add_len = 1;
	if (!strncmp(device_name, "vibrator", strlen("vibrator"))) {
		hwlog_info("virbator temp i2c detect success,chip_value:0x%x,len:%d\n",
			detected_device_id, len);
		return 0;
	}
	for (i = 0; i < len; i++) {
		if (detected_device_id == (char)wia[i]) {
			hwlog_info("%s:i2c detect suc!chip_value:0x%x\n",
				device_name, detected_device_id);
			return 0;
		}
	}
	hwlog_info("%s:i2c detect fail,chip_value:0x%x,len:%d\n",
		device_name, detected_device_id, len);
	return -1;
}

static int device_detect_i2c(struct device_node *dn, int index,
	struct detect_word *p_det_wd)
{
	int ret;
	uint32_t i2c_bus_num = 0;
	uint32_t i2c_address = 0;
	uint32_t register_add = 0;

	ret = detect_i2c_device(dn, sensor_manager[index].sensor_name_str);
	if (!ret) {
		if (of_property_read_u32(dn, "bus_number", &i2c_bus_num) ||
			of_property_read_u32(dn, "reg", &i2c_address) ||
			of_property_read_u32(dn, "chip_id_register", &register_add)) {
			hwlog_err("%s:read i2c bus_num %d or bus addr %x or chip_id_reg %x fail\n",
				sensor_manager[index].sensor_name_str, i2c_bus_num, i2c_address, register_add);
			return -1;
		}
		p_det_wd->cfg.bus_type = TAG_I2C;
		p_det_wd->cfg.bus_num = (uint8_t)i2c_bus_num;
		p_det_wd->cfg.i2c_address = (uint8_t)i2c_address;
	}

	return ret;
}

int _device_detect(struct device_node *dn, int index,
	struct sensor_combo_cfg *p_succ_ret)
{
	int ret;
	struct detect_word det_wd = { { 0 }, 0 };
	struct property *prop = of_find_property(dn, "combo_bus_type", NULL);
	uint8_t r_buf[MAX_TX_RX_LEN];

	if (prop) {
		uint32_t i, n;

		ret = get_combo_prop(dn, &det_wd);
		if (ret) {
			hwlog_err("%s:get_combo_prop fail\n", __func__);
			return ret;
		}

		hwlog_info("%s: combo detect bus type %d; num %d; data %d; txlen %d; tx[0] 0x%x; rxLen %d; rxmsk[0] 0x%x; n %d; rxexp[0] 0x%x",
			__func__, det_wd.cfg.bus_type, det_wd.cfg.bus_num,
			det_wd.cfg.data, det_wd.tx_len, det_wd.tx[0],
			det_wd.rx_len, det_wd.rx_msk[0], det_wd.exp_n,
			det_wd.rx_exp[0]);

		ret = combo_bus_trans(&det_wd.cfg, det_wd.tx, det_wd.tx_len, r_buf, det_wd.rx_len);
		hwlog_info("combo_bus_trans ret is %d; rx 0x%x;\n", ret, r_buf[0]);
		if (ret < 0)
			return ret;
		ret = -1; /* fail first */
		/* check expect value */
		for (n = 0; n < det_wd.exp_n; n++) {
			for (i = 0; i < det_wd.rx_len;) {
				/* check value */
				if ((r_buf[i] & det_wd.rx_msk[i]) !=
					det_wd.rx_exp[n * det_wd.rx_len + i])
					break;
				i++;
			}
			if (i == det_wd.rx_len) { /* get the success device */
				ret = 0;
				hwlog_info("%s: %s detect succ;\n", __func__,
					sensor_manager[index].sensor_name_str);
				break;
			}
		}
	} else {
		hwlog_info("%s: %s donot find combo prop;\n",
			__func__, sensor_manager[index].sensor_name_str);
		ret = device_detect_i2c(dn, index, &det_wd);
	}

	if (!ret)
		*p_succ_ret = det_wd.cfg;

	return ret;
}

static int device_detect_ex(struct device_node *dn, int index)
{
	int ret = 0;
	struct sensor_combo_cfg cfg = { 0 };

	if (sensor_manager[index].sensor_id == HANDPRESS) {
		return handpress_sensor_detect(dn, sensor_manager, index);
	} else if (sensor_manager[index].sensor_id == FINGERPRINT) {
		return inputhub_fingerprint_sensor_detect(dn, sensor_manager,
			index);
	} else if (sensor_manager[index].sensor_id == FINGERPRINT_UD) {
		return inputhub_fingerprint_ud_sensor_detect(dn,
			sensor_manager, index);
	} else if (sensor_manager[index].sensor_id == CAP_PROX) {
		return cap_prox_sensor_detect(dn, sensor_manager, index);
	} else if (sensor_manager[index].sensor_id == KEY) {
		return inputhub_key_sensor_detect(dn, sensor_manager, index);
	} else if (sensor_manager[index].sensor_id == MOTION) {
		hwlog_info("%s:motion detect always ok\n", __func__);
		return ret;
	} else if (sensor_manager[index].sensor_id == PS) {
		return ps_sensor_detect(dn, sensor_manager, index);
	} else if (sensor_manager[index].sensor_id == TP_UD) {
		hwlog_info("tp_ud sensor detect start\n");
		ret = tp_ud_sensor_detect(dn);
		if (ret)
			hwlog_err("tp ud sensor detect fail\n");
		return ret;
	} else if (sensor_manager[index].sensor_id == TOF ||
		sensor_manager[index].sensor_id == TOF1) {
		hwlog_info("[%s-%d]:tof sensor_id:%d sensor detect start\n",
			__func__, __LINE__, sensor_manager[index].sensor_id);
		ret = tof_sensor_detect(dn, sensor_manager, index);
		if (ret)
			hwlog_err("tof sensor detect fail\n");
		return ret;
	}
	ret = _device_detect(dn, index, &cfg);
	if (!ret) {
		if (memcpy_s((void *)sensor_manager[index].spara,
			sizeof(struct sensor_combo_cfg), (void *)&cfg,
			sizeof(struct sensor_combo_cfg)) != EOK)
			ret = RET_FAIL;
	}
	return ret;
}

static int device_detect(struct device_node *dn, int index)
{
	int ret;
	struct sensor_combo_cfg *p_cfg = NULL;
	uint32_t disable;

	if (sensor_manager[index].detect_result == DET_SUCC)
		return -1;

	ret = device_detect_ex(dn, index);
	if (ret) {
		sensor_manager[index].detect_result = DET_FAIL;
		return DET_FAIL;
	}

	/* check disable sensor task */
	p_cfg = (struct sensor_combo_cfg *)sensor_manager[index].spara;

	ret = detect_disable_sample_task_prop(dn, &disable);
	if (!ret)
		/* get disbale_sample_task property value */
		p_cfg->disable_sample_thread = (uint8_t)disable;

	sensor_manager[index].detect_result = DET_SUCC;
	return DET_SUCC;
}

static int get_sensor_index(const char *name_buf, int len)
{
	int i;

	for (i = 0; i < SENSOR_MAX; i++) {
		if (len != strlen(sensor_manager[i].sensor_name_str))
			continue;
		if (!strncmp(name_buf, sensor_manager[i].sensor_name_str, len))
			break;
	}
	if (i < SENSOR_MAX)
		return i;

	hwlog_err("get_sensor_detect_index fail\n");
	return -1;
}

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
static void __set_hw_dev_flag(enum sensor_detect_list s_id)
{
/* detect current device successful, set the flag as present */
	switch (s_id) {
	case ACC:
		set_hw_dev_flag(DEV_I2C_G_SENSOR);
		break;
	case MAG:
		set_hw_dev_flag(DEV_I2C_COMPASS);
		break;
	case GYRO:
		set_hw_dev_flag(DEV_I2C_GYROSCOPE);
		break;
	case ALS:
	case PS:
		set_hw_dev_flag(DEV_I2C_L_SENSOR);
		break;
	case AIRPRESS:
		set_hw_dev_flag(DEV_I2C_AIRPRESS);
		break;
	case HANDPRESS:
		set_hw_dev_flag(DEV_I2C_HANDPRESS);
		break;
	case VIBRATOR:
		set_hw_dev_flag(DEV_I2C_VIBRATOR_LRA);
		break;
	case CAP_PROX:
	case FINGERPRINT:
	case KEY:
	case MOTION:
	case MAGN_BRACKET:
	case GPS_4774_I2C:
		break;
	case TP_UD:
		break;
	case HUMITURE:
		set_hw_dev_flag(DEV_I2C_HUMITURE);
		break;
	default:
		hwlog_err("%s:err id %d\n", __func__, s_id);
		break;
	}
}
#endif

static int extend_config_before_sensor_detect(struct device_node *dn, int index)
{
	int ret = 0;
	enum sensor_detect_list s_id;

	s_id = sensor_manager[index].sensor_id;

	switch (s_id) {
	case GPS_4774_I2C:
		sensor_manager[index].detect_result = DET_SUCC;
		read_gps_4774_i2c_data_from_dts(dn);
		break;
	case MAGN_BRACKET:
		sensor_manager[index].detect_result = DET_SUCC;
		read_magn_bracket_data_from_dts(dn);
		break;
	case RPC:
		sensor_manager[index].detect_result = DET_SUCC;
		read_rpc_data_from_dts(dn);
		break;
	case SH_AOD:
		sensor_manager[index].detect_result = DET_SUCC;
		read_aod_data_from_dts(dn);
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}

static void extend_config_after_sensor_detect(struct device_node *dn, int index)
{
	int ret;
	enum sensor_detect_list s_id;

	s_id = sensor_manager[index].sensor_id;
	switch (s_id) {
	case ACC:
		read_acc_data_from_dts(dn);
		break;
	case MAG:
		read_mag_data_from_dts(dn);
		ret = fill_extend_data_in_dts(dn, str_soft_para, mag_data.pdc_data,
			PDC_SIZE, EXTEND_DATA_TYPE_IN_DTS_BYTE);
		if (ret)
			hwlog_err("%s:fill_extend_data_in_dts failed!\n", str_soft_para);
		break;
	case GYRO:
		read_gyro_data_from_dts(dn);
		break;
	case ALS:
		read_als_data_from_dts(dn, &sensor_manager[index]);
		break;
	case PS:
		read_ps_data_from_dts(dn, &sensor_manager[index]);
		break;
	case AIRPRESS:
		read_airpress_data_from_dts(dn);
		break;
	case HANDPRESS:
		read_handpress_data_from_dts(dn, &sensor_manager[index]);
		break;
	case CAP_PROX:
		read_capprox_data_from_dts(dn, &sensor_manager[index]);
		break;
	case KEY:
		read_key_i2c_data_from_dts(dn);
		break;
	case FINGERPRINT:
		read_fingerprint_from_dts(dn);
		break;
	case VIBRATOR:
		read_vibrator_from_dts(dn, &sensor_manager[index]);
		break;
	case FINGERPRINT_UD:
		read_fingerprint_ud_from_dts(dn);
		break;
	case MOTION:
		read_motion_data_from_dts(dn, &sensor_manager[index]);
		break;
	case TOF:
		read_tof_data_from_dts(dn);
		break;
	case HUMITURE:
		read_humiture_data_from_dts(dn);
		break;
	case TP_UD:
		read_tp_ud_from_dts(dn);
		break;
	case TOF1:
		read_tof_data_from_dts_ext(s_id, dyn_req, dn, &g_sensor_tof_flag);
		break;
	default:
		hwlog_err("%s:err id %d\n", __func__, s_id);
		break;
	}
}

#ifdef CONFIG_HUAWEI_DSM
static void update_detectic_client_info(void)
{
	char sensor_name[DSM_MAX_IC_NAME_LEN] = { 0 };
	uint8_t i;
	int total_len = 0;
	struct dsm_dev *dsm = get_dsm_sensorhub();

	for (i = 0; i < SENSOR_MAX; i++) {
		if (sensor_manager[i].detect_result == DET_FAIL) {
			total_len += strlen(sensor_manager[i].sensor_name_str);
			if (total_len < DSM_MAX_IC_NAME_LEN)
				strcat(sensor_name, sensor_manager[i].sensor_name_str);
		}
	}
	sensor_name[DSM_MAX_IC_NAME_LEN - 1] = '\0';
	hwlog_info("%s %s\n", __func__, sensor_name);
	dsm->ic_name = sensor_name;
	dsm_update_client_vendor_info(dsm);
}

static void boot_detect_fail_dsm(char *title, char *detect_result,
	uint32_t size)
{
	struct dsm_client *client = inputhub_get_shb_dclient();

	if (!dsm_client_ocuppy(client)) {
		update_detectic_client_info();
		dsm_client_record(client, "[%s]%s", title, detect_result);
		dsm_client_notify(client, DSM_SHB_ERR_IOM7_DETECT_FAIL);
	}
}
#endif

static uint8_t check_detect_result(enum detect_mode mode)
{
	int i;
	uint8_t detect_fail_num = 0;
	uint8_t result;
	int total_len = 0;
	char detect_result[MAX_STR_SIZE] = { 0 };
	const char *sf = " detect fail!";

	for (i = 0; i < SENSOR_MAX; i++) {
		result = sensor_manager[i].detect_result;
		if (result == DET_FAIL) {
			detect_fail_num++;
			total_len += strlen(sensor_manager[i].sensor_name_str);
			total_len += 2; /* 2 bytes */
			if (total_len < MAX_STR_SIZE) {
				strcat(detect_result, sensor_manager[i].sensor_name_str);
				strcat(detect_result, "  ");
			}
			hwlog_info("%s: %s detect fail\n", __func__,
				sensor_manager[i].sensor_name_str);
		} else if (result == DET_SUCC) {
			hwlog_info("%s: %s detect success\n", __func__,
				sensor_manager[i].sensor_name_str);
			if (i == GYRO)
				gyro_detect_flag = 1;
		}
	}

	if (detect_fail_num > 0) {
		s_redetect_state.need_redetect_sensor = 1;
		total_len += strlen(sf);
		if (total_len < MAX_STR_SIZE)
			strcat(detect_result, sf);

#ifdef CONFIG_HUAWEI_DSM
		if (mode == BOOT_DETECT_END)
			boot_detect_fail_dsm((char *)__func__, detect_result,
				MAX_STR_SIZE);
#endif
	} else {
		s_redetect_state.need_redetect_sensor = 0;
	}

	if ((detect_fail_num < s_redetect_state.detect_fail_num) &&
		(mode == REDETECT_LATER)) {
		s_redetect_state.need_recovery = 1;
		hwlog_info("%s : %u sensors detect success after redetect\n",
			__func__, s_redetect_state.detect_fail_num - detect_fail_num);
	}
	s_redetect_state.detect_fail_num = detect_fail_num;
	return detect_fail_num;
}

static void show_last_detect_fail_sensor(void)
{
	int i;
	uint8_t result;

	for (i = 0; i < SENSOR_MAX; i++) {
		result = sensor_manager[i].detect_result;
		if (result == DET_FAIL)
			hwlog_err("last detect fail sensor: %s\n",
				sensor_manager[i].sensor_name_str);
	}
}

static void read_cap_prox_info(struct device_node *dn)
{
	int register_add = 0;
	int i2c_address = 0;
	int i2c_bus_num = 0;
	u32 wia[CAP_CHIPID_DATA_LENGTH] = {0};
	char *chip_info = NULL;

	if (of_property_read_u32(dn, "bus_number", &i2c_bus_num) ||
		of_property_read_u32(dn, "reg", &i2c_address) ||
		of_property_read_u32(dn, "chip_id_register", &register_add))
		hwlog_err("read i2c bus_num %d or bus addr %x or chip_id_reg %x fail\n",
			i2c_bus_num, i2c_address, register_add);

	if (of_property_read_u32_array(dn, "chip_id_value",
		wia, CAP_CHIPID_DATA_LENGTH))
		hwlog_err("sar sensor:read chip_id_value id0=0x%x id1=0x%x fail\n",
			wia[0], wia[1]);

	hwlog_info("sar sensor:bus_num %d slave addr 0x%x chip_id_reg 0x%x chipid value 0x%x 0x%x\n",
		i2c_bus_num, i2c_address, register_add, wia[0], wia[1]);

	if (of_property_read_string(dn, "compatible", (const char **)&chip_info))
		hwlog_err("%s:read name_id:CAP_PROX info fail\n", __func__);

	if (!strncmp(chip_info, "huawei,semtech-sx9323",
		strlen("huawei,semtech-sx9323"))) {
		hwlog_info("sar sensor from dts is semtech-sx9323\n");
		semtech_sar_detect.detect_flag = 1;
		semtech_sar_detect.cfg.bus_num = (uint8_t)i2c_bus_num;
		semtech_sar_detect.cfg.i2c_address = (uint8_t)i2c_address;
		semtech_sar_detect.chip_id = (uint8_t)register_add;
		semtech_sar_detect.chip_id_value[0] = (uint8_t)wia[0];
		semtech_sar_detect.chip_id_value[1] = (uint8_t)wia[1];
	} else if (!strncmp(chip_info, "huawei,adi-adux1050",
		strlen("huawei,adi-adux1050"))) {
		hwlog_info("sar sensor from dts is adi-adux1050\n");
		adi_sar_detect.detect_flag = 1;
		adi_sar_detect.cfg.bus_num = (uint8_t)i2c_bus_num;
		adi_sar_detect.cfg.i2c_address = (uint8_t)i2c_address;
		adi_sar_detect.chip_id = (uint8_t)register_add;
		adi_sar_detect.chip_id_value[0] = (uint8_t)wia[0];
		adi_sar_detect.chip_id_value[1] = (uint8_t)wia[1];
	} else if (!strncmp(chip_info, "huawei,cypress_sar_psoc4000",
		strlen("huawei,cypress_sar_psoc4000"))) {
		hwlog_info("sar sensor from dts is cypress_sar_psoc4000\n");
		cypress_sar_detect.detect_flag = 1;
		cypress_sar_detect.cfg.bus_num = (uint8_t)i2c_bus_num;
		cypress_sar_detect.cfg.i2c_address = (uint8_t)i2c_address;
		cypress_sar_detect.chip_id = (uint8_t)register_add;
		cypress_sar_detect.chip_id_value[0] = (uint8_t)wia[0];
		cypress_sar_detect.chip_id_value[1] = (uint8_t)wia[1];
	} else if (!strncmp(chip_info, "huawei,abov-a96t3x6",
		strlen("huawei,abov-a96t3x6"))) {
		hwlog_info("sar sensor from dts is abov-a96t3x6\n");
		g_abov_sar_detect.detect_flag = 1;
		g_abov_sar_detect.cfg.bus_num = (uint8_t)i2c_bus_num;
		g_abov_sar_detect.cfg.i2c_address = (uint8_t)i2c_address;
		g_abov_sar_detect.chip_id = (uint8_t)register_add;
		g_abov_sar_detect.chip_id_value[0] = (uint8_t)wia[0];
		g_abov_sar_detect.chip_id_value[1] = (uint8_t)wia[1];
	} else if (!strncmp(chip_info, "huawei,awi-aw9610x",
		strlen("huawei,awi-aw9610x"))) {
		hwlog_info("sar sensor from dts is awi-aw9610x\n");
		aw9610_sar_detect.detect_flag = 1;
		aw9610_sar_detect.cfg.bus_num = (uint8_t)i2c_bus_num;
		aw9610_sar_detect.cfg.i2c_address = (uint8_t)i2c_address;
		aw9610_sar_detect.chip_id = (uint8_t)register_add;
		aw9610_sar_detect.chip_id_value[0] = (uint8_t)wia[0];
		aw9610_sar_detect.chip_id_value[1] = (uint8_t)wia[1];
	}
}

static void redetect_failed_sensors(enum detect_mode mode)
{
	int index;
	char *sensor_ty = NULL;
	char *sensor_st = NULL;
	struct device_node *dn = NULL;
	const char *st = "disabled";

	for_each_node_with_property(dn, "sensor_type") {
		/* sensor type */
		if (of_property_read_string(dn, "sensor_type", (const char **)&sensor_ty)) {
			hwlog_err("redetect get sensor type fail\n");
			continue;
		}
		index = get_sensor_index(sensor_ty, strlen(sensor_ty));
		if (index < 0) {
			hwlog_err("redetect get sensor index fail\n");
			continue;
		}
		/* sensor status:ok or disabled */
		if (of_property_read_string(dn, "status", (const char **)&sensor_st)) {
			hwlog_err("redetect get sensor status fail\n");
			continue;
		}
		if (!strcmp(st, sensor_st)) {
			hwlog_info("%s : sensor %s status is %s\n",
				__func__, sensor_ty, sensor_st);
			continue;
		}
		if (device_detect(dn, index) != DET_SUCC)
			continue;

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
		__set_hw_dev_flag(sensor_manager[index].sensor_id);
#endif

		extend_config_after_sensor_detect(dn, index);
	}
	check_detect_result(mode);
}

static void sensor_detect_exception_process(uint8_t result)
{
	int i;

	if (result > 0) {
		for (i = 0; i < SENSOR_DETECT_RETRY; i++) {
			hwlog_info("%s: %d redect sensor, fail sensor num %d\n",
				__func__, i, s_redetect_state.detect_fail_num);
			if (s_redetect_state.detect_fail_num > 0)
				redetect_failed_sensors(DETECT_RETRY + i);
		}
	}
}

static void init_sensors_cfg_data_each_node(void)
{
	int ret;
	int index;
	char *sensor_ty = NULL;
	char *sensor_st = NULL;
	struct device_node *dn = NULL;
	const char *st = "disabled";

	for_each_node_with_property(dn, "sensor_type") {
		/* sensor type */
		ret = of_property_read_string(dn, "sensor_type",
			(const char **)&sensor_ty);
		if (ret) {
			hwlog_err("get sensor type fail ret %d\n", ret);
			continue;
		}
		hwlog_info("%s : get sensor type %s\n", __func__, sensor_ty);
		index = get_sensor_index(sensor_ty, strlen(sensor_ty));
		if (index < 0) {
			hwlog_err("get sensor index fail ret %d\n", ret);
			continue;
		}
		if (sensor_manager[index].sensor_id == CAP_PROX)
			read_cap_prox_info(dn); /* for factory sar */

		/* sensor status:ok or disabled */
		ret = of_property_read_string(dn, "status",
			(const char **)&sensor_st);
		if (ret) {
			hwlog_err("get sensor status fail ret %d\n", ret);
			continue;
		}

		ret = strcmp(st, sensor_st);
		if (!ret)
			continue;
		if (!extend_config_before_sensor_detect(dn, index))
			continue;

		ret = device_detect(dn, index);
		if (ret != DET_SUCC)
			continue;

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
		__set_hw_dev_flag(sensor_manager[index].sensor_id);
#endif

		extend_config_after_sensor_detect(dn, index);
	}
}

int init_sensors_cfg_data_from_dts(void)
{
	int i;
	uint8_t sensor_detect_result;

	memset(&sensorlist_info, 0, SENSOR_MAX * sizeof(struct sensorlist_info));
	for (i = 0; i < SENSOR_MAX; i++) { /* init sensorlist_info struct array */
		sensorlist_info[i].version = -1;
		sensorlist_info[i].max_range = -1;
		sensorlist_info[i].resolution = -1;
		sensorlist_info[i].power = -1;
		sensorlist_info[i].min_delay = -1;
		sensorlist_info[i].max_delay = -1;
		sensorlist_info[i].fifo_reserved_event_count = 0xFFFFFFFF;
		sensorlist_info[i].fifo_max_event_count = 0xFFFFFFFF;
		sensorlist_info[i].flags = 0xFFFFFFFF;
	}

	init_sensors_cfg_data_each_node();

	if (sensor_manager[FINGERPRINT].detect_result != DET_SUCC) {
		hwlog_warn("fingerprint detect fail, bypass\n");
		sensor_manager[FINGERPRINT].detect_result = DET_SUCC;
	}
	if (sensor_manager[FINGERPRINT_UD].detect_result != DET_SUCC) {
		hwlog_warn("fingerprint_ud detect fail, bypass\n");
		sensor_manager[FINGERPRINT_UD].detect_result = DET_SUCC;
	}

	sensor_detect_result = check_detect_result(BOOT_DETECT);
	sensor_detect_exception_process(sensor_detect_result);

	if (get_adapt_id_and_send())
		return -EINVAL;

	return 0;
}

void send_parameter_to_mcu(enum sensor_detect_list s_id, int cmd)
{
	int ret;
	struct write_info pkg_ap = { 0 };
	struct read_info pkg_mcu = { 0 };
	pkt_parameter_req_t cpkt;
	struct pkt_header *hd = (struct pkt_header *)&cpkt;
	char buf[50] = { 0 }; /* buf size 50 */

	pkg_ap.tag = sensor_manager[s_id].tag;
	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	cpkt.subcmd = cmd;
	pkg_ap.wr_buf = &hd[1];
	pkg_ap.wr_len = sensor_manager[s_id].cfg_data_length + SUBCMD_LEN;
	memcpy(cpkt.para, sensor_manager[s_id].spara, sensor_manager[s_id].cfg_data_length);

	hwlog_info("%s g_iom3_state %d, tag %d, cmd %d\n",
		__func__, get_iom3_state(), sensor_manager[s_id].tag, cmd);

	if ((get_iom3_state() == IOM3_ST_RECOVERY) ||
		(get_iomcu_power_state() == ST_SLEEP))
		ret = write_customize_cmd(&pkg_ap, NULL, false);
	else
		ret = write_customize_cmd(&pkg_ap, &pkg_mcu, true);

	if (ret) {
		hwlog_err("send tag %d cfg data to mcu fail,ret %d\n", pkg_ap.tag, ret);
	} else {
		if (pkg_mcu.errno != 0) {
			/* buf size 50 */
			snprintf(buf, 50, "set %s cfg fail\n",
				sensor_manager[s_id].sensor_name_str);
			hwlog_err("%s\n", buf);
		} else {
			/* buf size 50 */
			snprintf(buf, 50, "set %s cfg to mcu success\n",
				sensor_manager[s_id].sensor_name_str);
			hwlog_info("%s\n", buf);

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
			if (get_iom3_state() != IOM3_ST_RECOVERY)
				__set_hw_dev_flag(s_id);
#endif
		}
	}
}

static int mag_data_from_mcu(const struct pkt_header *head)
{
	switch (((pkt_mag_calibrate_data_req_t *)head)->subcmd) {
	case SUB_CMD_CALIBRATE_DATA_REQ:
		if (akm_cal_algo == 1)
			return write_magsensor_calibrate_data_to_nv(((pkt_akm_mag_calibrate_data_req_t *)head)->calibrate_data);
		else
			return write_magsensor_calibrate_data_to_nv(((pkt_mag_calibrate_data_req_t *)head)->calibrate_data);
	default:
		hwlog_err("uncorrect subcmd 0x%x\n", ((pkt_mag_calibrate_data_req_t *)head)->subcmd);
	}
	return 0;
}

static int gyro_data_from_mcu(const struct pkt_header *head)
{
	switch (((pkt_gyro_calibrate_data_req_t *)head)->subcmd) {
	case SUB_CMD_SELFCALI_REQ:
		return write_gyro_sensor_offset_to_nv(
			((pkt_gyro_calibrate_data_req_t *)head)->calibrate_data, head->length - SUBCMD_LEN);
	case SUB_CMD_GYRO_TMP_OFFSET_REQ:
		return write_gyro_temperature_offset_to_nv(((pkt_gyro_temp_offset_req_t *)head)->calibrate_data, GYRO_TEMP_CALI_NV_SIZE);
	default:
		hwlog_err("uncorrect subcmd 0x%x\n",
			((pkt_gyro_calibrate_data_req_t *)head)->subcmd);
	}
	return 0;
}

static int ps_data_from_mcu(const struct pkt_header *head)
{
	switch (((pkt_ps_calibrate_data_req_t *)head)->subcmd) {
	case SUB_CMD_SELFCALI_REQ:
		return write_ps_sensor_offset_to_nv(
			((pkt_ps_calibrate_data_req_t *)head)->calibrate_data,
			head->length - SUBCMD_LEN);

	default:
		hwlog_err("uncorrect subcmd 0x%x\n",
			((pkt_ps_calibrate_data_req_t *)head)->subcmd);
	}
	return 0;
}

static void register_priv_notifier(enum sensor_detect_list s_id)
{
	switch (s_id) {
	case GYRO:
		register_mcu_event_notifier(TAG_GYRO, CMD_CMN_CONFIG_REQ, gyro_data_from_mcu);
		break;
	case MAG:
		register_mcu_event_notifier(TAG_MAG, CMD_CMN_CONFIG_REQ, mag_data_from_mcu);
		break;
	case PS:
		register_mcu_event_notifier(TAG_PS, CMD_CMN_CONFIG_REQ, ps_data_from_mcu);
		break;
	default:
		break;
	}
}

int sensor_set_cfg_data(void)
{
	int ret = 0;
	enum sensor_detect_list s_id;

	for (s_id = ACC; s_id < SENSOR_MAX; s_id++) {
		if (strlen(get_sensor_chip_info_address(s_id)) != 0) {
#ifdef CONFIG_CONTEXTHUB_SHMEM
			if (s_id != RPC) {
#endif
				send_parameter_to_mcu(s_id,
					SUB_CMD_SET_PARAMET_REQ);
				if (s_id == ALS) {
					struct als_device_info *info = NULL;

					info = als_get_device_info(TAG_ALS);
					if (info)
						info->send_para_flag = 1;
				}

				if (get_iom3_state() != IOM3_ST_RECOVERY)
					register_priv_notifier(s_id);
#ifdef CONFIG_CONTEXTHUB_SHMEM
			} else {
				ret = shmem_send(TAG_RPC, &rpc_data, sizeof(rpc_data));
				if (ret)
					hwlog_err("%s shmem_send error\n", __func__);
			}
#endif
		}
	}
	return ret;
}

static bool need_download_fw(uint8_t tag)
{
	return ((tag == TAG_KEY) || (tag == TAG_TOF) || (tag == TAG_CAP_PROX));
}

int sensor_set_fw_load(void)
{
	int val = 1;
	int ret;
	struct write_info pkg_ap;
	pkt_parameter_req_t cpkt;
	struct pkt_header *hd = (struct pkt_header *)&cpkt;
	enum sensor_detect_list s_id;

	hwlog_info("write fw dload\n");
	for (s_id = ACC; s_id < SENSOR_MAX; s_id++) {
		if (strlen(get_sensor_chip_info_address(s_id)) != 0) {
			if (need_download_fw(sensor_manager[s_id].tag)) {
				pkg_ap.tag = sensor_manager[s_id].tag;
				pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
				cpkt.subcmd = SUB_CMD_FW_DLOAD_REQ;
				pkg_ap.wr_buf = &hd[1];
				pkg_ap.wr_len = sizeof(val) + SUBCMD_LEN;
				memcpy(cpkt.para, &val, sizeof(val));
				ret = write_customize_cmd(&pkg_ap, NULL, false);
				hwlog_info("write %d fw dload\n", sensor_manager[s_id].tag);
			}
		}
	}
	return 0;
}

static void redetect_sensor_work_handler(struct work_struct *wk)
{
	__pm_stay_awake(sensor_rd);
	redetect_failed_sensors(REDETECT_LATER);

	if (s_redetect_state.need_recovery == 1) {
		s_redetect_state.need_recovery = 0;
		hwlog_info("%s: some sensor detect success after %d redetect, begin recovery\n",
			__func__, s_redetect_state.redetect_num);
		iom3_need_recovery(SENSORHUB_USER_MODID, SH_FAULT_REDETECT);
	} else {
		hwlog_info("%s: no sensor redetect success\n", __func__);
	}
	__pm_relax(sensor_rd);
}

void sensor_redetect_enter(void)
{
	if (get_iom3_state() == IOM3_ST_NORMAL) {
		if (s_redetect_state.need_redetect_sensor == 1) {
			if (s_redetect_state.redetect_num < MAX_REDETECT_NUM) {
				queue_work(system_power_efficient_wq, &redetect_work);
				s_redetect_state.redetect_num++;
			} else {
				hwlog_info("%s: sensors detect fail, over max redetect num\n",
					__func__);
				show_last_detect_fail_sensor();
			}
		}
	}
}

void sensor_redetect_init(void)
{
	memset(&s_redetect_state, 0, sizeof(s_redetect_state));
	als_detect_init(sensor_manager, SENSOR_MAX);
	cap_prox_detect_init(sensor_manager, SENSOR_MAX);
	handpress_detect_init(sensor_manager, SENSOR_MAX);
	motion_detect_init(sensor_manager, SENSOR_MAX);
	tof_detect_init(sensor_manager, SENSOR_MAX);
	vibrator_detect_init(sensor_manager, SENSOR_MAX);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	sensor_rd = wakeup_source_register(NULL, "sensorhub_redetect");
#else
	sensor_rd = wakeup_source_register("sensorhub_redetect");
#endif
	INIT_WORK(&redetect_work, redetect_sensor_work_handler);
}
