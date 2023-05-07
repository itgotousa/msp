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

void ReleaseD2DResource(D2DRenderNode n)
{
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
