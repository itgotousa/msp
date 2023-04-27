#ifndef __PALLOC_H__
#define __PALLOC_H__

/*
 * Type MemoryContextData is declared in nodes/memnodes.h.  Most users
 * of memory allocation should just treat it as an abstract type, so we
 * do not provide the struct contents here.
 */
typedef struct MemoryContextData *MemoryContext;

/*
 * A memory context can have callback functions registered on it.  Any such
 * function will be called once just before the context is next reset or
 * deleted.  The MemoryContextCallback struct describing such a callback
 * typically would be allocated within the context itself, thereby avoiding
 * any need to manage it explicitly (the reset/delete action will free it).
 */
typedef void (*MemoryContextCallbackFunction) (void *arg);

typedef struct MemoryContextCallback
{
	MemoryContextCallbackFunction func; /* function to call */
	void	   *arg;			/* argument to pass it */
	struct MemoryContextCallback *next; /* next in list of callbacks */
} MemoryContextCallback;


#endif /* __PALLOC_H__ */

