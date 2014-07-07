/*
 * Cypress capsense sensor key driver
 *
 * Author: Ben Hsu <ben.hsu@acer.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/regulator/consumer.h>
#include "../../../arch/arm/mach-msm/acer_hw_version.h"
#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#define I2C_PULL_UP_MAX_UV		1800000
#define I2C_PULL_UP_MIN_UV		1800000

#define I2C_READ_FW_LEN			2
#define I2C_REG_CTRL_LEN		2
#define I2C_REG_DIFF_LEN		2
#define I2C_ERR_RETRY			5
#define I2C_TRANSFER_SUCCESS	0
#define I2C_READ_REG_LEN		1

/* Button status register */
#define REG_BTN_STATUS			0x00
#define REG_BTN_R_LEBEL_MASK	0x01
#define REG_BTN_R_STATUS_MASK	0x10
#define REG_BTN_STATUS_MASK		0x01
#define REG_BTN_STATUS_SHIFT	4
#define REG_BTN_NEXT_SHIFT		1

/* FW version register */
#define REG_FW_VER				0x01
#define REG_FW_VER_SHIFT		8

/* General control register */
#define REG_CTRL				0x03
#define REG_CTRL_INT			0x01
#define REG_CTRL_LED_MODE		0x04
#define REG_CTRL_LED_ON			0x08

/* Clear interrupt register */
#define REG_INT_CLR_REG			0x08
#define REG_INT_CLR_VAL			0xAA

/* Sensitivity control register */
#define REG_SEN_CTRL			0x09

/* Sensitivity level */
enum {
	REG_SEN_LV1 = 1,
	REG_SEN_LV2,
	REG_SEN_LV3,
	REG_SEN_LV4,
	REG_SEN_MAX,
};

enum {
	STATUS_KEY_MENU,
	STATUS_KEY_HOME,
	STATUS_KEY_BACK,
	STATUS_KEY_MAX,
};

enum {
	KEY_STATUS_RELEASE,
	KEY_STATUS_PRESS,
	KEY_STATUS_MAX,
};

/* Sleep mode control register */
#define REG_SLEEP_CTRL			0x0A
enum {
	REG_SLEEP_MODE_DEEP = 1,
	REG_SLEEP_MODE_WAKEUP_2KEY,
	REG_SLEEP_MODE_WAKEUP_2S,
	REG_SLEEP_MODE_MAX,
};

/* Key detect threshold register */
#define REG_KEY_MENU_THRESHOLD		0x0C
#define REG_KEY_HOME_THRESHOLD		0x0D
#define REG_KEY_BACK_THRESHOLD		0x0E
#define REG_THRESHOLD_FW_VER		0x010C
#define REG_THRESHOLD_DOUBLE_FW_VER	0x0206
#define REG_THRESHOLD_BACK_DEF_L	130
#define REG_THRESHOLD_BACK_DEF_H	140
#define REG_THRESHOLD_HOME_DEF		120
#define REG_THRESHOLD_MENU_DEF		120
#define REG_THRESHOLD_INC_VAL		5
#define REG_THRESHOLD_INC_BASE		1
#define REG_THRESHOLD_MAX_VAL		255

#define REG_SW_RESET				0x0F
#define REG_RESET_COUNT				0x3

#define REG_SIGNAL_DIFF_CTRL		0x10
#define REG_SIGNAL_DIFF_VAL			0x41
#define REG_SIGNAL_DIFF_INVALID		40
#define REG_SIGNAL_DIFF_MENU		0x11
#define REG_SIGNAL_DIFF_HOME		0x13
#define REG_SIGNAL_DIFF_BACK		0x15

#define CSKEY_BUF_LEN				32
#define CSKEY_KEY_SWITCH_SUPPORT
#define CSKEY_WAKEUP_SUPPORT
#define CSKEY_RECOVERY_SUPPORT
#define CSKEY_RECOVERY_DELAY_L		5000
#define CSKEY_RECOVERY_DELAY_S		300
#define CSKEY_RECOVERY_DELAY_BASE	1000
#define CSKEY_REL_DEBOUNCE_FW_VER	0x0207
#define CSKEY_KEY_PRESS				1
#define CSKEY_KEY_RELEASE			0

#if defined(CSKEY_KEY_SWITCH_SUPPORT)
enum {
	KEY_SWITCH_OFF,
	KEY_SWITCH_ON,
	KEY_SWITCH_MAX,
};

static bool cskey_key_switch = false;
#endif

#if defined(CSKEY_WAKEUP_SUPPORT)
enum {
	IRQ_STATE_ENABLE,
	IRQ_STATE_ENABLE_WAKEUP,
	IRQ_STATE_DISABLE,
	IRQ_STATE_WAKEUP,
	IRQ_STATE_MAX,
};
#endif

struct cskey_platform_data {
	int reset_gpio;
	u32 reset_gpio_flags;
	int irq_gpio;
	u32 irq_gpio_flags;
	unsigned int irq;
	bool i2c_pull_up;
};

struct cypress_cskey {
	char *name;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct cskey_platform_data *pdata;
#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
	int fb_status;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
	struct workqueue_struct *cskey_wq;
	struct work_struct key_event_work;
	struct work_struct resume_work;
	struct regulator *vcc_i2c;
	int key_status[STATUS_KEY_MAX];
	int sensitivity;
	int sleep_mode;
	int key_threshold[STATUS_KEY_MAX];
#if defined(CSKEY_RECOVERY_SUPPORT)
	unsigned long long press_time_prev[STATUS_KEY_MAX];
	unsigned long long press_time[STATUS_KEY_MAX];
	unsigned int threshold_fix_count[STATUS_KEY_MAX];
	struct delayed_work recovery_work;
	bool key_recovery[STATUS_KEY_MAX];
#endif
	unsigned int fw_version;
	unsigned int irq_state;
	bool device_sleep;
	bool led_on;
	bool i2c_power_on;
	int (*reset)(struct cypress_cskey *cskey_dev);
#if defined(CSKEY_WAKEUP_SUPPORT)
	struct work_struct change_mode_work;
	spinlock_t lock;
#endif
};

static int cs_keycode[] = {
	KEY_MENU, KEY_HOME, KEY_BACK,
};

#if defined(CSKEY_KEY_SWITCH_SUPPORT)
static int cs_keycode_switch[] = {
	KEY_F5, KEY_F4, KEY_BACK,
};
#endif

static void cskey_led_control(struct led_classdev *cdev,
						enum led_brightness value);
static struct led_classdev cskey_led = {
	.name = "cskey:backlight",
	.brightness = 0,
	.brightness_set = cskey_led_control,
};

struct cypress_cskey *cskey;
static struct kobject *device_info_kobj = NULL;

static int cskey_i2c_write(struct cypress_cskey *cskey_dev, u8 *buf, unsigned int len);
static int cskey_i2c_read_reg(struct cypress_cskey *cskey_dev, u8 *buf,
						unsigned int len, u8 reg);
static int cskey_led_power(struct cypress_cskey *cskey_dev, bool on);
static int read_fw_version(struct cypress_cskey *cskey_dev);
static int set_sensitivity(struct cypress_cskey *cskey_dev, int level);
static int set_sleep_mode(struct cypress_cskey *cskey_dev, int mode);
static int cskey_configure(struct cypress_cskey *cskey_dev);
static int cskey_suspend(struct device *dev);
static int cskey_resume(struct device *dev);
static int cskey_set_threshold(struct cypress_cskey *cskey_dev, int keycode,
						int val);
static void cskey_threshold_init(struct cypress_cskey *cskey_dev);
extern int get_hall_sensor_state(void);


