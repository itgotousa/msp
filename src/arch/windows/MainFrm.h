// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CMainFrame : 
	public CFrameWindowImpl<CMainFrame>, 
	public CUpdateUI<CMainFrame>,
	public CMessageFilter, public CIdleHandler
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)
    
    HWND m_hWndTB;
    
	CView m_view;
	CCommandBarCtrl m_CmdBar;
    
	// Add text to a toolbar button
	int AddToolbarButtonText(HWND hWndToolBar, UINT nID, LPCTSTR lpsz)
	{
		if (::IsWindow(hWndToolBar) == FALSE) return 0;
		// Use built-in WTL toolbar wrapper class
		CToolBarCtrl toolbar(hWndToolBar);
		// Set extended style
		if ((toolbar.GetExtendedStyle() & TBSTYLE_EX_MIXEDBUTTONS) != TBSTYLE_EX_MIXEDBUTTONS)
			toolbar.SetExtendedStyle(toolbar.GetExtendedStyle() | TBSTYLE_EX_MIXEDBUTTONS);
		// Get the button index
		int nIndex = toolbar.CommandToIndex(nID);

		TBBUTTON tb = { 0 };
		toolbar.GetButton(nIndex, &tb);

		int nStringID = toolbar.AddStrings(lpsz);
		// Alter the button style
		tb.iString = nStringID;
		tb.fsStyle |= TBSTYLE_AUTOSIZE | BTNS_SHOWTEXT;
		// Delete and re-insert the button
		toolbar.DeleteButton(nIndex);
		toolbar.InsertButton(nIndex, &tb);

		return 0;
	}
    
	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
			return TRUE;

		return m_view.PreTranslateMessage(pMsg);
	}

	virtual BOOL OnIdle()
	{
		UIUpdateToolBar();
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_UI_NOTIFY, OnUINotify)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
		COMMAND_ID_HANDLER(IDM_MONITOR, OnFileMonitor)
		COMMAND_ID_HANDLER(ID_FILE_NEW_WINDOW, OnFileNewWindow)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDM_MAINMENU, OnMainMenu)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// create command bar window
		HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
		// attach menu
		m_CmdBar.AttachMenu(GetMenu());
		// load command bar images
		m_CmdBar.LoadImages(IDR_MAINFRAME);
		// remove old menu
		SetMenu(NULL);

		HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE | TBSTYLE_LIST);
        m_hWndTB = hWndToolBar;
        
		//AddToolbarButtonText(m_hWndTB, IDM_MAINMENU, _T("Menu(&M)"));

		CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
		AddSimpleReBarBand(hWndCmdBar);
		AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

		//CreateSimpleStatusBar();

		m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL, WS_EX_CLIENTEDGE);
		// replace with appropriate values for the app
		RenderRoot root = d2d.pDataDefault;
		if (NULL != root)
		{
				m_view.SetScrollSize(root->width, root->height);
		}
		else 
		{
			m_view.SetScrollSize(1, 1);
		}

		UIAddToolBar(hWndToolBar);
		UISetCheck(ID_VIEW_TOOLBAR, 1);
		//UISetCheck(ID_VIEW_STATUS_BAR, 1);

		CToolBarCtrl toolbar = m_hWndTB;
		toolbar.EnableButton(IDM_MONITOR, FALSE);
		// Get a handle to the status bar
		//CStatusBarCtrl statusBar = m_hWndStatusBar;
		// Set the text of the first pane to "Ready"
		//SetPaneText(0, _T("Ready"));

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		return 0;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		// unregister message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
		pLoop->RemoveIdleHandler(this);

		bHandled = FALSE;
		return 1;
	}

	LRESULT OnUINotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if(NULL != d2d.pData) 
		{
			CToolBarCtrl toolbar = m_hWndTB;
			toolbar.EnableButton(IDM_MONITOR, TRUE);
			toolbar.CheckButton(IDM_MONITOR, TRUE);
			SetWindowText(g_filepath);
		}
		else
		{
			SetWindowText(_T("A tiny Markdown/SVG/PNG Viewer"));
		}
		return 0;
	}

	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT OnFileMonitor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// TODO: add code to initialize document
		CToolBarCtrl toolbar = m_hWndTB;
		toolbar.CheckButton(IDM_MONITOR, g_monitor);
		g_monitor = ! g_monitor;
		return 0;
	}

	LRESULT OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		OPENFILENAME ofn = { 0 };       // common dialog box structure
		TCHAR path[MAX_PATH + 1] = { 0 };

		// Initialize OPENFILENAME
		ofn.lStructSize = sizeof(OPENFILENAMEW);
		ofn.hwndOwner = m_hWnd;
		ofn.lpstrFile = path;
		//
		// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
		// use the contents of file to initialize itself.
		//
		ofn.lpstrFile[0] = _T('\0');
		ofn.nMaxFile = MAX_PATH; //sizeof(path) / sizeof(TCHAR);
		ofn.lpstrFilter = _T("Markdown file(*.md)\0*.md\0SVG file(*.svg)\0*.svg\0PNG file(*.png)\0*.png\0All files(*.*)\0*.*\0\0");
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		/* Display the Open dialog box. */
		if (GetOpenFileName(&ofn) != TRUE) return 0;

		m_view.OpenMSPFile(path);

		return 0;
	}

	LRESULT OnFileNewWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		::PostThreadMessage(_Module.m_dwMainThreadID, WM_USER, 0, 0L);
		return 0;
	}

	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		static BOOL bVisible = TRUE;	// initially visible
		bVisible = !bVisible;
		CReBarCtrl rebar = m_hWndToolBar;
		int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
		rebar.ShowBand(nBandIndex, bVisible);
		UISetCheck(ID_VIEW_TOOLBAR, bVisible);
		UpdateLayout();
		return 0;
	}

	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
		::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
		UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
		UpdateLayout();
		return 0;
	}

	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}
    
	LRESULT OnMainMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		RECT r;
		POINT pt;
		if (::IsWindow(m_hWndTB) == FALSE) return 0;
		CToolBarCtrl tb(m_hWndTB);
		tb.GetItemRect(0, &r);
		pt.x = r.left; pt.y = r.bottom;
		tb.MapWindowPoints(HWND_DESKTOP, &pt, 1);
		CMenu menu;
		if (menu.LoadMenu(IDM_MAINMENU)) {
			CMenuHandle menuPopup = menu.GetSubMenu(0);
			ATLASSERT(menuPopup != NULL);
			// Display the menu				
			// Using command bar TrackPopupMenu method means menu icons are displayed
			menuPopup.CheckMenuItem(IDM_MONITOR, g_monitor? MF_UNCHECKED : MF_CHECKED);
			m_CmdBar.TrackPopupMenu(menuPopup, TPM_RIGHTBUTTON | TPM_VERTICAL, pt.x, pt.y);
		}
		return 0;
	}
    
};
