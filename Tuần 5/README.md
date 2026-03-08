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

**1.Bật cJSON trong Buildroot:**
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
3. Biên dịch chéo và nạp xuống mạch:
Trỏ Toolchain
```bash
export CC=~/embedded-linux/buildroot/buildroot/output/host/bin/arm-linux-gcc
```
Biên dịch liên kết với cJSON
```bash
$CC HelloJSON.c -o HelloJSON -lcjson
```
Copy vào thẻ nhớ (Cần copy cả thư viện động của cJSON xuống BBB)
```bash
sudo cp HelloJSON /media/$USER/rootfs/root/
sudo cp -a ~/embedded-linux/buildroot/buildroot/output/target/usr/lib/libcjson.so* /media/$USER/rootfs/usr/lib/
```
4. Khởi chạy:
Truy cập BBB qua Picocom, cấp quyền và chạy:
```Bash
chmod +x HelloJSON
./HelloJSON
```
<img width="439" height="100" alt="Screenshot 2026-03-08 194321" src="https://github.com/user-attachments/assets/aa7f3ea8-a0fe-4b16-93b9-0800aecefbc7" />

Bài tập 02: Tự tạo thư viện cá nhân 

Mục tiêu: Viết thư viện tính toán cơ bản (file .h và .c), biên dịch tĩnh/động và so sánh.

1. Mã nguồn thư viện mathlib.h và mathlib.c:
```bash
//mathlib.h
#ifndef MATHLIB_H
#define MATHLIB_H
int add_numbers(int a, int b);
#endif
```
```bash
// mathlib.c
#include "mathlib.h"
int add_numbers(int a, int b)
{
return a + b;
}
```
2. Biên dịch thư viện (.a và .so) và đưa vào Sysroot:

```Bash
export AR=~/buildroot/output/host/bin/arm-buildroot-linux-gnueabihf-ar
```
Tạo Object file
```
$CC -c -fPIC mathlib.c -o mathlib.o
```
Tạo Static Library (.a)
```
$AR rcs libmathlib.a mathlib.o
```
Tạo Dynamic Library (.so)
```
$CC -shared -o libmathlib.so mathlib.o
```
Copy vào Sysroot của Buildroot
```
cp mathlib.h embedded-linux/buildroot/buildroot/output/host/arm-linux-gnueabihf/sysroot/usr/include/
cp libmathlib.a libmathlib.so embedded-linux/buildroot/buildroot/output/host/arm-linux-gnueabihf/sysroot/usr/lib/
```
3. Ứng dụng app.c và quá trình biên dịch:
```
#include <stdio.h>
#include "mathlib.h"
int main() {
    printf("Ket qua: 15 + 25 = %d\n", add_numbers(15, 25));
    return 0;
}
```
Biên dịch 2 phiên bản
```
$CC app.c -o app_static -lmathlib -static
$CC app.c -o app_dynamic -lmathlib
```
5. Thử nghiệm và So sánh:
Copy file thực thi vào thẻ nhớ và thư viện động xuống mạch
```
sudo cp app_static app_dynamic /media/$USER/rootfs/root/
sudo cp libmathlib.so /media/$USER/rootfs/usr/lib/
```
Cắm vào BBB và chạy thử
```
./app_static
./app_dynamic
```
<img width="303" height="145" alt="image" src="https://github.com/user-attachments/assets/644a77ba-c7c7-4f07-9244-7ee7af651985" />


Đánh giá dung lượng (Bản static nặng hơn rất nhiều do chứa sẵn mã nguồn)
```
ls -lh app_static app_dynamic
```
<img width="521" height="83" alt="image" src="https://github.com/user-attachments/assets/399d6b48-73a9-4c29-8407-1feb464c57aa" />


Phân tích phụ thuộc (Bản dynamic cần libc.so và libmathlib.so)
```
~embedded-linux/buildroot/buildroot/output/host/bin/arm-linux-readelf -d app_dynamic
```
<img width="878" height="578" alt="image" src="https://github.com/user-attachments/assets/9387d05f-d655-4a37-aa61-39b20dc667c4" />

