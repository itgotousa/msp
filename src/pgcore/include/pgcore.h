#ifndef __PGCORE_H__
#define __PGCORE_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdlib.h>
#include <memory.h>
#include <intrin.h>
#include <string.h>
#include <process.h>

#include "pg_config_manual.h"
#include "c.h"
#include "pg_wchar.h"
#include "attnum.h"
#include "miscadmin.h"
#include "elog.h"
#include "pg_bitutils.h"
#include "palloc.h"
#include "nodes.h"
#include "memnodes.h"
#include "memdebug.h"
#include "memutils.h"
#include "hsearch.h"
#include "pg_list.h"
#include "bitmapset.h"
#include "plannodes.h"
#include "s_lock.h"
#include "spin.h"
#include "hashfn.h"
#include "port.h"
#include "postgres.h"
#include "stringinfo.h"

#if defined(__cplusplus)
}
#endif

#endif  /* __PGCORE_H__ */
