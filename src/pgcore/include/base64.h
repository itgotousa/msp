/*
 * base64.h
 *	  Encoding and decoding routines for base64 without whitespace
 *	  support.
 *
 * Portions Copyright (c) 2001-2022, PostgreSQL Global Development Group
 *
 * src/include/common/base64.h
 */
#ifndef __BASE64_H__
#define __BASE64_H__

/* base 64 */
extern int	pg_b64_encode(const char *src, int len, char *dst, int dstlen);
extern int	pg_b64_decode(const char *src, int len, char *dst, int dstlen);
extern int	pg_b64_enc_len(int srclen);
extern int	pg_b64_dec_len(int srclen);

#endif  /* __BASE64_H__ */