#if defined(CSKEY_WAKEUP_SUPPORT)
bool check_cskey_deep_sleep_mode(void)
{
	bool need_change = false;

	if (cskey == NULL) {
		goto exit;
	}

	spin_lock(&cskey->lock);
	if ((cskey != NULL) && cskey->device_sleep
		&& (cskey->sleep_mode != REG_SLEEP_MODE_DEEP)
		&& cskey->irq_state != IRQ_STATE_DISABLE)
		need_change = true;
	spin_unlock(&cskey->lock);

exit:
	return need_change;
}

void cskey_deep_sleep_mode(void)
{
	if (cskey == NULL) {
		return;
	}

	queue_work(cskey->cskey_wq, &cskey->change_mode_work);
}

static void cskey_deep_sleep_work(struct work_struct *work)
{
	struct cypress_cskey *cskey_dev = container_of(work,
										struct cypress_cskey, change_mode_work);
	int err = 0;

	cskey->reset(cskey_dev);

	err = set_sleep_mode(cskey_dev, REG_SLEEP_MODE_DEEP);
	if (err)
		dev_err(&cskey_dev->client->dev,
				"[Cypress cskey] Set deep sleep mode failed\n");

	spin_lock(&cskey_dev->lock);
	if ((device_may_wakeup(&cskey_dev->client->dev)
		&& cskey_dev->irq_state == IRQ_STATE_ENABLE_WAKEUP))
		disable_irq_wake(cskey_dev->pdata->irq);
	disable_irq(cskey_dev->pdata->irq);

	cskey_dev->irq_state = IRQ_STATE_DISABLE;
	spin_unlock(&cskey_dev->lock);

	return;
}
#else
bool check_cskey_deep_sleep_mode(void)
{
	return false;
}

void cskey_deep_sleep_mode(void)
{
	return;
}
#endif
EXPORT_SYMBOL_GPL(check_cskey_deep_sleep_mode);
EXPORT_SYMBOL_GPL(cskey_deep_sleep_mode);

#if defined(DEBUG)
static void test_send_key(struct device *dev, int keycode, const char *buf)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int ret, pressed;

	ret = sscanf(buf, "%d", &pressed);
	if ((ret != 1) || (pressed > KEY_STATUS_PRESS)) {
		printk("[Cypress cskey] Get wrong value %d\n", pressed);
		return;
	}

	input_report_key(cskey_dev->input_dev, keycode, pressed);
	input_sync(cskey_dev->input_dev);

	switch(keycode) {
		case KEY_BACK:
			cskey_dev->key_status[STATUS_KEY_BACK] = pressed;
			printk("[Cypress cskey] Send back key %s\n",
						pressed ? "press" : "release");
			break;
		case KEY_HOME:
			cskey_dev->key_status[STATUS_KEY_HOME] = pressed;
			printk("[Cypress cskey] Send home key %s\n",
						pressed ? "press" : "release");
			break;
		case KEY_MENU:
			cskey_dev->key_status[STATUS_KEY_MENU] = pressed;
			printk("[Cypress cskey] Send menu key %s\n",
						pressed ? "press" : "release");
			break;
		default:
			break;
	}
}

static ssize_t backkey_send(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	test_send_key(dev, KEY_BACK, buf);

	return size;
}

static ssize_t homekey_send(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	test_send_key(dev, KEY_HOME, buf);

	return size;
}

static ssize_t menukey_send(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	test_send_key(dev, KEY_MENU, buf);

	return size;
}
#endif

static int key_status_show(struct device *dev, int keycode, char *buf)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int i2c_key_status = 0;
	int key_status = 0;
	int count = 0;
	int err, shift;
	u8 data = 0;

	switch(keycode) {
		case KEY_BACK:
			key_status = cskey_dev->key_status[STATUS_KEY_BACK];
			shift = STATUS_KEY_BACK;
			break;
		case KEY_HOME:
			key_status = cskey_dev->key_status[STATUS_KEY_HOME];
			shift = STATUS_KEY_HOME;
			break;
		case KEY_MENU:
			key_status = cskey_dev->key_status[STATUS_KEY_MENU];
			shift = STATUS_KEY_MENU;
			break;
		default:
			goto exit;
	}

	err = cskey_i2c_read_reg(cskey_dev, &data, I2C_READ_REG_LEN,
								REG_BTN_STATUS);
	if (err < 0)
		goto exit;

	i2c_key_status = (data >> REG_BTN_STATUS_SHIFT) >> shift;
	i2c_key_status &= REG_BTN_STATUS_MASK;

	count = sprintf(buf, "%d, %d\n", key_status, i2c_key_status);

exit:
	return count;
}

static ssize_t backkey_status_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	int count = 0;

	count = key_status_show(dev, KEY_BACK, buf);

	return count;
}

static ssize_t homekey_status_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	int count = 0;

	count = key_status_show(dev, KEY_HOME, buf);

	return count;
}

static ssize_t menukey_status_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	int count = 0;

	count = key_status_show(dev, KEY_MENU, buf);

	return count;
}

static ssize_t cskey_fw_version(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int count, fw_version = 0;
	int err = 0;

	if(cskey_dev != NULL) {
		if (!cskey_dev->device_sleep) {
			err = read_fw_version(cskey_dev);
			if(err) {
				dev_err(&cskey_dev->client->dev,
						"[Cypress cskey] Failed to read FW version\n");
				goto exit;
			}
		}
		fw_version = cskey_dev->fw_version;
	}

exit:
	count = sprintf(buf, "%04x\n", fw_version);

	return count;
}

static ssize_t cskey_dev_reset(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int ret, value;

	ret = sscanf(buf, "%d", &value);
	if (value != 1) {
		printk("[Cypress cskey] Get wrong value %d\n", value);
		goto exit;
	}
	cskey_threshold_init(cskey_dev);
	cskey_dev->reset(cskey_dev);

exit:
	return size;
}

static void cskey_led_control(struct led_classdev *cdev,
						enum led_brightness value)
{
	struct cypress_cskey *cskey_dev = cskey;

	if (value == LED_OFF)
		cskey->led_on = false;
	else
		cskey->led_on = true;

	if((cskey_dev != NULL) && (!cskey_dev->device_sleep))
		cskey_led_power(cskey_dev, cskey->led_on);
}

static ssize_t cskey_sensitivity_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int count = 0;
	int err = 0;
	u8 data = 0;

	err = cskey_i2c_read_reg(cskey_dev, &data, I2C_READ_REG_LEN, REG_SEN_CTRL);
	if (err >= 0) {
		cskey_dev->sensitivity = data;
		err = I2C_TRANSFER_SUCCESS;
	} else {
		goto exit;
	}

	count = sprintf(buf, "%d\n", cskey_dev->sensitivity);

exit:
	return count;
}

static ssize_t cskey_sensitivity_set(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int err = 0;
	int level = 0;

	err = sscanf(buf, "%d", &level);
	if ((err != 1) || (level < REG_SEN_LV1) || (level > REG_SEN_MAX)) {
		printk("[Cypress cskey] Get wrong value %d\n", level);
		goto exit;
	}

	err = set_sensitivity(cskey_dev, level);
	if (err)
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] Set sensitivity failed %d\n", err);

exit:
	return size;
}

static int key_threshold_show(struct device *dev, int keycode, char *buf)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int key_threshold = 0;
	int count = 0;

	switch(keycode) {
		case KEY_BACK:
			key_threshold = cskey_dev->key_threshold[STATUS_KEY_BACK];
			break;
		case KEY_HOME:
			key_threshold = cskey_dev->key_threshold[STATUS_KEY_HOME];
			break;
		case KEY_MENU:
			key_threshold = cskey_dev->key_threshold[STATUS_KEY_MENU];
			break;
		default:
			key_threshold = -1;
	}

	if (cskey_dev->fw_version < REG_THRESHOLD_FW_VER)
		count = sprintf(buf, "Not support\n");
	else if (key_threshold < 0)
		count = sprintf(buf, "unknown\n");
	else
		count = sprintf(buf, "%d\n", key_threshold);

	return count;
}

