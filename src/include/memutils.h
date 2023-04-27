#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

/*
 * Recommended default alloc parameters, suitable for "ordinary" contexts
 * that might hold quite a lot of data.
 */
#define ALLOCSET_DEFAULT_MINSIZE   0
#define ALLOCSET_DEFAULT_INITSIZE  (8 * 1024)
#define ALLOCSET_DEFAULT_MAXSIZE   (8 * 1024 * 1024)
#define ALLOCSET_DEFAULT_SIZES \
	ALLOCSET_DEFAULT_MINSIZE, ALLOCSET_DEFAULT_INITSIZE, ALLOCSET_DEFAULT_MAXSIZE

/*
 * Recommended alloc parameters for "small" contexts that are never expected
 * to contain much data (for example, a context to contain a query plan).
 */
#define ALLOCSET_SMALL_MINSIZE	 0
#define ALLOCSET_SMALL_INITSIZE  (1 * 1024)
#define ALLOCSET_SMALL_MAXSIZE	 (8 * 1024)
#define ALLOCSET_SMALL_SIZES \
	ALLOCSET_SMALL_MINSIZE, ALLOCSET_SMALL_INITSIZE, ALLOCSET_SMALL_MAXSIZE

/*
 * Recommended alloc parameters for contexts that should start out small,
 * but might sometimes grow big.
 */
#define ALLOCSET_START_SMALL_SIZES \
	ALLOCSET_SMALL_MINSIZE, ALLOCSET_SMALL_INITSIZE, ALLOCSET_DEFAULT_MAXSIZE

extern PGDLLIMPORT MemoryContext TopMemoryContext;

#endif /* __MEMUTILS_H__ */
