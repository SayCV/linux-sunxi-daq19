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
#include <linux/sys_config.h>
#include <linux/mutex.h>
#include <linux/slab.h>


#define DEV_DBG_EN   		(1)
#if DEV_DBG_EN
   #define common_dev_dbg(x,arg...) printk(KERN_INFO"[LANDSEM][COMMON]"x,##arg)
#else
   #define common_dev_dbg(x,arg...) 
#endif
#define common_dev_err(x,arg...) printk(KERN_ERR"[LANDSEM][COMMON]"x,##arg)
#define common_dev_info(x,arg...) printk(KERN_INFO"[LANDSEM][COMMON]"x,##arg)

#define DRV_VERSION     "1.2.1"
#define UPDATE_DATE     "20161111"

#define NAME_MAX_LENTH  (256)

struct ls_io_config{
    int fetch_flag;//get config from sys_config.fex success?
	int request_flag;//request gpio success?	
	int hold_flag;//need to hold this gpio?
	unsigned int init_delay;//need to hold some time with ms.
    char name[NAME_MAX_LENTH];//name of sys_config and gpio request.
	struct mutex lock;//lock	
	struct gpio_config gpio;//available gpio.
	u32 private_data;//legacy status or value.This is user data.The gpio.data is default data.
};

#define NAME_POWER_ON      "power_on"           //ph22
#define NAME_LCD_POWER     "lcd_power"          //pb9
#define NAME_BT_POWER      "bt_power"           //pa9
#define NAME_BLK_POWER     "backlight_power"
#define NAME_BLK_EN        "backlight_en"
#define NAME_FM_POWER      "fm_power"           //PH17
#define NAME_RADAR_POWER   "radar_power"        //PH08  
#define NAME_CAMERA_EN     "camera_en"        //PI14  
#define NAME_AP_SHDN        "ap_shdn"        //PH06  
#define NAME_ARM_MUTE     "arm_mute"        //PH11  

struct ls_io_config landsem_common[] = {
//power on.
   	{
		.fetch_flag = 0,
		.request_flag = 0,
		.hold_flag = 1,
		.init_delay = 0,
		.name = NAME_POWER_ON,	
		.private_data = 0,
    },
//bt power.    
	{
		.fetch_flag = 0,	
		.request_flag = 0,	
		.hold_flag = 1,
		.init_delay = 0,
	    .name = NAME_BT_POWER,	
	    .private_data = 0,
	},
//fm power
	{
		.fetch_flag = 0,	
		.request_flag = 0,	
		.hold_flag = 1,
		.init_delay =0,
		.name = NAME_FM_POWER,	
		.private_data = 0,
	},
//back light enable.	
	{
		.fetch_flag = 0,	
		.request_flag = 0,	
		.hold_flag = 0,
		.init_delay = 0,
	    .name = NAME_BLK_EN,
	    .private_data = 0,
	},		
//lcd power	
	{
		.fetch_flag = 0,	
		.request_flag = 0,	
		.hold_flag = 1,
		.init_delay = 200,
	    .name = NAME_LCD_POWER,		
	    .private_data = 0,
	},		
//back light power	
	{
		.fetch_flag = 0,	
		.request_flag = 0,	
		.hold_flag = 1,
		.init_delay = 0,
	    .name = NAME_BLK_POWER,	    
	    .private_data = 0,
	},			
//radar power	
	{
		.fetch_flag = 0,	
		.request_flag = 0,	
		.hold_flag = 1,
		.init_delay = 0,
	    .name = NAME_RADAR_POWER,	    
	    .private_data = 0,
	},
//camera power	
	{
		.fetch_flag = 0,	
		.request_flag = 0,	
		.hold_flag = 1,
		.init_delay = 0,
	    .name = NAME_CAMERA_EN,	    
	    .private_data = 0,
	},
//ap_shdn	
	{
		.fetch_flag = 0,	
		.request_flag = 0,	
		.hold_flag = 1,
		.init_delay = 0,
	    .name = NAME_AP_SHDN,	    
	    .private_data = 0,
	},
//arm_mute
	{
		.fetch_flag = 0,	
		.request_flag = 0,	
		.hold_flag = 1,
		.init_delay = 0,
	    .name = NAME_ARM_MUTE,	    
	    .private_data = 0,
	},
};

