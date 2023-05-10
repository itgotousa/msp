// View.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

// Default DPI that maps image resolution directly to screen resoltuion
const FLOAT DEFAULT_DPI = 96.f; 

inline LONG RectWidth(RECT rc)
{
    return rc.right - rc.left;
}

inline LONG RectHeight(RECT rc)
{
    return rc.bottom - rc.top;
}

class CView : public CScrollWindowImpl<CView>
{
public:
	DECLARE_WND_CLASS(NULL)

private:
    enum DISPOSAL_METHODS
    {
        DM_UNDEFINED  = 0,
        DM_NONE       = 1,
        DM_BACKGROUND = 2,
        DM_PREVIOUS   = 3 
    };

	BOOL	m_timerStart;
	BOOL	m_InitSize;
	UINT32	m_width;
	UINT32	m_height;
	AnimationData m_Am;

    ID2D1HwndRenderTarget*		m_pRenderTarget;
	ID2D1BitmapRenderTarget*	m_pFrameComposeRT;
	// The temporary bitmap used for disposal 3 method
    ID2D1Bitmap*                m_pSavedFrame; 
	ID2D1Bitmap*                m_pRawFrame;

    BOOL IsLastFrame() { return (0 == m_Am.frameIndex); }

    BOOL EndOfAnimation() 
	{ 
		return (m_Am.hasLoop && IsLastFrame() && (m_Am.loopNumber == m_Am.totalLoopCount + 1));
    }

public:
	CView() : m_pRenderTarget(NULL), m_pFrameComposeRT(NULL), m_pSavedFrame(NULL), m_pRawFrame(NULL)
	{
		m_timerStart = FALSE;
		m_InitSize = FALSE;
		m_width = m_height = 0;
		ZeroMemory(&m_Am, sizeof(AnimationData));
	}

	~CView()
	{
		SAFERELEASE(m_pRenderTarget);
		SAFERELEASE(m_pFrameComposeRT);
		SAFERELEASE(m_pSavedFrame);
		SAFERELEASE(m_pRawFrame);
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

#if 0
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
#endif 
	/******************************************************************
	*                                                                 *
	*  CreateDeviceResources   			                              *
	*                                                                 *
	*  Creates a D2D hwnd render target for displaying 			      *
	*  to users and a D2D bitmap render for composing frames.         *
	*                                                                 *
	******************************************************************/

	HRESULT CreateDeviceResources()
	{
		HRESULT hr = S_OK;

		RECT rcClient;
		if (!GetClientRect(&rcClient))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}

		if (SUCCEEDED(hr))
		{
			if (NULL == m_pRenderTarget)
			{
				m_Am.frameIndex = 0;
				m_Am.frameDisposal = DM_NONE;  // No previous frames. Use disposal none.
				m_Am.loopNumber = 0;

				// Create a D2D hwnd render target
				D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties 
					= D2D1::RenderTargetProperties();

				// Set the DPI to be the default system DPI to allow direct mapping
				// between image pixels and desktop pixels in different system DPI
				// settings
				renderTargetProperties.dpiX = DEFAULT_DPI;
				renderTargetProperties.dpiY = DEFAULT_DPI;

				D2D1_HWND_RENDER_TARGET_PROPERTIES hwndRenderTragetproperties
					= D2D1::HwndRenderTargetProperties(m_hWnd,
						D2D1::SizeU(RectWidth(rcClient), RectHeight(rcClient)));

				hr = (d2d.pFactory)->CreateHwndRenderTarget(
					renderTargetProperties,
					hwndRenderTragetproperties,
					&m_pRenderTarget);
			}
			else
			{
				// We already have a hwnd render target, resize it to the window size
				D2D1_SIZE_U size;
				size.width = RectWidth(rcClient);
				size.height = RectHeight(rcClient);
				hr = m_pRenderTarget->Resize(size);
			}
		}

		if (SUCCEEDED(hr))
		{
			// Create a bitmap render target used to compose frames. Bitmap render 
			// targets cannot be resized, so we always recreate it.
			SAFERELEASE(m_pFrameComposeRT);
			hr = m_pRenderTarget->CreateCompatibleRenderTarget(
				D2D1::SizeF(
					static_cast<FLOAT>(RectWidth(rcClient)),
					static_cast<FLOAT>(RectHeight(rcClient))),
				&m_pFrameComposeRT);
		}

		return hr;
	}

#if 0		
	/******************************************************************
	*                                                                 *
	*  RecoverDeviceResources 			                              *
	*                                                                 *
	*  Discards device-specific resources and recreates them.         *
	*  Also starts the animation from the beginning.                  *
	*                                                                 *
	******************************************************************/

