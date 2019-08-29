// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/completion.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/clk.h>
#include <linux/platform_data/spi-mt65xx.h>

static bool spi_auto_test_flag;

#define SPI_DEBUG(fmt, args...) pr_info(fmt, ##args)

struct mtk_spi {
	void __iomem *base;
	u32 state;
	int pad_num;
	u32 *pad_sel;
	struct clk *parent_clk, *sel_clk, *spi_clk, *spare_clk;
	struct spi_transfer *cur_transfer;
	u32 xfer_len;
	u32 num_xfered;
	struct scatterlist *tx_sgl, *rx_sgl;
	u32 tx_sgl_len, rx_sgl_len;
	const struct mtk_spi_compatible *dev_comp;
};

void secspi_enable_clk(struct spi_device *spidev)
{
	struct spi_master *master;
	struct mtk_spi *ms;

	master = spidev->master;
	ms = spi_master_get_devdata(master);

	clk_prepare_enable(ms->spi_clk);
	if (!IS_ERR(ms->spare_clk))
		clk_prepare_enable(ms->spare_clk);
}

void secspi_disable_clk(struct spi_device *spidev)
{

	struct spi_master *master;
	struct mtk_spi *ms;

	master = spidev->master;
	ms = spi_master_get_devdata(master);

	clk_disable_unprepare(ms->spi_clk);
	if (!IS_ERR(ms->spare_clk))
		clk_disable_unprepare(ms->spare_clk);
}

void spi_transfer_malloc(struct spi_transfer *trans, int is2spis)
{
	int i;

	trans->tx_buf = kzalloc(trans->len, GFP_KERNEL);
	trans->rx_buf = kzalloc(trans->len, GFP_KERNEL);
	memset(trans->rx_buf, 0, trans->len);

	if (is2spis) {
		i = 1;
		*((char *)trans->tx_buf) = 0x55;
	} else
		i = 0;

	for (; i < trans->len; i++)
		*((char *)trans->tx_buf + i) = i;
}

void spi_transfer_free(struct spi_transfer *trans)
{
	kfree(trans->tx_buf);
	kfree(trans->rx_buf);
}

static void debug_packet(char *name, u8 *ptr, int len)
{
	int i;

	SPI_DEBUG("%s: ", name);
	for (i = 0; i < len; i++)
		SPI_DEBUG(" %02x", ptr[i]);
	 SPI_DEBUG("\n");
}

int spi_loopback_check(struct spi_device *spi,
	struct spi_transfer *trans, int is2spis)
{
	int i, j, value, err = 0;
	struct mtk_chip_config *chip_config = spi->controller_data;

	if (is2spis)
		i = 1;
	else
		i = 0;

	for (; i < trans->len; i++) {
		value = *((u8 *) trans->tx_buf + i);
		if (value != *((char *) trans->rx_buf + i))
			err++;
	}

	if (err) {
		SPI_DEBUG("spim_len:%d, err %d\n", trans->len, err);
		debug_packet("spim_tx_buf", (void *)trans->tx_buf, trans->len);
		debug_packet("spim_rx_buf", trans->rx_buf, trans->len);
		SPI_DEBUG("spim test fail.\n");
		spi_auto_test_flag = false;
	} else {
		SPI_DEBUG("spim_len:%d, err %d\n", trans->len, err);
		SPI_DEBUG("spim test pass.\n");
		spi_auto_test_flag = true;
	}

	if (err)
		return -1;
	else
		return 0;
}

int spi_loopback_transfer(struct spi_device *spi, int len, int is2spis)
{
	struct spi_transfer trans;
	struct spi_message msg;
	int ret = 0;

	memset(&trans, 0, sizeof(struct spi_transfer));
	spi_message_init(&msg);

	if (is2spis) {
		trans.speed_hz = 13000000;
		trans.len = len + 1;
	} else
		trans.len = len;
	trans.cs_change = 0;
	spi_transfer_malloc(&trans, is2spis);
	spi_message_add_tail(&trans, &msg);
	ret = spi_sync(spi, &msg);
	if (ret < 0)
		SPI_DEBUG("Message transfer err,line(%d):%d\n", __LINE__, ret);
	spi_loopback_check(spi, &trans, is2spis);
	spi_transfer_free(&trans);

	return ret;
}

static ssize_t spi_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	char *bp = buf;

	if (spi_auto_test_flag)
		bp += sprintf(bp, "spim talk with spis pass\n");
	else
		bp += sprintf(bp, "spim talk with spis fail\n");

	*bp++ = '\n';

	return bp - buf;
}

static ssize_t spi_store(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int len;
	struct spi_device *spi;

	spi = container_of(dev, struct spi_device, dev);

	if (!strncmp(buf, "-w", 2)) {
		buf += 3;

		if (!strncmp(buf, "len=", 4) &&
		    (sscanf(buf + 4, "%d", &len) == 1)) {
			spi_loopback_transfer(spi, len, 0);
		}
	}

	if (!strncmp(buf, "spim2spis", 9)) {
		buf += 10;

		if (!strncmp(buf, "len=", 4) &&
			(sscanf(buf + 4, "%d", &len) == 1)) {
			spi_loopback_transfer(spi, len, 1);
		}
	}

	if (!strncmp(buf, "enableclk", 9))
		secspi_enable_clk(spi);
	if (!strncmp(buf, "disableclk", 10))
		secspi_disable_clk(spi);

	return count;
}

static DEVICE_ATTR_RW(spi);

static struct device_attribute *spi_attribute[] = {
	&dev_attr_spi,
};

static void spi_create_attribute(struct device *dev)
{
	int size, idx;

	size = ARRAY_SIZE(spi_attribute);
	for (idx = 0; idx < size; idx++)
		device_create_file(dev, spi_attribute[idx]);
}

static int spi_mt65xx_dev_probe(struct spi_device *spi)
{
	spi_create_attribute(&spi->dev);
	return 0;
}

static int spi_mt65xx_dev_remove(struct spi_device *spi)
{
	return 0;
}

static const struct of_device_id spi_mt65xx_dev_dt_ids[] = {
	{ .compatible = "spi-mt65xx-dev" },
	{ .compatible = "spi-slave-autotest" },
	{},
};
MODULE_DEVICE_TABLE(of, spi_mt65xx_dev_dt_ids);

static struct spi_driver spi_mt65xx_dev_driver = {
	.driver = {
		.name	= "spi-mt65xx-dev",
		.of_match_table = of_match_ptr(spi_mt65xx_dev_dt_ids),
	},
	.probe		= spi_mt65xx_dev_probe,
	.remove		= spi_mt65xx_dev_remove,
};
module_spi_driver(spi_mt65xx_dev_driver);

MODULE_AUTHOR("Mediatek");
MODULE_LICENSE("GPL v2");
