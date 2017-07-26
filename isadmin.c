// public domain
#include <stdio.h>
#include <windows.h>

// TCC don't have it
BOOL IsUserAnAdmin(void);

int main(void) {
    if (IsUserAnAdmin()) {
            printf("1\n");
            return 0;
    }
    printf("0\n");
    return 1;
}