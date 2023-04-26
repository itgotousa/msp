//#include

/*
 * Constants
 *
 * A hash table has a top-level "directory", each of whose entries points
 * to a "segment" of ssize bucket headers.  The maximum number of hash
 * buckets is thus dsize * ssize (but dsize may be expansible).  Of course,
 * the number of records in the table can be larger, but we don't want a
 * whole lot of records per bucket or performance goes down.
 *
 * In a hash table allocated in shared memory, the directory cannot be
 * expanded because it must stay at a fixed address.  The directory size
 * should be selected using hash_select_dirsize (and you'd better have
 * a good idea of the maximum number of entries!).  For non-shared hash
 * tables, the initial directory size can be left at the default.
 */
#define DEF_SEGSIZE			   256
#define DEF_SEGSIZE_SHIFT	   8	/* must be log2(DEF_SEGSIZE) */
#define DEF_DIRSIZE			   256

/* Number of freelists to be used for a partitioned hash table. */
#define NUM_FREELISTS			32