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


