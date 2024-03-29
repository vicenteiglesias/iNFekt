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

#ifndef _NFO_RENDERER_H
#define _NFO_RENDERER_H

#include "nfo_data.h"
#include "cairo_box_blur.h"

// http://www.alanwood.net/unicode/block_elements.html
// http://www.alanwood.net/demos/wgl4.html

typedef enum _render_grid_shape_t
{
	RGS_NO_BLOCK = 0,
	RGS_FULL_BLOCK,
	RGS_BLOCK_LOWER_HALF,
	RGS_BLOCK_UPPER_HALF,
	RGS_BLOCK_LEFT_HALF,
	RGS_BLOCK_RIGHT_HALF,
	RGS_BLACK_SQUARE,
	RGS_BLACK_SMALL_SQUARE,
	RGS_WHITESPACE,
	RGS_WHITESPACE_IN_TEXT,

	_RGS_MAX
} ERenderGridShape;


typedef struct _render_grid_block_t
{
	ERenderGridShape shape;
	uint8_t alpha; /* 0 = invisible, 255 = opaque */
} CRenderGridBlock;


typedef struct _s_color_t
{
	uint8_t R, G, B;
	uint8_t A; /* 0 = invisible, 255 = opaque */

	_s_color_t() { R = G = B = A = 255; }
	_s_color_t(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) { R = r; G = g;  B = b; A = a; }
	_s_color_t(uint32_t wd) { A = (uint8_t)(wd & 0xFF); B = (uint8_t)((wd >> 8) & 0xFF); G = (uint8_t)((wd >> 16) & 0xFF); R = (uint8_t)((wd >> 24) & 0xFF); }

	bool operator==(const _s_color_t &o) const { return (o.R == R && o.G == G && o.B == B && o.A == A); }
	bool operator!=(const _s_color_t &o) const { return !(*this == o); }

	_s_color_t Invert() const { return _s_color_t(255 - R, 255 - G, 255 - B, A); }
	uint32_t AsWord() const { return (A) | (B << 8) | (G << 16) | (R << 24); }
	std::wstring AsHex(bool a_alpha) const { wchar_t l_buf[10] = {0};
		if(a_alpha) swprintf(l_buf, 9, L"%02x%02x%02x%02x", R, G, B, A); else swprintf(l_buf, 9, L"%02x%02x%02x", R, G, B);
		return l_buf; }
} S_COLOR_T;

#define _S_COLOR(R, G, B, A) _s_color_t(R, G, B, A)
#define _S_COLOR_RGB(R, G, B) _s_color_t(R, G, B, 0xFF)

#define S_COLOR_T_CAIRO(CLR) (CLR).R / 255.0, (CLR).G / 255.0, (CLR).B / 255.0
#define S_COLOR_T_CAIRO_A(CLR) (CLR).R / 255.0, (CLR).G / 255.0, (CLR).B / 255.0, (CLR).A / 255.0


class CNFORenderSettings
{
/* NEVER add string or pointer members to this class without
	precautions, it's being copied like a struct etc. */
public:
	CNFORenderSettings()
		: uBlockHeight(0)
		, uBlockWidth(0)
		, uFontSize(0)
		, cBackColor(0)
		, cTextColor(0)
		, cArtColor(0)
		, cGaussColor(0)
		, cHyperlinkColor(0)
		, bGaussShadow(false)
		, uGaussBlurRadius(0)
		, bGaussANSI(true)
		, bFontBold(false)
		, bFontAntiAlias(true)
		, bHilightHyperlinks(true)
		, bUnderlineHyperlinks(true)
#ifdef _WIN32
		, sFontFace(_T("Lucida Console"))
#else
		, sFontFace(_T("monospace"))
#endif
	{
	}

	// main settings:
	size_t uBlockHeight, uBlockWidth;
	size_t uFontSize;
	S_COLOR_T cBackColor, cTextColor, cArtColor;
	TCHAR sFontFace[LF_FACESIZE + 1];
	bool bFontBold;
	bool bFontAntiAlias;

