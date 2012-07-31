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
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#ifndef CONFIG_ATH_HS_UART
#include <linux/serial_8250.h>
#endif

#include <atheros.h>

#ifdef CONFIG_ATHRS_HW_CSUM
#include <asm/checksum.h>
csum_hw_ops *csum_hw = NULL;
EXPORT_SYMBOL(csum_hw);
#endif

#define ATH_PCI_EP_BASE_OFF 0x18127000
void ath_sys_frequency(void);
void UartInit(void);

void ath_dispatch_wlan_intr(void)
{
#ifdef CONFIG_PCI
	if (unlikely(ath_reg_rd(ATH_PCIE_WMAC_INT_STATUS) & PCI_WMAC_INTR))
		do_IRQ(ATH_PCI_IRQ_DEV0);
	else
#endif
		do_IRQ(ATH_CPU_IRQ_WLAN);
}

unsigned int ath_slic_cntrl_rd(void)
{
	return ath_reg_rd(ATH_SLIC_CTRL);
}
void ath_slic_cntrl_wr(unsigned int val)
{
	ath_reg_wr(ATH_SLIC_CTRL, val);
}

void ath_spi_raw_output_u8(unsigned char val)
{
	int ii;
	unsigned int cs;

	cs = ath_reg_rd(ATH_SPI_WRITE) & ~(ATH_SPI_D0_HIGH |
						 ATH_SPI_CLK_HIGH);
	for (ii = 7; ii >= 0; ii--) {
		unsigned char jj = (val >> ii) & 1;
		ath_reg_wr_nf(ATH_SPI_WRITE, cs | jj);
		ath_reg_wr_nf(ATH_SPI_WRITE,
				 cs | jj | ATH_SPI_CLK_HIGH);
	}
}

unsigned int ath_spi_raw_input_u8(void)
{
	unsigned int i, j;

	for (i = 0; i < 8; i++)	// TRANSMIT DATA
	{

		ath_reg_wr(0xbf000008, 0x50100);	//CS1 = 0 , CLK=1
		for (j = 0; j < 15; j++) {;
		};

		ath_reg_wr(0xbf000008, 0x50000);	//CS1 = 0 , CLK=0
		for (j = 0; j < 15; j++) {;
		};
	}
	ath_reg_wr(0xbf000008, 0x70000);	//CS1 = 1 , CLK=0
	for (j = 0; j < 15; j++) {;
	};

	return ath_reg_rd(ATH_SPI_RD_STATUS) & 0xff;
}

void UartInit(void)
{
#ifdef CONFIG_ATH_HS_UART
	extern void ath_hs_uart_init(void);

	ath_hs_uart_init();
#else
	int freq, div;
	extern uint32_t serial_inited;

	ath_sys_frequency();

	freq = ath_uart_freq;

	div = freq / (ATH_CONSOLE_BAUD * 16);

	/* set DIAB bit */
	UART_WRITE(OFS_LINE_CONTROL, 0x80);

	/* set divisor */
	UART_WRITE(OFS_DIVISOR_LSB, (div & 0xff));
	UART_WRITE(OFS_DIVISOR_MSB, (div >> 8) & 0xff);

	// UART16550_WRITE(OFS_DIVISOR_LSB, 0x61);
	// UART16550_WRITE(OFS_DIVISOR_MSB, 0x03);

	/* clear DIAB bit */
	UART_WRITE(OFS_LINE_CONTROL, 0x00);

	/* set data format */
	UART_WRITE(OFS_DATA_FORMAT, 0x3);

	UART_WRITE(OFS_INTR_ENABLE, 0);

	serial_inited = 1;
#endif /* CONFIG_ATH_HS_UART */
}

