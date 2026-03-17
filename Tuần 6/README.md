# BeagleBone Black - Custom LED Blink Project (Buildroot)

Dự án này là một bài tập thực hành toàn diện về Embedded Linux trên bo mạch BeagleBone Black (BBB). Dự án bao gồm việc phát triển một Kernel Device Driver (tương tác trực tiếp với thanh ghi phần cứng bằng MMIO), một ứng dụng không gian người dùng (User-space App) viết bằng C, và tích hợp toàn bộ vào hệ điều hành tự biên dịch thông qua Buildroot cùng cơ chế Rootfs Overlay.

## 🛠 Yêu cầu phần cứng
* **Bo mạch:** BeagleBone Black (Rev C)
* **Hệ điều hành:** Linux Kernel 6.6.121 (Buildroot)
* **Linh kiện:** 1x LED, 1x Điện trở (220Ω - 330Ω), Dây cắm cắm (Jumper wires)
* **Sơ đồ đấu dây:**
  * Cực dương LED (chân dài) -> Điện trở -> Chân **P9_12** (GPIO 60).
  * Cực âm LED (chân ngắn) -> Chân **P9_1** hoặc **P9_2** (GND).

## 🗂 Cấu trúc thư mục dự án (trong Buildroot)
Dự án được tích hợp trực tiếp vào cây thư mục của Buildroot:
buildroot/
├── package/
│   ├── gpio_driver/         # Gói Kernel Module (Device Driver)
│   │   ├── src/
│   │   │   ├── gpio_driver.c
│   │   │   └── Makefile
│   │   ├── Config.in
│   │   └── gpio_driver.mk
│   └── blink_app/           # Gói User-space Application (C App)
│       ├── src/
│       │   ├── blink.c
│       │   └── Makefile
│       ├── Config.in
│       └── blink_app.mk
└── board/beaglebone/rootfs_overlay/
    └── etc/init.d/
        └── S99blink         # Script khởi động tự động (Init script)
🚀 Kiến trúc hệ thống & Cách hoạt động
Dự án được chia làm 3 phần chính hoạt động phối hợp với nhau:

Phần 1: Kernel Device Driver (gpio_driver.ko)
Thử thách: Trên Kernel 6.6, việc dùng hàm gpio_request() thông thường cho chân P9_12 bị từ chối do Pin Muxing mặc định của hệ thống.

Giải pháp (Bypass): Sử dụng kỹ thuật MMIO (Memory-Mapped I/O). Driver sử dụng hàm ioremap để trỏ trực tiếp đến địa chỉ vật lý của bộ nhớ chip AM335x:

Map địa chỉ 0x44E10878 để ép mở cổng Pin Mux của P9_12 sang Mode 7 (GPIO).

Map địa chỉ 0x4804C000 (GPIO1_BASE) để điều khiển trực tiếp các thanh ghi OE (cấu hình Output), SETDATAOUT (Bật LED) và CLEARDATAOUT (Tắt LED).

Driver cung cấp giao diện giao tiếp qua hàm write, nhận chuỗi ký tự "1" hoặc "0".

Phần 2: User-space Application (blink)
Một chương trình C đơn giản nằm tại /usr/bin/blink.

Nó thực hiện vòng lặp vô hạn: Mở file thiết bị ảo /dev/my_led, gửi tín hiệu "1", chờ 1 giây, gửi tín hiệu "0", chờ 1 giây.

Phần 3: Init Script & Tự động hóa (S99blink)
Đặt tại /etc/init.d/S99blink, được cấp quyền thực thi (chmod +x).

Hoạt động theo cơ chế của Init system (BusyBox).

Quá trình Start:

Dùng insmod nạp driver gpio_driver.ko.

Tự động tìm số Major Number mà Kernel vừa cấp phát thông qua file /proc/devices.

Sử dụng lệnh mknod để tự động tạo node thiết bị /dev/my_led.

Gọi chương trình /usr/bin/blink & chạy ngầm.

Quá trình Stop:

Bắn hạ ứng dụng C bằng lệnh killall blink.

An toàn dập tắt LED bằng lệnh echo 0 > /dev/my_led.

Gỡ driver (rmmod) và xóa sạch node thiết bị (rm -f).

⚙️ Hướng dẫn Biên dịch (Build)
Mở terminal tại thư mục gốc của Buildroot.

Thêm 2 package vào hệ thống bằng cách gọi chúng ở cuối file package/Config.in.

Mở menu cấu hình:

Bash
make menuconfig
Đánh dấu [*] vào hai mục gpio_driver và blink_app. Lưu lại và thoát.

Biên dịch toàn bộ hệ thống (Nếu cập nhật code driver, hãy chạy make gpio_driver-rebuild trước):

Bash
make
File ảnh hệ thống cuối cùng sẽ nằm tại output/images/sdcard.img. Dùng phần mềm (như BalenaEtcher) nạp file này vào thẻ nhớ MicroSD.

💡 Hướng dẫn Sử dụng (Testing)
Chế độ Tự động: Ngay khi cấp nguồn cho BeagleBone Black, LED sẽ tự động nhấp nháy mà không cần người dùng can thiệp (nhờ script S99blink).

Điều khiển Thủ công: Bạn có thể đăng nhập vào bo mạch (user: root) và kiểm soát dịch vụ bằng các lệnh:

Bash
/etc/init.d/S99blink stop   # Dung nhap nhay va tat LED an toan
/etc/init.d/S99blink start  # Bat lai che do nhap nhay
Kiểm tra Log: Để xem quá trình Kernel thao tác với MMIO, sử dụng lệnh:

Bash
dmesg | tail -n 15