	HRESULT RecoverDeviceResources()
	{
		SAFERELEASE(m_pRenderTarget);
		SAFERELEASE(m_pFrameComposeRT);
		SAFERELEASE(m_pSavedFrame);

		m_Am.frameIndex = 0;
		m_Am.frameDisposal = DM_NONE;  // No previous frames. Use disposal none.
		m_Am.loopNumber = 0;

		HRESULT hr = CreateDeviceResources();

		if (SUCCEEDED(hr))
		{
			if (m_Am.frameCount > 0)
			{
				// Load the first frame
				hr = ComposeNextFrame();
				InvalidateRect(NULL, FALSE);
			}

		}
		return hr;
	}
#endif 		

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

		//HRESULT hr = CreateGraphicsResources();
		HRESULT hr = CreateDeviceResources();

		if(FALSE == get_it) 
		{
			if(SUCCEEDED(hr)) DrawDefault();
			return;
		}

		if(NULL == d2d.pData)
		{
			LeaveCriticalSection(&(d2d.cs));
			if(SUCCEEDED(hr)) DrawDefault();
			return;
		}

		if(FAILED(hr)) {
			LeaveCriticalSection(&(d2d.cs));
			return;
		}
		// Only render when the window is not occluded
		if ((m_pRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
		{
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
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
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
		HRESULT hr = CreateDeviceResources();
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
			wmemcpy((wchar_t*)g_filepath, path, MAX_PATH);
			
			tp.hWnd = m_hWnd;
			tp.pfilePath = (TCHAR*)g_filepath;
			_beginthreadex(NULL, 0, open_mspfile_thread, &tp, 0, NULL);
		}

		return 0;
	}

	LRESULT OnLBtnDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
        ::PostMessage(GetTopLevelParent(), WM_NCLBUTTONDOWN, HTCAPTION, lParam);
		return 0;
	}

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		ComposeNextFrame();
		InvalidateRect(NULL, FALSE);
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
		wmemcpy((wchar_t*)g_filepath, lpszURL, MAX_PATH);

		tp.hWnd = m_hWnd;
		tp.pfilePath = (TCHAR*)g_filepath;
		_beginthreadex(NULL, 0, open_mspfile_thread, &tp, 0, NULL);

