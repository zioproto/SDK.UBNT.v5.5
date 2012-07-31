/*
 * OHCI HCD (Host Controller Driver) for USB.
 *
 * Bus Glue for Atheros USB controller
 *
 */

#include <linux/platform_device.h>
#include <asm/mach-ar7240/ar7240.h>
#include <linux/delay.h>

extern int usb_disabled(void);

static void 
ar7240_start_hc(struct platform_device *dev)
{
   // int mask = AR7240_RESET_USB_HOST|AR7240_RESET_USB_PHY;

	printk(KERN_DEBUG __FILE__
		": starting AR7240 OHCI USB Controller...\n");

    ar7240_reg_rmw_set(AR7240_RESET, AR7240_RESET_USB_OHCI_DLL);
    udelay(500);
    ar7240_reg_rmw_clear(AR7240_RESET, AR7240_RESET_USB_OHCI_DLL);
    ar7240_reg_rmw_set(AR7240_RESET, AR7240_RESET_USB_HOST);
    udelay(500);
    ar7240_reg_rmw_clear(AR7240_RESET, AR7240_RESET_USB_HOST);
    udelay(500);
    ar7240_reg_rmw_set(AR7240_RESET, AR7240_RESET_USB_OHCI_DLL);

    printk("%s val %x , reg:%x\n",__func__,ar7240_reg_rd(AR7240_RESET),AR7240_RESET_USB_OHCI_DLL);
    ar7240_reg_wr(AR7240_USB_FLADJ_VAL,0x3); 
}

static void ar7240_stop_hc(struct platform_device *dev)
{
	printk(KERN_DEBUG __FILE__
	       ": stopping ar7240 OHCI USB Controller\n");
    /*
     * XXX put in release code here
     */

}

int 
usb_hcd_ar7240_probe(const struct   hc_driver        *driver, 
                     struct         platform_device  *dev)
{
	int retval;
	struct usb_hcd *hcd;

    printk("    probing...\n");
	if (dev->resource[1].flags != IORESOURCE_IRQ) {
		printk ("resource[1] is not IORESOURCE_IRQ");
		retval = -ENOMEM;
	}

	hcd = usb_create_hcd(driver, &dev->dev, "ar7240_usb");
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

	ar7240_start_hc(dev);
	ohci_hcd_init(hcd_to_ohci(hcd));

	retval = usb_add_hcd(hcd, dev->resource[1].start, SA_INTERRUPT);
	if (retval == 0) {
        printk("probing done\n");
		return retval;
    }

	ar7240_stop_hc(dev);
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
 * usb_hcd_ar7240_remove - shutdown processing for Au1xxx-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_ar7240_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 */
void 
usb_hcd_ar7240_remove (struct usb_hcd *hcd, struct platform_device *dev)
{
	usb_remove_hcd(hcd);
	ar7240_stop_hc(dev);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
}


static int __devinit
ohci_ar7240_start (struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	int		ret;

	ohci_dbg (ohci, "ohci_ar7240_start, ohci:%p", ohci);

	if ((ret = ohci_init (ohci)) < 0) {
        printk("cant init %s", hcd->self.bus_name);
		return ret;
    }

	if ((ret = ohci_run (ohci)) < 0) {
		err ("can't start %s", hcd->self.bus_name);
		ohci_stop (hcd);
		return ret;
	}

	return 0;
}

/*-------------------------------------------------------------------------*/

static const struct hc_driver ohci_ar7240_hc_driver = {
	.description        =   hcd_name,
	.product_desc       =   "AR7240 OHCI",
	.hcd_priv_size      =   sizeof(struct ohci_hcd),
	/*
	 * generic hardware linkage
	 */
	.irq                =   ohci_irq,
	.flags              =   HCD_USB11 | HCD_MEMORY,
	/*
	 * basic lifecycle operations
	 */
	.start              =   ohci_ar7240_start,
	.stop               =   ohci_stop,
	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue        =   ohci_urb_enqueue,
	.urb_dequeue        =   ohci_urb_dequeue,
	.endpoint_disable   =   ohci_endpoint_disable,
	/*
	 * scheduling support
	 */
	.get_frame_number   =   ohci_get_frame,
	/*
	 * root hub support
	 */
	.hub_status_data    =   ohci_hub_status_data,
	.hub_control        =   ohci_hub_control,
	.start_port_reset   =   ohci_start_port_reset,
};

/*-------------------------------------------------------------------------*/

static int 
ohci_hcd_ar7240_drv_probe(struct platform_device *pdev)
{
	printk ("In ohci_hcd_ar7240_drv_probe");

	if (usb_disabled()) {
        printk("USB disabled\n");
		return -ENODEV;
    }

	return(usb_hcd_ar7240_probe(&ohci_ar7240_hc_driver, pdev));
}

static int 
ohci_hcd_ar7240_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_hcd_ar7240_remove(hcd, pdev);

	return 0;
}

static struct platform_driver ohci_hcd_ar7240_driver = {
	.probe		= ohci_hcd_ar7240_drv_probe,
	.remove		= ohci_hcd_ar7240_drv_remove,
	.driver		= {
		.name	= "ar7240-ohci",
		.owner	= THIS_MODULE,
	},
};

static int __devinit ar7240_ohci_init (void)
{
	printk (DRIVER_INFO " (ar7240_ohci)");
	printk ("block sizes: ed %d td %d\n",
		sizeof (struct ed), sizeof (struct td));

	return platform_driver_register(&ohci_hcd_ar7240_driver);
}

static void __exit ar7240_ohci_cleanup (void)
{
	platform_driver_unregister(&ohci_hcd_ar7240_driver);
}

device_initcall(ar7240_ohci_init);
module_exit(ar7240_ohci_cleanup);
