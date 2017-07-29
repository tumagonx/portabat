// Public Domain

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <windows.h>

#ifdef __TINYC__
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR, int*);
#else
#include <shellapi.h>
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

#ifndef __MINGW32__
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
#endif

void dowrite (FILE *fp1, FILE *fp2, _off64_t initpos, _off64_t endpos, _off64_t outsize) {
    // 4096 is the typical sector size, make it twice
    unsigned int x;
    unsigned char *data[8192];
    _off64_t times = 0;
    _off64_t count = 0;
    //_off64_t written = 0;
    int mod;
    //multiplier
    for (x = 8192; x >= 1; x *= .5) {
        if (outsize >= x) {
            times = outsize / x;
            mod = outsize % x;
            break;
        }
    }
    while (initpos < endpos) {
        if (count >= times) {
            for (x = 8192; x >= 1; x *= .5) {
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
        //printf ("written: %I64d byte(s)\n", written);
    }
    data[4096] = 0;
    fclose (fp2);
}

int main (int argc,char *argv[])
{
    FILE *fp1;
    FILE *fp2;
    _off64_t insize = 0;
    _off64_t initpos = 0;
    _off64_t endpos = 0;
    _off64_t outsize = 0;
    _off64_t split = 0;
    unsigned int eof = 0;
    unsigned int pipeout = 0;
    wchar_t opath[_MAX_PATH];
    wchar_t odrive[_MAX_DRIVE];
    wchar_t odir[_MAX_DIR];
    wchar_t ofname[_MAX_FNAME];
    wchar_t ofnamenum[_MAX_FNAME];
    wchar_t oext[_MAX_EXT];
    wchar_t iterator[128];
    unsigned int i;
    wchar_t** wargv = CommandLineToArgvW (GetCommandLineW(), &argc);
    setmode (fileno (stdout), O_BINARY);
    setbuf(stdout, NULL);

    if (argc != 5) {
        printf ("Usage: %s [in] [out] [offset] [size]\n", argv[0]);
        printf ("replace [offset] with [-] to split input file\n");
        printf ("replace [size] with [-] to read remaining input file\n");
        printf ("replace [out] with [-] to pipe output\n");
        return -1;
    }

    fp1 = _wfopen (wargv[1], L"rb");
    if (fp1 == NULL) {
        printf ("Error: %s can not be opened\n", argv[1]);
        return -1;
    }
    fseeko64 (fp1, 0, SEEK_END);
    insize = ftello64 (fp1);

    if (!strcmp (argv[3], "-"))
        split = 1;
    else 
        initpos = _wtoi64 (wargv[3]);
    if (strcmp (argv[3], "0") && (!initpos) && (!split)) {
        printf ("Error: offset=%s\n", argv[3]);
        return -1;
    }

    if (!wcscmp (wargv[2], L"-"))
        pipeout = 1;
    if ((split + pipeout) == 2) {
        printf ("Error: can not split into pipe\n");
        return -1;
    }

    if (!strcmp (argv[4], "-")) {
        eof = 1;
        outsize = insize - initpos;
    }
    else 
        outsize = _wtoi64 (wargv[4]); 
    if ((outsize <= 0) || (eof + split == 2)) {
        printf ("Error: nothing to copy\n");
        return -1;
    }

    fseeko64 (fp1, initpos, SEEK_SET);
    endpos = initpos + outsize;

    _wsplitpath (wargv[2], odrive, odir, ofname, oext);
    
    if (split)
        split = insize / outsize;

    for (i = 0; i <= split; i++) {
        wcscpy (ofnamenum, ofname);
        if (split) {
            _itow (i, iterator, 10);
            wcscat (ofnamenum, iterator);
            if (i == split)
                outsize = insize % outsize;
            endpos = ftello64(fp1) + outsize;
        }
        _wmakepath (opath, odrive, odir, ofnamenum, oext) ;

        if (pipeout)
            fp2 = stdout;
        else
            fp2 = _wfopen (opath, L"wb");

        if (fp2 == NULL) {
            // TODO check disk space? GetDiskFreeSpaceEx
            if (!pipeout)
                printf ("Error: output file can not be opened\n");
            return -1;
        }

        dowrite (fp1, fp2, initpos, endpos, outsize);

        if (ferror(fp1)) {
            printf ("Error: reading input\n");
            return 1;
        } 
        else if (ferror(fp2)) {
            printf ("Error: writing output\n");
            return 1;
        }
    }
    fclose (fp1);
    return 0;
}