/*-------------------------------------------------------------------------
 *
 * pg_wchar.h
 *	  multibyte-character support
 *
 * Portions Copyright (c) 1996-2022, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/mb/pg_wchar.h
 *
 *	NOTES
 *		This is used both by the backend and by frontends, but should not be
 *		included by libpq client programs.  In particular, a libpq client
 *		should not assume that the encoding IDs used by the version of libpq
 *		it's linked to match up with the IDs declared here.
 *
 *-------------------------------------------------------------------------
 */
#ifndef __PG_WCHAR_H__
#define __PG_WCHAR_H__

/*
 * The pg_wchar type
 */
typedef unsigned int pg_wchar;

/*
 * Maximum byte length of multibyte characters in any backend encoding
 */
#define MAX_MULTIBYTE_CHAR_LEN	4

bool pg_utf8_islegal(const unsigned char *source, int length);


#endif /* __PG_WCHAR_H__ */