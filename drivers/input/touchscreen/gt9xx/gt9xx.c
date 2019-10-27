/* drivers/input/touchscreen/gt9xx.c
 * 
 * 2010 - 2012 Goodix Technology.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the GOODiX's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 * Version:1.4
 * Author:andrew@goodix.com
 * Release Date:2012/12/12
 * Revision record:
 *      V1.0:2012/08/31,first Release
 *      V1.2:2012/10/15,modify gtp_reset_guitar,slot report,tracking_id & 0x0F
 *      V1.4:2012/12/12,modify gt9xx_update.c
 *      
 */

#include <linux/irq.h>
#include "gt9xx_ts.h"
#include "gt9xx_info.h"
#include <linux/pm.h>
#include <linux/workqueue.h>
#include <linux/sys_config.h>

//#include <mach/carconfig.h>

#if GTP_ICS_SLOT_REPORT
    #include <linux/input/mt.h>
#endif

//extern struct input_dev *virtual_keyboard;

//extern unsigned int report_keys[3];//[0]form where ; [1]adc value/right or left knob; [2] press or realess/turn right or left
//extern wait_queue_head_t arm_key_queue;
#define DVDREJ 24 //simulate ad value
enum {
	ADC_KEY0,
	ADC_KEY1,
	KNOB
};
enum {
	RIGHT_KNOB,
	LEFT_KNOB
};
enum {
	RELEASE,
	PRESS,
	CONTINUE,
	TURN_RIGHT,
	TURN_LEFT
};
//LANDSEM
enum{
	DEBUG_INIT = 1U << 1,
	DEBUG_SUSPEND = 1U << 1,
	DEBUG_INT_INFO = 1U << 2,
	DEBUG_X_Y_INFO = 1U << 3,
	DEBUG_KEY_INFO = 1U << 4,
	DEBUG_WAKEUP_INFO = 1U << 5,
	DEBUG_OTHERS_INFO = 1U << 6,
};

static u32 debug_mask = 0;
static u32 irq_free_flag = 0;
static u32 irq_hold_flag = 0;

//LANDSEM
struct tpoffstatus_dev {
	const char	*name;
	struct device	*dev;
	int		index;
	int		state;
	int		gpio[4];//easy switch 4 maybe enough. we must konw which is used
	int (*tpoffstatus_init)(struct tpoffstatus_dev *sdev);
	ssize_t	(*tpoffstatus_write)(struct tpoffstatus_dev *sdev, const char *buf, ssize_t size);
	ssize_t	(*tpoffstatus_read)(struct tpoffstatus_dev *sdev, char *buf);
};
//extern int set_tp_detect_flag(int value);
//extern int get_tp_detect_flag(void);

static int create_tpoffstatus_class(void);
static void __exit tpoffstatus_class_exit(void);
static int tpoffstatus_dev_register(struct tpoffstatus_dev *sdev);
static int tpoffstatus_init(struct tpoffstatus_dev *sdev);
static ssize_t tpoffstatus_write(struct tpoffstatus_dev *sdev, const char *buf, ssize_t size);
static ssize_t tpoffstatus_read(struct tpoffstatus_dev *sdev, char *buf);

static struct tpoffstatus_dev tpstat_tpoffstatus_dev = {
	.name = "tpstat",
	.tpoffstatus_init    = tpoffstatus_init,
	.tpoffstatus_write = tpoffstatus_write,
	.tpoffstatus_read  = tpoffstatus_read,
};

static int TPOffStatus = 0;
struct class *tpoffstatus_class;
static atomic_t device_count;



static int long_press = 0;
static const char *goodix_ts_name = "gt9xx";
static struct workqueue_struct *goodix_wq;
struct i2c_client * i2c_connect_client = NULL; 
static u8 config[GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH+2]
                = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
static u8 config1[GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH+2]
				= {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
static u8 config2[GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH+2]
				= {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
static u8 config3[GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH+2]
				= {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};

#if GTP_HAVE_TOUCH_KEY
	static const u16 touch_key_array[] = GTP_KEY_TAB;
	#define GTP_MAX_KEY_NUM	 (sizeof(touch_key_array)/sizeof(touch_key_array[0]))
#endif


static int gt_vkey[VIRTUAL_KEY_NUM+1][3] = {
				{20,35, VKEY_CODE_EJECT}, 
				{20,135, VKEY_CODE_HOME}, 
				{20,240, VKEY_CODE_BACK}, 
				{20,340, VKEY_CODE_VOLADD}, 
				{20,445, VKEY_CODE_VOLDEC}
		};
static int gt_vkey_pressed = 0;
/*
	gt_tptype == 0;		//8,,,默认
	gt_tptype == 1;		//6.95
	gt_tptype == 2;		//7
	gt_tptype == 3;		//10.1,,,迈腾
	gt_tptype == 4;		//10.1,,,MAX
	gt_tptype == 5;		//10.1,,,凯美瑞
*/
static int gt_tptype = 0;	//0---8 inch,,,1---6.95 inch,,,2---7 inch,,,3---10.1 inch,,,4---10.1a,,,5---10.1b
static s32 virtual_key_xoffset = VIRTUAL_KEY_XOFFSET;

//extern lcd_variable_param lcdparam;
static int tplcok = 0;
#if 0
static s8 gtp_i2c_test(struct i2c_client *client);
#endif

void gtp_reset_guitar(struct i2c_client *client, s32 ms);
void gtp_int_sync(s32 ms);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
#endif
 
#if GTP_CREATE_WR_NODE
extern s32 init_wr_node(struct i2c_client*);
extern void uninit_wr_node(void);
#endif

#if GTP_AUTO_UPDATE
extern u8 gup_init_update_proc(struct goodix_ts_data *);
#endif

#if GTP_ESD_PROTECT
static struct delayed_work gtp_esd_check_work;
static struct workqueue_struct * gtp_esd_check_workqueue = NULL;
static void gtp_esd_check_func(struct work_struct *);
s32 gtp_init_ext_watchdog(struct i2c_client *client);
#endif

///////////////////////////////////////////////
//specific tp related macro: need be configured for specific tp

#define CTP_IRQ_NUMBER          (config_info.irq_gpio.gpio)
#define CTP_IRQ_MODE		(TRIG_EDGE_NEGATIVE)
#define CTP_NAME		("gt9xx")
#define SCREEN_MAX_X	(screen_max_x)
#define SCREEN_MAX_Y	(screen_max_y)
#define PRESS_MAX		(255)
//add by tab.wang . While at reverse status , TP will be useless. 2014.8.30
//extern volatile int reverse_flag;
static int reverse_flag;
//add end
static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static __u32 twi_id = 0;
static char* cfgname;
static int cfg_index = -1;
static u32 int_handle = 0;

static u32 test_cfg_len = 0;

#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("[CTP]:"fmt, ## arg)
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);

static const unsigned short normal_i2c[2] = {0x14, I2C_CLIENT_END};
//static const int chip_id_value[3] = {57};
//static uint8_t read_chip_value[3] = {GTP_REG_VERSION >> 8, GTP_REG_VERSION & 0xff,0};
struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
};

//static void goodix_init_events(struct work_struct *work);
static void goodix_resume_events(struct work_struct *work);
static struct workqueue_struct *goodix_wq;
//static struct workqueue_struct *goodix_init_wq;
static struct workqueue_struct *goodix_resume_wq;
//static DECLARE_WORK(goodix_init_work, goodix_init_events);
static DECLARE_WORK(goodix_resume_work, goodix_resume_events);
s32 gtp_read_version(struct i2c_client *client, u16* version);


struct gt911_cfg_array {
	const char*     name;
	unsigned int    size;
	uint8_t         *config_info;
} gt9xx_cfg_grp[] = {
	{"gt911_group1",        ARRAY_SIZE(gt911_group1),       gt911_group1},
	{"gt911_group2",        ARRAY_SIZE(gt911_group2),       gt911_group2},
	{"gt911_group3",        ARRAY_SIZE(gt911_group3),       gt911_group3},
	{"gt911_group4",        ARRAY_SIZE(gt911_group4),       gt911_group4},
	{"gt911_group5",        ARRAY_SIZE(gt911_group5),       gt911_group5},
	{"gt911_group6",        ARRAY_SIZE(gt911_group6),       gt911_group6},
};
//LANDSEM
s32 gtp_i2c_write(struct i2c_client *client,u8 *buf,s32 len);
bool i2c_test(struct i2c_client * client)
{
        int ret,retry;
        uint8_t test_data[1] = { 0 };	//only write a data address.
        
        for(retry=0; retry < 5; retry++)
        {
                ret = gtp_i2c_write(client, test_data, 1);	//Test i2c.
        	if (ret == 1)
        	        break;
        	msleep(10);
        }
        
        return ret==1 ? true : false;
} 

