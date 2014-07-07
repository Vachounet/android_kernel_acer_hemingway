/*
 * Acer Headset detection driver
 *
 * Copyright (C) 2012 Acer Corporation.
 *
 * Authors:
 *    Eric Cheng <Eric.Cheng@acer.com>
 */

#ifndef __ACER_HEADSET_H
#define __ACER_HEADSET_H

#include <linux/platform_device.h>
#include <linux/qpnp/qpnp-adc.h>
#include <sound/soc.h>

#define GPIO_HS_DET							46
#define GPIO_HS_BUTT						54
#define HEADSET_MIC_BIAS_WORK_DELAY_TIME	100

extern int acer_hs_remove(void);
extern void start_button_irq(void);
extern void stop_button_irq(void);
extern void acer_hs_detect_work(void);
extern int acer_hs_init(struct platform_device *pdev);
extern void acer_headset_jack_init(struct snd_soc_codec *codec);
#endif
