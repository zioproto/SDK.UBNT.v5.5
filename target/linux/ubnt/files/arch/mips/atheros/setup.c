//#include <linux/config.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <linux/serial_8250.h>
#include <linux/console.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/pci.h>

#include <asm/reboot.h>
#include <asm/io.h>
#include <asm/time.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/reboot.h>
#include <asm/system.h>
#include <asm/serial.h>
#include <asm/traps.h>

#include <atheros.h>

#if 0				/* For WLAN debug */
u_int32_t wasp_loop = 0;
EXPORT_SYMBOL(wasp_loop);

u_int32_t g_rxbuf_cnt = 0;
u_int32_t g_rxadded = 0;
u_int32_t g_rxdepth = 0;
u_int32_t g_rxtail = 0;
u_int32_t g_rxhead = 0;
u_int32_t g_rxbuf[512];
u_int32_t g_rxdp[512];
u_int32_t g_rxfifodepth[512];
EXPORT_SYMBOL(g_rxbuf_cnt);
EXPORT_SYMBOL(g_rxadded);
EXPORT_SYMBOL(g_rxdepth);
EXPORT_SYMBOL(g_rxtail);
EXPORT_SYMBOL(g_rxhead);
EXPORT_SYMBOL(g_rxbuf);
EXPORT_SYMBOL(g_rxdp);
EXPORT_SYMBOL(g_rxfifodepth);
#endif

uint32_t ath_cpu_freq, ath_ahb_freq, ath_ddr_freq,
	ath_ref_freq, ath_uart_freq;
uint32_t serial_inited;

static int __init ath_init_ioc(void);
void serial_print(const char *fmt, ...);
void writeserial(char *str, int count);
void ath_sys_frequency(void);

void UartInit(void);

/*
 * Export AHB freq value to be used by Ethernet MDIO.
 */
EXPORT_SYMBOL(ath_ahb_freq);

/*
 * Export Ref freq value to be used by I2S module.
 */
EXPORT_SYMBOL(ath_ref_freq);

void ath_restart(char *command)
{
	for (;;) {
		if (is_ar934x_10()) {
                	ath_reg_wr(ATH_GPIO_OE, ath_reg_rd(ATH_GPIO_OE) & (~(1 << 17)));
		} else {
			ath_reg_wr(ATH_RESET, ATH_RESET_FULL_CHIP);
		}
	}
}

void ath_halt(void)
{
	printk(KERN_NOTICE "\n** You can safely turn off the power\n");
	while (1) ;
}

void ath_power_off(void)
{
	ath_halt();
}

const char
*get_system_type(void)
{
#ifdef CONFIG_ATH_EMULATION
#	define	ath_sys_type(x)	x " emu"
#else
#	define	ath_sys_type(x)	x
#endif
	return ath_sys_type(CONFIG_ATH_SYS_TYPE);
}
EXPORT_SYMBOL(get_system_type);

int
valid_wmac_num(u_int16_t wmac_num)
{
	return (wmac_num == 0);
}

/*
 * HOWL has only one wmac device, hence the following routines
 * ignore the wmac_num parameter
 */
int
get_wmac_irq(u_int16_t wmac_num)
{
	return ATH_CPU_IRQ_WLAN;
}

unsigned long
get_wmac_base(u_int16_t wmac_num)
{
	return KSEG1ADDR(ATH_WMAC_BASE);
}

unsigned long
get_wmac_mem_len(u_int16_t wmac_num)
{
	return ATH_WMAC_LEN;
}

EXPORT_SYMBOL(valid_wmac_num);
EXPORT_SYMBOL(get_wmac_irq);
EXPORT_SYMBOL(get_wmac_base);
EXPORT_SYMBOL(get_wmac_mem_len);

#ifndef CONFIG_ATH_HS_UART
void __init ath_serial_setup(void)
{
	struct uart_port p;

	memset(&p, 0, sizeof(p));

	p.flags = (UPF_BOOT_AUTOCONF | UPF_SKIP_TEST);
	p.iotype = UPIO_MEM32;
	p.uartclk = ath_ahb_freq;
	p.irq = ATH_MISC_IRQ_UART;
	p.regshift = 2;
	p.mapbase = (u32) KSEG1ADDR(ATH_UART_BASE);
	p.membase = (void __iomem *)p.mapbase;

	if (early_serial_setup(&p) != 0)
		printk(KERN_ERR "early_serial_setup failed\n");

	serial_print("%s: early_serial_setup done..\n", __func__);

}
#endif

