#ifndef _VFE_SUB_DEVICE_H
   #define _VFE_SUB_DEVICE_H

   #include "sensor_helper.h"

   #ifndef ARRAY_SIZE(x)
        #define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
   #endif
   
   struct cfg_array {	   /* coming later */
	   struct regval_list *regs;
	   int size;
   };

   struct ls_vfe_sub_dev{
   	  unsigned char sensor_slave;/*sensor iic slave address*/
   	  struct cfg_array *sensor_chip_id_regs;/*Sensor chip id regs*/
      	  struct cfg_array *isp_default_regs;/*ISP default init regs.*/
	  struct cfg_array *sensor_default_regs;/*Sensor default init regs.*/
	  struct cfg_array *isp_1080p_regs;/*ISP 1080p regs.*/
	  struct cfg_array *sensor_1080p_regs;/*Sensor 1080p regs.*/
	  struct cfg_array *isp_720p_regs;/*ISP 720p regs.*/
	  struct cfg_array *sensor_720p_regs;/*Sensor 720p regs.*/
	  struct cfg_array *isp_480p_regs;/*ISP 480p regs.*/
	  struct cfg_array *sensor_480p_regs;/*Sensor 480p regs.*/
/*LANDSEM@liuxueneng 20160622 add for ov2718 start********/
	  unsigned char dark_threshold_value_2;/*Switch dark regs when value of gain greater than this value.*/	
	  unsigned char bright_threshold_value_2;/*Switch bright regs when value of gain greater than this value.*/	
/*LANDSEM@liuxueneng 20160622 add for ov2718 start********/
	  unsigned char dark_threshold_value;/*Switch dark regs when value of gain greater than this value.*/	
	  struct cfg_array *isp_dark_regs;/*ISP regs for darkness scene*/
	  struct cfg_array *sensor_dark_regs;/*Sensor regs for darkness scene*/
      	  unsigned char bright_threshold_value;/*Switch bright regs when value of gain less than this value.*/
	  struct cfg_array *isp_bright_regs;/*ISP regs for brightness scene.*/
	  struct cfg_array *sensor_bright_regs; /*Sensor regs for brightness scene*/
	  struct cfg_array *isp_0flip_regs;/*ISP 0flip regs*/
	  struct cfg_array *sensor_0flip_regs;/*Sensor 0flip regs*/
	  struct cfg_array *isp_180flip_regs;/*ISP 180flip regs*/
	  struct cfg_array *sensor_180flip_regs;/*Sensor 180flip regs*/
   };
   
#endif   

