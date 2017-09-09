/* SfxSetup.c - 7z SFX Setup
2017-04-04 : Igor Pavlov : Public domain */

#include "Precomp.h"

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define _CONSOLE

#include <stdio.h>
#include <direct.h>
#include <io.h>

#include "../../7z.h"
#include "../../7zAlloc.h"
#include "../../7zCrc.h"
#include "../../7zFile.h"
#include "../../CpuArch.h"
#include "../../DllSecur.h"

#define kInputBufSize ((size_t)1 << 18)

static const char * const kExts[] =
{
  "bat",
  "cmd",
  "inf",
  "exe",
  "msi",
  #ifdef UNDER_CE
  "cab",
  #endif
  "ps1",
  "vbs"
  "wsh"
};

static const char * const kNames[] =
{
    "setup"
  , "install"
  , "run"
  , "start"
};

typedef struct _SHITEMID {
    USHORT cb;
    BYTE abID[1];
} SHITEMID;
typedef struct _ITEMIDLIST {
    SHITEMID mkid;
} ITEMIDLIST;
typedef ITEMIDLIST *LPITEMIDLIST;
typedef struct _EnvMap {
  int		CSIDL;
  LPCWSTR	DIRID;
} EnvMap, * PEnvMap;

EnvMap CSIDLtoDIRID [] = {
  { 0x0024,				L"_10" }, //CSIDL_WINDOWS
  { 0x0025,				L"_11" }, //CSIDL_SYSTEM
  { 0x0014,				L"_20" }, //CSIDL_FONTS
  { 0x0025,				L"_25" }, //CSIDL_WINDOWS
  { 0x0028,				L"_53" }, //CSIDL_PROFILE
  { 0x0000,				L"_16384" }, //CSIDL_DESKTOP
  { 0x0002,				L"_16386" }, //CSIDL_PROGRAMS
  { 0x0005,				L"_16389" }, //CSIDL_PERSONAL
  { 0x0006,				L"_16390" }, //CSIDL_FAVORITES
  { 0x0007,				L"_16391" }, //CSIDL_STARTUP
  { 0x0008,				L"_16392" }, //CSIDL_RECENT
  { 0x0009,				L"_16393" }, //CSIDL_SENDTO
  { 0x000b,				L"_16395" }, //CSIDL_STARTMENU
  { 0x000d,				L"_16397" }, //CSIDL_MYMUSIC
  { 0x000e,				L"_16398" }, //CSIDL_MYVIDEO
  { 0x0010,				L"_16400" }, //CSIDL_DESKTOPDIRECTORY
  { 0x0013,				L"_16403" }, //CSIDL_NETHOOD
  { 0x0014,				L"_16404" }, //CSIDL_FONTS
  { 0x0015,				L"_16405" }, //CSIDL_TEMPLATES
  { 0x0016,				L"_16406" }, //CSIDL_COMMON_STARTMENU
  { 0x0017,				L"_16407" }, //CSIDL_COMMON_PROGRAMS
  { 0x0018,				L"_16408" }, //CSIDL_COMMON_STARTUP
  { 0x0019,				L"_16409" }, //CSIDL_COMMON_DESKTOPDIRECTORY
  { 0x001a,				L"_16410" }, //CSIDL_APPDATA
  { 0x001b,				L"_16411" }, //CSIDL_PRINTHOOD
  { 0x001c,				L"_16412" }, //CSIDL_LOCAL_APPDATA
  { 0x001f,				L"_16415" }, //CSIDL_COMMON_FAVORITES
  { 0x0020,				L"_16416" }, //CSIDL_INTERNET_CACHE
  { 0x0021,				L"_16417" }, //CSIDL_COOKIES
  { 0x0022,				L"_16418" }, //CSIDL_HISTORY
  { 0x0023,				L"_16419" }, //CSIDL_COMMON_APPDATA
  { 0x0024,				L"_16420" }, //CSIDL_WINDOWS
  { 0x0025,				L"_16421" }, //CSIDL_SYSTEM
  { 0x0026,				L"_16422" }, //CSIDL_PROGRAM_FILES
  { 0x0027,				L"_16423" }, //CSIDL_MYPICTURES
  { 0x0029,				L"_16425" }, //CSIDL_SYSTEMX86
  { 0x002a,				L"_16426" }, //CSIDL_PROGRAM_FILESX86
  { 0x002c,				L"_16428" }, //CSIDL_PROGRAM_FILES_COMMONX86
  { 0x0028,				L"_16424" }, //CSIDL_PROFILE
  { 0x002b,				L"_16427" }, //CSIDL_PROGRAM_FILES_COMMON
  { 0x002d,				L"_16429" }, //CSIDL_COMMON_TEMPLATES
  { 0x002e,				L"_16430" }, //CSIDL_COMMON_DOCUMENTS
  { 0x002f,				L"_16431" }, //CSIDL_COMMON_ADMINTOOLS
  { 0x0030,				L"_16432" }, //CSIDL_ADMINTOOLS
  { 0x0035,				L"_16437" }, //CSIDL_COMMON_MUSIC
  { 0x0036,				L"_16438" }, //CSIDL_COMMON_PICTURES
  { 0x0037,				L"_16439" }, //CSIDL_COMMON_VIDEO
  { 0x0038,				L"_16440" }, //CSIDL_RESOURCES
  { 0x003b,				L"_16443" }, //CSIDL_CDBURN_AREA
};
__declspec(dllimport) HRESULT WINAPI SHGetSpecialFolderLocation (HWND, int, LPITEMIDLIST *);
__declspec(dllimport) int WINAPI SHGetPathFromIDListW (LPITEMIDLIST, LPWSTR);

