//#include <linux/config.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <atheros.h>

/*
 * PCI IRQ map
 */
int __init
pcibios_map_irq(const struct pci_dev *dev, uint8_t slot, uint8_t pin)
{
	pr_debug("fixing irq for slot %d pin %d\n", slot, pin);
#ifdef CONFIG_PERICOM
	switch(dev->bus->number) {
	case ATH_PORT0_BUS_NO:
		printk("Returning IRQ %d for bus %d\n",
			ATH_PCI_IRQ_DEV0, dev->bus->number);
		return ATH_PCI_IRQ_DEV0;
	case ATH_PORT1_BUS_NO:
		printk("Returning IRQ %d for bus %d\n",
			ATH_PCI_IRQ_DEV1, dev->bus->number);
		return ATH_PCI_IRQ_DEV1;
	default:
		printk("%s: Unknown bus no. %d!\n",
			__func__, dev->bus->number);
		return -1;
	}
#else
	printk("Returning IRQ %d\n", ATH_PCI_IRQ_DEV0);
	return ATH_PCI_IRQ_DEV0;
#endif /* CONFIG_PERICOM */
}

int
pcibios_plat_dev_init(struct pci_dev *dev)
{
	return 0;
}

