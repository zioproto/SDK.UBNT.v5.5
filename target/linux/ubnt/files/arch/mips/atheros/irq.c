/*
 * General Interrupt handling for ATH soc
 */
//#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/kallsyms.h>

#include <asm/irq.h>
#include <asm/mipsregs.h>
//#include <asm/gdb-stub.h>

#include <atheros.h>
#include <asm/irq_cpu.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <linux/swap.h>
#include <linux/proc_fs.h>
#include <linux/pfn.h>
#include <linux/threads.h>
#include <asm/asm-offsets.h>

/*
 * dummy irqaction, so that interrupt controller cascading can work. Basically
 * when one IC is connected to another, this will be used to enable to Parent
 * IC's irq line to which the child IC is connected
 */
static struct irqaction cascade = {
	.handler = no_action,
	.name = "cascade",
};

static void ath_dispatch_misc_intr(void);
void ath_dispatch_wlan_intr(void);
static void ath_dispatch_gpio_intr(void);
static void ath_misc_irq_init(int irq_base);
extern pgd_t swapper_pg_dir[_PTRS_PER_PGD];
extern unsigned long pgd_current[NR_CPUS];

void __init arch_init_irq(void)
{
	/*
	 * initialize our interrupt controllers
	 */
	mips_cpu_irq_init();
	ath_misc_irq_init(ATH_MISC_IRQ_BASE);
	ath_gpio_irq_init(ATH_GPIO_IRQ_BASE);

#ifdef CONFIG_PCI
	ath_pci_irq_init(ATH_PCI_IRQ_BASE);
#endif

	/*
	 * enable cascades
	 */
	setup_irq(ATH_CPU_IRQ_MISC, &cascade);
	setup_irq(ATH_MISC_IRQ_GPIO, &cascade);

#ifdef CONFIG_PCI
	setup_irq(ATH_CPU_IRQ_PCI, &cascade);
#endif

	ath_arch_init_irq();

	set_c0_status(ST0_IM);
}

static void ath_misc_irq_enable(unsigned int);

static void ath_dispatch_misc_intr()
{
	int pending;

	pending = ath_reg_rd(ATH_MISC_INT_STATUS) &
	    ath_reg_rd(ATH_MISC_INT_MASK);

#ifndef CONFIG_ATH_HS_UART
	if (pending & MIMR_UART) {
		do_IRQ(ATH_MISC_IRQ_UART);
		ath_reg_rmw_clear(ATH_MISC_INT_STATUS, MIMR_UART);
	} else
#endif
	if (pending & MIMR_DMA) {
		do_IRQ(ATH_MISC_IRQ_DMA);
		ath_reg_rmw_clear(ATH_MISC_INT_STATUS, MIMR_DMA);
	} else if (pending & MIMR_PERF_COUNTER) {
		do_IRQ(ATH_MISC_IRQ_PERF_COUNTER);
		ath_reg_rmw_clear(ATH_MISC_INT_STATUS, MIMR_PERF_COUNTER);
	} else if (pending & MIMR_TIMER) {
		do_IRQ(ATH_MISC_IRQ_TIMER);
		ath_reg_rmw_clear(ATH_MISC_INT_STATUS, MIMR_TIMER);
#ifdef CONFIG_ATH_HS_UART
	} else if (pending & MIMR_HS_UART) {
		do_IRQ(ATH_MISC_IRQ_HS_UART);
		ath_reg_rmw_clear(ATH_MISC_INT_STATUS, MIMR_HS_UART);
#else
	} else if (pending & MIMR_OHCI_USB) {
		do_IRQ(ATH_MISC_IRQ_USB_OHCI);
		ath_reg_rmw_clear(ATH_MISC_INT_STATUS, MIMR_OHCI_USB);
#endif
	} else if (pending & MIMR_ERROR) {
		do_IRQ(ATH_MISC_IRQ_ERROR);
		ath_reg_rmw_clear(ATH_MISC_INT_STATUS, MIMR_ERROR);
	} else if (pending & MIMR_GPIO) {
		ath_dispatch_gpio_intr();
		ath_reg_rmw_clear(ATH_MISC_INT_STATUS, MIMR_GPIO);
	} else if (pending & MIMR_WATCHDOG) {
		do_IRQ(ATH_MISC_IRQ_WATCHDOG);
		ath_reg_rmw_clear(ATH_MISC_INT_STATUS, MIMR_WATCHDOG);
	} else if (pending & MIMR_ENET_LINK) {
		do_IRQ(ATH_MISC_IRQ_ENET_LINK);
		ath_reg_rmw_clear(ATH_MISC_INT_STATUS, MIMR_ENET_LINK);
	} else if (pending & MIMR_NAT_AGER) {
		do_IRQ(ATH_MISC_IRQ_NAT_AGER);
		ath_reg_rmw_clear(ATH_MISC_INT_STATUS, MIMR_NAT_AGER);
	}

}

