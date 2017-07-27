/*

by Luigi Auriemma

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <windows.h>

/*

Show_dump 0.1.1a

    Copyright 2004,2005,2006 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl.txt

This function, optimized for performace, shows the hex dump of a buffer and
places it in a stream

Usage:
        show_dump(buffer, buffer_length, stdout);
        show_dump(buffer, buffer_length, fd);
*/

void show_dump(unsigned char *data, unsigned int len, FILE *stream) {
    const static char       hex[] = "0123456789abcdef";
    static unsigned char    buff[67];   /* HEX  CHAR\n */
    unsigned char           chr,
                            *bytes,
                            *p,
                            *limit,
                            *glimit = data + len;

    memset(buff + 2, ' ', 48);

    while(data < glimit) {
        limit = data + 16;
        if(limit > glimit) {
            limit = glimit;
            memset(buff, ' ', 48);
        }

        p     = buff;
        bytes = p + 50;
        while(data < limit) {
            chr = *data;
            *p++ = hex[chr >> 4];
            *p++ = hex[chr & 15];
            p++;
            *bytes++ = ((chr < ' ') || (chr >= 0x7f)) ? '.' : chr;
            data++;
        }
        *bytes++ = '\n';

        fwrite(buff, bytes - buff, 1, stream);
    }
}


#ifdef WIN32
    typedef unsigned char   u_char;
#endif



#define VER     "0.2"
#define BUFFSZ  8192



u_char *parse_key(u_char *data, int *size, wchar_t *wdata);
void std_err(void);

#ifdef __TINYC__
LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR, int*);
#endif

int main(int argc, char *argv[]) {
    FILE    *inz,
            *outz;
    int     len;
    u_char  *buff,
            *p,
            *l,
            *key,
            *k,
            *kl;
    wchar_t *inf,
            *outf;

    setbuf(stdout, NULL);

    fputs("\n"
        "Xor "VER"\n"
        "by Luigi Auriemma\n"
        "e-mail: aluigi@autistici.org\n"
        "web:    aluigi.org\n"
        "\n", stderr);

    if (argc < 4) {
        printf("\n"
            "Usage: %s <input> <output> <key*>\n"
            "\n"
            "use - for stdin or stdout\n"
            "* this field can be:\n"
            "- a file containing the data (key) to use for xoring the input file\n"
            "- the string (key) to use for xoring the input file\n"
            "- a hex (0x) byte or a sequence of hex bytes\n"
            "the tool automatically understand what is the chosen format and shows the key\n"
            "\n", argv[0]);
        exit(1);
    }

    wchar_t** wargv = CommandLineToArgvW (GetCommandLineW(), &argc);

    inf  = wargv[1];
    outf = wargv[2];

    fprintf(stderr, "- input file: ");
    if(!wcscmp(inf, L"-")) {
        fprintf(stderr, "stdin\n");
        inz  = stdin;
    } else {
        fwprintf(stderr, L"%s\n", inf);
        inz  = _wfopen(inf, L"rb");
    }
    if(!inz) std_err();

    fprintf(stderr, "- output file: ");
    if(!wcscmp(outf, L"-")) {
        fprintf(stderr, "stdout\n");
        outz = stdout;
    } else {
        fwprintf(stderr, L"%s\n", outf);
        outz = _wfopen(outf, L"wb");
    }
    if(!outz) std_err();

    key = parse_key(argv[3], &len, wargv[3]);
    kl = key + len;
    fprintf(stderr, " (hex dump follows):\n");
    show_dump(key, len, stderr);

    buff = malloc(BUFFSZ + 1);
    if(!buff) std_err();

    fprintf(stderr, "- read and xor file\n");
    k = key;
    while((len = fread(buff, 1, BUFFSZ, inz))) {
        for(p = buff, l = buff + len; p != l; p++, k++) {
            if(k == kl) k = key;
            *p ^= *k;
        }

        if(fwrite(buff, len, 1, outz) != 1) {
            fprintf(stderr, "\nError: write error, probably the disk space is finished\n");
            exit(1);
        }
    }

    if(inz  != stdin)  fclose(inz);
    if(outz != stdout) fclose(outz);
    fprintf(stderr, "- finished\n");
    return(0);
}



u_char *parse_key(u_char *data, int *size, wchar_t *wdata) {
    FILE    *fd;
    struct  stat    xstat;
    int     i,
            t,
            datalen;
    u_char  *key,
            *k;

    datalen = strlen(data);

        /* HEX */
    if((data[0] == '0') && (tolower(data[1]) == 'x')) {
        fprintf(stderr, "- hex key");
        for(i = 0; i < datalen; i++) data[i] = tolower(data[i]);

        key = malloc((datalen / 2) + 1);
        if(!key) std_err();

        k = key;
        data += 2;
        for(;;) {
            if(sscanf(data, "%02x", &t) != 1) break;
            data += 2;
            *k++ = t;
            while(*data && (*data <= ' ')) data++;
            if((*data == '0') && (data[1] == 'x')) data += 2;
            if((*data >= '0') && (*data <= '9')) continue;
            if((*data >= 'a') && (*data <= 'f')) continue;
            break;
        }
        *size = k - key;
        return(key);
    }

    fd = _wfopen(wdata, L"rb");
    if(fd) {
        fprintf(stderr, "- file key");
        fstat(fileno(fd), &xstat);
        key = malloc(xstat.st_size);
        if(!key) std_err();
        fread(key, xstat.st_size, 1, fd);
        fclose(fd);
        *size = xstat.st_size;
        return(key);
    }

    fprintf(stderr, "- text string key");
    key = malloc(datalen);
    if(!key) std_err();
    memcpy(key, data, datalen);
    *size = datalen;
    return(key);
}



void std_err(void) {
    perror("\nError");
    exit(1);
}


