/*
 ******************************************************************************
 *
 * csi_reg.c
 *
 * Hawkview ISP - csi_reg.c module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    		Description
 *
 *   2.0		  Yang Feng   	2014/07/15	      Second Version
 *
 ******************************************************************************
 */

#include <linux/kernel.h>
#include "csi_reg_i.h"
#include "csi_reg.h"

#include "../utility/vin_io.h"

#if defined CONFIG_ARCH_SUN8IW1P1
#define ADDR_BIT_R_SHIFT 0
#define CLK_POL 0
#elif defined CONFIG_ARCH_SUN8IW3P1
#define ADDR_BIT_R_SHIFT 0
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN9IW1P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW5P1
#define ADDR_BIT_R_SHIFT 0
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW6P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW7P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW8P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW9P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN50I
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW10P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#endif

volatile void __iomem *csi_base_addr[2];

int csi_set_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > MAX_CSI - 1)
		return -1;
	csi_base_addr[sel] = (volatile void __iomem *)addr;

	return 0;
}

/* open module */
void csi_enable(unsigned int sel)
{
	vin_reg_set(csi_base_addr[sel] + CSI_EN_REG_OFF,
		    1 << CSI_EN_REG_CSI_EN);
}

void csi_disable(unsigned int sel)
{
	vin_reg_clr(csi_base_addr[sel] + CSI_EN_REG_OFF,
		    1 << CSI_EN_REG_CSI_EN);
}

/* configure */
void csi_if_cfg(unsigned int sel, struct csi_if_cfg *csi_if_cfg)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_SRC_TYPE_MASK,
			csi_if_cfg->src_type << CSI_IF_CFG_REG_SRC_TYPE);

	if (csi_if_cfg->interface < 0x80)
		vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
				CSI_IF_CFG_REG_CSI_IF_MASK,
				csi_if_cfg->interface << CSI_IF_CFG_REG_CSI_IF);
	else
		vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
				CSI_IF_CFG_REG_MIPI_IF_MASK,
				1 << CSI_IF_CFG_REG_MIPI_IF);

	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_IF_DATA_WIDTH_MASK,
			csi_if_cfg->data_width << CSI_IF_CFG_REG_IF_DATA_WIDTH);
}

void csi_timing_cfg(unsigned int sel, struct csi_timing_cfg *csi_tmg_cfg)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_VREF_POL_MASK,
			csi_tmg_cfg->vref << CSI_IF_CFG_REG_VREF_POL);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_HREF_POL_MASK,
			csi_tmg_cfg->href << CSI_IF_CFG_REG_HREF_POL);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_CLK_POL_MASK,
			((csi_tmg_cfg->sample ==
			  CLK_POL) ? 1 : 0) << CSI_IF_CFG_REG_CLK_POL);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_FIELD_MASK,
			csi_tmg_cfg->field << CSI_IF_CFG_REG_FIELD);
}

void csi_fmt_cfg(unsigned int sel, unsigned int ch,
		 struct csi_fmt_cfg *csi_fmt_cfg)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_CFG_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_CFG_REG_INPUT_FMT_MASK,
			csi_fmt_cfg->input_fmt << CSI_CH_CFG_REG_INPUT_FMT);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_CFG_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_CFG_REG_OUTPUT_FMT_MASK,
			csi_fmt_cfg->output_fmt << CSI_CH_CFG_REG_OUTPUT_FMT);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_CFG_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_CFG_REG_FIELD_SEL_MASK,
			csi_fmt_cfg->field_sel << CSI_CH_CFG_REG_FIELD_SEL);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_CFG_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_CFG_REG_INPUT_SEQ_MASK,
			csi_fmt_cfg->input_seq << CSI_CH_CFG_REG_INPUT_SEQ);
}

/* buffer */
void csi_set_buffer_address(unsigned int sel, unsigned int ch,
			    enum csi_buf_sel buf, u64 addr)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_F0_BUFA_REG_OFF +
			ch * CSI_CH_OFF + (buf << 2), 0xffffffff,
			addr >> ADDR_BIT_R_SHIFT);
}

u64 csi_get_buffer_address(unsigned int sel, unsigned int ch,
			   enum csi_buf_sel buf)
{
	unsigned int reg_val =
	    vin_reg_readl(csi_base_addr[sel] + CSI_CH_F0_BUFA_REG_OFF +
			  ch * CSI_CH_OFF + (buf << 2));
	return reg_val << ADDR_BIT_R_SHIFT;
}

