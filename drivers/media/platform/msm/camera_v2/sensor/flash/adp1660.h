/* Copyright (C) 2010, Acer Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <mach/camera.h>

int flash_set_state(int state);
int adp1660_information(void);
int32_t adp1660_flash_mode_control(int ctrl);
int32_t adp1660_torch_mode_control(int ctrl);

#define ADP1660_REG_OUTPUT_MODE                      0x01
#define ADP1660_REG_ADDITIONAL_MODE                  0x03
#define ADP1660_REG_BATTERY_LOW_MODE4                0x04
#define ADP1660_REG_BATTERY_LOW_MODE5                0x05
#define ADP1660_REG_LED1_FLASH_CURRENT_SET           0x06
#define ADP1660_REG_LED1_TORCH_ASSIST_SET            0x08
#define ADP1660_REG_LED2_FLASH_CURRENT_SET           0x09
#define ADP1660_REG_LED2_TORCH_ASSIST_SET            0x0B
#define ADP1660_REG_LED_Enable_Mode                  0x0F

/* Current Set Register (Register 0x06) */
/* I_FL */
#define ADP1660_I_FL_300mA  0x18
#define ADP1660_I_FL_350mA  0x1C
#define ADP1660_I_FL_500mA  0x28
#define ADP1660_I_FL_750mA  0x3F

/* I_TOR */
#define ADP1660_I_TOR_100mA 0x08
#define ADP1660_I_TOR_150mA 0x0c
#define ADP1660_I_TOR_200mA 0x10

/* Output Mode Register (Register 0x01) */
/* Register Shifts */
#define ADP1660_IL_PEAK_SHIFT            6
#define ADP1660_STR_LV_SHIFT             5
#define ADP1660_STR_MODE_SHIFT           4
#define ADP1660_STR_POL_SHIFT            3

/* IL_PEAK */
#define ADP1660_IL_PEAK_2P25A    0x00
#define ADP1660_IL_PEAK_2P75A    0x01
#define ADP1660_IL_PEAK_3P25A    0x02  /* default */
#define ADP1660_IL_PEAK_3P5A     0x03
/* STR_LV */
#define ADP1660_STR_LV_EDGE_SENSITIVE      0x00
#define ADP1660_STR_LV_LEVEL_SENSITIVE     0x01  /* defualt */
/* STR_MODE */
#define ADP1660_STR_MODE_SW  0x00
#define ADP1660_STR_MODE_HW  0x01  /* defualt */
/* STR_POL */
#define ADP1660_STR_ACTIVE_LOW   0x00
#define ADP1660_STR_ACTIVE_HIGH  0x01  /* defualt */
/* LED_MODE */
#define ADP1660_LED_MODE_STANDBY  0x00  /* defualt */
#define ADP1660_LED_MODE_VOUT     0x01  // VOUT=5V
#define ADP1660_LED_MODE_ASSIST   0x02
#define ADP1660_LED_MODE_FLASH    0x03

/* Additional Mode Register (Register 0x04) */
/* Register Shifts */
#define ADP1660_V_BATT_WINDOW_SHIFT  3
/*V_BATT_WINDOW*/
#define ADP1660_V_BATT_WINDOW_DISABLE   0x00  /* default */
#define ADP1660_V_BATT_WINDOW_1ms       0x01
#define ADP1660_V_BATT_WINDOW_2ms       0x02
#define ADP1660_V_BATT_WINDOW_3ms       0x03
/*V_VB_LO*/
#define ADP1660_V_VB_LO_DISABLED        0x00  /* default */
#define ADP1660_V_VB_LO_3P3V            0x01
#define ADP1660_V_VB_LO_3P35V           0x02
#define ADP1660_V_VB_LO_3P4V            0x03
#define ADP1660_V_VB_LO_3P45V           0x04
#define ADP1660_V_VB_LO_3P5V            0x05
#define ADP1660_V_VB_LO_3P55V           0x06
#define ADP1660_V_VB_LO_3P6V            0x07


/* Battery Low Mode Register (Register 0x05) */
/*I_VB_LO*/
#define ADP1660_I_VB_LO_300mA     0x18
#define ADP1660_I_VB_LO_500mA     0x28 /* default */
#define ADP1660_I_VB_LO_750mA     0x3F

/* LED Enable Mode Register (Register 0x0F) */
/* Register Shifts */
#define ADP1660_LED2_EN_SHIFT  1
/*LED1_EN and LED2_EN */
#define ADP1660_LED1_EN_Disable        0  /* default */
#define ADP1660_LED1_EN_Enable         1
#define ADP1660_LED2_EN_Disable        0  /* default */
#define ADP1660_LED2_EN_Enable         1
#define ADP1660_LED_ALL_EN_Enable      3
#define ADP1660_LED_ALL_EN_Disable     0
