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
//Header
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> 
#include <linux/slab.h> 
#include <linux/string.h> 
#include <linux/errno.h> 
#include <linux/uaccess.h>
#include "aesdchar.h"

//global variables
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Malola Simman"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev = NULL;
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    
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
    size_t entry_offset = 0;
    unsigned long rc = 0;
    struct aesd_dev *device = filp->private_data;
    int val = 0;
    ssize_t readbytes = 0;
    struct aesd_buffer_entry *ret =NULL;

    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);
    /**
     * TODO: handle read struct aesd_circular_buffer *buffer
     */
    //perfoming appropriate locking to ensure safe multi-thread and multi-process access
    val = mutex_lock_interruptible(&(device->dev_lock));
    if(val !=0)
    {
        return -ERESTARTSYS;
    }
    // getting the entry offset for given fpos in circular buffer
    ret = aesd_circular_buffer_find_entry_offset_for_fpos(&device->cb, *f_pos, &entry_offset);
    if (ret == NULL)
    {
        PDEBUG("read byte not found");
        *f_pos = 0;
        retval = 0;
        goto exit_read;
    }
    //calculating number of bytes to be read from offset position
    readbytes = ret->size - entry_offset ;
    //if readbytes exceed count truncating until count
    if(readbytes > count )
    { 
        readbytes = count ;
        rc = copy_to_user(buf, (ret->buffptr+entry_offset), readbytes);
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
        
        rc = copy_to_user(buf, (ret->buffptr+entry_offset), readbytes);
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
    //goto implementation
exit_read:
    mutex_unlock(&device->dev_lock);    
    return retval;
}


ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    char* newline = NULL;
    size_t ret=0;
    struct aesd_dev *device = NULL;
    const char *res = NULL;
    int rc = 0;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    device = (struct aesd_dev *)(filp->private_data);
    //perfoming appropriate locking to ensure safe multi-thread and multi-process access
    rc = mutex_lock_interruptible(&(device->dev_lock));
    if(rc !=0)
    {
        return -ERESTARTSYS;
    }
    if(device->cb_entry.size == 0)   // malloc for first time
    {
        device->cb_entry.buffptr  = kmalloc(count*sizeof(char), GFP_KERNEL);
        if(device->cb_entry.buffptr == NULL)
        {
            PDEBUG("malloc failed");
            goto exit_write;
        }
    }
    else //realloc
    {

        device->cb_entry.buffptr  = krealloc(device->cb_entry.buffptr, (count + device->cb_entry.size), GFP_KERNEL);
        if(device->cb_entry.buffptr == NULL)
        {
            PDEBUG("realloc failed");
            goto exit_write;
        }
    }
    ret = copy_from_user((void *)&device->cb_entry.buffptr[device->cb_entry.size], buf, count);
    if(ret)
    {
        PDEBUG("fail to copy from userspace");
    }
    //actual bytes coptied from user
    retval = count - ret;
    //entry copied 
    device->cb_entry.size += retval;
    //checking for first occurence of newline
    newline = strnchr(device->cb_entry.buffptr,device->cb_entry.size,'\n');
    if(newline != NULL)
    {
        //writing to circular buffer
        res  = aesd_circular_buffer_add_entry(&device->cb, &device->cb_entry);
        if(res != NULL)
        {
            //free memory if fail to write
            kfree(res);
        }
        //reset
        device->cb_entry.buffptr =  NULL;
        device->cb_entry.size = 0;
        newline = NULL;
    }
    *f_pos = 0;
    //goto implementation
exit_write:
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
