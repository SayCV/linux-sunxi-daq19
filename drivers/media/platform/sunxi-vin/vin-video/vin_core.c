
/*
 ******************************************************************************
 *
 * vin_core.c
 *
 * Hawkview ISP - vin_core.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		Author         Date		    Description
 *
 *   3.0		Yang Feng   	2015/12/02	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/freezer.h>

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-dma-contig.h>

#include <linux/regulator/consumer.h>

#include "vin_core.h"
#include "../utility/bsp_common.h"
#include "../vin-isp/bsp_isp_algo.h"
#include "../vin-cci/bsp_cci.h"
#include "../vin-cci/cci_helper.h"
#include "../utility/config.h"
#include "../modules/sensor/camera_cfg.h"
#include "../utility/sensor_info.h"
#include "../utility/vin_io.h"
#include "../vin-csi/sunxi_csi.h"
#include "../vin-isp/sunxi_isp.h"
#include "../vin-scaler/sunxi_scaler.h"
#include "../vin-mipi/sunxi_mipi.h"
#include "vin_isp_helper.h"
#include "../vin.h"

#define VIN_CORE_NAME "sunxi-vin-core"

struct vin_core *vin_core_gbl[VIN_MAX_DEV];

char ccm[I2C_NAME_SIZE] = "";
uint i2c_addr = 0xff;

char act_name[I2C_NAME_SIZE] = "";
uint act_slave = 0xff;
uint use_sensor_list = 0xff;

uint isp_reparse_flag;
uint vin_i2c_dbg;
uint isp_log;
uint frame_cnt;
uint vin_dump;

EXPORT_SYMBOL_GPL(frame_cnt);
EXPORT_SYMBOL_GPL(vin_dump);

module_param_string(ccm, ccm, sizeof(ccm), S_IRUGO | S_IWUSR);
module_param(i2c_addr, uint, S_IRUGO | S_IWUSR);

module_param_string(act_name, act_name, sizeof(act_name), S_IRUGO | S_IWUSR);
module_param(act_slave, uint, S_IRUGO | S_IWUSR);
module_param(use_sensor_list, uint, S_IRUGO | S_IWUSR);
module_param(vin_i2c_dbg, uint, S_IRUGO | S_IWUSR);

#define DEVICE_ATTR_SHOW(name) \
static ssize_t name##_show(struct device *dev, \
			struct device_attribute *attr, \
			char *buf) \
{\
	return sprintf(buf, "%d\n", name); \
}

#define DEVICE_ATTR_STORE(name) \
static ssize_t name##_store(struct device *dev, \
			struct device_attribute *attr, \
			const char *buf,\
			size_t count) \
{\
	unsigned long val;\
	val = simple_strtoul(buf, NULL, 10);\
	name = val;\
	vin_print("Set val = %ld\n", val);\
	return count;\
}

DEVICE_ATTR_SHOW(vin_log_mask)
DEVICE_ATTR_SHOW(isp_reparse_flag)
DEVICE_ATTR_SHOW(vin_dump)
DEVICE_ATTR_STORE(vin_log_mask)
DEVICE_ATTR_STORE(isp_reparse_flag)
DEVICE_ATTR_STORE(vin_dump)

static DEVICE_ATTR(vin_log_mask, S_IRUGO | S_IWUSR | S_IWGRP,
		   vin_log_mask_show, vin_log_mask_store);
static DEVICE_ATTR(vin_dump, S_IRUGO | S_IWUSR | S_IWGRP,
		   vin_dump_show, vin_dump_store);
static DEVICE_ATTR(isp_reparse_flag, S_IRUGO | S_IWUSR | S_IWGRP,
		   isp_reparse_flag_show, isp_reparse_flag_store);

static struct attribute *vin_attributes[] = {
	&dev_attr_vin_log_mask.attr,
	&dev_attr_vin_dump.attr,
	&dev_attr_isp_reparse_flag.attr,
	NULL
};

static struct attribute_group vin_attribute_group = {
	/*.name = "vin_attrs",*/
	.attrs = vin_attributes,
};

