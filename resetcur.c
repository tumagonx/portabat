// public domain
#include <windows.h>

int main(void) {
CONSOLE_CURSOR_INFO ConsoleCursorInfo;
ConsoleCursorInfo.dwSize = 1; //bottom hairline
ConsoleCursorInfo.bVisible = TRUE;
HANDLE conout = GetStdHandle(STD_OUTPUT_HANDLE);

if (conout != INVALID_HANDLE_VALUE) {
    if (SetConsoleCursorInfo(conout, &ConsoleCursorInfo))
        return 0;
}
return 1;
}