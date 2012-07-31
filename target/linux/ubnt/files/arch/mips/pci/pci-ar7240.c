#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/irq.h>

#include <asm/delay.h>
#include <asm/irq.h>

#define ag7100_delay1s()    mdelay(1000);

#include "ar7240.h"
/*
 * Support for Ar7100 pci interrupt and core pci initialization
 */
/*
 * PCI interrupts.
 * roughly the interrupts flow is:
 *
 * - save flags
 * - CLI (disable all)
 * - IC->ack (mask out the source)
 * - EI (enable all, except the source that was masked of course)
 * - action (ISR)
 * - IC->enable (unmask the source)
 *
 * The reason we have a separate PCI IC is beacause of the following:
 * If we dont, then Throughout the "action" of a PCI slot, the
 * entire PCI "IP" on the cpu will remain disabled. Which means that we cant
 * prioritize between PCI interrupts. Normally this should be ok, if all PCI 
 * interrupts are considered equal. However, creating a PCI IC gives 
 * the flexibility to prioritize.
 */

static void
ar7240_pci_irq_enable(unsigned int irq)
{
#ifdef CONFIG_PERICOM
	/* Copied from NetBSD */
	if (irq == AR7240_PCI_IRQ_DEV0) {
		ar7240_reg_rmw_set(AR7240_PCI_INT_MASK, AR7240_PCI_INT_B_L);
	} else {
		ar7240_reg_rmw_set(AR7240_PCI_INT_MASK, AR7240_PCI_INT_C_L);
	}
#else
	ar7240_reg_rmw_set(AR7240_PCI_INT_MASK, AR7240_PCI_INT_A_L);
#endif /* CONFIG_PERICOM */
}

static void
ar7240_pci_irq_disable(unsigned int irq)
{
#ifdef CONFIG_PERICOM
	/* Copied from NetBSD */
	if (irq == AR7240_PCI_IRQ_DEV0) {
		ar7240_reg_rmw_clear(AR7240_PCI_INT_MASK, AR7240_PCI_INT_B_L);
	} else if (irq == AR7240_PCI_IRQ_DEV1) {
		ar7240_reg_rmw_clear(AR7240_PCI_INT_MASK, AR7240_PCI_INT_C_L);
	}
#else
	ar7240_reg_rmw_clear(AR7240_PCI_INT_MASK, AR7240_PCI_INT_A_L);
	ar7240_reg_rmw_clear(AR7240_PCI_INT_STATUS, AR7240_PCI_INT_A_L);
#endif /* CONFIG_PERICOM */
}

static unsigned int
ar7240_pci_irq_startup(unsigned int irq)
{
	ar7240_pci_irq_enable(irq);
	return 0;
}

static void
ar7240_pci_irq_shutdown(unsigned int irq)
{
	ar7240_pci_irq_disable(irq);
}

static void
ar7240_pci_irq_ack(unsigned int irq)
{
	ar7240_pci_irq_disable(irq);
}

static void
ar7240_pci_irq_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS)))
		ar7240_pci_irq_enable(irq);
}

static void
ar7240_pci_irq_set_affinity(unsigned int irq, const cpumask_t *mask)
{
	/*
	 * Only 1 CPU; ignore affinity request
	 */
}

struct irq_chip /* hw_interrupt_type */ ar7240_pci_irq_controller = {
	.name		= 	"AR7240 PCI",
	.startup	= 	ar7240_pci_irq_startup,
	.shutdown	= 	ar7240_pci_irq_shutdown,
	.enable		= 	ar7240_pci_irq_enable,
	.disable	= 	ar7240_pci_irq_disable,
	.ack		= 	ar7240_pci_irq_ack,
	.end		= 	ar7240_pci_irq_end,
	.eoi		= 	ar7240_pci_irq_end,
	.set_affinity	= 	ar7240_pci_irq_set_affinity,
};

void
ar7240_pci_irq_init(int irq_base)
{
	int i;

	for (i = irq_base; i < irq_base + AR7240_PCI_IRQ_COUNT; i++) {
		irq_desc[i].status = IRQ_DISABLED;
		irq_desc[i].action = NULL;
		irq_desc[i].depth = 1;
		//irq_desc[i].chip = &ar7240_pci_irq_controller;
		set_irq_chip_and_handler(i, &ar7240_pci_irq_controller,
					 handle_percpu_irq);
	}
}

/*
 * init the pci controller
 */

#ifdef CONFIG_PERICOM
static struct resource ar7240_io_resource1 = {
	"PCI IO space",
	0x00, 0x00,
	IORESOURCE_IO | IORESOURCE_DISABLED
};

