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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include <linux/uaccess.h>  //copy_to_user
#include <linux/string.h>
#include "aesdchar.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Malola Simman"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *dev;
	PDEBUG("open");
	/**
	 * TODO: handle open
	 */
	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
	filp->private_data = dev;

	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	 
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t retval = 0;
	struct aesd_dev *device = NULL;
	struct aesd_buffer_entry *ret = NULL;
  	size_t entry_offset = 0;
  	size_t readbytes = 0;
  	size_t rc = 0;
	
  	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	device = (struct aesd_dev *)filp->private_data;
	if(mutex_lock_interruptible(&(device->dev_lock)))
	{
	  PDEBUG("failed to acquire mutexlock");
	  return -ERESTARTSYS;
	}
	
	ret = aesd_circular_buffer_find_entry_offset_for_fpos(&device->cb, *f_pos, &entry_offset);
	
	if(ret == NULL)
	{
		retval = 0;
		*f_pos = 0;
	  mutex_unlock(&(device->dev_lock));
	  return retval;
	}
	else if(ret != NULL)
	{   
	   readbytes = (ret->size - entry_offset);
	   if(readbytes > count)
	   {
	      readbytes = count;
 	   }
	}
	else{}
	rc = copy_to_user(buf, (ret->buffptr+entry_offset), readbytes);
	if(rc != 0)
	{
	  retval = -EFAULT;
	  mutex_unlock(&(device->dev_lock));
	  return retval;
	}
	else
	{
	  retval = readbytes;
	}
	*f_pos += readbytes;
	mutex_unlock(&(device->dev_lock));
	return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t retval = -ENOMEM;
	unsigned long  ret = 0;
	const char * res = 0;
	/**
	 * TODO: handle write
	 */
	char* newline = NULL;  
	struct aesd_dev *device = NULL;
	
	device = (struct aesd_dev *)(filp->private_data);
	
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	
	if(mutex_lock_interruptible(&(device->dev_lock)))
	{
	  PDEBUG("Failed to acquire lock");
	  return -ERESTARTSYS;
	}
	
	if(device->cb_entry.size == 0)
	{
                device->cb_entry.buffptr = kmalloc(count*sizeof(char),GFP_KERNEL);
                if(device->cb_entry.buffptr == NULL)
      		{
			PDEBUG("malloc failed");
        		mutex_unlock(&(device->dev_lock));
        		return retval;
      		}
	}
	else
	{
	  	device->cb_entry.buffptr = krealloc(device->cb_entry.buffptr, (device->cb_entry.size + count), GFP_KERNEL);
	  	if(device->cb_entry.buffptr == NULL)
	  	{
	     		PDEBUG("realloc failed");
	     		mutex_unlock(&(device->dev_lock));
	     		return retval;
	  	}
	}
	
	ret = copy_from_user((void *)(&device->cb_entry.buffptr[device->cb_entry.size]),buf,count);
	if(ret != 0)
	{
	  PDEBUG("fail to copy entire bytes");
	}
	retval = count - ret;
	device->cb_entry.size = device->cb_entry.size + retval;
	
	newline = strchr(device->cb_entry.buffptr, '\n');
	if(newline != NULL)
	{
	
	  res = aesd_circular_buffer_add_entry(&device->cb,&device->cb_entry);
	  if(res != NULL)
	  {
	    kfree(res);
	  }
	  device->cb_entry.buffptr = NULL; 
	  device->cb_entry.size = 0; 
	  newline = NULL;
	}
	
	*f_pos = 0;
	
	mutex_unlock(&(device->dev_lock));
	return retval;
}

struct file_operations aesd_fops = {
	.owner   =  THIS_MODULE,
	.read    =  aesd_read,
	.write   =  aesd_write,
	.open    =  aesd_open,
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
	result = alloc_chrdev_region(&dev, aesd_minor, 1,"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	
	//buffer initilaized
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/**
	 * TODO: initialize the AESD specific portion of the device
	 */
	mutex_init(&aesd_device.dev_lock);
	 
	result = aesd_setup_cdev(&aesd_device);

	if( result ) {
		unregister_chrdev_region(dev, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */
	//free memory, release lock, circular buffer related handling
  	cbuffer_exit_cleanup(&aesd_device.cb);
    
	unregister_chrdev_region(devno, 1);
}


module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
