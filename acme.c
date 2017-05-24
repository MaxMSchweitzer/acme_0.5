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
//struct *acme_devp;
static int blink_rate=BLINKRATE;

module_param(blink_rate,int,S_IRUGO|S_IWUSR);

static void acme_timer_cb(unsigned long data){
	acme_devp->led_ctl = readl(acme_devp->hw.led_addr);	
	acme_devp->led_ctl=(acme_devp->led_ctl==LED_OFF) ? LED_ON : LED_OFF;
	writel(acme_devp->led_ctl,acme_devp->hw.led_addr);
	mod_timer(&acme_timer,(jiffies+(HZ/acme_devp->blink_rate)/2));
}

static int acme_open(struct inode *inode,struct file *filp){
	int  err=0;
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
	int ret, _blink_rate;
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

static int descriptor_init(struct pci_dev *pdev){
	struct acme_ring *rx_ring = acme_devp->rx_ring;
	int i=0;
/*
	if(!rx_ring->count)rx_ring->count=ACME_RXD;

	rx_ring->buffer_info=kcalloc(rx_ring->count,sizeof(struct acme_buffer),GFP_KERNEL);

	if (!rx_ring->buffer_info) {
	//	ret_val = 5;
		goto err_nomem;
	}
	
	rx_ring->size = rx_ring->count * sizeof(struct acme_desc);
	rx_ring->desc = dma_alloc_coherent(&pdev, rx_ring->size,
									   &rx_ring->dma, GFP_KERNEL);

	if (!rx_ring->desc) {
		ret_val = 6;
		goto err_nomem;
	}
	rx_ring->next_to_use = 0;
	rx_ring->next_to_clean = 0;
*/

	do{
		acme_devp->rx_desc[i]=ACME_RX_DESC(*rx_ring,i);
		acme_devp->rx_desc[i]->buffer_addr=cpu_le64(rx_ring->buffer_info->dma);
//		acme_devp->rx_desc[i]->lower.data=cpu_to_le32(rxd_lower|buffer_info->length);
//		acme_devp->rx_desc[i]->upper.data=cpu_to_le32(rxd_upper);
		++i;
		if(i==rx_ring->count)i=0;
	}while(--count>0);



	return 0;
}

static int descriptor_free(void){
	int i=0;
	for (;i<ACME_RXD;++i){
//		kfree(desc[i].buff_addr);
	}
	return 0;
}

static int 
amce_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent){
	int bars, err;
	resource_size_t mmio_start, mmio_len;

	printk(KERN_INFO "It's dangerous to go alone, take this with you.\n");
	
	err=pci_enable_device_mem(pdev);
if(err)return err;

	bars=pci_select_bars(pdev, IORESOURCE_MEM);
	
	err=pci_request_selected_regions(pdev,bars,DEVNAME);
	if(err)goto err_pci_reg;
	descriptor_init();
	pci_set_master(pdev);
	
	mmio_start = pci_resource_start(pdev, 0);
	mmio_len = pci_resource_len(pdev, 0);
	acme_devp->hw.hw_addr = ioremap(mmio_start, mmio_len);
	acme_devp->hw.led_addr=acme_devp->hw.hw_addr+LEDCTL; 
	writel(acme_devp->led_ctl,acme_devp->hw.led_addr);
err_pci_reg:
	pci_disable_device(pdev);
	return 0;
}

static void amce_pci_remove(struct pci_dev *pdev){
	descriptor_free();
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