#define POWER_ON_ARRAY_INDEX       (0)
#define BT_POWER_ARRAY_INDEX       (POWER_ON_ARRAY_INDEX + 1)
#define FM_POWER_ARRAY_INDEX       (BT_POWER_ARRAY_INDEX + 1)
#define BLK_EN_ARRAY_INDEX         (FM_POWER_ARRAY_INDEX + 1)
#define LCD_POWER_ARRAY_INDEX      (BLK_EN_ARRAY_INDEX + 1)
#define BLK_POWER_ARRAY_INDEX      (LCD_POWER_ARRAY_INDEX + 1)
#define RADAR_POWER_ARRAY_INDEX    (BLK_POWER_ARRAY_INDEX + 1)
#define CAMERA_EN_ARRAY_INDEX       (RADAR_POWER_ARRAY_INDEX + 1)
#define AP_SHDN_ARRAY_INDEX         (CAMERA_EN_ARRAY_INDEX + 1)
#define ARM_MUTE_ARRAY_INDEX        (AP_SHDN_ARRAY_INDEX + 1)

#define N_DEVICES ARRAY_SIZE(landsem_common)

static int landsem_common_set_value(struct ls_io_config *cfg,const int value)  {
   int ret = -1;
   
   if(cfg && cfg->request_flag)  {
   	  mutex_lock(&cfg->lock);
      gpio_set_value(cfg->gpio.gpio, value);
	  cfg->private_data = value;
	  ret = 0;
	  mutex_unlock(&cfg->lock);
   }
   
   return ret;
}

static int landsem_common_get_value(struct ls_io_config *cfg)  {
	if(cfg && cfg->request_flag)  {
		return gpio_get_value(cfg->gpio.gpio);
	}
	return -1;
}

//power on control.
static ssize_t landsem_attr_power_on_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", landsem_common_get_value(&landsem_common[POWER_ON_ARRAY_INDEX]));
}

static ssize_t landsem_attr_power_on_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		common_dev_err("Invalid size\n");
		return err;
	}
	landsem_common_set_value(&landsem_common[POWER_ON_ARRAY_INDEX],(int)val);

	return count;
}

static DEVICE_ATTR(ls_power_on, S_IRWXUGO,landsem_attr_power_on_show, landsem_attr_power_on_store);


void set_power_on(void)  {
	struct ls_io_config *cfg = &landsem_common[POWER_ON_ARRAY_INDEX];

	if(cfg && cfg->request_flag)  {
		//1.lock.
		mutex_lock(&cfg->lock);
		common_dev_dbg("all power on.\n");
		gpio_set_value(cfg->gpio.gpio,1); 
		cfg->private_data = 1;
		//4.unlock.
		mutex_unlock(&cfg->lock);	
	}	
}
EXPORT_SYMBOL(set_power_on);

void set_power_off(void)  {
	struct ls_io_config *cfg = &landsem_common[POWER_ON_ARRAY_INDEX];

	if(cfg && cfg->request_flag)  {
		//1.lock.
		mutex_lock(&cfg->lock);
		common_dev_dbg("all power off.\n");
		gpio_set_value(cfg->gpio.gpio,0);
		cfg->private_data = 0;
		//4.unlock.
		mutex_unlock(&cfg->lock);	
	}			
}
EXPORT_SYMBOL(set_power_off);



//end power on control.


//LANDSEM@liuxueneng add start／／

void set_camera_enable(void)  {
	struct ls_io_config *cfg = &landsem_common[CAMERA_EN_ARRAY_INDEX];

	if(cfg && cfg->request_flag)  {
		//1.lock.
		mutex_lock(&cfg->lock);
		common_dev_dbg("csi1 camera power on.\n");
		gpio_set_value(cfg->gpio.gpio,1); 
		cfg->private_data = 1;
		//4.unlock.
		mutex_unlock(&cfg->lock);	
	}	
}
EXPORT_SYMBOL(set_camera_enable);

void set_camera_disable(void)  {
	struct ls_io_config *cfg = &landsem_common[CAMERA_EN_ARRAY_INDEX];

	if(cfg && cfg->request_flag)  {
		//1.lock.
		mutex_lock(&cfg->lock);
		common_dev_dbg("csi1 camera power off.\n");
		gpio_set_value(cfg->gpio.gpio,0);
		cfg->private_data = 0;
		//4.unlock.
		mutex_unlock(&cfg->lock);	
	}			
}
EXPORT_SYMBOL(set_camera_disable);



