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
 */
#ifndef __ARCH_ARM_MACH_MSM_GPIO_COMMON_H
#define __ARCH_ARM_MACH_MSM_GPIO_COMMON_H

extern int msm_show_resume_irq_mask;

#ifdef CONFIG_ARCH_ACER_MSM8974
/* Bits of interest in the GPIO_IN_OUT register.
 */
enum {
	GPIO_IN_BIT  = 0,
	GPIO_OUT_BIT = 1
};

/* Bits of interest in the GPIO_INTR_STATUS register.
 */
enum {
	INTR_STATUS_BIT = 0,
};

/* Bits of interest in the GPIO_CFG register.
 */
enum {
	GPIO_PULL_BIT = 0,
	FUNC_SEL_BIT = 2,
	DRV_STRENGTH_BIT = 6,
	GPIO_OE_BIT = 9,
};

/* Bits of interest in the GPIO_INTR_CFG register.
 */
enum {
	INTR_ENABLE_BIT        = 0,
	INTR_POL_CTL_BIT       = 1,
	INTR_DECT_CTL_BIT      = 2,
	INTR_RAW_STATUS_EN_BIT = 4,
	INTR_TARGET_PROC_BIT   = 5,
	INTR_DIR_CONN_EN_BIT   = 8,
};

/*
 * There is no 'DC_POLARITY_LO' because the GIC is incapable
 * of asserting on falling edge or level-low conditions.  Even though
 * the registers allow for low-polarity inputs, the case can never arise.
 */
enum {
	DC_GPIO_SEL_BIT = 0,
	DC_POLARITY_BIT	= 8,
};

#define INTR_RAW_STATUS_EN BIT(INTR_RAW_STATUS_EN_BIT)
#define INTR_ENABLE        BIT(INTR_ENABLE_BIT)
#define INTR_POL_CTL_HI    BIT(INTR_POL_CTL_BIT)
#define INTR_DIR_CONN_EN   BIT(INTR_DIR_CONN_EN_BIT)
#define DC_POLARITY_HI     BIT(DC_POLARITY_BIT)

#define INTR_TARGET_PROC_APPS    (4 << INTR_TARGET_PROC_BIT)
#define INTR_TARGET_PROC_NONE    (7 << INTR_TARGET_PROC_BIT)

#define INTR_DECT_CTL_LEVEL      (0 << INTR_DECT_CTL_BIT)
#define INTR_DECT_CTL_POS_EDGE   (1 << INTR_DECT_CTL_BIT)
#define INTR_DECT_CTL_NEG_EDGE   (2 << INTR_DECT_CTL_BIT)
#define INTR_DECT_CTL_DUAL_EDGE  (3 << INTR_DECT_CTL_BIT)
#define INTR_DECT_CTL_MASK       (3 << INTR_DECT_CTL_BIT)

#define GPIO_CONFIG(gpio)        (MSM_TLMM_BASE + 0x1000 + (0x10 * (gpio)))
#define GPIO_IN_OUT(gpio)        (MSM_TLMM_BASE + 0x1004 + (0x10 * (gpio)))
#define GPIO_INTR_CFG(gpio)      (MSM_TLMM_BASE + 0x1008 + (0x10 * (gpio)))
#define GPIO_INTR_STATUS(gpio)   (MSM_TLMM_BASE + 0x100c + (0x10 * (gpio)))
#define GPIO_DIR_CONN_INTR(intr) (MSM_TLMM_BASE + 0x2800 + (0x04 * (intr)))
#endif

unsigned __msm_gpio_get_inout(unsigned gpio);
void __msm_gpio_set_inout(unsigned gpio, unsigned val);
void __msm_gpio_set_config_direction(unsigned gpio, int input, int val);
void __msm_gpio_set_polarity(unsigned gpio, unsigned val);
unsigned __msm_gpio_get_intr_status(unsigned gpio);
void __msm_gpio_set_intr_status(unsigned gpio);
unsigned __msm_gpio_get_intr_config(unsigned gpio);
unsigned __msm_gpio_get_intr_cfg_enable(unsigned gpio);
void __msm_gpio_set_intr_cfg_enable(unsigned gpio, unsigned val);
void __msm_gpio_set_intr_cfg_type(unsigned gpio, unsigned type);
void __gpio_tlmm_config(unsigned config);
void __msm_gpio_install_direct_irq(unsigned gpio, unsigned irq,
					unsigned int input_polarity);
#endif
