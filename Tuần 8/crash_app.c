#include <stdio.h>

void cause_crash() {
    int *ptr = NULL;
    *ptr = 42; // Loi o day
}

int main() {
    printf("Chuan bi gay loi Segmentation fault...\n");
    cause_crash();
    return 0;
}
