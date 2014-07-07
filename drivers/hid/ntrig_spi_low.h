/*
 * drivers/hid/ntrig_spi_low.h
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

#ifndef _NTRIG_SPI_LOW_H
#define _NTRIG_SPI_LOW_H

#include <linux/spi/ntrig_spi.h>

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

//#define CONFIG_CONTROL_LVS1 1 //Control tag for LVS1 power conftrol.
								//Currently the LVS1 is controlled at SBL1 stage.

typedef int (*ntrig_spi_suspend_callback)(struct spi_device *dev,
	pm_message_t mesg);
typedef int (*ntrig_spi_resume_callback)(struct spi_device *dev);
void ntrig_spi_register_pwr_mgmt_callbacks(
	ntrig_spi_suspend_callback s, ntrig_spi_resume_callback r);

struct spi_device_data {
	struct spi_device *m_spi_device;
	struct ntrig_spi_platform_data m_platform_data;
	struct regulator *m_reg_ts_spi;
#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
};
struct spi_device_data *get_ntrig_spi_device_data(void);

struct ntrig_spi_ts_regulator {
	const char *name;
	u32     min_uV;
	u32     max_uV;
	u32     load_uA;
};


#endif /* #ifndef _NTRIG_SPI_LOW_H */
