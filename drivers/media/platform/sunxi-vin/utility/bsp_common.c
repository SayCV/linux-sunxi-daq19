/*
 ******************************************************************************
 *
 * bsp_common.c
 *
 * Hawkview ISP - bsp_common.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/12/02	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#include "bsp_common.h"

enum bus_pixeltype find_bus_type(enum v4l2_mbus_pixelcode code)
{
	switch (code) {
	case V4L2_MBUS_FMT_BGR565_2X8_BE:
	case V4L2_MBUS_FMT_BGR565_2X8_LE:
	case V4L2_MBUS_FMT_RGB565_2X8_BE:
	case V4L2_MBUS_FMT_RGB565_2X8_LE:
		return BUS_FMT_RGB565;
	case V4L2_MBUS_FMT_UYVY8_2X8:
	case V4L2_MBUS_FMT_UYVY8_1X16:
		return BUS_FMT_UYVY;
	case V4L2_MBUS_FMT_VYUY8_2X8:
	case V4L2_MBUS_FMT_VYUY8_1X16:
		return BUS_FMT_VYUY;
	case V4L2_MBUS_FMT_YUYV8_2X8:
	case V4L2_MBUS_FMT_YUYV10_2X10:
	case V4L2_MBUS_FMT_YUYV8_1X16:
	case V4L2_MBUS_FMT_YUYV10_1X20:
		return BUS_FMT_YUYV;
	case V4L2_MBUS_FMT_YVYU8_2X8:
	case V4L2_MBUS_FMT_YVYU10_2X10:
	case V4L2_MBUS_FMT_YVYU8_1X16:
	case V4L2_MBUS_FMT_YVYU10_1X20:
		return BUS_FMT_YVYU;
	case V4L2_MBUS_FMT_SBGGR8_1X8:
	case V4L2_MBUS_FMT_SBGGR10_DPCM8_1X8:
	case V4L2_MBUS_FMT_SBGGR10_1X10:
	case V4L2_MBUS_FMT_SBGGR12_1X12:
		return BUS_FMT_SBGGR;
	case V4L2_MBUS_FMT_SGBRG8_1X8:
	case V4L2_MBUS_FMT_SGBRG10_DPCM8_1X8:
	case V4L2_MBUS_FMT_SGBRG10_1X10:
	case V4L2_MBUS_FMT_SGBRG12_1X12:
		return BUS_FMT_SGBRG;
	case V4L2_MBUS_FMT_SGRBG8_1X8:
	case V4L2_MBUS_FMT_SGRBG10_DPCM8_1X8:
	case V4L2_MBUS_FMT_SGRBG10_1X10:
	case V4L2_MBUS_FMT_SGRBG12_1X12:
		return BUS_FMT_SGRBG;
	case V4L2_MBUS_FMT_SRGGB8_1X8:
	case V4L2_MBUS_FMT_SRGGB10_DPCM8_1X8:
	case V4L2_MBUS_FMT_SRGGB10_1X10:
	case V4L2_MBUS_FMT_SRGGB12_1X12:
		return BUS_FMT_SRGGB;
	default:
		return BUS_FMT_UYVY;
	}
}

enum bit_width find_bus_width(enum v4l2_mbus_pixelcode code)
{
	switch (code) {
	case V4L2_MBUS_FMT_BGR565_2X8_BE:
	case V4L2_MBUS_FMT_BGR565_2X8_LE:
	case V4L2_MBUS_FMT_RGB565_2X8_BE:
	case V4L2_MBUS_FMT_RGB565_2X8_LE:
	case V4L2_MBUS_FMT_UYVY8_2X8:
	case V4L2_MBUS_FMT_VYUY8_2X8:
	case V4L2_MBUS_FMT_YUYV8_2X8:
	case V4L2_MBUS_FMT_YVYU8_2X8:
	case V4L2_MBUS_FMT_SBGGR8_1X8:
	case V4L2_MBUS_FMT_SBGGR10_DPCM8_1X8:
	case V4L2_MBUS_FMT_SGBRG8_1X8:
	case V4L2_MBUS_FMT_SGBRG10_DPCM8_1X8:
	case V4L2_MBUS_FMT_SGRBG8_1X8:
	case V4L2_MBUS_FMT_SGRBG10_DPCM8_1X8:
	case V4L2_MBUS_FMT_SRGGB8_1X8:
	case V4L2_MBUS_FMT_SRGGB10_DPCM8_1X8:
		return W_8BIT;
	case V4L2_MBUS_FMT_YUYV10_2X10:
	case V4L2_MBUS_FMT_YVYU10_2X10:
	case V4L2_MBUS_FMT_SBGGR10_1X10:
	case V4L2_MBUS_FMT_SGBRG10_1X10:
	case V4L2_MBUS_FMT_SGRBG10_1X10:
	case V4L2_MBUS_FMT_SRGGB10_1X10:
		return W_10BIT;
	case V4L2_MBUS_FMT_SBGGR12_1X12:
	case V4L2_MBUS_FMT_SGBRG12_1X12:
	case V4L2_MBUS_FMT_SGRBG12_1X12:
	case V4L2_MBUS_FMT_SRGGB12_1X12:
		return W_12BIT;
	case V4L2_MBUS_FMT_UYVY8_1X16:
	case V4L2_MBUS_FMT_VYUY8_1X16:
	case V4L2_MBUS_FMT_YUYV8_1X16:
	case V4L2_MBUS_FMT_YVYU8_1X16:
		return W_16BIT;
	case V4L2_MBUS_FMT_YVYU10_1X20:
	case V4L2_MBUS_FMT_YUYV10_1X20:
		return W_20BIT;
	default:
		return W_8BIT;
	}
}

enum bit_width find_bus_precision(enum v4l2_mbus_pixelcode code)
{
	switch (code) {
	case V4L2_MBUS_FMT_BGR565_2X8_BE:
	case V4L2_MBUS_FMT_BGR565_2X8_LE:
	case V4L2_MBUS_FMT_RGB565_2X8_BE:
	case V4L2_MBUS_FMT_RGB565_2X8_LE:
	case V4L2_MBUS_FMT_SBGGR8_1X8:
	case V4L2_MBUS_FMT_SGBRG8_1X8:
	case V4L2_MBUS_FMT_SGRBG8_1X8:
	case V4L2_MBUS_FMT_SRGGB8_1X8:
	case V4L2_MBUS_FMT_SBGGR10_DPCM8_1X8:
	case V4L2_MBUS_FMT_SGBRG10_DPCM8_1X8:
	case V4L2_MBUS_FMT_SGRBG10_DPCM8_1X8:
	case V4L2_MBUS_FMT_SRGGB10_DPCM8_1X8:
	case V4L2_MBUS_FMT_UYVY8_2X8:
	case V4L2_MBUS_FMT_VYUY8_2X8:
	case V4L2_MBUS_FMT_YUYV8_2X8:
	case V4L2_MBUS_FMT_YVYU8_2X8:
	case V4L2_MBUS_FMT_UYVY8_1X16:
	case V4L2_MBUS_FMT_VYUY8_1X16:
	case V4L2_MBUS_FMT_YUYV8_1X16:
	case V4L2_MBUS_FMT_YVYU8_1X16:
		return W_8BIT;
	case V4L2_MBUS_FMT_SBGGR10_1X10:
	case V4L2_MBUS_FMT_SGBRG10_1X10:
	case V4L2_MBUS_FMT_SGRBG10_1X10:
	case V4L2_MBUS_FMT_SRGGB10_1X10:
	case V4L2_MBUS_FMT_YUYV10_2X10:
	case V4L2_MBUS_FMT_YVYU10_2X10:
	case V4L2_MBUS_FMT_YVYU10_1X20:
	case V4L2_MBUS_FMT_YUYV10_1X20:
		return W_10BIT;
	case V4L2_MBUS_FMT_SBGGR12_1X12:
	case V4L2_MBUS_FMT_SGBRG12_1X12:
	case V4L2_MBUS_FMT_SGRBG12_1X12:
	case V4L2_MBUS_FMT_SRGGB12_1X12:
		return W_12BIT;
	default:
		return W_8BIT;
	}
}

enum pixel_fmt_type find_pixel_fmt_type(unsigned int pix_fmt)
{
	switch (pix_fmt) {
	case V4L2_PIX_FMT_RGB565:
		return RGB565;
	case V4L2_PIX_FMT_RGB24:
		return RGB888;
	case V4L2_PIX_FMT_RGB32:
		return PRGB888;
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
		return YUV422_INTLVD;
	case V4L2_PIX_FMT_YUV422P:
		return YUV422_PL;
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YVU420:
		return YUV420_PL;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
		return YUV420_SPL;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		return YUV422_SPL;
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SRGGB8:
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SGBRG10:
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_SRGGB10:
	case V4L2_PIX_FMT_SBGGR12:
	case V4L2_PIX_FMT_SGBRG12:
	case V4L2_PIX_FMT_SGRBG12:
	case V4L2_PIX_FMT_SRGGB12:
		return BAYER_RGB;
	default:
		return BAYER_RGB;
	}
}

