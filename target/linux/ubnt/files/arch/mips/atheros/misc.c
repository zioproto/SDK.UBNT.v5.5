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

/*
 * GPIO Misc IRQ Functions
 */
void ath_misc_enable_irq(unsigned int mask)
{
	ath_reg_rmw_set(ATH_MISC_INT_MASK, mask);
}
void ath_misc_disable_irq(unsigned int mask)
{
	ath_reg_rmw_clear(ATH_MISC_INT_MASK, mask);
}

unsigned int ath_misc_get_irq_mask(void)
{
	return ath_reg_rd(ATH_MISC_INT_MASK);
}
unsigned int ath_misc_get_irq_status(void)
{
	return ath_reg_rd(ATH_MISC_INT_STATUS);
}

EXPORT_SYMBOL(ath_misc_enable_irq);
EXPORT_SYMBOL(ath_misc_disable_irq);
EXPORT_SYMBOL(ath_misc_get_irq_mask);
EXPORT_SYMBOL(ath_misc_get_irq_status);

/*
 * Reset function
 */
void ath_reset(unsigned int mask)
{
	ath_reg_rmw_set(ATH_RESET, mask);
	udelay(100);
	ath_reg_rmw_clear(ATH_RESET, mask);
}

EXPORT_SYMBOL(ath_reset);

/*
 * DMA Functions for SLIC/STEREO Blocks
 */
void ath_dma_addr_wr(int chan, unsigned int val)
{
	ath_reg_wr(ATH_DMA_BASE + 0 + chan * 12, val);
}
void ath_dma_config_wr(int chan, unsigned int val)
{
	ath_reg_wr(ATH_DMA_BASE + 4 + chan * 12, val);
}
void ath_dma_update_wr(int chan, unsigned int val)
{
	ath_reg_wr(ATH_DMA_BASE + 8 + chan * 12, val);
}

unsigned int ath_dma_addr_rd(int chan)
{
	return ath_reg_rd(ATH_DMA_BASE + 0 + chan * 12);
}
unsigned int ath_dma_config_rd(int chan)
{
	return ath_reg_rd(ATH_DMA_BASE + 4 + chan * 12);
}

void ath_dma_config_buffer(int chan, void *buffer, int sizeCfg)
{
	unsigned int addr = KSEG1ADDR(buffer);
	ath_dma_addr_wr(chan, (unsigned int)addr);
	ath_dma_config_wr(chan, ((sizeCfg & 0x7) << 4) | 0x100);
}

EXPORT_SYMBOL(ath_dma_addr_wr);
EXPORT_SYMBOL(ath_dma_config_wr);
EXPORT_SYMBOL(ath_dma_update_wr);
EXPORT_SYMBOL(ath_dma_addr_rd);
EXPORT_SYMBOL(ath_dma_config_rd);
EXPORT_SYMBOL(ath_dma_config_buffer);

/*
 * SLIC
 */
#ifdef ATH_SLIC_CNTRL
unsigned int ath_slic_cntrl_rd(void)
{
	return ath_reg_rd(ATH_SLIC_CNTRL);
}
void ath_slic_cntrl_wr(unsigned int val)
{
	ath_reg_wr(ATH_SLIC_CNTRL, val);
}
unsigned int ath_slic_status_rd(void)
{
	return ath_reg_rd(ATH_SLIC_STATUS);
}
void ath_slic_0_slot_pos_wr(unsigned int val)
{
	ath_reg_wr(ATH_SLIC_SLOT0_NUM, val);
}
void ath_slic_1_slot_pos_wr(unsigned int val)
{
	ath_reg_wr(ATH_SLIC_SLOT1_NUM, val);
}
void ath_slic_freq_div_wr(unsigned int val)
{
	ath_reg_wr(ATH_SLIC_FREQ_DIV, val);
}
void ath_slic_sample_pos_wr(unsigned int val)
{
	ath_reg_wr(ATH_SLIC_SAM_POS, val);
}

