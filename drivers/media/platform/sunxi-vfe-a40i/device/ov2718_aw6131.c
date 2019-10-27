/* **************************************************************************************
 *ov2710_aw6131.c
 *A V4L2 driver for OV2710_AW6131 cameras
 *Copyright (c) 2014 by Allwinnertech Co., Ltd.http://www.allwinnertech.com
 *	Version		Author		Date				Description
 *	1.0			liu baihao	2016/1/15		OV2710_AW6131 YUV sensor Support(liubaihao@sina.com)
 ****************************************************************************************
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>
#include "camera.h"
#include "sensor_helper.h"
//#include "ov2718_aw6131.h"
#include "vfe_sub_device.h"

MODULE_AUTHOR("yingxianFei");
MODULE_DESCRIPTION("A low-level driver for OV2718_AW6131  sensors");
MODULE_LICENSE("GPL");

/*for internel driver debug*/
#define DEV_DBG_EN      (1)
#if DEV_DBG_EN
#define vfe_dev_dbg(x, arg...)	printk("[LANDSEM][AW6131]"x,	##arg)
#else
#define vfe_dev_dbg(x, arg...)
#endif
#define vfe_dev_err(x, arg...)	printk("[LANDSEM][AW6131]"x,	##arg)
#define vfe_dev_print(x, arg...)	printk("[LANDSEM][AW6131 ]"x,	##arg)
#define LOG_ERR_RET(x)  { \
							int ret;  \
							ret = x; \
							if (ret < 0) {\
								vfe_dev_err("error at %s\n", __func__);  \
								return ret; \
							} \
						}

/*define module timing*/
#define MCLK              (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR  0x6131


#define PIXL_AVAILABLE  (1)
#define PIXL_DISABLE    (0)

#define PIXL_1080P      PIXL_AVAILABLE
#define PIXL_720P       PIXL_AVAILABLE
#define PIXL_480P       PIXL_AVAILABLE

#define DARK_SWITCH_THRESHOLD      (0x5F)
#define BRIGHT_SWITCH_THRESHOLD    (0x1A)
/*****LANDSEM@liuxueneng 20160622 add for ov2718 start*****/
#define DARK_SWITCH_THRESHOLD_2      (0xFF)
#define BRIGHT_SWITCH_THRESHOLD_2    (0x10)
/*****LANDSEM@liuxueneng 20160622 add for ov2718 start*****/



/*
 * Our nominal (default) frame rate.
 */
#ifdef FPGA
#define SENSOR_FRAME_RATE	15
#else
#define SENSOR_FRAME_RATE	25
#endif


/*static struct delayed_work sensor_s_ae_ratio_work;*/
static struct v4l2_subdev *glb_sd;
#define SENSOR_NAME "ov2718_aw6131"

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;	/* coming later */

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}

static struct regval_list isp_bypass_on[] = {	
	{0xfffd,0x80},
	{0xfffe,0x80},
	{0x004d,0x01},
};

static struct regval_list isp_bypass_off[] = {	
	{0xfffd,0x80},
	{0xfffe,0x80},
	{0x004d,0x00},
};

/*
 * The default register settings
 *
 */
//static struct regval_list *isp_default_regs = sensor_normal_default_regs;
/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
//default regs.
static unsigned char dark_threshold_val = DARK_SWITCH_THRESHOLD;
static unsigned char bright_threshold_val = BRIGHT_SWITCH_THRESHOLD;
/*****LANDSEM@liuxueneng 20160622 add for ov2718 start***/
static unsigned char dark_threshold_val_2 = DARK_SWITCH_THRESHOLD_2;
static unsigned char bright_threshold_val_2 = BRIGHT_SWITCH_THRESHOLD_2;
/*****LANDSEM@liuxueneng 20160622 add for ov2718 end***/

static unsigned char sensor_slave = 0xff;
static struct cfg_array *p_sensor_chip_id = NULL;
static struct cfg_array *p_isp_default_regs = NULL;
static struct cfg_array *p_sensor_default_regs = NULL;
static struct cfg_array *p_isp_dark_regs = NULL;
static struct cfg_array *p_sensor_dark_regs = NULL;
static struct cfg_array *p_isp_bright_regs = NULL;
static struct cfg_array *p_sensor_bright_regs = NULL;
static struct cfg_array *p_isp_0flip_regs = NULL;
static struct cfg_array *p_isp_180flip_regs = NULL;	
static struct cfg_array *p_sensor_0flip_regs = NULL;
static struct cfg_array *p_sensor_180flip_regs = NULL;	


static struct sensor_win_size isp_win_sizes[] = {
/* Full HD 1920 *1080 25fps*/
	{
		 .width = HD1080_WIDTH,
		 .height = HD1080_HEIGHT,
		 .hoffset = 0,
		 .voffset = 0,
		 .regs = NULL,
		 .regs_size = 0,
		 .set_size = NULL,
	 },
     {
	     .width = HD720_WIDTH,
	     .height = HD720_HEIGHT,
	     .hoffset = 0,
	     .voffset = 0,
		 .regs = NULL,
		 .regs_size = 0,	     
	     .set_size = NULL,
	 },
     {
		 .width = VGA_WIDTH,
		 .height = VGA_HEIGHT,
		 .hoffset = 0,
		 .voffset = 0,
		 .regs = NULL,
		 .regs_size = 0,	      
		 .set_size = NULL,
	 },
};