static struct vin_fmt vin_formats[] = {
#if 1
	{
		.name		= "RGB565",
		.fourcc		= V4L2_PIX_FMT_RGB565,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_SRGB,
		.memplanes	= 1,
		.colplanes	= 1,
		.flags		= VIN_FMT_RGB,
	}, {
		.name		= "ARGB888, 32 bpp",
		.fourcc		= V4L2_PIX_FMT_RGB32,
		.depth		= { 32 },
		.color		= V4L2_COLORSPACE_SRGB,
		.memplanes	= 1,
		.colplanes	= 1,
		.flags		= VIN_FMT_RGB,
	},
#endif

#if 1
	{
		.name		= "YUV 4:2:2 planar, Y/Cb/Cr",
		.fourcc		= V4L2_PIX_FMT_YUV422P,
		.depth		= { 12 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 3,
		.flags		= VIN_FMT_YUV,
	}, {
		.name		= "YUV 4:2:2 planar, Y/CbCr",
		.fourcc		= V4L2_PIX_FMT_NV16,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:2 planar, Y/CrCb",
		.fourcc		= V4L2_PIX_FMT_NV61,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 planar, YCbCr",
		.fourcc		= V4L2_PIX_FMT_YUV420,
		.depth		= { 12 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 3,/*1*/
		.colplanes	= 3,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 planar, Y/CbCr",
		.fourcc		= V4L2_PIX_FMT_NV12,
		.depth		= { 12 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 2p, Y/CbCr",
		.fourcc		= V4L2_PIX_FMT_NV12M,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 8, 4 },
		.memplanes	= 2,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 3p, Y/Cb/Cr",
		.fourcc		= V4L2_PIX_FMT_YUV420M,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 8, 2, 2 },
		.memplanes	= 3,
		.colplanes	= 3,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 2p, tiled",
		.fourcc		= V4L2_PIX_FMT_NV12MT,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 8, 4 },
		.memplanes	= 2,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer BGGR 8bit",
		.fourcc		= V4L2_PIX_FMT_SBGGR8,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GBRG 8bit",
		.fourcc		= V4L2_PIX_FMT_SGBRG8,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SGBRG8_1X8,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GRBG 8bit",
		.fourcc		= V4L2_PIX_FMT_SGRBG8,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SGRBG8_1X8,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer RGGB 8bit",
		.fourcc		= V4L2_PIX_FMT_SGRBG8,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SGRBG8_1X8,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer BGGR 10bit",
		.fourcc		= V4L2_PIX_FMT_SBGGR10,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GBRG 10bit",
		.fourcc		= V4L2_PIX_FMT_SGBRG10,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SGBRG10_1X10,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GRBG 10bit",
		.fourcc		= V4L2_PIX_FMT_SGRBG10,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SGRBG10_1X10,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer RGGB 10bit",
		.fourcc		= V4L2_PIX_FMT_SGRBG10,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SGRBG10_1X10,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer BGGR 12bit",
		.fourcc		= V4L2_PIX_FMT_SBGGR12,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR12_1X12,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GBRG 12bit",
		.fourcc		= V4L2_PIX_FMT_SGBRG12,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SGBRG12_1X12,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GRBG 12bit",
		.fourcc		= V4L2_PIX_FMT_SGRBG12,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SGRBG12_1X12,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer RGGB 12bit",
		.fourcc		= V4L2_PIX_FMT_SGRBG12,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_SGRBG12_1X12,
		.flags		= VIN_FMT_RAW,
	},
#endif
	{
		.name		= "YUV 4:2:2 packed, YCbYCr",
		.fourcc		= V4L2_PIX_FMT_YUYV,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,
		.flags		= VIN_FMT_YUV,

	}, {
		.name		= "YUV 4:2:2 packed, CbYCrY",
		.fourcc		= V4L2_PIX_FMT_UYVY,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_UYVY8_2X8,
		.flags		= VIN_FMT_YUV,
	}, {
		.name		= "YUV 4:2:2 packed, CrYCbY",
		.fourcc		= V4L2_PIX_FMT_VYUY,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_VYUY8_2X8,
		.flags		= VIN_FMT_YUV,
	}, {
		.name		= "YUV 4:2:2 packed, YCrYCb",
		.fourcc		= V4L2_PIX_FMT_YVYU,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= V4L2_MBUS_FMT_YVYU8_2X8,
		.flags		= VIN_FMT_YUV,
	},
};

struct vin_fmt *vin_get_format(unsigned int index)
{
	if (index >= ARRAY_SIZE(vin_formats))
		return NULL;

	return &vin_formats[index];
}

void vin_get_fmt_mplane(struct vin_frame *frame, struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	int i;

	pixm->width = frame->o_width;
	pixm->height = frame->o_height;
	pixm->field = V4L2_FIELD_NONE;
	pixm->pixelformat = frame->fmt->fourcc;
	pixm->colorspace = frame->fmt->color;/*V4L2_COLORSPACE_JPEG;*/
	pixm->num_planes = frame->fmt->memplanes;

	for (i = 0; i < pixm->num_planes; ++i) {
		pixm->plane_fmt[i].bytesperline = frame->bytesperline[i];
		pixm->plane_fmt[i].sizeimage = frame->payload[i];
	}
}