	// blur effect settings:
	S_COLOR_T cGaussColor;
	bool bGaussShadow;
	unsigned int uGaussBlurRadius;
	bool bGaussANSI;

	// hyperlink settings:
	bool bHilightHyperlinks;
	S_COLOR_T cHyperlinkColor;
	bool bUnderlineHyperlinks;

	std::wstring Serialize() const;
	bool UnSerialize(std::wstring, bool a_classic);
};


typedef enum _nfo_render_partial_t
{
	NRP_RENDER_TEXT = 1,
	NRP_RENDER_BLOCKS = 2,
	NRP_RENDER_GAUSS_BLOCKS = 4,
	NRP_RENDER_GAUSS_SHADOW = 8,
	NRP_RENDER_GAUSS = NRP_RENDER_GAUSS_BLOCKS | NRP_RENDER_GAUSS_SHADOW,

	NRP_RENDER_EVERYTHING = NRP_RENDER_TEXT | NRP_RENDER_BLOCKS | NRP_RENDER_GAUSS_BLOCKS | NRP_RENDER_GAUSS_SHADOW,
	_NRP_MAX
} ENFORenderPartial;


class CNFORenderer
{
private:
	CNFORenderSettings m_settings;
	bool m_classic;
	float m_zoomFactor;

	ENFORenderPartial m_partial;

	// internal state data:
	// don't mess with these, they are NOT settings:
	bool m_rendered;
	double m_fontSize;

	size_t m_linesPerStripe; // in no. of lines
	int m_stripeHeight; // in pixels

	// for background pre-rendering:
	std::shared_ptr<std::thread> m_preRenderThread;
	std::atomic<size_t> m_preRenderingStripe;
	std::mutex m_stripesLock;
	bool m_stopPreRendering;
	bool m_cancelRenderingImmediately;

	void PreRenderThreadProc();

protected:
	bool m_forceGPUOff;
	bool m_allowCPUFallback;
	bool m_onDemandRendering;

	int m_padding;

	PNFOData m_nfo;
	std::unique_ptr<TwoDimVector<CRenderGridBlock>> m_gridData;
	bool m_hasBlocks;

	size_t m_numStripes;
	std::map<size_t, PCairoSurface> m_stripes; // stripe no. -> surface

	// internal calls:
	bool IsRendered() const { return m_rendered; }
	bool IsAnsi() const { return m_nfo && m_nfo->HasColorMap(); }
	bool CalculateGrid();
	cairo_surface_t *GetStripeSurface(size_t a_stripe) const;
	int GetStripeHeight(size_t a_stripe) const;
	int GetStripeHeightExtraTop(size_t a_stripe) const;
	int GetStripeHeightExtraBottom(size_t a_stripe) const;
	int GetStripeHeightPhysical(size_t a_stripe) const;
	size_t GetStripeExtraLinesTop(size_t a_stripe) const;
	size_t GetStripeExtraLinesBottom(size_t a_stripe) const;
	void RenderStripe(size_t a_stripe) const;
	void RenderStripeBlocks(size_t a_stripe, bool a_opaqueBg, bool a_gaussStep, cairo_t* a_context = nullptr) const;
	void RenderBackgrounds(size_t a_rowStart, size_t a_rowEnd, double a_yBase, cairo_t* a_context) const;

	void RenderBlocks(bool a_opaqueBg, bool a_gaussStep, cairo_t* a_context = nullptr,
		size_t a_rowStart = (size_t)-1, size_t a_rowEnd = 0, double a_xBase = 0, double a_yBase = 0) const;
	void PreRenderText();
	void RenderText(const S_COLOR_T& a_textColor, const S_COLOR_T* a_backColor,
		const S_COLOR_T& a_hyperLinkColor,
		size_t a_rowStart, size_t a_colStart, size_t a_rowEnd, size_t a_colEnd,
		cairo_surface_t* a_surface, double a_xBase, double a_yBase) const noexcept;

