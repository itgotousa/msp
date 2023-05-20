// msp.cpp : main source file for msp.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlscrl.h>
#include <memory>

#include "msp.h"
#include "pgcore.h"
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
ThemeData		tm	= { 0 };
D2DContextData 	d2d = { 0 };

static const char svg_logo[] =
"<svg xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:cc=\"http://creativecommons.org/ns#\"\n"
"xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
"xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"320\" height=\"200\">\n"
"<g transform=\"translate(-40,-100) scale(0.5 0.5)\"> <g transform=\"matrix(3.4444625,0,0,3.4444625,-29.767859,-410.21148)\">\n"
"<g transform=\"matrix(1.25,0,0,-1.25,42.85675,232.39208)\" id=\"g3005\" style=\"fill:#001e5a;fill-opacity:1\">\n"
"<path d=\"m 0,0 c 0.306,2.457 0.409,4.699 0.409,8.064 l 0,16.445 c 0,4.059 -0.103,5.981 -0.409,8.062 l 10.143,0 0,-2.241\n"
"c 0,-0.215 0,-0.215 -0.052,-0.749 l 0,-0.266 0,-0.267 c 2.613,2.99 5.737,4.379 9.784,4.379 2.665,0 4.868,-0.641 6.557,\n"
"-1.923 0.922,-0.695 1.436,-1.336 2.101,-2.671 3.175,3.257 6.301,4.646 10.758,4.646 7.325,0 11.065,-4.164 11.065,-12.228\n"
"l 0,-13.241 c 0,-3.311 0.103,-5.5 0.41,-8.01 L 40.111,0 c 0.307,2.51 0.41,4.433 0.41,8.064 l 0,10.999 c 0,4.004 -1.126,5.553\n"
"-4.047,5.553 -2.614,0 -4.969,-2.135 -6.249,-5.714 l 0,-10.892 c 0,-3.203 0.101,-5.445 0.408,-8.01 L 19.978,0 c 0.307,2.404\n"
"0.41,4.646 0.41,8.064 l 0,10.999 c 0,2.243 -0.205,3.258 -0.871,4.165 -0.564,0.854 -1.589,1.334 -2.818,1.334 -2.766,0 -5.122,\n"
"-2.081 -6.556,-5.712 l 0,-10.786 C 10.143,4.699 10.245,2.617 10.552,0 L 0,0 z\"\n"
"style=\"fill:#001e5a;fill-opacity:1;fill-rule:nonzero;stroke:none\" /></g>\n"
"<g transform=\"matrix(1.25,0,0,-1.25,173.46425,220.31058)\" style=\"fill:#001e5a;fill-opacity:1\">\n"
"<path d=\"m 0,0 c 1.436,1.44 2.255,3.898 2.255,6.941 0,5.447 -2.765,8.596 -7.581,8.596 -4.253,0 -7.736,-3.897 -7.736,-8.756\n"
"0,-5.02 3.433,-8.81 8.044,-8.81 1.946,0 3.687,0.693 5.018,2.029 m -23.256,-19.597 c 0.256,2.296 0.409,4.966 0.409,7.798 l\n"
"0,26.055 c 0,3.471 -0.103,5.661 -0.409,8.383 l 10.091,0 0,-1.816 c 0,-0.32 -0.051,-0.748 -0.051,-1.014 3.073,2.668 6.25,3.791\n"
"10.503,3.791 4.507,0 8.196,-1.388 10.859,-4.111 2.716,-2.83 4.098,-7.049 4.098,-12.441 0,-5.446 -1.537,-9.878 -4.455,-12.921\n"
"-2.614,-2.724 -6.353,-4.218 -10.553,-4.218 -2.41,0 -5.022,0.534 -6.917,1.387 -1.332,0.588 -2.1,1.174 -3.535,2.457 0,-0.48 0,\n"
"-1.175 0.051,-1.924 l 0,-3.683 c 0,-2.991 0.103,-5.447 0.41,-7.743 l -10.501,0 z\"\n"
"style=\"fill:#001e5a;fill-opacity:1;fill-rule:nonzero;stroke:none\" /></g>\n"
"<g transform=\"matrix(1.25,0,0,-1.25,118.4105,220.00607)\" style=\"fill:#001e5a;fill-opacity:1\">\n"
"<path d=\"m 0,0 c 0.541,-2.9 2.382,-4.182 6.013,-4.182 3.25,0 5.093,0.948 5.093,2.677 0,0.836 -0.434,1.561 -1.192,2.008\n"
"C 9.155,0.948 8.451,1.171 5.471,1.785 1.354,2.62 -0.76,3.178 -2.548,3.793 c -2.6,1.003 -4.387,2.285 -5.416,3.959 -0.868,\n"
"1.505 -1.355,3.401 -1.355,5.297 0,6.691 5.472,10.762 14.52,10.762 5.148,0 8.992,-1.17 11.538,-3.568 1.789,-1.617 2.655,\n"
"-3.234 3.523,-6.357 L 9.805,12.714 c -0.271,2.287 -1.626,3.291 -4.551,3.291 -2.817,0 -4.551,-0.948 -4.551,-2.398 0,-1.505\n"
"1.138,-2.064 6.501,-3.29 5.31,-1.172 6.989,-1.672 8.886,-2.677 3.521,-1.785 5.202,-4.517 5.202,-8.532 0,-2.955 -0.977,-5.631\n"
"-2.764,-7.416 -2.601,-2.678 -7.043,-4.072 -12.84,-4.072 -6.827,0 -11.649,1.896 -14.249,5.577 -1.083,1.561 -1.625,2.899 -2.167,\n"
"5.575 L 0,0 z\" style=\"fill:#001e5a;fill-opacity:1;fill-rule:nonzero;stroke:none\"/></g>\n"
"<g transform=\"matrix(1.25,0,0,-1.25,92.19475,244.48932)\" style=\"fill:#001e5a;fill-opacity:1\">\n"
"<path d=\"M 0,0 C 0,0 46.273,-34.327 83.966,16.245 83.966,16.245 51.953,-22.544 0,0\"\n"
"style=\"fill:#001e5a;fill-opacity:1;fill-rule:nonzero;stroke:none\" /></g></g></g></svg>\n";