void
ath_sys_frequency(void)
{
#if !defined(CONFIG_ATH_EMULATION)
	uint32_t pll, out_div, ref_div, nint, frac, clk_ctrl;
#endif
	uint32_t ref;

	if (ath_cpu_freq)
		return;

	if ((ath_reg_rd(ATH_BOOTSTRAP_REG) & ATH_REF_CLK_40)) {
		ref = (40 * 1000000);
	} else {
		ref = (25 * 1000000);
	}

	ath_uart_freq = ath_ref_freq = ref;

#ifdef CONFIG_ATH_EMULATION
	ath_cpu_freq = 80000000;
	ath_ddr_freq = 80000000;
	ath_ahb_freq = 40000000;
#else
	printk("%s: ", __func__);

	clk_ctrl = ath_reg_rd(ATH_DDR_CLK_CTRL);

	pll = ath_reg_rd(CPU_DPLL2_ADDRESS);
	if (CPU_DPLL2_LOCAL_PLL_GET(pll)) {
		out_div	= CPU_DPLL2_OUTDIV_GET(pll);

		pll = ath_reg_rd(CPU_DPLL_ADDRESS);
		nint = CPU_DPLL_NINT_GET(pll);
		frac = CPU_DPLL_NFRAC_GET(pll);
		ref_div = CPU_DPLL_REFDIV_GET(pll);
		pll = ref >> 18;
		frac	= frac * pll / ref_div;
		printk("cpu srif ");
	} else {
		pll = ath_reg_rd(ATH_PLL_CONFIG);
		out_div	= CPU_PLL_CONFIG_OUTDIV_GET(pll);
		ref_div	= CPU_PLL_CONFIG_REFDIV_GET(pll);
		nint	= CPU_PLL_CONFIG_NINT_GET(pll);
		frac	= CPU_PLL_CONFIG_NFRAC_GET(pll);
		pll = ref >> 6;
		frac	= frac * pll / ref_div;
		printk("cpu apb ");
	}
	ath_cpu_freq = (((nint * (ref / ref_div)) + frac) >> out_div) /
			(CPU_DDR_CLOCK_CONTROL_CPU_POST_DIV_GET(clk_ctrl) + 1);

	pll = ath_reg_rd(DDR_DPLL2_ADDRESS);
	if (DDR_DPLL2_LOCAL_PLL_GET(pll)) {
		out_div	= DDR_DPLL2_OUTDIV_GET(pll);

		pll = ath_reg_rd(DDR_DPLL_ADDRESS);
		nint = DDR_DPLL_NINT_GET(pll);
		frac = DDR_DPLL_NFRAC_GET(pll);
		ref_div = DDR_DPLL_REFDIV_GET(pll);
		pll = ref >> 18;
		frac	= frac * pll / ref_div;
		printk("ddr srif ");
	} else {
		pll = ath_reg_rd(ATH_DDR_PLL_CONFIG);
		out_div	= DDR_PLL_CONFIG_OUTDIV_GET(pll);
		ref_div	= DDR_PLL_CONFIG_REFDIV_GET(pll);
		nint	= DDR_PLL_CONFIG_NINT_GET(pll);
		frac	= DDR_PLL_CONFIG_NFRAC_GET(pll);
		pll = ref >> 10;
		frac	= frac * pll / ref_div;
		printk("ddr apb ");
	}
	ath_ddr_freq = (((nint * (ref / ref_div)) + frac) >> out_div) /
			(CPU_DDR_CLOCK_CONTROL_DDR_POST_DIV_GET(clk_ctrl) + 1);

	if (CPU_DDR_CLOCK_CONTROL_AHBCLK_FROM_DDRPLL_GET(clk_ctrl)) {
		ath_ahb_freq = ath_ddr_freq /
			(CPU_DDR_CLOCK_CONTROL_AHB_POST_DIV_GET(clk_ctrl) + 1);
	} else {
		ath_ahb_freq = ath_cpu_freq /
			(CPU_DDR_CLOCK_CONTROL_AHB_POST_DIV_GET(clk_ctrl) + 1);
	}
#endif
	printk("cpu %u ddr %u ahb %u\n", 
		ath_cpu_freq / 1000000,
		ath_ddr_freq / 1000000,
		ath_ahb_freq / 1000000);
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
 * (PCI EP controller)
 */
static struct resource ath_pci_ep_resources[] = {
	[0] = {
		.start	= ATH_PCI_EP_BASE_OFF,
		.end	= ATH_PCI_EP_BASE_OFF + 0xdff - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= ATH_CPU_IRQ_PCI_EP,
		.end	= ATH_CPU_IRQ_PCI_EP,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 pci_ep_dmamask = ~(u32)0;
static struct platform_device ath_pci_ep_device = {
	.name				= "ath-pciep",
	.id				= 0,
	.dev = {
		.dma_mask		= &pci_ep_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources			= ARRAY_SIZE(ath_pci_ep_resources),
	.resource			= ath_pci_ep_resources,
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

#ifndef CONFIG_ATH_HS_UART
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
#endif

static struct platform_device *ath_platform_devices[] __initdata = {
#ifndef CONFIG_ATH_HS_UART
	&ath_uart,
#endif
	&ath_usb_ehci_device,
	&ath_pci_ep_device
};

extern void ath_serial_setup(void);
extern void ath_set_wd_timer(uint32_t usec /* micro seconds */);
extern int ath_set_wd_timer_action(uint32_t val);

void
ath_aphang_timer_fn(void)
{
	static int times;
	if (times == 0) {
		ath_set_wd_timer_action(ATH_WD_ACT_NONE);
		ath_set_wd_timer(2 * USEC_PER_SEC);
		ath_set_wd_timer_action(ATH_WD_ACT_RESET);
		//printk(".");
	}
	times = (times + 1) % HZ;
}

void
ath_aphang_timer_init(void)
{
	ath_set_wd_timer_action(ATH_WD_ACT_NONE);
}

int ath_platform_init(void)
{
	int ret;

#ifndef CONFIG_ATH_HS_UART
	ath_uart_data[0].uartclk = ath_uart_freq;
#endif

	ret = platform_add_devices(ath_platform_devices,
				ARRAY_SIZE(ath_platform_devices));

	if (ret < 0) {
		printk("%s: failed %d\n", __func__, ret);
		return ret;
	}

	if (!is_ar934x_10()) {
		ath_aphang_timer_init();
	}

	return 0;
}

arch_initcall(ath_platform_init);