static ssize_t backkey_threshold_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	int count = 0;

	count = key_threshold_show(dev, KEY_BACK, buf);

	return count;
}

static ssize_t homekey_threshold_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	int count = 0;

	count = key_threshold_show(dev, KEY_HOME, buf);

	return count;
}


static ssize_t menukey_threshold_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	int count = 0;

	count = key_threshold_show(dev, KEY_MENU, buf);

	return count;
}

static int check_threshold_valid(struct cypress_cskey *cskey_dev,
						const char *buf, int *value)
{
	int ret;
	int thd_max_val;

	/*
	 * FW v0206 change register value, so threshold value will be doubled.
	 *
     *    FW version       threshold range
     *   before v0206        1 - 255
     *      v0206            2 - 510
	 */
	if (cskey_dev->fw_version < REG_THRESHOLD_DOUBLE_FW_VER)
		thd_max_val = REG_THRESHOLD_MAX_VAL;
	else
		thd_max_val = REG_THRESHOLD_MAX_VAL * 2;

	ret = sscanf(buf, "%d", value);
	if ((*value > thd_max_val) || (*value < 0) || ret < 0) {
		printk("[Cypress cskey] Get wrong value %d\n", *value);
		ret = -1;
	}

	return ret;
}

static ssize_t backkey_threshold_store(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int value = 0;
	int ret;

	ret = check_threshold_valid(cskey_dev, buf, &value);
	if (ret < 0)
		goto exit;

	cskey_set_threshold(cskey_dev, KEY_BACK, value);

exit:
	return size;
}

static ssize_t homekey_threshold_store(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int value = 0;
	int ret;

	ret = check_threshold_valid(cskey_dev, buf, &value);
	if (ret < 0)
		goto exit;

	cskey_set_threshold(cskey_dev, KEY_HOME, value);

exit:
	return size;
}

static ssize_t menukey_threshold_store(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int value = 0;
	int ret;

	ret = check_threshold_valid(cskey_dev, buf, &value);
	if (ret < 0)
		goto exit;

	cskey_set_threshold(cskey_dev, KEY_MENU, value);

exit:
	return size;
}

#if defined(DEBUG)
static ssize_t cskey_dev_sleep(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int err = 0;

	err = set_sleep_mode(cskey_dev, cskey_dev->sleep_mode);
	if(err < 0)
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] Set sleep mode failed%d\n", err);

	return size;
}
#endif

#if defined(CSKEY_WAKEUP_SUPPORT)
static ssize_t cskey_dev_set_sleep_mode(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int err = 0;
	int mode = 0;

	err = sscanf(buf, "%d", &mode);
	if ((err != 1) || (mode < REG_SLEEP_MODE_DEEP) || (mode >= REG_SLEEP_MODE_MAX)) {
		printk("[Cypress cskey] Get wrong value %d\n", mode);
		goto exit;
	}

	cskey_dev->sleep_mode = mode;
	printk("[Cypress cskey] Set sleep mode %d\n", mode);

exit:
	return size;
}

static ssize_t cskey_dev_sleep_mode_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int count = 0;

	count = sprintf(buf, "%d\n", cskey_dev->sleep_mode);

	return count;
}
#endif

#if defined(CSKEY_KEY_SWITCH_SUPPORT)
static ssize_t cskey_homekey_switch_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	int count = 0;

	count = sprintf(buf, "%d\n", cskey_key_switch);

	return count;
}

static ssize_t cskey_homekey_switch(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	int ret, value;

	ret = sscanf(buf, "%d", &value);
	if ((value > KEY_SWITCH_MAX) || (value < 0)) {
		printk("[Cypress cskey] Get wrong value %d\n", value);
		goto exit;
	}

	if (value)
		cskey_key_switch = true;
	else
		cskey_key_switch = false;

exit:
	return size;
}
#endif

#if defined(CSKEY_RECOVERY_SUPPORT)
static ssize_t cskey_dev_sw_reset(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int ret, value;

	ret = sscanf(buf, "%d", &value);
	if (value < 0) {
		printk("[Cypress cskey] Get wrong value %d\n", value);
		goto exit;
	}

#if defined(DEBUG)
	printk("[Cypress cskey] SW reset signal after %d ms\n",
				value * CSKEY_RECOVERY_DELAY_BASE);
#endif
	queue_delayed_work(cskey_dev->cskey_wq, &cskey_dev->recovery_work,
						msecs_to_jiffies(value * CSKEY_RECOVERY_DELAY_BASE));

exit:
	return size;
}
#endif

#if defined(DEBUG)
static DEVICE_ATTR(key_back, S_IRUGO | S_IWUSR | S_IWGRP, backkey_status_show,
					backkey_send);
static DEVICE_ATTR(key_home, S_IRUGO | S_IWUSR | S_IWGRP, homekey_status_show,
					homekey_send);
static DEVICE_ATTR(key_menu, S_IRUGO | S_IWUSR | S_IWGRP, menukey_status_show,
					menukey_send);
static DEVICE_ATTR(dev_sleep, S_IWUSR, NULL, cskey_dev_sleep);
#else
static DEVICE_ATTR(key_back, S_IRUGO, backkey_status_show, NULL);
static DEVICE_ATTR(key_home, S_IRUGO, homekey_status_show, NULL);
static DEVICE_ATTR(key_menu, S_IRUGO, menukey_status_show, NULL);
#endif
#if defined(CSKEY_WAKEUP_SUPPORT)
static DEVICE_ATTR(dev_sleep_mode, S_IRUGO | S_IWUSR, cskey_dev_sleep_mode_show,
					cskey_dev_set_sleep_mode);
#endif
static DEVICE_ATTR(fw_version, S_IRUGO, cskey_fw_version, NULL);
static DEVICE_ATTR(dev_reset, S_IWUSR, NULL, cskey_dev_reset);
static DEVICE_ATTR(sensitivity, S_IRUGO | S_IWUSR, cskey_sensitivity_show,
					cskey_sensitivity_set);
#if defined(CSKEY_KEY_SWITCH_SUPPORT)
static DEVICE_ATTR(homekey_switch, S_IRUGO | S_IWUSR, cskey_homekey_switch_show,
					cskey_homekey_switch);
#endif
#if defined(CSKEY_RECOVERY_SUPPORT)
static DEVICE_ATTR(dev_sw_reset, S_IWUSR, NULL, cskey_dev_sw_reset);
#endif
static DEVICE_ATTR(key_back_thd, S_IRUGO | S_IWUSR, backkey_threshold_show,
					backkey_threshold_store);
static DEVICE_ATTR(key_home_thd, S_IRUGO | S_IWUSR, homekey_threshold_show,
					homekey_threshold_store);
static DEVICE_ATTR(key_menu_thd, S_IRUGO | S_IWUSR, menukey_threshold_show,
					menukey_threshold_store);

static struct attribute *cskey_attrs[] = {
	&dev_attr_key_back.attr,
	&dev_attr_key_home.attr,
	&dev_attr_key_menu.attr,
#if defined(DEBUG)
	&dev_attr_dev_sleep.attr,
#endif
	&dev_attr_key_back_thd.attr,
	&dev_attr_key_home_thd.attr,
	&dev_attr_key_menu_thd.attr,
#if defined(CSKEY_WAKEUP_SUPPORT)
	&dev_attr_dev_sleep_mode.attr,
#endif
#if defined(CSKEY_RECOVERY_SUPPORT)
	&dev_attr_dev_sw_reset.attr,
#endif
	&dev_attr_fw_version.attr,
	&dev_attr_dev_reset.attr,
	&dev_attr_sensitivity.attr,
#if defined(CSKEY_KEY_SWITCH_SUPPORT)
	&dev_attr_homekey_switch.attr,
#endif
	NULL,
};