/* capture */
void csi_capture_start(unsigned int sel, unsigned int ch_total_num,
		       enum csi_cap_mode csi_cap_mode)
{
	u32 reg_val = (((ch_total_num == 4) ? csi_cap_mode : 0) << 24) +
	    (((ch_total_num == 3) ? csi_cap_mode : 0) << 16) +
	    (((ch_total_num == 2) ? csi_cap_mode : 0) << 8) +
	    (((ch_total_num == 1) ? csi_cap_mode : 0));
	vin_reg_writel(csi_base_addr[sel] + CSI_CAP_REG_OFF, reg_val);
}

void csi_capture_stop(unsigned int sel, unsigned int ch_total_num,
		      enum csi_cap_mode csi_cap_mode)
{
	vin_reg_writel(csi_base_addr[sel] + CSI_CAP_REG_OFF, 0);
}

void csi_capture_get_status(unsigned int sel, unsigned int ch,
			    struct csi_capture_status *status)
{
	unsigned int reg_val =
	    vin_reg_readl(csi_base_addr[sel] + CSI_CH_STA_REG_OFF +
			  ch * CSI_CH_OFF);
	status->picture_in_progress =
	    (reg_val >> CSI_CH_STA_REG_SCAP_STA) & 0x1;
	status->video_in_progress = (reg_val >> CSI_CH_STA_REG_VCAP_STA) & 0x1;
}

/* size */
void csi_set_size(unsigned int sel, unsigned int ch, unsigned int length_h,
		  unsigned int length_v, unsigned int buf_length_y,
		  unsigned int buf_length_c)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_HSIZE_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_HSIZE_REG_HOR_LEN_MASK,
			length_h << CSI_CH_HSIZE_REG_HOR_LEN);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_VSIZE_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_VSIZE_REG_VER_LEN_MASK,
			length_v << CSI_CH_VSIZE_REG_VER_LEN);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_BUF_LEN_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_BUF_LEN_REG_BUF_LEN_MASK,
			buf_length_y << CSI_CH_BUF_LEN_REG_BUF_LEN);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_BUF_LEN_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_BUF_LEN_REG_BUF_LEN_C_MASK,
			buf_length_c << CSI_CH_BUF_LEN_REG_BUF_LEN_C);
}

/* offset */
void csi_set_offset(unsigned int sel, unsigned int ch, unsigned int start_h,
		    unsigned int start_v)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_HSIZE_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_HSIZE_REG_HOR_START_MASK,
			start_h << CSI_CH_HSIZE_REG_HOR_START);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_VSIZE_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_VSIZE_REG_VER_START_MASK,
			start_v << CSI_CH_VSIZE_REG_VER_START);
}

/* interrupt */
void csi_int_enable(unsigned int sel, unsigned int ch,
		    enum csi_int_sel interrupt)
{
	vin_reg_set(csi_base_addr[sel] + CSI_CH_INT_EN_REG_OFF +
		    ch * CSI_CH_OFF, interrupt);
}

void csi_int_disable(unsigned int sel, unsigned int ch,
		     enum csi_int_sel interrupt)
{
	vin_reg_clr(csi_base_addr[sel] + CSI_CH_INT_EN_REG_OFF +
		    ch * CSI_CH_OFF, interrupt);
}

void csi_int_get_status(unsigned int sel, unsigned int ch,
			struct csi_int_status *status)
{
	unsigned int reg_val =
	    vin_reg_readl(csi_base_addr[sel] + CSI_CH_INT_STA_REG_OFF +
			  ch * CSI_CH_OFF);
	status->capture_done = (reg_val >> CSI_CH_INT_STA_REG_CD_PD) & 0x1;
	status->frame_done = (reg_val >> CSI_CH_INT_STA_REG_FD_PD) & 0x1;
	status->buf_0_overflow =
	    (reg_val >> CSI_CH_INT_STA_REG_FIFO0_OF_PD) & 0x1;
	status->buf_1_overflow =
	    (reg_val >> CSI_CH_INT_STA_REG_FIFO1_OF_PD) & 0x1;
	status->buf_2_overflow =
	    (reg_val >> CSI_CH_INT_STA_REG_FIFO2_OF_PD) & 0x1;
	status->protection_error =
	    (reg_val >> CSI_CH_INT_STA_REG_PRTC_ERR_PD) & 0x1;
	status->hblank_overflow =
	    (reg_val >> CSI_CH_INT_STA_REG_HB_OF_PD) & 0x1;
	status->vsync_trig = (reg_val >> CSI_CH_INT_STA_REG_VS_PD) & 0x1;
}

void csi_int_clear_status(unsigned int sel, unsigned int ch,
			  enum csi_int_sel interrupt)
{
	vin_reg_writel(csi_base_addr[sel] + CSI_CH_INT_STA_REG_OFF, interrupt);
}
