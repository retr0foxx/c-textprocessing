#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "textreader.h"

#define TESTED_TYPE_FILE 0
#define TESTED_TYPE_FILE_PTR 1
#define TESTED_TYPE_MEM 2

#define test_func(name) int name(void *source, int type, size_t memlen)

#define test_create_reader(varname, encoding)                       \
    textreader_t varname;                                           \
    do {                                                            \
        int rv;                                                     \
        switch (type)                                               \
        {                                                           \
        case TESTED_TYPE_FILE:                                      \
            rv = textreader_initfile(&varname, source, encoding);   \
            break;                                                  \
        case TESTED_TYPE_FILE_PTR:                                  \
            varname = textreader_openfileptr((FILE*)source, encoding);\
            break;                                                  \
        case TESTED_TYPE_MEM:                                       \
            varname = textreader_openmem(source, memlen, encoding); \
            break;                                                  \
        }                                                           \
        if (rv != 0)                                                \
        {                                                           \
            fprintf(                                                \
                stderr,                                             \
                "Failed to open file of type \'%s\' with error: %s\n",      \
                (const char*[3]){"file", "file pointer", "memory"}[type],   \
                strerror(errno)                                     \
            );                                                      \
            exit(1);                                                \
        }                                                           \
    } while (0)

#define exchndl_eof(fcall) do (                                                                 \
        if ((fcall) == EOF)                                                                     \
        {                                                                                       \
            fprintf(stderr, "A function call failed with EOF with error: %s\n", strerror(errno));\
            exit(1);                                                                            \
        }                                                                                       \
    } while(0)

int32_t trgetc_exchndl(textreader_t *reader)
{
    int32_t c = textreader_getc(reader);
    if (c == EOF && textreader_error(reader))
    {
        fprintf(stderr, "A function call failed with EOF with error: %s\n", strerror(errno));\
        exit(1);
    }
    return c;
}

void test_trgetc_all(textreader_t *reader)
{
    int32_t uc;
    size_t read_count = 0;
    for (; (uc = trgetc_exchndl(reader)) != EOF; ++read_count)
    {
        wprintf(L"<charcode=%ld, char='%c'>\n", uc, uc);
    }
    printf("Read %I64u characters.\n", read_count);
}

test_func(test_utf16be_nobom)
{
    test_create_reader(reader, TRENC_UTF16BE);
    test_trgetc_all(&reader);
    textreader_close(&reader, 1);
}

test_func(test_utf16be_bom)
{
    test_create_reader(reader, TRENC_UTF16LE);
    test_trgetc_all(&reader);
    textreader_close(&reader, 1);
}

test_func(test_utf16_guess)
{
    test_create_reader(reader, -1);
    textreader_guess_encoding(&reader);
    printf("Guessed encoding: %i\n", reader.encoding);
    test_trgetc_all(&reader);
    textreader_close(&reader, 1);
}

test_func(test_utf8)
{
    
}

test_func(test_utf8_and_utf16_invalid_file)
{
    
}

test_func(test_ungetc)
{
    
}

int main()
{
    test_utf16be_nobom("\60\157\0\167\330\122\337\142", TESTED_TYPE_MEM, 8);
}