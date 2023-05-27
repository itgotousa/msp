#ifndef __UNICODE_H__
#define __UNICODE_H__

#include "msp_common.h"

U32 verify_utf8_string(U8* buffer, U32 bytes, U32* characters, U32* surrogate);

#endif /* __UNICODE_H__ */