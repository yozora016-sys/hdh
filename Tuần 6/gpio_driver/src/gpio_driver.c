#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#define DRIVER_NAME "my_led"
static int major_number;
void __iomem *gpio1_base; // Con tro vung nho GPIO

static ssize_t led_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos) {
    char buf[2];
    if (count > 2) count = 2;
    if (copy_from_user(buf, user_buf, count)) return -EFAULT;

    if (buf[0] == '1') {
        iowrite32(1 << 28, gpio1_base + 0x194); // SET (Bat LED)
    } else if (buf[0] == '0') {
        iowrite32(1 << 28, gpio1_base + 0x190); // CLEAR (Tat LED)
    }
    return count;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = led_write,
};

static int __init led_init(void) {
    unsigned int oe;
    void __iomem *pinmux;

    // 1. Ep Pin Mux P9_12 sang GPIO (Mode 7)
    pinmux = ioremap(0x44E10878, 4);
    if (pinmux) {
        iowrite32(0x07, pinmux);
        iounmap(pinmux);
    }

    // 2. Map truc tiep bo nho vat ly cua khoi GPIO1 (KHONG dung gpio_request nua)
    gpio1_base = ioremap(0x4804C000, 0x1000);
    if (!gpio1_base) return -ENOMEM;

    // 3. Cau hinh chan 28 (P9_12) thanh Output
    oe = ioread32(gpio1_base + 0x134);
    oe &= ~(1 << 28);
    iowrite32(oe, gpio1_base + 0x134);

    // 4. Tat LED mac dinh
    iowrite32(1 << 28, gpio1_base + 0x190);

    major_number = register_chrdev(0, DRIVER_NAME, &fops);
    if (major_number < 0) {
        iounmap(gpio1_base);
        return major_number;
    }
    
    printk(KERN_INFO "GPIO_LED: Da nap Driver MMIO thanh cong!\n");
    return 0; // LUON LUON TRA VE 0 (THANH CONG)
}

static void __exit led_exit(void) {
    iowrite32(1 << 28, gpio1_base + 0x190);
    iounmap(gpio1_base);
    unregister_chrdev(major_number, DRIVER_NAME);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
