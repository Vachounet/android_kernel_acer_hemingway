/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/wakelock.h>

#include "../mdss.h"
#include "../mdss_io_util.h"

#include "sii8334_drv.h"
#include "mhl_i2c_access.h"

#include "../../../../../arch/arm/mach-msm/acer_hw_version.h"

#define MHL_DRIVER_NAME "sii8334"
#define COMPATIBLE_NAME "qcom,mhl-sii8334"

#define KEY_REL_DETECT_TIME    100  //ms

enum mhl_gpio_type {
	MHL_TX_RESET_GPIO,
	MHL_TX_INTR_GPIO,
	MHL_TX_PMIC_PWR_GPIO,
	MHL_TX_MAX_GPIO,
};

enum mhl_vreg_type {
	MHL_TX_3V_VREG,
	MHL_TX_MAX_VREG,
};

struct mhl_tx_platform_data {
	/* Data filled from device tree nodes */
	struct dss_gpio *gpios[MHL_TX_MAX_GPIO];
	struct dss_vreg *vregs[MHL_TX_MAX_VREG];
	int irq;
};

struct sii8334_data {
	int chip_id;
	struct work_struct irq_work;
	struct workqueue_struct *wq;
	struct i2c_client *i2c_handle;
	struct mhl_tx_platform_data *pdata;
	struct wake_lock mhl_wakelock;
	struct timer_list key_release_timer;
	struct work_struct key_release_work;
	struct mutex key_mutex;
	struct input_dev *input;
	struct wake_lock mhl_resume_wakelock;
};

struct mhl_key_map {
	unsigned char rcp_key;
	unsigned char std_key;
};

static const struct mhl_key_map key_mapping[] = {
	{0x00, KEY_ENTER},
	{0x01, KEY_UP},
	{0x02, KEY_DOWN},
	{0x03, KEY_LEFT},
	{0x04, KEY_RIGHT},
	{0x09, KEY_HOMEPAGE},
	{0x0A, KEY_MENU},
	{0x0B, KEY_MENU},
	{0x0C, KEY_MENU},
	{0x0D, KEY_BACK},
	{0x20, KEY_0},
	{0x21, KEY_1},
	{0x22, KEY_2},
	{0x23, KEY_3},
	{0x24, KEY_4},
	{0x25, KEY_5},
	{0x26, KEY_6},
	{0x27, KEY_7},
	{0x28, KEY_8},
	{0x29, KEY_9},
	{0x2B, KEY_ENTER},
	{0x37, KEY_PAGEUP},
	{0x38, KEY_PAGEDOWN},
	{0x41, KEY_VOLUMEUP},
	{0x42, KEY_VOLUMEDOWN},
	{0x43, KEY_MUTE},
	{0x44, KEY_PLAYPAUSE},
	{0x45, KEY_STOPCD},
	{0x46, KEY_PLAYPAUSE},
	{0x48, KEY_REWIND},
	{0x49, KEY_FASTFORWARD},
	{0x4B, KEY_NEXTSONG},
	{0x4C, KEY_PREVIOUSSONG},
	{0xff, 0xff},
};

static struct sii8334_data *mhl_ctrl;
static int resume_flag = 1;
static int mhl_isr_flag = 1;

/***********************************************************
 *  i2c access function
 ************************************************************/
static uint8_t slave_addrs[5] = {
	DEV_PAGE_TPI,
	DEV_PAGE_TX_L1,
	DEV_PAGE_TX_2,
	DEV_PAGE_TX_3,
	DEV_PAGE_CBUS,
};

int get_mhl_chip_id(void) {
	return mhl_ctrl ? mhl_ctrl->chip_id : -1;
}

static void sii8334_key_release_work(struct work_struct *work)
{
	AppNotifyMhlEvent(MHL_TX_RCP_KEY_RELEASE, 0);
}

static void key_release_poll_func(unsigned long arg)
{
	queue_work(mhl_ctrl->wq, &mhl_ctrl->key_release_work);
}

