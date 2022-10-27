#include <string.h>

#include <textprocessing/textprocessing.h>

// Changes a 16 bit integer into big or little edian
uint16_t u16_endian_change(uint16_t n, int is_big_endian)
{
    uint16_t rslt;
    uint8_t *rsltp = (uint8_t*)&rslt;
    rsltp[is_big_endian] = n & 0xff;
    rsltp[(is_big_endian+1) % 2] = (n & 0xff00) >> 8;
    return rslt;
}

#define N_BIT_ON(n) ((1 << (n))-1)

// Encodes the character into the buffer
int textprocessing_encode_chr(textprocessing_encoding_t enc, int32_t chr, uint8_t *buffer)
{
    if (((TPENC_UTF8 | TPENC_UTF16LE | TPENC_UTF16BE) & enc) && chr > 1048576)
    {
        errno = EILSEQ;
        return -1;
    }
    switch (enc)
    {
    case TPENC_ASCII:
        if (chr > 0x80)
        {
            errno = EILSEQ;
            return -1;
        }
        buffer[0] = (char)chr;
        return 1;
    case TPENC_UTF16BE:
    case TPENC_UTF16LE:
    case TPENC_UTF16:
        {
            uint16_t *buffer16 = (uint16_t*)buffer;
            int len;
            if (chr <= 0xffff) // Meaning it can fit in a single 16 bit integer
            {
                // These are the prefix marker for characters that takes more than a single 16 bit integer
                // So a single 16 bit integer has this marker, a reader might mistake it for 2 characters
                // So it shouldn't be allowed for a single "byte" character to have these
                if ((chr & (0b111111 << 10)) == (0b110111 << 10))
                {
                    errno = EILSEQ;
                    return -1;
                }
                *buffer16 = (uint16_t)chr;
                len = 2;
            }
            else
            {
                // do the REAL utf 16 encoding
                chr -= 0x10000;
                buffer16[0] = (0x36 << 10) | ((chr & (((1 << 10)-1) << 10)) >> 10);
                buffer16[1] = (0x37 << 10) |  (chr & ((1 << 10)-1));
                len = 4;
            }
            if (enc != TPENC_UTF16)
            {
                // If there's a specific endianness then change the endianness to that
                int is_be   = (enc == TPENC_UTF16BE);
                buffer16[0] = u16_endian_change(buffer16[0], is_be);
                buffer16[1] = u16_endian_change(buffer16[1], is_be);
            }
            return len;
        }
    case TPENC_UTF8:
        if (chr <= 0x80)
        {
            buffer[0] = chr;
            return 1;
        }
        {
            int i = 0;
            for (; i < 4; ++i)
            {
                // create the mask for the 6 bits for the current byte
                uint32_t mask = (uint32_t)0x3f << (i * 6);

                // add the 6 bits for the current byte OR'd with 0x80
                buffer[3-i] = ((chr & mask) >> (i * 6)) | 0x80;

                // break if there are no remaining bits
                if ((chr & ((uint32_t)~0 << ((i+1) * 6))) == 0)
                    break;
            }
            int byte_count_indicator_bit_len = i + 2;
            int byte_count_indicator_offset = (8-byte_count_indicator_bit_len);
            uint8_t byte_count_indicator = (N_BIT_ON(byte_count_indicator_bit_len-1) << (byte_count_indicator_offset+1));

            // check if there's enough space on the last byte to add the byte count indicator
            if ((buffer[3-i] & (N_BIT_ON(byte_count_indicator_bit_len-2) << byte_count_indicator_offset)) != 0)
            {
                // if there isn't then add another `1` to the byte count indicator and then put it in another byte after
                ++i; ++byte_count_indicator_bit_len;
                byte_count_indicator |= (1 << byte_count_indicator_offset);
            }

            // reset the byte for the byte count indicator and add the byte count indicator
            // question: instead of resetting why not just set buffer[3-i] to 0 if it puts the indicator at the next byte ?
            buffer[3-i] &= ~(N_BIT_ON(byte_count_indicator_bit_len) << byte_count_indicator_offset);
            buffer[3-i] |= byte_count_indicator;

            memmove(buffer, buffer + (3-i), i+1);
            return i+1;
        }
        // break;
    }
    errno = EINVAL;
    return -1;
}
