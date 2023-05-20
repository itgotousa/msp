#ifndef __SVG_H__
#define __SVG_H__

#include "pgcore.h"

/* SVG object type */
#define SO_TYPE_INVALID     0
#define SO_TYPE_GRAPHIC     1
#define SO_TYPE_TEXT        2
#define SO_TYPE_IMAGE       3

#define SO_HINT_MATRIX      0x04
#define SO_HINT_BITMAP      0x08

typedef struct RenderNodeData *RenderNode;

typedef struct RenderNodeData
{
    uint32_t        flag;
    RenderNode      next;
    uint8_t         type;
    void*           data;
    uint32_t        length;
    uint32_t        width;
    uint32_t        height;
    float           x, y;
    float           a,b,c,d,e,f;
} RenderNodeData;

#endif /* __SVG_H__ */
