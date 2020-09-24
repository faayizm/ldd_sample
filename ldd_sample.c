#include<linux/module.h>
#include<linux/proc_fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include <linux/uaccess.h>
#define BUFF_MAX_SIZE (1024*1024)//1KB
/** Dynamic major number enabled */
//#define MAJOR_DYNAMIC 

#ifndef MAJOR_DYNAMIC
#define DEV_MAJOR 20
#define DEV_MINOR 32
#endif
struct cdev *cdev;
struct class *class;
struct device *dev;
struct priv_data
{
	int a;
	//private data goes here
};
unsigned char dev_buff[BUFF_MAX_SIZE];
int devn;

	static ssize_t
write_handler(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	pr_info("write requested %d bytes", count);
	pr_info("Current file position %lld \n", *f_pos);

	/** Adjust the count */
	if((*f_pos + count) > BUFF_MAX_SIZE)
	{
		count = BUFF_MAX_SIZE - *f_pos;
	}
	if(!count)
	{
		return -ENOMEM;
	}
	/** Copy from user */
	if(copy_from_user(&dev_buff[*f_pos], buf, count))
	{
		return -EFAULT;
	}
	/** Update the file position */
	*f_pos += count;

	pr_info("Number of bytes written %d ", count);
	pr_info("Updated file position %lld \n", *f_pos);
	return count;
}
	static ssize_t
read_handler(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	pr_info("Read requested %d bytes", count);
	pr_info("Current file position %lld \n", *f_pos);
	/** Adjust the count */
	if((*f_pos + count) > BUFF_MAX_SIZE)
	{
		count = BUFF_MAX_SIZE - *f_pos;
	}
	/** Copy to user */
	if(copy_to_user(buf, &dev_buff[*f_pos], count))
	{
		return -EFAULT;
	}
	/** Update the file position */
	*f_pos += count;

	pr_info("Number of bytes read %d ", count);
	pr_info("Updated file position %lld \n", *f_pos);
	return count;
}
static int
device_open(struct inode *inode, struct file *file)
{
	pr_info("Device opened succesfully");
	return 0;
}

	static int
device_release(struct inode *inode, struct file *file)
{
	pr_info("Device Closed succesfully");
	return 0;
}

struct file_operations Fops = {
	.open = device_open,
	.release = device_release,
	.read = read_handler,
	.write = write_handler,
};

static int __init ldd_init(void)
{
	int ret;
	{
#ifdef MAJOR_DYNAMIC
		ret = alloc_chrdev_region(&devn, 0, 1, "sample_dev_num");
		if (ret) {
			printk("failed to allocate chrdev region err=%d\n", ret);
			goto clean_exit;
		}
#else
		ret = register_chrdev(DEV_MAJOR, "chr_dev", &Fops);
		if (ret < 0)
		{
			pr_err("failed to register char drv %d\n", ret);
			goto clean_exit;
		}
		devn = MKDEV(DEV_MAJOR, DEV_MINOR);
#endif
		printk("alloc_chrdev_region <major>:<minor> = %d:%d\n", MAJOR(devn), MINOR(devn));


		class = class_create(THIS_MODULE, "chr_dev");
		if (!class) {
			ret = -ENOMEM;
			printk("failed to create device class err=%d\n", ret);
			goto clean_chrdev;
		}
		dev = device_create(class, 0, devn, 0, "chr_dev");
		if (!dev) {
			ret = -ENOMEM;
			printk("failed to create device err=%d\n", ret);
			goto clean_class;
		}

		cdev = cdev_alloc();
		cdev_init(cdev, &Fops);
		cdev->owner = THIS_MODULE;
		cdev->ops = &Fops;
		ret = cdev_add(cdev, devn, 1);
		if (ret) {
			printk(KERN_NOTICE "error %d failed to add sample chrdev", ret);
			goto clean_dev;
		}

	}
	printk("Module loaded succesfully\n");
	return 0;
clean_dev:
	if (dev) {
		printk("device_destroy\n");
		device_destroy(class, devn);
	}
clean_class:
	if (class) {
		printk("class_destroy\n");
		class_destroy(class);
	}
clean_chrdev:
	printk("unregister_chrdev_region 0x%08x\n", devn);
	unregister_chrdev_region(devn, 1);
clean_exit:
	printk("Module loading failed");
	return ret;
}
static void ldd_exit(void)
{
	if (cdev) {
		printk("cdev_del\n");
		cdev_del(cdev);
	}

	if (dev) {
		printk("device_destroy\n");
		device_destroy(class, devn);
	}
	if (class) {
		printk("class_destroy\n");
		class_destroy(class);
	}
#ifdef MAJOR_DYNAMIC
	printk("unregister_chrdev_region 0x%08x\n", devn);
	unregister_chrdev_region(devn, 1);
#else
	printk("unregister_chrdev 0x%08x\n", devn);
	unregister_chrdev(DEV_MAJOR, "chr_dev");
#endif

	printk("LDD unloaded successfully\n");
}
module_init(ldd_init);
module_exit(ldd_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Faayiz M");
MODULE_DESCRIPTION("Simple character device driver");
