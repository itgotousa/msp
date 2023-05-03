#ifndef __PG_CONFIG_MANUAL_H__
#define __PG_CONFIG_MANUAL_H__

#define PG_INT64_TYPE long int

#define NAMEDATALEN 64

#define BLCKSZ 8192
#define XLOG_BLCKSZ 8192

#define MAXIMUM_ALIGNOF 8

#define MEMSET_LOOP_LIMIT 1024

#define PG_USE_STDBOOL

#define HAVE__BUILTIN_CLZ

#define HAVE_LONG_INT_64

#undef USE_ASSERT_CHECKING

#define USE_ASSERT_CHECKING

#define SIZEOF_VOID_P 8

#endif /* #define __PG_CONFIG_MANUAL_H__ */
