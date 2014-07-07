/*
 * cyttsp4_i2c.c
 * Cypress TrueTouch(TM) Standard Product V4 I2C Driver module.
 * For use with Cypress Txx4xx parts.
 * Supported parts include:
 * TMA4XX
 * TMA1036
 *
 * Copyright (C) 2012 Cypress Semiconductor
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * Author: Aleksej Makarov <aleksej.makarov@sonyericsson.com>
 * Modified by: Cypress Semiconductor for test with device
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#include <linux/cyttsp4_bus.h>
#include "cyttsp4_i2c.h"
#include "cyttsp4_core.h"
#include "cyttsp4_mt.h"
#include "cyttsp4_btn.h"

#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include "../../../../arch/arm/mach-msm/acer_hw_version.h"


#define CY_I2C_DATA_SIZE  (3 * 256)

struct cyttsp4_i2c {
	struct i2c_client *client;
	u8 wr_buf[CY_I2C_DATA_SIZE];
	struct hrtimer timer;
	struct mutex lock;
	atomic_t timeout;
};

/*==============================added===================================*/

static struct regulator **ts_vdd;

static struct cypress_ts_regulator a12_ts_regulator_data[] = {
       {
               .name = "vdd_ana",
               .min_uV = VERG_L18_VTG_MIN_UV,
               .max_uV = VERG_L18_VTG_MAX_UV,
               .load_uA = VERG_L18_CURR_24HZ_UA,
       },
       /*  TODO: Remove after runtime PM is enabled in I2C drive */
       {
               .name = "vcc_i2c",
               .min_uV = VERG_LVS1_VTG_MIN_UV,
               .max_uV = VERG_LVS1_VTG_MIN_UV,
               .load_uA = VERG_LVS1_CURR_UA,
       },
};

#define CY_MAXX 1080
#define CY_MAXY 2016
#define CY_MINX 0
#define CY_MINY 0

#define CY_ABS_MIN_X CY_MINX
#define CY_ABS_MIN_Y CY_MINY
#define CY_ABS_MAX_X CY_MAXX
#define CY_ABS_MAX_Y CY_MAXY
#define CY_ABS_MIN_P 0
#define CY_ABS_MIN_W 0
#define CY_ABS_MAX_P 255
#define CY_ABS_MAX_W 255

#define CY_ABS_MIN_T 0

#define CY_ABS_MAX_T 15

#define CY_IGNORE_VALUE 0xFFFF

static const uint16_t cyttsp4_abs[] = {
	ABS_MT_POSITION_X, CY_ABS_MIN_X, CY_ABS_MAX_X, 0, 0,
	ABS_MT_POSITION_Y, CY_ABS_MIN_Y, CY_ABS_MAX_Y, 0, 0,
	ABS_MT_PRESSURE, CY_ABS_MIN_P, CY_ABS_MAX_P, 0, 0,
	CY_IGNORE_VALUE, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
	ABS_MT_TRACKING_ID, CY_ABS_MIN_T, CY_ABS_MAX_T, 0, 0,
};

struct touch_framework cyttsp4_framework = {
	.abs = (uint16_t *)&cyttsp4_abs[0],
	.size = ARRAY_SIZE(cyttsp4_abs),
	.enable_vkeys = 0,
};

static struct cyttsp4_btn_platform_data _cyttsp4_btn_platform_data = {
	.inp_dev_name = CYTTSP4_BTN_NAME,
};

struct cyttsp4_device cyttsp4_btn_device = {
	.name = CYTTSP4_BTN_NAME,
	.core_id = "main_ttsp_core",
	.dev = {
			.platform_data = &_cyttsp4_btn_platform_data,
	}
};

static struct cyttsp4_mt_platform_data _cyttsp4_mt_platform_data = {
	.frmwrk = &cyttsp4_framework,
	.flags = 0,
	.inp_dev_name = CYTTSP4_MT_NAME,
};

struct cyttsp4_device cyttsp4_mt_device = {
	.name = CYTTSP4_MT_NAME,
	.core_id = "main_ttsp_core",
	.dev = {
		.platform_data = &_cyttsp4_mt_platform_data,
	}
};

/* Button to keycode conversion */
static u16 cyttsp4_btn_keys[] = {
	/* use this table to map buttons to keycodes (see input.h) */
	KEY_HOME,			/* 102 */
	KEY_MENU,			/* 139 */
	KEY_BACK,			/* 158 */
	KEY_SEARCH,			/* 217 */
	KEY_VOLUMEDOWN,		/* 114 */
	KEY_VOLUMEUP,		/* 115 */
	KEY_CAMERA,			/* 212 */
	KEY_POWER			/* 116 */
};

