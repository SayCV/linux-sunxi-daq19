/* drivers/input/touchscreen/gt813_827_828.h
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
 * Version:1.0
 *      V1.0:2012/08/31,first release.
 */

#ifndef _LINUX_GOODIX_TOUCH_H
#define	_LINUX_GOODIX_TOUCH_H

#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/io.h>
//#include <mach/gpio.h>
//#include <linux/earlysuspend.h>

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <linux/irq.h>
#include <linux/gpio.h>
//#include <mach/irqs.h>
//#include <mach/system.h>
//#include <mach/hardware.h>
//#include <mach/sys_config.h>
#include <linux/init-input.h>

//#include <mach/carconfig.h>

static struct work_struct *work_test;

struct goodix_ts_data {
    spinlock_t irq_lock;
    struct i2c_client *client;
    struct input_dev  *input_dev;
    struct hrtimer timer;
    struct work_struct  work;
//    struct early_suspend early_suspend;
    s32 irq_is_disable;
    s32 use_irq;
    u16 abs_x_max;
    u16 abs_y_max;
    u8  max_touch_num;
    u8  int_trigger_type;
    u8  green_wake_mode;
    u8  chip_type;
    u8  enter_update;
    u8  gtp_is_suspend;
    u8  gtp_rawdiff_mode;
    u8  gtp_cfg_len;
};

extern u16 show_len;
extern u16 total_len;
extern struct ctp_config_info config_info;


//***************************PART1:ON/OFF define*******************************
#define GTP_CUSTOM_CFG        0
#define GTP_DRIVER_SEND_CFG   0
#define GTP_HAVE_TOUCH_KEY    0
#define GTP_POWER_CTRL_SLEEP  1
#define GTP_AUTO_UPDATE       0
#define GTP_CHANGE_X2Y        1
#define GTP_ESD_PROTECT       0
//LANDSEM
#define GTP_CREATE_WR_NODE    0
//LANDSEM
#define GTP_ICS_SLOT_REPORT   0

#define GTP_HEADER_FW_UPDATE  0     
#define GTP_SLIDING_WAKEUP    0     

#define GTP_DEBUG_ON          0
#define GTP_DEBUG_ARRAY_ON    0
#define GTP_DEBUG_FUNC_ON     0

//***************************PART2:TODO define**********************************
//STEP_1(REQUIRED):Change config table.
/*TODO: puts the config info corresponded to your TP here, the following is just 
a sample config, send this config should cause the chip cannot work normally*/
//default or float
//dpt L4459 s905






//STEP_2(REQUIRED):Change I/O define & I/O operation mode.
//#define GTP_RST_PORT    S5PV210_GPJ3(6)
//#define GTP_INT_PORT    S5PV210_GPH1(3)
//#define GTP_INT_IRQ     SW_INT_IRQNO_PIO
//#define GTP_INT_CFG     S3C_GPIO_SFN(0xF)
//#ifdef CONFIG_ARCH_SUN4I
//#define CTP_IRQ_NO			(IRQ_EINT21)
//#elif defined CONFIG_ARCH_SUN5I
//#define CTP_IRQ_NO			(IRQ_EINT9)
//#endif
//#define CTP_IRQ_MODE			(TRIG_EDGE_NEGATIVE)

#define GTP_GPIO_AS_INPUT(pin)          do{\
                                            gpio_direction_input(pin);\
                                            s3c_gpio_setpull(pin, S3C_GPIO_PULL_NONE);\
                                        }while(0)
#define GTP_GPIO_AS_INT(pin)            do{\
                                            GTP_GPIO_AS_INPUT(pin);\
                                            s3c_gpio_cfgpin(pin, GTP_INT_CFG);\
                                        }while(0)
#define GTP_GPIO_GET_VALUE(pin)         gpio_get_value(pin)
#define GTP_GPIO_OUTPUT(pin,level)      gpio_direction_output(pin,level)
#define GTP_GPIO_REQUEST(pin, label)    gpio_request(pin, label)
#define GTP_GPIO_FREE(pin)              gpio_free(pin)
#define GTP_IRQ_TAB                     {IRQ_TYPE_EDGE_RISING, IRQ_TYPE_EDGE_FALLING, IRQ_TYPE_LEVEL_LOW, IRQ_TYPE_LEVEL_HIGH}

//STEP_3(optional):Custom set some config by themself,if need.
#if GTP_CUSTOM_CFG
  #define GTP_MAX_HEIGHT   600
  #define GTP_MAX_WIDTH    1024
  #define GTP_INT_TRIGGER  0    //0:Rising 1:Falling
