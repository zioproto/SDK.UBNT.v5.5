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

#include <asm/mach-ar7240/ar7240.h>
#include <asm/delay.h>

#ifdef CONFIG_MACH_HORNET
#define AR7240_DEFAULT_WD_TMO    (10000000)                          /* 10s */
#define AR7240_DEFAULT_MOD_TMO   (jiffies + ((1000) * HZ) / 1000)    /*  1s */
static void wd_times_out(unsigned long dummy);
static DEFINE_TIMER(wd_timer, wd_times_out, 0, 0);
#else
#define AR7240_DEFAULT_WD_TMO	(20ul * USEC_PER_SEC)
#endif

#define FACTORY_RESET		0x89ABCDEF

#define AR7240_GPIO_RESET	AP_RESET_GPIO

#ifdef AR7240_WDT_TEST_CODE
#	define wddbg printk
#else
#	define wddbg(junk, ...)
#endif /* AR7240_WDT_TEST_CODE 8*/

typedef struct {
	int	open: 1,
		can_close: 1,
		tmo,
		action;
	wait_queue_head_t wq;
	uint32_t clk_freq;
} ar7240_wdt_t;

static ar7240_wdt_t wdt_softc_array;

static ar7240_wdt_t *wdt = &wdt_softc_array;

irqreturn_t ar7240_wdt_isr(int, void *, struct pt_regs *);


#ifdef AR7240_WDT_TEST_CODE
/* Return the value present in the watchdog register */
static inline uint32_t
ar7240_get_wd_timer(void)
{
	uint32_t val;

	val = (uint32_t)ar7240_reg_rd(AR7240_WATCHDOG_TMR);
	val = (val * USEC_PER_SEC) / wdt->clk_freq;

	return val;
}
#endif /* AR7240_WDT_TEST_CODE */

/* Set the timeout value in the watchdog register */
static inline void
ar7240_set_wd_timer(uint32_t usec /* micro seconds */)
{
	usec = usec * (wdt->clk_freq / USEC_PER_SEC);

	wddbg("%s: 0x%08x\n", __func__, usec);

	ar7240_reg_wr(AR7240_WATCHDOG_TMR, usec);
}

static inline int
ar7240_set_wd_timer_action(uint32_t val)
{
	if (val & ~AR7240_WD_ACT_MASK) {
		return EINVAL;
	}

	wdt->action = val;

	wddbg("%s: 0x%08x\n", __func__, val);
	/*
	 * bits  : 31 30 - 2 0-1
	 * access: RO  rsvd  Action
	 *
	 * Since bit 31 is read only and rest of the bits
	 * are zero, don't have to do a read-modify-write
	 */
	ar7240_reg_wr(AR7240_WATCHDOG_TMR_CONTROL, val);
	return 0;
}

#ifdef AR7240_WDT_TEST_CODE
static inline uint32_t
ar7240_get_wd_timer_action(void)
{
	return (uint32_t)(ar7240_reg_rd(AR7240_WATCHDOG_TMR_CONTROL) &
			AR7240_WD_ACT_MASK);
}

static inline uint32_t
ar7240_get_wd_timer_last(void)
{
	return ((uint32_t)(ar7240_reg_rd(AR7240_WATCHDOG_TMR_CONTROL) &
				AR7240_WD_LAST_MASK) >> AR7240_WD_LAST_SHIFT);
}
#endif /* AR7240_WDT_TEST_CODE */

irqreturn_t ar7240_wdt_isr(int cpl, void *dev_id, struct pt_regs *regs)
{
	unsigned delay;
	extern int ar7240_gpio_in_val(int);

#define UDELAY_COUNT 4000

	wddbg("%s: invoked\n", __func__);
#ifdef CONFIG_MACH_HORNET
    local_irq_disable();
#endif
	for (delay = UDELAY_COUNT; delay; delay--) {
		if (ar7240_gpio_in_val(AR7240_GPIO_RESET)) {
			break;
		}
		udelay(1000);
	}

	wddbg("%s: %d", __func__, delay);

	if (!delay) {
#ifdef CONFIG_MACH_HORNET
#ifdef AP_WATCHDOG_RESET_DISABLE
        ar7240_set_wd_timer(wdt->tmo);
        ar7240_set_wd_timer_action(AR7240_WD_ACT_RESET);
        udelay(100);
#endif
        ar7240_set_wd_timer(0);
#else
		wake_up(&wdt->wq);
#endif
	} else {
#ifdef CONFIG_MACH_HORNET
        local_irq_enable();
#else
		extern void ar7240_restart(char *);
		ar7240_restart(NULL);
#endif
	}
	return IRQ_HANDLED;
}