void ath_slic_setup(int _sam, int _s0n, int _s1n)
{
	unsigned int cntrl = 0;
	ath_reset(ATH_RESET_SLIC);
	ath_gpio_enable_slic();
	ath_slic_freq_div_wr(0x60);
	ath_slic_sample_pos_wr(_sam);
	if (_s0n) {
		cntrl |= ATH_SLIC_CNTRL_ENABLE;
		cntrl |= ATH_SLIC_CNTRL_SLOT0_ENABLE;
		ath_slic_0_slot_pos_wr(_s0n);
	}
	if (_s1n) {
		cntrl |= ATH_SLIC_CNTRL_ENABLE;
		cntrl |= ATH_SLIC_CNTRL_SLOT1_ENABLE;
		ath_slic_1_slot_pos_wr(_s1n);
	}
	if (cntrl)
		ath_slic_cntrl_wr(cntrl);
}

EXPORT_SYMBOL(ath_slic_0_slot_pos_wr);
EXPORT_SYMBOL(ath_slic_1_slot_pos_wr);
EXPORT_SYMBOL(ath_slic_freq_div_wr);
EXPORT_SYMBOL(ath_slic_sample_pos_wr);
EXPORT_SYMBOL(ath_slic_status_rd);
EXPORT_SYMBOL(ath_slic_setup);
#endif /* ATH_SLIC_CNTRL */

EXPORT_SYMBOL(ath_slic_cntrl_rd);
EXPORT_SYMBOL(ath_slic_cntrl_wr);

/*
 * STEREO Block Helper Functions
 */

/* Low-level registers */
void ath_stereo_config_wr(unsigned int val)
{
	ath_reg_wr(ATH_STEREO_CONFIG, val);
}
void ath_stereo_volume_wr(unsigned int val)
{
	ath_reg_wr(ATH_STEREO_VOLUME, val);
}

unsigned int ath_stereo_config_rd(void)
{
	return ath_reg_rd(ATH_STEREO_CONFIG);
}
unsigned int ath_stereo_volume_rd(void)
{
	return ath_reg_rd(ATH_STEREO_VOLUME);
}

/* Routine sets up STEREO block for use. Use one of the predefined
 * configurations. Example:
 *
 * ath_stereo_config_setup(
 *   ATH_STEREO_CFG_MASTER_STEREO_FS32_48KHZ(ATH_STEREO_WS_16B))
 *
 */
void ath_stereo_config_setup(unsigned int cfg)
{
	unsigned int reset;
	ath_gpio_enable_stereo();
	ath_stereo_config_wr(cfg & ~ATH_STEREO_CONFIG_ENABLE);
	do {
		reset = ath_stereo_config_rd();
	} while (reset & ATH_STEREO_CONFIG_RESET);

	do {
		reset = ath_reg_rd(ATH_GPIO_IN);
	} while (0 == (reset & 1 << 7));

	do {
		reset = ath_reg_rd(ATH_GPIO_IN);
	} while (reset & 1 << 7);

	ath_stereo_config_wr(cfg | ATH_STEREO_CONFIG_ENABLE);
}

/*
 * GPIO Access
 */
DECLARE_MUTEX(ath_gpio_sem);

void ath_gpio_init(void)
{
	init_MUTEX(&ath_gpio_sem);
}

void ath_gpio_down(void)
{
	down(&ath_gpio_sem);
}

void ath_gpio_up(void)
{
	up(&ath_gpio_sem);
}

EXPORT_SYMBOL(ath_gpio_init);
EXPORT_SYMBOL(ath_gpio_down);
EXPORT_SYMBOL(ath_gpio_up);

/*
 * GPIO Function Enables
 */

/* enable SLIC block, takes away GPIO 5, 4, 3, and 2 */
void ath_gpio_enable_slic(void)
{
	ath_reg_rmw_set(ATH_GPIO_FUNCTIONS, ATH_GPIO_FUNCTION_SLIC_EN);
}

/* enable UART block, takes away GPIO 10 and 9 */
void ath_gpio_enable_uart(void)
{
	ath_reg_rmw_set(ATH_GPIO_FUNCTIONS, ATH_GPIO_FUNCTION_UART_EN);
	ath_reg_rmw_clear(ATH_GPIO_OE, 1 << 9);
	ath_reg_rmw_set(ATH_GPIO_OE, 1 << 10);
}