static struct touch_settings cyttsp4_sett_btn_keys = {
	.data = (uint8_t *)&cyttsp4_btn_keys[0],
	.size = ARRAY_SIZE(cyttsp4_btn_keys),
	.tag = 0,
};

/**reset function*/
static int cyttsp4_xres(struct cyttsp4_core_platform_data *pdata, struct device *dev)
{
	int rc = 0;
	gpio_set_value(A12_GPIO_CYP_TS_RESET_N, 1);
	msleep(20);
	gpio_set_value(A12_GPIO_CYP_TS_RESET_N, 0);
	msleep(40);
	gpio_set_value(A12_GPIO_CYP_TS_RESET_N, 1);
	msleep(20);
	dev_info(dev,
		"%s: RESET CYTTSP gpio=%d r=%d\n", __func__,
		A12_GPIO_CYP_TS_RESET_N, rc);
	return rc;
}
/**wake up function*/
static int cyttsp4_wakeup(struct device *dev)
{
	int rc = 0;
	int i = 0;
	u8 num_reg = 0;
	const struct cypress_ts_regulator *reg_info = NULL;
	int ts_gpio_interrupt = 0;

	num_reg = ARRAY_SIZE(a12_ts_regulator_data);
	reg_info = a12_ts_regulator_data;
	i = num_reg;

	if ((acer_board_id == HW_ID_EVB1) || (acer_board_id == HW_ID_EVB2)){
		ts_gpio_interrupt = A12_GPIO_CYP_TS_INT_N_EVB;
	}
	else if ((acer_board_id == HW_ID_EVT1) || (acer_board_id == HW_ID_EVT2)){
		ts_gpio_interrupt = A12_GPIO_CYP_TS_INT_N_EVT;
	}

	dev_info(dev,
		"%s enable ts_vdd\n", __func__);
	while (--i >= 0) {
		regulator_enable(ts_vdd[i]);
		rc = regulator_set_optimum_mode(ts_vdd[i], reg_info[i].load_uA);
		if (rc < 0) {
			pr_err("%s: regulator_set_optimum_mode failed rc=%d\n",
				__func__, rc);
		}
	}

	rc = gpio_direction_output(ts_gpio_interrupt, 0);
	if (rc < 0) {
		dev_err(dev,
			"%s: Fail set output gpio=%d\n",
			__func__, ts_gpio_interrupt);
	} else {
		udelay(2000);
		rc = gpio_direction_input(ts_gpio_interrupt);
		if (rc < 0) {
			dev_err(dev,
				"%s: Fail set input gpio=%d\n",
				__func__, ts_gpio_interrupt);
		}
	}

	dev_info(dev,
		"%s: WAKEUP CYTTSP gpio=%d r=%d\n", __func__,
		ts_gpio_interrupt, rc);
	return rc;
}

/**sleep function*/
static int cyttsp4_sleep(struct device *dev)
{
	int i = 0;
	int rc = 0;
	u8 num_reg = 0;

	num_reg = ARRAY_SIZE(a12_ts_regulator_data);
	i = num_reg;

	dev_info(dev,
		"%s disable ts_vdd\n", __func__);
	while (--i >= 0) {
		regulator_disable(ts_vdd[i]);
		rc = regulator_set_optimum_mode(ts_vdd[i], 100);
		if (rc < 0) {
			pr_err("%s: regulator_set_optimum_mode failed rc=%d\n",
				__func__, rc);
		}
	}

	return 0;
}


/**Power Manager function*/
static int cyttsp4_power(struct cyttsp4_core_platform_data *pdata, int on,
	struct device *dev, atomic_t *ignore_irq)
{
	if (on)
		return cyttsp4_wakeup(dev);

	return cyttsp4_sleep(dev);
}

