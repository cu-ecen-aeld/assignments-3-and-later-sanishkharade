/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes, Modified by Sanish Kharade
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

//char *pointer;
void debug_print_string(char *s, size_t n);

// To print strings without \0 at the end
void debug_print_string(char *s, size_t n)
{	
	if(s == NULL)
	{
		printk(KERN_CONT "(null)\n");
	}
	else
	{
		int i = 0;
		for(i = 0; i < n; i++)
		{
			printk(KERN_CONT "%c", s[i]);
		}
	}

}

// This function is called whenever a file refereing to this driver is opened
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
// This function is called whenever a file refereing to this driver is released
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
						- (OR) max number of bytes to write to buf
 *			: buf		- buffer from user space that this function will fill
 *			: f_pos 	- pointer to the read offset
 *						  references a specific byte (char_offset) of the circular buffer linear content
 * 						  See aesd_circular_buffer_find_entry_offset_for_fpos for char_offset
 * 	
 *  @return : ssize_t	- number of bytes read
 * 
 * 	@note 	: Do appropriate locking in this function
 * 			  This function is using the partial read rule

*/
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	PDEBUG("Start of aesd_read function\n");
	ssize_t bytes_read = 0;

	//PDEBUG("buf = %s, count = %zu, fpos = %lld\n", buf, count, *f_pos);

	struct aesd_buffer_entry *read_entry = NULL;
	ssize_t read_offset = 0;
	size_t rem_bytes = 0;
	size_t unread_bytes;
	
	// For accessing the circular buffer
	struct aesd_dev *dev;
	dev = (struct aesd_dev *) filp->private_data;

	/**
	 * TODO: handle read
	 * filep has the private_data member setup to aesd_dev by the open function
	 * Thus we can access pointer to aesd_dev here
	 * 
	 * Since buf is a user space buffer we need to use copy_to_user to access it
	 */

	// offset
	// when count > number of bytes left in current entry, bytes read = size - offset - 1
	// user should update fpos = fpos + bytes read

	// else bytes read = count
	//PDEBUG("Buffer status = %d, in_offset = %d, out_offset = %d\n",dev->aesd_cbuf.full, dev->aesd_cbuf.in_offs, dev->aesd_cbuf.out_offs);
	PDEBUG("in_offset = %d, out_offset = %d, f_pos = %lld\n",dev->aesd_cbuf.in_offs, dev->aesd_cbuf.out_offs, *f_pos);

	// Lock the resource
	//mutex_lock_interruptible(&(dev->lock));
	if(mutex_lock_interruptible(&(dev->lock)) != 0)
	{
		printk(KERN_ALERT "Mutex lock failed in aesd_read \n");
		//printk(KERN_ALERT "Mutex lock failed\n");
		return -ERESTARTSYS;

	}
	read_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->aesd_cbuf), *f_pos, &read_offset);
	if(read_entry != NULL)
	{
		//PDEBUG("entry = %s, read_offset = %zu\n", read_entry->buffptr, read_offset);
		PDEBUG("Entry = ");
		debug_print_string(read_entry->buffptr, read_entry->size);
		PDEBUG("Read_offset = %zu\n", read_offset);
	}
	else
	{
		// nothing more to read
		mutex_unlock(&(dev->lock));
		return 0;
	}
	mutex_unlock(&(dev->lock));

	rem_bytes = read_entry->size - read_offset;
	if(count > rem_bytes)
	{
		bytes_read = rem_bytes;
	}
	else
	{
		bytes_read = count;
	}
	// free memory in read?????????????????????????????????????? - No
	unread_bytes = __copy_to_user(buf, (void*)(read_entry->buffptr + read_offset), bytes_read);
	if(unread_bytes != 0)
	{
		printk(KERN_ALERT "Unable to copy all bytes to user\n");
		return -EFAULT;
	}
	else
	{
		printk(KERN_ALERT "Copied %ld bytes to user\n", bytes_read);
	}
	// dev->aesd_cbuf.out_offs = (dev->aesd_cbuf.out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
	// if(dev->aesd_cbuf.out_offs == dev->aesd_cbuf.in_offs)
	// {
	// 	dev->aesd_cbuf.empty = true;
	// }
	// else
	// {
	// 	dev->aesd_cbuf.empty = false;
	// }
	*f_pos += bytes_read;


	// mutex_unlock(&(dev->lock));
	PDEBUG("End of aesd_read function\n\n");
	return bytes_read;
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
	//PDEBUG("buf = %s, count = %zu, fpos = %lld\n", buf, count, *f_pos);
	char *overwritten_buf = NULL;
	
	static size_t total_count;

	struct aesd_dev *dev = NULL;
	dev = (struct aesd_dev *) filp->private_data;
	
	// change to bool later
	static uint8_t write_complete = 1;

	total_count += count;

	size_t unwritten_bytes = 0;
	/**
	 * 	Check if the previous write was complete (complete if it had a \n character)
	 * 		If it was then we need to malloc new memory for the current write operation
	 * 
	 */
	//mutex_lock_interruptible(&(dev->lock));
	/**
	 * 	All below operations use dev which is the file's private data
	 * 	Hence these operations need to be locked by a mutex
	 */
	if(mutex_lock_interruptible(&(dev->lock)) != 0)
	{
		printk(KERN_ALERT "Mutex lock failed in aesd_write\n");
		//printk(KERN_ALERT "Mutex lock failed\n");
		return -ERESTARTSYS;

	}
	if(write_complete)
	{
		dev->aesd_cb_entry->buffptr = (char*)kmalloc(count, GFP_KERNEL);
		if(dev->aesd_cb_entry->buffptr == NULL)
		{
			// handle error - use goto
			printk(KERN_ALERT "Unable to mallocin aesd_write\n");
			mutex_unlock(&(dev->lock));
			return -ENOMEM;
		}

		// use copy_from_user to copy data into kernel
		unwritten_bytes = __copy_from_user((void*)(dev->aesd_cb_entry->buffptr), buf, count);
		if(unwritten_bytes != 0)
		{
			printk(KERN_ALERT "Unable to copy all bytes into kernel\n");
			mutex_unlock(&(dev->lock));
			return -EFAULT;
		}
		else
		{
			PDEBUG("Copied %ld bytes into the kernel\n", count-unwritten_bytes);
		}
	}
	else
	{
		dev->aesd_cb_entry->buffptr = (char*)krealloc(dev->aesd_cb_entry->buffptr, total_count, GFP_KERNEL);
		if(dev->aesd_cb_entry->buffptr == NULL)
		{
			// handle error
			printk(KERN_ALERT "Unable to krealloc in aesd_write\n");
			mutex_unlock(&(dev->lock));
			return -ENOMEM;
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
		unwritten_bytes = __copy_from_user((void*)(dev->aesd_cb_entry->buffptr + total_count - count), buf, count);
		if(unwritten_bytes != 0)
		{
			printk(KERN_ALERT "Unable to copy all bytes into kernel\n");
			mutex_unlock(&(dev->lock));
			return -EFAULT;
		}
		else
		{
			PDEBUG("Copied %ld bytes into the kernel\n", count-unwritten_bytes);
		}
		
	}
	//memcpy(dev->aesd_cb_entry->buffptr + total_count, "\0", 1);
	if(memchr(dev->aesd_cb_entry->buffptr, '\n', total_count) != NULL)
	{
		//pointer[total_count] = '\0';
		dev->aesd_cb_entry->size = total_count;
		total_count = 0;
		write_complete = 1;
		// add to buffer
		overwritten_buf = aesd_circular_buffer_add_entry(&(dev->aesd_cbuf),(dev->aesd_cb_entry));
		if(overwritten_buf != NULL)
		{
			PDEBUG("Freeing : %s\n", overwritten_buf);
			// PDEBUG("Freeing : ");
			// debug_print_string(overwritten_buf, dev->aesd_cbuf.entry[j].size);
			kfree(overwritten_buf);
		}
	}
	else
	{
		write_complete = 0;
	}

	int j = 0;
	for(j = 0; j < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; j++)
	{
		//PDEBUG("entry %d = %s", j, dev->aesd_cbuf.entry[j].buffptr);
		PDEBUG("Entry %d = ", j);
		debug_print_string(dev->aesd_cbuf.entry[j].buffptr, dev->aesd_cbuf.entry[j].size);
	}

	mutex_unlock(&(dev->lock));
	PDEBUG("End of aesd_write function\n\n");

	// return number of bytes written
	return count;

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
	PDEBUG("Start of aesd_init_module function12\n");

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
	 * Initialize the circular buffer 
	 * Initialize locking primitive
	 */
	aesd_circular_buffer_init(&(aesd_device.aesd_cbuf));

	aesd_device.aesd_cb_entry = kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
	if (aesd_device.aesd_cb_entry == NULL)
	{
		printk(KERN_ALERT "kmalloc failed in init function\n");
		return -ENOMEM;
	}

	mutex_init(&(aesd_device.lock));

	/**
	 * TODO: malloc the entry inside aesd device
	*/
	result = aesd_setup_cdev(&aesd_device);
	if( result ) {
		unregister_chrdev_region(dev, 1);
	}

	PDEBUG("End of aesd_init_module function8\n");
	return result;

}

// This function is called when the module is unloaded
void aesd_cleanup_module(void)
{
	uint8_t index;
	struct aesd_buffer_entry *entry;

	// Get the dev number from major and minor numbers
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 * 
	 * Free memory associated the driver
	 * Uninitialize everything
	 * 
	 */

	kfree(aesd_device.aesd_cb_entry);

	// Free all non NULL entries in the circular buffer
	AESD_CIRCULAR_BUFFER_FOREACH(entry, &(aesd_device.aesd_cbuf), index)
  	{
		/*
		 *	Only free the entry if it is not NULL
		 *	It can be NULL when the buffer was filled with entries < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED
		 *	and the driver was unloaded
		*/	
		if(entry->buffptr != NULL)
		{
			kfree(entry->buffptr);
		}
  	}
	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
