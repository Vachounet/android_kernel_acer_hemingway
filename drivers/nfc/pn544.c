/*
 * Driver for the PN544 NFC chip.
 *
 * Copyright (C) Nokia Corporation
 *
 * Author: Jari Vanhala <ext-jari.vanhala@nokia.com>
 * Contact: Matti Aaltonen <matti.j.aaltonen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#include <linux/completion.h>
#include <linux/crc-ccitt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/nfc/pn544.h>
#include <linux/poll.h>
#include <linux/regulator/consumer.h>
#include <linux/serial_core.h> /* for TCGETS */
#include <linux/slab.h>
#include <linux/gpio.h>
#include "nfc_codef.h"
#include "nfc_prj.h"
#include "pn544_test.h"
#include <linux/of_gpio.h>

extern int acer_boot_mode;
extern struct chip_data_t * S_H_version;

static ssize_t NfcHW_ID_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf);
static ssize_t NfcSW_ID_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf);

int pn544_enable(struct pn544_info *info, int mode);
void pn544_disable(struct pn544_info *info);

#define NFC_SHOW_ID_HW	0
#define NFC_SHOW_ID_SW	1

#define sh_ver 1
#define NFC_DEBUG 0

extern int NFC_TEST(int type, unsigned char *id, int status);
extern void Set_i2c_client(struct i2c_client * client);

#define DRIVER_CARD	"PN544 NFC"
#define DRIVER_DESC	"NFC driver for PN544"

static struct i2c_device_id pn544_id_table[] = {
	{ PN544_DRIVER_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, pn544_id_table);

#define HCI_MODE	0
#define FW_MODE		1

enum pn544_state {
	PN544_ST_COLD,
	PN544_ST_FW_READY,
	PN544_ST_READY,
};

enum pn544_irq {
	PN544_NONE,
	PN544_INT,
};

struct pn544_info {
	struct miscdevice miscdev;
	struct i2c_client *i2c_dev;
	/* [temp solution, L6C always on] struct regulator_bulk_data regs[2]; */
       struct regulator_bulk_data regs[1];

	enum pn544_state state;
	wait_queue_head_t read_wq;
	loff_t read_offset;
	enum pn544_irq read_irq;
	struct mutex read_mutex; /* Serialize read_irq access */
	struct mutex mutex; /* Serialize info struct access */
	spinlock_t irq_enabled_lock;
	u8 *buf;
	size_t buflen;
	bool irq_enabled;
	int irq_gpio;
	struct nfc_prj_info_t tNfc_pri_info;
};

static const char reg_vdd_io[]	= "nfc_vdd"; //"NFC_Vdd_IO";
/* [temp solution, L6C always on] static const char reg_vsim[]	= "NFC_VSim"; */
//static const char reg_vsim[] = "nfc_vsim";

static struct kobject *devInfoNfc_kobj;

#define NfcInfo_attr(_name) \
        static struct kobj_attribute _name##_attr = { \
        .attr = { \
        .name = __stringify(_name), \
        .mode = 0644, \
        }, \
        .show = _name##_show, \
        }

NfcInfo_attr(NfcHW_ID);
NfcInfo_attr(NfcSW_ID);

static struct attribute * nfc_group[] = {
        &NfcHW_ID_attr.attr,
        &NfcSW_ID_attr.attr,
        NULL,
};

static struct attribute_group attr_nfc_group = {
        .attrs = nfc_group,
};

/**
 * Request and set the HW resources
 */
static int nxp_pn544_request_res (struct i2c_client *client)
{
        int rc, result = 0;

        pr_info("[NFC] : Start reusest resource....\n");

	//gpio request reset pin
	rc = gpio_request(NXP_PN544_RESET, "nfc_ldoen_n");
        if (rc) {
	     pr_err("[NFC] : request PN544_RESET failed, rc=%d\n", rc);
                return -ENODEV;
        }

	rc = gpio_direction_output(NXP_PN544_RESET, 1);
        if (rc) {
             pr_err("%s: unable to set direction for PN544_RESET\n",
                     __func__);
                 return -ENODEV;
        }

	//gpio request firmware download pin
        rc = gpio_request(NXP_PN544_FW_DL, "nfc_fw_dl");
        if (rc) {
             pr_err("[NFC] : request PN544_FW_DL failed, rc=%d\n", rc);
                 return -ENODEV;
        }

	rc = gpio_direction_output(NXP_PN544_FW_DL, 0);
        if (rc) {
             pr_err("%s: unable to set direction for PN544_FW_DL\n",
                     __func__);
                return -ENODEV;
        }

	//gpio request interrupt pin
        rc = gpio_request(NXP_PN544_INTR, "nfc_intr");
        if (rc) {
                pr_err("[NFC] : request PN544_INTR failed, rc=%d\n", rc);
                return -ENODEV;
        }

	rc = gpio_direction_input(NXP_PN544_INTR);
        if (rc) {
             pr_err("%s: unable to set direction for PN544_INTR\n",
                     __func__);
                return -ENODEV;
        }

	client->irq = gpio_to_irq(NXP_PN544_INTR);

        pr_info("[NFC] : End of request resource...\n");
        return result;
}

