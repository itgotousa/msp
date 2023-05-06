#ifndef __SVG_H__
#define __SVG_H__

#include "pgcore.h"

/* SVG object type */
#define SO_TYPE_INVALID     0
#define SO_TYPE_GRAPHIC     1
#define SO_TYPE_TEXT        2
#define SO_TYPE_IMAGE       3

#define SO_HINT_MATRIX      0x04

typedef struct RenderNodeData *RenderNode;

typedef struct RenderNodeData
{
    unsigned int    flag;
    RenderNode      next;
    MemoryContext   ctx;
    void*           data;
    unsigned int    len;
    float           x;
    float           y;
    float           a;
    float           b;
    float           c;
    float           d;
    float           e;
    float           f;
} RenderNodeData;

#endif /* __SVG_H__ */