//end power on control.
//************************************/
//arm_mute start.
static ssize_t landsem_attr_arm_mute_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", landsem_common_get_value(&landsem_common[ARM_MUTE_ARRAY_INDEX]));
}

static ssize_t landsem_attr_arm_mute_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		common_dev_err("Invalid size\n");
		return err;
	}
	landsem_common_set_value(&landsem_common[ARM_MUTE_ARRAY_INDEX],(int)val);

	return count;
}

static DEVICE_ATTR(ls_arm_mute, S_IRWXUGO,landsem_attr_arm_mute_show, landsem_attr_arm_mute_store);
//arm_mute end
static void set_shdn_mute_on(void)
{
	landsem_common_set_value(&landsem_common[AP_SHDN_ARRAY_INDEX], 1);
	landsem_common_set_value(&landsem_common[ARM_MUTE_ARRAY_INDEX], 1);
	common_dev_dbg("ap_shdn and arm_mute set on\n");
}
//ap_shdn start
static ssize_t landsem_attr_ap_shdn_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", landsem_common_get_value(&landsem_common[AP_SHDN_ARRAY_INDEX]));
}

static ssize_t landsem_attr_ap_shdn_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		common_dev_err("Invalid size\n");
		return err;
	}
	landsem_common_set_value(&landsem_common[AP_SHDN_ARRAY_INDEX],(int)val);

	return count;
}

static DEVICE_ATTR(ls_ap_shdn, S_IRWXUGO,landsem_attr_ap_shdn_show, landsem_attr_ap_shdn_store);


//*********************************/
//bt control.
static ssize_t landsem_attr_bt_power_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", landsem_common_get_value(&landsem_common[BT_POWER_ARRAY_INDEX]));
}

static ssize_t landsem_attr_bt_power_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		common_dev_err("Invalid size\n");
		return err;
	}
	landsem_common_set_value(&landsem_common[BT_POWER_ARRAY_INDEX],(int)val);

	return count;
}

static DEVICE_ATTR(ls_bt_power, S_IRWXUGO,landsem_attr_bt_power_show, landsem_attr_bt_power_store);
//end bt control.

//lcd power control.


void set_lcd_power_on(void)  {
	struct ls_io_config *cfg = &landsem_common[LCD_POWER_ARRAY_INDEX];

	if(cfg && cfg->request_flag)  {
		//1.lock.
		mutex_lock(&cfg->lock);
		common_dev_dbg("lcd power on.\n");
		gpio_set_value(cfg->gpio.gpio,1); 
		cfg->private_data = 1;
		//4.unlock.
		mutex_unlock(&cfg->lock);	
	}	
}
EXPORT_SYMBOL(set_lcd_power_on);

void set_lcd_power_off(void)  {
//define LCD_POWER_ARRAY_INDEX      (BLK_EN_ARRAY_INDEX + 1)
	struct ls_io_config *cfg = &landsem_common[LCD_POWER_ARRAY_INDEX];

	if(cfg && cfg->request_flag)  {
		//1.lock.
		mutex_lock(&cfg->lock);
		common_dev_dbg("lcd power off.\n");
		gpio_set_value(cfg->gpio.gpio,0);
		cfg->private_data = 0;
		//4.unlock.
		mutex_unlock(&cfg->lock);	
	}			
}
EXPORT_SYMBOL(set_lcd_power_off);


static ssize_t landsem_attr_lcd_power_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", landsem_common_get_value(&landsem_common[LCD_POWER_ARRAY_INDEX]));
}

static ssize_t landsem_attr_lcd_power_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		common_dev_err("Invalid size\n");
		return err;
	}
	landsem_common_set_value(&landsem_common[LCD_POWER_ARRAY_INDEX],(int)val);

	return count;
}




static DEVICE_ATTR(ls_lcd_power, S_IRWXUGO,landsem_attr_lcd_power_show, landsem_attr_lcd_power_store);
//end lcd power.

