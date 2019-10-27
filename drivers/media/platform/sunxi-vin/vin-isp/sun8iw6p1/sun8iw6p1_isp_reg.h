
/*
 ******************************************************************************
 *
 * sun8iw6p1_isp_reg.h
 *
 * Hawkview ISP - sun8iw6p1_isp_reg.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng   	2014/06/19	      Second Version
 *
 ******************************************************************************
 */

#ifndef __REG20__ISP__H__
#define __REG20__ISP__H__

#ifdef __cplusplus
extern "C" {

#endif

#define ISP_FE_CFG_REG_OFF                  0x000
#define ISP_FE_CTRL_REG_OFF                 0x004
#define ISP_FE_INT_EN_REG_OFF               0x008
#define ISP_FE_INT_STA_REG_OFF              0x00c
#define ISP_REG_LOAD_ADDR_REG_OFF           0x020
#define ISP_REG_SAVED_ADDR_REG_OFF          0x024
#define ISP_LUT_LENS_GAMMA_ADDR_REG_OFF     0x028
#define ISP_DRC_ADDR_REG_OFF                0x02c
#define ISP_STATISTICS_ADDR_REG_OFF         0x030
#define ISP_SRAM_RW_OFFSET_REG_OFF          0x038
#define ISP_SRAM_RW_DATA_REG_OFF            0x03c

/*FOR SUN8IW3P1 ISP*/
#define SUN8IW3P1_ISP_BASE_ADDRESS                    0X03808000

#define SUN8IW3P1_ISP_FE_CFG_REG_OFF                  ISP_FE_CFG_REG_OFF
#define SUN8IW3P1_ISP_FE_CTRL_REG_OFF                 ISP_FE_CTRL_REG_OFF
#define SUN8IW3P1_ISP_FE_INT_EN_REG_OFF               ISP_FE_INT_EN_REG_OFF
#define SUN8IW3P1_ISP_FE_INT_STA_REG_OFF              ISP_FE_INT_STA_REG_OFF

#define SUN8IW3P1_ISP_LINE_INT_NUM_REG_OFF            0x018
#define SUN8IW3P1_ISP_ROT_OF_CFG_REG_OFF              0x01c

#define SUN8IW3P1_ISP_REG_LOAD_ADDR_REG_OFF           ISP_REG_LOAD_ADDR_REG_OFF
#define SUN8IW3P1_ISP_REG_SAVED_ADDR_REG_OFF          ISP_REG_SAVED_ADDR_REG_OFF
#define SUN8IW3P1_ISP_SRAM_RW_OFFSET_REG_OFF          ISP_SRAM_RW_OFFSET_REG_OFF
#define SUN8IW3P1_ISP_SRAM_RW_DATA_REG_OFF            ISP_SRAM_RW_DATA_REG_OFF

/*FOR SUN9IW1P1 ISP*/
#define SUN9IW1P1_ISP_FE_CFG_REG_OFF                  ISP_FE_CFG_REG_OFF
#define SUN9IW1P1_ISP_FE_CTRL_REG_OFF                 ISP_FE_CTRL_REG_OFF
#define SUN9IW1P1_ISP_FE_INT_EN_REG_OFF               ISP_FE_INT_EN_REG_OFF
#define SUN9IW1P1_ISP_FE_INT_STA_REG_OFF              ISP_FE_INT_STA_REG_OFF

#define SUN9IW1P1_ISP_LINE_INT_NUM_REG_OFF            SUN8IW3P1_ISP_LINE_INT_NUM_REG_OFF
#define SUN9IW1P1_ISP_ROT_OF_CFG_REG_OFF              SUN8IW3P1_ISP_ROT_OF_CFG_REG_OFF

#define SUN9IW1P1_ISP_REG_LOAD_ADDR_REG_OFF           ISP_REG_LOAD_ADDR_REG_OFF
#define SUN9IW1P1_ISP_REG_SAVED_ADDR_REG_OFF          ISP_REG_SAVED_ADDR_REG_OFF
#define SUN9IW1P1_ISP_LUT_LENS_GAMMA_ADDR_REG_OFF     ISP_LUT_LENS_GAMMA_ADDR_REG_OFF
#define SUN9IW1P1_ISP_DRC_ADDR_REG_OFF                ISP_DRC_ADDR_REG_OFF
#define SUN9IW1P1_ISP_STATISTICS_ADDR_REG_OFF         ISP_STATISTICS_ADDR_REG_OFF
#define SUN9IW1P1_ISP_SRAM_RW_OFFSET_REG_OFF          ISP_SRAM_RW_OFFSET_REG_OFF
#define SUN9IW1P1_ISP_SRAM_RW_DATA_REG_OFF            ISP_SRAM_RW_DATA_REG_OFF

/*FOR SUN8IW6P1 ISP*/
#define SUN8IW6P1_ISP_FE_CFG_REG_OFF                  ISP_FE_CFG_REG_OFF
#define SUN8IW6P1_ISP_FE_CTRL_REG_OFF                 ISP_FE_CTRL_REG_OFF
#define SUN8IW6P1_ISP_FE_INT_EN_REG_OFF               ISP_FE_INT_EN_REG_OFF
#define SUN8IW6P1_ISP_FE_INT_STA_REG_OFF              ISP_FE_INT_STA_REG_OFF

#define SUN8IW6P1_ISP_LINE_INT_NUM_REG_OFF            SUN8IW3P1_ISP_LINE_INT_NUM_REG_OFF
#define SUN8IW6P1_ISP_ROT_OF_CFG_REG_OFF              SUN8IW3P1_ISP_ROT_OF_CFG_REG_OFF

#define SUN8IW6P1_ISP_REG_LOAD_ADDR_REG_OFF           ISP_REG_LOAD_ADDR_REG_OFF
#define SUN8IW6P1_ISP_REG_SAVED_ADDR_REG_OFF          ISP_REG_SAVED_ADDR_REG_OFF
#define SUN8IW6P1_ISP_LUT_LENS_GAMMA_ADDR_REG_OFF     ISP_LUT_LENS_GAMMA_ADDR_REG_OFF
#define SUN8IW6P1_ISP_DRC_ADDR_REG_OFF                ISP_DRC_ADDR_REG_OFF
#define SUN8IW6P1_ISP_STATISTICS_ADDR_REG_OFF         ISP_STATISTICS_ADDR_REG_OFF
#define SUN8IW6P1_ISP_SRAM_RW_OFFSET_REG_OFF          ISP_SRAM_RW_OFFSET_REG_OFF
#define SUN8IW6P1_ISP_SRAM_RW_DATA_REG_OFF            ISP_SRAM_RW_DATA_REG_OFF

typedef union {
	unsigned int dwval;
	struct {
		unsigned int isp_enable:1;
		unsigned int res0:7;
		unsigned int src0_mode:2;
		unsigned int res1:6;
		unsigned int src1_mode:2;
		unsigned int res2:14;
	} bits;
} ISP_FE_CFG_REG_t;

typedef union {
unsigned int dwval;
	struct {
		unsigned int scap_en:1;
		unsigned int vcap_en:1;
		unsigned int para_ready:1;
		unsigned int lut_update:1;
		unsigned int lens_update:1;
		unsigned int gamma_update:1;
		unsigned int drc_update:1;
		unsigned int res0:9;
		unsigned int isp_output_speed_ctrl:2;
		unsigned int res1:13;
		unsigned int vcap_read_start:1;
	} bits;
} ISP_FE_CTRL_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int finish_int_en:1;
		unsigned int start_int_en:1;
		unsigned int para_save_int_en:1;
		unsigned int para_load_int_en:1;
		unsigned int src0_fifo_int_en:1;
		unsigned int src1_fifo_int_en:1;
		unsigned int rot_finish_int_en:1;
		unsigned int n_line_start_int_en:1;
		unsigned int res0:24;
	} bits;
} ISP_FE_INT_EN_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int finish_pd:1;
		unsigned int start_pd:1;
		unsigned int para_saved_pd:1;
		unsigned int para_load_pd:1;
		unsigned int src0_fifo_of_pd:1;
		unsigned int src1_fifo_of_pd:1;
		unsigned int rot_finish_pd:1;
		unsigned int n_line_start_pd:1;
		unsigned int res0:24;
	} bits;
} ISP_FE_INT_STA_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int line_int_num:14;
		unsigned int res0:18;
	} bits;
} SUN8IW3P1_ISP_LINE_INT_NUM_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int rot_of_line_num:14;
		unsigned int res0:18;
	} bits;
} SUN8IW3P1_ISP_ROT_OF_CFG_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int reg_load_addr;
	} bits;
} ISP_REG_LOAD_ADDR_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int reg_saved_addr;
	} bits;
} ISP_REG_SAVED_ADDR_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lut_lens_gamma_addr;
	} bits;
} ISP_LUT_LENS_GAMMA_ADDR_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int rgb_yuv_drc_addr;
	} bits;
} ISP_DRC_ADDR_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int statistics_addr;
	} bits;
} ISP_STATISTICS_ADDR_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int sram_addr:17;
		unsigned int res0:15;
	} bits;
} ISP_SRAM_RW_OFFSET_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int sram_data;
	} bits;
} ISP_SRAM_RW_DATA_REG_t;

#ifdef __cplusplus
}
#endif

#endif
