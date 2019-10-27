
/*
 ******************************************************************************
 *
 * sunxi_isp.h
 *
 * Hawkview ISP - sunxi_isp.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2014/12/11	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef _SUNXI_ISP_H_
#define _SUNXI_ISP_H_
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#include "../vin-video/vin_core.h"

#include "../vin-stat/vin_ispstat.h"
#include "../vin-stat/vin_h3a.h"

#define isp_dbg(l, x, arg...) { \
	if (isp_dbg_en && l <= isp_dbg_lv) \
	printk(KERN_DEBUG"[ISP_DEBUG]"x, ##arg);\
}
#define isp_err(x, arg...) printk(KERN_ERR"[ISP_ERR]"x, ##arg)
#define isp_warn(x, arg...) printk(KERN_WARNING"[ISP_WARN]"x, ##arg)
#define isp_print(x, arg...) printk(KERN_NOTICE"[ISP]"x, ##arg)

#define VIDIOC_SUNXI_ISP_MAIN_CH_CFG 		1

enum isp_pad {
	ISP_PAD_SINK,
	ISP_PAD_SOURCE_ST,
	ISP_PAD_SOURCE,
	ISP_PAD_NUM,
};

struct main_channel_cfg {
	enum bus_pixeltype bus_code;
	struct sensor_win_size win_cfg;
	struct v4l2_pix_format pix;
};

struct isp_pix_fmt {
	enum v4l2_mbus_pixelcode mbus_code;
	char *name;
	u32 fourcc;
	u32 color;
	u16 memplanes;
	u16 colplanes;
	u32 depth[VIDEO_MAX_PLANES];
	u16 mdataplanes;
	u16 flags;
};

struct isp_yuv_size_addr_info {
	unsigned int isp_byte_size;
	unsigned int line_stride_y;
	unsigned int line_stride_c;
	unsigned int buf_height_y;
	unsigned int buf_height_cb;
	unsigned int buf_height_cr;

	unsigned int valid_height_y;
	unsigned int valid_height_cb;
	unsigned int valid_height_cr;
	struct isp_yuv_channel_addr yuv_addr;
};

struct sunxi_isp_ctrls {
	struct v4l2_ctrl_handler handler;

	struct v4l2_ctrl *hflip;
	struct v4l2_ctrl *vflip;
	struct v4l2_ctrl *rotate;
	struct v4l2_ctrl *wb_gain[4];	/* wb gain cluster */
	struct v4l2_ctrl *ae_win[4];	/* wb win cluster */
	struct v4l2_ctrl *af_win[4];	/* af win cluster */
};
struct isp_fmt_cfg {
	int rot_angle;
	int rot_ch;
	enum bus_pixeltype bus_code;
	unsigned int isp_fmt[ISP_MAX_CH_NUM];
	struct isp_size isp_size[ISP_MAX_CH_NUM];
	struct isp_size ob_black_size;
	struct isp_size ob_valid_size;
	struct coor ob_start;
};

struct isp_dev {
	int use_cnt;
	struct v4l2_subdev subdev;
	struct media_pad isp_pads[ISP_PAD_NUM];
	struct v4l2_event event;
	struct platform_device *pdev;
	struct list_head isp_list;
	struct sunxi_isp_ctrls ctrls;
	int vflip;
	int hflip;
	int rotate;
	unsigned int id;
	spinlock_t slock;
	struct mutex subdev_lock;
	wait_queue_head_t wait;
	void __iomem *base;
	struct resource *ioarea;
	struct vin_mm isp_load_reg_mm;
	struct vin_mm isp_save_reg_mm;
	struct vin_mm isp_lut_tbl_buf_mm;
	struct vin_mm isp_drc_tbl_buf_mm;
	struct vin_mm isp_stat_buf_mm;
	struct isp_table_addr isp_tbl_addr;
	struct vin_isp_stat_buf_queue isp_stat_bq;
	struct isp_gen_settings *isp_gen_set_pt;
	struct isp_3a_result isp_3a_result;
	struct isp_3a_result *isp_3a_result_pt;
	int rotation_en;
	struct isp_fmt_cfg isp_fmt;
	struct v4l2_mbus_framefmt format[ISP_PAD_NUM];

	struct {
		struct v4l2_rect request;
		struct v4l2_rect active;
	} crop;

	enum enable_flag flip_en_glb[ISP_MAX_CH_NUM];
	int plannar_uv_exchange_flag[ISP_MAX_CH_NUM];
	struct isp_yuv_size_addr_info isp_yuv_size_addr[ISP_MAX_CH_NUM];
	struct isp_stat h3a_stat;
};

void update_isp_setting(struct v4l2_subdev *sd);

int isp_resource_request(struct v4l2_subdev *sd);
void isp_resource_release(struct v4l2_subdev *sd);

void sunxi_isp_set_fmt(struct isp_dev *isp, enum bus_pixeltype type,
		       unsigned int *fmt);
void sunxi_isp_set_flip(struct isp_dev *isp, enum isp_channel ch,
			enum enable_flag on_off);
void sunxi_isp_set_mirror(enum isp_channel ch, enum enable_flag on_off);
unsigned int sunxi_isp_set_size(struct isp_dev *isp, unsigned int *fmt,
				struct isp_size_settings *size_settings);
void sunxi_isp_set_output_addr(struct v4l2_subdev *sd,
			       unsigned long buf_base_addr);

void sunxi_isp_dump_regs(struct v4l2_subdev *sd);
void sunxi_isp_vsync_isr(struct v4l2_subdev *sd);
void sunxi_isp_frame_sync_isr(struct v4l2_subdev *sd);
struct v4l2_subdev *sunxi_isp_get_subdev(int id);
struct v4l2_subdev *sunxi_stat_get_subdev(int id);
int sunxi_isp_platform_register(void);
void sunxi_isp_platform_unregister(void);

#endif /*_SUNXI_ISP_H_*/