static inline unsigned char map_key(unsigned char buf)
{
	unsigned char i = 0;
	while((key_mapping[i].rcp_key != buf) &&
		(key_mapping[i].rcp_key != 0xff)) {
		i++;
	}
	return key_mapping[i].std_key;
}

uint8_t SiiRegRead(uint16_t reg)
{
	int rc = -1;
	uint8_t buffer = 0;
	uint8_t slave_addr_index = reg >> 8;

	if (!mhl_ctrl || !mhl_ctrl->i2c_handle) {
		pr_err("%s: get wrong client data\n", __func__);
		return rc;
	}

	rc = mdss_i2c_byte_read(mhl_ctrl->i2c_handle, slave_addrs[slave_addr_index],
				reg, &buffer);
	if (rc) {
		pr_err("%s: slave=%x, off=%x\n",
		       __func__, slave_addrs[slave_addr_index], reg);
		return rc;
	}
	return buffer;
}

void SiiRegWrite(uint16_t reg, uint8_t value)
{
	uint8_t slave_addr_index = reg >> 8;

	if (!mhl_ctrl || !mhl_ctrl->i2c_handle) {
		pr_err("%s: get wrong client data\n", __func__);
		return;
	}
	mdss_i2c_byte_write(mhl_ctrl->i2c_handle, slave_addrs[slave_addr_index],
			reg, &value);
	return;
}

void SiiRegModify(uint16_t reg, uint8_t mask, uint8_t val)
{
	uint8_t temp;

	temp = SiiRegRead(reg);
	temp &= (~mask);
	temp |= (mask & val);
	SiiRegWrite(reg, temp);
}

void SiiRegReadBlock(uint16_t reg, uint8_t *data, uint8_t len)
{
	int i = 0;
	if (!data) {
		pr_err("%s:empty data!!!\n", __func__);
		return;
	}

	for (i=0; i<len; i++)
			data[i] = SiiRegRead(reg + i);
}

void SiiRegWriteBlock(uint16_t reg, uint8_t *data, uint8_t len)
{
	int i = 0;
	if (!data) {
		pr_err("%s:empty data!!!\n", __func__);
		return;
	}

	for (i=0; i<len; i++)
		SiiRegWrite(reg + i, data[i]);
}
/***********************************************************
 *  i2c access function done.
 ************************************************************/

/***********************************************************
 *  charing and rcp function
 ************************************************************/
void AppNotifyMhlEvent(unsigned char eventCode, unsigned char eventParam)
{
	static unsigned char last_key_code = 0xff;
	unsigned char key_code;
	static bool key_pressed;

	if ((eventCode == MHL_TX_EVENT_RCP_RECEIVED) ||
		(eventCode == MHL_TX_RCP_KEY_RELEASE)) {

		key_code = map_key(eventParam);
		if (key_code == 0xff) {
			pr_info("%s:get worng key:%x\n", __func__, key_code);
			return;
		}

		if (!mhl_ctrl || !mhl_ctrl->input) {
			pr_info("%s:MHL input device is not ready!\n", __func__);
			return;
		}

		TX_DEBUG_PRINT("rcp key code:%x %x\n", eventCode, key_code);
		del_timer_sync(&mhl_ctrl->key_release_timer);

		mutex_lock(&mhl_ctrl->key_mutex);
		if (eventCode == MHL_TX_EVENT_RCP_RECEIVED) {
			if ((last_key_code != key_code) && key_pressed) {
				input_report_key(mhl_ctrl->input, last_key_code, 0);
				input_sync(mhl_ctrl->input);
			}

			input_report_key(mhl_ctrl->input, key_code, 1);
			input_sync(mhl_ctrl->input);
			key_pressed = true;
			last_key_code = key_code;
			mod_timer(&mhl_ctrl->key_release_timer,
					jiffies + msecs_to_jiffies(KEY_REL_DETECT_TIME));
		} else {
			if (key_pressed) {
				input_report_key(mhl_ctrl->input, last_key_code, 0);
				input_sync(mhl_ctrl->input);
			}
			key_pressed = false;
		}
		mutex_unlock(&mhl_ctrl->key_mutex);
	}
}

