#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#ifdef __TINYC__
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR,int*);
BOOL WINAPI PathStripToRootW(wchar_t*);
BOOL WINAPI PathIsRootW(wchar_t*);
#else
#include <shellapi.h>
#include <setupapi.h>
#endif

int main (int argc,char *argv[]) {
  wchar_t root[1000];
  unsigned int type;
  unsigned int i;
  if( argc < 2 )
    GetCurrentDirectoryW(sizeof(root),root);
  else {
    wchar_t** wargv = CommandLineToArgvW (GetCommandLineW(), &argc);
    wcscpy(root,wargv[1]);
  }
  if (GetDriveTypeW(root) == 1) {
    if (!PathStripToRootW(root))
        return 1;
  }
  switch (GetDriveTypeW(root)) {
    case (2) : printf("REMOVABLE\n"); break;
    case (3) : printf("FIXED\n"); break;
    case (4) : printf("REMOTE\n"); break;
    case (5) : printf("CDROM\n"); break;
    case (6) : printf("RAMDISK\n"); break;
  }
  return 1;
}