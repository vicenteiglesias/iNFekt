/**
 * Copyright (C) 2010 cxxjoe
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

#ifndef _SETTINGS_DLG_H
#define _SETTINGS_DLG_H

#include "main_frame.h"


class CFontListEntry
{
public:
	CFontListEntry(const ENUMLOGFONTEX* a_elf);

	bool IsFixedWidth() const { return ((m_logFont.lfPitchAndFamily & FIXED_PITCH) != 0); }
	const std::_tstring& GetFontName() const { return m_name; }
	int GetNiceSize();
protected:
	std::_tstring m_name;
	LOGFONT m_logFont;
	std::set<int> m_sizes;

	static int CALLBACK FontSizesProc(const LOGFONT*, const TEXTMETRIC*, DWORD, LPARAM);
};

typedef boost::shared_ptr<CFontListEntry> PFontListEntry;


class CSettingsTabDialog;

class CSettingsWindowDialog : public CDialog
{
public:
	CSettingsWindowDialog(UINT nResID, HWND hWndParent = NULL);
	virtual ~CSettingsWindowDialog();

	void SetMainWin(CMainFrame* a_mainWin) { m_mainWin = a_mainWin; }
	CMainFrame* GetMainWin() const { return m_mainWin; }
protected:
	CNonThemedTab m_tabControl;
	CMainFrame* m_mainWin;

	CSettingsTabDialog* m_tabPageGeneral;
	CSettingsTabDialog* m_tabPageRendered;
	CSettingsTabDialog* m_tabPageClassic;
	CSettingsTabDialog* m_tabPageTextOnly;

	std::vector<PFontListEntry> m_fonts;
	static int CALLBACK FontNamesProc(const ENUMLOGFONTEX*, const NEWTEXTMETRICEX*, DWORD, LPARAM);
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
};


class CSettingsTabDialog : public CDialog
{
public:
	CSettingsTabDialog(CSettingsWindowDialog* a_dlg, int a_pageId, UINT nResID);
	virtual ~CSettingsTabDialog();

	bool SaveViewSettings();
	const CNFORenderSettings* GetViewSettings() { return m_viewSettings; }
	void SetFontList(const std::vector<PFontListEntry>& a_fonts) { m_fonts = a_fonts; }
protected:
	int m_pageId;
	CNFORenderSettings* m_viewSettings;
	CMainFrame* m_mainWin;
	CSettingsWindowDialog* m_dlgWin;
	std::vector<PFontListEntry> m_fonts;

	bool IsViewSettingPage() const;
	static bool IsColorButton(UINT a_id);
	void DrawColorButton(const LPDRAWITEMSTRUCT a_dis);
	S_COLOR_T* ColorFromControlId(UINT a_id);
	void DrawFontComboItem(const LPDRAWITEMSTRUCT a_dis);
	void MeasureFontComboItems(LPMEASUREITEMSTRUCT a_mis);

	virtual BOOL OnInitDialog();
	virtual BOOL DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	static const int ms_fontComboPadding = 3;
};


#endif  /* !_SETTINGS_DLG_H */
