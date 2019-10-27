/**
 * Copyright (C) 2015-2016 Allwinner Technology Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Author: Albert Yu <yuxyun@allwinnertech.com>
 */

#ifndef _MALI_SUN5I_H_
#define _MALI_SUN5I_H_

#define MALI_PP_CORES_NUM 1
#define AW_MALI_GPU_RESOURCES_MALI400_MPX MALI_GPU_RESOURCES_MALI400_MP1

#define GPU_PBASE 0x01c40000
#define IRQ_GPU_GP  69
#define IRQ_GPU_PP0  71
#define IRQ_GPU_GPMMU 70
#define IRQ_GPU_PPMMU0 72

/*
#define GPU_PBASE           SUNXI_GPU_PBASE
#define IRQ_GPU_GP          SUNXI_IRQ_GPUGP
#define IRQ_GPU_GPMMU       SUNXI_IRQ_GPUGPMMU
#define IRQ_GPU_PP0         SUNXI_IRQ_GPUPP0
#define IRQ_GPU_PPMMU0      SUNXI_IRQ_GPUPPMMU0
*/

#define AW_GPU_PLL "ve_pll"
#define AW_MALI_CLK "mali"
#define AW_GPU_AHB "ahb_mali"

aw_private_data aw_private = {
	.tempctrl      = {
		.temp_ctrl_status = 0,
	},
	.pm            = {
		.regulator      = NULL,
		.regulator_id   = "vdd-gpu",
		.clk[0]         = {
			.clk_name   = "pll",
			.clk_id     = AW_GPU_PLL,
			.clk_handle = NULL,
			.parent_clk_num = -1,
		},
		.clk[1]         = {
			.clk_name   = "mali",
			.clk_id     = AW_MALI_CLK,
			.clk_handle = NULL,
			.parent_clk_num = 0,
		},
		.clk[2]         = {
			.clk_name   = "ahb",
			.clk_id     = AW_GPU_AHB,
			.clk_handle = NULL,
			.parent_clk_num = -1,
		},
		.vf_table[0]  = {
			.vol  = 1200,
			.freq = 320,
		},
		.dvfs_status       = 0,
		.begin_level       = 0,
		.max_level         = 0,
		.scene_ctrl_cmd    = 0,
		.scene_ctrl_status = 0,
		.independent_pow   = 0,
		.dvm               = 0,
	},
	.debug           = {
		.enable      = 0,
		.frequency   = 0,
		.voltage     = 0,
		.tempctrl    = 0,
		.scenectrl   = 0,
		.dvfs        = 0,
		.level       = 0,
	}
};

#endif /* _MALI_SUN5I_H_ */
