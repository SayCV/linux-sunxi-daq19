
/*
 ******************************************************************************
 *
 * sun8iw6p1_isp_reg_cfg.c
 *
 * Hawkview ISP - sun8iw6p1_isp_reg_cfg.c module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng   	2014/04/22	      Second Version
 *
 ******************************************************************************
 */

#include <linux/kernel.h>

#include "../isp_platform_drv.h"
#include "sun8iw6p1_isp_reg.h"
#include "sun8iw6p1_isp_reg_cfg.h"

#include <linux/kernel.h>
#ifdef ISP_SUN8I_DBG
#define ISP_SUN8I_REG_LOG do { \
		printk("[NOT SUPPORT]%s, line: %d\n", __FUNCTION__, __LINE__); \
	} while (0)
#else
#define ISP_SUN8I_REG_LOG
#endif

#define ABS(x) ((x) > 0 ? (x) : -(x))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/*
 *
 *  Load ISP register variables
 *
 */
ISP_FE_CFG_REG_t *sun8iw6p1_isp_fe_cfg;
ISP_FE_CTRL_REG_t *sun8iw6p1_isp_fe_ctrl;
ISP_FE_INT_EN_REG_t *sun8iw6p1_isp_fe_int_en;
ISP_FE_INT_STA_REG_t *sun8iw6p1_isp_fe_int_sta;
SUN8IW3P1_ISP_LINE_INT_NUM_REG_t *sun8iw6p1_isp_line_int_num;
SUN8IW3P1_ISP_ROT_OF_CFG_REG_t *sun8iw6p1_isp_rot_of_cfg;
ISP_REG_LOAD_ADDR_REG_t *sun8iw6p1_isp_reg_load_addr;
ISP_REG_SAVED_ADDR_REG_t *sun8iw6p1_isp_reg_saved_addr;
ISP_LUT_LENS_GAMMA_ADDR_REG_t *sun8iw6p1_isp_lut_lens_gamma_addr;
ISP_DRC_ADDR_REG_t *sun8iw6p1_isp_drc_addr;
ISP_STATISTICS_ADDR_REG_t *sun8iw6p1_isp_statistics_addr;
ISP_SRAM_RW_OFFSET_REG_t *sun8iw6p1_isp_sram_rw_offset;
ISP_SRAM_RW_DATA_REG_t *sun8iw6p1_isp_sram_rw_data;

/*
 * Register Address
 */
void sun8iw6p1_map_reg_addr(unsigned long isp_reg_base)
{
	sun8iw6p1_isp_fe_cfg =
		(ISP_FE_CFG_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_FE_CFG_REG_OFF);
	sun8iw6p1_isp_fe_ctrl =
		(ISP_FE_CTRL_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_FE_CTRL_REG_OFF);
	sun8iw6p1_isp_fe_int_en =
		(ISP_FE_INT_EN_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_FE_INT_EN_REG_OFF);
	sun8iw6p1_isp_fe_int_sta =
		(ISP_FE_INT_STA_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_FE_INT_STA_REG_OFF);
	sun8iw6p1_isp_line_int_num =
		(SUN8IW3P1_ISP_LINE_INT_NUM_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_LINE_INT_NUM_REG_OFF);
	sun8iw6p1_isp_rot_of_cfg =
		(SUN8IW3P1_ISP_ROT_OF_CFG_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_ROT_OF_CFG_REG_OFF);
	sun8iw6p1_isp_reg_load_addr =
		(ISP_REG_LOAD_ADDR_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_REG_LOAD_ADDR_REG_OFF);
	sun8iw6p1_isp_reg_saved_addr =
		(ISP_REG_SAVED_ADDR_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_REG_SAVED_ADDR_REG_OFF);
	sun8iw6p1_isp_lut_lens_gamma_addr =
		(ISP_LUT_LENS_GAMMA_ADDR_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_LUT_LENS_GAMMA_ADDR_REG_OFF);
	sun8iw6p1_isp_drc_addr =
		(ISP_DRC_ADDR_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_DRC_ADDR_REG_OFF);
	sun8iw6p1_isp_statistics_addr =
		(ISP_STATISTICS_ADDR_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_STATISTICS_ADDR_REG_OFF);
	sun8iw6p1_isp_sram_rw_offset =
		(ISP_SRAM_RW_OFFSET_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_SRAM_RW_OFFSET_REG_OFF);
	sun8iw6p1_isp_sram_rw_data =
		(ISP_SRAM_RW_DATA_REG_t *) (isp_reg_base + SUN8IW6P1_ISP_SRAM_RW_DATA_REG_OFF);
}