//backlight control.
void set_backlight_power_on(void)  {
	struct ls_io_config *cfg = &landsem_common[BLK_POWER_ARRAY_INDEX];

	if(cfg && cfg->request_flag)  {
		//1.lock.
		mutex_lock(&cfg->lock);
		common_dev_dbg("blk power on.\n");
		gpio_set_value(cfg->gpio.gpio,1); 
		cfg->private_data = 1;
		//4.unlock.
		mutex_unlock(&cfg->lock);	
	}	
}
EXPORT_SYMBOL(set_backlight_power_on);

void set_backlight_power_off(void)  {
	struct ls_io_config *cfg = &landsem_common[BLK_POWER_ARRAY_INDEX];

	if(cfg && cfg->request_flag)  {
		//1.lock.
		mutex_lock(&cfg->lock);
		common_dev_dbg("blk power off.\n");
		gpio_set_value(cfg->gpio.gpio,0);
		cfg->private_data = 0;
		//4.unlock.
		mutex_unlock(&cfg->lock);	
	}			
}
EXPORT_SYMBOL(set_backlight_power_off);

static ssize_t landsem_attr_backlight_power_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", landsem_common_get_value(&landsem_common[BLK_POWER_ARRAY_INDEX]));
}

static ssize_t landsem_attr_backlight_power_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		common_dev_err("Invalid size\n");
		return err;
	}
	//update blk status.
	if(val)  {
		set_backlight_power_on();
	}
	else {
		set_backlight_power_off();
	}
	return count;
}

static DEVICE_ATTR(ls_backlight_power, S_IRWXUGO,landsem_attr_backlight_power_show, landsem_attr_backlight_power_store);
//end backlight.
//fm power control
void fm_power_control(const int value)  {
	struct ls_io_config *cfg = &landsem_common[FM_POWER_ARRAY_INDEX];

	if(cfg && cfg->request_flag)  {
		//1.lock.
		mutex_lock(&cfg->lock);
		gpio_set_value(cfg->gpio.gpio,value);
		cfg->private_data = value;
		//4.unlock.
		mutex_unlock(&cfg->lock);	
	}	
}
EXPORT_SYMBOL(fm_power_control);
//end fm power control.

//radar power control.
static ssize_t radar_power_get(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", landsem_common_get_value(&landsem_common[RADAR_POWER_ARRAY_INDEX]));
}

static ssize_t radar_power_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		common_dev_err("Invalid size\n");
		return err;
	}
	landsem_common_set_value(&landsem_common[RADAR_POWER_ARRAY_INDEX],(int)val);

	return count;
}

static DEVICE_ATTR(ls_radar_power, S_IRWXUGO,radar_power_get, radar_power_set);
static struct attribute *common_attributes[] = {
	&dev_attr_ls_power_on.attr,
	&dev_attr_ls_lcd_power.attr,
	&dev_attr_ls_bt_power.attr,
	&dev_attr_ls_backlight_power.attr,	
	&dev_attr_ls_radar_power.attr,	
	&dev_attr_ls_arm_mute.attr,	
	&dev_attr_ls_ap_shdn.attr,	
	NULL
};
   
static struct attribute_group common_attribute_group = {
	.name = "common_attr",
	.attrs = common_attributes
};

struct ls_common_dev{
	struct platform_device *pdev;
	struct delayed_work work;
};
static struct ls_common_dev *ls_common_dev;

static int landsem_init_common_io(struct platform_device *pdev)  {
	struct ls_io_config *cfg = NULL;

	common_dev_dbg("call %s\n",__func__);
  	for(cfg = landsem_common;cfg < landsem_common + N_DEVICES;cfg ++)  {
		if(cfg && cfg->hold_flag && cfg->request_flag)  {			
			mutex_lock(&cfg->lock);
			gpio_set_value(cfg->gpio.gpio, (cfg->gpio.data | cfg->private_data));
			msleep(cfg->init_delay);
			mutex_unlock(&cfg->lock);	
		}	
		else if(cfg && !cfg->hold_flag && cfg->fetch_flag)  {
			mutex_lock(&cfg->lock);
			if(!devm_gpio_request(&(pdev->dev),cfg->gpio.gpio,cfg->name))  {//request
			    gpio_direction_output(cfg->gpio.gpio, 1);
				gpio_set_value(cfg->gpio.gpio, (cfg->gpio.data | cfg->private_data));//set value.
				msleep(cfg->init_delay);//delay	
				devm_gpio_free(&(pdev->dev),cfg->gpio.gpio);//free
			}
			mutex_unlock(&cfg->lock);	
		}
	}	
	common_dev_dbg("end %s\n",__func__);
	
    return 0;
}

