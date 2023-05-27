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

ThreadParam		tp = {0};
LONG			g_threadCount = 0;
BOOL 			g_monitor = FALSE;
HANDLE			g_kaSignal[2] = {NULL, NULL};
TCHAR			g_filepath[MAX_PATH + 1] = { 0 };
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
	FontResources() { m_refCount = 0; }

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
	ULONG	m_refCount;
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
		SAFERELEASE(n->pTextLayout);
        n = (D2DRenderNode)n->std.next;
    }
	MemoryContextDelete(mcxt);
}

static int InitThemeData(Theme t)
{
	HRESULT hr = S_OK;
	if (NULL == t) return -1;

	t->width = 800;  /* pixel */
	t->width_type = 'X';
	t->top_margin = 10;

	t->bkg_color = 0xFFFFFF;
	t->text_bkgcolor = 0xFFFFFF;
	t->text_color = 0x000000;

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
	t->block_size = 13;

	if (NULL == t->fontHash)
	{
		HASHCTL		hash_ctl;
		hash_ctl.keysize = FONT_INDEX_KEYSIZE;
		hash_ctl.entrysize = sizeof(FontEnt);

		t->fontHash = hash_create("fontHash", 512, &hash_ctl, HASH_ELEM | HASH_BLOBS);
		if (NULL == t->fontHash) return -1;
	}

	hr = d2d.pDWriteFactory->CreateTextFormat(L"Courier New", NULL,
		DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		t->block_size, L"", &(t->pblockTextFormat));

	if (SUCCEEDED(hr)) return 0;

	return -1;
}

static HRESULT InitFontHashTable()
{
	HRESULT hr = S_OK;
	bool found;
	unsigned char fontName[FONT_INDEX_KEYSIZE] = { 0 };

	IDWriteFontCollection* pFontCollection = NULL;
	hr = d2d.pDWriteFactory->GetSystemFontCollection(&pFontCollection);
	if (SUCCEEDED(hr))
	{
		IDWriteFontFamily* pFontFamily = NULL;
		IDWriteLocalizedStrings* pFamilyNames = NULL;

		UINT32 familyCount = pFontCollection->GetFontFamilyCount();

		for (UINT32 i = 0; i < familyCount; ++i)
		{
			hr = pFontCollection->GetFontFamily(i, &pFontFamily);
			if (SUCCEEDED(hr))
			{
				hr = pFontFamily->GetFamilyNames(&pFamilyNames);
				if (SUCCEEDED(hr))
				{
					UINT32 index = 0;
					BOOL exists = false;

					wchar_t localeName[LOCALE_NAME_MAX_LENGTH];

					// Get the default locale for this user.
					int defaultLocaleSuccess = GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);

					// If the default locale is returned, find that locale name, otherwise use "en-us".
					if (defaultLocaleSuccess)
					{
						hr = pFamilyNames->FindLocaleName(localeName, &index, &exists);
					}
					if (SUCCEEDED(hr) && !exists) // if the above find did not find a match, retry with US English
					{
						hr = pFamilyNames->FindLocaleName(L"en-us", &index, &exists);
					}
					// If the specified locale doesn't exist, select the first on the list.
					if (!exists) index = 0;

					UINT32 length = 0;

					// Get the string length.
					hr = pFamilyNames->GetStringLength(index, &length);
					// Get the family name.
					if (SUCCEEDED(hr) && length < MAX_FONT_FAMILY_NAME)
					{
						ZeroMemory(fontName, FONT_INDEX_KEYSIZE);
						hr = pFamilyNames->GetString(index, (WCHAR*)fontName, length + 1);
						if (SUCCEEDED(hr))
						{
#if 0
							for (int i = 0; i < FONT_INDEX_KEYSIZE; i++)
							{
								fprintf(stdout, "%02X:", fontName[i]);
							}
							fprintf(stdout, "\n");

							hash_search((HTAB*)tm.fontHash, fontName, HASH_ENTER_NULL, &found);
#endif 
						}
					}
				}
			}
		}
	}

	return hr;
}

