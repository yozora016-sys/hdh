# Báo cáo Bài tập Hệ điều hành Nhúng - Biên dịch chéo thư viện và ứng dụng


## 📖 Tổng quan dự án
Dự án này giải quyết 03 bài toán cốt lõi trong phát triển Linux nhún:
1. Sử dụng thư viện có sẵn (cJSON) bằng Toolchain của Buildroot.
2. Tự thiết kế thư viện tĩnh/động (Static/Dynamic Library) và quản lý Sysroot.
3. Đóng gói ứng dụng và thư viện thành một Package chuẩn, tích hợp sâu vào quy trình tự động của Buildroot.

---

## 🚀 Hướng dẫn thực hiện chi tiết

### Bài tập 01: Biên dịch ứng dụng với thư viện đã có 
**Mục tiêu:** Viết chương trình C/C++ parse gói tin JSON và in lên Terminal.

**1.Bật cJSON trong Buildroot[cite: 4]:**
```bash
make menuconfig
# Đường dẫn: Target packages -> Libraries -> JSON/XML -> Chọn [*] cJSON
make
```
2. Mã nguồn HelloJSON.c:
```bash
#include <stdio.h>
#include <cjson/cJSON.h>

int main() {
    const char *json_string = "{\"name\":\"BeagleBone\", \"status\":\"active\"}";
    cJSON *json = cJSON_Parse(json_string);
    if (json != NULL) {
        printf("Parsed Data - Name: %s, Status: %s\n", 
               cJSON_GetObjectItem(json, "name")->valuestring,
               cJSON_GetObjectItem(json, "status")->valuestring);
        cJSON_Delete(json);
    }
    return 0;
}
```
3. Biên dịch chéo và nạp xuống mạch:Bash# Trỏ Toolchain
export CC=~/buildroot/output/host/bin/arm-buildroot-linux-gnueabihf-gcc

# Biên dịch liên kết với cJSON
$CC HelloJSON.c -o HelloJSON -lcjson

# Copy vào thẻ nhớ (Cần copy cả thư viện động của cJSON xuống BBB)
sudo cp HelloJSON /media/$USER/rootfs/root/
sudo cp -a ~/buildroot/output/target/usr/lib/libcjson.so* /media/$USER/rootfs/usr/lib/
4. Khởi chạy:
Truy cập BBB qua Minicom, cấp quyền và chạy:Bashchmod +x HelloJSON
./HelloJSON
Bài tập 02: Tự tạo thư viện cá nhân Mục tiêu: Viết thư viện tính toán cơ bản (file .h và .c), biên dịch tĩnh/động và so sánh.1. Mã nguồn thư viện mathlib.h và mathlib.c:C// mathlib.h
#ifndef MATHLIB_H
#define MATHLIB_H
int add_numbers(int a, int b);
#endif

// mathlib.c
#include "mathlib.h"
int add_numbers(int a, int b) { return a + b; }
2. Biên dịch thư viện (.a và .so) và đưa vào Sysroot:Bashexport AR=~/buildroot/output/host/bin/arm-buildroot-linux-gnueabihf-ar

# Tạo Object file
$CC -c -fPIC mathlib.c -o mathlib.o

# Tạo Static Library (.a)
$AR rcs libmathlib.a mathlib.o

# Tạo Dynamic Library (.so)
$CC -shared -o libmathlib.so mathlib.o

# Copy vào Sysroot của Buildroot
SYSROOT=~/buildroot/output/host/arm-buildroot-linux-gnueabihf/sysroot
cp mathlib.h $SYSROOT/usr/include/
cp libmathlib.a libmathlib.so $SYSROOT/usr/lib/
3. Ứng dụng app.c và quá trình biên dịch:C#include <stdio.h>
#include "mathlib.h"
int main() {
    printf("Ket qua: 15 + 25 = %d\n", add_numbers(15, 25));
    return 0;
}
Bash# Biên dịch 2 phiên bản
$CC app.c -o app_static -lmathlib -static
$CC app.c -o app_dynamic -lmathlib
4. Thử nghiệm và So sánh:Bash# Copy file thực thi và thư viện động xuống mạch
sudo cp app_static app_dynamic /media/$USER/rootfs/root/
sudo cp libmathlib.so /media/$USER/rootfs/usr/lib/

# Đánh giá dung lượng (Bản static nặng hơn rất nhiều do chứa sẵn mã nguồn)
ls -lh app_static app_dynamic

# Phân tích phụ thuộc (Bản dynamic cần libc.so và libmathlib.so)
~/buildroot/output/host/bin/arm-buildroot-linux-gnueabihf-readelf -d app_dynamic
Bài tập 03: Tích hợp ứng dụng, thư viện vào Buildroot Mục tiêu: Đóng gói toàn bộ mã nguồn vào hệ thống package của Buildroot để tự động hóa.1. Khởi tạo Package libmathlib:
Cấu trúc: package/libmathlib/Config.in và package/libmathlib/libmathlib.mk.Makefile# libmathlib.mk
LIBMATHLIB_VERSION = 1.0
LIBMATHLIB_SITE = $(HOME)/workspace/libmathlib
LIBMATHLIB_SITE_METHOD = local
LIBMATHLIB_INSTALL_STAGING = YES
LIBMATHLIB_INSTALL_TARGET = YES

define LIBMATHLIB_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef
define LIBMATHLIB_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0644 $(@D)/mathlib.h $(STAGING_DIR)/usr/include/mathlib.h
	$(INSTALL) -D -m 0755 $(@D)/libmathlib.so $(STAGING_DIR)/usr/lib/libmathlib.so
endef
define LIBMATHLIB_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libmathlib.so $(TARGET_DIR)/usr/lib/libmathlib.so
endef
$(eval $(generic-package))
2. Khởi tạo Package myapp (Tích hợp Bài 1 & 2):
Sử dụng cJSON và mathlib. Cấu hình file Config.in thiết lập điều kiện phụ thuộc.Plaintext# myapp/Config.in
config BR2_PACKAGE_MYAPP
	bool "myapp"
	select BR2_PACKAGE_CJSON
	select BR2_PACKAGE_LIBMATHLIB
	help
	  Ung dung tong hop chay tren BBB.
Makefile# myapp/myapp.mk
MYAPP_VERSION = 1.0
MYAPP_SITE = $(HOME)/workspace/myapp
MYAPP_SITE_METHOD = local
MYAPP_DEPENDENCIES = cjson libmathlib

define MYAPP_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef
define MYAPP_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/myapp $(TARGET_DIR)/usr/bin/myapp
endef
$(eval $(generic-package))
3. Khai báo và Biên dịch:Sửa file package/Config.in để thêm đường dẫn đến libmathlib/Config.in và myapp/Config.in.Bật ứng dụng trong make menuconfig (các thư viện phụ thuộc sẽ tự động được chọn).Chạy make để Buildroot biên dịch toàn bộ.

