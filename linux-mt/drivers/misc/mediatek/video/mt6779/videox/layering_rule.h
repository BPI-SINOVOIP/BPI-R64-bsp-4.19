/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Joey Pan <joey.pan@mediatek.com>
 */

#ifndef __LAYER_STRATEGY_EX__
#define __LAYER_STRATEGY_EX__

#include "layering_rule_base.h"
#include "lcm_drv.h"

#define MAX_PHY_OVL_CNT (12)
/* #define HAS_LARB_HRT */
#define HRT_AEE_LAYER_MASK 0xFFFFFFDF

enum DISP_DEBUG_LEVEL {
	DISP_DEBUG_LEVEL_CRITICAL = 0,
	DISP_DEBUG_LEVEL_ERR,
	DISP_DEBUG_LEVEL_WARN,
	DISP_DEBUG_LEVEL_DEBUG,
	DISP_DEBUG_LEVEL_INFO,
};

enum HRT_LEVEL {
	HRT_LEVEL_LEVEL0 = 0,
	HRT_LEVEL_LEVEL1,
	HRT_LEVEL_LEVEL2,
	HRT_LEVEL_LEVEL3,
	HRT_LEVEL_NUM,
	HRT_LEVEL_DEFAULT = HRT_LEVEL_NUM + 1,
};

enum HRT_TB_TYPE {
	HRT_TB_TYPE_GENERAL = 0,
	HRT_TB_TYPE_RPO_L0,
	HRT_TB_TYPE_RPO_L0L1,
	HRT_TB_NUM,
};

enum HRT_BOUND_TYPE {
	HRT_BOUND_TYPE_LP4 = 0,
	HRT_BOUND_NUM,
};

enum HRT_PATH_SCENARIO {
	HRT_PATH_GENERAL = MAKE_UNIFIED_HRT_PATH_FMT(HRT_PATH_RSZ_NONE,
						HRT_PATH_PIPE_SINGLE,
						HRT_PATH_DISP_DUAL_EXT, 1),
	HRT_PATH_RPO_L0 = MAKE_UNIFIED_HRT_PATH_FMT(HRT_PATH_RSZ_PARTIAL,
						HRT_PATH_PIPE_SINGLE,
						HRT_PATH_DISP_SINGLE, 2),
	HRT_PATH_RPO_L0L1 = MAKE_UNIFIED_HRT_PATH_FMT(HRT_PATH_RSZ_PARTIAL,
						HRT_PATH_PIPE_SINGLE,
						HRT_PATH_DISP_SINGLE, 3),
	HRT_PATH_UNKNOWN = MAKE_UNIFIED_HRT_PATH_FMT(0, 0, 0, 4),
	HRT_PATH_NUM = MAKE_UNIFIED_HRT_PATH_FMT(0, 0, 0, 5),
};

void layering_rule_init(void);
void update_layering_opt_by_disp_opt(enum DISP_HELPER_OPT option, int value);
unsigned int layering_rule_get_hrt_idx(void);
unsigned long long layering_get_frame_bw(void);
int layering_get_valid_hrt(void);
void copy_hrt_bound_table(int is_larb, int *hrt_table);

#endif
