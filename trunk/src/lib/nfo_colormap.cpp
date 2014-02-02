﻿/**
 * Copyright (C) 2014 cxxjoe
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
#include "nfo_colormap.h"

#define NFORGB(R, G, B) (1 | ((B) << 8) | ((G) << 16) | ((R) << 24))


CNFOColorMap::CNFOColorMap()
{
	// default mapping = VGA colors
	m_rgbMapping[NFOCOLOR_BLACK] = NFORGB(0, 0, 0);
	m_rgbMapping[NFOCOLOR_RED] = NFORGB(170, 0, 0);
	m_rgbMapping[NFOCOLOR_GREEN] = NFORGB(0, 170, 0);
	m_rgbMapping[NFOCOLOR_YELLOW] = NFORGB(170, 85, 0);
	m_rgbMapping[NFOCOLOR_BLUE] = NFORGB(0, 0, 170);
	m_rgbMapping[NFOCOLOR_MAGENTA] = NFORGB(170, 0, 170);
	m_rgbMapping[NFOCOLOR_CYAN] = NFORGB(0, 170, 170);
	m_rgbMapping[NFOCOLOR_GRAY] = NFORGB(170, 170, 170);
	m_rgbMapping[NFOCOLOR_DARK_GRAY] = NFORGB(85, 85, 85);
	m_rgbMapping[NFOCOLOR_BRIGHT_RED] = NFORGB(255, 85, 85);
	m_rgbMapping[NFOCOLOR_BRIGHT_GREEN] = NFORGB(85, 255, 85);
	m_rgbMapping[NFOCOLOR_BRIGHT_YELLOW] = NFORGB(255, 255, 85);
	m_rgbMapping[NFOCOLOR_BRIGHT_BLUE] = NFORGB(85, 85, 255);
	m_rgbMapping[NFOCOLOR_PINK] = NFORGB(288, 85, 255);
	m_rgbMapping[NFOCOLOR_BRIGHT_CYAN] = NFORGB(85, 255, 255);
	m_rgbMapping[NFOCOLOR_WHITE] = NFORGB(255, 255, 255);

	// init m_previous...
	Clear();
}


void CNFOColorMap::Clear()
{
	m_previousBack.bold = false;
	m_previousBack.color = NFOCOLOR_DEFAULT;

	m_previousFore.bold = false;
	m_previousFore.color = NFOCOLOR_DEFAULT;

	m_stopsFore.clear();
	m_stopsBack.clear();
}


void CNFOColorMap::PushGraphicRendition(size_t a_row, size_t a_col, const std::vector<uint8_t>& a_params)
{
	int bold = -1;
	ENFOColor fore_color = _NFOCOLOR_MAX;
	ENFOColor back_color = _NFOCOLOR_MAX;
	uint32_t fore_color_rgba = 0, back_color_rgba = 0;

	for(uint8_t p : a_params)
	{
		// from SGR parameters on http://en.wikipedia.org/wiki/ANSI_escape_code#CSI_codes

		switch(p)
		{
		case 0: // Reset / Normal
			bold = 0;
			fore_color = NFOCOLOR_DEFAULT;
			back_color = NFOCOLOR_DEFAULT;
			break;
		case 1: // Bold or increased intensity
			bold = 1;
			break;
		case 21: // Bold: off or Underline: Double
		case 22: // Normal color or intensity
			bold = 0;
			break;
		case 38: // Set xterm-256 text color (foreground)
			if(!InterpretAdvancedColor(a_params, fore_color, fore_color_rgba))
				return;
			break;
		case 39: // Default text color (foreground)
			fore_color = NFOCOLOR_DEFAULT;
			break;
		case 48: // Set xterm-256 background color
			if(!InterpretAdvancedColor(a_params, back_color, back_color_rgba))
				return;
			break;
		case 49: // Default background color
			back_color = NFOCOLOR_DEFAULT;
			break;
		default:
			if(p >= 30 && p <= 37)
			{
				fore_color = static_cast<ENFOColor>(NFOCOLOR_BLACK + (p - 30));
			}
			else if(p >= 40 && p <= 47)
			{
				back_color = static_cast<ENFOColor>(NFOCOLOR_BLACK + (p - 40));
			}
			else if(p >= 90 && p <= 97)
			{
				fore_color = static_cast<ENFOColor>(NFOCOLOR_DARK_GRAY + (p - 90));
			}
			else if(p >= 100 && p <= 107)
			{
				back_color = static_cast<ENFOColor>(NFOCOLOR_DARK_GRAY + (p - 100));
			}
		} // end of switch
	} // end of for loop

	if(fore_color != _NFOCOLOR_MAX || bold != -1)
	{
		SNFOColorStop fore_stop(m_previousFore);

		fore_stop.color = fore_color;
		fore_stop.color_rgba = fore_color_rgba;

		m_stopsFore[a_row][a_col] = fore_stop;

		m_previousFore = fore_stop;
	}

	if(back_color != _NFOCOLOR_MAX)
	{
		SNFOColorStop back_stop(m_previousBack);

		back_stop.color = back_color;
		back_stop.color_rgba = back_color_rgba;

		m_stopsBack[a_row][a_col] = back_stop;

		m_previousBack = back_stop;
	}
}


bool CNFOColorMap::InterpretAdvancedColor(const std::vector<uint8_t>& a_params, ENFOColor& ar_color, uint32_t& ar_rgba)
{
	ar_color = NFOCOLOR_RGB; // default

	if(a_params.size() == 5 && a_params[1] == 2)
	{
		ar_rgba = NFORGB(a_params[2], a_params[3], a_params[4]);
	}
	else if(a_params.size() == 3 && a_params[1] == 5)
	{
		uint8_t p = a_params[2];

		if(p >= 0 && p <= 7) // standard colors (as in ESC [ 30..37 m)
		{
			ar_color = static_cast<ENFOColor>(NFOCOLOR_BLACK + p);
		}
		else if(p >= 8 && p <= 0x0F) // high intensity colors (as in ESC [ 90..97 m)
		{
			ar_color = static_cast<ENFOColor>(NFOCOLOR_DARK_GRAY + (p - 8));
		}
		else if(p >= 0x10 && p <= 0xE7) // 6*6*6=216 colors: 16 + 36*r + 6*g + b (0≤r,g,b≤5)
		{
			p -= 0x10;

			uint8_t r = p / 36,
				g = (p - r * 36) / 6,
				b = (p - r * 36 - g * 6);

			ar_rgba = NFORGB((255 / 5) * r, (255 / 5) * g, (255 / 5) * b);
		}
		else // 0xe8-0xff: grayscale from black to white in 24 steps
		{
			uint8_t g = (255 / 23) * (p - 0xE8);

			ar_rgba = NFORGB(g, g, g);
		}
	}
	else
	{
		return false;
	}

	return true;
}


bool CNFOColorMap::GetForegroundColor(size_t a_row, size_t a_col, uint32_t& ar_color)
{
	if(m_stopsFore.empty())
	{
		return false;
	}
}


bool CNFOColorMap::GetLineBackgrounds(size_t a_row, std::vector<size_t>& ar_sections, std::vector<uint32_t>& ar_colors)
{
	if(m_stopsBack.empty())
	{
		return false;
	}

	bool found = false;
	size_t row = (size_t)-1;

	for(row = a_row; row >= 0; --row)
	{
		if(m_stopsBack.find(row) == m_stopsBack.end())
		{
			continue;
		}

		found = true;
		break;
	}

	if(!found)
	{
		return false;
	}

	ar_sections.clear();
	ar_colors.clear();

	if(row == a_row)
	{
	}
	else
	{
		SNFOColorStop last_stop = m_stopsBack[row].rbegin()->second;

		if(last_stop.color == NFOCOLOR_DEFAULT)
		{
			return false;
		}

		ar_colors.push_back(last_stop.color == NFOCOLOR_RGB ? last_stop.color_rgba : m_rgbMapping[last_stop.color]);
	}

	return true;
}
