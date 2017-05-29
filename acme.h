#ifndef ACME_H
#define ACME_H

#include <linux/pci.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/if.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

#define DEVCOUNT					1
#define DEVNAME						"ece_led"
#define BLINKRATE					2
#define LED_ON						0x4E
#define LED_OFF						0x0F
#define LEDCTL 						0x00E00
#define ACME_RXD	 				0x10	//number of descriptors
#define DESC_SIZE					0x800 // 2048 bytes
#define REG_IRQ_RX				0x0001
#define TDT								0x03818
#define TDH								0x03810
#define ICR								0x000C0
#define RDBAL0						0x02800
#define RDBAH0						0x02804
#define RDLEN0						0x02808
#define RDH0							0x02810
#define RDT0							0x02818
#define RCTL							0x00100
#define CTRL							0x00000
#define IMC								0x000D8
#define GCR								0x05B00
#define GCR2							0x05B64
#define MDIC							0x00020
#define CTRL_RST					(1<<26)
#define IFCS							0x02000000

#define ACME_GET_DESC(R, i, type) (&(((struct type *)((R).desc))[i]))
#define ACME_RX_DESC(R, i)	ACME_GET_DESC(R, i, acme_rx_desc)

//static void acme_timer_cb(unsigned long);

struct ixgbe_adapter{
	struct work_struct service_task;
};


struct pci_hw{
	void __iomem *hw_addr;
	void __iomem *led_addr;
};

struct acme_dev{
	struct cdev cdev;
	struct pci_dev *pdev;
	struct pci_hw hw;
	u32 rxd_cmd;
	struct acme_ring *rx_ring;
 	struct acme_tx_desc *rx_desc[ACME_RXD];
	struct acme_ring *tx_ring;
	u32 led_ctl;
	int irq_info;
	int blink_rate; //blinks per second
} *acme_devp;

struct acme_page{
	struct page *page;
	u64 dma;
};

// alloc_ring_dma where
// tx_map
struct acme_buffer{

	// physical address back from dma_map_single
	dma_addr_t dma;
	void *buffer;
	// skb has a data field that is the payload 
	// which is being sent to dma_map_single
	// pinning so we can dma it when we get the
	// pysical address back for dma_map_single
	// it is assigned to dma
	struct sk_buff *skb;// points to kernal logical
	union {
		/* Tx */
		struct {
			unsigned long time_stamp;
			u16 length;
			u16 next_to_watch;
			unsigned int segs;
			unsigned int bytecount;
			u16 mapped_as_page;
		};
		/* Rx */
		struct {
			/* arrays of page information for packet split */
			struct acme_page *ps_pages;
			struct page *page;
		};
	};
};

struct acme_ring{
	void *desc;
	dma_addr_t dma;
	u8 *wtf[ACME_RXD];
	dma_addr_t handle[ACME_RXD];
	unsigned int size;
	unsigned int count;
	u16 next_to_use;
	u16 next_to_clean;
	void __iomem *head;
	void __iomem *tail;
	struct acme_buffer *buffer_info;
	char name[IFNAMSIZ+5];
	u32 ims_val;
	u32 itr_val;
	void __iomem * itr_register;
	int set_itr;
	struct sk_buff *rx_skb_top;
};


struct acme_rx_desc{
	__le64 buff_addr;
	__le16 length;
	__le16 csum;
	u8 status;
	u8 errors;
	__le16 special;
};


//struct acme_rx_desc{
struct acme_tx_desc{
	__le64 buff_addr;
	union{
		__le32 data;
		struct{
			__le16 length;
			u8 cso;
			u8 cmd;
		}flags;
	}lower;
	union{
		__le32 data;
		struct{
			u8 status;
			u8 css;
			__le16 special;
		}fields;
	}upper;
};


#endif
