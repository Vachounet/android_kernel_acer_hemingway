/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#include <linux/init.h>
#include <linux/ioport.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>
#include "acer_hw_version.h"

int acer_board_id;
int acer_rf_id;
int acer_sku_id;
int acer_boot_mode;
int acer_ddr_vendor;

static int __init boot_mode(char *options)
{
	if (options && !strcmp(options, "charger"))
		acer_boot_mode = CHARGER_BOOT;
	else if (options && !strcmp(options, "recovery"))
		acer_boot_mode = RECOVERY_BOOT;
	else
		acer_boot_mode = NORMAL_BOOT;
	return 0;
}
early_param("androidboot.mode", boot_mode);

static int __init hw_ver_arg(char *options)
{
    int hw_ver = 0;  // 76543210 FEDCBA9876543210
                     // bbbbbbbb rrrrrrrrssssssss
                     // b: board id bits
                     // r: rf id bits
                     // s: sku id bits
    hw_ver = simple_strtoul(options, &options, 16);
    acer_board_id  = (hw_ver >> 16);
    acer_rf_id = ((hw_ver & 0xFF00) >> 8);
    acer_sku_id = (hw_ver & 0xFF);

    return 0;
}
early_param("hw_version", hw_ver_arg);

static int __init ddr_vendor(char *options)
{
	acer_ddr_vendor = simple_strtoul(options, &options, 16);
	return 0;
}
early_param("ddr_vendor", ddr_vendor);

#define KS8851_IRQ_GPIO 94

static struct gpiomux_setting gpio_unused_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

#if defined(CONFIG_MACH_ACER_A12)
static struct msm_gpiomux_config a12_dvt1_gpio_unused_configs[] __initdata = {
	{
		.gpio = 26,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 27,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 31,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 49,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 50,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 52,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 69,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 102,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 103,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 104,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 105,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 109,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 112,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 114,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 117,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 118,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 119,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 121,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 122,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 124,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 125,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 129,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 130,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 131,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 132,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 135,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 136,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 144,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
};

static struct msm_gpiomux_config a12_dvt2_gpio_unused_configs[] __initdata = {
	{
		.gpio = 25,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 26,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 27,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 31,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 49,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 50,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 52,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 60,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
#ifndef CONFIG_MACH_ACER_A12
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
#endif
	{
		.gpio = 69,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 102,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 103,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 104,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 105,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 109,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 112,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 114,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 117,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 118,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 119,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 121,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 122,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 124,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 125,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 129,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 130,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 131,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 132,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 135,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 136,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 144,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
};

static struct msm_gpiomux_config a12_dvt3_4_gpio_unused_configs[] __initdata = {
	{
		.gpio = 25,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 26,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 27,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 31,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 49,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 50,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 52,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 60,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 69,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 102,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 103,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 104,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 105,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 109,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 112,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 114,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 117,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 118,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 119,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 121,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 122,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 124,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 125,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 129,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 130,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 131,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 132,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 135,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 136,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 144,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
};

static struct msm_gpiomux_config a12_pvt_gpio_unused_configs[] __initdata = {
	{
		.gpio = 8,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 25,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 26,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 27,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 31,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 49,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 50,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 52,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 60,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 69,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 85,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 102,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 103,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 104,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 105,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 109,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 112,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 114,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 117,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 118,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 119,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 121,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 122,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 124,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 125,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 129,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 130,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 131,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 132,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 135,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 136,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 144,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
};
#endif

static struct gpiomux_setting ap2mdm_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mdm2ap_status_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting mdm2ap_errfatal_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting mdm2ap_pblrdy = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};


static struct gpiomux_setting ap2mdm_soft_reset_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting ap2mdm_wakeup = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config mdm_configs[] __initdata = {
	/* AP2MDM_STATUS */
	{
		.gpio = 105,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* MDM2AP_STATUS */
	{
		.gpio = 46,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
	/* MDM2AP_ERRFATAL */
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_errfatal_cfg,
		}
	},
	/* AP2MDM_ERRFATAL */
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* AP2MDM_SOFT_RESET, aka AP2MDM_PON_RESET_N */
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_soft_reset_cfg,
		}
	},
	/* AP2MDM_WAKEUP */
	{
		.gpio = 104,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_wakeup,
		}
	},
	/* MDM2AP_PBL_READY*/
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_pblrdy,
		}
	},
};

