#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

void delay_work() {
    int sum = 0;
    for(int i = 0; i < 1000000; i++) {
        sum += i;
    }
}

int main() {
    int counter = 0;
    printf("Bat dau chuong trinh chinh...\n");
    
    // Ghi file de test strace
    int fd = open("/tmp/test.txt", O_CREAT | O_WRONLY, 0644);
    if(fd > 0) {
        write(fd, "Hello BBB\n", 10);
        close(fd);
    }

    while(counter < 5) {
        printf("Vong lap thu: %d\n", counter);
        delay_work(); // Goi ham de test perf va gdb step
        counter++;
        sleep(1);
    }
    
    printf("Ket thuc!\n");
    return 0;
}