#define N_ISP_SIZES (ARRAY_SIZE(isp_win_sizes))

static struct sensor_win_size sensor_win_sizes[] = {
/* Full HD 1920 *1080 25fps*/
	{
		 .width = HD1080_WIDTH,
		 .height = HD1080_HEIGHT,
		 .hoffset = 0,
		 .voffset = 0,
		 .regs = NULL,
		 .regs_size = 0,
		 .set_size = NULL,
	 },
     {
	     .width = HD720_WIDTH,
	     .height = HD720_HEIGHT,
	     .hoffset = 0,
	     .voffset = 0,
		 .regs = NULL,
		 .regs_size = 0,	     
	     .set_size = NULL,
	 },
     {
		 .width = VGA_WIDTH,
		 .height = VGA_HEIGHT,
		 .hoffset = 0,
		 .voffset = 0,
		 .regs = NULL,
		 .regs_size = 0,	      
		 .set_size = NULL,
	 },
};

#define N_SENSOR_SIZES  (ARRAY_SIZE(sensor_win_sizes))

static inline int CHECK_REGS(struct cfg_array *cfg)  {
	if((NULL != cfg) && (NULL != cfg->regs))  {
		return 0;
	}
	return -1;
}

static DECLARE_RWSEM(rwsem);
static int register_vfe_first = 0;
static int register_vfe_flag = 0;

int register_aw6131_ov2718_regs(struct ls_vfe_sub_dev *sub_dev)  {
	if(NULL == glb_sd)  {
		vfe_dev_err("No device has been found!\n");
	    return -ENODEV;
	}
    if(NULL == sub_dev)  {
		vfe_dev_err("Input data is null!\n");
	    return -ENODATA;
    }
	//down sem.
	down_write(&rwsem);		
	dark_threshold_val = sub_dev->dark_threshold_value;
	bright_threshold_val = sub_dev->bright_threshold_value;
	dark_threshold_val_2 = sub_dev->dark_threshold_value_2;
	bright_threshold_val_2 = sub_dev->bright_threshold_value_2;
	sensor_slave = sub_dev->sensor_slave;
	if(!CHECK_REGS(sub_dev->sensor_chip_id_regs))  {
		p_sensor_chip_id= sub_dev->sensor_chip_id_regs;
	}	
	if(!CHECK_REGS(sub_dev->isp_default_regs))  {
		p_isp_default_regs = sub_dev->isp_default_regs;
	}
	if(!CHECK_REGS(sub_dev->sensor_default_regs))  {
		p_sensor_default_regs = sub_dev->sensor_default_regs;
	}
	if(!CHECK_REGS(sub_dev->isp_dark_regs))  {
		p_isp_dark_regs = sub_dev->isp_dark_regs;
	}
	if(!CHECK_REGS(sub_dev->sensor_dark_regs))  {
		p_sensor_dark_regs = sub_dev->sensor_dark_regs;
	}
	if(!CHECK_REGS(sub_dev->isp_bright_regs))  {
		p_isp_bright_regs = sub_dev->isp_bright_regs;
	}
	if(!CHECK_REGS(sub_dev->sensor_bright_regs))  {
		p_sensor_bright_regs = sub_dev->sensor_bright_regs;
	}	
	if(!CHECK_REGS(sub_dev->isp_0flip_regs))  {
		p_isp_0flip_regs = sub_dev->isp_0flip_regs;
	}
	if(!CHECK_REGS(sub_dev->isp_180flip_regs))  {
		p_isp_180flip_regs = sub_dev->isp_180flip_regs;
	}
	if(!CHECK_REGS(sub_dev->sensor_0flip_regs))  {
		p_sensor_0flip_regs = sub_dev->sensor_0flip_regs;
	}
	if(!CHECK_REGS(sub_dev->sensor_180flip_regs))  {
		p_sensor_180flip_regs = sub_dev->sensor_180flip_regs;
	}	
	//isp's regs update.
	if(!CHECK_REGS(sub_dev->isp_1080p_regs))  {
		isp_win_sizes[0].regs = sub_dev->isp_1080p_regs->regs;
	    isp_win_sizes[0].regs_size = sub_dev->isp_1080p_regs->size;
	}
	if(!CHECK_REGS(sub_dev->isp_720p_regs))  {
		isp_win_sizes[1].regs = sub_dev->isp_720p_regs->regs;
	    isp_win_sizes[1].regs_size = sub_dev->isp_720p_regs->size;
	}
	if(!CHECK_REGS(sub_dev->isp_480p_regs))  {
		isp_win_sizes[2].regs = sub_dev->isp_480p_regs->regs;
	    isp_win_sizes[2].regs_size = sub_dev->isp_480p_regs->size;
	}	
	//sensor's regs update.
	if(!CHECK_REGS(sub_dev->sensor_1080p_regs))  {
		sensor_win_sizes[0].regs = sub_dev->sensor_1080p_regs->regs;
	    sensor_win_sizes[0].regs_size = sub_dev->sensor_1080p_regs->size;
	}
	if(!CHECK_REGS(sub_dev->sensor_720p_regs))  {
		sensor_win_sizes[1].regs = sub_dev->sensor_720p_regs->regs;
	    sensor_win_sizes[1].regs_size = sub_dev->sensor_720p_regs->size;
	}
	if(!CHECK_REGS(sub_dev->sensor_480p_regs))  {
		sensor_win_sizes[2].regs = sub_dev->sensor_480p_regs->regs;
	    sensor_win_sizes[2].regs_size = sub_dev->sensor_480p_regs->size;
	}	
	//update flag.
	register_vfe_flag = 1;
	register_vfe_first = 1;
	//update sem.
    up_write(&rwsem);	
	
   return 0;
}
EXPORT_SYMBOL(register_aw6131_ov2718_regs);