static struct gpiomux_setting gpio_uart_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting slimbus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct gpiomux_setting gpio_eth_config = {
	.pull = GPIOMUX_PULL_UP,
	.drv = GPIOMUX_DRV_2MA,
	.func = GPIOMUX_FUNC_GPIO,
};

static struct gpiomux_setting gpio_spi_cs2_config = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gpio_spi_cs1_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm_eth_configs[] = {
	{
		.gpio = KS8851_IRQ_GPIO,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
		}
	},
};
#endif

static struct gpiomux_setting gpio_suspend_config[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,  /* IN-NP */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,  /* O-LOW */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},
};

static struct gpiomux_setting wcnss_5wire_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting wcnss_5wire_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting ath_gpio_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ath_gpio_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	/*
	 * Please keep I2C GPIOs drive-strength at minimum (2ma). It is a
	 * workaround for HW issue of glitches caused by rapid GPIO current-
	 * change.
	 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_spi_blsp11_config[] = {
	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_2,
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},
};

#if defined(CONFIG_FB_MSM_MDSS_MIPI_DSI_SHARP) || defined(CONFIG_FB_MSM_MDSS_MIPI_DSI_INNOLUX)
static struct gpiomux_setting lcd_5v_en_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting lcd_5v_en_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_HIGH,
};
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4
static struct gpiomux_setting atmel_resout_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting atmel_resout_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting atmel_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting atmel_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif

#ifdef CONFIG_NTRIG_SPI
static struct gpiomux_setting ntrig_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting ntrig_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif

static struct gpiomux_setting taiko_reset = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting taiko_int = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting nfc_active_cfg = {
        .func = GPIOMUX_FUNC_GPIO,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_NONE,
        .dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting nfc_suspend_cfg = {
        .func = GPIOMUX_FUNC_GPIO,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_NONE,
        .dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting nfc_int_active_cfg = {
        .func = GPIOMUX_FUNC_GPIO,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_DOWN,
        .dir = GPIOMUX_IN,
};
static struct gpiomux_setting nfc_int_suspend_cfg = {
        .func = GPIOMUX_FUNC_GPIO,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_DOWN,
        .dir = GPIOMUX_IN,
};

#ifdef CONFIG_ACER_HEADSET
static struct gpiomux_setting a12_hs_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting a12_hs_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN
};
#endif

#ifdef CONFIG_ACER_HEADSET_BUTT
static struct gpiomux_setting a12_hs_butt_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN
};

static struct gpiomux_setting a12_hs_butt_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN
};
#endif
static struct gpiomux_setting hap_lvl_shft_suspended_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hap_lvl_shft_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};
static struct msm_gpiomux_config hap_lvl_shft_config[] __initdata = {
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_SUSPENDED] = &hap_lvl_shft_suspended_config,
			[GPIOMUX_ACTIVE] = &hap_lvl_shft_active_config,
		},
	},
};

static struct msm_gpiomux_config msm_touch_configs[] __initdata = {
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4
	{
		.gpio      = 60,		/* A11 TOUCH RESET */
		.settings = {
			[GPIOMUX_ACTIVE] = &atmel_resout_act_cfg,
			[GPIOMUX_SUSPENDED] = &atmel_resout_sus_cfg,
		},
	},
	{
		.gpio      = 77,		/* A11 TOUCH IRQ*/
		.settings = {
			[GPIOMUX_ACTIVE] = &atmel_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &atmel_int_sus_cfg,
		},
	},