static const struct attribute_group cskey_attr_group = {
	.attrs = cskey_attrs,
};

static ssize_t vendor_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char * s = buf;
	s32 err = 0;

	if (!cskey->device_sleep)
		err = read_fw_version(cskey);

	if(err == 0)
		s += sprintf(s, "Cypress\n");
	else
		s += sprintf(s, "unknow\n");
	return (s - buf);
}

static ssize_t firmware_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char * s = buf;

	s += sprintf(s, "%04x\n", cskey->fw_version);
	return (s - buf);
}

#define debug_attr(_name) \
		static struct kobj_attribute _name##_attr = { \
		.attr = { \
		.name = __stringify(_name), \
		.mode = 0644, \
		}, \
		.show = _name##_show, \
		}

debug_attr(vendor);
debug_attr(firmware);

static struct attribute *dev_info_attrs[] = {
	&vendor_attr.attr,
	&firmware_attr.attr,
	NULL,
};
static const struct attribute_group dev_info_attr_group = {
	.attrs = dev_info_attrs,
};

static int i2c_power_on(struct cypress_cskey *cskey_dev, bool on)
{
	struct cskey_platform_data *pdata = cskey_dev->pdata;
	int rc = 0;

	if ((cskey_dev->i2c_power_on) == on) {
		printk("[Cypress cskey] I2C power already on\n");
		goto exit;
	}

	cskey_dev->i2c_power_on = on;

	if (on == false)
		goto power_off;

	if (pdata->i2c_pull_up && cskey_dev->vcc_i2c) {
		rc = regulator_enable(cskey_dev->vcc_i2c);
		if(rc) {
			dev_err(&cskey_dev->client->dev,
				"[Cypress cskey] Regulator vcc_i2c enable failed rc=%d\n", rc);
			goto exit;
		}
		msleep(10);
	}

exit:
	return rc;

power_off:
	if (pdata->i2c_pull_up && cskey_dev->vcc_i2c) {
		regulator_disable(cskey_dev->vcc_i2c);
		msleep(10);
	}

	return 0;
}

static int cskey_regulator_configure(struct cypress_cskey *cskey_dev, bool on)
{
	struct cskey_platform_data *pdata = cskey_dev->pdata;
	int rc = 0;

	if (on == false)
		goto hw_shutdown;

	if (pdata->i2c_pull_up) {
		cskey_dev->vcc_i2c = regulator_get(&cskey_dev->client->dev, "vcc_i2c");
		if (IS_ERR(cskey_dev->vcc_i2c)) {
			rc = PTR_ERR(cskey_dev->vcc_i2c);
			dev_err(&cskey_dev->client->dev,
			"[Cypress cskey] Regulator get failed rc=%d\n",	rc);
			return rc;
		}
		if (regulator_count_voltages(cskey_dev->vcc_i2c) > 0) {
			rc = regulator_set_voltage(cskey_dev->vcc_i2c,
				I2C_PULL_UP_MIN_UV, I2C_PULL_UP_MAX_UV);
			if (rc) {
				dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] Regulator set_vtg failed rc=%d\n", rc);
				goto err_set_i2c_vreg;
			}
		}
	}

	return 0;

err_set_i2c_vreg:
	regulator_put(cskey_dev->vcc_i2c);
	return rc;

hw_shutdown:
	if (cskey_dev->vcc_i2c != NULL && pdata->i2c_pull_up) {
		if (regulator_count_voltages(cskey_dev->vcc_i2c) > 0)
			regulator_set_voltage(cskey_dev->vcc_i2c, 0,
						I2C_PULL_UP_MAX_UV);
		regulator_put(cskey_dev->vcc_i2c);
	}

	return 0;
}

#if defined(CONFIG_FB)
static int cskey_fb_notifier_callback(struct notifier_block *self,
						unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	int err = 0;
	struct cypress_cskey *cskey_dev =
		container_of(self, struct cypress_cskey, fb_notif);

	if (evdata && evdata->data && event == FB_EVENT_BLANK && cskey_dev &&
			cskey_dev->client) {
		blank = evdata->data;

		if (*blank == cskey_dev->fb_status)
			goto exit;

		if (*blank == FB_BLANK_UNBLANK)
			err = cskey_resume(&cskey_dev->client->dev);
		else if (*blank == FB_BLANK_POWERDOWN)
			err = cskey_suspend(&cskey_dev->client->dev);

		if (err == 0)
			cskey_dev->fb_status = *blank;
	}

exit:
	return err;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void cskey_early_suspend(struct early_suspend *h)
{
	struct cypress_cskey *cskey_dev = container_of(h, struct cypress_cskey,
										early_suspend);

	cskey_suspend(&cskey_dev->client->dev);
}

static void cskey_late_resume(struct early_suspend *h)
{
	struct cypress_cskey *cskey_dev = container_of(h, struct cypress_cskey,
										early_suspend);

	cskey_resume(&cskey_dev->client->dev);
}
#endif

#if defined(CONFIG_PM)
#if defined(CSKEY_WAKEUP_SUPPORT)
static int cskey_suspend(struct device *dev)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int err = 0;
	int sleep_mode = REG_SLEEP_MODE_DEEP;

	if (acer_boot_mode == CHARGER_BOOT)
		goto exit;

	if (cskey_dev->device_sleep)
		goto exit;

	if (get_hall_sensor_state() > 0) {
		spin_lock(&cskey->lock);
		disable_irq(cskey_dev->pdata->irq);
		cskey_dev->irq_state = IRQ_STATE_DISABLE;
		sleep_mode = REG_SLEEP_MODE_DEEP;
		spin_unlock(&cskey->lock);
	} else if (device_may_wakeup(dev)) {
		spin_lock(&cskey->lock);
		enable_irq_wake(cskey_dev->pdata->irq);
		cskey_dev->irq_state = IRQ_STATE_ENABLE_WAKEUP;
		sleep_mode = cskey_dev->sleep_mode;
		spin_unlock(&cskey->lock);
	}
	err = set_sleep_mode(cskey_dev, sleep_mode);

#if defined(DEBUG)
	if (err == 0)
		printk("[Cypress cskey] cskey set sleep mode success\n");
#endif
	i2c_power_on(cskey_dev, false);

exit:
	return err;
}

static void cskey_resume_work(struct work_struct *work)
{
	struct cypress_cskey *cskey_dev = container_of(work,
										struct cypress_cskey, resume_work);

	spin_lock(&cskey->lock);
	switch (cskey_dev->irq_state) {
		case IRQ_STATE_DISABLE:
			enable_irq(cskey_dev->pdata->irq);
			break;
		case IRQ_STATE_ENABLE_WAKEUP:
		case IRQ_STATE_WAKEUP:
			if (device_may_wakeup(&cskey_dev->client->dev))
				disable_irq_wake(cskey_dev->pdata->irq);
			break;
		default:
			dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] Abnormal irq state\n");
			break;
	}
	cskey_dev->irq_state = IRQ_STATE_ENABLE;
	spin_unlock(&cskey->lock);

	i2c_power_on(cskey_dev, true);
	cskey_dev->reset(cskey_dev);

	return;
}
#else
static int cskey_suspend(struct device *dev)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int err = 0;

	if (acer_boot_mode == CHARGER_BOOT)
		goto exit;

	disable_irq(cskey_dev->pdata->irq);
	err = set_sleep_mode(cskey_dev, cskey_dev->sleep_mode);
#if defined(DEBUG)
	if (err == 0)
		printk("[Cypress cskey] cskey set sleep mode success\n");