int unregister_aw6131_ov2718_regs(void)  {
	down_write(&rwsem); 
	register_vfe_flag = 0;
	up_write(&rwsem);
	
	return 0;
}
EXPORT_SYMBOL(unregister_aw6131_ov2718_regs);
//---------end-------------
/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 */

static struct regval_list sensor_fmt_yuv422_yuyv[] = {

};

static struct regval_list sensor_fmt_yuv422_yvyu[] = {

};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {

};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {

};

static struct regval_list sensor_fmt_raw[] = {

};

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	vfe_dev_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

/*
static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	struct sensor_info *info = to_state(sd);

	info->exp = exp_val;
	return 0;
}
*/

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->gain;
	vfe_dev_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}

/*
static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);

	info->gain = gain_val;

	return 0;

}
*/

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret = 0;
	return ret;
}

//--------------------------------------add to switch model-------------------
#define TEST_COLOR_BAR    (0)
#define GET_CPU_INFO      (0)
#define GET_GAN_INFO      (0)

static int sensor_s_internal_scene(struct v4l2_subdev *sd)  {
    int ret = 0;
    data_type val = 0;
    static int night_mode = 0;

	LOG_ERR_RET(sensor_write(sd,0xfffd, 0x80));
	LOG_ERR_RET(sensor_write(sd,0xfffe, 0x14));
#if GET_GAN_INFO//get picture brightness.
	if(!sensor_read(sd,0x003c, &val))  {//\B5\B1ǰ\BB\AD\C3\E6\C1\C1\B6\C8ֵ,Խ\B0\B5ԽС		 //7,12,15,open light is 5c,day is 64,
		vfe_dev_dbg("currect 0x003c value of val is %x\n",val);
	}
#endif
	LOG_ERR_RET(sensor_read(sd,0x002b, &val));//\B5\B1ǰ\D4\F6\D2棬Խ\B0\B5Խ\B4\F3\A1\A3\D4\EB\B5\E3\D4\F6\BC\D3---------------!!!!!!!do not modify code location at will.max bf
	vfe_dev_dbg("currect 0x002b value of val is %x, dark_val = %d night_mode  = %d\n",val, dark_threshold_val, night_mode); 
	vfe_dev_dbg("bright_val = %d  bright_val_2 = %d night_mode  = %d\n",bright_threshold_val, bright_threshold_val_2, night_mode); 
/*LANDSEM@liuxueneng 20160621 modify for 0v2718 start */
	//if ((val > dark_threshold_val) && (night_mode != 1))	{
	if ((val >= dark_threshold_val) && (night_mode != 1)){
/*LANDSEM@liuxueneng 20160621 modify for 0v2718 start */
		vfe_dev_dbg("%s switch to night model.0x002b is %x\n",__func__,val);
		//isp darkness regs
		ret = sensor_write_array(sd, p_isp_dark_regs->regs, p_isp_dark_regs->size);
		if (ret < 0) {
			vfe_dev_err("write isp_darkness_regs error\n");
			return ret;
		}	
		//sensor darkness regs
		if((NULL != p_sensor_dark_regs) && (0 < p_sensor_dark_regs->size))  {
			//enable passby.
			vfe_dev_dbg("Write isp passby on regs.\n");
			LOG_ERR_RET(sensor_write_array(sd, isp_bypass_on, ARRAY_SIZE(isp_bypass_on)));
			msleep(10);			
			vfe_dev_dbg("Write sensor dark regs.\n");
			ret = sensor_write_array_passby(sd,sensor_slave, p_sensor_dark_regs->regs, p_sensor_dark_regs->size);
			if(ret < 0)  {
				vfe_dev_err("Passby write sensor dark regs error!\n");
				goto passby_err;
			}
			msleep(10);			
			//disenable passby
			vfe_dev_dbg("Write isp passby off regs.\n");
			LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));	
		}				
#if TEST_COLOR_BAR// color bar test
		LOG_ERR_RET(sensor_write(sd,0xfffe,0x80));
		LOG_ERR_RET(sensor_write(sd,0x0090,0x2b));	
#endif
		night_mode = 1;
	}
