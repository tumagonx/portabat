// public domain
#include <stdio.h>
#include <windows.h>

#ifdef __TINYC__
BOOL IsUserAnAdmin(void);
#endif
int main(void) {
    if (IsUserAnAdmin()) {
            printf("1\n");
            return 0;
    }
    printf("0\n");
    return 1;
}