/**
 * Release the HW resources
 */
static void nxp_pn544_free_res (void)
{
        pr_info("[NFC] : start of free resource...\n");
	gpio_free(NXP_PN544_RESET);
        gpio_free(NXP_PN544_FW_DL);
}

/**
 * Setup pn544's GPIO4
 * 0 : HCI mode
 * 1 : FW download mode
 */
static void nxp_pn544_enable (int fw)
{
        if (fw) {
		pr_info("[NFC] : FW download mode\n");
		gpio_direction_output(NXP_PN544_RESET, 0);
		gpio_direction_output(NXP_PN544_FW_DL, 1);
		msleep(10);
		gpio_direction_output(NXP_PN544_RESET, 1);
		msleep(70);
		gpio_direction_output(NXP_PN544_RESET, 0);
		msleep(50);
        }
        else{
		pr_info("[NFC] : start of pn544 enable!\n");
		gpio_direction_output(NXP_PN544_RESET, 0);
		gpio_direction_output(NXP_PN544_FW_DL, 0);
		msleep(50);
        }
}

/**
  *Put the device into the FW update mode
  *and then back to the normal mode.
  *Check the behavior and return one on success zore on failure.
  *
  *PN544's GPIO4
  *0 : HCI mode (normal mode)
  *1 : FW download mode
  */
static int nxp_pn544_test (void)
{
        int result = 0;
        /*TODO : PN544 TEST$*/
        return result;
}

/**
 * Free GPIO
 */
static void nxp_pn544_disable (void)
{
	pr_info("[NFC] : start of pn544 disable!\n");
	gpio_direction_output(NXP_PN544_RESET, 1);
	gpio_direction_output(NXP_PN544_FW_DL, 0);
	msleep(70);
}

static struct pn544_nfc_platform_data nxp_pn544_pdata = {
        .request_resources = nxp_pn544_request_res,
        .free_resources = nxp_pn544_free_res,
        .enable = nxp_pn544_enable,
        .test = nxp_pn544_test,
        .disable = nxp_pn544_disable,
        .irq_gpio = NXP_PN544_INTR,
};

/* sysfs interface */
static ssize_t pn544_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	struct pn544_info *info = dev_get_drvdata(dev);
	struct pn544_nfc_platform_data *pdata = &nxp_pn544_pdata;

	info->tNfc_pri_info.bNfcTestStarted = TRUE;

	return snprintf(buf, PAGE_SIZE, "%d\n", pdata->test());
}

static ssize_t pn544_test_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct pn544_info *info = dev_get_drvdata(dev);
	int i = 0;

	U8 au8Buf[6] = {0}; /* for read ID/ver - output, reg read/write - input */

	info->tNfc_pri_info.bNfcTestStarted = TRUE;

	if (act_reg_read == buf[0] || act_reg_write == buf[0])
		NFC_DrvOp(buf[0], (void *) &(buf[1]));
	else /* Other than reg read/write operations */
	{
		while ('\n' != buf[i]) /* end of line*/
			NFC_DrvOp((buf[i++]), au8Buf);
	}

	return count;
}

int pn544_enable(struct pn544_info *info, int mode)
{
	struct pn544_nfc_platform_data *pdata;
	struct i2c_client *client = info->i2c_dev;

	int r;

	r = regulator_bulk_enable(ARRAY_SIZE(info->regs), info->regs);
	if (r < 0)
		return r;

	pdata = &nxp_pn544_pdata;
	info->read_irq = PN544_NONE;
	if (pdata->enable)
		pdata->enable(mode);

	if (mode) {
		info->state = PN544_ST_FW_READY;
		dev_dbg(&client->dev, "now in FW-mode\n");
	} else {
		info->state = PN544_ST_READY;
		dev_dbg(&client->dev, "now in HCI-mode\n");
	}

	info->tNfc_pri_info.bNfcEnabled = TRUE;
	usleep_range(10000, 15000);

	return 0;
}