static int
ar7240wdt_open(struct inode *inode, struct file *file)
{
	wddbg("%s: called\n", __func__);

	if (MINOR(inode->i_rdev) != WATCHDOG_MINOR) {
		return -ENODEV;
	}

	if (wdt->open) {
		return -EBUSY;
	}

	wdt->open = 1;
	wdt->tmo = AR7240_DEFAULT_WD_TMO;
	wdt->action = AR7240_WD_ACT_NONE;
	wdt->can_close = 0;
	init_waitqueue_head(&wdt->wq);

	ar7240_set_wd_timer(wdt->tmo);
#ifndef CONFIG_MACH_HORNET
	ar7240_set_wd_timer_action(AR7240_WD_ACT_NONE);
#endif
	return nonseekable_open(inode, file);
}

static int
ar7240wdt_close(struct inode *inode, struct file *file)
{
	wddbg("%s: called\n", __func__);

	if (MINOR(inode->i_rdev) != WATCHDOG_MINOR) {
		return -ENODEV;
	}

	if (!wdt->can_close) {
		wddbg("%s: clearing action\n", __func__);
#ifndef CONFIG_MACH_HORNET
		ar7240_set_wd_timer_action(AR7240_WD_ACT_NONE);
#endif
	} else {
		wddbg("%s: not clearing action\n", __func__);
	}
	wdt->open = 0;
	return 0;
}

static ssize_t
ar7240wdt_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	wddbg("%s: called\n", __func__);

	return -ENOTSUPP;
}

static int
ar7240wdt_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int ret = 0;

	wddbg("%s: called\n", __func__);

	switch(cmd) {
		case FACTORY_RESET:
			wddbg("%s: intr action\n", __func__);
#ifndef CONFIG_MACH_HORNET
			if ((ret = request_irq(
					AR7240_MISC_IRQ_WATCHDOG,
					ar7240_wdt_isr,
					0,
					"Watchdog Timer",
					wdt))) {
				wddbg("%s: request_irq %d\n", __func__, ret);
				return ret;
			}

			ar7240_set_wd_timer_action(AR7240_WD_ACT_GP_INTR);
			sleep_on(&wdt->wq);
			free_irq(AR7240_MISC_IRQ_WATCHDOG, wdt);
#endif
			break;

		default: ret = -EINVAL;
	}

	return ret;
}

static ssize_t
ar7240wdt_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
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
		ar7240_set_wd_timer(wdt->tmo);
		return 1;
	}

	return 0;
}

static struct file_operations ar7240wdt_fops = {
	read:	ar7240wdt_read,
	write:	ar7240wdt_write,
	ioctl:	ar7240wdt_ioctl,
	open:	ar7240wdt_open,
	release:ar7240wdt_close
};

static struct miscdevice ar7240wdt_miscdev = {
	WATCHDOG_MINOR,
	"watchdog",
	&ar7240wdt_fops
};

#ifdef CONFIG_WASP_SUPPORT
	extern uint32_t ath_ref_clk_freq;
#else
	extern uint32_t ar7240_ahb_freq;
#endif

void
ar7240wdt_init(void)
{
	int ret;
	extern void ar7240_gpio_config_input(int);

	printk("%s: Registering WDT ", __func__);
	if ((ret = misc_register(&ar7240wdt_miscdev))) {
		printk("failed %d\n", ret);
		return;
	} else {
		printk("success\n");
	}

#ifdef CONFIG_WASP_SUPPORT
	wdt->clk_freq = ath_ref_clk_freq;
#else
	wdt->clk_freq = ar7240_ahb_freq;
#endif

#ifdef CONFIG_MACH_HORNET
   	wdt->tmo = AR7240_DEFAULT_WD_TMO;
    ar7240_set_wd_timer(wdt->tmo * 6);
    ar7240_set_wd_timer_action(AR7240_WD_ACT_RESET);
    mod_timer(&wd_timer, AR7240_DEFAULT_MOD_TMO);

    if (request_irq(
        AR7240_GPIO_IRQn(AR7240_GPIO_RESET),
        ar7240_wdt_isr,
        0,
        "Watchdog Reset",
        wdt)) {
        wddbg("%s: request_irq %d\n", __func__, ret);
    }
#endif
	ar7240_gpio_config_input(AR7240_GPIO_RESET);
}

#ifdef CONFIG_MACH_HORNET
static void wd_times_out(unsigned long dummy)
{

#ifdef AP_WATCHDOG_RESET_DISABLE
    printk("%s: Disable watchdog timer\n", __func__);
    ar7240_set_wd_timer_action(AR7240_WD_ACT_NONE);
    del_timer(&wd_timer);
#else
    ar7240_set_wd_timer(wdt->tmo);   
    mod_timer(&wd_timer, AR7240_DEFAULT_MOD_TMO);
#endif
}
#endif

late_initcall(ar7240wdt_init);

