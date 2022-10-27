#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <textprocessing/textreader.h>

#define N_BIT_ON(n) ((1 << (n))-1)

textreader_t textreader_openfileptr(FILE *file, textprocessing_encoding_t encoding)
{
    return (textreader_t){
        .data = {
            .file = file
        },
        .flags = TRFLG_IS_INIT_FILEPTR,
        .encoding = encoding,
        .ungetc_stack = {
            .size = 0
        }
    };
}

int textreader_initfile(textreader_t *reader, const char *filename, textprocessing_encoding_t encoding)
{
    printf("Opening: %s\n", filename);
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
        return -1;

    *reader = textreader_openfileptr(file, encoding);
    return 0;
}

textreader_t textreader_openmem(const void *mem, size_t size, textprocessing_encoding_t encoding)
{
    return (textreader_t){
        .data = {
            .mem = {
                .mem = mem,
                .size = size,
                .index = 0,
            }
        },
        .flags = TRFLG_ISMEM,
        .encoding = encoding,
        .ungetc_stack = {
            .size = 0
        }
    };
}

textprocessing_encoding_t textreader_check_bom(textreader_t *reader)
{
    unsigned char buf[2];
    textprocessing_encoding_t rslt = -2;
    // printf("Entered check_bom\n");
    for (int i = 0; i < 4; ++i)
    {
        int c = textreader_get_byte(reader);
        buf[i] = c;
        // printf("%i. Read '\\x%x'\n", i, (unsigned char)c);
        if (c == EOF)
        {
            // printf("EOF when checking for BOM\n");
            if (textreader_eof(reader))
                return rslt;

            return -1;
        }
        if (i == 1)
        {
            // printf("First two characters are: \\x%x\\x%x\n", buf[0], buf[1]);
            if (!memcmp(buf, "\xff\xfe", 2))
                rslt = TPENC_UTF16LE;
            else if (!memcmp(buf, "\xfe\xff", 2))
                rslt = TPENC_UTF16BE;
        }
        else if (i == 2)
        {
            // printf("First three characters are: \\x%x\\x%x\\x%x\n", buf[0], buf[1], buf[2]);
            if (!memcmp(buf, "\xef\xbb\xbf", 3))
                rslt = TPENC_UTF8;
        }
        // printf("rslt is %i\n", rslt);
        if (rslt != -2)
            return rslt;
    }

    return rslt;
}

// Guess encoding is done in textreader_init_encoding
/*
// currently just tries to guess from BOM
textprocessing_encoding_t textreader_guess_encoding(textreader_t *reader)
{
    printf("Entered guess encoding\n");
    textprocessing_encoding_t rslt = textreader_check_bom(reader);
    if (rslt == -1) {
        return -1; printf("Exited guess encoding\n"); }
    else if (rslt == -2)
        rslt = TPENC_UTF8;

    reader->encoding = rslt;
    printf("Exited guess encoding\n");
    return rslt;
}*/

#define TEXTREADER_INIT_FILEPTR 0
#define TEXTREADER_INIT_FILE    1
#define TEXTREADER_INIT_MEM     2

int textreader_init(textreader_t *reader, void *data, size_t memlen, textprocessing_encoding_t encoding, int init_type)
{
    /*#define error_close_reader(reader, type) do {   \
        if (type == TEXTREADER_INIT_FILEPTR)        \
            textreader_close(reader, 0);            \
        else                                        \
            textreader_close(reader, 1);            \
    } while (0)*/

    switch (init_type)
    {
    case TEXTREADER_INIT_FILEPTR:
        *reader = textreader_openfileptr(data, encoding);
        break;
    case TEXTREADER_INIT_FILE:
        if (textreader_initfile(reader, data, encoding) != 0)
            return -1;

        break;
    case TEXTREADER_INIT_MEM:
        *reader = textreader_openmem(data, memlen, encoding);
        break;
    }
    // if this language has exceptions man
    // this would all just be like 5 lines
    /*
    if (encoding == -1)
        textreader_guess_encoding(reader);

    textreader_seek(reader, 0, SEEK_START);
    textreader_init_encoding(reader);
    */
    /*if (encoding == -1)
    {
        printf("Calling guess encoding\n");
        if (textreader_guess_encoding(reader) < 0)
        {
            error_close_reader(reader, init_type);
            return -1;
        }
    }
    if (textreader_seek(reader, 0, SEEK_SET) != 0)
    {
        error_close_reader(reader, init_type);
        return -1;
    }*/
    if (textreader_init_encoding(reader) != 0)
    {
        textreader_close(reader, init_type);
        return -1;
    }
    return 0;
}

