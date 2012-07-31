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

void UartInit(void);
void UartPut(u8 byte);

void ath_dispatch_wlan_intr(void)
{
	do_IRQ(ATH_CPU_IRQ_WLAN);
}

static void ath_sys_frequency(void)
{
#ifdef CONFIG_ATH_EMULATION
#ifdef CONFIG_HORNET_EMULATION_WLAN_HARDI	/* FPGA WLAN emulation */
	ath_cpu_freq = 48000000;
	ath_ddr_freq = 48000000;
	ath_ahb_freq = 24000000;
#else
	ath_cpu_freq = 80000000;
	ath_ddr_freq = 80000000;
	ath_ahb_freq = 40000000;
#endif
#else
	/* Hornet's PLL is completely different from Python's */
	u32 ref_clock_rate, pll_freq;
	u32 pllreg, clockreg;
	u32 nint, refdiv, outdiv;
	u32 cpu_div, ahb_div, ddr_div;

	if (ath_reg_rd(HORNET_BOOTSTRAP_STATUS) &
	    HORNET_BOOTSTRAP_SEL_25M_40M_MASK)
		ref_clock_rate = 40 * 1000000;
	else
		ref_clock_rate = 25 * 1000000;

	pllreg = ath_reg_rd(ATH_CPU_PLL_CONFIG);
	clockreg = ath_reg_rd(ATH_CPU_CLOCK_CONTROL);

	if (clockreg & HORNET_CLOCK_CONTROL_BYPASS_MASK) {
		/* Bypass PLL */
		pll_freq = ref_clock_rate;
		cpu_div = ahb_div = ddr_div = 1;
	} else {
		nint =
		    (pllreg & HORNET_PLL_CONFIG_NINT_MASK) >>
		    HORNET_PLL_CONFIG_NINT_SHIFT;
		refdiv =
		    (pllreg & HORNET_PLL_CONFIG_REFDIV_MASK) >>
		    HORNET_PLL_CONFIG_REFDIV_SHIFT;
		outdiv =
		    (pllreg & HORNET_PLL_CONFIG_OUTDIV_MASK) >>
		    HORNET_PLL_CONFIG_OUTDIV_SHIFT;

		pll_freq = (ref_clock_rate / refdiv) * nint;

		if (outdiv == 1)
			pll_freq /= 2;
		else if (outdiv == 2)
			pll_freq /= 4;
		else if (outdiv == 3)
			pll_freq /= 8;
		else if (outdiv == 4)
			pll_freq /= 16;
		else if (outdiv == 5)
			pll_freq /= 32;
		else if (outdiv == 6)
			pll_freq /= 64;
		else if (outdiv == 7)
			pll_freq /= 128;
		else		/* outdiv == 0 --> illegal value */
			pll_freq /= 2;

		cpu_div =
		    (clockreg & HORNET_CLOCK_CONTROL_CPU_POST_DIV_MASK) >>
		    HORNET_CLOCK_CONTROL_CPU_POST_DIV_SHIFT;
		ddr_div =
		    (clockreg & HORNET_CLOCK_CONTROL_DDR_POST_DIV_MASK) >>
		    HORNET_CLOCK_CONTROL_DDR_POST_DIV_SFIFT;
		ahb_div =
		    (clockreg & HORNET_CLOCK_CONTROL_AHB_POST_DIV_MASK) >>
		    HORNET_CLOCK_CONTROL_AHB_POST_DIV_SFIFT;

		/*
		 * b00 : div by 1, b01 : div by 2, b10 : div by 3, b11 : div by 4
		 */
		cpu_div++;
		ddr_div++;
		ahb_div++;
	}

	ath_cpu_freq = pll_freq / cpu_div;
	ath_ddr_freq = pll_freq / ddr_div;
	ath_ahb_freq = pll_freq / ahb_div;
#endif
}


