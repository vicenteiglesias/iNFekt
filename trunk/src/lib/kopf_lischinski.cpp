/**
 * Copyright (C) 2011 cxxjoe
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
#include "kopf_lischinski.h"


CKopfLischinskiNFORenderer::CKopfLischinskiNFORenderer()
	: CNFORenderer(false), m_intermediate(NULL)
{

}


bool CKopfLischinskiNFORenderer::AssignNFO(const PNFOData& a_nfo)
{
	if(!CNFORenderer::AssignNFO(a_nfo))
	{
		return false;
	}

	if(!CalculateGrid())
	{
		return false;
	}

	delete m_intermediate;
	m_intermediate = NULL;

	Transform_GridToBitmap(false);
	Transform_BitmapToGraph();

	return true;
}


template<typename T> void _TwoDimVectorFill(TwoDimVector<typename T>& a_vector, const T a_value, size_t a_x, size_t a_y, size_t a_width, size_t a_height)
{
	for(size_t x = a_x; x < a_x + a_width; x++)
	{
		for(size_t y = a_y; y < a_y + a_height; y++)
		{
			a_vector[x][y] = a_value;
		}
	}
}


/**
 * HighRes = 8x8 pixels, otherwise 2x2 (= losing support for two rare block types)
 **/
void CKopfLischinskiNFORenderer::Transform_GridToBitmap(bool a_highRes)
{
	unsigned short l_blockPx = (a_highRes ? 8 : 2),
		l_halfBlockPx = l_blockPx / 2;

	m_intermediate = new TwoDimVector<unsigned short>(m_nfo->GetGridHeight() * l_blockPx + 2,
		m_nfo->GetGridWidth() * l_blockPx + 2, 0);
	// values in this "bitmap": 0 = background clr, 10 = block color, 5 = half-transparent block color
	// use an extra row+col for easier access to neighbours / less boundary checks

	const TwoDimVector<CRenderGridBlock>& l_grid = (*m_gridData);
	TwoDimVector<unsigned short>& l_intermediate = (*m_intermediate);

	for(size_t row_t = 1; row_t <= m_gridData->GetRows(); row_t++)
	{
		for(size_t col_t = 1; col_t <= m_gridData->GetCols(); col_t++)
		{
			const CRenderGridBlock *l_block = &l_grid[row_t - 1][col_t - 1];
			switch(l_block->shape)
			{
			case RGS_FULL_BLOCK:
				_TwoDimVectorFill<unsigned short>(l_intermediate, (l_block->alpha == 255 ? 10 : 5), row_t * l_blockPx, col_t * l_blockPx, l_blockPx, l_blockPx);
				break;
			case RGS_BLOCK_LOWER_HALF:
				_TwoDimVectorFill<unsigned short>(l_intermediate, (l_block->alpha == 255 ? 10 : 5), row_t * l_blockPx, col_t * l_blockPx + l_halfBlockPx, l_blockPx, l_halfBlockPx);
				break;
			case RGS_BLOCK_UPPER_HALF:
				_TwoDimVectorFill<unsigned short>(l_intermediate, (l_block->alpha == 255 ? 10 : 5), row_t * l_blockPx, col_t * l_blockPx, l_blockPx, l_halfBlockPx);
				break;
			case RGS_BLOCK_LEFT_HALF:
				_TwoDimVectorFill<unsigned short>(l_intermediate, (l_block->alpha == 255 ? 10 : 5), row_t * l_blockPx, col_t * l_blockPx, l_halfBlockPx, l_blockPx);
				break;
			case RGS_BLOCK_RIGHT_HALF:
				_TwoDimVectorFill<unsigned short>(l_intermediate, (l_block->alpha == 255 ? 10 : 5), row_t * l_blockPx + l_halfBlockPx, col_t * l_blockPx, l_halfBlockPx, l_blockPx);
				break;
			case RGS_BLACK_SQUARE: // 75% centered
				if(a_highRes) _TwoDimVectorFill<unsigned short>(l_intermediate, (l_block->alpha == 255 ? 10 : 5), row_t * l_blockPx + 1, col_t * l_blockPx + 1, 6, 6);
				break;
			case RGS_BLACK_SMALL_SQUARE: // 50% centered
				if(a_highRes) _TwoDimVectorFill<unsigned short>(l_intermediate, (l_block->alpha == 255 ? 10 : 5), row_t * l_blockPx + 2, col_t * l_blockPx + 2, 4, 4);
				break;
			}
		}
	}

	m_pxPerRow = l_blockPx * m_gridData->GetCols();
}


