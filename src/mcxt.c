
#include "include/memnodes.h"

/*****************************************************************************
 *	  GLOBAL MEMORY															 *
 *****************************************************************************/

/*
 * CurrentMemoryContext
 *		Default memory context for allocations.
 */
MemoryContext CurrentMemoryContext = NULL;
MemoryContext TopMemoryContext = NULL;

