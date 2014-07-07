/* Copyright (c) 2009-2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/export.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>

#include "adp1660.h"

#define ADP1660_TEST 0
#define ENABLE_VREG_LVS2 0

#ifdef debug_flash
#ifdef CDBG
#undef CDBG
#endif
#define CDBG(fmt, args...) pr_info(fmt, ##args)
#endif

/* Modify them for changing default flash driver ic output current */
#define DEFAULT_FLASH_HIGH_CURRENT	ADP1660_I_FL_350mA
#define DEFAULT_FLASH_LOW_CURRENT	ADP1660_I_FL_300mA
#define DEFAULT_TORCH_CURRENT		ADP1660_I_TOR_100mA

#define FLASH_DRIVER_NAME "adp1660"
#define COMPATIBLE_NAME "qcom,flash-adp1660"


enum flash_gpio_type {
        FLASH_EN_GPIO,
        FLASH_TX_GPIO,
        FLASH_STR_GPIO,
        FLASH_MAX_GPIO,
};

struct flash_gpio {
	unsigned gpio;
	unsigned value;
	char gpio_name[32];
};

struct flash_platform_data {
        /* Data filled from device tree nodes */
        struct flash_gpio *gpios[FLASH_MAX_GPIO];
	struct regulator *reg_8941_lvs2;
};

struct adp1660_data {
        int chip_id;
        struct work_struct irq_work;
        struct workqueue_struct *wq;
        struct i2c_client *i2c_handle;
        struct flash_platform_data *pdata;
};

static struct i2c_client *adp1660_client;

static struct adp1660_data *flash_ctrl;
static struct i2c_driver adp1660_i2c_driver;

