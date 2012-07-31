#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/dmapool.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>


#include <atheros.h>
#include <linux/proc_fs.h>
#define ATH_PCI_MAX_EP_IN_SYSTEM	2
#define ATH_PCI_MAX_DTD			(512)
#define DMA_ADDR_INVALID		((dma_addr_t)~0)
#define DRIVER_DESC			"Atheros PCI EP Driver"

#define NET_IP_ALIGN			4

static const char device_name[] = "ath_dev";
static const char driver_name[] = "ath-pciep";
static struct proc_dir_entry *ath_pci_proc;

unsigned long int_count, case1, case2, int_process_count, int_process_count_rx, int_process_count_tx;
unsigned long actual_data_count;
unsigned long complete_count;
unsigned long queue_count;
unsigned long wipe_count;
unsigned long alloc_init_dtd_count;
unsigned long start_trans_count;
unsigned long isr_count, prime_count;
unsigned long retire_dtd_count, alloc_count;
unsigned long total_rx, total_tx;
extern int total_rx_packet, total_rx_complete, total_tx_packet, total_tx_complete;


/* First segament bit */
#define ZM_LS_BIT		0x100
/* Last segament bit */
#define ZM_FS_BIT		0x200

enum pci_regs{
	ATH_REG_AHB_RESET	= 0xb806001c,
	ATH_REG_MISC2_RESET	= 0xb80600bc,
	ATH_PCIEP_VD		= 0xb8230000,
};
#define PCI_AHB_RESET_DMA		(1 << 29)
#define PCI_AHB_RESET_DMA_HST_RAW	(1 << 31)
/* Will be asserted on reset and cleared on s/w read */
#define PCI_AHB_RESET_DMA_HST		(1 << 19)
#define PCI_MISC2_RST_CFG_DONE		(1 <<  0)


#define PCI_OWN_BITS_MASK		0x3
#define PCI_OWN_BITS_SW			0x0
#define PCI_OWN_BITS_HW			0x1
#define PCI_OWN_BITS_SE			0x2

struct pci_ep {
	void			*driver_data;
	const char		*name;
	const struct pci_ep_ops	*ops;
	unsigned		maxpacket:16;
};

typedef enum channel_id {
	CHANNEL_ID_RX0,
	CHANNEL_ID_RX1,
	CHANNEL_ID_RX2,
	CHANNEL_ID_RX3,
	CHANNEL_ID_TX0,
	CHANNEL_ID_TX1,
	CHANNEL_ID_MAX
} channel_id_t;
typedef enum pci_ep_type{
	PCI_EP_RX,
	PCI_EP_TX
}pci_ep_type_t;


struct ath_pci_ep {
	struct pci_ep ep;
	pci_ep_type_t type;
	channel_id_t id;
	struct list_head queue;
	struct list_head skipped_queue;
	struct ath_pci_dev *dev;
	unsigned maxpacket:16;
};
struct ath_pci_ep_rx_regs {
	__u32	rx_base,
		rx_start,
		rx_intr_lim,
		rx_burst_size,
		rx_resrv[3],
		rx_swap,
		rx_resrv1[56];
};


struct ath_pci_ep_tx_regs {
	__u32	tx_base,
		tx_start,
		tx_intr_lim,
		tx_burst_size,
		tx_resrv[2],
		tx_swap,
		tx_resrv1[57];
};
struct ath_pci {
	__u32 intr_status;
	__u32 intr_mask;
	__u32 resv[510];
	struct ath_pci_ep_rx_regs rx_regs[4];
	struct ath_pci_ep_tx_regs tx_regs[2];
};

struct ath_pci_dev {
	struct device *dev;
	struct dma_pool *dtd_pool;
	struct ath_pci __iomem *pci_ep_base;
	struct list_head dtd_list[ATH_PCI_MAX_EP_IN_SYSTEM * 2];
	struct ep_dtd *dtd_heads[ATH_PCI_MAX_EP_IN_SYSTEM * 2];
	struct ep_dtd *dtd_tails[ATH_PCI_MAX_EP_IN_SYSTEM * 2];
	void __iomem *reg_base;
	spinlock_t lock;
	struct ath_pci_ep ep[6];
};
struct ath_pci_dev *pci_dev;

struct ep_dtd {
	/* Hardware access fields */
	unsigned short	ctrl,
			status,
			totalLen,
			dataSize;
	unsigned int	last_dtd,
			dataAddr,
			next_dtd;
	/* Software access fields */
	dma_addr_t	dtd_dma;
	struct ep_dtd	*next;
	struct list_head tr_list;
};

#define DMA_BASE_OFF_PCIEP 0xb8127000
#if 0
struct ath_pci {
	__u32 rx0_base,
		rx0_start,
		rx0_intr_lim,
		rx0_burst_size,
		rx0_resrv[3],
		rx0_swap,
		rx0_resrv1[56],
		rx1_base,
		rx1_start,
		rx1_intr_lim,
		rx1_burst_size,
		rx1_resrv[3],
		rx1_swap,
		rx1_resrv1[56],
		rx2_base,
		rx2_start,
		rx2_intr_lim,
		rx2_burst_size,
		rx2_resrv[3],
		rx2_swap,
		rx2_resrv1[56],
		rx3_base,
		rx3_start,
		rx3_intr_lim,
		rx3_burst_size,
		rx3_resrv[3],
		rx3_swap,
		rx3_resrv1[56],

}
#endif
enum __dma_desc_status {
	DMA_STATUS_OWN_DRV = 0x0,
	DMA_STATUS_OWN_DMA = 0x1,
	DMA_STATUS_OWN_MSK = 0x3
};