static void ath_dispatch_gpio_intr(void)
{
	int pending, i;

	pending = ath_reg_rd(ATH_GPIO_INT_PENDING) &
	    ath_reg_rd(ATH_GPIO_INT_MASK);

	for (i = 0; i < ATH_GPIO_IRQ_COUNT; i++) {
		if (pending & (1 << i))
			do_IRQ(ATH_GPIO_IRQn(i));
	}
}

/* Will be defined in chip specific file, if needed */
void ath_aphang_timer_fn(void) __attribute__ ((weak));
void ath_aphang_timer_fn(void) { }

/*
 * Dispatch interrupts.
 * XXX: This currently does not prioritize except in calling order. Eventually
 * there should perhaps be a static map which defines, the IPs to be masked for
 * a given IP.
 */
asmlinkage void plat_irq_dispatch(void)
{
	int pending = read_c0_status() & read_c0_cause();
#if 0
	if (!(pending & CAUSEF_IP7))
		printk("%s: in irq dispatch \n", __func__);
#endif
	if (pending & CAUSEF_IP7) {
		do_IRQ(ATH_CPU_IRQ_TIMER);
		ath_aphang_timer_fn();
	}
	else if (pending & CAUSEF_IP2)
		ath_dispatch_wlan_intr();

	else if (pending & CAUSEF_IP4)
		do_IRQ(ATH_CPU_IRQ_GE0);

	else if (pending & CAUSEF_IP5)
		do_IRQ(ATH_CPU_IRQ_GE1);

	else if (pending & CAUSEF_IP3)
		do_IRQ(ATH_CPU_IRQ_USB);

	else if (pending & CAUSEF_IP6)
		ath_dispatch_misc_intr();

	/*
	 * Some PCI devices are write to clear. These writes are posted and might
	 * require a flush (r8169.c e.g.). Its unclear what will have more
	 * performance impact - flush after every interrupt or taking a few
	 * "spurious" interrupts. For now, its the latter.
	 */
	/*else
	   printk("spurious IRQ pending: 0x%x\n", pending); */
}

#if 1
#define vpk(...)
#define vps(...)
#else
#define vpk     printk
#define vps     print_symbol
#endif

static void ath_misc_irq_enable(unsigned int irq)
{
#if 0
	vpk("%s: %u ", __func__, irq);
	vps("%s\n", __builtin_return_address(0));
#endif
	ath_reg_rmw_set(ATH_MISC_INT_MASK,
			   (1 << (irq - ATH_MISC_IRQ_BASE)));
}

static void ath_misc_irq_disable(unsigned int irq)
{
#if 0
	vpk("%s: %u ", __func__, irq);
	vps("%s\n", __builtin_return_address(0));
#endif
	ath_reg_rmw_clear(ATH_MISC_INT_MASK,
			     (1 << (irq - ATH_MISC_IRQ_BASE)));
}
static unsigned int ath_misc_irq_startup(unsigned int irq)
{
#if 0
	vpk("%s: %u ", __func__, irq);
	vps("%s\n", __builtin_return_address(0));
#endif
	ath_misc_irq_enable(irq);
	return 0;
}

static void ath_misc_irq_shutdown(unsigned int irq)
{
#if 0
	vpk("%s: %u ", __func__, irq);
	vps("%s\n", __builtin_return_address(0));
#endif
	ath_misc_irq_disable(irq);
}

static void ath_misc_irq_ack(unsigned int irq)
{
#if 0
	vpk("%s: %u ", __func__, irq);
	vps("%s\n", __builtin_return_address(0));
#endif
	ath_misc_irq_disable(irq);
}

static void ath_misc_irq_end(unsigned int irq)
{
#if 0
	vpk("%s: %u ", __func__, irq);
	vps("%s\n", __builtin_return_address(0));
#endif
	if (!(irq_desc[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS)))
		ath_misc_irq_enable(irq);
}

static int
ath_misc_irq_set_affinity(unsigned int irq, const struct cpumask *dest)
{
	/*
	 * Only 1 CPU; ignore affinity request
	 */
	return 0;
}

struct irq_chip ath_misc_irq_controller = {
	.name = "ATH MISC",
	.startup = ath_misc_irq_startup,
	.shutdown = ath_misc_irq_shutdown,
	.enable = ath_misc_irq_enable,
	.disable = ath_misc_irq_disable,
	.ack = ath_misc_irq_ack,
	.end = ath_misc_irq_end,
	.eoi = ath_misc_irq_end,
	.set_affinity = ath_misc_irq_set_affinity,
};

/*
 * Determine interrupt source among interrupts that use IP6
 */
static void ath_misc_irq_init(int irq_base)
{
	int i;

	for (i = irq_base; i < irq_base + ATH_MISC_IRQ_COUNT; i++) {
		irq_desc[i].status = IRQ_DISABLED;
		irq_desc[i].action = NULL;
		irq_desc[i].depth = 1;
		set_irq_chip_and_handler(i, &ath_misc_irq_controller,
					 handle_percpu_irq);
	}
}
