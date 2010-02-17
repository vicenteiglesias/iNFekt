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

#ifndef _APP_H
#define _APP_H

#include "main_frame.h"

class CNFOApp : public CWinApp
{
public:
	CNFOApp(); 
	virtual ~CNFOApp();
	virtual BOOL InitInstance();

	CMainFrame& GetMainFrame() { return m_Frame; }
	void SetStartupFilePath(std::_tstring a_filePath) { m_startupFilePath = a_filePath; }
	const std::_tstring& GetStartupFilePath() { return m_startupFilePath; }
protected:
	CMainFrame m_Frame;
	std::_tstring m_startupFilePath;
};

extern HINSTANCE g_hInstance;

#endif /* !_APP_H */