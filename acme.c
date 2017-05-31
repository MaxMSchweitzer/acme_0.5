/*
	Chad Coates
	ECE 373
	Homework #6
	May 18, 2017

	This is the ACME: ece_led driver
*/

#include "acme.h"

static dev_t acme_dev_number;
static struct class *acme_class;
static struct timer_list acme_timer;
static int blink_rate=BLINKRATE;

module_param(blink_rate,int,S_IRUGO|S_IWUSR);

//*
static void acme_timer_cb(unsigned long data){
	acme_devp->led_ctl = readl(acme_devp->hw.led_addr);	
	acme_devp->led_ctl=(acme_devp->led_ctl==LED_OFF) ? LED_ON : LED_OFF;
	writel(acme_devp->led_ctl,acme_devp->hw.led_addr);
	mod_timer(&acme_timer,(jiffies+(HZ/acme_devp->blink_rate)/2));
}

static void service_task(struct work_struct *work){
	int tail;
	msleep(500);
	writel(LED_OFF,acme_devp->hw.hw_addr+LEDCTL);
	tail=readl(acme_devp->hw.hw_addr+RDT0);
	if(tail==0)tail=acme_devp->count-1;
	else --tail;
	writel(tail,acme_devp->hw.hw_addr+RDT0);

}

//*
static irqreturn_t acme_irq_handler(int irq, void *data){
//	struct acme_data_t *acme_data=data;
	u32 cause;
	acme_devp->irq_info=irq;
	cause=readl(acme_devp->hw.hw_addr+ICR);
	switch(cause){
		case REG_IRQ_RX:
			writel(GREENS,acme_devp->hw.hw_addr+LEDCTL);
			schedule_work(&acme_devp->task);
			break;
		default:
			break;
	}
	return IRQ_HANDLED;
}
//*/
static int acme_open(struct inode *inode,struct file *filp){
	int err=0;
	setup_timer(&acme_timer, acme_timer_cb, (unsigned long)&acme_devp);
	writel(LED_ON,acme_devp->hw.led_addr);
	mod_timer(&acme_timer,(jiffies+(HZ/acme_devp->blink_rate)/2));
	return err;
}

static int acme_close(struct inode *inode,struct file *filp){
	int err=0;
	writel(LED_OFF,acme_devp->hw.led_addr);
	del_timer_sync(&acme_timer);
	return err;
}

static ssize_t 
acme_read(struct file *filp,char __user *buf,size_t len,loff_t *offset){
	int ret;
	
	if(*offset >= sizeof(int))return 0;
	if(!buf){
		ret = -EINVAL;
		goto out;
	}

	if(copy_to_user(buf,&acme_devp->blink_rate,sizeof(int))){
		ret = -EFAULT;
		goto out;
	}
	ret = sizeof(int);
	*offset += len;
out:
	return ret;
}

static ssize_t acme_write(struct file *filp,
													const char __user *buf,
													size_t len,loff_t *offset){
	int ret=0, _blink_rate;
	char temp[30];
	if(!buf){
		ret = -EINVAL;
		goto out;
	}

	if(copy_from_user(temp,buf,len)){
		ret = -EFAULT;
		goto out;
	}
	kstrtoint(temp,10,&_blink_rate);
	
	if(_blink_rate <= 0){
		ret = -EINVAL;
		printk("Ignorring invalid blink_rate!\n");
		goto out;
	}
out:
	return ret;
}

static const struct file_operations acme_fops = {
	.owner 		= THIS_MODULE,
	.open 		= acme_open,
	.release 	= acme_close,
	.read 		= acme_read,		
	.write 		= acme_write,
};

static int rx_ring_init(struct pci_dev *pdev){
	struct acme_ring *rx_ring = &acme_devp->rx_ring;
	int i,err=-ENOMEM;
	
	rx_ring->count=ACME_RXD;

	rx_ring->size=rx_ring->count*sizeof(struct acme_rx_desc);

	rx_ring->desc=(struct acme_rx_desc *)dma_alloc_coherent(&pdev->dev,rx_ring->size,&rx_ring->dma,GFP_KERNEL);
	if(!rx_ring->desc)goto err;;

//*	
	for (i=0;i<rx_ring->count;++i) {
		rx_ring->wtf[i]=(u8*)dma_alloc_coherent(&pdev->dev,DESC_SIZE,&rx_ring->desc[i].buff_addr,GFP_KERNEL);
		if (!rx_ring->wtf[i]) goto err_pages;
	}
	//*/
	//*
	return 0;
err_pages:
	for (i=0;i<rx_ring->count;++i) {
		dma_free_coherent(&pdev->dev,DESC_SIZE,rx_ring->wtf[i],rx_ring->desc[i].buff_addr);
	}
	//*/
	return 0;
err:
	dma_free_coherent(&pdev->dev,rx_ring->size,rx_ring->desc,rx_ring->dma);
	return err;
}

