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
#include <linux/slab.h> //dynamic memory
#include <linux/string.h> //string header
#include <linux/errno.h> 
#include "aesdchar.h"
#include "aesd-circular-buffer.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Malola Simman"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    struct aesd_dev *dev; /* device information */
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev; /* for other methods */
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
    size_t *entry_offset = 0;
    unsigned long rc = 0;
    struct aesd_dev *device = filp->private_data;
    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);
    /**
     * TODO: handle read struct aesd_circular_buffer *buffer
     */
    int rc = mutex_lock_interruptible(&(device->dev_lock));
    if(rc !=0)
    {
        return -ERESTARTSYS;
    }
    struct aesd_buffer_entry *ret = aesd_circular_buffer_find_entry_offset_for_fpos(&device->cb_entry,*fpos, &entry_offset);
    if (ret == NULL)
    {
        PDEBUG("read byte not found");
        *f_pos = 0;
        retval = 0;
        goto exit_read:
    }
    ssize_t readbytes = ret->size - entry_offset ;
    if(readbytes < = count )
    { 
        rc = copy_to_user(buf, ret->buffptr+entry_offset, readbytes);
        if(rc)
        {
            PDEBUG("fail to copy from userspace");
            retval = -EFAULT;
		    goto exit_read;
        }
        else 
        {
            retval = readbytes;
            *f_pos = *f_pos + readbytes;
        }
    }
    else 
    {
        readbytes = count ;
        rc = copy_to_user(buf, ret->buffptr+entry_offset, readbytes);
        if(rc)
        {
            PDEBUG("fail to copy from userspace");
            retval = -EFAULT;
		    goto exit_read;
        }
        else 
        {
            retval = readbytes;
            *f_pos = *f_pos + readbytes;
        }
    }
exit_read:
    mutex_unlock(&device->dev_lock);    
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    unsigned long ret=0;
    struct aesd_dev *device = NULL;
    int rc = 0;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    rc = mutex_lock_interruptible(&(device->dev_lock));
    if(rc !=0)
    {
        return -ERESTARTSYS;
    }
    device = filp->private_data;
    if(device->cb_entry.size == 0)   // malloc for first time
    {

        device->cb_entry.buffptr  = kmalloc(count, GFP_KERNEL);
        if(device->cb_entry.buffptr == NULL)
        {
            PDEBUG("malloc failed");
            goto exit_write;
        }
        ret = copy_from_user((void *)&device->cb_entry.buffptr[device->cb_entry.size], buf, count);
        if(ret)
        {
            retval = -EFAULT;
		    goto exit_write;
        }
        device->cb_entry.size += count;

    }
    else //realloc
    {
        
        device->cb_entry.buffptr  = krealloc(device->cb_entry.buffptr, count + device->cb_entry.size, GFP_KERNEL);
        if(device->cb_entry.buffptr == NULL)
        {
            PDEBUG("realloc failed");
            goto exit_write;
        }
        ret = copy_from_user((void *)&device->cb_entry.buffptr[device->cb_entry.size], buf, count);
        if(ret)
        {
            PDEBUG("fail to copy from userspace");
            retval = -EFAULT;
		    goto exit_write;
        }
        device->cb_entry.size += count;

	}
    if(strpbrk(device->buffer_entry.buffptr, '\n'))
	{
		//adding entry to circular buffer  
		const char *res  = aesd_circular_buffer_add_entry(&device->cb, &device->cb_entry);
		if(res != NULL)
        {
            kfree(res);
        }
		device->cb_entry.buffptr =  NULL; 
		device->cb_entry.size = 0;
	}
    retval = count;

exit_write:
    *f_pos = 0;
    mutex_unlock(&device->dev_lock);    
    return retval;
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
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
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
    cbuffer_exit_cleanup(&aesd_device.cb);
    mutex_destroy(&aesd_device.dev_lock);
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
