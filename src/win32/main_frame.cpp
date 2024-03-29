/**
 * Copyright (C) 2010 syndicode
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 **/

#include "stdafx.h"
#include "app.h"
#include "main_frame.h"
#include "settings_dlg.h"
#include "about_dlg.h"
#include "plugin_manager.h"
#include "nfo_renderer_export.h"
#include "resource.h"
#include "default_app.h"


using namespace std;
using namespace std::placeholders;


enum _toolbar_button_ids {
	// main toolbar:
	TBBID_OPEN = 9001, // over 9000!
	TBBID_SETTINGS,
	TBBID_VIEW_RENDERED,
	TBBID_VIEW_CLASSIC,
	TBBID_VIEW_TEXTONLY,
	TBBID_ABOUT,
	TBBID_CLEARMRU,
	TBBID_SHOWMENU,
	// search toolbar:
	TBBID_SEARCH_UP,
	TBBID_SEARCH_DOWN,
	TBBID_SEARCH_CLOSE,
	TBBID_SEARCH_NOTFOUND,
	// MRU + x:
	TBBID_OPENMRUSTART // must be the last item in this list.
};

#define VIEW_MENU_POS 1
#define STATUSBAR_PANE_CHARSET 3
#define IDW_SEARCHTOOLBAR (IDW_TOOLBAR + 1)


CMainFrame::CMainFrame()
	: CFrame()
	, m_showingAbout(false)
	, m_dropHelper(nullptr)
	, m_nfoInFolderIndex((size_t)-1)
	, m_menuBarVisible(false)
	, m_searchToolbar(nullptr)
	, m_hSearchEditBox(nullptr)
{
	SetView(m_view);
}


void CMainFrame::PreCreate(CREATESTRUCT& cs)
{
	// allow some class flags to be set...
	CFrame::PreCreate(cs);

	// read saved positions and dimensions:
	PSettingsSection l_sect;

	if (CNFOApp::GetInstance()->GetSettingsBackend()->OpenSectionForReading(L"Frame Settings", l_sect))
	{
		cs.x = l_sect->ReadDword(L"Left");
		cs.y = l_sect->ReadDword(L"Top");
		cs.cx = l_sect->ReadDword(L"Width");
		cs.cy = l_sect->ReadDword(L"Height");

		m_bShowStatusbar = l_sect->ReadBool(L"Statusbar", true);
		m_bShowToolbar = l_sect->ReadBool(L"Toolbar", true);
	}

	if (cs.cx == 0 && cs.cy == 0)
	{
		// default dimensions:
		cs.cx = 630;
		cs.cy = 680;
	}

	// get work area of the target (saved) monitor
	// and perform checks based on that:
	POINT l_pt = { cs.x, cs.y };
	MONITORINFO l_moInfo = { sizeof(MONITORINFO), 0 };
	HMONITOR l_hMonitor = ::MonitorFromPoint(l_pt, MONITOR_DEFAULTTOPRIMARY);

	if (::GetMonitorInfo(l_hMonitor, &l_moInfo))
	{
		RECT l_wa = l_moInfo.rcWork;

		// don't let the window grow larger than the size of the work area
		if (cs.cx > l_wa.right - l_wa.left)
			cs.cx = l_wa.right - l_wa.left;
		if (cs.cy > l_wa.bottom - l_wa.top)
			cs.cy = l_wa.bottom - l_wa.top;

		// try to always keep the window visible
		if (cs.x >= l_wa.right - 20)
			cs.x = l_wa.right - cs.cx;
		if (cs.y >= l_wa.bottom - 100)
			cs.y = l_wa.bottom - cs.cy;

		// this check is necessary in case the monitor setup
		// has changed since the last time iNFekt ran
		if (cs.x < l_wa.left)
			cs.x = l_wa.left;
		if (cs.y < l_wa.top)
			cs.y = l_wa.top;
	}

	// enforce minimum window size:
	if (cs.cx < ms_minWidth)
		cs.cx = ms_minWidth;
	if (cs.cy < ms_minHeight)
		cs.cy = ms_minHeight;

	// hide the window to avoid flicker:
	cs.style &= ~WS_VISIBLE;

	// we need a unique class name for the "single window" functionality:
	cs.lpszClass = INFEKT_MAIN_WINDOW_CLASS_NAME;
}


void CMainFrame::OnCreate()
{
	// load settings:
	LoadOpenMruList();

	m_settings = std::make_shared<CMainSettings>(true);

	// tame Win32++:
	m_bUseThemes = FALSE;
	m_bShowIndicatorStatus = FALSE;
	m_bShowMenuStatus = FALSE;

	CFrame::OnCreate();

	SetIconLarge(IDI_APPICON);
	SetIconSmall(IDI_APPICON);
}


void CMainFrame::OnInitialUpdate()
{
	CNFOApp* l_app = CNFOApp::GetInstance();
	std::wstring l_path, l_viewMode;
	bool l_wrap, l_noGpu = false;

	if (!l_app)
		abort();

	CNFORenderer::SetGlobalUseGPUFlag(m_settings->bUseGPU);

	ShowStatusbar(m_bShowStatusbar);

	if (m_settings->bCenterWindow)
	{
		CenterWindow();
	}

	UpdateCaption();

	LoadRenderSettingsFromRegistry(_T("RenderedView"), m_view.GetRenderCtrl().get());
	LoadRenderSettingsFromRegistry(_T("ClassicView"), m_view.GetClassicCtrl().get());
	LoadRenderSettingsFromRegistry(_T("TextOnlyView"), m_view.GetTextOnlyCtrl().get());

	l_wrap = m_settings->bWrapLines;
	l_app->GetStartupOptions(l_path, l_viewMode, l_wrap, l_noGpu);

	UpdateStatusbar();

	if (!l_viewMode.empty())
	{
		// set from command line.
		if (l_viewMode == L"rendered") SwitchView(MAIN_VIEW_RENDERED);
		else if (l_viewMode == L"classic") SwitchView(MAIN_VIEW_CLASSIC);
		else if (l_viewMode == L"text") SwitchView(MAIN_VIEW_TEXTONLY);
	}
	else if (GetSettings()->iDefaultView == -1)
	{
		SwitchView((EMainView)GetSettings()->iLastView);
	}
	else
	{
		SwitchView((EMainView)GetSettings()->iDefaultView);
	}

	m_dropHelper = new (std::nothrow) CMainDropTargetHelper(m_hWnd);

	if (m_dropHelper)
	{
		m_dropHelper->Register();
	}

	PSettingsSection l_sect;
	bool l_maximize = false;

	if (l_app->GetSettingsBackend()->OpenSectionForReading(L"Frame Settings", l_sect))
	{
		l_maximize = l_sect->ReadBool(L"Maximized", false);
		l_sect.reset();
	}

	m_view.SetCopyOnSelect(m_settings->bCopyOnSelect);
	m_view.SetCenterNfo(m_settings->bCenterNFO);
	m_view.SetOnDemandRendering(m_settings->bOnDemandRendering);
	m_view.SetWrapLines(l_wrap);

	if (l_noGpu)
	{
		// will toggle on again if settings are opened + OK is used, but whatever.
		CNFORenderer::SetGlobalUseGPUFlag(false);
	}

	ShowWindow(l_maximize ? SW_MAXIMIZE : SW_SHOWNORMAL);

	if (m_settings->bCheckDefaultOnStartup)
	{
		CWinDefaultApp::Factory()->CheckDefaultNfoViewer(m_hWnd);
	}

	LoadActivePluginsFromRegistry();

	if (!l_path.empty())
	{
		::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

		OpenFile(l_path);
	}

	::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
}


void CMainFrame::SetupToolbar()
{
	// turn off Win32++ "theme":
	CRebar& RB = GetRebar();
	ThemeRebar RBTheme = RB.GetRebarTheme();
	RBTheme.UseThemes = FALSE;
	RB.SetRebarTheme(RBTheme);

	// add main window menu into a rebar:
	GetMenubar().SetMenu(::LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MAIN_MENU)));

	// always show grippers in classic themes:
	RB.ShowGripper(RB.IDToIndex(IDW_TOOLBAR), !::IsThemeActive());
	RB.ShowGripper(RB.IDToIndex(IDW_MENUBAR), FALSE);

	AddToolbarButtons();

	ShowMenuBar(m_settings->bAlwaysShowMenubar);
}


void CMainFrame::AddToolbarButtons()
{
	const int IML = 0;

	// add icons:
	enum _icon_type {
		ICO_FILEOPEN = 0,
		ICO_INFO,
		ICO_SETTINGS,
		ICO_VIEW_RENDERED,
		ICO_VIEW_CLASSIC,
		ICO_VIEW_TEXTONLY,
		ICO_SHOWMENU,
		_ICOMAX
	};

	HIMAGELIST l_imgLst = ImageList_Create(22, 22, ILC_COLOR32, _ICOMAX, 0);
	GetToolbar().SendMessage(TB_SETIMAGELIST, IML, (LPARAM)l_imgLst);
	// CToolbar::OnDestroy destroys the image list...

	int l_icoId[_ICOMAX]{};
	l_icoId[ICO_FILEOPEN] = CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_FILEOPEN, 22, 22);
	l_icoId[ICO_INFO] = CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_INFO, 22, 22);
	l_icoId[ICO_SETTINGS] = CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_SETTINGS, 22, 22);
	l_icoId[ICO_VIEW_RENDERED] = CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_VIEW_RENDERED, 22, 22);
	l_icoId[ICO_VIEW_CLASSIC] = CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_VIEW_CLASSIC, 22, 22);
	l_icoId[ICO_VIEW_TEXTONLY] = CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_VIEW_TEXTONLY, 22, 22);
	l_icoId[ICO_SHOWMENU] = CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_SHOWMENU, 22, 22);

	const BYTE defState = TBSTATE_ENABLED;
	const BYTE defStyle = BTNS_AUTOSIZE;