static struct resource ar7240_io_resource2 = {
	"PCI IO space",
	0x01, 0x01,
	IORESOURCE_IO | IORESOURCE_DISABLED
};

#define AR7240_PCI_MEM_BASE_1	(AR7240_PCI_MEM_BASE)
#define AR7240_PCI_MEM_BASE_2	(AR7240_PCI_MEM_BASE + 0x2000000)
#define AR7240_PCI_MEM_SIZE	(1 << 20)

static struct resource ar7240_mem_resource1 = {
	"PCI memory space",
	AR7240_PCI_MEM_BASE_1,
	AR7240_PCI_MEM_BASE_1 + AR7240_PCI_MEM_SIZE,
	IORESOURCE_MEM
};

static struct resource ar7240_mem_resource2 = {
	"PCI memory space",
	AR7240_PCI_MEM_BASE_2,
	AR7240_PCI_MEM_BASE_2 + AR7240_PCI_MEM_SIZE,
	IORESOURCE_MEM
};

extern struct pci_ops ar7240_pci_ops;

static struct pci_controller ar7240_pci_controller1 = {
	.pci_ops	= &ar7240_pci_ops,
	.mem_resource	= &ar7240_mem_resource1,
	.io_resource	= &ar7240_io_resource1,
};

static struct pci_controller ar7240_pci_controller2 = {
	.pci_ops	= &ar7240_pci_ops,
	.mem_resource	= &ar7240_mem_resource2,
	.io_resource	= &ar7240_io_resource2,
};
#else
static struct resource ar7240_io_resource = {
	.name  =        "PCI IO space",
	.start =        0x0000,
	.end   =        0,
	.flags =        IORESOURCE_IO
};

static struct resource ar7240_mem_resource = {
	.name  =	"PCI memory space",
	.start =	AR7240_PCI_MEM_BASE,
	.end   =	AR7240_PCI_MEM_BASE + AR7240_PCI_WINDOW - 1,
	.flags =	IORESOURCE_MEM
};

extern struct pci_ops ar7240_pci_ops;

static struct pci_controller ar7240_pci_controller = {
	.pci_ops	= &ar7240_pci_ops,
	.mem_resource	= &ar7240_mem_resource,
	.io_resource	= &ar7240_io_resource,
};
#endif /* CONFIG_PERICOM */

irqreturn_t 
ar7240_pci_core_intr(int cpl, void *dev_id, struct pt_regs *regs)
{
    printk("PCI error intr\n");
#if 0
    ar7240_check_error(1);
#endif

    return IRQ_HANDLED;
}

/*
 * We want a 1:1 mapping between PCI and DDR for inbound and outbound.
 * The PCI<---AHB decoding works as follows:
 *
 * 8 registers in the DDR unit provide software configurable 32 bit offsets
 * for each of the eight 16MB PCI windows in the 128MB. The offsets will be 
 * added to any address in the 16MB segment before being sent to the PCI unit.
 *
 * Essentially  for any AHB address generated by the CPU,
 * 1. the MSB  four bits are stripped off, [31:28],
 * 2. Bit 27 is used to decide between the lower 128Mb (PCI) or the rest of 
 *    the AHB space
 * 3. Bits 26:24 are used to access one of the 8 window registers and are 
 *    masked off.
 * 4. If it is a PCI address, then the WINDOW offset in the WINDOW register 
 *    corresponding to the next 3 bits (bit 26:24) is ADDED to the address, 
 *    to generate the address to PCI unit.
 *
 *     eg. CPU address = 0x100000ff
 *         window 0 offset = 0x10000000
 *         This points to lowermost 16MB window in PCI space.
 *         So the resulting address would be 0x000000ff+0x10000000
 *         = 0x100000ff
 *
 *         eg2. CPU address = 0x120000ff
 *         WINDOW 2 offset = 0x12000000
 *         resulting address would be 0x000000ff+0x12000000
 *                         = 0x120000ff 
 *
 * There is no translation for inbound access (PCI device as a master)
 */ 
