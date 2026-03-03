# Bài tập Hệ điều hành Nhúng - System Build với Buildroot

**Mục tiêu:** - Sử dụng Buildroot để biên dịch một hệ điều hành Linux nhúng tùy chỉnh cho bo mạch BeagleBone Black (BBB).
- Tự động hóa quá trình tạo Toolchain, Bootloader (U-Boot/MLO), Kernel (zImage), Device Tree (DTB) và Root Filesystem.
- Tùy chỉnh hệ điều hành: Thêm các phần mềm tiện ích (`nano`, `vim`, `htop`).
- Viết chương trình C cơ bản, sử dụng Toolchain để biên dịch chéo (cross-compile) và chạy thử nghiệm thành công trên BBB.

---

## Phần 1: Cấu hình và Biên dịch Buildroot cho BBB (Bài tập 01)

### 1.1 Cấu hình hệ thống 
Tải và di chuyển vào thư mục Buildroot và mở giao diện cấu hình:
```bash
git clone https://gitlab.com/buildroot.org/buildroot.git
cd buildroot
make menuconfig
```
Cấu hình như tài liệu tham khảo trang 53, 54. Ngoài ra cần bật thêm U-boot trong Bootloader.

### 1.2 Tùy chỉnh gói phần mềm (Packages)
Vào mục **Target packages** để thêm các công cụ quản trị hệ thống vào RootFS:
- **Text editors and viewers:** Tích chọn `[*] nano` và `[*] vim`.
<img width="740" height="446" alt="image" src="https://github.com/user-attachments/assets/d1990c55-fa26-4444-acb7-ce1322d83349" />

- **System tools:** Tích chọn `[*] htop`.
<img width="739" height="441" alt="image" src="https://github.com/user-attachments/assets/cb31ff50-893d-4346-b72b-438af8218294" />



Lưu cấu hình và tiến hành biên dịch (Quá trình này sẽ mất thời gian cho lần build đầu tiên):
```bash
make
```
**Kết quả:** Các file image hoàn chỉnh sẽ được tạo tại `output/images/` bao gồm: `MLO`, `u-boot.img`, `zImage`, `am335x-boneblack-custom.dtb`, và `rootfs.tar`.

---

## Phần 2: Sử dụng Toolchain từ Buildroot (Bài tập 02)

### 2.1 Viết mã nguồn ứng dụng C
Tạo file `hello.c` trên máy Host PC:
```c
#include <stdio.h>

int main() {
    printf("hello\n");
    return 0;
}
```

### 2.2 Biên dịch chéo (Cross-compile)
Xuất đường dẫn chứa Toolchain của Buildroot vào biến môi trường và tiến hành biên dịch:
```bash
export PATH=~/embedded-linux/buildroot/buildroot/output/host/bin:$PATH
arm-linux-gcc -o hello hello.c
```
Kiểm tra lại bằng lệnh `file hello`, kết quả trả về `ELF 32-bit LSB executable, ARM` chứng tỏ file đã được dịch đúng kiến trúc.

---

## Phần 3: Đưa hệ điều hành và Ứng dụng lên thẻ nhớ SD


**1. Copy file khởi động và Kernel vào phân vùng Boot:**
```bash
cd ~/embedded-linux/buildroot/buildroot/output/images/
cp MLO u-boot.img zImage am335x-boneblack-custom.dtb /media/user/boot/
```

**2. Giải nén Root Filesystem vào phân vùng Rootfs:**
```bash
sudo tar xpf rootfs.tar -C /media/user/rootfs
```

**3. Đưa chương trình C vào RootFS:**
```bash
sudo cp ~/hello /media/user/rootfs/usr/bin/
```

**4. Đồng bộ và tháo thẻ an toàn:**
```bash
sync
sudo umount /media/user/boot
sudo umount /media/user/rootfs
```

---

## Phần 4: Khởi chạy và Kiểm thử trên BeagleBone Black

1. Lắp thẻ microSD vào khe cắm của mạch BeagleBone Black.
2. Mở kết nối Serial trên Host PC:
   ```bash
   sudo picocom -b 115200 /dev/ttyUSB0
   ```
3. Nhấn giữ nút **S2 (BOOT)** trên mạch và cắm nguồn để ép mạch khởi động từ thẻ SD.
4. Nạp Kernel
```bash
=> setenv bootargs console=ttyS0,115200n8 root=/dev/mmcblk0p2 rootwait rw
=> setenv bootcmd 'load mmc 0:1 0x81000000 zImage; load mmc 0:1 0x82000000 am335x-boneblack-custom.dtb; bootz 0x81000000 - 0x82000000'
=> saveenv
=> boot

```
5. Chờ hệ thống nạp Kernel. Khi màn hình hiện `Welcome to Buildroot`, đăng nhập bằng tài khoản `root` (không cần mật khẩu).
6. **Kiểm tra phần mềm cài thêm:** Gõ lệnh `htop` hoặc `nano` để xác nhận package đã hoạt động.
7. **Kiểm tra chương trình biên dịch chéo:** Gõ lệnh ứng dụng tại terminal:
   ```bash
   # hello
   hello
   ```
<img width="735" height="493" alt="Screenshot 2026-03-02 110532" src="https://github.com/user-attachments/assets/88795278-d045-4cb8-89ba-976949f8abef" />

------------------------------------







