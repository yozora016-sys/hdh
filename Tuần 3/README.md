# Bài tập HDH Nhúng: Cài đặt Root File System (RootFS) Cơ Bản

## Giới thiệu
Dự án này là Bài tập 01 thuộc học phần Hệ Điều Hành Nhúng. Mục tiêu chính của dự án là tự xây dựng và cài đặt một Root File System (RootFS) cơ bản cho hệ thống nhúng thông qua công cụ **BusyBox**, sau đó cấu hình để Linux Kernel có thể nhận diện, gắn kết (mount) và khởi động thành công.

## Mục tiêu dự án
Dự án yêu cầu hoàn thành các tiêu chí sau:
1. **Biên dịch BusyBox:** Cấu hình và biên dịch thành công RootFS sử dụng mã nguồn BusyBox.
2. **Triển khai lên thẻ nhớ:** Copy và phân quyền chuẩn xác các file thực thi, thư viện và file cấu hình đã biên dịch vào phân vùng RootFS trên thẻ nhớ (SD Card).
3. **Cấu hình Bootloader:** Thiết lập biến môi trường `bootargs` trong U-Boot, đảm bảo Kernel liên kết đúng với phân vùng chứa RootFS thông qua tham số `root=`.
4. **Kiểm thử hệ thống:** Khởi động thành công vào không gian người dùng (User-space), hiển thị Shell prompt và thực thi được các lệnh Linux cơ bản như `ls`, `cat`, `echo`, v.v.

## Yêu cầu hệ thống (Prerequisites)
* **Phần cứng:** Board mạch nhúng (BeagleBone Black), Thẻ nhớ SD/MicroSD, Cáp kết nối Serial/UART.
* **Phần mềm/Công cụ:**
  * Host PC chạy hệ điều hành Linux (Ubuntu/Debian).
  * Cross-compiler toolchain (`arm-linux-gnueabihf-`).
  * Mã nguồn BusyBox (phiên bản ổn định mới nhất).
  * Mã nguồn/Image của Linux Kernel đã được biên dịch (`zImage`).

## Các bước thực hiện

### Bước 1: Biên dịch RootFS với BusyBox
1. Tải và giải nén mã nguồn BusyBox.
2. Cấu hình BusyBox (Sử dụng `make ARCH=arm CROSS_COMPILE=... menuconfig`).
3. Tạo thư mục chứa rootfs.
```bash
cd ~
mkdir my_rootfs
cd my_rootfs

```
4. Copy file `_install` trong BusyBox sang thư mục rootfs.
```bash
cp -a ~/embedded-linux/tinysystem/busybox/_install/* .
```
5. Tạo các thư mục hệ thống còn thiếu.
```bash
mkdir -p dev proc sys etc tmp lib usr/lib var/log home/root
```
6. Tạo file cấu hình khởi động /etc/inittab.
   
Copy nội dung này vào file inittab.
```bash
::sysinit:/etc/init.d/rcS
::askfirst:-/bin/sh
::restart:/sbin/init
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
```
7. Tạo kịch bản khởi động.

Tạo file /etc/init.d/rcS và copy nội dung này.
```bash
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
/bin/echo "Chuc mung! RootFS da khoi dong thanh cong!"
```

Lưu lại, cấp quyền thực thi.
```bash
chmod +x etc/init.d/rcS
```
### Bước 2: Chuyển dữ liệu vào thẻ nhớ
1. Phân vùng thẻ nhớ , tạo ít nhất một phân vùng định dạng `ext4` cho RootFS.
Mở công cụ phân vùng:
```bash
sudo cfdisk /dev/sdb
```
Tạo Phân vùng 2 (RootFS):
[New] -> Size: Còn lại -> [Primary] -> [Type]: Linux.

Format phân vùng 2.
```bash
sudo mkfs.ext4 -L rootfs /dev/sdb2
```
3. Chép RootFS vào phân vùng 2 của thẻ nhớ.
```bash
sudo mount /dev/sdb2 /mnt
sudo cp -a ~/my_rootfs/* /mnt/
sudo sync
sudo umount /mnt
```
### Bước 3: Cấu hình `bootargs` để liên kết Kernel.


Mở cổng giao tiếp với BBB.
```bash
sudo picocom -b 115200 /dev/ttyUSB0
```
Trong giao diện dòng lệnh của Bootloader (thường là U-Boot), tiến hành thiết lập biến `bootargs`:
```bash
setenv bootargs console=ttyO0,115200n8 root=/dev/mmcblk0p2 rootfstype=ext4 rw
saveenv
```

### Bước 4: Khởi động và test
1. Nạp Kernel
```bash
load mmc 0:1 0x81000000 zImage
load mmc 0:1 0x82000000 am335x-boneblack.dtb
bootz 0x81000000 - 0x82000000
```
Khi thấy dòng này ngĩa là đã nạp thành công
```bash
Please press Enter to activate this console.
```
2. Dùng thử các lệnh

ls
echo
cat
```bash
ls -la

echo "Hello Embedded World!"

cat /proc/version
```








