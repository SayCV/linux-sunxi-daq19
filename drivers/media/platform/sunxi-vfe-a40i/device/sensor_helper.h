/*
 * sensor helper header file
 * 
 */
#ifndef __SENSOR__HELPER__H__
#define __SENSOR__HELPER__H__

#include <media/v4l2-subdev.h>
#include <linux/videodev2.h>
#include "../vfe.h"
#include "../vfe_subdev.h"
#include "../csi_cci/cci_helper.h"
#include "camera_cfg.h"
#include "../platform_cfg.h"

#define REG_DLY  0xffff

struct regval_list {
	addr_type addr;
	data_type data;
};

extern int sensor_read(struct v4l2_subdev *sd, addr_type addr,  data_type *value);
extern int sensor_write(struct v4l2_subdev *sd, addr_type addr, data_type value);
extern int sensor_write_array(struct v4l2_subdev *sd, struct regval_list *regs, int array_size);
//LANDSEM@yingxianFei add to read or write passby
extern int sensor_read_passby(struct v4l2_subdev *sd, unsigned char slave,addr_type reg, data_type *value);
extern int sensor_write_array_passby(struct v4l2_subdev *sd,unsigned char slave, struct regval_list *regs, int array_size);
extern int sensor_write_passby(struct v4l2_subdev *sd, unsigned char slave,addr_type reg, data_type value);
//LANDSEM@yingxianFei end add.

//extern int sensor_power(struct v4l2_subdev *sd, int on);

#endif //__SENSOR__HELPER__H__