void pn544_disable(struct pn544_info *info)
{
	struct pn544_nfc_platform_data *pdata;
	struct i2c_client *client = info->i2c_dev;

	pdata = &nxp_pn544_pdata;
	if (pdata->disable)
		pdata->disable();

	info->state = PN544_ST_COLD;
	dev_dbg(&client->dev, "Now in OFF-mode\n");

	msleep(PN544_RESETVEN_TIME);

	info->read_irq = PN544_NONE;
	regulator_bulk_disable(ARRAY_SIZE(info->regs), info->regs);

	info->tNfc_pri_info.bNfcEnabled = FALSE;

}

static void pn544_disable_irq(struct pn544_info *pn544_dev)
{
	unsigned long flags = 0;;

	spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
	if (pn544_dev->irq_enabled) {
		disable_irq_nosync(pn544_dev->i2c_dev->irq);
		pn544_dev->irq_enabled = false;
	}

	spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);
}

static irqreturn_t pn544_dev_irq_handler(int irq, void *dev_id)
{

	struct pn544_info *pn544_dev = dev_id;

	if (!gpio_get_value(pn544_dev->irq_gpio))
		return IRQ_HANDLED;

	pn544_disable_irq(pn544_dev);

	if (FALSE == pn544_dev->tNfc_pri_info.bNfcTestStarted)
		wake_up(&pn544_dev->read_wq);	/* Wake up waiting readers */

	return IRQ_HANDLED;
}

static ssize_t pn544_dev_read(struct file *filp, char __user *buf,
		size_t count, loff_t *offset)
{

	struct pn544_info *pn544_dev = filp->private_data;
	char tmp[PN544_MAX_BUFFER_SIZE];
	int ret,i;

	if (TRUE == pn544_dev->tNfc_pri_info.bNfcTestStarted)
		return EIO;

	if (count > PN544_MAX_BUFFER_SIZE)
		count = PN544_MAX_BUFFER_SIZE;

	if (NFC_DEBUG)
		printk("%s : reading %zu bytes.\n", __func__, count);

	mutex_lock(&pn544_dev->read_mutex);

	if (!gpio_get_value(pn544_dev->irq_gpio)) {
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto fail;
		}
		pn544_dev->irq_enabled = true;
		enable_irq(pn544_dev->i2c_dev->irq);
		ret = wait_event_interruptible(pn544_dev->read_wq,
				gpio_get_value(pn544_dev->irq_gpio));

		pn544_disable_irq(pn544_dev);
		if (ret)
			goto fail;
	}

	/* Read data */
	ret = i2c_master_recv(pn544_dev->i2c_dev, tmp, count);
	mutex_unlock(&pn544_dev->read_mutex);

	if (ret < 0) {
		pr_err("%s: i2c_master_recv returned %d\n", __func__, ret);
		return ret;
	}

	if (ret > count) {
		pr_err("%s: received too many bytes from i2c (%d)\n",
			__func__, ret);
		return -EIO;
	}

	if (copy_to_user(buf, tmp, ret)) {
		pr_warning("%s : failed to copy to user space\n", __func__);
		return -EFAULT;
	}

	if (NFC_DEBUG) {
		printk("PN544->Dev:");
		for(i = 0; i < ret; i++)
			printk(" %02X", tmp[i]);
		printk("\n");
	}

	return ret;

fail:
	mutex_unlock(&pn544_dev->read_mutex);
	return ret;
}