/**
 * input_set_int_enable - input set irq enable
 * Input:
 * 	type:
 *      enable:
 * return value: 0 : success
 *               -EIO :  i/o err.
 */
static int gt_set_int_enable(int gpio, u32 enable)
{
	int ret = -1;
	u32 irq_number = 0;
	irq_number = gpio_to_irq(gpio);

	if ((enable != 0) && (enable != 1)) {
		return ret;
	}
	
	if (1 == enable)
		enable_irq(irq_number);
	else
		disable_irq_nosync(irq_number);

	return 0;
}

/**
 * input_free_int - input free irq
 * Input:
 * 	type:
 * return value: 0 : success
 *               -EIO :  i/o err.
 */
static int gt_free_int(struct device *dev, int gpio, void *para)
{
	int irq_number = 0;
	irq_number = gpio_to_irq(gpio);
	
	devm_free_irq(dev, irq_number, para);
	irq_free_flag = 1;
	irq_hold_flag = 0;
	return 0;
}
/**
 * input_request_int - input request irq
 * Input:
 * 	type:
 *      handle:
 *      trig_gype:
 *      para:
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int gt_request_int(struct device *dev, int gpio, irq_handler_t handle,
			unsigned long trig_type, void *para)
{
	int ret = -1;
	int irq_number = 0;
	irq_hold_flag = 1;
	irq_free_flag = 0;
	irq_number = gpio_to_irq(gpio);
	if(irq_number < 0){
		printk("gpio_to_irq error %d\n", irq_number);
		return -1;
	}
	ret = devm_request_irq(dev, irq_number, handle,
			       trig_type, "gt9xx_EINT", para);
	if(ret < 0){
		printk("devm_request_irq error %d\n", ret);
		return -1;
	}
	
	return 0;
}

//LANDSEM
static int gtp_find_cfg_idx(const char* name)
{
	int i = 0;
	
	for (i=0; i<ARRAY_SIZE(gt9xx_cfg_grp); i++) {
		if (!strcmp(name, gt9xx_cfg_grp[i].name))
			return i;
	}
	return 0;
}
/**
 * ctp_wakeup - function
 *
 */
static int ctp_wakeup(int gpio, int status, int ms)
{
	dprintk(DEBUG_INIT,"***CTP*** %s:status:%d,ms = %d\n",__func__,status,ms);
	dprintk(DEBUG_INIT,"***config_info.wakeup_gpio.gpio = %d\n", config_info.wakeup_gpio.gpio);
	
	if (status == 0) {

		if(ms == 0) {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
		}else {
			gpio_direction_output(config_info.wakeup_gpio.gpio, 0);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
			msleep(ms);
			gpio_direction_output(config_info.wakeup_gpio.gpio, 1);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
		}
	}
	if (status == 1) {
		if(ms == 0) {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
		}else {
			gpio_direction_output(config_info.wakeup_gpio.gpio, 1);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
			msleep(ms);
			gpio_direction_output(config_info.wakeup_gpio.gpio, 0);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
		}
	}
	msleep(3);
	return 0;
}

/**
 * ctp_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */

static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
        int  ret = -1;
      
        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)){
        	printk("======return=====\n");
                return -ENODEV;
        }
        
        if(twi_id == adapter->nr){
                dprintk(DEBUG_INIT,"%s: addr = %x\n", __func__, client->addr);
                ret = i2c_test(client);
                if(!ret){
        		printk("%s:I2C connection might be something wrong \n", __func__);
        		return -ENODEV;
        	}else{           	    
            	        strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
    		    return 0;	
	        }
	}else{
	        return -ENODEV;
	}
}

void gtp_set_int_value(int status)
{
		/*
        int gpio_status = -1;
        
        gpio_status = sw_gpio_getcfg(CTP_IRQ_NUMBER);
        if(gpio_status != 1){
                sw_gpio_setcfg(CTP_IRQ_NUMBER, 1);
        }
        */
        gpio_direction_output(config_info.irq_gpio.gpio, status);
        __gpio_set_value(config_info.irq_gpio.gpio, status);
        
}

void gtp_set_io_int(void)
{
        int gpio_status = -1;
        /*
        gpio_status = sw_gpio_getcfg(CTP_IRQ_NUMBER);
        if(gpio_status != 6){
                sw_gpio_setcfg(CTP_IRQ_NUMBER, 6);
        }
		*/
}

void gtp_io_init(int ms)
{
	//KyleHu added for config item read 2014-10-16
	printk("tab:-----------------ms %d----------------------\n", ms);
	//KYLE_DEBUG("lcdparam.pcb_ver %d\r\n", lcdparam.pcb_ver);
	/*
	if (20 == lcdparam.pcb_ver)
	{
		config_info.wakeup_number = 37;	//old PCB
	}
	else
	{
		config_info.wakeup_number = 75;	//new PCB
	}
	*/
    ctp_wakeup(config_info.wakeup_gpio.gpio, 0, 0);
    msleep(ms);

    gtp_set_int_value(1);
    msleep(5);

    ctp_wakeup(config_info.wakeup_gpio.gpio, 1, 0);

#if GTP_ESD_PROTECT
    gtp_init_ext_watchdog(client);
#endif
        
}

/*******************************************************	
Function:
	Read data from the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:read data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer
*********************************************************/
s32 gtp_i2c_read(struct i2c_client *client, u8 *buf, s32 len)
{
        struct i2c_msg msgs[2];
        s32 ret = -1;
        s32 retries = 0;
               
        msgs[0].flags = !I2C_M_RD;
        msgs[0].addr  = client->addr;
        msgs[0].len   = GTP_ADDR_LENGTH;
        msgs[0].buf   = &buf[0];
        
        msgs[1].flags = I2C_M_RD;
        msgs[1].addr  = client->addr;
        msgs[1].len   = len - GTP_ADDR_LENGTH;
        msgs[1].buf   = &buf[GTP_ADDR_LENGTH];

        while(retries < 2) {
                ret = i2c_transfer(client->adapter, msgs, 2);
                if(ret == 2) 
                        break;
                retries++;
        }

        if(retries >= 2) {
                printk("%s:I2C retry timeout, reset chip now.\n", __func__);
				if(tplcok == 0){
					tplcok = 1;
					goodix_resume_events(NULL);
					tplcok = 0;
				}
        }
        return ret;
}

/*******************************************************	
Function:
	write data to the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:write data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer.
*********************************************************/
s32 gtp_i2c_write(struct i2c_client *client,u8 *buf,s32 len)
{
        struct i2c_msg msg;
        s32 ret = -1;
        s32 retries = 0;
        
        msg.flags = !I2C_M_RD;
        msg.addr  = client->addr;
        msg.len   = len;
        msg.buf   = buf;
        
        while(retries < 2) {
                ret = i2c_transfer(client->adapter, &msg, 1);
                if (ret == 1) 
                        break;
                retries++;
        }

        if(retries >= 2) {
                printk("%s:I2C retry timeout, reset chip.", __func__);
        }
        return ret;
}

static void test(struct i2c_client *client)
{
	int ret, retry;

	memset(config3+2, 0, sizeof(GTP_CONFIG_MAX_LENGTH));
 	ret = gtp_i2c_read(client, config3, GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH +2);
	printk("ret === %d\n", ret);
	for (retry = 0; retry < (GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH +2); retry++)
	{
		printk("0x%02X,", config3[retry]);
	}
	printk("\r\n");
}