#endif
#ifdef CONFIG_NTRIG_SPI
	{
		.gpio      = 61,		/* NTRIG TOUCH IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &ntrig_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &ntrig_int_sus_cfg,
		},
	},
	{
		.gpio      = 76,		/* TOUCH POWER OE (TP_EN) */
		.settings = {
			[GPIOMUX_ACTIVE] = &ntrig_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &ntrig_int_sus_cfg,
		},
	},
	{
		.gpio      = 90,		/* TP_DATA_OE */
		.settings = {
			[GPIOMUX_ACTIVE] = &ntrig_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &ntrig_int_sus_cfg,
		},
	},
	{
		.gpio      = 92,		/* TP_INT_OE */
		.settings = {
			[GPIOMUX_ACTIVE] = &ntrig_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &ntrig_int_sus_cfg,
		},
	},
#endif
};

#ifdef CONFIG_MACH_ACER_A12
static struct gpiomux_setting cypress_resout_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting cypress_resout_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting cypress_irq_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting cypress_irq_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm_capsense_configs[] __initdata = {
	{
		.gpio      = 13,		/* Capsense controller RESET */
		.settings = {
			[GPIOMUX_ACTIVE] = &cypress_resout_act_cfg,
			[GPIOMUX_SUSPENDED] = &cypress_resout_sus_cfg,
		},
	},
	{
		.gpio      = 73,		/* Capsense controller IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &cypress_irq_act_cfg,
			[GPIOMUX_SUSPENDED] = &cypress_irq_sus_cfg,
		},
	},

};

static struct gpiomux_setting hall_irq_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hall_irq_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm_hall_configs[] __initdata = {
	{
		.gpio      = 65,		/* Hall sensor IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &hall_irq_act_cfg,
			[GPIOMUX_SUSPENDED] = &hall_irq_sus_cfg,
		},
	},
};

static struct gpiomux_setting vib_en_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting vib_en_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config vib_en_configs[] __initdata = {
	{
		.gpio      = 91,
		.settings = {
			[GPIOMUX_ACTIVE] = &vib_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &vib_en_sus_cfg,
		},
	},
};
#endif

static struct gpiomux_setting hsic_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hsic_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hsic_hub_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting hsic_resume_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting hsic_resume_susp_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm_hsic_configs[] = {
	{
		.gpio = 144,               /*HSIC_STROBE */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 145,               /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_resume_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_resume_susp_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_hsic_hub_configs[] = {
	{
		.gpio = 50,               /* HSIC_HUB_INT_N */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_hub_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
};

static struct gpiomux_setting mhl_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mhl_active_1_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting mhl_active_2_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting hdmi_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting hdmi_active_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_DOWN,
};


static struct msm_gpiomux_config msm_mhl_configs[] __initdata = {
	{
		/* mhl-sii8334 pwr */
		.gpio = 8,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mhl_suspend_config,
			[GPIOMUX_ACTIVE]    = &mhl_active_1_cfg,
		},
	},
	{
		/* mhl-sii8334 intr */
		.gpio = 86,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mhl_suspend_config,
			[GPIOMUX_ACTIVE]    = &mhl_active_1_cfg,
		},
	},
	{
		/* mhl-sii8334 reset */
		.gpio = 85,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mhl_suspend_config,
			[GPIOMUX_ACTIVE]    = &mhl_active_2_cfg,
		},
	},
};


static struct msm_gpiomux_config msm_hdmi_configs[] __initdata = {
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 33,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
};

#ifndef CONFIG_MACH_ACER_A12
static struct gpiomux_setting gpio_uart7_active_cfg = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_uart7_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm_blsp2_uart7_configs[] __initdata = {
	{
		.gpio	= 41,	/* BLSP2 UART7 TX */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_uart7_active_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_uart7_suspend_cfg,
		},
	},
	{
		.gpio	= 42,	/* BLSP2 UART7 RX */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_uart7_active_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_uart7_suspend_cfg,
		},
	},
	{
		.gpio	= 43,	/* BLSP2 UART7 CTS */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_uart7_active_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_uart7_suspend_cfg,
		},
	},
	{
		.gpio	= 44,	/* BLSP2 UART7 RFR */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_uart7_active_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_uart7_suspend_cfg,
		},
	},
};
#endif

