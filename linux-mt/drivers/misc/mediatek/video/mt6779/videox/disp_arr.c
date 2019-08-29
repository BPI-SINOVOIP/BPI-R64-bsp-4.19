// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
/* #include <linux/switch.h> */

#include "disp_drv_platform.h"
#include "debug.h"
#include "disp_drv_log.h"
#include "disp_lcm.h"
#include "disp_utils.h"
#include "mtkfb.h"
#include "disp_session.h"
#include "ddp_manager.h"
#include "mtkfb_fence.h"
#include "display_recorder.h"
#include "fbconfig_kdebug.h"
#include "disp_session.h"
#include "disp_helper.h"
#include "mtk_disp_mgr.h"
#include "mtkfb_console.h"
#include "disp_lowpower.h"
#include "disp_recovery.h"
#include "layering_rule.h"
#include "disp_rect.h"
#include "disp_partial.h"
#include "disp_arr.h"
#include "primary_display.h"


/* used by ARR2.0 */
int primary_display_get_cur_refresh_rate(void)
{
	return primary_display_force_get_vsync_fps();
}

int primary_display_get_max_refresh_rate(void)
{
	int fps = 60;
	struct LCM_PARAMS *lcm_params = NULL;

	lcm_params = disp_lcm_get_params(primary_get_lcm());
	/* _primary_path_lock(__func__); */
	if (lcm_params->max_refresh_rate)
		fps = lcm_params->max_refresh_rate;
	/* _primary_path_unlock(__func__); */

	return fps;
}

int primary_display_get_min_refresh_rate(void)
{
	int ret = 60;
	struct LCM_PARAMS *lcm_params = NULL;

	lcm_params = disp_lcm_get_params(primary_get_lcm());
	/* _primary_path_lock(__func__); */
	if (lcm_params->min_refresh_rate)
		ret = lcm_params->min_refresh_rate;
	/* _primary_path_unlock(__func__); */

	return ret;
}

int primary_display_set_refresh_rate(unsigned int refresh_rate)
{
	int ret = -1;
	int temp_refresh_rate_min = 0;
	int temp_refresh_rate_max = 0;

	temp_refresh_rate_min = primary_display_get_min_refresh_rate();
	temp_refresh_rate_max = primary_display_get_max_refresh_rate();

	if ((refresh_rate > temp_refresh_rate_max) ||
	    (refresh_rate < temp_refresh_rate_min))
		return ret;

	/* AP set refresh rate */
	ret = primary_display_force_set_vsync_fps(refresh_rate, 0);
	return ret;
}
