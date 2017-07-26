// Public domain
#include <windows.h>
int main (int argc, char *argv[]) {
    unsigned int hz;
    if(argc < 3) {
        printf("Usage: %s [frequency] [duration]\n", argv[0]);
    return -1;
    }
    if (abs( atoi( argv[1])) < 32)
        hz = 32;
    else if (abs (atoi (argv[1])) > 32000)
        hz=32000;
    else 
        hz=abs (atoi (argv[1]));

    if (Beep (hz /*32 - 32Khz*/, abs (atoi (argv[2])) /*milisecond*/))
        return 0;
    return 1;
}