struct vin_fmt *vin_find_format(const u32 *pixelformat, const u32 *mbus_code,
				  unsigned int mask, int index, bool have_code)
{
	struct vin_fmt *fmt, *def_fmt = NULL;
	unsigned int i;
	int id = 0;

	if (index >= (int)ARRAY_SIZE(vin_formats))
		return NULL;

	for (i = 0; i < ARRAY_SIZE(vin_formats); ++i) {
		fmt = &vin_formats[i];

		if (!(fmt->flags & mask))
			continue;
		if (have_code && (0 == fmt->mbus_code))
			continue;

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


/*
 *  the interrupt routine
 */

static irqreturn_t vin_isr(int irq, void *priv)
{
	int i, valid_idx;
	unsigned long flags;
	struct vin_buffer *buf;
	struct vin_vid_cap *cap = (struct vin_vid_cap *)priv;
	struct vin_core *vinc = cap->vinc;
	struct csi_int_status status;
	struct isp_dev *isp = v4l2_get_subdevdata(cap->pipe.sd[VIN_IND_ISP]);
	struct vin_isp_stat_buf_queue *stat_bq = &isp->isp_stat_bq;
	struct vin_isp_stat_buf *stat_buf_pt;
	struct sensor_instance *inst = NULL;
	valid_idx = vinc->modu_cfg.sensors.valid_idx;
	inst = &vinc->modu_cfg.sensors.inst[valid_idx];
	FUNCTION_LOG;
	vin_log(VIN_LOG_VIDEO, "vfe interrupt!!!\n");
	if (vin_is_generating(cap) == 0) {
		bsp_csi_int_clear_status(vinc->csi_sel, vinc->cur_ch,
					 CSI_INT_ALL);
		if (inst->is_isp_used)
			bsp_isp_clr_irq_status(ISP_IRQ_EN_ALL);
		return IRQ_HANDLED;
	}
	bsp_csi_int_get_status(vinc->csi_sel, vinc->cur_ch, &status);
	if ((status.capture_done == 0) && (status.frame_done == 0)
	    && (status.vsync_trig == 0)) {
		vin_print("enter vfe int for nothing\n");
		bsp_csi_int_clear_status(vinc->csi_sel, vinc->cur_ch,
					 CSI_INT_ALL);
		if (inst->is_isp_used)
			bsp_isp_clr_irq_status(ISP_IRQ_EN_ALL);
		return IRQ_HANDLED;
	}
	if (status.vsync_trig)
		if (cap->pipe.sd[VIN_IND_ISP])
			sunxi_isp_vsync_isr(cap->pipe.sd[VIN_IND_ISP]);

	if (inst->is_isp_used && inst->is_bayer_raw) {
		/* update_sensor_setting */
		if (status.vsync_trig) {
			bsp_csi_int_clear_status(vinc->csi_sel, vinc->cur_ch,
						 CSI_INT_VSYNC_TRIG);
			return IRQ_HANDLED;
		}
	}

	if (vin_dump & DUMP_CSI)
		if (5 == frame_cnt % 10)
			sunxi_csi_dump_regs(vinc->vid_cap.pipe.sd[VIN_IND_CSI]);
	frame_cnt++;

	FUNCTION_LOG;
	spin_lock_irqsave(&cap->slock, flags);
	FUNCTION_LOG;

	/* exception handle */
	if ((status.buf_0_overflow) || (status.buf_1_overflow) ||
	    (status.buf_2_overflow) || (status.hblank_overflow)) {
		if ((status.buf_0_overflow) || (status.buf_1_overflow) ||
		    (status.buf_2_overflow)) {
			bsp_csi_int_clear_status(vinc->csi_sel, vinc->cur_ch,
						 CSI_INT_BUF_0_OVERFLOW |
						 CSI_INT_BUF_1_OVERFLOW |
						 CSI_INT_BUF_2_OVERFLOW);
			vin_err("fifo overflow\n");
		}
		if (status.hblank_overflow) {
			bsp_csi_int_clear_status(vinc->csi_sel, vinc->cur_ch,
						 CSI_INT_HBLANK_OVERFLOW);
			vin_err("hblank overflow\n");
		}
		vin_err("reset csi module\n");
		bsp_csi_reset(vinc->csi_sel);
		if (inst->is_isp_used)
			goto isp_exp_handle;
		else
			goto unlock;
	}
isp_exp_handle:
	if (inst->is_isp_used) {
		if (bsp_isp_get_irq_status(SRC0_FIFO_INT_EN)) {
			vin_err("isp source0 fifo overflow\n");
			bsp_isp_clr_irq_status(SRC0_FIFO_INT_EN);
			goto unlock;
		}
	}
	vin_log(VIN_LOG_VIDEO, "status vsync = %d, framedone = %d, capdone = %d\n",
		status.vsync_trig, status.frame_done, status.capture_done);
	if (cap->capture_mode == V4L2_MODE_IMAGE) {
		if (inst->is_isp_used)
			bsp_isp_irq_disable(FINISH_INT_EN);
		else
			bsp_csi_int_disable(vinc->csi_sel, vinc->cur_ch,
					    CSI_INT_CAPTURE_DONE);
		vin_print("capture image mode!\n");
		buf =
		    list_entry(cap->vidq_active.next, struct vin_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);
		goto unlock;
	} else {
		if (inst->is_isp_used)
			bsp_isp_irq_disable(FINISH_INT_EN);
		else
			bsp_csi_int_disable(vinc->csi_sel, vinc->cur_ch,
					    CSI_INT_FRAME_DONE);
		if (cap->first_flag == 0) {
			cap->first_flag++;
			vin_print("capture video mode!\n");
			goto set_isp_stat_addr;
		}
		if (cap->first_flag == 1) {
			cap->first_flag++;
			vin_print("capture video first frame done!\n");
		}
		/* video buffer handle */
		if ((&cap->vidq_active) == cap->vidq_active.next->next->next) {
			vin_warn("Only two buffer left for csi\n");
			cap->first_flag = 0;
			goto unlock;
		}
		buf =
		    list_entry(cap->vidq_active.next, struct vin_buffer, list);

		/* Nobody is waiting on this buffer */
		if (!waitqueue_active(&buf->vb.vb2_queue->done_wq)) {
			vin_warn("Nobody is waiting on this video buffer,"
				"buf = 0x%p\n", buf);
		}
		list_del(&buf->list);
		v4l2_get_timestamp(&buf->vb.v4l2_buf.timestamp);

		vin_log(VIN_LOG_VIDEO, "video buffer frame interval = %ld\n",
			buf->vb.v4l2_buf.timestamp.tv_sec * 1000000 +
			buf->vb.v4l2_buf.timestamp.tv_usec -
			(cap->sec * 1000000 + cap->usec));
		cap->sec = buf->vb.v4l2_buf.timestamp.tv_sec;
		cap->usec = buf->vb.v4l2_buf.timestamp.tv_usec;
		cap->ms += jiffies_to_msecs(jiffies - cap->jiffies);
		cap->jiffies = jiffies;
		buf->vb.image_quality =
		    isp->isp_3a_result_pt->image_quality.dwval;

		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);

		/* isp_stat_handle */
		if (inst->is_isp_used  && inst->is_bayer_raw) {
			list_for_each_entry(stat_buf_pt, &stat_bq->locked, queue) {
				if (stat_buf_pt->isp_stat_buf.buf_status ==
				    BUF_LOCKED) {
					vin_log(VIN_LOG_VIDEO,
						"find isp stat buf locked!\n");
					goto set_next_output_addr;
				}
			}
			for (i = 0; i < MAX_ISP_STAT_BUF; i++) {
				stat_buf_pt = &stat_bq->isp_stat[i];
				if (stat_buf_pt->isp_stat_buf.buf_status ==
				    BUF_IDLE) {
					vin_log(VIN_LOG_VIDEO, "find isp stat buf idle!\n");
					list_move_tail(&stat_buf_pt->queue,
						       &stat_bq->active);
					stat_buf_pt->isp_stat_buf.buf_status =
					    BUF_ACTIVE;
				}
			}

			vin_log(VIN_LOG_VIDEO, "before list empty active = %p\n",
				&stat_bq->active);
			vin_log(VIN_LOG_VIDEO, "before list empty active.prev = %p\n",
				stat_bq->active.prev);
			vin_log(VIN_LOG_VIDEO, "before list empty active.next = %p\n",
				stat_bq->active.next);

			/*judge if the isp stat queue has been written to the last*/
			if (list_empty(&stat_bq->active)) {
				vin_err("No active isp stat queue to serve\n");
				goto set_next_output_addr;
			}
			vin_log(VIN_LOG_VIDEO, "after list empty active = %p\n",
				&stat_bq->active);
			vin_log(VIN_LOG_VIDEO, "after list empty active.prev = %p\n",
				stat_bq->active.prev);
			vin_log(VIN_LOG_VIDEO, "after list empty active.next = %p\n",
				stat_bq->active.next);

			/*
			 * delete the ready buffer from the actvie queue
			 * add the ready buffer to the locked queue
			 */
			stat_buf_pt =
			    list_entry(stat_bq->active.next,
				       struct vin_isp_stat_buf, queue);

			list_move_tail(&stat_buf_pt->queue, &stat_bq->locked);
			stat_buf_pt->isp_stat_buf.buf_status = BUF_LOCKED;
			isp->isp_gen_set_pt->stat.stat_buf_whole =
			    &stat_bq->isp_stat[stat_buf_pt->id].
			    isp_stat_buf;
			stat_bq->isp_stat[stat_buf_pt->id].isp_stat_buf.frame_number++;

			if ((&stat_bq->active) == stat_bq->active.next->next) {
				vin_warn("No more free stat buf next time\n");
				goto set_next_output_addr;
			}
		}
	}

set_isp_stat_addr:
	if (inst->is_isp_used && inst->is_bayer_raw) {
		stat_buf_pt =
		    list_entry(stat_bq->active.next,
			       struct vin_isp_stat_buf, queue);
		bsp_isp_set_statistics_addr((unsigned long)(stat_buf_pt->dma_addr));
	}
set_next_output_addr:
	if (list_empty(&cap->vidq_active)
	    || cap->vidq_active.next->next == (&cap->vidq_active)) {
		vin_print("No active queue to serve\n");
		goto unlock;
	}
	buf = list_entry(cap->vidq_active.next->next, struct vin_buffer, list);
	vin_set_addr(vinc, &buf->vb, &vinc->vid_cap.frame,
			&vinc->vid_cap.frame.paddr);

unlock:
	spin_unlock_irqrestore(&cap->slock, flags);

	if (((cap->capture_mode == V4L2_MODE_VIDEO)
	     || (cap->capture_mode == V4L2_MODE_PREVIEW))
	    && inst->is_isp_used
	    && bsp_isp_get_irq_status(FINISH_INT_EN)) {
		/* if(bsp_isp_get_para_ready()) */
		{
			vin_log(VIN_LOG_VIDEO, "call tasklet schedule! \n");
			bsp_isp_clr_para_ready();
			schedule_work(&vinc->isp_isr_bh_task);
			bsp_isp_set_para_ready();
		}
	}
	if (inst->is_isp_used) {
		bsp_isp_clr_irq_status(FINISH_INT_EN);
		bsp_isp_irq_enable(FINISH_INT_EN);
	} else {
		bsp_csi_int_clear_status(vinc->csi_sel, vinc->cur_ch,
					 CSI_INT_FRAME_DONE);
		if ((cap->capture_mode == V4L2_MODE_VIDEO)
		    || (cap->capture_mode == V4L2_MODE_PREVIEW))
			bsp_csi_int_enable(vinc->csi_sel, vinc->cur_ch,
					   CSI_INT_FRAME_DONE);
	}

	sunxi_isp_frame_sync_isr(cap->pipe.sd[VIN_IND_ISP]);
	return IRQ_HANDLED;
}

static int vin_irq_request(struct vin_core *vinc, int i)
{
	int ret;
	struct device_node *np = vinc->pdev->dev.of_node;
	vin_log(VIN_LOG_VIDEO, "get irq resource\n");
	/*get irq resource */
	vinc->vid_cap.irq = irq_of_parse_and_map(np, i);
	if (vinc->vid_cap.irq <= 0) {
		vin_err("failed to get IRQ resource\n");
		return -ENXIO;
	}
#ifndef FPGA_VER
	ret = request_irq(vinc->vid_cap.irq, vin_isr, IRQF_DISABLED,
			vinc->pdev->name, &vinc->vid_cap);
#else
	ret = request_irq(vinc->vid_cap.irq, vin_isr, IRQF_SHARED,
			vinc->pdev->name, &vinc->vid_cap);
#endif
	if (ret) {
		vin_err("failed to install irq (%d)\n", ret);
		return -ENXIO;
	}
	return 0;
}

static void vin_irq_release(struct vin_core *vinc)
{
	if (vinc->vid_cap.irq > 0)
		free_irq(vinc->vid_cap.irq, &vinc->vid_cap);
}

int __vin_request_gpio(struct vin_core *vinc)
{
#ifdef VIN_GPIO
	unsigned int j;
	for (j = 0; j < MAX_GPIO_NUM; j++) {
		os_gpio_request(&vinc->modu_cfg.sensors.gpio[j], 1);
	}
#endif
	return 0;
}

static int vin_core_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct vin_core *vinc;
	enum module_type type;
	int ret = 0;
	unsigned int i;

	vin_log(VIN_LOG_VIDEO, "%s\n", __func__);
	/*request mem for vinc */
	vinc = kzalloc(sizeof(struct vin_core), GFP_KERNEL);
	if (!vinc) {
		vin_err("request vinc mem failed!\n");
		ret = -ENOMEM;
		goto ekzalloc;
	}

	pdev->id = of_alias_get_id(np, "vinc");
	if (pdev->id < 0) {
		vin_err("failed to get alias id\n");
		ret = -EINVAL;
		goto freedev;
	}

	if (of_property_read_u32(np, "cci_sel", &vinc->modu_cfg.bus_sel))
		vinc->modu_cfg.bus_sel = 0;
	if (of_property_read_u32(np, "csi_sel", &vinc->csi_sel))
		vinc->csi_sel = 0;
	if (of_property_read_u32(np, "mipi_sel", &vinc->mipi_sel))
		vinc->mipi_sel = 0xff;
	if (of_property_read_u32(np, "isp_sel", &vinc->vid_cap.isp_sel))
		vinc->mipi_sel = 0xff;
	if (of_property_read_u32(np, "scaler_sel", &vinc->scaler_sel))
		vinc->scaler_sel = 0;

	vinc->pipe_cfg.mipi_ind = vinc->mipi_sel;
	vinc->pipe_cfg.csi_ind = vinc->csi_sel;
	vinc->pipe_cfg.isp_ind = vinc->vid_cap.isp_sel;
	vinc->pipe_cfg.scaler_ind = vinc->scaler_sel;
#if defined(CONFIG_CCI_MODULE) || defined (CONFIG_CCI)
	type = VIN_MODULE_TYPE_CCI;
#elif defined(CONFIG_I2C)
	type = VIN_MODULE_TYPE_I2C;
#endif
	vinc->modu_cfg.modules.sensor[0].type = type;
	vinc->modu_cfg.modules.sensor[1].type = type;
	vinc->modu_cfg.modules.sensor[2].type = type;
	vinc->modu_cfg.modules.act[0].type = type;
	vinc->modu_cfg.modules.flash.type = type;
	vinc->platform_id = SUNXI_PLATFORM_ID;

	vinc->id = pdev->id;
	vinc->pdev = pdev;
	vinc->vin_sensor_power_cnt = 0;
	dev_set_drvdata(&vinc->pdev->dev, vinc);

	vin_print("pdev->id = %d\n", pdev->id);
	vin_print("cci_i2c_sel = %d\n", vinc->modu_cfg.bus_sel);
	vin_print("csi_sel = %d\n", vinc->csi_sel);
	vin_print("mipi_sel = %d\n", vinc->mipi_sel);
	vin_print("isp_sel = %d\n", vinc->vid_cap.isp_sel);
	vin_print("scaler_sel = %d\n", vinc->scaler_sel);

	vin_log(VIN_LOG_VIDEO, "parse device tree!\n");
	/* parse device tree */
	vinc->modu_cfg.sensors.inst[0].cam_addr = i2c_addr;
	strcpy(vinc->modu_cfg.sensors.inst[0].cam_name, ccm);
	strcpy(vinc->modu_cfg.sensors.inst[0].isp_cfg_name, ccm);
	vinc->modu_cfg.sensors.inst[0].act_addr = act_slave;
	strcpy(vinc->modu_cfg.sensors.inst[0].act_name, act_name);
	vinc->modu_cfg.sensors.use_sensor_list = use_sensor_list;
	for (i = 0; i < MAX_GPIO_NUM; i++)
		vinc->modu_cfg.sensors.gpio[i].gpio = GPIO_INDEX_INVALID;

	vin_core_gbl[vinc->id] = vinc;

	ret = parse_device_tree(vinc);
	if (ret) {
		vin_err("parse device tree failed!\n");
		goto freedev;
	}

	__vin_request_gpio(vinc);

	vin_irq_request(vinc, 0);

	ret = vin_initialize_capture_subdev(vinc);
	if (ret)
		goto freedev;

	/*initial parameter */
	vinc->cur_ch = 0;
	vinc->isp_init_para.isp_src_ch_mode = ISP_SINGLE_CH;
	for (i = 0; i < MAX_ISP_SRC_CH_NUM; i++)
		vinc->isp_init_para.isp_src_ch_en[i] = 0;
	vinc->isp_init_para.isp_src_ch_en[vinc->id] = 1;

	ret = sysfs_create_group(&vinc->pdev->dev.kobj, &vin_attribute_group);
	if (ret) {
		vin_err("sysfs_create_group failed!\n");
		goto freedev;
	}

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_enable(&pdev->dev);
#endif
	return 0;
freedev:
	kfree(vinc);
ekzalloc:
	vin_print("%s error!\n", __func__);
	return ret;
}

