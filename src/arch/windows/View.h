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

inline int RoundToInt(float x)
{
	return static_cast<int>(floor(x + .5));
}

inline double DegreesToRadians(float degrees)
{
	return degrees * MSP_PI * 2.0f / 360.0f;
}

inline D2D1::Matrix3x2F& Cast(DWRITE_MATRIX& matrix)
{
	// DWrite's matrix, D2D's matrix, and GDI's XFORM
	// are all compatible.
	return *reinterpret_cast<D2D1::Matrix3x2F*>(&matrix);
}

bool IsLandscapeAngle(float angle)
{
	// Returns true if the angle is rotated 90 degrees clockwise
	// or anticlockwise (or any multiple of that).
	return fmod(abs(angle) + 45.0f, 180.0f) >= 90.0f;
}

inline float GetDeterminant(DWRITE_MATRIX const& matrix)
{
	return matrix.m11 * matrix.m22 - matrix.m12 * matrix.m21;
}

void ComputeInverseMatrix(DWRITE_MATRIX const& matrix, OUT DWRITE_MATRIX& result)
{
	// Used for hit-testing, mouse scrolling, panning, and scroll bar sizing.
	float invdet = 1.f / GetDeterminant(matrix);
	result.m11 = matrix.m22 * invdet;
	result.m12 = -matrix.m12 * invdet;
	result.m21 = -matrix.m21 * invdet;
	result.m22 = matrix.m11 * invdet;
	result.dx = (matrix.m21 * matrix.dy - matrix.dx * matrix.m22) * invdet;
	result.dy = (matrix.dx * matrix.m12 - matrix.m11 * matrix.dy) * invdet;
}

struct CaretFormat
{
	// The important range based properties for the current caret.
	// Note these are stored outside the layout, since the current caret
	// actually has a format, independent of the text it lies between.
	wchar_t fontFamilyName[100];
	wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
	FLOAT fontSize;
	DWRITE_FONT_WEIGHT fontWeight;
	DWRITE_FONT_STRETCH fontStretch;
	DWRITE_FONT_STYLE fontStyle;
	UINT32 color;
	BOOL hasUnderline;
	BOOL hasStrikethrough;
};

class CView : public CScrollWindowImpl<CView>
{
public:
	DECLARE_WND_CLASS(NULL)

	CView() : m_pRenderTarget(NULL), m_pFrameComposeRT(NULL), m_pSavedFrame(NULL), m_pRawFrame(NULL)
	{
		ZeroMemory(&m_Am, sizeof(AnimationData));
		m_timerStart = FALSE;
		InitDefaults();
		InitViewDefaults();
	}

	~CView()
	{
		SAFERELEASE(m_pRenderTarget);
		SAFERELEASE(m_pFrameComposeRT);
		SAFERELEASE(m_pSavedFrame);
		SAFERELEASE(m_pRawFrame);
	}

private:
	enum SetSelectionMode
	{
		SetSelectionModeLeft,               // cluster left
		SetSelectionModeRight,              // cluster right
		SetSelectionModeUp,                 // line up
		SetSelectionModeDown,               // line down
		SetSelectionModeLeftChar,           // single character left (backspace uses it)
		SetSelectionModeRightChar,          // single character right
		SetSelectionModeLeftWord,           // single word left
		SetSelectionModeRightWord,          // single word right
		SetSelectionModeHome,               // front of line
		SetSelectionModeEnd,                // back of line
		SetSelectionModeFirst,              // very first position
		SetSelectionModeLast,               // very last position
		SetSelectionModeAbsoluteLeading,    // explicit position (for mouse click)
		SetSelectionModeAbsoluteTrailing,   // explicit position, trailing edge
		SetSelectionModeAll                 // select all text
	};

	enum DISPOSAL_METHODS
	{
		DM_UNDEFINED = 0,
		DM_NONE = 1,
		DM_BACKGROUND = 2,
		DM_PREVIOUS = 3
	};

	////////////////////////////////////////////////////////////////
	// Selection/Caret navigation
	///
	// caretAnchor equals caretPosition when there is no selection.
	// Otherwise, the anchor holds the point where shift was held
	// or left drag started.
	//
	// The offset is used as a sort of trailing edge offset from
	// the caret position. For example, placing the caret on the
	// trailing side of a surrogate pair at the beginning of the
	// text would place the position at zero and offset of two.
	// So to get the absolute leading position, sum the two.
	UINT32 m_caretAnchor;
	UINT32 m_caretPosition;
	UINT32 m_caretPositionOffset;    // > 0 used for trailing edge

	// Current attributes of the caret, which can be independent of the text.
	CaretFormat m_caretFormat;

	////////////////////
	// Mouse manipulation
	bool m_currentlySelecting : 1;
	bool m_currentlyPanning : 1;
	float m_previousMouseX;
	float m_previousMouseY;

	enum { MouseScrollFactor = 10 };

	////////////////////
	// Current view
	float m_scaleX;          // horizontal scaling
	float m_scaleY;          // vertical scaling
	float m_angle;           // in degrees
	float m_originX;         // focused point in document (moves on panning and caret navigation)
	float m_originY;
	float m_contentWidth;    // page size - margin left - margin right (can be fixed or variable)
	float m_contentHeight;

	void InitDefaults()
	{
		m_caretPosition = 0;
		m_caretAnchor = 0;
		m_caretPositionOffset = 0;

		m_currentlySelecting = false;
		m_currentlyPanning = false;
		m_previousMouseX = 0;
		m_previousMouseY = 0;
	}

	void InitViewDefaults()
	{
		m_scaleX = 1;
		m_scaleY = 1;
		m_angle = 0;
		m_originX = 0;
		m_originY = 0;
	}

	BOOL			m_timerStart;
	AnimationData	m_Am;

	ID2D1HwndRenderTarget*		m_pRenderTarget;
	ID2D1BitmapRenderTarget*	m_pFrameComposeRT;
	// The temporary bitmap used for disposal 3 method
	ID2D1Bitmap* m_pSavedFrame;
	ID2D1Bitmap* m_pRawFrame;

	BOOL IsLastFrame() { return (0 == m_Am.frameIndex); }

