// msp.h
#ifndef __MSPWIN_H__
#define __MSPWIN_H__

#define WM_UI_NOTIFY      (WM_USER + 102)
#define WM_UI_NOTIFYEX    (WM_USER + 103)

/* MSP_ALIGN() is only to be used to align on a power of 2 boundary */
#define MSP_ALIGN(size, boundary) (((size) + ((boundary) - 1)) & ~((boundary) - 1))
/* Default alignment */
#define MSP_ALIGN_DEFAULT(size)	MSP_ALIGN(size, 8)
#define MSP_ALIGN_PAGE(size)	MSP_ALIGN(size, 1<<12)

#define MAX_BUF_LEN   (128 * 1024 * 1024) /* the max length of supported file is 128 MB */

typedef enum 
{
    fileUnKnown = 0,
    fileMD,
    fileSVG,
    filePNG,
    fileJPG,
    fileBMP,
    fileGIF
} fileType;

#define UI_NOTIFY_MONITOR	0x01
#define UI_NOTIFY_FILEOPEN	0x02
#define UI_NOTIFY_FILEFAIL	0x03

#define ANIMATION_TIMER_ID  123

#define SAFERELEASE(p)  do { if(NULL != (p)) { (p)->Release(); (p) = NULL; } } while(0)

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
    RenderNodeData          std;
    Animation               am;
    IWICStream*             pStream;
    IWICBitmapDecoder*      pDecoder;
    IWICBitmapFrameDecode*  pFrame;
    IWICFormatConverter*    pConverter;
    ID2D1PathGeometry*      pGeometry;
    ID2D1StrokeStyle*       pStrokeStyle;
} D2DRenderNodeData;

typedef struct D2DRenderNodeData *D2DRenderNode;

class FontResources;

typedef struct D2DContextData
{
    ID2D1Factory*       pFactory;
    IDWriteFactory5*     pDWriteFactory;
    IWICImagingFactory* pIWICFactory;
    IDWriteTextFormat*  pTextFormat;
    fileType            ft;
    CRITICAL_SECTION    cs;
    D2DRenderNode       pData;
    D2DRenderNode       pDataDefault;
    FontResources*      pFontResource;
    BYTE*               fontData;
    DWORD               fontDataSize;
    IDWriteFontSet*     fontSet;
    IDWriteInMemoryFontFileLoader* fontLoader;
} D2DContextData;

typedef struct
{
    HWND    hWnd;
    TCHAR*  pfilePath;
} ThreadParam;

extern ThreadParam  tp;
extern TCHAR   g_filepath[MAX_PATH + 1];
extern BOOL    g_monitor;
extern HANDLE  g_kaSignal[2];
extern LONG    g_threadCount;

extern  D2DContextData d2d;

extern void ReleaseD2DResource(D2DRenderNode n);

void render_svg_logo(const char*);
unsigned WINAPI open_mspfile_thread(LPVOID lpData);

#endif /* __MSPWIN_H__ */