static unsigned FindExt(const wchar_t *s, unsigned *extLen)
{
  unsigned len = (unsigned)wcslen(s);
  unsigned i;
  for (i = len; i > 0; i--)
  {
    if (s[i - 1] == '.')
    {
      *extLen = len - i;
      return i - 1;
    }
  }
  *extLen = 0;
  return len;
}

#define MAKE_CHAR_UPPER(c) ((((c) >= 'a' && (c) <= 'z') ? (c) -= 0x20 : (c)))

static unsigned FindItem(const char * const *items, unsigned num, const wchar_t *s, unsigned len)
{
  unsigned i;
  for (i = 0; i < num; i++)
  {
    const char *item = items[i];
    unsigned itemLen = (unsigned)strlen(item);
    unsigned j;
    if (len != itemLen)
      continue;
    for (j = 0; j < len; j++)
    {
      unsigned c = (Byte)item[j];
      if (c != s[j] && MAKE_CHAR_UPPER(c) != s[j])
        break;
    }
    if (j == len)
      return i;
  }
  return i;
}

#ifdef _CONSOLE
static BOOL WINAPI HandlerRoutine(DWORD ctrlType)
{
  UNUSED_VAR(ctrlType);
  return TRUE;
}
#endif

static void PrintErrorMessage(const char *message)
{
  printf("\n7-Zip Error: %s\n", message);
}
static void PrintErrorMessageBox(const char *message)
{
  #ifdef UNDER_CE
  WCHAR messageW[256 + 4];
  unsigned i;
  for (i = 0; i < 256 && message[i] != 0; i++)
    messageW[i] = message[i];
  messageW[i] = 0;
  MessageBoxW(0, messageW, L"7-Zip Error", MB_ICONERROR);
  #else
  MessageBoxA(0, message, "7-Zip Error", MB_ICONERROR);
  #endif
}

static WRes MyCreateDir(const WCHAR *name)
{
  return CreateDirectoryW(name, NULL) ? 0 : GetLastError();
}

#ifdef UNDER_CE
#define kBufferSize (1 << 13)
#else
#define kBufferSize (1 << 15)
#endif

#define kSignatureSearchLimit (1 << 22)

