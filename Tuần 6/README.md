# BeagleBone Black - Custom LED Blink Project (Buildroot)


## Cấu trúc thư mục dự án (trong Buildroot)
Dự án được tích hợp trực tiếp vào cây thư mục của Buildroot:
```text
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
```
Kiến trúc hệ thống & Cách hoạt động
Dự án được chia làm 3 phần chính hoạt động phối hợp với nhau:

Phần 1: Kernel Device Driver (gpio_driver.ko)
Copy folder gpio_driver vào buildroot/package.

Khai báo package với Buildroot.
```bash
//copy vào cuối file package/config.in
source "package/gpio_driver/Config.in"
```

Chạy lệnh 'make menuconfig', bật gpio_driver trong Target packages.

Save và make.

Kiểm tra file thiết bị đã được tạo chưa
```bash
ls -l /dev/my_led
```

Tương tác cơ bản
```bash
echo 1 > /dev/my_led  # LED phải sáng
echo 0 > /dev/my_led  # LED phải tắt
cat /dev/my_led       # Xem trạng thái (nếu hàm read của bạn chuẩn)
```
<img width="735" height="260" alt="Screenshot 2026-03-17 194714" src="https://github.com/user-attachments/assets/fab98968-20f2-40eb-a29b-bcc2c04eadd9" />

Phần 2+3: User-space Application (blink) + tạo script tự khởi chạy
Copy folder blink_app vào buildroot/package.

Khai báo package với Buildroot.
```bash
//copy vào cuối file package/config.in
source "package/blink_app/Config.in"
```
Tìm thư mục overlay
```text
make menuconfig -> System configuration ->  Root filesystem overlay directories
```

Trong thư mục overlay tạo thư mục /etc/init.d/

Tạo file script tên S99blink tại board/beaglebone/rootfs_overlay/etc/init.d/S99blink với nội dung:
```bash
#!/bin/sh

case "$1" in
  start)
    echo "Starting Blink LED..."
    insmod /lib/modules/6.6.121/updates/gpio_driver.ko
    
    MAJOR=$(awk '$2=="my_led" {print $1}' /proc/devices)
    rm -f /dev/my_led
    if [ ! -z "$MAJOR" ]; then
        mknod /dev/my_led c $MAJOR 0
    fi
    sleep 1
    
    /usr/bin/blink &
    ;;
  stop)
    echo "Stopping Blink LED..."
    killall blink
    sleep 1
    if [ -e /dev/my_led ]; then
        echo 0 > /dev/my_led
    fi
    
    rmmod gpio_driver
    rm -f /dev/my_led
    ;;
  *)
    echo "Usage: $0 {start|stop}"
    exit 1
esac

exit 0
```

Lưu lại và cấp quyền thực thi.
```bash
chmod +x board/beaglebone/rootfs_overlay/etc/init.d/S99blink
```

Vào menuconfig và bật blink_app.

Video demo: https://drive.google.com/file/d/1fwKrGnQ2T9xgynjv-JxEEwx_XCicrT8c/view?usp=sharing

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


