#ifndef __MEMDEBUG_H__
#define __MEMDEBUG_H__

#define VALGRIND_CHECK_MEM_IS_DEFINED(addr, size)			do {} while (0)
#define VALGRIND_CREATE_MEMPOOL(context, redzones, zeroed)	do {} while (0)
#define VALGRIND_DESTROY_MEMPOOL(context)					do {} while (0)
#define VALGRIND_MAKE_MEM_DEFINED(addr, size)				do {} while (0)
#define VALGRIND_MAKE_MEM_NOACCESS(addr, size)				do {} while (0)
#define VALGRIND_MAKE_MEM_UNDEFINED(addr, size)				do {} while (0)
#define VALGRIND_MEMPOOL_ALLOC(context, addr, size)			do {} while (0)
#define VALGRIND_MEMPOOL_FREE(context, addr)				do {} while (0)
#define VALGRIND_MEMPOOL_CHANGE(context, optr, nptr, size)	do {} while (0)

static inline void
wipe_mem(void *ptr, size_t size)
{
	VALGRIND_MAKE_MEM_UNDEFINED(ptr, size);
	memset(ptr, 0x7F, size);
	VALGRIND_MAKE_MEM_NOACCESS(ptr, size);
}

#endif /* __MEMDEBUG_H__ */
