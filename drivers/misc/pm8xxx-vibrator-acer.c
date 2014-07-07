/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/mfd/pm8xxx/vibrator.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/spmi.h>
#include <linux/gpio.h>


#include "../staging/android/timed_output.h"
#include "../../arch/arm/mach-msm/acer_hw_version.h"

#define VIB_VOLTAGE_CTL(base)	(base + 0x41)
#define VIB_EN_CTL(base)	(base + 0x46)
#define VIB_EN_CTL_VAL_ENABLE	0x80
#define VIB_EN_CTL_VAL_DISABLE	0x00
#define VIB_EN_GPIO 91

struct pm8xxx_vib {
	struct hrtimer vib_timer;
	struct timed_output_dev timed_dev;
	spinlock_t lock;
	struct work_struct work;
	struct device *dev;
	const struct pm8xxx_vibrator_platform_data *pdata;
	int state;
	u8 level;
	int max_timeout_ms;
	int min_vib_time_ms;
	u8  reg_vib_drv;
	struct spmi_device *spmi_dev;
	u16 base;
	u8 reg;
};

static struct pm8xxx_vib *vib_dev;

static int pm8xxx_vib_read_u8(struct pm8xxx_vib *vib,
				 u8 *data, u16 reg)
{
	int rc;

	rc = spmi_ext_register_readl(vib->spmi_dev->ctrl,
				     vib->spmi_dev->sid, reg, data, 1);
	if (rc < 0)
		dev_warn(vib->dev, "Error reading pm8xxx: %X - ret %X\n",
				reg, rc);

	return rc;
}

static int pm8xxx_vib_write_u8(struct pm8xxx_vib *vib,
				 u8 data, u16 reg)
{
	int rc;

	rc = spmi_ext_register_writel(vib->spmi_dev->ctrl,
				      vib->spmi_dev->sid, reg, &data, 1);
	if (rc < 0)
		dev_warn(vib->dev, "Error writing pm8xxx: %X - ret %X\n",
				reg, rc);
	return rc;
}

static int pm8xxx_vib_set(struct pm8xxx_vib *vib, int on)
{
	int rc;
	u8 val;

	if (on) {
		if(acer_board_id <= HW_ID_DVT3) {
			val = VIB_EN_CTL_VAL_ENABLE;
			rc = pm8xxx_vib_write_u8(vib, val, VIB_EN_CTL(vib->base));
			if (rc < 0)
				return rc;
			vib->reg_vib_drv = val;
		} else {
			rc = gpio_direction_output(VIB_EN_GPIO, 1);
			if (rc < 0)
				return rc;
		}
	} else {
		if(acer_board_id <= HW_ID_DVT3) {
			val = VIB_EN_CTL_VAL_DISABLE;
			rc = pm8xxx_vib_write_u8(vib, val, VIB_EN_CTL(vib->base));
			if (rc < 0)
				return rc;
			vib->reg_vib_drv = val;
		} else {
			rc= gpio_direction_output(VIB_EN_GPIO, 0);
			if (rc < 0)
				return rc;
		}
	}
	return rc;
}

