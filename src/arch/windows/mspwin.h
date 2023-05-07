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

#define MAX_BUF_LEN   (512 * 1024 * 1024) /* the max length of supported file is 512M */

typedef enum 
{
    fileUnKnown = 0,
    fileMD,
    fileSVG,
    filePNG
} fileType;

#define UI_NOTIFY_MONITOR	0x01
#define UI_NOTIFY_FILEOPN	0x02

typedef struct D2DContextData
{
    ID2D1Factory*       pFactory;
    IDWriteFactory*     pDWriteFactory;
    IDWriteTextFormat*  pTextFormat;
    CRITICAL_SECTION    cs;
    RenderNode          pData;
} D2DContextData;


extern TCHAR   g_filepath[MAX_PATH + 1];
extern BOOL    g_fileloaded;
extern BOOL    g_monitor;
extern char   *g_buffer;
extern HANDLE  g_kaSignal[2];
extern LONG    g_threadCount;

extern  D2DContextData d2d;

#endif /* __MSPWIN_H__ */