void UartInit(void)
{
	unsigned int rdata;
	unsigned int baudRateDivisor, clock_step;
	unsigned int fcEnable = 0;

	ath_sys_frequency();

	MY_WRITE(0xb8040000, 0xcff);
	MY_WRITE(0xb8040008, 0x3b);
	/* Enable UART , SPI and Disable S26 UART */
	MY_WRITE(0xb8040028, (ath_reg_rd(0xb8040028) | 0x48002));

	MY_WRITE(0xb8040008, 0x2f);

#ifdef CONFIG_HORNET_EMULATION
	baudRateDivisor = (ath_ahb_freq / (16 * ATH_CONSOLE_BAUD)) - 1;	// 24 MHz clock is taken as UART clock
#else

	rdata = ath_reg_rd(HORNET_BOOTSTRAP_STATUS);
	rdata &= HORNET_BOOTSTRAP_SEL_25M_40M_MASK;

	if (rdata)
		baudRateDivisor = (40000000 / (16 * ATH_CONSOLE_BAUD)) - 1;	// 40 MHz clock is taken as UART clock
	else
		baudRateDivisor = (25000000 / (16 * ATH_CONSOLE_BAUD)) - 1;	// 25 MHz clock is taken as UART clock
#endif

	clock_step = 8192;

	rdata =
	    UARTCLOCK_UARTCLOCKSCALE_SET(baudRateDivisor) |
	    UARTCLOCK_UARTCLOCKSTEP_SET(clock_step);
	uart_reg_write(UARTCLOCK_ADDRESS, rdata);

	/* Config Uart Controller */
#if 1				/* No interrupt */
	rdata =
	    UARTCS_UARTDMAEN_SET(0) | UARTCS_UARTHOSTINTEN_SET(0) |
	    UARTCS_UARTHOSTINT_SET(0)
	    | UARTCS_UARTSERIATXREADY_SET(0) |
	    UARTCS_UARTTXREADYORIDE_SET(~fcEnable)
	    | UARTCS_UARTRXREADYORIDE_SET(~fcEnable) |
	    UARTCS_UARTHOSTINTEN_SET(0);
#else
	rdata =
	    UARTCS_UARTDMAEN_SET(0) | UARTCS_UARTHOSTINTEN_SET(0) |
	    UARTCS_UARTHOSTINT_SET(0)
	    | UARTCS_UARTSERIATXREADY_SET(0) |
	    UARTCS_UARTTXREADYORIDE_SET(~fcEnable)
	    | UARTCS_UARTRXREADYORIDE_SET(~fcEnable) |
	    UARTCS_UARTHOSTINTEN_SET(1);
#endif

	/* is_dte == 1 */
	rdata = rdata | UARTCS_UARTINTERFACEMODE_SET(2);

	if (fcEnable) {
		rdata = rdata | UARTCS_UARTFLOWCONTROLMODE_SET(2);
	}

	/* invert_fc ==0 (Inverted Flow Control) */
	//rdata = rdata | UARTCS_UARTFLOWCONTROLMODE_SET(3);

	/* parityEnable == 0 */
	//rdata = rdata | UARTCS_UARTPARITYMODE_SET(2); -->Parity Odd
	//rdata = rdata | UARTCS_UARTPARITYMODE_SET(3); -->Parity Even
	uart_reg_write(UARTCS_ADDRESS, rdata);

	serial_inited = 1;
}

u8 UartGetPoll(void)
{
	u8 ret_val;
	unsigned int rdata;

	do {
		rdata = uart_reg_read(UARTDATA_ADDRESS);
	} while (!UARTDATA_UARTRXCSR_GET(rdata));

	ret_val = (u8) UARTDATA_UARTTXRXDATA_GET(rdata);
	rdata = UARTDATA_UARTRXCSR_SET(1);
	uart_reg_write(UARTDATA_ADDRESS, rdata);

	return ret_val;
}

void UartPut(u8 byte)
{
	unsigned int rdata;

	if (!serial_inited) {
		serial_inited = 1;
		UartInit();
	}

	do {
		rdata = uart_reg_read(UARTDATA_ADDRESS);
	} while (UARTDATA_UARTTXCSR_GET(rdata) == 0);

	rdata = UARTDATA_UARTTXRXDATA_SET((unsigned int)byte);
	rdata |= UARTDATA_UARTTXCSR_SET(1);

	uart_reg_write(UARTDATA_ADDRESS, rdata);
}

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

extern void ath_serial_setup(void);

int ath_platform_init(void)
{
	int ret;

	ath_uart_data[0].uartclk = ath_uart_freq;

	ret = platform_add_devices(ath_platform_devices,
				ARRAY_SIZE(ath_platform_devices));

	if (ret < 0) {
		printk("%s: failed %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

arch_initcall(ath_platform_init);
