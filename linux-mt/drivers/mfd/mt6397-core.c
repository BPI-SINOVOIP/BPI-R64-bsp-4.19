/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Flora Fu, MediaTek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>
#include <linux/mfd/core.h>
#include <linux/mfd/mt6323/core.h>
#include <linux/mfd/mt6358/core.h>
#include <linux/mfd/mt6359/core.h>
#include <linux/mfd/mt6397/core.h>
#include <linux/mfd/mt6323/registers.h>
#include <linux/mfd/mt6358/registers.h>
#include <linux/mfd/mt6359/registers.h>
#include <linux/mfd/mt6397/registers.h>

#define MT6358_RTC_BASE		0x0588
#define MT6358_RTC_SIZE		0x3c
#define MT6358_RTC_WRTGR_OFFSET	0x3a
#define MT6397_RTC_BASE		0xe000
#define MT6397_RTC_SIZE		0x3e
#define MT6397_RTC_WRTGR_OFFSET	0x3c

static const struct resource mt6359_rtc_resources[] = {
	{
		.start = MT6358_RTC_BASE,
		.end   = MT6358_RTC_BASE + MT6358_RTC_SIZE,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MT6359_IRQ_RTC,
		.end   = MT6359_IRQ_RTC,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = MT6358_RTC_WRTGR_OFFSET,
		.end   = MT6358_RTC_WRTGR_OFFSET,
		.flags = IORESOURCE_REG,
	},
};

static const struct resource mt6358_rtc_resources[] = {
	{
		.start = MT6358_RTC_BASE,
		.end   = MT6358_RTC_BASE + MT6358_RTC_SIZE,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MT6358_IRQ_RTC,
		.end   = MT6358_IRQ_RTC,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = MT6358_RTC_WRTGR_OFFSET,
		.end   = MT6358_RTC_WRTGR_OFFSET,
		.flags = IORESOURCE_REG,
	},
};

static const struct resource mt6397_rtc_resources[] = {
	{
		.start = MT6397_RTC_BASE,
		.end   = MT6397_RTC_BASE + MT6397_RTC_SIZE,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MT6397_IRQ_RTC,
		.end   = MT6397_IRQ_RTC,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = MT6397_RTC_WRTGR_OFFSET,
		.end   = MT6397_RTC_WRTGR_OFFSET,
		.flags = IORESOURCE_REG,
	},
};

static const struct resource mt6323_keys_resources[] = {
	DEFINE_RES_IRQ(MT6323_IRQ_STATUS_PWRKEY),
	DEFINE_RES_IRQ(MT6323_IRQ_STATUS_FCHRKEY),
};

static const struct resource mt6397_keys_resources[] = {
	DEFINE_RES_IRQ(MT6397_IRQ_PWRKEY),
	DEFINE_RES_IRQ(MT6397_IRQ_HOMEKEY),
};

static const struct resource mt6359_auxadc_resources[] = {
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_AUXADC_IMP, "imp"),
};

static const struct resource mt6359_keys_resources[] = {
	DEFINE_RES_IRQ(MT6359_IRQ_PWRKEY),
	DEFINE_RES_IRQ(MT6359_IRQ_HOMEKEY),
	DEFINE_RES_IRQ(MT6359_IRQ_PWRKEY_R),
	DEFINE_RES_IRQ(MT6359_IRQ_HOMEKEY_R),
};

static const struct resource mt6359_regulators_resources[] = {
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VPU_OC, "VPU"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VCORE_OC, "VCORE"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VGPU11_OC, "VGPU11"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VGPU12_OC, "VGPU12"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VMODEM_OC, "VMODEM"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VPROC1_OC, "VPROC1"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VPROC2_OC, "VPROC2"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VS1_OC, "VS1"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VS2_OC, "VS2"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VPA_OC, "VPA"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VFE28_OC, "VFE28"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VXO22_OC, "VXO22"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VRF18_OC, "VRF18"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VRF12_OC, "VRF12"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VEFUSE_OC, "VEFUSE"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VCN33_1_OC, "VCN33_1"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VCN33_2_OC, "VCN33_2"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VCN13_OC, "VCN13"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VCN18_OC, "VCN18"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VA09_OC, "VA09"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VCAMIO_OC, "VCAMIO"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VA12_OC, "VA12"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VAUX18_OC, "VAUX18"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VAUD18_OC, "VAUD18"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VIO18_OC, "VIO18"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VSRAM_PROC1_OC, "VSRAM_PROC1"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VSRAM_PROC2_OC, "VSRAM_PROC2"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VSRAM_OTHERS_OC, "VSRAM_OTHERS"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VSRAM_MD_OC, "VSRAM_MD"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VEMC_OC, "VEMC"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VSIM1_OC, "VSIM1"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VSIM2_OC, "VSIM2"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VUSB_OC, "VUSB"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VRFCK_OC, "VRFCK"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VBBCK_OC, "VBBCK"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VBIF28_OC, "VBIF28"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VIBR_OC, "VIBR"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VIO28_OC, "VIO28"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VM18_OC, "VM18"),
	DEFINE_RES_IRQ_NAMED(MT6359_IRQ_VUFS_OC, "VUFS"),
};