```
~embedded-linux/buildroot/buildroot/output/host/bin/arm-linux-readelf -d app_static
```
<img width="883" height="99" alt="image" src="https://github.com/user-attachments/assets/12b821e9-782a-4ca9-abf0-2855de8bc19c" />


Bài tập 03: Tích hợp ứng dụng, thư viện vào Buildroot 

Bước 1: Tạo một thư mục chứa code ở ngoài, sau đó cấu hình để Buildroot trỏ tới đó.

Tạo thư mục làm việc:
```
mkdir -p ~/workspace/libmathlib
mkdir -p ~/workspace/myapp
```
Tạo file mathlib.c, mathlib.h như ở bài 2 vào thư mục ~/workspace/libmathlib/.
Tạo thêm file Makefile tại ~/workspace/libmathlib/Makefile với nội dung:
```
	$(CC) $(CFLAGS) -c -fPIC mathlib.c -o mathlib.o
	$(CC) $(LDFLAGS) -shared -o libmathlib.so mathlib.o
```
Tạo file myapp.c tại ~/workspace/myapp/myapp.c kết hợp cả cJSON và mathlib:
```
#include <stdio.h>
#include "mathlib.h"
#include <cjson/cJSON.h>

int main() {
    // 1. Test Mathlib
    int sum = add_numbers(10, 20);
    printf("Mathlib Test: 10 + 20 = %d\n", sum);

    // 2. Test cJSON
    const char *json_string = "{\"name\":\"BeagleBone\", \"status\":\"active\"}";
    cJSON *json = cJSON_Parse(json_string);
    if (json != NULL) {
        printf("cJSON Test - Name: %s\n", cJSON_GetObjectItem(json, "name")->valuestring);
        cJSON_Delete(json);
    }
    return 0;
}
```
Tạo file Makefile tại ~/workspace/myapp/Makefile:
```
	$(CC) $(CFLAGS) myapp.c -o myapp $(LDFLAGS) -lmathlib -lcjson
```

Bước 2: Tạo Package thư viện libmathlib trong Buildroot

Trỏ vào thư mục buildroot và tạo thư mục
```
mkdir -p package/libmathlib
```

Tạo file package/libmathlib/Config.in:
```
config BR2_PACKAGE_LIBMATHLIB
    bool "libmathlib"
    help
      Thu vien toan hoc .
```
Tạo file package/libmathlib/libmathlib.mk:
```
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
```
Bước 3: Tạo Package ứng dụng myapp trong Buildroot

Tạo thư mục: mkdir -p package/myapp

Tạo file package/myapp/Config.in. 
```
config BR2_PACKAGE_MYAPP
    bool "myapp"
    select BR2_PACKAGE_CJSON
    select BR2_PACKAGE_LIBMATHLIB
    help
      Ung dung tich hop cJSON va libmathlib.
```
Tạo file package/myapp/myapp.mk. 
```
MakefileMYAPP_VERSION = 1.0
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
```
Bước 4: Khai báo vào Menu chung và kích hoạt

Mở file package/Config.in và chỉnh sửa
```
nano package/Config.in
//Trong menu "Other" thêm
source "package/libmathlib/Config.in"
//Trong menu "Miscellaneous" thêm
source "package/myapp/Config.in"
```
Vào thư mục buildroot và make menuconfig
```
---->Target packages ---> Miscellaneous ---> [*] myapp
```

Save và make

Bước 5: Cài đặt và khởi chạy

Giải nén và nạp lại file rootfs.tar vào thẻ SD
```
sudo rm -rf /media/$USER/rootfs/*
sudo tar -xf output/images/rootfs.tar -C /media/$USER/rootfs/
sync
sudo umount /media/$USER/rootfs
```

Gõ lệnh myapp
<img width="280" height="109" alt="image" src="https://github.com/user-attachments/assets/21cbcf74-a09f-42a6-b074-3e1a3d0f2715" />

-----------------
Mở picocom: sudo picocom -b 115200 /dev/ttyUSB0