static int vin_core_remove(struct platform_device *pdev)
{
	struct vin_core *vinc = (struct vin_core *)dev_get_drvdata(&pdev->dev);

	mutex_destroy(&vinc->vid_cap.stream_lock);
	mutex_destroy(&vinc->vid_cap.opened_lock);
	sysfs_remove_group(&vinc->pdev->dev.kobj, &vin_attribute_group);

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_disable(&vinc->pdev->dev);
#endif
	vin_gpio_release(vinc);
	vin_irq_release(vinc);
	vin_cleanup_capture_subdev(vinc);
	kfree(vinc);
	vin_print("%s end\n", __func__);
	return 0;
}

static const char *vin_regulator_name[] = {
	VFE_ISP_REGULATOR,
	VFE_CSI_REGULATOR,
};

int __vin_enable_regulator_all(struct vin_core *vinc)
{
	struct regulator *regul;
	unsigned int i, ret = -1;
	for (i = 0; i < ARRAY_SIZE(vin_regulator_name); ++i) {
		if (strcmp(vin_regulator_name[i], "") != 0) {
			regul = regulator_get(NULL, vin_regulator_name[i]);
			if (IS_ERR_OR_NULL(regul)) {
				vin_err("get regulator error, i = %d!\n", i);
				regul = NULL;
			}
		} else {
			regul = NULL;
		}
		vinc->vin_system_power[i] = regul;
		if (vinc->vin_system_power[i] != NULL) {
			ret = regulator_enable(vinc->vin_system_power[i]);
		}
	}
	usleep_range(5000, 6000);
	return ret;
}