int textreader_init_encoding(textreader_t *reader)
{
    textprocessing_encoding_t bom = textreader_check_bom(reader);

    // mark
    if (bom != -2 && (bom == reader->encoding || reader->encoding == -1))
    {
        reader->encoding = bom;
        if (reader->encoding & TPENC_UTF16)
            reader->text_start_offset = 2;
        else
            reader->text_start_offset = 3;
    }

    if (textreader_seek(reader, 0, TRSEEK_TEXT_SET) != 0)
    {
        textreader_close(reader, -1);
        return -1;
    }
    return 0;
}

// gets a single byte to be decoded
int textreader_get_byte(textreader_t *reader)
{
    if (reader->ungetc_stack.size > 0)
        return reader->ungetc_stack.buffer[--reader->ungetc_stack.size];

    if ((reader->flags & TRFLG_ISMEM) == 0)
    {
        // This exists so that I can easily get unicode stdin input on Windows.
        // Couldn't find a way to get unicode input without fgetwc on Windows.
        if (reader->flags & TRFLG_USE_FGETWC && reader->total_ungetc_count == 0)
        {
            // Even though it uses fgetwc here it will still return the result byte by byte
            // That's what wchar_buffer is for
           if (reader->wchar_buffer_used == 0)
            {
                reader->wchar_buffer = fgetwc(stdin);
                if (reader->wchar_buffer == WEOF)
                    return EOF;

                reader->wchar_buffer_used = sizeof(wchar_t);
            }
            // Returns the offset-th byte
            wchar_t offset = ((sizeof(wchar_t) - reader->wchar_buffer_used--) * 8);
            return (reader->wchar_buffer & (0xff << offset)) >> offset;
        }
        return fgetc(reader->data.file);
    }
    if (reader->data.mem.index >= reader->data.mem.size)
    {
        reader->flags |= TRFLG_EOF;
        return EOF;
    }
    return ((unsigned char*)reader->data.mem.mem)[reader->data.mem.index++];
}

// idek man ill just do this for now

#define DEFINE_TELL_AND_SEEK(seek_fname, tell_fname, fseek_fname, ftell_fname, offset_type)         \
    int seek_fname(textreader_t *reader, offset_type pos, int whence)\
    {                                                                \
        if (whence == TRSEEK_TEXT_SET)                               \
            pos += reader->text_start_offset;                        \
                                                                     \
        if (whence >= 4 && whence <= 6)                              \
            whence -= 4;                                             \
                                                                     \
        if ((reader->flags & TRFLG_ISMEM) == 0)                      \
        {                                                            \
            int rv = fseek_fname(reader->data.file, pos, whence);    \
            if (rv != EOF)                                           \
                reader->ungetc_stack.size = 0;                       \
                                                                     \
            return rv;                                               \
        }                                                            \
                                                                     \
        size_t new_index = 0;                                        \
        switch (whence)                                              \
        {                                                            \
        case SEEK_SET:                                               \
            if (pos < 0)                                             \
                return -1;                                           \
                                                                     \
            new_index = pos;                                        \
            break;                                                   \
        case SEEK_CUR:                                               \
            if (pos < 0 && -pos > reader->data.mem.index)            \
                return -1;                                           \
                                                                     \
            new_index = reader->data.mem.index + pos;                \
            break;                                                   \
        case SEEK_END:                                               \
            if (pos < 0 && -pos > reader->data.mem.size)             \
                return -1;                                           \
                                                                     \
            new_index = reader->data.mem.size + pos;                 \
            break;                                                   \
        }                                                            \
        reader->flags ^= TRFLG_EOF;                                  \
        reader->ungetc_stack.size = 0;                               \
        reader->total_ungetc_count = 0;                              \
        return 0;                                                    \
    }                                                                \
                                                                     \
    offset_type tell_fname(textreader_t *reader)                     \
    {                                                                \
        if (reader->ungetc_stack.size > 0)                           \
            return EOF;                                              \
                                                                     \
        if ((reader->flags & TRFLG_ISMEM) == 0)                      \
            return ftell_fname(reader->data.file);                   \
                                                                     \
        if ((offset_type)reader->data.mem.index !=                   \
            reader->data.mem.index)                                  \
        {                                                            \
            reader->flags |= TRFLG_ERROR;                            \
            errno = EOVERFLOW;                                       \
            return EOF;                                              \
        }                                                            \
                                                                     \
        return (offset_type)reader->data.mem.index;                  \
    }