static void write_config(struct i2c_client *client)
{
	int ret, retry;
	printk("->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
    for (retry = 0; retry < 5; retry++)
    {
//        ret = gtp_i2c_write(client, config , GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH);
        ret = gtp_i2c_write(client, config , test_cfg_len + GTP_ADDR_LENGTH+1);
		printk("write_config == (%d)\n", ret);
        if (ret > 0)
        {
            break;
        }
    }
}
/*******************************************************
Function:
	Send config Function.

Input:
	client:	i2c client.

Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
s32 gtp_send_cfg(struct i2c_client *client)
{
    s32 ret = 1;

return ret;	//Kyle added for new tp

#if GTP_DRIVER_SEND_CFG
    s32 retry = 0;
#if 1
printk("kyleprint: I2C read::: ");
	ret = gtp_i2c_read(client, config1, GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH +2);
	printk("ret === %d\n", ret);
	for (retry = 0; retry < (GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH); retry++)
	{
		printk("0x%02X,", config1[retry]);
	}
printk("\r\n");
printk("kyleprint: I2C write::: ");
    for (retry = 0; retry < (test_cfg_len + GTP_ADDR_LENGTH+1); retry++)
	{
		printk("0x%02X,", config[retry]);
	}
printk("\r\n");
#endif
    for (retry = 0; retry < 5; retry++)
    {
//        ret = gtp_i2c_write(client, config , GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH);
        ret = gtp_i2c_write(client, config , test_cfg_len + GTP_ADDR_LENGTH+1);
//		printk("ret === %d\n", ret);
        if (ret > 0)
        {
            break;
        }
    }
#if 1
printk("kyleprint: I2C read again::: ");
		ret = gtp_i2c_read(client, config2, GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH +2);
		printk("ret === %d\n", ret);
		for (retry = 0; retry < (GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH +2); retry++)
		{
			printk("0x%02X,", config2[retry]);
		}
printk("\r\n");
#endif
#endif

    return ret;
}

/*******************************************************
Function:
	Disable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
void gtp_irq_disable(struct goodix_ts_data *ts)
{
        unsigned long irqflags;

        dprintk(DEBUG_INT_INFO, "%s ---start!---\n", __func__);

        spin_lock_irqsave(&ts->irq_lock, irqflags);
        if (!ts->irq_is_disable) {
                ts->irq_is_disable = 1; 
                gt_set_int_enable(CTP_IRQ_NUMBER, 0);
        }
        spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
	Disable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
void gtp_irq_enable(struct goodix_ts_data *ts)
{
        unsigned long irqflags = 0;

        dprintk(DEBUG_INT_INFO, "%s ---start!---\n", __func__);
    
        spin_lock_irqsave(&ts->irq_lock, irqflags);
        if (ts->irq_is_disable) {
                ts->irq_is_disable = 0; 
                gt_set_int_enable(CTP_IRQ_NUMBER, 0);
        }
        spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
	Touch down report function.

Input:
	ts:private data.
	id:tracking id.
	x:input x.
	y:input y.
	w:input weight.
	
Output:
	None.
*******************************************************/
static void gtp_touch_down(struct goodix_ts_data* ts,s32 id,s32 x,s32 y,s32 w)
{
	int keyind;
	static int prekey = -1;
	static unsigned long jiff = 0;

	
	KYLE_DEBUG("report data:ID:%d, X:%d, Y:%d, W:%d(%d,%d,%d,%d,%d)\n", id, x, y, w, exchange_x_y_flag, revert_x_flag, revert_y_flag, ts->abs_x_max, ts->abs_y_max);
	//Kyle added for Lcd off
	if (TPOffStatus) return;

//	exchange_x_y_flag = 1;
//	revert_x_flag = 1;
//	revert_y_flag = 1;
	//add by tab.wang . While at reverse status , TP will be useless. 2014.8.30
	if(reverse_flag == 1){
		printk("reverse status TP is useless\n");
		return;
	}
	//add end
#if 1
        if (1 == exchange_x_y_flag)
	{
                swap(x, y);
        }
        
        if (1 == revert_x_flag)
	{
                x = ts->abs_x_max - x;
        }
        
        if (1 == revert_y_flag)
	{
                y = ts->abs_y_max - y;
        }
#endif
#if 1
	gt_vkey_pressed = -1;
	if (1==gt_tptype || 2==gt_tptype || 3==gt_tptype || 5==gt_tptype || 7==gt_tptype || 8==gt_tptype)
	{//6.95,,,7,,,10.1
		if (x<virtual_key_xoffset)
		{//按键区
			for(keyind=0; keyind<VIRTUAL_KEY_NUM; keyind++)
			{
				if ((x>(gt_vkey[keyind][0]-VIRTUAL_KEY_XRADIUS)) 
					&& (x<(gt_vkey[keyind][0]+VIRTUAL_KEY_XRADIUS)) 
					&& (y>(gt_vkey[keyind][1]-VIRTUAL_KEY_YRADIUS))
					&& (y<(gt_vkey[keyind][1]+VIRTUAL_KEY_YRADIUS))
				)
				{
					break;
				}
			}
			if (keyind<VIRTUAL_KEY_NUM)
			{
				//KYLE_DEBUG("Key %d,%d jiffies %d jiff %d \n", prekey, keyind, jiffies, jiff);
				if ((prekey != keyind) || ((jiffies-jiff) > msecs_to_jiffies(200)))		//去抖,200ms报一次键
				{
					jiff = jiffies;
					if(keyind == 0){
						if(long_press == 0){
							/*
							report_keys[0] = ADC_KEY1; 
							report_keys[1] = DVDREJ;
							report_keys[2] = PRESS;
							wake_up(&arm_key_queue);
							*/
							long_press = 1;
						}
					}else{
						//input_report_key(virtual_keyboard, gt_vkey[keyind][2], 1);
						//input_sync(virtual_keyboard);
						;
					}
				}
				gt_vkey_pressed = gt_vkey[keyind][2];
				KYLE_DEBUG("Key %d be pressed %d !\n", keyind, jiff);
				prekey = keyind;
			}
			else
			{
				gt_vkey_pressed = 0;
				KYLE_DEBUG("No any key be pressed !\n");
			}
		}
		else
		{//报点区
			x -= virtual_key_xoffset;
			//x = ((((x*1000) / (800-virtual_key_xoffset))) * 800)/1000;
			gt_vkey_pressed = -1;
		}
	}
	if (gt_vkey_pressed<0)
	{
#if GTP_ICS_SLOT_REPORT
	        input_mt_slot(ts->input_dev, id);
	        input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
	        input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
	        input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
	        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
	        input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
#else
	        input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
	        input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
	        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
	        input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
	        input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
	        input_mt_sync(ts->input_dev);
#endif
	}

	KYLE_DEBUG("report data:ID:%d, X:%d, Y:%d, W:%d(%d,%d,%d,%d,%d)\n", id, x, y, w, exchange_x_y_flag, revert_x_flag, revert_y_flag, ts->abs_x_max, ts->abs_y_max);

}

/*******************************************************
Function:
	Touch up report function.

Input:
	ts:private data.
	
Output:
	None.
*******************************************************/
static void gtp_touch_up(struct goodix_ts_data* ts, s32 id)
{
	//Kyle added for Lcd off
	if (TPOffStatus) return;

	//add by tab.wang . While at reverse status , TP will be useless. 2014.8.30
	if(reverse_flag == 1){
		printk("reverse status TP is useless\n");
		return;
	}
	//add end
	if (gt_vkey_pressed<0)
	{
#if GTP_ICS_SLOT_REPORT
	        input_mt_slot(ts->input_dev, id);
	        input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
	        dprintk(DEBUG_X_Y_INFO, "Touch id[%2d] release!", id);
#else
	        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
	        input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
	        input_mt_sync(ts->input_dev);
#endif
	}
	else
	{
		//KYLE_DEBUG("***** gt_vkey_pressed = %d\n", gt_vkey_pressed);
		if (gt_vkey_pressed>0)
		{
			if(gt_vkey_pressed == VKEY_CODE_EJECT){
				/*
				report_keys[0] = ADC_KEY1; 
				report_keys[1] = DVDREJ;
				report_keys[2] = RELEASE;
				wake_up(&arm_key_queue);
				*/
				long_press = 0;
			}else{
				//KYLE_DEBUG("gt_vkey_pressed = %d\n", gt_vkey_pressed);
				//input_report_key(virtual_keyboard, gt_vkey_pressed, 0);
				//input_sync(virtual_keyboard);
				;
			}
		}
	}
}

/*******************************************************
Function:
	Goodix touchscreen work function.

Input:
	work:	work_struct of goodix_wq.
	
Output:
	None.
*******************************************************/
static void goodix_ts_work_func(struct work_struct *work)
{
        u8  end_cmd[3] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF, 0};
        u8  point_data[2 + 1 + 8 * GTP_MAX_TOUCH + 1]={GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF};
        u8  touch_num = 0;
        u8  finger = 0;
        static u16 pre_touch = 0;
        static u8 pre_key = 0;
        u8  key_value = 0;
        u8* coor_data = NULL;
        s32 input_x = 0;
        s32 input_y = 0;
        s32 input_w = 0;
        s32 id = 0;
        s32 i  = 0;
        s32 ret = -1;
        struct goodix_ts_data *ts = NULL;

        dprintk(DEBUG_X_Y_INFO,"===enter %s===\n",__func__);

        ts = container_of(work, struct goodix_ts_data, work);
        if (ts->enter_update){
                return;
        }

        ret = gtp_i2c_read(ts->client, point_data, 12);
        if (ret < 0){
                printk("I2C transfer error. errno:%d\n ", ret);
                goto exit_work_func;
        }

        finger = point_data[GTP_ADDR_LENGTH];    
        if((finger & 0x80) == 0) {
                goto exit_work_func;
        }

        touch_num = finger & 0x0f;
        if (touch_num > GTP_MAX_TOUCH) {
                goto exit_work_func;
        }

        if (touch_num > 1) {
                u8 buf[8 * GTP_MAX_TOUCH] = {(GTP_READ_COOR_ADDR + 10) >> 8, (GTP_READ_COOR_ADDR + 10) & 0xff};

                ret = gtp_i2c_read(ts->client, buf, 2 + 8 * (touch_num - 1)); 
                memcpy(&point_data[12], &buf[2], 8 * (touch_num - 1));
        }

#if GTP_HAVE_TOUCH_KEY
        key_value = point_data[3 + 8 * touch_num];
    
        if(key_value || pre_key) {
                for (i = 0; i < GTP_MAX_KEY_NUM; i++) {
                        input_report_key(ts->input_dev, touch_key_array[i], key_value & (0x01<<i));   
                }
                touch_num = 0;
                pre_touch = 0;
        }
#endif
        pre_key = key_value;

        dprintk(DEBUG_X_Y_INFO, "pre_touch:%02x, finger:%02x.", pre_touch, finger);

#if GTP_ICS_SLOT_REPORT
        if (pre_touch || touch_num) {
                s32 pos = 0;
                u16 touch_index = 0;
                coor_data = &point_data[3];
                
                if(touch_num) {
                        id = coor_data[pos] & 0x0F;
                        touch_index |= (0x01<<id);
                }

                dprintk(DEBUG_X_Y_INFO, 
                       "id=%d, touch_index=0x%x, pre_touch=0x%x\n", id, touch_index, pre_touch);
                
                for (i = 0; i < GTP_MAX_TOUCH; i++) {
                        if (touch_index & (0x01<<i)) {
                                input_x  = coor_data[pos + 1] | coor_data[pos + 2] << 8;
                                input_y  = coor_data[pos + 3] | coor_data[pos + 4] << 8;
                                input_w  = coor_data[pos + 5] | coor_data[pos + 6] << 8;

                                gtp_touch_down(ts, id, input_x, input_y, input_w);
                                pre_touch |= 0x01 << i;

                                pos += 8;
                                id = coor_data[pos] & 0x0F;
                                touch_index |= (0x01<<id);
                        }else {// if (pre_touch & (0x01 << i))
            
                                gtp_touch_up(ts, i);
                                pre_touch &= ~(0x01 << i);
                        }
                }
        }

#else
        if (touch_num ) {
                for (i = 0; i < touch_num; i++) {
                        coor_data = &point_data[i * 8 + 3];

                        id = coor_data[0] & 0x0F;
                        input_x  = coor_data[1] | coor_data[2] << 8;
                        input_y  = coor_data[3] | coor_data[4] << 8;
                        input_w  = coor_data[5] | coor_data[6] << 8;

                        gtp_touch_down(ts, id, input_x, input_y, input_w);
                }
        }else if(pre_touch){
                dprintk(DEBUG_X_Y_INFO, "Touch Release!");
                gtp_touch_up(ts, 0);
        }
        
        pre_touch = touch_num;

#endif

        input_sync(ts->input_dev);

exit_work_func:
        if(!ts->gtp_rawdiff_mode) {
                ret = gtp_i2c_write(ts->client, end_cmd, 3);
                if (ret < 0) {
                        printk("I2C write end_cmd  error!"); 
                }
        }
        return ;
}

