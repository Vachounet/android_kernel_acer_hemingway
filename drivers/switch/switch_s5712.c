/*
 * Hall sensor driver
 *
 * Author: Ben Hsu <ben.hsu@acer.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

struct hall_sensor_data {
	struct device *dev;
	struct switch_dev sdev;
	struct input_dev *input_dev;
	struct delayed_work state_delay_work;
#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
	int fb_status;
#endif
	int irq_gpio;
	u32 irq_gpio_flags;
	unsigned int irq;
	bool device_sleep;
	bool wakeup_enable;
	spinlock_t lock;
	struct wake_lock irq_wake_lock;
};

#define DRIVER_NAME				"hall-sensor"
#define DEVICE_NAME				"S-5712"
#define SENSOR_NAME				"hall"

#define HALL_WAKEUP_ENABLE		1
#define HALL_WAKEUP_DISABLE		0

#define HALL_KEY_PRESS			1
#define HALL_KEY_RELEASE		0

#define HALL_DEBOUNCE_MS		200

struct hall_sensor_data *hsdev;
static int hall_sensor_suspend(struct device *dev);
static int hall_sensor_resume(struct device *dev);
#if defined(CONFIG_KEYBOARD_CYPRESS)
extern bool check_cskey_deep_sleep_mode(void);
extern void cskey_deep_sleep_mode(void);
#endif

static ssize_t hall_wakeup_enable_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	struct hall_sensor_data *hsdata = hsdev;
	int count = 0;

	count = sprintf(buf, "%s\n",
				(hsdata->wakeup_enable) ? "enable" : "disable");

	return count;
}

static ssize_t hall_wakeup_enable_store(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t size)
{
	struct hall_sensor_data *hsdata = hsdev;
	int err = 0;
	int enable = 0;

	err = sscanf(buf, "%d", &enable);
	if (err < 0) {
		printk("[Hall] Set wakeup enable failed: %d\n", err);
		goto exit;
	}

	enable = !!enable;

	if (!device_may_wakeup(hsdata->dev))
		goto exit;

	if ((enable) && (!hsdata->wakeup_enable)) {
		enable_irq(hsdata->irq);
		enable_irq_wake(hsdata->irq);
	} else if (!(enable) && (hsdata->wakeup_enable)) {
		disable_irq_wake(hsdata->irq);
		disable_irq(hsdata->irq);
	}

	hsdata->wakeup_enable = enable;

#if defined(DEBUG)
	printk("[Hall] Set wakeup status : %d\n", (int)hsdata->wakeup_enable);
#endif

exit:
	return size;
}

static DEVICE_ATTR(wakeup_enable, S_IRUGO | S_IWUSR, hall_wakeup_enable_show,
					hall_wakeup_enable_store);

static struct attribute *hall_sensor_attrs[] = {
	&dev_attr_wakeup_enable.attr,
	NULL,
};

static const struct attribute_group hall_sensor_attr_group = {
	.attrs = hall_sensor_attrs,
};

#if defined(CONFIG_FB)
static int hall_fb_notifier_callback(struct notifier_block *self,
						unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	int err = 0;
	struct hall_sensor_data *hsdata =
		container_of(self, struct hall_sensor_data, fb_notif);

	if (evdata && evdata->data && event == FB_EVENT_BLANK && hsdata) {
		blank = evdata->data;

		if (*blank == hsdata->fb_status)
			goto exit;

		if (*blank == FB_BLANK_UNBLANK)
			err = hall_sensor_resume(hsdata->dev);
		else if (*blank == FB_BLANK_POWERDOWN)
			err = hall_sensor_suspend(hsdata->dev);

		if (err == 0)
			hsdata->fb_status = *blank;
	}

exit:
	return err;
}
#endif

#if defined(CONFIG_PM)
static int hall_sensor_suspend(struct device *dev)
{
	struct hall_sensor_data *hsdata = dev_get_drvdata(dev);
	int err = 0;

	if (hsdata == NULL) {
		printk("[Hall] Invalid data\n");
		err = -EINVAL;
		goto exit;
	}

	spin_lock(&hsdata->lock);
	hsdata->device_sleep = true;
	spin_unlock(&hsdata->lock);

exit:
	return err;
}

static int hall_sensor_resume(struct device *dev)
{
	struct hall_sensor_data *hsdata = dev_get_drvdata(dev);
	int err = 0;

	if (hsdata == NULL) {
		printk("[Hall] Invalid data\n");
		err = -EINVAL;
		goto exit;
	}

	spin_lock(&hsdata->lock);
	hsdata->device_sleep = false;
	spin_unlock(&hsdata->lock);

exit:
	return err;
}

static const struct dev_pm_ops hall_sensor_pm_ops = {
#if !defined(CONFIG_FB)
	.suspend	= hall_sensor_suspend,
	.resume		= hall_sensor_resume,
#endif
};
#endif

void hall_sensor_send_key(struct hall_sensor_data *hsdata, int keycode,
			int pressed)
{
	input_report_key(hsdata->input_dev, keycode, pressed);
	input_sync(hsdata->input_dev);
}

int get_hall_sensor_state(void)
{
	int state = -1;

	if (hsdev != NULL)
		state = switch_get_state(&hsdev->sdev);

	return state;
}
EXPORT_SYMBOL_GPL(get_hall_sensor_state);

static ssize_t switch_print_state(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%s\n", (switch_get_state(sdev) ? "1" : "0"));
}

static ssize_t switch_print_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%s\n", DEVICE_NAME);
}

static void hall_sensor_work(struct work_struct *work)
{
	struct hall_sensor_data *hsdata = container_of(work,
										struct hall_sensor_data,
										state_delay_work.work);
	int gpio_status = 0;
	int dev_sleep = 0;

	if (hsdata == NULL)
		printk("[Hall] Invalid data\n");

	if (gpio_is_valid(hsdata->irq_gpio)) {
		gpio_status = gpio_get_value(hsdata->irq_gpio);
#ifdef DEBUG
		printk("[Hall] Get gpio status : %d\n", gpio_status);
#endif
	} else
		dev_err(hsdata->dev, "[Hall] Invalid gpio\n");

	/* hall sensor gpio state
	 * gpio value    switch state
	 *     0              1
	 *     1              0
     */
	switch_set_state(&hsdata->sdev, !gpio_status);

	spin_lock(&hsdata->lock);
	dev_sleep = hsdata->device_sleep;
	if (gpio_status == dev_sleep) {
		if (dev_sleep) {
			printk("[Hall] Hall sensor wakeup device\n");
			hsdata->device_sleep = false;
		} else {
			printk("[Hall] Hall sensor suspend device\n");
			hsdata->device_sleep = true;
		}

		/* Send power key to suspend/wakeup device */
		hall_sensor_send_key(hsdata, KEY_POWER, HALL_KEY_PRESS);
		hall_sensor_send_key(hsdata, KEY_POWER, HALL_KEY_RELEASE);
	}