int __vin_disable_regulator_all(struct vin_core *vinc)
{
	unsigned int i, ret = -1;
	for (i = 0; i < ARRAY_SIZE(vin_regulator_name); ++i) {
		if (vinc->vin_system_power[i] != NULL) {
			ret = regulator_disable(vinc->vin_system_power[i]);
			regulator_put(vinc->vin_system_power[i]);
		}
	}
	return ret;
}

void __vin_clk_open(struct vin_core *vinc)
{
	vin_print("%s\n", __func__);
	v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_CSI], core, s_power, 1);
	v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_MIPI], core, s_power, 1);
}
void __vin_clk_close(struct vin_core *vinc)
{
	vin_print("%s\n", __func__);
	v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_CSI], core, s_power, 0);
	v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_MIPI], core, s_power, 0);
}

int __vin_gpio_config(struct vin_core *vinc, int bon)
{
#ifdef VIN_GPIO
	unsigned int j;
	struct gpio_config gpio_item;
	for (j = 0; j < MAX_GPIO_NUM; j++) {
		memcpy(&gpio_item, &vinc->modu_cfg.sensors.gpio[j],
		       sizeof(struct gpio_config));
		if (0 == bon)
			gpio_item.mul_sel = GPIO_DISABLE;
		os_gpio_set(&gpio_item, 1);
	}
#endif
	return 0;
}

