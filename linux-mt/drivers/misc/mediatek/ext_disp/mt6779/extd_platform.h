/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Joey Pan <joey.pan@mediatek.com>
 */

#ifndef __EXTD_PLATFORM_H__
#define __EXTD_PLATFORM_H__

#include "ddp_hal.h"


/* #define MTK_LCD_HW_3D_SUPPORT */
#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))


#define EXTD_SHADOW_REGISTER_SUPPORT

/* #define EXTD_SMART_OVL_SUPPORT */
/* #define EXTD_DEBUG_SUPPORT */

/* /#define EXTD_DBG_USE_INNER_BUF */

/* #define EXTD_DUAL_PIPE_SWITCH_SUPPORT */

#ifdef EXTD_SMART_OVL_SUPPORT
#  define EXTD_OVERLAY_CNT  (6+6)
#else
#  define EXTD_OVERLAY_CNT  6
#endif

#define HW_DPI_VSYNC_SUPPORT 1

#define DISP_MODULE_RDMA DISP_MODULE_RDMA1

/* #define MM_MHL_DVFS */
#define MHL_DYNAMIC_VSYNC_OFFSET

#define MTK_AUDIO_MULTI_CHANNEL_SUPPORT
#define FIX_EXTD_TO_OVL_PATH EXTD_OVERLAY_CNT

#define CONFIG_IO_DRIVING

/* #define DP_EINT_GPIO_NUMBER 98 */

#endif