#define _TBBTN(A_ICO, A_IDF, A_STATE, A_STYLE, A_TEXT) \
		{ MAKELONG(l_icoId[A_ICO], IML), A_IDF, A_STATE, A_STYLE, {0}, 0, (INT_PTR)_T(A_TEXT) }
#define _TBSEP { 0, 0, 0, BTNS_SEP, {0}, 0, -1 }

	// define buttons:
	TBBUTTON l_btns[] = {
		_TBBTN(ICO_FILEOPEN, TBBID_OPEN, defState, defStyle | BTNS_DROPDOWN, "Open (Ctrl+O)"),
		_TBSEP,
		_TBBTN(ICO_SETTINGS, TBBID_SETTINGS, defState, defStyle, "Settings (F6)"),
		_TBBTN(ICO_SHOWMENU, TBBID_SHOWMENU, defState, defStyle | BTNS_CHECK, "Show Menu (Alt)"),
		_TBSEP,
		_TBBTN(ICO_VIEW_RENDERED, TBBID_VIEW_RENDERED, defState | TBSTATE_CHECKED, defStyle | BTNS_CHECKGROUP, "Rendered (F2)"),
		_TBBTN(ICO_VIEW_CLASSIC, TBBID_VIEW_CLASSIC, defState, defStyle | BTNS_CHECKGROUP, "Classic (F3)"),
		_TBBTN(ICO_VIEW_TEXTONLY, TBBID_VIEW_TEXTONLY, defState, defStyle | BTNS_CHECKGROUP, "Text Only (F4)"),
		_TBSEP,
		_TBBTN(ICO_INFO, TBBID_ABOUT, defState, defStyle, "About (F1)"),
	};

#undef _TBSEP
#undef _TBBTN

	GetToolbar().SendMessage(TB_ADDBUTTONS, sizeof(l_btns) / sizeof(l_btns[0]), (LPARAM)&l_btns);

	// adjust size of the toolbar and the parent rebar:
	GetToolbar().SendMessage(TB_AUTOSIZE);

	SIZE l_updatedSize = GetToolbar().GetMaxSize();
	::SendMessage(GetToolbar().GetParent(), UWM_TOOLBAR_RESIZE, (WPARAM)GetToolbar().GetHwnd(), (LPARAM)&l_updatedSize);
}


void CMainFrame::CreateSearchToolbar()
{
	const int IML = 0;
	const int EDITCTRL_WIDTH = 100;
	const int EDITCTRL_HEIGHT = 21;
	auto LABEL_FIND = L" Find: ";

	HIMAGELIST l_imgLst = ImageList_Create(16, 16, ILC_COLOR32, 1, 0);
	// we also use this image list for the toolbar, so CToolbar::OnDestroy destroys it.
	CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_FIND16, 16, 16);
	CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_DOWN16, 16, 16);
	CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_UP16, 16, 16);
	CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_CLOSE16, 16, 16);
	CUtilWin32GUI::AddPngToImageList(l_imgLst, g_hInstance, IDB_PNG_WARNING16, 16, 16);

	// add ImageList to rebar (not limited to this band, but currently only used by it):
	REBARINFO l_rbi;
	l_rbi.fMask = RBIM_IMAGELIST;
	l_rbi.himl = l_imgLst;
	GetRebar().SetBarInfo(l_rbi);

	// create toolbar container:
	m_searchToolbar = new CToolbar();
	m_searchToolbar->Create(GetRebar());
	m_searchToolbar->SendMessage(TB_SETIMAGELIST, IML, (LPARAM)l_imgLst);
	m_searchToolbar->SendMessage(TB_SETMAXTEXTROWS, 1, 0);
	m_searchToolbar->SendMessage(TB_SETSTYLE, 0, m_searchToolbar->SendMessage(TB_GETSTYLE) | TBSTYLE_LIST);
	m_searchToolbar->SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);

	TBBUTTON l_btns[] = {
		{ EDITCTRL_WIDTH, 0, TBSTATE_ENABLED, BTNS_SEP, { 0 }, 0, -1 }, // space for edit control
		{ MAKELONG(1, IML), TBBID_SEARCH_DOWN, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_NOPREFIX, { 0 }, 0, (INT_PTR)L"Next" },
		{ MAKELONG(2, IML), TBBID_SEARCH_UP, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_NOPREFIX, { 0 }, 0, (INT_PTR)L"Previous" },
		{ MAKELONG(3, IML), TBBID_SEARCH_CLOSE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_NOPREFIX, { 0 }, 0, 0 },
		{ MAKELONG(4, IML), TBBID_SEARCH_NOTFOUND, TBSTATE_HIDDEN | TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_NOPREFIX, { 0 }, 0, (INT_PTR)L"Phrase not found" },
	};

	m_searchToolbar->SendMessage(TB_ADDBUTTONS, sizeof(l_btns) / sizeof(l_btns[0]), (LPARAM)&l_btns);

	// create search edit control:
	m_hSearchEditBox = ::CreateWindowEx(WS_EX_CLIENTEDGE, L"Edit", nullptr,
		WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
		0, 0, EDITCTRL_WIDTH, EDITCTRL_HEIGHT,
		m_hWnd, nullptr, g_hInstance, 0);
	::SendMessage(m_hSearchEditBox, WM_SETFONT, m_searchToolbar->SendMessage(WM_GETFONT, 0, 0), FALSE);

	// notifications still go to the main window, but the control should live inside the toolbar window:
	::SetParent(m_hSearchEditBox, m_searchToolbar->GetHwnd());

	// create rebar band:
	REBARBANDINFO l_rbbi;

	l_rbbi.fMask = RBBIM_ID | RBBIM_STYLE | RBBIM_TEXT | RBBIM_IMAGE | RBBIM_HEADERSIZE | RBBIM_CHILD | RBBIM_SIZE | RBBIM_CHILDSIZE;
	l_rbbi.wID = IDW_SEARCHTOOLBAR;
	l_rbbi.fStyle = RBBS_BREAK | RBBS_NOGRIPPER | RBBS_NOVERT;
	l_rbbi.lpText = const_cast<wchar_t*>(LABEL_FIND);
	l_rbbi.iImage = 0;
	l_rbbi.cxHeader = 55;
	l_rbbi.hwndChild = m_searchToolbar->GetHwnd();

	// dat size:
	m_searchToolbar->SendMessage(TB_AUTOSIZE);
	CSize sz = m_searchToolbar->GetMaxSize();
	l_rbbi.cyMinChild = sz.cy;
	l_rbbi.cyMaxChild = sz.cy;
	l_rbbi.cxMinChild = sz.cx + 2;

	GetRebar().InsertBand(-1, l_rbbi);

	GetRebar().MaximizeBand(GetRebar().IDToIndex(IDW_SEARCHTOOLBAR));
}


void CMainFrame::ShowSearchToolbar(bool a_show)
{
	// don't show the toolbar if no NFO is loaded:
	a_show = a_show && m_view.GetActiveCtrl()->HasNfoData();

	if (m_searchToolbar)
	{
		GetRebar().ShowBand(GetRebar().IDToIndex(IDW_SEARCHTOOLBAR), a_show ? 1 : 0);
	}
	else if (a_show)
	{
		CreateSearchToolbar();
	}

	if (a_show)
	{
		::SetFocus(m_hSearchEditBox);
		::SendMessage(m_hSearchEditBox, EM_SETSEL, 0, -1);
	}
	else
	{
		::SetFocus(GetHwnd());
	}
}


void CMainFrame::DoFindText(bool a_up, bool a_force)
{
	if (!m_hSearchEditBox)
		return;

	wchar_t l_text[100]{};

	::GetWindowText(m_hSearchEditBox, l_text, 99);

	bool l_showNotFound;

	if (a_force || _wcsicmp(l_text, m_lastSearchTerm.c_str()) != 0)
	{
		m_lastSearchTerm = l_text;

		l_showNotFound = !m_view.GetActiveCtrl()->FindAndSelectTerm(m_lastSearchTerm, a_up);
	}

	if (!*l_text) l_showNotFound = false;

	m_searchToolbar->SendMessage(TB_HIDEBUTTON, TBBID_SEARCH_NOTFOUND, MAKELONG(l_showNotFound ? FALSE : TRUE, 0));
}


void CMainFrame::ShowMenuBar(bool a_show)
{
	BOOL b = (a_show ? TRUE : FALSE);

	GetRebar().ShowBand(GetRebar().IDToIndex(IDW_MENUBAR), b);
	GetToolbar().SendMessage(TB_CHECKBUTTON, TBBID_SHOWMENU, b);

	m_menuBarVisible = a_show;
}


BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
	DWORD l_item = LOWORD(wParam);

	if (l_item >= TBBID_OPENMRUSTART && l_item < TBBID_OPENMRUSTART + ms_mruLength)
	{
		l_item = l_item - TBBID_OPENMRUSTART;

		if (l_item < m_mruPaths.size())
		{
			const std::wstring l_path = m_mruPaths[l_item];

			if (!::PathFileExists(l_path.c_str()))
			{
				this->MessageBox(L"The file does no longer exist at its previous location.", L"Sorry", MB_ICONEXCLAMATION);
			}
			else
			{
				OpenFile(l_path);
			}
		}

		return TRUE;
	}

	if (m_hSearchEditBox && (HWND)lParam == m_hSearchEditBox)
	{
		if (HIWORD(wParam) == EN_CHANGE)
		{
			DoFindText(false, false);
		}
	}

	switch (l_item)
	{
	case IDM_EXIT:
		SendMessage(WM_CLOSE);
		return TRUE;

	case TBBID_OPEN:
	case IDM_FILE_OPEN:
		OpenChooseFileName();
		return TRUE;

	case TBBID_SETTINGS:
	case IDM_SETTINGS: {
		CSettingsWindowDialog l_dlg(m_hWnd);
		l_dlg.SetMainWin(this);
		l_dlg.DoModal();
		return TRUE; }

	case TBBID_SHOWMENU:
		ShowMenuBar(!m_menuBarVisible);
		m_settings->bAlwaysShowMenubar = m_menuBarVisible;
		m_settings->SaveToRegistry();
		return TRUE;

	case TBBID_ABOUT:
	case IDM_ABOUT:
		OnHelp();
		return TRUE;

	case IDMC_COPY:
		m_view.CopySelectedTextToClipboard();
		if (!m_settings->bCopyOnSelect) {
			m_view.GetActiveCtrl()->ClearSelection(true);
		}
		return TRUE;

	case IDMC_SELECTALL:
		m_view.SelectAll();
		return TRUE;

	case IDM_CHECKFORUPDATES:
		this->CheckForUpdates();
		return TRUE;

	case IDM_EXPORT_PNG:
	case IDM_EXPORT_PNG_TRANSP:
	case IDM_EXPORT_UTF8:
	case IDM_EXPORT_UTF16:
	case IDM_EXPORT_CP437:
	case IDM_EXPORT_XHTML:
	case IDM_EXPORT_XHTML_CANVAS:
	case IDM_EXPORT_PDF:
	case IDM_EXPORT_PDF_DIN:
		DoNfoExport(LOWORD(wParam));
		return TRUE;

	case TBBID_VIEW_RENDERED:
	case IDM_VIEW_RENDERED:
		SwitchView(MAIN_VIEW_RENDERED);
		return TRUE;

	case TBBID_VIEW_CLASSIC:
	case IDM_VIEW_CLASSIC:
		SwitchView(MAIN_VIEW_CLASSIC);
		return TRUE;

	case TBBID_VIEW_TEXTONLY:
	case IDM_VIEW_TEXTONLY:
		SwitchView(MAIN_VIEW_TEXTONLY);
		return TRUE;

	case IDM_ALWAYSONTOP:
	case IDMC_ALWAYSONTOP:
		GetSettings()->bAlwaysOnTop = !GetSettings()->bAlwaysOnTop;
		UpdateAlwaysOnTop();
		return TRUE;

	case IDM_VIEW_SHOWTOOLBAR:
	case IDMC_TOGGLE_TOOLBAR:
		ShowToolbar(m_bShowToolbar = !m_bShowToolbar);
		return TRUE;

	case IDM_VIEW_RELOAD:
	case IDMC_RELOAD:
		if (m_view.ReloadFile())
		{
			UpdateStatusbar();
		}
		return TRUE;

	case IDM_SHOWSTATUSBAR:
		ShowStatusbar(m_bShowStatusbar = !m_bShowStatusbar);
		return TRUE;

	case IDM_OPTIWIDTH:
		ShowWindow(SW_SHOWNORMAL);
		AdjustWindowToNFOWidth(false);
		return TRUE;

	case TBBID_CLEARMRU:
		if (!m_mruPaths.empty())
		{
			// "empty list" menu item
			m_mruPaths.clear();
			SaveOpenMruList();
		}
		else
		{
			// "browse..." menu item
			OpenChooseFileName();
		}
		return TRUE;

	case IDM_ZOOM_IN:
		if (m_view.GetNfoData() && m_view.GetNfoData()->HasData())
			m_view.GetActiveCtrl()->ZoomIn();
		break;

	case IDM_ZOOM_OUT:
		if (m_view.GetNfoData() && m_view.GetNfoData()->HasData())
			m_view.GetActiveCtrl()->ZoomOut();
		break;

	case IDM_ZOOM_RESET:
		if (m_view.GetNfoData() && m_view.GetNfoData()->HasData())
		{
			m_view.GetActiveCtrl()->ZoomReset();
			AdjustWindowToNFOWidth(true, true);
		}
		break;

	case IDM_TOOLS_NEXTNFO:
		BrowseFolderNfoMove(1);
		::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
		break;

	case IDM_TOOLS_PREVIOUSNFO:
		BrowseFolderNfoMove(-1);
		::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
		break;

	case ID_SCROLL_PAGEDOWN:
		m_view.ScrollPageDown();
		break;

	case ID_SCROLL_PAGEUP:
		m_view.ScrollPageUp();
		break;

	case IDM_FINDTEXT:
		ShowSearchToolbar();
		break;

	case TBBID_SEARCH_DOWN:
		DoFindText(false);
		break;

	case TBBID_SEARCH_UP:
		DoFindText(true);
		break;

	case TBBID_SEARCH_CLOSE:
		ShowSearchToolbar(false);
		break;
	}

	return FALSE;
}