/*
 * Load DRAM Register Address
 */
void sun8iw6p1_map_load_dram_addr(unsigned long isp_load_dram_base)
{

}

/*
 * Saved DRAM Register Address
 */
void sun8iw6p1_map_saved_dram_addr(unsigned long isp_saved_dram_base)
{
}

void sun8iw6p1_isp_enable(void)
{
	sun8iw6p1_isp_fe_cfg->bits.isp_enable = 1;
}

void sun8iw6p1_isp_disable(void)
{
	sun8iw6p1_isp_fe_cfg->bits.isp_enable = 0;
}

void sun8iw6p1_isp_set_interface(enum isp_src_interface iface,
				enum isp_src input_src)
{
	switch (input_src) {
	case ISP_SRC0:
		sun8iw6p1_isp_fe_cfg->bits.src0_mode = iface;
		break;
	case ISP_SRC1:
		sun8iw6p1_isp_fe_cfg->bits.src1_mode = iface;
		break;
	default:
		break;
	}
}

void sun8iw6p1_isp_set_para_ready(enum ready_flag ready)
{
	if (ready == PARA_READY)
		sun8iw6p1_isp_fe_ctrl->bits.para_ready = 1;
	else
		sun8iw6p1_isp_fe_ctrl->bits.para_ready = 0;
}

unsigned int sun8iw6p1_isp_get_para_ready(void)
{
	return sun8iw6p1_isp_fe_ctrl->bits.para_ready;
}

void sun8iw6p1_isp_update_table(unsigned short table_update)
{
	if (table_update & LUT_UPDATE)
		sun8iw6p1_isp_fe_ctrl->bits.lut_update = 1;
	else
		sun8iw6p1_isp_fe_ctrl->bits.lut_update = 0;
	if (table_update & LENS_UPDATE)
		sun8iw6p1_isp_fe_ctrl->bits.lens_update = 1;
	else
		sun8iw6p1_isp_fe_ctrl->bits.lens_update = 0;
	if (table_update & GAMMA_UPDATE)
		sun8iw6p1_isp_fe_ctrl->bits.gamma_update = 1;
	else
		sun8iw6p1_isp_fe_ctrl->bits.gamma_update = 0;
	if (table_update & DRC_UPDATE)
		sun8iw6p1_isp_fe_ctrl->bits.drc_update = 1;
	else
		sun8iw6p1_isp_fe_ctrl->bits.drc_update = 0;
}

void sun8iw6p1_isp_set_output_speed(enum isp_output_speed speed)
{
	sun8iw6p1_isp_fe_ctrl->bits.isp_output_speed_ctrl = speed;
}

void sun8iw6p1_isp_capture_start(enum isp_capture_mode mode)
{
	switch (mode) {
	case VCAP_EN:
		if (sun8iw6p1_isp_fe_ctrl->bits.scap_en)
			sun8iw6p1_isp_fe_ctrl->bits.scap_en = 0;
		sun8iw6p1_isp_fe_ctrl->bits.vcap_en = 1;
		break;
	case SCAP_EN:
		if (sun8iw6p1_isp_fe_ctrl->bits.vcap_en)
			sun8iw6p1_isp_fe_ctrl->bits.vcap_en = 0;
		sun8iw6p1_isp_fe_ctrl->bits.scap_en = 1;
		break;
	default:
		break;
	}
}

void sun8iw6p1_isp_capture_stop(enum isp_capture_mode mode)
{
	switch (mode) {
	case VCAP_EN:
		sun8iw6p1_isp_fe_ctrl->bits.vcap_en = 0;
		break;
	case SCAP_EN:
		sun8iw6p1_isp_fe_ctrl->bits.scap_en = 0;
		break;
	default:
		break;
	}
}

void sun8iw6p1_isp_irq_enable(unsigned int irq_flag)
{
	sun8iw6p1_isp_fe_int_en->dwval |= irq_flag;
}

void sun8iw6p1_isp_irq_disable(unsigned int irq_flag)
{
	sun8iw6p1_isp_fe_int_en->dwval &= ~irq_flag;
}

int sun8iw6p1_isp_int_get_enable(void)
{
	return sun8iw6p1_isp_fe_int_en->dwval & 0x7f;
}