#if defined(CONFIG_KEYBOARD_CYPRESS)
	else if (check_cskey_deep_sleep_mode()) {
		cskey_deep_sleep_mode();
		printk("[Hall] Change cskey sleep mode\n");
	}
#endif
	spin_unlock(&hsdata->lock);
	wake_unlock(&hsdata->irq_wake_lock);
}

static irqreturn_t hall_sensor_irq_handler(int irq, void *dev_id)
{
	struct hall_sensor_data *hsdata = (struct hall_sensor_data *)dev_id;

	wake_lock_timeout(&hsdata->irq_wake_lock, HALL_DEBOUNCE_MS * 2);
	schedule_delayed_work(&hsdata->state_delay_work,
				msecs_to_jiffies(HALL_DEBOUNCE_MS));

	return IRQ_HANDLED;
}

static int hall_sensor_parse_dt(struct device *dev,
						struct hall_sensor_data *hsdata)
{
	struct device_node *np = dev->of_node;
	u32 gpio_flag;

	hsdata->irq_gpio = of_get_named_gpio_flags(np, "Seiko,irq-gpio",
							0, &gpio_flag);
	if (gpio_is_valid(hsdata->irq_gpio))
		hsdata->irq_gpio_flags = gpio_flag;
	else
		return -EINVAL;

	hsdata->irq = gpio_to_irq(hsdata->irq_gpio);
	return 0;
}

