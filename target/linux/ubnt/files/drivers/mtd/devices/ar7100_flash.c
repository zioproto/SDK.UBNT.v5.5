/*
 * This file contains glue for Atheros ar7100 spi flash interface
 * Primitives are ar7100_spi_*
 * mtd flash implements are ar7100_flash_*
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

#include "ar7100.h"
#include "ar7100_flash.h"

/* this is passed in as a boot parameter by bootloader */
extern int __ath_flash_size;

/*
 * statics
 */
static void ar7100_spi_write_enable(void);
static void ar7100_spi_poll(void);
static void ar7100_spi_write_page(uint32_t addr, uint8_t *data, int len);
static void ar7100_spi_sector_erase(uint32_t addr);

static const char *part_probes[] __initdata = {"cmdlinepart", "RedBoot", NULL};

static DECLARE_MUTEX(ar7100_flash_sem);

/* GLOBAL FUNCTIONS */
void
ar7100_flash_spi_down(void) {
  down(&ar7100_flash_sem);
}

void
ar7100_flash_spi_up(void) {
  up(&ar7100_flash_sem);
}

EXPORT_SYMBOL(ar7100_flash_spi_down);
EXPORT_SYMBOL(ar7100_flash_spi_up);

#define AR7100_FLASH_SIZE_2MB          (2*1024*1024)
#define AR7100_FLASH_SIZE_4MB          (4*1024*1024)
#define AR7100_FLASH_SIZE_8MB          (8*1024*1024)
#define AR7100_FLASH_SIZE_16MB         (16*1024*1024)
#define AR7100_FLASH_PG_SIZE_256B       256
#ifndef ST25P28
#define AR7100_FLASH_SECTOR_SIZE_64KB  (64*1024)
#else
#define AR7100_FLASH_SECTOR_SIZE_256KB  (256*1024)
#endif

#define AR7100_FLASH_NAME               "ar7100-nor0"
/*
 * bank geometry
 */
typedef struct ar7100_flash_geom {
    uint32_t     size;
    uint32_t     sector_size;
    uint32_t     nsectors;
    uint32_t     pgsize;
}ar7100_flash_geom_t; 

ar7100_flash_geom_t flash_geom_tbl[AR7100_FLASH_MAX_BANKS] = 
        {
            {
#ifndef ST25P28
                .size           =   AR7100_FLASH_SIZE_4MB,
                .sector_size    =   AR7100_FLASH_SECTOR_SIZE_64KB,
                .pgsize         =   AR7100_FLASH_PG_SIZE_256B
#else
                .size           =   AR7100_FLASH_SIZE_16MB,
                .sector_size    =   AR7100_FLASH_SECTOR_SIZE_256KB,
                .pgsize         =   AR7100_FLASH_PG_SIZE_256B
#endif

            }
        };
          
static int
ar7100_flash_probe()
{
    return 0;
}

static int
ar7100_flash_erase(struct mtd_info *mtd,struct erase_info *instr)
{
    int nsect, s_curr, s_last;
	uint64_t  res;
    if (instr->addr + instr->len > mtd->size) 
	return (-EINVAL);

    ar7100_flash_spi_down();

	res = instr->len;
	do_div(res, mtd->erasesize);
	nsect = res;

	if (((uint32_t)instr->len) % mtd->erasesize)
		nsect ++;

	res = instr->addr;
	do_div(res,mtd->erasesize);
	s_curr = res;

    s_last  = s_curr + nsect;

    do {
        ar7100_spi_sector_erase(s_curr * AR7100_SPI_SECTOR_SIZE);
    } while (++s_curr < s_last);

    ar7100_spi_done();

    ar7100_flash_spi_up();

    if (instr->callback) {
	instr->state |= MTD_ERASE_DONE;
        instr->callback (instr);
    }

    return 0;
}

static int
ar7100_flash_read(struct mtd_info *mtd, loff_t from, size_t len,
                  size_t *retlen ,u_char *buf)
{
    uint32_t addr = from | 0xbf000000;

    if (!len) return (0);
    if (from + len > mtd->size) return (-EINVAL);

    ar7100_flash_spi_down();

    memcpy(buf, (uint8_t *)(addr), len);
    *retlen = len;

    ar7100_flash_spi_up();

    return 0;
}

