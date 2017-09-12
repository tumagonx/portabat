/*  General License for Open Source projects published by
  Olof Lagerkvist - LTR Data.

    Copyright (c) Olof Lagerkvist
    http://www.ltr-data.se
    olof@ltr-data.se

  The above copyright notice shall be included in all copies or
  substantial portions of the Software.
  
  modified as needed to compile with TinyCC */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
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

int main(int argc, char **argv)
{
    REPARSE_DATA_MOUNT_POINT ReparseData = { 0 };
    HANDLE h;
    DWORD dw;
    BOOL bDirectoryCreated = FALSE;
    BOOL bRemoval = FALSE;

    wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if ((argc < 2) | (argc > 3))
    {
        printf("Usage:\ncreate: %S path_to_empty_directory \"\\??\\drive:\\\\path_to_target_directory\\\\\"\
\ninfo  : %S path_to_junction_directory\ndelete: %S -r path_to_junction_directory\n\
\nnote: junction directory will be created if not exist\n", wargv[0], wargv[0], wargv[0]);
        return 5;
    }

    if (argc == 3 ? wcscmp(wargv[1], L"-r") == 0 : FALSE)
    {
        bRemoval = TRUE;
        argc--;
        wargv++;
        argv++;
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
                return 3;
            }

            h = CreateFileW(wargv[1], GENERIC_READ | GENERIC_WRITE, 0, NULL,
                OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS |
                FILE_FLAG_OPEN_REPARSE_POINT, NULL);
        }
    }

    if (h == INVALID_HANDLE_VALUE)
        return 3;

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
                    "The reparse data on %S is invalid.\n",
                    wargv[1]);
                break;

            case ERROR_INVALID_PARAMETER:
                fputs("Need Windows 2000 or later.\r\n",
                    stderr);
                break;

            case ERROR_INVALID_FUNCTION:
                fputs("Need NTFS volumes.\r\n",
                    stderr);
                break;

            case ERROR_NOT_A_REPARSE_POINT:
            case ERROR_DIRECTORY:
            case ERROR_DIR_NOT_EMPTY:
                fprintf(stderr,
                    "Not a reparse point: %S \r\n", wargv[1]);
                break;

            default:
                fprintf(stderr,
                    "Error getting reparse data from %S\r\n",
                    wargv[1]);
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
            fprintf(stderr, "Name is too long: %S\r\n", wargv[2]);
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
                fprintf(stderr, "Invalid target path: %S\r\n", wargv[2]);
                break;

            case ERROR_INVALID_PARAMETER:
                fputs("Need Windows 2000 or later.\r\n",
                    stderr);
                break;

            case ERROR_INVALID_FUNCTION:
            case ERROR_NOT_A_REPARSE_POINT:
                fputs("Need NTFS volumes.\r\n",
                    stderr);
                break;

            case ERROR_DIRECTORY:
            case ERROR_DIR_NOT_EMPTY:
                fputs("Directory is not empty.\r\n", stderr);
                break;

            default:
                fprintf(stderr,
                    "Error joining %S to %S\r\n",
                    wargv[2], wargv[1]);
            }

            CloseHandle(h);

            if (bDirectoryCreated)
                RemoveDirectoryW(wargv[1]);

            return 1;
        }
        fprintf(stdout,"%S -> %S\r\n", wargv[1], wargv[2]);
        return 0;
    }

    if (!DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, &ReparseData,
        sizeof ReparseData, &dw, NULL))
    {
        switch (GetLastError())
        {
        case ERROR_INVALID_REPARSE_DATA:
            fprintf(stderr, "The reparse data on %S is invalid.\r\n",
                wargv[1]);
            return 1;

        case ERROR_INVALID_PARAMETER:
            fputs("Need Windows 2000 or later.\r\n",
                stderr);
            break;

        case ERROR_INVALID_FUNCTION:
            fputs("Need NTFS volumes.\r\n",
                stderr);
            break;

        case ERROR_NOT_A_REPARSE_POINT:
        case ERROR_DIRECTORY:
        case ERROR_DIR_NOT_EMPTY:
            fprintf(stderr, "Not a reparse point: %S\r\n", wargv[1]);
            break;

        default:
            fprintf(stderr,
                "Error getting reparse data from %S\r\n",
                wargv[1]);
        }

        return 1;
    }

    if (ReparseData.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT)
    {
        fputs("Not a junction.\r\n", stderr);
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

        fprintf(stdout,"%S -> %S\r\n", wargv[1], target_name.Buffer);

        return 0;
    }
}
