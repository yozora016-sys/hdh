<div align="center">

# Bài tập Hệ điều hành Nhúng: U-Boot & Kernel 

</div>

---

## Tổng quan (Overview)
Dự án này thực hiện quy trình xây dựng một hệ thống nhúng Linux cơ bản từ mã nguồn (from scratch). Mục tiêu bao gồm biên dịch Bootloader (U-Boot), Linux Kernel và nạp chúng lên bo mạch BeagleBone Black (BBB) để kiểm chứng quá trình khởi động phần cứng.

### Yêu cầu đạt được
1.  **U-Boot:** Khởi tạo U-boot thành công.
2.  **Kernel:** Kernel khởi động thành công.

---

## Phần 1: U-Boot (Bootloader)

### 1.1. Biên dịch (Compilation)
Sử dụng Cross-Compiler Toolchain đã được cấu hình trước đó để build U-Boot.

| File đầu ra | Mô tả |
| :--- | :--- |
| `MLO` | Secondary Program Loader (Chạy trước). |
| `u-boot.img` | Main Bootloader (Giao diện dòng lệnh). |

```bash
# Clean project
make distclean

# Cấu hình cho BBB
make am335x_evm_defconfig

# Biên dịch
make -j4
```
### 1.2. Cài đặt lên thẻ nhớ (Installation)
Thẻ nhớ được chia phân vùng Boot (FAT32). Các file cần được copy theo thứ tự nghiêm ngặt:

Copy MLO (Bắt buộc copy đầu tiên).

Copy u-boot.img.

### 1.3. Kiểm thử (Verification)
Kết nối BBB với máy tính qua UART (USB-TTL) và sử dụng Terminal (Putty/Minicom).

Kết quả mong đợi:

[x] Hiển thị đúng phiên bản U-Boot (Build date).

[x] Hiển thị thông tin phần cứng (DRAM: 512 MiB, MMC,...).

[x] Gõ lệnh tương tác được với hệ thống.

## Phần 2: Linux Kernel
### 2.1. Biên dịch Kernel
Sử dụng Toolchain đồng nhất với U-Boot. Quá trình tạo ra 2 thành phần:

zImage: Nhân hệ điều hành (Compressed Kernel Image).

am335x-boneblack.dtb: Cấu hình phần cứng (Device Tree Blob).

```Bash
# Biên dịch Kernel & DTB
make -j4 zImage dtbs
```
### 2.2. Boot Kernel từ U-Boot
Sau khi copy zImage và file .dtb vào thẻ nhớ, thực hiện các bước sau trên giao diện U-Boot:

Bước 1: Setup môi trường
```bash
setenv bootargs console=ttyS0,115200n8
```
Bước 2: Load file vào RAM
```Bash
# Load Kernel vào địa chỉ 0x82000000
load mmc 0:1 0x82000000 zImage

# Load DTB vào địa chỉ 0x88000000
load mmc 0:1 0x88000000 am335x-boneblack.dtb
```
Bước 3: Khởi động (Bootz)
```Bash
bootz 0x82000000 - 0x88000000
```
Kết quả thực nghiệm
Sau khi gõ lệnh bootz, hệ thống hiển thị log khởi động của Linux:

Starting kernel ... (U-Boot đã chuyển quyền thành công).

Linux version... (Hiển thị đúng phiên bản đã build).

CPU/Memory: Nhận diện đúng CPU ARM Cortex-A8.

Trạng thái cuối: Hệ thống dừng lại và chờ nạp Root Filesystem 

Kết luận: Hoàn thành yêu cầu nạp U-Boot và Kernel lên phần cứng thật.

