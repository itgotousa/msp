
/*
 * Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
 */

#include <stdio.h>
#include "unicode.h"

#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

static const U8 utf8d[] = {
    // The first part of the table maps bytes to character classes that
    // to reduce the size of the transition table and create bitmasks.
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

    // The second part is a transition table that maps a combination
    // of a state of the automaton and a character class to a state.
     0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
    12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
    12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
    12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
    12,36,12,12,12,12,12,12,12,12,12,12
};

U32 inline decode_utf8(U32* state, U32* codep, U8 byte) 
{
  U32 type = utf8d[byte];

  *codep = (*state != UTF8_ACCEPT) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);

  *state = utf8d[256 + *state + type];

  return *state;
}

#if 1
U32 CodePointCount(U8* s, U32 bytes, U32* count)
{
    U8* p;
    U32 codepoint, i, num;
    U32 state = UTF8_ACCEPT;

    p = s;  num = 0;
    for (i = 0; i < bytes; i++)
    {
        if (UTF8_ACCEPT == decode_utf8(&state, &codepoint, *p++)) num++;
    }

    if (NULL != count) *count = num;
    return (UTF8_ACCEPT != state);
}
#endif 

U32 UTF8toUTF16(U8* src, U32 srcBytes, U16* dst, U32 dstWords)
{
    U8* src_actual_end = src + srcBytes;
    U8* s = src;
    U16* d = dst;
    U32 codepoint;
    U32 state = UTF8_ACCEPT;
    U32 dst_words_free;
    U8* src_current_end;

    while (s < src_actual_end)
    {
        dst_words_free = dstWords - (d - dst);
        src_current_end = s + dst_words_free - 1;
        if (src_actual_end < src_current_end) src_current_end = src_actual_end;
        if (src_current_end <= s) goto TooSmall;
        while (s < src_current_end)
        {
            if (decode_utf8(&state, &codepoint, *s++)) continue;
            if (codepoint > 0xFFFF)
            {
                *d++ = (U16)(0xD7C0 + (codepoint >> 10));
                *d++ = (U16)(0xDC00 + (codepoint & 0x3FF));
            }
            else { *d++ = (U16)codepoint; }
        }
    }

    if (UTF8_ACCEPT != state)
    { }

    if (0 == (dstWords - (d - dst))) goto TooSmall;
    *d++ = 0;
TooSmall:
    return 0;
}

/* 
 * giving a string, scan from the beginning, and stop until the character that is not UTF8 character.
 * surrogate is used to conver UTF8 to UTF16
 */
U32 verify_utf8_string(U8* buffer, U32 bytes, U32* characters, U32* surrogate)
{
    U8* p;
    U8 charlen, k;
    U32 i, char_num, sgt_num;

    i = 0; char_num = sgt_num = 0;
    p = buffer;
    while (i < bytes) /* get all UTF-8 characters until we meed a none-UTF8 chararcter */
    {
        if (0 == (0x80 & *p))           charlen = 1;  /* 1-byte character */
        else if (0xE0 == (0xF0 & *p))   charlen = 3;  /* 3-byte character */
        else if (0xC0 == (0xE0 & *p))   charlen = 2;  /* 2-byte character */
        else if (0xF0 == (0xF8 & *p))   charlen = 4;  /* 4-byte character */
        else goto verification_end;  /* it is not UTF-8 character anymore */

        if (i > bytes - charlen) goto verification_end; 
        for (k = 1; k < charlen; k++)
        {
            if (0x80 != (0xC0 & *(p + k))) goto verification_end;
        }
        p += charlen; i += charlen; char_num++;

        if (4 == charlen) sgt_num += 2;
    }

verification_end:
    if (NULL != characters) *characters = char_num;
    if (NULL != surrogate)  *surrogate = sgt_num;
    return i;
}