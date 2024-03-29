/*
 * drivers/usb/host/xhci_sunxi.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangjx, 2016-9-9, create this file
 *
 * SoftWinner XHCI Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/dma-mapping.h>

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/io.h>

#include <linux/clk.h>
#include "sunxi_hci.h"
#include "xhci.h"

#define  SUNXI_XHCI_NAME	"sunxi-xhci"
static const char xhci_name[] = SUNXI_XHCI_NAME;
#define SUNXI_ALIGN_MASK		(16 - 1)

#ifdef CONFIG_USB_SUNXI_XHCI
#define  SUNXI_XHCI_OF_MATCH	"allwinner,sunxi-xhci"
#else
#define  SUNXI_XHCI_OF_MATCH   "NULL"
#endif

static struct sunxi_hci_hcd *g_sunxi_xhci;
static struct sunxi_hci_hcd *g_dev_data;

static ssize_t xhci_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_hci_hcd *sunxi_xhci = NULL;

	if (dev == NULL) {
		DMSG_PANIC("ERR: Argment is invalid\n");
		return 0;
	}

	sunxi_xhci = dev->platform_data;
	if (sunxi_xhci == NULL) {
		DMSG_PANIC("ERR: sunxi_xhci is null\n");
		return 0;
	}

	return sprintf(buf, "xhci:%d, probe:%u\n",
			sunxi_xhci->usbc_no, sunxi_xhci->probe);
}

static ssize_t xhci_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sunxi_hci_hcd *sunxi_xhci = NULL;
	int value = 0;

	if (dev == NULL) {
		DMSG_PANIC("ERR: Argment is invalid\n");
		return 0;
	}

	sunxi_xhci = dev->platform_data;
	if (sunxi_xhci == NULL) {
		DMSG_PANIC("ERR: sunxi_xhci is null\n");
		return 0;
	}

	sunxi_xhci->host_init_state = 0;

	sscanf(buf, "%d", &value);
	if (value == 1)
		sunxi_usb_enable_xhci();
	else if (value == 0)
		sunxi_usb_disable_xhci();
	else
		DMSG_INFO("unkown value (%d)\n", value);

	return count;
}

static DEVICE_ATTR(xhci_enable, 0664, xhci_enable_show, xhci_enable_store);


int sunxi_core_open_phy(void __iomem *regs)
{
	int reg_val = 0;

	reg_val = USBC_Readl(regs + (SUNXI_PHY_EXTERNAL_CONTROL - SUNXI_GLOBALS_REGS_START));
	reg_val |= SUNXI_PEC_EXTERN_VBUS; /* Use extern vbus to phy */
	reg_val |= SUNXI_PEC_SSC_EN; /* SSC_EN */
	reg_val |= SUNXI_PEC_REF_SSP_EN; /*REF_SSP_EN */
	USBC_Writel(reg_val, regs + (SUNXI_PHY_EXTERNAL_CONTROL - SUNXI_GLOBALS_REGS_START));

	reg_val = USBC_Readl(regs + (SUNXI_PIPE_CLOCK_CONTROL - SUNXI_GLOBALS_REGS_START));
	reg_val |= SUNXI_PPC_PIPE_CLK_OPEN; /* open PIPE clock */
	USBC_Writel(reg_val, regs + (SUNXI_PIPE_CLOCK_CONTROL - SUNXI_GLOBALS_REGS_START));

	reg_val = USBC_Readl(regs + (SUNXI_APP - SUNXI_GLOBALS_REGS_START));
	reg_val |= SUNXI_APP_FOCE_VBUS; /* open PIPE clock */
	USBC_Writel(reg_val, regs + (SUNXI_APP - SUNXI_GLOBALS_REGS_START));

	/* It is set 0x0047fc87 on bare-metal. */
	USBC_Writel(0xffffffff, regs + (SUNXI_PHY_TURN_LOW - SUNXI_GLOBALS_REGS_START));

	reg_val = USBC_Readl(regs + (SUNXI_PHY_TURN_HIGH - SUNXI_GLOBALS_REGS_START));
	reg_val |= SUNXI_TXVBOOSTLVL(0x7);
	reg_val |= SUNXI_LOS_BIAS(0x7);

	reg_val &= ~(SUNXI_TX_SWING_FULL(0x7f));
	reg_val |= SUNXI_TX_SWING_FULL(0x55);

	reg_val &= ~(SUNXI_TX_DEEMPH_6DB(0x3f));
	reg_val |= SUNXI_TX_DEEMPH_6DB(0x20);

	reg_val &= ~(SUNXI_TX_DEEMPH_3P5DB(0x3f));
	reg_val |= SUNXI_TX_DEEMPH_3P5DB(0x15);
	USBC_Writel(reg_val, regs + (SUNXI_PHY_TURN_HIGH - SUNXI_GLOBALS_REGS_START));

	return 0;

}

