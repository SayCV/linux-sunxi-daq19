
/*
 ******************************************************************************
 *
 * sunxi_isp.c
 *
 * Hawkview ISP - sunxi_isp.c module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2014/12/11	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>
#include "../platform/platform_cfg.h"
#include "bsp_isp.h"
#include "sunxi_isp.h"
#include "../vin-video/vin_core.h"

#define ISP_MODULE_NAME "vin_isp"

#if defined CONFIG_ARCH_SUN50I
#define ISP_HEIGHT_16B_ALIGN 0
#else
#define ISP_HEIGHT_16B_ALIGN 1
#endif
static int isp_dbg_en;
static int isp_dbg_lv = 1;

static LIST_HEAD(isp_drv_list);

#define MIN_IN_WIDTH			32
#define MIN_IN_HEIGHT			32
#define MAX_IN_WIDTH			4095
#define MAX_IN_HEIGHT			4095

#define MIN_OUT_WIDTH			16
#define MIN_OUT_HEIGHT			2
#define MAX_OUT_WIDTH			4095
#define MAX_OUT_HEIGHT			4095

static const struct isp_pix_fmt sunxi_isp_formats[] = {
	{
		.name = "RAW8 (GRBG)",
		.fourcc = V4L2_PIX_FMT_SGRBG8,
		.depth = {8},
		.color = 0,
		.memplanes = 1,
		.mbus_code = V4L2_MBUS_FMT_SGRBG8_1X8,
	}, {
		.name = "RAW10 (GRBG)",
		.fourcc = V4L2_PIX_FMT_SGRBG10,
		.depth = {10},
		.color = 0,
		.memplanes = 1,
		.mbus_code = V4L2_MBUS_FMT_SGRBG10_1X10,
	}, {
		.name = "RAW12 (GRBG)",
		.fourcc = V4L2_PIX_FMT_SGRBG12,
		.depth = {12},
		.color = 0,
		.memplanes = 1,
		.mbus_code = V4L2_MBUS_FMT_SGRBG12_1X12,
	},
};
static int __isp_set_input_fmt_internal(enum bus_pixeltype type)
{
	enum isp_input_fmt fmt;
	enum isp_input_seq seq_t;
	switch (type) {
		/* yuv420 */
	case BUS_FMT_YY_YUYV:
		fmt = ISP_YUV420;
		seq_t = ISP_YUYV;
		break;
	case BUS_FMT_YY_YVYU:
		fmt = ISP_YUV420;
		seq_t = ISP_YVYU;
		break;
	case BUS_FMT_YY_UYVY:
		fmt = ISP_YUV420;
		seq_t = ISP_UYVY;
		break;
	case BUS_FMT_YY_VYUY:
		fmt = ISP_YUV420;
		seq_t = ISP_VYUY;
		break;

		/* yuv422 */
	case BUS_FMT_YUYV:
		fmt = ISP_YUV422;
		seq_t = ISP_YUYV;
		break;
	case BUS_FMT_YVYU:
		fmt = ISP_YUV422;
		seq_t = ISP_YVYU;
		break;
	case BUS_FMT_UYVY:
		fmt = ISP_YUV422;
		seq_t = ISP_UYVY;
		break;
	case BUS_FMT_VYUY:
		fmt = ISP_YUV422;
		seq_t = ISP_VYUY;
		break;

		/* raw */
	case BUS_FMT_SBGGR:
		fmt = ISP_RAW;
		seq_t = ISP_BGGR;
		break;
	case BUS_FMT_SGBRG:
		fmt = ISP_RAW;
		seq_t = ISP_GBRG;
		break;
	case BUS_FMT_SGRBG:
		fmt = ISP_RAW;
		seq_t = ISP_GRBG;
		break;
	case BUS_FMT_SRGGB:
		fmt = ISP_RAW;
		seq_t = ISP_RGGB;
		break;
	default:
		return -1;
		break;
	}
	bsp_isp_set_input_fmt(fmt, seq_t);
	return 0;
}

static int __isp_set_output_fmt_internal(unsigned int fmt,
					 enum isp_channel ch)
{
	enum isp_output_fmt isp_fmt;
	enum isp_output_seq seq_t;
	switch (fmt) {
		/* yuv_p */
	case V4L2_PIX_FMT_YUV422P:
		isp_fmt = ISP_YUV422_P;
		seq_t = ISP_UV;
		break;
	case V4L2_PIX_FMT_YUV420:
		isp_fmt = ISP_YUV420_P;
		seq_t = ISP_UV;
		break;
	case V4L2_PIX_FMT_YVU420:
		isp_fmt = ISP_YUV420_P;
		seq_t = ISP_VU;
		break;

		/* yuv_sp */
	case V4L2_PIX_FMT_NV12:
		isp_fmt = ISP_YUV420_SP;
		seq_t = ISP_UV;
		break;
	case V4L2_PIX_FMT_NV21:
		isp_fmt = ISP_YUV420_SP;
		seq_t = ISP_VU;
		break;
	case V4L2_PIX_FMT_NV16:
		isp_fmt = ISP_YUV422_SP;
		seq_t = ISP_UV;
		break;
	case V4L2_PIX_FMT_NV61:
		isp_fmt = ISP_YUV422_SP;
		seq_t = ISP_VU;
		break;

	default:
		return -1;
		break;
	}
	bsp_isp_set_output_fmt(isp_fmt, seq_t, ch);
	return 0;
}

static int __isp_cal_ch_size(unsigned int fmt, struct isp_size *size,
			     struct isp_yuv_size_addr_info *info)
{
	switch (fmt) {
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YVU420:
		info->line_stride_y = ALIGN_16B(size->width);
		info->line_stride_c =
		    ALIGN_16B(info->line_stride_y >> 1);
		if (ISP_HEIGHT_16B_ALIGN) {
			info->buf_height_y = ALIGN_16B(size->height);
		} else {
			info->buf_height_y = size->height;
		}

		info->buf_height_cb = info->buf_height_y >> 1;
		info->buf_height_cr = info->buf_height_y >> 1;

		info->valid_height_y = size->height;
		info->valid_height_cb =
		    info->valid_height_y >> 1;
		info->valid_height_cr =
		    info->valid_height_y >> 1;
		break;
	case V4L2_PIX_FMT_YUV422P:
		info->line_stride_y = ALIGN_16B(size->width);
		info->line_stride_c =
		    ALIGN_16B(info->line_stride_y >> 1);
		if (ISP_HEIGHT_16B_ALIGN) {
			info->buf_height_y = ALIGN_16B(size->height);
		} else {
			info->buf_height_y = size->height;
		}
		info->buf_height_cb = info->buf_height_y;
		info->buf_height_cr = info->buf_height_y;

		info->valid_height_y = size->height;
		info->valid_height_cb = info->valid_height_y;
		info->valid_height_cr = info->valid_height_y;
		break;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
		info->line_stride_y = ALIGN_16B(size->width);
		info->line_stride_c = info->line_stride_y;
		if (ISP_HEIGHT_16B_ALIGN) {
			info->buf_height_y = ALIGN_16B(size->height);
		} else {
			info->buf_height_y = size->height;
		}

		info->buf_height_cb = info->buf_height_y >> 1;
		info->buf_height_cr = 0;

		info->valid_height_y = size->height;
		info->valid_height_cb =
		    info->valid_height_y >> 1;
		info->valid_height_cr = 0;
		break;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		info->line_stride_y = ALIGN_16B(size->width);
		info->line_stride_c = info->line_stride_y;
		if (ISP_HEIGHT_16B_ALIGN) {
			info->buf_height_y = ALIGN_16B(size->height);
		} else {
			info->buf_height_y = size->height;
		}
		info->buf_height_cb = info->buf_height_y;
		info->buf_height_cr = 0;

		info->valid_height_y = size->height;
		info->valid_height_cb = info->valid_height_y;
		info->valid_height_cr = 0;
		break;
	default:
		break;
	}
	info->isp_byte_size =
	    info->line_stride_y * info->buf_height_y +
	    info->line_stride_c * info->buf_height_cb +
	    info->line_stride_c * info->buf_height_cr;
	return info->isp_byte_size;
}

