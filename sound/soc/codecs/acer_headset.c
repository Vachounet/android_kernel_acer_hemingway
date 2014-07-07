/*
 * Acer Headset detection driver.
 *
 * Copyright (C) 2012 Acer Corporation.
 *
 * Authors:
 *    Eric Cheng <Eric.Cheng@acer.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <mach/pmic.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/mfd/pm8xxx/misc.h>
#include <linux/of_gpio.h>
#include <linux/printk.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include "wcd9320.h"
#include "acer_headset.h"
#ifdef CONFIG_ACER_HEADSET_BUTT
#include "acer_headset_butt.h"
#endif

struct hs_res {
	struct switch_dev sdev;
	unsigned int det;
	unsigned int irq;
	unsigned int mic_bias_en;
	unsigned int hph_amp_en;
	bool headsetOn;
	struct hrtimer timer;
	ktime_t debounce_time;
};

enum {
	NO_DEVICE			= 0,
	ACER_HEADSET		= 1,
	ACER_HEADSET_NO_MIC	= 2,
};

static struct hs_res *hr;
static struct work_struct short_wq;
static int hs_bias_gpio;
static int hs_depop_gpio;
static int hs_uart_gpio;
struct snd_soc_jack hs_jack;
struct snd_soc_jack button_jack;

int acer_hs_detect(void);
static void acer_headset_delay_set_work(struct work_struct *work);
static void acer_headset_delay_butt_work(struct work_struct *work);
static DECLARE_DELAYED_WORK(set_hs_state_wq, acer_headset_delay_set_work);
static DECLARE_DELAYED_WORK(set_hs_butt_wq, acer_headset_delay_butt_work);

extern struct platform_device *acer_taiko_pdev;
extern void acer_headset_jack_report(int headset_type);

static ssize_t acer_hs_print_name(struct switch_dev *sdev, char *buf)
{
	switch (switch_get_state(&hr->sdev)) {
		case NO_DEVICE:
			return sprintf(buf, "No Device\n");
		case ACER_HEADSET:
			return sprintf(buf, "Headset\n");
		case ACER_HEADSET_NO_MIC:
			return sprintf(buf, "Headset no mic\n");
	}
	return -EINVAL;
}

static ssize_t acer_hs_print_state(struct switch_dev *sdev, char *buf)
{
	switch (switch_get_state(&hr->sdev)) {
		case NO_DEVICE:
			return sprintf(buf, "%s\n", "0");
		case ACER_HEADSET:
			return sprintf(buf, "%s\n", "1");
		case ACER_HEADSET_NO_MIC:
			return sprintf(buf, "%s\n", "2");
	}
	return -EINVAL;
}

void start_button_irq(void)
{
	schedule_delayed_work(&set_hs_butt_wq, 200);
}

void stop_button_irq(void)
{
	set_hs_state(false);
	cancel_delayed_work_sync(&set_hs_butt_wq);
}

static void remove_headset(void)
{
	pr_debug("Remove Headset\n");
	switch_set_state(&hr->sdev, NO_DEVICE);
}

static void acer_headset_delay_butt_work(struct work_struct *work)
{
	printk("Enable headset button !!!\n");
#ifdef CONFIG_ACER_HEADSET_BUTT
	set_hs_state(true);
#endif
}

static void acer_update_state_work(struct work_struct *work)
{
#ifdef CONFIG_ACER_HEADSET_BUTT
	set_hs_state(false);
#endif
}

static void acer_headset_delay_set_work(struct work_struct *work)
{
	bool hs_disable;
	int32_t adc_reply;
	int hs_flag, count = 0;

	hs_disable = gpio_get_value(hr->det);
	printk("acer hs detect state = %d...\n", hs_disable);

	if (!hs_disable) {
		// HEADSET BIAS enable
		gpio_direction_output(hs_bias_gpio, 1);

		// Delay time for waiting Headset MIC BIAS working voltage.
		mdelay(HEADSET_MIC_BIAS_WORK_DELAY_TIME);

		if (0 == gpio_get_value(hs_uart_gpio)) {
			// HEADSET DE-POP enable, 0: Enable; 1: Disable
			gpio_direction_output(hs_depop_gpio, 0);
		}

		// Check Headset type
		get_mpp_hs_mic_adc(&adc_reply);
		do {
			if (adc_reply > 0x6500) {
				hs_flag = ACER_HEADSET;
				break;
			} else {
				hs_flag = ACER_HEADSET_NO_MIC;
				mdelay(HEADSET_MIC_BIAS_WORK_DELAY_TIME);
				get_mpp_hs_mic_adc(&adc_reply);
				count++;
			}
		} while (count < 10);
		acer_headset_jack_report(hs_flag);

#ifdef CONFIG_ACER_HEADSET_BUTT
		if (hs_flag == ACER_HEADSET)
			start_button_irq();
#endif
	} else {
#ifdef CONFIG_ACER_HEADSET_BUTT
		set_hs_state(false);
#endif
		// HEADSET DE-POP disable, 0: Enable; 1: Disable
		if (0 == gpio_get_value(hs_uart_gpio)) {
			gpio_direction_output(hs_depop_gpio, 1);
		}

		// HEADSET BIAS disable
		gpio_direction_output(hs_bias_gpio, 0);

#ifdef CONFIG_ACER_HEADSET_BUTT
		stop_button_irq();
#endif
		acer_headset_jack_report(NO_DEVICE);
		printk("Remove HEADSET\n");
	}
}

void acer_hs_detect_work(void)
{
	// Check headset status when device boot.
	schedule_delayed_work(&set_hs_state_wq, 50);
}

static enum hrtimer_restart detect_event_timer_func(struct hrtimer *data)
{
	schedule_work(&short_wq);
	schedule_delayed_work(&set_hs_state_wq, 20);

	return HRTIMER_NORESTART;
}

static irqreturn_t hs_det_irq(int irq, void *dev_id)
{
	hrtimer_start(&hr->timer, hr->debounce_time, HRTIMER_MODE_REL);

	return IRQ_HANDLED;
}

int acer_hs_init(struct platform_device *pdev)
{
	int ret;

	pr_debug("acer_hs_probe start...\n");

	hr = kzalloc(sizeof(struct hs_res), GFP_KERNEL);
	if (!hr)
		return -ENOMEM;

	hr->debounce_time = ktime_set(0, 500000000);  /* 500 ms */

	INIT_WORK(&short_wq, acer_update_state_work);
	hr->sdev.name = "h2w";
	hr->sdev.print_name = acer_hs_print_name;
	hr->sdev.print_state = acer_hs_print_state;
	hr->headsetOn = false;
	hr->det = of_get_named_gpio(pdev->dev.of_node, "qcom,hs-dett-irq-gpio", 0);
	hr->irq = gpio_to_irq(hr->det);

	ret = switch_dev_register(&hr->sdev);
	if (ret < 0) {
		pr_err("switch_dev fail!\n");
		goto err_switch_dev_register;
	}

	if (gpio_is_valid(hr->det)) {
		/* configure touchscreen reset out gpio */
		printk("Eric Test : gpio %d\n", hr->det);
		ret = gpio_request(hr->det, "hs_detect");
		if (ret) {
			pr_err("%s: unable to request reset gpio %d\n", __func__, hr->det);
			goto err_request_detect_irq;
		}

		ret = gpio_direction_input(hr->det);
		if (ret) {
			pr_err("%s: unable to set direction for gpio %d\n", __func__, hr->det);
			goto err_request_detect_gpio;
		}
	}

	hrtimer_init(&hr->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr->timer.function = detect_event_timer_func;

	ret = request_irq(hr->irq, hs_det_irq, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "hs_detect", NULL);
	if (ret < 0) {
		pr_err("request detect irq fail!\n");
		goto err_request_detect_irq;
	}

	hs_bias_gpio = of_get_named_gpio(pdev->dev.of_node, "qcom,hs-bias-gpio", 0);
	if (hs_bias_gpio >= 0) {
		printk("Eric Test : hs_bias_gpio %d\n", hs_bias_gpio);
		ret = gpio_request(hs_bias_gpio, "hs_bias_gpio");
		if (ret) {
			pr_err("%s: gpio_request failed for hs_bias_gpio.\n", __func__);
			goto err_request_detect_irq;
		}
		gpio_direction_output(hs_bias_gpio, 0);
	}

	hs_uart_gpio = of_get_named_gpio(pdev->dev.of_node, "qcom,hs-uart-gpio", 0);
	if (hs_uart_gpio >= 0) {
		printk("Eric Test : hs_uart_gpio %d\n", hs_uart_gpio);
		ret = gpio_request(hs_uart_gpio, "hs_uart_gpio");
		if (ret) {
			pr_err("%s: gpio_request failed for hs_uart_gpio.\n", __func__);
			goto err_request_detect_irq;
		}
	}

	hs_depop_gpio = of_get_named_gpio(pdev->dev.of_node, "qcom,hs-depop-gpio", 0);
	if (hs_depop_gpio >= 0) {
		printk("Eric Test : hs_depop_gpio %d\n", hs_depop_gpio);
		ret = gpio_request(hs_depop_gpio, "hs_depop_gpio");
		if (ret) {
			pr_err("%s: gpio_request failed for hs_depop_gpio.\n", __func__);
			goto err_request_detect_irq;
		}
		if (0 == gpio_get_value(hs_uart_gpio)) {
			gpio_direction_output(hs_depop_gpio, 1);
		} else {
			gpio_direction_output(hs_depop_gpio, 0);
		}
	}

	pr_debug("acer_hs_probe done.\n");

	return 0;

err_request_detect_irq:
	free_irq(hr->irq, 0);
err_request_detect_gpio:
	gpio_free(hr->det);
err_switch_dev_register:
	pr_err("ACER-HS: Failed to register driver\n");

	return ret;
}

