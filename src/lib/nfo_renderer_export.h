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

#ifndef _NFO_RENDERER_EXPORT_H
#define _NFO_RENDERER_EXPORT_H

#include "nfo_renderer.h"


class CNFOToHTML
{
protected:
	CNFORenderSettings m_settings;
	PNFOData m_nfo;

	std::wstring m_title;

	static std::wstring XMLEscape(const std::wstring& s);
public:
	CNFOToHTML(const PNFOData& a_nfoData);

	const std::wstring GetHTML(bool a_includeHeaderAndFooter = true);

	const CNFORenderSettings GetSettings() const { return m_settings; }
	void SetSettings(const CNFORenderSettings& ns) { m_settings = ns; }

	void SetTitle(const std::wstring& snt) { m_title = snt; }
};

class CNFOToHTMLCanvas : public CNFORenderer
{
public:
	CNFOToHTMLCanvas();

	const std::string GetSettingsJSONString() const;
	const std::string GetRenderJSONString();
	const std::string GetRenderCodeString() const;
	const std::string GetFullHTML();

protected:
};


#ifdef CAIRO_HAS_PDF_SURFACE

class CNFOToPDF : public CNFORenderer
{
public:
	CNFOToPDF(bool a_classicMode = false);

	bool GetUseDINSizes() const { return m_dinSizes; }
	void SetUseDINSizes(bool nb) { m_dinSizes = nb; }

	bool SavePDF(const std::_tstring& a_filePath);
protected:
	bool m_dinSizes;

	bool CalcPageDimensions(double& a_width, double& a_height);
};

#endif

// PNG export wrapper,
// uses cairo+pixman for small images and
// direct libpng calls for large ones.
class CNFOToPNG : public CNFORenderer
{
public:
	CNFOToPNG(bool a_classicMode = false);

	bool SavePNG(const std::_tstring& a_filePath);
protected:
	bool SaveWithLibpng(const std::_tstring& a_filePath);
};

#endif /* !_NFO_RENDERER_EXPORT_H */
