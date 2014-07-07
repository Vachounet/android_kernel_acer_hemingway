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

#include <linux/module.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include "mdss_io_util.h"
#include "../../../../arch/arm/mach-msm/acer_hw_version.h"

#define MHL_DRIVER_NAME "isl98607"
#define COMPATIBLE_NAME "qcom,isl98607"

#define I2C_ADDR_EVT	0x28
#define I2C_ADDR_DVT	0x29

#define VBST_ADDR	0x06
#define VBST_VAL	0x07 /* 5.2v */
#define VP_ADDR	0x09
#define VP_VAL	0x06 /* 5.2v */
#define VN_ADDR	0x08
#define VN_VAL	0x06 /* 5.2v */

static struct regulator *reg_8941_smps3a;
static int chip_ok = 1;

int ftm_chip_test(void)
{
	return chip_ok;
}

static int isl98607_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	u8 addr, val;
	u8 slave_addr;

	if (!reg_8941_smps3a) {
		reg_8941_smps3a = regulator_get(&client->dev,
			"smps3a");
		if (IS_ERR(reg_8941_smps3a)) {
			pr_err("could not get reg_8038_l20, rc = %ld\n",
				PTR_ERR(reg_8941_smps3a));
			return -ENODEV;
		}
		regulator_enable(reg_8941_smps3a);
	}

	if (acer_board_id <= HW_ID_EVT)
		slave_addr = I2C_ADDR_EVT << 1;
	else
		slave_addr = I2C_ADDR_DVT << 1;

	addr = VBST_ADDR;
	val = VBST_VAL;
	mdss_i2c_byte_write(client, slave_addr, addr, &val);

	addr = VP_ADDR;
	val = VP_VAL;
	mdss_i2c_byte_write(client, slave_addr, addr, &val);

	addr = VN_ADDR;
	val = VN_VAL;
	mdss_i2c_byte_write(client, slave_addr, addr, &val);
	pr_info("%s: panel config p/n 5V done\n", __func__);

	val = 0xff;
	mdss_i2c_byte_read(client, slave_addr, VP_ADDR, &val);
	if (val != VP_VAL)
		chip_ok = -1;
	mdss_i2c_byte_read(client, slave_addr, VN_ADDR, &val);
	if (val != VN_VAL)
		chip_ok = -1;
	return 0;
}


static int isl98607_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_device_id isl98607_i2c_id[] = {
	{ MHL_DRIVER_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, isl98607_i2c_id);

static struct of_device_id match_table[] = {
	{.compatible = COMPATIBLE_NAME,},
	{ },
};

static struct i2c_driver isl98607_i2c_driver = {
	.driver = {
		.name = MHL_DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = match_table,
	},
	.probe = isl98607_probe,
	.remove =  isl98607_i2c_remove,
	.id_table = isl98607_i2c_id,
};

module_i2c_driver(isl98607_i2c_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ISL98607 driver IC");