	BOOL EndOfAnimation()
	{
		return (m_Am.hasLoop && IsLastFrame() && (m_Am.loopNumber == m_Am.totalLoopCount + 1));
	}

public:
	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}


	void DrawDefault()
	{
#if 0
		D2DRenderNode n = d2d.pDataDefault;
		HRESULT hr = S_OK;
		ID2D1Bitmap* bmp = NULL;
		if(NULL == n) return;
		if(NULL == n->pConverter) return;

		int x, y, width, height;
		SIZE sz;
		RECT rc;
		GetClientRect(&rc);
		GetScrollSize(sz);
		if (sz.cx > rc.right)  rc.right = sz.cx;
		if (sz.cy > rc.bottom) rc.bottom = sz.cy;

		width = n->std.width; height = n->std.height;
		x = ((rc.right - rc.left - width) >> 1);
		y = ((rc.bottom - rc.top - height) >> 1);
		if(x < 0) x = 0; if(y < 0) y = 0;

		hr = m_pRenderTarget->CreateBitmapFromWicBitmap(n->pConverter, NULL, &bmp);
		if(SUCCEEDED(hr))
		{
			m_pRenderTarget->BeginDraw();

			m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
			m_pRenderTarget->Clear( D2D1::ColorF(D2D1::ColorF::White) );
			D2D1_RECT_F srcRect = D2D1::RectF(
				static_cast<float>(x),
				static_cast<float>(y),
				static_cast<float>(x + width),
				static_cast<float>(y + height));				
			m_pRenderTarget->DrawBitmap(bmp, &srcRect);
			bmp->Release();
			bmp = NULL;

			hr = m_pRenderTarget->EndDraw();
			if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
			{
				m_pRenderTarget->Release();
				m_pRenderTarget = NULL;
			}			
		}
#endif
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
			SIZE sz;
			GetScrollSize(sz);
			if (sz.cx > rcClient.right)  rcClient.right = sz.cx;
			if (sz.cy > rcClient.bottom) rcClient.bottom = sz.cy;

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

				if (SUCCEEDED(hr))
				{
					SAFERELEASE(m_pFrameComposeRT);
					hr = m_pRenderTarget->CreateCompatibleRenderTarget(
						D2D1::SizeF(
							static_cast<FLOAT>(RectWidth(rcClient)),
							static_cast<FLOAT>(RectHeight(rcClient))),
						&m_pFrameComposeRT);
				}
			}
#if 0
			else
			{
				// We already have a hwnd render target, resize it to the window size
				D2D1_SIZE_U size;
				size.width = RectWidth(rcClient);
				size.height = RectHeight(rcClient);
				hr = m_pRenderTarget->Resize(size);
			}
#endif
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

	void DrawMSPGraphic(D2DRenderNode n)
	{
#if 0
		HRESULT hr = S_OK;

		if (NULL == n->pGeometry) return;

		ID2D1SolidColorBrush* brush = NULL;

		hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);
		if (FAILED(hr)) return;

		m_pRenderTarget->DrawGeometry(n->pGeometry, brush, 1, n->pStrokeStyle);

		brush->Release();
		brush = NULL;
#endif
	}

	DWRITE_TEXT_RANGE GetSelectionRange(D2DRenderNode n)
	{
		// Returns a valid range of the current selection,
		// regardless of whether the caret or anchor is first.
		UINT32 caretBegin = m_caretAnchor;
		UINT32 caretEnd = m_caretPosition + m_caretPositionOffset;
		if (caretBegin > caretEnd)
			std::swap(caretBegin, caretEnd);

		// Limit to actual text length.
		UINT32 textLength = n->std.length;
		caretBegin = std::min(caretBegin, textLength);
		caretEnd = std::min(caretEnd, textLength);

		DWRITE_TEXT_RANGE textRange = { caretBegin, caretEnd - caretBegin };
		return textRange;
	}

	void GetCaretRect(D2DRenderNode n, OUT D2D1_RECT_F& rect)
	{
		// Gets the current caret position (in untransformed space).
		D2D1_RECT_F zeroRect = { 0 };
		rect = zeroRect;

		//if (textLayout_ == NULL) return;
		// Translate text character offset to point x,y.
		DWRITE_HIT_TEST_METRICS caretMetrics;
		float caretX, caretY;

		n->pTextLayout->HitTestTextPosition(
			m_caretPosition,
			m_caretPositionOffset > 0, // trailing if nonzero, else leading edge
			&caretX,
			&caretY,
			&caretMetrics
		);

		// If a selection exists, draw the caret using the
		// line size rather than the font size.
		DWRITE_TEXT_RANGE selectionRange = GetSelectionRange(n);
		if (selectionRange.length > 0)
		{
			UINT32 actualHitTestCount = 1;
			n->pTextLayout->HitTestTextRange(
				m_caretPosition,
				0, // length
				0, // x
				0, // y
				&caretMetrics,
				1,
				&actualHitTestCount
			);

			caretY = caretMetrics.top;
		}

		// The default thickness of 1 pixel is almost _too_ thin on modern large monitors,
		// but we'll use it.
		DWORD caretIntThickness = 2;
		SystemParametersInfo(SPI_GETCARETWIDTH, 0, &caretIntThickness, FALSE);
		const float caretThickness = float(caretIntThickness);

		// Return the caret rect, untransformed.
		rect.left = caretX - caretThickness / 2.0f;
		rect.right = rect.left + caretThickness;
		rect.top = caretY;
		rect.bottom = caretY + caretMetrics.height;
	}

	void DrawMSPText(D2DRenderNode n)
	{
		HRESULT hr = S_OK;

		// Calculate actual location in render target based on the
		// current page transform.
		D2D1::Matrix3x2F pageTransform;
		GetViewMatrix(reinterpret_cast<DWRITE_MATRIX*>(&pageTransform));

		// Scale/Rotate canvas as needed
		D2D1::Matrix3x2F previousTransform;
		m_pRenderTarget->GetTransform(&previousTransform);
		m_pRenderTarget->SetTransform(&pageTransform);

		// Draw the page
		D2D1_POINT_2F pageSize = GetPageSize();
		D2D1_RECT_F pageRect = { 0, 0, pageSize.x, pageSize.y };

		ID2D1SolidColorBrush* brush = NULL;
		hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush);
		if (SUCCEEDED(hr))
		{
			m_pRenderTarget->FillRectangle(pageRect, brush);
			SAFERELEASE(brush);
		}

		DWRITE_TEXT_RANGE caretRange = GetSelectionRange(n);
		UINT32 actualHitTestCount = 0;

		if (caretRange.length > 0)
		{
			n->pTextLayout->HitTestTextRange(
				caretRange.startPosition,
				caretRange.length,
				0, // x
				0, // y
				NULL,
				0, // metrics count
				&actualHitTestCount
			);
		}

		// Allocate enough room to return all hit-test metrics.
		std::vector<DWRITE_HIT_TEST_METRICS> hitTestMetrics(actualHitTestCount);

		if (caretRange.length > 0)
		{
			n->pTextLayout->HitTestTextRange(
				caretRange.startPosition,
				caretRange.length,
				0, // x
				0, // y
				&hitTestMetrics[0],
				static_cast<UINT32>(hitTestMetrics.size()),
				&actualHitTestCount
			);
		}

		// Draw the selection ranges behind the text.
		if (actualHitTestCount > 0)
		{
			// Note that an ideal layout will return fractional values,
			// so you may see slivers between the selection ranges due
			// to the per-primitive antialiasing of the edges unless
			// it is disabled (better for performance anyway).
			m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

			hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSkyBlue), &brush);
			if (SUCCEEDED(hr))
			{
				for (size_t i = 0; i < actualHitTestCount; ++i)
				{
					const DWRITE_HIT_TEST_METRICS& htm = hitTestMetrics[i];
					D2D1_RECT_F highlightRect = {
						htm.left,
						htm.top,
						(htm.left + htm.width),
						(htm.top + htm.height)
					};

					m_pRenderTarget->FillRectangle(highlightRect, brush);
				}
				SAFERELEASE(brush);
			}
			m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		}

		// Draw our caret onto the render target.
		D2D1_RECT_F caretRect;
		GetCaretRect(n, caretRect);
		m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);
		if (SUCCEEDED(hr))
		{
			m_pRenderTarget->FillRectangle(caretRect, brush);

			m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

			// Draw text
			D2D1_POINT_2F  origin = { pageRect.left, pageRect.top };
			m_pRenderTarget->DrawTextLayout(origin, n->pTextLayout, brush);

			SAFERELEASE(brush);
		}