#endif
	i2c_power_on(cskey_dev, false);

exit:
	return err;
}

static void cskey_resume_work(struct work_struct *work)
{
	struct cypress_cskey *cskey_dev = container_of(work,
										struct cypress_cskey, resume_work);

	enable_irq(cskey_dev->pdata->irq);
	i2c_power_on(cskey_dev, true);
	cskey_dev->reset(cskey_dev);

	return;
}
#endif

static int cskey_resume(struct device *dev)
{
	struct cypress_cskey *cskey_dev = dev_get_drvdata(dev);
	int err = 0;

	if (acer_boot_mode == CHARGER_BOOT)
		goto exit;

	queue_work(cskey_dev->cskey_wq, &cskey_dev->resume_work);

exit:
	return err;
}

static const struct dev_pm_ops cskey_pm_ops = {
#if (!defined(CONFIG_FB) && !defined(CONFIG_HAS_EARLYSUSPEND))
	.suspend	= cskey_suspend,
	.resume		= cskey_resume,
#endif
};
#endif

static irqreturn_t cskey_irq_handler(int irq, void *dev_id)
{
	struct cypress_cskey *cskey_dev = (struct cypress_cskey *)dev_id;

	queue_work(cskey_dev->cskey_wq, &cskey_dev->key_event_work);

	return IRQ_HANDLED;
}

static int cskey_reset(struct cypress_cskey *cskey_dev)
{
	struct cskey_platform_data *pdata = cskey_dev->pdata;
	struct i2c_client *client = cskey_dev->client;
	int i, err = 0;

	err = gpio_direction_output(pdata->reset_gpio, 1);
	if (err) {
		dev_err(&client->dev,
				"[Cypress cskey] Unable to set direction for reset gpio %d\n",
				pdata->reset_gpio);
		goto exit;
	}

	/* Minimum txres pulse length is 300us */
	usleep(300);

	err = gpio_direction_output(pdata->reset_gpio, 0);
	if (err) {
		dev_err(&client->dev,
				"[Cypress cskey] Unable to set direction for reset gpio %d\n",
				pdata->reset_gpio);
		goto exit;
	}

	mdelay(50);

	/* re-configure cskey */
	err = cskey_configure(cskey_dev);
	if (err)
		dev_err(&client->dev,
				"[Cypress cskey] Re-configure register failed\n");
	else
		cskey_dev->device_sleep = false;

	for (i = 0; i < STATUS_KEY_MAX; i++)
		cskey_dev->key_status[i] = 0;

exit:
	return err;
}

static int cskey_i2c_write(struct cypress_cskey *cskey_dev, u8 *buf,
						unsigned int len)
{
	struct i2c_client *client = cskey_dev->client;
	struct i2c_msg msg[1];
	int err = 0;
	int retry = I2C_ERR_RETRY;

	if (client == NULL) {
		printk("[Cypress cskey] Invalid i2c client\n");
		return -EINVAL;
	}

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = len;
	msg->buf = buf;

	while(retry--) {
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0)
			break;

		dev_err(&client->dev, "[Cypress cskey] I2C write failed %d\n", err);
		mdelay(10);
	}

	return err;
}

static int cskey_i2c_read_reg(struct cypress_cskey *cskey_dev, u8 *buf,
						unsigned int len, u8 reg)
{
	struct i2c_client *client = cskey_dev->client;
	struct i2c_msg msg[2];
	int err = 0;
	int retry = I2C_ERR_RETRY;
	u8 reg_data = reg;

	if (client == NULL) {
		printk("[Cypress cskey] Invalid i2c client\n");
		return -EINVAL;
	}

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &reg_data;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = buf;

	while(retry--) {
		err = i2c_transfer(client->adapter, msg, 2);
		if (err >= 0)
			break;

		dev_err(&client->dev, "[Cypress cskey] I2C read failed %d\n", err);
		mdelay(10);
	}

	return err;
}

static int cskey_i2c_write_reg(struct cypress_cskey *cskey_dev, u8 value,
						u8 reg)
{
	u8 data[I2C_REG_CTRL_LEN] = {0};
	int ret = 0;

	data[0] = reg;
	data[1] = value;

	ret = cskey_i2c_write(cskey_dev, data, I2C_REG_CTRL_LEN);
	if (ret > 0)
		ret = I2C_TRANSFER_SUCCESS;

	return ret;
}

static int read_fw_version(struct cypress_cskey *cskey_dev)
{
	u8 data[I2C_READ_FW_LEN] = {0};
	int err = 0;

	err = cskey_i2c_read_reg(cskey_dev, data, I2C_READ_FW_LEN, REG_FW_VER);
	if (err >= 0) {
		cskey_dev->fw_version = (data[0] << REG_FW_VER_SHIFT) | data[1];
		printk("[Cypress cskey] FW version : %04x\n", cskey_dev->fw_version);
		err = I2C_TRANSFER_SUCCESS;
	} else
		dev_err(&cskey_dev->client->dev,
						"[Cypress cskey] Read FW version failed\n");

	return err;
}

static int set_led_auto_control_mode(struct cypress_cskey *cskey_dev, bool led_auto)
{
	int err = 0;
	u8 led_data = REG_CTRL_INT;

	if (!led_auto)
		led_data |= REG_CTRL_LED_MODE;
	if (cskey->led_on)
		led_data |= REG_CTRL_LED_ON;

	err = cskey_i2c_write_reg(cskey_dev, led_data, REG_CTRL);
	if (err < 0)
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] LED control setting failed\n");

	return err;
}

static int set_sensitivity(struct cypress_cskey *cskey_dev, int level)
{
	int err = 0;

	err = cskey_i2c_write_reg(cskey_dev, level, REG_SEN_CTRL);
	if (err < 0)
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] Sensitivity setting failed\n");

	cskey_dev->sensitivity = level;

	return err;
}

static int set_sleep_mode(struct cypress_cskey *cskey_dev, int mode)
{
	int err = 0;

	err = cskey_i2c_write_reg(cskey_dev, mode, REG_SLEEP_CTRL);
	if (err < 0)
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] Enter sleep mode failed\n");
	else
		cskey_dev->device_sleep = true;

	return err;
}

static int cskey_set_threshold(struct cypress_cskey *cskey_dev, int keycode,
						int val)
{
	int err = 0;
	int index = 0;
	u8 reg = 0;
	u8 thd_val = 0;

	if (cskey_dev->fw_version < REG_THRESHOLD_FW_VER)
		goto exit;

	if (cskey_dev->fw_version >= REG_THRESHOLD_DOUBLE_FW_VER)
		thd_val = (u8)((val / 2) & 0xFF);
	else
		thd_val = (u8)(val & 0xFF);

	switch(keycode) {
		case KEY_BACK:
			reg = REG_KEY_BACK_THRESHOLD;
			index = STATUS_KEY_BACK;
			break;
		case KEY_HOME:
			reg = REG_KEY_HOME_THRESHOLD;
			index = STATUS_KEY_HOME;
			break;
		case KEY_MENU:
			reg = REG_KEY_MENU_THRESHOLD;
			index = STATUS_KEY_MENU;
			break;
	}

	err = cskey_i2c_write_reg(cskey_dev, thd_val, reg);

	if (err < 0)
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] Set key threshold failed\n");
	else
		cskey_dev->key_threshold[index] = val;

exit:
	return err;
}

static int cskey_clear_interrupt(struct cypress_cskey *cskey_dev)
{
	int err = 0;

	err = cskey_i2c_write_reg(cskey_dev, REG_INT_CLR_VAL, REG_INT_CLR_REG);
	if (err < 0)
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] Clear interrupt failed\n");

	return err;
}

