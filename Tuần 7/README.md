# Linux Kernel Character Device Driver

## Yêu cầu 1 

### Cơ chế Khởi tạo và Giải phóng

A. Cấu trúc Vòng đời (Init/Exit)

Đây là hai điểm đầu và cuối của một module khi được nạp vào nhân Linux.

__init: Chạy khi thực hiện lệnh insmod. Nhiệm vụ là cấp phát tài nguyên.

__exit: Chạy khi thực hiện lệnh rmmod. Nhiệm vụ là giải phóng hoàn toàn tài nguyên đã mượn.

```
static int __init basic_driver_init(void) {
    // Code khởi tạo ở đây
    return 0;
}

static void __exit basic_driver_exit(void) {
    // Code dọn dẹp ở đây
}

module_init(basic_driver_init);
module_exit(basic_driver_exit);
```

B. Các hàm Giao tiếp (Callbacks)

Các hàm này là cầu nối khi User Space tương tác với file thiết bị:

Open: Được gọi khi ứng dụng dùng open(). Dùng để kiểm tra quyền truy cập.

Release: Được gọi khi ứng dụng dùng close(). Dùng để dọn dẹp phiên làm việc.

Read: Cho phép User đọc dữ liệu từ Kernel lên.

Write: Cho phép User ghi dữ liệu (lệnh điều khiển) xuống Kernel.

```
static int basic_driver_open(struct inode *inode, struct file *file) {}
static int basic_driver_release(struct inode *inode, struct file *file) {}
static ssize_t basic_driver_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {}
static ssize_t basic_driver_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {}
```


C. Đăng ký qua struct file_operations

Đây là thành phần quan trọng nhất để Kernel biết phải gọi hàm nào khi có yêu cầu từ người dùng.
```
static const struct file_operations basic_driver_fops = {
    .owner   = THIS_MODULE,
    .open    = basic_driver_open,
    .release = basic_driver_release,
    .read    = basic_driver_read,
    .write   = basic_driver_write,
};
```
## Yêu cầu 2
A. Khái niệm Major và Minor

Major Number: Định danh Driver nào sẽ quản lý thiết bị.

Minor Number: Phân biệt các thực thể thiết bị cụ thể (ví dụ: LED1, LED2) sử dụng chung một Driver.

B. Cấp phát động với alloc_chrdev_region

Hàm này yêu cầu Kernel tự động tìm một số Major còn trống và cấp cho Driver.
```
dev_t dev_num; // Biến lưu trữ số hiệu thiết bị (32-bit)

// Cú pháp: alloc_chrdev_region(&biến_lưu, minor_bắt_đầu, số_lượng, "tên_thiết_bị");
int ret = alloc_chrdev_region(&dev_num, 0, 1, "yozora_device");
```

C. Giải phóng tài nguyên

Khi gỡ Module, bắt buộc phải trả lại số hiệu đã mượn để tránh rò rỉ tài nguyên hệ thống.
```
// Cú pháp: unregister_chrdev_region(số_hiệu, số_lượng);
unregister_chrdev_region(dev_num, 1);
```
## Yêu cầu 3

A. Tạo Device Class (class_create)

Class là một cách để nhóm các thiết bị có tính chất tương tự nhau. Việc tạo Class là bước đệm để tạo Device file.

static struct class *basic_class;
basic_class = class_create(THIS_MODULE, "basic_class");


B. Tạo Device File (device_create)

Hàm này sẽ thực hiện tạo file thực sự trong thư mục /dev/ với cái tên bạn chỉ định. Nó liên kết trực tiếp tên file này với số hiệu Major/Minor đã cấp phát.
```
static struct device *basic_device;
basic_device = device_create(basic_class, NULL, dev_num, NULL, "basic_device");
```

C. Giải phóng tài nguyên (Hàm Exit)

Theo nguyên tắc LIFO (Vào sau ra trước), bạn phải hủy Device trước khi hủy Class.
```
device_destroy(basic_class, dev_num); // Xóa file /dev/basic_device
class_destroy(basic_class);           // Xóa class trong /sys/class/
```

## Yêu cầu 4

A. Hàm Write (copy_from_user)

Sử dụng khi người dùng gửi dữ liệu xuống Driver (ví dụ: gửi lệnh bật/tắt LED).
```
static ssize_t basic_driver_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    // Copy dữ liệu từ buffer của người dùng vào buffer của Kernel
    if (copy_from_user(kernel_buffer, buf, count)) {
        return -EFAULT;
    }
    // Dữ liệu lúc này đã nằm an toàn trong kernel_buffer
    return count;
}
```

B. Hàm Read (copy_to_user)

Sử dụng khi người dùng muốn đọc dữ liệu từ Driver lên (ví dụ: đọc trạng thái cảm biến).
```
static ssize_t basic_driver_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    // Copy dữ liệu từ buffer của Kernel lên cho người dùng
    if (copy_to_user(buf, kernel_buffer + *offset, count)) {
        return -EFAULT;
    }
    return count;
}
```

C. Quản lý biến offset

Biến offset cực kỳ quan trọng để Kernel biết vị trí đọc/ghi hiện tại trong file. Nếu không cập nhật offset, các chương trình như cat sẽ bị lặp vô hạn dữ liệu.

Sau khi đọc/ghi thành công, ta phải cộng dồn: *offset += count;.

Khi *offset đạt đến giới hạn buffer, hàm read phải trả về 0 để báo hiệu kết thúc file (EOF).

## Build & Test

```
# Build module
make
```
```
# Load driver
sudo insmod basic_driver.ko
```
<img width="788" height="58" alt="image" src="https://github.com/user-attachments/assets/9d50c35a-66e7-4656-ac7e-5d2b22faea2b" />

