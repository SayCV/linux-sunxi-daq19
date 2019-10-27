
/*
 ******************************************************************************
 *
 * vin_isp_helper.c
 *
 * Hawkview ISP - vin_isp_helper.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/11/30	ISP Tuning Tools Support
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

#include "vin_video.h"
#include "../utility/bsp_common.h"
#include "../vin-isp/bsp_isp_algo.h"
#include "../vin-cci/bsp_cci.h"
#include "../vin-cci/cci_helper.h"
#include "../utility/config.h"
#include "../modules/sensor/camera_cfg.h"
#include "../utility/sensor_info.h"
#include "../utility/vin_io.h"
#include "../vin-isp/sunxi_isp.h"

#include "vin_isp_helper.h"

int vin_is_opened(struct vin_vid_cap *cap)
{
	int ret;
	mutex_lock(&cap->opened_lock);
	ret = test_bit(0, &cap->opened);
	mutex_unlock(&cap->opened_lock);
	return ret;
}

void vin_start_opened(struct vin_vid_cap *cap)
{
	mutex_lock(&cap->opened_lock);
	set_bit(0, &cap->opened);
	mutex_unlock(&cap->opened_lock);
}

void vin_stop_opened(struct vin_vid_cap *cap)
{
	mutex_lock(&cap->opened_lock);
	clear_bit(0, &cap->opened);
	mutex_unlock(&cap->opened_lock);
}

/* The color format (colplanes, memplanes) must be already configured. */
int vin_set_addr(struct vin_core *vinc, struct vb2_buffer *vb,
		      struct vin_frame *frame, struct vin_addr *paddr)
{
	int ret = 0, valid_inst = 0;
	u32 pix_size;

	if (vb == NULL || frame == NULL)
		return -EINVAL;

	pix_size = ALIGN(frame->width, VIN_ALIGN_WIDTH)
		* ALIGN(frame->height, VIN_ALIGN_HEIGHT);

	vin_log(VIN_LOG_ISP, "memplanes= %d, colplanes= %d, pix_size= %d",
		frame->fmt->memplanes, frame->fmt->colplanes, pix_size);

	paddr->y = vb2_dma_contig_plane_dma_addr(vb, 0);

	if (frame->fmt->memplanes == 1) {
		switch (frame->fmt->colplanes) {
		case 1:
			paddr->cb = 0;
			paddr->cr = 0;
			break;
		case 2:
			/* decompose Y into Y/Cb */
			paddr->cb = (u32)(paddr->y + pix_size);
			paddr->cr = 0;
			break;
		case 3:
			paddr->cb = (u32)(paddr->y + pix_size);
			/* 420 */
			if (12 == frame->fmt->depth[0])
				paddr->cr = (u32)(paddr->cb + (pix_size >> 2));
			else /* 422 */
				paddr->cr = (u32)(paddr->cb + (pix_size >> 1));

			break;
		default:
			return -EINVAL;
		}
	} else if (!frame->fmt->mdataplanes) {
		if (frame->fmt->memplanes >= 2)
			paddr->cb = vb2_dma_contig_plane_dma_addr(vb, 1);

		if (frame->fmt->memplanes == 3)
			paddr->cr = vb2_dma_contig_plane_dma_addr(vb, 2);
	}

	vin_log(VIN_LOG_ISP, "PHYS_ADDR: y= 0x%X  cb= 0x%X cr= 0x%X ret= %d",
	    paddr->y, paddr->cb, paddr->cr, ret);

	valid_inst = vinc->modu_cfg.sensors.valid_idx;
	if (vinc->modu_cfg.sensors.inst[valid_inst].is_isp_used) {
		sunxi_isp_set_output_addr(vinc->vid_cap.pipe.sd[VIN_IND_ISP],
					  paddr->y);
	} else {
		bsp_csi_set_addr(vinc->csi_sel, paddr->y, paddr->y, paddr->cb,
				 paddr->cr);
	}
	return 0;
}

/*

void vin_set_addr(struct vin_core *dev, struct vin_buffer *buffer)
{
	dma_addr_t addr_org, addr_y, addr_u, addr_v;
	int i;
	struct vb2_buffer *vb_buf = &buffer->vb;
	if (vb_buf == NULL || vb_buf->planes[0].mem_priv == NULL) {
		vin_err("vb_buf->priv is NULL!\n");
		return;
	}
	addr_org =
	    vb2_dma_contig_plane_dma_addr(vb_buf,
					  0) - CPU_DRAM_PADDR_ORG +
	    HW_DMA_OFFSET;
	for (i = 0; i < dev->vid_cap.frame.fmt.memplanes; i++) {
		addr_org = vb2_dma_contig_plane_dma_addr(vb_buf, i)
			- CPU_DRAM_PADDR_ORG + HW_DMA_OFFSET;
	}

	addr_y =
	    vb2_dma_contig_plane_dma_addr(vb_buf,
					  0) - CPU_DRAM_PADDR_ORG +
	    HW_DMA_OFFSET;
	addr_u =
	    vb2_dma_contig_plane_dma_addr(vb_buf,
					  1) - CPU_DRAM_PADDR_ORG +
	    HW_DMA_OFFSET;
	addr_v =
	    vb2_dma_contig_plane_dma_addr(vb_buf,
					  2) - CPU_DRAM_PADDR_ORG +
	    HW_DMA_OFFSET;
	if (dev->ccm_cfg.sensor_list.camera_inst[0].is_isp_used) {
		sunxi_isp_set_output_addr(dev->vid_cap.pipe.sd[VIN_IND_ISP],
					  addr_org);
	} else {
		bsp_csi_set_addr(dev->csi_sel, addr_org, addr_y, addr_u,
				 addr_v);
	}
	vin_dbg(3, "csi_buf_addr_orginal=%pa\n", &addr_org);
}
*/

int vin_set_sensor_power_on(struct vin_core *vinc)
{
	int ret = 0;
	struct v4l2_subdev *sensor = vinc->vid_cap.pipe.sd[VIN_IND_SENSOR];
#ifdef CONFIG_ARCH_SUN8IW6P1
#else
	if (!IS_ERR_OR_NULL(sensor))
		vin_set_pmu_channel(sensor, IOVDD, ON);
	usleep_range(10000, 12000);
#endif
	ret = v4l2_subdev_call(sensor, core, s_power, PWR_ON);
	vinc->vin_sensor_power_cnt++;
	vin_log(VIN_LOG_POWER, "power_on______________________________\n");
	return ret;
}

int vin_set_sensor_power_off(struct vin_core *vinc)
{
	int ret = 0;
	struct v4l2_subdev *sensor = vinc->vid_cap.pipe.sd[VIN_IND_SENSOR];

	if (vinc->vin_sensor_power_cnt > 0) {
		ret = v4l2_subdev_call(sensor, core, s_power, PWR_OFF);
		vinc->vin_sensor_power_cnt--;
#ifdef CONFIG_ARCH_SUN8IW6P1
#else
		usleep_range(10000, 12000);
		if (!IS_ERR_OR_NULL(sensor))
			vin_set_pmu_channel(sensor, IOVDD, OFF);
#endif
	} else {
		vin_warn("Sensor is already power off!\n");
		vinc->vin_sensor_power_cnt = 0;
	}
	vin_log(VIN_LOG_POWER, "power_off______________________________\n");
	return ret;
}
