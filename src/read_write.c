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
#include <linux/semaphore.h>     /* Mutual exclusion semaphore. */

#include "read_write.h"        /* definitions */

int my_major = MY_MAJOR;
int my_minor = MY_MINOR;
int num_devices = NUM_DEVICES;
int rw_buff_size = RW_BUFF_SIZE;

module_param (my_major, int, S_IRUGO);
module_param (my_minor, int, S_IRUGO);
module_param (num_devices, int, S_IRUGO);
module_param (rw_buff_size, int, S_IRUGO);


/*
 * The devices.
 * See device_init() for initialization.
 */
static struct rw_dev *my_devices;


/*
 * Open the device.
 * 
 * -Check for device specific errors 
 *  (such as device-not-ready or similar hardware problems).
 * - Initialize the device if it is being opened for the first time.
 * - Update the f_ops pointer, if necessary.
 * - Allocate and fill any data structure to be put in filp->private_data.
 * 
 * @inode The inode refers to the device node found on the file system.
 * @filp  The file is a structure, created by the kernel, to hold the state
 *   	  of our use of the device within the device driver.
 */
int device_open (struct inode *inode, struct file *filp)
{
	// First thing is to identify which device is being opened.
	struct rw_dev *dev; 
	dev = container_of (inode->i_cdev, struct rw_dev, cdev); 
	filp->private_data = dev; /* For other methods. */

	if (down_interruptible (&dev->sem))
		return -ERESTARTSYS;	

	if (!dev->buffer) {
		dev->buffer = kmalloc (rw_buff_size, GFP_KERNEL);
		if (!dev->buffer) { // No memory. 
			up(&dev->sem);
			return -ENOMEM;
		}	
	}
	
	dev->buffer_size = rw_buff_size;
	dev->end = dev->buffer + dev->buffer_size;
	
	// Start reading and writing at beginning of circular buffer.
	dev->read_ptr = dev->write_ptr = dev->buffer; 

	/*
	 * Count readers and writers of this device.
	 * I used f_mode, and not f_flags because it's cleaner (fs/open.c tells why). 
	 */
	if (filp->f_mode & FMODE_READ)
		dev->num_readers++;
	if (filp->f_mode & FMODE_WRITE)
		dev->num_writers++;

	up (&dev->sem);
	return nonseekable_open(inode, filp);	
}


/*
 * Release the device (Reverse of open).
 * 
 * - Deallocate anything that open allocated in filp->private_data.
 * - Shut down device on last close.
 */
int device_release (struct inode *inode, struct file *filp)
{
	struct rw_dev *dev;
	dev = filp->private_data;
	
	down(&dev->sem);
	if (filp->f_mode & FMODE_READ)
		dev->num_readers--;
	if (filp->f_mode & FMODE_WRITE)
		dev->num_writers--;	
	
	if ((dev->num_readers + dev->num_writers) == 0) {
		kfree (dev->buffer);
		dev->buffer = NULL; // The other fields are not checked on open. 
	}
	up (&dev->sem);
	return 0;
}


/*
 * Read from the device.
 */
ssize_t device_read (struct file *filp, char __user *buf, size_t count, 
			loff_t *f_pos)
{
	/*
 	 * In the case of blocking operations, the following are the standard semantics:
	 * 
	 * If a process calls 'read' but no data is (yet) available, the process must
	 * block. The process is awakened as soon as some data arrives, and that data
	 * is returned to the caller, even if there is less than the ammount requested
    	 * 'count'.
 	 */ 	
	
	return count;
}


/* 
 * Write to the device.
 */ 
ssize_t device_write (struct file *filp, const char __user *buf, size_t count, 
			loff_t *f_pos)
{
	/* 
	 *  In the case of blocking operations, the following are the standard semantics: 	
	 * 
	 * If a process calls write and there is no space in the buffer, the process
	 * must block, and it must be on a different wait queue from the one used for 
	 * reading. When some data has been written to the hardware device, and space
	 * becomes fee in the output buffer, the process is awakened and the write
	 * call succeeds, although the data may be only partially written if there
	 * isn't room in the buffer for the 'count' bytes that were requested.
	 */

	return count;
}


/* 
 * Operations that can be perfomed on 
 * the device.
 */
struct file_operations fops = {
	.owner = THIS_MODULE,
	.open  = device_open,
	.release = device_release,
	.read  = device_read,
	.write = device_write,
};


/*
 * Sets up the provided cdev. 
 * 
 * @the_cdev The cdev to initialize. 
 * @the_index The index of the cdev in my_devices.
 */
static void setup_cdev (struct cdev *the_cdev, const int the_index)
{
	int result;
	int device;
	
	device = MKDEV (my_major, my_minor + the_index);
	cdev_init(the_cdev, &fops);
	the_cdev->owner = THIS_MODULE;
	the_cdev->ops   = &fops;
	result = cdev_add (the_cdev, device, 1);
	
	if (result) { 
		printk (KERN_NOTICE "Unable to register (%s%d) device.\n",
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
			cdev_del (&my_devices[i].cdev);	
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
	my_devices = kmalloc (num_devices * sizeof (struct rw_dev), GFP_KERNEL);
	if (!my_devices) {
		result = -ENOMEM;
		device_cleanup ();
		return result;	
	}	

	memset (my_devices, 0, num_devices * sizeof (struct rw_dev));

// SETUP DEVICES
	for (i = 0; i < num_devices; i++) {
		// Initialize the semaphore.  
		init_MUTEX (&my_devices[i].sem); 
		setup_cdev (&my_devices[i].cdev, i);	
	} 
	return 0;
}


module_init (device_init);
module_exit (device_cleanup);

MODULE_AUTHOR ("John Steele");
MODULE_LICENSE ("DUAL BSD/GPL");

