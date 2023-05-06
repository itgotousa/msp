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

#define UI_NOTIFY_MONITOR	0x01
#define UI_NOTIFY_FILEOPN	0x02

unsigned WINAPI _monitor_msp_thread(LPVOID lpData)
{
#if 0	
	DWORD dwRet;
	HWND hWndUI = (HWND)lpData;
	ATLASSERT(::IsWindow(hWndUI));

	g_kaSignal[1] = FindFirstChangeNotification(g_filepath, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);

	if(INVALID_HANDLE_VALUE == g_kaSignal[1]) {
		MessageBox(NULL, g_filepath, _T("Cannot Monitor"), MB_OK);
		return 0;
	}

	InterlockedIncrement(&g_threadCount);

	dwRet = MsgWaitForMultipleObjects(2, g_kaSignal, FALSE, INFINITE, QS_ALLINPUT);

	if(WAIT_OBJECT_0 + 1 == dwRet)
	{
		//MessageBox(NULL, g_filepath, _T("MB_OK"), MB_OK);
		PostMessage(hWndUI, WM_UI_NOTIFY, UI_NOTIFY_MONITOR, 0);
	}

	InterlockedDecrement(&g_threadCount);

	MessageBox(NULL, _T("Monitoring thead is quiting!"), _T("MB_OK"), MB_OK);
#endif 
	return 0;
}

unsigned WINAPI open_msp_thread(LPVOID lpData)
{
	char *p;
	int   fd;
	unsigned int size, align_size, bytes, ret;
	fileType ft = fileUnKnown;

	HWND hWndUI = (HWND)lpData;
	ATLASSERT(::IsWindow(hWndUI));

#if 0
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

	g_buffer = (char*)malloc(align_size);
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

	PostMessage(hWndUI, WM_UI_NOTIFY, UI_NOTIFY_FILEOPN, ft);
#endif 
	return 0;
}

class CView : public CScrollWindowImpl<CView>
{
public:
	DECLARE_WND_CLASS(NULL)

    ID2D1HwndRenderTarget*	m_pRenderTarget;

	CComPtr<ID2D1PathGeometry> GetGraphicsPath() {}

	CView() : m_pRenderTarget(NULL) {}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	void CalculateLayout()
	{
#if 0		
		if (NULL != m_pRenderTarget)
		{
			D2D1_SIZE_F size = m_pRenderTarget->GetSize();
			const float x = size.width / 2;
			const float y = size.height / 2;
			const float radius = std::min(x/2, y/2);
			m_ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
		}
#endif 		
	}

	HRESULT CreateGraphicsResources()
	{
		HRESULT hr = S_OK;
		if (NULL == m_pRenderTarget)
		{
			RECT rc;
			GetClientRect(&rc);

			D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

			hr = (d2d.pFactory)->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(m_hWnd, size),
				&m_pRenderTarget);
#if 0
			if (SUCCEEDED(hr))
			{
				const D2D1_COLOR_F color = D2D1::ColorF(0, 0, 0);
				hr = m_pRenderTarget->CreateSolidColorBrush(color, &m_pBrush);

				if (SUCCEEDED(hr))
				{
					CalculateLayout();
				}
			}
#endif 			
		}
		return hr;
	}

	void DoPaint(CDCHandle dc)
	{
		if(NULL != g_renderdata)
		{
			HRESULT hr = CreateGraphicsResources();
			if (SUCCEEDED(hr))
			{
				unsigned int so_type;
				RenderNode	n = g_renderdata;
				
				m_pRenderTarget->BeginDraw();
				m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
				m_pRenderTarget->Clear( D2D1::ColorF(D2D1::ColorF::White) );
				while(NULL != n)
				{
					so_type = (0x03 & n->flag);
					switch(so_type)
					{
						case SO_TYPE_GRAPHIC	:	DrawSVGGraphic(m_pRenderTarget, n);
													break;
						case SO_TYPE_TEXT		:	DrawSVGText(m_pRenderTarget, n);
													break;
						case SO_TYPE_IMAGE		:	DrawSVGImage(m_pRenderTarget, n);
													break;
						default					:	break;
					}
					n = n->next;
				}

				hr = m_pRenderTarget->EndDraw();
				
				if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
				{
					m_pRenderTarget->Release();
					m_pRenderTarget = NULL;
				}			
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

		g_renderdata = (RenderNode)palloc(sizeof(RenderNodeData));
		RenderNode n = g_renderdata;
		if(NULL != n)
		{
			n->flag = SO_TYPE_TEXT;
			n->next = NULL;
			//n->ctx  = TopMemoryContext;
			n->x	= -10;
			n->y	= 400;
			n->data	= _T("Hello, Huawei!");
			n->len  = wcsnlen_s((const wchar_t*)n->data, 100);
		}
		
		HRESULT hr = CreateGraphicsResources();
		return hr;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		m_pRenderTarget->Release();
		m_pRenderTarget = NULL;
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
			//_beginthreadex(NULL, 0, _monitor_msp_thread, m_hWnd, 0, NULL);

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

		//_beginthreadex(NULL, 0, _monitor_msp_thread, m_hWnd, 0, NULL);
		// _beginthreadex(NULL, 0, open_msp_thread, m_hWnd, 0, NULL);
		return 0;
	}

	LRESULT OnUINotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		if(UI_NOTIFY_MONITOR == lParam)
		{
			MessageBox(_T("File is changed"), _T("MB_OK"), MB_OK);
		}

#if 0		
		fileType ft = (fileType)lParam;

		switch(ft)
		{
			case filePNG:
				MessageBox(_T("PNG"), _T("MB_OK"), MB_OK);				
				break;
			default:
				return 0;
		}
#endif 
		return 0;
	}

	static void DrawSVGGraphic(ID2D1HwndRenderTarget* target, RenderNode n)
	{}
	static void DrawSVGText(ID2D1HwndRenderTarget* target, RenderNode n)
	{
		HRESULT hr = S_OK;
		ID2D1SolidColorBrush* brush = NULL;

		hr = target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);

		if (!SUCCEEDED(hr)) return;

		D2D1_RECT_F layoutRect = D2D1::RectF(n->x, n->y, 400, 200);

		target->DrawText((const WCHAR *)n->data, n->len, d2d.pTextFormat, layoutRect, brush);

		brush->Release();
		brush = NULL;

	}
	static void DrawSVGImage(ID2D1HwndRenderTarget* target, RenderNode n)
	{}

};