	void RenderClassic(const S_COLOR_T& a_textColor, const S_COLOR_T* a_backColor,
		const S_COLOR_T& a_hyperLinkColor, bool a_backBlocks,
		size_t a_rowStart, size_t a_colStart, size_t a_rowEnd, size_t a_colEnd,
		cairo_surface_t* a_surface, double a_xBase, double a_yBase) const;
	bool CalcClassicModeBlockSizes(bool a_force = false);

	bool IsTextChar(size_t a_row, size_t a_col, bool a_allowWhiteSpace = false) const;
	static void _FixUpRowColStartEnd(size_t& a_rowStart, size_t& a_colStart, size_t& a_rowEnd, size_t& a_colEnd);

	void ClearStripes();
	void CalcStripeDimensions();
	void WaitForPreRender();
	void StopPreRendering(bool a_cancel = false);
	void PreRender();

	static const size_t ms_defaultClassicFontSize = 12;
	static bool ms_useGPU;

public:
	CNFORenderer(bool a_classicMode = false);
	virtual ~CNFORenderer();

	static ERenderGridShape CharCodeToGridShape(wchar_t a_char, uint8_t* ar_alpha = nullptr);

	static void SetGlobalUseGPUFlag(bool nb) {
		ms_useGPU = nb; }
	static bool GetGlobalUseGPUFlag() {
		return ms_useGPU; }

	// mainly important methods:
	virtual void UnAssignNFO();
	virtual bool AssignNFO(const PNFOData& a_nfo);
	bool HasNfoData() const { return (m_nfo && m_nfo->HasData() ? true : false); }
	const PNFOData& GetNfoData() const { return m_nfo; }
	virtual bool DrawToSurface(cairo_surface_t *a_surface, int dest_x, int dest_y,
		int source_x, int source_y, int width, int height);
	virtual bool DrawToClippedHandle(cairo_t* a_cr, int dest_x, int dest_y);
	// you should not call this directly without a good reason, prefer DrawToSurface:
	bool Render(size_t a_stripeFrom = 0, size_t a_stripeTo = (~1));

	bool IsClassicMode() const { return m_classic; }

	void SetPartialMode(ENFORenderPartial pm) { m_rendered = m_rendered && (m_partial == pm); m_partial = pm; }

	unsigned int GetZoom() const { return static_cast<unsigned int>(m_zoomFactor * 100); }
	virtual void SetZoom(unsigned int a_percent);

	// return the calculated image dimensions:
	size_t GetWidth() const;
	size_t GetHeight() const;

	// color setters & getters:
	void SetBackColor(const S_COLOR_T& nc) { m_rendered = m_rendered && (m_settings.cBackColor == nc); m_settings.cBackColor = nc; }
	void SetTextColor(const S_COLOR_T& nc) { m_rendered = m_rendered && (m_settings.cTextColor == nc); m_settings.cTextColor = nc; }
	void SetArtColor(const S_COLOR_T& nc) { m_rendered = m_rendered && (m_settings.cArtColor == nc); m_settings.cArtColor = nc; }
	void SetGaussColor(const S_COLOR_T& nc) { m_rendered = m_rendered && (m_settings.cGaussColor == nc); m_settings.cGaussColor = nc; }
	void SetHyperLinkColor(const S_COLOR_T& nc) { m_rendered = m_rendered && (m_settings.cHyperlinkColor == nc); m_settings.cHyperlinkColor = nc; }
	S_COLOR_T GetBackColor() const {
		return IsAnsi() ? S_COLOR_T(0, 0, 0) : m_settings.cBackColor;
	}
	S_COLOR_T GetTextColor() const {
		return IsAnsi() ? S_COLOR_T(100, 100, 100) : m_settings.cTextColor;
	}
	S_COLOR_T GetArtColor() const {
		return IsAnsi() ? S_COLOR_T(100, 100, 100) : m_settings.cArtColor;
	}
	S_COLOR_T GetGaussColor() const {
		return IsAnsi() ? S_COLOR_T(100, 100, 100) /* relevant when default color is used */ : m_settings.cGaussColor;
	}
	S_COLOR_T GetHyperLinkColor() const {
		return IsAnsi() ? S_COLOR_T(92, 92, 255) : m_settings.cHyperlinkColor;
	}