static struct msm_gpiomux_config msm_rumi_blsp_configs[] __initdata = {
#ifndef CONFIG_MACH_ACER_A12
	{
		.gpio      = 45,	/* BLSP2 UART8 TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 46,	/* BLSP2 UART8 RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
#endif
};

#if defined(CONFIG_FB_MSM_MDSS_MIPI_DSI_SHARP) || defined(CONFIG_FB_MSM_MDSS_MIPI_DSI_INNOLUX)
static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	{
		.gpio      = 0,		/* LCDC_P5V_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_5v_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_5v_en_sus_cfg,
		},
	},
	{
		.gpio      = 1,		/* LCDC_N5V_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_5v_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_5v_en_sus_cfg,
		},
	},
	{
		.gpio      = 2,		/* LCDC_pn5v controller */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 3,		/* LCDC_pn5v controller */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
};
#endif

static struct msm_gpiomux_config msm_blsp_configs[] __initdata = {
#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	{
		.gpio      = 9,		/* BLSP1 QUP SPI_CS2A_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs2_config,
		},
	},
	{
		.gpio      = 8,		/* BLSP1 QUP SPI_CS1_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs1_config,
		},
	},
#endif
	{
		.gpio      = 4,			/* BLSP2 UART TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 5,			/* BLSP2 UART RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 6,		/* BLSP1 QUP2 I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 7,		/* BLSP1 QUP2 I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 29,		/* BLSP6 I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 30,		/* BLSP6 I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 47,		/* BLSP8 I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 48,		/* BLSP8 I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 55,		/* BLSP10 I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 56,		/* BLSP10 I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 81,		/* BLSP11 SPI_MOSI */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_blsp11_config[1],
		},
	},
	{
		.gpio      = 82,		/* BLSP11 SPI_MISO */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_blsp11_config[1],
		},
	},
	{
		.gpio      = 83,		/* BLSP11 SPI_CS_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_blsp11_config[0],
		},
	},
	{
		.gpio      = 84,		/* BLSP11 SPI_MOSI */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_blsp11_config[0],
		},
	},
};

