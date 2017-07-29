/*  General License for Open Source projects published by
  Olof Lagerkvist - LTR Data.

    Copyright (c) Olof Lagerkvist
    http://www.ltr-data.se
    olof@ltr-data.se

  The above copyright notice shall be included in all copies or
  substantial portions of the Software.
  
  modified as needed to compile with TinyCC */

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0500
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __TINYC__
#define __MSABI_LONG(x) x ## l
#define ERROR_INVALID_REPARSE_DATA __MSABI_LONG(4392)
#define ERROR_NOT_A_REPARSE_POINT __MSABI_LONG(4390)
#define CTL_CODE(DeviceType,Function,Method,Access) (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#define FILE_DEVICE_FILE_SYSTEM 0x00000009
#define METHOD_BUFFERED 0
#define FILE_ANY_ACCESS 0
#define FILE_SPECIAL_ACCESS (FILE_ANY_ACCESS)
#define FSCTL_SET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM,41,METHOD_BUFFERED,FILE_SPECIAL_ACCESS)
#define FSCTL_GET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM,42,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define FSCTL_DELETE_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM,43,METHOD_BUFFERED,FILE_SPECIAL_ACCESS)
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR, int*);
#else
#include <winioctl.h>
#include <shellapi.h>
#endif

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

int fwprintf_line_length;
void SetOemPrintFLineLength(HANDLE hConsole) {
    CONSOLE_SCREEN_BUFFER_INFO con_info;
    if (GetConsoleScreenBufferInfo(hConsole, &con_info)) {
        fwprintf_line_length = con_info.srWindow.Right - con_info.srWindow.Left - 2;
    }
    else
        fwprintf_line_length = 0;
}

typedef struct _REPARSE_DATA_MOUNT_POINT
{
    DWORD ReparseTag;
    WORD ReparseDataLength;
    WORD Reserved;
    WORD NameOffset;
    WORD NameLength;
    WORD DisplayNameOffset;
    WORD DisplayNameLength;
    BYTE Data[65536];
} REPARSE_DATA_MOUNT_POINT, *PREPARSE_DATA_MOUNT_POINT;

LPSTR win_errmsgA(DWORD dwErrNo)
{
  LPSTR errmsg = NULL;

  if (FormatMessageA(FORMAT_MESSAGE_MAX_WIDTH_MASK |
		     FORMAT_MESSAGE_FROM_SYSTEM |
		     FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, dwErrNo,
		     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		     (LPSTR)&errmsg, 0, NULL))
    return errmsg;
  else
    return NULL;
}