static int __isp_cal_ch_addr(enum enable_flag flip, unsigned int buf_base_addr,
			     struct isp_yuv_size_addr_info *info)
{
	info->yuv_addr.y_addr = buf_base_addr;
	info->yuv_addr.u_addr =
	    info->yuv_addr.y_addr +
	    info->line_stride_y * info->buf_height_y;
	info->yuv_addr.v_addr =
	    info->yuv_addr.u_addr +
	    info->line_stride_c * info->buf_height_cb;
	if (flip == ENABLE) {
		info->yuv_addr.y_addr =
		    info->yuv_addr.y_addr +
		    info->line_stride_y * info->valid_height_y -
		    info->line_stride_y;
		info->yuv_addr.u_addr =
		    info->yuv_addr.u_addr +
		    info->line_stride_c * info->valid_height_cb -
		    info->line_stride_c;
		info->yuv_addr.v_addr =
		    info->yuv_addr.v_addr +
		    info->line_stride_c * info->valid_height_cr -
		    info->line_stride_c;
	}
	return 0;
}
static unsigned int __isp_new_set_size_internal(struct isp_dev *isp,
						unsigned int *fmt,
						struct isp_size_settings
						*size_settings)
{
	int x_ratio, y_ratio, weight_shift;
	struct coor *ob_start = &size_settings->ob_start;
	struct isp_size *ob_black_size, *ob_valid_size, *full_size, *scale_size,
	    *rot_size;
	struct isp_yuv_size_addr_info *info = &isp->isp_yuv_size_addr[0];

	ob_black_size = &size_settings->ob_black_size;
	ob_valid_size = &size_settings->ob_valid_size;
	full_size = &size_settings->full_size;
	scale_size = &size_settings->scale_size;
	rot_size = &size_settings->ob_rot_size;

	bsp_isp_set_ob_zone(ob_black_size, ob_valid_size, ob_start, ISP_SRC0);
	if (scale_size && scale_size->width != 0 && scale_size->height != 0) {

		full_size->width = full_size->width & 0x1ffc;
		full_size->height = full_size->height & (~1);
		printk("[ISP] full_size width = %d, height = %d.\n",
		       full_size->width, full_size->height);
		x_ratio = ob_valid_size->width * 256 / full_size->width;
		y_ratio = ob_valid_size->height * 256 / full_size->height;
		weight_shift = min_scale_w_shift(x_ratio, y_ratio);
		bsp_isp_channel_enable(MAIN_CH);
		bsp_isp_scale_enable(MAIN_CH);
		bsp_isp_set_output_size(MAIN_CH, full_size);
		bsp_isp_scale_cfg(MAIN_CH, x_ratio, y_ratio, weight_shift);

		__isp_cal_ch_size(fmt[MAIN_CH], full_size, &info[MAIN_CH]);
		bsp_isp_set_stride_y(info[MAIN_CH].line_stride_y, MAIN_CH);
		bsp_isp_set_stride_uv(info[MAIN_CH].line_stride_c, MAIN_CH);

		scale_size->width = scale_size->width & 0x1ffc;/*4 byte*/
		scale_size->height = scale_size->height & (~1);
		printk("[ISP] scale width = %d, height = %d\n",
		       scale_size->width, scale_size->height);
		x_ratio = ob_valid_size->width * 256 / scale_size->width;
		y_ratio = ob_valid_size->height * 256 / scale_size->height;
		weight_shift = min_scale_w_shift(x_ratio, y_ratio);

		bsp_isp_channel_enable(SUB_CH);
		bsp_isp_scale_enable(SUB_CH);
		bsp_isp_set_output_size(SUB_CH, scale_size);
		bsp_isp_scale_cfg(SUB_CH, x_ratio, y_ratio, weight_shift);

		__isp_cal_ch_size(fmt[SUB_CH], scale_size, &info[SUB_CH]);

		bsp_isp_set_stride_y(info[SUB_CH].line_stride_y, SUB_CH);
		bsp_isp_set_stride_uv(info[SUB_CH].line_stride_c, SUB_CH);
	} else {

		full_size->width = full_size->width & 0x1ffc;
		full_size->height = full_size->height & (~1);
		printk("[ISP] full_size: %d %d, ob_valid: %d %d\n",
			full_size->width, full_size->height,
			ob_valid_size->width, ob_valid_size->height);
		x_ratio = ob_valid_size->width * 256 / full_size->width;
		y_ratio = ob_valid_size->height * 256 / full_size->height;
		weight_shift = min_scale_w_shift(x_ratio, y_ratio);

		bsp_isp_channel_enable(SUB_CH);
		bsp_isp_scale_enable(SUB_CH);
		bsp_isp_set_output_size(SUB_CH, full_size);
		bsp_isp_scale_cfg(SUB_CH, x_ratio, y_ratio, weight_shift);

		__isp_cal_ch_size(fmt[MAIN_CH], full_size, &info[SUB_CH]);

		bsp_isp_set_stride_y(info[SUB_CH].line_stride_y, SUB_CH);
		bsp_isp_set_stride_uv(info[SUB_CH].line_stride_c, SUB_CH);

		bsp_isp_channel_disable(MAIN_CH);
		info[MAIN_CH].isp_byte_size = 0;
	}

	if (rot_size && 0 != rot_size->height && 0 != rot_size->width) {
		__isp_cal_ch_size(fmt[ROT_CH], rot_size, &info[ROT_CH]);
		bsp_isp_set_stride_y(info[ROT_CH].line_stride_y, ROT_CH);
		bsp_isp_set_stride_uv(info[ROT_CH].line_stride_c, ROT_CH);
	} else {
		info[ROT_CH].isp_byte_size = 0;
	}

	info[MAIN_CH].isp_byte_size = ALIGN_4K(info[MAIN_CH].isp_byte_size);
	info[SUB_CH].isp_byte_size = ALIGN_4K(info[SUB_CH].isp_byte_size);
	info[ROT_CH].isp_byte_size = ALIGN_4K(info[ROT_CH].isp_byte_size);
	return info[MAIN_CH].isp_byte_size + info[SUB_CH].isp_byte_size +
		info[ROT_CH].isp_byte_size;
}

