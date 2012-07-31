//#include <linux/config.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/resource.h>

#include <linux/console.h>
#include <asm/serial.h>

#include <linux/tty.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>

#include <asm/mach-ar7240/ar7240.h>

#ifdef CONFIG_WASP_SUPPORT
extern uint32_t ath_ref_clk_freq;
#else
extern uint32_t ar7240_ahb_freq;
#endif

#ifdef CONFIG_USB_OHCI_AR7240
/*
 * OHCI (USB full speed host controller)
 */
static struct resource ar7240_usb_ohci_resources[] = {
	[0] = {
		.start		= AR7240_USB_OHCI_BASE,
		.end		= AR7240_USB_OHCI_BASE + AR7240_USB_WINDOW - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= AR7240_CPU_IRQ_USB,
		.end		= AR7240_CPU_IRQ_USB,
		.flags		= IORESOURCE_IRQ,
	},
};

/*
 * The dmamask must be set for OHCI to work
 */
static u64 ohci_dmamask = ~(u32)0;
static struct platform_device ar7240_usb_ohci_device = {
	.name		= "ar7240-ohci",
	.id		= 0,
	.dev = {
		.dma_mask		= &ohci_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(ar7240_usb_ohci_resources),
	.resource	= ar7240_usb_ohci_resources,
};
#endif

/*
 * EHCI (USB full speed host controller)
 */
static struct resource ar7240_usb_ehci_resources[] = {
	[0] = {
		.start		= AR7240_USB_EHCI_BASE,
		.end		= AR7240_USB_EHCI_BASE + AR7240_USB_WINDOW - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= AR7240_CPU_IRQ_USB,
		.end		= AR7240_CPU_IRQ_USB,
		.flags		= IORESOURCE_IRQ,
	},
};

/*
 * The dmamask must be set for EHCI to work
 */
static u64 ehci_dmamask = ~(u32)0;

static struct platform_device ar7240_usb_ehci_device = {
	.name		= "ar7240-ehci",
	.id		= 0,
	.dev = {
		.dma_mask		= &ehci_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(ar7240_usb_ehci_resources),
	.resource	= ar7240_usb_ehci_resources,
};

static struct resource ar7240_uart_resources[] = {
	{
		.start	= AR7240_UART_BASE,
		.end	= AR7240_UART_BASE+0x0fff,
		.flags	= IORESOURCE_MEM,
	},
};

extern unsigned int ar7240_serial_in(int offset);
extern void ar7240_serial_out(int offset, int value);
unsigned int ar7240_plat_serial_in(struct uart_port *up, int offset)
{
	return ar7240_serial_in(offset);
}

void ar7240_plat_serial_out(struct uart_port *up, int offset, int value)
{
	ar7240_serial_out(offset, value);

}

static struct plat_serial8250_port ar7240_uart_data[] = {
	{
		.mapbase	= (u32)KSEG1ADDR(AR7240_UART_BASE),
		.membase	= (void __iomem *)((u32)(KSEG1ADDR(AR7240_UART_BASE))),
		.irq		= AR7240_MISC_IRQ_UART,
		.flags		= (UPF_BOOT_AUTOCONF | UPF_SKIP_TEST),
		.iotype		= UPIO_MEM32,
		.regshift	= 2,
		.uartclk	= 0,
	},
	{ },
};

static struct platform_device ar7240_uart = {
	.name			= "serial8250",
	.id			= 0,
	.dev.platform_data	= ar7240_uart_data,
	.num_resources		= 1,
	.resource		= ar7240_uart_resources
};

static struct platform_device *ar7241_platform_devices[] __initdata = {
//#ifdef CONFIG_USB_EHCI_AR9130
	&ar7240_usb_ehci_device,
//#endif
};

static struct platform_device *ar7240_platform_devices[] __initdata = {
#ifdef CONFIG_USB_OHCI_AR7240
	&ar7240_usb_ohci_device,
#endif
};

static struct platform_device *ar724x_platform_devices[] __initdata = {
	&ar7240_uart
};
extern void ar7240_serial_setup(void);

int ar7240_platform_init(void)
{
	int ret;
	/* need to set clock appropriately */
#ifdef CONFIG_WASP_SUPPORT
	ar7240_uart_data[0].uartclk = ath_ref_clk_freq;
#else
	ar7240_uart_data[0].uartclk = ar7240_ahb_freq;
#endif
#if 1
	ret = platform_add_devices(ar724x_platform_devices,
				ARRAY_SIZE(ar724x_platform_devices));
        printk("===== ar7240_platform_init: %d\n", ret);
	if (ret < 0)
		return ret;
#endif
	if (is_ar7241() || is_ar7242() || is_ar933x() || is_wasp()) {
		return (platform_add_devices(ar7241_platform_devices,
				ARRAY_SIZE(ar7241_platform_devices)));
	}
	if (is_ar7240()) {
		return (platform_add_devices(ar7240_platform_devices,
				ARRAY_SIZE(ar7240_platform_devices)));
	}

	return 0;
}

arch_initcall(ar7240_platform_init);
