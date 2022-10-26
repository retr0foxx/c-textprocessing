#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

#include "textreader.h"

int main()
{
    uint8_t buffer[4];
    int buflen = textreader_encode_chr(TRENC_UTF8, 12525, buffer);
    printf("%i\n", buflen);
    printf("%x %x %x", buffer[0], buffer[1], buffer[2]);

    return 0;
}
