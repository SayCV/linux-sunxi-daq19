
/*
 ******************************************************************************
 *
 * sensor_helper.h
 *
 * Hawkview ISP - sensor_helper.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/12/02	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef __SENSOR__HELPER__H__
#define __SENSOR__HELPER__H__

#include <media/v4l2-subdev.h>
#include <linux/videodev2.h>
#include "../../vin-video/vin_core.h"
#include "../../utility/vin_supply.h"
#include "../../vin-cci/cci_helper.h"
#include "camera_cfg.h"
#include "../../platform/platform_cfg.h"

#define REG_DLY  0xffff

struct regval_list {
	addr_type addr;
	data_type data;
};

extern int sensor_read(struct v4l2_subdev *sd, addr_type addr,
		       data_type *value);
extern int sensor_write(struct v4l2_subdev *sd, addr_type addr,
			data_type value);
extern int sensor_write_array(struct v4l2_subdev *sd,
				struct regval_list *regs,
				int array_size);
#endif
