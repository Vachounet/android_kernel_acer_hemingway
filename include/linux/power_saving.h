#ifndef __POWER_SAVING_H__
#define __POWER_SAVING_H__

/*
 * if CABC_DEBUG is enabled,
 *  you can control each level of cabc
 */
//#define CABC_DEBUG

#ifdef CONFIG_MACH_ACER_A12
#define ECO_ENABLE_MODE	2  //still image mode
#define ECO_DISABLE_MODE	1 //UI mode

#define GPU_LIMIT_MODE	'2'//power level 0~4: 450MHz, 300Mhz, 300Mhz, 200Mhz, 200Mhz
#define GPU_NORMAL_MODE	'0'
#else
#define ECO_ENABLE_MODE	0
#define ECO_DISABLE_MODE	0

#define GPU_LIMIT_MODE	'0' //maximum power level
#define GPU_NORMAL_MODE	'0'
#endif

#endif