/*LANDSEM@liuxueneng 20160621 modify for 0v2718 start */
	//else  if ((val < bright_threshold_val) && (night_mode != 0))			{
	else if ((val >= bright_threshold_val_2 && val <= bright_threshold_val) && (night_mode != 0)){
/*LANDSEM@liuxueneng 20160621 modify for 0v2718 end */
		vfe_dev_dbg("%s switch to day model.0x002b is %x\n",__func__,val);
		//isp brightness regs
		ret = sensor_write_array(sd, p_isp_bright_regs->regs, p_isp_bright_regs->size);
		if (ret < 0) {
			vfe_dev_err("write isp_brightness_regs error\n");
			return ret;
		}	
		//sensor brightness regs
		if((NULL != p_sensor_bright_regs) && (0 < p_sensor_bright_regs->size))  {
			//enable passby.
			vfe_dev_dbg("Write isp passby on regs.\n");
		  	LOG_ERR_RET(sensor_write_array(sd, isp_bypass_on, ARRAY_SIZE(isp_bypass_on)));
			msleep(10);			
			vfe_dev_dbg("Write sensor bright regs.\n");
			ret = sensor_write_array_passby(sd,sensor_slave, p_sensor_bright_regs->regs, p_sensor_bright_regs->size);
			if(ret < 0)  {
				vfe_dev_err("Passby write sensor bright regs error!\n");
				goto passby_err;
			}
			msleep(10);				
			//disenable passby
			vfe_dev_dbg("Write isp passby off regs.\n");
		 	LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));					
		}
#if TEST_COLOR_BAR  // color bar test
		LOG_ERR_RET(sensor_write(sd,0xfffe,0x80));
		LOG_ERR_RET(sensor_write(sd,0x0090,0x2b));	
#endif
		night_mode = 0;
	}

	return 0;

passby_err:
	LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));//passby off	
	return -ENODEV;
}


static int sensor_s_scene(struct v4l2_subdev *sd)
{
#if  0	
	LOG_ERR_RET(sensor_write(sd,0xfffe, 0x80));//
	if(!sensor_read(sd,0x1050, &val))// the avg in hardware
	   vfe_dev_dbg("currect 0x1050(hardware) value of val is %x\n",val);
	if(!sensor_read(sd,0x0025, &val))//\B5\B1ǰ\C6ع\E2ֵ  \B8\DFλ
	   vfe_dev_dbg("currect 0x0025 value of val is %x\n",val);
	if(!sensor_read(sd,0x0026, &val))//8-15
	   vfe_dev_dbg("currect 0x0026 value of val is %x\n",val);
	if(!sensor_read(sd,0x0027, &val))//0-7
	   vfe_dev_dbg("currect 0x0027 value of val is %x\n",val);
#endif	
    sensor_s_internal_scene(sd);
#if GET_CPU_INFO//get cpu state reg.
	LOG_ERR_RET(sensor_write(sd,0xfffe, 0x80));//
	LOG_ERR_RET(sensor_write(sd,0x0137, 0x66));//\D0\E8Ҫ\CF\C8д\B2\C5\C4ܶ\C1ȡ
    LOG_ERR_RET(sensor_read(sd,0x0137, &val));//cpu״̬\BCĴ\E6\C6\F7,\D5\FD\B3\A3һ\B0\E3Ϊ0x88\A3\AC\B7\C7\D5\FD\B3\A3Ϊ0x66
    vfe_dev_dbg("currect 0x0137(cpu) value of val is %x\n",val);
	if(0x88 != val)  {//cpu work unnormal.
	          
    }
#endif	

    return 0;
}

static inline void switch_scene_work(struct v4l2_subdev *sd) {
	int ret = 0;
	struct sensor_info *info = sd ? to_state(sd) : NULL;
		
	if(sd && info && !info->init_first_flag && !info->preview_first_flag)  {
		ret = sensor_s_scene((sd));
		if(ret)  {
		   vfe_dev_err("switch error!\n");
		}
	}	
}

//-------add to turn img view---
static inline int sensor_init_0flip(struct v4l2_subdev *sd)  {
	int ret = 0;

	if(!CHECK_REGS(p_isp_0flip_regs))  {
		LOG_ERR_RET(sensor_write_array(sd, p_isp_0flip_regs->regs, p_isp_0flip_regs->size));	
	}
	if(!CHECK_REGS(p_sensor_0flip_regs))  {
		//passby on
		vfe_dev_dbg("Write isp passby on regs.\n");
		LOG_ERR_RET(sensor_write_array(sd, isp_bypass_on, ARRAY_SIZE(isp_bypass_on)));	
		msleep(10);
		//flip.
		ret = sensor_write_array_passby(sd,sensor_slave,p_sensor_0flip_regs->regs,p_sensor_0flip_regs->size);
		if(ret)  {
			goto passby_err;
		}
		//passby off
		msleep(10);		
		vfe_dev_dbg("Write isp passby off regs.\n");
		LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));				
	}
	return 0;
	
passby_err:
	LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));//passby off	
	return -ENODEV;		
}

static inline int sensor_init_180flip(struct v4l2_subdev *sd)  {
	int ret = 0;
	
	if(!CHECK_REGS(p_isp_180flip_regs))  {
		LOG_ERR_RET(sensor_write_array(sd, p_isp_180flip_regs->regs, p_isp_180flip_regs->size));	
	}
	if(!CHECK_REGS(p_sensor_180flip_regs))  {
		//passby on
		vfe_dev_dbg("Write isp passby on regs.\n");
		LOG_ERR_RET(sensor_write_array(sd, isp_bypass_on, ARRAY_SIZE(isp_bypass_on)));	
		msleep(10);
		//flip.
		ret = sensor_write_array_passby(sd,sensor_slave,p_sensor_180flip_regs->regs,p_sensor_180flip_regs->size);
		if(ret)  {
			goto passby_err;
		}
		//passby off
		msleep(10);		
		vfe_dev_dbg("Write isp passby off regs.\n");
		LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));				
	}	
	return 0;
	
