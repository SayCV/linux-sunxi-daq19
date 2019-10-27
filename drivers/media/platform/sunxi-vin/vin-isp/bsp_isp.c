
/*
 ******************************************************************************
 *
 * bsp_isp.c
 *
 * Hawkview ISP - bsp_isp.c module
 *
 * Copyright (c) 2013 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   1.0		Yang Feng   	2013/11/07	    First Version
 *
 ******************************************************************************
 */

#include <linux/string.h>
#include <linux/kernel.h>

#include "bsp_isp.h"
#include "isp_platform_drv.h"

#include "bsp_isp_comm.h"
#include "bsp_isp_algo.h"

#define  Q16_1_1                  ((1 << 16) >> 0)
#define  Q16_1_2                  ((1 << 16) >> 1)
#define  Q16_1_4                  ((1 << 16) >> 2)
static int isp_platform_id;
struct isp_bsp_fun_array *fun_array_curr;

void bsp_isp_enable(void)
{
	fun_array_curr->isp_enable();
}

void bsp_isp_disable(void)
{
	fun_array_curr->isp_disable();
}
void bsp_isp_rot_enable(void)
{
	fun_array_curr->isp_module_enable(ROT_EN);
}

void bsp_isp_rot_disable(void)
{

	fun_array_curr->isp_module_disable(ROT_EN);
}

void bsp_isp_set_rot(enum isp_channel ch, enum isp_rot_angle angle)
{
	fun_array_curr->isp_rot_src_ch_sel(ch);
	fun_array_curr->isp_rot_set_angle(angle);
}

int min_scale_w_shift(int x_ratio, int y_ratio)
{

	int m, n;
	int sum_weight = 0;
	int weight_shift = -8;
	int x = (x_ratio >> 8) + 1;
	int y = (y_ratio >> 8) + 1;

	for (m = 0; m <= x; m++) {
		for (n = 0; n <= y; n++) {
			sum_weight +=
				(y - abs((n << 8) - (y << 7))) *
				(x - abs((m << 8) - (x << 7)));
		}
	}

	sum_weight >>= 8;
	while (sum_weight != 0) {
		weight_shift++;
		sum_weight >>= 1;
	}

	return weight_shift;
}


void bsp_isp_channel_enable(enum isp_channel ch)
{
	fun_array_curr->isp_channel_enable(ch);
}

void bsp_isp_channel_disable(enum isp_channel ch)
{


	fun_array_curr->isp_channel_disable(ch);
}
void bsp_isp_video_capture_start(void)
{

	fun_array_curr->isp_capture_start(VCAP_EN);
}

void bsp_isp_video_capture_stop(void)
{

	fun_array_curr->isp_capture_stop(VCAP_EN);
}

void bsp_isp_image_capture_start(void)
{

	fun_array_curr->isp_capture_start(SCAP_EN);
}

void bsp_isp_image_capture_stop(void)
{

	fun_array_curr->isp_capture_stop(SCAP_EN);
}

unsigned int bsp_isp_get_para_ready(void)
{
	return fun_array_curr->isp_get_para_ready();
}


void bsp_isp_set_para_ready(void)
{
	fun_array_curr->isp_set_para_ready(PARA_READY);
}
void bsp_isp_clr_para_ready(void)
{
	fun_array_curr->isp_set_para_ready(PARA_NOT_READY);
}

/*
 * irq_flag:
 *
 * FINISH_INT_EN
 * START_INT_EN
 * PARA_SAVE_INT_EN
 * PARA_LOAD_INT_EN
 * SRC0_FIFO_INT_EN
 * SRC1_FIFO_INT_EN
 * ROT_FINISH_INT_EN
 * ISP_IRQ_EN_ALL
 */

void bsp_isp_irq_enable(unsigned int irq_flag)
{
	fun_array_curr->isp_irq_enable(irq_flag);
}
void bsp_isp_irq_disable(unsigned int irq_flag)
{
	fun_array_curr->isp_irq_disable(irq_flag);
}

unsigned int bsp_isp_get_irq_status(unsigned int irq)
{
	return fun_array_curr->isp_get_irq_status(irq);
}


void bsp_isp_clr_irq_status(unsigned int irq)
{
	fun_array_curr->isp_clr_irq_status(irq);
}

int bsp_isp_int_get_enable()
{
	return fun_array_curr->isp_int_get_enable();
}


void bsp_isp_set_statistics_addr(unsigned int addr)
{
	fun_array_curr->isp_set_statistics_addr(addr);
}
void bsp_isp_set_flip(enum isp_channel ch, enum enable_flag on_off)
{
	fun_array_curr->isp_flip_enable(ch, on_off);
}

void bsp_isp_set_mirror(enum isp_channel ch, enum enable_flag on_off)
{
	fun_array_curr->isp_mirror_enable(ch, on_off);
}

void bsp_isp_set_base_addr(unsigned long vaddr)
{
	fun_array_curr->map_reg_addr(vaddr);
}

void bsp_isp_set_dma_load_addr(unsigned long dma_addr)
{
	fun_array_curr->isp_set_load_addr(dma_addr);
}

void bsp_isp_set_dma_saved_addr(unsigned long dma_addr)
{
	fun_array_curr->isp_set_saved_addr(dma_addr);
}

void bsp_isp_set_map_load_addr(unsigned long vaddr)
{
	fun_array_curr->map_load_dram_addr(vaddr);
}

void bsp_isp_set_map_saved_addr(unsigned long vaddr)
{
	fun_array_curr->map_saved_dram_addr(vaddr);
}