unsigned int sunxi_isp_set_size(struct isp_dev *isp, unsigned int *fmt,
				struct isp_size_settings *size_settings)
{
	return __isp_new_set_size_internal(isp, fmt, size_settings);
}
void sunxi_isp_set_fmt(struct isp_dev *isp, enum bus_pixeltype type,
		       unsigned int *fmt)
{
	__isp_set_input_fmt_internal(type);

	if (fmt[SUB_CH] != 0xffff) {
		__isp_set_output_fmt_internal(fmt[MAIN_CH], MAIN_CH);

		if (fmt[MAIN_CH] == V4L2_PIX_FMT_YVU420) {
			isp->plannar_uv_exchange_flag[MAIN_CH] = 1;
		} else {
			isp->plannar_uv_exchange_flag[MAIN_CH] = 0;
		}

		bsp_isp_module_enable(TG_EN);
		__isp_set_output_fmt_internal(fmt[SUB_CH], SUB_CH);

		if (fmt[SUB_CH] == V4L2_PIX_FMT_YVU420) {
			isp->plannar_uv_exchange_flag[SUB_CH] = 1;
		} else {
			isp->plannar_uv_exchange_flag[SUB_CH] = 0;
		}
	} else {
		__isp_set_output_fmt_internal(fmt[MAIN_CH], SUB_CH);

		if (fmt[MAIN_CH] == V4L2_PIX_FMT_YVU420) {
			isp->plannar_uv_exchange_flag[SUB_CH] = 1;
		} else {
			isp->plannar_uv_exchange_flag[SUB_CH] = 0;
		}
		bsp_isp_module_disable(TG_EN);
	}

	if (fmt[ROT_CH] != 0xffff) {
		isp->rotation_en = 1;
		bsp_isp_module_enable(ROT_EN);
		__isp_set_output_fmt_internal(fmt[ROT_CH], ROT_CH);

		if (fmt[SUB_CH] == V4L2_PIX_FMT_YVU420) {
			isp->plannar_uv_exchange_flag[ROT_CH] = 1;
		} else {
			isp->plannar_uv_exchange_flag[ROT_CH] = 0;
		}
	} else {
		bsp_isp_module_disable(ROT_EN);
	}
}

void sunxi_isp_set_flip(struct isp_dev *isp, enum isp_channel ch,
			enum enable_flag on_off)
{
	enum enable_flag *flip_en_glb = &isp->flip_en_glb[0];
	/*
	 * bsp_isp_set_flip(ch, on_off);
	 */
	flip_en_glb[ch] = on_off;
}
void sunxi_isp_set_mirror(enum isp_channel ch, enum enable_flag on_off)
{
	/*
	 * bsp_isp_set_mirror(ch, on_off);
	 */
}

void sunxi_isp_set_output_addr(struct v4l2_subdev *sd,
			       unsigned long buf_base_addr)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int tmp_addr;
	struct isp_yuv_size_addr_info *info = &isp->isp_yuv_size_addr[0];

	if (isp->use_cnt > 1)
		return;

	__isp_cal_ch_addr(isp->flip_en_glb[MAIN_CH], buf_base_addr,
			  &info[MAIN_CH]);
	if (isp->plannar_uv_exchange_flag[MAIN_CH] == 1) {
		tmp_addr = info[MAIN_CH].yuv_addr.u_addr;
		info[MAIN_CH].yuv_addr.u_addr = info[MAIN_CH].yuv_addr.v_addr;
		info[MAIN_CH].yuv_addr.v_addr = tmp_addr;
	}

	__isp_cal_ch_addr(isp->flip_en_glb[SUB_CH],
			  ALIGN_4K(buf_base_addr + info[MAIN_CH].isp_byte_size),
			  &info[SUB_CH]);
	if (isp->plannar_uv_exchange_flag[SUB_CH] == 1) {
		tmp_addr = info[SUB_CH].yuv_addr.u_addr;
		info[SUB_CH].yuv_addr.u_addr =
		    info[SUB_CH].yuv_addr.v_addr;
		info[SUB_CH].yuv_addr.v_addr = tmp_addr;
	}

	bsp_isp_set_yuv_addr(&info[MAIN_CH].yuv_addr, MAIN_CH, ISP_SRC0);
	bsp_isp_set_yuv_addr(&info[SUB_CH].yuv_addr, SUB_CH, ISP_SRC0);

	if (isp->rotation_en == 1) {
		__isp_cal_ch_addr(DISABLE,
				  ALIGN_4K(buf_base_addr +
					   info[MAIN_CH].isp_byte_size +
					   info[SUB_CH].isp_byte_size),
				  &info[ROT_CH]);

		if (isp->plannar_uv_exchange_flag[ROT_CH] == 1) {
			tmp_addr = info[ROT_CH].yuv_addr.u_addr;
			info[ROT_CH].yuv_addr.u_addr =
			    info[ROT_CH].yuv_addr.v_addr;
			info[ROT_CH].yuv_addr.v_addr = tmp_addr;
		}
		bsp_isp_set_yuv_addr(&info[ROT_CH].yuv_addr, ROT_CH, ISP_SRC0);
	}
}

static int sunxi_isp_subdev_s_power(struct v4l2_subdev *sd, int enable)
{
	return 0;
}
static int sunxi_isp_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (isp->use_cnt > 1)
		return 0;

	if (enable) {
		if (isp->vflip == 0) {
			sunxi_isp_set_flip(isp, MAIN_CH, DISABLE);
			sunxi_isp_set_flip(isp, SUB_CH, DISABLE);
		} else {
			sunxi_isp_set_flip(isp, MAIN_CH, ENABLE);
			sunxi_isp_set_flip(isp, SUB_CH, ENABLE);
		}
		if (isp->hflip == 0) {
			sunxi_isp_set_mirror(MAIN_CH, DISABLE);
			sunxi_isp_set_mirror(SUB_CH, DISABLE);
		} else {
			sunxi_isp_set_mirror(MAIN_CH, ENABLE);
			sunxi_isp_set_mirror(SUB_CH, ENABLE);
		}
	}
	return 0;
}

static struct v4l2_rect *__isp_get_crop(struct isp_dev *isp,
					struct v4l2_subdev_fh *fh,
					enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return fh ? v4l2_subdev_get_try_crop(fh, ISP_PAD_SINK) : NULL;
	else
		return &isp->crop.request;
}

static void __isp_try_crop(const struct v4l2_mbus_framefmt *sink,
			 const struct v4l2_mbus_framefmt *source,
			 struct v4l2_rect *crop)
{
	unsigned int min_width = source->width;
	unsigned int min_height = source->height;
	unsigned int max_width = sink->width;
	unsigned int max_height = sink->height;

	crop->width = clamp_t(u32, crop->width, min_width, max_width);
	crop->height = clamp_t(u32, crop->height, min_height, max_height);

	/* Crop can not go beyond of the input rectangle */
	crop->left = clamp_t(u32, crop->left, 0, sink->width - MIN_IN_WIDTH);
	crop->width =
	    clamp_t(u32, crop->width, MIN_IN_WIDTH, sink->width - crop->left);
	crop->top = clamp_t(u32, crop->top, 0, sink->height - MIN_IN_HEIGHT);
	crop->height =
	    clamp_t(u32, crop->height, MIN_IN_HEIGHT, sink->height - crop->top);
}

