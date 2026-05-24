#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

#define DRIVER_NAME     "gpio_btnled"
#define DEVICE_NAME     "btnled"
#define CLASS_NAME      "envmon"


#define BTNLED_MAGIC    'B'
#define IOCTL_GET_BTN   _IOR(BTNLED_MAGIC, 0, int)
#define IOCTL_SET_LED   _IOW(BTNLED_MAGIC, 1, int)
#define IOCTL_GET_LED   _IOR(BTNLED_MAGIC, 2, int)


#define GPIO_BUTTON     540
#define GPIO_LED        530

struct btnled_dev {
    dev_t           devnum;
    struct cdev     cdev;
    struct class   *cls;
    struct device  *dev;
    
    int             btn_gpio;
    int             led_gpio;
    int             btn_irq;

    wait_queue_head_t   wq;
    int                 btn_event;  
    int                 btn_state;  
    spinlock_t          lock;
};

static struct btnled_dev *bdev;

static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
    struct btnled_dev *dev = dev_id;
    unsigned long flags;
    int val;

    val = gpio_get_value(dev->btn_gpio);
    
    spin_lock_irqsave(&dev->lock, flags);
    dev->btn_state = !val; 
    dev->btn_event = 1;
    spin_unlock_irqrestore(&dev->lock, flags);

    wake_up_interruptible(&dev->wq);
    
    pr_info("btnled: button %s\n", dev->btn_state ? "pressed" : "released");
    return IRQ_HANDLED;
}

/*File operations */

static int btnled_open(struct inode *inode, struct file *file)
{
    file->private_data = bdev;
    pr_info("btnled: device opened\n");
    return 0;
}

static int btnled_release(struct inode *inode, struct file *file)
{
    pr_info("btnled: device closed\n");
    return 0;
}

static ssize_t btnled_read(struct file *file, char __user *buf,
                            size_t count, loff_t *ppos)
{
    struct btnled_dev *dev = file->private_data;
    char kbuf[4];
    int len;
    unsigned long flags;
    int state;

    if (file->f_flags & O_NONBLOCK) {
        spin_lock_irqsave(&dev->lock, flags);
        if (!dev->btn_event) {
            spin_unlock_irqrestore(&dev->lock, flags);
            return -EAGAIN;
        }
        dev->btn_event = 0;
        state = dev->btn_state;
        spin_unlock_irqrestore(&dev->lock, flags);
    } else {
        if (wait_event_interruptible(dev->wq, dev->btn_event))
            return -ERESTARTSYS;
        spin_lock_irqsave(&dev->lock, flags);
        dev->btn_event = 0;
        state = dev->btn_state;
        spin_unlock_irqrestore(&dev->lock, flags);
    }

    len = snprintf(kbuf, sizeof(kbuf), "%d\n", state);
    if (copy_to_user(buf, kbuf, len))
        return -EFAULT;

    return len;
}

static ssize_t btnled_write(struct file *file, const char __user *buf,
                             size_t count, loff_t *ppos)
{
    struct btnled_dev *dev = file->private_data;
    char kbuf[8];
    int val;

    if (count > sizeof(kbuf) - 1)
        return -EINVAL;

    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;
    kbuf[count] = '\0';

    if (kstrtoint(kbuf, 10, &val))
        return -EINVAL;

    gpio_set_value(dev->led_gpio, val ? 1 : 0);
    pr_info("btnled: LED set to %d\n", val);
    return count;
}

static long btnled_ioctl(struct file *file, unsigned int cmd,
                          unsigned long arg)
{
    struct btnled_dev *dev = file->private_data;
    int val, ret = 0;
    unsigned long flags;

    switch (cmd) {
    case IOCTL_GET_BTN:
        spin_lock_irqsave(&dev->lock, flags);
        val = dev->btn_state;
        spin_unlock_irqrestore(&dev->lock, flags);
        if (copy_to_user((int __user *)arg, &val, sizeof(val)))
            ret = -EFAULT;
        break;

    case IOCTL_SET_LED:
        if (copy_from_user(&val, (int __user *)arg, sizeof(val))) {
            ret = -EFAULT;
            break;
        }
        gpio_set_value(dev->led_gpio, val ? 1 : 0);
        break;

    case IOCTL_GET_LED:
        val = gpio_get_value(dev->led_gpio);
        if (copy_to_user((int __user *)arg, &val, sizeof(val)))
            ret = -EFAULT;
        break;

    default:
        ret = -ENOTTY;
    }
    return ret;
}

