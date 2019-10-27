
/*
 ******************************************************************************
 *
 * isp_platform_drv.h
 *
 * Hawkview ISP - isp_platform_drv.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng   	2014/06/20	      Second Version
 *
 ******************************************************************************
 */
#ifndef _ISP_PLATFORM_DRV_H_
#define _ISP_PLATFORM_DRV_H_
#include <linux/string.h>
#include "bsp_isp_comm.h"

enum isp_channel_real {
	SUB_CH_REAL = 0,
	MAIN_CH_REAL = 1,
	ROT_CH_REAL = 2,
	ISP_MAX_CH_NUM_REAL,
};

struct isp_bsp_fun_array {

	void (*map_reg_addr) (unsigned long);
	void (*map_load_dram_addr) (unsigned long);
	void (*map_saved_dram_addr) (unsigned long);
	void (*isp_set_interface) (enum isp_src_interface, enum isp_src);
	void (*isp_enable) (void);
	void (*isp_disable) (void);
	void (*isp_set_para_ready) (enum ready_flag);
	unsigned int (*isp_get_para_ready) (void);
	void (*isp_capture_start) (enum isp_capture_mode);
	void (*isp_capture_stop) (enum isp_capture_mode);
	void (*isp_irq_enable) (unsigned int);
	void (*isp_irq_disable) (unsigned int);
	unsigned int (*isp_get_irq_status) (unsigned int);
	void (*isp_clr_irq_status) (unsigned int);
	int (*isp_int_get_enable) (void);
	void (*isp_set_line_int_num) (unsigned int);
	void (*isp_set_rot_of_line_num) (unsigned int);
	void (*isp_set_load_addr) (unsigned long);
	void (*isp_set_saved_addr) (unsigned long);
	void (*isp_set_table_addr) (enum isp_input_tables, unsigned long);
	void (*isp_set_statistics_addr) (unsigned long);
	void (*isp_module_enable) (unsigned int);
	void (*isp_module_disable) (unsigned int);
	void (*isp_set_input_fmt) (enum isp_input_fmt, enum isp_input_seq);
	void (*isp_set_ob_zone) (struct isp_size *, struct isp_size *, struct coor *, enum isp_src);
	void (*isp_set_output_size) (enum isp_channel, struct isp_size *);
	void (*isp_scale_cfg) (enum isp_channel, int, int, int);
	void (*isp_scale_enable) (enum isp_channel);
	void (*isp_scale_disable) (enum isp_channel);
	void (*isp_set_tb_scale_mode) (enum isp_scale_mode);
	void (*isp_rot_src_ch_sel) (enum isp_channel);
	void (*isp_rot_set_angle) (enum isp_rot_angle);
	void (*isp_channel_enable) (enum isp_channel);
	void (*isp_channel_disable) (enum isp_channel);
	void (*isp_tb_channel_sel) (enum isp_thumb_sel, enum isp_channel);
	void (*isp_set_output_fmt) (enum isp_output_fmt, enum isp_output_seq, enum isp_channel);
	void (*isp_mirror_enable) (enum isp_channel, enum enable_flag);
	void (*isp_flip_enable) (enum isp_channel, enum enable_flag);
	void (*isp_set_stride_y) (unsigned int, enum isp_channel);
	void (*isp_set_stride_uv) (unsigned int, enum isp_channel);
	void (*isp_set_yuv_addr) (struct isp_yuv_channel_addr *, enum isp_channel, enum isp_src);
	void (*isp_reg_test) (void);
	void (*isp_print_reg_saved) (void);
	unsigned int (*isp_get_saved_cfa_min_rgb) (void);
	unsigned int (*isp_get_saved_cfa_pic_tex) (void);
	void (*isp_get_saved_wb_gain) (struct isp_white_balance_gain *);
	void (*isp_get_saved_awb_avp) (struct isp_awb_avp_stat *);
	void (*isp_get_saved_awb_diff_thresh) (struct isp_wb_diff_threshold *);
	unsigned short (*isp_get_saved_awb_sum_thresh) (void);
	void (*isp_get_saved_ae_win_reg) (struct isp_h3a_reg_win *);
	int (*isp_get_saved_cnr_noise) (void);
};


struct isp_feature_array {

	void (*isp_update_table) (unsigned short);
	void (*isp_module_enable) (unsigned int);
	void (*isp_module_disable) (unsigned int);
	void (*isp_set_hist_src) (enum isp_src);
	void (*isp_set_hist_mode) (enum isp_hist_mode);
	void (*isp_set_bdnf_mode) (enum isp_bndf_mode);
	void (*isp_set_rgb_drc_mode) (enum isp_rgb_drc_mode);
	void (*isp_set_lut_dpc_mode) (enum isp_lut_dpc_mode);
	void (*isp_set_lut_dc) (unsigned short, enum isp_src);
	void (*isp_set_otf_dc) (unsigned short, unsigned short, unsigned short);
	void (*isp_set_2d_denoise_filter) (struct isp_denoise *);
	void (*isp_set_bayer_gain_offset) (struct isp_bayer_gain_offset *);
	void (*isp_set_yuv_gain_offset) (struct isp_yuv_gain_offset *);
	void (*isp_set_wb_gain) (struct isp_white_balance_gain *);
	void (*isp_set_wb_clip) (unsigned short);
	void (*isp_set_lsc) (struct isp_lsc_config *);
	void (*isp_set_dir_th) (unsigned int);
	void (*isp_set_sharp) (int, unsigned char, unsigned char);
	void (*isp_set_contrast) (unsigned char, unsigned char, unsigned char);
	void (*isp_set_saturation) (short, short, short, short);

	void (*isp_set_rgb2rgb_gain_offset) (struct isp_rgb2rgb_gain_offset *);
	void (*isp_set_afs_anti_flick) (unsigned char);
	void (*isp_set_cfa_min_rgb) (int);
	void (*isp_set_ae_win_reg) (struct isp_h3a_reg_win *);
	void (*isp_set_ae_bri_thresh) (unsigned short, unsigned short);
	void (*isp_set_af_win_reg) (struct isp_h3a_reg_win *);
	void (*isp_set_af_sap_lim) (unsigned short);
	void (*isp_set_awb_win_reg) (struct isp_h3a_reg_win *);
	void (*isp_set_awb_sum_thresh) (unsigned short);
	void (*isp_set_awb_diff_thresh) (struct isp_wb_diff_threshold *);
	void (*isp_set_awb_satur_lim) (unsigned short, unsigned short, unsigned short);
	void (*isp_set_cnr) (unsigned short, unsigned short);
	void (*isp_set_hist_win_reg) (struct isp_h3a_reg_win *);

	void (*isp_set_output_speed) (enum isp_output_speed);
	void (*isp_set_sprite_zone) (struct isp_size *, struct coor *);
	void (*isp_set_para_ready) (enum ready_flag);
	void (*isp_set_disc) (struct isp_disc_config *);
};

struct isp_platform_drv {
	int platform_id;
	struct isp_bsp_fun_array *fun_array;
	struct isp_feature_array *feature_array;
};

int isp_platform_register(struct isp_platform_drv *isp_drv);

int isp_platform_init(unsigned int platform_id);

struct isp_platform_drv *isp_get_driver(void);


#endif	/*_ISP_PLATFORM_DRV_H_*/