void vin_gpio_release(struct vin_core *vinc)
{
#ifdef VIN_GPIO
	unsigned int j;
	for (j = 0; j < MAX_GPIO_NUM; j++)
		os_gpio_release(vinc->modu_cfg.sensors.gpio[j].gpio, 1);
#endif
}

#ifdef CONFIG_PM_SLEEP
int vin_core_suspend(struct device *d)
{
	struct vin_core *vinc = (struct vin_core *)dev_get_drvdata(d);
	vin_print("%s\n", __func__);
	if (vin_is_opened(&vinc->vid_cap)) {
		vin_err("FIXME: vinc %s, err happened when calling %s.",
			dev_name(&vinc->pdev->dev), __func__);
		return -1;
	}
	__vin_clk_close(vinc);
	__vin_disable_regulator_all(vinc);
	__vin_gpio_config(vinc, 0);
	return 0;
}
int vin_core_resume(struct device *d)
{
	struct vin_core *vinc = (struct vin_core *)dev_get_drvdata(d);
	vin_print("%s\n", __func__);
	__vin_gpio_config(vinc, 1);
	__vin_enable_regulator_all(vinc);
	__vin_clk_open(vinc);
	return 0;
}
#endif

#ifdef CONFIG_PM_RUNTIME
int vin_core_runtime_suspend(struct device *d)
{
	struct vin_core *vinc = (struct vin_core *)dev_get_drvdata(d);
	vin_print("%s\n", __func__);
	__vin_clk_close(vinc);
	__vin_disable_regulator_all(vinc);
	__vin_gpio_config(vinc, 0);
	return 0;
}
int vin_core_runtime_resume(struct device *d)
{
	struct vin_core *vinc = (struct vin_core *)dev_get_drvdata(d);
	vin_print("%s\n", __func__);
	__vin_gpio_config(vinc, 1);
	__vin_enable_regulator_all(vinc);
	__vin_clk_open(vinc);
	return 0;
}
int vin_core_runtime_idle(struct device *d)
{
	if (d) {
		pm_runtime_mark_last_busy(d);
		pm_request_autosuspend(d);
	} else {
		vin_err("%s, vfe device is null\n", __func__);
	}
	return 0;
}
#endif
static void vin_core_shutdown(struct platform_device *pdev)
{
#if defined (CONFIG_PM_RUNTIME) || defined(CONFIG_SUSPEND)
	vin_print("Defined suspend!\n");
#else
	struct vin_core *vinc = (struct vin_core *)dev_get_drvdata(&pdev->dev);
	int ret = 0;
	ret = vin_set_sensor_power_off(vinc);
	if (ret)
		vin_err("sensor power off error!\n");
#endif
	vin_print("%s\n", __func__);
}

