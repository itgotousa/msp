// msp.cpp : main source file for msp.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlscrl.h>
#include <memory>

#include "svg.h"

#include "resource.h"
#include "mspwin.h"
#include "View.h"
#include "aboutdlg.h"
#include "MainFrm.h"

ThreadParam  tp = {0};
LONG	g_threadCount = 0;
BOOL 	g_monitor = FALSE;
HANDLE  g_kaSignal[2] = {NULL, NULL};
TCHAR   g_filepath[MAX_PATH + 1] = { 0 };

static const char svg_logo[] =
"<svg version=\"1.1\" id=\"Layer_1\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
"x=\"0px\" y=\"0px\" viewBox=\"0 0 291.5 280\" style=\"enable-background:new 0 0 291.5 280;\" xml:space=\"preserve\">\n"
"<g transform=\"translate(0, 0) scale(2 2)\">\n"
"<path fill=\"#D3D800\" d=\"M47.5,15.6L47.5,15.6c1.6-0.2,4.1-0.9,6.1-3.6c0,0-3.5-1.2-7.4,3.6c0,0,0,0,0.1,0c-0.1,0.1-0.1,0.1-0.2,0.2\n"
"c0.7-1.4,1.5-3.9,0.1-7.1c0,0-2.7,2.3-0.9,7.9c-5.1,4.7-10,4.3-10.1,4.3l-0.2,0l-0.1,0.9l0.2,0c0,0,0.2,0,0.4,0\n"
"c1.2,0,4.9-0.3,9-3.4c0.9,0.6,3,1.3,6.5,0c0,0-2.1-1.6-5.2-1C46.4,16.7,46.9,16.2,47.5,15.6\"/>\n"
"<path fill=\"#003D70\" d=\"M83.8,56.3c1.4,0,2.8-0.2,3.9-0.5c1.2-0.3,2.2-0.8,3-1.4c0.8-0.6,1.4-1.3,1.8-2.2c0.4-0.9,0.6-1.8,0.6-2.9\n"
"v-0.1c0-1-0.2-1.9-0.6-2.6c-0.4-0.7-1-1.3-1.8-1.8c-0.8-0.5-1.8-1-2.9-1.3c-1.2-0.4-2.4-0.7-3.9-0.9c-0.6-0.1-1.1-0.2-1.5-0.3\n"
"c-0.4-0.1-0.8-0.2-1-0.4c-0.3-0.2-0.5-0.3-0.7-0.5c-0.1-0.2-0.2-0.5-0.2-0.7v-0.1c0-0.3,0.1-0.8,0.7-1.1\n"
"c0.4-0.2,0.9-0.3,1.5-0.3 l0.7,0h9.1v-4.8l-9.7,0c-1.5,0-2.8,0.2-3.9,0.5\n"
"c-1.1,0.4-2.1,0.9-2.8,1.5c-0.7,0.6-1.3,1.4-1.7,2.2c-0.4,0.8-0.6,1.8-0.6,2.7v0.1\n"
"c0,1.1,0.2,2,0.7,2.7c0.5,0.7,1.1,1.3,1.9,1.8c0.8,0.5,1.8,0.9,2.9,1.2c1.1,0.3,2.3,0.6,3.6,0.9c1.3,0.2,2.1,0.5,2.6,0.7\n"
"c0.8,0.4,1,0.9,1,1.3V50c0,0.5-0.3,0.9-0.7,1.2c-0.4,0.2-0.9,0.3-1.6,0.3l-0.6,0v0h-9.2v4.8l9.1,0C83.7,56.3,83.7,56.3,83.8,56.3\"/>\n"
"<path class=\"st1\" fill=\"#003D70\" d=\"M101.6,39.3h2.7c1.2,0,2.1,0.3,2.7,0.8c0.7,0.5,1.1,1.3,1.1,2.3v0.1c0,1-0.4,1.8-1.1,2.4\n"
"c-0.7,0.5-1.6,0.7-2.7,0.7h-2.6V39.3z M104.7,50c1.5,0,2.9-0.2,4.1-0.5c1.2-0.3,2.3-0.9,3.1-1.5c0.9-0.7,1.5-1.5,2-2.5\n"
"c0.5-1,0.7-2.2,0.7-3.4v-0.1c0-1.3-0.2-2.4-0.7-3.4c-0.5-1-1.1-1.8-2-2.4c-0.8-0.6-1.9-1.1-3.1-1.4c-1.2-0.3-2.6-0.5-4.1-0.5\n"
"H95v22h1c3.1,0,5.6-2.5,5.6-5.6l0-0.7l0.8,0H104.7z\"/>\n"
"<path fill=\"#003D70\" d=\"M72,56.3V41.2c0-3.8-3.1-6.9-6.9-6.9h-0.3l-5.3,8.6l-5.3-8.6h-7.2v15.1c0,3.6,2.8,6.6,6.4,6.9l0.1-12.8l6,9.2\n"
"l6-9.2v12.8H72z\"/>\n"
"<path fill=\"#D3D800\" d=\"M21.4,27.2c-12.7-4-17.2-8.7-19.7-16c1.6,11.3,7.8,19.3,20,23.9c6.8,2.6,14.4,6,15.6,14.1\n"
"c0.6-1.4,1.8-6,0.2-10.2C35.6,33.9,30.2,29.9,21.4,27.2\"/>\n"
"<path fill=\"#003D70\" d=\"M20.9,36.3c-9.2-3.6-15.5-9.5-18.5-17.4c1,6.4,4.6,19.6,18.5,24.8c7.7,2.9,12.6,6.4,14.5,11.3c1,2.5,1,5,0.7,7\n"
"C40.4,42.3,27.5,38.9,20.9,36.3\"/>\n"
"<path fill=\"#D3D800\" d=\"M12.9,78.8c1.3-1.5,5-5,10.3-6l0.4-0.1c3.2-0.7,7.2-2.6,9.2-5.8c1.2-1.7,3.6-5.6,1.5-11\n"
"c-1-2.5-2.8-4.7-5.3-6.6c1.6,2.6,3.3,7.3,0.2,12.7c-2,3.4-5.1,5-8.1,6.6C17,70.8,13.2,72.9,12.9,78.8\"/>\n"
"<path fill=\"#003D70\" d=\"M22.2,25.9c7,2.2,13.4,5.3,15.7,10.9c0.8-10.9-8.8-15.6-16.9-18C14.4,16.9,5.9,15,2.3,6.1\n"
"C2.2,5.9,2.1,5.7,2,5.4c-0.5-1.5-0.9-3-1-4.3C1.6,14.1,7.5,21.3,22.2,25.9\"/>\n"
"<path fill=\"#003D70\" d=\"M37.1,14.5c-0.5-0.4-1.5-2.1-1.2-2.6c0.3-0.5,1.9,0.3,2.4,0.7c0.5,0.4,0.5,1.9,0.5,1.9S37.5,14.9,37.1,14.5\n"
"M41.2,15.3c-1.1-5.7-5.3-7-10.8-7c-5.5,0-6.8-6.7-6.8-6.7c0,12,4.8,13.9,12.4,15.9c5.5,1.5,7.7,5.8,7.7,5.8\n"
"C46.5,19.6,42.5,17.2,41.2,15.3\"/></g></svg>\n";