/*******************************************************
Function:
	External interrupt service routine.

Input:
	irq:	interrupt number.
	dev_id: private data pointer.
	
Output:
	irq execute status.
*******************************************************/

static u32 goodix_ts_irq_handler(struct goodix_ts_data *ts)
{
        //printk("==========------TS Interrupt-----============\n");
        //queue_work(goodix_wq, &ts->work);
        //LANDSEM
        queue_work(goodix_wq, work_test);
        //schedule_work(&work_test);
        //LANDSEM
        return 0;
}
/*******************************************************
Function:
	Request irq Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_request_irq(struct goodix_ts_data *ts)
{   
    //int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)goodix_ts_irq_handler,ts);
    int_handle = gt_request_int(&(ts->input_dev->dev), CTP_IRQ_NUMBER, goodix_ts_irq_handler, IRQF_TRIGGER_FALLING, ts);
   	if (int_handle) {
		pr_info( "goodix_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}


	//LANDSEM
	//ctp_set_int_port_rate(config_info.int_number, 1);
	//ctp_set_int_port_deb(config_info.int_number, 0x07);
	//LANDSEM
	dprintk(DEBUG_INIT,"reg clk: 0x%08x\n", readl(0xf1c20a18));

	return 0;
	
exit_irq_request_failed:
        gt_free_int(&(ts->input_dev->dev), CTP_IRQ_NUMBER, ts);
        return -1;

      
}


/*******************************************************
Function:
	Eter sleep function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_enter_sleep(struct goodix_ts_data * ts)
{
        s8 ret = -1;
        s8 retry = 0;
        u8 i2c_control_buf[3] = {(u8)(GTP_REG_SLEEP >> 8), (u8)GTP_REG_SLEEP, 5};
        
        dprintk(DEBUG_SUSPEND, "%s start!\n", __func__);
        
        gtp_set_int_value(0);

        while(retry++ < 2) {
                ret = gtp_i2c_write(ts->client, i2c_control_buf, 3);
                if (ret > 0) {
                        dprintk(DEBUG_SUSPEND, "GTP enter sleep!\n");
                        return ret;
                }
                msleep(10);
        }
        dprintk(DEBUG_SUSPEND, "GTP send sleep cmd failed.");
        
        return ret;
}

/*******************************************************
Function:
	Wakeup from sleep mode Function.

Input:
	ts:	private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_wakeup_sleep(struct goodix_ts_data * ts)
{
        u8 retry = 0;
        s8 ret = -1;
        
        gtp_io_init(50);
		msleep(50);
        gtp_set_io_int();
       
#if GTP_POWER_CTRL_SLEEP
    while(retry++ < 5)
    {
        ret = gtp_send_cfg(ts->client);
        if (ret > 0)
        {
            dprintk(DEBUG_SUSPEND, "Wakeup sleep send config success.");
            return ret;
        }
    }

    printk("GTP wakeup sleep failed.");
    return ret;
#endif

}


/*******************************************************
Function:
	GTP initialize function.

Input:
	ts:	i2c client private struct.
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
static s32 gtp_init_panel(struct goodix_ts_data *ts)
{
        s32 ret = -1;
		u16 ver=0;
        
#if GTP_DRIVER_SEND_CFG
        s32 i;
        u8 check_sum = 0;
        dprintk(DEBUG_INIT, "******%s start!\n******", __func__); 
        u8 rd_cfg_buf[16];
        int index = 0;
        
//        u8 cfg_info_group1[] = CTP_CFG_GROUP1;
//        u8 cfg_info_group2[] = CTP_CFG_GROUP2;
//        u8 cfg_info_group3[] = CTP_CFG_GROUP3;
//        u8 *send_cfg_buf[3] = {cfg_info_group1, cfg_info_group2, cfg_info_group3};
//        u8 cfg_info_len[3] = {sizeof(cfg_info_group1)/sizeof(cfg_info_group1[0]), 
//                              sizeof(cfg_info_group2)/sizeof(cfg_info_group2[0]),
//                              sizeof(cfg_info_group3)/sizeof(cfg_info_group3[0])};
//   
//         for(i=0; i<3; i++) {
//                if(cfg_info_len[i] > ts->gtp_cfg_len) {
//                        ts->gtp_cfg_len = cfg_info_len[i];
//                }
//        }
//        dprintk(DEBUG_INIT, "len1=%d,len2=%d,len3=%d,send_len:%d",
//                  cfg_info_len[0],cfg_info_len[1],cfg_info_len[2],ts->gtp_cfg_len);
//        
//        if ((!cfg_info_len[1]) && (!cfg_info_len[2])) {
//                rd_cfg_buf[GTP_ADDR_LENGTH] = 0; 
//        }else {
//                rd_cfg_buf[0] = GTP_REG_SENSOR_ID >> 8;
//                rd_cfg_buf[1] = GTP_REG_SENSOR_ID & 0xff;
//                ret = gtp_i2c_read(ts->client, rd_cfg_buf, 3);
//                if (ret < 0) {
//                        printk("Read SENSOR ID failed,default use group1 config!");
//                        rd_cfg_buf[GTP_ADDR_LENGTH] = 0;
//                }
//                rd_cfg_buf[GTP_ADDR_LENGTH] &= 0x07;
//        }
//        
//        index = 0;
//	ts->gtp_cfg_len = cfg_info_len[index];

        dprintk(DEBUG_INIT, "SENSOR ID:%d", rd_cfg_buf[GTP_ADDR_LENGTH]);
      dprintk(DEBUG_INIT, "cfg_index:%d\n", cfg_index);
      memset(&config[GTP_ADDR_LENGTH], 0, GTP_CONFIG_MAX_LENGTH);
	 ts->gtp_cfg_len = gt9xx_cfg_grp[cfg_index].size;
        memcpy(&config[GTP_ADDR_LENGTH], gt9xx_cfg_grp[cfg_index].config_info, ts->gtp_cfg_len);
printk("kyleprint: (%d) gtp_cfg_len=%d\r\n", cfg_index, ts->gtp_cfg_len);

//	test(ts->client);

#if GTP_CUSTOM_CFG
        config[RESOLUTION_LOC]     = (u8)GTP_MAX_WIDTH;
        config[RESOLUTION_LOC + 1] = (u8)(GTP_MAX_WIDTH>>8);
        config[RESOLUTION_LOC + 2] = (u8)GTP_MAX_HEIGHT;
        config[RESOLUTION_LOC + 3] = (u8)(GTP_MAX_HEIGHT>>8);
    
        if (GTP_INT_TRIGGER == 0) {  //RISING
        
                config[TRIGGER_LOC] &= 0xfe; 
        }else if (GTP_INT_TRIGGER == 1) {  //FALLING
    
                config[TRIGGER_LOC] |= 0x01;
        }
#endif  //endif GTP_CUSTOM_CFG
    
        check_sum = 0;
        for (i = GTP_ADDR_LENGTH; i < (ts->gtp_cfg_len+GTP_ADDR_LENGTH); i++) {
                check_sum += config[i];
        }
        config[ts->gtp_cfg_len+GTP_ADDR_LENGTH] = (~check_sum) + 1;
    
#else //else DRIVER NEED NOT SEND CONFIG

        if(ts->gtp_cfg_len == 0) {
                ts->gtp_cfg_len = GTP_CONFIG_MAX_LENGTH;
        }
        
        ret = gtp_i2c_read(ts->client, config, ts->gtp_cfg_len + GTP_ADDR_LENGTH);
        if (ret < 0) {
                printk("GTP read resolution & max_touch_num failed, use default value!");
                ts->abs_x_max = GTP_MAX_WIDTH;
                ts->abs_y_max = GTP_MAX_HEIGHT;
                ts->int_trigger_type = GTP_INT_TRIGGER;
        }
#endif //endif GTP_DRIVER_SEND_CFG

        GTP_DEBUG_FUNC();

        ts->abs_x_max = (config[RESOLUTION_LOC + 1] << 8) + config[RESOLUTION_LOC];
        ts->abs_y_max = (config[RESOLUTION_LOC + 3] << 8) + config[RESOLUTION_LOC + 2];
        ts->int_trigger_type = (config[TRIGGER_LOC]) & 0x03;
        if ((!ts->abs_x_max)||(!ts->abs_y_max)) {
                printk("GTP resolution & max_touch_num invalid, use default value!");
                ts->abs_x_max = GTP_MAX_WIDTH;
                ts->abs_y_max = GTP_MAX_HEIGHT;
        }
#if 0
	test_cfg_len = ts->gtp_cfg_len;
        ret = gtp_send_cfg(ts->client);
        if (ret < 0) {
                GTP_ERROR("Send config error.");
		printk("kyleprint: Send config error.\r\n");
        }
		printk("kyleprint: X_MAX = %d,Y_MAX = %d,TRIGGER = 0x%02x\r\n", ts->abs_x_max,ts->abs_y_max,ts->int_trigger_type);
#endif
        dprintk(DEBUG_INIT, "X_MAX = %d,Y_MAX = %d,TRIGGER = 0x%02x",
                 ts->abs_x_max,ts->abs_y_max,ts->int_trigger_type);

        msleep(10);
        
        dprintk(DEBUG_INIT, "******%s start!\n******", __func__); 

        return 0;
}

/*******************************************************
Function:
	Read goodix touchscreen version function.

Input:
	client:	i2c client struct.
	version:address to store version info
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
s32 gtp_read_version(struct i2c_client *client, u16* version)
{
        s32 ret = -1;
        u8 buf[8] = {GTP_REG_VERSION >> 8, GTP_REG_VERSION & 0xff};

        dprintk(DEBUG_INIT, "%s ---start!.---\n", __func__);

        ret = gtp_i2c_read(client, buf, sizeof(buf));
        if (ret < 0) {
                printk("GTP read version failed");
                return ret;
        }

        if (version) {
                *version = (buf[7] << 8) | buf[6];
        }

        if (buf[5] == 0x00) {
                printk("IC Version: %c%c%c_%02x%02x", buf[2], buf[3], buf[4], buf[7], buf[6]);
        }
        else {
                printk("IC Version: %c%c%c%c_%02x%02x", buf[2], buf[3], buf[4], buf[5], buf[7], buf[6]);
        }
        return ret;
}

/*******************************************************
Function:
	I2c test Function.

Input:
	client:i2c client.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
#if 0
static s8 gtp_i2c_test(struct i2c_client *client)
{
        u8 test[3] = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
        u8 retry = 0;
        s8 ret = -1;
  
        while(retry++ < 2) {
                ret = gtp_i2c_read(client, test, 3);
                if (ret > 0) {
                        return ret;
                }
                printk("GTP i2c test failed time %d.",retry);
                msleep(10);
        }
        return ret;
}
#endif

/*******************************************************
Function:
	Request input device Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_request_input_dev(struct goodix_ts_data *ts)
{
        s8 ret = -1;
        //s8 phys[32];
#if GTP_HAVE_TOUCH_KEY
        u8 index = 0;
#endif
  
        ts->input_dev = input_allocate_device();
        if (ts->input_dev == NULL) {
                GTP_ERROR("Failed to allocate input device.");
                return -ENOMEM;
        }

        ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
#if GTP_ICS_SLOT_REPORT
        __set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
        input_mt_init_slots(ts->input_dev, 255);
#else
        ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif

#if GTP_HAVE_TOUCH_KEY
        for (index = 0; index < GTP_MAX_KEY_NUM; index++) {
                input_set_capability(ts->input_dev,EV_KEY,touch_key_array[index]);	
        }
#endif

//#if GTP_CHANGE_X2Y
//        GTP_SWAP(ts->abs_x_max, ts->abs_y_max);
//#endif
	//set_bit(gt_vkey[GT_VK_IND_HOME][2], ts->input_dev->keybit);
	//set_bit(gt_vkey[GT_VK_IND_BACK][2], ts->input_dev->keybit);
	//set_bit(gt_vkey[GT_VK_IND_VOLADD][2], ts->input_dev->keybit);
	//set_bit(gt_vkey[GT_VK_IND_VOLDEC][2], ts->input_dev->keybit);
	if (1==gt_tptype || 2==gt_tptype || 3==gt_tptype || 5==gt_tptype || 7==gt_tptype || 8==gt_tptype)
	{//6.95,,,7,,,10.1
		;
		/*
		KYLE_DEBUG("virtual_keyboard is %p", virtual_keyboard);
		input_set_capability(virtual_keyboard,EV_KEY,gt_vkey[GT_VK_IND_HOME][2]);
		input_set_capability(virtual_keyboard,EV_KEY,gt_vkey[GT_VK_IND_BACK][2]);
		input_set_capability(virtual_keyboard,EV_KEY,gt_vkey[GT_VK_IND_VOLADD][2]);
		input_set_capability(virtual_keyboard,EV_KEY,gt_vkey[GT_VK_IND_VOLDEC][2]);
		*/
	}
