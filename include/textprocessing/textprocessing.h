#ifndef TEXTPROCESSING_H_INCLUDED
#define TEXTPROCESSING_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

typedef enum textprocessing_encoding
{
    TPENC_ASCII,
    TPENC_UTF8,
    TPENC_UTF16 = 0b100,
    TPENC_UTF16LE,
    TPENC_UTF16BE,
} textprocessing_encoding_t;

uint16_t u16_endian_change(uint16_t n, int is_big_endian);

int textprocessing_encode_chr(textprocessing_encoding_t enc, int32_t chr, uint8_t *buffer);

#endif // TEXTPROCESSING_H_INCLUDED
