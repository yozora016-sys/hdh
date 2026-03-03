# Bài tập Hệ điều hành Nhúng - System Build với Buildroot

**Mục tiêu:** - Sử dụng Buildroot để biên dịch một hệ điều hành Linux nhúng tùy chỉnh cho bo mạch BeagleBone Black (BBB).
- Tự động hóa quá trình tạo Toolchain, Bootloader (U-Boot/MLO), Kernel (zImage), Device Tree (DTB) và Root Filesystem.
- Tùy chỉnh hệ điều hành: Thêm các phần mềm tiện ích (`nano`, `vim`, `htop`).
- Viết chương trình C cơ bản, sử dụng Toolchain để biên dịch chéo (cross-compile) và chạy thử nghiệm thành công trên BBB.

**Môi trường thực hành:**
- **Host PC:** Ubuntu Linux (Tài khoản: yozora)
- **Target Board:** BeagleBone Black (BBB) - Chip TI AM335x
- **Giao tiếp:** Serial Console qua cáp USB-to-UART (Baudrate: 115200)

---

## Phần 1: Cấu hình và Biên dịch Buildroot cho BBB (Bài tập 01)

### 1.1 Cấu hình hệ thống và Bootloader
Di chuyển vào thư mục Buildroot và mở giao diện cấu hình:
```bash
cd ~/embedded-linux/buildroot/buildroot
make menuconfig
```

Thực hiện các thiết lập chính:
1. **Target Options:** Chọn kiến trúc ARM (Cortex-A8).
2. **Bootloaders:** Cấu hình để Buildroot tự động biên dịch U-Boot:
   - Tích chọn `[*] U-Boot`.
   - **U-Boot board defconfig:** `am335x_evm`
   - **U-Boot Version:** `Custom version` -> Nhập `2023.04` (hoặc phiên bản tương ứng).
   - **U-Boot binary format:** Chọn `u-boot.img`.
   - Tích chọn `[*] Install U-Boot SPL binary image` và nhập tên là `MLO`.

### 1.2 Tùy chỉnh gói phần mềm (Packages)
Vào mục **Target packages** để thêm các công cụ quản trị hệ thống vào RootFS:
- **Text editors and viewers:** Tích chọn `[*] nano` và `[*] vim`.
- **System tools:** Tích chọn `[*] htop`.

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

Giả sử thẻ nhớ đã được chia làm 2 phân vùng (boot - FAT32 và rootfs - ext4) và được mount tại `/media/yozora/boot` và `/media/yozora/rootfs`.

**1. Copy file khởi động và Kernel vào phân vùng Boot:**
```bash
cd ~/embedded-linux/buildroot/buildroot/output/images/
cp MLO u-boot.img zImage am335x-boneblack-custom.dtb /media/yozora/boot/
```

**2. Giải nén Root Filesystem vào phân vùng Rootfs:**
```bash
sudo tar xpf rootfs.tar -C /media/yozora/rootfs
```

**3. Đưa chương trình C vào RootFS:**
```bash
sudo cp ~/hello /media/yozora/rootfs/usr/bin/
```

**4. Đồng bộ và tháo thẻ an toàn:**
```bash
sync
sudo umount /media/yozora/boot
sudo umount /media/yozora/rootfs
```

---

## Phần 4: Khởi chạy và Kiểm thử trên BeagleBone Black

1. Lắp thẻ microSD vào khe cắm của mạch BeagleBone Black.
2. Mở kết nối Serial trên Host PC:
   ```bash
   sudo picocom -b 115200 /dev/ttyUSB0
   ```
3. Nhấn giữ nút **S2 (BOOT)** trên mạch và cắm nguồn để ép mạch khởi động từ thẻ SD.
4. Chờ hệ thống nạp Kernel. Khi màn hình hiện `Welcome to Buildroot`, đăng nhập bằng tài khoản `root` (không cần mật khẩu).
5. **Kiểm tra phần mềm cài thêm:** Gõ lệnh `htop` hoặc `nano` để xác nhận package đã hoạt động.
6. **Kiểm tra chương trình biên dịch chéo:** Gõ lệnh ứng dụng tại terminal:
   ```bash
   # hello
   hello
   ```

**Hoàn thành xuất sắc bài tập System Build & Cross-Compile!**