enum __dma_bit_op {
	DMA_BIT_CLEAR	= 0x0,
	DMA_BIT_SET	= 0x1
};

enum __dma_burst_size {
	DMA_BURST_4W	= 0x00,
	DMA_BURST_8W	= 0x01,
	DMA_BURST_16W	= 0x02
};
enum __dma_byte_swap {
	DMA_BYTE_SWAP_OFF	= 0x00,
	DMA_BYTE_SWAP_ON	= 0x01
};
#if 0
typedef enum __dma_eng_off{
	DMA_ENG_OFF_RX0 = 0x800,
	DMA_ENG_OFF_RX1 = 0x900,
	DMA_ENG_OFF_RX2 = 0xa00,
	DMA_ENG_OFF_RX3 = 0xb00,
	DMA_ENG_OFF_TX0 = 0xc00,
	DMA_ENG_OFF_TX1 = 0xd00
}__dma_eng_off_t;

typedef enum __dma_reg_off{
	/**
	 * Common or Non Engine specific
	 */
	DMA_REG_IFTYPE	= 0x00,/*XXX*/
	DMA_REG_ISR	= 0x00,/*Interrupt Status Register*/
	DMA_REG_IMR	= 0x04,/*Interrupt Mask Register*/
	/**
	 * Transmit
	 */
	DMA_REG_TXDESC	= 0x00,/*TX DP*/
	DMA_REG_TXSTART	= 0x04,/*TX start*/
	DMA_REG_INTRLIM	= 0x08,/*TX Interrupt limit*/
	DMA_REG_TXBURST	= 0x0c,/*TX Burst Size*/
	DMA_REG_TXSWAP	= 0x18,
	/**
	 * Receive
	 */
	DMA_REG_RXDESC	= 0x00,/*RX DP*/
	DMA_REG_RXSTART	= 0x04,/*RX Start*/
	DMA_REG_RXBURST	= 0x08,/*RX Burst Size*/
	DMA_REG_RXPKTOFF = 0x0c,/*RX Packet Offset*/
	DMA_REG_RXSWAP	= 0x1c
}__dma_reg_off_t;
#endif
typedef enum pci_intr_bits{
	PCI_INTR_TX1_END	= 25,	/*TX1 reached the end or Under run*/
	PCI_INTR_TX0_END	= 24,	/*TX0 reached the end or Under run*/
	PCI_INTR_TX1_DONE	= 17,	/*TX1 has transmitted a packet*/
	PCI_INTR_TX0_DONE	= 16,	/*TX1 has transmitted a packet*/
	PCI_INTR_RX3_END	= 11,	/*RX3 reached the end or Under run*/
	PCI_INTR_RX2_END	= 10,	/*RX2 reached the end or Under run*/
	PCI_INTR_RX1_END	= 9,	/*RX1 reached the end or Under run*/
	PCI_INTR_RX0_END	= 8,	/*RX0 reached the end or Under run*/
	PCI_INTR_RX3_DONE	= 3,	/*RX3 received a packet*/
	PCI_INTR_RX2_DONE	= 2,	/*RX2 received a packet*/
	PCI_INTR_RX1_DONE	= 1,	/*RX1 received a packet*/
	PCI_INTR_RX0_DONE	= 0,	/*RX0 received a packet*/
}pci_intr_bits_t;

struct pci_request {
	void			*buf;
	unsigned		length;
	dma_addr_t		dma;

	void			(*complete)(struct pci_ep *ep,
				struct pci_request *req);
	void			*context;
	struct list_head	list;

	int			status;
	unsigned		actual;
};

struct ath_pci_req {
	struct pci_request req;
	struct list_head queue;
	struct ep_dtd *ep_dtd;
	unsigned mapped:1;
};

int ath_pci_ep_queue(struct pci_ep*, struct pci_request *, gfp_t );
struct pci_request *ath_pci_ep_alloc_request(struct pci_ep *, gfp_t );
void ath_pci_ep_free_request(struct pci_ep *, struct pci_request *);
int ath_pci_ep_disable(struct pci_ep *);
int ath_pci_ep_enable(struct pci_ep *);
#include "u_ether.c"
#if 0
#define DMA_ADDR_INIT(_eng) (DMA_BASE_OFF_HST + DMA_ENG_OFF_##_eng)
#define DMA_INTR_INIT(_eng) {			\
	.end	= DMA_INTR_##_eng##_END,	\
	.done	= DMA_INTR_##_eng##_DONE	\
}