void CMainFrame::ShowStatusbar(BOOL bShow)
{
	::CheckMenuItem(::GetSubMenu(GetMenubar().GetMenu(), VIEW_MENU_POS), IDM_SHOWSTATUSBAR,
		(m_bShowStatusbar ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

	GetStatusbar().ShowWindow(m_bShowStatusbar ? SW_SHOW : SW_HIDE);

	RecalcLayout();
	Invalidate();

	UpdateStatusbar();
}


void CMainFrame::ShowToolbar(BOOL bShow)
{
	::CheckMenuItem(::GetSubMenu(GetMenubar().GetMenu(), VIEW_MENU_POS), IDM_VIEW_SHOWTOOLBAR,
		(m_bShowToolbar ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

	GetRebar().SendMessage(RB_SHOWBAND, GetRebar().GetBand(GetToolbar()), bShow);
}


LRESULT CMainFrame::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR l_lpnm = (LPNMHDR)lParam;

	// charset statusbar pane right-click menu:
	if (l_lpnm->hwndFrom == GetStatusbar().GetHwnd() && l_lpnm->code == NM_RCLICK)
	{
		LPNMMOUSE l_pnmm = (LPNMMOUSE)lParam;

		if (l_pnmm->dwItemSpec == STATUSBAR_PANE_CHARSET)
		{
			if (DoCharsetMenu(l_pnmm))
				return FALSE;
		}
	}
	// MRU toolbar dropdown:
	else if (l_lpnm->code == TBN_DROPDOWN)
	{
		if (DoOpenMruMenu((LPNMTOOLBAR)lParam))
			return FALSE;
	}

	return CFrame::OnNotify(wParam, lParam);
}


bool CMainFrame::DoOpenMruMenu(const LPNMTOOLBAR a_lpnm)
{
	if (a_lpnm->iItem == TBBID_OPEN)
	{
		// Get the coordinates of the button:
		RECT rc;
		::SendMessage(a_lpnm->hdr.hwndFrom, TB_GETRECT, (WPARAM)a_lpnm->iItem, (LPARAM)&rc);
		::MapWindowPoints(a_lpnm->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);

		// Create a temp menu:
		HMENU hPopupMenu = ::CreatePopupMenu();

		size_t l_idx = 0;
		for (vector<_tstring>::const_iterator it = m_mruPaths.begin(); it != m_mruPaths.end(); it++, l_idx++)
		{
			const wchar_t* l_entry = it->c_str();
			l_entry = ::PathFindFileName(l_entry);
			::AppendMenu(hPopupMenu, MF_STRING, TBBID_OPENMRUSTART + l_idx, l_entry);
		}

		if (m_mruPaths.empty())
		{
			::AppendMenu(hPopupMenu, MF_STRING, TBBID_CLEARMRU, _T("Browse..."));
		}
		else
		{
			::AppendMenu(hPopupMenu, MF_SEPARATOR, 0, nullptr);
			::AppendMenu(hPopupMenu, MF_STRING, TBBID_CLEARMRU, _T("Empty Recently Viewed List"));
		}

		// Set up the popup menu.
		// Set rcExclude equal to the button rectangle so that if the toolbar
		// is too close to the bottom of the screen, the menu will appear above
		// the button rather than below it.
		TPMPARAMS tpm;
		tpm.cbSize = sizeof(TPMPARAMS);
		tpm.rcExclude = rc;

		// Show the menu and wait for input.
		// If the user selects an item, its WM_COMMAND is sent.
		::TrackPopupMenuEx(hPopupMenu,
			TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
			rc.left, rc.bottom, m_hWnd, &tpm);

		::DestroyMenu(hPopupMenu);

		return true;
	}

	return false;
}


bool CMainFrame::DoCharsetMenu(const LPNMMOUSE a_pnmm)
{
	HMENU hPopupMenu = ::CreatePopupMenu();

	// auto
	::AppendMenu(hPopupMenu, MF_STRING, NFOC_AUTO, L"Auto");
	::AppendMenu(hPopupMenu, MF_SEPARATOR, 0, nullptr);
	// regular things
	::AppendMenu(hPopupMenu, MF_STRING, NFOC_CP437, L"CP437");
	::AppendMenu(hPopupMenu, MF_STRING, NFOC_CP437_STRICT, L"CP437 (strict)");
	::AppendMenu(hPopupMenu, MF_STRING, NFOC_UTF8, L"UTF-8");
	::AppendMenu(hPopupMenu, MF_SEPARATOR, 0, nullptr);
	// more exotic things:
	::AppendMenu(hPopupMenu, MF_STRING, NFOC_UTF16, L"UTF-16");
	::AppendMenu(hPopupMenu, MF_STRING, NFOC_WINDOWS_1252, L"Windows-1252");
	::AppendMenu(hPopupMenu, MF_SEPARATOR, 0, nullptr);
	// crazy shit:
	::AppendMenu(hPopupMenu, MF_STRING, NFOC_CP437_IN_UTF8, L"CP437 in UTF-8");
	::AppendMenu(hPopupMenu, MF_STRING, NFOC_CP437_IN_UTF16, L"CP437 in UTF-16");
	::AppendMenu(hPopupMenu, MF_STRING, NFOC_CP437_IN_CP437, L"double encoded CP437");

	POINT pt = a_pnmm->pt;
	::ClientToScreen(GetStatusbar().GetHwnd(), &pt);

	int l_cmd = ::TrackPopupMenuEx(hPopupMenu,
		TPM_TOPALIGN | TPM_CENTERALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
		pt.x, pt.y, m_hWnd, nullptr);

	::DestroyMenu(hPopupMenu);

	if (l_cmd > 0 && l_cmd < _NFOC_MAX)
	{
		if (m_view.ReloadFile((ENfoCharset)l_cmd))
		{
			UpdateStatusbar();
		}
	}

	return true;
}


void CMainFrame::OnHelp()
{
	if (!m_showingAbout)
	{
		CAboutDialog l_pAboutDialog(m_hWnd);
		l_pAboutDialog.SetMainWin(this);

		m_showingAbout = true;
		l_pAboutDialog.DoModal();
		m_showingAbout = false;
	}
}


void CMainFrame::SwitchView(EMainView a_view)
{
	if (CPluginManager::GetInstance()->TriggerViewChanging(a_view))
	{
		// WARNING: We use menu positions here exclusively. Using
		// the COMMAND identifiers just didn't work for no apparent reason :(

		HMENU l_hPopup = ::GetSubMenu(GetMenubar().GetMenu(), VIEW_MENU_POS);

		if (l_hPopup)
		{
			::CheckMenuRadioItem(l_hPopup, 0, 2,
				(a_view == MAIN_VIEW_RENDERED ? 0 : (a_view == MAIN_VIEW_CLASSIC ? 1 : 2)),
				MF_BYPOSITION);
		}

		GetToolbar().SendMessage(TB_CHECKBUTTON, TBBID_VIEW_RENDERED, (a_view == MAIN_VIEW_RENDERED ? TRUE : FALSE));
		GetToolbar().SendMessage(TB_CHECKBUTTON, TBBID_VIEW_CLASSIC, (a_view == MAIN_VIEW_CLASSIC ? TRUE : FALSE));
		GetToolbar().SendMessage(TB_CHECKBUTTON, TBBID_VIEW_TEXTONLY, (a_view == MAIN_VIEW_TEXTONLY ? TRUE : FALSE));

		m_settings->iLastView = a_view;
		if (m_settings->iDefaultView == -1)
		{
			m_settings->SaveToRegistry();
		}

		m_view.SwitchView(a_view);

		AdjustWindowToNFOWidth(true, true);

		CPluginManager::GetInstance()->TriggerViewChanged();
	}
}


void CMainFrame::OpenFile(const std::wstring& a_filePath)
{
	if (!m_view.OpenFile(a_filePath))
	{
		return;
	}

	UpdateAfterFileLoad();
	AddToMruList(a_filePath);
}


bool CMainFrame::OpenLoadedFile(PNFOData a_nfoData, bool a_showError)
{
	if (!m_view.OpenLoadedFile(a_nfoData))
	{
		if (a_showError)
		{
			this->MessageBox(a_nfoData->GetLastErrorDescription().c_str(), L"Fail", MB_ICONEXCLAMATION);
		}

		return false;
	}

	UpdateAfterFileLoad();
	AddToMruList(a_nfoData->GetFilePath());

	return true;
}


void CMainFrame::UpdateAfterFileLoad()
{
	WatchFileStop();

	m_nfoPreloadData.reset();
	m_nfoInFolderIndex = (size_t)-1;
	m_nfoPathsInFolder.clear();

	UpdateCaption();

	UpdateStatusbar();

	AdjustWindowToNFOWidth(true);
	AdjustWindowToNFOHeight();

	m_lastSearchTerm = L"";

	WatchFileStart();
}


void CMainFrame::AddToMruList(const std::wstring a_filePath)
// do not use a reference since it might be a string from m_mruPaths and that
// would turn out badly.
{
	for (auto it = m_mruPaths.begin(); it != m_mruPaths.end(); it++)
	{
		if (_wcsicmp(it->c_str(), a_filePath.c_str()) == 0)
		{
			m_mruPaths.erase(it);
			break;
		}
	}

	m_mruPaths.insert(m_mruPaths.begin(), a_filePath);

	if (m_mruPaths.size() > ms_mruLength)
	{
		m_mruPaths.erase(m_mruPaths.begin() + ms_mruLength, m_mruPaths.end());
	}

	if (m_settings->bKeepOpenMRU)
	{
		SaveOpenMruList();
	}
}


void CMainFrame::WatchFileStart()
{
	if (!m_settings->bMonitorFileChanges)
	{
		return;
	}

	if (!m_fileChangeWatcher)
	{
		m_fileChangeWatcher = std::make_unique<CWinFileWatcher>([this] { OnFileChanged(); });
	}

	if (PNFOData l_nfo = m_view.GetActiveCtrl()->GetNfoData())
	{
		m_fileChangeWatcher->SetFile(l_nfo->GetFilePath());

		m_fileChangeWatcher->StartWatching();
	}
}


void CMainFrame::WatchFileStop()
{
	if (!m_fileChangeWatcher)
	{
		return;
	}

	m_fileChangeWatcher->StopWatching();
}


// warning: most likely running in background thread
void CMainFrame::OnFileChanged()
{
	::PostMessage(m_hWnd, WM_RELOAD_NFO, 0, 0);
}


void CMainFrame::OnAfterSettingsChanged() // meh
{
	// stop+start watching the file to reflect new setting state:
	WatchFileStop();
	WatchFileStart();

	CNFORenderer::SetGlobalUseGPUFlag(m_settings->bUseGPU);

	if (m_view.GetWrapLines() != m_settings->bWrapLines)
	{
		m_view.SetWrapLines(m_settings->bWrapLines);

		if (m_view.GetNfoData())
		{
			AdjustWindowToNFOWidth(true, false);
			AdjustWindowToNFOHeight();
		}
	}
}


void CMainFrame::UpdateCaption()
{
	wstring l_caption;

	if (m_view.GetNfoData() && m_view.GetNfoData()->HasData())
	{
		l_caption = m_view.GetNfoData()->GetFileName();
	}

	if (!l_caption.empty()) l_caption += _T(" - ");
	l_caption += L"iNFekt v" + InfektVersionAsString();

	if (CNFOApp::GetInstance()->InPortableMode())
	{
		l_caption += L" (portable)";
	}

	SetWindowText(l_caption.c_str());
}


void CMainFrame::UpdateStatusbar()
{
	if (!m_bShowStatusbar)
		return;

	CStatusbar& l_sb = GetStatusbar();

	l_sb.SendMessage(WM_SIZE, 0, 0);

	if (m_view.GetNfoData() && m_view.GetNfoData()->HasData())
	{
		RECT l_rc = l_sb.GetWindowRect();
		LONG l_width = l_rc.right - l_rc.left;

		const wstring l_charset = m_view.GetNfoData()->GetCharsetName();
		wstring l_fileNameInfo = m_view.GetNfoData()->GetFileName();

		if (m_nfoInFolderIndex != (size_t)-1)
		{
			l_fileNameInfo += FORMAT(L" (%d / %d in folder)", (m_nfoInFolderIndex + 1) % m_nfoPathsInFolder.size());
		}

		wstring l_timeInfo, l_sizeInfo;
		if (!m_view.GetNfoData()->GetFilePath().empty())
		{
			CUtilWin32GUI::FormatFileTimeSize(m_view.GetNfoData()->GetFilePath(), l_timeInfo, l_sizeInfo);
		}

		int l_sbWidths[5]{};
		l_sbWidths[1] = CUtilWin32GUI::StatusCalcPaneWidth(l_sb.GetHwnd(), l_timeInfo.c_str());
		l_sbWidths[2] = CUtilWin32GUI::StatusCalcPaneWidth(l_sb.GetHwnd(), l_sizeInfo.c_str());
		l_sbWidths[STATUSBAR_PANE_CHARSET] = CUtilWin32GUI::StatusCalcPaneWidth(l_sb.GetHwnd(), l_charset.c_str());
		l_sbWidths[4] = 25;

		int l_sbParts[5] = { l_width, 0 };
		for (int i = 1; i < 5; i++) l_sbParts[0] -= l_sbWidths[i];
		for (int i = 1; i < 5; i++) l_sbParts[i] = l_sbParts[i - 1] + l_sbWidths[i];

		l_sb.CreateParts(5, l_sbParts);
		l_sb.SetPartText(0, l_fileNameInfo.c_str());
		l_sb.SetPartText(1, l_timeInfo.c_str());
		l_sb.SetPartText(2, l_sizeInfo.c_str());
		l_sb.SetPartText(STATUSBAR_PANE_CHARSET, l_charset.c_str());

		l_sb.SetSimple(FALSE);
	}
	else
	{
		l_sb.SendMessage(SB_SETTEXT, SB_SIMPLEID, (LPARAM)_T("Hit the Alt key to toggle the menu bar."));
		l_sb.SetSimple(TRUE);
	}
}


void CMainFrame::AdjustWindowToNFOWidth(bool a_preflightCheck, bool a_growOnly)
{
	if (a_preflightCheck)
	{
		if (!m_settings->bAutoWidth)
		{
			return;
		}

		WINDOWPLACEMENT l_wpl = { sizeof(WINDOWPLACEMENT), 0 };

		if (GetWindowPlacement(l_wpl) && l_wpl.showCmd == SW_MAXIMIZE)
		{
			return;
		}
	}

	int l_desiredWidth = static_cast<int>(m_view.GetActiveCtrl()->GetWidth());

	l_desiredWidth += ::GetSystemMetrics(SM_CXSIZEFRAME) * 2;
	l_desiredWidth += ::GetSystemMetrics(SM_CXVSCROLL);
	l_desiredWidth += 16; // some extra padding
	l_desiredWidth = std::max(l_desiredWidth, ms_minWidth);

	RECT l_rc;
	if (!::GetWindowRect(GetHwnd(), &l_rc))
	{
		return;
	}

	if (l_desiredWidth == l_rc.right - l_rc.left ||
		(a_growOnly && l_desiredWidth < l_rc.right - l_rc.left))
	{
		::RedrawWindow(GetHwnd(), nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
		return;
	}

	int l_mid = l_rc.left + (l_rc.right - l_rc.left) / 2;
	int l_oldLeft = l_rc.left;

	// center around previous location:
	l_rc.left = l_mid - l_desiredWidth / 2;

	// make sure windows don't go off-screen to the left:
	if (l_rc.left < 0 && l_oldLeft >= 0)
	{
		l_rc.left = 0;
	}

	// set width:
	l_rc.right = l_rc.left + l_desiredWidth;

	// perform extra checks on the new window size:
	POINT l_pt = { l_rc.left, l_rc.top };
	MONITORINFO l_moInfo = { sizeof(MONITORINFO), 0 };
	HMONITOR l_hMonitor = ::MonitorFromPoint(l_pt, MONITOR_DEFAULTTONEAREST);
	// use MonitorFromPoint instead of MonitorFromWindow to make the left-top corner count.

	if (::GetMonitorInfo(l_hMonitor, &l_moInfo))
	{
		const RECT& l_wa = l_moInfo.rcWork;

		// don't let the window grow larger than the size of the work area
		if (l_rc.right - l_rc.left > l_wa.right - l_wa.left)
		{
			l_rc.left = l_wa.left;
			l_rc.right = l_wa.right;
		}
		// and don't allow parts of the window to go off-screen to the right:
		else if (l_rc.right > l_wa.right)
		{
			int l_excess = (l_rc.right - l_wa.right);
			l_rc.left -= l_excess;
			l_rc.right -= l_excess;
		}
		// also keep window titlebar on-screen at all costs:
		if (l_rc.top < l_wa.top)
		{
			l_rc.bottom -= l_wa.top - l_rc.top;
			l_rc.top = l_wa.top;
		}
	}

	MoveWindow(l_rc);
}


void CMainFrame::AdjustWindowToNFOHeight()
{
	if (!m_settings->bAutoHeight)
	{
		return;
	}

	WINDOWPLACEMENT l_wpl = { sizeof(WINDOWPLACEMENT), 0 };

	if (GetWindowPlacement(l_wpl) && l_wpl.showCmd == SW_MAXIMIZE)
	{
		return;
	}

	int l_desiredHeight = static_cast<int>(m_view.GetActiveCtrl()->GetHeight());

	l_desiredHeight += ::GetSystemMetrics(SM_CYSIZEFRAME) * 2;
	l_desiredHeight += ::GetSystemMetrics(SM_CYCAPTION);
	l_desiredHeight += ::GetSystemMetrics(SM_CYHSCROLL);
	l_desiredHeight += GetRebar().GetBarHeight();
	// :TODO: InfoBar height if visible!
	l_desiredHeight += GetStatusbar().GetWindowRect().Height();

	l_desiredHeight = std::max(l_desiredHeight, ms_minHeight);

	RECT l_rc;
	if (!::GetWindowRect(GetHwnd(), &l_rc) || l_desiredHeight == l_rc.bottom - l_rc.top)
	{
		return;
	}

	if (l_rc.top < 0)
	{
		l_rc.top = 0;
	}

	l_rc.bottom = l_rc.top + l_desiredHeight;

	POINT l_pt = { l_rc.left, l_rc.top };
	MONITORINFO l_moInfo = { sizeof(MONITORINFO), 0 };
	HMONITOR l_hMonitor = ::MonitorFromPoint(l_pt, MONITOR_DEFAULTTONEAREST);

	if (::GetMonitorInfo(l_hMonitor, &l_moInfo))
	{
		const RECT& l_wa = l_moInfo.rcWork;

		// don't let the window grow larger than the size of the work area:
		if (l_rc.bottom - l_rc.top > l_wa.bottom - l_wa.top)
		{
			l_rc.top = l_wa.top;
			l_rc.bottom = l_wa.bottom;
		}
		// and center vertically otherwise:
		else if (m_settings->bCenterWindow)
		{
			l_rc.top = (l_wa.top + (l_wa.bottom - l_wa.top) / 2) - l_desiredHeight / 2;
			l_rc.bottom = l_rc.top + l_desiredHeight;
		}
	}

	MoveWindow(l_rc);
}


BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	/* return TRUE if the message has been translated */

	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		if (pMsg->wParam == VK_ESCAPE)
		{
			if (m_searchToolbar && GetRebar().IsBandVisible(GetRebar().IDToIndex(IDW_SEARCHTOOLBAR)))
			{
				ShowSearchToolbar(false);
			}
			else if (m_settings->bCloseOnEsc)
			{
				PostMessage(WM_CLOSE);
			}
			return TRUE;
		}

		if (m_hSearchEditBox && ::GetFocus() == m_hSearchEditBox)
		{
			if (pMsg->wParam == VK_RETURN)
			{
				DoFindText(false);

				return TRUE;
			}

			return FALSE;
		}
		// fall through
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		if (!m_view.ForwardFocusTypeMouseKeyboardEvent(pMsg))
		{
			return TRUE;
		}
		break;
	case WM_SYSKEYUP:
		if (pMsg->wParam == VK_MENU || pMsg->wParam == VK_F10)
		{
			if (pMsg->wParam == VK_F10 && (::GetAsyncKeyState(VK_LSHIFT) || ::GetAsyncKeyState(VK_RSHIFT)))
			{
				// OH LAWD HAVE MERCY :(
				MSG l_msg{};
				l_msg.message = WM_KEYDOWN;
				l_msg.wParam = VK_APPS;
				m_view.ForwardFocusTypeMouseKeyboardEvent(&l_msg);
			}
			else
			{
				ShowMenuBar(!m_menuBarVisible);

				/*if(m_menuBarVisible)
				{
					::SetFocus(GetMenubar().GetHwnd());
				} we can't let this happen or the menu bar will grab all keyboard events...
				... gonna have to figure this out some other day :TODO: */
			}
		}
		return TRUE;
	}

	if (CWnd::PreTranslateMessage(pMsg))
	{
		return TRUE;
	}

	if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
	{
		static HACCEL hAccelTable = nullptr;
		if (!hAccelTable) hAccelTable = ::LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDR_MAIN_KEYBOARD_SHORTCUTS));

		return ::TranslateAccelerator(m_hWnd, hAccelTable, pMsg);
	}

	return FALSE;
}