/* enable STEREO block, takes away GPIO 11,8,7, and 6 */
void ath_gpio_enable_stereo(void)
{
	ath_reg_rmw_clear(ATH_GPIO_INT_ENABLE, 1 << 11);
	ath_reg_rmw_clear(ATH_GPIO_OE, 1 << 11);
	ath_reg_rmw_set(ATH_GPIO_FUNCTIONS,
			   ATH_GPIO_FUNCTION_STEREO_EN);
}

/* allow CS0/CS1 to be controlled via SPI register, takes away GPIO0/GPIO1 */
void ath_gpio_enable_spi_cs1_cs0(void)
{
	ath_reg_rmw_set(ATH_GPIO_FUNCTIONS,
			   ATH_GPIO_FUNCTION_SPI_CS_0_EN |
			   ATH_GPIO_FUNCTION_SPI_CS_1_EN);
	ath_reg_rmw_clear(ATH_GPIO_INT_ENABLE, 3);
	ath_reg_rmw_set(ATH_GPIO_OE, 3);
}

/* allow GPIO0/GPIO1 to be used as SCL/SDA for software based i2c */
void ath_gpio_enable_i2c_on_gpio_0_1(void)
{
	ath_reg_rmw_clear(ATH_GPIO_FUNCTIONS,
			     ATH_GPIO_FUNCTION_SPI_CS_0_EN |
			     ATH_GPIO_FUNCTION_SPI_CS_1_EN);
	ath_reg_rmw_clear(ATH_GPIO_INT_ENABLE, 3);
	ath_reg_rmw_clear(ATH_GPIO_OE, 3);
}

EXPORT_SYMBOL(ath_gpio_enable_slic);
EXPORT_SYMBOL(ath_gpio_enable_uart);
EXPORT_SYMBOL(ath_gpio_enable_stereo);
EXPORT_SYMBOL(ath_gpio_enable_spi_cs1_cs0);
EXPORT_SYMBOL(ath_gpio_enable_i2c_on_gpio_0_1);

/*
 * GPIO General Functions
 */

/* drive bits in mask low */
void ath_gpio_drive_low(unsigned int mask)
{
	ath_reg_wr(ATH_GPIO_CLEAR, mask);
	ath_reg_rmw_set(ATH_GPIO_OE, mask);
}

/* drive bits in mask high */
void ath_gpio_drive_high(unsigned int mask)
{
	ath_reg_wr(ATH_GPIO_SET, mask);
	ath_reg_rmw_set(ATH_GPIO_OE, mask);
}

/* Allow bits in mask to float to their quiescent state and test results */
unsigned int ath_gpio_float_high_test(unsigned int mask)
{
	volatile unsigned int d;
	ath_reg_rmw_clear(ATH_GPIO_OE, mask);
	d = ath_reg_rd(ATH_GPIO_IN);
	d = ath_reg_rd(ATH_GPIO_IN) & mask;
	return d != mask;
}

EXPORT_SYMBOL(ath_gpio_drive_low);
EXPORT_SYMBOL(ath_gpio_drive_high);
EXPORT_SYMBOL(ath_gpio_float_high_test);

#ifdef USE_TEST_CODE

void ath_gpio_test_toggle(unsigned int mask)
{
	do {
		ath_gpio_drive_low(mask);
		udelay(10);
		ath_gpio_drive_high(mask);
		udelay(10);
	} while (0 == test_ui_char_present());
}

void ath_gpio_test_toggle_pull_high(unsigned int mask)
{
	do {
		ath_gpio_drive_low(mask);
		udelay(10);
		ath_gpio_float_high_test(mask);
		udelay(10);
	} while (0 == test_ui_char_present());
}

EXPORT_SYMBOL(ath_gpio_test_toggle)
    EXPORT_SYMBOL(ath_gpio_test_toggle_pull_high)
