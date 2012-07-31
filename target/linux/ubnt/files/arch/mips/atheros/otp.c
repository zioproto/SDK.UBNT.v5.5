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

#include <asm/delay.h>

#include <atheros.h>

uint32_t
ath_otp_read(uint32_t addr)
{
	uint32_t	rdata, i = 0;

	if (addr >= ATH_OTP_SIZE)
		return 0xdeadbeef;

	rdata = ath_reg_rd(ATH_OTP_MEM_ADDRESS + addr);

	do {
		if (i++ > 65536)
			return 0xdeadbeef;

		udelay(1);

		rdata = ath_reg_rd(ATH_OTP_STATUS0_ADDRESS);

	} while(OTP_STATUS0_EFUSE_ACCESS_BUSY_GET(rdata) ||
		!OTP_STATUS0_EFUSE_READ_DATA_VALID_GET(rdata));

	return (ath_reg_rd(ATH_OTP_STATUS1_ADDRESS));

}
EXPORT_SYMBOL(ath_otp_read);

uint32_t
ath_otp_write(uint32_t addr, uint32_t data)
{
	if (addr >= ATH_OTP_SIZE)
		return 0xdeadbeef;

#define ATH_OTP_MAGIC		0x10ad079

	ath_reg_wr(ATH_OTP_LDO_CONTROL, 1);

	while ((ath_reg_rd(ATH_OTP_LDO_STATUS) & 1) == 0);

	ath_reg_wr(ATH_OTP_INTF0_ADDRESS, ATH_OTP_MAGIC);

	ath_reg_wr(ATH_OTP_MEM_ADDRESS + addr, data);
	udelay(500);

	ath_reg_wr(ATH_OTP_LDO_CONTROL, 0);

	while ((ath_reg_rd(ATH_OTP_LDO_STATUS) & 1) != 0);

	return 0;

}
EXPORT_SYMBOL(ath_otp_write);

static ssize_t
ath_otp_fop_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned int	val, i, pos = (unsigned int)*ppos;

	for (i = 0; i < count; i += sizeof(val), pos += sizeof(val)) {

		val = ath_otp_read(pos);

		if (val == 0xdeadbeef)
			break;

		if (copy_to_user(buf + i, &val, sizeof(val)))
			return -EFAULT;
	}

 	*ppos += i;
 	return i;
}

static struct file_operations ath_otp_fops = {
	.read		= ath_otp_fop_read,
};

static struct miscdevice ath_otp_miscdev = {
	ATH_OTP_MINOR,
	"otp",
	&ath_otp_fops
};


static int __init
ath_otp_init(void)
{
	uint32_t	rdata, tdata;

	/*
	 * Calculate how many nanoseconds does a ref clock cycle take
	 *	40MHz -> 25 ns
	 *	25MHz -> 40 ns
	 */
	tdata = 1000 / (ath_ref_freq / 1000000);

	/*
	 * Set OTP registers:
	 *	Set OTP_INTF2_PG_STROBE_PW_REG_V
	 *		OTP_INTF2_PG_STROBE_PW_REG_V *
	 *		(how many nanoseconds does a ref clock cycle take)
	 *			= 5000 ns
	 *	40MHz -> 25 ns  --> 5000 / 25 = 1000 (0xC8)
	 *	25MHz -> 40 ns  --> 5000 / 40 = 1000 (0x7D)
	 */
	tdata = 5000 / tdata;
	rdata = ath_reg_rd(ATH_OTP_INTF2_ADDRESS);
	rdata &= ~OTP_INTF2_PG_STROBE_PW_REG_V_MASK;
	ath_reg_wr(ATH_OTP_INTF2_ADDRESS,
		rdata | OTP_INTF2_PG_STROBE_PW_REG_V_SET(tdata));

	/*
	 * Set OTP registers : Set OTP_INTF3_RD_STROBE_PW_REG_V to 8
	 */
	rdata = ath_reg_rd(ATH_OTP_INTF3_ADDRESS);
	rdata &= ~OTP_INTF3_RD_STROBE_PW_REG_V_MASK;
	ath_reg_wr(ATH_OTP_INTF3_ADDRESS,
		rdata | OTP_INTF3_RD_STROBE_PW_REG_V_SET(8));

	/*
	 * Set OTP registers : Set OTP_PGENB_SETUP_HOLD_TIME to 3
	 */
	rdata = ath_reg_rd(ATH_OTP_PGENB_SETUP_HOLD_TIME_ADDRESS);
	rdata &= ~OTP_PGENB_SETUP_HOLD_TIME_DELAY_MASK;
	ath_reg_wr(ATH_OTP_PGENB_SETUP_HOLD_TIME_ADDRESS,
		rdata | OTP_PGENB_SETUP_HOLD_TIME_DELAY_SET(3));

	printk("%s: Registering OTP ", __func__);

	if ((tdata = misc_register(&ath_otp_miscdev))) {
		printk("failed %d\n", tdata);
		return tdata;
	} else {
		printk("success\n");
	}

	return 0;
}

late_initcall(ath_otp_init);