static ssize_t pn544_dev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *offset)
{

	struct pn544_info *pn544_dev = filp->private_data;
	char tmp[PN544_MAX_BUFFER_SIZE];
	int ret,i;

	if (TRUE == pn544_dev->tNfc_pri_info.bNfcTestStarted)
		return EIO;

	pn544_dev = filp->private_data;

	if (count > PN544_MAX_BUFFER_SIZE)
		count = PN544_MAX_BUFFER_SIZE;

	if (copy_from_user(tmp, buf, count)) {
		pr_err("%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}

	if (NFC_DEBUG)
		printk("%s : writing %zu bytes.\n", __func__, count);

	/* Write data */
	ret = i2c_master_send(pn544_dev->i2c_dev, tmp, count);

	if(ret == -EREMOTEIO)
	{
		usleep_range(6000,10000);
		ret = i2c_master_send(pn544_dev->i2c_dev, tmp, count);
		pr_err("[PN544_DEBUG],I2C Writer Retry,%d",ret);
	}

	if (ret != count) {
		pr_err("%s : i2c_master_send returned %d\n", __func__, ret);
		ret = -EIO;
	}

	if (NFC_DEBUG) {
		printk("Dev->PN544:");
		for(i = 0; i < count; i++)
			printk(" %02X", tmp[i]);

		printk("\n");
	}

	return ret;
}

static long pn544_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct pn544_info *info = file->private_data;
	struct i2c_client *client = info->i2c_dev;
	struct pn544_nfc_platform_data *pdata;
	int r = 0;

	if (TRUE == info->tNfc_pri_info.bNfcTestStarted)
		return EIO;

	dev_dbg(&client->dev, "%s: info: %p, cmd: 0x%x\n", __func__, info, cmd);

	printk("pn544_ioctl(),cmd = %d, arg = %d\n", (int)cmd, (int)arg);

	mutex_lock(&info->mutex);
	pdata = &nxp_pn544_pdata;

	switch (cmd) {
	case PN544_SET_PWR:
		pdata = &nxp_pn544_pdata;
		dev_dbg(&client->dev, "%s:  PN544_SET_PWR_MODE\n", __func__);
		if (arg == 2) {
			pn544_enable(info, FW_MODE);
		} else if (arg == 1) {
			pn544_enable(info, HCI_MODE);
		} else if (arg == 0) {
			pn544_disable(info);
		} else {
			printk("[PN544] the pwr set error...");
			r = -EINVAL;
			goto out;
		}
		break;

	default:
		printk("[PN544] The IOCTL isn't exist!!\n");
		r = -EINVAL;
		break;
	}
out:
	mutex_unlock(&info->mutex);
	return r;
}

static int pn544_open(struct inode *inode, struct file *file)
{
	struct pn544_info *info = container_of(file->private_data,
					       struct pn544_info, miscdev);
	struct i2c_client *client = info->i2c_dev;
	int r = 0;

	dev_dbg(&client->dev, "%s: info: %p, client %p\n", __func__,
		info, info->i2c_dev);

	file->private_data = info;

	return r;
}

static int pn544_close(struct inode *inode, struct file *file)
{
	struct pn544_info *info = file->private_data;
	struct i2c_client *client = info->i2c_dev;

	dev_dbg(&client->dev, "%s: info: %p, client %p\n",
		__func__, info, info->i2c_dev);

	mutex_lock(&info->mutex);
	if (info->tNfc_pri_info.bNfcEnabled)
		pn544_disable(info);
	mutex_unlock(&info->mutex);

	return 0;
}

static const struct file_operations pn544_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.read		= pn544_dev_read,
	.write		= pn544_dev_write,
	.open		= pn544_open,
	.release	= pn544_close,
	.unlocked_ioctl	= pn544_ioctl,
};

#ifdef CONFIG_PM
static int pn544_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pn544_info *info;
	int r = 0;

	dev_info(&client->dev, "***\n%s: client %p\n***\n", __func__, client);

	info = i2c_get_clientdata(client);
	dev_info(&client->dev, "%s: info: %p, client %p\n", __func__,
		 info, client);

	mutex_lock(&info->mutex);

	switch (info->state) {
	case PN544_ST_FW_READY:
		/* Do not suspend while upgrading FW, please! */
		r = -EPERM;
		break;

	case PN544_ST_READY:
		/*
		 * CHECK: Device should be in standby-mode. No way to check?
		 * Allowing low power mode for the regulator is potentially
		 * dangerous if pn544 does not go to suspension.
		 */
		break;

	case PN544_ST_COLD:
		break;
	};

	mutex_unlock(&info->mutex);
	return r;
}

static int pn544_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pn544_info *info = i2c_get_clientdata(client);
	int r = 0;

	dev_dbg(&client->dev, "%s: info: %p, client %p\n", __func__,
		info, client);

	mutex_lock(&info->mutex);

	switch (info->state) {
	case PN544_ST_READY:
		/*
		 * CHECK: If regulator low power mode is allowed in
		 * pn544_suspend, we should go back to normal mode
		 * here.
		 */
		break;

	case PN544_ST_COLD:
		break;

	case PN544_ST_FW_READY:
		break;
	};

	mutex_unlock(&info->mutex);

	return r;
}