static void pm8xxx_vib_enable(struct timed_output_dev *dev, int value)
{
	struct pm8xxx_vib *vib = container_of(dev, struct pm8xxx_vib,
					 timed_dev);
	unsigned long flags;

retry:
	spin_lock_irqsave(&vib->lock, flags);
	if (hrtimer_try_to_cancel(&vib->vib_timer) < 0) {
		spin_unlock_irqrestore(&vib->lock, flags);
		cpu_relax();
		goto retry;
	}

	if (value == 0)
		vib->state = 0;
	else {
		value = (value < vib->min_vib_time_ms ?
				 vib->min_vib_time_ms : value);
		value = (value > vib->max_timeout_ms ?
				 vib->max_timeout_ms : value);

		vib->state = 1;
		hrtimer_start(&vib->vib_timer,
			      ktime_set(value / 1000, (value % 1000) * 1000000),
			      HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&vib->lock, flags);
	schedule_work(&vib->work);
}

static void pm8xxx_vib_update(struct work_struct *work)
{
	struct pm8xxx_vib *vib = container_of(work, struct pm8xxx_vib,
					 work);

	pm8xxx_vib_set(vib, vib->state);
}

static int pm8xxx_vib_get_time(struct timed_output_dev *dev)
{
	struct pm8xxx_vib *vib = container_of(dev, struct pm8xxx_vib,
							 timed_dev);

	if (hrtimer_active(&vib->vib_timer)) {
		ktime_t r = hrtimer_get_remaining(&vib->vib_timer);
		return (int)ktime_to_us(r);
	} else
		return 0;
}

static enum hrtimer_restart pm8xxx_vib_timer_func(struct hrtimer *timer)
{
	struct pm8xxx_vib *vib = container_of(timer, struct pm8xxx_vib,
							 vib_timer);

	vib->state = 0;
	schedule_work(&vib->work);

	return HRTIMER_NORESTART;
}

#ifdef CONFIG_PM
static int pm8xxx_vib_suspend(struct device *dev)
{
	struct pm8xxx_vib *vib = dev_get_drvdata(dev);

	hrtimer_cancel(&vib->vib_timer);
	cancel_work_sync(&vib->work);
	/* turn-off vibrator */
	pm8xxx_vib_set(vib, 0);

	return 0;
}

static const struct dev_pm_ops pm8xxx_vib_pm_ops = {
	.suspend = pm8xxx_vib_suspend,
};
#endif

static int __devinit pm8xxx_vib_probe(struct spmi_device *spdev)

{
	struct pm8xxx_vib *vib;
	struct resource *vib_resource;
	struct device_node *node;
	u8 val;
	int temp;
	int rc;

	node = spdev->dev.of_node;
	if (node == NULL)
		return -ENODEV;

	vib = kzalloc(sizeof(*vib), GFP_KERNEL);
	if (!vib)
		return -ENOMEM;

	vib_resource	= spmi_get_resource(spdev, NULL, IORESOURCE_MEM, 0);
	if (! vib_resource) {
		dev_err(vib->dev,"Unable to get Vib base address\n");
		rc = -ENXIO;
		goto err_read_vib;
	}

	vib->base	= vib_resource->start;
	vib->pdata	= NULL;
	vib->dev	= &spdev->dev;
	vib->spmi_dev	= spdev;

	rc = of_property_read_u32(node, "qcom,max-timeout-ms", &temp);
	if (!rc) {
		vib->max_timeout_ms = temp;
	} else {
		dev_err(vib->dev, "Failed to read vibrator timeout ms\n");
		goto err_read_vib;
	}
	rc = of_property_read_u32(node, "qcom,min-vib-time-ms", &temp);
	if (!rc) {
		vib->min_vib_time_ms = temp;
	} else {
		dev_err(vib->dev, "Failed to read vibrator minimal time ms\n");
		goto err_read_vib;
	}
	rc = of_property_read_u32(node, "qcom,level-mV", &temp);
	if (!rc) {
		vib->level = (u8)temp;
	} else {
		dev_err(vib->dev, "Failed to read vibrator level mV\n");
		goto err_read_vib;
	}
	spin_lock_init(&vib->lock);
	INIT_WORK(&vib->work, pm8xxx_vib_update);

	hrtimer_init(&vib->vib_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vib->vib_timer.function = pm8xxx_vib_timer_func;

	vib->timed_dev.name = "vibrator";
	vib->timed_dev.get_time = pm8xxx_vib_get_time;
	vib->timed_dev.enable = pm8xxx_vib_enable;

	/*
	 * Configure the vibrator, it operates in manual mode
	 * for timed_output framework.
	 */
	if(acer_board_id <= HW_ID_DVT3) {
		rc = pm8xxx_vib_read_u8(vib, &val, VIB_EN_CTL(vib->base));
		if (rc < 0)
			goto err_read_vib;

		rc = pm8xxx_vib_read_u8(vib, &val, VIB_VOLTAGE_CTL(vib->base));
		if (rc < 0)
			goto err_read_vib;

		val = vib->level;
		rc = pm8xxx_vib_write_u8(vib, val, VIB_VOLTAGE_CTL(vib->base));
		if (rc < 0)
			goto err_read_vib;

		vib->reg_vib_drv = val;
	} else {
		rc = gpio_request(VIB_EN_GPIO, "vib-en-gpio");
		if (rc) {
			dev_err(vib->dev,
				"[Vibrator] failed to request reset gpio\n");
			if (gpio_is_valid(VIB_EN_GPIO))
				gpio_free(VIB_EN_GPIO);
			goto err_read_vib;
		}

		rc = gpio_direction_output(VIB_EN_GPIO, 0);
		if (rc) {
			dev_err(vib->dev,
			"[Vibrator] Unable to set direction for reset gpio \n");
			if (gpio_is_valid(VIB_EN_GPIO))
				gpio_free(VIB_EN_GPIO);
			goto err_read_vib;
		}
	}

	rc = timed_output_dev_register(&vib->timed_dev);
	if (rc < 0)
		goto err_read_vib;

	dev_set_drvdata(&spdev->dev, vib);
	vib_dev = vib;
	dev_info(vib->dev, "Probe success !\n");
	return 0;

err_read_vib:
	kfree(vib);
	return rc;
}

static int __devexit pm8xxx_vib_remove(struct spmi_device *spdev)
{
	struct pm8xxx_vib *vib = dev_get_drvdata(&spdev->dev);

	cancel_work_sync(&vib->work);
	hrtimer_cancel(&vib->vib_timer);
	timed_output_dev_unregister(&vib->timed_dev);
	dev_set_drvdata(&spdev->dev, NULL);
	device_unregister(&spdev->dev);
	kfree(vib);

	return 0;
}
static struct of_device_id spmi_match_table[] = {
	{	.compatible = "qcom,pm8xxx-vib",
	}
};

static struct spmi_driver pm8xxx_vib_driver = {
	.probe		= pm8xxx_vib_probe,
	.remove		= __devexit_p(pm8xxx_vib_remove),
	.driver		= {
		.name	= "qcom,pm8xxx-vib",
		.of_match_table = spmi_match_table,
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &pm8xxx_vib_pm_ops,
#endif
	},
};

static int __init pm8xxx_vib_init(void)
{
	return spmi_driver_register(&pm8xxx_vib_driver);
}
module_init(pm8xxx_vib_init);

static void __exit pm8xxx_vib_exit(void)
{
	spmi_driver_unregister(&pm8xxx_vib_driver);
}
module_exit(pm8xxx_vib_exit);

MODULE_ALIAS("qcom:pm8xxx-vib");
MODULE_DESCRIPTION("pm8xxx vibrator driver");
MODULE_LICENSE("GPL v2");
