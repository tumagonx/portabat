// public domain
#include <stdio.h>
#include <windows.h>

#ifdef __TINYC__
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR, int*);
#endif
int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s [src] [dest]\n", argv[0]);
        return -1;
    }
    wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (CreateHardLinkW(wargv[2], wargv[1], NULL))
        return 0;
    return 1;
}