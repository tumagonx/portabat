// Public domain
#include <windows.h>
int main(int argc,char *argv[]) {
    if( argc < 2 ) {
        printf("Usage: %s [milisecond]\n",argv[0]);
        return -1;
    }
    Sleep(abs(_atoi64(argv[1])));
    return 0;
}