unsigned int sun8iw6p1_isp_get_irq_status(unsigned int irq_flag)
{
	return sun8iw6p1_isp_fe_int_sta->dwval & irq_flag;
}

void sun8iw6p1_isp_clr_irq_status(unsigned int irq_flag)
{
	sun8iw6p1_isp_fe_int_sta->dwval |= irq_flag;
}

void sun8iw6p1_isp_set_line_int_num(unsigned int line_num)
{
	sun8iw6p1_isp_line_int_num->bits.line_int_num = line_num;
}

void sun8iw6p1_isp_set_rot_of_line_num(unsigned int line_num)
{
	sun8iw6p1_isp_rot_of_cfg->bits.rot_of_line_num = line_num;
}

void sun8iw6p1_isp_set_load_addr(unsigned long addr)
{
	sun8iw6p1_isp_reg_load_addr->dwval = addr >> ISP_ADDR_BIT_R_SHIFT;
}

void sun8iw6p1_isp_set_saved_addr(unsigned long addr)
{
	sun8iw6p1_isp_reg_saved_addr->dwval = addr >> ISP_ADDR_BIT_R_SHIFT;
}

void sun8iw6p1_isp_set_table_addr(enum isp_input_tables table,
					unsigned long addr)
{
	switch (table) {
	case LUT_LENS_GAMMA_TABLE:
		sun8iw6p1_isp_lut_lens_gamma_addr->dwval = addr >> ISP_ADDR_BIT_R_SHIFT;
		break;
	case DRC_TABLE:
		sun8iw6p1_isp_drc_addr->dwval = addr >> ISP_ADDR_BIT_R_SHIFT;
		break;
	default:
		break;
	}
}

void sun8iw6p1_isp_set_statistics_addr(unsigned long addr)
{
	sun8iw6p1_isp_statistics_addr->dwval = addr >> ISP_ADDR_BIT_R_SHIFT;
}

static struct isp_bsp_fun_array sun8iw6p1_fun_array = {
	.map_reg_addr = sun8iw6p1_map_reg_addr,
	.map_load_dram_addr = sun8iw6p1_map_load_dram_addr,
	.map_saved_dram_addr = sun8iw6p1_map_saved_dram_addr,
	.isp_set_interface = sun8iw6p1_isp_set_interface,
	.isp_enable = sun8iw6p1_isp_enable,
	.isp_disable = sun8iw6p1_isp_disable,
	.isp_set_para_ready = sun8iw6p1_isp_set_para_ready,
	.isp_get_para_ready = sun8iw6p1_isp_get_para_ready,
	.isp_capture_start = sun8iw6p1_isp_capture_start,
	.isp_capture_stop = sun8iw6p1_isp_capture_stop,
	.isp_irq_enable = sun8iw6p1_isp_irq_enable,
	.isp_irq_disable = sun8iw6p1_isp_irq_disable,
	.isp_get_irq_status = sun8iw6p1_isp_get_irq_status,
	.isp_clr_irq_status = sun8iw6p1_isp_clr_irq_status,
	.isp_int_get_enable = sun8iw6p1_isp_int_get_enable,
	.isp_set_line_int_num =	sun8iw6p1_isp_set_line_int_num,
	.isp_set_rot_of_line_num = sun8iw6p1_isp_set_rot_of_line_num,
	.isp_set_load_addr = sun8iw6p1_isp_set_load_addr,
	.isp_set_saved_addr = sun8iw6p1_isp_set_saved_addr,
	.isp_set_table_addr = sun8iw6p1_isp_set_table_addr,
	.isp_set_statistics_addr = sun8iw6p1_isp_set_statistics_addr,
};

struct isp_feature_array sun8iw6p1_feature_array = {
	.isp_update_table = sun8iw6p1_isp_update_table,
	.isp_set_output_speed = sun8iw6p1_isp_set_output_speed,
	.isp_set_para_ready = sun8iw6p1_isp_set_para_ready,
	.isp_set_disc = NULL,
};


struct isp_platform_drv sun8iw6p1_isp_drv = {
	.platform_id = ISP_PLATFORM_SUN8IW6P1,
	.fun_array = &sun8iw6p1_fun_array,
	.feature_array = &sun8iw6p1_feature_array,
};

int sun8iw6p1_isp_platform_register(void)
{
	return isp_platform_register(&sun8iw6p1_isp_drv);
}