LRESULT CMainFrame::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_GETMINMAXINFO: {
		PMINMAXINFO l_info = (PMINMAXINFO)lParam;
		l_info->ptMinTrackSize.x = ms_minWidth;
		l_info->ptMinTrackSize.y = ms_minHeight;
		return 0; }
	case WM_LOAD_NFO: {
		const wstring l_path = (wchar_t*)wParam;
		if (::PathFileExists(l_path.c_str()))
		{
			OpenFile(l_path);
		}
		return 1; }
	case WM_RELOAD_NFO:
		if (m_view.ReloadFile())
		{
			UpdateStatusbar();
		}
		return 1;
	case WM_SYNC_PLUGIN_TO_CORE:
		return CPluginManager::GetInstance()->SynchedPluginToCore((void*)lParam);
	case WM_COPYDATA: {
		const COPYDATASTRUCT* l_cpds = (PCOPYDATASTRUCT)lParam;
		if (l_cpds->dwData == WM_LOAD_NFO)
		{
			if (l_cpds->cbData < 1000 && l_cpds->lpData &&
				::IsTextUnicode(l_cpds->lpData, l_cpds->cbData, nullptr))
			{
				const wstring l_path = (wchar_t*)l_cpds->lpData;
				if (::PathFileExists(l_path.c_str()))
				{
					OpenFile(l_path);

					WINDOWPLACEMENT wplm;
					if (GetWindowPlacement(wplm))
					{
						wplm.showCmd = SW_RESTORE;
						SetWindowPlacement(wplm);
					}

					ShowWindow(SW_SHOW);
					BringWindowToTop();
					SetForegroundWindow();
					SetActiveWindow();

					return TRUE;
				}
			}
		}
		break; }
	case WM_SIZE:
		UpdateStatusbar();
		break; // also invoke default
	case WM_THEMECHANGED: {
		CRebar& RB = GetRebar();
		RB.ShowGripper(RB.IDToIndex(IDW_TOOLBAR), !::IsThemeActive());
		break; }
	case WM_DESTROY:
		if (m_dropHelper)
		{
			m_dropHelper->Unregister();
			m_dropHelper->Release();
		}
		break; // also invoke default
	}

	return WndProcDefault(uMsg, wParam, lParam);
}


