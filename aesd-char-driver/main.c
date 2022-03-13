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

#include <linux/slab.h>		// for kmalloc and kfree
// #endif
#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
//#include <stdio.h>
#endif

#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Sanish Kharade"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

char *pointer;

int aesd_open(struct inode *inode, struct file *filp)
{
	PDEBUG("open\n\n");
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
	PDEBUG("release\n\n");
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

	struct aesd_buffer_entry *read_entry = NULL;
	ssize_t read_offset = 0;
	struct aesd_dev *dev;
	dev = (struct aesd_dev *) filp->private_data;
	// offset
	// when count > number of bytes left in current entry, bytes read = size - offset - 1
	// user should update fpos = fpos + bytes read

	// else bytes read = count
	PDEBUG("Buffer status = %d, in_offset = %d, out_offset = %d\n",dev->aesd_cbuf.full, dev->aesd_cbuf.in_offs, dev->aesd_cbuf.out_offs);
	// if( (dev->aesd_cbuf.in_offs == dev->aesd_cbuf.out_offs) && (dev->aesd_cbuf.full != true) )
	// {
	// 	// buffer is empty
	// 	PDEBUG("Buffer is empty\n");
	// 	return 0;
	// }
	if(dev->aesd_cbuf.empty == true )
	{
		// buffer is empty
		PDEBUG("Buffer is empty\n");
		return 0;
	}
	read_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->aesd_cbuf), *f_pos, &read_offset);
	if(read_entry != NULL)
	{
		PDEBUG("entry = %s, read_offset = %zu\n", read_entry->buffptr, read_offset);
	}
	size_t rem_bytes = read_entry->size - read_offset;
	if(count > rem_bytes)
	{
		retval = rem_bytes;
	}
	else
	{
		retval = count;
	}
	size_t status = __copy_to_user(buf, (void*)(read_entry->buffptr + read_offset), retval);
	if(status != 0)
	{
		PDEBUG("Unable to copy all bytes to user\n");
	}
	else
	{
		PDEBUG("Copied %ld bytes to user\n", retval);
	}
	dev->aesd_cbuf.out_offs = (dev->aesd_cbuf.out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
	if(dev->aesd_cbuf.out_offs == dev->aesd_cbuf.in_offs)
	{
		dev->aesd_cbuf.empty = true;
	}
	else
	{
		dev->aesd_cbuf.empty = false;
	}
	//*f_pos += retval;
	/**
	 * TODO: handle read
	 * filep has the private_data member setup to aesd_dev by the open function
	 * Thus we can access pointer to aesd_dev here
	 * 
	 * Since buf is a user space buffer we need to use copy_to_user to access it
	 */


	PDEBUG("End of aesd_read function\n\n");
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
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	PDEBUG("Start of aesd_write function\n");
	ssize_t retval = -ENOMEM;
	PDEBUG("buf = %s, count = %zu, fpos = %lld\n", buf, count, *f_pos);
	char *overwritten_buf = NULL;
	
	static size_t total_count;

	struct aesd_dev *dev;
	dev = (struct aesd_dev *) filp->private_data;
	
	// change to bool later
	static uint8_t write_complete = 1;

	total_count += count;
	if(write_complete)
	{
		dev->aesd_cb_entry.buffptr = (char*)kmalloc(count+1, GFP_KERNEL);
		if(dev->aesd_cb_entry.buffptr == NULL)
		{
			// handle error
			PDEBUG("Unable to malloc\n");
			return retval;
		}
		// kmalloc count bytes, add sizeof(char)
		// pointer = (char*)kmalloc(count, GFP_KERNEL);
		// if(pointer == NULL)
		// {
		// 	// handle error
		// 	PDEBUG("Unable to malloc\n");
		// 	return retval;
		// }
		//memset(dev->aesd_cb_entry.buffptr, 0, count);

		// use copy_from_user to copy data into kernel
		size_t status = __copy_from_user((void*)(dev->aesd_cb_entry.buffptr), buf, count);
		if(status != 0)
		{
			PDEBUG("Unable to copy all bytes into kernel\n");
		}
		else
		{
			PDEBUG("Copied %ld bytes into the kernel\n", count-status);
		}
	}
	else
	{
		dev->aesd_cb_entry.buffptr = (char*)krealloc(dev->aesd_cb_entry.buffptr, total_count, GFP_KERNEL);
		if(dev->aesd_cb_entry.buffptr == NULL)
		{
			// handle error
			PDEBUG("Unable to kmalloc\n");
			return retval;
		}
		// kmalloc count bytes, add sizeof(char)
		// pointer = (char*)krealloc(pointer, total_count, GFP_KERNEL);
		// if(pointer == NULL)
		// {
		// 	// handle error
		// 	PDEBUG("Unable to malloc\n");
		// 	return retval;
		// }
		//memset(pointer, 0, count);

		// use copy_from_user to copy data into kernel
		size_t status = __copy_from_user((void*)(dev->aesd_cb_entry.buffptr + total_count - count), buf, count);
		if(status != 0)
		{
			PDEBUG("Unable to copy all bytes into kernel\n");
		}
		else
		{
			PDEBUG("Copied %ld bytes into the kernel\n", count-status);
		}
		
	}
	memcpy(dev->aesd_cb_entry.buffptr + total_count, "\0", 1);
	if(memchr(dev->aesd_cb_entry.buffptr, '\n', total_count) != NULL)
	{
		//pointer[total_count] = '\0';
		dev->aesd_cb_entry.size = total_count;
		total_count = 0;
		write_complete = 1;
		// add to buffer
		overwritten_buf = aesd_circular_buffer_add_entry(&(dev->aesd_cbuf),&(dev->aesd_cb_entry));
		if(overwritten_buf != NULL)
		{
			PDEBUG("Freeing : %s\n", overwritten_buf);
			kfree(overwritten_buf);
		}
	}
	else
	{
		write_complete = 0;
	}
	//PDEBUG("pointer = %s, total_count = %zu, count = %zu, write_complete = %d\n", pointer, total_count, count, write_complete);

	int j = 0;
	for(j = 0; j < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; j++)
	{
		PDEBUG("entry %d = %s", j, dev->aesd_cbuf.entry[j].buffptr);
	}



/*
	// Basic write test
	// kmalloc count bytes
	pointer = kmalloc(count, GFP_KERNEL);
	if(pointer == NULL)
	{
		// handle error
		PDEBUG("Unable to malloc\n");
		return retval;
	}
	memset(pointer, 0, count);

	// use copy_from_user to copy data into kernel
	size_t status = __copy_from_user(pointer, buf, count);
	if(status != 0)
	{
		PDEBUG("Unable to copy all bytes into kernel\n");
	}
	else
	{
		PDEBUG("Copied %ld bytes into the kernel\n", count-status);
	}

*/
	//


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
// if found put ot a write 

	PDEBUG("End of aesd_write function\n\n");
	return count;
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
	uint8_t index;
	struct aesd_buffer_entry *entry;
	AESD_CIRCULAR_BUFFER_FOREACH(entry, &(aesd_device.aesd_cbuf), index)
  	{
		if(entry->buffptr != NULL)
		{
			kfree(entry->buffptr);
		}
  	}
	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