static void sunxi_start_xhci(struct sunxi_hci_hcd *sunxi_xhci)
{
	sunxi_xhci->open_clock(sunxi_xhci, 1);
	sunxi_xhci->set_power(sunxi_xhci, 1);
}

static void sunxi_stop_xhci(struct sunxi_hci_hcd *sunxi_xhci)
{
	sunxi_xhci->set_power(sunxi_xhci, 0);
	sunxi_xhci->close_clock(sunxi_xhci, 0);
}

void sunxi_set_mode(struct sunxi_hci_hcd *sunxi_xhci, u32 mode)
{
	u32 reg;

	reg = USBC_Readl(sunxi_xhci->regs + (SUNXI_GLOBALS_REGS_GCTL - SUNXI_GLOBALS_REGS_START));
	reg &= ~(SUNXI_GCTL_PRTCAPDIR(SUNXI_GCTL_PRTCAP_OTG));
	reg |= SUNXI_GCTL_PRTCAPDIR(mode);
	USBC_Writel(reg, sunxi_xhci->regs + (SUNXI_GLOBALS_REGS_GCTL - SUNXI_GLOBALS_REGS_START));
}

int xhci_host_init(struct sunxi_hci_hcd *sunxi_xhci)
{
	struct platform_device	*xhci;
	int			ret;

	xhci = platform_device_alloc("xhci-hcd", PLATFORM_DEVID_AUTO);
	if (!xhci) {
		dev_err(sunxi_xhci->dev, "couldn't allocate xHCI device\n");
		ret = -ENOMEM;
		goto err0;
	}

	dma_set_coherent_mask(&xhci->dev, sunxi_xhci->dev->coherent_dma_mask);

	xhci->dev.parent	= sunxi_xhci->dev;
	xhci->dev.dma_mask	= sunxi_xhci->dev->dma_mask;
	xhci->dev.dma_parms	= sunxi_xhci->dev->dma_parms;

	sunxi_xhci->pdev = xhci;

	ret = platform_device_add_resources(xhci, sunxi_xhci->xhci_resources,
						XHCI_RESOURCES_NUM);
	if (ret) {
		dev_err(sunxi_xhci->dev, "couldn't add resources to xHCI device\n");
		goto err1;
	}

	ret = platform_device_add(xhci);
	if (ret) {
		dev_err(sunxi_xhci->dev, "failed to register xHCI device\n");
		goto err1;
	}

	return 0;

err1:
	platform_device_put(xhci);

err0:
	return ret;
}

void xhci_host_exit(struct sunxi_hci_hcd *sunxi_xhci)
{
	platform_device_unregister(sunxi_xhci->pdev);
}

static int sunxi_xhci_hcd_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct sunxi_hci_hcd *sunxi_xhci = NULL;
	void			*mem;

	struct device		*dev = &pdev->dev;

	struct resource		*res;
	void __iomem	*regs;

	if (pdev == NULL) {
		DMSG_PANIC("ERR: %s, Argment is invalid\n", __func__);
		return -1;
	}

	/* if usb is disabled, can not probe */
	if (usb_disabled()) {
		DMSG_PANIC("ERR: usb hcd is disabled\n");
		return -ENODEV;
	}

	mem = devm_kzalloc(dev, sizeof(*sunxi_xhci) + SUNXI_ALIGN_MASK, GFP_KERNEL);
	if (!mem) {
		dev_err(dev, "not enough memory\n");
		return -ENOMEM;
	}
	sunxi_xhci = PTR_ALIGN(mem, SUNXI_ALIGN_MASK + 1);
	sunxi_xhci->mem = mem;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(dev, "missing IRQ\n");
		return -ENODEV;
	}
	sunxi_xhci->xhci_resources[1].start = res->start;
	sunxi_xhci->xhci_resources[1].end = res->end;
	sunxi_xhci->xhci_resources[1].flags = res->flags;
	sunxi_xhci->xhci_resources[1].name = res->name;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "missing memory resource\n");
		return -ENODEV;
	}
	sunxi_xhci->xhci_resources[0].start = res->start;
	sunxi_xhci->xhci_resources[0].end = sunxi_xhci->xhci_resources[0].start +
					XHCI_REGS_END;
	sunxi_xhci->xhci_resources[0].flags = res->flags;
	sunxi_xhci->xhci_resources[0].name = res->name;

	/*
	* Request memory region but exclude xHCI regs,
	* since it will be requested by the xhci-plat driver.
	*/
	res = devm_request_mem_region(dev, res->start + SUNXI_GLOBALS_REGS_START,
			resource_size(res) - SUNXI_GLOBALS_REGS_START,
			dev_name(dev));
	if (!res) {
		dev_err(dev, "can't request mem region\n");
		return -ENOMEM;
	}

	regs = devm_ioremap_nocache(dev, res->start, resource_size(res));
	if (!regs) {
		dev_err(dev, "ioremap failed\n");
		return -ENOMEM;
	}

	spin_lock_init(&sunxi_xhci->lock);

	sunxi_xhci->regs	= regs;
	sunxi_xhci->regs_size	= resource_size(res);
	sunxi_xhci->dev	= dev;

	dev->dma_mask	= dev->parent->dma_mask;
	dev->dma_parms	= dev->parent->dma_parms;
	dma_set_coherent_mask(dev, dev->parent->coherent_dma_mask);

	ret = init_sunxi_hci(pdev, SUNXI_USB_XHCI);
	if (ret != 0) {
		dev_err(&pdev->dev, "init_sunxi_hci is fail\n");
		return 0;
	}

	platform_set_drvdata(pdev, sunxi_xhci);

	sunxi_start_xhci(pdev->dev.platform_data);
	sunxi_core_open_phy(sunxi_xhci->regs);
	sunxi_set_mode(sunxi_xhci, SUNXI_GCTL_PRTCAP_HOST);

	xhci_host_init(sunxi_xhci);

	device_create_file(&pdev->dev, &dev_attr_xhci_enable);

	g_sunxi_xhci = sunxi_xhci;
	g_dev_data = pdev->dev.platform_data;

	g_dev_data->probe = 1;

	return 0;
}

