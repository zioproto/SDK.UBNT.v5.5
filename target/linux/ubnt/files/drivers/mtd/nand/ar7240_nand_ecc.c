/*
 * This file contains glue for Atheros ar7240 spi flash interface
 * Primitives are ar7240_spi_*
 * mtd flash implements are ar7240_flash_*
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <asm/div64.h>

#include "ar7240.h"

#define AR7240_NAND_FLASH_BASE	0x1b000000u
#define AR7240_NF_RST		(AR7240_NAND_FLASH_BASE + 0x200u)
#define AR7240_NF_CTRL		(AR7240_NAND_FLASH_BASE + 0x204u)
#define AR7240_NF_RST_REG	(AR7240_NAND_FLASH_BASE + 0x208u)
#define AR7240_NF_ADDR0_0	(AR7240_NAND_FLASH_BASE + 0x21cu)
#define AR7240_NF_ADDR0_1	(AR7240_NAND_FLASH_BASE + 0x224u)
#define AR7240_NF_DMA_ADDR	(AR7240_NAND_FLASH_BASE + 0x264u)
#define AR7240_NF_DMA_COUNT	(AR7240_NAND_FLASH_BASE + 0x268u)
#define AR7240_NF_DMA_CTRL	(AR7240_NAND_FLASH_BASE + 0x26cu)
#define AR7240_NF_MEM_CTRL	(AR7240_NAND_FLASH_BASE + 0x280u)
#define AR7240_NF_PG_SIZE	(AR7240_NAND_FLASH_BASE + 0x284u)
#define AR7240_NF_RD_STATUS	(AR7240_NAND_FLASH_BASE + 0x288u)
#define AR7240_NF_TIMINGS_ASYN	(AR7240_NAND_FLASH_BASE + 0x290u)

// Number of Pages per block; 0=32, 1=64, 2=128, 3=256
#define AR7240_NF_BLK_SIZE		0x2
// No of bytes per page; 0=256, 1=512, 2=1024,
// 3=2048, 4=4096, 5=8182, 6= 16384, 7=0
#define AR7240_NF_PAGE_SIZE		0x4
// 1 = Enable, 0 = Disable
#define AR7240_NF_CUSTOM_SIZE_EN	0x1
// No of Address Cycles
#define AR7240_NF_ADDR_CYCLES_NUM	0x5
#define AR7240_NF_TIMING_ASYN		0x0
#define AR7240_NF_STATUS_OK		0xc0

#define AR7240_NF_BLOCK_SIZE		2112u

#if 1
#define nf	printk
#else
#define nf(...)
#endif

typedef int (*ar7240_rw_t)(loff_t, unsigned, const u_char *);
static int ar7240_nf_rw(struct mtd_info *, loff_t, size_t,
		size_t *, const u_char *, ar7240_rw_t);

int
ar7240_nf_status(const char *f, loff_t nf_addr,
		unsigned count, const u_char *addr)
{
	int status;

	while (ar7240_reg_rd(AR7240_NF_RST_REG) != 0xff);
	// READ STATUS
	ar7240_reg_wr(AR7240_NF_RST, 0x07024);
	status = ar7240_reg_rd(AR7240_NF_RD_STATUS);
#define AR7240_NF_DEBUG	1
#if AR7240_NF_DEBUG
	if (status != AR7240_NF_STATUS_OK) {
		nf("%s:Fail (0x%x) %llu %u %p",
			f, status, nf_addr, count, addr);
	} else {
		int i;
		char sep[] = { " \n" };
		for (i = 0; i < count; i++) {
			printk("%02x%c", addr[i], sep[(i % 16) == 0]);
		}
	}
#endif
	return status;
}


int
ar7240_nand_erase(loff_t nf_addr, unsigned __junk1, const u_char *__junk2)
{
	nf("%s: %llu\n", __func__, nf_addr);

	// ADDR0_0 Reg Settings
	ar7240_reg_wr(AR7240_NF_ADDR0_0, nf_addr);
	// ADDR0_1 Reg Settings
	ar7240_reg_wr(AR7240_NF_ADDR0_1, 0);

	// BLOCK ERASE
	ar7240_reg_wr(AR7240_NF_RST, 0xd0600e);

	return ar7240_nf_status(__func__, nf_addr, 0, 0);
}

int
ar7240_nand_write(loff_t nf_addr, unsigned count, const u_char *buf)
{
	// ADDR0_0 Reg Settings
	ar7240_reg_wr(AR7240_NF_ADDR0_0, nf_addr);
	// ADDR0_1 Reg Settings
	ar7240_reg_wr(AR7240_NF_ADDR0_1, 0x0);
	// DMA Start Addr
	ar7240_reg_wr(AR7240_NF_DMA_ADDR, (unsigned)buf);
	// DMA count
	ar7240_reg_wr(AR7240_NF_DMA_COUNT, count);
	// Custom Page Size
	ar7240_reg_wr(AR7240_NF_PG_SIZE, count);
	// DMA Control Reg
	ar7240_reg_wr(AR7240_NF_DMA_CTRL, 0x8c);

	// PROGRAM PAGE
	ar7240_reg_wr(AR7240_NF_RST, 0x10804c);

	return ar7240_nf_status(__func__, nf_addr, count, buf);
}

static int
ar7240_nf_erase(struct mtd_info *mtd, struct erase_info *ei)
{
	int	__junk1;

#define do_we_need_this	0
#if do_we_need_this
	__junk1 = ei->len % AR7240_NF_BLOCK_SIZE;
	if (__junk1) {
		ei->len -= __junk1;
	}
#endif

	return ar7240_nf_rw(mtd, ei->addr, ei->len, &__junk1,
				NULL, ar7240_nand_erase);
}

static int
ar7240_nf_rw(	struct mtd_info *mtd, loff_t nf_addr, size_t len,
		size_t *retlen, const u_char *buf, ar7240_rw_t rw)
{
	unsigned	i, n;

	n = len / AR7240_NF_BLOCK_SIZE;

	for (i = 0; i < n; i ++) {
		if (rw(nf_addr, AR7240_NF_BLOCK_SIZE, buf) !=
			AR7240_NF_STATUS_OK) {
			*retlen = i * AR7240_NF_BLOCK_SIZE;
			return -EIO;
		}
		len -= AR7240_NF_BLOCK_SIZE;
		buf += AR7240_NF_BLOCK_SIZE;
		nf_addr += AR7240_NF_BLOCK_SIZE;
	}

	/*
	 * read the reminder if read request is
	 * not aligned to block size
	 */
	if (len && rw(nf_addr, AR7240_NF_BLOCK_SIZE, buf) !=
			AR7240_NF_STATUS_OK) {
		*retlen = n * AR7240_NF_BLOCK_SIZE;
		return -EIO;
	}

	*retlen = n * AR7240_NF_BLOCK_SIZE + len;
	return 0;
}