passby_err:
	LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));//passby off	
	return -ENODEV;		
}

static int sensor_s_vflip(struct v4l2_subdev *sd,int value)  {
	int ret = 0;
	struct sensor_info *info = to_state(sd);
	
	vfe_dev_dbg("sensor_s_vflip = %d\n",value);
	if(value && !info->vflip)  {
		ret = sensor_init_180flip(sd);
		if(ret) {
			vfe_dev_err("sensor_s_vflip set 180 flip error.\n");
			return -1;
		}
		info->vflip = 1;
		info->hflip = 1;//Notice.aw6131 turn vflip and hflip at same time.
	}
	
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd,int value)  {
	int ret = 0;
	struct sensor_info *info = to_state(sd);
	
	vfe_dev_dbg("sensor_s_hflip = %d\n",value);
	if(value && !info->hflip)  {
		ret = sensor_init_180flip(sd);
		if(ret) {
			vfe_dev_err("sensor_s_vflip set 180 flip error.\n");
			return -1;
		}
		info->vflip = 1;//Notice.aw6131 turn vflip and hflip at same time.
		info->hflip = 1;
	}

	return 0;
}
//---------------------------end add-----------

/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret;
	static int power_flag = 0;//LANDSEM@yingxianFei add to mark power on or power off.
	struct sensor_info *info = sd ? to_state(sd) : NULL;
	
	ret = 0;
	switch (on) {
	case CSI_SUBDEV_STBY_ON:
		vfe_dev_dbg("CSI_SUBDEV_STBY_ON!\n");
		cci_lock(sd);
		vfe_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vfe_set_mclk(sd, OFF);
		//add 2016.3.18
		info->preview_first_flag = 1;
		info->init_first_flag = 1;		
		//end.		
		cci_unlock(sd);
		break;
		
	case CSI_SUBDEV_STBY_OFF:
		vfe_dev_dbg("CSI_SUBDEV_STBY_OFF!\n");
		cci_lock(sd);
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		ret = sensor_s_sw_stby(sd, CSI_GPIO_LOW);
		if (ret < 0)
			vfe_dev_err("soft stby off falied!\n");
		usleep_range(10000, 12000);
		break;
		
	case CSI_SUBDEV_PWR_ON:
		vfe_dev_dbg("CSI_SUBDEV_PWR_ON!\n");
		if(power_flag)  {//power off and delay power on.
			sensor_power(sd,CSI_SUBDEV_PWR_OFF);
			usleep_range(1000, 1200);
		}
		cci_lock(sd);
		vfe_gpio_set_status(sd, PWDN, 1);	/*set the gpio to output */
		vfe_gpio_set_status(sd, RESET, 1);	/*set the gpio to output */
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vfe_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vfe_set_pmu_channel(sd, AFVDD, ON);	/*1.2V  CVDD_12 */
		vfe_set_pmu_channel(sd, DVDD, ON);	/*VCAM_D 1.5v */
		usleep_range(1000, 1200);
		vfe_set_pmu_channel(sd, IOVDD, ON);	/*VCAM_IO 2.8v */

		usleep_range(1000, 1200);
		vfe_set_pmu_channel(sd, AVDD, ON);	/* VCAM_AF 3.3v */
		usleep_range(1000, 1200);

		vfe_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(30000, 32000);
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		power_flag = 1;//LANDSEM@yingxianFei power on.
		cci_unlock(sd);
		break;

	case CSI_SUBDEV_PWR_OFF:
		vfe_dev_dbg("CSI_SUBDEV_PWR_OFF!\n");	
		if(!power_flag)  {
			vfe_dev_dbg("power has been power off.");
			return 0;
		}
		cci_lock(sd);
		vfe_gpio_set_status(sd, PWDN, 1);	/*set the gpio to output */
		vfe_gpio_set_status(sd, RESET, 1);	/*set the gpio to output */
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vfe_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vfe_set_mclk(sd, OFF);
		vfe_set_pmu_channel(sd, AVDD, OFF);	/* VCAM_AF 3.3v */
		usleep_range(10000, 12000);
		vfe_set_pmu_channel(sd, DVDD, OFF);	/*VCAM_D 1.5v */
		usleep_range(10000, 12000);
		vfe_set_pmu_channel(sd, IOVDD, OFF);	/*VCAM_IO 2.8v */
		vfe_set_pmu_channel(sd, AFVDD, OFF);	/*1.2V  CVDD_12 */
		vfe_gpio_set_status(sd, RESET, 0);	/*set the gpio to input */
		vfe_gpio_set_status(sd, PWDN, 0);	/*set the gpio to input */
		//add 2016.3.18
		info->preview_first_flag = 1;
		info->init_first_flag = 1;		
		//end	
		power_flag = 0;//LANDSEM@yingxianFei power off.
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);		
		usleep_range(20000, 32000);
		break;
	case 1:
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(20000, 32000);
		break;	
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_debug(struct v4l2_subdev *sd)
{
	struct regval_list regs;

	vfe_dev_dbg("********into aw6131 sensor_debug********\n");

	regs.addr = 0xfffe;
	regs.data = 0x80;
	LOG_ERR_RET(sensor_write(sd, regs.addr, regs.data));

	regs.addr = 0x50;
	LOG_ERR_RET(sensor_read(sd, regs.addr, &regs.data));
	vfe_dev_dbg("********read reg[0x50]= %x ********\n", regs.data);

	regs.addr = 0x58;
	LOG_ERR_RET(sensor_read(sd, regs.addr, &regs.data));
	vfe_dev_dbg("********read reg[0x58]= %x ********\n", regs.data);
	//-------------
	regs.addr = 0xfffe;
	regs.data = 0x26;
	LOG_ERR_RET(sensor_write(sd, regs.addr, regs.data));	
	//0x38
	regs.addr = 0x38;
	LOG_ERR_RET(sensor_read(sd, regs.addr, &regs.data));
	vfe_dev_dbg("********read reg[0x38]= %x ********\n", regs.data);
	//0x39
	regs.addr = 0x39;
	LOG_ERR_RET(sensor_read(sd, regs.addr, &regs.data));
	vfe_dev_dbg("********read reg[0x39]= %x ********\n", regs.data);
	//0x3a
	regs.addr = 0x3a;
	LOG_ERR_RET(sensor_read(sd, regs.addr, &regs.data));
	vfe_dev_dbg("********read reg[0x3a]= %x ********\n", regs.data);
	//0x3b
	regs.addr = 0x3b;
	LOG_ERR_RET(sensor_read(sd, regs.addr, &regs.data));
	vfe_dev_dbg("********read reg[0x3b]= %x ********\n", regs.data);

	return 0;
}