D2DContextData 	d2d = {0};

CAppModule _Module;

class CMspThreadManager
{
public:
	// thread init param
	struct _RunData
	{
		LPTSTR lpstrCmdLine;
		int nCmdShow;
	};

	// thread proc
	static DWORD WINAPI RunThread(LPVOID lpData)
	{
		CMessageLoop theLoop;
		_Module.AddMessageLoop(&theLoop);

		_RunData* pData = (_RunData*)lpData;
		CMainFrame wndFrame;

		RECT desktop, rc;
		// Get a handle to the desktop window
		const HWND hDesktop = ::GetDesktopWindow();
		// Get the size of screen to the variable desktop
		::GetWindowRect(hDesktop, &desktop);
		rc.top = rc.left = 0;
		rc.right = desktop.right >> 2;
		rc.bottom = desktop.bottom - 200;
		if(wndFrame.CreateEx(NULL, rc) == NULL)
		{
			ATLTRACE(_T("Frame window creation failed!\n"));
			return 0;
		}

        wndFrame.CenterWindow();
		wndFrame.ShowWindow(pData->nCmdShow);
		delete pData;

		int nRet = theLoop.Run();

		_Module.RemoveMessageLoop();
		return nRet;
	}

	DWORD m_dwCount;
	HANDLE m_arrThreadHandles[MAXIMUM_WAIT_OBJECTS - 1];

	CMspThreadManager() : m_dwCount(0)
	{ }

// Operations
	DWORD AddThread(LPTSTR lpstrCmdLine, int nCmdShow)
	{
		if(m_dwCount == (MAXIMUM_WAIT_OBJECTS - 1))
		{
			::MessageBox(NULL, _T("ERROR: Cannot create ANY MORE threads!!!"), _T("msp"), MB_OK);
			return 0;
		}

		_RunData* pData = new _RunData;
		pData->lpstrCmdLine = lpstrCmdLine;
		pData->nCmdShow = nCmdShow;
		DWORD dwThreadID;
		HANDLE hThread = ::CreateThread(NULL, 0, RunThread, pData, 0, &dwThreadID);
		if(hThread == NULL)
		{
			::MessageBox(NULL, _T("ERROR: Cannot create thread!!!"), _T("msp"), MB_OK);
			return 0;
		}

		m_arrThreadHandles[m_dwCount] = hThread;
		m_dwCount++;
		return dwThreadID;
	}

	void RemoveThread(DWORD dwIndex)
	{
		::CloseHandle(m_arrThreadHandles[dwIndex]);
		if(dwIndex != (m_dwCount - 1))
			m_arrThreadHandles[dwIndex] = m_arrThreadHandles[m_dwCount - 1];
		m_dwCount--;
	}

