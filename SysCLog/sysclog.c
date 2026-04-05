#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/types.h>


#define DEVICE_FIRST 0
#define DEVICE_COUNT 1
#define DGROUP_NAME "gpio device"
#define DEVICE_NAME "syslog"

#define DEVICE_OPENED 1
#define DEVICE_CLOSED 0


static int major = 0;
static struct cdev syslog_dev;
static struct class* cl;


static int  device_status;

static int __init syslog_init(void);
static void __exit syslog_exit(void);

static int syslog_open(struct inode* n, struct file *f);
static int syslog_release(struct inode *n, struct file *f);
static ssize_t syslog_read(struct file *f, char __user *buffer, size_t count, loff_t *ppos);


static const struct file_operations syslog_fops ={
	.owner = THIS_MODULE,
	.open = syslog_open,
	.release = syslog_release,
	.read = syslog_read,
};


module_init(syslog_init);
module_exit(syslog_exit);


static int __init syslog_init(void){

	int result = 0;
	device_status = DEVICE_CLOSED;


	printk(KERN_INFO "<syslog>: Trying to alloc device region...\n");

	dev_t dev = 0;
	result = alloc_chrdev_region(&dev, DEVICE_FIRST, DEVICE_COUNT, DGROUP_NAME);
	major = MAJOR(dev);
	
	if (result < 0){
		printk(KERN_ERR "<syslog>: Cat not alloc device region!\n");
			
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);

		goto err;
	}
	printk(KERN_INFO "<syslog>: Device region created: %d:%d...%d\n", major, DEVICE_FIRST, DEVICE_COUNT);
	
		
	printk(KERN_INFO "<syslog>: Creating device class...\n");
	cl = class_create("syscals_logging");
	if (NULL == cl){
		printk(KERN_ERR "<syslog>: Can not create device class!\n");
		
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);

		result = -1;
		goto err;
	}	
	printk(KERN_INFO "<syslog>: Device class created\n");
	

	printk(KERN_INFO "<syslog>: Creating device...\n");
	if (NULL == device_create(cl, NULL, MKDEV(major, DEVICE_FIRST), NULL, DEVICE_NAME)){
		printk(KERN_ERR "<syslog>: Can not create device!\n");
		
		class_destroy(cl);
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
	
		result = -1;	
		goto err;
		}
	
	cdev_init(&syslog_dev, &syslog_fops);
	printk(KERN_INFO "<syslog>: Adding device...\n");
	
	result = cdev_add(&syslog_dev, MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
	if (result < 0){
		printk(KERN_ERR "<syslog>: Can not add device!\n");
		
		device_destroy(cl, MKDEV(major, DEVICE_FIRST));
		class_destroy(cl);
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);

		goto err;

	}
	printk(KERN_INFO "<syslog>: Device added\n");
	

err:
	return result;
}


static void __exit syslog_exit(void){
	
	cdev_del(&syslog_dev);
	device_destroy(cl, MKDEV(major, DEVICE_FIRST));
	class_destroy(cl);
	unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);


	return;
}


static int syslog_open(struct inode* n, struct file *f){
	if (device_status == DEVICE_OPENED){
		printk(KERN_ERR "<syslog>: Device is already opened\n");
		return -EBUSY;
	}
	device_status = DEVICE_OPENED;

	return 0;
}

static int syslog_release(struct inode *n, struct file *f){
	device_status = DEVICE_CLOSED;
	return 0;
}


static ssize_t syslog_read(struct file *f, char __user *buffer, size_t count, loff_t *ppos){

	return 0;

}





MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roman Opalyuk");