static const struct dev_pm_ops vin_core_runtime_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(vin_core_suspend, vin_core_resume)
	SET_RUNTIME_PM_OPS(vin_core_runtime_suspend, vin_core_runtime_resume,
			       vin_core_runtime_idle)
};

static const struct of_device_id sunxi_vin_core_match[] = {
	{.compatible = "allwinner,sunxi-vin-core",},
	{},
};

static struct platform_driver vin_core_driver = {
	.probe = vin_core_probe,
	.remove = vin_core_remove,
	.shutdown = vin_core_shutdown,
	.driver = {
		   .name = VIN_CORE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_vin_core_match,
		   .pm = &vin_core_runtime_pm_ops,
		   }
};
int sunxi_vin_core_register_driver(void)
{
	int ret;
	vin_print("vin core register driver\n");
	ret = platform_driver_register(&vin_core_driver);
	return ret;
}

void sunxi_vin_core_unregister_driver(void)
{
	vin_print("vin core unregister driver\n");
	platform_driver_unregister(&vin_core_driver);
}

struct vin_core *sunxi_vin_core_get_dev(int index)
{
	return vin_core_gbl[index];
}

static int __vin_core_sensor_verify(struct v4l2_subdev *sensor)
{
	int ret = 0;
	vin_print("Check sensor \"%s\"!\n", sensor->name);
	v4l2_subdev_call(sensor, core, s_power, PWR_ON);
	ret = v4l2_subdev_call(sensor, core, init, 0);
	if (ret == 0)
		vin_print("Camera \"%s\" verify OK!\n", sensor->name);
	if (vin_i2c_dbg) {
		vin_print("IIC dbg: sensor always power on for test!\n");
	} else {
		v4l2_subdev_call(sensor, core, s_power, PWR_OFF);
	}

	return ret;
}