#endif
        input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
        input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
        input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
        input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);	
        input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 255, 0, 0);
	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
    
	//sprintf(phys, "input/ts");
        ts->input_dev->name = goodix_ts_name;
        //ts->input_dev->phys = phys;
		ts->input_dev->phys = "input/goodix-ts";
        ts->input_dev->id.bustype = BUS_I2C;
        ts->input_dev->id.vendor = 0xDEAD;
        ts->input_dev->id.product = 0xBEEF;
        ts->input_dev->id.version = 10427;
        ret = input_register_device(ts->input_dev);
        if (ret) {
                printk("Register %s input device failed", ts->input_dev->name);
                return -ENODEV;
        }
    
#ifdef CONFIG_HAS_EARLYSUSPEND
        ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
        ts->early_suspend.suspend = goodix_ts_early_suspend;
        ts->early_suspend.resume = goodix_ts_late_resume;
        register_early_suspend(&ts->early_suspend);
#endif

        
        return 0;
}


/*******************************************************
Function:
	Goodix touchscreen probe function.

Input:
	client:	i2c device struct.
	id:device id.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
static ssize_t wangrong_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);;
	int retry, ret = 0;
	printk("wangrong_show\n");
	printk("-----------------------------\n");

	memset(config3+2, 0, sizeof(GTP_CONFIG_MAX_LENGTH));
	ret = gtp_i2c_read(client, config3, GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH +2);
	printk("ret === %d\n", ret);
	for (retry = 0; retry < (GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH +2); retry++)
	{
		printk("0x%02X,", config3[retry]);
	}
	printk("\r\n");
	printk("-----------------------------\n");
	return 0;
}
static ssize_t wangrong_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);;
	int retry, ret = 0;
	printk("wangrong_store\n");
	printk("-----------------------------\n");
	
	for (retry = 0; retry < (GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH +2); retry++)
	{
		printk("0x%02X,", config[retry]);
	}
	printk("\r\n");
	printk("-----------------------------\n");

	ret = gtp_i2c_write(client, config, GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH +2);
	printk("ret === %d\n", ret);

	return size;
}

static DEVICE_ATTR(wangrong, S_IRWXUGO, wangrong_show, wangrong_store);
static int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        s32 ret = -1;
        struct goodix_ts_data *ts;
        u16 version_info;   
    
        dprintk(DEBUG_INIT, "GTP Driver Version:%s\n", GTP_DRIVER_VERSION);
        dprintk(DEBUG_INIT, "GTP Driver build@%s,%s\n", __TIME__,__DATE__);
        dprintk(DEBUG_INIT, "GTP I2C Address:0x%02x\n", client->addr);
		//set_tp_detect_flag(1);//set tp flag

/*LANDSEM@liuxueneng 20160928 modify for ctp ctl start*/
	create_tpoffstatus_class();
