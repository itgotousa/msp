// msp.h
#ifndef __MSPWIN_H__
#define __MSPWIN_H__

#define WM_UI_NOTIFY      (WM_USER + 102)
#define WM_UI_NOTIFYEX    (WM_USER + 103)

extern TCHAR *g_file;
extern BOOL g_fileloaded;
extern BOOL g_monitor;

extern  ID2D1Factory    *g_pFactory;

#endif /* __MSPWIN_H__ */

