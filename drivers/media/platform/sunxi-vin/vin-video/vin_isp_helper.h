
/*
 ******************************************************************************
 *
 * vin_isp_helper.h
 *
 * Hawkview ISP - vin_isp_helper.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/11/30	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef _VIM_ISP_HELPER_H_
#define _VIM_ISP_HELPER_H_

#include "vin_core.h"
#define IS_FLAG(x, y) (((x)&(y)) == y)

static inline int vin_is_generating(struct vin_vid_cap *cap)
{
	int ret;
	unsigned long flags = 0;
	spin_lock_irqsave(&cap->slock, flags);
	ret = test_bit(0, &cap->generating);
	spin_unlock_irqrestore(&cap->slock, flags);
	return ret;
}

static inline void vin_start_generating(struct vin_vid_cap *cap)
{
	unsigned long flags = 0;
	spin_lock_irqsave(&cap->slock, flags);
	set_bit(0, &cap->generating);
	spin_unlock_irqrestore(&cap->slock, flags);
}

static inline void vin_stop_generating(struct vin_vid_cap *cap)
{
	unsigned long flags = 0;
	spin_lock_irqsave(&cap->slock, flags);
	cap->first_flag = 0;
	clear_bit(0, &cap->generating);
	spin_unlock_irqrestore(&cap->slock, flags);
}

int vin_is_opened(struct vin_vid_cap *cap);
void vin_start_opened(struct vin_vid_cap *cap);
void vin_stop_opened(struct vin_vid_cap *cap);
int vin_set_addr(struct vin_core *vinc, struct vb2_buffer *vb,
		      struct vin_frame *frame, struct vin_addr *paddr);
int vin_set_sensor_power_on(struct vin_core *vinc);
int vin_set_sensor_power_off(struct vin_core *vinc);

void vin_gpio_release(struct vin_core *vinc);

#endif /*_VIM_ISP_HELPER_H_*/