static int cskey_led_power(struct cypress_cskey *cskey_dev, bool on)
{
	int err = 0;
	u8 led_data = REG_CTRL_INT | REG_CTRL_LED_MODE;

	if (on)
		led_data |= REG_CTRL_LED_ON;

	err = cskey_i2c_write_reg(cskey_dev, led_data, REG_CTRL);
	if (err < 0)
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] Set LED power failed %d\n", err);

	return err;
}

static void cskey_threshold_init(struct cypress_cskey *cskey_dev)
{
	/* DVT5(PVT) sensitivity worse than DVT4 */
	if (acer_board_id >= HW_ID_PVT)
		cskey_dev->key_threshold[STATUS_KEY_BACK] = REG_THRESHOLD_BACK_DEF_L;
	else
		cskey_dev->key_threshold[STATUS_KEY_BACK] = REG_THRESHOLD_BACK_DEF_H;
	cskey_dev->key_threshold[STATUS_KEY_HOME] = REG_THRESHOLD_HOME_DEF;
	cskey_dev->key_threshold[STATUS_KEY_MENU] = REG_THRESHOLD_MENU_DEF;

#if defined(CSKEY_RECOVERY_SUPPORT)
	cskey_dev->threshold_fix_count[STATUS_KEY_BACK] = 0;
	cskey_dev->threshold_fix_count[STATUS_KEY_HOME] = 0;
	cskey_dev->threshold_fix_count[STATUS_KEY_MENU] = 0;
#endif
}

#if defined(CSKEY_WAKEUP_SUPPORT)
static void cskey_wakeup_device(struct cypress_cskey *cskey_dev, int keycode)
{
	input_report_key(cskey_dev->input_dev, keycode, CSKEY_KEY_PRESS);
	input_sync(cskey_dev->input_dev);
	input_report_key(cskey_dev->input_dev, keycode, CSKEY_KEY_RELEASE);
	input_sync(cskey_dev->input_dev);
}
#endif

#if defined(CSKEY_RECOVERY_SUPPORT)
static int cskey_signal_diff_ctrl(struct cypress_cskey *cskey_dev, bool enable)
{
	int err = -1;
	int ctrl_val = 0x0;

	if (enable)
		ctrl_val = REG_SIGNAL_DIFF_VAL;

	err = cskey_i2c_write_reg(cskey_dev, ctrl_val, REG_SIGNAL_DIFF_CTRL);
	if (err < 0)
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] Signal diff control failed %d\n", err);

	return err;
}

static int cskey_get_signal_diff(struct cypress_cskey *cskey_dev, int keycode, u16 *val)
{
	u8 reg;
	u8 diff_val[I2C_REG_DIFF_LEN] = {0};
	int err = -1;

	switch(keycode) {
		case KEY_BACK:
			reg = REG_SIGNAL_DIFF_BACK;
			break;
		case KEY_HOME:
			reg = REG_SIGNAL_DIFF_HOME;
			break;
		case KEY_MENU:
			reg = REG_SIGNAL_DIFF_MENU;
			break;
		default:
			dev_err(&cskey_dev->client->dev,
						"[Cypress cskey] Invalid keycode %d\n", keycode);
			goto exit;
	}

	err = cskey_i2c_read_reg(cskey_dev, diff_val, I2C_REG_DIFF_LEN, reg);
	if (err < 0) {
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] Get key signal diff failed %d\n", err);
		goto exit;
	}

	*val = (diff_val[0] << 8) + diff_val[1];

exit:
	return err;
}

static int cskey_sw_reset(struct cypress_cskey *cskey_dev)
{
	int err = 0;
	int i;

	err = cskey_i2c_write_reg(cskey_dev, REG_RESET_COUNT, REG_SW_RESET);
	if (err < 0)
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] SW reset fail %d\n", err);
	else
		printk("[Cypress cskey] cskey sw reset\n");

	for (i = 0; i < STATUS_KEY_MAX; i++)
		cskey_dev->key_recovery[i] = false;

	return err;
}

static void cskey_recovery(struct work_struct *work)
{
	struct cypress_cskey *cskey_dev = container_of(work, struct cypress_cskey,
										recovery_work.work);
	int i;

	if (cskey_dev == NULL)
		return;

	for (i = 0; i < STATUS_KEY_MAX; i++) {
		if (cskey_dev->key_recovery[i])
			continue;

		if (cskey_dev->key_status[i] != CSKEY_KEY_RELEASE) {
#if defined(DEBUG)
			printk("[Cypress cskey] key %d pressed delay %d ms\n", i,
						CSKEY_RECOVERY_DELAY_S);
#endif
			queue_delayed_work(cskey_dev->cskey_wq, &cskey_dev->recovery_work,
								msecs_to_jiffies(CSKEY_RECOVERY_DELAY_S));
			return;
		}
	}

	cskey_sw_reset(cskey_dev);
}

static void check_key_valid(struct cypress_cskey *cskey_dev, int index)
{
	unsigned long msec_time = 0;
	int err = 0;

	cskey_dev->press_time_prev[index] = cskey_dev->press_time[index];
	cskey_dev->press_time[index] = cpu_clock(UINT_MAX);
	msec_time = (unsigned long)(cskey_dev->press_time[index]
					- cskey_dev->press_time_prev[index]) / 1000000;

	/* time < 100ms */
	if (msec_time <= 100) {
		char  key_name[CSKEY_BUF_LEN];

		switch(index) {
			case STATUS_KEY_BACK:
				sprintf(key_name, "Back key");
				break;
			case STATUS_KEY_HOME:
				sprintf(key_name, "Home key");
				break;
			case STATUS_KEY_MENU:
				sprintf(key_name, "Menu key");
				break;
		}
		cskey_dev->key_recovery[index] = true;
		printk("[Cypress cskey] %s abnormal trigger(%lu ms)\n", key_name, msec_time);
		/*
		 * if key abnormal trigger occurred, try to increase key
		 * threshold to avoid.
		 */
		if ((cskey_dev->threshold_fix_count[index]++) <
					(REG_THRESHOLD_INC_BASE * index)) {
			err = cskey_set_threshold(cskey_dev, cs_keycode[index],
					cskey_dev->key_threshold[index] + REG_THRESHOLD_INC_VAL);
			if (err >= 0)
				printk("[Cypress cskey] Increase %s threshold to %d\n",
						key_name, cskey_dev->key_threshold[index]);
		}

		usleep(200000);
		queue_delayed_work(cskey_dev->cskey_wq, &cskey_dev->recovery_work,
							msecs_to_jiffies(CSKEY_RECOVERY_DELAY_L));
	}
}

static void check_key_diff_valid(struct cypress_cskey *cskey_dev, int keycode)
{
	u16 diff_val = 0;

	cskey_get_signal_diff(cskey_dev, keycode, &diff_val);
	if (diff_val >= REG_SIGNAL_DIFF_INVALID) {
		char  key_name[CSKEY_BUF_LEN];

		switch(keycode) {
			case KEY_BACK:
				sprintf(key_name, "Back key");
				break;
			case KEY_HOME:
				sprintf(key_name, "Home key");
				break;
			case KEY_MENU:
				sprintf(key_name, "Menu key");
		}
		dev_err(&cskey_dev->client->dev,
					"[Cypress cskey] %s siganl diff value too large(%d)\n",
					 key_name, diff_val);
		cskey_sw_reset(cskey_dev);
	}
}
#endif