static int32_t adp1660_i2c_txdata(unsigned short saddr, unsigned char *txdata,
					int length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = length,
		 .buf = txdata,
		 },
	};

	if (i2c_transfer(adp1660_client->adapter, msg, 1) < 0) {
		pr_err("%s failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int32_t adp1660_i2c_write_b(unsigned short saddr, unsigned short baddr,
					unsigned short bdata)
{
	int32_t rc = -EIO;
	unsigned char buf[2];

	memset(buf, 0, sizeof(buf));
	buf[0] = baddr;
	buf[1] = bdata;
	rc = adp1660_i2c_txdata(saddr, buf, 2);

	if (rc < 0)
		pr_err("%s failed, saddr = 0x%x, baddr = 0x%x,\
			 bdata = 0x%x\n", __func__, saddr, baddr, bdata);

	return rc;
}

static int32_t adp1660_i2c_rxdata(unsigned short saddr, unsigned char *rxdata,
					int length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = 1,
		 .buf = rxdata,
		 },
		{
		 .addr  = saddr,
		 .flags = I2C_M_RD,
		 .len   = length,
		 .buf   = rxdata,
		},
	};

	if (i2c_transfer(adp1660_client->adapter, msg, 2) < 0) {
		pr_err("%s failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int32_t adp1660_i2c_read_b(unsigned short saddr, unsigned short baddr,unsigned short *bdata)
{
	int32_t rc = -EIO;
	unsigned char buf[1];

	buf[0] = baddr;
	rc = adp1660_i2c_rxdata(saddr, buf, 1);

	*bdata = buf[0];
	if (rc < 0)
		pr_err("%s failed, saddr = 0x%x, baddr = 0x%x,\
			 bdata = 0x%x!\n", __func__, saddr, baddr, *bdata);
	return rc;
}

static int32_t adp1660_set_drv_ic_output_torch(uint32_t torch)
{
	int32_t rc = -EIO;

	CDBG("%s called.\n", __func__);
	rc = adp1660_i2c_write_b(adp1660_client->addr,
				 ADP1660_REG_LED1_TORCH_ASSIST_SET, torch);
	if (rc < 0)
		pr_err("%s: Set ADP1660_REG_LED1_FLASH_CURRENT_SET failed\n", __func__);

	rc = adp1660_i2c_write_b(adp1660_client->addr,
				 ADP1660_REG_LED2_TORCH_ASSIST_SET, torch);
	if (rc < 0)
		pr_err("%s: Set ADP1660_REG_LED2_FLASH_CURRENT_SET failed\n", __func__);

	return rc;
}

static int32_t adp1660_set_drv_ic_output_flash(uint32_t flashc)
{
	int32_t rc = -EIO;

	CDBG("%s called.\n", __func__);
	rc = adp1660_i2c_write_b(adp1660_client->addr,
				 ADP1660_REG_LED1_FLASH_CURRENT_SET, flashc);
	if (rc < 0)
		pr_err("%s: Set ADP1660_REG_LED1_FLASH_CURRENT_SET failed\n", __func__);

	rc = adp1660_i2c_write_b(adp1660_client->addr,
				 ADP1660_REG_LED2_FLASH_CURRENT_SET, flashc);
	if (rc < 0)
		pr_err("%s: Set ADP1660_REG_LED2_FLASH_CURRENT_SET failed\n", __func__);

	return rc;
}

int32_t adp1660_flash_mode_control(int ctrl)
{
	int32_t rc = -EIO;
	unsigned short reg = 0, data = 0;

	CDBG("%s called. case(%d)\n", __func__, ctrl);
	reg = ADP1660_REG_OUTPUT_MODE;
	data = (ADP1660_IL_PEAK_3P25A << ADP1660_IL_PEAK_SHIFT) |
	       (ADP1660_STR_LV_LEVEL_SENSITIVE << ADP1660_STR_LV_SHIFT) |
	       (ADP1660_STR_MODE_SW << ADP1660_STR_MODE_SHIFT);

	switch (ctrl) {
	case MSM_CAMERA_LED_OFF:
		/* Disable flash light output */
		data |= ADP1660_LED_MODE_STANDBY;
		rc = adp1660_i2c_write_b(adp1660_client->addr, reg, data);
		if (rc < 0)
			pr_err("%s: Disable flash light failed\n", __func__);

		rc = adp1660_i2c_write_b(adp1660_client->addr,
					 ADP1660_REG_LED_Enable_Mode, ADP1660_LED_ALL_EN_Disable);
		if (rc < 0)
			pr_err("%s: ADP1660_LED_ALL_EN_Disable failed\n", __func__);

		break;

	case MSM_CAMERA_LED_HIGH:
		/* Enable flash light output at high current (750 mA)*/
		adp1660_set_drv_ic_output_flash(DEFAULT_FLASH_HIGH_CURRENT);
		data |= ADP1660_LED_MODE_FLASH;
		rc = adp1660_i2c_write_b(adp1660_client->addr, reg, data);
		if (rc < 0)
			pr_err("%s: Enable flash light failed\n", __func__);

		rc = adp1660_i2c_write_b(adp1660_client->addr,
					 ADP1660_REG_LED_Enable_Mode, ADP1660_LED_ALL_EN_Enable);
		if (rc < 0)
			pr_err("%s: ADP1660_LED_ALL_EN_Enable failed\n", __func__);

		break;

	case MSM_CAMERA_LED_LOW:
		/* Enable flash light output at low current (300 mA)*/
		adp1660_set_drv_ic_output_flash(DEFAULT_FLASH_LOW_CURRENT);
		data |= ADP1660_LED_MODE_FLASH;
		rc = adp1660_i2c_write_b(adp1660_client->addr, reg, data);
		if (rc < 0)
			pr_err("%s: Enable flash light failed\n", __func__);

		rc = adp1660_i2c_write_b(adp1660_client->addr,
					 ADP1660_REG_LED_Enable_Mode, ADP1660_LED_ALL_EN_Enable);
		if (rc < 0)
			pr_err("%s: ADP1660_LED_ALL_EN_Enable failed\n", __func__);

		break;

	default:
		CDBG("%s: Illegal flash light parameter\n", __func__);
		break;
	}
	return rc;
}

int32_t adp1660_torch_mode_control(int ctrl)
{
	int32_t rc = -EIO;
	unsigned short reg = 0, data = 0;

	CDBG("%s called(%d).\n", __func__,ctrl);
	reg = ADP1660_REG_OUTPUT_MODE;
	data = (ADP1660_IL_PEAK_3P25A << ADP1660_IL_PEAK_SHIFT) |
	       (ADP1660_STR_LV_LEVEL_SENSITIVE << ADP1660_STR_LV_SHIFT) |
	       (ADP1660_STR_MODE_HW << ADP1660_STR_MODE_SHIFT) |
	       (ADP1660_STR_ACTIVE_HIGH << ADP1660_STR_POL_SHIFT);

	switch (ctrl) {
	case MSM_CAMERA_TORCH_OFF:
		CDBG("%s: MSM_CAMERA_LED_TORCH_OFF\n", __func__);
		/* Disable torch light output */
		data |= ADP1660_LED_MODE_STANDBY;
		rc = adp1660_i2c_write_b(adp1660_client->addr, reg, data);
		if (rc < 0)
			pr_err("%s: Disable torch light failed\n", __func__);

		rc = adp1660_i2c_write_b(adp1660_client->addr,
					 ADP1660_REG_LED_Enable_Mode, ADP1660_LED_ALL_EN_Disable);
		if (rc < 0)
			pr_err("%s: ADP1660_LED_ALL_EN_Disable failed\n", __func__);

		break;

	case MSM_CAMERA_TORCH_ON:
		CDBG("%s: MSM_CAMERA_LED_TORCH_ON\n", __func__);
		/* Enable torch light output */
		adp1660_set_drv_ic_output_torch(DEFAULT_TORCH_CURRENT);
		data |= ADP1660_LED_MODE_ASSIST;
		rc = adp1660_i2c_write_b(adp1660_client->addr, reg, data);
		if (rc < 0)
			pr_err("%s: Enable torch light failed\n", __func__);

		rc = adp1660_i2c_write_b(adp1660_client->addr,
					 ADP1660_REG_LED_Enable_Mode, ADP1660_LED_ALL_EN_Enable);
		if (rc < 0)
			pr_err("%s: ADP1660_LED_ALL_EN_Enable failed\n", __func__);

		break;

	default:
		CDBG("%s: Illegal torch light parameter\n", __func__);
		break;
	}

	return rc;
}

static int flash_get_dt_data(struct device *dev, struct flash_platform_data *pdata)
{
        int i, rc = 0;
        struct device_node *of_node = NULL;
        struct flash_gpio *temp_gpio = NULL;
        i = 0;

        if (!dev || !pdata) {
                pr_err("%s: invalid input\n", __func__);
                return -EINVAL;
        }

        of_node = dev->of_node;
        if (!of_node) {
                pr_err("%s: invalid of_node\n", __func__);
                goto error;
        }

        temp_gpio = NULL;
        temp_gpio = devm_kzalloc(dev, sizeof(struct flash_gpio), GFP_KERNEL);
        CDBG("%s: gpios allocd\n", __func__);
        if (!(temp_gpio)) {
                pr_err("%s: can't alloc %d gpio mem\n", __func__, i);
                goto error;
        }

	/*ENABLE*/
        temp_gpio->gpio = of_get_named_gpio(of_node, "flash-en-gpio", 0);
        snprintf(temp_gpio->gpio_name, 32, "%s", "flash-en-gpio");
        CDBG("%s: EN gpio=[%d]\n", __func__,
                 temp_gpio->gpio);
        pdata->gpios[FLASH_EN_GPIO] = temp_gpio;

	/*TXMASK*/
        temp_gpio = NULL;
        temp_gpio = devm_kzalloc(dev, sizeof(struct flash_gpio), GFP_KERNEL);
        CDBG("%s: gpios allocd\n", __func__);
        if (!(temp_gpio)) {
                pr_err("%s: can't alloc %d gpio mem\n", __func__, i);
                goto error;
        }
        temp_gpio->gpio = of_get_named_gpio(of_node, "flash-tx-gpio", 0);
        snprintf(temp_gpio->gpio_name, 32, "%s", "flash-tx-gpio");
        CDBG("%s: TXMASK gpio=[%d]\n", __func__,
                 temp_gpio->gpio);
        pdata->gpios[FLASH_TX_GPIO] = temp_gpio;

	/*STROBE*/
        temp_gpio = NULL;
        temp_gpio = devm_kzalloc(dev, sizeof(struct flash_gpio), GFP_KERNEL);
        CDBG("%s: gpios allocd\n", __func__);
        if (!(temp_gpio)) {
                pr_err("%s: can't alloc %d gpio mem\n", __func__, i);
                goto error;
        }
        temp_gpio->gpio = of_get_named_gpio(of_node, "flash-str-gpio", 0);
        snprintf(temp_gpio->gpio_name, 32, "%s", "flash-str-gpio");
        CDBG("%s: STROBE gpio=[%d]\n", __func__,
                 temp_gpio->gpio);
        pdata->gpios[FLASH_STR_GPIO] = temp_gpio;

        return 0;
error:
        pr_err("%s: ret due to err\n", __func__);
        for (i = 0; i < FLASH_MAX_GPIO; i++)
                if (pdata->gpios[i])
                        devm_kfree(dev, pdata->gpios[i]);
        return rc;
}

static int flash_gpio_config(bool on)
{
	int ret;
	struct flash_gpio *temp_en_gpio, *temp_str_gpio;

	temp_en_gpio = flash_ctrl->pdata->gpios[FLASH_EN_GPIO];
	temp_str_gpio = flash_ctrl->pdata->gpios[FLASH_STR_GPIO];

	CDBG("%s: enable=%d\n",__func__, on);
	if (on) {
		if (gpio_is_valid(temp_str_gpio->gpio)) {
			ret = gpio_request(temp_str_gpio->gpio, temp_str_gpio->gpio_name);
			if (ret < 0) {
				pr_err("%s:str_gpio=[%d] req failed:%d\n", __func__,
						temp_str_gpio->gpio, ret);
				return -EBUSY;
			}

			ret = gpio_direction_output(temp_str_gpio->gpio, 0);
			if (ret < 0) {
				pr_err("%s: set str_gpio failed: %d\n", __func__, ret);
				return -EBUSY;
			}
		}

		if (gpio_is_valid(temp_en_gpio->gpio)) {
			ret = gpio_request(temp_en_gpio->gpio, temp_en_gpio->gpio_name);
			if (ret < 0) {
				pr_err("%s:en_gpio=[%d] req failed:%d\n", __func__,
						temp_en_gpio->gpio, ret);
				return -EBUSY;
			}

			ret = gpio_direction_output(temp_en_gpio->gpio, 1);
			if (ret < 0) {
				pr_err("%s: set en_gpio failed: %d\n", __func__, ret);
				return -EBUSY;
			}
		}
	} else {
		gpio_free(temp_en_gpio->gpio);
		gpio_free(temp_str_gpio->gpio);
	}
	return 0;
}
static int flash_vreg_config(struct i2c_client *client, bool enable)
{
        int rc = -ENODEV;

	CDBG("%s: enable=%d\n",__func__, enable);
	if (enable) {
                flash_ctrl->pdata->reg_8941_lvs2 = regulator_get(&client->dev,
                        "flash-vio");
                if (IS_ERR(flash_ctrl->pdata->reg_8941_lvs2)) {
                        pr_err("could not get reg_8941_lvs2, rc = %ld\n",
                                PTR_ERR(flash_ctrl->pdata->reg_8941_lvs2));
                        return -ENODEV;
                }
                rc = regulator_enable(flash_ctrl->pdata->reg_8941_lvs2);
                if (rc) {
                        pr_err("'%s' regulator configure[%u] failed, rc=%d\n",
                                 "flash vio", enable, rc);
                        return rc;
                } else {
                        CDBG("%s: vreg LVS2 %s\n",
                                 __func__, (enable ? "enabled" : "disabled"));
                }
	} else {
                rc = regulator_disable(flash_ctrl->pdata->reg_8941_lvs2);
                if (rc) {
                        pr_err("'%s' regulator configure[%u] failed, rc=%d\n",
                                 "flash vio", enable, rc);
                        return rc;
                } else {
                        CDBG("%s: vreg LVS2 %s\n",
                                 __func__, (enable ? "enabled" : "disabled"));
                }
                regulator_put(flash_ctrl->pdata->reg_8941_lvs2);
		flash_ctrl->pdata->reg_8941_lvs2 = NULL;
	}
        return rc;
}


int flash_set_state(int state)
{
	int rc = 0;
#if ENABLE_VREG_LVS2
        struct i2c_client *client = flash_ctrl->i2c_handle;
#endif

	CDBG("flash_set_state: %d\n", state);
	if (state == MSM_CAMERA_LED_INIT) {
		rc = flash_gpio_config(true);
		if (rc) {
			pr_err("%s: gpio enable failed\n", __func__);
			return -EINVAL;
		}
#if ENABLE_VREG_LVS2
		rc = flash_vreg_config(client, true);
		if (rc) {
			pr_err("%s: regulator enable failed\n", __func__);
		return -EINVAL;
		}
#endif
		msleep(20);
	}

	if (state != MSM_CAMERA_LED_INIT || state != MSM_CAMERA_LED_RELEASE) {
		if (state == MSM_CAMERA_TORCH_OFF || state == MSM_CAMERA_TORCH_ON)
			adp1660_torch_mode_control(state);
		else
			adp1660_flash_mode_control(state);
	}

	if (state == MSM_CAMERA_LED_RELEASE) {
#if ENABLE_VREG_LVS2
		flash_vreg_config(client, false);
#endif
		flash_gpio_config(false);
	}
	CDBG("flash_set_led_state: return %d\n", rc);
	return rc;

}
int32_t adp1660_information(void)
{
	int32_t rc = -EIO;
        struct i2c_client *client = flash_ctrl->i2c_handle;
        unsigned short read_data = 0;
	unsigned short device_id =0;
	unsigned short rev_id =0;

	rc = flash_gpio_config(true);
	if (rc) {
		pr_err("%s: gpio enable failed\n", __func__);
		return -EINVAL;
	}
	rc = flash_vreg_config(client, true);
	if (rc) {
		pr_err("%s: regulator enable failed\n", __func__);
	return -EINVAL;
	}
	msleep(20);

	rc = adp1660_i2c_read_b(adp1660_client->addr,0x0,&read_data);
	if (rc < 0)
		pr_err("%s: Read design information failed\n", __func__);

	CDBG("%s: reg 0x%x = 0x%x\n", __func__, 0x0, read_data);

	device_id = (read_data >> 3) & 0x1F;
	rev_id = read_data & 0x7;
	pr_info("%s: adp1660 DEVICE_ID 0x%x; REV_ID 0x%x\n", __func__, device_id, rev_id);

	flash_vreg_config(client, false);
	flash_gpio_config(false);

	return rc;
}

static int adp1660_i2c_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
        int rc = 0;
        struct flash_platform_data *pdata = NULL;

        CDBG("%s", __func__);

        flash_ctrl = devm_kzalloc(&client->dev, sizeof(*flash_ctrl), GFP_KERNEL);
        if (flash_ctrl == NULL) {
                pr_err("%s: no memory for driver data\n", __func__);
                rc = -ENOMEM;
                goto failed_no_mem;
        }

        if (client->dev.of_node) {
                pdata = devm_kzalloc(&client->dev,
                             sizeof(struct flash_platform_data), GFP_KERNEL);
                if (!pdata) {
                        dev_err(&client->dev, "Failed to allocate memory\n");
                        rc = -ENOMEM;
                        goto failed_no_mem;
                }

                rc = flash_get_dt_data(&client->dev, pdata);
                if (rc) {
                        pr_err("%s: FAILED: parsing device tree data; rc=%d\n",
                                __func__, rc);
                        goto failed_dt_data;
                }

                flash_ctrl->i2c_handle = client;
                flash_ctrl->pdata = pdata;
                i2c_set_clientdata(client, flash_ctrl);
        }

	adp1660_client = client;
	adp1660_information();
#if ADP1660_TEST
	flash_set_state(3);
	msleep(500);
	flash_set_state(1);
	msleep(500);
	flash_set_state(0);
	flash_set_state(4);
#endif
        pr_info("%s: adp1660 probe succeeded\n", __func__);
        return 0;

failed_dt_data:
        if (pdata)
                devm_kfree(&client->dev, pdata);
failed_no_mem:
        if (flash_ctrl)
                devm_kfree(&client->dev, flash_ctrl);
        pr_err("%s: PROBE FAILED, rc=%d\n", __func__, rc);
        return rc;
}


static int adp1660_i2c_remove(struct i2c_client *client)
{
        if (!flash_ctrl) {
                pr_warn("%s: i2c get client data failed\n", __func__);
                return -EINVAL;
        }

	if (flash_ctrl->pdata)
                devm_kfree(&client->dev, flash_ctrl->pdata);
        devm_kfree(&client->dev, flash_ctrl);

        return 0;
}

static const struct i2c_device_id adp1660_i2c_id[] = {
	{FLASH_DRIVER_NAME, 0 },
	{ }
};


MODULE_DEVICE_TABLE(i2c, adp1660_i2c_id);

static struct of_device_id adp1660_match_table[] = {
        {.compatible = COMPATIBLE_NAME,},
        { },
};


static struct i2c_driver adp1660_i2c_driver = {
        .driver = {
                .name = FLASH_DRIVER_NAME,
                .owner = THIS_MODULE,
                .of_match_table = adp1660_match_table,
        },
	.probe  = adp1660_i2c_probe,
	.remove = adp1660_i2c_remove,
	.id_table = adp1660_i2c_id,
};

module_i2c_driver(adp1660_i2c_driver);

MODULE_DESCRIPTION("ADP1660 FLASH");
MODULE_LICENSE("GPL v2");
