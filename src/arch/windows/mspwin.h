// msp.h
#ifndef __MSPWIN_H__
#define __MSPWIN_H__

#include "msp_common.h"

#define WM_UI_NOTIFY      (WM_USER + 102)
#define WM_UI_NOTIFYEX    (WM_USER + 103)

/* MSP_ALIGN() is only to be used to align on a power of 2 boundary */
#define MSP_ALIGN(size, boundary) (((size) + ((boundary) - 1)) & ~((boundary) - 1))
/* Default alignment */
#define MSP_ALIGN_DEFAULT(size)	MSP_ALIGN(size, 8)
#define MSP_ALIGN_PAGE(size)	MSP_ALIGN(size, 1<<12)

#define MAX_BUF_LEN   (32 * 1024 * 1024) /* the max length of supported file is 32 MB */

typedef enum 
{
    fileUnKnown = 0,
    fileMD,
    fileSVG,
    filePNG,
    fileGIF
} fileType;

#define UI_NOTIFY_MONITOR	0x01
#define UI_NOTIFY_FILEOPEN	0x02
#define UI_NOTIFY_FILEFAIL	0x03

#define ANIMATION_TIMER_ID  123

#define MSP_PI       3.14159265358979323846   // pi

#define SAFERELEASE(p)  do { if(NULL != (p)) { (p)->Release(); (p) = NULL; } } while(0)

#define MSP_DEFAULT_FONT    (L"Arial")
//#define MSP_BUILDIN_FONT    (L"OPlusSans 3.0")

#define MAX_FONT_FAMILY_NAME		(LOCALE_NAME_MAX_LENGTH)
typedef struct ThemeData
{
    /* size */
    U16     top_margin;
    U16     bottom_margin;
    U16     width;
    U8      width_type; /* pixel/X or percentage/P */
    U8      svgpng_aligment; /* LCR : left, center, right */
    /* color in RGB format */
    U32     text_color;
    U32     text_bkgcolor;
    U32     bkg_color;
    U32     text_selectcolor;
    /* font type */
    TCHAR*  text_font;
    TCHAR*  h1_font;
    TCHAR*  h2_font;
    TCHAR*  h3_font;
    TCHAR*  h4_font;
    TCHAR*  h5_font;
    TCHAR*  h6_font;
    TCHAR*  block_font;
    /* font size */
    U16     text_size;
    U16     h1_size;
    U16     h2_size;
    U16     h3_size;
    U16     h4_size;
    U16     h5_size;
    U16     h6_size;
    U16     block_size;

    void*   fontHash; /* HTAB */
    TCHAR   fontDefault[MAX_FONT_FAMILY_NAME+1];

    IDWriteTextFormat* pblockTextFormat;
} ThemeData;

typedef struct ThemeData* Theme;

#define FONT_INDEX_KEYSIZE		 ((MAX_FONT_FAMILY_NAME<<1) + 2)
typedef struct
{
    U8      key[FONT_INDEX_KEYSIZE];
    void*   value;
} FontEnt;

typedef struct AnimationData
{
    UINT            cxImage;
    UINT            cyImage;
    UINT            cxImagePixel;
    UINT            cyImagePixel;
    UINT            totalLoopCount;
    UINT            loopNumber;
    UINT            frameCount;
    UINT            frameIndex;
    UINT            frameDelay;
    UINT            frameDisposal;
    BOOL            hasLoop;
    D2D1_RECT_F     framePosition;
    D2D1_COLOR_F    color_bkg;
} AnimationData;

typedef struct AnimationData *Animation;

typedef struct D2DRenderNodeData
{
    RenderNodeData      std;
    AnimationData       am;
    IDWriteTextLayout*  pTextLayout;
} D2DRenderNodeData;

typedef D2DRenderNodeData *D2DRenderNode;

class FontResources;

typedef struct D2DContextData
{
    CRITICAL_SECTION    cs;
    RenderRoot          pData;
    RenderRoot          pData0;
    RenderRoot          pDataDefault;

    ID2D1Factory*       pFactory;
    IDWriteFactory5*    pDWriteFactory;
    IDWriteTextFormat*  pDefaultTextFormat;
    IDWriteInMemoryFontFileLoader* fontLoader;
    FontResources* fontResource;
} D2DContextData;

typedef struct
{
    HWND        hWnd;
    wchar_t*    pfilePath;
} ThreadParam;

extern ThreadParam  tp;
extern TCHAR   g_filepath[MAX_PATH + 1];
extern BOOL    g_monitor;
extern HANDLE  g_kaSignal[2];
extern LONG    g_threadCount;

extern D2DContextData d2d;
extern ThemeData tm;
extern void ReleaseD2DResource(RenderRoot root);

void render_svg_logo(const char*);
unsigned WINAPI open_mspfile_thread(LPVOID lpData);

#endif /* __MSPWIN_H__ */