static int cskey_configure(struct cypress_cskey *cskey_dev)
{
	int err = 0;

	/* read FW version */
	err = read_fw_version(cskey_dev);
	if (err)
		goto exit;

	/* LED control mode setting */
	err = set_led_auto_control_mode(cskey_dev, false);
	if (err)
		goto exit;

	/* Sensitivity level setting */
	err = set_sensitivity(cskey_dev, REG_SEN_LV1);
	if (err)
		goto exit;

	/* Key detect threshold setting */
	err = cskey_set_threshold(cskey_dev, KEY_BACK,
					cskey_dev->key_threshold[STATUS_KEY_BACK]);
	if (err)
		goto exit;
	err = cskey_set_threshold(cskey_dev, KEY_HOME,
					cskey_dev->key_threshold[STATUS_KEY_HOME]);
	if (err)
		goto exit;
	err = cskey_set_threshold(cskey_dev, KEY_MENU,
					cskey_dev->key_threshold[STATUS_KEY_MENU]);
	if (err)
		goto exit;

#if defined(CSKEY_RECOVERY_SUPPORT)
	cskey_signal_diff_ctrl(cskey_dev, true);
#endif

exit:
	return err;
}

static void update_key_event(struct work_struct *work)
{
	struct cypress_cskey *cskey_dev = container_of(work,
										struct cypress_cskey, key_event_work);
	u8 data = 0;
	u8 pressed, changed;
	int i, err = 0;

#if defined(CSKEY_WAKEUP_SUPPORT)
	if (cskey_dev->irq_state == IRQ_STATE_WAKEUP)
		return;

	if (cskey_dev->device_sleep) {
		printk("[Cypress cskey] wakeup device\n");
		spin_lock(&cskey->lock);
		cskey_dev->irq_state = IRQ_STATE_WAKEUP;
		spin_unlock(&cskey->lock);
		cskey_wakeup_device(cskey_dev, KEY_POWER);
		return;
	}
#endif

	err = cskey_i2c_read_reg(cskey_dev, &data, I2C_READ_REG_LEN,
								REG_BTN_STATUS);
	if (err >= 0) {
#if defined(DEBUG)
		printk("[Cypress cskey] Read key data %02x\n", (unsigned int)data);
#endif
	} else {
		dev_err(&cskey_dev->client->dev,
				"[Cypress cskey] I2C read key failed - reset\n");
		cskey_dev->reset(cskey_dev);
		msleep(10);
	}

	for (i = 0; i < STATUS_KEY_MAX; i++)
	{
		/*
		 * Button bit map:
		 *		[0:3] button label bit
		 *		[4:7] button status bit
		 */
		changed = (data & REG_BTN_R_LEBEL_MASK);
		pressed = (data & REG_BTN_R_STATUS_MASK) >> REG_BTN_STATUS_SHIFT;
		if(changed && (cskey_dev->key_status[i] != pressed)) {
#if defined(CSKEY_RECOVERY_SUPPORT)
			if (changed && pressed)
				check_key_valid(cskey_dev, i);
#endif
#if defined(CSKEY_KEY_SWITCH_SUPPORT)
			if (cskey_key_switch)
				input_report_key(cskey_dev->input_dev, cs_keycode_switch[i],
									pressed);
			else
#endif
				input_report_key(cskey_dev->input_dev, cs_keycode[i], pressed);
			input_sync(cskey_dev->input_dev);
			cskey_dev->key_status[i] = pressed;
#if defined(DEBUG)
			switch(i) {
				case STATUS_KEY_BACK:
					printk("[Cypress cskey] Back key %s\n",
								pressed ? "press" : "release");
					break;
				case STATUS_KEY_HOME:
					printk("[Cypress cskey] Home key %s\n",
								pressed ? "press" : "release");
					break;
				case STATUS_KEY_MENU:
					printk("[Cypress cskey] Menu key %s\n",
								pressed ? "press" : "release");
					break;
				default:
					printk("[Cypress cskey] Invalid key\n");
					break;
			}
#endif
		}
		data >>= REG_BTN_NEXT_SHIFT;
	}

#if defined(CSKEY_RECOVERY_SUPPORT)
	/* FW v0207 add release key debounce */
	if (cskey_dev->fw_version < CSKEY_REL_DEBOUNCE_FW_VER)
		goto exit;

	for (i = 0; i < STATUS_KEY_MAX; i++)
		if (cskey_dev->key_status[i] == CSKEY_KEY_PRESS)
			goto exit;

	check_key_diff_valid(cskey_dev, KEY_BACK);

exit:
#endif
	cskey_clear_interrupt(cskey_dev);
}

static int cskey_parse_dt(struct device *dev,
						struct cskey_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	u32 gpio_flag;

	pdata->reset_gpio = of_get_named_gpio_flags(np, "cypress,reset-gpio",
							0, &gpio_flag);
	if (gpio_is_valid(pdata->reset_gpio)) {
		pdata->reset_gpio_flags = gpio_flag;
	} else
		return -EINVAL;

	pdata->irq_gpio = of_get_named_gpio_flags(np, "cypress,irq-gpio",
							0, &gpio_flag);
	if (gpio_is_valid(pdata->irq_gpio))
		pdata->irq_gpio_flags = gpio_flag;
	else
		return -EINVAL;

	pdata->irq = gpio_to_irq(pdata->irq_gpio);
	pdata->i2c_pull_up = of_property_read_bool(np, "cypress,i2c-pull-up");

	return 0;
}

static int __devinit capsense_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct cskey_platform_data *pdata = client->dev.platform_data;
	struct cypress_cskey *cskey_dev;
	struct input_dev *input_dev;
	int err = 0, key_count, i;

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct cskey_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev,
					"[Cypress cskey] Failed to allocate memory\n");
			return -ENOMEM;
		}

		err = cskey_parse_dt(&client->dev, pdata);
		if (err)
			goto err_free_mem;

	}

	if (!pdata)
		return -EINVAL;

	cskey_dev = kzalloc(sizeof(struct cypress_cskey), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!cskey_dev || !input_dev) {
		dev_err(&client->dev,
				"[Cypress cskey] Failed to allocat input device\n");
		err = -ENOMEM;
		goto err_free_input_dev;
	}

	input_dev->name = "capsense-key";
	input_dev->phys = "capsense-key/input0";
	input_dev->dev.parent = &client->dev;
	input_dev->id.bustype = BUS_HOST;

	cskey_dev->client = client;
	cskey_dev->pdata = pdata;
	cskey_dev->input_dev = input_dev;
	cskey_dev->name = "capsense-key";

	__set_bit(EV_KEY, input_dev->evbit);

	key_count = sizeof(cs_keycode) / sizeof(int);
	for (i = 0; i < key_count; i++)
		__set_bit(cs_keycode[i], input_dev->keybit);
#if defined(CSKEY_KEY_SWITCH_SUPPORT)
	key_count = sizeof(cs_keycode_switch) / sizeof(int);
	for (i = 0; i < key_count; i++)
		__set_bit(cs_keycode_switch[i], input_dev->keybit);
#endif
#if defined(CSKEY_WAKEUP_SUPPORT)
	__set_bit(KEY_POWER, input_dev->keybit);