void mhl_enable_charging(unsigned int chg_type)
{
	static bool is_charging;

	if(!is_mhl_dongle_on() && (chg_type != POWER_SUPPLY_TYPE_BATTERY)) {
		pr_info("%s:set wrong charging current\n", __func__);
		return;
	}

	if (!is_charging && (chg_type == POWER_SUPPLY_TYPE_BATTERY)) {
		pr_info("%s: should not disable charging!\n", __func__);
		return;
	}

	if (CHARGING_FUNC_NAME) {
		pr_info("MHL set charing type to %d\n", chg_type);
		is_charging = (chg_type != POWER_SUPPLY_TYPE_BATTERY) ? true : false;
		if (is_charging)
			wake_lock(&mhl_ctrl->mhl_wakelock);
		else
			wake_unlock(&mhl_ctrl->mhl_wakelock);
		CHARGING_FUNC_NAME(chg_type);
	} else
		pr_info("%s:Can't find charging function!\n", __func__);
}

/***********************************************************
 *  charing and rcp function done.
 ************************************************************/

static int mhl_tx_get_dt_data(struct device *dev, struct mhl_tx_platform_data *pdata)
{
	int i, rc = 0;
	struct device_node *of_node = NULL;
	struct dss_gpio *temp_gpio = NULL;
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
	temp_gpio = devm_kzalloc(dev, sizeof(struct dss_gpio), GFP_KERNEL);
	pr_debug("%s: gpios allocd\n", __func__);
	if (!(temp_gpio)) {
		pr_err("%s: can't alloc %d gpio mem\n", __func__, i);
		goto error;
	}
	temp_gpio->gpio = of_get_named_gpio(of_node, "mhl-rst-gpio", 0);
	snprintf(temp_gpio->gpio_name, 32, "%s", "mhl-rst-gpio");
	pr_debug("%s: rst gpio=[%d]\n", __func__,
		 temp_gpio->gpio);
	pdata->gpios[MHL_TX_RESET_GPIO] = temp_gpio;

	temp_gpio = NULL;
	temp_gpio = devm_kzalloc(dev, sizeof(struct dss_gpio), GFP_KERNEL);
	pr_debug("%s: gpios allocd\n", __func__);
	if (!(temp_gpio)) {
		pr_err("%s: can't alloc %d gpio mem\n", __func__, i);
		goto error;
	}
	temp_gpio->gpio = of_get_named_gpio(of_node, "mhl-pwr-gpio", 0);
	snprintf(temp_gpio->gpio_name, 32, "%s", "mhl-pwr-gpio");
	pr_debug("%s: pmic gpio=[%d]\n", __func__,
		 temp_gpio->gpio);
	pdata->gpios[MHL_TX_PMIC_PWR_GPIO] = temp_gpio;

	temp_gpio = NULL;
	temp_gpio = devm_kzalloc(dev, sizeof(struct dss_gpio), GFP_KERNEL);
	pr_debug("%s: gpios allocd\n", __func__);
	if (!(temp_gpio)) {
		pr_err("%s: can't alloc %d gpio mem\n", __func__, i);
		goto error;
	}
	temp_gpio->gpio = of_get_named_gpio(of_node, "mhl-intr-gpio", 0);
	snprintf(temp_gpio->gpio_name, 32, "%s", "mhl-intr-gpio");
	pr_debug("%s: intr gpio=[%d]\n", __func__,
		 temp_gpio->gpio);
	pdata->gpios[MHL_TX_INTR_GPIO] = temp_gpio;

	return 0;
error:
	pr_err("%s: ret due to err\n", __func__);
	for (i = 0; i < MHL_TX_MAX_GPIO; i++)
		if (pdata->gpios[i])
			devm_kfree(dev, pdata->gpios[i]);
	return rc;
} /* mhl_tx_get_dt_data */

