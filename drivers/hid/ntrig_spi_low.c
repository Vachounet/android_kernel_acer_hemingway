/*
 * drivers/hid/ntrig_spi_low.c
 *
 * Copyright (c) 2012, N-Trig
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/delay.h>


#include "ntrig_spi_low.h"

#define DRIVER_NAME "ntrig_spi"

static struct spi_device_data s_spi_device_data;
static struct regulator **ts_vdd;

#define VERG_LVS1_VTG_MIN_UV       1800000
#define VERG_LVS1_VTG_MAX_UV       1800000
#define VERG_LVS1_CURR_UA          9630

#ifdef CONFIG_CONTROL_LVS1
static struct ntrig_spi_ts_regulator a12_ts_regulator_data[] = {
       {
               .name = "vcc_spi",
               .min_uV = VERG_LVS1_VTG_MIN_UV,
               .max_uV = VERG_LVS1_VTG_MIN_UV,
               .load_uA = VERG_LVS1_CURR_UA,
       },
};
#endif

#ifdef CONFIG_PM

static ntrig_spi_suspend_callback suspend_callback;
static ntrig_spi_resume_callback resume_callback;

void ntrig_spi_register_pwr_mgmt_callbacks(
	ntrig_spi_suspend_callback s, ntrig_spi_resume_callback r)
{
	suspend_callback = s;
	resume_callback = r;
}
EXPORT_SYMBOL_GPL(ntrig_spi_register_pwr_mgmt_callbacks);

/**
 * called when suspending the device (sleep with preserve
 * memory)
 */
static int ntrig_spi_suspend(struct spi_device *dev, pm_message_t mesg)
{
	printk(KERN_DEBUG "in %s\n", __func__);
	if (suspend_callback)
		return suspend_callback(dev, mesg);
	return 0;
}

/**
 * called when resuming after sleep (suspend)
 */
static int ntrig_spi_resume(struct spi_device *dev)
{
	printk(KERN_DEBUG "in %s\n", __func__);
	if (resume_callback)
		return resume_callback(dev);
	return 0;
}

#endif	/* CONFIG_PM */

#ifdef CONFIG_CONTROL_LVS1
/**
       Ntrig_init hw function
       configure regulator
*/
static int ntrig_spi_hw_init(struct device *dev){
	int rc = 0, i;
	const struct ntrig_spi_ts_regulator *reg_info = NULL;
	u8 num_reg = 0;

	reg_info = a12_ts_regulator_data;
	num_reg = ARRAY_SIZE(a12_ts_regulator_data);

	ts_vdd = kzalloc(num_reg * sizeof(struct regulator *), GFP_KERNEL);
	printk("\n[ntrig_spi_hw_init] start-----------\n");
	if (!reg_info) {
		printk("\n[ntrig_spi_hw_init]regulator pdata not specified\n");
		return -EINVAL;
	}

	if (!ts_vdd) {
		printk("\n[ntrig_spi_hw_init]unable to allocate memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < num_reg; i++) {
		ts_vdd[i] = regulator_get(dev, reg_info[i].name);

		if (IS_ERR(ts_vdd[i])) {
			rc = PTR_ERR(ts_vdd[i]);
			printk("\n[ntrig_spi_hw_init]%s:regulator get failed rc=%d\n",__func__, rc);
			goto error_vdd;
		}

		if (regulator_count_voltages(ts_vdd[i]) > 0) {
			rc = regulator_set_voltage(ts_vdd[i],reg_info[i].min_uV, reg_info[i].max_uV);
			if (rc) {
				printk("\n[ntrig_spi_hw_init]%s: regulator_set_voltage failed rc =%d\n", __func__, rc);
				regulator_put(ts_vdd[i]);
				goto error_vdd;
			}
		}

		rc = regulator_set_optimum_mode(ts_vdd[i], reg_info[i].load_uA);
		if (rc < 0) {
			printk("\n[ntrig_spi_hw_init]%s: regulator_set_optimum_mode failed rc=%d\n",
                                                               __func__, rc);

			regulator_set_voltage(ts_vdd[i], 0,reg_info[i].max_uV);
			regulator_put(ts_vdd[i]);
			goto error_vdd;
		}

		rc = regulator_enable(ts_vdd[i]);
		if (rc) {
			printk("\n[ntrig_spi_hw_init]%s: regulator_enable failed rc =%d\n",
                                                               __func__, rc);
			regulator_set_voltage(ts_vdd[i], 0, reg_info[i].max_uV);
			regulator_put(ts_vdd[i]);
			goto error_vdd;
		}
	}

	return rc;

error_vdd:
	return rc;
}
#endif //CONFIG_CONTROL_LVS1

/**
 * release the SPI driver
 */
static int ntrig_spi_remove(struct spi_device *spi)
{
	printk(KERN_DEBUG "in %s\n", __func__);
	return 0;
}

#define NTRIG_SPI_POWER_OUTPUT_ENABLE 		76
#define NTRIG_SPI_DATA_OUTPUT_ENABLE		90
#define NTRIG_SPI_INT_OUTPUT_ENABLE			92

static struct ntrig_spi_platform_data ntrig_spi_pdata = {
	.data_oe_gpio = NTRIG_SPI_DATA_OUTPUT_ENABLE,          /*data oe*/
	.power_oe_gpio = NTRIG_SPI_POWER_OUTPUT_ENABLE,   /*power oe*/
	.int_oe_gpio = NTRIG_SPI_INT_OUTPUT_ENABLE,       /*interrupt oe*/
	.oe_inverted = 0,
};


struct spi_device_data *get_ntrig_spi_device_data(void)
{
	return &s_spi_device_data;
}
EXPORT_SYMBOL_GPL(get_ntrig_spi_device_data);

static int __devinit ntrig_spi_probe(struct spi_device *spi)
{
	printk("\n[ntrig_spi_low] ntrig spi probe...\n");

#ifdef CONFIG_CONTROL_LVS1 //Currently the LVS1 is controlled at SBL1 stage, we don't control this in touch.
	ntrig_spi_hw_init(&spi->dev);
	msleep(200);
#endif

	//printk(KERN_DEBUG "in %s\n", __func__);
	s_spi_device_data.m_spi_device = spi;
	s_spi_device_data.m_platform_data = ntrig_spi_pdata;//*(struct ntrig_spi_platform_data *)spi->dev.platform_data;
	if (ts_vdd){
		printk("\n[ntrig_spi_low] add regulator information\n");
		s_spi_device_data.m_reg_ts_spi = ts_vdd[0];
	}
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id ntrig_match_table[] = {
       { .compatible = "ntrig,ntrig-spi",},
       { },
};
#else
#endif

static struct spi_driver ntrig_spi_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = ntrig_match_table,
	},
	.suspend = ntrig_spi_suspend,
	.resume = ntrig_spi_resume,
	.probe		= ntrig_spi_probe,
	.remove		= __devexit_p(ntrig_spi_remove),
};


static int __init ntrig_spi_init(void)
{
	int ret = 0;

	printk(KERN_DEBUG "in %s\n", __func__);
	ret = spi_register_driver(&ntrig_spi_driver);
	if (ret != 0)
		printk(KERN_ERR "Failed to register N-trig SPI driver: %d\n",
			ret);
	return ret;
}
module_init(ntrig_spi_init);

static void __exit ntrig_spi_exit(void)
{
	spi_unregister_driver(&ntrig_spi_driver);
}
module_exit(ntrig_spi_exit);


MODULE_ALIAS("ntrig_spi");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("N-Trig SPI driver");