/*LANDSEM@liuxueneng 20160928 modify for ctp ctl start*/
        i2c_connect_client = client;
        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
                printk("I2C check functionality failed.");
                return -ENODEV;
        }
        
        ts = kzalloc(sizeof(*ts), GFP_KERNEL);
        if (ts == NULL) {
                printk("Alloc GFP_KERNEL memory failed.");
                return -ENOMEM;
        }
   
        memset(ts, 0, sizeof(*ts));
        INIT_WORK(&ts->work, goodix_ts_work_func);
		work_test = &ts->work;
		//printk("----------work_test 0x%x ------------\n", work_test);
        ts->client = client;
        i2c_set_clientdata(client, ts);
        //ts->irq_lock = SPIN_LOCK_UNLOCKED;
        ts->gtp_rawdiff_mode = 0;


//        ret = gtp_i2c_test(client);
//        if (ret < 0){
//                printk("I2C communication ERROR!");
//		goto exit_device_detect;
//        }

	goodix_resume_wq = create_singlethread_workqueue("goodix_resume");
	if (goodix_resume_wq == NULL) {
		printk("create goodix_resume_wq fail!\n");
		return -ENOMEM;
	}

	goodix_wq = create_singlethread_workqueue("goodix_wq");
	if (!goodix_wq) {
		printk(KERN_ALERT "Creat goodix_wq workqueue failed.\n");
		return -ENOMEM;
	}

#if GTP_AUTO_UPDATE
        ret = gup_init_update_proc(ts);
        if (ret < 0) {
                printk("Create update thread error.");
        }
#endif

        ret = gtp_init_panel(ts);
        if (ret < 0) {
                printk("GTP init panel failed.");
        }

    
	ret = gtp_request_input_dev(ts);
        if (ret < 0) {
                printk("GTP request input dev failed");
		goto exit_device_detect;
        }

     ret = gtp_request_irq(ts); 
     if (ret < 0) {
               printk("Request irq fail!.\n");
     }

        ret = gtp_read_version(client, &version_info);
        if (ret < 0) {
                printk("Read version failed.");
        }
 		printk("read version delete\n");

        spin_lock_init(&ts->irq_lock);
		ret = device_create_file(&client->dev, &dev_attr_wangrong);
		if(ret < 0){
			printk("[ch7026]device create file screen fail\n");
			return -1;
		}
#if 0	//Kyle added for new tp
		msleep(3000);
		write_config(client);	//added for config cannot download;
#endif
#if GTP_CREATE_WR_NODE
        init_wr_node(client);
#endif
//	test(client);

#if GTP_ESD_PROTECT
        INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
        gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
        queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE); 
#endif

	dprintk(DEBUG_INIT, "gt9xx probe success!\n");
        return 0;
exit_device_detect:
	i2c_set_clientdata(client, NULL);
	kfree(ts);
	return ret;
}


/*******************************************************
Function:
	Goodix touchscreen driver release function.

Input:
	client:	i2c device struct.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
static int goodix_ts_remove(struct i2c_client *client)
{
        struct goodix_ts_data *ts = i2c_get_clientdata(client);
	
	dprintk(DEBUG_INIT,"%s start!\n", __func__);
		
#ifdef CONFIG_HAS_EARLYSUSPEND
        unregister_early_suspend(&ts->early_suspend);
#endif

#if GTP_CREATE_WR_NODE
        uninit_wr_node();
#endif

#if GTP_ESD_PROTECT
        destroy_workqueue(gtp_esd_check_workqueue);
#endif
	
    gt_free_int(&(ts->input_dev->dev), CTP_IRQ_NUMBER, ts);
	flush_workqueue(goodix_wq);
	//cancel_work_sync(&goodix_init_work);
  	cancel_work_sync(&goodix_resume_work);
	destroy_workqueue(goodix_wq);
  	//destroy_workqueue(goodix_init_wq);
  	destroy_workqueue(goodix_resume_wq);
  	i2c_set_clientdata(ts->client, NULL);
	input_unregister_device(ts->input_dev);
	input_free_device(ts->input_dev);
	kfree(ts);
	
/*LANDSEM@liuxueneng 20160928 modify for ctp ctl start*/
        tpoffstatus_class_exit();
/*LANDSEM@liuxueneng 20160928 modify for ctp ctl end*/

    return 0;
}

static void goodix_resume_events (struct work_struct *work)
{
	int ret;
    struct goodix_ts_data *ts = i2c_get_clientdata(i2c_connect_client);
    dprintk(DEBUG_SUSPEND, "******%s start!*****\n", __func__);
    dprintk(DEBUG_SUSPEND, "GTP I2C Address:0x%02x\n", ts->client->addr);
    
	ret = gtp_wakeup_sleep(ts);
	if (ret < 0)
		printk("resume power on failed\n");
	//gt_set_int_enable(CTP_IRQ_NUMBER, 1);
	gtp_request_irq(ts);
}

/*******************************************************
Function:
	Early suspend function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
        struct goodix_ts_data *ts;
        s8 ret = -1;	
        ts = container_of(h, struct goodix_ts_data, early_suspend);

        dprintk(DEBUG_SUSPEND, "******%s start!*****\n", __func__);
#if GTP_ESD_PROTECT
        ts->gtp_is_suspend = 1;
        cancel_delayed_work_sync(&gtp_esd_check_work);
#endif

        gt_set_int_enable(CTP_IRQ_NUMBER, 0);
        cancel_work_sync(&goodix_resume_work);
  	flush_workqueue(goodix_resume_wq);
        ret = cancel_work_sync(&ts->work);
        flush_workqueue(goodix_wq);
    
	ret = gtp_enter_sleep(ts);
        if (ret < 0) {
                printk("GTP early suspend failed.");
        }
}

/*******************************************************
Function:
	Late resume function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
static void goodix_ts_late_resume(struct early_suspend *h)
{
        struct goodix_ts_data *ts;
        ts = container_of(h, struct goodix_ts_data, early_suspend);
	dprintk(DEBUG_SUSPEND, "******%s start!*****\n", __func__);
        queue_work(goodix_resume_wq, &goodix_resume_work);//gandy

#if GTP_ESD_PROTECT
        ts->gtp_is_suspend = 0;
        queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
#endif
}
#else
#ifdef CONFIG_PM
static int goodix_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
        struct goodix_ts_data *ts;
        s8 ret = -1;	
        ts = i2c_get_clientdata(client);
        dprintk(DEBUG_SUSPEND, "******%s start!*****\n", __func__);

#if GTP_ESD_PROTECT
        ts->gtp_is_suspend = 1;
        cancel_delayed_work_sync(&gtp_esd_check_work);
#endif

        gt_set_int_enable(CTP_IRQ_NUMBER, 0);
        cancel_work_sync(&goodix_resume_work);
  	flush_workqueue(goodix_resume_wq);
        ret = cancel_work_sync(&ts->work);
        flush_workqueue(goodix_wq);
    
	ret = gtp_enter_sleep(ts);
        if (ret < 0) {
                printk("GTP suspend failed.");
        }
	
        ctp_wakeup(config_info.wakeup_gpio.gpio, 0, 0);
        gtp_set_int_value(0);
		if(irq_hold_flag == 1){
			gt_free_int(&(ts->input_dev->dev), CTP_IRQ_NUMBER, ts);
		}
		return 0;
}

static int goodix_ts_resume(struct i2c_client *client)
{
        struct goodix_ts_data *ts;
        ts = i2c_get_clientdata(client);
	dprintk(DEBUG_SUSPEND, "******%s start!*****\n", __func__);
        queue_work(goodix_resume_wq, &goodix_resume_work);//gandy

#if GTP_ESD_PROTECT
        ts->gtp_is_suspend = 0;
        queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
#endif
	return 0;
}
#endif
#endif

#if GTP_ESD_PROTECT
/*******************************************************
Function:
    Initialize external watchdog for esd protect
Input:
    client:  i2c device.
Output:
    result of i2c write operation. 
        1: succeed, otherwise: failed
*********************************************************/
s32 gtp_init_ext_watchdog(struct i2c_client *client)
{
        u8 opr_buffer[4] = {0x80, 0x40, 0xAA, 0xAA};
        dprintk(DEBUG_INIT, "Init external watchdog...");
        return gtp_i2c_write(client, opr_buffer, 4);
}
/*******************************************************
Function:
    Esd protect function.
    Added external watchdog by meta, 2013/03/07
Input:
    work: delayed work
Output:
    None.
*******************************************************/
static void gtp_esd_check_func(struct work_struct *work)
{
        s32 i;
        s32 ret = -1;
        struct goodix_ts_data *ts = NULL;
        u8 test[4] = {0x80, 0x40};
        
        dprintk(DEBUG_INIT, "enter %s work!\n", __func__);

         ts = i2c_get_clientdata(i2c_connect_client);

        if (ts->gtp_is_suspend || ts->enter_update) {
                return;
        }
    
        for (i = 0; i < 3; i++) {
                ret = gtp_i2c_read(ts->client, test, 4);
        
                dprintk(DEBUG_INIT, "0x8040 = 0x%02X, 0x8041 = 0x%02X", test[2], test[3]);
                if ((ret < 0)) {
                        // IC works abnormally..
                        continue;
                }else { 
                        if ((test[2] == 0xAA) || (test[3] != 0xAA)) {
                                // IC works abnormally..
                                i = 3;
                                break;  
                        }else {
                                // IC works normally, Write 0x8040 0xAA
                                test[2] = 0xAA; 
                                gtp_i2c_write(ts->client, test, 3);
                                break;
                        }
                }
        }
        
        if (i >= 3) {
                printk("IC Working ABNORMALLY, Resetting Guitar...");
                gtp_reset_guitar(ts->client, 50);
        }

        if(!ts->gtp_is_suspend) {
                queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
        }

        return;
}
#endif

