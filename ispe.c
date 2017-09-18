/* Assumption: ispe is 32bit and OS is at minimum XP (no OS2 and POSIX emulation)
 * This app return error if file not a PE, 
 * or if PE but not "executable" with respect to Windows bitness
 */
 
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#ifdef __TINYC__
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR,int*);
BOOL WINAPI IsWow64Process(LPCTSTR, PBOOL);
#else
#include <shellapi.h>
#endif


// copied from mingw-w64 gendef
/*
    Copyright (C) 2009, 2010, 2011, 2012, 2013  mingw-w64 project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

static PVOID revert; /*revert pointer*/
static HMODULE kernel32handle;
typedef WINBOOL (__stdcall (*redirector))(PVOID *);
typedef WINBOOL (__stdcall (*revertor))(PVOID);
static redirector redirectorfunction; /*Wow64DisableWow64FsRedirection*/
static revertor revertorfunction;     /*Wow64RevertWow64FsRedirection*/

static void undoredirect(void) {
    revertorfunction(revert);
}

void doredirect(const int redir) {
  if (redir) {
    kernel32handle = GetModuleHandleW(L"kernel32.dll");
    if (!kernel32handle)
      return;
    redirectorfunction = (redirector)GetProcAddress(kernel32handle, "Wow64DisableWow64FsRedirection");
    revertorfunction = (revertor)GetProcAddress(kernel32handle, "Wow64RevertWow64FsRedirection");
    if (!redirectorfunction || ! revertorfunction) {
      FreeLibrary(kernel32handle);
      return;
    }
    if (!redirectorfunction(&revert))
      return;
    else
      atexit(undoredirect);
  }
}
// end of gendef chunk


typedef struct _PETYPE {
  wchar_t	*name;
  DWORD type;
  DWORD isexe;
} PETYPE, * PPETYPE;

PETYPE PEList [] = {
  { L"WIN32", 0, 0},
  { L"MSDOS", 1, 2},
  { L"WIN16", 2, 2},
  { L"PIF", 3, 2},
  { L"POSIX", 4, 1},
  { L"OS216", 5, 1},
  { L"WIN64", 6, 2}
};

int main (int argc,char *argv[]) {
  DWORD BinaryType;
  BOOL iswow = FALSE;
  unsigned int i; 
  if( argc < 2 ) {
    printf ("No file specified\n");
    return -1;
  }
  wchar_t** wargv = CommandLineToArgvW (GetCommandLineW(), &argc);
  IsWow64Process(GetCurrentProcess(), &iswow);
  doredirect(iswow);
  if (GetBinaryTypeW(wargv[1], &BinaryType))
    for (i=0; i<=6; i++) 
    {
      if (BinaryType == PEList[i].type) 
      {
        undoredirect();
        printf("%S\n", PEList[i].name);
        if (2 == PEList[i].isexe) 
        {
          if (iswow)
            if (6 == PEList[i].type)
              return 0;
            else
              return 1;
          else
            return 0;
        }
        return PEList[i].isexe;
      }
    }
  undoredirect();
  return 1;
}