static void free_rx_ring(struct pci_dev *pdev){
	int i;
	struct acme_ring *rx_ring=&acme_devp->rx_ring;
	//*
	for (i=0;i<rx_ring->count;++i) {
		dma_free_coherent(&pdev->dev,DESC_SIZE,rx_ring->wtf[i],rx_ring->desc[i].buff_addr);
	}
	//*/
	dma_free_coherent(&pdev->dev,rx_ring->size,rx_ring->desc,rx_ring->dma);
}

static int 
amce_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent){
	int bars, err;
	struct acme_ring *rx_ring=&acme_devp->rx_ring;
	resource_size_t mmio_start, mmio_len;

	printk(KERN_INFO "It's dangerous to go alone, take this with you.\n");

	acme_devp->pdev=pdev;

	err=pci_enable_device_mem(pdev);
	if(err)return err;

	bars=pci_select_bars(pdev, IORESOURCE_MEM);
	
	err=pci_request_selected_regions(pdev,bars,DEVNAME);
	if(err)goto err_pci_reg;
	
	pci_set_master(pdev);
	
	mmio_start = pci_resource_start(pdev, 0);
	mmio_len = pci_resource_len(pdev, 0);
	acme_devp->hw.hw_addr = ioremap(mmio_start, mmio_len);
	acme_devp->hw.led_addr=acme_devp->hw.hw_addr+LEDCTL; 
	writel(acme_devp->led_ctl,acme_devp->hw.led_addr);

	rx_ring_init(pdev);
//*	
	writel((rx_ring->dma>>32) & 0xFFFFFFFF,acme_devp->hw.hw_addr+RDBAH0);
	writel(rx_ring->dma & 0xFFFFFFFF,acme_devp->hw.hw_addr+RDBAL0);
	writel(DESC_SIZE*ACME_RXD,acme_devp->hw.hw_addr+RDLEN0);
//*/
	//*
	writel(0,acme_devp->hw.hw_addr+RDH0);
	writel(DESC_SIZE-1,acme_devp->hw.hw_addr+RDT0);

	rx_ring->next_to_use=0;
	rx_ring->next_to_clean=0;
	
	writel(0x01,acme_devp->hw.hw_addr+CTRL);
	//*/
//*
	//initialize the workqueue
	INIT_WORK(&acme_devp->task,service_task);

	//disable interrupts
//	WRITE_REG(ece_led,IMC,0xffffffff);
	writel(0xFFFFFFFF,acme_devp->hw.hw_addr+IMC);
//	FLUSH(ece_led);

	//global reset
//	WRITE_REG(ece_led,CTRL,(READ_REG(ece_led,CTRL) | CTRL_RST);
	writel((readl(acme_devp->hw.hw_addr+CTRL) | CTRL_RST),acme_devp->hw.hw_addr+CTRL);
//	FLUSH(ece_led);

	//wait for the PHY reset
	msleep(25);

	//disable interrupts again ater reset
//	WRITE_REG(ece_led,IMC,0xffffffff);
	writel(0xFFFFFFFF,acme_devp->hw.hw_addr+IMC);
//	FLUSH(ece_led);

	// needed for PCIe workaraounds - reserved chicken bits
	// write GCR bit 22, GCR2 bit 1
	//
//	WRITE_REG(ece_led,GCR,(READ_REG(ece_led,GCR) | (1<<22)));
	writel((readl(acme_devp->hw.hw_addr+GCR) | (1<<22)),acme_devp->hw.hw_addr+GCR);
//	WRITE_REG(ece_led,GCR2,(READ_REG(ece_led,GCR2) | 1));
	writel((readl(acme_devp->hw.hw_addr+GCR2) | 1),acme_devp->hw.hw_addr+GCR2);

	//needed for a forced PHY setup
	//PHY setups
//	WRITE_REG(ece_led,MDIC,0x1831af08);
	writel(0x1831af08,acme_devp->hw.hw_addr+MDIC);
	writel((readl(acme_devp->hw.hw_addr+RCTL) | UPE | MPE | BAM),acme_devp->hw.hw_addr+RCTL);
	//*/
	//*
	pci_enable_msi(pdev);
	err=request_irq(pdev->irq,acme_irq_handler,0,"acme_int",NULL);
	if(err){
		err=-EBUSY;
		goto err_irq;
	}
	//*/
	return 0;
err_irq:
	free_rx_ring(pdev);
	return err;
err_pci_reg:
	pci_disable_device(pdev);
	return err;
}

