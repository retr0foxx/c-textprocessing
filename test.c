#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

#include <textprocessing/textreader.h>

int main()
{
    // 16384
    /*printf("%i %i %i %i %i %i %i %i %i %i %i %i %i %i\n", STDIN_FILENO, _O_U16TEXT);
    int32_t conv = 128169;
    uint8_t converted[4];
    int len = textprocessing_encode_chr(TPENC_UTF16, conv, converted);

    for (int i = 0; i < len; ++i)
        printf("%x\n", converted[i]);
*/
    unsigned old_mode = _setmode(STDIN_FILENO, _O_U16TEXT);
    old_mode = _setmode(STDOUT_FILENO, _O_U16TEXT);
    wprintf(L"%u\n", old_mode);
  /*  putwc(*((wchar_t*)(converted)), stdout);
    putwc(*((wchar_t*)(converted+1)), stdout);*/
    //wprintf(L"\xd83d\xdca9 \x043a\x043e\x0448\x043a\x0430 \x65e5\x672c\x56fd\n");

    while (1)
    {
        wchar_t c = fgetwc(stdin);
        wprintf(L"%lc", c);
    }
    return 0;
}

//#include "./examples/winnix-read-unicode-stdin.c"
