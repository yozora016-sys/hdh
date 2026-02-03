Bài tập HDH Nhúng: Xây dựng U-Boot và Kernel

Phần 1: Biên dịch và Cài đặt U-Boot
1.1. Thiết lập môi trường
Export biến môi trường trỏ đến Toolchain đã tạo:

Bash
export ARCH=arm
export CROSS_COMPILE=/path/to/your/custom/toolchain/bin/arm-custom-linux-gnueabi-
1.2. Biên dịch U-Boot
Sử dụng cấu hình mặc định cho BeagleBone Black (am335x_evm_defconfig):

Bash
make distclean
make am335x_evm_defconfig
make -j4
Kết quả đầu ra: Sau khi biên dịch thành công, hai file quan trọng được tạo ra:

MLO (Secondary Program Loader)

u-boot.img (Main Bootloader)

1.3. Chuẩn bị thẻ nhớ (SD Card)
Phân vùng thẻ nhớ (ví dụ /dev/sdb) thành phân vùng boot (FAT32/FAT16) và cài đặt U-Boot:

Format phân vùng boot định dạng FAT.

Copy file vào thẻ nhớ:

Bash
cp MLO /media/user/BOOT/
cp u-boot.img /media/user/BOOT/
(Lưu ý: MLO phải được copy trước)

1.4. Kiểm thử U-Boot (Debug qua UART)
Cắm thẻ nhớ vào BBB -> Nhấn giữ nút Boot (S2) -> Cấp nguồn.

Kết quả trên Terminal:

[x] Hiển thị phiên bản U-Boot (Build date/time).

[x] Hiển thị thông tin phần cứng (CPU, Board model, DRAM size).

[x] Dấu nhắc lệnh => xuất hiện, gõ lệnh version hoặc bdinfo có phản hồi.

Phần 2: Biên dịch và Cài đặt Kernel
2.1. Biên dịch Kernel
Sử dụng cùng Toolchain với U-Boot để đảm bảo tính đồng nhất.

Bash
# Cấu hình cho BBB (thường dùng bb.org_defconfig hoặc multi_v7_defconfig)
make bb.org_defconfig

# Biên dịch Kernel image và Device Tree Blobs
make -j4 zImage dtbs
Kết quả đầu ra:

arch/arm/boot/zImage

arch/arm/boot/dts/am335x-boneblack.dtb

2.2. Đưa file vào thẻ nhớ
Copy 2 file vừa tạo vào cùng phân vùng với U-Boot trên thẻ nhớ:

Bash
cp arch/arm/boot/zImage /media/user/BOOT/
cp arch/arm/boot/dts/am335x-boneblack.dtb /media/user/BOOT/
2.3. Khởi động Kernel từ U-Boot (lệnh bootz)
Khởi động lại BBB, nhấn phím bất kỳ để dừng ở U-Boot =>.

Nạp Kernel và DTB vào RAM (Địa chỉ vật lý của BBB):

Bash
# Load zImage vào địa chỉ 0x82000000
load mmc 0:1 0x82000000 zImage

# Load DTB vào địa chỉ 0x88000000
load mmc 0:1 0x88000000 am335x-boneblack.dtb
Khởi động Kernel:

Bash
bootz 0x82000000 - 0x88000000
2.4. Kết quả khởi động Kernel
Quan sát trên màn hình Terminal:

[x] Thông báo Starting kernel ... xuất hiện.

[x] Log khởi động của Linux Kernel hiển thị (phiên bản Linux, CPU init, Memory map).

[x] Hệ thống dừng lại ở dòng trạng thái chờ rootfs (ví dụ: Waiting for root device... hoặc Kernel panic - not syncing: VFS: Unable to mount root fs) -> Thành công bước nạp Kernel.