static int __devinit hall_sensor_probe(struct platform_device *pdev)
{
	struct hall_sensor_data *hsdata = pdev->dev.platform_data;
	struct input_dev *input_dev;
	int err = 0;

	if (pdev->dev.of_node) {
		hsdata = devm_kzalloc(&pdev->dev, sizeof(struct hall_sensor_data),
					GFP_KERNEL);
		if (!hsdata) {
			dev_err(&pdev->dev, "[Hall] Failed to allocate memory");
			return -ENOMEM;
		}

		err = hall_sensor_parse_dt(&pdev->dev, hsdata);
		if (err)
			goto err_free_mem;
	}

	if (!hsdata)
		return -EINVAL;

	hsdata->dev = &pdev->dev;

	input_dev = input_allocate_device();
	if (input_dev == NULL) {
		dev_err(&pdev->dev, "[Hall] Failed to allocate input device\n");
		goto err_free_mem;
	}

	input_dev->name = DRIVER_NAME;
	input_dev->dev.parent = &pdev->dev;
	input_dev->id.bustype = BUS_HOST;

	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(KEY_POWER, input_dev->keybit);

	spin_lock_init(&hsdata->lock);
	wake_lock_init(&hsdata->irq_wake_lock, WAKE_LOCK_SUSPEND, DRIVER_NAME);

	hsdata->input_dev = input_dev;

	err = input_register_device(input_dev);
	if (err) {
		dev_err(&pdev->dev, "[Hall] Failed to register input device\n");
		goto err_free_input_dev;
	}

	/* GPIO irq */
	err = gpio_request(hsdata->irq_gpio, "hall-irq-gpio");
	if (err) {
		dev_err(&pdev->dev, "[Hall] Failed to request irq gpio\n");
		goto err_unregister_input;
	}

	err = gpio_direction_input(hsdata->irq_gpio);
	if (err) {
		dev_err(&pdev->dev, "[Hall] Unable to set direction for irq gpio %d\n",
				hsdata->irq_gpio);
		goto err_free_irq_gpio;
	}

	err = request_threaded_irq(hsdata->irq, NULL, hall_sensor_irq_handler,
								hsdata->irq_gpio_flags, DRIVER_NAME,
								hsdata);
	if (err) {
		dev_err(&pdev->dev, "[Hall] Failed to request irq %d\n", hsdata->irq);
		goto err_free_irq_gpio;
	}

	hsdata->sdev.print_state = switch_print_state;
	hsdata->sdev.name = SENSOR_NAME;
	hsdata->sdev.print_name = switch_print_name;
	err = switch_dev_register(&hsdata->sdev);
	if (err) {
		dev_err(&pdev->dev, "[Hall] Register switch device failed\n");
		goto err_free_irq;
	}

#if defined(CONFIG_FB)
	hsdata->fb_notif.notifier_call = hall_fb_notifier_callback;
	err = fb_register_client(&hsdata->fb_notif);
	if (err) {
		dev_err(&pdev->dev, "[Hall] Failed to register fb_notifier:%d\n", err);
		goto err_unregister_switch;
	}
#endif

	err = sysfs_create_group(&hsdata->sdev.dev->kobj, &hall_sensor_attr_group);
	if (err) {
		dev_err(&pdev->dev, "[Hall] Failed to create sysfs group:%d\n", err);
		goto err_unregister_fb;
	}

	dev_set_drvdata(&pdev->dev, hsdata);
	INIT_DELAYED_WORK(&hsdata->state_delay_work, hall_sensor_work);

	/* default enable wakeup feature */
	hsdata->wakeup_enable = true;
	device_init_wakeup(&pdev->dev, HALL_WAKEUP_ENABLE);
	enable_irq_wake(hsdata->irq);

	hsdev = hsdata;

	return 0;

err_unregister_fb:
	fb_unregister_client(&hsdata->fb_notif);
err_unregister_switch:
	switch_dev_unregister(&hsdata->sdev);
err_free_irq:
	free_irq(hsdata->irq, hsdata);
err_free_irq_gpio:
	if (gpio_is_valid(hsdata->irq_gpio))
		gpio_free(hsdata->irq_gpio);
err_unregister_input:
	input_unregister_device(hsdata->input_dev);
err_free_input_dev:
	if (input_dev)
		input_free_device(input_dev);
err_free_mem:
	devm_kfree(&pdev->dev, (void *)hsdata);
	return err;
}

static int __devexit hall_sensor_remove(struct platform_device *pdev)
{
	struct hall_sensor_data *hsdata = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&hsdata->state_delay_work);
	fb_unregister_client(&hsdata->fb_notif);
	switch_dev_unregister(&hsdata->sdev);
	free_irq(hsdata->irq, hsdata);
	if (gpio_is_valid(hsdata->irq_gpio))
		gpio_free(hsdata->irq_gpio);
	input_unregister_device(hsdata->input_dev);
	input_free_device(hsdata->input_dev);
	disable_irq_wake(hsdata->irq);
	device_init_wakeup(&pdev->dev, HALL_WAKEUP_DISABLE);
	devm_kfree(&pdev->dev, (void *)hsdata);

	return 0;
}

static struct of_device_id hall_sensor_of_match[] = {
	{ .compatible = "Seiko,hall-sensor",},
	{ },
};
MODULE_DEVICE_TABLE(of, hall_sensor_of_match);


static struct platform_driver hall_sensor_device_driver = {
	.probe		= hall_sensor_probe,
	.remove		= __devexit_p(hall_sensor_remove),
	.driver		= {
		.name	= "hall-sensor",
		.owner	= THIS_MODULE,
		.of_match_table = hall_sensor_of_match,
#if defined(CONFIG_PM)
		.pm		= &hall_sensor_pm_ops,
#endif
	}
};

static int __init hall_senor_init(void)
{
	return platform_driver_register(&hall_sensor_device_driver);
}

static void __exit hall_sensor_exit(void)
{
	platform_driver_unregister(&hall_sensor_device_driver);
}

late_initcall(hall_senor_init);
module_exit(hall_sensor_exit);

MODULE_AUTHOR("Ben Hsu <ben.hsu@acer.com>");
MODULE_DESCRIPTION("Seiko S-5712ACDL1-I4T1U hall sensor driver");
MODULE_LICENSE("GPL");