static int isp_detect(struct v4l2_subdev *sd)
{
	unsigned int chip_id = 0;
	data_type rdval;
	vfe_dev_dbg("call %s",__func__);

    LOG_ERR_RET(sensor_write(sd, 0xfffd, 0x80));
	LOG_ERR_RET(sensor_write(sd, 0xfffe, 0x80));
    LOG_ERR_RET(sensor_read(sd, 0x0003, &rdval))
    chip_id |= (rdval << 8);
	LOG_ERR_RET(sensor_read(sd, 0x0002, &rdval))
	chip_id |= (rdval << 0);
	chip_id &= (0xffff);
    if(0x5843 != chip_id)  {
		vfe_dev_err("sensor read chip id:%x error!\n",chip_id);
		return -ENODEV;
	}			
	
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)  {
	int i = 0,ret = 0;
	data_type rdval;

	if(CHECK_REGS(p_sensor_chip_id))  {
		vfe_dev_err("Sensor chip id regs is null\n");
		return -ENODEV;
	}
	vfe_dev_dbg("Write isp passby on regs.\n");
	LOG_ERR_RET(sensor_write_array(sd, isp_bypass_on, ARRAY_SIZE(isp_bypass_on)));	
	msleep(10);
	//read and check chip id.
	for(i = 0;i < p_sensor_chip_id->size;i ++)  {
		ret = sensor_read_passby(sd,sensor_slave,p_sensor_chip_id->regs[i].addr,&rdval);
		if(ret)  {
			vfe_dev_err("Passby sensor read error\n");
			goto passby_err;
		}
		//check data.
		if(p_sensor_chip_id->regs[i].data != rdval)  {
			goto passby_err;
		}
	}
	msleep(10);		
	//disenable passby
	vfe_dev_dbg("Write isp passby off regs.\n");
	LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));	

	return 0;
passby_err:
	LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));//passby off	
	return -ENODEV;		
}

static int sensor_init_internal(struct v4l2_subdev *sd, u32 val)  {
	int ret;
	struct sensor_info *info = to_state(sd);

	vfe_dev_dbg("sensor_init\n");

	/*Make sure it is a target isp */
	ret = isp_detect(sd);
	if (ret) {
		vfe_dev_err("chip found is not an target isp.\n");
		return ret;
	}

	vfe_get_standby_mode(sd, &info->stby_mode);

	//Add it 3016.3.18
	if(likely(0 == info->init_first_flag))	{
		vfe_dev_dbg("Do not repeat init sensor.\n");
		return 0;
	}
	//end.
	if ((info->stby_mode == HW_STBY || info->stby_mode == SW_STBY) && (info->init_first_flag == 0)) {
		vfe_dev_print("stby_mode and init_first_flag = 0\n");
		return 0;
	}
	//init
	info->focus_status = 0;
	info->low_speed = 0;
	info->width = HD1080_WIDTH;
	info->height = HD1080_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->tpf.numerator = 1;
	info->tpf.denominator = 25;

    //isp default regs.
    if(CHECK_REGS(p_isp_default_regs))  {
		vfe_dev_err("ISP default regs is null!\n");
		return -ENODEV;
	}	
	vfe_dev_dbg("Write isp default regs.length=%d\n",p_isp_default_regs->size);
	LOG_ERR_RET(sensor_write_array(sd, p_isp_default_regs->regs, p_isp_default_regs->size));	
	/*Make sure it is a target sensor*/
	ret = sensor_detect(sd);
	if (ret) {
		vfe_dev_err("Chip found is not an target sensor.\n");
		return ret;
	}	
	//write sensor's default regs
	if(!CHECK_REGS(p_sensor_default_regs))  {
		//enable passby
		vfe_dev_dbg("Write isp passby on regs.\n");
		LOG_ERR_RET(sensor_write_array(sd, isp_bypass_on, ARRAY_SIZE(isp_bypass_on)));	
		msleep(10);	
		//write sensor default regs.
		vfe_dev_dbg("Write sensor default regs.length=%d\n",p_sensor_default_regs->size);
		ret = sensor_write_array_passby(sd,sensor_slave, p_sensor_default_regs->regs, p_sensor_default_regs->size);
		if(ret < 0)  {
			vfe_dev_err("Passby write sensor default regs error!\n");
			goto passby_err;
		}
		msleep(10);
		//disenable passby
		vfe_dev_dbg("Write isp passby off regs.\n");
		LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));			
	}	
	sensor_debug(sd);

	if (info->stby_mode == 0)  {
		info->init_first_flag = 0;
	}

	info->preview_first_flag = 1;

	return 0;