static int sunxi_xhci_hcd_remove(struct platform_device *pdev)
{
	struct sunxi_hci_hcd *sunxi_xhci = NULL;
	struct sunxi_hci_hcd *dev_data = NULL;

	if (pdev == NULL) {
		DMSG_PANIC("ERR: %s, Argment is invalid\n", __func__);
		return -1;
	}

	sunxi_xhci = g_sunxi_xhci;
	dev_data = g_dev_data;
	if (sunxi_xhci == NULL) {
		DMSG_PANIC("ERR: %s, sunxi_xhci is null\n", __func__);
		return -1;
	}

	device_remove_file(&pdev->dev, &dev_attr_xhci_enable);

	xhci_host_exit(sunxi_xhci);
	sunxi_stop_xhci(dev_data);

	dev_data->probe = 0;

	return 0;
}

static void sunxi_xhci_hcd_shutdown(struct platform_device *pdev)
{
	struct sunxi_hci_hcd *sunxi_xhci = NULL;
	struct sunxi_hci_hcd *dev_data = NULL;

	if (pdev == NULL) {
		DMSG_PANIC("ERR: %s, Argment is invalid\n", __func__);
		return;
	}

	sunxi_xhci = g_sunxi_xhci;
	dev_data = g_dev_data;
	if (sunxi_xhci == NULL) {
		DMSG_PANIC("ERR: %s, is null\n", __func__);
		return;
	}

	if (dev_data->probe == 0) {
		DMSG_INFO("%s, %s is disable, need not shutdown\n",  __func__, sunxi_xhci->hci_name);
		return;
	}

	DMSG_INFO("[%s]: xhci shutdown start\n", sunxi_xhci->hci_name);

	usb_hcd_platform_shutdown(sunxi_xhci->pdev);

	DMSG_INFO("[%s]: xhci shutdown end\n", sunxi_xhci->hci_name);

	return;
}

int sunxi_usb_disable_xhci(void)
{
	struct sunxi_hci_hcd *sunxi_xhci = NULL;
	struct sunxi_hci_hcd *dev_data = NULL;

	sunxi_xhci = g_sunxi_xhci;
	dev_data = g_dev_data;
	if (sunxi_xhci == NULL || dev_data == NULL) {
		DMSG_PANIC("ERR: sunxi_xhci is null\n");
		return -1;
	}

	if (dev_data->probe == 0) {
		DMSG_PANIC("sunxi_xhci is disable, can not disable again\n");
		return -1;
	}

	dev_data->probe = 0;

	DMSG_INFO("[%s]: sunxi_usb_disable_xhci\n", sunxi_xhci->hci_name);

	xhci_host_exit(sunxi_xhci);
	sunxi_stop_xhci(dev_data);

	return 0;
}
EXPORT_SYMBOL(sunxi_usb_disable_xhci);