static int __init ar7240_pcibios_init(void)
{
	uint32_t cmd;

#ifdef CONFIG_WASP_SUPPORT
	if (is_ar9341()) {
		return 0;
	}
#endif
	/*
	 * Check if the WLAN PCI-E H/W is present, If the
	 * WLAN H/W is not present, skip the PCI
	 * initialization code and just return.
	 */

	if (((ar7240_reg_rd(AR7240_PCI_LCL_RESET)) & 0x1) == 0x0) {
		printk("***** Warning *****: PCIe WLAN H/W not found !!!\n");
		return 0;
	}
        if ((is_ar7241() || is_ar7242()))
		ar7240_reg_wr(AR7240_PCI_LCL_APP, (ar7240_reg_rd(AR7240_PCI_LCL_APP) | (0x1 << 16)));

#ifdef CONFIG_PERICOM

	ar7240_reg_wr(0xb80f0000, 0x0ffc1);     // Address Translation disabled
	ar7240_reg_wr(0x180f0040, 0);           // Enable Type 0
	ar7240_reg_rd(0x14000000);              // Reading the Config space of Upstream port of Switch
	ar7240_reg_wr(0x1400003c, 0x400000);    // Assert Reset to the Downstream ports
	ar7240_reg_wr(0x1400003c, 0x0);         // Deassert Reset
	ar7240_reg_wr(0x14000004, 0x106);
	ar7240_reg_wr(0x14000018, 0x070504);    // Program the Primary Bus, Sec Bus and Subordinate Bus
	ar7240_reg_wr(0x14000020, 0x1ff01000);  // Memory Base and Limit
	ar7240_reg_wr(0x14000024, 0x1ff01000);  // Prefetch Memory Base and Limit
	ar7240_reg_wr(0x140000b4, 0x0200010a);  // Enable Round robin priority on the ports
	ar7240_reg_wr(0x180f0040, 1);           // Enable Type 1

	// Configure the Pericom Switch's Downstream Port0 using Type1 Configuration
	ar7240_reg_rd(0x15080000);              // Reading the Config space of Downstream port0 of Switch
	ar7240_reg_wr(0x15080004, 0x106);       // Command register
	ar7240_reg_wr(0x15080018, 0x060605);    // Program the pri bus, sec bus and subordinate bus
	ar7240_reg_wr(0x15080020, 0x11f01000);  // Memory base and limit
	ar7240_reg_wr(0x15080024, 0x11f01000);

	// Configure the Pericom Switch's Downstream Port1 using Type1 Configuration
	ar7240_reg_rd(0x15100000);              // Reading the Config space of Downstream port1 of Switch
	ar7240_reg_wr(0x15100004, 0x106);
	ar7240_reg_wr(0x15100018, 0x070705);
	ar7240_reg_wr(0x15100020, 0x13f01200);
	ar7240_reg_wr(0x15100024, 0x13f01200);

	ar7240_reg_wr(0xb80f0000, 0x1ffc1);     // Address Translation enabled
#endif /* CONFIG_PERICOM */

	printk("PCI init:%s\n", __func__);
#ifndef CONFIG_PCI_INIT_IN_MONITOR
	cmd =	PCI_COMMAND_MEMORY |
		PCI_COMMAND_MASTER |
		PCI_COMMAND_INVALIDATE |
		PCI_COMMAND_PARITY |
		PCI_COMMAND_SERR |
		PCI_COMMAND_FAST_BACK;

	printk("%s(%d): PCI CMD write: 0x%x\n", __func__, __LINE__, cmd);

	ar7240_local_write_config(PCI_COMMAND, 4, cmd);

	/*
	 * clear any lingering errors and register core error IRQ
	 */
#if 0
	ar7240_check_error(0);
#endif

#	if !defined(CONFIG_PERICOM)
	ar7240_pci_ops.write(NULL, 0, PCI_COMMAND, 4, cmd);
#	endif /* CONFIG_PERICOM */
#endif

#ifdef CONFIG_PERICOM
#define ar7240_udelay(us) do {						\
	extern uint32_t ar7240_cpu_freq;				\
	volatile register int N = (ar7240_cpu_freq / 1000000) * (us);	\
	while (--N > 0);						\
} while(0)

	/* For Pericom -> Merlin link availability */
	ar7240_udelay(600);

	cmd =	PCI_COMMAND_MEMORY |
		PCI_COMMAND_MASTER |
		PCI_COMMAND_SERR;
	printk("%s: cmd = 0x%x\n", __func__, cmd);

	ar7240_pci_ops.write(2, 0, PCI_COMMAND, 4, cmd);
	ar7240_pci_ops.write(1, 0, PCI_COMMAND, 4, cmd);

	register_pci_controller(&ar7240_pci_controller1);
	register_pci_controller(&ar7240_pci_controller2);
#else
	register_pci_controller(&ar7240_pci_controller);
#endif /* CONFIG_PERICOM */

	return 0;
}
#ifndef CONFIG_AR7240_EMULATION
arch_initcall(ar7240_pcibios_init);
#endif
