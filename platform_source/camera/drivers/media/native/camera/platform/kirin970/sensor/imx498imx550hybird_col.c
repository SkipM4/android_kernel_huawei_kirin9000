/*
 *  SOC camera driver source file
 *
 *  Copyright (C) Huawei Technology Co., Ltd.
 *
 * Date:       2017-6-26
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

#include "../pmic/hw_pmic.h"

//lint -save -e846 -e866 -e826 -e785 -e838 -e715 -e747 -e774 -e778 -e732 -e731
//lint -save -e514 -e30 -e84 -e64 -e650 -e737 -e31 -e64 -esym(528,*) -esym(753,*)
#define MASTER_SENSOR_INDEX  (0)
#define SLAVE_SENSOR_INDEX   (2)
#define DELAY_1MS            (1)
#define DELAY_0MS            (0)
#define CTL_RESET_HOLD       (0)
#define CTL_RESET_RELEASE    (1)

#define I2S(i) container_of((i), sensor_t, intf)
#define Sensor2Pdev(s) container_of((s).dev, struct platform_device, dev)
static bool power_on_status = false; //false for power down, ture for power up

struct mutex imx498imx550hybird_col_power_lock;

static struct platform_device *s_pdev = NULL;
static sensor_t *s_sensor = NULL;

static struct sensor_power_setting imx498imx550hybird_power_setting [] = {

    //M0 AVDD0  2.80V  [PMU-LDO33]
    {
        .seq_type = SENSOR_AVDD,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M1 AVDD1  2.80V  [PMU-LDO13]
    {
        .seq_type = SENSOR_AVDD2,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 DVDD0  1.1V  [PMU-LDO19]
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_V1P1V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M1 DVDD1  1.1V  [PMU-LDO20]
    {
        .seq_type = SENSOR_DVDD2,
        .config_val = LDO_VOLTAGE_V1P1V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 + M1 AFVDD 2.85V [PMU-LDO16]
    {
        .seq_type = SENSOR_VCM_AVDD,
        .config_val = LDO_VOLTAGE_V2P85V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 + M1  IOVDD  1.8V  [PMU-LDO9]
    {
        .seq_type = SENSOR_IOVDD,
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 MCLK [ISP-CLK0]
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = MASTER_SENSOR_INDEX,
        .delay = DELAY_1MS,
    },

    //M1 MCLK [ISP-CLK2]
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SLAVE_SENSOR_INDEX,
        .delay = DELAY_1MS,
    },

    //M0 RESET  [GPIO_013]
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,//pull up
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },

    //M1 RESET  [GPIO_136]
    {
        .seq_type = SENSOR_RST2,
        .config_val = SENSOR_GPIO_LOW,//pull up
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },
};

static char const*
imx498imx550hybird_get_name(
        hwsensor_intf_t* si)
{
    sensor_t* sensor = NULL;
    if(NULL == si)
    {
        cam_err("%s. si is NULL.", __func__);
        return NULL;
    }

    sensor = I2S(si);

    if (NULL == sensor || NULL == sensor->board_info) {
        cam_err("%s. sensor or sensor->board_info is NULL.", __func__);
        return NULL;
    }

    return sensor->board_info->name;
}

static int
imx498imx550hybird_power_up(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;
    if(NULL == si)
    {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);

    if (NULL == sensor || NULL == sensor->board_info) {
        cam_err("%s. sensor or sensor->board_info is NULL.", __func__);
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
imx498imx550hybird_power_down(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;
    if(NULL == si)
    {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);

    if (NULL == sensor || NULL == sensor->board_info) {
        cam_err("%s. sensor or sensor->board_info is NULL.", __func__);
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

static int imx498imx550hybird_csi_enable(hwsensor_intf_t* si)
{
    return 0;
}

static int imx498imx550hybird_csi_disable(hwsensor_intf_t* si)
{
    return 0;
}

static int
imx498imx550hybird_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = NULL;
    struct sensor_cfg_data *cdata = NULL;
    if(NULL == si || NULL == data)
    {
        cam_err("%s. si or data is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
    cdata = (struct sensor_cfg_data *)data;

    if (NULL == sensor || NULL == sensor->board_info || NULL == sensor->board_info->name) {
        cam_err("%s. sensor or board_info or name is NULL.", __func__);
        return -EINVAL;
    }

    cam_info("%s name:%s", __func__, sensor->board_info->name);

    memset(cdata->cfg.name, 0, DEVICE_NAME_SIZE);
    strncpy(cdata->cfg.name, sensor->board_info->name, DEVICE_NAME_SIZE-1);
    cdata->data = sensor->board_info->sensor_index;

    return 0;
}

enum camera_metadata_enum_android_hw_dual_primary_mode{
    ANDROID_HW_DUAL_PRIMARY_FIRST  = 0,
    ANDROID_HW_DUAL_PRIMARY_SECOND = 2,
    ANDROID_HW_DUAL_PRIMARY_BOTH   = 3,
};
static int imx498imx550hybird_do_hw_reset(hwsensor_intf_t* si, int ctl, int id)
{
//lint -save -e826 -e778 -e774 -e747
    sensor_t* sensor = I2S(si);
    hwsensor_board_info_t *b_info;
    int ret = 0;

    if((NULL == sensor) || (NULL == sensor->board_info)) {
        cam_warn("%s invalid sensor or board info", __func__);
        return 0;
    }

    b_info = sensor->board_info;
    ret = gpio_request(b_info->gpios[RESETB2].gpio, "imx350reset-1");
    if (ret) {
        cam_err("%s requeset reset2 pin failed", __func__);
        return ret;
    }

    if (CTL_RESET_HOLD == ctl) {
        if ((ANDROID_HW_DUAL_PRIMARY_SECOND == id) || (ANDROID_HW_DUAL_PRIMARY_BOTH == id)) {
            ret = gpio_direction_output(b_info->gpios[RESETB2].gpio, CTL_RESET_HOLD);
            cam_info("RESETB2 = CTL_RESET_HOLD");
        }
    } else if (CTL_RESET_RELEASE == ctl) {
        if ((ANDROID_HW_DUAL_PRIMARY_SECOND == id) || (ANDROID_HW_DUAL_PRIMARY_BOTH == id)) {
            ret = gpio_direction_output(b_info->gpios[RESETB2].gpio, CTL_RESET_RELEASE);
            cam_info("RESETB2 = CTL_RESET_RELEASE");
            hw_camdrv_msleep(2);
        }
    }

    gpio_free(b_info->gpios[RESETB2].gpio);

    if (ret) {
        cam_err("%s set reset pin failed", __func__);
    } else {
        cam_info("%s: set reset state=%d, mode=%d", __func__, ctl, id);
    }
//lint -restore
    return ret;
}

static int
imx498imx550hybird_config(
        hwsensor_intf_t* si,
        void  *argp)
{
    struct sensor_cfg_data *data = NULL;
    int ret =0;

    if ((NULL == si) || (NULL == argp) || (NULL == si->vtbl)) {
        cam_err("%s : si, argp or si->vtbl is null", __func__);
        return -EINVAL;
    }

    data = (struct sensor_cfg_data *)argp;
    cam_debug("imx498imx550hybird cfgtype = %d",data->cfgtype);
    switch(data->cfgtype){
        case SEN_CONFIG_POWER_ON:
            mutex_lock(&imx498imx550hybird_col_power_lock);
            if (NULL == si->vtbl->power_up)
            {
                cam_err("%s. si power_up is null", __func__);
                /*lint -e455 -esym(455,*)*/
                mutex_unlock(&imx498imx550hybird_col_power_lock);
                /*lint -e455 +esym(455,*)*/
                return -EINVAL;
            }
            if (!power_on_status)
            {
                ret = si->vtbl->power_up(si);
                if (0 == ret) {
                    power_on_status = true;
                } else {
                    cam_err("%s. power up fail.", __func__);
                }
            }
            /*lint -e455 -esym(455,*)*/
            mutex_unlock(&imx498imx550hybird_col_power_lock);
            /*lint -e455 +esym(455,*)*/
            break;
        case SEN_CONFIG_POWER_OFF:
            mutex_lock(&imx498imx550hybird_col_power_lock);
            if (NULL == si->vtbl->power_down)
            {
                cam_err("%s. si power_up is null", __func__);
                /*lint -e455 -esym(455,*)*/
                mutex_unlock(&imx498imx550hybird_col_power_lock);
                /*lint -e455 +esym(455,*)*/
                return -EINVAL;
            }
            if (power_on_status)
            {
                ret = si->vtbl->power_down(si);
                if (0 != ret) {
                    cam_err("%s. power down fail.", __func__);
                }
                power_on_status = false;
            }
            /*lint -e455 -esym(455,*)*/
            mutex_unlock(&imx498imx550hybird_col_power_lock);
            /*lint -e455 +esym(455,*)*/
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
            if (NULL == si->vtbl->match_id)
            {
                cam_err("%s. si power_up is null", __func__);
                return -EINVAL;
            }

            ret = si->vtbl->match_id(si,argp);
            break;
        case SEN_CONFIG_RESET_HOLD:
            ret = imx498imx550hybird_do_hw_reset(si, CTL_RESET_HOLD, data->mode);
            break;
        case SEN_CONFIG_RESET_RELEASE:
            ret = imx498imx550hybird_do_hw_reset(si, CTL_RESET_RELEASE, data->mode);
            break;
        default:
            cam_err("%s cfgtype(%d) is error", __func__, data->cfgtype);
            break;
    }
    return ret;
}

