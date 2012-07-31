#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/resource.h>
#include <linux/percpu.h>
#include <linux/smp.h>

#include <linux/console.h>
#include <asm/time.h>
#include <linux/miscdevice.h>
#include <asm/delay.h>
#include <asm/cevt-r4k.h>

#include <atheros.h>

#define ATH_CPU_DDR_CLOCK_CONTROL	(ATH_PLL_BASE + CPU_DDR_CLOCK_CONTROL_OFFSET)
#define ATH_CPU_PLL_CONFIG		(ATH_PLL_BASE + CPU_PLL_CONFIG_OFFSET)

#if 0
static ssize_t
ath_clksw_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}
#endif

static ssize_t
ath_clksw_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	struct clock_event_device *cd;
	unsigned int cpu = smp_processor_id();
	uint8_t setting;
	uint32_t val;
	struct { uint32_t freq, pre, post; } pll[] = {
		{ 300,	CPU_PLL_CONFIG_NINT_SET(0x18)	|
			CPU_PLL_CONFIG_REFDIV_SET(1)	|
			CPU_PLL_CONFIG_RANGE_SET(1)	|
			CPU_PLL_CONFIG_OUTDIV_SET(1),
			CPU_PLL_CONFIG_OUTDIV_SET(1)		},
		{ 400,	CPU_PLL_CONFIG_NINT_SET(32)	|
			CPU_PLL_CONFIG_REFDIV_SET(1)	|
			CPU_PLL_CONFIG_RANGE_SET(0)	|
			CPU_PLL_CONFIG_OUTDIV_SET(1),
			CPU_PLL_CONFIG_OUTDIV_SET(1)		},
		{ 500,	CPU_PLL_CONFIG_NINT_SET(20)	|
			CPU_PLL_CONFIG_REFDIV_SET(1)	|
			CPU_PLL_CONFIG_RANGE_SET(3)	|
			CPU_PLL_CONFIG_OUTDIV_SET(1),
			CPU_PLL_CONFIG_OUTDIV_SET(0)		},
		{ 600,	CPU_PLL_CONFIG_NINT_SET(24)	|
			CPU_PLL_CONFIG_REFDIV_SET(1)	|
			CPU_PLL_CONFIG_RANGE_SET(0)	|
			CPU_PLL_CONFIG_OUTDIV_SET(1),
			CPU_PLL_CONFIG_OUTDIV_SET(0)		},
	};

	if (copy_from_user(&setting, buf, 1)) {
		return -EFAULT;
	}

	setting = setting - '0';

	if (setting < 0 ||
	    setting >= (sizeof(pll) / sizeof(pll[0]))) {
		return -EINVAL;
	}

	printk("%s: setting - %d 0x%x 0x%x\n", __func__, setting,
		pll[setting].pre, pll[setting].post);

	// bypass for cpu pll
	val = ath_reg_rd(ATH_CPU_DDR_CLOCK_CONTROL);
	val &= ~CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK;
	val |= CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_SET(1);
	ath_reg_wr(ATH_CPU_DDR_CLOCK_CONTROL, val);
	udelay(10);

	// pll settings...
	ath_reg_wr(ATH_CPU_PLL_CONFIG,
		pll[setting].pre | CPU_PLL_CONFIG_PLLPWD_SET(1));
	udelay(10);

	// clear pll power
	val = ath_reg_rd(ATH_CPU_PLL_CONFIG);
	val &= ~CPU_PLL_CONFIG_PLLPWD_MASK;
	val |= CPU_PLL_CONFIG_PLLPWD_SET(0);
	ath_reg_wr(ATH_CPU_PLL_CONFIG, val);
	udelay(100);

	// reset out div
	val = ath_reg_rd(ATH_CPU_PLL_CONFIG);
	val &= ~CPU_PLL_CONFIG_OUTDIV_MASK;
	val |= pll[setting].post;
	ath_reg_wr(ATH_CPU_PLL_CONFIG, val);
	udelay(10);

	// unset bypass for cpu pll
	val = ath_reg_rd(ATH_CPU_DDR_CLOCK_CONTROL);
	val &= ~CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK;
	val |= CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_SET(0);
	ath_reg_wr(ATH_CPU_DDR_CLOCK_CONTROL, val);
	udelay(10);

	// reconfigure kernel's notion of time
	mips_hpt_frequency = pll[setting].freq * 1000000 / 2;

	// see r4k_clockevent_init()
	cd = &per_cpu(mips_clockevent_device, cpu);
	cd->mult		= div_sc((unsigned long) mips_hpt_frequency, NSEC_PER_SEC, 32);
	cd->max_delta_ns	= clockevent_delta2ns(0x7fffffff, cd);
	cd->min_delta_ns	= clockevent_delta2ns(0x300, cd);
	printk("%s: mult = %lu\n", __func__, cd->mult);

	return 1;
}

static struct file_operations ath_clksw_fops = {
	//.read		= ath_clksw_read,
	.write		= ath_clksw_write,
};

static struct miscdevice ath_clksw_miscdev = {
	ATH_CLKSW_MINOR,
	"clksw",
	&ath_clksw_fops
};


static int __init
ath_clksw_init(void)
{
	u32	tdata;

	printk("%s: Registering Clock Switch Interface ", __func__);

	if ((tdata = misc_register(&ath_clksw_miscdev))) {
		printk("failed %d\n", tdata);
		return tdata;
	} else {
		printk("success\n");
	}

	return 0;
}

late_initcall(ath_clksw_init);