static Bool FindSignature(CSzFile *stream, UInt64 *resPos)
{
  Byte buf[kBufferSize];
  size_t numPrevBytes = 0;
  *resPos = 0;
  for (;;)
  {
    size_t processed, pos;
    if (*resPos > kSignatureSearchLimit)
      return False;
    processed = kBufferSize - numPrevBytes;
    if (File_Read(stream, buf + numPrevBytes, &processed) != 0)
      return False;
    processed += numPrevBytes;
    if (processed < k7zStartHeaderSize ||
        (processed == k7zStartHeaderSize && numPrevBytes != 0))
      return False;
    processed -= k7zStartHeaderSize;
    for (pos = 0; pos <= processed; pos++)
    {
      for (; pos <= processed && buf[pos] != '7'; pos++);
      if (pos > processed)
        break;
      if (memcmp(buf + pos, k7zSignature, k7zSignatureSize) == 0)
        if (CrcCalc(buf + pos + 12, 20) == GetUi32(buf + pos + 8))
        {
          *resPos += pos;
          return True;
        }
    }
    *resPos += processed;
    numPrevBytes = k7zStartHeaderSize;
    memmove(buf, buf + processed, k7zStartHeaderSize);
  }
}

static Bool DoesFileOrDirExist(const WCHAR *path)
{
  WIN32_FIND_DATAW fd;
  HANDLE handle;
  handle = FindFirstFileW(path, &fd);
  if (handle == INVALID_HANDLE_VALUE)
    return False;
  FindClose(handle);
  return True;
}

static WRes RemoveDirWithSubItems(WCHAR *path)
{
  WIN32_FIND_DATAW fd;
  HANDLE handle;
  WRes res = 0;
  size_t len = wcslen(path);
  wcscpy(path + len, L"*");
  handle = FindFirstFileW(path, &fd);
  path[len] = L'\0';
  if (handle == INVALID_HANDLE_VALUE)
    return GetLastError();
  
  for (;;)
  {
    if (wcscmp(fd.cFileName, L".") != 0 &&
        wcscmp(fd.cFileName, L"..") != 0)
    {
      wcscpy(path + len, fd.cFileName);
      if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
      {
        wcscat(path, WSTRING_PATH_SEPARATOR);
        res = RemoveDirWithSubItems(path);
      }
      else
      {
        SetFileAttributesW(path, 0);
        if (DeleteFileW(path) == 0)
          res = GetLastError();
      }
    
      if (res != 0)
        break;
    }
  
    if (!FindNextFileW(handle, &fd))
    {
      res = GetLastError();
      if (res == ERROR_NO_MORE_FILES)
        res = 0;
      break;
    }
  }
  
  path[len] = L'\0';
  FindClose(handle);
  if (res == 0)
  {
    if (!RemoveDirectoryW(path))
      res = GetLastError();
  }
  return res;
}

#ifdef _CONSOLE
int MY_CDECL main()
#else
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
  #ifdef UNDER_CE
  LPWSTR
  #else
  LPSTR
  #endif
  lpCmdLine, int nCmdShow)