static hwsensor_vtbl_t s_imx498imx550hybird_vtbl =
{
    .get_name = imx498imx550hybird_get_name,
    .config = imx498imx550hybird_config,
    .power_up = imx498imx550hybird_power_up,
    .power_down = imx498imx550hybird_power_down,
    .match_id = imx498imx550hybird_match_id,
    .csi_enable = imx498imx550hybird_csi_enable,
    .csi_disable = imx498imx550hybird_csi_disable,
};

static sensor_t s_imx498imx550hybird =
{
    .intf = { .vtbl = &s_imx498imx550hybird_vtbl, },
    .power_setting_array = {
        .size = ARRAY_SIZE(imx498imx550hybird_power_setting),
        .power_setting = imx498imx550hybird_power_setting,
    },
};

static const struct of_device_id
s_imx498imx550hybird_dt_match[] =
{
    {
        .compatible = "huawei,imx498imx550hybird_col",
        .data = &s_imx498imx550hybird.intf,
    },
    { } /* terminate list */
};
MODULE_DEVICE_TABLE(of, s_imx498imx550hybird_dt_match);
/* platform driver struct */
static int32_t imx498imx550hybird_platform_probe(struct platform_device* pdev);
static int32_t imx498imx550hybird_platform_remove(struct platform_device* pdev);
static struct platform_driver
s_imx498imx550hybird_driver =
{
    .probe = imx498imx550hybird_platform_probe,
    .remove = imx498imx550hybird_platform_remove,
    .driver =
    {
        .name = "huawei,imx498imx550hybird_col",
        .owner = THIS_MODULE,
        .of_match_table = s_imx498imx550hybird_dt_match,
    },
};

