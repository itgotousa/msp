// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//  are changed infrequently
//

#pragma once

// Change these values to use different versions
//#define WINVER		0x0601
//#define _WIN32_WINNT	0x0601
#define WINVER		    0x0A00
#define _WIN32_WINNT	0x0A00
#define _WIN32_IE	0x0700
#define _RICHEDIT_VER	0x0500

#ifndef _UNICODE
#define _UNICODE
#define UNICODE
#endif 

#define NOMINMAX

#include <iostream>
#include <sstream>
#include <vector>
#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <processthreadsapi.h>
#include <WinBase.h>
#include <libloaderapi.h>
#include <VersionHelpers.h>
// C RunTime Header Files
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <Shellapi.h>
#include <Strsafe.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite_3.h>
#include <dwrite.h>
#include <wincodec.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#include <Wincodec.h> // Windows Imaging Component (WIC)


#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
