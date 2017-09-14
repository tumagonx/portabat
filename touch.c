#include <windows.h>
#include <stdio.h>
#ifdef __TINYC__
BOOL DECLSPEC_IMPORT WINAPI TouchFileTimes(HANDLE ,LPSYSTEMTIME);
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR, int*);
#else
#include <imagehlp.h>
#endif
int main (int argc, char **argv) {
  HANDLE hFile;
  BOOL res;
  wchar_t** wargv = CommandLineToArgvW (GetCommandLineW(), &argc);
  if (argc != 2) {
    printf("Usage: %S file\n", wargv[0]);
    return 1;
  }
  hFile = CreateFileW(wargv[1], GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
  if (!(hFile == INVALID_HANDLE_VALUE)) {
    res = TouchFileTimes(hFile, NULL);
    CloseHandle(hFile);
  }
  if (res)
    return 0;
  printf("Error: %S\n", wargv[1]);
  return 1;
}