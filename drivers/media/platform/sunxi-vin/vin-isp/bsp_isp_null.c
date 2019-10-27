#include "bsp_isp.h"
#include "bsp_isp_comm.h"
#include "bsp_isp_algo.h"

void bsp_isp_enable(void)
{
}

void bsp_isp_disable(void)
{
}

void bsp_isp_rot_enable(void)
{
}

void bsp_isp_rot_disable(void)
{
}

void bsp_isp_set_rot(enum isp_channel ch, enum isp_rot_angle angle)
{
}

int min_scale_w_shift(int x_ratio, int y_ratio)
{
	return 0;
}

void bsp_isp_channel_enable(enum isp_channel ch)
{
}

void bsp_isp_channel_disable(enum isp_channel ch)
{
}
void bsp_isp_video_capture_start(void)
{
}

void bsp_isp_video_capture_stop(void)
{
}

void bsp_isp_image_capture_start(void)
{
}

void bsp_isp_image_capture_stop(void)
{
}

unsigned int bsp_isp_get_para_ready(void)
{
	return 0;
}

void bsp_isp_set_para_ready(void)
{
}
void bsp_isp_clr_para_ready(void)
{
}

void bsp_isp_irq_enable(unsigned int irq_flag)
{
}
void bsp_isp_irq_disable(unsigned int irq_flag)
{
}

unsigned int bsp_isp_get_irq_status(unsigned int irq)
{
	return 0;
}

void bsp_isp_clr_irq_status(unsigned int irq)
{
}

int bsp_isp_int_get_enable()
{
	return 0;
}

void bsp_isp_set_statistics_addr(unsigned int addr)
{
}
void bsp_isp_set_flip(enum isp_channel ch, enum enable_flag on_off)
{
}

void bsp_isp_set_mirror(enum isp_channel ch, enum enable_flag on_off)
{
}

void bsp_isp_set_base_addr(unsigned long vaddr)
{
}

void bsp_isp_set_dma_load_addr(unsigned long dma_addr)
{
}

void bsp_isp_set_dma_saved_addr(unsigned long dma_addr)
{
}

void bsp_isp_set_map_load_addr(unsigned long vaddr)
{
}

void bsp_isp_set_map_saved_addr(unsigned long vaddr)
{
}

void bsp_isp_update_lut_lens_gamma_table(struct isp_table_addr *tbl_addr)
{
}

void bsp_isp_update_drc_table(struct isp_table_addr *tbl_addr)
{
}
void bsp_isp_set_input_fmt(enum isp_input_fmt fmt, enum isp_input_seq seq_t)
{
}
void bsp_isp_set_output_fmt(enum isp_output_fmt isp_fmt,
			    enum isp_output_seq seq_t, enum isp_channel ch)
{
}

void bsp_isp_set_ob_zone(struct isp_size *black, struct isp_size *valid,
			 struct coor *xy, enum isp_src obc_valid_src)
{
}

void bsp_isp_set_output_size(enum isp_channel ch, struct isp_size *size)
{
}
void bsp_isp_scale_cfg(enum isp_channel ch, int x_ratio, int y_ratio,
		       int weight_shift)
{
}
void bsp_isp_set_stride_y(unsigned int stride_val, enum isp_channel ch)
{
}
void bsp_isp_set_stride_uv(unsigned int stride_val, enum isp_channel ch)
{
}
void bsp_isp_set_yuv_addr(struct isp_yuv_channel_addr *addr,
			  enum isp_channel ch, enum isp_src channel_src)
{
}

void bsp_isp_scale_enable(enum isp_channel ch)
{
}
void bsp_isp_module_enable(unsigned int module)
{
}

void bsp_isp_module_disable(unsigned int module)
{
}

void bsp_isp_init(struct isp_init_para *para)
{
}

void bsp_isp_exit(void)
{
}

unsigned int bsp_isp_get_saved_cfa_min_rgb(void)
{
	return 0;
}
unsigned int bsp_isp_get_saved_cfa_pic_tex(void)
{
	return 0;
}
void bsp_isp_get_saved_wb_gain(struct isp_white_balance_gain *wb_gain_saved)
{
}

void bsp_isp_get_saved_awb_avp(struct isp_awb_avp_stat *awb_avp_saved)
{
}
void bsp_isp_get_saved_awb_diff_thresh(struct isp_wb_diff_threshold
				       *diff_th_saved)
{
}
unsigned short bsp_isp_get_saved_awb_sum_thresh(void)
{
	return 0;
}
void bsp_isp_get_saved_ae_win_reg(struct isp_h3a_reg_win *ae_reg_win_saved)
{
}
int bsp_isp_get_saved_cnr_noise(void)
{
	return 0;
}

void bsp_isp_print_reg_saved()
{
}

void bsp_isp_init_platform(unsigned int platform_id)
{
}

