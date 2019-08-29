/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Joey Pan <joey.pan@mediatek.com>
 */

#ifndef _DDP_OVL_H_
#define _DDP_OVL_H_

#include "ddp_hal.h"
#include "ddp_info.h"

/*limit 18:9 */
#define SBCH_WIDTH      (1080)
#define SBCH_HEIGHT     (2160)

#define OVL_MAX_WIDTH  (4095)
#define OVL_MAX_HEIGHT (4095)

#define TOTAL_OVL_LAYER_NUM	(12) /* 4+3+2+3 */
#define OVL_NUM			(3)
#define SBCH_EN_NUM    (1)
#define OVL_MODULE_MAX_PHY_LAYER (4)
#define OVL_MODULE_MAX_EXT_LAYER (3)

#define PRIMARY_THREE_OVL_CASCADE

/* start overlay module */
int ovl_start(enum DISP_MODULE_ENUM module, void *handle);

/* stop overlay module */
int ovl_stop(enum DISP_MODULE_ENUM module, void *handle);

/* reset overlay module */
int ovl_reset(enum DISP_MODULE_ENUM module, void *handle);

/* set region of interest */
int ovl_roi(enum DISP_MODULE_ENUM module, unsigned int bgW, unsigned int bgH,
	    unsigned int bgColor, /* border color */ void *handle);

/* switch layer on/off */
int ovl_layer_switch(enum DISP_MODULE_ENUM module, unsigned int layer,
		     unsigned int en, void *handle);
/* get ovl input address */
void ovl_get_address(enum DISP_MODULE_ENUM module, unsigned long *addr);

int ovl_3d_config(enum DISP_MODULE_ENUM module, unsigned int layer_id,
		  unsigned int en_3d, unsigned int landscape,
		  unsigned int r_first, void *handle);

void ovl_dump_analysis(enum DISP_MODULE_ENUM module);
void ovl_dump_reg(enum DISP_MODULE_ENUM module);
unsigned long ovl_base_addr(enum DISP_MODULE_ENUM module);
unsigned int ovl_to_index(enum DISP_MODULE_ENUM module);

void ovl_get_info(enum DISP_MODULE_ENUM module, void *data);
unsigned int ddp_ovl_get_cur_addr(bool rdma_mode, int layerid);

unsigned int ovl_set_bg_color(unsigned int bg_color);
unsigned int ovl_set_dim_color(unsigned int dim_color);

void ovl_cal_golden_setting(enum dst_module_type dst_mod_type,
	unsigned int *gs);
unsigned int MMPathTracePrimaryOVL(char *str, unsigned int strlen,
	unsigned int n);
unsigned int MMPathTraceSecondOVL(char *str, unsigned int strlen,
	unsigned int n);
unsigned long ovl_layer_num(enum DISP_MODULE_ENUM module);
#endif /* _DDP_OVL_H_ */
