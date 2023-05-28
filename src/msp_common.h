#ifndef __MSP_H__
#define __MSP_H__

typedef unsigned int	U32, *P_U32, **PP_U32;
typedef signed int		S32, *P_S32, **PP_S32;
typedef unsigned short	U16, *P_U16, **PP_U16;
typedef signed short	S16, *P_S16, **PP_S16;
typedef unsigned char	U8, *P_U8, **PP_U8;
typedef signed char		S8, *P_S8, **PP_S8;

/* MSP object type */
#define MSP_TYPE_INVALID    0
#define MSP_TYPE_TEXT       1
#define MSP_TYPE_IMAGE      2
#define MSP_TYPE_GRAPHIC    3

#define MSP_HINT_PNG        0x01
#define MSP_HINT_GIF        0x02
#define MSP_HINT_BMP        0x04

#define MSP_LOGFILE     ("msp.log")

typedef struct RenderNodeData *RenderNode;

typedef struct RenderNodeData
{
    U32         flag;
    RenderNode  next;
    U8          type;
    void*       text;
    U32         text_length; /* in bytes */
    void*       image;
    U32         image_length; /* in bytes */
    U32         width;
    U32         height;
    U32         top;
#if 0
    float       x, y;
    float       a,b,c,d,e,f;
#endif
} RenderNodeData;

typedef struct RenderRootData
{
    U32         count;   /* how many render nodes in this list? */
    U32         width;
    U32         height;
    RenderNode  node;
} RenderRootData;

typedef struct RenderRootData* RenderRoot;

#endif /* __MSP_H__ */