passby_err:
	LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));//passby off	
	return -ENODEV;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{	
   //reset power.
   if(register_vfe_first)  {	
   	  vfe_dev_print("Reset camera's power after reg registered.\n");
   	  //power off and power on again.
   	  sensor_power(sd,CSI_SUBDEV_PWR_OFF);
	  msleep(10);//delay 10ms
	  sensor_power(sd,CSI_SUBDEV_PWR_ON);	
	  //update flag.
   	  register_vfe_first = 0;
   }
   //detect it.
   if(!register_vfe_flag)  {
   	  vfe_dev_print("Call sensor init without register regs..\n");
   	  return isp_detect(sd);
   }
   //init sensor.
   return sensor_init_internal(sd,val);
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);
	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
			       sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			vfe_dev_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case ISP_SET_EXP_GAIN:
		sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	//add it to switch scene.2016.3.18
	case SET_SCENE_SWITCH: {
		switch_scene_work(sd);
		break;
	}
	//end add.
	default:
		return -EINVAL;
	}
	return ret;
}

static struct sensor_format_struct {
	__u8 *desc;
	enum v4l2_mbus_pixelcode mbus_code;
	struct regval_list *regs;
	int regs_size;
	int bpp;   /* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,
		.regs 		= sensor_fmt_yuv422_yuyv,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
		.bpp		= 2,
	} , {
		.desc		= "YVYU 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_YVYU8_2X8,
		.regs 		= sensor_fmt_yuv422_yvyu,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
		.bpp		= 2,
	} , {
		.desc		= "UYVY 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_UYVY8_2X8,
		.regs 		= sensor_fmt_yuv422_uyvy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
		.bpp		= 2,
	} , {
		.desc		= "VYUY 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_VYUY8_2X8,
		.regs 		= sensor_fmt_yuv422_vyuy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
		.bpp		= 2,
	} , {
		.desc		= "Raw RGB Bayer",
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.regs 		= sensor_fmt_raw,
		.regs_size  = ARRAY_SIZE(sensor_fmt_raw),
		.bpp		= 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned index,
			   enum v4l2_mbus_pixelcode *code)
{
	if (index >= N_FMTS)
		return -EINVAL;

	*code = sensor_formats[index].mbus_code;
	return 0;
}

static int sensor_enum_size(struct v4l2_subdev *sd,
			    struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index > N_ISP_SIZES - 1)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = isp_win_sizes[fsize->index].width;
	fsize->discrete.height = isp_win_sizes[fsize->index].height;

	return 0;
}

static int isp_try_fmt_internal(struct v4l2_subdev *sd,
				   struct v4l2_mbus_framefmt *fmt,
				   struct sensor_format_struct **ret_fmt,
				   struct sensor_win_size **ret_wsize)
{
	int index;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);

	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].mbus_code == fmt->code)
			break;

	if (index >= N_FMTS)
		return -EINVAL;

	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;

	/*
	 * Fields: the sensor devices claim to be progressive.
	 */
	fmt->field = V4L2_FIELD_NONE;

	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = isp_win_sizes; wsize < isp_win_sizes + N_ISP_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;

	if (wsize >= isp_win_sizes + N_ISP_SIZES)
		wsize--;	/* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;
	fmt->height = wsize->height;
	info->current_wins = wsize;
	
	return 0;
}

static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
				   struct v4l2_mbus_framefmt *fmt,
				   struct sensor_format_struct **ret_fmt,
				   struct sensor_win_size **ret_wsize)  
{
	int index;
	struct sensor_win_size *wsize;

	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].mbus_code == fmt->code)
			break;

	if (index >= N_FMTS)
		return -EINVAL;

	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;

	/*
	 * Fields: the sensor devices claim to be progressive.
	 */
	fmt->field = V4L2_FIELD_NONE;

	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_SENSOR_SIZES;wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;