static SIMPLE_DEV_PM_OPS(pn544_pm_ops, pn544_suspend, pn544_resume);
#endif

static struct device_attribute pn544_attr =
	__ATTR(nfc_test, S_IRUGO |S_IWUSR, pn544_test_show, pn544_test_store);

static int power_set(struct regulator_bulk_data *regs, int size)
{

	int rc, i;
	for (i = 0; i < size ; i++) {
		rc = regulator_set_voltage(regs[i].consumer, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage reg[%d] failed, rc=%d\n", i, rc);
			return -1;
		}
		rc = regulator_set_optimum_mode(regs[i].consumer, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode reg[%d] failed, rc=%d\n", i,  rc);
			return -1;
		}
	}

	return 0;
}


static int __devinit pn544_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	struct pn544_info *info;
	struct pn544_nfc_platform_data *pdata;
	int r = 0, iRet;

	dev_dbg(&client->dev, "%s\n", __func__);
	dev_dbg(&client->dev, "IRQ: %d\n", client->irq);
	pr_info("[NFC]IRQ = %d ....\n", client->irq);
	pr_info("[NFC]PN544 probe start....\n");

	/* private data allocation */
	info = kzalloc(sizeof(struct pn544_info), GFP_KERNEL);
	if (!info) {
		dev_err(&client->dev,
			"Cannot allocate memory for pn544_info.\n");
		r = -ENOMEM;
		goto err_info_alloc;
	}

	info->buflen = max(PN544_MSG_MAX_SIZE, PN544_MAX_I2C_TRANSFER);
	info->buf = kzalloc(info->buflen, GFP_KERNEL);
	if (!info->buf) {
		dev_err(&client->dev,
			"Cannot allocate memory for pn544_info->buf.\n");
		r = -ENOMEM;
		goto err_buf_alloc;
	}

	pr_info("[NFC]PN544 probe : power setting....\n");
	info->regs[0].supply = reg_vdd_io;
	/* [temp solution, L6C always on] info->regs[1].supply = reg_vsim; */
//	info->regs[1].supply = reg_vsim;

	r = regulator_bulk_get(&client->dev, ARRAY_SIZE(info->regs),
				 info->regs);
	if (r < 0) {
		pr_info("[NFC]PN544 probe : regulator get fail ...\n");
		goto err_kmalloc;
	}

	power_set(info->regs, ARRAY_SIZE(info->regs));

	pr_info("[NFC]PN544 probe : resources setting....\n");
	info->i2c_dev = client;
	mutex_init(&info->read_mutex);
	mutex_init(&info->mutex);
	init_waitqueue_head(&info->read_wq);
	spin_lock_init(&info->irq_enabled_lock);
	i2c_set_clientdata(client, info);
	pdata = &nxp_pn544_pdata;
	if (!pdata) {
		dev_err(&client->dev, "No platform data\n");
		r = -EINVAL;
		goto err_reg;
	}

	if (!pdata->request_resources) {
		dev_err(&client->dev, "request_resources() missing\n");
		r = -EINVAL;
		goto err_reg;
	}

	r = pdata->request_resources(client);
	if (r) {
		dev_err(&client->dev, "Cannot get platform resources\n");
		goto err_reg;
	}

	pr_info("[NFC]PN544 probe : misc devices setting....\n");
	info->miscdev.minor = MISC_DYNAMIC_MINOR;
	info->miscdev.name = PN544_DRIVER_NAME;
	info->miscdev.fops = &pn544_fops;
	info->miscdev.parent = &client->dev;
	r = misc_register(&info->miscdev);
	if (r < 0) {
		dev_err(&client->dev, "Device registration failed\n");
		goto err_sysfs;
	}

	dev_dbg(&client->dev, "%s: info: %p, pdata %p, client %p\n",
		__func__, info, pdata, client);

	pr_info("[NFC]PN544 probe : IRQ setting....\n");

	info->irq_enabled = true;
	info->irq_gpio = pdata->irq_gpio;

	r = request_irq(client->irq, pn544_dev_irq_handler,
					(IRQF_TRIGGER_HIGH | IRQF_ONESHOT), PN544_DRIVER_NAME, info);
	if (r < 0) {
		dev_err(&client->dev, "Unable to register IRQ handler\n");
		goto err_res;
	}

	pn544_disable_irq(info);

	/* If we don't have the test we don't need the sysfs file */
	if (pdata->test) {
		r = device_create_file(&client->dev, &pn544_attr);
		if (r) {
			dev_err(&client->dev,
				"sysfs registration failed, error %d\n", r);
			goto err_irq;
		}
	}

	devInfoNfc_kobj = kobject_create_and_add("dev-info_nfc", NULL);

	if (devInfoNfc_kobj == NULL)
		pr_info("## %s, kobject_create_and_add failed\n", __FUNCTION__);

	iRet = sysfs_create_group(devInfoNfc_kobj, &attr_nfc_group);

	if (iRet)
		pr_info("## %s, sysfs_create_group failed\n", __FUNCTION__);

	Set_i2c_client(client);

	g_ptPn544Info = info;
	g_ptNfcPrjInfo = &(info->tNfc_pri_info);

	return 0;