#if 0
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
#endif

CAppModule _Module;

// Struct for in-memory, raw font data.
struct MemoryFontInfo
{
	const BYTE* fontData;
	DWORD		fontDataSize;
};

class FontResources : public IUnknown
{
public:
    FontResources() {}

    // IUnknown interface
    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement(&m_refCount);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void **object)
    {
        *object = nullptr;
        if (riid == __uuidof(IUnknown))
        {
            AddRef();
            *object = this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        ULONG newCount = InterlockedDecrement(&m_refCount);
        if (newCount == 0)
            delete this;
        return newCount;
    }
	void AttachFontResource(BYTE* data, DWORD size)
	{
		m_fontData = data; m_fontDataSize = size;
	}
private:
	//std::vector<MemoryFontInfo> m_appFontResources;
	BYTE*	m_fontData;
	DWORD	m_fontDataSize;
	ULONG	m_refCount = 0;
};

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
		rc.right = desktop.right >> 1;
		rc.bottom = desktop.bottom - 300;
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
		
//		DWORD tid = GetThreadId(hThread);
		
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
		SAFERELEASE(n->pConverter);
		SAFERELEASE(n->pFrame);
		SAFERELEASE(n->pDecoder);
		SAFERELEASE(n->pStream);
		SAFERELEASE(n->pGeometry);
		SAFERELEASE(n->pStrokeStyle);
		SAFERELEASE(n->pTextLayout);
//		SAFERELEASE();
//		SAFERELEASE();
        n = (D2DRenderNode)n->std.next;
    }
	MemoryContextDelete(mcxt);
}

static void InitThemeData(Theme t)
{
	if (NULL != t)
	{
		t->width = 600;  /* pixel */
		t->top_margin = 10;
		t->text_font = L"Arial";
		t->h1_font = t->text_font;
		t->h2_font = t->text_font;
		t->h3_font = t->text_font;
		t->h4_font = t->text_font;
		t->h5_font = t->text_font;
		t->h6_font = t->text_font;

		t->text_size = 16;
		t->h6_size = 20;
		t->h5_size = 22;
		t->h4_size = 26;
		t->h3_size = 28;
		t->h2_size = 32;
		t->h1_size = 36;
	}
}

