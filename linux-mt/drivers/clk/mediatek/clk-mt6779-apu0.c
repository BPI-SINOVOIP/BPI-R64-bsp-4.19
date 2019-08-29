// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Wendell Lin <wendell.lin@mediatek.com>
 */

#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <dt-bindings/clock/mt6779-clk.h>

#include "clk-mtk.h"
#include "clk-gate.h"

static const struct mtk_gate_regs apu0_cg_regs = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_APU0(_id, _name, _parent, _shift) {		\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &apu0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate apu0_clks[] = {
	GATE_APU0(CLK_APU0_APU, "apu0_apu", "dsp1_sel", 0),
	GATE_APU0(CLK_APU0_AXI_M, "apu0_axi", "dsp1_sel", 1),
	GATE_APU0(CLK_APU0_JTAG, "apu0_jtag", "dsp1_sel", 2),
};

static const struct of_device_id of_match_clk_mt6779_apu0[] = {
	{ .compatible = "mediatek,mt6779-apu0", },
	{}
};

static int clk_mt6779_apu0_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_APU0_NR_CLK);

	mtk_clk_register_gates(node, apu0_clks, ARRAY_SIZE(apu0_clks),
			       clk_data);

	return of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
}

static struct platform_driver clk_mt6779_apu0_drv = {
	.probe = clk_mt6779_apu0_probe,
	.driver = {
		.name = "clk-mt6779-apu0",
		.of_match_table = of_match_clk_mt6779_apu0,
	},
};

builtin_platform_driver(clk_mt6779_apu0_drv);