/**
   GPIO init function

*/
static int cyttsp4_init(struct cyttsp4_core_platform_data *pdata, int on, struct device *dev)
{
	int retval = 0;
	int ts_gpio_interrupt = 0;

	if (on == false){
		goto ts_reg_disable;
	}

	if ((acer_board_id == HW_ID_EVB1) || (acer_board_id == HW_ID_EVB2)){
		ts_gpio_interrupt = A12_GPIO_CYP_TS_INT_N_EVB;
	}
	else if ((acer_board_id == HW_ID_EVT1) || (acer_board_id == HW_ID_EVT2)){
		ts_gpio_interrupt = A12_GPIO_CYP_TS_INT_N_EVT;
	}
printk("\n[cyttsp4_init] [ts_gpio_interrupt:%d]\n", ts_gpio_interrupt);
//JOHNNY test
#if 1

	/**
	TODO : GPIO 0 need to confirm with
    */
	if (gpio_is_valid(0)) {
		retval = gpio_request(0,
						"CTP_PWR");
		if (retval) {
			pr_err("%s: unable to request reset gpio 0\n",
				__func__);
			//goto error_irq_gpio_req;
		}
		retval = gpio_direction_output(
					0, 1);
		if (retval) {
			pr_err("%s: unable to set direction for gpio 0\n",
				__func__);
			//goto error_reset_gpio_dir;
		}

		printk(" \n [CONFIG GPIO 0] pull high\n");
		gpio_set_value(0, 1);
		printk("\n [cyttsp4_init] before sleep gpio 0:%d\n", gpio_get_value(0));
		msleep(1000);
		printk("\n [cyttsp4_init] after sleep gpio 0:%d\n", gpio_get_value(0));
	}
	else{
		printk(" \n [CONFIG GPIO 0] GPIO 0 is not valid\n");
	}
#endif
	if (gpio_is_valid(ts_gpio_interrupt)) {
		/* configure touchscreen interrupt out gpio */
		retval = gpio_request(ts_gpio_interrupt,
						"CTP_INT");
		if (retval) {
			pr_err("%s: unable to request reset gpio %d\n",
				__func__, ts_gpio_interrupt);
			goto error_irq_gpio_req;
		}

		retval = gpio_direction_input(ts_gpio_interrupt);
		if (retval) {
			pr_err("%s: unable to set direction for gpio %d\n",
				__func__, ts_gpio_interrupt);
			goto error_irq_gpio_dir;
		}
	}

	if (gpio_is_valid(A12_GPIO_CYP_TS_RESET_N)) {
		/* configure touchscreen reset out gpio */
		retval = gpio_request(A12_GPIO_CYP_TS_RESET_N,
						"TP_RESET");
		if (retval) {
			pr_err("%s: unable to request reset gpio %d\n",
				__func__, A12_GPIO_CYP_TS_RESET_N);
			goto error_reset_gpio_req;
		}

		retval = gpio_direction_output(
					A12_GPIO_CYP_TS_RESET_N, 0);
		if (retval) {
			pr_err("%s: unable to set direction for gpio %d\n",
				__func__, A12_GPIO_CYP_TS_RESET_N);
			goto error_reset_gpio_dir;
		}
	}
	goto success;

ts_reg_disable:
error_reset_gpio_dir:

	if (gpio_is_valid(A12_GPIO_CYP_TS_RESET_N))
		gpio_free(A12_GPIO_CYP_TS_RESET_N);
error_reset_gpio_req:

	if (gpio_is_valid(A12_GPIO_CYP_TS_RESET_N))
		gpio_direction_output(A12_GPIO_CYP_TS_RESET_N, 0);

error_irq_gpio_dir:

	if (gpio_is_valid(ts_gpio_interrupt))
		gpio_free(ts_gpio_interrupt);
error_irq_gpio_req:

	if (gpio_is_valid(ts_gpio_interrupt))
		gpio_direction_output(ts_gpio_interrupt, 1);

success:
	dev_info(dev,
		"%s: INIT CYTTSP RST gpio=%d and IRQ gpio=%d r=%d\n",
		__func__, A12_GPIO_CYP_TS_RESET_N, ts_gpio_interrupt, retval);

printk("\n [cyttsp4_init] gpio 0:%d\n", gpio_get_value(0));
	return retval;
}

static struct cyttsp4_core_platform_data _cyttsp4_core_platform_data = {
	//.irq_gpio = A12_GPIO_CYP_TS_INT_N,
	.xres = cyttsp4_xres,
	.init = cyttsp4_init,
	//.hw_init = cyttsp4_hw_init,
	.power = cyttsp4_power,
	.sett = {
		NULL,	/* Reserved */
		NULL,	/* Command Registers */
		NULL,	/* Touch Report */
		NULL,	/* Cypress Data Record */
		NULL,	/* Test Record */
		NULL,	/* Panel Configuration Record */
		NULL, /* &cyttsp4_sett_param_regs, */
		NULL, /* &cyttsp4_sett_param_size, */
		NULL,	/* Reserved */
		NULL,	/* Reserved */
		NULL,	/* Operational Configuration Record */
		NULL, /* &cyttsp4_sett_ddata, *//* Design Data Record */
		NULL, /* &cyttsp4_sett_mdata, *//* Manufacturing Data Record */
		NULL,	/* Config and Test Registers */
		&cyttsp4_sett_btn_keys,	/* button-to-keycode table */
	},
};


