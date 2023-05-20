#ifndef __MSP_H__
#define __MSP_H__

/* MSP object type */
#define MSP_TYPE_INVALID    0
#define MSP_TYPE_GRAPHIC    1
#define MSP_TYPE_TEXT       2
#define MSP_TYPE_IMAGE      3

#define MSP_HINT_PNG        0x01
#define MSP_HINT_GIF        0x02
#define MSP_HINT_BMP        0x04

typedef struct RenderNodeData *RenderNode;

typedef struct RenderNodeData
{
    unsigned int    flag;
    RenderNode      next;
    unsigned char   type;
    void*           data;
    unsigned int    length;
    unsigned int    width;
    unsigned int    height;
    float           x, y;
    float           a,b,c,d,e,f;
} RenderNodeData;

#endif /* __MSP_H__ */
