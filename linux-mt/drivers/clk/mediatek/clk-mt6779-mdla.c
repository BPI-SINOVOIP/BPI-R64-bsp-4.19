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

static const struct mtk_gate_regs apumdla_cg_regs = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_APU_MDLA(_id, _name, _parent, _shift) {		\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &apumdla_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate apumdla_clks[] = {
	GATE_APU_MDLA(CLK_APU_MDLA_B0, "mdla_b0", "dsp3_sel", 0),
	GATE_APU_MDLA(CLK_APU_MDLA_B1, "mdla_b1", "dsp3_sel", 1),
	GATE_APU_MDLA(CLK_APU_MDLA_B2, "mdla_b2", "dsp3_sel", 2),
	GATE_APU_MDLA(CLK_APU_MDLA_B3, "mdla_b3", "dsp3_sel", 3),
	GATE_APU_MDLA(CLK_APU_MDLA_B4, "mdla_b4", "dsp3_sel", 4),
	GATE_APU_MDLA(CLK_APU_MDLA_B5, "mdla_b5", "dsp3_sel", 5),
	GATE_APU_MDLA(CLK_APU_MDLA_B6, "mdla_b6", "dsp3_sel", 6),
	GATE_APU_MDLA(CLK_APU_MDLA_B7, "mdla_b7", "dsp3_sel", 7),
	GATE_APU_MDLA(CLK_APU_MDLA_B8, "mdla_b8", "dsp3_sel", 8),
	GATE_APU_MDLA(CLK_APU_MDLA_B9, "mdla_b9", "dsp3_sel", 9),
	GATE_APU_MDLA(CLK_APU_MDLA_B10, "mdla_b10", "dsp3_sel", 10),
	GATE_APU_MDLA(CLK_APU_MDLA_B11, "mdla_b11", "dsp3_sel", 11),
	GATE_APU_MDLA(CLK_APU_MDLA_B12, "mdla_b12", "dsp3_sel", 12),
	GATE_APU_MDLA(CLK_APU_MDLA_APB, "mdla_apb", "dsp3_sel", 13),
};

static const struct of_device_id of_match_clk_mt6779_apumdla[] = {
	{ .compatible = "mediatek,mt6779-apu_mdla", },
	{}
};

static int clk_mt6779_apumdla_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_APU_MDLA_NR_CLK);

	mtk_clk_register_gates(node, apumdla_clks, ARRAY_SIZE(apumdla_clks),
			       clk_data);

	return of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
}

static struct platform_driver clk_mt6779_apumdla_drv = {
	.probe = clk_mt6779_apumdla_probe,
	.driver = {
		.name = "clk-mt6779-apu_mdla",
		.of_match_table = of_match_clk_mt6779_apumdla,
	},
};

builtin_platform_driver(clk_mt6779_apumdla_drv);
