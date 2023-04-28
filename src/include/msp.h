#ifndef __MSP_H__
#define __MSP_H__

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

#if defined(__cplusplus)
}
#endif

#endif  /* __MSP_H__ */