static const struct isp_pix_fmt *__isp_find_format(const u32 *
							 pixelformat,
							 const u32 *mbus_code,
							 int index)
{
	const struct isp_pix_fmt *fmt, *def_fmt = NULL;
	unsigned int i;
	int id = 0;

	if (index >= (int)ARRAY_SIZE(sunxi_isp_formats))
		return NULL;

	for (i = 0; i < ARRAY_SIZE(sunxi_isp_formats); ++i) {
		fmt = &sunxi_isp_formats[i];
		if (pixelformat && fmt->fourcc == *pixelformat)
			return fmt;
		if (mbus_code && fmt->mbus_code == *mbus_code)
			return fmt;
		if (index == id)
			def_fmt = fmt;
		id++;
	}
	return def_fmt;
}

static struct v4l2_mbus_framefmt *__isp_get_format(struct isp_dev *isp,
						struct v4l2_subdev_fh
						*fh, u32 pad, enum
						v4l2_subdev_format_whence
						which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return fh ? v4l2_subdev_get_try_format(fh, pad) : NULL;
	return &isp->format[pad];
}

static void __isp_try_format(struct isp_dev *isp, struct v4l2_subdev_fh *fh,
			   unsigned int pad, struct v4l2_mbus_framefmt *fmt,
			   enum v4l2_subdev_format_whence which)
{
	struct v4l2_mbus_framefmt *format;
	struct v4l2_rect crop;

	switch (pad) {
	case ISP_PAD_SINK:
		fmt->width =
		    clamp_t(u32, fmt->width, MIN_IN_WIDTH, MAX_IN_WIDTH);
		fmt->height =
		    clamp_t(u32, fmt->height, MIN_IN_HEIGHT, MAX_IN_HEIGHT);
		break;
	case ISP_PAD_SOURCE:
		format = __isp_get_format(isp, fh, ISP_PAD_SINK, which);
		fmt->code = format->code;

		crop = *__isp_get_crop(isp, fh, which);
		break;
	}
	fmt->colorspace = V4L2_COLORSPACE_JPEG;
	fmt->field = V4L2_FIELD_NONE;
}

static int sunxi_isp_enum_mbus_code(struct v4l2_subdev *sd,
				    struct v4l2_subdev_fh *fh,
				    struct v4l2_subdev_mbus_code_enum *code)
{
	const struct isp_pix_fmt *fmt;

	fmt = __isp_find_format(NULL, NULL, code->index);
	if (!fmt)
		return -EINVAL;
	code->code = fmt->mbus_code;
	return 0;
}

static int sunxi_isp_enum_frame_size(struct v4l2_subdev *sd,
				     struct v4l2_subdev_fh *fh,
				     struct v4l2_subdev_frame_size_enum *fse)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt format;

	if (fse->code != V4L2_MBUS_FMT_SGRBG10_1X10
	    && fse->code != V4L2_MBUS_FMT_SGRBG12_1X12)
		return -EINVAL;

	if (fse->index != 0)
		return -EINVAL;

	format.code = fse->code;
	format.width = 1;
	format.height = 1;
	__isp_try_format(isp, fh, fse->pad, &format, V4L2_SUBDEV_FORMAT_ACTIVE);
	fse->min_width = format.width;
	fse->min_height = format.height;

	if (format.code != fse->code)
		return -EINVAL;

	format.code = fse->code;
	format.width = -1;
	format.height = -1;
	__isp_try_format(isp, fh, fse->pad, &format, V4L2_SUBDEV_FORMAT_ACTIVE);
	fse->max_width = format.width;
	fse->max_height = format.height;

	return 0;
}

static int sunxi_isp_subdev_get_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_fh *fh,
				    struct v4l2_subdev_format *fmt)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;

	mf = __isp_get_format(isp, fh, fmt->pad, fmt->which);
	if (!mf)
		return -EINVAL;

	mutex_lock(&isp->subdev_lock);
	fmt->format = *mf;
	mutex_unlock(&isp->subdev_lock);
	return 0;
}

static int sunxi_isp_subdev_set_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_fh *fh,
				    struct v4l2_subdev_format *fmt)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;
	struct v4l2_rect *crop;

	format = __isp_get_format(isp, fh, fmt->pad, fmt->which);
	if (format == NULL)
		return -EINVAL;
	vin_print("%s %d*%d %x %d\n", __func__, fmt->format.width,
		  fmt->format.height, fmt->format.code, fmt->format.field);
	__isp_try_format(isp, fh, fmt->pad, &fmt->format, fmt->which);
	*format = fmt->format;

	if (fmt->pad == ISP_PAD_SINK) {
		/* reset crop rectangle */
		crop = __isp_get_crop(isp, fh, fmt->which);
		crop->left = 0;
		crop->top = 0;
		crop->width = fmt->format.width;
		crop->height = fmt->format.height;

		/* Propagate the format from sink to source */
		format =
		    __isp_get_format(isp, fh, ISP_PAD_SOURCE, fmt->which);
		*format = fmt->format;
		__isp_try_format(isp, fh, ISP_PAD_SOURCE, format, fmt->which);
	}

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
		isp->crop.active = isp->crop.request;
	}

	return 0;

}

int sunxi_isp_addr_init(struct v4l2_subdev *sd, u32 val)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	if (isp->use_cnt > 1)
		return 0;
	bsp_isp_set_dma_load_addr((unsigned long)isp->isp_load_reg_mm.dma_addr);
	bsp_isp_set_dma_saved_addr((unsigned long)isp->isp_save_reg_mm.
				   dma_addr);
	return 0;
}
static int sunxi_isp_subdev_cropcap(struct v4l2_subdev *sd,
				    struct v4l2_cropcap *a)
{
	return 0;
}
int sunxi_isp_set_mainchannel(struct isp_dev *isp,
			      struct main_channel_cfg *main_cfg)
{
	struct isp_size_settings isp_size;
	struct isp_fmt_cfg *isp_fmt = &isp->isp_fmt;
	struct sensor_win_size *win_cfg = &main_cfg->win_cfg;
	memset(isp_fmt, 0, sizeof(struct isp_fmt_cfg));

	isp_fmt->isp_fmt[MAIN_CH] = main_cfg->pix.pixelformat;
	isp_fmt->isp_size[MAIN_CH].width = main_cfg->pix.width;
	isp_fmt->isp_size[MAIN_CH].height = main_cfg->pix.height;
	isp_fmt->isp_fmt[SUB_CH] = 0xffff;
	isp_fmt->isp_fmt[ROT_CH] = 0xffff;

	isp_dbg(0, "bus_code = %d, isp_fmt = %p\n", isp_fmt->bus_code,
		isp_fmt->isp_fmt);
	isp_fmt->bus_code = main_cfg->bus_code;
	sunxi_isp_set_fmt(isp, isp_fmt->bus_code, &isp_fmt->isp_fmt[0]);

	if (0 == win_cfg->width || 0 == win_cfg->height) {
		win_cfg->width = isp_fmt->isp_size[MAIN_CH].width;
		win_cfg->height = isp_fmt->isp_size[MAIN_CH].height;
	}
	if (0 == win_cfg->width_input || 0 == win_cfg->height_input) {
		win_cfg->width_input = win_cfg->width;
		win_cfg->height_input = win_cfg->height;
	}

	isp_print("width_input = %d, height_input = %d, w = %d, h = %d\n",
		win_cfg->width_input, win_cfg->height_input,
		win_cfg->width, win_cfg->height);
	isp_fmt->ob_black_size.width = win_cfg->width_input + 2 * win_cfg->hoffset;
	isp_fmt->ob_black_size.height = win_cfg->height_input + 2 * win_cfg->voffset;
	isp_fmt->ob_valid_size.width = win_cfg->width_input;
	isp_fmt->ob_valid_size.height = win_cfg->height_input;
	isp_fmt->ob_start.hor = win_cfg->hoffset;
	isp_fmt->ob_start.ver = win_cfg->voffset;

