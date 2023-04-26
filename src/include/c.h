#ifndef __C_H__
#define __C_H__

#include <stdint.h>
#include "include/pg_config_manual.h"

#ifndef NULL
#define NULL    0
#endif

#if 0
#ifndef bool
typedef unsigned char bool;
#endif

#ifndef true
#define true	((bool) 1)
#endif

#ifndef false
#define false	((bool) 0)
#endif
#endif

typedef signed char int8;		/* == 8 bits */
typedef signed short int16;		/* == 16 bits */
typedef signed int int32;		/* == 32 bits */

typedef unsigned char uint8;	/* == 8 bits */
typedef unsigned short uint16;	/* == 16 bits */
typedef unsigned int uint32;	/* == 32 bits */

typedef uint8 bits8;			/* >= 8 bits */
typedef uint16 bits16;			/* >= 16 bits */
typedef uint32 bits32;			/* >= 32 bits */


typedef long long int int64;
typedef unsigned long long int uint64;

typedef size_t Size;

#define TYPEALIGN(ALIGNVAL,LEN)  \
	(((uintptr_t) (LEN) + ((ALIGNVAL) - 1)) & ~((uintptr_t) ((ALIGNVAL) - 1)))

#define MAXIMUM_ALIGNOF     8 
#define MAXALIGN64(LEN)		TYPEALIGN64(MAXIMUM_ALIGNOF, (LEN))

#define Assert(condition)	((void)true)
#define AssertMacro(condition)	((void)true)
#define AssertArg(condition)	((void)true)
#define AssertState(condition)	((void)true)
#define AssertPointerAlignment(ptr, bndr)	((void)true)
#define Trap(condition, errorType)	((void)true)
#define TrapMacro(condition, errorType) (true)


#endif /* __C_H__ */