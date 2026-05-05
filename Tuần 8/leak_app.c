#include <stdlib.h>
#include <stdio.h>

int main() {
    printf("Cap phat bo nho nhung khong giai phong...\n");
    int *arr = (int*)malloc(100 * sizeof(int));
    arr[0] = 10;
    // Khong goi free(arr);
    return 0;
}
