#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/switch.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>  
#include <linux/uaccess.h>
#include "vfe_sub_device.h"
#include "ov2718_aw6131_data.h"


#define DRV_VERSION   "V1.0.0"
#define DEV_DBG_EN   		(1)
#if(DEV_DBG_EN == 1)		
   #define ls_dev_dbg(x,arg...) printk(KERN_INFO"[LANDSEM][OV2718_AW6131]"x,##arg)
#else
   #define ls_dev_dbg(x,arg...) 
#endif
#define ls_dev_err(x,arg...) printk(KERN_ERR"[LANDSEM][OV2718_AW6131]"x,##arg)
#define ls_dev_info(x,arg...) printk(KERN_INFO"[LANDSEM][OV2718_AW6131]"x,##arg)

extern int register_aw6131_ov2718_regs(struct ls_vfe_sub_dev *sub_dev);
extern int unregister_aw6131_ov2718_regs(void);

struct ls_vfe_sub_dev data_ov2718_aw6131;

struct cfg_array sensor_chip_id_regs = {
	ov2718_chip_id_regs,
	ARRAY_SIZE(ov2718_chip_id_regs),
};

struct cfg_array aw6131_default = {
	aw6131_default_regs,
	ARRAY_SIZE(aw6131_default_regs),
};
struct cfg_array ov2718_default = {
	ov2718_default_regs,
	ARRAY_SIZE(ov2718_default_regs),
};
struct cfg_array aw6131_1080p = {
	aw6131_1080p_25fps_regs,
	ARRAY_SIZE(aw6131_1080p_25fps_regs),
};
struct cfg_array ov2718_1080p = {
	ov2718_1080p_25fps_regs,
	ARRAY_SIZE(ov2718_1080p_25fps_regs),
};
struct cfg_array aw6131_720p = {
	aw6131_720p_30fps_regs,
	ARRAY_SIZE(aw6131_720p_30fps_regs),
};
struct cfg_array ov2718_720p = {
	ov2718_720p_30fps_regs,
	ARRAY_SIZE(ov2718_720p_30fps_regs),
};
struct cfg_array aw6131_480p = {
	aw6131_480p_30fps_regs,
	ARRAY_SIZE(aw6131_480p_30fps_regs),
};
struct cfg_array ov2718_480p = {
	ov2718_480p_30fps_regs,
	ARRAY_SIZE(ov2718_480p_30fps_regs),
};
struct cfg_array aw6131_dark = {
	aw6131_darkness_regs,
	ARRAY_SIZE(aw6131_darkness_regs),
};
struct cfg_array ov2718_dark = {
	ov2718_darkness_regs,
	ARRAY_SIZE(ov2718_darkness_regs),
};
struct cfg_array aw6131_bright = {
	aw6131_brightness_regs,
	ARRAY_SIZE(aw6131_brightness_regs),
};
struct cfg_array ov2718_bright = {
	ov2718_brightness_regs,
	ARRAY_SIZE(ov2718_brightness_regs),
};
//flip regs.
struct cfg_array aw6131_0flip = {
	aw6131_fmt_0flip,
	ARRAY_SIZE(aw6131_fmt_0flip),
};
struct cfg_array ov2718_0flip = {
	ov2718_fmt_0flip,
	ARRAY_SIZE(ov2718_fmt_0flip),
};
struct cfg_array aw6131_180flip = {
	aw6131_fmt_180flip,
	ARRAY_SIZE(aw6131_fmt_180flip),
};
struct cfg_array ov2718_180flip = {
	ov2718_fmt_180flip,
	ARRAY_SIZE(ov2718_fmt_180flip),
};

static void register_ov2718_data(struct ls_vfe_sub_dev *sub_dev) {
	int ret = 0;
	if(NULL != sub_dev)  {
		sub_dev->sensor_slave = OV2718_I2C_ADDR;
		sub_dev->sensor_chip_id_regs = &sensor_chip_id_regs;
		sub_dev->isp_default_regs = &aw6131_default;
		sub_dev->sensor_default_regs = &ov2718_default;
		sub_dev->isp_1080p_regs = &aw6131_1080p;
		sub_dev->sensor_1080p_regs = &ov2718_1080p;
		sub_dev->isp_720p_regs = &aw6131_720p;
		sub_dev->sensor_720p_regs = &ov2718_720p;
		sub_dev->isp_480p_regs = &aw6131_480p;
		sub_dev->sensor_480p_regs = &ov2718_480p;
		sub_dev->dark_threshold_value = DARK_SWITCH_THRESHOLD;
		sub_dev->bright_threshold_value = BRIGHT_SWITCH_THRESHOLD;
/*****LANDSEM@liuxueneng 20160622add for ov2718 start*******/
		sub_dev->dark_threshold_value_2 = DARK_SWITCH_THRESHOLD_2;
		sub_dev->bright_threshold_value_2 = BRIGHT_SWITCH_THRESHOLD_2;
/*****LANDSEM@liuxueneng 20160622add for ov2718 start*******/
		sub_dev->isp_dark_regs = &aw6131_dark;
		sub_dev->sensor_dark_regs = NULL;
		sub_dev->isp_bright_regs = &aw6131_bright;
		sub_dev->sensor_bright_regs = NULL;
		sub_dev->isp_0flip_regs = NULL;
		sub_dev->sensor_0flip_regs = NULL;
/*****LANDSEM@liuxueneng 20160720 modify for ov2718flip start*******/
		sub_dev->isp_180flip_regs = &aw6131_180flip;
		sub_dev->sensor_180flip_regs = NULL;
/*****LANDSEM@liuxueneng 20160720 modify for ov2718flip end******/

        ls_dev_info("Register ov2718 data.\n");
		ret = register_aw6131_ov2718_regs(sub_dev);
		if(ret)  {
			ls_dev_err("Register ov2718 data error!\n");
		}		
	}
}

static int __init landsem_data_init(void)
{
	ls_dev_info("landsem OV2718_AW6131 data driver.VERSION=%s\n",DRV_VERSION);
	register_ov2718_data(&data_ov2718_aw6131);

	return 0;
}

static void __exit landsem_data_exit(void)
{
   ls_dev_err("Remove ov2718 data driver.\n");
   unregister_aw6131_ov2718_regs();
}

module_init(landsem_data_init);
module_exit(landsem_data_exit);

MODULE_AUTHOR("LANDSEM@yingxianFei");
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("OV2718 data for aw6131 driver for landsem");