```
# Test ghi/đọc
echo "1" > /dev/basic_device
cat /dev/basic_device
```
<img width="460" height="118" alt="image" src="https://github.com/user-attachments/assets/9aa0defc-8889-44f4-86c8-dd133107d358" />

<img width="594" height="96" alt="image" src="https://github.com/user-attachments/assets/657e28d3-d1c5-4aa1-ba73-14380dbdd4de" />



```
# Unload driver
sudo rmmod basic_driver
```
<img width="454" height="105" alt="Screenshot 2026-03-31 210503" src="https://github.com/user-attachments/assets/17f53e1b-53ec-4cef-a3df-022d1eba3759" />

## Yêu cầu 5

Mã nguồn device driver tích hợp ioremap
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/version.h>

#define DRIVER_NAME "basic_driver"
#define CLASS_NAME  "basic_class"
#define DEVICE_NAME "basic_device"

#define CONTROL_MODULE_BASE 0x44E10000
#define MUX_USR3_LED        0x860      
#define GPIO1_BASE          0x4804C000
#define GPIO1_SIZE          0x1000

#define GPIO_OE             0x134   
#define GPIO_DATAIN         0x138   
#define GPIO_CLEARDATAOUT   0x190   
#define GPIO_SETDATAOUT     0x194   
#define LED_USR3            (1 << 24) 

static dev_t dev_num;
static struct cdev basic_cdev;
static struct class *basic_class;
static struct device *basic_device;
static void __iomem *gpio1_addr; 
static void __iomem *control_module_addr;

static int basic_driver_open(struct inode *inode, struct file *file) { return 0; }
static int basic_driver_release(struct inode *inode, struct file *file) { return 0; }

static ssize_t basic_driver_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    uint32_t reg_val;
    char status;
    if (*offset > 0) return 0;
    reg_val = ioread32(gpio1_addr + GPIO_DATAIN);
    status = (reg_val & LED_USR3) ? '1' : '0';
    if (copy_to_user(buf, &status, 1)) return -EFAULT;
    *offset += 1;
    return 1;
}

static ssize_t basic_driver_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    char input;
    if (copy_from_user(&input, buf, 1)) return -EFAULT;
    if (input == '1') {
        iowrite32(LED_USR3, gpio1_addr + GPIO_SETDATAOUT);
        pr_info("%s: LED ON\n", DRIVER_NAME);
    } else if (input == '0') {
        iowrite32(LED_USR3, gpio1_addr + GPIO_CLEARDATAOUT);
        pr_info("%s: LED OFF\n", DRIVER_NAME);
    }
    return count;
}

static const struct file_operations basic_driver_fops = {
    .owner = THIS_MODULE,
    .open = basic_driver_open,
    .release = basic_driver_release,
    .read = basic_driver_read,
    .write = basic_driver_write,
};

static int __init basic_driver_init(void) {
    uint32_t reg_val;

    if (alloc_chrdev_region(&dev_num, 0, 1, DRIVER_NAME) < 0) return -1;
    cdev_init(&basic_cdev, &basic_driver_fops);
    cdev_add(&basic_cdev, dev_num, 1);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    basic_class = class_create(CLASS_NAME);
#else
    basic_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
    basic_device = device_create(basic_class, NULL, dev_num, NULL, DEVICE_NAME);

    control_module_addr = ioremap(CONTROL_MODULE_BASE, 0x1000);
    if (control_module_addr) {
        iowrite32(0x07, control_module_addr + MUX_USR3_LED);
        iounmap(control_module_addr);
        pr_info("%s: Forced USR3 Pin Mux to GPIO Mode 7\n", DRIVER_NAME);
    }
    gpio1_addr = ioremap(GPIO1_BASE, GPIO1_SIZE);
    
    reg_val = ioread32(gpio1_addr + GPIO_OE);
    reg_val &= ~LED_USR3; 
    iowrite32(reg_val, gpio1_addr + GPIO_OE);

    iowrite32(LED_USR3, gpio1_addr + GPIO_CLEARDATAOUT);

    pr_info("%s: Driver loaded and LED forced OFF\n", DRIVER_NAME);
    return 0;
}

static void __exit basic_driver_exit(void) {
    iowrite32(LED_USR3, gpio1_addr + GPIO_CLEARDATAOUT);
    iounmap(gpio1_addr);
    device_destroy(basic_class, dev_num);
    class_destroy(basic_class);
    cdev_del(&basic_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("%s: Driver unloaded\n", DRIVER_NAME);
}

module_init(basic_driver_init);
module_exit(basic_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yozora");
```
Chạy lại lệnh make để tạo ra file .ko mới

## Yêu cầu 6

Mã nguồn ứng dụng
```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE_PATH "/dev/basic_device"

int main(int argc, char *argv[]) {
    int fd;
    int blink_count = 10;
    int delay_ms = 500;
    if (argc == 2) {
        delay_ms = atoi(argv[1]);
    }

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device file");
        return -1;
    }

    printf("Starting LED Blink on USR3 with delay %dms...\n", delay_ms);

    for (int i = 0; i < blink_count; i++) {
        write(fd, "1", 1);
        printf("LED ON\n");
        usleep(delay_ms * 1000);

        write(fd, "0", 1);
        printf("LED OFF\n");
        usleep(delay_ms * 1000);
    }
    close(fd);
    printf("Blink finished.\n");
    return 0;
}
```

Dùng trình biên dịch arm-linux-gcc để build.

Nạp file vừa build được và flie .ko ở trên vào thẻ nhớ.

Kết nối BBB với terminal rồi nạp device driver mới.
```
insmod basic_driver.ko
```

Chạy chương trình.
```
./blink_app 500
```