static __poll_t btnled_poll(struct file *file, poll_table *wait)
{
    struct btnled_dev *dev = file->private_data;
    __poll_t mask = 0;
    unsigned long flags;

    poll_wait(file, &dev->wq, wait);

    spin_lock_irqsave(&dev->lock, flags);
    if (dev->btn_event)
        mask |= EPOLLIN | EPOLLRDNORM;
    spin_unlock_irqrestore(&dev->lock, flags);

    return mask;
}

static const struct file_operations btnled_fops = {
    .owner          = THIS_MODULE,
    .open           = btnled_open,
    .release        = btnled_release,
    .read           = btnled_read,
    .write          = btnled_write,
    .unlocked_ioctl = btnled_ioctl,
    .poll           = btnled_poll,
};

/* Module init/exit */

static int __init btnled_init(void)
{
    int ret;

    bdev = kzalloc(sizeof(*bdev), GFP_KERNEL);
    if (!bdev)
        return -ENOMEM;

    bdev->btn_gpio = GPIO_BUTTON;
    bdev->led_gpio = GPIO_LED;
    init_waitqueue_head(&bdev->wq);
    spin_lock_init(&bdev->lock);

    ret = gpio_request(bdev->btn_gpio, "button");
    if (ret) {
        pr_err("btnled: cannot request button GPIO %d\n", bdev->btn_gpio);
        goto err_free;
    }
    gpio_direction_input(bdev->btn_gpio);

    ret = gpio_request(bdev->led_gpio, "led");
    if (ret) {
        pr_err("btnled: cannot request LED GPIO %d\n", bdev->led_gpio);
        goto err_btn;
    }
    gpio_direction_output(bdev->led_gpio, 0);

    bdev->btn_irq = gpio_to_irq(bdev->btn_gpio);
    ret = request_irq(bdev->btn_irq, button_irq_handler,
                      IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                      "btnled_irq", bdev);
    if (ret) {
        pr_err("btnled: cannot request IRQ %d\n", bdev->btn_irq);
        goto err_led;
    }

    ret = alloc_chrdev_region(&bdev->devnum, 0, 1, DEVICE_NAME);
    if (ret) {
        pr_err("btnled: cannot alloc char dev region\n");
        goto err_irq;
    }

    bdev->cls = class_create(CLASS_NAME);
    if (IS_ERR(bdev->cls)) {
        ret = PTR_ERR(bdev->cls);
        goto err_region;
    }

    bdev->dev = device_create(bdev->cls, NULL, bdev->devnum,
                               NULL, DEVICE_NAME);
    if (IS_ERR(bdev->dev)) {
        ret = PTR_ERR(bdev->dev);
        goto err_class;
    }

    cdev_init(&bdev->cdev, &btnled_fops);
    bdev->cdev.owner = THIS_MODULE;
    ret = cdev_add(&bdev->cdev, bdev->devnum, 1);
    if (ret) {
        pr_err("btnled: cdev_add failed\n");
        goto err_device;
    }

    pr_info("btnled: driver loaded. /dev/%s ready\n", DEVICE_NAME);
    pr_info("btnled: Button GPIO=%d, LED GPIO=%d, IRQ=%d\n",
            bdev->btn_gpio, bdev->led_gpio, bdev->btn_irq);
    return 0;

err_device:
    device_destroy(bdev->cls, bdev->devnum);
err_class:
    class_destroy(bdev->cls);
err_region:
    unregister_chrdev_region(bdev->devnum, 1);
err_irq:
    free_irq(bdev->btn_irq, bdev);
err_led:
    gpio_free(bdev->led_gpio);
err_btn:
    gpio_free(bdev->btn_gpio);
err_free:
    kfree(bdev);
    return ret;
}

static void __exit btnled_exit(void)
{
    cdev_del(&bdev->cdev);
    device_destroy(bdev->cls, bdev->devnum);
    class_destroy(bdev->cls);
    unregister_chrdev_region(bdev->devnum, 1);
    free_irq(bdev->btn_irq, bdev);
    gpio_set_value(bdev->led_gpio, 0);
    gpio_free(bdev->led_gpio);
    gpio_free(bdev->btn_gpio);
    kfree(bdev);
    pr_info("btnled: driver unloaded\n");
}

module_init(btnled_init);
module_exit(btnled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nhóm 10");
MODULE_DESCRIPTION("GPIO Button & LED driver for BeagleBone Black");
MODULE_VERSION("1.0");