	if (wsize >= sensor_win_sizes + N_SENSOR_SIZES)
		wsize--;	/* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;
	fmt->height = wsize->height;
	
	return 0;

}

static int sensor_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *fmt)
{
	return isp_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL;

	return 0;
}

static int sensor_s_fmt_internal(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	int ret;
	struct sensor_format_struct *isp_fmt;
	struct sensor_win_size *isp_wsize;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *sensor_wsize;	
	struct sensor_info *info = to_state(sd);

	vfe_dev_dbg("sensor_s_fmt\n");

	ret = isp_try_fmt_internal(sd, fmt, &isp_fmt, &isp_wsize);
	if (ret)  {
		return ret;
	}
	//detect sensor's cfg
	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &sensor_wsize);
	if(ret)  {
		return ret;
	}

	//add it 2016.3.18
	if(likely(0 == info->preview_first_flag) && likely(info->width == isp_wsize->width && info->height == isp_wsize->height))  {
		vfe_dev_dbg("Do not repeat set same fmt.\n");
		return 0;
	}
    vfe_dev_dbg("Set isp fmt %5d x %5d\n",fmt->width,fmt->height);
	//end

	if (info->capture_mode == V4L2_MODE_VIDEO) {
		/*video */
	} else if (info->capture_mode == V4L2_MODE_IMAGE) {
		/*image */
	}
	//sensor set regs.
	LOG_ERR_RET(sensor_write_array(sd, isp_fmt->regs, isp_fmt->regs_size))
	if(NULL != isp_wsize)  {
		vfe_dev_dbg("Write isp wsize regs.length=%d\n",isp_wsize->regs_size);
		if (isp_wsize->regs)  {
			LOG_ERR_RET(sensor_write_array(sd, isp_wsize->regs, isp_wsize->regs_size))
		}
		if (isp_wsize->set_size)  {
			LOG_ERR_RET(isp_wsize->set_size(sd))
		}
	}
	//write sensor's regs passby
	if((NULL != sensor_wsize) && (NULL != sensor_wsize->regs) && (0 < sensor_wsize->regs_size))  {
		vfe_dev_dbg("Write isp passby on regs.\n");
		LOG_ERR_RET(sensor_write_array(sd, isp_bypass_on, ARRAY_SIZE(isp_bypass_on)));	
		msleep(10);//delay
		vfe_dev_dbg("Write sensor size regs.length=%d\n",sensor_wsize->regs_size);
		ret = sensor_write_array_passby(sd, sensor_slave,sensor_wsize->regs, sensor_wsize->regs_size);		
		if(ret)  {
			goto passby_err;
		}
		msleep(10);//delay
		vfe_dev_dbg("Write isp passby off regs.\n");
		LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));	
	}	
	info->fmt = isp_fmt;
	info->width = isp_wsize->width;
	info->height = isp_wsize->height;

	vfe_dev_print("s_fmt set width = %d, height = %d\n", isp_wsize->width,isp_wsize->height);

	if (info->capture_mode == V4L2_MODE_VIDEO) {
		/*video */
	} else {
		/*capture image */
	}

   //add it 2016.3.18
   info->preview_first_flag = 0;
   //end.

	sensor_debug(sd);

	return 0;
	
passby_err:
	LOG_ERR_RET(sensor_write_array(sd, isp_bypass_off, ARRAY_SIZE(isp_bypass_off)));	
	return ret;
}

static int sensor_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)  {
	if(!register_vfe_flag)  {
		vfe_dev_err("Please call set format after register vfe data regs.\n");
		return -1;
	}
	return sensor_s_fmt_internal(sd,fmt);
}
/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = info->capture_mode;

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	vfe_dev_dbg("sensor_s_parm\n");

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (info->tpf.numerator == 0)
		return -EINVAL;

	info->capture_mode = cp->capturemode;

	return 0;
}

static int sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */

	switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 1 * 16, 16 * 16, 1, 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 1, 65536 * 16, 1, 1);
	case V4L2_CID_FRAME_RATE:
		return v4l2_ctrl_query_fill(qc, 15, 120, 1, 30);
	}
	return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
#if 0
	struct v4l2_queryctrl qc;
	int ret;

	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);
	if (ret < 0) {
		return ret;
	}

	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
		vfe_dev_err("max gain qurery is %d,min gain qurey is %d\n",
			    qc.maximum, qc.minimum);
		return -ERANGE;
	}

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->value);
	}
	return -EINVAL;
#else
//add to flip.2016.03.21
    switch(ctrl->id)  {
	case V4L2_CID_VFLIP : {
		return sensor_s_vflip(sd,ctrl->value);
		break;
	}
	case V4L2_CID_HFLIP: {
		return sensor_s_hflip(sd,ctrl->value);
		break;
	}
	}
//end add to flip.2016.03.21	
	return 0;
#endif
}

static int sensor_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SENSOR, 0);
}
/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_chip_ident = sensor_g_chip_ident,
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.enum_mbus_fmt = sensor_enum_fmt,
	.enum_framesizes = sensor_enum_size,
	.try_mbus_fmt = sensor_try_fmt,
	.s_mbus_fmt = sensor_s_fmt,
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_16,
	.data_width = CCI_BITS_8,
};

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;

    vfe_dev_dbg("sensor_probe\n");
	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	glb_sd = sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	info->fmt = &sensor_formats[0];
	info->af_first_flag = 1;
	info->init_first_flag = 1;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	sd = cci_dev_remove_helper(client, &cci_drv);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = SENSOR_NAME,
		   },
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static __init int init_sensor(void)
{
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);