static const struct i2c_device_id goodix_ts_id[] = {
        { CTP_NAME, 0 },
        { }
};

static struct i2c_driver goodix_ts_driver = {
        .class          = I2C_CLASS_HWMON,
        .probe          = goodix_ts_probe,
        .remove         = goodix_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_PM
        .suspend        = goodix_ts_suspend,
        .resume         = goodix_ts_resume,
#endif
#endif
        .id_table       = goodix_ts_id,
        .driver = {
                .name   = CTP_NAME,
                .owner  = THIS_MODULE,
        },
        .address_list	= normal_i2c,
};
/*
static script_item_u get_para_value(char* keyname, char* subname)
{
        script_item_u	val;
        script_item_value_type_e type = 0;

        
        if((!keyname) || (!subname)) {
               printk("keyname:%s  subname:%s \n", keyname, subname);
               goto script_get_item_err;
        }
                
        memset(&val, 0, sizeof(val));
        type = script_get_item(keyname, subname, &val);
         
        if((SCIRPT_ITEM_VALUE_TYPE_INT != type) && (SCIRPT_ITEM_VALUE_TYPE_STR != type) &&
          (SCIRPT_ITEM_VALUE_TYPE_PIO != type)) {
	        goto script_get_item_err;
	}
	
        return val;
        
script_get_item_err:
        printk("keyname:%s  subname:%s ,get error!\n", keyname, subname);
        val.val = -1;
	return val;
}

static void get_str_para(char* name[], char* value[], int num)
{
        script_item_u	val;
        int ret = 0;
        
        if(num < 1) {
                printk("%s: num:%d ,error!\n", __func__, num);
                return;
        }
        
        while(ret < num)
        {     
                val = get_para_value(name[0],name[ret+1]); 
                if(val.val == -1) {                        
                        val.val = 0;
                        val.str = "";
                }
                ret ++;
                *value = val.str;
                
                if(ret < num)
                        value++;

        }
}
*/
static int ctp_get_system_config(void)
{   
        char *name[] = {"ctp_para", "ctp_name"};
		//LANDSEM
		/*
        get_str_para(name, &cfgname, 1);
        dprintk(DEBUG_INIT,"%s:cfgname:%s\n",__func__,cfgname);
        cfg_index = gtp_find_cfg_idx(cfgname);
        if (cfg_index == -1) {
        	printk("gt82x: no matched TP cfgware(%s)!\n", cfgname);
        	return 0;
        }
        printk("kyleprint: (%d) %s\r\n", cfg_index, cfgname);
		*/
		//LANDSEM
		
        twi_id = config_info.twi_id;
	 //kylehu modify for touch panel max error 2014-10-29
	 /*
        screen_max_x = lcdparam.lcd_x;		//config_info.screen_max_x;
        screen_max_y = lcdparam.lcd_y;		//config_info.screen_max_y;
     */
     	//LANDSEM
     	screen_max_x = config_info.screen_max_x;
        screen_max_y = config_info.screen_max_y;
		//LANDSEM
        revert_x_flag = config_info.revert_x_flag;
        revert_y_flag = config_info.revert_y_flag;
        exchange_x_y_flag = config_info.exchange_x_y_flag;

	KYLE_DEBUG("screen_max_x %d, screen_max_y %d\r\n", screen_max_x, screen_max_y);

        if((twi_id == 0) || (screen_max_x == 0) || (screen_max_y == 0)){
                printk("%s:read config error!\n",__func__);
                return 0;
        }
        return 1;
}

