// View.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CView : public CScrollWindowImpl<CView>
{
public:
	DECLARE_WND_CLASS(NULL)

	CBitmap   m_bmpLogo;
	SIZE      m_szLogo;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	void DoPaint(CDCHandle dc)
	{
		//TODO: Add your drawing code here
#if 0
		SIZE sz;
		RECT rc = { 0 };
		CDC dcMem, dcTemp;
		CBitmap bmpMem;
		CBrush brush;
		HBITMAP hbmpOld, hbmpTemp;

		GetClientRect(&rc);
		GetScrollSize(sz);
		if (sz.cx > rc.right)  rc.right = sz.cx;
		if (sz.cy > rc.bottom) rc.bottom = sz.cy;

		dcMem.CreateCompatibleDC(dc);
		bmpMem.CreateCompatibleBitmap(dc, rc.right, rc.bottom);
		hbmpOld = dcMem.SelectBitmap(bmpMem);
		brush.CreateSolidBrush(RGB(255, 255, 255));
		dcMem.FillRect(&rc, brush);

		dcTemp.CreateCompatibleDC(dcMem);
		hbmpTemp = dcTemp.SelectBitmap(m_bmpLogo);
		dcMem.BitBlt((rc.right - m_szLogo.cx)>>1, (rc.bottom - m_szLogo.cy)>>1, m_szLogo.cx, m_szLogo.cy, dcTemp, 0, 0, SRCCOPY);
		dcTemp.SelectBitmap(hbmpTemp);

		dc.BitBlt(0, 0, rc.right, rc.bottom, dcMem, 0, 0, SRCCOPY);
		dcMem.SelectBitmap(hbmpOld);
#endif 		
	}

	BEGIN_MSG_MAP(CView)
		CHAIN_MSG_MAP(CScrollWindowImpl<CView>)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLBtnDown)
		MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		DragAcceptFiles(); /* accept the drag and drop file */
		//m_bmpLogo.LoadBitmap(IDR_BMP_LOGO);
		//m_bmpLogo.GetSize(m_szLogo);

		return 0;
	}

	LRESULT OnDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		TCHAR path[MAX_PATH + 1] = { 0 };

		if (DragQueryFile((HDROP)wParam, 0, path, MAX_PATH)) {

			//MessageBox(path, _T("MB_OK"), MB_OK);
			wmemcpy((wchar_t*)g_file, path, MAX_PATH);
			g_fileloaded = TRUE;
			::PostMessage(GetTopLevelParent(), WM_UI_NOTIFY, 0, 0);
		}

		return 0;
	}

	LRESULT OnLBtnDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
        ::PostMessage(GetTopLevelParent(), WM_NCLBUTTONDOWN, HTCAPTION, lParam);
		return 0;
	}

};