static int32_t
imx498imx550hybird_platform_probe(
        struct platform_device* pdev)
{
    int rc = 0;
    const struct of_device_id *id = NULL;
    hwsensor_intf_t *intf = NULL;
    sensor_t *sensor = NULL;
    struct device_node *np = NULL;
    cam_info("enter %s ",__func__);

    if(NULL == pdev)
    {
        cam_err("%s pdev is NULL", __func__);
        return -EINVAL;
    }

    mutex_init(&imx498imx550hybird_col_power_lock);
    np = pdev->dev.of_node;
    if (NULL == np) {
        cam_err("%s of_node is NULL", __func__);
        return -ENODEV;
    }

    id = of_match_node(s_imx498imx550hybird_dt_match, np);
    if (NULL == id) {
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

static int32_t
imx498imx550hybird_platform_remove(
    struct platform_device * pdev)
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
    return 0;
}
static int __init
imx498imx550hybird_init_module(void)
{
    cam_info("enter %s",__func__);
    return platform_driver_probe(&s_imx498imx550hybird_driver,
            imx498imx550hybird_platform_probe);
}

static void __exit
imx498imx550hybird_exit_module(void)
{
    platform_driver_unregister(&s_imx498imx550hybird_driver);
}

module_init(imx498imx550hybird_init_module);
module_exit(imx498imx550hybird_exit_module);
MODULE_DESCRIPTION("imx498imx550hybird_col");
MODULE_LICENSE("GPL v2");
//lint -restore
