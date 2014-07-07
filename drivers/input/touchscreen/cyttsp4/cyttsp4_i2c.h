/*
 * cyttsp4_i2c.h
 * Cypress TrueTouch(TM) Standard Product V4 I2C driver module.
 * For use with Cypress Txx4xx parts.
 * Supported parts include:
 * TMA4XX
 * TMA1036
 *
 * Copyright (C) 2012 Cypress Semiconductor
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#ifndef _LINUX_CYTTSP4_I2C_H
#define _LINUX_CYTTSP4_I2C_H

#define CYTTSP4_I2C_NAME "cypress-ts"

#define CYTTSP4_I2C_TCH_ADR 			 0x24
#define A12_GPIO_CYP_TS_INT_N_EVB  	     61
#define A12_GPIO_CYP_TS_INT_N_EVT        77
#define A12_GPIO_CYP_TS_RESET_N          60

#define VERG_L29_VTG_MIN_UV        1800000
#define VERG_L29_VTG_MAX_UV        1800000
#define VERG_L29_CURR_24HZ_UA      17500
#define VERG_LVS4_VTG_MIN_UV       1800000
#define VERG_LVS4_VTG_MAX_UV       1800000
#define VERG_LVS4_CURR_UA          9630


#define VERG_L18_VTG_MIN_UV        2850000
#define VERG_L18_VTG_MAX_UV        2850000
#define VERG_L18_CURR_24HZ_UA      17500
#define VERG_LVS1_VTG_MIN_UV       1800000
#define VERG_LVS1_VTG_MAX_UV       1800000
#define VERG_LVS1_CURR_UA          9630


struct cypress_ts_regulator {
	const char *name;
	u32	min_uV;
	u32	max_uV;
	u32	load_uA;
};


#endif /* _LINUX_CYTTSP4_I2C_H */
