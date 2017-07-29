#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <windows.h>
#ifdef __TINYC__
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR, int*);
#else
#inlude <shellapi.h>
#endif
int WINAPI WinMain(HINSTANCE hCurInstance, HINSTANCE hPrevInstance,
		   LPSTR lpCmdLine, int nCmdShow)
{
  wchar_t** wargv = CommandLineToArgvW (GetCommandLineW(), &__argc);
  return MessageBoxW (NULL, __argc > 1 ? wargv[1] : L"Usage:\nmsgbox [msg] [0-6] [title]",
		    __argc > 3 ? wargv[3] : NULL,
		    __argc > 2 ? wcstoul(wargv[2], NULL, 0) : 0);
}

