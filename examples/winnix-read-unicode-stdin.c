// Compile with textreader.c

// The "unix" part is only tested on WSL2 by me

// Yeah the comments below is pasted from the windows example

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

#include <wchar.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <textprocessing/textprocessing.h>
#include <textprocessing/textreader.h>

#if defined(_WIN32)
    #define OS_WIN32

    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>

    #define READER_ENCODING TPENC_UTF16
    #define OUTPUT_CONVERT_ENCODING TPENC_UTF8

    // Because after doing _setmode to a certain unicode encoding on windows
    // Only string functions will work on that file stream
    //#define FPUTS_LITERAL(string) fputws(L##string, stdout)
#elif defined(unix) || defined(__unix__) || defined(__unix)
    #define OS_UNIX

    #include <locale.h>

    #define READER_ENCODING TPENC_UTF8
    #define OUTPUT_CONVERT_ENCODING TPENC_UTF8

    #define SET_LOCALE_TO "en_US.UTF-8"

#endif

// Because wide and byte oriented functions aren't compatible with each other
// and for some reason fwide doesn't work for me
// Windows will strictly use wide print functions and linux will strictly use byte oriented print functions
// And to make it easier, I will use a macro to print literal strings. The macros are gonna be PRINT_LITERAL

#ifdef OS_WIN32

BOOL SetInputModeAndOutputCP(LPLONG lpdwInputModeAndOutputCP)
{
    lpdwInputModeAndOutputCP[0] = _setmode(STDIN_FILENO, lpdwInputModeAndOutputCP[0]);
    // I did LPLONG just because of this
    if (lpdwInputModeAndOutputCP[0] < 0)
        return 0;
;
    UINT wOldOutputCP = GetConsoleOutputCP();
    if (lpdwInputModeAndOutputCP[1] == 0 || !SetConsoleOutputCP(lpdwInputModeAndOutputCP[1]))
    {
        _setmode(STDIN_FILENO, lpdwInputModeAndOutputCP[0]);
        return 0;
    }
    lpdwInputModeAndOutputCP[1] = wOldOutputCP;
    return 1;
}
#endif // OS_WIN32

int main()
{
    #ifdef OS_WIN32
    // Sets the mode of stdin to UTF-16 to stdout to UTF-8 in a hacky way
    /*for (int i = 0; i < 2; ++i)
    {
        if (_setmode((int[2]){STDIN_FILENO, STDOUT_FILENO}[i], _O_U16TEXT) == -1)
        {
            printf("Failed to _setmode to with error %s\n", strerror(errno));
            return 1;
        }
    }*/

    LONG lpdwInputModeAndOutputCP[2] = {
        _O_U16TEXT,
        CP_UTF8
    };
    if (!SetInputModeAndOutputCP(lpdwInputModeAndOutputCP))
    {
        printf("Failed to set input mode and output console CP (errno %s, WinError %lu)\n", strerror(errno), GetLastError());
        return 1;
    }
    #endif // elifdef wasnt working for some reason
    #ifdef OS_UNIX
    const char *old_locale = setlocale(LC_ALL, SET_LOCALE_TO);
    if (old_locale == NULL)
    {
        printf("Failed to setlocale to '%s' with error %s\n", SET_LOCALE_TO, strerror(errno));
        return 1;
    }
    #endif

    textreader_t reader = textreader_openfileptr(stdin, READER_ENCODING);
    #ifdef OS_WIN32
    reader.flags |= TRFLG_USE_FGETWC;
    #endif

    const char *quit_msg = "exit";
    int quit_msg_len = 4;
    int matched_quit_msg = 0;

    int printed_start_of_input = 0;
    int printed_start_of_output = 0;
    int skip_current_line = 0;
    int32_t prev_chr = 0;

    uint8_t echo_chr_buffer[5];

    puts("Enter 'exit' to exit\n");
    while (1)
    {
        if (!printed_start_of_input)
        {
            fputs("echo >> ", stdout);
            printed_start_of_input = 1;
        }
        int32_t chr = textreader_getc(&reader);
        if (chr == EOF)
        {
            if (errno == EILSEQ)
            {
                printf("Invalid multibyte sequence!\n");
                skip_current_line = 1;
                continue;
            }
            if (textreader_eof(&reader))
                puts("Reached EOF!");
            else
                printf("Error while reading file: %s\n", strerror(errno));

            break;
        }
        int32_t chr_lower = chr >= 'A' && chr <= 'Z' ? chr + 32 : chr;
        if (matched_quit_msg >= 0 && (matched_quit_msg >= quit_msg_len || chr_lower == quit_msg[matched_quit_msg]))
        {
            if (matched_quit_msg >= quit_msg_len)
            {
                if (chr == '\r' || chr == '\n')
                {
                    puts("\"");
                    break;
                }
                matched_quit_msg = -1;
            }

            ++matched_quit_msg;
        }
        else matched_quit_msg = -1;

        // For whatever reason, \n always appears twice after pressing enter on windows
        // Maybe when you press enters it sends a \r\n and because of _setmode
        // There's an error that makes it convert \r into \n instead of \r\n into a single \n but who knows
        if (chr == '\n')
        {
            if (prev_chr != '\n')
            {
                if (!skip_current_line)
                    puts("\"\n");

                printed_start_of_input = 0;
                printed_start_of_output = 0;
            }
            skip_current_line = 0;
            prev_chr = chr;
            matched_quit_msg = 0;
            continue;
        }
        if (!skip_current_line)
        {
            if (!printed_start_of_output)
            {
                fputs("Echo'd text: \"", stdout);
                printed_start_of_output = 1;
            }

            int len = textprocessing_encode_chr(OUTPUT_CONVERT_ENCODING, chr, echo_chr_buffer);
            echo_chr_buffer[len] = 0;

            fputs((char*)echo_chr_buffer, stdout);
        }
        prev_chr = chr;
    }
    textreader_close(&reader, 0);
    #if defined(OS_UNIX)
    if (setlocale(LC_ALL, old_locale) == NULL)
        printf("Note: Failed to reset the locale to it's original state\n");

    #elif defined(OS_WIN32)
    if (!SetInputModeAndOutputCP(lpdwInputModeAndOutputCP))
        printf("Note: Failed to reset input mode and output console CP to their original state (errno %s, WinError %lu)\n", strerror(errno), GetLastError());

    #endif
    return 0;
}