#endif
/*
 * Software support of i2c on gpio 0/1
 */
#define ATH_I2C_SCL  (1<<0)
#define ATH_I2C_SDA  (1<<1)
#define ATH_I2C_PAUSE 2
static int ath_i2c_errcnt = 0;

static void ath_i2c_errclr(void)
{
	ath_i2c_errcnt = 0;
}

static void ath_i2c_check_rc(unsigned int rc)
{
	if (rc)
		ath_i2c_errcnt++;
}

static int ath_i2c_errget(void)
{
	return ath_i2c_errcnt;
}

static void ath_i2c_chigh_dhigh(void)
{
	ath_i2c_check_rc(ath_gpio_float_high_test
			    (ATH_I2C_SCL | ATH_I2C_SDA));
	udelay(ATH_I2C_PAUSE);
}

static void ath_i2c_chigh_dlow(void)
{
	ath_i2c_check_rc(ath_gpio_float_high_test(ATH_I2C_SCL));
	ath_gpio_drive_low(ATH_I2C_SDA);
	udelay(ATH_I2C_PAUSE);
}

static void ath_i2c_clow_dlow(void)
{
	ath_gpio_drive_low(ATH_I2C_SCL | ATH_I2C_SDA);
	udelay(ATH_I2C_PAUSE);
}

static void ath_i2c_clow_dhigh(void)
{
	ath_gpio_drive_low(ATH_I2C_SCL);
	ath_i2c_check_rc(ath_gpio_float_high_test(ATH_I2C_SDA));
	udelay(ATH_I2C_PAUSE);
}

static void ath_i2c_clow_dfloat(void)
{
	ath_gpio_drive_low(ATH_I2C_SCL);
	ath_reg_rmw_clear(ATH_GPIO_OE, ATH_I2C_SDA);
	udelay(ATH_I2C_PAUSE);
}

static void ath_i2c_chigh_dfloat(void)
{
	ath_gpio_drive_high(ATH_I2C_SCL);
	ath_reg_rmw_clear(ATH_GPIO_OE, ATH_I2C_SDA);
	udelay(ATH_I2C_PAUSE);
}

static int ath_i2c_chigh_dread(void)
{
	int d;

	ath_gpio_float_high_test(ATH_I2C_SCL);
	ath_reg_rmw_clear(ATH_GPIO_OE, ATH_I2C_SDA);
	udelay(ATH_I2C_PAUSE / 2);

	d = (ath_reg_rd(ATH_GPIO_IN) & ATH_I2C_SDA) ? 1 : 0;
	udelay(ATH_I2C_PAUSE / 2);

	return d;
}

static void ath_i2c_start(void)
{
	ath_i2c_chigh_dhigh();
	ath_i2c_chigh_dlow();
	ath_i2c_clow_dlow();
}

static void ath_i2c_stop(void)
{
	ath_i2c_clow_dlow();
	ath_i2c_chigh_dlow();
	ath_i2c_chigh_dhigh();
}

static int ath_i2c_raw_write_8(unsigned char v)
{
	int ack;
	int ii = 7;
	do {
		if ((1 << ii) & v) {
			ath_i2c_clow_dhigh();
			ath_i2c_chigh_dhigh();
		} else {
			ath_i2c_clow_dlow();
			ath_i2c_chigh_dlow();
		}
	} while (ii--);

	ath_i2c_clow_dfloat();
	ack = ath_i2c_chigh_dread();
	ath_i2c_clow_dfloat();

	return ack;
}

static void ath_i2c_raw_read_8(char lastByte, unsigned char *v)
{
	int d;
	int ii = 7;
	int jj = 0;
	do {
		ath_i2c_clow_dfloat();
		d = ath_i2c_chigh_dread();
		if (d)
			jj |= 1 << ii;
	} while (ii--);

	if (lastByte) {
		ath_i2c_clow_dfloat();
		ath_i2c_chigh_dfloat();
	} else {
		ath_i2c_clow_dlow();
		ath_i2c_chigh_dlow();
	}
	*v = jj & 0xff;
}

