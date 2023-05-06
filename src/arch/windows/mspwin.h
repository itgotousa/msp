// msp.h
#ifndef __MSPWIN_H__
#define __MSPWIN_H__

#define WM_UI_NOTIFY      (WM_USER + 102)
#define WM_UI_NOTIFYEX    (WM_USER + 103)

typedef struct D2DContextData
{
    ID2D1Factory*       pFactory;
    IDWriteFactory*     pDWriteFactory;
    IDWriteTextFormat*  pTextFormat;    
} D2DContextData;


extern TCHAR   g_filepath[MAX_PATH + 1];
extern BOOL    g_fileloaded;
extern BOOL    g_monitor;
extern char   *g_buffer;
extern HANDLE  g_kaSignal[2];
extern LONG    g_threadCount;

extern  D2DContextData d2d;
extern  RenderNode	g_renderdata;

#endif /* __MSPWIN_H__ */

