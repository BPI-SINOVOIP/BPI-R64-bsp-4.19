// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "disp_dts_gpio.h"
#include "disp_helper.h"
#include "disp_drv_log.h"
#include <linux/kernel.h> /* printk */
#include <linux/pinctrl/consumer.h>

static struct pinctrl *this_pctrl; /* static pinctrl instance */

/* DTS state mapping name */
static const char *this_state_name[DTS_GPIO_STATE_MAX] = {
	"lcd_bias_enp1_gpio",
	"lcd_bias_enp0_gpio",
	"lcd_bias_enn1_gpio",
	"lcd_bias_enn0_gpio",
	"lcm_rst_out1_gpio",
	"lcm_rst_out0_gpio",
	"lcm_dsi_te",
	"lcm_mipi0_sdata",
	"lcm_mipi0_sclk",
	"lcm_mipi1_sdata",
	"lcm_mipi1_sclk",
	"lcm_mipi2_sdata",
	"lcm_mipi2_sclk",
	"lcm_mipi3_sdata",
	"lcm_mipi3_sclk",
	"lcm_mipi4_sdata",
	"lcm_mipi4_sclk",
};

/* pinctrl implementation */
static long _set_state(const char *name)
{
	long ret = 0;
	struct pinctrl_state *pState = 0;

	if (!this_pctrl) {
		pr_err("this pctrl is null\n");
		return -1;
	}

	pState = pinctrl_lookup_state(this_pctrl, name);
	if (IS_ERR(pState)) {
		pr_err("lookup state '%s' failed\n", name);
		ret = PTR_ERR(pState);
		goto exit;
	}

	/* select state! */
	pinctrl_select_state(this_pctrl, pState);

exit:
	return ret; /* Good! */
}

long disp_dts_gpio_init(struct platform_device *pdev)
{
	long ret = 0;
	struct pinctrl *pctrl;

	/* retrieve */
	pctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pctrl)) {
		dev_err(&pdev->dev, "Cannot find disp pinctrl!");
		ret = PTR_ERR(pctrl);
		goto exit;
	}

	this_pctrl = pctrl;

	/* setup DSI pin by default */
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_DSI_TE);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_MIPI0_SDATA);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_MIPI0_SCLK);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_MIPI1_SDATA);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_MIPI1_SCLK);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_MIPI2_SDATA);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_MIPI2_SCLK);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_MIPI3_SDATA);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_MIPI3_SCLK);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_MIPI4_SDATA);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_MIPI4_SCLK);

exit:
	return ret;
}

long disp_dts_gpio_select_state(enum DTS_GPIO_STATE s)
{
	if (!((unsigned int)(s) < (unsigned int)(DTS_GPIO_STATE_MAX))) {
		pr_info("GPIO STATE is invalid,state=%d\n", (unsigned int)s);
		return -1;
	}
	return _set_state(this_state_name[s]);
}
