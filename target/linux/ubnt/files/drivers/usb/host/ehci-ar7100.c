/*
 * EHCI HCD (Host Controller Driver) for USB.
 *
 * Bus Glue for Atheros USB controller
 *
 */

#include <linux/platform_device.h>
#include <asm/mach-ar7100/ar7100.h>
#include <linux/delay.h>

extern int usb_disabled(void);

static void 
ar7100_start_ehc(struct platform_device *dev)
{
    int mask = AR7100_RESET_USB_HOST|AR7100_RESET_USB_PHY;

	printk(KERN_DEBUG __FILE__
		": Starting AR7100 EHCI USB Controller...");

    ar7100_reg_rmw_set(AR7100_RESET, mask);
    mdelay(1000);
    ar7100_reg_rmw_clear(AR7100_RESET, mask);

    //ar7100_reg_wr(AR7100_USB_CONFIG, 0x20);
    //ar7100_reg_rmw_clear(AR7100_USB_CONFIG, 0x4);

    /*Turning on the Buff and Desc swap bits */
    ar7100_reg_wr(AR7100_USB_CONFIG, 0x30000);

    /* WAR for HW bug. Here it adjusts the duration between two SOFS */
    /* Was: ar7100_reg_wr(AR7100_USB_FLADJ_VAL,0x20400); */
    ar7100_reg_wr(AR7100_USB_FLADJ_VAL,0x20c00);

    mdelay(900);
    printk("done. reset %#x usb config %#x\n", ar7100_reg_rd(AR7100_RESET),
            ar7100_reg_rd(AR7100_USB_CONFIG));
}

static void ar7100_stop_ehc(struct platform_device *dev)
{
	printk(KERN_DEBUG __FILE__
	       ": stopping AR7100 EHCI USB Controller\n");
    /*
     * XXX put in release code here
     */

}

int 
usb_ehci_ar7100_probe (const struct hc_driver       *driver, 
                       struct       usb_hcd         **hcd_out,
                       struct       platform_device *dev)
{
	int retval;
	struct usb_hcd *hcd;
    struct ehci_hcd *ehci;

    printk("\nProbing ehci...\n");
	if (dev->resource[1].flags != IORESOURCE_IRQ) {
		printk ("resource[1] is not IORESOURCE_IRQ");
		retval = -ENOMEM;
	}

	hcd = usb_create_hcd(driver, &dev->dev, "AR7100_usb");
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = dev->resource[0].start;
	hcd->rsrc_len   = dev->resource[0].end - dev->resource[0].start + 1;

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		printk("request_mem_region failed");
		retval = -EBUSY;
		goto err1;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		printk("ioremap failed");
		retval = -ENOMEM;
		goto err2;
	}

    printk("hcd->regs is %#x\n", hcd->regs);
	ar7100_start_ehc(dev);

    ehci              =   hcd_to_ehci(hcd);
    ehci->caps        =   hcd->regs;
    printk("ehci->caps is %#x\n", ehci->caps);
    printk("ehci->caps->hc_base is %#x\n", ehci->caps->hc_capbase);
    ehci->regs        =   hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));
    ehci->hcs_params  =   readl(&ehci->caps->hcs_params);


    /* retval = usb_add_hcd(hcd, dev->resource[1].start, SA_INTERRUPT | SA_SHIRQ); */
    retval = usb_add_hcd(hcd,dev->resource[1].start,IRQF_DISABLED);
	if (retval == 0) {
        printk("...probing done\n");
		return retval;
    }

	ar7100_stop_ehc(dev);
	iounmap(hcd->regs);
 err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
 err1:
	usb_put_hcd(hcd);
	return retval;
}


/* may be called without controller electrically present */
/* may be called with controller, bus, and devices active */

/**
 * usb_hcd_ar7100_remove - shutdown processing for AR7100-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_ar7100_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 */
void 
usb_ehci_ar7100_remove(struct usb_hcd *hcd, struct platform_device *dev)
{
	usb_remove_hcd(hcd);
	ar7100_stop_ehc(dev);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
}

/*-------------------------------------------------------------------------*/

static const struct hc_driver ehci_ar7100_hc_driver = {
	.description        =   hcd_name,
	.product_desc       =   "AR7100 EHCI",
	.hcd_priv_size      =   sizeof(struct ehci_hcd),
	/*
	 * generic hardware linkage
	 */
	.irq                =   ehci_irq,
	.flags              =   HCD_MEMORY | HCD_USB2,
	/*
	 * basic lifecycle operations
	 */
    .reset              =   ehci_init,
	.start              =	ehci_run,
	.stop               =   ehci_stop,
	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue        =   ehci_urb_enqueue,
	.urb_dequeue        =   ehci_urb_dequeue,
	.endpoint_disable   =   ehci_endpoint_disable,
	/*
	 * scheduling support
	 */
	.get_frame_number   =   ehci_get_frame,
	/*
	 * root hub support
	 */
	.hub_status_data    =   ehci_hub_status_data,
	.hub_control        =   ehci_hub_control,
};

/*-------------------------------------------------------------------------*/

static int 
ehci_hcd_ar7100_drv_probe(struct platform_device *pdev)
{
    struct usb_hcd *hcd  =  platform_get_drvdata(pdev);

	printk ("\nIn ar7100_ehci_drv_probe");

	if (usb_disabled()) {
        printk("usb disabled...?\n");
		return -ENODEV;
    }

	return(usb_ehci_ar7100_probe(&ehci_ar7100_hc_driver, &hcd, pdev));
}

static int 
ehci_hcd_ar7100_drv_remove(struct platform_device *pdev)
{
    struct usb_hcd *hcd  = platform_get_drvdata(pdev);

	usb_ehci_ar7100_remove(hcd, pdev);
	return 0;
}


static struct platform_driver ehci_hcd_ar7100_driver ={
    .probe  = ehci_hcd_ar7100_drv_probe,
    .remove = ehci_hcd_ar7100_drv_remove,
    .driver = {
        .name   = "ar7100-ehci",
    },
};


