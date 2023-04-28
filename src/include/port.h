#ifndef __PORT_H__
#define __PORT_H__

#if !HAVE_DECL_STRLCPY
extern size_t strlcpy(char *dst, const char *src, size_t siz);
#endif


#endif /* __PORT_H__ */