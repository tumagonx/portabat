// C program to copy N bytes of from a specific offset to another file.
// Dont know who originally wrote it, modded to be LFS and unicode aware
// hopefully Public Domain

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef __TINYC__
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR, int*);
#endif
/** fseeko/ftello
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <io.h>
#include <errno.h>

#ifndef _OFF64_T_DEFINED
typedef long long _off64_t;
#endif

int fseeko64 (FILE* stream, _off64_t offset, int whence)
{
  fpos_t pos;
  if (whence == SEEK_CUR)
    {
      /* If stream is invalid, fgetpos sets errno. */
      if (fgetpos (stream, &pos))
        return (-1);
      pos += (fpos_t) offset;
    }
  else if (whence == SEEK_END)
    {
      /* If writing, we need to flush before getting file length.  */
      fflush (stream);
      pos = (fpos_t) (_filelengthi64 (_fileno (stream)) + offset);
    }
  else if (whence == SEEK_SET)
    pos = (fpos_t) offset;
  else
    {
      errno = EINVAL;
      return (-1);
    }
  return fsetpos (stream, &pos);
}

_off64_t ftello64 (FILE * stream)
{
  fpos_t pos;
  if (fgetpos (stream, &pos))
    return  -1LL;
  else
   return ((_off64_t) pos);
}

int main (int argc,char *argv[])
{
    FILE *fp1;
    FILE *fp2;
    _off64_t insize = 0;
    _off64_t initpos = 0;
    _off64_t endpos = 0;
    _off64_t times = 0;
    _off64_t count = 0;
    _off64_t total = 0;
    _off64_t written = 0;
    unsigned int x = 4096;
    int mod = 0;
    unsigned char *data[4096];
    wchar_t** wargv = CommandLineToArgvW (GetCommandLineW(), &argc);

    if (argc < 5) {
        printf ("Usage: %s [in] [out] [offset] [size]\n", argv[0]);
        return -1;
    }

    fp1 = _wfopen (wargv[1], L"rb");
    if (fp1 == NULL) {
        printf("%s File can not be opened : \n", argv[1]);
        return -1;
    }

    fseeko64 (fp1, 0, SEEK_END);
    insize = ftello64 (fp1);
    initpos = _atoi64 (argv[3]);        // offset of source file to copy
    total = _atoi64 (argv[4]);        // number of bytes to copy

    //multiplier
    for (x; x >= 1; x *= .5) {
        if (total >= x) {
            times = total / x;
            mod = total % x;
            break;
        }
    }

    if (insize <= initpos)    {
        printf ("Error: nothing to do.\n");
        return -1;
    }
    if (insize < (initpos + total))    {
        total = insize - initpos;
        printf ("Warning: 'size' is too big, will copying until end of input file.\n");
    }

    fp2 = _wfopen (wargv[2], L"wb");
    if (fp2 == NULL) {
        wprintf (L"%s : can not be opened\n", wargv[2]);
        return -1;
    }

    fseeko64 (fp1, initpos, SEEK_SET);
    endpos = initpos + total;
    while (initpos < endpos) {
        if (count >= times) {
            for (x; x >= 1; x *= .5) {
                if (mod >= x) {
                    mod -= x;
                    break;
                }
            }
        }
        fread (data, 1, x, fp1);
        fwrite (data, 1, x, fp2);
        count += 1;
        initpos = ftello64 (fp1);
        //written = ftello64 (fp2);
        //printf("written: %I64d\n",written);
    }
    data[4096] = 0;
    fclose (fp1);
    fclose (fp2);
    return 0;
}