void CKopfLischinskiNFORenderer::Transform_BitmapToGraph()
{
	m_graph = PGraph(new TGraph(m_intermediate->GetRows() * m_intermediate->GetCols()));

	TwoDimVector<unsigned short>& l_intermediate = (*m_intermediate);

	size_t l_rows = m_intermediate->GetRows(), l_cols = m_intermediate->GetCols();
	boost::property_map<TGraph, TVertexColor>::type l_prop = boost::get(TVertexColor(), *m_graph);

	for(size_t row = 1; row < l_rows - 1; row++)
	{
		for(size_t col = 1; col < l_cols - 1; col++)
		{
			size_t l_this = row * col;
			unsigned short l_thisColor = l_intermediate[row][col];

			l_prop[l_this] = l_thisColor;

			if(l_thisColor == 0) continue;

			if(l_thisColor == l_intermediate[row][col + 1]) // to the right
			{
				boost::add_edge(l_this, l_this + 1, *m_graph);
			}
			if(l_thisColor == l_intermediate[row + 1][col + 1]) // right+down
			{
				boost::add_edge(l_this, l_this + l_cols + 1, *m_graph);
			}
			if(l_thisColor == l_intermediate[row + 1][col]) // down
			{
				boost::add_edge(l_this, l_this + l_cols, *m_graph);
			}
			if(l_thisColor == l_intermediate[row + 1][col - 1]) // down+left
			{
				boost::add_edge(l_this, l_this + l_cols - 1, *m_graph);
			}
		}
	}

	delete m_intermediate;
	m_intermediate = NULL;
}


bool CKopfLischinskiNFORenderer::Render()
{
	CNFORenderer::Render();

	cairo_t* cr = cairo_create(m_imgSurface);
	cairo_set_source_rgb(cr, 255, 255, 255);
	cairo_paint(cr);

	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_set_line_width(cr, 1);
	//cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

	/*TwoDimVector<unsigned short>& l_intermediate = (*m_intermediate);

	size_t l_rows = m_intermediate->GetRows(), l_cols = m_intermediate->GetCols();

	for(size_t row = 1; row < l_rows - 1; row++)
	{
		for(size_t col = 1; col < l_cols - 1; col++)
		{
			if(l_intermediate[row][col] == 0) continue;

			cairo_rectangle(cr, col * 2, row * 2, 2, 2);
			cairo_stroke(cr);
		}
	}*/

	boost::property_map<TGraph, boost::vertex_index_t>::type index = boost::get(boost::vertex_index, *m_graph);

    boost::graph_traits<TGraph>::edge_iterator ei, ei_end;
    for(boost::tie(ei, ei_end) = boost::edges(*m_graph); ei != ei_end; ++ei)
	{
		auto src = boost::source(*ei, *m_graph), dst = boost::target(*ei, *m_graph);
		auto a = index[src], b = index[dst];

		/*double x = (a % m_pxPerRow) * GetBlockWidth() + GetBlockWidth() / 2;
		double y = floor((double)a / m_pxPerRow) * GetBlockHeight() + GetBlockHeight() / 2;
		cairo_move_to(cr, x, y);

		cairo_rel_line_to(cr, (b % m_pxPerRow) * GetBlockWidth() + GetBlockWidth() / 2,
			floor((double)b / m_pxPerRow) * GetBlockHeight() + GetBlockHeight() / 2);

		cairo_stroke(cr);*/
		cairo_rectangle(cr, (a % m_pxPerRow) * 5, floor((double)a / m_pxPerRow) * 5, 2, 2);
		cairo_rectangle(cr, (b % m_pxPerRow) * 5, floor((double)b / m_pxPerRow) * 5, 2, 2);
			cairo_stroke(cr);

		if(a > 2000) break;
	}

	cairo_destroy(cr);

	return true;
}