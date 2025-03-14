 /*
 *  camera driver source file
 *
 *  Copyright (C) Huawei Technology Co., Ltd.
 *
 * Date:	  2015-04-28
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>

#include "hwsensor.h"
#include "sensor_commom.h"


#define I2S(i) container_of(i, sensor_t, intf)
#define Sensor2Pdev(s) container_of((s).dev, struct platform_device, dev)
#define POWER_SETTING_DELAY_0 0
#define POWER_SETTING_DELAY_1 1

//lint -save -e846 -e866 -e826 -e785 -e838 -e715 -e747 -e774 -e778 -e732 -e731 -e569 -e650 -e31

extern struct hw_csi_pad hw_csi_pad;
static hwsensor_vtbl_t s_imx258_cmr2_vtbl;
static struct platform_device *s_pdev = NULL;
static sensor_t *s_sensor = NULL;

struct sensor_power_setting hw_imx258_cmr2_power_setting[] = {

	//set camera reset gpio 146 to low
/*	{
		.seq_type = SENSOR_SUSPEND,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},*/

	//enable gpio51 output iovdd 1.8v
	{
        .seq_type = SENSOR_IOVDD,
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_0,
        },

        {
        .seq_type = SENSOR_VCM_AVDD,
        .data = (void*)"cameravcm-vcc",
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_1,
        },

	//enable gpio34 output ois af vdd 2.95v
	{
		.seq_type = SENSOR_VCM_PWDN,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM AVDD VOUT19 2.8V
	{
		.seq_type = SENSOR_AVDD2,
		.data = (void*)"main-sensor-avdd",
		.config_val = LDO_VOLTAGE_V2P8V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM0 DVDD VLDO1.2V
	{
		.seq_type = SENSOR_DVDD2,
		.data = (void*)"main-sensor-dvdd",
		.config_val = LDO_VOLTAGE_1P2V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM ISP_CLK0 16M
	{
		.seq_type = SENSOR_MCLK,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},

	//MCAM Reset GPIO_052
	{
		.seq_type = SENSOR_RST,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},
};

struct sensor_power_setting hw_imx258_cmr2_power_setting_v4[] = {

	//SCAM1 Reset
	{
		.seq_type = SENSOR_SUSPEND,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//SCAM DVDD 1.2V
	{
		.seq_type = SENSOR_DVDD,
		.config_val = LDO_VOLTAGE_1P2V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},

	//enable gpio51 output iovdd 1.8v
	{
		.seq_type = SENSOR_IOVDD,
		.config_val = LDO_VOLTAGE_1P8V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	{
		.seq_type = SENSOR_VCM_AVDD,
		.data = (void*)"cameravcm-vcc",
		.config_val = LDO_VOLTAGE_V2P8V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},

	//enable gpio34 output ois af vdd 2.95v
	{
		.seq_type = SENSOR_VCM_PWDN,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM AVDD VOUT19 2.8V
	{
		.seq_type = SENSOR_AVDD2,
		.data = (void*)"main-sensor-avdd",
		.config_val = LDO_VOLTAGE_V2P8V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM0 DVDD VLDO1.2V
	{
		.seq_type = SENSOR_DVDD2,
		.data = (void*)"main-sensor-dvdd",
		.config_val = LDO_VOLTAGE_1P2V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM ISP_CLK0 16M
	{
		.seq_type = SENSOR_MCLK,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},

	//MCAM Reset GPIO_052
	{
		.seq_type = SENSOR_RST,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},
};

static sensor_t s_imx258_cmr2 =
{
    .intf = { .vtbl = &s_imx258_cmr2_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hw_imx258_cmr2_power_setting),
            .power_setting = hw_imx258_cmr2_power_setting,
     },
};

static sensor_t s_imx258_cmr2_v4 =
{
    .intf = { .vtbl = &s_imx258_cmr2_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hw_imx258_cmr2_power_setting_v4),
            .power_setting = hw_imx258_cmr2_power_setting_v4,
     },
};


static const struct of_device_id
s_imx258_cmr2_dt_match[] =
{
    {
        .compatible = "huawei,imx258",
        .data = &s_imx258_cmr2.intf,
    },
    {
        .compatible = "huawei,imx258_v4",
        .data = &s_imx258_cmr2_v4.intf,
    },
    {
    },
};

MODULE_DEVICE_TABLE(of, s_imx258_cmr2_dt_match);

static struct platform_driver
s_imx258_cmr2_driver =
{
	.driver =
    {
		.name = "huawei,imx258",
		.owner = THIS_MODULE,
		.of_match_table = s_imx258_cmr2_dt_match,
	},
};

char const*
imx258_cmr2_get_name(
        hwsensor_intf_t* si)
{
    sensor_t* sensor = I2S(si);
    return sensor->board_info->name;
}
static int
imx258_cmr2_power_up(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;

    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
    if (NULL == sensor || NULL == sensor->board_info || NULL == sensor->board_info->name) {
        cam_err("%s. sensor or board_info->name is NULL.", __func__);
        return -EINVAL;
    }
    cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);

    if (hw_is_fpga_board()) {
        ret = do_sensor_power_on(sensor->board_info->sensor_index, sensor->board_info->name);
    } else {
        ret = hw_sensor_power_up(sensor);
    }
    if (0 == ret )
    {
        cam_info("%s. power up sensor success.", __func__);
    }
    else
    {
        cam_err("%s. power up sensor fail.", __func__);
    }
    return ret;
}

static int
imx258_cmr2_power_down(
        hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
    if (NULL == sensor || NULL == sensor->board_info || NULL == sensor->board_info->name) {
        cam_err("%s. sensor or board_info->name is NULL.", __func__);
        return -EINVAL;
    }
	cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);
	if (hw_is_fpga_board()) {
		ret = do_sensor_power_off(sensor->board_info->sensor_index, sensor->board_info->name);
	} else {
		ret = hw_sensor_power_down(sensor);
	}
    if (0 == ret )
    {
        cam_info("%s. power down sensor success.", __func__);
    }
    else
    {
        cam_err("%s. power down sensor fail.", __func__);
    }

	return ret;
}

static int imx258_cmr2_csi_enable(hwsensor_intf_t* si)
{

    return 0;
}

static int imx258_cmr2_csi_disable(hwsensor_intf_t* si)
{

    return 0;
}

static int
imx258_cmr2_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = NULL;
    struct sensor_cfg_data *cdata = NULL;

    cam_info("%s enter.", __func__);

    if ((NULL == si)||(NULL == data)) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
    if ((NULL == sensor->board_info) || (NULL == sensor->board_info->name)){
        cam_err("%s. sensor->board_info or sensor->board_info->name is NULL .", __func__);
        return -EINVAL;
    }
    cdata  = (struct sensor_cfg_data *)data;
    cdata->data = sensor->board_info->sensor_index;
    cam_info("%s name:%s", __func__, sensor->board_info->name);

    return 0;
}

static int
imx258_cmr2_config(
        hwsensor_intf_t* si,
        void  *argp)
{
	struct sensor_cfg_data *data;

	int ret =0;
	static bool imx258_cmr2_power_on = false;

    if (NULL == si || NULL == argp || NULL == si->vtbl) {
        cam_err("%s : si or argp or si->vtbl is null", __func__);
        return -EINVAL;
    }

	data = (struct sensor_cfg_data *)argp;
	cam_debug("imx258_cmr2 cfgtype = %d",data->cfgtype);
	switch(data->cfgtype){
		case SEN_CONFIG_POWER_ON:
			if (!imx258_cmr2_power_on) {
				ret = si->vtbl->power_up(si);
				imx258_cmr2_power_on = true;
			}
			break;
		case SEN_CONFIG_POWER_OFF:
			if (imx258_cmr2_power_on) {
				ret = si->vtbl->power_down(si);
				imx258_cmr2_power_on = false;
			}
			break;
		case SEN_CONFIG_WRITE_REG:
			break;
		case SEN_CONFIG_READ_REG:
			break;
		case SEN_CONFIG_WRITE_REG_SETTINGS:
			break;
		case SEN_CONFIG_READ_REG_SETTINGS:
			break;
		case SEN_CONFIG_ENABLE_CSI:
			break;
		case SEN_CONFIG_DISABLE_CSI:
			break;
		case SEN_CONFIG_MATCH_ID:
			ret = si->vtbl->match_id(si,argp);
			break;
		default:
            cam_err("%s cfgtype(%d) is error", __func__, data->cfgtype);
			break;
	}
	cam_debug("%s exit",__func__);
	return ret;
}

static hwsensor_vtbl_t
s_imx258_cmr2_vtbl =
{
	.get_name = imx258_cmr2_get_name,
	.config = imx258_cmr2_config,
	.power_up = imx258_cmr2_power_up,
	.power_down = imx258_cmr2_power_down,
	.match_id = imx258_cmr2_match_id,
	.csi_enable = imx258_cmr2_csi_enable,
	.csi_disable = imx258_cmr2_csi_disable,
};
static int32_t
imx258_cmr2_platform_probe(
        struct platform_device* pdev)
{
	int rc = 0;
    const struct of_device_id *id = NULL;
    hwsensor_intf_t *intf = NULL;
    sensor_t *sensor = NULL;
    struct device_node *np = NULL;
    cam_notice("enter %s",__func__);

    if (NULL == pdev) {
        cam_err("%s pdev is NULL", __func__);
        return -EINVAL;
    }

    np = pdev->dev.of_node;
    if (NULL == np) {
        cam_err("%s of_node is NULL", __func__);
        return -ENODEV;
    }

    id = of_match_node(s_imx258_cmr2_dt_match, np);
    if (!id) {
        cam_err("%s none id matched", __func__);
        return -ENODEV;
    }

    intf = (hwsensor_intf_t*)id->data;
    if (NULL == intf) {
        cam_err("%s intf is NULL", __func__);
        return -ENODEV;
    }
    sensor = I2S(intf);
    if(NULL == sensor){
        cam_err("%s sensor is NULL rc %d", __func__, rc);
        return -ENODEV;
    }
    rc = hw_sensor_get_dt_data(pdev, sensor);
    if (rc < 0) {
        cam_err("%s no dt data rc %d", __func__, rc);
        return -ENODEV;
    }
    sensor->dev = &pdev->dev;

    rc = hwsensor_register(pdev, intf);
    if (rc < 0) {
        cam_err("%s hwsensor_register failed rc %d\n", __func__, rc);
        return -ENODEV;
    }
    s_pdev = pdev;
    rc = rpmsg_sensor_register(pdev, (void*)sensor);
    if (rc < 0) {
        hwsensor_unregister(s_pdev);
        s_pdev = NULL;
        cam_err("%s rpmsg_sensor_register failed rc %d\n", __func__, rc);
        return -ENODEV;
    }
    s_sensor = sensor;

    return rc;
}

static int __init
imx258_cmr2_init_module(void)
{
    cam_notice("enter %s",__func__);
    return platform_driver_probe(&s_imx258_cmr2_driver,
            imx258_cmr2_platform_probe);
}

static void __exit
imx258_cmr2_exit_module(void)
{
    if( NULL != s_sensor)
    {
        rpmsg_sensor_unregister((void*)s_sensor);
        s_sensor = NULL;
    }
    if (NULL != s_pdev) {
        hwsensor_unregister(s_pdev);
        s_pdev = NULL;
    }
    platform_driver_unregister(&s_imx258_cmr2_driver);
}

module_init(imx258_cmr2_init_module);
module_exit(imx258_cmr2_exit_module);
MODULE_DESCRIPTION("imx258_cmr2");
MODULE_LICENSE("GPL v2");
//lint -restore