#endif
{
  CFileInStream archiveStream;
  CLookToRead2 lookStream;
  CSzArEx db;
  SRes res = SZ_OK;
  ISzAlloc allocImp;
  ISzAlloc allocTempImp;
  WCHAR sfxPath[MAX_PATH + 2];
  WCHAR path[MAX_PATH * 3 + 2];
  #ifndef UNDER_CE
  WCHAR workCurDir[MAX_PATH + 32];
  #endif
  size_t pathLen;
  DWORD winRes;
  const wchar_t *cmdLineParams;
  const char *errorMessage = NULL;
  unsigned useShellExecute = 8;
  DWORD exitCode = 0;
  const wchar_t *workDir;
  wchar_t tempDir[MAX_PATH *3 + 2];
  wchar_t envDir[MAX_PATH *3 + 2];
  wchar_t sfxDrive[_MAX_DRIVE];
  wchar_t sfxDir[_MAX_DIR * 3];
  wchar_t sfxFName[_MAX_FNAME + 5];
  char sfxFNameA[_MAX_FNAME + 5];
  wchar_t sfxExt[_MAX_EXT];
  wchar_t sfxDirPath[_MAX_PATH * 3 + 2];
  int id;
  HRESULT hr;
  LPITEMIDLIST pidl = NULL;
  BOOL b;
  
  // alternate behavior
  //if (_isatty(_fileno(stdout)))
  //  if (_wgetenv(L"PROMPT") == NULL)
  //    return 1; //silently quit, avoid extraction

  workDir = _wgetcwd(NULL,0);

  LoadSecurityDlls();

  #ifdef _CONSOLE
  SetConsoleCtrlHandler(HandlerRoutine, TRUE);
  #else
  UNUSED_VAR(hInstance);
  UNUSED_VAR(hPrevInstance);
  UNUSED_VAR(lpCmdLine);
  UNUSED_VAR(nCmdShow);
  #endif

  CrcGenerateTable();

  allocImp.Alloc = SzAlloc;
  allocImp.Free = SzFree;

  allocTempImp.Alloc = SzAllocTemp;
  allocTempImp.Free = SzFreeTemp;

  FileInStream_CreateVTable(&archiveStream);
  LookToRead2_CreateVTable(&lookStream, False);
  lookStream.buf = NULL;
 
  winRes = GetModuleFileNameW(NULL, sfxPath, MAX_PATH);
  if (winRes == 0 || winRes > MAX_PATH)
    return 1;
  {
    cmdLineParams = GetCommandLineW();
    #ifndef UNDER_CE
    {
      Bool quoteMode = False;
      for (;; cmdLineParams++)
      {
        wchar_t c = *cmdLineParams;
        if (c == L'\"')
          quoteMode = !quoteMode;
        else if (c == 0 || (c == L' ' && !quoteMode))
          break;
      }
    }
    #endif
  }
  
  
  for( id = 0; id < (sizeof(CSIDLtoDIRID)/sizeof(CSIDLtoDIRID[0])); id++ )
  {
  hr = SHGetSpecialFolderLocation (NULL, CSIDLtoDIRID[id].CSIDL, &pidl);
  if (hr == S_OK)
    {
      b = SHGetPathFromIDListW (pidl, envDir);
      if (b)
        SetEnvironmentVariableW(CSIDLtoDIRID[id].DIRID,envDir);
      CoTaskMemFree (pidl);
    }
  }
  _wsplitpath(sfxPath, sfxDrive, sfxDir, sfxFName, sfxExt);
  _wmakepath(sfxDirPath, sfxDrive, sfxDir, NULL, NULL);
  wcstombs(sfxFNameA, sfxFName, sizeof(sfxFNameA));
  SetEnvironmentVariableW(L"SFXPATH",sfxDirPath);
  SetEnvironmentVariableW(L"SFX",wcscat(sfxFName,sfxExt));
  SetEnvironmentVariableW(L"WORKDIR",workDir);

  {
    unsigned i;
    DWORD d;

    winRes = GetTempPathW(MAX_PATH, path);
    if (winRes == 0 || winRes > MAX_PATH)
      return 1;
    pathLen = wcslen(path);
    d = (GetTickCount() << 12) ^ (GetCurrentThreadId() << 14) ^ GetCurrentProcessId();
    
    for (i = 0;; i++, d += GetTickCount())
    {
      if (i >= 100)
      {
        res = SZ_ERROR_FAIL;
        break;
      }
      wcscpy(path + pathLen, L"7z");

      {
        wchar_t *s = path + wcslen(path);
        UInt32 value = d;
        unsigned k;
        for (k = 0; k < 8; k++)
        {
          unsigned t = value & 0xF;
          value >>= 4;
          s[7 - k] = (wchar_t)((t < 10) ? ('0' + t) : ('A' + (t - 10)));
        }
        s[k] = '\0';
      }

      if (DoesFileOrDirExist(path))
        continue;
      if (CreateDirectoryW(path, NULL))
      {
        wcscat(path, WSTRING_PATH_SEPARATOR);
        pathLen = wcslen(path);
        wcscpy(tempDir, path);
        break;
      }
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
        res = SZ_ERROR_FAIL;
        break;
      }
    }
    
    #ifndef UNDER_CE
    wcscpy(workCurDir, path);
    #endif
    if (res != SZ_OK)
      errorMessage = "Can't create temp folder";
  }

  if (res != SZ_OK)
  {
    if (!errorMessage)
      errorMessage = "Error";
    if (_isatty(_fileno(stdout)))
      PrintErrorMessage(errorMessage);
    else
      PrintErrorMessageBox(errorMessage);
    return 1;
  }

  if (InFile_OpenW(&archiveStream.file, sfxPath) != 0)
  {
    errorMessage = "can not open input file";
    res = SZ_ERROR_FAIL;
  }
  else
  {
    UInt64 pos = 0;
    if (!FindSignature(&archiveStream.file, &pos))
      res = SZ_ERROR_FAIL;
    else if (File_Seek(&archiveStream.file, (Int64 *)&pos, SZ_SEEK_SET) != 0)
      res = SZ_ERROR_FAIL;
    if (res != 0)
      errorMessage = "Can't find 7z archive";
  }

  if (res == SZ_OK)
  {
    lookStream.buf = ISzAlloc_Alloc(&allocImp, kInputBufSize);
    if (!lookStream.buf)
      res = SZ_ERROR_MEM;
    else
    {
      lookStream.bufSize = kInputBufSize;
      lookStream.realStream = &archiveStream.vt;
      LookToRead2_Init(&lookStream);
    }
  }

  SzArEx_Init(&db);
  
  if (res == SZ_OK)
  {
    res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);
  }
  
  if (res == SZ_OK)
  {
    UInt32 executeFileIndex = (UInt32)(Int32)-1;
    UInt32 minPrice = 1 << 30;
    UInt32 i;
    UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
    Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
    size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */
    
    for (i = 0; i < db.NumFiles; i++)
    {
      size_t offset = 0;
      size_t outSizeProcessed = 0;
      WCHAR *temp;

      if (SzArEx_GetFileNameUtf16(&db, i, NULL) >= MAX_PATH)
      {
        res = SZ_ERROR_FAIL;
        break;
      }
      
      temp = path + pathLen;
      
      SzArEx_GetFileNameUtf16(&db, i, temp);
      {
        res = SzArEx_Extract(&db, &lookStream.vt, i,
          &blockIndex, &outBuffer, &outBufferSize,
          &offset, &outSizeProcessed,
          &allocImp, &allocTempImp);
        if (res != SZ_OK)
          break;
      }
      {
        CSzFile outFile;
        size_t processedSize;
        size_t j;
        size_t nameStartPos = 0;
        for (j = 0; temp[j] != 0; j++)
        {
          if (temp[j] == '/')
          {
            temp[j] = 0;
            MyCreateDir(path);
            temp[j] = CHAR_PATH_SEPARATOR;
            nameStartPos = j + 1;
          }
        }

        if (SzArEx_IsDir(&db, i))
        {
          MyCreateDir(path);
          continue;
        }
        else
        {
          unsigned extLen;
          const WCHAR *name = temp + nameStartPos;
          unsigned len = (unsigned)wcslen(name);
          char *fNames[] = {
          sfxFNameA //unportable!
          , "setup"
          , "install"
          , "run"
          , "start"
          };
          unsigned nameLen = FindExt(temp + nameStartPos, &extLen);
          unsigned extPrice = FindItem(kExts, sizeof(kExts) / sizeof(kExts[0]), name + len - extLen, extLen);
          unsigned namePrice = FindItem((const char * const *)fNames, sizeof(fNames) / sizeof(fNames[0]), name, nameLen);
          unsigned price = namePrice + extPrice * 64 + (nameStartPos == 0 ? 0 : (1 << 12));
          if (minPrice > price)
          {
            minPrice = price;
            executeFileIndex = i;
            useShellExecute = extPrice;
          }
         
          if (DoesFileOrDirExist(path))
          {
            errorMessage = "Duplicate file";
            res = SZ_ERROR_FAIL;
            break;
          }
          if (OutFile_OpenW(&outFile, path))
          {
            errorMessage = "Can't open output file";
            res = SZ_ERROR_FAIL;
            break;
          }
        }
  
        processedSize = outSizeProcessed;
        if (File_Write(&outFile, outBuffer + offset, &processedSize) != 0 || processedSize != outSizeProcessed)
        {
          errorMessage = "Can't write output file";
          res = SZ_ERROR_FAIL;
        }
        
        #ifdef USE_WINDOWS_FILE
        if (SzBitWithVals_Check(&db.MTime, i))
        {
          const CNtfsFileTime *t = db.MTime.Vals + i;
          FILETIME mTime;
          mTime.dwLowDateTime = t->Low;
          mTime.dwHighDateTime = t->High;
          SetFileTime(outFile.handle, NULL, NULL, &mTime);
        }
        #endif
        
        {
          SRes res2 = File_Close(&outFile);
          if (res != SZ_OK)
            break;
          if (res2 != SZ_OK)
          {
            res = res2;
            break;
          }
        }
        #ifdef USE_WINDOWS_FILE
        if (SzBitWithVals_Check(&db.Attribs, i))
          SetFileAttributesW(path, db.Attribs.Vals[i]);
        #endif
      }
    }

    if (res == SZ_OK)
    {
      if (executeFileIndex == (UInt32)(Int32)-1)
      {
        errorMessage = "There is no file to execute";
        res = SZ_ERROR_FAIL;
      }
      else
      {
        WCHAR *temp = path + pathLen;
        UInt32 j;
        SzArEx_GetFileNameUtf16(&db, executeFileIndex, temp);
        for (j = 0; temp[j] != 0; j++)
          if (temp[j] == '/')
            temp[j] = CHAR_PATH_SEPARATOR;
      }
    }
    ISzAlloc_Free(&allocImp, outBuffer);
  }

  SzArEx_Free(&db, &allocImp);

  ISzAlloc_Free(&allocImp, lookStream.buf);

  File_Close(&archiveStream.file);

  if (res == SZ_OK)
  {
    HANDLE hProcess = 0;
    
    #ifndef UNDER_CE
    WCHAR oldCurDir[MAX_PATH + 2];
    oldCurDir[0] = 0;
    {
      DWORD needLen = GetCurrentDirectory(MAX_PATH + 1, oldCurDir);
      if (needLen == 0 || needLen > MAX_PATH)
        oldCurDir[0] = 0;
      SetCurrentDirectory(workCurDir);
    }
    #endif
    if (useShellExecute == 3)
    {
      STARTUPINFOW si;
      PROCESS_INFORMATION pi;
      WCHAR cmdLine[MAX_PATH * 3];
      wcscpy(cmdLine, path);
      wcscat(cmdLine, cmdLineParams);
      memset(&si, 0, sizeof(si));
      si.cb = sizeof(si);
      if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) == 0)
        res = SZ_ERROR_FAIL;
      else
      {
        CloseHandle(pi.hThread);
        hProcess = pi.hProcess;
      }
    }
    else if (useShellExecute == 2)
    {
      STARTUPINFOW si;
      PROCESS_INFORMATION pi;
      WCHAR cmdLine[MAX_PATH * 3];
      wcscpy(cmdLine, L"rundll32.exe advpack.dll,LaunchINFSection ");
      wcscat(cmdLine, path);
      memset(&si, 0, sizeof(si));
      si.cb = sizeof(si);
      if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) == 0)
        res = SZ_ERROR_FAIL;
      else
      {
        CloseHandle(pi.hThread);
        hProcess = pi.hProcess;
      }
    }
    else if (0 <= useShellExecute <= 1)
    {
      PROCESS_INFORMATION pi;
      WCHAR fcmdLine[MAX_PATH * 3 + 10];
      WCHAR roscmdLine[MAX_PATH * 3 + 10];
      WCHAR newPATH[32767];
      WCHAR oldPATH[32767];
      STARTUPINFOW si = { 0 };
      si.cb = sizeof(si);
      si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
      si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
      si.hStdOutput =  GetStdHandle(STD_OUTPUT_HANDLE);
      si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
      wcscpy(fcmdLine, L"/c ");
      wcscat(fcmdLine, path);
      wcscat(fcmdLine, cmdLineParams);
      GetEnvironmentVariableW(L"PATH", oldPATH, sizeof(oldPATH));
      wcscpy(newPATH, tempDir);
      wcscat(newPATH, L"bin;");
      wcscat(newPATH, oldPATH);
      SetEnvironmentVariableW(L"PATH", newPATH);
      wcscpy(roscmdLine, tempDir);
      wcscat(roscmdLine, L"bin\\roscmd.exe");
      if (DoesFileOrDirExist(roscmdLine)) {
        SetEnvironmentVariableW(L"COMSPEC", roscmdLine);
      } else
        GetEnvironmentVariableW(L"COMSPEC", roscmdLine, sizeof(roscmdLine));
      memset(&si, 0, sizeof(si));
      si.cb = sizeof(si);
      if (CreateProcessW(roscmdLine, fcmdLine, NULL, NULL, FALSE, 0, NULL, workDir, &si, &pi) == 0)
        res = SZ_ERROR_FAIL;
      else
      {
        CloseHandle(pi.hThread);
        hProcess = pi.hProcess;
      }
    }
    else if (3 < useShellExecute < 8)
    {
      SHELLEXECUTEINFO ei;
      UINT32 executeRes;
      BOOL success;
      
      memset(&ei, 0, sizeof(ei));
      ei.cbSize = sizeof(ei);
      ei.lpFile = path;
      ei.fMask = SEE_MASK_NOCLOSEPROCESS
          #ifndef UNDER_CE
          | SEE_MASK_FLAG_DDEWAIT
          #endif
          | SEE_MASK_NO_CONSOLE
          ;
      if (wcslen(cmdLineParams) != 0)
        ei.lpParameters = cmdLineParams;
      ei.nShow = SW_SHOWNORMAL; /* SW_HIDE; */
      success = ShellExecuteEx(&ei);
      executeRes = (UINT32)(UINT_PTR)ei.hInstApp;
      if (!success || (executeRes <= 32 && executeRes != 0))  /* executeRes = 0 in Windows CE */
        res = SZ_ERROR_FAIL;
      else
        hProcess = ei.hProcess;
    }
    
    if (hProcess != 0)
    {
      WaitForSingleObject(hProcess, INFINITE);
      if (!GetExitCodeProcess(hProcess, &exitCode))
        exitCode = 1;
      CloseHandle(hProcess);
    }
    
    #ifndef UNDER_CE
    SetCurrentDirectory(oldCurDir);
    #endif
  }

  path[pathLen] = L'\0';
  RemoveDirWithSubItems(path);

  if (res == SZ_OK)
    return (int)exitCode;
  
  {
    if (res == SZ_ERROR_UNSUPPORTED)
      errorMessage = "Decoder doesn't support this archive";
    else if (res == SZ_ERROR_MEM)
      errorMessage = "Can't allocate required memory";
    else if (res == SZ_ERROR_CRC)
      errorMessage = "CRC error";
    else
    {
      if (!errorMessage)
        errorMessage = "ERROR";
    }
 
    if (errorMessage)
    {
      if (_isatty(_fileno(stdout)))
        PrintErrorMessage(errorMessage);
      else
        PrintErrorMessageBox(errorMessage);
    }
  }
  return 1;
}