int
ar7240_nand_read(loff_t nf_addr, unsigned count, const u_char *buf)
{
	nf("%s: %llu %u\n", __func__, nf_addr, count);
	// ADDR0_0 Reg Settings
	ar7240_reg_wr(AR7240_NF_ADDR0_0, nf_addr);

	// ADDR0_1 Reg Settings
	ar7240_reg_wr(AR7240_NF_ADDR0_1, 0x0);

	// DMA Start Addr
	ar7240_reg_wr(AR7240_NF_DMA_ADDR, (unsigned)buf);

	// DMA count
	ar7240_reg_wr(AR7240_NF_DMA_COUNT, count);

	// Custom Page Size
	ar7240_reg_wr(AR7240_NF_PG_SIZE, count);

	// DMA Control Reg
	ar7240_reg_wr(AR7240_NF_DMA_CTRL, 0xcc);

	// READ PAGE
	ar7240_reg_wr(AR7240_NF_RST, 0x30006a);

	return ar7240_nf_status(__func__, nf_addr, count, buf);
}

static int
ar7240_nf_read(	struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u_char *buf)
{
	return ar7240_nf_rw(mtd, from, len, retlen, buf, ar7240_nand_read);
}

static int
ar7240_nf_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	return ar7240_nf_rw(mtd, to, len, retlen, buf, ar7240_nand_write);
}

struct nand_chip ar_nf_chip = {
}

static struct mtd_info ar_nf_mtd = {
	.name			= "ar7240-nand0",
	.type			= MTD_NANDFLASH,
	.flags			= MTD_CAP_NANDFLASH,
	/*
	 * Giving some maximum size. So that the partition
	 * parsers can work. Will have to figure a way to
	 * determine the actual size.
	 *	-Varada Thu Dec 17 20:36:55 IST 2009
	 */
	.size			= (8ull << 30),
	.erasesize		= AR7240_NF_BLOCK_SIZE,
	.owner			= THIS_MODULE,
	.erase			= ar7240_nf_erase,
	.read			= ar7240_nf_read,
	.write			= ar7240_nf_write,
	.writesize		= AR7240_NF_BLOCK_SIZE,
	.priv			= &ar_nf_chip;
};

static int __init
ar7240_nf_init (void)
{
	int			np;
	struct mtd_partition	*mtd_parts;
	const char		*part_probes[] = {"cmdlinepart", NULL};

	np = parse_mtd_partitions(&ar_nf_mtd, part_probes, &mtd_parts, 0);
	if (np > 0) {
		add_mtd_partitions(&ar_nf_mtd, mtd_parts, np);

		// Control Reg Setting
		ar7240_reg_wr(AR7240_NF_CTRL,
				(AR7240_NF_ADDR_CYCLES_NUM) |
				(AR7240_NF_BLK_SIZE << 6) |
				(AR7240_NF_PAGE_SIZE << 8) |
				(AR7240_NF_CUSTOM_SIZE_EN << 11));

		// TIMINGS_ASYN Reg Settings
		ar7240_reg_wr(AR7240_NF_TIMINGS_ASYN, AR7240_NF_TIMING_ASYN);

		// NAND Mem Control Reg
		ar7240_reg_wr(AR7240_NF_MEM_CTRL, 0xff00);

		// Reset Command
		ar7240_reg_wr(AR7240_NF_RST, 0xff00);

	} else {
		printk("%s: No partitions found\n", __func__);
	}

	return 0;
}

static void
__exit ar7240_nf_exit(void)
{
}

module_init(ar7240_nf_init);
module_exit(ar7240_nf_exit);