#else
  #define GTP_MAX_HEIGHT   4096
  #define GTP_MAX_WIDTH    4096
  #define GTP_INT_TRIGGER  1
#endif
#define GTP_MAX_TOUCH         5
#define GTP_ESD_CHECK_CIRCLE  2000

//STEP_4(optional):If this project have touch key,Set touch key config.                                    
#if GTP_HAVE_TOUCH_KEY
    #define GTP_KEY_TAB	 {KEY_MENU, KEY_HOME, KEY_BACK, KEY_SEND}
#endif

//***************************PART3:OTHER define*********************************
#define GTP_DRIVER_VERSION    "V1.4<2012/12/12>"
#define GTP_I2C_NAME          "gt9xx_ts"
#define GTP_POLL_TIME         10
#define GTP_ADDR_LENGTH       2
#define GTP_CONFIG_MAX_LENGTH 240
#define FAIL                  0
#define SUCCESS               1

//Register define
#define GTP_READ_COOR_ADDR    0x814E
#define GTP_REG_SLEEP         0x8040
#define GTP_REG_SENSOR_ID     0x814A
#define GTP_REG_CONFIG_DATA   0x8047
#define GTP_REG_VERSION       0x8140

#define RESOLUTION_LOC        3
#define TRIGGER_LOC           8

#define KYLE_DEBUG_ON				(0)

//Log define
#define KYLE_DEBUG(fmt,arg...)	do{\
									if (KYLE_DEBUG_ON)\
									printk("<<-kyleprint->> "fmt, ##arg);\
								}while(0)

#define GTP_INFO(fmt,arg...)           printk("<<-GTP-INFO->> "fmt"\n",##arg)
#define GTP_ERROR(fmt,arg...)          printk("<<-GTP-ERROR->> "fmt"\n",##arg)
#define GTP_DEBUG(fmt,arg...)          do{\
                                         if(GTP_DEBUG_ON)\
                                         printk("<<-GTP-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                       }while(0)
#define GTP_DEBUG_ARRAY(array, num)    do{\
                                         s32 i;\
                                         u8* a = array;\
                                         if(GTP_DEBUG_ARRAY_ON)\
                                         {\
                                            printk("<<-GTP-DEBUG-ARRAY->>\n");\
                                            for (i = 0; i < (num); i++)\
                                            {\
                                                printk("%02x   ", (a)[i]);\
                                                if ((i + 1 ) %10 == 0)\
                                                {\
                                                    printk("\n");\
                                                }\
                                            }\
                                            printk("\n");\
                                        }\
                                       }while(0)
#define GTP_DEBUG_FUNC()               do{\
                                         if(GTP_DEBUG_FUNC_ON)\
                                         printk("<<-GTP-FUNC->> Func:%s@Line:%d\n",__func__,__LINE__);\
                                       }while(0)
#define GTP_SWAP(x, y)                 do{\
                                         typeof(x) z = x;\
                                         x = y;\
                                         y = z;\
                                       }while (0)

//****************************PART4:UPDATE define*******************************
//Error no
#define ERROR_NO_FILE           2   //ENOENT
#define ERROR_FILE_READ         23  //ENFILE
#define ERROR_FILE_TYPE         21  //EISDIR
#define ERROR_GPIO_REQUEST      4   //EINTR
#define ERROR_I2C_TRANSFER      5   //EIO
#define ERROR_NO_RESPONSE       16  //EBUSY
#define ERROR_TIMEOUT           110 //ETIMEDOUT

//*****************************End of Part III********************************

enum GT_VKey_Index
{
	GT_VK_IND_EJECT = 0,
	GT_VK_IND_HOME,
	GT_VK_IND_BACK,
	GT_VK_IND_VOLADD,
	GT_VK_IND_VOLDEC
};

#define VIRTUAL_KEY_NUM			(5)
#define VIRTUAL_KEY_XRADIUS		(20)
#define VIRTUAL_KEY_YRADIUS		(30)
#define VIRTUAL_KEY_XOFFSET		(50)
#define VKEY_CODE_EJECT			(91)
#define VKEY_CODE_HOME				(3)
#define VKEY_CODE_BACK				(4)
#define VKEY_CODE_VOLADD			(24)
#define VKEY_CODE_VOLDEC			(25)

//extern CarConfig *get_gcarconfig();

#endif /* _LINUX_GOODIX_TOUCH_H */