unsigned int __cpuinit get_c0_compare_int(void)
{
	//printk("%s: returning timer irq : %d\n",__func__, ATH_CPU_IRQ_TIMER);
	return ATH_CPU_IRQ_TIMER;
}

void __init plat_time_init(void)
{
	mips_hpt_frequency = ath_cpu_freq / 2;
	printk("%s: plat time init done\n", __func__);
}

int ath_be_handler(struct pt_regs *regs, int is_fixup)
{
#if 0
	if (!is_fixup && (regs->cp0_cause & 4)) {
		/* Data bus error - print PA */
		printk("DBE physical address: %010Lx\n",
		       __read_64bit_c0_register($26, 1));
	}
#endif
#ifdef CONFIG_PCI
	int error = 0, status, trouble = 0;
	error = ath_reg_rd(ATH_PCI_ERROR) & 3;

	if (error) {
		printk("PCI error %d at PCI addr 0x%x\n",
		       error, ath_reg_rd(ATH_PCI_ERROR_ADDRESS));
		ath_reg_wr(ATH_PCI_ERROR, error);
#if !defined(CONFIG_PERICOM)
		ath_local_read_config(PCI_STATUS, 2, &status);
		printk("PCI status: %#x\n", status);
#endif
		trouble = 1;
	}

	error = 0;
	error = ath_reg_rd(ATH_PCI_AHB_ERROR) & 1;

	if (error) {
		printk("AHB error at AHB address 0x%x\n",
		       ath_reg_rd(ATH_PCI_AHB_ERROR_ADDRESS));
		ath_reg_wr(ATH_PCI_AHB_ERROR, error);
#if !defined(CONFIG_PERICOM)
		ath_local_read_config(PCI_STATUS, 2, &status);
		printk("PCI status: %#x\n", status);
#endif
		trouble = 1;
	}
#endif

#ifdef CONFIG_MACH_AR934x
	printk("ath data bus error: cause %#x\nrebooting...", read_c0_cause());
	ath_restart(NULL);
#else
	printk("ath data bus error: cause %#x\n", read_c0_cause());
#endif
	return (is_fixup ? MIPS_BE_FIXUP : MIPS_BE_FATAL);
}

void __init plat_mem_setup(void)
{

#if 1
	board_be_handler = ath_be_handler;
#endif
	_machine_restart = ath_restart;
	_machine_halt = ath_halt;
	pm_power_off = ath_power_off;

	/*
	 ** early_serial_setup seems to conflict with serial8250_register_port()
	 ** In order for console to work, we need to call register_console().
	 ** We can call serial8250_register_port() directly or use
	 ** platform_add_devices() function which eventually calls the
	 ** register_console(). AP71 takes this approach too. Only drawback
	 ** is if system screws up before we register console, we won't see
	 ** any msgs on the console.  System being stable now this should be
	 ** a special case anyways. Just initialize Uart here.
	 */

	UartInit();

#ifdef CONFIG_MACH_AR933x
	/* clear wmac reset */
	ath_reg_wr(ATH_RESET,
		      (ath_reg_rd(ATH_RESET) & (~ATH_RESET_WMAC)));
#endif
	serial_print("Booting %s\n", get_system_type());
}

/*
 * -------------------------------------------------
 * Early printk hack
 */
u8 UartGetPoll(void) __attribute__ ((weak));
void UartPut(u8 byte) __attribute__ ((weak));

u8 UartGetPoll()
{
#ifdef CONFIG_ATH_HS_UART
	char ch;
	u_int32_t rx_data;

	do {
		rx_data = ath_reg_rd(0xB8500000);	// UART DATA Reg
	} while ((rx_data & 0x100) != 0x100);
	ch = rx_data & 0xff;
	ath_reg_wr(0xB8500000, 0x100);

	return ch;
#else
	while ((UART_READ(OFS_LINE_STATUS) & 0x1) == 0) ;
	return UART_READ(OFS_RCV_BUFFER);
#endif
}