static struct cyttsp4_core cyttsp4_core_device = {
	.name = CYTTSP4_CORE_NAME,
	.id = "main_ttsp_core",
	.adap_id = CYTTSP4_I2C_NAME,
	.dev = {
		.platform_data = &_cyttsp4_core_platform_data,
	},
};
void ts_init(void){
	cyttsp4_register_core_device(&cyttsp4_core_device);
	cyttsp4_register_device(&cyttsp4_mt_device);
	cyttsp4_register_device(&cyttsp4_btn_device);
}


static int cyttsp4_i2c_hw_init(struct device *dev)
{


	int rc = 0, i;
	const struct cypress_ts_regulator *reg_info = NULL;
	u8 num_reg = 0;

	reg_info = a12_ts_regulator_data;
	num_reg = ARRAY_SIZE(a12_ts_regulator_data);

	ts_vdd = kzalloc(num_reg * sizeof(struct regulator *), GFP_KERNEL);

	if (!reg_info) {
		pr_err("regulator pdata not specified\n");
		return -EINVAL;
	}

	if (!ts_vdd) {
		pr_err("unable to allocate memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < num_reg; i++) {
		ts_vdd[i] = regulator_get(dev, reg_info[i].name);

		if (IS_ERR(ts_vdd[i])) {
			rc = PTR_ERR(ts_vdd[i]);
			pr_err("%s:regulator get failed rc=%d\n",
							__func__, rc);
			goto error_vdd;
		}

		if (regulator_count_voltages(ts_vdd[i]) > 0) {
			rc = regulator_set_voltage(ts_vdd[i],
				reg_info[i].min_uV, reg_info[i].max_uV);
			if (rc) {
				pr_err("%s: regulator_set_voltage"
					"failed rc =%d\n", __func__, rc);
				regulator_put(ts_vdd[i]);
				goto error_vdd;
			}
		}

		rc = regulator_set_optimum_mode(ts_vdd[i],
						reg_info[i].load_uA);
		if (rc < 0) {
			pr_err("%s: regulator_set_optimum_mode failed rc=%d\n",
								__func__, rc);

			regulator_set_voltage(ts_vdd[i], 0,
						reg_info[i].max_uV);
			regulator_put(ts_vdd[i]);
			goto error_vdd;
		}

		rc = regulator_enable(ts_vdd[i]);
		if (rc) {
			pr_err("%s: regulator_enable failed rc =%d\n",
								__func__, rc);

			regulator_set_voltage(ts_vdd[i], 0,
						reg_info[i].max_uV);
			regulator_put(ts_vdd[i]);
			goto error_vdd;
		}
	}

	return rc;

error_vdd:
	return rc;

}
/*==============================added===================================*/


static int cyttsp4_i2c_read_block_data(struct cyttsp4_i2c *ts_i2c, u8 addr,
	size_t length, void *values)
{
	int rc;

	/* write addr */
	rc = i2c_master_send(ts_i2c->client, &addr, sizeof(addr));
	if (rc < 0)
		return rc;
	else if (rc != sizeof(addr))
		return -EIO;

	/* read data */
	rc = i2c_master_recv(ts_i2c->client, values, length);

	return (rc < 0) ? rc : rc != length ? -EIO : 0;
}

static int cyttsp4_i2c_write_block_data(struct cyttsp4_i2c *ts_i2c, u8 addr,
	size_t length, const void *values)
{
	int rc;

	if (sizeof(ts_i2c->wr_buf) < (length + 1))
		return -ENOMEM;

	ts_i2c->wr_buf[0] = addr;
	memcpy(&ts_i2c->wr_buf[1], values, length);
	length += 1;

	/* write data */
	rc = i2c_master_send(ts_i2c->client, ts_i2c->wr_buf, length);

	return (rc < 0) ? rc : rc != length ? -EIO : 0;
}

static int cyttsp4_i2c_write(struct cyttsp4_adapter *adap, u8 addr,
	const void *buf, int size)
{
	struct cyttsp4_i2c *ts = dev_get_drvdata(adap->dev);
	int rc;

	pm_runtime_get_noresume(adap->dev);
	mutex_lock(&ts->lock);
	rc = cyttsp4_i2c_write_block_data(ts, addr, size, buf);
	mutex_unlock(&ts->lock);
	pm_runtime_put_noidle(adap->dev);

	return rc;
}

static int cyttsp4_i2c_read(struct cyttsp4_adapter *adap, u8 addr,
	void *buf, int size)
{
	struct cyttsp4_i2c *ts = dev_get_drvdata(adap->dev);
	int rc;

	pm_runtime_get_noresume(adap->dev);
	mutex_lock(&ts->lock);
	rc = cyttsp4_i2c_read_block_data(ts, addr, size, buf);
	mutex_unlock(&ts->lock);
	pm_runtime_put_noidle(adap->dev);

	return rc;
}

static struct cyttsp4_ops ops = {
	.write = cyttsp4_i2c_write,
	.read = cyttsp4_i2c_read,
};

static int __devinit cyttsp4_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *i2c_id)
{
	struct cyttsp4_i2c *ts_i2c;
	struct device *dev = &client->dev;
	char const *adap_id = dev_get_platdata(dev);
	char const *id;
	int rc;

	dev_err(dev, "%s: Starting %s probe...\n", __func__, CYTTSP4_I2C_NAME);
	dev_dbg(dev, "%s: debug on\n", __func__);
	dev_vdbg(dev, "%s: verbose debug on\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(dev, "%s: fail check I2C functionality\n", __func__);
		rc = -EIO;
		goto error_alloc_data_failed;
	}

	ts_i2c = kzalloc(sizeof(struct cyttsp4_i2c), GFP_KERNEL);
	if (ts_i2c == NULL) {
		dev_err(dev, "%s: Error, kzalloc.\n", __func__);
		rc = -ENOMEM;
		goto error_alloc_data_failed;
	}

	mutex_init(&ts_i2c->lock);
	ts_i2c->client = client;
	client->dev.bus = &i2c_bus_type;
	i2c_set_clientdata(client, ts_i2c);
	dev_set_drvdata(&client->dev, ts_i2c);

	if (adap_id)
		id = adap_id;
	else
		id = CYTTSP4_I2C_NAME;

	dev_dbg(dev, "%s: add adap='%s' (CYTTSP4_I2C_NAME=%s)\n", __func__, id,
		CYTTSP4_I2C_NAME);

	pm_runtime_enable(&client->dev);

	rc = cyttsp4_add_adapter(id, &ops, dev);
	if (rc) {
		dev_err(dev, "%s: Error on probe %s\n", __func__,
			CYTTSP4_I2C_NAME);
		goto add_adapter_err;
	}

	dev_info(dev, "%s: Successful probe %s\n", __func__, CYTTSP4_I2C_NAME);

	cyttsp4_i2c_hw_init(dev);
	ts_init();

	return 0;

add_adapter_err:
	pm_runtime_disable(&client->dev);
	dev_set_drvdata(&client->dev, NULL);
	i2c_set_clientdata(client, NULL);
	kfree(ts_i2c);
error_alloc_data_failed:
	return rc;
}