static int
ar7100_flash_write (struct mtd_info *mtd, loff_t to, size_t len, 
                    size_t *retlen, const u_char *buf)
{
    int total = 0, len_this_lp, bytes_this_page;
    uint32_t addr = 0;
    u_char *mem;

    ar7100_flash_spi_down();

    while(total < len) {
        mem              = (u_char *)buf + total;
        addr             = to  + total;
        bytes_this_page  = AR7100_SPI_PAGE_SIZE - (addr % AR7100_SPI_PAGE_SIZE);
        len_this_lp      = min(((int)len - total), bytes_this_page);

        ar7100_spi_write_page(addr, mem, len_this_lp);
        total += len_this_lp;
    }

    ar7100_spi_done();

    ar7100_flash_spi_up();

    *retlen = len;
    return 0;
}


/*
 * sets up flash_info and returns size of FLASH (bytes)
 */
static int __init ar7100_flash_init (void)
{
    int i, np;
    ar7100_flash_geom_t *geom;
    struct mtd_info *mtd;
    struct mtd_partition *mtd_parts;
    uint8_t index;

    init_MUTEX(&ar7100_flash_sem);

    ar7100_reg_wr_nf(AR7100_SPI_CLOCK, 0x43);
    for(i = 0; i < AR7100_FLASH_MAX_BANKS; i++) {

        index = ar7100_flash_probe();
        geom  = &flash_geom_tbl[index];

        /* set flash size to value from bootloader if it passed valid value */ 
        /* otherwise use the default 4MB.                                   */
        if (__ath_flash_size >= 4 && __ath_flash_size <= 16) 
            geom->size = __ath_flash_size * 1024 * 1024;

        mtd         =  kmalloc(sizeof(struct mtd_info), GFP_KERNEL);
        if (!mtd) {
            printk("Cant allocate mtd stuff\n");
            return -1;
        }
        memset(mtd, 0, sizeof(struct mtd_info));

        mtd->name               =   AR7100_FLASH_NAME;
        mtd->type               =   MTD_NORFLASH;
        mtd->flags              =   (MTD_CAP_NORFLASH|MTD_WRITEABLE); 
        mtd->size               =   geom->size;
        mtd->erasesize          =   geom->sector_size;
        mtd->numeraseregions    =   0;
        mtd->eraseregions       =   NULL;
        mtd->owner              =   THIS_MODULE;
        mtd->erase              =   ar7100_flash_erase;
        mtd->read               =   ar7100_flash_read;
        mtd->write              =   ar7100_flash_write;
		mtd->writesize		= 1;
        np = parse_mtd_partitions(mtd, part_probes, &mtd_parts, 0);
        if (np > 0) {
            add_mtd_partitions(mtd, mtd_parts, np);
        }
        else
            printk("No partitions found on flash bank %d\n", i);
    }

    return 0;
}

static void __exit ar7100_flash_exit(void)
{
    /*
     * nothing to do
     */
}


/*
 * Primitives to implement flash operations
 */
static void
ar7100_spi_write_enable()  
{
    ar7100_reg_wr_nf(AR7100_SPI_FS, 1);                  
    ar7100_reg_wr_nf(AR7100_SPI_WRITE, AR7100_SPI_CS_DIS);     
    ar7100_spi_bit_banger(AR7100_SPI_CMD_WREN);             
    ar7100_spi_go();
}

static void
ar7100_spi_poll()   
{
    int rd;                                                 

    do {
        ar7100_reg_wr_nf(AR7100_SPI_WRITE, AR7100_SPI_CS_DIS);     
        ar7100_spi_bit_banger(AR7100_SPI_CMD_RD_STATUS);        
        ar7100_spi_delay_8();
        rd = (ar7100_reg_rd(AR7100_SPI_RD_STATUS) & 1);               
    }while(rd);
}

static void
ar7100_spi_write_page(uint32_t addr, uint8_t *data, int len)
{
    int i;
    uint8_t ch;

    ar7100_spi_write_enable();
    ar7100_spi_bit_banger(AR7100_SPI_CMD_PAGE_PROG);
    ar7100_spi_send_addr(addr);

    for(i = 0; i < len; i++) {
        ch = *(data + i);
        ar7100_spi_bit_banger(ch);
    }

    ar7100_spi_go();
    ar7100_spi_poll();
}

static void
ar7100_spi_sector_erase(uint32_t addr)
{
    ar7100_spi_write_enable();
    ar7100_spi_bit_banger(AR7100_SPI_CMD_SECTOR_ERASE);
    ar7100_spi_send_addr(addr);
    ar7100_spi_go();
    ar7100_spi_poll();
}

module_init(ar7100_flash_init);
module_exit(ar7100_flash_exit);
