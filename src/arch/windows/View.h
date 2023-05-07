// View.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "threadwin.h"

class CView : public CScrollWindowImpl<CView>
{
public:
	DECLARE_WND_CLASS(NULL)

	BOOL	m_InitSize;
	UINT32	m_width;
	UINT32	m_height;

    ID2D1HwndRenderTarget*	m_pRenderTarget;

	CView() : m_pRenderTarget(NULL)
	{
		m_InitSize = FALSE;
		m_width = m_height = 0;
	}

	~CView()
	{
		if(NULL!= m_pRenderTarget)
		{
			m_pRenderTarget->Release();
			m_pRenderTarget = NULL;
		}
	}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	void DrawDefault()
	{
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
		}
		return hr;
	}

	void DoPaint(CDCHandle dc)
	{
		int tries	= 10;
		BOOL get_it = FALSE;

		while(tries > 0 )
		{
			if(FALSE != TryEnterCriticalSection(&(d2d.cs))) 
			{
				get_it = TRUE;
				break;
			}
			tries--; 
		}

		HRESULT hr = CreateGraphicsResources();

		if(FALSE == get_it) 
		{
			if(SUCCEEDED(hr)) DrawDefault();
			return;
		}

		if(NULL == d2d.pData)
		{
			if(SUCCEEDED(hr)) DrawDefault();
			LeaveCriticalSection(&(d2d.cs));
			return;
		}

		if(FAILED(hr)) {
			LeaveCriticalSection(&(d2d.cs));
			return;
		}

		unsigned int so_type;
		D2DRenderNode	n = d2d.pData;
		
		m_pRenderTarget->BeginDraw();
		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		m_pRenderTarget->Clear( D2D1::ColorF(D2D1::ColorF::White) );
		while(NULL != n)
		{
			so_type = (0x03 & n->std.flag);
			switch(so_type)
			{
				case SO_TYPE_GRAPHIC	:	
						DrawSVGGraphic(n);
						break;
				case SO_TYPE_TEXT		:	
						DrawSVGText(n);
						break;
				case SO_TYPE_IMAGE		:
						DrawSVGImage(n);
						break;
				default					:	
						break;
			}
			n = (D2DRenderNode)n->std.next;
		}

		LeaveCriticalSection(&(d2d.cs));

		hr = m_pRenderTarget->EndDraw();
		
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			m_pRenderTarget->Release();
			m_pRenderTarget = NULL;
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
		m_pRenderTarget = NULL;
#if 0		
		d2d.pData = (RenderNode)palloc(sizeof(RenderNodeData));
		RenderNode n = d2d.pData;
		if(NULL != n)
		{
			n->flag = SO_TYPE_TEXT;
			n->next = NULL;
			n->x	= -10;
			n->y	= 10;
			n->data	= _T("Bitcoin!!!");
			n->len  = wcsnlen_s((const wchar_t*)n->data, 100);
		}
#endif 		
		HRESULT hr = CreateGraphicsResources();
		return hr;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(NULL!= m_pRenderTarget)
		{
			m_pRenderTarget->Release();
			m_pRenderTarget = NULL;
		}
		return 0;
	}

	LRESULT OnDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		TCHAR path[MAX_PATH + 1] = { 0 };

		if (DragQueryFile((HDROP)wParam, 0, path, MAX_PATH)) {

			ZeroMemory(g_filepath, MAX_PATH + 1);
			wmemcpy((wchar_t*)g_filepath, path, MAX_PATH + 1);
			
			_beginthreadex(NULL, 0, open_mspfile_thread, m_hWnd, 0, NULL);
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
		return 0;
	}

	LRESULT OnUINotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		if(UI_NOTIFY_FILEOPEN == wParam)
		{
			fileType ft = (fileType)lParam;

			switch(ft)
			{
				case filePNG	:
				case fileJPG	:
				case fileGIF	:				
					g_fileloaded = TRUE;
					m_InitSize = FALSE;  // we need to check the size of the image of the next painting
					InvalidateRect(NULL);
					UpdateWindow();
					break;
				default:
					//::PostMessage(GetTopLevelParent(), WM_UI_NOTIFY, 0, 0);
					return 0;
			}
		}

		return 0;
	}

	void DrawSVGGraphic(D2DRenderNode n)
	{}
	
	void DrawSVGText(D2DRenderNode n)
	{
#if 0		
		HRESULT hr = S_OK;
		ID2D1SolidColorBrush* brush = NULL;

		hr = target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);

		if (!SUCCEEDED(hr)) return;

		D2D1_RECT_F layoutRect = D2D1::RectF(n->x, n->y, 400, 200);

		target->DrawText((const WCHAR *)n->data, n->len, d2d.pTextFormat, layoutRect, brush);

		brush->Release();
		brush = NULL;
#endif 
	}

	void DrawSVGImage(D2DRenderNode n)
	{
		if(NULL != n->pConverter)
		{
			ID2D1Bitmap*	b = NULL;
			m_pRenderTarget->CreateBitmapFromWicBitmap(n->pConverter, NULL, &b);
			if(NULL != b) 
			{
				POINT pt;
				if(!m_InitSize)
				{
					D2D1_SIZE_U s = b->GetPixelSize();
					m_width 	= s.width;
					m_height 	= s.height;
					m_InitSize	= TRUE;
					SetScrollSize(m_width, m_height);
				}
				GetScrollOffset(pt);
				D2D1_RECT_F srcRect = D2D1::RectF(
					static_cast<float>(-pt.x),
					static_cast<float>(-pt.y),
					static_cast<float>(m_width - pt.x),
					static_cast<float>(m_height - pt.y));				
				m_pRenderTarget->DrawBitmap(b, &srcRect);
				b->Release();
				b = NULL;
			}
		}
	}

};
