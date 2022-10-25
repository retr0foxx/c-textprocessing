#ifndef TEXTREADER_H_INCLUDED
#define TEXTREADER_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#ifndef TEXTREADER_PUSHBACK_BUFFER_SIZE
#define TEXTREADER_PUSHBACK_BUFFER_SIZE 512
#endif

/*
    future supported encodings:
    utf-8, utf-16 be/le

    ok.. so i think this is how this is going to work:
    getc will return an int32_t and ungetc will accept int32_t to support unicode characters
    ~~the textreader_t will handle the ungetc stuff because it needs to support int32_t~~
    actually nevermind, so i literally just realized that the ungetc buffer could be more than just 1 character
    and on some systems, the buffer on an actual FILE could be 4kb so I don't really wanna waste it if that was the case
    so ungetc would still accept int32_t but now it will try to use the file's ungetc if that is the type of the textreader
*/
typedef enum textreader_encoding
{
    TRENC_ASCII,
    TRENC_UTF8,
    TRENC_UTF16 = 0b100,
    TRENC_UTF16LE,
    TRENC_UTF16BE,
} textreader_encoding_t;

int textreader_encode_chr(textreader_encoding_t enc, int32_t chr, char *buffer);

typedef enum textreader_flags
{
    TRFLG_ISMEM = 1,
    TRFLG_EOF,
    TRFLG_ERROR = 4,
    TRFLG_USE_FGETWC = 8,
    TRFLG_IS_INIT_FILEPTR = 16
} textreader_flags_t;

typedef struct textreader
{
    union
    {
        FILE *file;
        struct
        {
            const void *mem;
            size_t size;
            size_t index;
        } mem;
    } data;
    struct {
        size_t size;
        char buffer[TEXTREADER_PUSHBACK_BUFFER_SIZE];
    } ungetc_stack;
    size_t total_ungetc_count;
    off_t text_start_offset;
    textreader_flags_t flags;
    textreader_encoding_t encoding;
    int wchar_buffer_used;
    wchar_t wchar_buffer;
} textreader_t;

int textreader_initfile(textreader_t *reader, const char *filename, textreader_encoding_t encoding);
textreader_t textreader_openfileptr(FILE *file, textreader_encoding_t encoding);
textreader_t textreader_openmem(const void *mem, size_t size, textreader_encoding_t encoding);

#define TEXTREADER_INIT_FILEPTR 0
#define TEXTREADER_INIT_FILE    1
#define TEXTREADER_INIT_MEM     2

/*
 *  textreader_init acts as a higher level textreader init function
 *  it will atuomatically initialize the text encoding and stuff
 *
 *  it will try call the other init/open functions according to the init type and then try to init the encoding
 *  if text_encoding is -1 then it will try to guess the encoding first and then init the encoding
 *
 *  if all of that succeded, it'll end up at the start of the text
 *  the data memlem parameter is not needed for files
 */
int textreader_init(textreader_t *reader, void *data, size_t memlen, textreader_encoding_t encoding, int init_type);

// guesses the encoding and set the textreader's encoding to the guessed encoding
// also sets text_start_offset if the file has a BOM
textreader_encoding_t textreader_guess_encoding(textreader_t *reader);

// im not sure what to name this
// but it currently just does something like setting the text start offset if there's a BOM for the specified encoding
int textreader_init_encoding(textreader_t *reader);

int textreader_get_byte(textreader_t *reader);
int32_t textreader_getc(textreader_t *reader);
int32_t textreader_ungetc(textreader_t *reader, int32_t chr);

#define TRSEEK_TEXT_SET 4
#define TRSEEK_TEXT_CUR 5
#define TRSEEK_TEXT_END 6

// i just observed ftell's behaviour if i called it after doing ungetc and it looks like it just returns -1 if that happens and errno is just.. not set ?
// well it sets it to 2 when i tried it but that just means no such file or directory which doesn't make sensex
int textreader_seek(textreader_t *reader, long offset, int whence);
long textreader_tell(textreader_t *reader);

int textreader_seeko(textreader_t *reader, off_t offset, int whence);
off_t textreader_tello(textreader_t *reader);

int textreader_error(textreader_t *reader);
int textreader_eof(textreader_t *reader);

void textreader_clearerr(textreader_t *reader);

// if close_file is negative then it will close it depending on if it was open with a file pointer (if it's not then close)
int textreader_close(textreader_t *reader, int close_file);

#endif
