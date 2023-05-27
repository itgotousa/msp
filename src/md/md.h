#ifndef __MD_H__
#define __MD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "msp_common.h"

int md_parse_buffer(const U8* query_string, const U32 bytes);


#ifdef __cplusplus
}
#endif

#endif /* __MD_H__ */