// public domain
#include <stdio.h>
#include <windows.h>

//not available in TCC
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR, int*);

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