void bsp_isp_update_lut_lens_gamma_table(struct isp_table_addr *tbl_addr)
{
	fun_array_curr->isp_set_table_addr(LUT_LENS_GAMMA_TABLE,
		(unsigned long)(tbl_addr->isp_def_lut_tbl_dma_addr));
}

void bsp_isp_update_drc_table(struct isp_table_addr *tbl_addr)
{
	fun_array_curr->isp_set_table_addr(DRC_TABLE,
		(unsigned long)(tbl_addr->isp_drc_tbl_dma_addr));
}
void bsp_isp_set_input_fmt(enum isp_input_fmt fmt,
				enum isp_input_seq seq_t)
{
	fun_array_curr->isp_set_input_fmt(fmt, seq_t);
}
void bsp_isp_set_output_fmt(enum isp_output_fmt isp_fmt,
				enum isp_output_seq seq_t,
				enum isp_channel ch)
{
	fun_array_curr->isp_set_output_fmt(isp_fmt, seq_t, ch);
}

void bsp_isp_set_ob_zone(struct isp_size *black, struct isp_size *valid,
			struct coor *xy, enum isp_src obc_valid_src)
{
	fun_array_curr->isp_set_ob_zone(black, valid, xy, obc_valid_src);
}

void bsp_isp_set_output_size(enum isp_channel ch, struct isp_size *size)
{
	fun_array_curr->isp_set_output_size(ch, size);
}
void bsp_isp_scale_cfg(enum isp_channel ch, int x_ratio, int y_ratio,
			int weight_shift)
{
	fun_array_curr->isp_scale_cfg(ch, x_ratio, y_ratio, weight_shift);
}
void bsp_isp_set_stride_y(unsigned int stride_val, enum isp_channel ch)
{
	fun_array_curr->isp_set_stride_y(stride_val, ch);
}
void bsp_isp_set_stride_uv(unsigned int stride_val, enum isp_channel ch)
{
	fun_array_curr->isp_set_stride_uv(stride_val, ch);
}
void bsp_isp_set_yuv_addr(struct isp_yuv_channel_addr *addr,
			enum isp_channel ch, enum isp_src channel_src)
{
	fun_array_curr->isp_set_yuv_addr(addr, ch, channel_src);
}


void bsp_isp_scale_enable(enum isp_channel ch)
{
	fun_array_curr->isp_scale_enable(ch);
}
void bsp_isp_module_enable(unsigned int module)
{
	fun_array_curr->isp_module_enable(module);
}

void bsp_isp_module_disable(unsigned int module)
{
	fun_array_curr->isp_module_disable(module);
}

void bsp_isp_init(struct isp_init_para *para)
{
	unsigned int i, j;

	enum isp_src_interface isp_src_sel[] = {ISP_CSI0, ISP_CSI1, ISP_CSI2};

	enum isp_src isp_src_ch[] = {ISP_SRC0, ISP_SRC1};


	fun_array_curr->isp_module_enable(SRC0_EN);

	j = 0;

	for (i = 0; i < MAX_ISP_SRC_CH_NUM; i++) {

		if (para->isp_src_ch_en[i] == 1) {

			fun_array_curr->isp_set_interface(isp_src_sel[i],
							isp_src_ch[j]);
			j++;
			if (para->isp_src_ch_mode == ISP_SINGLE_CH) {
				if (j == 1)
					break;
			} else if (para->isp_src_ch_mode == ISP_DUAL_CH) {
				if (j == 2)
					break;
			}
		}
	}
}


void bsp_isp_exit(void)
{
	bsp_isp_disable();
	bsp_isp_irq_disable(ISP_IRQ_EN_ALL);
	bsp_isp_video_capture_stop();
	bsp_isp_rot_disable();
	bsp_isp_set_mirror(MAIN_CH, DISABLE);
	bsp_isp_set_mirror(SUB_CH, DISABLE);
	bsp_isp_set_flip(MAIN_CH, DISABLE);
	bsp_isp_set_flip(SUB_CH, DISABLE);
	fun_array_curr->isp_module_disable(SRC0_EN);
}

unsigned int bsp_isp_get_saved_cfa_min_rgb(void)
{
	return fun_array_curr->isp_get_saved_cfa_min_rgb();
}

unsigned int bsp_isp_get_saved_cfa_pic_tex(void)
{
	return fun_array_curr->isp_get_saved_cfa_pic_tex();
}

void bsp_isp_get_saved_awb_avp(struct isp_awb_avp_stat *awb_avp_saved)
{
	fun_array_curr->isp_get_saved_awb_avp(awb_avp_saved);
}
void bsp_isp_get_saved_awb_diff_thresh(struct isp_wb_diff_threshold *diff_th_saved)
{
	fun_array_curr->isp_get_saved_awb_diff_thresh(diff_th_saved);
}
unsigned short bsp_isp_get_saved_awb_sum_thresh(void)
{
	return fun_array_curr->isp_get_saved_awb_sum_thresh();
}

void bsp_isp_get_saved_ae_win_reg(struct isp_h3a_reg_win *ae_reg_win_saved)
{
	fun_array_curr->isp_get_saved_ae_win_reg(ae_reg_win_saved);
}
int bsp_isp_get_saved_cnr_noise(void)
{
	return fun_array_curr->isp_get_saved_cnr_noise();
}

void bsp_isp_print_reg_saved()
{
	fun_array_curr->isp_print_reg_saved();
}

void bsp_isp_init_platform(unsigned int platform_id)
{
	struct isp_platform_drv *isp_platform;


	isp_platform_id = platform_id;
	isp_platform_init(platform_id);
	isp_platform = isp_get_driver();
	fun_array_curr = isp_platform->fun_array;
}
