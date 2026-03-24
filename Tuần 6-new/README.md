# BeagleBone Black: Điều khiển LED qua Sysfs & Tích hợp Buildroot Package

## Cấu trúc dự án

```text
~/embedded-linux/buildroot/
├── appdev/
│   └── blink1/                 # Thư mục chứa mã nguồn ứng dụng
│       ├── blink1.c            # Code C điều khiển Sysfs
│       └── Makefile            # File biên dịch nội bộ
└── buildroot/
    └── package/
        └── blink1/             # Thư mục cấu hình Package cho Buildroot
            ├── Config.in       # Khai báo menu hiển thị
            ├── blink1.mk       # Kịch bản tải code, biên dịch và cài đặt
            └── S99blink1       # Script tự động chạy lúc khởi động (init.d)
```
Bước 1: Cấu hình Kernel (Bật tính năng Sysfs cho GPIO)

1. Mở menu cấu hình riêng của Kernel:
```bash
make linux-menuconfig
```
2. Tìm và bật tính năng GPIO_SYSFS:

Điều hướng theo đường dẫn: Device Drivers  --->  GPIO Support  --->
Kéo xuống và tìm mục [ ] /sys/class/gpio/... (sysfs interface).

3. Kích hoạt và Lưu:

Tại dòng /sys/class/gpio/... (sysfs interface), nhấn phím Y để đánh dấu [*] (Kích hoạt tích hợp thẳng vào Kernel).

Dùng phím mũi tên phải chọn nút < Save >, nhấn Enter hai lần để lưu với tên mặc định.

Chọn < Exit > liên tục để thoát hoàn toàn khỏi menuconfig.

4. Cập nhật thay đổi:
Sau khi lưu, bạn bắt buộc phải ra lệnh cho Buildroot dịch lại phần Kernel để áp dụng cấu hình mới này vào Image:

```Bash
make linux-rebuild
```
Nạp lại kernel cho sdcard

5. Chạy thử
<img width="430" height="133" alt="image" src="https://github.com/user-attachments/assets/dd01c846-51ba-476d-82c1-a8be09bd32f9" />

Bước 2: Viết mã nguồn ứng dụng (Appdev)
Di chuyển vào thư mục ~/embedded-linux/buildroot/appdev/blink1.

1. File blink1.c: Chương trình yêu cầu Kernel mở cổng 540, cấu hình chiều xuất tín hiệu (out) và ghi giá trị 1/0 liên tục vào file value để nháy đèn.

```C
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int fd;
    
    // 1. Xin he thong mo cong GPIO 540
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd >= 0) { write(fd, "540", 3); close(fd); }
    
    usleep(100000); // Doi 100ms de he thong tao thu muc

    // 2. Thiet lap chan lam dau ra (out)
    fd = open("/sys/class/gpio/gpio540/direction", O_WRONLY);
    if (fd >= 0) { write(fd, "out", 3); close(fd); }

    // 3. Vong lap nhay den
    while(1) {
        fd = open("/sys/class/gpio/gpio540/value", O_WRONLY);
        if (fd >= 0) { write(fd, "1", 1); close(fd); }
        sleep(1);

        fd = open("/sys/class/gpio/gpio540/value", O_WRONLY);
        if (fd >= 0) { write(fd, "0", 1); close(fd); }
        sleep(1);
    }
    return 0;
}
```
2. File Makefile:

```Makefile
all: blink1

blink1: blink1.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o blink1 blink1.c

clean:
	rm -f blink1
```
Bước 3: Tạo Buildroot Package
Di chuyển vào thư mục ~/embedded-linux/buildroot/buildroot/package/blink1.

1. File Config.in: Khai báo giao diện menuconfig.

```Plaintext
config BR2_PACKAGE_BLINK1
	bool "blink1"
	help
	  Day la goi phan mem nhay den LED thong qua Sysfs tren BeagleBone Black.
```
2. File blink1.mk: Kịch bản chỉ định Buildroot lấy code từ thư mục appdev, gọi Make và copy vào Image cuối cùng.

```Makefile
BLINK1_VERSION = 1.0
BLINK1_SITE = $(HOME)/embedded-linux/buildroot/appdev/blink1
BLINK1_SITE_METHOD = local

define BLINK1_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef

define BLINK1_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/blink1 $(TARGET_DIR)/usr/bin/blink1
	$(INSTALL) -D -m 0755 $(BLINK1_PKGDIR)/S99blink1 $(TARGET_DIR)/etc/init.d/S99blink1
endef

$(eval $(generic-package))
```
3. File S99blink1: Script quản lý ứng dụng chạy nền và dọn dẹp phần cứng khi tắt.

```Bash
#!/bin/sh
case "$1" in
  start)
    echo "Starting Sysfs Blink1 LED..."
    /usr/bin/blink1 &
    ;;
  stop)
    echo "Stopping Sysfs Blink1 LED..."
    killall blink1
    sleep 0.5
    echo 0 > /sys/class/gpio/gpio540/value 2>/dev/null
    echo 540 > /sys/class/gpio/unexport 2>/dev/null
    ;;
  *)
    echo "Usage: $0 {start|stop}"
    exit 1
esac
exit 0
```
Bước 4: Cấu hình hệ thống (Menuconfig)
Khai báo package mới vào file tổng package/Config.in (thêm dòng source "package/blink1/Config.in" ngay dưới menu "Hardware handling").

Sau đó di chuyển về gốc thư mục buildroot và cấu hình:

```Bash
make menuconfig
# Vao Target packages ---> Hardware handling ---> Tich chon [*] blink1
```

Save và make
Bước 5: Chạy thử


Giải nén và nạp lại rootfs.tar

<img width="449" height="317" alt="image" src="https://github.com/user-attachments/assets/4c888182-b2f0-42d7-97bb-5f957bdd1f2c" />

---------------------------------------
Mở picocom: 
```bash 
sudo picocom -b 115200 /dev/ttyUSB0
```

Giải nén và nạp lại file rootfs.tar vào thẻ SD
```
sudo rm -rf /media/$USER/rootfs/*
sudo tar -xf output/images/rootfs.tar -C /media/$USER/rootfs/
sync
sudo umount /media/$USER/rootfs
```