err_sysfs:
	if (pdata->test)
		device_remove_file(&client->dev, &pn544_attr);
err_irq:
	free_irq(client->irq, info);
err_res:
	if (pdata->free_resources)
		pdata->free_resources();
err_reg:
	regulator_bulk_free(ARRAY_SIZE(info->regs), info->regs);

err_kmalloc:
	kfree(info->buf);
err_buf_alloc:
	kfree(info);
err_info_alloc:
	return r;
}

static ssize_t Nfc_ID_show_proc(int iHW_SW_ID, char * buf)
{
	const char sNFC_DEFAULT_HW_ID[] = "NFC HW_ID unavailable";
	const char sNFC_DEFAULT_SW_ID[] = "NFC SW_ID unavailable";

	if ((NULL == S_H_version) && (0 == pn544_enable(g_ptPn544Info, HCI_MODE)))
	{
		msleep(200);
		NFC_TEST(r_ver, 0, 0);
		msleep(200);
		pn544_disable(g_ptPn544Info);
	}

	if (S_H_version)
	{	if (NFC_SHOW_ID_HW == iHW_SW_ID)
			sprintf(buf,"NFC HW_ID: %d%d%d\n",S_H_version->hw_ver[0], S_H_version->hw_ver[1], S_H_version->hw_ver[2]);
		else /* (NFC_SHOW_ID_SW == iHW_SW_ID) */
			sprintf(buf,"NFC SW_ID: %d%d%d\n",S_H_version->sw_ver[0], S_H_version->sw_ver[1], S_H_version->sw_ver[2]);
	}
	else
	{	if (NFC_SHOW_ID_HW == iHW_SW_ID)
			sprintf(buf,"%s\n",sNFC_DEFAULT_HW_ID);
		else /* (NFC_SHOW_ID_SW == iHW_SW_ID) */
			sprintf(buf,"%s\n",sNFC_DEFAULT_SW_ID);
	}

	return strlen(buf);

}

static ssize_t NfcHW_ID_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	return Nfc_ID_show_proc(NFC_SHOW_ID_HW, buf);
}

static ssize_t NfcSW_ID_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	return Nfc_ID_show_proc(NFC_SHOW_ID_SW, buf);
}

static __devexit int pn544_remove(struct i2c_client *client)
{
	struct pn544_info *info = i2c_get_clientdata(client);
	struct pn544_nfc_platform_data *pdata = &nxp_pn544_pdata;

	dev_dbg(&client->dev, "%s\n", __func__);

	misc_deregister(&info->miscdev);
	if (pdata->test)
		device_remove_file(&client->dev, &pn544_attr);

	if (info->state != PN544_ST_COLD) {
		if (pdata->disable)
			pdata->disable();

		info->read_irq = PN544_NONE;
	}

	free_irq(client->irq, info);
	if (pdata->free_resources)
		pdata->free_resources();

	regulator_bulk_free(ARRAY_SIZE(info->regs), info->regs);
	kfree(info->buf);
	kfree(info);

	return 0;
}

MODULE_DEVICE_TABLE(i2c, pn544_id_table);

static struct of_device_id nfc_match_table[] = {
        {.compatible = "nxp,pn544",},
        { },
};

static struct i2c_driver pn544_driver = {
        .driver = {
                .name = PN544_DRIVER_NAME,
                .owner = THIS_MODULE,
                .of_match_table = nfc_match_table,
#ifdef CONFIG_PM
                .pm = &pn544_pm_ops,
#endif
        },
        .probe = pn544_probe,
        .id_table = pn544_id_table,
        .remove = __devexit_p(pn544_remove),
};

module_i2c_driver(pn544_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);