int
ath_i2c_raw_write_bytes_to_addr(int addr, unsigned char *buffer, int count)
{
	volatile int ack;
	int ii;
	ath_i2c_errclr();
	ath_i2c_start();
	ack = ath_i2c_raw_write_8(addr & 0xfe);
	if (ack)
		return 1;

	for (ii = 0; ii < count; ii++) {
		ack = ath_i2c_raw_write_8(buffer[ii]);
	}
	ath_i2c_stop();
	return ath_i2c_errget();
}

int
ath_i2c_raw_read_bytes_from_addr(int addr, unsigned char *buffer, int count)
{
	int ack;
	int ii;
	ath_i2c_errclr();
	ath_i2c_start();
	ack = ath_i2c_raw_write_8((addr & 0xff) | 0x01);
	for (ii = 0; ii < count; ii++)
		ath_i2c_raw_read_8(ii == (count - 1), &buffer[ii]);
	ath_i2c_stop();
	return ath_i2c_errget();
}

EXPORT_SYMBOL(ath_i2c_raw_write_bytes_to_addr);
EXPORT_SYMBOL(ath_i2c_raw_read_bytes_from_addr);

#ifdef USE_TEST_CODE

void ath_i2c_test_write_bits(void)
{
	printk("Writing bit stream of AA00\n");
	ath_i2c_errclr();
	do {
		ath_i2c_start();
		ath_i2c_raw_write_8(0xAA);
		ath_i2c_raw_write_8(0x00);
		ath_i2c_stop();
		udelay(1000);
	} while (0 == test_ui_char_present());
}

void ath_i2c_test_addr_strapping(void)
{
	int jj;

	int end = 0x7e;
	int addr = 0x20;

	jj = 0;
	printk("Looping through addresses %02x .. %02x\n", addr, end);
	while (addr < end) {
		volatile int ack;
		ath_i2c_start();
		ack = ath_i2c_raw_write_8(addr & 0xfe);
		ath_i2c_stop();
		if (0 == ack) {
			jj++;
			printk(" Found addr:  %02x\n", addr);
		}
		addr += 2;
	};

	if (0 == jj)
		printk(" Failed test, no i2c found\n");
}

EXPORT_SYMBOL(ath_i2c_test_write_bits);
EXPORT_SYMBOL(ath_i2c_test_addr_strapping);

#endif

/*
 * SPI Access Functions
 */

DECLARE_MUTEX(ath_spi_sem);

void ath_spi_init(void)
{
	init_MUTEX(&ath_spi_sem);
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x43);
}

void ath_spi_down(void)
{
	down(&ath_spi_sem);
}

void ath_spi_up(void)
{
	up(&ath_spi_sem);
}

EXPORT_SYMBOL(ath_spi_init);
EXPORT_SYMBOL(ath_spi_down);
EXPORT_SYMBOL(ath_spi_up);

void ath_spi_raw_output_u8(unsigned char val) __attribute__ ((weak));

void ath_spi_raw_output_u8(unsigned char val)
{
	unsigned int reg = 0, i, j;
	unsigned int dac_data = 0;

	// Start Write transaction
	reg = val;
	for (j = 0; j < 5; j++);

	for (i = 0; i < 8; i++)	// TRANSMIT DATA
	{
		dac_data = 0x50000;
		if ((reg >> (7 - i) & 0x1) == 0x1) {
			dac_data = dac_data | 0x1;
		} else {
			dac_data = dac_data & 0xfffffffe;
		}
		ath_reg_wr(0xbf000008, dac_data);
		for (j = 0; j < 5; j++);
		dac_data = dac_data | 0x50100;	//RISING_CLK
		ath_reg_wr(0xbf000008, dac_data);
		for (j = 0; j < 15; j++);
	}
}

void ath_spi_raw_output_u32(unsigned int val)
{
	int ii;
	unsigned int cs;
	cs = ath_reg_rd(ATH_SPI_WRITE) & ~(ATH_SPI_D0_HIGH |
						 ATH_SPI_CLK_HIGH);
	for (ii = 31; ii >= 0; ii--) {
		unsigned char jj = (val >> ii) & 1;
		ath_reg_wr_nf(ATH_SPI_WRITE, cs | jj);
		ath_reg_wr_nf(ATH_SPI_WRITE,
				 cs | jj | ATH_SPI_CLK_HIGH);
	}
}