static inline int mhl_sii_reset_pin(int on)
{
	gpio_set_value(mhl_ctrl->pdata->gpios[MHL_TX_RESET_GPIO]->gpio, on);
	return 0;
}

static int mhl_sii_reg_config(struct i2c_client *client, bool enable)
{
	static struct regulator *reg_8941_l02;
	int rc = -ENODEV;;

	if (!reg_8941_l02) {
		reg_8941_l02 = regulator_get(&client->dev,
			"avcc_12");
		if (IS_ERR(reg_8941_l02)) {
			pr_err("could not get reg_8941_l02, rc = %ld\n",
				PTR_ERR(reg_8941_l02));
			return -ENODEV;
		}
		if (enable)
			rc = regulator_enable(reg_8941_l02);
		else
			rc = regulator_disable(reg_8941_l02);
		if (rc) {
			pr_debug("'%s' regulator configure[%u] failed, rc=%d\n",
				 "avcc_1.2V", enable, rc);
			return rc;
		} else {
			pr_debug("%s: vreg L02 %s\n",
				 __func__, (enable ? "enabled" : "disabled"));
		}
	}
	return rc;
}

static int mhl_vreg_config(uint8_t on)
{
	int ret;
	struct i2c_client *client = mhl_ctrl->i2c_handle;
	int pwr_gpio = mhl_ctrl->pdata->gpios[MHL_TX_PMIC_PWR_GPIO]->gpio;

	if (on) {
		ret = gpio_request(pwr_gpio,
		    mhl_ctrl->pdata->gpios[MHL_TX_PMIC_PWR_GPIO]->gpio_name);
		if (ret < 0) {
			pr_err("%s: mhl pwr gpio req failed: %d\n", __func__, ret);
			return ret;
		}
		ret = gpio_direction_output(pwr_gpio, 1);
		if (ret < 0) {
			pr_err("%s: set gpio MHL_PWR_EN dircn failed: %d\n", __func__, ret);
			return ret;
		}

		ret = mhl_sii_reg_config(client, true);
		if (ret) {
			pr_err("%s: regulator enable failed\n", __func__);
			return -EINVAL;
		}
	} else {
		mhl_sii_reg_config(client, false);
		gpio_free(pwr_gpio);
	}
	return 0;
}

/*
 * Request for GPIO allocations
 * Set appropriate GPIO directions
 */
static int mhl_gpio_config(int on)
{
	int ret;
	struct dss_gpio *temp_reset_gpio, *temp_intr_gpio;

	/* caused too many line spills */
	temp_reset_gpio = mhl_ctrl->pdata->gpios[MHL_TX_RESET_GPIO];
	temp_intr_gpio = mhl_ctrl->pdata->gpios[MHL_TX_INTR_GPIO];

	if (on) {
		if (gpio_is_valid(temp_reset_gpio->gpio)) {
			ret = gpio_request(temp_reset_gpio->gpio, temp_reset_gpio->gpio_name);
			if (ret < 0) {
				pr_err("%s:rst_gpio=[%d] req failed:%d\n", __func__,
						temp_reset_gpio->gpio, ret);
				return -EBUSY;
			}
			ret = gpio_direction_output(temp_reset_gpio->gpio, 0);
			if (ret < 0) {
				pr_err("%s: set dirn rst failed: %d\n", __func__, ret);
				return -EBUSY;
			}
		}
		if (gpio_is_valid(temp_intr_gpio->gpio)) {
			ret = gpio_request(temp_intr_gpio->gpio, temp_intr_gpio->gpio_name);
			if (ret < 0) {
				pr_err("%s: intr_gpio req failed: %d\n", __func__, ret);
				return -EBUSY;
			}
			ret = gpio_direction_input(temp_intr_gpio->gpio);
			if (ret < 0) {
				pr_err("%s: set dirn intr failed: %d\n", __func__, ret);
				return -EBUSY;
			}
			mhl_ctrl->i2c_handle->irq = gpio_to_irq(temp_intr_gpio->gpio);
			pr_debug("%s: gpio_to_irq=%d\n",
				 __func__, mhl_ctrl->i2c_handle->irq);
		}
	} else {
		gpio_free(temp_intr_gpio->gpio);
		gpio_free(temp_reset_gpio->gpio);
	}
	return 0;
}

