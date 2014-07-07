/*
 * Acer Headset button detection driver.
 *
 * Copyright (C) 2012 Acer Corporation.
 *
 * Authors:
 *    Eric Cheng <Eric.Cheng@acer.com>
 */
#ifndef __ACER_HEADSET_BUTT_H
#define __ACER_HEADSET_BUTT_H

extern int acer_hs_butt_init(struct platform_device *pdev);
extern int acer_hs_butt_remove(void);
extern int get_mpp_hs_mic_adc(int32_t *val);
extern void set_hs_state(bool state);

#endif
