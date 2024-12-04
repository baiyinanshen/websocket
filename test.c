#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    // 检查 F_LOCK 是否定义
    #ifdef F_TLOCK
        printf("F_TLOCK is defined!\n");
    #else
        printf("F_TLOCK is NOT defined!\n");
    #endif

    return 0;
}
