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
//#include <linux/slab.h>
#include <asm/uaccess.h>

#define DEVCOUNT					1
#define DEVNAME						"ece_led"
#define BLINKRATE					2
#define LED_ON						0x4E
#define LED_OFF						0x0F
#define LEDCTL 						0x00E00
#define ACME_RXD	 				0x10	//number of descriptors
#define DESC_SIZE					0x800 // 2048 bytes

static void acme_timer_cb(unsigned long);

struct pci_hw{
	void __iomem *hw_addr;
	void __iomem *led_addr;
};

struct acme_dev{
	struct cdev cdev;				
	struct pci_hw hw;
	struct acme_ring *rx_ring;
	struct acme_ring *tx_ring;
 	struct acme_desc desc[ACME_RXD];
	u32 led_ctl;    
	int blink_rate; //blinks per second
} *acme_devp;

struct acme_data{
	void *dma_buffer_rx;
	void *dma_buffer_tx;
dma_addr_t dma_rx;
	dma_addr_t dma_tx;
} *dev_data;

struct acme_page{
	struct page *page;
	u64 dma;
};

struct acme_buffer{
	dma_addr_t dma;
	struct sk_buff *skb;
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

struct acme_desc{
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

struct acme_ring{
	void *desc;
	dma_addr_t dma;
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

#endif