static void probe_work_handle(struct work_struct *work)  {	
	struct ls_common_dev *common_dev = container_of((struct delayed_work *)work,struct ls_common_dev, work);

	common_dev_dbg("call %s\n",__func__);	
	if(common_dev && common_dev->pdev) {
		if(landsem_init_common_io(common_dev->pdev))  {
			common_dev_err("landsem init common error.\n");
		}
	}
}

static int sub_fetch_config(struct device_node *np,const char *name, struct gpio_config *dev_cfg)  {
	if (!gpio_is_valid(of_get_named_gpio_flags(np, name, 0,(enum of_gpio_flags *)dev_cfg))) {
		common_dev_err("get %s gpio config failed\n",name);
		return -1;
	} 
	common_dev_info("%s gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",name,dev_cfg->gpio,dev_cfg->mul_sel,dev_cfg->pull,dev_cfg->drv_level,dev_cfg->data);	
	
	return 0;
}

static int fetch_config(struct platform_device *pdev)  {
	struct device_node *np = NULL;
	struct ls_io_config *cfg = NULL;
	
    common_dev_dbg("call %s\n",__func__);	
	np = of_find_node_by_name(NULL,"landsem_io");
    if (!np) {
		common_dev_err("ERROR! get landsem io failed, func:%s, line:%d\n",__FUNCTION__, __LINE__);
		return -1;
    }	
	if (!of_device_is_available(np)) {
	    common_dev_err("%s: landsem io is not used\n", __func__);
		return -1;
	}
	//fetch sub device config.
	for(cfg = landsem_common;cfg < landsem_common + N_DEVICES;cfg ++)  {
		if(cfg && !sub_fetch_config(np,cfg->name,&(cfg->gpio))) {
			common_dev_dbg("%s fetch success!\n",cfg->name);
			cfg->fetch_flag = 1;
		}
	}

	return 0;
}


static int landsem_common_request(struct platform_device *pdev)  {
	struct ls_io_config *cfg = NULL;

	for(cfg = landsem_common;cfg < landsem_common + N_DEVICES;cfg ++)  {
		if(cfg)  {
			if(cfg->fetch_flag && cfg->hold_flag)  {			
				//request gpio.
				if(devm_gpio_request(&(pdev->dev),cfg->gpio.gpio,cfg->name))  {
					common_dev_err("request %s goio faild\n",cfg->name);
					continue;
				}
				gpio_direction_output(cfg->gpio.gpio, cfg->gpio.data);//use for output.				
				cfg->request_flag = 1;
			}
			//init lock
			mutex_init(&(cfg->lock));			
		}
	}
	
	return 0;
}

static int landsem_common_free(struct platform_device *pdev)  {
	int i = 0;
	struct ls_io_config *cfg = NULL;

    common_dev_dbg("call %s\n",__func__);	
    for(i = N_DEVICES;i > 0;i --)  {
		cfg = &landsem_common[i - 1];
		if(cfg)  {
			if(cfg->request_flag && cfg->hold_flag)  {
				gpio_set_value(cfg->gpio.gpio, 0);
				devm_gpio_free(&(pdev->dev),cfg->gpio.gpio);				
				cfg->request_flag = 0;
			}		
			mutex_destroy(&cfg->lock);
		}
    }

	return 0;
}

static int landsem_common_probe(struct platform_device *pdev)  {
	int ret = 0;

    common_dev_dbg("call %s\n",__func__);	
	ls_common_dev = kzalloc(sizeof(struct ls_common_dev), GFP_KERNEL);
	if (!ls_common_dev) {
		return -ENOMEM;
	}
	ls_common_dev->pdev = pdev;
	//fetch config.
	ret = fetch_config(pdev);
	if(ret)  {
		common_dev_err("%s %d fetch config error!",__func__,__LINE__);
		return ret;
	}
	//request source
	ret = landsem_common_request(pdev);
	if(ret)  {
		common_dev_err("%s %d request resource error.\n",__func__,__LINE__);
		return ret;
	}
	//create debug attr
	ret = sysfs_create_group(&pdev->dev.kobj, &common_attribute_group);
	if(ret)  {
		common_dev_err("%s %d create attr group error,\n",__func__,__LINE__);
		goto err_free_request;
	}
	//schedule delay work
	INIT_DELAYED_WORK(&ls_common_dev->work, probe_work_handle);
    schedule_delayed_work(&ls_common_dev->work,msecs_to_jiffies(1));
/*LANDSEM@liuxueneng add start*/
    set_power_on();
    set_backlight_power_on();
    set_lcd_power_on();
    set_camera_enable();
    set_shdn_mute_on();
/*LANDSEM@liuxueneng add end*/
    return 0;

err_free_request:
	landsem_common_free(pdev);
//err_free_dev:
	if(ls_common_dev)  {
		kfree(ls_common_dev);
	}

	return -ENODEV;
}

