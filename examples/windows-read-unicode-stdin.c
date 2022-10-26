// Compile with textreader.c

// The way this library reads unicode input from stdin is weird
// I don't really know if there's another proper way to do this

/* But basically it does it by doing this:
 *   1. Set the mode of stdin to UTF-16 so that text read from stdin will be in UTF-16
 *      Note: Any other modes other than UTF-16 does not work, or at least it doesn't in my computer
 *            This also makes it so that you can only read from stdin using wide string io functions like fgetwc, fgetws, etc.
 *   
 *   2. Read from stdin by using fgetwc. And then each of those 2 bytes are a single UTF-16 byte.
 *      Note: Textreader abstracts the process of reading bytes from files/memory using a single function, textreader_get_byte
 *            And that function only returns bytes.
 *    
 *   3. Each 2 bytes received from fgetwc will be put into a temporary wchar_t buffer
 *      And then it will be returned byte-by-byte starting from the least significant byte number and decoded by the decoder
 *
 * The cringe part about it in my opinion is that it returns each UTF-16 bytes immediately 2 bytes by 2 bytes
 * This makes me have to create a flag and variable dedicated for reading stdin input on Windows
 * Because of the way textreader actually reads from files before decoding it
 * I thought maybe I would be able to _setmode to utf-8 or something and read it byte by byte
 * But that doesn't work.
 * 
 * If I'm not mistaken, on linux you can setlocale to utf-8 and then normally read byte-by-byte so there's no problem there
 */

#include <windows.h>
#include <io.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../textreader.h"

int main()
{
    // Sets the mode of stdin to UTF-16 to stdout to UTF-8 in a hacky way
    for (int i = 0; i < 2; ++i)
    {
        if (_setmode((int[2]){STDIN_FILENO, STDOUT_FILENO}[i], _O_U16TEXT) == -1)
        {
            printf("Failed to _setmode to with error %s\n", strerror(errno));
            return 1;
        }
    }
        
    textreader_t reader = textreader_openfileptr(stdin, TRENC_UTF16);
    reader.flags |= TRFLG_USE_FGETWC;

    const char *quit_msg = "exit";
    int quit_msg_len = 4;
    int matched_quit_msg = 0;
    
    int printed_start_of_input = 0;
    int32_t prev_chr = 0;
    
    char echo_chr_buffer[4];
    
    wprintf(L"Enter `exit` to exit\n\n");
    while (1)
    {
        if (!printed_start_of_input)
            wprintf(L"echo >> ");

        int32_t chr = textreader_getc(&reader);
        int32_t chr_lower = chr >= 'A' && chr <= 'Z' ? chr + 32 : chr;
        if (matched_quit_msg >= 0 && (matched_quit_msg >= quit_msg_len || chr_lower == quit_msg[matched_quit_msg]))
        {
            if (matched_quit_msg >= quit_msg_len)
            {
                if (chr == '\r' || chr == '\n')
                {
                    wprintf(L"\"\n");
                    return 1;
                }
                matched_quit_msg = -1;
            }

            ++matched_quit_msg;
        }
        else matched_quit_msg = -1;
        // For whatever reason, \n always appears twice after pressing enter
        // Maybe when you press enters it sends a \r\n and because of _setmode
        // There's an error that makes it convert \r into \n instead of \r\n into a single \n but who knows
        if (chr == '\n' && prev_chr == '\n')
        {
            printed_start_of_input = 0;
            matched_quit_msg = 0;
            continue;
        }
        else if (chr == '\n') wprintf(L"\"\n");

        if (!printed_start_of_input)
        {
            wprintf(L"Echo'd text: \"");
            printed_start_of_input = 1;
        }
        
        int len = textreader_encode_chr(TRENC_UTF16, chr, echo_chr_buffer)/2;
        //wprintf(L"%i\n", len);
        for (int i = 0; i < len; ++i) putwc(((wchar_t*)(echo_chr_buffer))[i], stdout);
        prev_chr = chr;
    }
    textreader_close(&reader, 0);
    return 0;
}