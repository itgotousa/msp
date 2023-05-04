// View.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

/* BBK_ALIGN() is only to be used to align on a power of 2 boundary */
#define BBK_ALIGN(size, boundary) (((size) + ((boundary) - 1)) & ~((boundary) - 1))
/* Default alignment */
#define BBK_ALIGN_DEFAULT(size)	BBK_ALIGN(size, 8)
#define BBK_ALIGN_PAGE(size)	BBK_ALIGN(size, 1<<12)

#define MAX_BUF_LEN   (512 * 1024 * 1024) /* the max length of supported file is 512M */

typedef enum 
{
    fileUnKnown = 0,
    fileMD,
    fileSVG,
    filePNG
} fileType;

unsigned WINAPI open_msp_thread(LPVOID lpData)
{
	unsigned char *p;
	int   fd;
	unsigned int size, align_size, bytes, ret;
	fileType ft = fileUnKnown;

	HWND hWndUI = (HWND)lpData;
	ATLASSERT(::IsWindow(hWndUI));

	if (0 != _tsopen_s(&fd, g_filepath, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0)) 
	{
		return 0;
	}
	
	size = (unsigned int)_lseek(fd, 0, SEEK_END); /* get the video file size */
	if (size > MAX_BUF_LEN) 
	{
		_close(fd);
		return 0;
	}

	align_size = BBK_ALIGN_PAGE(size); /* 4K page align for performance */

	if(NULL != g_buffer) { free(g_buffer); }

	g_buffer = (unsigned char*)malloc(align_size);
	if (NULL == g_buffer)
	{
		_close(fd);
		return 0;
	}
	_lseek(fd, 0, SEEK_SET); /* go to the begin of the file */
	bytes = (unsigned int)_read(fd, g_buffer, size); /* read the entire file into g_buffer */
	if (bytes != size) /* read error, since bytes != size */
	{
		free(g_buffer);
		g_buffer = NULL;
		_close(fd);
		return 0;
	}
	_close(fd);

	/* check PNG magic number: 89 50 4e 47 0d 0a 1a 0a */
	p = g_buffer;
	if((0x89 == p[0]) && (0x50 == p[1]) && (0x4e == p[2]) && (0x47 == p[3]) && 
		(0x0d == p[4]) && (0x0a == p[5]) && (0x1a == p[6]) && (0x0a == p[7]))
	{
		ft = filePNG;
	}

	if(fileUnKnown == ft) 
	{
		free(g_buffer);
		g_buffer = NULL;
	}

	PostMessage(hWndUI, WM_UI_NOTIFY, 0, ft);

	return 0;
}

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

class CView : public CScrollWindowImpl<CView>
{
public:
	DECLARE_WND_CLASS(NULL)

	ID2D1Factory			*m_pFactory;
    ID2D1HwndRenderTarget   *m_pRenderTarget;
    ID2D1SolidColorBrush    *m_pBrush;
    D2D1_ELLIPSE            m_ellipse;

	CView() : m_pFactory(NULL), m_pRenderTarget(NULL), m_pBrush(NULL)
	{}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	void CalculateLayout()
	{
		if (NULL != m_pRenderTarget)
		{
			D2D1_SIZE_F size = m_pRenderTarget->GetSize();
			const float x = size.width / 2;
			const float y = size.height / 2;
			const float radius = min(x/2, y/2);
			m_ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
		}
	}

	HRESULT CreateGraphicsResources()
	{
		if(NULL == g_pFactory) return S_FALSE;

		HRESULT hr = S_OK;
		if (NULL == m_pRenderTarget)
		{
			RECT rc;
			GetClientRect(&rc);

			D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

			hr = g_pFactory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(m_hWnd, size),
				&m_pRenderTarget);

			if (SUCCEEDED(hr))
			{
				const D2D1_COLOR_F color = D2D1::ColorF(0, 1.0f, 0);
				hr = m_pRenderTarget->CreateSolidColorBrush(color, &m_pBrush);

				if (SUCCEEDED(hr))
				{
					CalculateLayout();
				}
			}
		}
		return hr;
	}

	void DoPaint(CDCHandle dc)
	{
		//TODO: Add your drawing code here
		 HRESULT hr = CreateGraphicsResources();
		 if (SUCCEEDED(hr))
		 {
			m_pRenderTarget->BeginDraw();

			m_pRenderTarget->Clear( D2D1::ColorF(D2D1::ColorF::White) );
			m_pRenderTarget->FillEllipse(m_ellipse, m_pBrush);

			hr = m_pRenderTarget->EndDraw();
			if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
			{
				SafeRelease(&m_pRenderTarget);
				SafeRelease(&m_pBrush);
			}			
		 }
	}

	BEGIN_MSG_MAP(CView)
		CHAIN_MSG_MAP(CScrollWindowImpl<CView>)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLBtnDown)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_UI_NOTIFY, OnUINotify)
		MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		DragAcceptFiles(); /* accept the drag and drop file */
		HRESULT hr = CreateGraphicsResources();
		return hr;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		SafeRelease(&m_pRenderTarget);
		SafeRelease(&m_pBrush);
		SafeRelease(&g_pFactory);
		return 0;
	}

	LRESULT OnDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		TCHAR path[MAX_PATH + 1] = { 0 };

		if (DragQueryFile((HDROP)wParam, 0, path, MAX_PATH)) {

			//MessageBox(path, _T("MB_OK"), MB_OK);
			ZeroMemory(g_filepath, MAX_PATH + 1);
			wmemcpy((wchar_t*)g_filepath, path, MAX_PATH + 1);
			g_fileloaded = TRUE;
			_beginthreadex(NULL, 0, open_msp_thread, m_hWnd, 0, NULL);

			::PostMessage(GetTopLevelParent(), WM_UI_NOTIFY, 0, 0);
		}

		return 0;
	}

	LRESULT OnLBtnDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
        ::PostMessage(GetTopLevelParent(), WM_NCLBUTTONDOWN, HTCAPTION, lParam);
		return 0;
	}

	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		if (NULL != m_pRenderTarget)
		{
			RECT rc;
			GetClientRect(&rc);

			D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

			m_pRenderTarget->Resize(size);
			CalculateLayout();
			InvalidateRect(&rc);
		}		
		return 0;
	}

	int OpenMSPFile(LPCTSTR lpszURL)
	{
		ZeroMemory(g_filepath, MAX_PATH + 1);
		wmemcpy((wchar_t*)g_filepath, lpszURL, MAX_PATH + 1);
		g_fileloaded = TRUE;

		_beginthreadex(NULL, 0, open_msp_thread, m_hWnd, 0, NULL);
		return 0;
	}

	LRESULT OnUINotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		fileType ft = (fileType)lParam;

		switch(ft)
		{
			case filePNG:
				MessageBox(_T("PNG"), _T("MB_OK"), MB_OK);				
				break;
			default:
				return 0;
		}

		return 0;
	}

};
