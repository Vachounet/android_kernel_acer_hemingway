/*
 *  Acer Simulator Input Driver
 *
 *  Copyright (c) 2012 Acer
 *  Copyright (c) 2012 Erix Cheng, Peng Chang, Travis Lin

 *
 *  Based on Acer Input Driver code from Acer Corporation.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/input.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/mach-types.h>
#include <linux/regulator/consumer.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/time.h>
#include <linux/android_alarm.h>
#include <linux/input/mt.h>

struct input_dev *input_dev;
struct work_struct  work;
struct work_struct  work2;
static struct workqueue_struct *acer_input_wq;
bool enable_play = false;
bool apmt_state = false;

#define INPUT_NEED_DEBUG 0
#define PATH_MAX_LEN 100
#define IGNORE_DELAY_USEC 10

#if defined(CONFIG_MACH_ACER_A12)
#define ABS_X_MAX 7200
#define ABS_Y_MAX 9600
#define ABS_MAX_TRACKING_ID 65535
#else
#define ABS_X_MAX 1279
#define ABS_Y_MAX 799
#define ABS_MAX_TRACKING_ID 10
#endif
#define ABS_MAX_SLOTS 22
#define ABS_PRESSURE_MAX 255
#define KEY_PLAY_FINISH 255

#define COMMAND_PATH "/sdcard/doutool/command/current_cmd"
#define SCRIPT_PATH "/sdcard/doutool/script/"
#define APMT_PATH "/sdcard/doutool/apmt_is_running"

struct completion delay_completion;
struct hrtimer delay_timer;
static int pre_key_is_power_key, power_key_press = 0;
ktime_t resume_time;
void acer_set_alarm(ktime_t next_time);
ktime_t acer_get_elapsed_realtime(void);
#if defined(CONFIG_MACH_ACER_A12)
void qpnp_batt_chg_enable(int en);
#endif

static int str_to_int(const char *name)
{
	int val = 0;

	for (;; name++) {
		switch (*name) {
		case '0' ... '9':
			val = 16*val+(*name-'0');
			break;
		case 'a' ... 'f':
			val = 16*val+(*name-'a')+10;
			break;
		default:
			return val;
		}
	}
}

static ktime_t str_to_ktime(const char *name)
{
	long val1 = 0, val2 = 0;
	int has_point = 0;

	for (;; name++) {
		switch (*name) {
		case ' ':
			break;
		case '0' ... '9':
			if (has_point == 1)
				val2 = 10*val2+(*name-'0');
			else
				val1 = 10*val1+(*name-'0');
			break;
		case '.':
			has_point = 1;
			break;
		default:
			return ktime_set(val1, val2 * 1000);
		}
	}
}

static void acer_input_notify_usb_plugin(int en)
{
#if defined(CONFIG_MACH_ACER_A12)
	qpnp_batt_chg_enable(en);
#endif
}

static void acer_input_enable_usb(struct work_struct *work)
{
	struct file *filp = NULL;
	char *script_path = NULL;
	char buffer[1];

	script_path = kmalloc(PATH_MAX_LEN, GFP_KERNEL);
	if (script_path == NULL) {
		printk("Fail to allocate mem for file\n");
		return;
	}
	strcpy(script_path, APMT_PATH);

	filp = filp_open(script_path, O_RDWR, 0);
	if (IS_ERR(filp)) {
		printk("[Acer_input] : %s file not exist\n", APMT_PATH);
		goto open_err;
	}
	if (!(filp->f_op) || !(filp->f_op->read) || !(filp->f_op->write)) {
		printk("Can't read script file\n");
		goto error;
	}

	if (enable_play) {
		if (filp->f_op->read(filp, buffer, 1, &filp->f_pos) > 0) {
			if (buffer[0] == '1') {
				acer_input_notify_usb_plugin(0);
				apmt_state = true;
			}
		}
	} else {
		buffer[0] = '0';
		if (filp->f_op->write(filp, buffer, 1, &filp->f_pos) > 0) {
			acer_input_notify_usb_plugin(1);
			apmt_state = false;
		}
	}
error:
	if (filp_close(filp, NULL))
		printk("%s: Fail to close script file\n", __func__);

open_err:
	if (script_path)
		kfree(script_path);
}

static enum hrtimer_restart delay_timer_handler(struct hrtimer *hrtimer)
{
	complete(&delay_completion);
	return HRTIMER_NORESTART;
}

static ssize_t acer_input_enable_store(struct device *device,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char event_string[64];
	char *envp[] = { event_string, NULL };
	struct device *dev = &input_dev->dev;

	if (!strncmp(buf, "1", 1) && !enable_play) {
		printk("[Acer_input] : Run acer input work queue.\n");
		enable_play = true;
		if (!apmt_state)
			queue_work(acer_input_wq, &work2);

		queue_work(acer_input_wq, &work);
	}
	else if (!strncmp(buf, "2", 1)) {
		printk("[Acer_input] : All PlayList Finish\n");
		enable_play = false;
		if (apmt_state)
			queue_work(acer_input_wq, &work2);

		input_report_key(input_dev, KEY_PLAY_FINISH, 1);// Fake event for Playing finish
		input_sync(input_dev);

		input_report_key(input_dev, KEY_PLAY_FINISH, 0);
		input_sync(input_dev);
	}
	else if (!strncmp(buf, "0", 1)) {
		printk("[Acer_input] : cancel acer input  work queue\n");
		enable_play = false;
		if (apmt_state)
			queue_work(acer_input_wq, &work2);

		complete(&delay_completion);

		sprintf(event_string, "SUBSYSTEM=stop_dou_play");
		kobject_uevent_env(&dev->kobj , KOBJ_CHANGE, envp);
	}
	return 1;
}

static ssize_t acer_input_time_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%17lld", acer_get_elapsed_realtime().tv64);
}

static DEVICE_ATTR(enable, 0220, NULL, acer_input_enable_store);
static DEVICE_ATTR(uptime, 0444, acer_input_time_show, NULL);

static struct attribute *acer_input_sysfs_entries[] = {
	&dev_attr_enable.attr,
	&dev_attr_uptime.attr,
	NULL,
};

static struct attribute_group acer_input_attr_group = {
	.attrs	= acer_input_sysfs_entries,
};

void acer_input_process_script(char *script_file)
{
	int x = 0, y = 0, touch_major = 0, touch_minor = 0, width_major = 0, width_minor = 0;
	int orientation = 0, track_id = 0, press = 0, tool_type = 0, misc = 0, slot = 0;
	int type = 0, code = 0, value = 0, length_read, line_count = 0;
	char buffer[1], line_buf[100];
	ktime_t time, pre_time, diff_time;
	char *script_path = NULL, *start_p = NULL;
	struct file *filp = NULL;
	struct timespec now;

	time = ktime_set(0, 0);
	pre_time = ktime_set(0, 0);

	script_path = kmalloc(PATH_MAX_LEN, GFP_KERNEL);
	if (script_path == NULL) {
		printk("Fail to allocate mem for file\n");
		goto mem_err;
	}
	strcpy(script_path, SCRIPT_PATH);
	strcat(script_path, script_file);
	printk("[Acer_input] : Run Script file : %s\n", script_path);

	filp = filp_open(script_path, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk("Script file Not exist\n");
		goto open_err;
	}

	if (!(filp->f_op) || !(filp->f_op->read)) {
		printk("Can't read script file\n");
		goto error;
	}

	while (1) {
		length_read = 0;
		memset(line_buf, 0, sizeof(line_buf));
		while (filp->f_op->read(filp, buffer, 1, &filp->f_pos) > 0) {
			line_buf[length_read++] = buffer[0];
			if (buffer[0] == '\r' || buffer[0] == '\n')
				break;
		}

		if (length_read > 0) {
			if (line_buf[length_read-1] == '\n' || line_buf[length_read-1] == '\r') {
				line_count++;
				line_buf[length_read] = 0;
			} else {
				goto error;
			}
		} else {
			goto error;
		}

		if (line_count == 1) {
			printk("[Acer_input] : APP Package name : %s", line_buf);
			input_report_abs(input_dev, ABS_MT_SLOT, 1);
			continue;
		}

#if INPUT_NEED_DEBUG
		printk("======================\n");
		printk("Read one event : count %d , %s", length_read, line_buf);
		printk("======================\n");
#endif

		start_p = line_buf;

		//Parsing one event
		strsep(&start_p, "[");
		pre_time = time;//Recored time stamp of previous event
		time = str_to_ktime(strsep(&start_p, "]"));
		strsep(&start_p, " ");
		type = str_to_int(strsep(&start_p, " "));
		code = str_to_int(strsep(&start_p, " "));
		value = str_to_int(strsep(&start_p, "\n"));

#if INPUT_NEED_DEBUG
		printk("time=%lld\n", ktime_to_us(time);
		printk("type=%d\n", type);
		printk("code=%d\n", code);
		printk("value=%d\n", value);
#endif

		if ((u64)pre_time.tv64 > 0) {
			diff_time = ktime_sub(time, pre_time);
			if (pre_key_is_power_key == 1) {
				getnstimeofday(&now);
				resume_time = ktime_add(timespec_to_ktime(now), diff_time);
				acer_set_alarm(resume_time);
				hrtimer_start(&delay_timer, diff_time, HRTIMER_MODE_REL);
				wait_for_completion(&delay_completion);
				pre_key_is_power_key = 0;
			} else {
				hrtimer_start(&delay_timer, diff_time, HRTIMER_MODE_REL);
				wait_for_completion(&delay_completion);
			}
			if (enable_play == false)
				goto error;
		}

		if (type == EV_KEY) {
			if (code == KEY_VOLUMEUP) {
				press = value;
				input_report_key(input_dev, KEY_VOLUMEUP, press);
			} else if (code == KEY_VOLUMEDOWN) {
				press = value;
				input_report_key(input_dev, KEY_VOLUMEDOWN, press);
			} else if (code == KEY_POWER) {
				if (value == 0)
					power_key_press = 1;
				else
					power_key_press = 0;
				press = value;
				input_report_key(input_dev, KEY_POWER, press);
			} else if (code == BTN_STYLUS) {
				press = value;
				input_report_key(input_dev, BTN_STYLUS, press);
			} else if (code == BTN_STYLUS2) {
				press = value;
				input_report_key(input_dev, BTN_STYLUS2, press);
			} else if (code == KEY_MENU) {
				press = value;
				input_report_key(input_dev, KEY_MENU, press);
			} else if (code == KEY_HOME) {
				press = value;
				input_report_key(input_dev, KEY_HOME, press);
			} else if (code == KEY_BACK) {
				press = value;
				input_report_key(input_dev, KEY_BACK, press);
			} else {
				printk("EV_KEY Unknown code =%d\n", code);
			}
			continue;
		} else if (type == EV_ABS) {
			if (code == ABS_MT_POSITION_X ) {
				x = value;
				input_report_abs(input_dev, ABS_MT_POSITION_X, x);
			} else if (code == ABS_MT_POSITION_Y) {
				y = value;
				input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
			} else if (code == ABS_MT_TOUCH_MAJOR) {
				touch_major = value;
				input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, touch_major);
			} else if (code == ABS_MT_TOUCH_MINOR) {
				touch_minor = value;
				input_report_abs(input_dev, ABS_MT_TOUCH_MINOR, touch_minor);
			} else if (code == ABS_MT_WIDTH_MAJOR) {
				width_major = value;
				input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, width_major);
			} else if (code == ABS_MT_WIDTH_MINOR) {
				width_minor = value;
				input_report_abs(input_dev, ABS_MT_WIDTH_MINOR, width_minor);
			} else if (code == ABS_MT_ORIENTATION) {
				orientation = value;
				input_report_abs(input_dev, ABS_MT_ORIENTATION, orientation);
			} else if (code == ABS_MT_SLOT) {
				slot = value;
				input_report_abs(input_dev, ABS_MT_SLOT, slot);
			} else if (code == ABS_MT_TRACKING_ID) {
				track_id = value;
				input_report_abs(input_dev, ABS_MT_TRACKING_ID, track_id);
			} else if (code == ABS_MT_PRESSURE) {
				press = value;
				input_report_abs(input_dev, ABS_MT_PRESSURE, press);
			} else if (code == ABS_MT_TOOL_TYPE) {
				tool_type = value;
				input_report_abs(input_dev, ABS_MT_TOOL_TYPE, tool_type);
			} else if (code == ABS_MISC) {
				misc = value;
				input_report_abs(input_dev, ABS_MISC, misc);
			} else {
				printk("EV_ABS Unknown code =%d\n", code);
			}
			continue;
		} else if (type == EV_SYN) {
			if (code == 2) {
#if INPUT_NEED_DEBUG
				printk("x=%d, y=%d, touch_major=%d, touch_minor=%d, width_major=%d, width_minor=%d\n",
					x, y, touch_major, touch_minor, width_major, width_minor);
				printk("orientation=%d, track_id=%d, press=%d, tool_type=%d\n",
					orientation, track_id, press, tool_type);
				printk("Send mt_sync event\n");
#endif
				input_mt_sync(input_dev);
			} else if (code == 0) {
#if INPUT_NEED_DEBUG
				printk("Send TS or Key event\n");
#endif
				if(power_key_press == 1)
					pre_key_is_power_key = 1;

				input_sync(input_dev);
				if (enable_play == false)
					goto error;
			}
		}
	}

error:
	if (filp_close(filp, NULL))
		printk("Fail to close script file\n");

open_err:
mem_err:
	printk("[Acer_input] : Finish script file : %s\n", script_path);
	if (script_path)
		kfree(script_path);

	return;
}

static void acer_input_work_func(struct work_struct *work)
{
	int length_read = 0;
	char buffer[1], line_buf[100], *start_p = NULL;
	struct file *cmd_filp = NULL;
	unsigned char *script_file = NULL;
	unsigned char *cmd_path = COMMAND_PATH;
	char event_string[64];
	struct device *dev = &input_dev->dev;
	char *envp[] = { event_string, NULL };

	printk("[Acer_input] : Load command file : %s\n", cmd_path);

	cmd_filp = filp_open(cmd_path, O_RDONLY, 0);
	if (IS_ERR(cmd_filp)) {
		printk("Can't open command file\n");
		return;
	}

	if (!(cmd_filp->f_op) || !(cmd_filp->f_op->read)) {
		printk("Can't read command file\n");
		goto error;
	}

	while (1) {
		if (enable_play == false)
			goto error;

		length_read = 0;
		memset(line_buf, 0, sizeof(line_buf));
		while (cmd_filp->f_op->read(cmd_filp, buffer, 1, &cmd_filp->f_pos) > 0) {
			line_buf[length_read++] = buffer[0];
			if (buffer[0] == '\r' || buffer[0] == '\n')
				break;
		}

		if (length_read > 0) {
			if (line_buf[length_read-1] == '\n' || line_buf[length_read-1] == '\r')
				line_buf[length_read] = 0;
			else
				goto error;
		} else {
			goto error;
		}

#if INPUT_NEED_DEBUG
		printk("======================\n");
		printk("Read one command : count %d , %s", length_read, line_buf);
		printk("======================\n");
#endif

		start_p = line_buf;

		//Parsing one line
		script_file = strsep(&start_p, "\n");

#if INPUT_NEED_DEBUG
		printk("script_file=%s\n", script_file);
#endif
		acer_input_process_script(script_file);

	}
error:
	printk("[Acer_input] : Finish command file : %s\n", cmd_path);

	sprintf(event_string, "SUBSYSTEM=acer_input");
	kobject_uevent_env(&dev->kobj , KOBJ_CHANGE, envp);

	if (filp_close(cmd_filp, NULL))
		printk("Fail to close command file\n");

	enable_play = false;
	return;
}

static int __devinit acer_input_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct kobject *acer_input_kobj;

	pr_info("%s: enter\n", __func__);

	input_dev = input_allocate_device();
	if (input_dev == NULL) {
		ret = -ENOMEM;
		pr_err("%s: Failed to allocate input device\n", __func__);
	}

	input_dev->name = "acer-input";
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(ABS_MT_SLOT, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MINOR, input_dev->absbit);
	set_bit(ABS_MT_ORIENTATION, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_TOOL_TYPE, input_dev->absbit);
	set_bit(ABS_MT_TRACKING_ID, input_dev->absbit);
	set_bit(ABS_MT_PRESSURE, input_dev->absbit);
	set_bit(KEY_PLAY_FINISH, input_dev->keybit);

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(KEY_VOLUMEUP, input_dev->keybit);
	set_bit(KEY_VOLUMEDOWN, input_dev->keybit);
	set_bit(KEY_POWER, input_dev->keybit);
	set_bit(KEY_MENU, input_dev->keybit);
	set_bit(KEY_HOME, input_dev->keybit);
	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(BTN_STYLUS, input_dev->keybit);
	set_bit(BTN_STYLUS2, input_dev->keybit);

	pr_info("%s: Acer input max X = %d , max Y = %d\n", __func__, ABS_X_MAX, ABS_Y_MAX);

	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 7200, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MINOR, 0, 7200, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_ORIENTATION, 0, 1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ABS_X_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ABS_Y_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOOL_TYPE, 0, 1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, ABS_MAX_TRACKING_ID, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 1, ABS_PRESSURE_MAX, 0, 0);

	if (input_mt_init_slots(input_dev, ABS_MAX_SLOTS) < 0) {
		printk(KERN_INFO "failed allocate slots\n");
		input_free_device(input_dev);
		return -ENOMEM;
	}

	ret = input_register_device(input_dev);
	if (ret)
		pr_err("%s: Unable to register %s input device\n",
			__func__, input_dev->name);

	INIT_WORK(&work, acer_input_work_func);
	INIT_WORK(&work2, acer_input_enable_usb);

	acer_input_kobj = kobject_create_and_add("acer_input", NULL);
	if (acer_input_kobj == NULL)
		pr_err("Failed to create acer_input kobject\n");

	ret = sysfs_create_group(acer_input_kobj, &acer_input_attr_group);
	if (ret)
		pr_err("Failed to create acer_input sysfs group\n");

	hrtimer_init(&delay_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	delay_timer.function = delay_timer_handler;
	init_completion(&delay_completion);

	return 0;
}

static int acer_input_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct timespec now;

	if (enable_play) {
		getnstimeofday(&now);
		if (pre_key_is_power_key == 1 && (resume_time.tv64 < timespec_to_ktime(now).tv64))
			complete(&delay_completion);
	}

	return 0;
}

static int acer_input_resume(struct platform_device *pdev)
{
	return 0;
}


static int acer_input_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id acer_input_table[] = {
	       {.compatible = "virtual-device,acer-input"},
	       {},
};

static struct platform_driver acer_input_device_driver = {
	.probe          = acer_input_probe,
	.remove         = __devexit_p(acer_input_remove),
	.driver         = {
		.name   = "acer-input",
		.owner  = THIS_MODULE,
		.of_match_table = acer_input_table,
	},
	.suspend = acer_input_suspend,
	.resume = acer_input_resume,
};

static int __init acer_input_init(void)
{
	pr_info("%s: enter\n", __func__);

	acer_input_wq = create_singlethread_workqueue("acer_input_wq");
	if (!acer_input_wq)
		return -ENOMEM;

	return platform_driver_register(&acer_input_device_driver);
}

static void __exit acer_input_exit(void)
{
	if (acer_input_wq)
		destroy_workqueue(acer_input_wq);
	platform_driver_unregister(&acer_input_device_driver);
}

module_init(acer_input_init);
module_exit(acer_input_exit);

MODULE_DESCRIPTION("Acer input driver");
MODULE_LICENSE("GPL v2");