void acer_headset_jack_init(struct snd_soc_codec *codec)
{
	int err;

	err = snd_soc_jack_new(codec, "Headset Jack",
					(SND_JACK_HEADSET | SND_JACK_OC_HPHL |
					SND_JACK_OC_HPHR | SND_JACK_UNSUPPORTED),
					&hs_jack);
	if (err) {
		pr_err("failed to create new jack\n");
		return;
	}

	err = snd_soc_jack_new(codec, "Button Jack",
					(SND_JACK_BTN_0 | SND_JACK_BTN_1 |
					SND_JACK_BTN_2 | SND_JACK_BTN_3 |
					SND_JACK_BTN_4 | SND_JACK_BTN_5 |
					SND_JACK_BTN_6 | SND_JACK_BTN_7),
					&button_jack);
	if (err) {
		pr_err("failed to create new jack\n");
		return;
	}
}

int acer_hs_remove(void)
{
	if (switch_get_state(&hr->sdev))
		remove_headset();

	gpio_free(hr->det);
	free_irq(hr->irq, 0);
	switch_dev_unregister(&hr->sdev);

	return 0;
}

int acer_hs_detect(void)
{
	int curstate;
	bool hs_disable;

	hs_disable = gpio_get_value(hr->det);

	if (!hs_disable) {
		pr_debug("ACER_HEADSET!!\n");
		curstate = ACER_HEADSET;
	} else {
		pr_debug("NO_DEVICE!!\n");
		curstate = NO_DEVICE;
	}

	return curstate;
}