#if 0
		// Draw the selection ranges in front of images.
		// This shades otherwise opaque images so they are visibly selected,
		// checking the isText field of the hit-test metrics.
		if (actualHitTestCount > 0)
		{
			// Note that an ideal layout will return fractional values,
			// so you may see slivers between the selection ranges due
			// to the per-primitive antialiasing of the edges unless
			// it is disabled (better for performance anyway).
			target.SetAntialiasing(false);

			for (size_t i = 0; i < actualHitTestCount; ++i)
			{
				const DWRITE_HIT_TEST_METRICS& htm = hitTestMetrics[i];
				if (htm.isText)
					continue; // Only draw selection if not text.

				RectF highlightRect = {
					htm.left,
					htm.top,
					(htm.left + htm.width),
					(htm.top + htm.height)
				};

				target.FillRectangle(highlightRect, *imageSelectionEffect_);
			}

			target.SetAntialiasing(true);
		}
#endif
		// Restore transform
		m_pRenderTarget->SetTransform(previousTransform);

#if 0
		ID2D1SolidColorBrush* brush = NULL;

		hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);

		if (!SUCCEEDED(hr)) return;

		//		D2D1_RECT_F layoutRect = D2D1::RectF(n->x, n->y, 400, 200);
		//		target->DrawText((const WCHAR *)n->data, n->len, d2d.pTextFormat, layoutRect, brush);

		D2D1_POINT_2F  origin = { 0 };
		m_pRenderTarget->DrawTextLayout(origin, n->pTextLayout, brush);

		brush->Release();
		brush = NULL;