static void amce_pci_remove(struct pci_dev *pdev){
	//*
	free_irq(pdev->irq,NULL);
	pci_disable_msi(pdev);
	//cancel_work_sync(&acme_devp->task);
	//*/
	free_rx_ring(pdev);
	pci_release_selected_regions(pdev,pci_select_bars(pdev,IORESOURCE_MEM));
	//pci_release_mem_regions(pdev);
	pci_disable_device(pdev);
	printk(KERN_INFO "So long!!\n");
}

static DEFINE_PCI_DEVICE_TABLE(amce_pci_tbl) = {
	{ PCI_DEVICE(0x8086, 0x150c) },
	{ }, 
};

static struct pci_driver acme_pci_driver = {
	.name = DEVNAME,
	.id_table = amce_pci_tbl,
	.probe = amce_pci_probe,
	.remove = amce_pci_remove,
};

static int __init amce_init(void){
	acme_class = class_create(THIS_MODULE,DEVNAME);
	
	if(alloc_chrdev_region(&acme_dev_number,0,DEVCOUNT,DEVNAME) < 0) {
		printk(KERN_DEBUG "Can't register device\n"); 
		return -ENODEV;
	}
	
	acme_devp = kmalloc(sizeof(struct acme_dev), GFP_KERNEL);
	if(!acme_devp){
		printk("Bad Kmalloc\n"); 
		return -ENOMEM;
	}
	
	cdev_init(&acme_devp->cdev,&acme_fops);
	acme_devp->led_ctl=LED_OFF;
	if(blink_rate<0)return -EINVAL;
	if(blink_rate==0){
		printk("blink_rate > 0 required, resetting to default\n");
		blink_rate = BLINKRATE;
	}
	acme_devp->blink_rate=blink_rate;

	if(cdev_add(&acme_devp->cdev,acme_dev_number,DEVCOUNT)){
		printk(KERN_NOTICE "cdev_add() failed");
		unregister_chrdev_region(acme_dev_number,DEVCOUNT);
		return -1;
	}

	device_create(acme_class,NULL,acme_dev_number,NULL,DEVNAME);
	
	printk("ACME: character device %s loaded!\n",DEVNAME);
	
	return pci_register_driver(&acme_pci_driver);
}

static void __exit amce_exit(void){
//	WRITE_REG(ece_led,IMC,0xffffffff);
//	WRITE_REG(ece_led,RCTL,(READ_REG(ece_led,RCTL) & ~RCTL_EN);
	
	cdev_del(&acme_devp->cdev);
	unregister_chrdev_region(acme_dev_number,DEVCOUNT);
	kfree(acme_devp);
	device_destroy(acme_class,acme_dev_number);
	class_destroy(acme_class);
	pci_unregister_driver(&acme_pci_driver);
	printk("ACME: character device %s unloaded.\n",DEVNAME);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chad Coates");
MODULE_VERSION("0.5");

module_init(amce_init);
module_exit(amce_exit);
//*/
/*// e1000_tx_queue
 	
	do{
		buffer_info=&rx_ring->buffer_info[i];

		rx_desc=ACME_RX_DESC(*rx_ring,i);
		rx_desc->buff_addr=cpu_to_le64(rx_ring->buffer_info->dma);
		rx_desc->lower.data=cpu_to_le32(rxd_lower|buffer_info->length);
		rx_desc->upper.data=cpu_to_le32(rxd_upper);
		++i;
		if(i==rx_ring->count)i=0;
	}while(--count>0);

	rx_desc->lower.data |= cpu_to_le32(acme_devp->rxd_cmd);
	writel(i,acme_devp->hw.hw_addr+TDT);
//*/

