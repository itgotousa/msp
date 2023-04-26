#ifndef __PG_BITUTILS_H__
#define __PG_BITUTILS_H__

#include "c.h"

static inline uint32
pg_rotate_left32(uint32 word, int n)
{
        return (word << n) | (word >> (32 - n));
}


#endif /* __PG_BITUTILS_H__ */
