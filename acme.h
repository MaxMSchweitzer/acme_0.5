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
#define LED_OFF						0x00
#define LEDCTL 						0x00E00
#define GREENS						0x4E4E00

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
#define UPE								(1<<3)	
#define MPE								(1<<4)
#define BAM								(1<<15)

#define CTRL							0x00000
#define IMC								0x000D8
#define GCR								0x05B00
#define GCR2							0x05B64
#define MDIC							0x00020
#define CTRL_RST					(1<<26)

#define ACME_GET_DESC(R, i, type) (&(((struct type *)((R).desc))[i]))
#define ACME_RX_DESC(R, i)	ACME_GET_DESC(R, i, acme_rx_desc)

//static void acme_timer_cb(unsigned long);

struct pci_hw{
	void __iomem *hw_addr;
	void __iomem *led_addr;
};

struct acme_ring{
	struct acme_rx_desc *desc;
	dma_addr_t dma;
	u8 *wtf[ACME_RXD];
	unsigned int size;
	unsigned int count;
	u16 next_to_use;
	u16 next_to_clean;
};

struct acme_dev{
	struct cdev cdev;
	struct pci_dev *pdev;
	struct pci_hw hw;
	struct work_struct task;
	u32 rxd_cmd;
	struct acme_ring rx_ring;
 	struct acme_tx_desc *rx_desc[ACME_RXD];
	struct acme_ring *tx_ring;
	u32 led_ctl;
	int irq_info;
	int blink_rate; //blinks per second
} *acme_devp;

struct acme_rx_desc{
	__le64 buff_addr;
	__le16 length;
	__le16 csum;
	u8 status;
	u8 errors;
	__le16 special;
};

#endif