void UartPut(u8 byte)
{
#ifdef CONFIG_ATH_HS_UART
	u_int32_t tx_data;
#endif
	if (!serial_inited) {
		serial_inited = 1;
		UartInit();
	}

#ifdef CONFIG_ATH_HS_UART
	do {
		tx_data = ath_reg_rd(0xB8500000);	// UART DATA Reg
	} while ((tx_data & 0x200) != 0x200);

	tx_data = byte | 0x200;
	ath_reg_wr(0xB8500000, tx_data);
	//tx_data = ath_reg_rd(0xB8500000);

#else
	while (((UART_READ(OFS_LINE_STATUS)) & 0x20) == 0x0) ;
	UART_WRITE(OFS_SEND_BUFFER, byte);
#endif
}

extern int vsprintf(char *buf, const char *fmt, va_list args);
static char sprint_buf[1024];

void serial_print(const char *fmt, ...)
{
	va_list args;
	int n;

	va_start(args, fmt);
	n = vsprintf(sprint_buf, fmt, args);
	va_end(args);
	writeserial(sprint_buf, n);
}

void writeserial(char *str, int count)
{
	int i;
	for (i = 0; i <= count; i++)
		UartPut(str[i]);

	UartPut('\r');
	memset(str, '\0', 1024);
	return;
}

unsigned int ath_serial_in(int offset)
{
	return UART_READ(offset);
}

void ath_serial_out(int offset, int value)
{
	UART_WRITE(offset, (u8) value);
}

#include <asm/uaccess.h>
#define M_PERFCTL_EVENT(event)          ((event) << 5)
unsigned int clocks_at_start;

void start_cntrs(unsigned int event0, unsigned int event1)
{
	write_c0_perfcntr0(0x00000000);
	write_c0_perfcntr1(0x00000000);
	/*
	 * go...
	 */
	write_c0_perfctrl0(0x80000000 | M_PERFCTL_EVENT(event0) | 0xf);
	write_c0_perfctrl1(0x00000000 | M_PERFCTL_EVENT(event1) | 0xf);
}

void stop_cntrs(void)
{
	write_c0_perfctrl0(0);
	write_c0_perfctrl1(0);
}

void read_cntrs(unsigned int *c0, unsigned int *c1)
{
	*c0 = read_c0_perfcntr0();
	*c1 = read_c0_perfcntr1();
}

static int ath_ioc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t
ath_ioc_read(struct file *file, char *buf, size_t count, loff_t * ppos)
{

	unsigned int c0, c1, ticks = (read_c0_count() - clocks_at_start);
	char str[256];
	unsigned int secs = ticks / mips_hpt_frequency;

	read_cntrs(&c0, &c1);
	stop_cntrs();
	sprintf(str, "%d secs (%#x) event0:%#x event1:%#x", secs, ticks, c0,
		c1);
	copy_to_user(buf, str, strlen(str));

	return (strlen(str));
}

#if 0
static void ath_dcache_test(void)
{
	int i, j;
	unsigned char p;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < (10 * 1024); j++) {
			p = *((unsigned char *)0x81000000 + j);
		}
	}
}
#endif

static ssize_t
ath_ioc_write(struct file *file, const char *buf, size_t count,
		 loff_t * ppos)
{
	int event0, event1;

	sscanf(buf, "%d:%d", &event0, &event1);
	printk("\nevent0 %d event1 %d\n", event0, event1);

	clocks_at_start = read_c0_count();
	start_cntrs(event0, event1);

	return (count);
}

struct file_operations ath_ioc_fops = {
      open:ath_ioc_open,
      read:ath_ioc_read,
      write:ath_ioc_write,
};

/*
 * General purpose ioctl i/f
 */
static int __init ath_init_ioc()
{
	static int _mymajor;

	_mymajor = register_chrdev(77, "ATH_GPIOC", &ath_ioc_fops);

	if (_mymajor < 0) {
		printk("Failed to register GPIOC\n");
		return _mymajor;
	}

	printk("ATH GPIOC major %d\n", _mymajor);
	return 0;
}

device_initcall(ath_init_ioc);