int sunxi_usb_enable_xhci(void)
{
	struct sunxi_hci_hcd *sunxi_xhci = NULL;
	struct sunxi_hci_hcd *dev_data = NULL;

	sunxi_xhci = g_sunxi_xhci;
	dev_data = g_dev_data;
	if (sunxi_xhci == NULL || dev_data == NULL) {
		DMSG_PANIC("ERR: sunxi_xhci is null\n");
		return -1;
	}

	if (dev_data->probe == 1) {
		DMSG_PANIC("sunxi_xhci is already enable, can not enable again\n");
		return -1;
	}

	dev_data->probe = 1;

	DMSG_INFO("[%s]: sunxi_usb_enable_xhci\n", sunxi_xhci->hci_name);

	sunxi_start_xhci(dev_data);
	sunxi_core_open_phy(sunxi_xhci->regs);
	sunxi_set_mode(sunxi_xhci, SUNXI_GCTL_PRTCAP_HOST);

	xhci_host_init(sunxi_xhci);

	return 0;
}
EXPORT_SYMBOL(sunxi_usb_enable_xhci);

#ifdef CONFIG_PM
static int sunxi_xhci_hcd_suspend(struct device *dev)
{
	struct sunxi_hci_hcd *sunxi_xhci = NULL;
	struct usb_hcd *hcd = NULL;
	struct xhci_hcd *xhci = NULL;

	if (dev == NULL) {
		DMSG_PANIC("ERR: %s, Argment is invalid\n", __func__);
		return 0;
	}

	hcd = dev_get_drvdata(dev);
	if (hcd == NULL) {
		DMSG_PANIC("ERR: hcd is null\n");
		return 0;
	}

	sunxi_xhci = dev->platform_data;
	if (sunxi_xhci == NULL) {
		DMSG_PANIC("ERR: sunxi_xhci is null\n");
		return 0;
	}

	if (sunxi_xhci->probe == 0) {
		DMSG_INFO("[%s]: is disable, can not suspend\n",
			sunxi_xhci->hci_name);
		return 0;
	}

	xhci = hcd_to_xhci(hcd);
	if (xhci == NULL) {
		DMSG_PANIC("ERR: xhci is null\n");
		return 0;
	}

	if (sunxi_xhci->wakeup_suspend) {
		DMSG_INFO("[%s]: not suspend\n", sunxi_xhci->hci_name);
	} else {
		DMSG_INFO("[%s]: sunxi_xhci_hcd_suspend\n", sunxi_xhci->hci_name);

		/*xhci_suspend(hcd, device_may_wakeup(dev));*/
		sunxi_stop_xhci(sunxi_xhci);
	}

	return 0;
}

static int sunxi_xhci_hcd_resume(struct device *dev)
{
	struct sunxi_hci_hcd *sunxi_xhci = NULL;
	struct usb_hcd *hcd = NULL;
	struct xhci_hcd *xhci = NULL;

	if (dev == NULL) {
		DMSG_PANIC("ERR: Argment is invalid\n");
		return 0;
	}

	hcd = dev_get_drvdata(dev);
	if (hcd == NULL) {
		DMSG_PANIC("ERR: hcd is null\n");
		return 0;
	}

	sunxi_xhci = dev->platform_data;
	if (sunxi_xhci == NULL) {
		DMSG_PANIC("ERR: sunxi_xhci is null\n");
		return 0;
	}

	if (sunxi_xhci->probe == 0) {
		DMSG_INFO("[%s]: is disable, can not resume\n",
			sunxi_xhci->hci_name);
		return 0;
	}

	xhci = hcd_to_xhci(hcd);
	if (xhci == NULL) {
		DMSG_PANIC("ERR: xhci is null\n");
		return 0;
	}

	if (sunxi_xhci->wakeup_suspend) {
		DMSG_INFO("[%s]: controller not suspend, need not resume\n", sunxi_xhci->hci_name);
	} else {
		DMSG_INFO("[%s]: sunxi_xhci_hcd_resume\n", sunxi_xhci->hci_name);

		sunxi_start_xhci(sunxi_xhci);

		/*xhci_resume(hcd, false);*/
	}

	return 0;
}

static const struct dev_pm_ops  xhci_pmops = {
	.suspend	= sunxi_xhci_hcd_suspend,
	.resume		= sunxi_xhci_hcd_resume,
};
#endif

static const struct of_device_id sunxi_xhci_match[] = {
	{.compatible = SUNXI_XHCI_OF_MATCH, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_xhci_match);

static struct platform_driver sunxi_xhci_hcd_driver = {
	.probe  = sunxi_xhci_hcd_probe,
	.remove	= sunxi_xhci_hcd_remove,
	.shutdown = sunxi_xhci_hcd_shutdown,
	.driver = {
			.name	= xhci_name,
			.owner	= THIS_MODULE,
#ifdef CONFIG_PM
			.pm	= &xhci_pmops,
#endif
			.of_match_table = sunxi_xhci_match,
		}
};

module_platform_driver(sunxi_xhci_hcd_driver);

MODULE_ALIAS("platform:sunxi xhci");
MODULE_AUTHOR("wangjx <wangjx@allwinnertech.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Allwinnertech Xhci Controller Driver");