int vin_core_check_sensor_list(struct vin_core *vinc, int i)
{
	struct v4l2_subdev *csi = sunxi_csi_get_subdev(vinc->csi_sel);
	struct modules_config *modu_cfg = &vinc->modu_cfg;
	struct sensor_list *sensors = &vinc->modu_cfg.sensors;
	struct v4l2_subdev *sensor = NULL;

	sensor = modu_cfg->modules.sensor[i].sd;
	if (sensor == NULL) {
		sensors->valid_idx = -1;
		goto check_end;
	}
	vinc->csi_sd = sunxi_csi_get_subdev(vinc->csi_sel);
	v4l2_subdev_call(csi, core, s_power, 1);
	__vin_enable_regulator_all(vinc);
	__vin_gpio_config(vinc, 1);
	csi_cci_init_helper(vinc->modu_cfg.bus_sel);
	if (!__vin_core_sensor_verify(sensor)) {
		sensors->valid_idx = i;
	} else {
		sensors->valid_idx = -1;
	}
	if (vin_i2c_dbg)
		goto check_end;
	csi_cci_exit_helper(vinc->modu_cfg.bus_sel);
	__vin_gpio_config(vinc, 0);
	__vin_disable_regulator_all(vinc);
	v4l2_subdev_call(csi, core, s_power, 0);
check_end:
	vin_print("the final valid index is %d\n", sensors->valid_idx);
	return sensors->valid_idx;
}