static int landsem_common_remove(struct platform_device *pdev)  {	
	
    common_dev_dbg("call %s\n",__func__);	
	//wait delay work.
	if(ls_common_dev)  {
	   flush_delayed_work(&ls_common_dev->work);
	}
	//remove attr.
	sysfs_remove_group(&pdev->dev.kobj, &common_attribute_group);
	//free io.
	landsem_common_free(pdev);
	if(ls_common_dev)  {
		kfree(ls_common_dev);
	}
	
	return 0;
}

static int landsem_common_suspend(struct platform_device *pdev, pm_message_t state)
{
	int i = 0;
	struct ls_io_config *cfg = NULL;

    common_dev_dbg("call %s\n",__func__);
    for(i = N_DEVICES;i > 0;i --)  {
		cfg = &landsem_common[i - 1];
		if(cfg && cfg->fetch_flag && cfg->hold_flag && cfg->request_flag)  {
			gpio_direction_output(cfg->gpio.gpio, 1);
//			gpio_set_value(cfg->gpio.gpio, !(cfg->gpio.data));
		    gpio_set_value(cfg->gpio.gpio, 0);//all pin pull low.
		}
    }
	
    return 0;
}

static int landsem_common_resume(struct platform_device *pdev)
{
	struct ls_io_config *cfg = NULL;

    common_dev_dbg("call %s\n",__func__);	
	for(cfg = landsem_common;cfg < landsem_common + N_DEVICES;cfg ++)  {
		if(cfg && cfg->fetch_flag && cfg->hold_flag && cfg->request_flag)  {
			//set value
			gpio_set_value(cfg->gpio.gpio, (cfg->gpio.data | cfg->private_data));
			//hold
			msleep(cfg->init_delay);
		}
	}	
	
    return 0;
}

static struct platform_driver landsem_common_driver = {
	.driver = {
		.name	= "landsem_common",
		.owner	= THIS_MODULE,
	},
	.probe      = landsem_common_probe,
	.suspend	= landsem_common_suspend,
	.resume		= landsem_common_resume,
	.remove     = landsem_common_remove,
};

static void landsem_common_release(struct device * dev)
{
    common_dev_dbg("call %s\n",__func__);
}

static struct platform_device landsem_common_device = {
	.name           	= "landsem_common",
    .id             	= 0,
    .dev = {
		.release = landsem_common_release,
	}
};

static int kernel_create_node(void)  {
	u32 ret = 0;

	common_dev_dbg("call %s\n",__func__);
	//create plarform device.
	if(platform_driver_register(&landsem_common_driver)) {
		common_dev_err("platform driver register failed\n");
		return -1;
	}	
	if(platform_device_register(&landsem_common_device))	{
		common_dev_err("platform device register failed\n");
		goto driver_unregister;
	}
	return 0;

driver_unregister:
	platform_driver_unregister(&landsem_common_driver);
	return ret;
}

static int __init landsem_common_init(void)
{
	u32 ret = 0;

	//print version
	common_dev_info("landsem common io driver.VERSION=%s-%s\n",DRV_VERSION,UPDATE_DATE);
	//create file node.
	ret = kernel_create_node();
	if(ret)  {
		common_dev_err("create kernel node failed\n");
		return ret;
	}

	return 0;
}

static void __exit landsem_common_exit(void)
{
	//free device
	platform_device_unregister(&landsem_common_device);
	//free driver	
	platform_driver_unregister(&landsem_common_driver);	
}

subsys_initcall(landsem_common_init);
module_exit(landsem_common_exit);

MODULE_AUTHOR("LANDSEM@yingxianFei");
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("landsem common driver for landsem.");