	isp_size.full_size = isp_fmt->isp_size[MAIN_CH];
	isp_size.scale_size = isp_fmt->isp_size[SUB_CH];
	isp_size.ob_black_size = isp_fmt->ob_black_size;
	isp_size.ob_start = isp_fmt->ob_start;
	isp_size.ob_valid_size = isp_fmt->ob_valid_size;
	isp_size.ob_rot_size = isp_fmt->isp_size[ROT_CH];
	main_cfg->pix.sizeimage =
	    sunxi_isp_set_size(isp, &isp_fmt->isp_fmt[0], &isp_size);
	isp_print("main_cfg->pix: sizeimage = %d, width = %d, height = %d\n",
	     main_cfg->pix.sizeimage, main_cfg->pix.width,
	     main_cfg->pix.height);
	return 0;
}

static long sunxi_isp_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd,
				   void *arg)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int ret;

	if (isp->use_cnt > 1)
		return 0;

	switch (cmd) {
	case VIDIOC_SUNXI_ISP_MAIN_CH_CFG:
		mutex_lock(&isp->subdev_lock);
		ret = sunxi_isp_set_mainchannel(isp, arg);
		mutex_unlock(&isp->subdev_lock);
		break;
	default:
		return -ENOIOCTLCMD;
	}

	return ret;
}

static int sunxi_isp_subdev_get_selection(struct v4l2_subdev *sd,
					  struct v4l2_subdev_fh *fh,
					  struct v4l2_subdev_selection *sel)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_source, *format_sink;

	if (sel->pad != ISP_PAD_SINK)
		return -EINVAL;

	format_sink = __isp_get_format(isp, fh, ISP_PAD_SINK, sel->which);
	format_source =
	    __isp_get_format(isp, fh, ISP_PAD_SOURCE, sel->which);

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = INT_MAX;
		sel->r.height = INT_MAX;

		__isp_try_crop(format_sink, format_source, &sel->r);
		break;

	case V4L2_SEL_TGT_CROP:
		sel->r = *__isp_get_crop(isp, fh, sel->which);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_isp_subdev_set_selection(struct v4l2_subdev *sd,
					  struct v4l2_subdev_fh *fh,
					  struct v4l2_subdev_selection *sel)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_sink, *format_source;

	if (sel->target != V4L2_SEL_TGT_CROP || sel->pad != ISP_PAD_SINK)
		return -EINVAL;

	format_sink = __isp_get_format(isp, fh, ISP_PAD_SINK, sel->which);
	format_source =
	    __isp_get_format(isp, fh, ISP_PAD_SOURCE, sel->which);

	vin_print("%s: L = %d, T = %d, W = %d, H = %d\n", __func__,
		  sel->r.left, sel->r.top, sel->r.width, sel->r.height);

	vin_print("%s: input = %dx%d, output = %dx%d\n", __func__,
		  format_sink->width, format_sink->height,
		  format_source->width, format_source->height);

	__isp_try_crop(format_sink, format_source, &sel->r);
	*__isp_get_crop(isp, fh, sel->which) = sel->r;

	if (sel->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	isp->crop.active = sel->r;

	return 0;
}

static int sunxi_isp_subdev_get_crop(struct v4l2_subdev *sd,
			struct v4l2_subdev_fh *fh,
			struct v4l2_subdev_crop *crop)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (crop->pad != ISP_PAD_SINK)
		return -EINVAL;

	crop->rect = *__isp_get_crop(isp, fh, crop->which);
	return 0;
}

static int sunxi_isp_subdev_set_crop(struct v4l2_subdev *sd,
			struct v4l2_subdev_fh *fh,
			struct v4l2_subdev_crop *crop)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_sink, *format_source;

	if (crop->pad != ISP_PAD_SINK)
		return -EINVAL;

	format_sink = __isp_get_format(isp, fh, ISP_PAD_SINK, crop->which);
	format_source =
	    __isp_get_format(isp, fh, ISP_PAD_SOURCE, crop->which);

	vin_print("%s: L = %d, T = %d, W = %d, H = %d\n", __func__,
		  crop->rect.left, crop->rect.top,
		  crop->rect.width, crop->rect.height);

	vin_print("%s: input = %dx%d, output = %dx%d\n", __func__,
		  format_sink->width, format_sink->height,
		  format_source->width, format_source->height);

	__isp_try_crop(format_sink, format_source, &crop->rect);
	*__isp_get_crop(isp, fh, crop->which) = crop->rect;

	if (crop->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	isp->crop.active = crop->rect;

	return 0;
}

void sunxi_isp_vsync_isr(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_event event;

	memset(&event, 0, sizeof(event));
	event.type = V4L2_EVENT_VSYNC;
	event.id = 0;
	v4l2_event_queue(isp->subdev.devnode, &event);
}

void sunxi_isp_frame_sync_isr(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct vin_pipeline *pipe = NULL;
	struct v4l2_event event;

	vin_isp_stat_isr_frame_sync(&isp->h3a_stat);

	memset(&event, 0, sizeof(event));
	event.type = V4L2_EVENT_FRAME_SYNC;
	event.id = 0;
	v4l2_event_queue(isp->subdev.devnode, &event);

	pipe = to_vin_pipeline(&sd->entity);
	atomic_inc(&pipe->frame_number);

	pipe = to_vin_pipeline(&isp->h3a_stat.sd.entity);
	vin_isp_stat_isr(&isp->h3a_stat);
}

int sunxi_isp_subscribe_event(struct v4l2_subdev *sd,
				  struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	vin_log(VIN_LOG_ISP, "%s id = %d\n", __func__, sub->id);
	if (sub->type == V4L2_EVENT_CTRL)
		return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
	else
		return v4l2_event_subscribe(fh, sub, 1, NULL);
}

int sunxi_isp_unsubscribe_event(struct v4l2_subdev *sd,
				    struct v4l2_fh *fh,
				    struct v4l2_event_subscription *sub)
{
	vin_log(VIN_LOG_ISP, "%s id = %d\n", __func__, sub->id);
	return v4l2_event_unsubscribe(fh, sub);
}

static const struct v4l2_subdev_core_ops sunxi_isp_core_ops = {
	.s_power = sunxi_isp_subdev_s_power,
	.init = sunxi_isp_addr_init,
	.queryctrl = v4l2_subdev_queryctrl,
	.g_ctrl = v4l2_subdev_g_ctrl,
	.s_ctrl = v4l2_subdev_s_ctrl,
	.ioctl = sunxi_isp_subdev_ioctl,
	.subscribe_event = sunxi_isp_subscribe_event,
	.unsubscribe_event = sunxi_isp_unsubscribe_event,
};

static const struct v4l2_subdev_video_ops sunxi_isp_subdev_video_ops = {
	.s_stream = sunxi_isp_subdev_s_stream,
	.cropcap = sunxi_isp_subdev_cropcap,
};