void CMainFrame::UpdateAlwaysOnTop()
{
	if (GetSettings()->bAlwaysOnTop)
	{
		this->SetForegroundWindow();
		this->SetFocus();
		this->SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	else
	{
		this->SetWindowPos(HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	::CheckMenuItem(::GetSubMenu(GetMenubar().GetMenu(), VIEW_MENU_POS), IDM_ALWAYSONTOP,
		(GetSettings()->bAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

	GetSettings()->SaveToRegistry();
}


void CMainFrame::OpenChooseFileName()
{
	COMDLG_FILTERSPEC l_filter[] = {
		{ L"All Supported File Types", L"*.nfo;*.diz;*.asc;*.ans" },
		{ L"NFO Files", L"*.nfo;*.diz" },
		{ L"ANSI Art Files", L"*.asc;*.ans" },
		{ L"Text Files", L"*.txt;*.nfo;*.diz;*.sfv" },
		{ L"All Files", L"*" }
	};

	const std::wstring l_filePath = CUtilWin32GUI::OpenFileDialog(g_hInstance, GetHwnd(), l_filter, 5);

	if (!l_filePath.empty())
	{
		OpenFile(l_filePath);
	}
}


void CMainFrame::DoNfoExport(UINT a_id)
{
	if (!m_view.GetNfoData() || !m_view.GetNfoData()->HasData())
	{
		this->MessageBox(_T("No file has been loaded!"), _T("Error"), MB_ICONEXCLAMATION);
		return;
	}

	_tstring l_baseFileName = CUtilWin32::PathRemoveExtension(m_view.GetNfoData()->GetFileName());

	_tstring l_defaultPath;
	if (m_settings->bDefaultExportToNFODir)
	{
		l_defaultPath = CUtilWin32::PathRemoveFileSpec(m_view.GetNfoData()->GetFilePath());
	}

	if (a_id == IDM_EXPORT_PNG || a_id == IDM_EXPORT_PNG_TRANSP)
	{
		COMDLG_FILTERSPEC l_filter[] = { { L"PNG File", L"*.png" } };

		const wstring l_filePath = CUtilWin32GUI::SaveFileDialog(g_hInstance, GetHwnd(),
			l_filter, 1, _T("png"), l_baseFileName + _T(".png"), l_defaultPath);

		if (!l_filePath.empty())
		{
			CNFOToPNG l_exporter(m_view.GetViewType() != MAIN_VIEW_RENDERED);
			CNFORenderSettings l_settings = m_view.GetActiveCtrl()->GetSettings();

			if (a_id == IDM_EXPORT_PNG_TRANSP)
			{
				l_settings.cBackColor.A = 0;
			}

			bool l_internalError = true;

			::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

			l_exporter.InjectSettings(l_settings);
			l_exporter.AssignNFO(m_view.GetActiveCtrl()->GetNfoData());

			if (!l_exporter.SavePNG(l_filePath))
			{
				this->MessageBox(_T("Unable to open file for writing!"), _T("Fail"), MB_ICONEXCLAMATION);
			}
			else
			{
				this->MessageBox(_T("File saved!"), _T("Success"), MB_ICONINFORMATION);
			}
		}
	}
	else if (a_id == IDM_EXPORT_UTF8 || a_id == IDM_EXPORT_UTF16)
	{
		bool l_utf8 = (a_id == IDM_EXPORT_UTF8);

		COMDLG_FILTERSPEC l_filter[] = { { L"NFO File", L"*.nfo" }, { L"Text File", L"*.txt" } };

		const std::wstring l_filePath = CUtilWin32GUI::SaveFileDialog(g_hInstance, GetHwnd(),
			l_filter, 2, _T("nfo"), l_baseFileName + (l_utf8 ? _T("-utf8.nfo") : _T("-utf16.nfo")),
			l_defaultPath);

		if (!l_filePath.empty())
		{
			::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

			if (m_view.GetActiveCtrl()->GetNfoData()->SaveToUnicodeFile(l_filePath, l_utf8))
			{
				this->MessageBox(_T("File saved!"), _T("Success"), MB_ICONINFORMATION);
			}
			else
			{
				std::wstring l_msg = m_view.GetActiveCtrl()->GetNfoData()->GetLastErrorDescription();

				if (l_msg.empty())
				{
					l_msg = L"Writing to the file failed. Please select a different file or folder.";
				}

				this->MessageBox(l_msg.c_str(), _T("Fail"), MB_ICONEXCLAMATION);
			}
		}
	}
	else if (a_id == IDM_EXPORT_CP437)
	{
		size_t l_inconvertible;

		COMDLG_FILTERSPEC l_filter[] = { { L"NFO File", L"*.nfo" }, { L"Text File", L"*.txt" } };

		const std::wstring l_filePath = CUtilWin32GUI::SaveFileDialog(g_hInstance, GetHwnd(),
			l_filter, 2, L"nfo", l_baseFileName + L"-msdos.nfo", l_defaultPath);

		if (!l_filePath.empty())
		{
			::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

			if (m_view.GetActiveCtrl()->GetNfoData()->SaveToCP437File(l_filePath, l_inconvertible))
			{
				std::wstringstream l_msg;
				std::wstring l_msgstr;

				l_msg << L"File saved!\r\n\r\n";

				if (l_inconvertible)
					l_msg << l_inconvertible << L" characters could not be converted to CP 437.";
				else
					l_msg << L"All characters in this NFO are CP 437-compatible and have been converted.";

				l_msgstr = l_msg.str();
				this->MessageBox(l_msgstr.c_str(), L"Success", MB_ICONINFORMATION);
			}
			else
			{
				std::wstring l_msg = m_view.GetActiveCtrl()->GetNfoData()->GetLastErrorDescription();

				if (l_msg.empty())
				{
					l_msg = L"Writing to the file failed. Please select a different file or folder.";
				}

				this->MessageBox(l_msg.c_str(), L"Fail", MB_ICONEXCLAMATION);
			}
		}
	}
	else if (a_id == IDM_EXPORT_XHTML || a_id == IDM_EXPORT_XHTML_CANVAS)
	{
		COMDLG_FILTERSPEC l_filter[] = { { L"HTML File", L"*.html" } };

		const std::wstring l_filePath = CUtilWin32GUI::SaveFileDialog(g_hInstance, GetHwnd(),
			l_filter, 1, _T("html"), l_baseFileName + _T(".html"), l_defaultPath);

		if (!l_filePath.empty())
		{
			string l_utf8;

			if (a_id == IDM_EXPORT_XHTML_CANVAS)
			{
				CNFOToHTMLCanvas l_exporter;

				l_exporter.AssignNFO(m_view.GetNfoData());
				l_exporter.InjectSettings(m_view.GetActiveCtrl()->GetSettings());

				l_utf8 = l_exporter.GetFullHTML();
			}
			else
			{
				CNFOToHTML l_exporter(m_view.GetActiveCtrl()->GetNfoData());

				l_exporter.SetSettings(m_view.GetActiveCtrl()->GetSettings());
				l_exporter.SetTitle(m_view.GetNfoData()->GetFileName());

				l_utf8 = CUtil::FromWideStr(l_exporter.GetHTML(), CP_UTF8);
			}

			::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

			FILE* l_file;
			if (_tfopen_s(&l_file, l_filePath.c_str(), _T("wb")) == 0 && l_file)
			{
				fwrite(l_utf8.c_str(), l_utf8.size(), 1, l_file);
				fclose(l_file);

				this->MessageBox(_T("File saved!"), _T("Success"), MB_ICONINFORMATION);
			}
			else
			{
				this->MessageBox(_T("Unable to open file for writing!"), _T("Fail"), MB_ICONEXCLAMATION);
			}
		}
	}
	else if (a_id == IDM_EXPORT_PDF || a_id == IDM_EXPORT_PDF_DIN)
	{
		COMDLG_FILTERSPEC l_filter[] = { { L"PDF File", L"*.pdf" } };

		const std::wstring l_filePath = CUtilWin32GUI::SaveFileDialog(g_hInstance, GetHwnd(),
			l_filter, 1, _T("pdf"), l_baseFileName + _T(".pdf"), l_defaultPath);

		if (!l_filePath.empty())
		{
			::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

			CNFOToPDF l_exporter(m_view.GetActiveCtrl() != m_view.GetRenderCtrl());
			l_exporter.SetUseDINSizes(a_id == IDM_EXPORT_PDF_DIN);
			l_exporter.AssignNFO(m_view.GetNfoData());
			l_exporter.InjectSettings(m_view.GetActiveCtrl()->GetSettings());

			if (l_exporter.SavePDF(l_filePath))
			{
				this->MessageBox(_T("File saved!"), _T("Success"), MB_ICONINFORMATION);
			}
			else
			{
				this->MessageBox(_T("An error occured while trying to save this NFO as PDF!"), _T("Fail"), MB_ICONEXCLAMATION);
			}
		}
	}

	::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
}


void CMainFrame::BrowseFolderNfoMove(int a_direction)
{
	if (m_nfoInFolderIndex == (size_t)-1 /* :TODO: || directory timestamp changed */)
	{
		if (!LoadFolderNfoList())
		{
			return;
		}
		// else: m_nfoInFolderIndex is now set to the current file
	}

	WatchFileStop();

	m_nfoInFolderIndex = BrowseFolderNfoGetNext(a_direction);

	bool bSuccess;

	::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

	// use preloaded NFO if there is one:
	if (m_nfoPreloadData && m_nfoPreloadData->GetFilePath() == m_nfoPathsInFolder[m_nfoInFolderIndex])
	{
		bSuccess = m_view.OpenLoadedFile(m_nfoPreloadData);
	}
	else if (m_nfoPathsInFolder.size() > 0) // load from disk otherwise:
	{
		PNFOData l_nfo = std::make_shared<CNFOData>();
		l_nfo->SetWrapLines(m_settings->bWrapLines);

		if (l_nfo->LoadFromFile(m_nfoPathsInFolder[m_nfoInFolderIndex]))
		{
			bSuccess = m_view.OpenLoadedFile(l_nfo);
		}
		else
		{
			// skip files that can not be opened, also remove them from the list:

			m_nfoPathsInFolder.erase(m_nfoPathsInFolder.begin() + m_nfoInFolderIndex);

			return BrowseFolderNfoMove(0);
		}
	}

	if (bSuccess)
	{
		// don't add to MRU.

		UpdateStatusbar();

		AdjustWindowToNFOWidth(true);

		UpdateCaption();

		m_lastSearchTerm = L"";

		RedrawWindow();

		WatchFileStart();
	}

	// pre-load next:
	size_t l_preLoadIndex = BrowseFolderNfoGetNext(a_direction);

	if (l_preLoadIndex != m_nfoInFolderIndex && l_preLoadIndex != (size_t)-1)
	{
		m_nfoPreloadData.reset();

		m_nfoPreloadData = std::make_shared<CNFOData>();
		m_nfoPreloadData->SetWrapLines(m_settings->bWrapLines);

		if (!m_nfoPreloadData->LoadFromFile(m_nfoPathsInFolder[l_preLoadIndex]))
		{
			// ignore failed pre-loads.

			m_nfoPreloadData.reset();
		}
	}
}


size_t CMainFrame::BrowseFolderNfoGetNext(int a_direction)
{
	if (a_direction <= 0 && m_nfoInFolderIndex == 0)
	{
		return m_nfoPathsInFolder.size() - 1;
	}
	else if (a_direction >= 0 && m_nfoInFolderIndex >= m_nfoPathsInFolder.size() - 1)
	{
		return 0;
	}
	else
	{
		return m_nfoInFolderIndex + sgn(a_direction);
	}
}


bool CMainFrame::LoadFolderNfoList()
{
	if (!m_view || !m_view.GetNfoData())
	{
		return false;
	}

	m_nfoInFolderIndex = (size_t)-1;
	m_nfoPathsInFolder.clear();

	const std::wstring l_currentNfoPath = m_view.GetNfoData()->GetFilePath();
	wchar_t l_nfoPathFull[1000]{};

	const std::wstring l_folderPath = CUtilWin32::PathRemoveFileSpec(l_currentNfoPath.c_str()) + L"\\",
		l_filePattern = l_folderPath + L"*.*";

	if (!::PathIsDirectory(l_folderPath.c_str()) ||
		::GetFullPathName(l_currentNfoPath.c_str(), 999, l_nfoPathFull, nullptr) > 999)
	{
		// what happened?!
		return false;
	}

	const std::wstring l_extraExtension = ::PathFindExtension(l_currentNfoPath.c_str());

	WIN32_FIND_DATA l_ffd{};
	HANDLE l_fh;

	if ((l_fh = ::FindFirstFile(l_filePattern.c_str(), &l_ffd)) == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	do
	{
		const std::wstring l_nfoPath = l_folderPath + l_ffd.cFileName;
		const std::wstring l_extension = ::PathFindExtension(l_nfoPath.c_str());

		if (_wcsicmp(l_extension.c_str(), L".nfo") == 0 || _wcsicmp(l_extension.c_str(), L".asc") == 0
			|| _wcsicmp(l_extension.c_str(), L".ans") == 0 || _wcsicmp(l_extension.c_str(), l_extraExtension.c_str()) == 0)
		{
			wchar_t l_buf[1000]{};

			if (::GetFullPathName(l_nfoPath.c_str(), 999, l_buf, nullptr) < 1000)
			{
				m_nfoPathsInFolder.emplace_back(l_buf);
			}
		}
	} while (::FindNextFile(l_fh, &l_ffd));

	::FindClose(l_fh);

	std::sort(m_nfoPathsInFolder.begin(), m_nfoPathsInFolder.end(), [](const std::wstring& a, const std::wstring& b) {
		return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
		});

	if (::PathFileExists(l_nfoPathFull))
	{
		size_t l_index = 0;
		for (const std::wstring& l_nfoPath : m_nfoPathsInFolder)
		{
			if (wcscmp(l_nfoPathFull, l_nfoPath.c_str()) == 0)
			{
				m_nfoInFolderIndex = l_index;
				break;
			}

			++l_index;
		}
	}
	else if (m_nfoPathsInFolder.size() > 0)
	{
		// if the file is gone, just start from the beginning:
		m_nfoInFolderIndex = m_nfoPathsInFolder.size();
	}

	return (m_nfoInFolderIndex != (size_t)-1);
}


bool CMainFrame::SaveRenderSettingsToRegistry(const std::_tstring& a_key,
	const CNFORenderSettings& a_settings, bool a_classic)
{
	PSettingsSection l_sect;

	if (!CNFOApp::GetInstance()->GetSettingsBackend()->OpenSectionForWriting(a_key, l_sect))
	{
		return false;
	}

	l_sect->WriteDword(L"ClrText", a_settings.cTextColor.AsWord());
	l_sect->WriteDword(L"ClrBack", a_settings.cBackColor.AsWord());
	l_sect->WriteDword(L"ClrArt", a_settings.cArtColor.AsWord());
	l_sect->WriteDword(L"ClrLink", a_settings.cHyperlinkColor.AsWord());

	l_sect->WriteBool(L"HilightHyperlinks", a_settings.bHilightHyperlinks);
	l_sect->WriteBool(L"UnderlineHyperlinks", a_settings.bUnderlineHyperlinks);
	l_sect->WriteBool(L"FontBold", a_settings.bFontBold);
	l_sect->WriteBool(L"FontAntiAlias", a_settings.bFontAntiAlias);

	if (!a_classic)
	{
		l_sect->WriteDword(L"BlockHeight", static_cast<DWORD>(a_settings.uBlockHeight));
		l_sect->WriteDword(L"BlockWidth", static_cast<DWORD>(a_settings.uBlockWidth));

		l_sect->WriteBool(L"GaussShadow", a_settings.bGaussShadow);
		l_sect->WriteDword(L"GaussBlurRadius", a_settings.uGaussBlurRadius);
		l_sect->WriteBool(L"GaussANSI", a_settings.bGaussANSI);

		l_sect->WriteDword(L"ClrGauss", a_settings.cGaussColor.AsWord());
	}
	else
	{
		l_sect->WriteDword(L"FontSize", static_cast<DWORD>(a_settings.uFontSize));
	}

	return l_sect->WriteString(L"FontName", a_settings.sFontFace); /* somewhat representative success check */
}


bool CMainFrame::LoadRenderSettingsFromRegistry(const std::_tstring& a_key, CNFORenderer* a_target)
{
	PSettingsSection l_sect;

	if (!CNFOApp::GetInstance()->GetSettingsBackend()->OpenSectionForReading(a_key, l_sect))
	{
		return false;
	}

	CNFORenderer l_dummy;
	CNFORenderSettings l_newSets, l_defaults = l_dummy.GetSettings();

	l_newSets.cTextColor = _s_color_t(l_sect->ReadDword(L"ClrText", l_defaults.cTextColor.AsWord()));
	l_newSets.cBackColor = _s_color_t(l_sect->ReadDword(L"ClrBack", l_defaults.cBackColor.AsWord()));
	l_newSets.cArtColor = _s_color_t(l_sect->ReadDword(L"ClrArt", l_defaults.cArtColor.AsWord()));
	l_newSets.cHyperlinkColor = _s_color_t(l_sect->ReadDword(L"ClrLink", l_defaults.cHyperlinkColor.AsWord()));

	l_newSets.bHilightHyperlinks = l_sect->ReadBool(L"HilightHyperlinks", l_defaults.bHilightHyperlinks);
	l_newSets.bUnderlineHyperlinks = l_sect->ReadBool(L"UnderlineHyperlinks", l_defaults.bUnderlineHyperlinks);
	l_newSets.bFontBold = l_sect->ReadBool(L"FontBold", l_defaults.bFontBold);
	l_newSets.bFontAntiAlias = l_sect->ReadBool(L"FontAntiAlias", l_defaults.bFontAntiAlias);

	if (!a_target->IsClassicMode())
	{
		l_newSets.cGaussColor = _s_color_t(l_sect->ReadDword(L"ClrGauss", l_defaults.cGaussColor.AsWord()));
		l_newSets.bGaussShadow = l_sect->ReadBool(L"GaussShadow", l_defaults.bGaussShadow);
		l_newSets.bGaussANSI = l_sect->ReadBool(L"GaussANSI", l_defaults.bGaussANSI);
		l_newSets.uBlockHeight = l_sect->ReadDword(L"BlockHeight", (DWORD)l_defaults.uBlockHeight);
		l_newSets.uBlockWidth = l_sect->ReadDword(L"BlockWidth", (DWORD)l_defaults.uBlockWidth);
		l_newSets.uGaussBlurRadius = l_sect->ReadDword(L"GaussBlurRadius", (DWORD)l_defaults.uGaussBlurRadius);
	}
	else
	{
		l_newSets.uFontSize = l_sect->ReadDword(L"FontSize", (DWORD)l_defaults.uFontSize);
	}

	std::wstring l_fontFace = l_sect->ReadString(L"FontName");
	if (l_fontFace.size() > 0 && l_fontFace.size() < LF_FACESIZE)
	{
		_tcsncpy_s(l_newSets.sFontFace, LF_FACESIZE + 1, l_fontFace.c_str(), l_fontFace.size());
	}

	a_target->InjectSettings(l_newSets);

	return true;
}


const std::_tstring CMainFrame::InfektVersionAsString()
{
	return FORMAT(_T("%d.%d.%d"), INFEKT_VERSION_MAJOR % INFEKT_VERSION_MINOR % INFEKT_VERSION_REVISION);
}


void CMainFrame::CheckForUpdates()
{
	PWinHttpClient l_client(new CWinHttpClient(GetApp()->GetInstanceHandle()));

	const wchar_t* urls[] = {
		L"https://infekt.ws/prog/CurrentVersion.txt",
		L"https://syndicode.org/infekt/prog/CurrentVersion.txt",
	};

	const auto url = urls[::time(nullptr) % 2];

	PWinHttpRequest l_req = l_client->CreateRequestForTextFile(url,
		[this](const auto& req) { CheckForUpdates_Callback(req); });

	l_req->SetBypassCache(true);
	l_client->StartRequest(l_req);
}


void CMainFrame::CheckForUpdates_Callback(PWinHttpRequest a_req)
{
	wstring l_contents = CUtil::ToWideStr(a_req->GetBufferContents(), CP_UTF8);
	wstring l_serverVersion, l_newDownloadUrl, l_autoUpdateUrl, l_autoUpdateHash;

	wstring::size_type l_pos = l_contents.find(L"{{{"), l_endPos, l_prevPos;

	if (l_pos != wstring::npos)
	{
		l_pos += 3;
		l_endPos = l_contents.find(L"}}}", l_pos);

		if (l_endPos != wstring::npos)
		{
			map<const wstring, wstring> l_pairs;
			l_contents = l_contents.substr(l_pos, l_endPos - l_pos);

			l_prevPos = 0;
			l_pos = l_contents.find(L'\n');

			while (l_pos != wstring::npos)
			{
				wstring l_line = l_contents.substr(l_prevPos, l_pos - l_prevPos);
				wstring::size_type l_equalPos = l_line.find(L'=');

				if (l_equalPos != wstring::npos)
				{
					wstring l_left = l_line.substr(0, l_equalPos);
					wstring l_right = l_line.substr(l_equalPos + 1);
					CUtil::StrTrim(l_left);
					CUtil::StrTrim(l_right);

					l_pairs[l_left] = l_right;
				}

				l_prevPos = l_pos + 1;
				l_pos = l_contents.find(L'\n', l_prevPos);
			}

			l_serverVersion = l_pairs[L"latest[stable].1"];
			l_newDownloadUrl = l_pairs[L"download_latest[stable].1"];
			// 087 defines the compatible auto-update method
			l_autoUpdateUrl = l_pairs[L"autoupdate_download[stable].1/087"];
			l_autoUpdateHash = l_pairs[L"autoupdate_hash[stable].1/087"];
		}
	}

	l_contents.clear();

	bool l_validData = !l_serverVersion.empty() && ::PathIsURL(l_newDownloadUrl.c_str()) &&
		(l_newDownloadUrl.find(L"http://") == 0 || l_newDownloadUrl.find(L"https://") == 0);

	if (!l_validData)
	{
		const wstring l_msg = L"Failed to contact the update server to get the latest version's info. "
			L"Please make sure you are connected to the internet and try again later.\n\nDo you want to visit https://infekt.ws/ now instead?";

		if (this->MessageBox(l_msg.c_str(), L"Connection Problem", MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
		{
			::ShellExecute(0, L"open", L"https://infekt.ws/", nullptr, nullptr, SW_SHOWNORMAL);
		}

		return;
	}

	int l_result = CUtil::VersionCompare(InfektVersionAsString(), l_serverVersion);

	if (l_result == 0)
	{
		const wstring l_msg = L"You are using the latest stable version (" + InfektVersionAsString() + L")!";
		this->MessageBox(l_msg.c_str(), L"Nice.", MB_ICONINFORMATION);
	}
	else if (l_result < 0)
	{
		const wstring l_auExePath = CUtilWin32::GetExeDir() + L"\\infekt-win32-updater.exe";
		bool l_auPossible = ::PathFileExists(l_auExePath.c_str()) &&
			!l_autoUpdateHash.empty() && !l_autoUpdateUrl.empty();

		wstring l_msg = L"Attention! A new version is available (iNFekt v" + l_serverVersion + L")!\n\n";

		if (!l_auPossible)
		{
			// auto update has been disabled from this version or is not available for other reasons,
			// use classic manual approach.

			l_msg += L"Auto-update is not possible - e.g. because this is a portable version. Do you want to go to the download page now?";
		}
		else
		{
			l_msg += L"Do you want to close iNFekt and install the new version now?";
		}

		if (this->MessageBox(l_msg.c_str(), L"New Version Found", MB_ICONEXCLAMATION | MB_YESNO) != IDYES)
		{
			// update cancelled
			return;
		}

		if (!l_auPossible)
		{
			// URL has been validated above.
			::ShellExecute(0, L"open", l_newDownloadUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		}
		else
		{
			// try to perform auto-update.
			std::wstring l_tmpPath = CUtilWin32::GetTempDir();

			if (!l_tmpPath.empty())
			{
				std::wstring l_tempExePath(l_tmpPath);
				l_tempExePath += L"infekt-" + InfektVersionAsString() + L"-updater.exe";

				if (::CopyFile(l_auExePath.c_str(), l_tempExePath.c_str(), FALSE))
				{
					const std::wstring l_args = L"\"" + l_autoUpdateUrl + L"\" " + l_autoUpdateHash;

					::ShellExecute(0, L"open", l_tempExePath.c_str(), l_args.c_str(), nullptr, SW_SHOWNORMAL);
				}
				else
				{
					this->MessageBox(L"Error copying to temp folder. Please make sure auto-update is not already running.", L"Error", MB_ICONSTOP);
				}
			}
		}
	}
	else if (l_result > 0)
	{
		this->MessageBox(L"Looks like you compiled from source. Your version is newer than the latest stable one!",
			L"Nice.", MB_ICONINFORMATION);
	}
}


void CMainFrame::LoadOpenMruList()
{
	PSettingsSection l_sect;

	if (!CNFOApp::GetInstance()->GetSettingsBackend()->OpenSectionForReading(L"OpenMRU", l_sect))
	{
		return;
	}

	m_mruPaths.clear();

	for (size_t i = 0; i < ms_mruLength; i++)
	{
		const wstring l_valName = FORMAT(L"%d", i);
		const std::wstring l_path = l_sect->ReadString(l_valName.c_str());

		if (!l_path.empty())
		{
			// 2011-08: Don't check whether the file exists here
			// in order to avoid unnecessarily spinning up drives.
			// Check when clicking the menu entry instead (bug #58).
			m_mruPaths.push_back(l_path);
		}
	}
}


void CMainFrame::SaveOpenMruList()
{
	PSettingsSection l_sect;

	if (!CNFOApp::GetInstance()->GetSettingsBackend()->OpenSectionForWriting(L"OpenMRU", l_sect))
	{
		return;
	}

	vector<_tstring>::const_iterator l_it = m_mruPaths.begin();
	for (size_t i = 0; i < ms_mruLength; i++)
	{
		const wstring l_valName = FORMAT(L"%d", i);

		if (l_it != m_mruPaths.end())
		{
			l_sect->WriteString(l_valName.c_str(), *l_it);
			l_it++;
		}
		else
		{
			l_sect->DeletePair(l_valName.c_str());
		}
	}
}


void CMainFrame::SavePositionSettings()
{
	PSettingsSection l_sect;

	if (!CNFOApp::GetInstance()->GetSettingsBackend()->OpenSectionForWriting(L"Frame Settings", l_sect))
	{
		return;
	}

	WINDOWPLACEMENT l_wpl = { sizeof(WINDOWPLACEMENT), 0 };
	if (GetWindowPlacement(l_wpl))
	{
		CRect rc = l_wpl.rcNormalPosition;

		l_sect->WriteDword(L"Top", rc.top);
		l_sect->WriteDword(L"Left", rc.left);
		l_sect->WriteDword(L"Width", MAX(rc.Width(), ms_minWidth));
		l_sect->WriteDword(L"Height", MAX(rc.Height(), ms_minHeight));

		l_sect->WriteBool(L"Maximized", l_wpl.showCmd == SW_MAXIMIZE);
	}

	l_sect->WriteBool(L"Toolbar", m_bShowToolbar != FALSE);
	l_sect->WriteBool(L"Statusbar", m_bShowStatusbar != FALSE);
}


// :TODO: move to nicer place
void CMainFrame::SaveActivePluginsToRegistry()
{
	PSettingsSection l_sect;

	if (!CNFOApp::GetInstance()->GetSettingsBackend()->OpenSectionForWriting(L"ActivePlugins", l_sect))
	{
		return;
	}

	std::vector<std::wstring> l_dllPaths;

	CPluginManager::GetInstance()->GetLoadedPlugins(l_dllPaths);

	l_sect->WriteDword(L"COUNT", static_cast<DWORD>(l_dllPaths.size()));

	DWORD i = 0;
	for (const std::wstring& path : l_dllPaths)
	{
		const std::wstring l_dllName = ::PathFindFileName(path.c_str()); // that shit better be in "plugins"...
		const std::wstring l_valName = FORMAT(L"%d", i);

		l_sect->WriteString(l_valName.c_str(), l_dllName);
	}
}


// :TODO: move to nicer place
void CMainFrame::LoadActivePluginsFromRegistry()
{
	PSettingsSection l_sect;

	if (!CNFOApp::GetInstance()->GetSettingsBackend()->OpenSectionForReading(L"ActivePlugins", l_sect))
	{
		return;
	}

	const std::wstring l_pluginDirPath = CUtilWin32::GetExeDir() + _T("\\plugins\\");

	DWORD count = l_sect->ReadDword(L"COUNT");

	for (DWORD i = 0; i < count; ++i)
	{
		const std::wstring l_valName = FORMAT(L"%d", i);
		const std::wstring l_dllName = l_sect->ReadString(l_valName.c_str());

		if (!l_dllName.empty())
		{
			const std::wstring l_dllPath = l_pluginDirPath + l_dllName;

			// more checks here?!

			if (::PathFileExists(l_dllPath.c_str()))
			{
				CPluginManager::GetInstance()->LoadPlugin(l_dllPath);
			}
		}
	}
}


void CMainFrame::OnClose()
{
	SavePositionSettings();

	if (!m_settings->bKeepOpenMRU)
	{
		m_mruPaths.clear();
		SaveOpenMruList();
	}

	SaveActivePluginsToRegistry();

	CFrame::OnClose();
}


CMainFrame::~CMainFrame()
{
	delete m_searchToolbar;
}
