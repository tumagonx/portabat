#include <stdio.h>
#include <windows.h>
int main()
{
  char *lpData;
  setmode (fileno (stdout), 0x8000);
  if (!OpenClipboard(NULL))
      return 1;
  lpData = (char*)GetClipboardData(CF_OEMTEXT);
  if (lpData == NULL)
      return 1;
	fputs(lpData, stdout);
  return 0;
}