static void sii8334_irq_work(struct work_struct *work)
{
	mhl_isr_flag = 0;

	if (resume_flag) {
		wake_lock(&mhl_ctrl->mhl_resume_wakelock);
		SiiMhlTxDeviceIsr();
		mhl_isr_flag = 1;
		wake_unlock(&mhl_ctrl->mhl_resume_wakelock);
	}
}

static irqreturn_t sii8334_irq(int irq, void *handle)
{
	queue_work(mhl_ctrl->wq, &mhl_ctrl->irq_work);
	return IRQ_HANDLED;
}

static void mhl_dev_id(void)
{
	uint8_t id_lo, id_hi;

	id_lo = SiiRegRead(TX_PAGE_TPI | 0x02);
	id_hi = SiiRegRead(TX_PAGE_TPI | 0x03);
	if (mhl_ctrl) {
		mhl_ctrl->chip_id = id_hi << 8 | id_lo;
		pr_info("%s: Dev_ID = %x\n", __func__, mhl_ctrl->chip_id);
	} else
		pr_info("%s: warning, data is null!\n", __func__);
}

static int mhl_tx_chip_init(void)
{
	mhl_sii_reset_pin(0);
	mdelay(3);
	mhl_sii_reset_pin(1);
	mdelay(3);

	mhl_dev_id();
	SiiMhlTxInitialize(30);
	return 0;
}

static int mhl_i2c_suspend(struct i2c_client *client, pm_message_t message)
{
	resume_flag = 0;

	return 0;
}

static int mhl_i2c_resume(struct i2c_client *client)
{
	resume_flag = 1;

	wake_lock(&mhl_ctrl->mhl_resume_wakelock);
	if (!mhl_isr_flag) {
		mhl_tx_chip_init();
	}
	wake_unlock(&mhl_ctrl->mhl_resume_wakelock);

	return 0;
}