static const struct mfd_cell mt6323_devs[] = {
	{
		.name = "mt6323-regulator",
		.of_compatible = "mediatek,mt6323-regulator"
	}, {
		.name = "mt6323-led",
		.of_compatible = "mediatek,mt6323-led"
	}, {
		.name = "mtk-pmic-keys",
		.num_resources = ARRAY_SIZE(mt6323_keys_resources),
		.resources = mt6323_keys_resources,
		.of_compatible = "mediatek,mt6323-keys"
	},
};

static const struct mfd_cell mt6358_devs[] = {
	{
		.name = "mt6358-regulator",
		.of_compatible = "mediatek,mt6358-regulator"
	}, {
		.name = "mt6397-rtc",
		.num_resources = ARRAY_SIZE(mt6358_rtc_resources),
		.resources = mt6358_rtc_resources,
		.of_compatible = "mediatek,mt6358-rtc",
	}, {
		.name = "mt6358-sound",
		.of_compatible = "mediatek,mt6358-sound"
	},
};

static const struct mfd_cell mt6359_devs[] = {
	{
		.name = "mt635x-auxadc",
		.of_compatible = "mediatek,mt6359-auxadc",
		.num_resources = ARRAY_SIZE(mt6359_auxadc_resources),
		.resources = mt6359_auxadc_resources,
	}, {
		.name = "mt6359-efuse",
		.of_compatible = "mediatek,mt6359-efuse",
	}, {
		.name = "mt6359-regulator",
		.of_compatible = "mediatek,mt6359-regulator",
		.num_resources = ARRAY_SIZE(mt6359_regulators_resources),
		.resources = mt6359_regulators_resources,
	}, {
		.name = "mt6397-rtc",
		.num_resources = ARRAY_SIZE(mt6359_rtc_resources),
		.resources = mt6359_rtc_resources,
		.of_compatible = "mediatek,mt6359-rtc",
	}, {
		.name = "mtk-pmic-keys",
		.num_resources = ARRAY_SIZE(mt6359_keys_resources),
		.resources = mt6359_keys_resources,
		.of_compatible = "mediatek,mt6359-keys"
	}
};

static const struct mfd_cell mt6397_devs[] = {
	{
		.name = "mt6397-rtc",
		.num_resources = ARRAY_SIZE(mt6397_rtc_resources),
		.resources = mt6397_rtc_resources,
		.of_compatible = "mediatek,mt6397-rtc",
	}, {
		.name = "mt6397-regulator",
		.of_compatible = "mediatek,mt6397-regulator",
	}, {
		.name = "mt6397-codec",
		.of_compatible = "mediatek,mt6397-codec",
	}, {
		.name = "mt6397-clk",
		.of_compatible = "mediatek,mt6397-clk",
	}, {
		.name = "mt6397-pinctrl",
		.of_compatible = "mediatek,mt6397-pinctrl",
	}, {
		.name = "mtk-pmic-keys",
		.num_resources = ARRAY_SIZE(mt6397_keys_resources),
		.resources = mt6397_keys_resources,
		.of_compatible = "mediatek,mt6397-keys"
	}
};

struct chip_data {
	u32 cid_addr;
	u32 cid_shift;
};

static const struct chip_data mt6323_core = {
	.cid_addr = MT6323_CID,
	.cid_shift = 0,
};

static const struct chip_data mt6358_core = {
	.cid_addr = MT6358_SWCID,
	.cid_shift = 8,
};

static const struct chip_data mt6359_core = {
	.cid_addr = MT6359_SWCID,
	.cid_shift = 8,
};

