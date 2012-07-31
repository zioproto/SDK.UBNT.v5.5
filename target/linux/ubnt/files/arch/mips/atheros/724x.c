#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/byteorder.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/bitops.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <atheros.h>

void ath_dispatch_wlan_intr(void)
{
	do_IRQ(ATH_PCI_IRQ_DEV0);
}

void UartInit()
{
	int freq, div;

	ath_sys_frequency();

	freq = ath_uart_freq;

	MY_WRITE(0xb8040000, 0xcff);

	MY_WRITE(0xb8040008, 0x3b);

	/* Enable UART , SPI and Disable S26 UART */
	MY_WRITE(0xb8040028, (ath_reg_rd(0xb8040028) | 0x48002));

	MY_WRITE(0xb8040008, 0x2f);

	div = freq / (ATH_CONSOLE_BAUD * 16);

	/* set DIAB bit */
	UART16550_WRITE(OFS_LINE_CONTROL, 0x80);

	/* set divisor */
	UART16550_WRITE(OFS_DIVISOR_LSB, (div & 0xff));
	UART16550_WRITE(OFS_DIVISOR_MSB, (div >> 8) & 0xff);

	// UART16550_WRITE(OFS_DIVISOR_LSB, 0x61);
	// UART16550_WRITE(OFS_DIVISOR_MSB, 0x03);

	/* clear DIAB bit */
	UART16550_WRITE(OFS_LINE_CONTROL, 0x00);

	/* set data format */
	UART16550_WRITE(OFS_DATA_FORMAT, 0x3);

	UART16550_WRITE(OFS_INTR_ENABLE, 0);

	serial_inited = 1;
}

void ath_sys_frequency(void)
{
#ifdef CONFIG_ATH_EMULATION
	ath_cpu_freq = 80000000;
	ath_ddr_freq = 80000000;
	ath_ahb_freq = 40000000;
#else
	uint32_t pll, pll_div, ahb_div, ddr_div, freq, ref_div;

	if (ath_cpu_freq)
		return;

	pll = ath_reg_rd(ATH_PLL_CONFIG);

	pll_div = ((pll >> PLL_DIV_SHIFT) & PLL_DIV_MASK);
	ref_div = (pll >> REF_DIV_SHIFT) & REF_DIV_MASK;
	ddr_div = ((pll >> DDR_DIV_SHIFT) & DDR_DIV_MASK) + 1;
	ahb_div = (((pll >> AHB_DIV_SHIFT) & AHB_DIV_MASK) + 1) * 2;

	freq = pll_div * ref_div * 5000000;

	ath_cpu_freq = freq;
	ath_ddr_freq = freq / ddr_div;
	ath_ahb_freq = ath_cpu_freq / ahb_div;

	ath_uart_freq = ath_ahb_freq;
#endif
}

/*
 * OHCI (USB full speed host controller)
 */
static struct resource ath_usb_ohci_resources[] = {
	[0] = {
		.start = ATH_USB_OHCI_BASE,
		.end = ATH_USB_OHCI_BASE + ATH_USB_WINDOW - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = ATH_CPU_IRQ_USB,
		.end = ATH_CPU_IRQ_USB,
		.flags = IORESOURCE_IRQ,
	},
};

/*
 * The dmamask must be set for OHCI to work
 */
static u64 ohci_dmamask = ~(u32) 0;
static struct platform_device ath_usb_ohci_device = {
	.name = "ath-ohci",
	.id = 0,
	.dev = {
		.dma_mask = &ohci_dmamask,
		.coherent_dma_mask = 0xffffffff,
		},
	.num_resources = ARRAY_SIZE(ath_usb_ohci_resources),
	.resource = ath_usb_ohci_resources,
};

/*
 * EHCI (USB full speed host controller)
 */
static struct resource ath_usb_ehci_resources[] = {
	[0] = {
		.start = ATH_USB_EHCI_BASE,
		.end = ATH_USB_EHCI_BASE + ATH_USB_WINDOW - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = ATH_CPU_IRQ_USB,
		.end = ATH_CPU_IRQ_USB,
		.flags = IORESOURCE_IRQ,
	},
};

/*
 * The dmamask must be set for EHCI to work
 */
static u64 ehci_dmamask = ~(u32) 0;

static struct platform_device ath_usb_ehci_device = {
	.name = "ath-ehci",
	.id = 0,
	.dev = {
		.dma_mask = &ehci_dmamask,
		.coherent_dma_mask = 0xffffffff,
		},
	.num_resources = ARRAY_SIZE(ath_usb_ehci_resources),
	.resource = ath_usb_ehci_resources,
};

static struct resource ath_uart_resources[] = {
	{
	 .start = ATH_UART_BASE,
	 .end = ATH_UART_BASE + 0x0fff,
	 .flags = IORESOURCE_MEM,
	 },
};

extern unsigned int ath_serial_in(int offset);
extern void ath_serial_out(int offset, int value);
unsigned int ath_plat_serial_in(struct uart_port *up, int offset)
{
	return ath_serial_in(offset);
}

void ath_plat_serial_out(struct uart_port *up, int offset, int value)
{
	ath_serial_out(offset, value);

}

static struct plat_serial8250_port ath_uart_data[] = {
	{
	 .mapbase = (u32) KSEG1ADDR(ATH_UART_BASE),
	 .membase = (void __iomem *)((u32) (KSEG1ADDR(ATH_UART_BASE))),
	 .irq = ATH_MISC_IRQ_UART,
	 .flags = (UPF_BOOT_AUTOCONF | UPF_SKIP_TEST),
	 .iotype = UPIO_MEM32,
	 .regshift = 2,
	 .uartclk = 0,		/* ath_ahb_freq, */
	 },
	{},
};

static struct platform_device ath_uart = {
	.name = "serial8250",
	.id = 0,
	.dev.platform_data = ath_uart_data,
	.num_resources = 1,
	.resource = ath_uart_resources
};

static struct platform_device *ar7241_platform_devices[] __initdata = {
	&ath_uart,
	&ath_usb_ehci_device
};

static struct platform_device *ar7240_platform_devices[] __initdata = {
	&ath_uart,
#ifdef CONFIG_USB_OHCI_ATH
	&ath_usb_ohci_device
#endif
};

extern void ath_serial_setup(void);

int ath_platform_init(void)
{
	int ret;

	ath_uart_data[0].uartclk = ath_ahb_freq;

	if (is_ar7240()) {
		ret = platform_add_devices(ar7240_platform_devices,
					ARRAY_SIZE(ar7240_platform_devices));
	} else {
		// 7241 and 7242
		ret = platform_add_devices(ar7241_platform_devices,
					ARRAY_SIZE(ar7241_platform_devices));
	}

	if (ret < 0) {
		printk("ath_platform_init: failed %d\n", ret);
		return ret;
	}

	return 0;
}

arch_initcall(ath_platform_init);