	int Run(LPTSTR lpstrCmdLine, int nCmdShow)
	{
		MSG msg;
		// force message queue to be created
		::PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

		AddThread(lpstrCmdLine, nCmdShow);

		int nRet = m_dwCount;
		DWORD dwRet;
		while(m_dwCount > 0)
		{
			dwRet = ::MsgWaitForMultipleObjects(m_dwCount, m_arrThreadHandles, FALSE, INFINITE, QS_ALLINPUT);

			if(dwRet == 0xFFFFFFFF)
			{
				::MessageBox(NULL, _T("ERROR: Wait for multiple objects failed!!!"), _T("msp"), MB_OK);
			}
			else if(dwRet >= WAIT_OBJECT_0 && dwRet <= (WAIT_OBJECT_0 + m_dwCount - 1))
			{
				RemoveThread(dwRet - WAIT_OBJECT_0);
			}
			else if(dwRet == (WAIT_OBJECT_0 + m_dwCount))
			{
				if(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					if(msg.message == WM_USER)
						AddThread(_T(""), SW_SHOWNORMAL);
				}
			}
			else
			{
				::MessageBeep((UINT)-1);
			}
		}

		return nRet;
	}
};

/* release all D2D device independent resource and the whole memory pool */
void ReleaseD2DResource(D2DRenderNode n)
{
	if(NULL == n) return;

	MemoryContext mcxt = *(MemoryContext *) (((char *) n) - sizeof(void *)); 
    while(NULL != n)
    {
        if(NULL != n->pConverter)
        {
            n->pConverter->Release();
            n->pConverter = NULL;
        }
        if(NULL != n->pFrame)
        {
            n->pFrame->Release();
            n->pFrame = NULL;
        }
        if(NULL != n->pDecoder)
        {
            n->pDecoder->Release();
            n->pDecoder = NULL;
        }
        if(NULL != n->pStream)
        {
            n->pStream->Release();
            n->pStream = NULL;
        }
        if(NULL != n->pGeometry)
        {
            n->pGeometry->Release();
            n->pGeometry = NULL;
        }
        if(NULL != n->pStrokeStyle)
        {
            n->pStrokeStyle->Release();
            n->pStrokeStyle = NULL;
        }
        
        n = (D2DRenderNode)n->std.next;
    }

	MemoryContextDelete(mcxt);
}

static int InitInstance(HINSTANCE hInstance)
{
	g_threadCount = 0;
	g_monitor = FALSE;
	d2d.pFactory = NULL;
	d2d.ft = fileUnKnown;
	
	InitializeCriticalSection(&(d2d.cs));

	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &(d2d.pFactory));
	if(FAILED(hr)) return -1;
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&(d2d.pDWriteFactory)));
	if(FAILED(hr)) return -1;
	
	hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, 
						IID_IWICImagingFactory, reinterpret_cast<void**>(&(d2d.pIWICFactory)));	
	if(FAILED(hr)) return -1;

#if 0	
	hr = d2d.pDWriteFactory->CreateTextFormat(
            L"Courier New",                // Font family name.
            NULL,                       // Font collection (NULL sets it to use the system font collection).
            DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            25.0f,
            L"en-us",
            &(d2d.pTextFormat)
			);
	if(FAILED(hr)) return -1;

	hr = d2d.pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	if (SUCCEEDED(hr)) d2d.pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
#endif 

	g_kaSignal[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(NULL == g_kaSignal[0])
	{
		return -1;
	}

	ZeroMemory(g_filepath, MAX_PATH + 1);

	MemoryContextInit();
	//MessageBox(NULL, _T("MemoryContextInit!"), _T("MB_OK"), MB_OK);
	render_svg_logo(svg_logo);
	return 0;
}

static int ExitInstance(HINSTANCE hInstance)
{
	int tries = 20;
	SetEvent(g_kaSignal[0]); /* wakeup the monitoring thread */


	EnterCriticalSection(&(d2d.cs));
		ReleaseD2DResource(d2d.pData);
	LeaveCriticalSection(&(d2d.cs));

	DeleteCriticalSection(&(d2d.cs));
	
	if(NULL != d2d.pFactory) {
		(d2d.pFactory)->Release();
		(d2d.pFactory) = NULL;
	}

	if(NULL != d2d.pDWriteFactory) {
		(d2d.pDWriteFactory)->Release();
		(d2d.pDWriteFactory) = NULL;
	}

	if(NULL != d2d.pIWICFactory) {
		(d2d.pIWICFactory)->Release();
		(d2d.pIWICFactory) = NULL;
	}

	if(NULL != d2d.pTextFormat) {
		(d2d.pTextFormat)->Release();
		(d2d.pTextFormat) = NULL;
	}

	while(0 != g_threadCount)
	{
		Sleep(1000);
		tries--;
		if(0 >= tries) break;
	}

	//if(0 == g_threadCount) MessageBox(NULL, _T("All threads are quited!"), _T("MB_OK"), MB_OK);	

	return 0;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = InitInstance(hInstance);

	if(0 != nRet) goto app_quit;
	// BLOCK: Run application
	{
		CMspThreadManager mgr;
		nRet = mgr.Run(lpstrCmdLine, nCmdShow);
	}

app_quit:
	ExitInstance(hInstance);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