static const struct chip_data mt6397_core = {
	.cid_addr = MT6397_CID,
	.cid_shift = 0,
};

static int mt6397_probe(struct platform_device *pdev)
{
	int ret;
	unsigned int id;
	struct mt6397_chip *pmic;
	const struct chip_data *pmic_core;

	pmic = devm_kzalloc(&pdev->dev, sizeof(*pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;

	pmic->dev = &pdev->dev;

	/*
	 * mt6397 MFD is child device of soc pmic wrapper.
	 * Regmap is set from its parent.
	 */
	pmic->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!pmic->regmap)
		return -ENODEV;

	pmic_core = of_device_get_match_data(&pdev->dev);
	if (!pmic_core)
		return -ENODEV;

	ret = regmap_read(pmic->regmap, pmic_core->cid_addr, &id);
	if (ret) {
		dev_err(&pdev->dev, "Failed to read chip id: %d\n", ret);
		return ret;
	}

	pmic->chip_id = (id >> pmic_core->cid_shift) & 0xff;

	platform_set_drvdata(pdev, pmic);

	pmic->irq = platform_get_irq(pdev, 0);
	if (pmic->irq <= 0) {
		dev_err(&pdev->dev,
			"failed to get platform irq, ret=%d", pmic->irq);
		return pmic->irq;
	}

	switch (pmic->chip_id) {
	case MT6358_CHIP_ID:
	case MT6359_CHIP_ID:
		ret = mt6358_irq_init(pmic);
		break;

	case MT6323_CHIP_ID:
	case MT6397_CHIP_ID:
	case MT6391_CHIP_ID:
		ret = mt6397_irq_init(pmic);
		break;

	default:
		dev_err(&pdev->dev, "unsupported chip: 0x%x\n", pmic->chip_id);
		ret = -ENODEV;
		break;
	}
	if (ret)
		return ret;

	switch (pmic->chip_id) {
	case MT6323_CHIP_ID:
		ret = devm_mfd_add_devices(&pdev->dev, -1, mt6323_devs,
					   ARRAY_SIZE(mt6323_devs), NULL,
					   0, pmic->irq_domain);
		break;

	case MT6358_CHIP_ID:
		ret = devm_mfd_add_devices(&pdev->dev, -1, mt6358_devs,
					   ARRAY_SIZE(mt6358_devs), NULL,
					   0, pmic->irq_domain);
		break;

	case MT6359_CHIP_ID:
		ret = devm_mfd_add_devices(&pdev->dev, -1, mt6359_devs,
					   ARRAY_SIZE(mt6359_devs), NULL,
					   0, pmic->irq_domain);
		break;

	case MT6397_CHIP_ID:
	case MT6391_CHIP_ID:
		ret = devm_mfd_add_devices(&pdev->dev, -1, mt6397_devs,
					   ARRAY_SIZE(mt6397_devs), NULL,
					   0, pmic->irq_domain);
		break;

	default:
		dev_err(&pdev->dev, "unsupported chip: 0x%x\n", pmic->chip_id);
		return -ENODEV;
	}

	if (ret) {
		irq_domain_remove(pmic->irq_domain);
		dev_err(&pdev->dev, "failed to add child devices: %d\n", ret);
	}

	return ret;
}

static const struct of_device_id mt6397_of_match[] = {
	{
		.compatible = "mediatek,mt6323",
		.data = &mt6323_core,
	}, {
		.compatible = "mediatek,mt6358",
		.data = &mt6358_core,
	}, {
		.compatible = "mediatek,mt6359",
		.data = &mt6359_core,
	}, {
		.compatible = "mediatek,mt6397",
		.data = &mt6397_core,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, mt6397_of_match);

static const struct platform_device_id mt6397_id[] = {
	{ "mt6397", 0 },
	{ },
};
MODULE_DEVICE_TABLE(platform, mt6397_id);

static struct platform_driver mt6397_driver = {
	.probe = mt6397_probe,
	.driver = {
		.name = "mt6397",
		.of_match_table = of_match_ptr(mt6397_of_match),
	},
	.id_table = mt6397_id,
};

module_platform_driver(mt6397_driver);

MODULE_AUTHOR("Flora Fu, MediaTek");
MODULE_DESCRIPTION("Driver for MediaTek MT6397 PMIC");
MODULE_LICENSE("GPL");