DEFINE_TELL_AND_SEEK(textreader_seek, textreader_tell, fseek, ftell, long);
DEFINE_TELL_AND_SEEK(textreader_seeko, textreader_tello, fseeko, ftello, off_t);

/*int textreader_seek(textreader_t *reader, long pos, int whence)
{
    if ((reader->flags & TRFLG_ISMEM) == 0)
        return fseek(reader->data.file, pos, whence);

    size_t new_index;
    switch (whence)
    {
    case SEEK_SET:
        if (pos < 0)
            return -1;

        new_index = pos;
        break;
    case SEEK_CUR:
        if (pos < 0 && -pos > reader->data.mem.index)
            return -1;

        new_index = reader->data.mem.index + pos;
        break;
    case SEEK_END:
        if (pos < 0 && -pos > reader->data.mem.size)
            return -1;

        new_index = reader->data.mem.size + pos;
        break;
    }
    reader->flags ^= TRFLG_EOF;
    return 0;
}

long textreader_tell(textreader_t *reader)
{
    if ((reader->flags & TRFLG_ISMEM) == 0)
        return ftell(reader->data.file);

    if (reader->data.mem.ungetc_chr != EOF)
        return EOF;

    return reader->data.mem.index;
}*/

int textreader_eof(textreader_t *reader)
{
    if ((reader->flags & TRFLG_ISMEM) == 0)
        return feof(reader->data.file);

    return reader->flags & TRFLG_EOF;
}

int textreader_error(textreader_t *reader)
{
    if ((reader->flags & TRFLG_ISMEM) == 0)
        return ferror(reader->data.file);

    return reader->flags & TRFLG_ERROR;
}

int textreader_ungetc(textreader_t *reader, int32_t chr)
{
    if (TEXTREADER_PUSHBACK_BUFFER_SIZE - reader->ungetc_stack.size < 4)
        return EOF;

    char ungetcd[4];
    int ungetcd_len = textprocessing_encode_chr(TPENC_UTF8, chr, ungetcd);
    if (ungetcd_len < 0)
        return EOF;

    int successfully_ungetcd = 0;
    // If it's a file, ungetc what it can to the file
    // And then ungetc it into the buffer textreader buffer so 0 memory wasted :DDDDDDDd
    if ((reader->flags & TRFLG_ISMEM) == 0 && reader->ungetc_stack.size == 0)
    {
        for (; successfully_ungetcd < ungetcd_len; ++successfully_ungetcd)
        {
            if (ungetc(ungetcd[ungetcd_len - successfully_ungetcd - 1], reader->data.file) == EOF)
                break;
        }
    }
    for (; successfully_ungetcd < ungetcd_len; ++successfully_ungetcd)
        reader->ungetc_stack.buffer[reader->ungetc_stack.size++] = ungetcd[ungetcd_len - successfully_ungetcd - 1];

    ++reader->total_ungetc_count;
    reader->flags &= ~TRFLG_EOF;
    return 0;
}