static const struct v4l2_subdev_pad_ops sunxi_isp_subdev_pad_ops = {
	.enum_mbus_code = sunxi_isp_enum_mbus_code,
	.enum_frame_size = sunxi_isp_enum_frame_size,
	.get_fmt = sunxi_isp_subdev_get_fmt,
	.set_fmt = sunxi_isp_subdev_set_fmt,
	.get_selection = sunxi_isp_subdev_get_selection,
	.set_selection = sunxi_isp_subdev_set_selection,
	.get_crop = sunxi_isp_subdev_get_crop,
	.set_crop = sunxi_isp_subdev_set_crop,
};

static struct v4l2_subdev_ops sunxi_isp_subdev_ops = {
	.core = &sunxi_isp_core_ops,
	.video = &sunxi_isp_subdev_video_ops,
	.pad = &sunxi_isp_subdev_pad_ops,
};

/*
static int sunxi_isp_subdev_registered(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);
	int ret = 0;
	return ret;
}

static void sunxi_isp_subdev_unregistered(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);
	return;
}

static const struct v4l2_subdev_internal_ops sunxi_isp_sd_internal_ops = {
	.registered = sunxi_isp_subdev_registered,
	.unregistered = sunxi_isp_subdev_unregistered,
};
*/

/* media operations */
static const struct media_entity_operations isp_media_ops = {
};


static int __sunxi_isp_ctrl(struct isp_dev *isp, struct v4l2_ctrl *ctrl)
{
	int ret = 0;

	if (ctrl->flags & V4L2_CTRL_FLAG_INACTIVE)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		if (ctrl->val == 0)
			sunxi_isp_set_mirror(MAIN_CH, DISABLE);
		else
			sunxi_isp_set_mirror(MAIN_CH, ENABLE);
		isp->hflip = ctrl->val;
		break;
	case V4L2_CID_VFLIP:
		if (ctrl->val == 0)
			sunxi_isp_set_flip(isp, MAIN_CH, DISABLE);
		else
			sunxi_isp_set_flip(isp, MAIN_CH, ENABLE);
		isp->vflip = ctrl->val;
		break;
	case V4L2_CID_ROTATE:
		isp->rotate = ctrl->val;
		break;
	default:
		break;
	}
	return ret;
}

#define ctrl_to_sunxi_isp(ctrl) \
	container_of(ctrl->handler, struct isp_dev, ctrls.handler)

static int sunxi_isp_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct isp_dev *isp = ctrl_to_sunxi_isp(ctrl);
	unsigned long flags;
	int ret;

	if (isp->use_cnt > 1)
		return 0;
	vin_log(VIN_LOG_ISP, "id = %d, val = %d, cur.val = %d\n",
		  ctrl->id, ctrl->val, ctrl->cur.val);
	spin_lock_irqsave(&isp->slock, flags);
	ret = __sunxi_isp_ctrl(isp, ctrl);
	spin_unlock_irqrestore(&isp->slock, flags);

	return ret;
}

static const struct v4l2_ctrl_ops sunxi_isp_ctrl_ops = {
	.s_ctrl = sunxi_isp_s_ctrl,
};

