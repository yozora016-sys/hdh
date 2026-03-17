#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    // Mở device file đã được tạo bởi driver
    int fd = open("/dev/my_led", O_RDWR);
    if (fd < 0) {
        perror("Không thể mở thiết bị /dev/my_led");
        return -1;
    }

    printf("Bắt đầu chương trình Blink LED...\n");

    // Vòng lặp nhấp nháy LED
    while(1) {
        write(fd, "1", 1);
        printf("LED ON\n");
        sleep(1); // Dừng 1 giây
        
        write(fd, "0", 1);
        printf("LED OFF\n");
        sleep(1); // Dừng 1 giây
    }

    close(fd);
    return 0;
}