/* registered in driver struct */
static int __devexit cyttsp4_i2c_remove(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct cyttsp4_i2c *ts_i2c = dev_get_drvdata(dev);
	char const *adap_id = dev_get_platdata(dev);
	char const *id;

	if (adap_id)
		id = adap_id;
	else
		id = CYTTSP4_I2C_NAME;

	dev_info(dev, "%s\n", __func__);
	cyttsp4_del_adapter(id);
	pm_runtime_disable(&client->dev);
	dev_set_drvdata(&client->dev, NULL);
	i2c_set_clientdata(client, NULL);
	kfree(ts_i2c);
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id cypress_match_table[] = {
	{ .compatible = "cypress,cypress-ts",},
	{ },
};
#else
#endif

static const struct i2c_device_id cyttsp4_i2c_id[] = {
	{ CYTTSP4_I2C_NAME, 0 },  { }
};

static struct i2c_driver cyttsp4_i2c_driver = {
	.driver = {
		.name = CYTTSP4_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = cypress_match_table,
	},
	.probe = cyttsp4_i2c_probe,
	.remove = __devexit_p(cyttsp4_i2c_remove),
	.id_table = cyttsp4_i2c_id,
};

module_i2c_driver(cyttsp4_i2c_driver);
MODULE_ALIAS(CYTTSP4_I2C_NAME);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cypress TrueTouch(R) Standard Product (TTSP) I2C driver");
MODULE_AUTHOR("Cypress");
MODULE_DEVICE_TABLE(i2c, cyttsp4_i2c_id);