__dma_eng_q_t eng_q[6] = {
	{.addr = DMA_ADDR_INIT(RX0), .intr = DMA_INTR_INIT(RX0)},
	{.addr = DMA_ADDR_INIT(RX1), .intr = DMA_INTR_INIT(RX1)},
	{.addr = DMA_ADDR_INIT(RX2), .intr = DMA_INTR_INIT(RX2)},
	{.addr = DMA_ADDR_INIT(RX3), .intr = DMA_INTR_INIT(RX3)},
	{.addr = DMA_ADDR_INIT(TX0), .intr = DMA_INTR_INIT(TX0)},
	{.addr = DMA_ADDR_INIT(TX1), .intr = DMA_INTR_INIT(TX1)},
};
#endif
void
ath_pci_handle_reset(void)
{
	volatile unsigned int r_data;

//	printk("Waiting for host reset..%d\n", __LINE__);
	ath_reg_rmw_set(ATH_REG_MISC2_RESET, PCI_MISC2_RST_CFG_DONE);
//	printk("Waiting for host reset..%d\n", __LINE__);
	ath_reg_rmw_clear(ATH_REG_MISC2_RESET, PCI_MISC2_RST_CFG_DONE);
	printk("Waiting for host reset..%d\n", __LINE__);
	/**
	 * Poll until the Host has reset
	 */
	for (;;) {
		r_data = ath_reg_rd(ATH_REG_AHB_RESET);

		if (r_data & PCI_AHB_RESET_DMA_HST_RAW)
			break;
	}
	printk("received.\n");

	/**
	 * Pull the AHB out of reset
	 */
	ath_reg_rmw_clear(ATH_REG_AHB_RESET, PCI_AHB_RESET_DMA);
	udelay(10);

	/**
	 * Put the AHB into reset
	 */
	ath_reg_rmw_set(ATH_REG_AHB_RESET, PCI_AHB_RESET_DMA);
	udelay(10);

	/**
	 * Pull the AHB out of reset
	 */
	ath_reg_rmw_clear(ATH_REG_AHB_RESET, PCI_AHB_RESET_DMA);

	r_data = ath_reg_rd(ATH_PCIEP_VD);
	if ((r_data & 0xffff0000) == 0xff1c0000) {
		/*
		 * Program the vendor and device ids after reset.
		 * In the actual chip this may come from OTP
		 * The actual values will be finalised later
		 */
		ath_reg_wr(ATH_PCIEP_VD, 0x0034168c);
	}
}
#define DMA_ENG_CHECK(_val, _low, _high)	\
	((_val) < DMA_ENGINE_##_low || (_val) > DMA_ENGINE_##_high)

void ath_pci_ep_free_request(struct pci_ep *ep, struct pci_request *_req)
{
	struct ath_pci_req *req;
//	printk("__enter %s\n", __func__);
	if (_req) {
		req = container_of(_req, struct ath_pci_req, req);
		kfree(req);
	}
}

static void ath_pci_dev_mem_free(struct ath_pci_dev *udc)
{
	struct ep_dtd *ep_dtd;
	int count;
	//int ret = 0;
	//printk("ath_pci_dev_mem_free \n");
	if (udc->dtd_pool) {
		for(count = 0; count < (ATH_PCI_MAX_EP_IN_SYSTEM*2); count++) {
			while (!list_empty(&udc->dtd_list[count])) {
				struct list_head *temp;
				temp = udc->dtd_list[count].next;
				ep_dtd = list_entry(temp, struct ep_dtd, tr_list);
				dma_pool_free(udc->dtd_pool, ep_dtd, ep_dtd->dtd_dma);
				list_del(temp);
			}
		}
		dma_pool_destroy(udc->dtd_pool);
		udc->dtd_pool = NULL;
	}
}
struct pci_request *ath_pci_ep_alloc_request(struct pci_ep *ep, gfp_t gfp_flags)
{
	struct ath_pci_req *req;
//	printk("__enter %s\n", __func__);

	req = (struct ath_pci_req *)kmalloc(sizeof(struct ath_pci_req), GFP_ATOMIC);
	if (req) {
		memset(req, 0, sizeof(struct ath_pci_req));
		req->req.dma = DMA_ADDR_INVALID;
		req->ep_dtd = NULL;
		INIT_LIST_HEAD(&req->queue);
	} else {
		return NULL;
	}
	return &req->req;
}
int ath_pci_ep_enable(struct pci_ep *_ep)
{
	struct ath_pci_ep *ep = container_of(_ep, struct ath_pci_ep, ep);
	__u8 epno, epdir, qh_offset;
	__u32 bits = 0;
	__u32 bit_pos;
	unsigned long flags;
	struct ath_pci_dev *udc;

//	printk("__enter %s\n", __func__);

	/* Get endpoint number and direction */
	epno = ep->id;
	epdir = ep->type;
	bit_pos = (1 << (16 * epdir + epno));

	qh_offset = (2 * epno) + epdir;
	udc = ep->dev;
	if(epdir == PCI_EP_RX)
		bits |= (((1<<PCI_INTR_RX0_DONE) << epno) | ((1<<PCI_INTR_RX0_END) << epno));
	else
		bits |= (((1<<PCI_INTR_TX0_DONE) << epno) | ((1<<PCI_INTR_TX0_END) << epno));

	spin_lock_irqsave(&udc->lock, flags);

	if(epdir == PCI_EP_RX) {
		writel(DMA_BURST_16W, &udc->pci_ep_base->rx_regs[epno].rx_burst_size);
		writel(DMA_BYTE_SWAP_ON, &udc->pci_ep_base->rx_regs[epno].rx_swap);
	} else {
		writel(DMA_BURST_8W, &udc->pci_ep_base->tx_regs[epno].tx_burst_size);
		writel(DMA_BYTE_SWAP_ON, &udc->pci_ep_base->tx_regs[epno].tx_swap);
	}
	writel((readl(&udc->pci_ep_base->intr_mask) | bits), &udc->pci_ep_base->intr_mask);
	/* Enable endpoint in Hardware TODO*/

	spin_unlock_irqrestore(&udc->lock, flags);
	return 0;
}

static void ath_pci_free_dtd(struct ath_pci_dev *udc, struct ep_dtd *ep_dtd, __u32 index)
{
	list_add_tail(&ep_dtd->tr_list, &udc->dtd_list[index]);
}

static void ath_pci_complete_transfer(struct ath_pci_ep *, struct ath_pci_req *,
	__u8 , int );
static void ath_pci_dev_ep_wipe(struct ath_pci_ep *ep, int status)
{
	struct ath_pci_req *req;
	struct ath_pci_dev *dev;
	struct ep_dtd *ep_dtd;
	dev = ep->dev;
	while (!list_empty(&ep->queue)) {
		__u8 epdir = ep->type;
		__u8 epno = ep->id;

		if ((ep_dtd = dev->dtd_heads[(epno * 2) + epdir]) != NULL) {
			dev->dtd_heads[(epno * 2) + epdir] = NULL;
		}

		if ((ep_dtd = dev->dtd_tails[(epno * 2) + epdir]) != NULL) {
			dev->dtd_tails[(epno * 2) + epdir] = NULL;
		}
		req = list_entry(ep->queue.next, struct ath_pci_req, queue);
		ath_pci_free_dtd(dev, req->ep_dtd, ((epno * 2) + epdir));

		list_del_init(&req->queue);
		ath_pci_complete_transfer(ep, req, epdir, status);
	}
	while(!list_empty(&ep->skipped_queue)) {
		__u8 epdir = ep->type;
		req = list_entry(ep->skipped_queue.next, struct ath_pci_req, queue);
		list_del_init(&req->queue);
		ath_pci_complete_transfer(ep, req, epdir, status);
	}
}

int ath_pci_ep_disable(struct pci_ep *_ep)
{
	struct ath_pci_ep *ep;
	unsigned long flags;
	__u8 epno, epdir;

	ep = container_of(_ep, struct ath_pci_ep, ep);
	if (!_ep) {
		return -EINVAL;
	}

	spin_lock_irqsave(&pci_dev->lock, flags);

	/* Cancel all current and pending requests for this endpoint */
	ath_pci_dev_ep_wipe(ep, -ESHUTDOWN);
	ep->ep.maxpacket = ep->maxpacket;
	/* Get endpoint number and direction */
	epno = ep->id;
	epdir = ep->type;

	/* Disable the endpoint in hardware */
	//TODO
	spin_unlock_irqrestore(&pci_dev->lock, flags);

	return 0;
}

static int ath_pci_ep_mem_init(struct ath_pci_dev *dev)
{
	int count, i;
	struct ep_dtd *ep_dtd, *prev_dtd = NULL, *first_dtd = NULL;

	dev->dtd_pool = dma_pool_create(
				"pci-ep",
				dev->dev,
				sizeof(struct ep_dtd),
				32 /* byte alignment (for hw parts) */ ,
				4096/* can't cross 4K */);
	if (!dev->dtd_pool) {
		printk("ath_pci_ep: dtd dma_pool_create failure\n");
		return -ENOMEM;
	}

	for (count = 0; count < (ATH_PCI_MAX_EP_IN_SYSTEM*2); count++) {
		for (i = 0; i < ATH_PCI_MAX_DTD; i++) {
			dma_addr_t dma;

			ep_dtd = dma_pool_alloc(dev->dtd_pool, GFP_ATOMIC, &dma);
			if (ep_dtd == NULL) {
				ath_pci_dev_mem_free(dev);
				return -ENOMEM;
			}
			if(!first_dtd) {
				first_dtd = ep_dtd;
				prev_dtd = ep_dtd;
			}
			//printk("Init DTD Alloc %p, %x\n", ep_dtd, dma);
			memset(ep_dtd, 0, 20); /* only Hardware access fields */
			ep_dtd->dtd_dma = dma;
			list_add_tail(&ep_dtd->tr_list, &dev->dtd_list[count]);
			ep_dtd->status = PCI_OWN_BITS_SW;
			ep_dtd->last_dtd = ep_dtd->dtd_dma;
			ep_dtd->ctrl |= (ZM_FS_BIT | ZM_LS_BIT);
			if(prev_dtd) {
				prev_dtd->next = ep_dtd;
				prev_dtd->next_dtd = dma;
				prev_dtd = ep_dtd;
			}
		}
		prev_dtd->next = first_dtd;
		prev_dtd->next_dtd = first_dtd->dtd_dma;
		prev_dtd = NULL;
		first_dtd = NULL;
	}

	return 0;
}
//static int dtd_count[2];
#define ATH_PCI_TD_NEXT_TERMINATE 0
static struct ep_dtd *
ath_pci_alloc_init_dtd(struct ath_pci_dev *dev, struct ath_pci_ep *ep, struct ath_pci_req *req, __u32 catalyst)
{
	struct list_head *temp;
	struct ep_dtd *ep_dtd = NULL;
	alloc_count++;

	if (!list_empty(&dev->dtd_list[catalyst])) {
		temp = dev->dtd_list[catalyst].next;
		ep_dtd = list_entry(temp, struct ep_dtd, tr_list);
		list_del(temp);
		//memset(ep_dtd, 0, 20); /* only Hardware access fields */
		//printk("****** DTD allocation success***** %u\n", catalyst);
	} else {
		//TODO:Cannot allocate new dtd. Logic needs to be changed.
		dma_addr_t dma;
		ep_dtd = dma_pool_alloc(dev->dtd_pool, GFP_ATOMIC, &dma);
		if (ep_dtd == NULL) {
			printk("Error: %s:%d\n", __FILE__, __LINE__);
			return NULL;
		}
		//printk("DTD Alloc %p, %x\n", ep_dtd, dma);
		ep_dtd->dtd_dma = dma;
		ep_dtd->next = NULL;
		list_add_tail(&ep_dtd->tr_list, &dev->dtd_list[catalyst]);
	}
	/* Initialize dtd */
	ep_dtd->dataSize = req->req.length;
	if (catalyst == 1) {
		ep_dtd->totalLen = ep_dtd->dataSize;
	}
	if (req->req.length) {
		ep_dtd->dataAddr = (__u32) (req->req.dma);
	} else {
		ep_dtd->dataAddr = 0;
	}
	ep_dtd->status |= PCI_OWN_BITS_HW;
	//printk("Length = %d, data addr = %x DTD = %p DMA = %x catalist = %u\n", req->req.length, ep_dtd->dataAddr, ep_dtd, ep_dtd->dtd_dma, catalyst);
	req->ep_dtd = ep_dtd;
	return ep_dtd;
}
static int ath_pci_ep_prime(struct ath_pci_dev *dev, struct ep_dtd *ep_dtd,
			__u8 epno, __u8 epdir, __u8 empty)
{
	prime_count++;
	if(empty) {
		//printk("Empty DTD List, epno = %u, epdir = %u ep_dtd = %p \n", epno, epdir, ep_dtd);
		if(epdir == PCI_EP_RX) {
			writel((__u32)ep_dtd->dtd_dma, &dev->pci_ep_base->rx_regs[epno].rx_base);
			writel(1, &dev->pci_ep_base->rx_regs[epno].rx_start);
		} else {
			writel((__u32)ep_dtd->dtd_dma, &dev->pci_ep_base->tx_regs[epno].tx_base);
			writel(1, &dev->pci_ep_base->tx_regs[epno].tx_start);
		}
	} else {
		//printk("epno = %u, epdir = %u, dtd_temp= %x\n", epno, epdir, dtd_temp);
		if(epdir == PCI_EP_RX) {
			writel(1, &dev->pci_ep_base->rx_regs[epno].rx_start);
		} else {
			writel(1, &dev->pci_ep_base->tx_regs[epno].tx_start);
		}
	}
	return 0;
}

static int ath_pci_start_trans(struct ath_pci_dev *udc, struct ath_pci_ep *ep,
	struct ath_pci_req *req, int flag)
{
	struct ep_dtd *ep_dtd = NULL;
	//unsigned long flags;
	__u32 catalyst;
	__u8 epno, epdir;
	__u8 empty;
	start_trans_count++;

	/* Get endpoint number and direction */
	epno = ep->id;
	epdir = ep->type;

	if (epdir == PCI_EP_RX)
		total_rx++;
	else
		total_tx++;

	catalyst = ((2 * epno) + epdir);

	if (catalyst < 0) {
		printk("Warning... wrong dtd head position \n");
	}
	/* Get a free device transfer descriptor from the pre-allocated dtd list */
	if (flag == 1) {
		ep_dtd = ath_pci_alloc_init_dtd(udc, ep, req, catalyst);
		if (!ep_dtd) {
			return -ENOMEM;
		}
		//printk("epno = %u, epdir = %u, ep_dtd = %x\n", epno, epdir, ep_dtd);
		/*
		 * If the endpoint is already primed we have nothing to do here; just
		 * return; TODO - attach the current dtd to the dtd list in the queue
		 * head if the endpoint is already primed.
		 */
	} else {
		if (!list_empty(&ep->skipped_queue)) {
			req = container_of(ep->skipped_queue.next, struct ath_pci_req, queue);
			list_del_init(&req->queue);
		}
		ep_dtd = req->ep_dtd;
		//printk("epno = %u, epdir = %u, ep_dtd = %x\n", epno, epdir, ep_dtd);
	}
	empty = list_empty(&ep->queue);
	if (empty) {
/*		printk("epno = %u, epdir = %u ep_dtd = %x tail= %x, head = %x, catalist = %u flag = %d\n", \
			epno, epdir, ep_dtd, udc->dtd_tails[catalyst], udc->dtd_heads[catalyst], catalyst, flag);
*/
		udc->dtd_heads[catalyst] = ep_dtd;
		udc->dtd_tails[catalyst] = ep_dtd;
	} else {
		udc->dtd_tails[catalyst] = ep_dtd;
	}

	ath_pci_ep_prime(udc, ep_dtd, epno, epdir, empty);

	return 0;
}

int ath_pci_ep_queue(struct pci_ep *_ep, struct pci_request *_req,
					gfp_t gfp_flags)
{
	struct ath_pci_req *req;
	struct ath_pci_ep *ep;
	struct ath_pci_dev *dev;
	unsigned long flags;
	//__u8 empty;
	queue_count++;
	//printk("__enter %s\n", __func__);

	//printk("_ep->name :%s, _ep->maxpacket :%d\n", _ep->name,
	//		_ep->maxpacket);

	/* Sanity checks */
	req = container_of(_req, struct ath_pci_req, req);
	if (!_req || !req->req.buf || !list_empty(&req->queue)) {
			printk("%s, Invalid Params %p %d, %d\n", __func__,
					_req->buf, _req->length, list_empty(&req->queue));
		return -EINVAL;
	}

	ep = container_of(_ep, struct ath_pci_ep, ep);
	if (!_ep) {
		return -EINVAL;
	}

	dev = ep->dev;
	/* If the request contains data transfer, then synchronize the buffer
	 * for DMA Transfer */
	if (_req->length) {
		/* DMA for All Trans */
		if (_req->dma == DMA_ADDR_INVALID) {
			_req->dma = dma_map_single(ep->dev->dev->parent,
					_req->buf, _req->length,
					(ep->type == PCI_EP_TX) ?
					DMA_TO_DEVICE : DMA_FROM_DEVICE);
			req->mapped = 1;
		} else {
			dma_sync_single_for_device(ep->dev->dev->parent,
					_req->dma, _req->length,
					(ep->type == PCI_EP_TX) ?
					DMA_TO_DEVICE : DMA_FROM_DEVICE);
			req->mapped = 0;
		}
	} else {
		_req->dma = DMA_ADDR_INVALID;
	}

	_req->status = -EINPROGRESS;
	_req->actual = 0;


	spin_lock_irqsave (&pci_dev->lock, flags);
	ath_pci_start_trans(dev, ep, req, 1);
	/*
	 * Add the request to Endpoint queue. If there are no transfers happening
	 * right now, start the current transfer
	 */

	list_add_tail(&req->queue, &ep->queue);
	spin_unlock_irqrestore (&pci_dev->lock, flags);

	return 0;
}

static void ath_pci_complete_transfer(struct ath_pci_ep *ep, struct ath_pci_req *req,
	__u8 epdir, int status)
{
//	printk("__enter %s\n", __func__);

	complete_count++;

	if (req->req.status == -EINPROGRESS) {
		req->req.status = status;
	} else {
		status = req->req.status;
	}

	if (req->req.length) {
		if (req->mapped) {
			dma_unmap_single(ep->dev->dev->parent,
					 req->req.dma, req->req.length,
					 (ep->type == PCI_EP_RX)
					 ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
			req->req.dma = DMA_ADDR_INVALID;
			req->mapped = 0;
		} else {
			dma_sync_single_for_cpu(ep->dev->dev->parent,
						req->req.dma, req->req.length,
						(ep->type == PCI_EP_RX)
						? DMA_TO_DEVICE : DMA_FROM_DEVICE);
		}
	}
#if 1
      	if(status != 0) {
    //	    spin_unlock(&ep->dev->lock);
		if (req->req.complete) {
			req->req.complete(&ep->ep, &req->req);
		}
     //	   spin_lock(&ep->dev->lock);
	}
#endif
}

static struct ath_pci_req *
ath_pci_retire_dtd(struct ath_pci_dev *dev, struct ep_dtd  *ep_dtd, __u8 epno, __u8 epdir)
{
	__u32 bit_pos, tmp;
	//unsigned long flags;
	struct ath_pci_ep *ep;
	struct ath_pci_req *req;
	//struct ep_dtd *temp;

	bit_pos = (1 << (16 * epdir + epno));
	tmp = (2 * epno + epdir);
	ep = &dev->ep[tmp];

	retire_dtd_count++;

//	printk("retire_dtd epno = %u, epdir = %u dtd = %p\n", epno, epdir, ep_dtd);
	if (ep_dtd) {

		ath_pci_free_dtd(dev, ep_dtd, tmp);
		if (!list_empty(&ep->queue)) {
			req = container_of(ep->queue.next, struct ath_pci_req, queue);
			req->req.actual = ep_dtd->totalLen - 2;
			actual_data_count += req->req.actual;
			list_del_init(&req->queue);
#if 1
			if(list_empty(&ep->queue)) {
				dev->dtd_heads[tmp] = NULL;
			} else {
				dev->dtd_heads[tmp] = (struct ep_dtd *)(dev->dtd_heads[tmp]->next);
				//printk("dtd_heads = %x, dtd_tails = %x, ep_dtd = %x\n", dev->dtd_heads[tmp], dev->dtd_tails[tmp], ep_dtd);
			}
#endif
			ath_pci_complete_transfer(ep, req, epdir, 0);
			req->req.status = 0;
		}
	} else {
		printk("Null ep_dtd Err \n");
	}
	return req;
}

static int ath_pci_ep_read_procmem(char *buf, char **start, off_t offset,
					int count, int *eof, void *data)
{
	return sprintf(buf,
			"Total ep queue count = %li\n"
			"Total rx %li Total tx %li\n"
			"Total interrupt count = %li\n"
			"Total interrupt process count = %li\n"
			"Total interrupt process count tx = %li\n"
			"Total interrupt process count rx = %li\n"
			"Total complete count = %li\n"
			"Total actual data count = %li\n"
			"Total retire dtd count = %li\n"
			"Total start transaction count = %li\n"
			"Total prime count = %li\n"
			"Total alloc count = %li\n"
			"Total throttle    = %d\n"
			"skbuff alignment  = %d\n"
			"Application rx (%d) rx_complete (%d) tx (%d) tx_complete(%d)\n"
			"DTD Head = %p DTD Tail = %p\n"
			"DTD Head = %p DTD Tail = %p\n",
			queue_count, total_rx, total_tx, int_count, int_process_count,
			int_process_count_tx, int_process_count_rx, complete_count,
			actual_data_count, retire_dtd_count, start_trans_count,
			prime_count, alloc_count, total_throttle, NET_IP_ALIGN, total_rx_packet, total_rx_complete, total_tx_packet,
			total_tx_complete, pci_dev->dtd_heads[0], pci_dev->dtd_tails[0],
			pci_dev->dtd_heads[1], pci_dev->dtd_tails[1]);
}
static void ath_pci_process_Intr(struct ath_pci_dev *dev, unsigned int bit_pos)
{
	struct ep_dtd *ep_dtd;
	__u8 epno = 0, epdir = 0;
	__u32 tmp;
	__u8 epDetect;
	int i, count, prev_count = 0;

	int_process_count++;

	if (bit_pos) {
		for (i = 0; i < (ATH_PCI_MAX_EP_IN_SYSTEM); i++) {
			/* Based on the bit position get EP number and direction */
			epDetect = 0;
				epno = i;
			if (bit_pos & ((1 << (PCI_INTR_RX0_DONE + i)) | (1 << (PCI_INTR_RX0_END + i)))) {
				epdir = PCI_EP_RX;
				epDetect = 1;
				int_process_count_rx++;
			}
			if (bit_pos & ((1 << (PCI_INTR_TX0_DONE + i)) | (1 << (PCI_INTR_TX0_END + i)))) {
				if(!epDetect) {
					epdir = PCI_EP_TX;
				}
				epDetect++;
				int_process_count_tx++;
			}
			for(;epDetect > 0; epDetect--) {
				count = 0;
				ep_dtd = NULL;
				if(epDetect) {
					unsigned long flags;
					struct ath_pci_req *req;
					struct ath_pci_ep *ep;

					/* Based on EP number and direction Get Queue head and dtd */
					tmp = ((2 * epno) + epdir);
					ep = &dev->ep[tmp];

					/*Searching for all the inactive DTDs*/
					do {
						spin_lock_irqsave(&dev->lock, flags);
						if (!ep_dtd) {
							ep_dtd = dev->dtd_heads[tmp];
							if (ep_dtd) {
					   			if((ep_dtd->status) & DMA_STATUS_OWN_DMA) {
//									printk("Hitting here ep_dtd = %p\n", ep_dtd);
									spin_unlock_irqrestore(&dev->lock, flags);
									break;
								}
							} else {
//							       printk("dtd Null tmp = %d, epno = %u, epdir = %u count = %u\n", tmp, epno, epdir, count);
								spin_unlock_irqrestore(&dev->lock, flags);
								break;
							}
						}
						if (ep_dtd) {
							/*TODO: Check for error cases*/
						}
						count++;
						/* Retire dtd & start next transfer */
						req = ath_pci_retire_dtd(dev, ep_dtd, epno, epdir);
						ep_dtd = dev->dtd_heads[tmp];
						spin_unlock_irqrestore(&dev->lock, flags);

						if (req) {
							if (req->req.complete) {
								req->req.complete(&ep->ep, &req->req);
							}
						} /* else {
							printk("bit_pos = %x epno = %u epdir = %u epDetect = %u tmp = %u count = %d prev_count = %d\n", bit_pos, epno, epdir, epDetect, tmp, count, prev_count);
						} */
						if (!ep_dtd) /*If no discriptor in the queue*/
							break;
					} while (!(ep_dtd->status & DMA_STATUS_OWN_DMA));
				}
				epdir = 1;
				prev_count = count;
			}
		}
	}
	return;
}

irqreturn_t ath_pci_dev_isr(int irq, void *dev)
{
	struct ath_pci_dev *pci = (struct ath_pci_dev *)dev;
	__u32 status = 0;
	int_count++;

	if(!pci){
		printk("pci null condition \n");
		return IRQ_NONE;
	}
//	printk("Flush the entry \n");
	ath_flush_pcie();
	for (;;) {
		status = readl(&pci->pci_ep_base->intr_status);
		if (!(status & readl(&pci->pci_ep_base->intr_mask))) {
			/* Nothing to do - exit */
//			break;
			return IRQ_HANDLED;
		}
		if (status & ((1 << PCI_INTR_TX0_END) |
				(1 << PCI_INTR_TX0_DONE) |
				(1 << PCI_INTR_RX0_DONE) |
				(1 << PCI_INTR_RX0_END)
				)) {
//			printk("PCI Interrupt\n");
			ath_pci_process_Intr(pci, (status & readl(&pci->pci_ep_base->intr_mask)));
		}
	}
	return IRQ_HANDLED;
}
static void ath_pci_dev_release(struct device *dev)
{
//	printk("__enter %s\n", __func__);
}

static noinline int ath_pci_dev_init(struct ath_pci_dev *pci, struct device *dev)
{
	int temp;
	struct ath_pci_ep *ep;
	struct pci_ep *_ep;

	pci->dev = dev; //TODO: check whether pointer is enough
	spin_lock_init(&pci->lock);
	for(temp = 0; temp < (ATH_PCI_MAX_EP_IN_SYSTEM*2); temp++) {
		INIT_LIST_HEAD(&pci->dtd_list[temp]);
		ep = &pci->ep[temp];
		ep->dev = pci;
		if(temp < 2)
		{
			ep->id = 0;
			ep->type = temp;
		} else {
			ep->id = 1;
			ep->type = temp-2;
		}

		INIT_LIST_HEAD(&ep->queue);
		INIT_LIST_HEAD(&ep->skipped_queue);
		_ep = &ep->ep;
		_ep->ops = NULL;
       		_ep->maxpacket = ep->maxpacket = 1696;

	}

	device_initialize(dev);
	//strcpy(udc->gadget.dev.bus_id, "gadget");
	pci->dev->release = ath_pci_dev_release;
	pci->dev->parent = dev;

	pci_dev = pci;
//	printk("UDC %p\n", pci_dev);

	/* Setup all endpoints */
	device_add(pci->dev);
#if 1
	if (ath_pci_ep_mem_init(pci) != 0) {
		return -ENOMEM;
	}
#endif
	ath_pci_handle_reset();
	return 0;
}

static int ath_pci_dev_probe(struct platform_device *pdev)
{
	struct ath_pci_dev *dev;
	void __iomem *reg_base;
	int retval;

//	printk("__enter %s \n", __func__);

	dev = (struct ath_pci_dev *)kmalloc(sizeof(struct ath_pci_dev), GFP_ATOMIC);
	if (dev == NULL) {
		printk("Unable to allocate pci device\n");
		return -ENOMEM;
	}
	memset(dev, 0, sizeof(struct ath_pci_dev));

	/* Allocate and map resources */
	if (!request_mem_region(pdev->resource[0].start,
				pdev->resource[0].end - pdev->resource[0].start + 1,
				driver_name)) {
		printk("ath_pci_dev_probe: controller already in use\n");
		retval = -EBUSY;
		goto err1;
	}

	reg_base = ioremap(pdev->resource[0].start,
			pdev->resource[0].end - pdev->resource[0].start + 1);
	if (!reg_base) {
		printk("ath_pci_dev_probe: error mapping memory\n");
		retval = -EFAULT;
		goto err2;
	}

	dev->reg_base = reg_base;
	dev->pci_ep_base = reg_base;

	/* Interrupt Request */
	if ((retval = request_irq(pdev->resource[1].start, ath_pci_dev_isr,
				IRQF_SHARED, driver_name, dev)) != 0) {
		printk("request interrupt %x failed\n", pdev->resource[1].start);
		retval = -EBUSY;
		goto err3;
	}
	if (ath_pci_dev_init(dev, &pdev->dev) == 0) {
		return 0;
	}
	free_irq(pdev->resource[1].start, dev);
err3:
	iounmap(reg_base);
err2:
	release_mem_region(pdev->resource[0].start,
				pdev->resource[0].end - pdev->resource[0].start + 1);
err1:
	pci_dev = NULL;
	kfree(dev);
	return retval;
}
static int ath_pci_dev_remove(struct platform_device *pdev)
{
	struct ath_pci_dev *dev = pci_dev;

	ath_pci_dev_mem_free(dev);
	free_irq(pdev->resource[1].start, dev);
	iounmap(dev->reg_base);
	release_mem_region(pdev->resource[0].start,
			pdev->resource[0].end - pdev->resource[0].start + 1);
	device_unregister(dev->dev);
	pci_dev = NULL;
	kfree(dev);

	return 0;
}

static struct platform_driver ath_pci_ep_drv = {
	.probe  = ath_pci_dev_probe,
	.remove = ath_pci_dev_remove,
	.driver = {
		.name = (char *)driver_name,
		.owner = THIS_MODULE,
	},
};

static int __init ath_pci_init(void)
{
	int ret;
	u8 ethaddr[ETH_ALEN]= {23,12,44,44,56,22};
	create_proc_read_entry("pci", 0, ath_pci_proc,
				ath_pci_ep_read_procmem, NULL);
	ret = platform_driver_register(&ath_pci_ep_drv);
	gether_setup(ethaddr);
	gether_connect(pci_dev);
	return ret;
}
static void __exit ath_pci_exit(void)
{
	platform_driver_unregister(&ath_pci_ep_drv);
	if (ath_pci_proc) {
		remove_proc_entry("pci", ath_pci_proc);
	}
}

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

arch_initcall(ath_pci_init);
module_exit(ath_pci_exit);