static int mhl_i2c_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int i = 0;
	int rc = 0;
	struct mhl_tx_platform_data *pdata = NULL;

	if (acer_board_id >= HW_ID_PVT) {
		pr_err("MHL not support for PVT\n");
		return 0;
	}

	mhl_ctrl = devm_kzalloc(&client->dev, sizeof(*mhl_ctrl), GFP_KERNEL);
	if (mhl_ctrl == NULL) {
		pr_err("%s: no memory for driver data\n", __func__);
		rc = -ENOMEM;
		goto failed_no_mem;
	}
	wake_lock_init(&mhl_ctrl->mhl_wakelock, WAKE_LOCK_SUSPEND, "MHL dongle");

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			     sizeof(struct mhl_tx_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			rc = -ENOMEM;
			goto failed_no_mem;
		}

		rc = mhl_tx_get_dt_data(&client->dev, pdata);
		if (rc) {
			pr_err("%s: FAILED: parsing device tree data; rc=%d\n",
				__func__, rc);
			goto failed_dt_data;
		}
		mhl_ctrl->i2c_handle = client;
		mhl_ctrl->pdata = pdata;
		i2c_set_clientdata(client, mhl_ctrl);
	}

	mhl_ctrl->wq = create_workqueue("mhl_sii8334_wq");
	if (!mhl_ctrl->wq) {
		pr_err("%s: can't create workqueue\n", __func__);
		rc = -EFAULT;
		goto failed_probe;
	}
	INIT_WORK(&mhl_ctrl->irq_work, sii8334_irq_work);

	INIT_WORK(&mhl_ctrl->key_release_work, sii8334_key_release_work);
	init_timer(&mhl_ctrl->key_release_timer);
	mhl_ctrl->key_release_timer.function = key_release_poll_func;
	mhl_ctrl->key_release_timer.data = (unsigned long)NULL;
	wake_lock_init(&mhl_ctrl->mhl_resume_wakelock, WAKE_LOCK_SUSPEND, "MHL dongle resume");

	rc = mhl_vreg_config(1);
	if (rc) {
		pr_err("%s: vreg init failed [%d]\n", __func__, rc);
		goto failed_probe;
	}

	rc = mhl_gpio_config(1);
	if (rc) {
		pr_err("%s: gpio init failed [%d]\n", __func__, rc);
		goto failed_probe;
	}

	rc = mhl_tx_chip_init();
	if (rc) {
		pr_err("%s: tx chip init failed [%d]\n", __func__, rc);
		goto failed_probe;
	}

	pr_debug("%s: IRQ from GPIO INTR = %d\n", __func__, mhl_ctrl->i2c_handle->irq);
	pr_debug("%s: Driver name = [%s]\n", __func__, client->dev.driver->name);
	rc = request_irq(mhl_ctrl->i2c_handle->irq, &sii8334_irq,
				  IRQF_TRIGGER_FALLING, client->dev.driver->name, mhl_ctrl);
	if (rc) {
		pr_err("request_threaded_irq failed, status: %d\n",	rc);
		goto failed_probe;
	} else
		pr_debug("request_threaded_irq succeeded\n");

	pr_debug("%s: i2c client addr is [%x]\n", __func__, client->addr);

	/* Input register for RCP */
	mhl_ctrl->input = input_allocate_device();
	if (!mhl_ctrl->input) {
		pr_err("%s: input_allocate_device failed!\n", __func__);
	} else {
		mutex_init(&mhl_ctrl->key_mutex);
		mhl_ctrl->input->name = MHL_DRIVER_NAME;
		set_bit(EV_KEY, mhl_ctrl->input->evbit);
		i = 0;
		while (key_mapping[i].rcp_key != 0xff) {
			input_set_capability(mhl_ctrl->input, EV_KEY, key_mapping[i].std_key);
			i++;
		}
		rc = input_register_device(mhl_ctrl->input);
		if (rc) {
			pr_err("%s input_register_device failed!\n", __func__);
			mhl_ctrl->input = NULL;
		}
	}

	return 0;

failed_probe:
failed_dt_data:
	if (pdata)
		devm_kfree(&client->dev, pdata);
failed_no_mem:
	if (mhl_ctrl)
		devm_kfree(&client->dev, mhl_ctrl);
	pr_err("%s: PROBE FAILED, rc=%d\n", __func__, rc);
	return rc;
}


static int mhl_i2c_remove(struct i2c_client *client)
{
	if (!mhl_ctrl) {
		pr_warn("%s: i2c get client data failed\n", __func__);
		return -EINVAL;
	}

	free_irq(mhl_ctrl->i2c_handle->irq, mhl_ctrl);
	mhl_gpio_config(0);
	mhl_vreg_config(0);
	wake_lock_destroy(&mhl_ctrl->mhl_wakelock);
	wake_lock_destroy(&mhl_ctrl->mhl_resume_wakelock);
	if (mhl_ctrl->pdata)
		devm_kfree(&client->dev, mhl_ctrl->pdata);
	devm_kfree(&client->dev, mhl_ctrl);
	return 0;
}

static struct i2c_device_id mhl_sii_i2c_id[] = {
	{ MHL_DRIVER_NAME, 0 },
	{ }
};


MODULE_DEVICE_TABLE(i2c, mhl_sii_i2c_id);

static struct of_device_id mhl_match_table[] = {
	{.compatible = COMPATIBLE_NAME,},
	{ },
};

static struct i2c_driver mhl_sii_i2c_driver = {
	.driver = {
		.name = MHL_DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = mhl_match_table,
	},
	.suspend = mhl_i2c_suspend,
	.resume = mhl_i2c_resume,
	.probe = mhl_i2c_probe,
	.remove =  mhl_i2c_remove,
	.id_table = mhl_sii_i2c_id,
};

module_i2c_driver(mhl_sii_i2c_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ACER MHL SII 8334 TX Driver");