int32_t textreader_getc(textreader_t *reader)
{
    reader->flags &= ~TRFLG_EOF;
    int enc = reader->total_ungetc_count == 0 ? reader->encoding : TPENC_UTF8;
    int32_t rslt = 0;
    switch (enc)
    {
    case TPENC_ASCII:
        rslt = textreader_get_byte(reader);
        break;
    case TPENC_UTF16BE:
    case TPENC_UTF16LE:
    case TPENC_UTF16:
        {
            int multiple_nums = 0;
            for (int i = 0; i < 2; ++i)
            {
                uint16_t curchr = 0;
                for (int l = 0; l < 2; ++l)
                {
                    int c = textreader_get_byte(reader);
                    if (c == EOF)
                    {
                        if (textreader_eof(reader) && i != 0 && l != 0)
                        {
                            errno = EILSEQ;
                            reader->flags |= TRFLG_ERROR;
                        }
                        return EOF;
                    }
                    if (enc == TPENC_UTF16)
                    {
                        ((uint8_t*)&curchr)[l] = c;
                    }
                    else
                    {
                        int offset_byte = (enc == TPENC_UTF16BE ? 1-l : l);
                        curchr |= c << (offset_byte * 8);
                    }
                }
                if (i == 0 && !(multiple_nums = ((curchr & (0x3f << 10)) == 0xd800)))
                {
                    rslt = curchr;
                    break;
                }
                else if (i == 1 && (curchr & (0x3f << 10)) != 0xdc00)
                {
                    errno = EILSEQ;
                    reader->flags |= TRFLG_ERROR;
                    return EOF;
                }
                rslt |= ((curchr & 0x3ff) << ((1-i) * 10));
            }
            // this really caused me a lot of trouble
            // because i did not frickin realize that i was supposed to do this
            if (multiple_nums)
                rslt += 0x10000;
        }
        break;
    case TPENC_UTF8:
        {
            int c = textreader_get_byte(reader);
            // printf("\nGot byte: %02x\n", c);
            if (c == EOF)
                return EOF;

            int byte_count = 0;
            for (; (c & (1 << (7-byte_count))) != 0; ++byte_count);
            if (byte_count == 0)
            {
                rslt = c;
                break;
            }
            int bits = 7-byte_count;
            rslt |= ((uint8_t)c & N_BIT_ON(bits)) << 6;
            --byte_count;
            //printf("Bits = %i; byte_count = %i; n = %02x\n", bits, byte_count, ((uint8_t)c & N_BIT_ON(bits)) << bits);
            for (int i = 0; i < byte_count; ++i)
            {
                if ((c = textreader_get_byte(reader)) == EOF)
                {
                    if (textreader_eof(reader))
                    {
                        errno = EILSEQ;
                        reader->flags |= TRFLG_ERROR;
                    }

                    return EOF;
                }
                // printf("Got byte: %02x\n", c);
                if ((c & 0xc0) != 0x80)
                {
                    errno = EILSEQ;
                    reader->flags |= TRFLG_ERROR;
                    return EOF;
                }
                if (i > 0)
                    rslt <<= 6;

                rslt |= ((int32_t)(c & 0x7f));
                bits += 7;
            }
        }
        break;
    default:
        errno = EINVAL;
        return EOF;
    }
    if (reader->total_ungetc_count)
        --reader->total_ungetc_count;

    return rslt;
}

void textreader_clearerr(textreader_t *reader)
{
    if ((reader->flags & TRFLG_ISMEM) == 0)
        clearerr(reader->data.file);

    reader->flags &= ~TRFLG_ERROR;
}

int textreader_close(textreader_t *reader, int close_file)
{
    if ((reader->flags & TRFLG_ISMEM) == 0 && (close_file || reader->flags & TRFLG_IS_INIT_FILEPTR))
        return fclose(reader->data.file);

    return 0;
}