#endif 
	}

	void DrawMSPImage(D2DRenderNode n)
	{
		HRESULT hr = S_OK;
		ID2D1Bitmap* bmp = NULL;

		if (NULL == n->pConverter) return;

		POINT pt;
		GetScrollOffset(pt);

		D2D1_RECT_F srcRect = D2D1::RectF(
			static_cast<float>(-pt.x),
			static_cast<float>(-pt.y),
			static_cast<float>(n->std.width - pt.x),
			static_cast<float>(n->std.height - pt.y));

		if (0 && n->am.frameCount > 1)
		{
			// Get the bitmap to draw on the hwnd render target
			//hr = m_pFrameComposeRT->GetBitmap(&bmp);
			IWICBitmapFrameDecode* pf;
			IWICFormatConverter* pc;
			hr = n->pDecoder->GetFrame(m_Am.frameIndex, &(pf));
			m_Am.frameIndex++;
			if (m_Am.frameIndex >= m_Am.frameCount) m_Am.frameIndex = 0;
			hr = d2d.pIWICFactory->CreateFormatConverter(&(pc));
			pc->Initialize(pf, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeMedianCut);
			hr = m_pRenderTarget->CreateBitmapFromWicBitmap(pc, NULL, &bmp);
			SAFERELEASE(pf);
			SAFERELEASE(pc);
		}
		else
		{
			hr = m_pRenderTarget->CreateBitmapFromWicBitmap(n->pConverter, NULL, &bmp);
		}

		if (SUCCEEDED(hr))
		{
			// Draw the bitmap onto the calculated rectangle
			m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
			m_pRenderTarget->DrawBitmap(bmp, &srcRect);
		}

		SAFERELEASE(bmp);
	}

	void DoPaint(CDCHandle dc)
	{
		HRESULT hr = CreateDeviceResources();
		if (FAILED(hr)) return;

		// Only render when the window is not occluded
		if ((m_pRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED)) return;

		if(NULL == d2d.pData)
		{
			DrawDefault();
			return;
		}

		D2DRenderNode	n = d2d.pData;
		
		m_pRenderTarget->BeginDraw();
		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
		while(NULL != n)
		{
			switch(n->std.type)
			{
				case MSP_TYPE_GRAPHIC	:	
						DrawMSPGraphic(n);
						break;
				case MSP_TYPE_TEXT		:	
						DrawMSPText(n);
						break;
				case MSP_TYPE_IMAGE		:
						DrawMSPImage(n);
						break;
				default					:	
						break;
			}
			n = (D2DRenderNode)n->std.next;
		}

		hr = m_pRenderTarget->EndDraw();
		
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			SAFERELEASE(m_pRenderTarget);
		}			
	}


	BEGIN_MSG_MAP(CView)
		CHAIN_MSG_MAP(CScrollWindowImpl<CView>)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMousePress)
		MESSAGE_HANDLER(WM_RBUTTONDOWN, OnMousePress)
		MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMousePress)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnMousePress)
		MESSAGE_HANDLER(WM_MBUTTONDBLCLK, OnMousePress)
		MESSAGE_HANDLER(WM_RBUTTONDBLCLK, OnMousePress)
		MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseExit)
		MESSAGE_HANDLER(WM_CAPTURECHANGED, OnMouseExit)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnMouseRelease)
		MESSAGE_HANDLER(WM_RBUTTONUP, OnMouseRelease)
		MESSAGE_HANDLER(WM_MBUTTONUP, OnMouseRelease)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
		MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyPress)
		//MESSAGE_HANDLER(WM_CHAR, OnKeyCharacter)
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
////////////////////////////////////////////////////////////////////////////////////////////////////
	LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return FALSE; // don't want flicker
	}

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

	LRESULT OnMouseExit(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		m_currentlySelecting = FALSE;
		m_currentlyPanning = FALSE;
		return 0;
	}

	LRESULT OnLBtnDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		::PostMessage(GetTopLevelParent(), WM_NCLBUTTONDOWN, HTCAPTION, lParam);
		return 0;
	}

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		//ComposeNextFrame();
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

	void AlignCaretToNearestCluster(BOOL isTrailingHit, BOOL skipZeroWidth)
	{
		// Uses hit-testing to align the current caret position to a whole cluster,
		// rather than residing in the middle of a base character + diacritic,
		// surrogate pair, or character + UVS.
		DWRITE_HIT_TEST_METRICS hitTestMetrics;
		float caretX, caretY;

		// Align the caret to the nearest whole cluster.
		IDWriteTextLayout* tlt = (d2d.pData)->pTextLayout;
		if (NULL != tlt)
		{
			tlt->HitTestTextPosition(
				m_caretPosition,
				FALSE,
				&caretX,
				&caretY,
				&hitTestMetrics
			);

			// The caret position itself is always the leading edge.
			// An additional offset indicates a trailing edge when non-zero.
			// This offset comes from the number of code-units in the
			// selected cluster or surrogate pair.
			m_caretPosition = hitTestMetrics.textPosition;
			m_caretPositionOffset = (isTrailingHit) ? hitTestMetrics.length : 0;

			// For invisible, zero-width characters (like line breaks
			// and formatting characters), force leading edge of the
			// next position.
			if (skipZeroWidth && hitTestMetrics.width == 0)
			{
				m_caretPosition += m_caretPositionOffset;
				m_caretPositionOffset = 0;
			}
		}
	}

	void GetLineMetrics(OUT std::vector<DWRITE_LINE_METRICS>& lineMetrics)
	{
		// Retrieves the line metrics, used for caret navigation, up/down and home/end.
		DWRITE_TEXT_METRICS textMetrics;

		IDWriteTextLayout* tlt = (d2d.pData)->pTextLayout;

		tlt->GetMetrics(&textMetrics);

		lineMetrics.resize(textMetrics.lineCount);
		tlt->GetLineMetrics(&lineMetrics.front(), textMetrics.lineCount, &textMetrics.lineCount);
	}

	void GetLineFromPosition(const DWRITE_LINE_METRICS* lineMetrics, // [lineCount]
							UINT32 lineCount,UINT32 textPosition, OUT UINT32* lineOut, OUT UINT32* linePositionOut)
	{
		// Given the line metrics, determines the current line and starting text
		// position of that line by summing up the lengths. When the starting
		// line position is beyond the given text position, we have our line.

		UINT32 line = 0;
		UINT32 linePosition = 0;
		UINT32 nextLinePosition = 0;
		for (; line < lineCount; ++line)
		{
			linePosition = nextLinePosition;
			nextLinePosition = linePosition + lineMetrics[line].length;
			if (nextLinePosition > textPosition)
			{
				// The next line is beyond the desired text position,
				// so it must be in the current line.
				break;
			}
		}
		*linePositionOut = linePosition;
		*lineOut = std::min(line, lineCount - 1);
		return;
	}

	void UpdateSystemCaret(const D2D1_RECT_F& rect)
	{
		// Moves the system caret to a new position.
		// Although we don't actually use the system caret (drawing our own
		// instead), this is important for accessibility, so the magnifier
		// can follow text we type. The reason we draw our own directly
		// is because intermixing DirectX and GDI content (the caret) reduces
		// performance.

		// Gets the current caret position (in untransformed space).
		if (GetFocus() != m_hWnd) // Only update if we have focus.
			return;

		D2D1::Matrix3x2F pageTransform;
		GetViewMatrix((DWRITE_MATRIX*) &pageTransform);

		// Transform caret top/left and size according to current scale and origin.
		D2D1_POINT_2F caretPoint = pageTransform.TransformPoint(D2D1::Point2F(rect.left, rect.top));

		float width = (rect.right - rect.left);
		float height = (rect.bottom - rect.top);
		float transformedWidth = width * pageTransform._11 + height * pageTransform._21;
		float transformedHeight = width * pageTransform._12 + height * pageTransform._22;

		// Update the caret's location, rounding to nearest integer so that
		// it lines up with the text selection.

		int intX = RoundToInt(caretPoint.x);
		int intY = RoundToInt(caretPoint.y);
		int intWidth = RoundToInt(transformedWidth);
		int intHeight = RoundToInt(caretPoint.y + transformedHeight) - intY;

		CreateSolidCaret(intWidth, intHeight);
		SetCaretPos(intX, intY);

		// Don't actually call ShowCaret. It's enough to just set its position.
	}

	BOOL SetSelection(SetSelectionMode moveMode, UINT32 advance, BOOL extendSelection, BOOL updateCaretFormat)
	{
		// Moves the caret relatively or absolutely, optionally extending the
		// selection range (for example, when shift is held).
		UINT32 line = UINT32_MAX; // current line number, needed by a few modes
		UINT32 absolutePosition = m_caretPosition + m_caretPositionOffset;
		UINT32 oldAbsolutePosition = absolutePosition;
		UINT32 oldCaretAnchor = m_caretAnchor;

		IDWriteTextLayout* tlt = (d2d.pData)->pTextLayout;

		switch (moveMode)
		{
		case SetSelectionModeLeft:
			m_caretPosition += m_caretPositionOffset;
			if (m_caretPosition > 0)
			{
				--m_caretPosition;
				AlignCaretToNearestCluster(FALSE, TRUE);

				// special check for CR/LF pair
#if 0
				absolutePosition = m_caretPosition + m_caretPositionOffset;
				if (absolutePosition >= 1
					&& absolutePosition < text_.size()
					&& text_[absolutePosition - 1] == '\r'
					&& text_[absolutePosition] == '\n')
				{
					m_caretPosition = absolutePosition - 1;
					AlignCaretToNearestCluster(FALSE, TRUE);
				}
#endif
			}
			break;

		case SetSelectionModeRight:
			m_caretPosition = absolutePosition;
			AlignCaretToNearestCluster(TRUE, TRUE);
#if 0
			// special check for CR/LF pair
			absolutePosition = caretPosition_ + caretPositionOffset_;
			if (absolutePosition >= 1
				&& absolutePosition < text_.size()
				&& text_[absolutePosition - 1] == '\r'
				&& text_[absolutePosition] == '\n')
			{
				caretPosition_ = absolutePosition + 1;
				AlignCaretToNearestCluster(false, true);
			}
#endif
			break;

		case SetSelectionModeLeftChar:
			m_caretPosition = absolutePosition;
			m_caretPosition -= std::min(advance, absolutePosition);
			m_caretPositionOffset = 0;
			break;

		case SetSelectionModeRightChar:
			m_caretPosition = absolutePosition + advance;
			m_caretPositionOffset = 0;
			{
				// Use hit-testing to limit text position.
				DWRITE_HIT_TEST_METRICS hitTestMetrics;
				float caretX, caretY;
				tlt->HitTestTextPosition(
					m_caretPosition,
					FALSE,
					&caretX,
					&caretY,
					&hitTestMetrics
				);
				m_caretPosition = std::min(m_caretPosition, hitTestMetrics.textPosition + hitTestMetrics.length);
			}
			break;

		case SetSelectionModeUp:
		case SetSelectionModeDown:
		{
			// Retrieve the line metrics to figure out what line we are on.
			std::vector<DWRITE_LINE_METRICS> lineMetrics;
			GetLineMetrics(lineMetrics);

			UINT32 linePosition;
			GetLineFromPosition(
				&lineMetrics.front(),
				static_cast<UINT32>(lineMetrics.size()),
				m_caretPosition,
				&line,
				&linePosition
			);

			// Move up a line or down
			if (moveMode == SetSelectionModeUp)
			{
				if (line <= 0)
					break; // already top line
				line--;
				linePosition -= lineMetrics[line].length;
			}
			else
			{
				linePosition += lineMetrics[line].length;
				line++;
				if (line >= lineMetrics.size())
					break; // already bottom line
			}

			// To move up or down, we need three hit-testing calls to determine:
			// 1. The x of where we currently are.
			// 2. The y of the new line.
			// 3. New text position from the determined x and y.
			// This is because the characters are variable size.

			DWRITE_HIT_TEST_METRICS hitTestMetrics;
			float caretX, caretY, dummyX;

			// Get x of current text position
			tlt->HitTestTextPosition(
				m_caretPosition,
				m_caretPositionOffset > 0, // trailing if nonzero, else leading edge
				&caretX,
				&caretY,
				&hitTestMetrics
			);

			// Get y of new position
			tlt->HitTestTextPosition(
				linePosition,
				FALSE, // leading edge
				&dummyX,
				&caretY,
				&hitTestMetrics
			);

			// Now get text position of new x,y.
			BOOL isInside, isTrailingHit;
			tlt->HitTestPoint(
				caretX,
				caretY,
				&isTrailingHit,
				&isInside,
				&hitTestMetrics
			);

			m_caretPosition = hitTestMetrics.textPosition;
			m_caretPositionOffset = isTrailingHit ? (hitTestMetrics.length > 0) : 0;
		}
		break;

		case SetSelectionModeLeftWord:
		case SetSelectionModeRightWord:
		{
			// To navigate by whole words, we look for the canWrapLineAfter
			// flag in the cluster metrics.

			// First need to know how many clusters there are.
			std::vector<DWRITE_CLUSTER_METRICS> clusterMetrics;
			UINT32 clusterCount;
			tlt->GetClusterMetrics(NULL, 0, &clusterCount);

			if (clusterCount == 0)
				break;

			// Now we actually read them.
			clusterMetrics.resize(clusterCount);
			tlt->GetClusterMetrics(&clusterMetrics.front(), clusterCount, &clusterCount);

			m_caretPosition = absolutePosition;

			UINT32 clusterPosition = 0;
			UINT32 oldCaretPosition = m_caretPosition;

			if (moveMode == SetSelectionModeLeftWord)
			{
				// Read through the clusters, keeping track of the farthest valid
				// stopping point just before the old position.
				m_caretPosition = 0;
				m_caretPositionOffset = 0; // leading edge
				for (UINT32 cluster = 0; cluster < clusterCount; ++cluster)
				{
					clusterPosition += clusterMetrics[cluster].length;
					if (clusterMetrics[cluster].canWrapLineAfter)
					{
						if (clusterPosition >= oldCaretPosition)
							break;

						// Update in case we pass this point next loop.
						m_caretPosition = clusterPosition;
					}
				}
			}
			else // SetSelectionModeRightWord
			{
				// Read through the clusters, looking for the first stopping point
				// after the old position.
				for (UINT32 cluster = 0; cluster < clusterCount; ++cluster)
				{
					UINT32 clusterLength = clusterMetrics[cluster].length;
					m_caretPosition = clusterPosition;
					m_caretPositionOffset = clusterLength; // trailing edge
					if (clusterPosition >= oldCaretPosition && clusterMetrics[cluster].canWrapLineAfter)
						break; // first stopping point after old position.

					clusterPosition += clusterLength;
				}
			}
		}
		break;

		case SetSelectionModeHome:
		case SetSelectionModeEnd:
		{
			// Retrieve the line metrics to know first and last position
			// on the current line.
			std::vector<DWRITE_LINE_METRICS> lineMetrics;
			GetLineMetrics(lineMetrics);

			GetLineFromPosition(
				&lineMetrics.front(),
				static_cast<UINT32>(lineMetrics.size()),
				m_caretPosition,
				&line,
				&m_caretPosition
			);

			m_caretPositionOffset = 0;
			if (moveMode == SetSelectionModeEnd)
			{
				// Place the caret at the last character on the line,
				// excluding line breaks. In the case of wrapped lines,
				// newlineLength will be 0.
				UINT32 lineLength = lineMetrics[line].length - lineMetrics[line].newlineLength;
				m_caretPositionOffset = std::min(lineLength, 1u);
				m_caretPosition += lineLength - m_caretPositionOffset;
				AlignCaretToNearestCluster(TRUE, TRUE);
			}
		}
		break;

		case SetSelectionModeFirst:
			m_caretPosition = 0;
			m_caretPositionOffset = 0;
			break;

		case SetSelectionModeAll:
			m_caretAnchor = 0;
			extendSelection = true;
			__fallthrough;

		case SetSelectionModeLast:
			m_caretPosition = UINT32_MAX;
			m_caretPositionOffset = 0;
			AlignCaretToNearestCluster(TRUE, TRUE);
			break;

		case SetSelectionModeAbsoluteLeading:
			m_caretPosition = advance;
			m_caretPositionOffset = 0;
			break;

		case SetSelectionModeAbsoluteTrailing:
			m_caretPosition = advance;
			AlignCaretToNearestCluster(TRUE, TRUE);
			break;
		}

		absolutePosition = m_caretPosition + m_caretPositionOffset;

		if (!extendSelection)
			m_caretAnchor = absolutePosition;

		bool caretMoved = (absolutePosition != oldAbsolutePosition)
			|| (m_caretAnchor != oldCaretAnchor);

		if (caretMoved)
		{
			// update the caret formatting attributes
			if (updateCaretFormat)	UpdateCaretFormatting();

			InvalidateRect(NULL, FALSE);

			D2D1_RECT_F rect;
			GetCaretRect(d2d.pData, rect);
			UpdateSystemCaret(rect);
		}

		return caretMoved;
	}

	void CopyToClipboard()
	{
		// Copies selected text to clipboard.
		DWRITE_TEXT_RANGE selectionRange = GetSelectionRange(d2d.pData);
		if (selectionRange.length <= 0)
			return;

		// Open and empty existing contents.
		if (OpenClipboard())
		{
			if (EmptyClipboard())
			{
				// Allocate room for the text
				size_t byteSize = sizeof(wchar_t) * (selectionRange.length + 1);
				HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, byteSize);

				if (hClipboardData != NULL)
				{
					void* memory = GlobalLock(hClipboardData);  // [byteSize] in bytes

					if (memory != NULL)
					{
						// Copy text to memory block.
						const wchar_t* text = (wchar_t*)(d2d.pData)->std.data;
						memcpy(memory, &text[selectionRange.startPosition], byteSize);
						GlobalUnlock(hClipboardData);

						if (SetClipboardData(CF_UNICODETEXT, hClipboardData) != NULL)
						{
							hClipboardData = NULL; // system now owns the clipboard, so don't touch it.
						}
					}
					GlobalFree(hClipboardData); // free if failed
				}
			}
			CloseClipboard();
		}
	}

	LRESULT OnKeyPress(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		UINT32 keyCode = wParam;
		// Handles caret navigation and special presses that
		// do not generate characters.

		if (NULL == d2d.pData) return 0;

		bool heldShift = (GetKeyState(VK_SHIFT) & 0x80) != 0;
		bool heldControl = (GetKeyState(VK_CONTROL) & 0x80) != 0;

		UINT32 absolutePosition = m_caretPosition + m_caretPositionOffset;

		switch (keyCode)
		{
		case VK_TAB:
			break; // want tabs

		case VK_LEFT: // seek left one cluster
			SetSelection(heldControl ? SetSelectionModeLeftWord : SetSelectionModeLeft, 1, heldShift, TRUE);
			break;

		case VK_RIGHT: // seek right one cluster
			SetSelection(heldControl ? SetSelectionModeRightWord : SetSelectionModeRight, 1, heldShift, TRUE);
			break;

		case VK_UP: // up a line
			SetSelection(SetSelectionModeUp, 1, heldShift, TRUE);
			break;

		case VK_DOWN: // down a line
			SetSelection(SetSelectionModeDown, 1, heldShift, TRUE);
			break;

		case VK_HOME: // beginning of line
			SetSelection(heldControl ? SetSelectionModeFirst : SetSelectionModeHome, 0, heldShift, TRUE);
			break;

		case VK_END: // end of line
			SetSelection(heldControl ? SetSelectionModeLast : SetSelectionModeEnd, 0, heldShift, TRUE);
			break;

		case 'C':
			if (heldControl)
				CopyToClipboard();
			break;

		case 'A':
			if (heldControl)
				SetSelection(SetSelectionModeAll, 0, TRUE, TRUE);
			break;
		}

		return 0;
	}

	void MirrorXCoordinate(IN OUT float& x)
	{
		// On RTL builds, coordinates may need to be restored to or converted
		// from Cartesian coordinates, where x increases positively to the right.
		if (GetWindowLong(GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
		{
			RECT rect;
			GetClientRect(&rect);
			x = float(rect.right) - x - 1;
		}
	}

	BOOL SetSelectionFromPoint(float x, float y, bool extendSelection)
	{
		// Returns the text position corresponding to the mouse x,y.
		// If hitting the trailing side of a cluster, return the
		// leading edge of the following text position.
		BOOL isTrailingHit;
		BOOL isInside;
		DWRITE_HIT_TEST_METRICS caretMetrics;

		// Remap display coordinates to actual.
		DWRITE_MATRIX matrix;
		GetInverseViewMatrix(&matrix);

		float transformedX = (x * matrix.m11 + y * matrix.m21 + matrix.dx);
		float transformedY = (x * matrix.m12 + y * matrix.m22 + matrix.dy);

		IDWriteTextLayout* tlt = (d2d.pData)->pTextLayout;

		HRESULT hr = tlt->HitTestPoint(transformedX, transformedY, &isTrailingHit,&isInside, &caretMetrics);

		// Update current selection according to click or mouse drag.
		SetSelection(
			isTrailingHit ? SetSelectionModeAbsoluteTrailing : SetSelectionModeAbsoluteLeading,
			caretMetrics.textPosition,
			extendSelection, TRUE
		);

		return true;
	}

	LRESULT OnMouseRelease(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		ReleaseCapture();

		if (NULL == d2d.pData) return 0;

		float x = float(GET_X_LPARAM(lParam));
		float y = float(GET_Y_LPARAM(lParam));

		MirrorXCoordinate(x);

		if (WM_LBUTTONUP == uMsg)
		{
			m_currentlySelecting = FALSE;
		}
		else if (WM_MBUTTONUP == uMsg)
		{
			m_currentlyPanning = FALSE;
		}

		return 0;
	}

	void UpdateScrollInfo()
	{
		// Updates scroll bars.
		// Determine scroll bar's step size in pixels by multiplying client rect by current view.
		RECT clientRect;
		GetClientRect(&clientRect);

		D2D1::Matrix3x2F pageTransform;
		GetInverseViewMatrix((DWRITE_MATRIX*)&pageTransform);

		// Transform vector of viewport size
		D2D1_POINT_2F clientSize = { float(clientRect.right), float(clientRect.bottom) };
		D2D1_POINT_2F scaledSize = { clientSize.x * pageTransform._11 + clientSize.y * pageTransform._21,
									clientSize.x * pageTransform._12 + clientSize.y * pageTransform._22 };

		float x = m_originX;
		float y = m_originY;
		D2D1_POINT_2F pageSize = GetPageSize();

		SCROLLINFO scrollInfo = { sizeof(scrollInfo) };
		scrollInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;

		if (IsLandscapeAngle(m_angle))
		{
			std::swap(x, y);
			std::swap(pageSize.x, pageSize.y);
			std::swap(scaledSize.x, scaledSize.y);
		}

		// Set vertical scroll bar.
		scrollInfo.nPage = int(abs(scaledSize.y));
		scrollInfo.nPos = int(scaledSize.y >= 0 ? y : pageSize.y - y);
		scrollInfo.nMin = 0;
		scrollInfo.nMax = int(pageSize.y) + scrollInfo.nPage;
		SetScrollInfo(SB_VERT, &scrollInfo, TRUE);
		scrollInfo.nPos = 0;
		scrollInfo.nMax = 0;
		GetScrollInfo(SB_VERT, &scrollInfo);

		// Set horizontal scroll bar.
		scrollInfo.nPage = int(abs(scaledSize.x));
		scrollInfo.nPos = int(scaledSize.x >= 0 ? x : pageSize.x - x);
		scrollInfo.nMin = 0;
		scrollInfo.nMax = int(pageSize.x) + scrollInfo.nPage;
		SetScrollInfo(SB_HORZ, &scrollInfo, TRUE);
	}

	void ConstrainViewOrigin()
	{
		// Keep the page on-screen by not allowing the origin
		// to go outside the page bounds.
		D2D1_POINT_2F pageSize = GetPageSize();

		if (m_originX > pageSize.x)	m_originX = pageSize.x;
		if (m_originX < 0) m_originX = 0;

		if (m_originY > pageSize.y) m_originY = pageSize.y;
		if (m_originY < 0) m_originY = 0;
	}

	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (NULL == d2d.pData) return 0;
		// Selects text or pans.
		float x = float(GET_X_LPARAM(lParam));
		float y = float(GET_Y_LPARAM(lParam));

		MirrorXCoordinate(x);

		if (m_currentlySelecting)
		{
			// Drag current selection.
			SetSelectionFromPoint(x, y, true);
		}
		else if (m_currentlyPanning)
		{
			DWRITE_MATRIX matrix;
			GetInverseViewMatrix(&matrix);

			float xDif = x - m_previousMouseX;
			float yDif = y - m_previousMouseY;
			m_previousMouseX = x;
			m_previousMouseY = y;

			m_originX -= (xDif * matrix.m11 + yDif * matrix.m21);
			m_originY -= (xDif * matrix.m12 + yDif * matrix.m22);

			ConstrainViewOrigin();
			UpdateScrollInfo();
			InvalidateRect(NULL, FALSE); 
		}
		return 0;
	}

	LRESULT OnMousePress(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		float x, y;

		if (WM_LBUTTONDOWN == uMsg)
		{
			::PostMessage(GetTopLevelParent(), WM_NCLBUTTONDOWN, HTCAPTION, lParam);
		}
		if (NULL == d2d.pData) return 0;
		if (MSP_TYPE_IMAGE == d2d.pData->std.type) return 0;
		
		SetFocus();
		SetCapture();

		x = float(GET_X_LPARAM(lParam));
		y = float(GET_Y_LPARAM(lParam));

		MirrorXCoordinate(x);

		if (WM_LBUTTONDOWN == uMsg)
		{
			// Start dragging selection.
			m_currentlySelecting = true;
			bool heldShift = (GetKeyState(VK_SHIFT) & 0x80) != 0;
			SetSelectionFromPoint(x, y, heldShift);
		}
		else if (WM_MBUTTONDOWN == uMsg)
		{
			m_previousMouseX = x;
			m_previousMouseY = y;
			m_currentlyPanning = true;
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

	void GetViewMatrix(OUT DWRITE_MATRIX* matrix) const
	{
		// Generates a view matrix from the current origin, angle, and scale.
		// Need the editor size for centering.
		RECT rect;
		GetClientRect(&rect);

		// Translate the origin to 0,0
		DWRITE_MATRIX translationMatrix = {
			1, 0,
			0, 1,
			-m_originX, -m_originY
		};

		// Scale and rotate
#if 0
		double radians = DegreesToRadians(fmod(m_angle, 360.0f));
		double cosValue = cos(radians);
		double sinValue = sin(radians);

		// If rotation is a quarter multiple, ensure sin and cos are exactly one of {-1,0,1}
		if (fmod(m_angle, 90.0f) == 0)
		{
			cosValue = floor(cosValue + .5);
			sinValue = floor(sinValue + .5);
		}
#endif
		double cosValue = 1;
		double sinValue = 0;
		DWRITE_MATRIX rotationMatrix = {
			float(cosValue * m_scaleX), float(sinValue * m_scaleX),
			float(-sinValue * m_scaleY), float(cosValue * m_scaleY),
			0, 0
		};

		// Set the origin in the center of the window
		float centeringFactor = .5f;
		DWRITE_MATRIX centerMatrix = {
			1, 0,
			0, 1,
			floor(float(rect.right * centeringFactor)), floor(float(rect.bottom * centeringFactor))
		};

		D2D1::Matrix3x2F resultA, resultB;

		resultB.SetProduct(Cast(translationMatrix), Cast(rotationMatrix));
		resultA.SetProduct(resultB, Cast(centerMatrix));

		// For better pixel alignment (less blurry text)
		resultA._31 = floor(resultA._31);
		resultA._32 = floor(resultA._32);

		*matrix = *reinterpret_cast<DWRITE_MATRIX*>(&resultA);
	}

	void GetInverseViewMatrix(OUT DWRITE_MATRIX* matrix) const
	{
		// Inverts the view matrix for hit-testing and scrolling.
		DWRITE_MATRIX viewMatrix;
		GetViewMatrix(&viewMatrix);
		ComputeInverseMatrix(viewMatrix, *matrix);
	}

	D2D1_POINT_2F GetPageSize()
	{
		// Use the layout metrics to determine how large the page is, taking
		// the maximum of the content size and layout's maximal dimensions.
		D2DRenderNode	n;
		D2D1_POINT_2F pageSize = { 0, 0 };
		n = d2d.pData;
		if (NULL != n)
		{
			pageSize.x = n->std.width;
			pageSize.y = n->std.height;
#if 0
			if (NULL != n->pTextLayout)
			{
				DWRITE_TEXT_METRICS textMetrics;
				HRESULT hr = n->pTextLayout->GetMetrics(&textMetrics);
				if (SUCCEEDED(hr))
				{
					float width = std::max(textMetrics.layoutWidth, textMetrics.left + textMetrics.width);
					float height = std::max(textMetrics.layoutHeight, textMetrics.height);
					pageSize.x = width;
					pageSize.y = height;
				}
			}
#endif
		}

		return pageSize;
	}

#if 0
	void ConstrainViewOrigin()
	{
		// Keep the page on-screen by not allowing the origin
		// to go outside the page bounds.
		D2D1_POINT_2F pageSize = GetPageSize(textLayout_);

		if (m_originX > pageSize.x) m_originX = pageSize.x;
		if (m_originX < 0) m_originX = 0;

		if (m_originY > pageSize.y) m_originY = pageSize.y;
		if (m_originY < 0) m_originY = 0;
	}
#endif
	void OnScroll(UINT message, UINT request)
	{
		SCROLLINFO scrollInfo = { sizeof(scrollInfo) };
		scrollInfo.fMask = SIF_ALL;

		int barOrientation = (message == WM_VSCROLL) ? SB_VERT : SB_HORZ;

		if (!GetScrollInfo(barOrientation, &scrollInfo)) return;

		// Save the position for comparison later on
		int oldPosition = scrollInfo.nPos;

		switch (request)
		{
		case SB_TOP:        scrollInfo.nPos = scrollInfo.nMin;		break;
		case SB_BOTTOM:     scrollInfo.nPos = scrollInfo.nMax;      break;
		case SB_LINEUP:     scrollInfo.nPos -= 10;                  break;
		case SB_LINEDOWN:   scrollInfo.nPos += 10;                  break;
		case SB_PAGEUP:     scrollInfo.nPos -= scrollInfo.nPage;    break;
		case SB_PAGEDOWN:   scrollInfo.nPos += scrollInfo.nPage;    break;
		case SB_THUMBTRACK: scrollInfo.nPos = scrollInfo.nTrackPos; break;
		default:
			break;
		}

		if (scrollInfo.nPos < 0) scrollInfo.nPos = 0;
		if (scrollInfo.nPos > scrollInfo.nMax - signed(scrollInfo.nPage))
			scrollInfo.nPos = scrollInfo.nMax - scrollInfo.nPage;

		scrollInfo.fMask = SIF_POS;
		SetScrollInfo(barOrientation, &scrollInfo, TRUE);

		// If the position has changed, scroll the window 
		if (scrollInfo.nPos != oldPosition)
		{
			// Need the view matrix in case the editor is flipped/mirrored/rotated.
			//D2D1::Matrix3x2F pageTransform;
			DWRITE_MATRIX pageTransform;
			GetInverseViewMatrix(&pageTransform);

			float inversePos = float(scrollInfo.nMax - scrollInfo.nPage - scrollInfo.nPos);

			D2D1_POINT_2F scaledSize = { pageTransform.m11 + pageTransform.m21,
										pageTransform.m12 + pageTransform.m22 };

			// Adjust the correct origin.
			if ((barOrientation == SB_VERT)) // ^ IsLandscapeAngle(m_angle))
			{
				m_originY = float(scaledSize.y >= 0 ? scrollInfo.nPos : inversePos);
			}
			else
			{
				m_originX = float(scaledSize.x >= 0 ? scrollInfo.nPos : inversePos);
			}

			ConstrainViewOrigin();
			InvalidateRect(NULL, FALSE);
		}
	}

	LRESULT OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		OnScroll(uMsg, LOWORD(wParam));
		return 0;
	}

	LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		OnScroll(uMsg, LOWORD(wParam));
		return 0;
	}

	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		return 0;
	}

	LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
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

	void UpdateCaretFormatting()
	{
		UINT32 currentPos = m_caretPosition + m_caretPositionOffset;

		IDWriteTextLayout* textLayout = d2d.pData->pTextLayout;
		if (currentPos > 0)
		{
			--currentPos; // Always adopt the trailing properties.
		}

		// Get the family name
		m_caretFormat.fontFamilyName[0] = '\0';
		textLayout->GetFontFamilyName(currentPos, &m_caretFormat.fontFamilyName[0], ARRAYSIZE(m_caretFormat.fontFamilyName));

		// Get the locale
		m_caretFormat.localeName[0] = '\0';
		textLayout->GetLocaleName(currentPos, &m_caretFormat.localeName[0], ARRAYSIZE(m_caretFormat.localeName));

		// Get the remaining attributes...
		textLayout->GetFontWeight(currentPos, &m_caretFormat.fontWeight);
		textLayout->GetFontStyle(currentPos, &m_caretFormat.fontStyle);
		textLayout->GetFontStretch(currentPos, &m_caretFormat.fontStretch);
		textLayout->GetFontSize(currentPos, &m_caretFormat.fontSize);
		textLayout->GetUnderline(currentPos, &m_caretFormat.hasUnderline);
		textLayout->GetStrikethrough(currentPos, &m_caretFormat.hasStrikethrough);

		// Get the current color.
		m_caretFormat.color = 0;
#if 0
		IUnknown* drawingEffect = NULL;
		textLayout->GetDrawingEffect(currentPos, &drawingEffect);
		if (drawingEffect != NULL)
		{
			DrawingEffect& effect = *reinterpret_cast<DrawingEffect*>(drawingEffect);
			caretFormat.color = effect.GetColor();
		}

		SafeRelease(&drawingEffect);
#endif
	}

	void InitMarkDownLayout()
	{
#if 0
		D2DRenderNode	n;
		float layoutWidth;
		float layoutHeight;

		EnterCriticalSection(&(d2d.cs));
		n = d2d.pData;
		if (NULL != n)
		{
			layoutWidth = n->pTextLayout->GetMaxWidth();
			layoutHeight = n->pTextLayout->GetMaxHeight();

			m_originX = layoutWidth / 2;
			m_originY = layoutHeight / 2;
		}
		LeaveCriticalSection(&(d2d.cs));
#endif
	}

	LRESULT OnUINotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		UINT32 w = 1, h = 1;

		if (UI_NOTIFY_FILEOPEN != wParam)
		{
			return 0;
		}

		D2DRenderNode n = d2d.pData;
		EnterCriticalSection(&(d2d.cs));
			d2d.pData = d2d.pData0;
			d2d.pData0 = NULL;
		LeaveCriticalSection(&(d2d.cs));

		ReleaseD2DResource(n);
		if (NULL == d2d.pData) return 0;

		SetScrollSize((d2d.pData)->std.width, (d2d.pData)->std.height);

		n = d2d.pData;

		if(m_timerStart) {
			KillTimer(ANIMATION_TIMER_ID);
			m_timerStart = FALSE;
		}

		fileType ft = (fileType)lParam;
		if (MSP_TYPE_IMAGE == n->std.type)
		{
			if (MSP_HINT_PNG & n->std.flag) ft = filePNG;
			if (MSP_HINT_GIF & n->std.flag) ft = fileGIF;
			if (MSP_HINT_BMP & n->std.flag) ft = fileBMP;
		}

		switch(ft)
		{
		case fileMD		:
			//InitMarkDownLayout();
		case fileGIF	:
			//ZeroMemory(&m_Am, sizeof(AnimationData));
			if (n->am.frameCount > 1) // it is animation GIF
			{
				m_Am = n->am;
				ComposeNextFrame();
				if (0 != SetTimer(ANIMATION_TIMER_ID, m_Am.frameDelay, NULL))
					m_timerStart = TRUE;
			}
		case filePNG	:
		case fileBMP	:
		case fileJPG	:
		case fileSVG	:
			InvalidateRect(NULL);
			UpdateWindow();
			break;
		default:
			//::PostMessage(GetTopLevelParent(), WM_UI_NOTIFY, 0, 0);
			return 0;
		}

		return 0;
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
#if 0
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
#endif
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
		D2DRenderNode n = d2d.pData;
		if (NULL == n) return hr;
		if (NULL == n->pDecoder) return hr;

		hr = n->pDecoder->GetFrame(uFrameIndex, &pWicFrame);

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
				WICBitmapPaletteTypeMedianCut);
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
#if 0
			// If we have more frames to play, set the timer according to the delay.
			// Set the timer regardless of whether we succeeded in composing a frame
			// to try our best to continue displaying the animation.
			if (!EndOfAnimation() && m_Am.frameCount > 1)
			{
				// Set the timer according to the delay
				//SetTimer(ANIMATION_TIMER_ID, m_Am.frameDelay, NULL);
			}
#endif
		}

		return hr;
	}

};
