#ifndef __UNICODE_H__
#define __UNICODE_H__

#include "msp_common.h"

U32 verify_utf8_string(U8* buffer, U32 bytes, U32* characters, U32* surrogate);

U32 UTF8toUTF16(U8* utf8, U32 utf8Bytes, U16* utf16);

#endif /* __UNICODE_H__ */