int main(int argc, char **argv)
{
    REPARSE_DATA_MOUNT_POINT ReparseData = { 0 };
    HANDLE h;
    DWORD dw;
    BOOL bDirectoryCreated = FALSE;
    BOOL bRemoval = FALSE;

    SetOemPrintFLineLength(GetStdHandle(STD_ERROR_HANDLE));
    wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if ((argc < 2) | (argc > 3))
    {
        fputs("Syntax 1 - Create a junction:\r\n"
            "junc DIRECTORY TARGET\r\n"
            "\n"
            "Where DIRECTORY is an empty directory on an NTFS volume and TARGET is a native\r\n"
            "path to a target directory, e.g. \"\\??\\C:\\Windows\", \"\\??\\D:\\\\\" or\r\n"
            "\"\\Device\\Harddisk0\\Partition1\\\\\". Double quotes are optional unless the\r\n"
            "path contains spaces. If the string ends with a backslash and you surround it\r\n"
            "with quotes, you will need an extra backslash at the end of the string:\r\n"
            "\\??\\D:\\ is equal to \"\\??\\D:\\\\\".\r\n"
            "\n"
            "Syntax 2 - Display where a junction points:\r\n"
            "junc JUNCTION\r\n"
            "\n"
            "Syntax 3 - Remove a junction:\r\n"
            "junc -r JUNCTION\r\n"
            "The junction will be converted back to an empty directory.\r\n",
            stderr);
        return 5;
    }

    if (argc == 3 ? wcscmp(wargv[1], L"-r") == 0 : FALSE)
    {
        bRemoval = TRUE;
        argc--;
        wargv++;
    }

    if (bRemoval)
        h = CreateFileW(wargv[1], GENERIC_WRITE, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS |
            FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    else if (argc == 2)
        h = CreateFileW(wargv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
            NULL);
    else
    {
        h = CreateFileW(wargv[1], GENERIC_WRITE, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS |
            FILE_FLAG_OPEN_REPARSE_POINT, NULL);
        if ((h == INVALID_HANDLE_VALUE) &&
            (GetLastError() == ERROR_FILE_NOT_FOUND))
        {
            if (CreateDirectoryW(wargv[1], NULL))
                bDirectoryCreated = TRUE;
            else
            {
                //win_perror(argv[1]);
                return 3;
            }

            h = CreateFileW(wargv[1], GENERIC_READ | GENERIC_WRITE, 0, NULL,
                OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS |
                FILE_FLAG_OPEN_REPARSE_POINT, NULL);
        }
    }

    if (h == INVALID_HANDLE_VALUE)
    {
        //win_perror(argv[1]);
        return 3;
    }

    if (bRemoval)
    {
        ReparseData.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
        ReparseData.ReparseDataLength = 0;

        if (!DeviceIoControl(h, FSCTL_DELETE_REPARSE_POINT, &ReparseData,
            REPARSE_GUID_DATA_BUFFER_HEADER_SIZE, NULL, 0, &dw,
            NULL))
        {
            switch (GetLastError())
            {
            case ERROR_INVALID_REPARSE_DATA:
                fprintf(stderr,
                    "The reparse data on %s is invalid.\n",
                    argv[1]);
                break;

            case ERROR_INVALID_PARAMETER:
                fputs("This OS does not support reparse points.\r\n"
                    "Windows 2000 or higher is required.\r\n",
                    stderr);
                break;

            case ERROR_INVALID_FUNCTION:
                fputs("Reparse points are only supported on NTFS volumes.\r\n",
                    stderr);
                break;

            case ERROR_NOT_A_REPARSE_POINT:
            case ERROR_DIRECTORY:
            case ERROR_DIR_NOT_EMPTY:
                fprintf(stderr,
                    "Not a reparse point: %s \r\n", argv[1]);
                break;

            default:
                fprintf(stderr,
                    "Error getting reparse data from %s: %s \r\n",
                    argv[1], win_errmsgA(GetLastError()));
            }

            return 1;
        }

        return 0;
    }

    if (argc > 2)
    {
        int iSize = (int)wcslen(wargv[2]) << 1;

        if ((iSize + 6 > sizeof(ReparseData.Data)) | (iSize == 0))
        {
            fprintf(stderr, "Name is too long: %s\r\n", argv[2]);
            return 4;
        }

        ReparseData.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
        ReparseData.ReparseDataLength = (WORD)(8 + iSize + 2 + iSize + 2);
        ReparseData.NameLength = (WORD)iSize;
        ReparseData.DisplayNameOffset = (WORD)(iSize + 2);
        ReparseData.DisplayNameLength = (WORD)iSize;
        wcscpy((LPWSTR)ReparseData.Data, wargv[2]);
        wcscpy((LPWSTR)(ReparseData.Data + ReparseData.DisplayNameOffset),
            wargv[2]);

        if (!DeviceIoControl(h, FSCTL_SET_REPARSE_POINT, &ReparseData,
            16 + iSize + 2 + iSize + 2, NULL, 0, &dw, NULL))
        {
            switch (GetLastError())
            {
            case ERROR_INVALID_REPARSE_DATA:
                fprintf(stderr, "Invalid target path: %s\r\n", argv[2]);
                break;

            case ERROR_INVALID_PARAMETER:
                fputs("This OS does not support reparse points.\r\n"
                    "Windows 2000 or higher is required.\r\n",
                    stderr);
                break;

            case ERROR_INVALID_FUNCTION:
            case ERROR_NOT_A_REPARSE_POINT:
                fputs("Reparse points are only supported on NTFS volumes.\r\n",
                    stderr);
                break;

            case ERROR_DIRECTORY:
            case ERROR_DIR_NOT_EMPTY:
                fputs("Reparse points can only created on empty "
                    "directories.\r\n", stderr);
                break;

            default:
                fprintf(stderr,
                    "Error joining %s to %s: %s\r\n",
                    argv[2], argv[1], win_errmsgA(GetLastError()));
            }

            CloseHandle(h);

            if (bDirectoryCreated)
                RemoveDirectoryW(wargv[1]);

            return 1;
        }

        return 0;
    }

    if (!DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, &ReparseData,
        sizeof ReparseData, &dw, NULL))
    {
        switch (GetLastError())
        {
        case ERROR_INVALID_REPARSE_DATA:
            fprintf(stderr, "The reparse data on %s is invalid.\r\n",
                argv[1]);
            return 1;

        case ERROR_INVALID_PARAMETER:
            fputs("This OS does not support reparse points.\r\n"
                "Windows 2000 or higher is required.\r\n",
                stderr);
            break;

        case ERROR_INVALID_FUNCTION:
            fputs("Reparse points are only supported on NTFS volumes.\r\n",
                stderr);
            break;

        case ERROR_NOT_A_REPARSE_POINT:
        case ERROR_DIRECTORY:
        case ERROR_DIR_NOT_EMPTY:
            fprintf(stderr, "Not a reparse point: %s\r\n", argv[1]);
            break;

        default:
            fprintf(stderr,
                "Error getting reparse data from %s: %s\r\n",
                argv[1], win_errmsgA(GetLastError()));
        }

        return 1;
    }

    if (ReparseData.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT)
    {
        fputs("This reparse point is not a junction.\r\n", stderr);
        return 2;
    }
    else
    {
        UNICODE_STRING target_name;

        target_name.MaximumLength =
            target_name.Length =
            ReparseData.NameLength;

        target_name.Buffer =
            (PWSTR)ReparseData.Data + ReparseData.NameOffset;

        //fprintf(stdout,
        //   "%s -> %s\r\n",
        //    wargv[1],
        //    &target_name);

        return 0;
    }
}
