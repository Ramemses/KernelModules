#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>


#define DEVICE_FIRST 0
#define DEVICE_COUNT 1
#define DGROUP_NAME "custom_null_driver"
#define MAX_LOG_SIZE 100

static int major = 0;



static int null2_open(struct inode *inode, struct file *file);
static int null2_release(struct inode* inode, struct file *file);
static ssize_t null2_read(struct file* file,  char __user *user_buffer, size_t size, loff_t *offset);
static ssize_t null2_write(struct file *file, const char __user *user_buffer, size_t size, loff_t * offset);


static struct cdev my_dev;


static const struct file_operations null2_fops ={
	.owner = THIS_MODULE,
	.open = null2_open,
	.release = null2_release,
	.read = null2_read,
 	.write = null2_write,
};

#define EOK 0
static int device_open = 0;



static int __init null2_init(void);
static void __exit null2_exit(void);

module_init(null2_init);
module_exit(null2_exit);



static int __init null2_init(void){
	int result = 0;
	
	printk(KERN_INFO "Trying to register <null2> device region...\n");
	dev_t dev = 0;
	
	result = alloc_chrdev_region(&dev, DEVICE_FIRST, DEVICE_COUNT, DGROUP_NAME);
	major = MAJOR(dev);
	
	if (result < 0){
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
		printk(KERN_ERR "Can not register <null2> device region\n");
		
		goto err;
	}

	printk(KERN_INFO "<null2> device region created: %d:%d...%d\n", major, DEVICE_FIRST, DEVICE_COUNT);
	
	cdev_init(&my_dev, &null2_fops);
	my_dev.owner = THIS_MODULE;
	
	result = cdev_add(&my_dev, dev, DEVICE_COUNT);
	if (result < 0){
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
		printk(KERN_ERR "Can not add <null2> device\n");

		goto err;
	}	

err:
	return result;
}


static void __exit null2_exit(void){
	cdev_del(&my_dev);
	printk(KERN_INFO "null2: Device removed\n");

	unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
	printk(KERN_INFO "Char device region %d:%d...%d destroyed\n", major, DEVICE_FIRST, DEVICE_COUNT);

}


static int null2_open(struct inode *inode, struct file *file){
	if (device_open){
		printk(KERN_INFO "Device <null2> is already opened somwhere.\n");
		return -EBUSY;
	}
	device_open++;
	printk(KERN_INFO "null2: Device open\n");
	return EOK;
}


static int null2_release(struct inode* inode, struct file *file){
	device_open--;
	printk(KERN_INFO "null2: Device closed\n");
	return EOK;
}


static ssize_t null2_read(struct file *file, char __user *user_buffer, size_t size, loff_t* offset){
	const char *reply = "This is a null2 driver for redirect some text.\n";
	size_t len = strlen(reply);
	
	if (*offset >= len) return 0;
	if (size > len - *offset) size = len - *offset;

	if (copy_to_user(user_buffer, reply, len)) return -EINVAL;

	*offset += size;
	return size;
}

static ssize_t null2_write(struct file *file, const char __user *user_buffer, size_t size, loff_t * offset){

	char* kernel_buf;
	size_t log_size;

	if (size == 0) return 0;

	kernel_buf = kmalloc(size+1, GFP_KERNEL);
	if (!kernel_buf)  return -ENOMEM;

	if (copy_from_user(kernel_buf, user_buffer, size)){
		kfree(kernel_buf);
		return -EFAULT;
	}

	kernel_buf[size]='\0';
		
	log_size = (size > MAX_LOG_SIZE) ? MAX_LOG_SIZE : size;

	printk(KERN_INFO "null2: recieved %zu bytes: '%.*s'\n", size, (int)log_size, kernel_buf);


	kfree(kernel_buf);
	return size;
}



MODULE_LICENSE("GPL");
MODULE_AUTHOR("User: roman");