static const struct v4l2_ctrl_config ae_win_ctrls[] = {
	{
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_Y1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_Y2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

static const struct v4l2_ctrl_config af_win_ctrls[] = {
	{
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_Y1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_Y2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

static const struct v4l2_ctrl_config wb_gain_ctrls[] = {
	{
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_R_GAIN,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_GR_GAIN,
		.name = "GR GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_GB_GAIN,
		.name = "GB GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_B_GAIN,
		.name = "B GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

int sunxi_isp_init_subdev(struct isp_dev *isp)
{
	const struct v4l2_ctrl_ops *ops = &sunxi_isp_ctrl_ops;
	struct v4l2_ctrl_handler *handler = &isp->ctrls.handler;
	struct v4l2_subdev *sd = &isp->subdev;
	struct sunxi_isp_ctrls *ctrls = &isp->ctrls;
	enum enable_flag *flip_en_glb = &isp->flip_en_glb[0];
	int i, ret;
	mutex_init(&isp->subdev_lock);

	v4l2_subdev_init(sd, &sunxi_isp_subdev_ops);
	sd->grp_id = VIN_GRP_ID_ISP;
	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS | V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_isp.%u", isp->id);
	v4l2_set_subdevdata(sd, isp);

	v4l2_ctrl_handler_init(handler, 3 + ARRAY_SIZE(ae_win_ctrls)
		+ ARRAY_SIZE(af_win_ctrls) + ARRAY_SIZE(wb_gain_ctrls));

	ctrls->rotate =
	    v4l2_ctrl_new_std(handler, ops, V4L2_CID_ROTATE, 0, 270, 90, 0);
	ctrls->hflip =
	    v4l2_ctrl_new_std(handler, ops, V4L2_CID_HFLIP, 0, 1, 1, 0);
	ctrls->vflip =
	    v4l2_ctrl_new_std(handler, ops, V4L2_CID_VFLIP, 0, 1, 1, 0);

	for (i = 0; i < ARRAY_SIZE(wb_gain_ctrls); i++)
		ctrls->wb_gain[i] = v4l2_ctrl_new_custom(handler,
						&wb_gain_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(wb_gain_ctrls), &ctrls->wb_gain[0]);

	for (i = 0; i < ARRAY_SIZE(ae_win_ctrls); i++)
		ctrls->ae_win[i] = v4l2_ctrl_new_custom(handler,
						&ae_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(ae_win_ctrls), &ctrls->ae_win[0]);

	for (i = 0; i < ARRAY_SIZE(af_win_ctrls); i++)
		ctrls->af_win[i] = v4l2_ctrl_new_custom(handler,
						&af_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(af_win_ctrls), &ctrls->af_win[0]);

	if (handler->error) {
		return handler->error;
	}

	/*sd->entity->ops = &isp_media_ops;*/
	isp->isp_pads[ISP_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	isp->isp_pads[ISP_PAD_SOURCE_ST].flags = MEDIA_PAD_FL_SOURCE;
	isp->isp_pads[ISP_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV;

	ret = media_entity_init(&sd->entity, ISP_PAD_NUM, isp->isp_pads, 0);
	if (ret < 0)
		return ret;

	sd->ctrl_handler = handler;
	/*sd->internal_ops = &sunxi_isp_sd_internal_ops;*/

	flip_en_glb[MAIN_CH] = 0;
	flip_en_glb[SUB_CH] = 0;
	return 0;
}

void update_isp_setting(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = sd_to_vin_core(sd);
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int valid_idx = vinc->modu_cfg.sensors.valid_idx;
	struct sensor_instance *inst =
		&vinc->modu_cfg.sensors.inst[valid_idx];
	printk("isp->use_cnt = %d\n", isp->use_cnt);
	if (isp->use_cnt > 1)
		return;

	isp->isp_3a_result_pt = &isp->isp_3a_result;
	/*isp->isp_gen_set_pt = &isp->isp_gen_set;*/
	isp->isp_gen_set_pt->module_cfg.isp_platform_id = vinc->platform_id;
	if (inst->is_bayer_raw) {
		mutex_init(&vinc->isp_3a_result_mutex);
		isp->isp_gen_set_pt->module_cfg.lut_src0_table =
		    isp->isp_tbl_addr.isp_def_lut_tbl_vaddr;
		isp->isp_gen_set_pt->module_cfg.gamma_table =
		    isp->isp_tbl_addr.isp_gamma_tbl_vaddr;
		isp->isp_gen_set_pt->module_cfg.lens_table =
		    isp->isp_tbl_addr.isp_lsc_tbl_vaddr;
		isp->isp_gen_set_pt->module_cfg.linear_table =
		    isp->isp_tbl_addr.isp_linear_tbl_vaddr;
		isp->isp_gen_set_pt->module_cfg.disc_table =
		    isp->isp_tbl_addr.isp_disc_tbl_vaddr;
		if (inst->is_isp_used)
			bsp_isp_update_lut_lens_gamma_table(&isp->isp_tbl_addr);
	}
	isp->isp_gen_set_pt->module_cfg.drc_table =
	    isp->isp_tbl_addr.isp_drc_tbl_vaddr;
	if (inst->is_isp_used)
		bsp_isp_update_drc_table(&isp->isp_tbl_addr);
}

/*static int isp_resource_request(struct isp_dev *isp)*/
int isp_resource_request(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = sd_to_vin_core(sd);
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int valid_idx = vinc->modu_cfg.sensors.valid_idx;
	struct sensor_instance *inst =
		&vinc->modu_cfg.sensors.inst[valid_idx];
	struct vin_isp_stat_buf *stat_buf = NULL;
	unsigned int isp_used_flag = 0, i = 0;
	void *pa_base, *va_base, *dma_base;
	int ret;

	if (isp->use_cnt > 1)
		return 0;

	/*requeset for isp table and statistic buffer*/
	if (inst->is_isp_used && inst->is_bayer_raw) {
		isp->isp_lut_tbl_buf_mm.size =
		    ISP_LINEAR_LUT_LENS_GAMMA_MEM_SIZE;
		ret = os_mem_alloc(&isp->isp_lut_tbl_buf_mm);
		if (!ret) {
			pa_base = isp->isp_lut_tbl_buf_mm.phy_addr;
			va_base = isp->isp_lut_tbl_buf_mm.vir_addr;
			dma_base = isp->isp_lut_tbl_buf_mm.dma_addr;
			isp->isp_tbl_addr.isp_def_lut_tbl_paddr =
			    (void *)(pa_base + ISP_LUT_MEM_OFS);
			isp->isp_tbl_addr.isp_def_lut_tbl_dma_addr =
			    (void *)(dma_base + ISP_LUT_MEM_OFS);
			isp->isp_tbl_addr.isp_def_lut_tbl_vaddr =
			    (void *)(va_base + ISP_LUT_MEM_OFS);
			isp->isp_tbl_addr.isp_lsc_tbl_paddr =
			    (void *)(pa_base + ISP_LENS_MEM_OFS);
			isp->isp_tbl_addr.isp_lsc_tbl_dma_addr =
			    (void *)(dma_base + ISP_LENS_MEM_OFS);
			isp->isp_tbl_addr.isp_lsc_tbl_vaddr =
			    (void *)(va_base + ISP_LENS_MEM_OFS);
			isp->isp_tbl_addr.isp_gamma_tbl_paddr =
			    (void *)(pa_base + ISP_GAMMA_MEM_OFS);
			isp->isp_tbl_addr.isp_gamma_tbl_dma_addr =
			    (void *)(dma_base + ISP_GAMMA_MEM_OFS);
			isp->isp_tbl_addr.isp_gamma_tbl_vaddr =
			    (void *)(va_base + ISP_GAMMA_MEM_OFS);

			isp->isp_tbl_addr.isp_linear_tbl_paddr =
			    (void *)(pa_base + ISP_LINEAR_MEM_OFS);
			isp->isp_tbl_addr.isp_linear_tbl_dma_addr =
			    (void *)(dma_base + ISP_LINEAR_MEM_OFS);
			isp->isp_tbl_addr.isp_linear_tbl_vaddr =
			    (void *)(va_base + ISP_LINEAR_MEM_OFS);
			vin_log(VIN_LOG_ISP, "isp_def_lut_tbl_vaddr[%d] = %p\n", i,
				isp->isp_tbl_addr.isp_def_lut_tbl_vaddr);
			vin_log(VIN_LOG_ISP, "isp_lsc_tbl_vaddr[%d] = %p\n", i,
				isp->isp_tbl_addr.isp_lsc_tbl_vaddr);
			vin_log(VIN_LOG_ISP, "isp_gamma_tbl_vaddr[%d] = %p\n", i,
				isp->isp_tbl_addr.isp_gamma_tbl_vaddr);
		} else {
			vin_err
			    ("isp lut_lens_gamma table request pa failed!\n");
			return -ENOMEM;
		}

		if (inst->is_isp_used && inst->is_bayer_raw) {
			isp->isp_drc_tbl_buf_mm.size = ISP_DRC_DISC_MEM_SIZE;
			ret = os_mem_alloc(&isp->isp_drc_tbl_buf_mm);
			if (!ret) {
				pa_base = isp->isp_drc_tbl_buf_mm.phy_addr;
				va_base = isp->isp_drc_tbl_buf_mm.vir_addr;
				dma_base = isp->isp_drc_tbl_buf_mm.dma_addr;

				isp->isp_tbl_addr.isp_drc_tbl_paddr =
				    (void *)(pa_base + ISP_DRC_MEM_OFS);
				isp->isp_tbl_addr.isp_drc_tbl_dma_addr =
				    (void *)(dma_base + ISP_DRC_MEM_OFS);
				isp->isp_tbl_addr.isp_drc_tbl_vaddr =
				    (void *)(va_base + ISP_DRC_MEM_OFS);

				isp->isp_tbl_addr.isp_disc_tbl_paddr =
				    (void *)(pa_base + ISP_DISC_MEM_OFS);
				isp->isp_tbl_addr.isp_disc_tbl_dma_addr =
				    (void *)(dma_base + ISP_DISC_MEM_OFS);
				isp->isp_tbl_addr.isp_disc_tbl_vaddr =
				    (void *)(va_base + ISP_DISC_MEM_OFS);

				vin_log(VIN_LOG_ISP, "isp_drc_tbl_vaddr[%d] = %p\n", i,
					isp->isp_tbl_addr.isp_drc_tbl_vaddr);
			} else {
				vin_err("isp drc table request pa failed!\n");
				return -ENOMEM;
			}
		}
	}

	if (inst->is_isp_used && inst->is_bayer_raw) {
		isp_used_flag = 1;
	}

	if (isp_used_flag) {
		for (i = 0; i < MAX_ISP_STAT_BUF; i++) {
			isp->isp_stat_buf_mm.size = ISP_STAT_TOTAL_SIZE;
			ret = os_mem_alloc(&isp->isp_stat_buf_mm);
			if (!ret) {
				pa_base = isp->isp_stat_buf_mm.phy_addr;
				va_base = isp->isp_stat_buf_mm.vir_addr;
				dma_base = isp->isp_stat_buf_mm.dma_addr;
				stat_buf = &isp->isp_stat_bq.isp_stat[i];
				INIT_LIST_HEAD(&stat_buf->queue);
				stat_buf->id = i;
				stat_buf->paddr = pa_base;
				stat_buf->dma_addr = dma_base;
				stat_buf->isp_stat_buf.stat_buf = va_base;
				stat_buf->isp_stat_buf.buf_size = ISP_STAT_TOTAL_SIZE;
				stat_buf->isp_stat_buf.buf_status = BUF_IDLE;
				vin_log(VIN_LOG_ISP, "the %d stat buf addr = %p\n",
					i, stat_buf->isp_stat_buf.stat_buf);
			} else {
				vin_err("isp stat buf request failed!\n");
				return -ENOMEM;
			}
		}
	}
	return 0;
}

/*static void isp_resource_release(struct isp_dev *isp)*/
void isp_resource_release(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = sd_to_vin_core(sd);
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int valid_idx = vinc->modu_cfg.sensors.valid_idx;
	struct sensor_instance *inst =
		&vinc->modu_cfg.sensors.inst[valid_idx];
	unsigned int isp_used_flag = 0, i = 0;

	if (isp->use_cnt > 1)
		return;

	/* release isp table and statistic buffer */
	if (inst->is_isp_used && inst->is_bayer_raw) {
		os_mem_free(&isp->isp_lut_tbl_buf_mm);
		os_mem_free(&isp->isp_drc_tbl_buf_mm);
	}

	if (inst->is_isp_used && inst->is_bayer_raw) {
		isp_used_flag = 1;
	}

	if (isp_used_flag) {
		for (i = 0; i < MAX_ISP_STAT_BUF; i++) {
			os_mem_free(&isp->isp_stat_buf_mm);
		}
	}
}

static int isp_resource_alloc(struct isp_dev *isp)
{
	int ret = 0;
	isp->isp_load_reg_mm.size = ISP_LOAD_REG_SIZE;
	isp->isp_save_reg_mm.size = ISP_SAVED_REG_SIZE;

	os_mem_alloc(&isp->isp_load_reg_mm);
	os_mem_alloc(&isp->isp_save_reg_mm);

	if (isp->isp_load_reg_mm.phy_addr != NULL) {
		if (!isp->isp_load_reg_mm.vir_addr) {
			vin_err("isp load regs va requset failed!\n");
			return -ENOMEM;
		}
	} else {
		vin_err("isp load regs pa requset failed!\n");
		return -ENOMEM;
	}

	if (isp->isp_save_reg_mm.phy_addr != NULL) {
		if (!isp->isp_save_reg_mm.vir_addr) {
			vin_err("isp save regs va requset failed!\n");
			return -ENOMEM;
		}
	} else {
		vin_err("isp save regs pa requset failed!\n");
		return -ENOMEM;
	}
	return ret;

}
static void isp_resource_free(struct isp_dev *isp)
{
	os_mem_free(&isp->isp_load_reg_mm);
	os_mem_free(&isp->isp_save_reg_mm);
}

static int isp_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct isp_dev *isp = NULL;
	int ret = 0;

	if (np == NULL) {
		vin_err("ISP failed to get of node\n");
		return -ENODEV;
	}

	isp = kzalloc(sizeof(struct isp_dev), GFP_KERNEL);
	if (!isp) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	isp->isp_gen_set_pt =
	    kzalloc(sizeof(struct isp_gen_settings), GFP_KERNEL);

	if (!isp->isp_gen_set_pt) {
		vin_err("request isp_gen_settings mem failed!\n");
		return -ENOMEM;
	}

	pdev->id = of_alias_get_id(np, "isp");
	if (pdev->id < 0) {
		vin_err("ISP failed to get alias id\n");
		ret = -EINVAL;
		goto freedev;
	}

	isp->id = pdev->id;
	isp->pdev = pdev;

#if 0
	isp->base = of_iomap(np, 0);
	if (!isp->base) {
		ret = -EIO;
		goto freedev;
	}
#endif
	list_add_tail(&isp->isp_list, &isp_drv_list);
	isp->rotation_en = 0;
	sunxi_isp_init_subdev(isp);

	spin_lock_init(&isp->slock);
	init_waitqueue_head(&isp->wait);

	if (isp_resource_alloc(isp) < 0) {
		ret = -ENOMEM;
		goto ehwinit;
	}

	ret = vin_isp_h3a_init(isp);
	if (ret < 0) {
		vin_err("VIN H3A initialization failed\n");
			goto free_res;
	}

	bsp_isp_init_platform(SUNXI_PLATFORM_ID);
	bsp_isp_set_base_addr((unsigned long)isp->base);
	bsp_isp_set_map_load_addr((unsigned long)isp->isp_load_reg_mm.vir_addr);
	bsp_isp_set_map_saved_addr((unsigned long)isp->isp_save_reg_mm.vir_addr);
	memset((unsigned int *)isp->isp_load_reg_mm.vir_addr, 0,
	       ISP_LOAD_REG_SIZE);
	memset((unsigned int *)isp->isp_save_reg_mm.vir_addr, 0,
	       ISP_SAVED_REG_SIZE);
	platform_set_drvdata(pdev, isp);
	vin_print("isp%d probe end!\n", isp->id);
	return 0;
free_res:
	isp_resource_free(isp);
ehwinit:
	iounmap(isp->base);
freedev:
	kfree(isp);
ekzalloc:
	vin_print("isp probe err!\n");
	return ret;
}

static int isp_remove(struct platform_device *pdev)
{
	struct isp_dev *isp = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &isp->subdev;

	platform_set_drvdata(pdev, NULL);
	v4l2_ctrl_handler_free(sd->ctrl_handler);
	v4l2_set_subdevdata(sd, NULL);

	isp_resource_free(isp);
	if (isp->ioarea) {
		release_resource(isp->ioarea);
		kfree(isp->ioarea);
	}
	if (isp->base)
		iounmap(isp->base);
	list_del(&isp->isp_list);
	kfree(isp->isp_gen_set_pt);
	vin_isp_h3a_cleanup(isp);
	kfree(isp);
	return 0;
}

static const struct of_device_id sunxi_isp_match[] = {
	{.compatible = "allwinner,sunxi-isp",},
	{},
};

static struct platform_driver isp_platform_driver = {
	.probe = isp_probe,
	.remove = isp_remove,
	.driver = {
		   .name = ISP_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_isp_match,
		   }
};

void sunxi_isp_dump_regs(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int i = 0;
	printk("vin dump ISP regs :\n");
	for (i = 0; i < 0x40; i = i + 4) {
		if (i % 0x10 == 0)
			printk("0x%08x:  ", i);
		printk("0x%08x, ", readl(isp->base + i));
		if (i % 0x10 == 0xc)
			printk("\n");
	}
	for (i = 0x40; i < 0x240; i = i + 4) {
		if (i % 0x10 == 0)
			printk("0x%08x:  ", i);
		printk("0x%08x, ", readl(isp->isp_load_reg_mm.vir_addr + i));
		if (i % 0x10 == 0xc)
			printk("\n");
	}
}

struct v4l2_subdev *sunxi_isp_get_subdev(int id)
{
	struct isp_dev *isp;
	list_for_each_entry(isp, &isp_drv_list, isp_list) {
		if (isp->id == id) {
			isp->use_cnt++;
			return &isp->subdev;
		}
	}
	return NULL;
}

struct v4l2_subdev *sunxi_stat_get_subdev(int id)
{
	struct isp_dev *isp;
	list_for_each_entry(isp, &isp_drv_list, isp_list) {
		if (isp->id == id) {
			return &isp->h3a_stat.sd;
		}
	}
	return NULL;
}

int sunxi_isp_platform_register(void)
{
	return platform_driver_register(&isp_platform_driver);
}

void sunxi_isp_platform_unregister(void)
{
	platform_driver_unregister(&isp_platform_driver);
	vin_print("isp_exit end\n");
}