unsigned int ath_spi_raw_input_u8(void) __attribute__ ((weak));

unsigned int ath_spi_raw_input_u8(void)
{
	int ii;
	unsigned int cs;

	cs = ath_reg_rd(ATH_SPI_WRITE) & ~(ATH_SPI_D0_HIGH |
						 ATH_SPI_CLK_HIGH);

	for (ii = 7; ii >= 0; ii--) {
		ath_reg_wr_nf(ATH_SPI_WRITE, cs);
		ath_reg_wr_nf(ATH_SPI_WRITE, cs | ATH_SPI_CLK_HIGH);
	}

	return ath_reg_rd(ATH_SPI_RD_STATUS) & 0xff;
}

unsigned int ath_spi_raw_input_u32(void)
{
	int ii;
	unsigned int cs;

	cs = ath_reg_rd(ATH_SPI_WRITE) & ~(ATH_SPI_D0_HIGH |
						 ATH_SPI_CLK_HIGH);

	for (ii = 31; ii >= 0; ii--) {
		ath_reg_wr_nf(ATH_SPI_WRITE, cs);
		ath_reg_wr_nf(ATH_SPI_WRITE, cs | ATH_SPI_CLK_HIGH);
	}

	return ath_reg_rd(ATH_SPI_RD_STATUS);
}

EXPORT_SYMBOL(ath_spi_raw_output_u8);
EXPORT_SYMBOL(ath_spi_raw_output_u32);
EXPORT_SYMBOL(ath_spi_raw_input_u8);
EXPORT_SYMBOL(ath_spi_raw_input_u32);

#define ATH_SPI_CMD_WREN         0x06
#define ATH_SPI_CMD_RD_STATUS    0x05
#define ATH_SPI_CMD_FAST_READ    0x0b
#define ATH_SPI_CMD_PAGE_PROG    0x02
#define ATH_SPI_CMD_SECTOR_ERASE 0xd8

static void ath_spi_wait_done(void)
{
	int rd;

	do {
		ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
		ath_spi_raw_output_u8(ATH_SPI_CMD_RD_STATUS);
		ath_spi_raw_output_u8(0);
		rd = (ath_reg_rd(ATH_SPI_RD_STATUS) & 1);
	} while (rd);
}

static void ath_spi_send_addr(unsigned int addr)
{
	ath_spi_raw_output_u8(((addr & 0xff0000) >> 16));
	ath_spi_raw_output_u8(((addr & 0x00ff00) >> 8));
	ath_spi_raw_output_u8(addr & 0x0000ff);
}

void ath_spi_flash_read_page(unsigned int addr, unsigned char *data, int len)
{
	printk("### %s not implemented \n", __FUNCTION__);
}

void
ath_spi_flash_write_page(unsigned int addr, unsigned char *data, int len)
{
	int i;
	uint8_t ch;

	ath_spi_raw_output_u8(ATH_SPI_CMD_WREN);
	ath_spi_raw_output_u8(ATH_SPI_CMD_PAGE_PROG);
	ath_spi_send_addr(addr);

	for (i = 0; i < len; i++) {
		ch = *(data + i);
		ath_spi_raw_output_u8(ch);
	}
	ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
	ath_spi_wait_done();
}

void ath_spi_flash_sector_erase(unsigned int addr)
{
	ath_spi_raw_output_u8(ATH_SPI_CMD_WREN);
	ath_spi_raw_output_u8(ATH_SPI_CMD_SECTOR_ERASE);
	ath_spi_send_addr(addr);
	ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
	ath_spi_wait_done();
}

EXPORT_SYMBOL(ath_spi_flash_read_page);
EXPORT_SYMBOL(ath_spi_flash_write_page);
EXPORT_SYMBOL(ath_spi_flash_sector_erase);