	// various other setters & getters:
	void SetEnableGaussShadow(bool nb, bool nb_ansi = true) {
		m_rendered = m_rendered && (m_settings.bGaussShadow == nb); m_settings.bGaussShadow = nb;
		m_rendered = m_rendered && (m_settings.bGaussANSI == nb_ansi); m_settings.bGaussANSI = nb_ansi;
	}
	bool GetEnableGaussShadow() const { return (IsAnsi() ? m_settings.bGaussANSI : m_settings.bGaussShadow); }
	void SetGaussBlurRadius(unsigned int r) {
		m_rendered = m_rendered && (m_settings.uGaussBlurRadius == r); m_settings.uGaussBlurRadius = r;
	}
	unsigned int GetGaussBlurRadius() const { return m_settings.uGaussBlurRadius; }

	void SetHilightHyperLinks(bool nb) { m_rendered = m_rendered && (m_settings.bHilightHyperlinks == nb); m_settings.bHilightHyperlinks = nb; }
	bool GetHilightHyperLinks() const { return m_settings.bHilightHyperlinks; }
	void SetUnderlineHyperLinks(bool nb) { m_rendered = m_rendered && (m_settings.bUnderlineHyperlinks == nb); m_settings.bUnderlineHyperlinks = nb; }
	bool GetUnderlineHyperLinks() const { return m_settings.bUnderlineHyperlinks; }

	void SetFontBold(bool nb) { m_rendered = m_rendered && (m_settings.bFontBold == nb); m_settings.bFontBold = nb; }
	bool GetFontBold() const { return m_settings.bFontBold; }

	void SetFontAntiAlias(bool nb) { m_rendered = m_rendered && (m_settings.bFontAntiAlias == nb); m_settings.bFontAntiAlias = nb; }
	bool GetFontAntiAlias() const { return m_settings.bFontAntiAlias; }

	void SetFontFace(const std::_tstring& ns) { m_rendered = m_rendered && (_tcscmp(m_settings.sFontFace, ns.c_str()) == 0);
		_tcsncpy_s(m_settings.sFontFace, LF_FACESIZE + 1, ns.c_str(), LF_FACESIZE);
	}
	std::_tstring GetFontFace() const { return m_settings.sFontFace; }

	int GetPadding() const {
		if(!GetEnableGaussShadow()) {
			return m_padding;
		} else {
			return std::max(m_settings.uGaussBlurRadius, 8u); // space for blur/shadow effect near the edges
		}
	}

	// for the non-classic mode:
	void SetBlockSize(size_t a_width, size_t a_height) { if(!m_classic) { m_rendered = m_rendered &&
		a_width == m_settings.uBlockWidth && a_height == m_settings.uBlockHeight; m_settings.uBlockWidth = a_width; m_settings.uBlockHeight = a_height;
		if(!m_rendered) { m_fontSize = -1; }
	}}
	size_t GetBlockWidth() const { return (!m_classic ? static_cast<size_t>(m_settings.uBlockWidth * m_zoomFactor + 0.5) : m_settings.uBlockWidth); }
	size_t GetBlockHeight() const { return (!m_classic ? static_cast<size_t>(m_settings.uBlockHeight * m_zoomFactor + 0.5) : m_settings.uBlockHeight); }

	// for the classic mode:
	size_t GetFontSize() const { return (m_classic ? static_cast<size_t>(m_settings.uFontSize * m_zoomFactor + 0.5) : (size_t)-1); }
	void SetFontSize(size_t r) {
		if(m_classic) { m_rendered = m_rendered && (m_settings.uFontSize == r); m_settings.uFontSize = r; m_fontSize = -1; }
	}

	// for quick switching between settings:
	const CNFORenderSettings GetSettings() const { return m_settings; }
	virtual void InjectSettings(const CNFORenderSettings& ns);

	// static color helper methods for anyone to use:
	static bool ParseColor(const char* a_str, S_COLOR_T* ar);
	static bool ParseColor(const wchar_t* a_str, S_COLOR_T* ar);
};

#endif /* !_NFO_RENDERER_H */