#endif

	i2c_set_clientdata(client, cskey_dev);

	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
				"[Cypress cskey] Failed to register input device\n");
		goto err_free_input_dev;
	}

	err = cskey_regulator_configure(cskey_dev, true);
	if (err) {
		dev_err(&client->dev,
				"[Cypress cskey] Failed to intialize regulator\n");
		goto err_reglator_on;
	}

	err = i2c_power_on(cskey_dev, true);
	if (err) {
		dev_err(&client->dev,
				"[Cypress cskey] Failed to power on i2c pull up\n");
		goto err_i2c_on;
	}

	/* GPIO irq */
	err = gpio_request(pdata->irq_gpio, "cskey-irq-gpio");
	if (err) {
		dev_err(&client->dev,
				"[Cypress cskey] Failed to request irq gpio\n");
		goto err_unregister_dev;
	}
	err = gpio_direction_input(pdata->irq_gpio);
	if (err) {
		dev_err(&client->dev,
				"[Cypress cskey] Unable to set direction for irq gpio %d\n",
				pdata->irq_gpio);
		goto err_free_irq_gpio;
	}
	err = request_threaded_irq(pdata->irq, NULL, cskey_irq_handler,
								pdata->irq_gpio_flags, cskey_dev->name,
								cskey_dev);
	if (err) {
		dev_err(&client->dev,
				"[Cypress cskey] Failed to request irq %d\n", pdata->irq);
		goto err_free_irq_gpio;
	}

	/* GPIO reset */
	err = gpio_request(pdata->reset_gpio, "cskey-reset-gpio");
	if (err) {
		dev_err(&client->dev,
				"[Cypress cskey] Failed to request reset gpio\n");
		goto err_free_irq;
	}
	err = gpio_direction_output(pdata->reset_gpio, 0);	//Active high
	if (err) {
		dev_err(&client->dev,
				"[Cypress cskey] Unable to set direction for reset gpio %d\n",
				pdata->reset_gpio);
		goto err_free_reset_gpio;
	}
	cskey_dev->reset = cskey_reset;

	err = sysfs_create_group(&client->dev.kobj, &cskey_attr_group);
	if (err)
		goto err_free_reset_gpio;

	err = led_classdev_register(&client->dev, &cskey_led);
	if (err)
		goto err_free_reset_gpio;
	cskey = cskey_dev;

#if defined(CONFIG_FB)
	cskey_dev->fb_notif.notifier_call = cskey_fb_notifier_callback;
	err = fb_register_client(&cskey_dev->fb_notif);
	if (err) {
		dev_err(&client->dev,
				"[Cypress cskey] Failed to register fb_notifier:%d\n", err);
		goto err_free_reset_gpio;
	}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	cskey_dev->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	cskey_dev->early_suspend.suspend = (void *)cskey_early_suspend;
	cskey_dev->early_suspend.resume = (void *)cskey_late_resume;
	register_early_suspend(&cskey_dev->early_suspend);
#endif

	cskey_dev->cskey_wq = create_singlethread_workqueue("cskey_wq");
	if (!cskey_dev->cskey_wq) {
		dev_err(&client->dev,
				"[Cypress cskey] Failed to create workqueue\n");
		err = -ENOMEM;
		goto err_free_reset_gpio;
	}
	INIT_WORK(&cskey_dev->key_event_work, update_key_event);
	INIT_WORK(&cskey_dev->resume_work, cskey_resume_work);
#if defined(CSKEY_RECOVERY_SUPPORT)
	INIT_DELAYED_WORK(&cskey->recovery_work, cskey_recovery);
#endif

#if defined(CSKEY_WAKEUP_SUPPORT)
	INIT_WORK(&cskey_dev->change_mode_work, cskey_deep_sleep_work);
	device_init_wakeup(&client->dev, 1);
	spin_lock_init(&cskey_dev->lock);
#endif

	if (acer_boot_mode != CHARGER_BOOT) {
		/* Default LED ON when normal boot */
		cskey_dev->led_on = true;
#if defined(CSKEY_WAKEUP_SUPPORT)
		cskey_dev->sleep_mode = REG_SLEEP_MODE_WAKEUP_2S;
#else
		cskey_dev->sleep_mode = REG_SLEEP_MODE_DEEP;
#endif
	} else {
		cskey_dev->sleep_mode = REG_SLEEP_MODE_DEEP;
	}

	cskey_threshold_init(cskey_dev);

	if (cskey_configure(cskey_dev)) {
		dev_err(&client->dev,
				"[Cypress cskey] Configure register failed\n");
		goto err_ignore;
	}

	if (acer_boot_mode == CHARGER_BOOT) {
		if (set_sleep_mode(cskey_dev, cskey_dev->sleep_mode) < 0) {
			i2c_power_on(cskey_dev, false);
			goto err_ignore;
		}
		i2c_power_on(cskey_dev, false);
	}

	device_info_kobj = kobject_create_and_add("dev-info_key", NULL);
	if (device_info_kobj == NULL) {
		dev_err(&client->dev,
				"[Cypress cskey] kobject_create_and_add failed\n");
	} else {
		if (sysfs_create_group(device_info_kobj, &dev_info_attr_group)) {
			dev_err(&client->dev,
				"[Cypress cskey] sysfs_create_group failed\n");
			goto err_ignore;
		}
	}

	return 0;

err_free_reset_gpio:
	if (gpio_is_valid(pdata->reset_gpio))
		gpio_free(pdata->reset_gpio);
err_free_irq:
	free_irq(pdata->irq, cskey_dev);
err_free_irq_gpio:
	if (gpio_is_valid(pdata->irq_gpio))
		gpio_free(pdata->irq_gpio);
err_unregister_dev:
	input_unregister_device(input_dev);
	input_dev = NULL;
err_i2c_on:
	i2c_power_on(cskey_dev, false);
err_reglator_on:
	cskey_regulator_configure(cskey_dev, false);
err_free_input_dev:
	if(input_dev)
		input_free_device(input_dev);
	if(cskey_dev)
		kfree(cskey_dev);
err_free_mem:
	devm_kfree(&client->dev, (void *)pdata);
err_ignore:
	return err;
}

static int __devexit capsense_remove(struct i2c_client *client)
{
	struct cypress_cskey *cskey_dev = i2c_get_clientdata(client);

#if defined(CSKEY_RECOVERY_SUPPORT)
	cancel_delayed_work_sync(&cskey->recovery_work);
#endif
	led_classdev_unregister(&cskey_led);
	sysfs_remove_group(&client->dev.kobj, &cskey_attr_group);
	free_irq(cskey_dev->pdata->irq, cskey_dev);
	input_unregister_device(cskey_dev->input_dev);

#if defined(CONFIG_FB)
	if (fb_unregister_client(&cskey_dev->fb_notif))
		dev_err(&client->dev,
				"[Cypress cskey] Error occurred while unregistering fb_notifier.\n");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&cskey_dev->early_suspend);
#endif

	i2c_power_on(cskey_dev, false);
	cskey_regulator_configure(cskey_dev, false);

	if (gpio_is_valid(cskey_dev->pdata->reset_gpio))
		gpio_free(cskey_dev->pdata->reset_gpio);
	if (gpio_is_valid(cskey_dev->pdata->irq_gpio))
		gpio_free(cskey_dev->pdata->irq_gpio);

	input_free_device(cskey_dev->input_dev);
	devm_kfree(&client->dev, (void *)cskey_dev->pdata);
	if (cskey_dev->cskey_wq)
		destroy_workqueue(cskey_dev->cskey_wq);
	kfree(cskey_dev);

#if defined(CSKEY_WAKEUP_SUPPORT)
	device_init_wakeup(&client->dev, 0);
#endif

	return 0;
}

static const struct i2c_device_id capsense_id[] = {
	{ "cypress_capsense", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, capsense_id);

static struct of_device_id capsense_match_table[] = {
	{ .compatible = "cypress,capsense-key",},
	{ },
};

static struct i2c_driver capsense_driver = {
	.driver = {
		.name	= "cypress_capsense",
		.owner	= THIS_MODULE,
		.of_match_table = capsense_match_table,
#if defined(CONFIG_PM)
		.pm		= &cskey_pm_ops,
#endif
	},
	.probe		= capsense_probe,
	.remove		= __devexit_p(capsense_remove),
	.id_table	= capsense_id,
};

module_i2c_driver(capsense_driver);

MODULE_AUTHOR("Ben Hsu <ben.hsu@acer.com>");
MODULE_DESCRIPTION("Cypress CY8C20236A capsense key driver");
MODULE_LICENSE("GPL");