		return 0;
	}

	LRESULT OnUINotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		D2DRenderNode	n;

		if(UI_NOTIFY_FILEOPEN == wParam)
		{
			if(m_timerStart) {
				KillTimer(ANIMATION_TIMER_ID);
				m_timerStart = FALSE;
			}

			fileType ft = (fileType)lParam;

			switch(ft)
			{
				case fileGIF	:
#if 0				
					ZeroMemory(&m_Am, sizeof(AnimationData));
					EnterCriticalSection(&(d2d.cs));
						n = d2d.pData;
						if(NULL != n)
						{
							if(NULL != n->am)
							{
								// cache it!
								memcpy_s(&m_Am, sizeof(AnimationData), n->am, sizeof(AnimationData));
							}
						}
					LeaveCriticalSection(&(d2d.cs));
					if(m_Am.frameCount > 1 ) // it is animation GIF
					{
						ComposeNextFrame();
						if(0 != SetTimer(ANIMATION_TIMER_ID, m_Am.frameDelay, NULL)) m_timerStart = TRUE;
						
					}
#endif 					
				case filePNG	:
				case fileBMP	:
				case fileJPG	:
				case fileSVG	:
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
	{
	    HRESULT hr = S_OK;

		if(NULL == n->pGeometry) return;
		
		ID2D1SolidColorBrush* brush = NULL;

		hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);
		if(FAILED(hr)) return;
		
		m_pRenderTarget->DrawGeometry(n->pGeometry, brush, 1, n->pStrokeStyle);

		brush->Release();
		brush = NULL;
	}
	
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
		HRESULT hr = S_OK;
		ID2D1Bitmap* b = NULL;

		if(n->std.flag | SO_HINT_BITMAP)
		{
			hr = m_pRenderTarget->CreateBitmap(
				D2D1::SizeU(n->std.width, n->std.height),
				n->std.data,
				n->std.stride,
				D2D1::BitmapProperties(
					D2D1::PixelFormat(
						DXGI_FORMAT_B8G8R8A8_UNORM,
						D2D1_ALPHA_MODE_IGNORE
					)
				),
				&b
			);
			if (SUCCEEDED(hr))
			{
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
					SAFERELEASE(b);
				}
			}
			
			return;
		}

		if(NULL == n->pConverter) return;

		if(fileGIF == d2d.ft)
		{
			ID2D1Bitmap *pFrameToRender = NULL;
			//D2D1_RECT_F drawRect;
			//hr = CalculateDrawRectangle(drawRect);
			if (SUCCEEDED(hr))
			{
				// Get the bitmap to draw on the hwnd render target
				hr = m_pFrameComposeRT->GetBitmap(&pFrameToRender);
			}

			if (SUCCEEDED(hr))
			{
				// Draw the bitmap onto the calculated rectangle
				m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
				m_pRenderTarget->DrawBitmap(pFrameToRender, m_Am.framePosition);
				SAFERELEASE(pFrameToRender);
			}
		}
		else 
		{
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

	/******************************************************************
	*                                                                 *
	*  CalculateDrawRectangle() 		                              *
	*                                                                 *
	*  Calculates a specific rectangular area of the hwnd             *
	*  render target to draw a bitmap containing the current          *
	*  composed frame.                                                *
	*                                                                 *
	******************************************************************/

	HRESULT CalculateDrawRectangle(D2D1_RECT_F &drawRect)
	{
		HRESULT hr = S_OK;
		RECT rcClient;

		// Top and left of the client rectangle are both 0
		if (!GetClientRect(&rcClient))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}

		if (SUCCEEDED(hr))
		{
			// Calculate the area to display the image
			// Center the image if the client rectangle is larger
			drawRect.left = (static_cast<FLOAT>(rcClient.right) - m_Am.cxImagePixel) / 2.f;
			drawRect.top = (static_cast<FLOAT>(rcClient.bottom) - m_Am.cyImagePixel) / 2.f;
			drawRect.right = drawRect.left + m_Am.cxImagePixel;
			drawRect.bottom = drawRect.top + m_Am.cyImagePixel;

			// If the client area is resized to be smaller than the image size, scale
			// the image, and preserve the aspect ratio
			FLOAT aspectRatio = static_cast<FLOAT>(m_Am.cxImagePixel) /
				static_cast<FLOAT>(m_Am.cyImagePixel);

			if (drawRect.left < 0)
			{
				FLOAT newWidth = static_cast<FLOAT>(rcClient.right);
				FLOAT newHeight = newWidth / aspectRatio;
				drawRect.left = 0;
				drawRect.top = (static_cast<FLOAT>(rcClient.bottom) - newHeight) / 2.f;
				drawRect.right = newWidth;
				drawRect.bottom = drawRect.top + newHeight;
			}

			if (drawRect.top < 0)
			{
				FLOAT newHeight = static_cast<FLOAT>(rcClient.bottom);
				FLOAT newWidth = newHeight * aspectRatio;
				drawRect.left = (static_cast<FLOAT>(rcClient.right) - newWidth) / 2.f;
				drawRect.top = 0;
				drawRect.right = drawRect.left + newWidth;
				drawRect.bottom = newHeight;
			}
		}

		return hr;
	}

	/******************************************************************
	*                                                                 *
	*  ClearCurrentFrameArea() 			                              *
	*                                                                 *
	*  Clears a rectangular area equal to the area overlaid by the    *
	*  current raw frame in the bitmap render target with background  *
	*  color.                                                         *
	*                                                                 *
	******************************************************************/

	HRESULT ClearCurrentFrameArea()
	{
		HRESULT hr = S_FALSE;

		m_pFrameComposeRT->BeginDraw();

		// Clip the render target to the size of the raw frame
		m_pFrameComposeRT->PushAxisAlignedClip(&(m_Am.framePosition),
												D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

		m_pFrameComposeRT->Clear(m_Am.color_bkg);
		// Remove the clipping
		m_pFrameComposeRT->PopAxisAlignedClip();

		hr = m_pFrameComposeRT->EndDraw();

		return hr;
	}

	/******************************************************************
	*                                                                 *
	*  DemoApp::RestoreSavedFrame()                                   *
	*                                                                 *
	*  Copys the saved frame to the frame in the bitmap render        * 
	*  target.                                                        *
	*                                                                 *
	******************************************************************/

	HRESULT RestoreSavedFrame()
	{
		HRESULT hr = S_OK;

		ID2D1Bitmap *pFrameToCopyTo = NULL;

		hr = m_pSavedFrame ? S_OK : E_FAIL;

		if(SUCCEEDED(hr))
		{
			hr = m_pFrameComposeRT->GetBitmap(&pFrameToCopyTo);
		}

		if (SUCCEEDED(hr))
		{
			// Copy the whole bitmap
			hr = pFrameToCopyTo->CopyFromBitmap(NULL, m_pSavedFrame, NULL);
		}

		SAFERELEASE(pFrameToCopyTo);

		return hr;
	}

	/******************************************************************
	*                                                                 *
	*  DisposeCurrentFrame() 		                                  *
	*                                                                 *
	*  At the end of each delay, disposes the current frame           *
	*  based on the disposal method specified.                        *
	*                                                                 *
	******************************************************************/

	HRESULT DisposeCurrentFrame()
	{
		HRESULT hr = S_OK;

		switch (m_Am.frameDisposal)
		{
		case DM_UNDEFINED:
		case DM_NONE:
			// We simply draw on the previous frames. Do nothing here.
			break;
		case DM_BACKGROUND:
			// Dispose background
			// Clear the area covered by the current raw frame with background color
			hr = ClearCurrentFrameArea();
			break;
		case DM_PREVIOUS:
			// Dispose previous
			// We restore the previous composed frame first
			hr = RestoreSavedFrame();
			break;
		default:
			// Invalid disposal method
			hr = E_FAIL;
		}
		return hr;
	}

	/******************************************************************
	*                                                                 *
	*  GetRawFrame() 		                                          *
	*                                                                 *
	*  Decodes the current raw frame, retrieves its timing            *
	*  information, disposal method, and frame dimension for          *
	*  rendering.  Raw frame is the frame read directly from the gif  *
	*  file without composing.                                        *
	*                                                                 *
	******************************************************************/

	HRESULT GetRawFrame(UINT uFrameIndex)
	{
		HRESULT hr = E_FAIL;
		IWICFormatConverter *pConverter = NULL;
		IWICBitmapFrameDecode *pWicFrame = NULL;
		IWICMetadataQueryReader *pFrameMetadataQueryReader = NULL;
		
		PROPVARIANT propValue;
		PropVariantInit(&propValue);

		// Retrieve the current frame
		EnterCriticalSection(&(d2d.cs));
			D2DRenderNode n = d2d.pData;
			if(NULL != n)
			{
				if(NULL != n->pDecoder)
				{
					hr = n->pDecoder->GetFrame(uFrameIndex, &pWicFrame);
				}
			}
		LeaveCriticalSection(&(d2d.cs));

		if (SUCCEEDED(hr))
		{
			// Format convert to 32bppPBGRA which D2D expects
			hr = d2d.pIWICFactory->CreateFormatConverter(&pConverter);
		}

		if (SUCCEEDED(hr))
		{
			hr = pConverter->Initialize(
				pWicFrame,
				GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone,
				NULL,
				0.0,
				WICBitmapPaletteTypeCustom);
		}

		if (SUCCEEDED(hr))
		{
			// Create a D2DBitmap from IWICBitmapSource
			SAFERELEASE(m_pRawFrame);
			hr = m_pRenderTarget->CreateBitmapFromWicBitmap(
				pConverter,
				NULL,
				&m_pRawFrame);
		}

		if (SUCCEEDED(hr))
		{
			// Get Metadata Query Reader from the frame
			hr = pWicFrame->GetMetadataQueryReader(&pFrameMetadataQueryReader);
		}

		// Get the Metadata for the current frame
		if (SUCCEEDED(hr))
		{
			hr = pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Left", &propValue);
			if (SUCCEEDED(hr))
			{
				hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL); 
				if (SUCCEEDED(hr))
				{
					m_Am.framePosition.left = static_cast<FLOAT>(propValue.uiVal);
				}
				PropVariantClear(&propValue);
			}
		}

		if (SUCCEEDED(hr))
		{
			hr = pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Top", &propValue);
			if (SUCCEEDED(hr))
			{
				hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL); 
				if (SUCCEEDED(hr))
				{
					m_Am.framePosition.top = static_cast<FLOAT>(propValue.uiVal);
				}
				PropVariantClear(&propValue);
			}
		}

		if (SUCCEEDED(hr))
		{
			hr = pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Width", &propValue);
			if (SUCCEEDED(hr))
			{
				hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL); 
				if (SUCCEEDED(hr))
				{
					m_Am.framePosition.right = static_cast<FLOAT>(propValue.uiVal) 
						+ m_Am.framePosition.left;
				}
				PropVariantClear(&propValue);
			}
		}

		if (SUCCEEDED(hr))
		{
			hr = pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Height", &propValue);
			if (SUCCEEDED(hr))
			{
				hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
				if (SUCCEEDED(hr))
				{
					m_Am.framePosition.bottom = static_cast<FLOAT>(propValue.uiVal)
						+ m_Am.framePosition.top;
				}
				PropVariantClear(&propValue);
			}
		}

		if (SUCCEEDED(hr))
		{
			// Get delay from the optional Graphic Control Extension
			if (SUCCEEDED(pFrameMetadataQueryReader->GetMetadataByName(
				L"/grctlext/Delay", 
				&propValue)))
			{
				hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL); 
				if (SUCCEEDED(hr))
				{
					// Convert the delay retrieved in 10 ms units to a delay in 1 ms units
					hr = UIntMult(propValue.uiVal, 10, &m_Am.frameDelay);
				}
				PropVariantClear(&propValue);
			}
			else
			{
				// Failed to get delay from graphic control extension. Possibly a
				// single frame image (non-animated gif)
				m_Am.frameDelay = 0;
			}

			if (SUCCEEDED(hr))
			{
				// Insert an artificial delay to ensure rendering for gif with very small
				// or 0 delay.  This delay number is picked to match with most browsers' 
				// gif display speed.
				//
				// This will defeat the purpose of using zero delay intermediate frames in 
				// order to preserve compatibility. If this is removed, the zero delay 
				// intermediate frames will not be visible.
				if (m_Am.frameDelay < 90)
				{
					m_Am.frameDelay = 90;
				}
			}
		}

		if (SUCCEEDED(hr))
		{
			if (SUCCEEDED(pFrameMetadataQueryReader->GetMetadataByName(
					L"/grctlext/Disposal", 
					&propValue)))
			{
				hr = (propValue.vt == VT_UI1) ? S_OK : E_FAIL;
				if (SUCCEEDED(hr))
				{
					m_Am.frameDisposal = propValue.bVal;
				}
			}
			else
			{
				// Failed to get the disposal method, use default. Possibly a 
				// non-animated gif.
				m_Am.frameDisposal = DM_UNDEFINED;
			}
		}

		PropVariantClear(&propValue);

		SAFERELEASE(pConverter);
		SAFERELEASE(pWicFrame);
		SAFERELEASE(pFrameMetadataQueryReader);

		return hr;
	}

	/******************************************************************
	*                                                                 *
	*  SaveComposedFrame() 			                                  *
	*                                                                 *
	*  Saves the current composed frame in the bitmap render target   *
	*  into a temporary bitmap. Initializes the temporary bitmap if   *
	*  needed.                                                        *
	*                                                                 *
	******************************************************************/

	HRESULT SaveComposedFrame()
	{
		HRESULT hr = S_OK;

		ID2D1Bitmap *pFrameToBeSaved = NULL;

		hr = m_pFrameComposeRT->GetBitmap(&pFrameToBeSaved);
		if (SUCCEEDED(hr))
		{
			// Create the temporary bitmap if it hasn't been created yet 
			if (NULL == m_pSavedFrame)
			{
				D2D1_SIZE_U bitmapSize = pFrameToBeSaved->GetPixelSize();
				D2D1_BITMAP_PROPERTIES bitmapProp;
				pFrameToBeSaved->GetDpi(&bitmapProp.dpiX, &bitmapProp.dpiY);
				bitmapProp.pixelFormat = pFrameToBeSaved->GetPixelFormat();

				hr = m_pFrameComposeRT->CreateBitmap(
					bitmapSize,
					bitmapProp,
					&m_pSavedFrame);
			}
		}

		if (SUCCEEDED(hr))
		{
			// Copy the whole bitmap
			hr = m_pSavedFrame->CopyFromBitmap(NULL, pFrameToBeSaved, NULL);
		}

		SAFERELEASE(pFrameToBeSaved);

		return hr;
	}

	/******************************************************************
	*                                                                 *
	*  OverlayNextFrame() 			                                  *
	*                                                                 *
	*  Loads and draws the next raw frame into the composed frame     *
	*  render target. This is called after the current frame is       *
	*  disposed.                                                      *
	*                                                                 *
	******************************************************************/

	HRESULT OverlayNextFrame()
	{
		// Get Frame information
		HRESULT hr = GetRawFrame(m_Am.frameIndex);
		if (SUCCEEDED(hr))
		{
			// For disposal 3 method, we would want to save a copy of the current
			// composed frame
			if (DM_PREVIOUS == m_Am.frameDisposal)
			{
				hr = SaveComposedFrame();
			}
		}

		if (SUCCEEDED(hr))
		{
			// Start producing the next bitmap
			m_pFrameComposeRT->BeginDraw();

			// If starting a new animation loop
			if (0 == m_Am.frameIndex)
			{
				// Draw background and increase loop count
				m_pFrameComposeRT->Clear(m_Am.color_bkg);
				m_Am.loopNumber++;
			}

			// Produce the next frame
			m_pFrameComposeRT->DrawBitmap(
				m_pRawFrame,
				m_Am.framePosition);

			hr = m_pFrameComposeRT->EndDraw();
		}

		// To improve performance and avoid decoding/composing this frame in the 
		// following animation loops, the composed frame can be cached here in system 
		// or video memory.

		if (SUCCEEDED(hr))
		{
			// Increase the frame index by 1
			m_Am.frameIndex = (++m_Am.frameIndex) % m_Am.frameCount;
		}

		return hr;
	}

	/******************************************************************
	*                                                                 *
	*  ComposeNextFrame()			                                  *
	*                                                                 *
	*  Composes the next frame by first disposing the current frame   *
	*  and then overlaying the next frame. More than one frame may    *
	*  be processed in order to produce the next frame to be          *
	*  displayed due to the use of zero delay intermediate frames.    *
	*  Also, sets a timer that is equal to the delay of the frame.    *
	*                                                                 *
	******************************************************************/

	HRESULT ComposeNextFrame()
	{
		HRESULT hr = S_OK;
		// Check to see if the render targets are initialized
		if (m_pRenderTarget && m_pFrameComposeRT)
		{
			// First, kill the timer since the delay is no longer valid
			//KillTimer(ANIMATION_TIMER_ID);

			// Compose one frame
			hr = DisposeCurrentFrame();
			if (SUCCEEDED(hr))
			{
				hr = OverlayNextFrame();
			}

			// Keep composing frames until we see a frame with delay greater than
			// 0 (0 delay frames are the invisible intermediate frames), or until
			// we have reached the very last frame.
			while (SUCCEEDED(hr) && (0 == m_Am.frameDelay) && !IsLastFrame())
			{
				hr = DisposeCurrentFrame();
				if (SUCCEEDED(hr))
				{
					hr = OverlayNextFrame();
				}
			}

			// If we have more frames to play, set the timer according to the delay.
			// Set the timer regardless of whether we succeeded in composing a frame
			// to try our best to continue displaying the animation.
			if (!EndOfAnimation() && m_Am.frameCount > 1)
			{
				// Set the timer according to the delay
				//SetTimer(ANIMATION_TIMER_ID, m_Am.frameDelay, NULL);
			}
		}

		return hr;
	}
};
