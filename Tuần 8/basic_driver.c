#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/slab.h>

#define buffer_size 1024

dev_t dev_num;
static struct cdev basic_cdev;
static struct class *basic_class;  
static struct device *basic_device;

static char kernel_buffer[buffer_size];

//op read release write
static int basic_driver_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Basic driver opened\n");
    return 0;
}
static int basic_driver_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Basic driver released\n");
    return 0;
}
static ssize_t basic_driver_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    if (*offset >= buffer_size)return 0; 
    if (*offset + count > buffer_size) count = buffer_size - *offset;
    
    int unread_bytes = copy_to_user(buf, kernel_buffer + *offset, count);

    if (unread_bytes == 0)
    {
        *offset += count;
        printk(KERN_INFO "Read %zu bytes from kernel buffer\n", count);
        return count;
    }
    else
    {
        printk(KERN_ERR "Failed to copy data to user space\n");
        return -EFAULT;
    }
    
}
static ssize_t basic_driver_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    memset(kernel_buffer, 0, buffer_size);
    if (count>buffer_size) count = buffer_size;
    int unwritten_bytes = copy_from_user(kernel_buffer, buf, count);
    
    if (unwritten_bytes == 0)
    {
        *offset += count;
        printk(KERN_INFO "Written %zu bytes to kernel buffer\n", count);
        return count;
    }
    else
    {
        printk(KERN_ERR "Failed to copy data from user space\n");
        return -EFAULT;
    }

}

//
static const struct file_operations basic_driver_fops = {
    .owner = THIS_MODULE,
    .open = basic_driver_open,
    .release = basic_driver_release,
    .read = basic_driver_read,
    .write = basic_driver_write,
};

//init exit
static int __init basic_driver_init(void) {
    //cap phat major/minor
    int result = alloc_chrdev_region(&dev_num, 0, 1, "basic_driver");
    if (result<0)
    {
        printk(KERN_ERR "Failed to allocate character device region\n");
        return result;
    }

    //tao cdev
    cdev_init(&basic_cdev, &basic_driver_fops);
    result = cdev_add(&basic_cdev, dev_num, 1);
    if (result<0)   
    {
        printk(KERN_ERR "Failed to add character device\n");
        unregister_chrdev_region(dev_num, 1);
        return result;
    }

    //tao device class
    basic_class = class_create(THIS_MODULE, "basic_class");
    if (IS_ERR(basic_class))
    {
        printk(KERN_ERR "Failed to create device class\n");
        unregister_chrdev_region(dev_num, 1);
        cdev_del(&basic_cdev);
        return PTR_ERR(basic_class);
    }
    basic_device = device_create(basic_class, NULL, dev_num, NULL, "basic_device");
    if (IS_ERR(basic_device))
    {
        printk(KERN_ERR "Failed to create device\n");
        class_destroy(basic_class);
        unregister_chrdev_region(dev_num, 1);
        cdev_del(&basic_cdev);
        return PTR_ERR(basic_device);
    }

    printk(KERN_INFO "Basic driver initialized\n");
    printk(KERN_INFO "Registered character device with major number %d and minor number %d\n", MAJOR(dev_num), MINOR(dev_num));
    return 0;
}
static void __exit basic_driver_exit(void) {
    //huy device class
    device_destroy(basic_class, dev_num);
    class_destroy(basic_class);

    //xoa device
    unregister_chrdev_region(dev_num, 1);
    cdev_del(&basic_cdev);
    printk(KERN_INFO "Basic driver exited\n");
    
}
module_init(basic_driver_init);
module_exit(basic_driver_exit);

//info
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yozora"); 
MODULE_DESCRIPTION("A basic Linux kernel driver example");