static int InitD2D(HINSTANCE hInstance)
{
	HRESULT hr = S_OK;

	ZeroMemory(&d2d, sizeof(D2DContextData));
	
	InitializeCriticalSection(&(d2d.cs));

	/* Create the factories for D2D, DWrite, and WIC. */
	// Create D2D factory
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &(d2d.pFactory));
	if(FAILED(hr)) return -1;
	// Create DWrite factory
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&(d2d.pDWriteFactory)));
	if(FAILED(hr)) return -1;
	// Create WIC factory to load images.
	hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&(d2d.pIWICFactory)));	
	if(FAILED(hr)) return -1;

	/* load the build-in font file/*.ttf */
	HRSRC  res = FindResource(hInstance, MAKEINTRESOURCE(IDR_DEFAULT_FONT), RT_RCDATA);
	if (NULL == res) return 0;

	HGLOBAL res_handle = LoadResource(hInstance, res);
	if (NULL == res_handle) return 0;

	BYTE* data = (BYTE*)LockResource(res_handle);
	DWORD size = SizeofResource(hInstance, res);

	d2d.fontResource = new FontResources();
	if (nullptr == d2d.fontResource) return 0;
	d2d.fontResource->AttachFontResource(data, size);

	hr = d2d.pDWriteFactory->CreateInMemoryFontFileLoader(&(d2d.fontLoader));
	if (FAILED(hr))
	{
		delete d2d.fontResource;
		d2d.fontResource = nullptr;
		return 0;
	}
	hr = d2d.pDWriteFactory->RegisterFontFileLoader(d2d.fontLoader);
	if (FAILED(hr))
	{
		delete d2d.fontResource;
		d2d.fontResource = nullptr;
		return 0;
	}
	IDWriteFontFile* fontFileReference;
	hr = d2d.fontLoader->CreateInMemoryFontFileReference(d2d.pDWriteFactory, 
							data, 
							size, 
							d2d.fontResource,
							&fontFileReference);
	if (FAILED(hr))
	{
		delete d2d.fontResource;
		d2d.fontResource = nullptr;
		return 0;
	}
	IDWriteFontSetBuilder1* fb;
	hr = d2d.pDWriteFactory->CreateFontSetBuilder(&fb);
	if (FAILED(hr))
	{
		delete d2d.fontResource;
		d2d.fontResource = nullptr;
		SAFERELEASE(fontFileReference);
		return 0;
	}
	hr = fb->AddFontFile(fontFileReference);
	if (FAILED(hr))
	{
		delete d2d.fontResource;
		d2d.fontResource = nullptr;
		SAFERELEASE(fb);
		SAFERELEASE(fontFileReference);
		return 0;
	}
	IDWriteFontSet* fs;
	hr = fb->CreateFontSet(&fs);
	if (FAILED(hr))
	{
		delete d2d.fontResource;
		d2d.fontResource = nullptr;
		SAFERELEASE(fb);
		SAFERELEASE(fontFileReference);
		return 0;
	}
	IDWriteFontCollection1* fc;
	hr = d2d.pDWriteFactory->CreateFontCollectionFromFontSet(fs, &fc);
	if (FAILED(hr))
	{
		delete d2d.fontResource;
		d2d.fontResource = nullptr;
		SAFERELEASE(fs);
		SAFERELEASE(fb);
		SAFERELEASE(fontFileReference);
		return 0;
	}

#if 0
	UINT32 cnt = fc1->GetFontFamilyCount();
	IDWriteFontFamily* ffm;
	hr = fc1->GetFontFamily(0, &ffm);
	if (FAILED(hr))
	{
		delete d2d.fontResource;
		d2d.fontResource = nullptr;
		SAFERELEASE(fc1);
		SAFERELEASE(pFontSet);
		SAFERELEASE(fb);
		SAFERELEASE(fontFileReference);
		return 0;
	}
	UINT32 index = 0;
	BOOL exists = false;
	IDWriteLocalizedStrings* pFamilyNames = NULL;
	hr = ffm->GetFamilyNames(&pFamilyNames);
	if (FAILED(hr))
	{
		delete d2d.fontResource;
		d2d.fontResource = nullptr;
		SAFERELEASE(ffm);
		SAFERELEASE(fc1);
		SAFERELEASE(pFontSet);
		SAFERELEASE(fb);
		SAFERELEASE(fontFileReference);
		return 0;
	}
	wchar_t name[256] = { 0 };
	pFamilyNames->GetString(0, name, 255);
#endif

	hr = d2d.pDWriteFactory->CreateTextFormat(MSP_BUILDIN_FONT, fc,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			tm.text_size,
			L"",
			&(d2d.pTextFormat));

	if (FAILED(hr))
	{
		delete d2d.fontResource;
		d2d.fontResource = nullptr;
		d2d.pDWriteFactory->CreateTextFormat(MSP_DEFAULT_FONT, NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			tm.text_size,
			L"",
			&(d2d.pTextFormat));
	}

	SAFERELEASE(fc);
	SAFERELEASE(fs);
	SAFERELEASE(fb);
	SAFERELEASE(fontFileReference);

	return 0;
}

static void ReleaseD2D(HINSTANCE hInstance)
{
	EnterCriticalSection(&(d2d.cs));
		ReleaseD2DResource(d2d.pData);
		ReleaseD2DResource(d2d.pData0);
	LeaveCriticalSection(&(d2d.cs));

	DeleteCriticalSection(&(d2d.cs));
	
	if(nullptr != d2d.fontResource)
	{
		delete d2d.fontResource;
		d2d.fontResource = nullptr;
	}
	if(NULL != d2d.fontLoader)
	{
		(d2d.pDWriteFactory)->UnregisterFontFileLoader(d2d.fontLoader);
		//SAFERELEASE(d2d.fontLoader);
	}

	SAFERELEASE(d2d.pTextFormat);
	SAFERELEASE(d2d.pDWriteFactory);
	SAFERELEASE(d2d.pIWICFactory);
	SAFERELEASE(d2d.pFactory);
}

static int InitInstance(HINSTANCE hInstance)
{
	int iRet = 0;
	g_threadCount = 0;
	g_monitor = FALSE;

	/* initialize the default theme */
	InitThemeData(&tm);

	iRet = InitD2D(hInstance);
	if(iRet < 0) return -1;

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

	ReleaseD2D(hInstance);

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

	int iRet = InitInstance(hInstance);

	if(0 != iRet) goto app_quit;
	// BLOCK: Run application
	{
		CMspThreadManager mgr;
		iRet = mgr.Run(lpstrCmdLine, nCmdShow);
	}

app_quit:
	ExitInstance(hInstance);

	_Module.Term();
	::CoUninitialize();

	return iRet;
}