static HRESULT InitFont(HINSTANCE hInstance)
{
	HRESULT hr = S_OK;

	/* load the build-in font file/*.ttf */
	HRSRC  res = FindResource(hInstance, MAKEINTRESOURCE(IDR_DEFAULT_FONT), RT_RCDATA);
	if (NULL != res)
	{
		HGLOBAL res_handle = LoadResource(hInstance, res);
		if (NULL != res_handle)
		{
			BYTE* data = (BYTE*)LockResource(res_handle);
			DWORD size = SizeofResource(hInstance, res);

			d2d.fontResource = new FontResources();
			if (nullptr != d2d.fontResource)
			{
				d2d.fontResource->AttachFontResource(data, size);
				hr = d2d.pDWriteFactory->CreateInMemoryFontFileLoader(&(d2d.fontLoader));
				if (SUCCEEDED(hr))
				{
					hr = d2d.pDWriteFactory->RegisterFontFileLoader(d2d.fontLoader);
					if (SUCCEEDED(hr))
					{
						IDWriteFontFile* fr;
						hr = d2d.fontLoader->CreateInMemoryFontFileReference(d2d.pDWriteFactory,
							data, size, d2d.fontResource, &fr);
						if (SUCCEEDED(hr))
						{
							IDWriteFontSetBuilder1* fb;
							hr = d2d.pDWriteFactory->CreateFontSetBuilder(&fb);
							if (SUCCEEDED(hr))
							{
								hr = fb->AddFontFile(fr);
								if (SUCCEEDED(hr))
								{
									IDWriteFontSet* fs;
									hr = fb->CreateFontSet(&fs);
									if (SUCCEEDED(hr))
									{
										IDWriteFontCollection1* fc;
										hr = d2d.pDWriteFactory->CreateFontCollectionFromFontSet(fs, &fc);
										if (SUCCEEDED(hr))
										{
											IDWriteFontFamily* ffm;
											hr = fc->GetFontFamily(0, &ffm);
											if (SUCCEEDED(hr))
											{
												IDWriteLocalizedStrings* pFamilyNames = NULL;
												hr = ffm->GetFamilyNames(&pFamilyNames);
												if (SUCCEEDED(hr))
												{
													hr = pFamilyNames->GetString(0, tm.fontDefault, MAX_FONT_FAMILY_NAME);
													if (SUCCEEDED(hr))
													{
														hr = d2d.pDWriteFactory->CreateTextFormat(tm.fontDefault, fc,
																DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
																19, L"", &(d2d.pDefaultTextFormat));
														SAFERELEASE(pFamilyNames);
													}
												}
												SAFERELEASE(pFamilyNames);
											}

											if (FAILED(hr))
											{
												hr = d2d.pDWriteFactory->CreateTextFormat(MSP_DEFAULT_FONT, NULL,
														DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
														19, L"", &(d2d.pDefaultTextFormat));
											}
											SAFERELEASE(fc);
										}
										SAFERELEASE(fs);
									}
								}
								SAFERELEASE(fb);
							}
							SAFERELEASE(fr);
						}
					}
				}
			}
		}
	}

	return hr;
}

static int InitD2D(HINSTANCE hInstance)
{
	HRESULT hr = S_OK;

	ZeroMemory(&d2d, sizeof(D2DContextData));
	
	InitializeCriticalSection(&(d2d.cs));

	// Create D2D factory
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &(d2d.pFactory));
	if(FAILED(hr)) return -1;

	// Create DWrite factory
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&(d2d.pDWriteFactory)));
	if (FAILED(hr)) return -1;

	hr = InitFont(hInstance);
	if (FAILED(hr)) return -1;

	//hr = InitFontHashTable();
	//if (FAILED(hr)) return -1;

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
	if(nullptr != d2d.fontLoader)
	{
		(d2d.pDWriteFactory)->UnregisterFontFileLoader(d2d.fontLoader);
		//SAFERELEASE(d2d.fontLoader);
	}

	SAFERELEASE(d2d.pDefaultTextFormat);
	SAFERELEASE(d2d.pDWriteFactory);
	SAFERELEASE(d2d.pFactory);

}

static int InitInstance(HINSTANCE hInstance)
{
	int iRet = 0;
	g_threadCount = 0;
	g_monitor = FALSE;

	freopen(MSP_LOGFILE, "a+", stdout);
	freopen(MSP_LOGFILE, "a+", stderr);

	MemoryContextInit();
	/* redirect the stdout and stderr to the logfile */

	iRet = InitD2D(hInstance);
	if(iRet < 0) return -1;

	/* initialize the default theme */
	iRet = InitThemeData(&tm);
	if (iRet < 0) return -1;

	g_kaSignal[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(NULL == g_kaSignal[0])
	{
		return -1;
	}

	ZeroMemory(g_filepath, MAX_PATH + 1);

	render_svg_logo(svg_logo);

	return 0;
}

static int ExitInstance(HINSTANCE hInstance)
{
	int tries = 20;
	SetEvent(g_kaSignal[0]); /* wakeup the monitoring thread */

	ReleaseD2D(hInstance);
	if (NULL != tm.fontHash)
	{
		hash_destroy((HTAB*)tm.fontHash);
		tm.fontHash = NULL;
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
