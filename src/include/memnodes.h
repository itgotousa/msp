#ifndef __MEMNODES_H__
#define __MEMNODES_H__

#include "c.h"

typedef enum NodeTag
{
    T_Invalid = 0,
    T_AllocSetContext
} NodeTag;

typedef struct MemoryContextData *MemoryContext;

typedef struct MemoryContextData
{
	NodeTag		type;			/* identifies exact kind of context */
	/* these two fields are placed here to minimize alignment wastage: */
	bool		isReset;		/* T = no space alloced since last reset */
	bool		allowInCritSection; /* allow palloc in critical section */
	Size		mem_allocated;	/* track memory allocated for this context */
	//const MemoryContextMethods *methods;	/* virtual function table */
	MemoryContext parent;		/* NULL if no parent (toplevel context) */
	MemoryContext firstchild;	/* head of linked list of children */
	MemoryContext prevchild;	/* previous child of same parent */
	MemoryContext nextchild;	/* next child of same parent */
	const char *name;			/* context name (just for debugging) */
	const char *ident;			/* context ID if any (just for debugging) */
	//MemoryContextCallback *reset_cbs;	/* list of reset/delete callbacks */
} MemoryContextData;

#endif __MEMNODES_H__