static struct msm_gpiomux_config msm8974_slimbus_config[] __initdata = {
	{
		.gpio	= 70,		/* slimbus clk */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
	{
		.gpio	= 71,		/* slimbus data */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
};

static struct gpiomux_setting cam_settings[] = {
	{
		.func = GPIOMUX_FUNC_1, /*active 1*/ /* 0 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_1, /*suspend*/ /* 1 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},

	{
		.func = GPIOMUX_FUNC_1, /*i2c suspend*/ /* 2 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 0*/ /* 3 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend 0*/ /* 4 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend 0*/ /* 5 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_UP,
	},
};

static struct gpiomux_setting sd_card_det_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config sd_card_det __initdata = {
	.gpio = 62,
	.settings = {
		[GPIOMUX_ACTIVE]    = &sd_card_det_config,
		[GPIOMUX_SUSPENDED] = &sd_card_det_config,
	},
};

static struct msm_gpiomux_config msm_sensor_configs[] __initdata = {
	{
		.gpio = 15, /* CAM_MCLK0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
#ifndef CONFIG_MACH_ACER_A12
	{
		.gpio = 16, /* CAM_MCLK1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
#endif
	{
		.gpio = 17, /* CAM_MCLK2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 18, /* WEBCAM1_RESET_N / CAM_MCLK3 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 19, /* CCI_I2C_SDA0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 20, /* CCI_I2C_SCL0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 21, /* CCI_I2C_SDA1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 22, /* CCI_I2C_SCL1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
#ifndef CONFIG_MACH_ACER_A12
#if 0	//NFC_DL use 23 in A12
	{
		.gpio = 23, /* FLASH_LED_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
	{
		.gpio = 24, /* FLASH_LED_NOW */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 25, /* WEBCAM2_RESET_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 26, /* CAM_IRQ */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 27, /* OIS_SYNC */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
	{
		.gpio = 75, /* CAM2_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[5],
		},
	},
	{
		.gpio = 89, /* CAM1_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#ifndef CONFIG_MACH_ACER_A12
	{
		.gpio = 90, /* CAM1_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 91, /* CAM2_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 92, /* CAM2_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
#ifdef CONFIG_MACH_ACER_A12
	{
		.gpio = 94, /* ADP1660 ENABLE */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 95, /* ADP1660 TX */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 96, /* ADP1660 STROBE */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
#endif
};

#ifndef CONFIG_MACH_ACER_A12
static struct gpiomux_setting auxpcm_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};


static struct gpiomux_setting auxpcm_sus_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

/* Primary AUXPCM port sharing GPIO lines with Primary MI2S */
static struct msm_gpiomux_config msm8974_pri_pri_auxpcm_configs[] __initdata = {
#ifndef CONFIG_MACH_ACER_A12
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
#endif
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 67,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
#if 0	//NFC_RST use 68 in A12
	{
		.gpio = 68,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
#endif
};

/* Primary AUXPCM port sharing GPIO lines with Tertiary MI2S */
static struct msm_gpiomux_config msm8974_pri_ter_auxpcm_configs[] __initdata = {
	{
		.gpio = 74,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 75,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 76,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
};

static struct msm_gpiomux_config msm8974_sec_auxpcm_configs[] __initdata = {
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 81,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
};
#endif

static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	{
		.gpio = 36,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 37,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 38,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 39,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
};

static struct msm_gpiomux_config ath_gpio_configs[] = {
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ath_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &ath_gpio_suspend_cfg,
		},
	},
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ath_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &ath_gpio_suspend_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_taiko_config[] __initdata = {
	{
		.gpio	= 63,		/* SYS_RST_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &taiko_reset,
		},
	},
	{
		.gpio	= 72,		/* CDC_INT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &taiko_int,
		},
	},
};

#ifdef CONFIG_ACER_HEADSET
static struct msm_gpiomux_config a12_hs_configs[] __initdata = {
	{	/* HS INTERRUPT */
		.gpio = 46,
		.settings = {
			[GPIOMUX_ACTIVE]    = &a12_hs_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &a12_hs_int_sus_cfg,
		},
	},
};
#endif

#ifdef CONFIG_ACER_HEADSET_BUTT
static struct msm_gpiomux_config a12_hs_butt_configs[] __initdata = {
	{	/* HS BUTT INTERRUPT */
		.gpio = 54,
		.settings = {
			[GPIOMUX_ACTIVE]    = &a12_hs_butt_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &a12_hs_butt_int_sus_cfg,
		},
	},
};
#endif

#ifndef CONFIG_MACH_ACER_A12
static struct gpiomux_setting sdc3_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdc3_cmd_data_0_3_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdc3_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdc3_data_1_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm8974_sdc3_configs[] __initdata = {
	{
		/* DAT3 */
		.gpio      = 35,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* DAT2 */
		.gpio      = 36,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* DAT1 */
		.gpio      = 37,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_data_1_suspend_cfg,
		},
	},
	{
		/* DAT0 */
		.gpio      = 38,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* CMD */
		.gpio      = 39,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* CLK */
		.gpio      = 40,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
};

static void msm_gpiomux_sdc3_install(void)
{
	msm_gpiomux_install(msm8974_sdc3_configs,
			    ARRAY_SIZE(msm8974_sdc3_configs));
}

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct gpiomux_setting sdc4_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdc4_cmd_data_0_3_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdc4_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdc4_data_1_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm8974_sdc4_configs[] __initdata = {
#if 0
	{
		/* DAT3 */
		.gpio      = 92,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
#endif
	{
		/* DAT2 */
		.gpio      = 94,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
	{
		/* DAT1 */
		.gpio      = 95,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_data_1_suspend_cfg,
		},
	},
	{
		/* DAT0 */
		.gpio      = 96,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
#if 0
	{
		/* CMD */
		.gpio      = 91,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
#endif
	{
		/* CLK */
		.gpio      = 93,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
};

static void msm_gpiomux_sdc4_install(void)
{
	msm_gpiomux_install(msm8974_sdc4_configs,
			    ARRAY_SIZE(msm8974_sdc4_configs));
}
#else
static void msm_gpiomux_sdc4_install(void) {}
#endif /* CONFIG_MMC_MSM_SDC4_SUPPORT */
#endif

static struct msm_gpiomux_config apq8074_dragonboard_ts_config[] __initdata = {
	{
		/* BLSP1 QUP I2C_DATA */
		.gpio      = 2,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		/* BLSP1 QUP I2C_CLK */
		.gpio      = 3,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
};

static struct msm_gpiomux_config msm8974_nfc_configs[] __initdata = {
        {
                .gpio = 23,	//NFC_DL
                .settings = {
                        [GPIOMUX_ACTIVE]    = &nfc_active_cfg,
                        [GPIOMUX_SUSPENDED] = &nfc_suspend_cfg,
                },
        },
        {
                .gpio = 59,	//NFC_INT
                .settings = {
                        [GPIOMUX_ACTIVE]    = &nfc_int_active_cfg,
                        [GPIOMUX_SUSPENDED] = &nfc_int_suspend_cfg,
                },
        },
        {
                .gpio = 68,	//NFC_RST
                .settings = {
                        [GPIOMUX_ACTIVE]    = &nfc_active_cfg,
                        [GPIOMUX_SUSPENDED] = &nfc_suspend_cfg,
                },
        },
};

void __init msm_8974_init_gpiomux(void)
{
	int rc;

	rc = msm_gpiomux_init_dt();
	if (rc) {
		pr_err("%s failed %d\n", __func__, rc);
		return;
	}

	pr_info("%s:%d socinfo_get_version %x\n", __func__, __LINE__,
		socinfo_get_version());
	if (socinfo_get_version() >= 0x20000)
		msm_tlmm_misc_reg_write(TLMM_SPARE_REG, 0x5);

#if defined(CONFIG_MACH_ACER_A12)
	if (acer_board_id <= HW_ID_DVT1_2){
		pr_info("Use A12 DVT1 unused table");
		msm_gpiomux_install(a12_dvt1_gpio_unused_configs,
			ARRAY_SIZE(a12_dvt1_gpio_unused_configs));
	}
	else if (acer_board_id == HW_ID_DVT2){
		pr_info("Use A12 DVT2 unused table");
		msm_gpiomux_install(a12_dvt2_gpio_unused_configs,
		ARRAY_SIZE(a12_dvt2_gpio_unused_configs));
	}
	else if (acer_board_id == HW_ID_DVT3 || acer_board_id == HW_ID_DVT4){
		pr_info("Use A12 DVT3_4 unused table");
		msm_gpiomux_install(a12_dvt3_4_gpio_unused_configs,
		ARRAY_SIZE(a12_dvt3_4_gpio_unused_configs));
	}
	else if (acer_board_id >= HW_ID_PVT){
		pr_info("Use A12 PVT unused table");
		msm_gpiomux_install(a12_pvt_gpio_unused_configs,
		ARRAY_SIZE(a12_pvt_gpio_unused_configs));
	}
#endif

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	msm_gpiomux_install(msm_eth_configs, ARRAY_SIZE(msm_eth_configs));
#endif

#if defined(CONFIG_FB_MSM_MDSS_MIPI_DSI_SHARP) || defined(CONFIG_FB_MSM_MDSS_MIPI_DSI_INNOLUX)
	msm_gpiomux_install(msm_lcd_configs, ARRAY_SIZE(msm_lcd_configs));
#endif
	msm_gpiomux_install(msm_blsp_configs, ARRAY_SIZE(msm_blsp_configs));
#ifndef CONFIG_MACH_ACER_A12
	msm_gpiomux_install(msm_blsp2_uart7_configs,
			 ARRAY_SIZE(msm_blsp2_uart7_configs));
#endif
	msm_gpiomux_install(wcnss_5wire_interface,
				ARRAY_SIZE(wcnss_5wire_interface));
	if (of_board_is_liquid())
		msm_gpiomux_install_nowrite(ath_gpio_configs,
					ARRAY_SIZE(ath_gpio_configs));
	msm_gpiomux_install(msm8974_slimbus_config,
			ARRAY_SIZE(msm8974_slimbus_config));

	msm_gpiomux_install(msm_touch_configs, ARRAY_SIZE(msm_touch_configs));
		msm_gpiomux_install(hap_lvl_shft_config,
				ARRAY_SIZE(hap_lvl_shft_config));

	msm_gpiomux_install(msm_sensor_configs, ARRAY_SIZE(msm_sensor_configs));

	msm_gpiomux_install(&sd_card_det, 1);
#ifndef CONFIG_MACH_ACER_A12
	if (machine_is_apq8074() && (of_board_is_liquid() || \
	    of_board_is_dragonboard()))
		msm_gpiomux_sdc3_install();
	msm_gpiomux_sdc4_install();
#endif

	msm_gpiomux_install(msm_taiko_config, ARRAY_SIZE(msm_taiko_config));

	msm_gpiomux_install(msm8974_nfc_configs, ARRAY_SIZE(msm8974_nfc_configs));

#ifdef CONFIG_ACER_HEADSET
	msm_gpiomux_install(a12_hs_configs, ARRAY_SIZE(a12_hs_configs));
#endif

#ifdef CONFIG_ACER_HEADSET_BUTT
	msm_gpiomux_install(a12_hs_butt_configs, ARRAY_SIZE(a12_hs_butt_configs));
#endif

	msm_gpiomux_install(msm_hsic_configs, ARRAY_SIZE(msm_hsic_configs));
	msm_gpiomux_install(msm_hsic_hub_configs,
				ARRAY_SIZE(msm_hsic_hub_configs));

	msm_gpiomux_install(msm_hdmi_configs, ARRAY_SIZE(msm_hdmi_configs));
	if (of_board_is_fluid() && (acer_board_id < HW_ID_PVT))
		msm_gpiomux_install(msm_mhl_configs,
				    ARRAY_SIZE(msm_mhl_configs));

#ifndef CONFIG_MACH_ACER_A12
	if (of_board_is_liquid() ||
	    (of_board_is_dragonboard() && machine_is_apq8074()))
		msm_gpiomux_install(msm8974_pri_ter_auxpcm_configs,
				 ARRAY_SIZE(msm8974_pri_ter_auxpcm_configs));
	else
		msm_gpiomux_install(msm8974_pri_pri_auxpcm_configs,
				 ARRAY_SIZE(msm8974_pri_pri_auxpcm_configs));

	/* The gpio configure will affect touch spi function, so remove the config */
	if (0)
		msm_gpiomux_install(msm8974_sec_auxpcm_configs,
				 ARRAY_SIZE(msm8974_sec_auxpcm_configs));
#endif

	msm_gpiomux_install_nowrite(msm_lcd_configs,
			ARRAY_SIZE(msm_lcd_configs));

	if (of_board_is_rumi())
		msm_gpiomux_install(msm_rumi_blsp_configs,
				    ARRAY_SIZE(msm_rumi_blsp_configs));

	/* The gpio configure will affect touch spi function, so remove the config */
	if (0 && socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_MDM)
		msm_gpiomux_install(mdm_configs,
			ARRAY_SIZE(mdm_configs));

	if (of_board_is_dragonboard() && machine_is_apq8074())
		msm_gpiomux_install(apq8074_dragonboard_ts_config,
			ARRAY_SIZE(apq8074_dragonboard_ts_config));
#ifdef CONFIG_MACH_ACER_A12
	msm_gpiomux_install(msm_capsense_configs, ARRAY_SIZE(msm_capsense_configs));
	msm_gpiomux_install(vib_en_configs, ARRAY_SIZE(vib_en_configs));
	msm_gpiomux_install(msm_hall_configs, ARRAY_SIZE(msm_hall_configs));
#endif
}
