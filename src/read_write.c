#include <linux/module.h>
#include <linux/init.h>      
#include <linux/kernel.h>      /* printk() */
#include <linux/sched.h>       /* current */
#include <linux/wait.h>        /* wait_queue_head_t */
#include <linux/types.h>       /* size_t */
#include <linux/fs.h>          /* file_operations*/
#include <linux/errno.h>       /* error codes */
#include <linux/cdev.h>        /* cdev */
#include <linux/moduleparam.h> /* cmd args */
#include <asm/uaccess.h>       /* copy_*_user */

#include "read_write.h"        /* definitions */

int my_major = MY_MAJOR;
int my_minor = MY_MINOR;
int num_devices = NUM_DEVICES;

module_param (my_major, int, S_IRUGO);
module_param (my_minor, int, S_IRUGO);
module_param (num_devices, int, S_IRUGO);


/*
 * Dynacially allocated character devices.
 * See device_init() for initialization.
 */
static struct cdev *my_devices;


/*
 * Name of driver, also writen to /proc/devices.
 */
static char driver_name[] = "read_write_module";


/*
 * Read from the device.
 */
ssize_t device_read (struct file *filp, char __user *buf, size_t count, 
			loff_t *f_pos)
{
	return count;
}


/* 
 * Write to the device.
 */ 
ssize_t device_write (struct file *filp, char __user *buf, size_t count, 
			loff_t *f_pos)
{
	return count;
}


/*
 * Sets up the provided cdev. 
 * 
 * @the_cdev The cdev to initialize. 
 * @the_index The index of the cdev in my_devices.
 */
static void setup_cdev (struct cdev *the_cdev, const int the_index)
{
	int result, i;
	int device;
	
	device = MKDEV (my_major, my_minor + the_index);
	cdev_init(the_cdev, &fops);
	the_cdev->owner = THIS_MODULE;
	the_cdev->ops   = &fops;
	result = cdev_add (the_cdev, device, 1);
	
	if (result) { 
		prink (KERN_NOTICE "Unable to register (%s%d) device.\n",
				driver_name, the_index); 
 	}
}


/* 
 * Cleans up resources. 
 */ 
static void  __exit device_cleanup (void) 
{
	int i;
	if (my_devices) {
		for (i = 0; i < num_devices; i++) {
			cdev_del (&my_device[i]);	
		}	
		kfree (my_devices);
	}

	unregister_chrdev_region (MKDEV (my_major, my_minor), num_devices);
}


/*
 * Initialize this device driver.
 */ 
static int __init device_init (void)
{
	int result, i;
	dev_t dev;

// GET MAJOR	
	if (my_major) {
		dev = MKDEV (my_major, my_minor);
		result = register_chrdev_region (dev, num_devices, driver_name); 
	} else {
		result = alloc_chrdev_region (&dev, my_minor, num_devices, driver_name);
		my_major = MAJOR (dev);
	}

	if (result < 0) {
		printk (KERN_NOTICE "%s can't get major.\n", driver_name);
		return result;
	} 	

// ALLOCATE DEVICES
 	my_devices = kmalloc (num_devices * sizeof (struct cdev), GFP_KERNEL);
	if (!my_devices) {
		result = -ENOMEM;
		device_cleanup ();
		return result;	
	}	

	memset (my_devices, 0, num_devices * sizeof (struct cdev));

// SETUP DEVICES
	for (i = 0; i < num_devices; i++) {
		setup_cdev (&my_devices[i], i);	
	}

	return 0;
}


module_init (device_init);
module_exit (device_exit);

MOUDLE_AUTHOR ("John Steele");
MODULE_LICENSE ("DUAL BSD/GPL");

