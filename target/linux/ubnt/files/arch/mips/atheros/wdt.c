//#include <linux/config.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/resource.h>

#include <linux/console.h>
#include <asm/serial.h>

#include <linux/tty.h>
#include <linux/time.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/miscdevice.h>

#include <asm/mach-atheros/atheros.h>
#include <asm/delay.h>

#define ATH_DEFAULT_WD_TMO	(20ul * USEC_PER_SEC)

#define FACTORY_RESET		0x89ABCDEF

#define ATH_GPIO_RESET	21

#ifdef ATH_WDT_TEST_CODE
#	define wddbg printk
#else
#	define wddbg(junk, ...)
#endif /* ATH_WDT_TEST_CODE 8 */

extern uint32_t ath_ahb_freq;

typedef struct {
	int open:1, can_close:1, tmo, action;
	wait_queue_head_t wq;
} ath_wdt_t;

static ath_wdt_t wdt_softc_array;

static ath_wdt_t *wdt = &wdt_softc_array;

irqreturn_t ath_wdt_isr(int, void *);

#ifdef ATH_WDT_TEST_CODE
/* Return the value present in the watchdog register */
static inline uint32_t ath_get_wd_timer(void)
{
	uint32_t val;

	val = (uint32_t) ath_reg_rd(ATH_WATCHDOG_TMR);
	val = (val * USEC_PER_SEC) / ath_ahb_freq;

	return val;
}
#endif /* ATH_WDT_TEST_CODE */

/* Set the timeout value in the watchdog register */
void ath_set_wd_timer(uint32_t usec /* micro seconds */)
{
#ifdef CONFIG_MACH_AR934x
	usec = usec * (ath_ref_freq / USEC_PER_SEC);
#else
	usec = usec * (ath_ahb_freq / USEC_PER_SEC);
#endif

	wddbg("%s: 0x%08x\n", __func__, usec);

	ath_reg_wr(ATH_WATCHDOG_TMR, usec);
}

int ath_set_wd_timer_action(uint32_t val)
{
	if (val & ~ATH_WD_ACT_MASK) {
		return EINVAL;
	}

	wdt->action = val;

	/*
	 * bits  : 31 30 - 2 0-1
	 * access: RO  rsvd  Action
	 *
	 * Since bit 31 is read only and rest of the bits
	 * are zero, don't have to do a read-modify-write
	 */
	ath_reg_wr(ATH_WATCHDOG_TMR_CONTROL, val);
	return 0;
}

#ifdef ATH_WDT_TEST_CODE
static inline uint32_t ath_get_wd_timer_action(void)
{
	return (uint32_t) (ath_reg_rd(ATH_WATCHDOG_TMR_CONTROL) &
			   ATH_WD_ACT_MASK);
}

static inline uint32_t ath_get_wd_timer_last(void)
{
	return ((uint32_t) (ath_reg_rd(ATH_WATCHDOG_TMR_CONTROL) &
			    ATH_WD_LAST_MASK) >> ATH_WD_LAST_SHIFT);
}
#endif /* ATH_WDT_TEST_CODE */

irqreturn_t ath_wdt_isr(int cpl, void *dev_id)
{
	unsigned delay;
	extern int ath_gpio_in_val(int);

#define UDELAY_COUNT 4000

	wddbg("%s: invoked\n", __func__);

	for (delay = UDELAY_COUNT; delay; delay--) {
		if (ath_gpio_in_val(ATH_GPIO_RESET)) {
			break;
		}
		udelay(1000);
	}

	wddbg("%s: %d", __func__, delay);

	if (!delay) {
		wake_up(&wdt->wq);
	} else {
		extern void ath_restart(char *);
		ath_restart(NULL);
	}
	return IRQ_HANDLED;
}

static int athwdt_open(struct inode *inode, struct file *file)
{
	wddbg("%s: called\n", __func__);

	if (MINOR(inode->i_rdev) != WATCHDOG_MINOR) {
		return -ENODEV;
	}

	if (wdt->open) {
		return -EBUSY;
	}

	wdt->open = 1;
	wdt->tmo = ATH_DEFAULT_WD_TMO;
	wdt->action = ATH_WD_ACT_NONE;
	wdt->can_close = 0;
	init_waitqueue_head(&wdt->wq);

	ath_set_wd_timer(wdt->tmo);
	ath_set_wd_timer_action(ATH_WD_ACT_NONE);

	return nonseekable_open(inode, file);
}

static int athwdt_close(struct inode *inode, struct file *file)
{
	wddbg("%s: called\n", __func__);

	if (MINOR(inode->i_rdev) != WATCHDOG_MINOR) {
		return -ENODEV;
	}

	if (!wdt->can_close) {
		wddbg("%s: clearing action\n", __func__);
		ath_set_wd_timer_action(ATH_WD_ACT_NONE);
	} else {
		wddbg("%s: not clearing action\n", __func__);
	}
	wdt->open = 0;
	return 0;
}

static ssize_t
athwdt_read(struct file *file, char *buf, size_t count, loff_t * ppos)
{
	wddbg("%s: called\n", __func__);

	return -ENOTSUPP;
}

static int
athwdt_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int ret = 0;

	wddbg("%s: called\n", __func__);

	switch (cmd) {
	case FACTORY_RESET:
		wddbg("%s: intr action\n", __func__);

		if ((ret = request_irq(ATH_MISC_IRQ_WATCHDOG,
				       ath_wdt_isr,
				       0, "Watchdog Timer", wdt))) {
			wddbg("%s: request_irq %d\n", __func__, ret);
			return ret;
		}

		ath_set_wd_timer_action(ATH_WD_ACT_GP_INTR);
		sleep_on(&wdt->wq);
		free_irq(ATH_MISC_IRQ_WATCHDOG, wdt);
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

static ssize_t
athwdt_write(struct file *file, const char *buf, size_t count, loff_t * ppos)
{
	int i;
	char c;

	wddbg("%s: called\n", __func__);

	for (i = 0; i != count; i++) {
		if (get_user(c, buf + i)) {
			return -EFAULT;
		}

		if (c == 'V') {
			wdt->can_close = 1;
			break;
		}
	}

	if (i) {
		ath_set_wd_timer(wdt->tmo);
		return 1;
	}

	return 0;
}

static struct file_operations athwdt_fops = {
      read:athwdt_read,
      write:athwdt_write,
      ioctl:athwdt_ioctl,
      open:athwdt_open,
      release:athwdt_close
};

static struct miscdevice athwdt_miscdev = {
	WATCHDOG_MINOR,
	"watchdog",
	&athwdt_fops
};

int __init athwdt_init(void)
{
	int ret;
	extern void ath_gpio_config_input(int);

	printk("%s: Registering WDT ", __func__);
	if ((ret = misc_register(&athwdt_miscdev))) {
		printk("failed %d\n", ret);
		return ret;
	} else {
		printk("success\n");
	}

	ath_gpio_config_input(ATH_GPIO_RESET);

	return 0;
}

late_initcall(athwdt_init);
