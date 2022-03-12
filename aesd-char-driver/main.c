/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */


// #ifdef __KERNEL__
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
// #endif

#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Sanish Kharade"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	PDEBUG("open");
	/**
	 * TODO: handle open
	 * 
	 * Set filep->private_data to aesd_dev struct
	 * To get a pointer to aesd_dev struct use inode->i_cdev with container_of
	 * Now filep has the private_data member setup to aesd_dev and can be used in other functions
	 */

	/*
	 *	To get the pointer of aesd_dev structure we need to use container_of MACRO
	 *	Ref LDD3 pg 58 
	*/
	struct aesd_dev *dev;

	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);

	filp->private_data = dev;
	
	return 0;

}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release2");
	/**
	 * TODO: handle release
	 */
	return 0;
}
/**
 *	@name 	: aesd_read
 *
 *	@param	
 *			: count		- number of bytes to read
						- OR max number of bytes to write to buf
 *			: buf		- buffer from user space that we are going to fill
 *			: f_pos 	- pointer to the read offset
 *						  references a specific byte (char_offset) of the circular buffer linear content
 * 						  See aesd_circular_buffer_find_entry_offset_for_fpos for char_offset
 * 	
 *  @return : ssize_t	- number of bytes read
 * 
 * 	@note 	: Do appropriate locking in this function
 * 			  See partial read rule

*/
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	PDEBUG("Start of aesd_read function\n");
	ssize_t retval = 0;
	//PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	PDEBUG("buf = %s, count = %zu, fpos = %lld\n", buf, count, *f_pos);
	/**
	 * TODO: handle read
	 * filep has the private_data member setup to aesd_dev by the open function
	 * Thus we can access pointer to aesd_dev here
	 * 
	 * Since buf is a user space buffer we need to use copy_to_user to access it
	 */

	PDEBUG("End of aesd_read function\n");
	return retval;
}
/**
 *	@name 	: aesd_write
 *
 *	@param	: f_pos 	- typically location to write at
						- A8 will ignore this and follow previous rules of new/ongoing buffer
 *			: count		- number of bytes to write
 * 	
 *  @return : ssize_t	- number of bytes written
 * 
 * 	@note 	: 	Do appropriate locking in this function
 * 
 * 				Any write operation that doesn't end with a \n must not increment the offset pointer
 * 				Next write should happen in the same buffer location until a \n is received
 * 
 * 				If buffer is full we need to free the memory assigned for the oldest command before 
 * 				assigning new memory for the latest command and overwriting the oldest one
*/
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	PDEBUG("Start of aesd_write function\n");
	//ssize_t retval = -ENOMEM;
	PDEBUG("buf = %s, count = %zu, fpos = %lld\n", buf, count, *f_pos);
	//ssize_t retval =  2;
	//PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle write
	 */
	/*
	 *	go through string and find \n
	 *	buf[count] = \n ??
	 *	kmalloc(count)
	*/
	/// go through string and fin \n
	// buf[count] = \n
	// kmalloc(count)
	// copy_frm user
	// src and dest
// if found put ot a weite 

	PDEBUG("End of aesd_write function\n");
	return 2;
	//return 2;	
}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}



int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	PDEBUG("Start of aesd_init_module function\n");

	/*
	 *	int alloc_chrdev_region(dev_t * dev, unsigned baseminor, unsigned count, const char * name);
	 * 	Dynamic allocation of major number
	*/
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");

	// Get major number from dev
	aesd_major = MAJOR(dev);
	if (result < 0)
	{
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/**
	 * TODO: initialize the AESD specific portion of the device
	 * 
	 * Initialize locking primitive
	 */
	aesd_circular_buffer_init(&(aesd_device.aesd_cbuf));

	result = aesd_setup_cdev(&aesd_device);
	if( result ) {
		unregister_chrdev_region(dev, 1);
	}

	PDEBUG("End of aesd_init_module function\n");
	return result;

}

void aesd_cleanup_module(void)
{
	// Get the dev number from major and minor numbers
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 * 
	 * free memory associated with filp->private_data
	 * Uninitialize everything
	 * 
	 */

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