/*******************************************************	
Function:
	Driver Install function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static int __init goodix_ts_init(void)
{
    s32 ret = -1;
	//CarConfig *lCarConfig;
	
	printk("kyleprint: >>> goodix_ts_init\n");
	/*
	if(get_tp_detect_flag() > 0){
		printk("get_tp_detect_flag > 0, already detect an other tp\n");
		return -1;
	}
	*/
    if (input_fetch_sysconfig_para(&(config_info.input_type))) {
		printk("%s: ctp_fetch_sysconfig_para err.\n", __func__);
		return 0;
	} else {
		ret = input_init_platform_resource(&(config_info.input_type));
		if (0 != ret) {
			printk("%s:ctp_ops.init_platform_resource err. \n", __func__);    
		}
	}
        
        if(config_info.ctp_used == 0){
	        printk("*** ctp_used set to 0 !\n");
	        printk("*** if use ctp,please put the sys_config.fex ctp_used set to 1. \n");
	        return 0;
	}
	
        if(!ctp_get_system_config()){
                printk("%s:read config fail!\n",__func__);
                return ret;
        }
	ret = gpio_request(config_info.irq_gpio.gpio, "ctp-int");
	if(ret < 0){
			printk("%s:requst irq gpio fail!\n",__func__);
	}
	//KYLE_DEBUG("lCarConfig->tpsize --- %s", lCarConfig->tpsize);
	/*
	lCarConfig = get_gcarconfig();
	gt_tptype = 0;
	sscanf(lCarConfig->tpsize, "*%d*", &gt_tptype);
	if (gt_tptype<0)
	{
		gt_tptype = 0;
	}
	KYLE_DEBUG("**gt_tptype**%d**\r\n", gt_tptype);
	switch(gt_tptype)
	{
		case 0:
			break;
		case 1: //6.95
			virtual_key_xoffset = VIRTUAL_KEY_XOFFSET;
			gt_vkey[GT_VK_IND_EJECT][0] = 20;
			gt_vkey[GT_VK_IND_EJECT][1] = 35;
			gt_vkey[GT_VK_IND_HOME][0] = 20;
			gt_vkey[GT_VK_IND_HOME][1] = 135;
			gt_vkey[GT_VK_IND_BACK][0] = 20;
			gt_vkey[GT_VK_IND_BACK][1] = 240;
			gt_vkey[GT_VK_IND_VOLADD][0] = 20;
			gt_vkey[GT_VK_IND_VOLADD][1] = 340;
			gt_vkey[GT_VK_IND_VOLDEC][0] = 20;
			gt_vkey[GT_VK_IND_VOLDEC][1] = 445;
			break;
		case 2: //7,,,圆方通
			virtual_key_xoffset = VIRTUAL_KEY_XOFFSET;
			gt_vkey[GT_VK_IND_EJECT][0] = 20;
			gt_vkey[GT_VK_IND_EJECT][1] = 55;
			gt_vkey[GT_VK_IND_HOME][0] = 20;
			gt_vkey[GT_VK_IND_HOME][1] = 175;
			gt_vkey[GT_VK_IND_BACK][0] = 20;
			gt_vkey[GT_VK_IND_BACK][1] = 290;
			gt_vkey[GT_VK_IND_VOLADD][0] = 20;
			gt_vkey[GT_VK_IND_VOLADD][1] = 400;
			gt_vkey[GT_VK_IND_VOLDEC][0] = 20;
			gt_vkey[GT_VK_IND_VOLDEC][1] = 510;
			break;
		case 3: //10.1,,,迈腾
			virtual_key_xoffset = VIRTUAL_KEY_XOFFSET;
			gt_vkey[GT_VK_IND_EJECT][0] = 20;
			gt_vkey[GT_VK_IND_EJECT][1] = 75;
			gt_vkey[GT_VK_IND_HOME][0] = 20;
			gt_vkey[GT_VK_IND_HOME][1] = 195;
			gt_vkey[GT_VK_IND_BACK][0] = 20;
			gt_vkey[GT_VK_IND_BACK][1] = 310;
			gt_vkey[GT_VK_IND_VOLADD][0] = 20;
			gt_vkey[GT_VK_IND_VOLADD][1] = 430;
			gt_vkey[GT_VK_IND_VOLDEC][0] = 20;
			gt_vkey[GT_VK_IND_VOLDEC][1] = 545;
			break;
		case 4: //10.1,,,MAX
			break;
		case 5: //10.1,,,凯美瑞
			virtual_key_xoffset = VIRTUAL_KEY_XOFFSET;
			gt_vkey[GT_VK_IND_EJECT][0] = 20;
			gt_vkey[GT_VK_IND_EJECT][1] = 60;
			gt_vkey[GT_VK_IND_HOME][0] = 20;
			gt_vkey[GT_VK_IND_HOME][1] = 170;
			gt_vkey[GT_VK_IND_BACK][0] = 20;
			gt_vkey[GT_VK_IND_BACK][1] = 270;
			gt_vkey[GT_VK_IND_VOLADD][0] = 20;
			gt_vkey[GT_VK_IND_VOLADD][1] = 375;
			gt_vkey[GT_VK_IND_VOLDEC][0] = 20;
			gt_vkey[GT_VK_IND_VOLDEC][1] = 480;
			break;
		case 6:
			break;
		case 7: //8,,,爱行
			virtual_key_xoffset = 80;
			gt_vkey[GT_VK_IND_EJECT][0] = 30;
			gt_vkey[GT_VK_IND_EJECT][1] = 95;
			gt_vkey[GT_VK_IND_HOME][0] = 30;
			gt_vkey[GT_VK_IND_HOME][1] = 210;
			gt_vkey[GT_VK_IND_BACK][0] = 30;
			gt_vkey[GT_VK_IND_BACK][1] = 330;
			gt_vkey[GT_VK_IND_VOLADD][0] = 30;
			gt_vkey[GT_VK_IND_VOLADD][1] = 450;
			gt_vkey[GT_VK_IND_VOLDEC][0] = 30;
			gt_vkey[GT_VK_IND_VOLDEC][1] = 570;
			break;
		case 8: //7,,,爱行
			virtual_key_xoffset = VIRTUAL_KEY_XOFFSET;
			gt_vkey[GT_VK_IND_EJECT][0] = 20;
			gt_vkey[GT_VK_IND_EJECT][1] = 55;
			gt_vkey[GT_VK_IND_HOME][0] = 20;
			gt_vkey[GT_VK_IND_HOME][1] = 175;
			gt_vkey[GT_VK_IND_BACK][0] = 20;
			gt_vkey[GT_VK_IND_BACK][1] = 290;
			gt_vkey[GT_VK_IND_VOLADD][0] = 20;
			gt_vkey[GT_VK_IND_VOLADD][1] = 400;
			gt_vkey[GT_VK_IND_VOLDEC][0] = 20;
			gt_vkey[GT_VK_IND_VOLDEC][1] = 510;
			break;
		case 9:
			break;
	}
	cfg_index = 0; //gt_tptype;
	printk("kyleprint: >>> Touch panel %s gt_tptype %d\n", lCarConfig->tpsize, gt_tptype);
	*/
	
    gtp_io_init(1);

	goodix_ts_driver.detect = ctp_detect;
    ret = i2c_add_driver(&goodix_ts_driver);
/*LANDSEM@liuxueneng  20160928 modify for ctp ctl start*/
//	create_tpoffstatus_class();
/*LANDSEM@liuxueneng 20160928 modify for ctp ctl start*/

	return ret; 
}


/*******************************************************	
Function:
	Driver uninstall function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static void __exit goodix_ts_exit(void)
{
        printk("GTP driver exited.");
        i2c_del_driver(&goodix_ts_driver);
        input_free_platform_resource(&(config_info.input_type));

/*LANDSEM@liuxueneng 20160928 modify for ctp ctl start*/
//        tpoffstatus_class_exit();
/*LANDSEM@liuxueneng 20160928 modify for ctp ctl end*/
}

//////////////////////////////////////////////////////////////////////

static ssize_t ctltp_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{

#if KYLE_CODE//LANDSEM@yingxianFei add to disenable kyle code.
	printk("-----ctltp_show %d\r\n", TPOffStatus);

	struct tpoffstatus_dev *sdev = (struct tpoffstatus_dev *)
	dev_get_drvdata(dev);

	if (sdev->tpoffstatus_read) {
		int ret = sdev->tpoffstatus_read(sdev, buf);
		if (ret >= 0)
			return ret;
	}
	memcpy(buf, &sdev->state, sizeof(sdev->state));
	return sizeof(sdev->state);
#else//LANDSEM@yingxianFei code.
   return sprintf(buf, "%d\n", TPOffStatus);
#endif

}

static ssize_t ctltp_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
#if KYLE_CODE//LANDSEM@yingxianFei add to disenable kyle code.
	TPOffStatus = buf[0];
	printk("-----ctltp_store %d\r\n", size );
	printk("%d, %d, %d, %d\r\n", buf[0], buf[1], buf[2], buf[3]);
	struct tpoffstatus_dev *sdev = (struct tpoffstatus_dev *)
	dev_get_drvdata(dev);
	
	if (sdev->tpoffstatus_write) {
		int ret = sdev->tpoffstatus_write(sdev, buf, size);
		if (ret >= 0)
			return ret;
	}

	return size;
#else//LANDSEM@yingxianFei code.
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk(KERN_ERR"Invalid size\n");
		return err;
	}
	TPOffStatus = (int)val;

	return size;
#endif
}


static DEVICE_ATTR(ctl, S_IRWXUGO, ctltp_show, ctltp_store);	//every one control this kind of node

static int tpoffstatus_dev_register(struct tpoffstatus_dev *sdev)
{
	int ret;

	if (!tpoffstatus_class) {
		ret = create_tpoffstatus_class();
		if (ret < 0)
			return ret;
	}
	if(sdev->tpoffstatus_init){
		ret = sdev->tpoffstatus_init(sdev);
		if(ret < 0){
			printk(KERN_ERR "create_tpoffstatus_class: Failed to init gpio %s\n", sdev->name);
			return ret;
		}
	}

	sdev->index = atomic_inc_return(&device_count);
	sdev->dev = device_create(tpoffstatus_class, NULL,
		MKDEV(0, sdev->index), NULL, sdev->name);
	if (IS_ERR(sdev->dev))
		return PTR_ERR(sdev->dev);

	ret = device_create_file(sdev->dev, &dev_attr_ctl);
	if (ret < 0)
		goto err_create_file_1;

	dev_set_drvdata(sdev->dev, sdev);
	sdev->state = 0;
	
	return 0;

err_create_file_1:
	device_destroy(tpoffstatus_class, MKDEV(0, sdev->index));
	printk(KERN_ERR "tpoffstatus_class: Failed to register driver %s\n", sdev->name);

	return ret;
}


static int create_tpoffstatus_class(void)
{
	script_item_value_type_e type = 0;
	script_item_u item_temp;
	
	if (!tpoffstatus_class) {
		tpoffstatus_class = class_create(THIS_MODULE, "tpoffstatus");
		if (IS_ERR(tpoffstatus_class))
			return PTR_ERR(tpoffstatus_class);
		atomic_set(&device_count, 0);
	}

	tpoffstatus_dev_register(&tpstat_tpoffstatus_dev);
	return 0;
}

static void __exit tpoffstatus_class_exit(void)
{

	class_destroy(tpoffstatus_class);
}


static int tpoffstatus_init(struct tpoffstatus_dev *sdev)
{
	TPOffStatus = 0;
	printk("kyleprint: tpoffstatus_init %d\r\n", TPOffStatus);
	return 0;
}

static ssize_t tpoffstatus_write(struct tpoffstatus_dev *sdev, const char *buf, ssize_t size)
{
	TPOffStatus = buf[0];
	printk("kyleprint: tpoffstatus_write %d\r\n", TPOffStatus);
	return 0;
}

static ssize_t tpoffstatus_read(struct tpoffstatus_dev *sdev, char *buf)
{
	buf[0] = TPOffStatus;
	printk("kyleprint: tpoffstatus_read %d\r\n", TPOffStatus);
	return 0;
}

//////////////////////////////////////////////////////////////////////

late_initcall(goodix_ts_init);
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("GTP Series Driver");
MODULE_LICENSE("GPL");

