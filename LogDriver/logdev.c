#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/namei.h>
#include <linux/fsnotify.h>
#include <linux/fsnotify_backend.h>
#include <linux/err.h>
#include <linux/jiffies.h>

#define DEVICE_FIRST 0
#define DEVICE_COUNT 1
#define DGROUP_NAME "my-log-interface"

static unsigned long last_event_jiffies = 0;
#define DEBOUNCE_INTERVAL_MS 500

static char filename[] = "/var/logdev.txt";

static struct path path;
static struct fsnotify_group *group;

struct inode *target_inode;
struct fsnotify_mark *mark;

static int major = 0;
static struct cdev my_dev;


static int handle_openfile(struct fsnotify_group *group, u32 mask, 
                            const void *data, int data_type, struct inode *inode,
                            const struct qstr *name, u32 cookie,
                            struct fsnotify_iter_info *iter_info);



static struct fsnotify_ops log_ops={
	.handle_event = handle_openfile,
};


static struct file_operations log_fops ={
	.owner = THIS_MODULE,
};


static int __init log_init(void);
static void __exit log_exit(void);

module_init(log_init);
module_exit(log_exit);


static int __init log_init(void){
	int result = 0;

	printk(KERN_INFO "<logdev>: Trying to register device region...\n");
	dev_t dev = 0;
	result = alloc_chrdev_region(&dev, DEVICE_FIRST, DEVICE_COUNT, DGROUP_NAME);
	major = MAJOR(dev);

	if (result < 0){
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
		printk(KERN_ERR "<logdev>: Can not register device region\n");
		
		goto err;
	}

	printk(KERN_INFO "<logdev>: Device region created %d:%d...%d\n", major, DEVICE_FIRST, DEVICE_COUNT);
	
	cdev_init(&my_dev, &log_fops);
	my_dev.owner = THIS_MODULE;
	
	result = cdev_add(&my_dev, dev, DEVICE_COUNT);	
	if (result < 0){
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
		printk(KERN_ERR "<logdev>: Can not add device\n");

		goto err;
	}

	printk(KERN_INFO "<logdev>: Device added\n");	

	
	printk(KERN_INFO "<logdev>: Trying to create path to file <%s>...\n", filename);
	result = kern_path(filename, LOOKUP_FOLLOW, &path);
	if (result < 0){
		cdev_del(&my_dev);
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
		printk(KERN_ERR "<logdev>: Can not find file <%s>", filename);		
		goto err;
	}
	printk(KERN_INFO "<logdev>: Path to file created\n");	


	printk(KERN_INFO "<logdev>: Trying to create group\n");
	group = fsnotify_alloc_group(&log_ops, 0);
	if (IS_ERR(group)){
		result=-1;
		path_put(&path);
		cdev_del(&my_dev);
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
		printk(KERN_ERR "<logdev>: Can not create group\n");
		goto err;
	}
	printk(KERN_INFO "<logdev>: Group created\n");

	target_inode = path.dentry->d_inode;
	mark = kmalloc(sizeof(*mark), GFP_KERNEL);
	if (!mark){
		result = ENOMEM;
		fsnotify_put_group(group);
		path_put(&path);
		printk(KERN_ERR "<logdev>: Can not allocate memory for mark\n");
		goto err;
	}
	fsnotify_init_mark(mark, group);
	mark->mask = FS_CLOSE_WRITE;

	printk(KERN_INFO "<logdev>: Mark created\n");
	printk(KERN_INFO "<logdev>: Mark pointer: %p\n", mark);	


	result = fsnotify_add_inode_mark(mark, target_inode, 0);
	if (result){	
		kfree(mark);
		fsnotify_put_group(group);
		path_put(&path);
		cdev_del(&my_dev);
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
		printk(KERN_ERR "<logdev>: Can not add inode mark");
	
		goto err;
	}
	path_put(&path);
	printk(KERN_INFO "<logdev>: Inode mark added\n");

err:
	return result;
}


static void __exit log_exit(void){

	if (mark){
		fsnotify_put_mark(mark);
		mark = NULL;
	
		
		printk(KERN_INFO "<logdev>: Mark deleted\n");
	}

	fsnotify_put_group(group);
	printk(KERN_INFO "<logdev>: Group deleted\n");

	cdev_del(&my_dev);
	printk(KERN_INFO "<logdev>: Device removed\n");

	unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
	printk(KERN_INFO "<logdev>: Device region %d:%d...%d destroyed\n", major, DEVICE_FIRST, DEVICE_COUNT);

}

#define BUF_SIZE 1000
static char buff[BUF_SIZE];


static int handle_openfile(struct fsnotify_group *group, u32 mask, 
                            const void *data, int data_type, struct inode *inode,
                            const struct qstr *name, u32 cookie,
                            struct fsnotify_iter_info *iter_info)
{
		
	int n = 0;
	unsigned long current_jiffies = jiffies;
    unsigned long debounce_interval = msecs_to_jiffies(DEBOUNCE_INTERVAL_MS);

    if (mask & FS_CLOSE_WRITE) {
        if (time_after(current_jiffies, last_event_jiffies + debounce_interval)) {
            printk(KERN_INFO "<logdev>: File %s was written and closed\n", filename);
            last_event_jiffies = current_jiffies;


			struct file *f;
			f = filp_open(filename, O_RDONLY, 0);
			
    		n = kernel_read(f, buff, BUF_SIZE, 0); 
    		if(n) { 
    		    printk(KERN_ERR "<logdev>: read first %d bytes:\n", n ); 
       			buff[n] = '\0'; 
        		printk("%s\n", buff);; 
    		} else { 
        		printk(KERN_ERR  "<logdev>: kernel_read failed\n" ); 
        		return -EIO; 
    		}

 			filp_close(f, NULL);
        }    
    }

    return 0;
}


MODULE_LICENSE("GPL");
