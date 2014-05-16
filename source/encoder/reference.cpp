/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Deepthi Devaki <deepthidevaki@multicorewareinc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibCommon/TComSlice.h"
#include "primitives.h"
#include "reference.h"
#include "common.h"

using namespace x265;

MotionReference::MotionReference()
{
    m_weightBuffer = NULL;
}

int MotionReference::init(TComPicYuv* pic, wpScalingParam *w)
{
    m_reconPic = pic;
    lumaStride = pic->getStride();
    intptr_t startpad = pic->m_lumaMarginY * lumaStride + pic->m_lumaMarginX;

    /* directly reference the pre-extended integer pel plane */
    fpelPlane = pic->m_picBufY + startpad;
    isWeighted = false;

    if (w)
    {
        if (!m_weightBuffer)
        {
            size_t padheight = (pic->m_numCuInHeight * g_maxCUHeight) + pic->m_lumaMarginY * 2;
            m_weightBuffer = (pixel*)X265_MALLOC(pixel, lumaStride * padheight);
            if (!m_weightBuffer)
                return -1;
        }

        isWeighted = true;
        weight = w->inputWeight;
        offset = w->inputOffset * (1 << (X265_DEPTH - 8));
        shift  = w->log2WeightDenom;
        round  = (w->log2WeightDenom >= 1) ? (1 << (w->log2WeightDenom - 1)) : (0);
        m_numWeightedRows = 0;

        /* use our buffer which will have weighted pixels written to it */
        fpelPlane = m_weightBuffer + startpad;
    }

    return 0;
}

void MotionReference::extendEdge(short *pic, int nMarginY, int nMarginX, int stride, int picWidth, int picHeight)
{
	for (int i = 1; i <= nMarginY; i++)
	{
		for (int j = 1; j <= nMarginY; j++)
		{
			int idx = -j - i*stride;
			pic[idx] = pic[0];
		}
	}
	for (int i = 1; i <= nMarginY; i++)
	{
		for (int j = 0; j < picWidth; j++)
		{
			int idx = j - i*stride;
			pic[idx] = pic[j];
		}

	}
	short *pMargin = pic + picWidth - 1;
	for (int i = 1; i <= nMarginY; i++)
	{
		for (int j = 1; j <= nMarginX; j++)
		{
			int idx = j - i*stride;
			pMargin[idx] = pMargin[0];
		}
	}
	for (int i = 0; i < picHeight; i++)
	{
		for (int j = 1; j <= nMarginX; j++)
		{
			int idx = j + i*stride;
			pMargin[idx] = pMargin[i*stride];
		}
	}
	pMargin += (picHeight - 1)*stride;
	for (int i = 1; i <= nMarginY; i++)
	{
		for (int j = 1; j <= nMarginX; j++)
		{
			pMargin[j + i*stride] = pMargin[0];
		}
	}
	for (int i = 1; i <= nMarginY; i++)
	{
		for (int j = 0; j < picWidth; j++)
		{
			int idx = -j + i*stride;
			pMargin[idx] = pMargin[-j];
		}
	}
	pMargin -= (picWidth - 1);
	for (int i = 1; i <= nMarginY; i++)
	{
		for (int j = 1; j <= nMarginX; j++)
		{
			int idx = -j + i*stride;
			pMargin[idx] = pMargin[0];
		}
	}
	for (int i = 0; i < picHeight; i++)
	{
		for (int j = 1; j <= nMarginX; j++)
		{
			int idx = -j - i*stride;
			pMargin[idx] = pMargin[-i*stride];
		}
	}
}

void MotionReference::downSampleAndSum(pixel *src, int width, int height, int src_stride, short *dst, int dst_stride, int sampDist)
{
	for (int i = 0; i < height; i +=sampDist)
	{
		for (int j = 0; j < width; j +=sampDist)
		{
			short sum = 0;
			for (int m = 0; m < sampDist; m ++)
			{
				for (int n = 0; n < sampDist; n ++)
				{
					sum += src[(j + n) + (i + m)*src_stride];
				}
			}
			dst[j/sampDist + i/sampDist*dst_stride] = sum;
		}
	}
}

int MotionReference::init(TComPicYuv* reconPic, TComPicYuv* origPic, wpScalingParam *w)
{
	m_reconPic = reconPic;
	m_origPic = origPic;
	lumaStride = reconPic->getStride();
	intptr_t startpad = reconPic->m_lumaMarginY * lumaStride + reconPic->m_lumaMarginX;

	/* directly reference the pre-extended integer pel plane */
	fpelPlane = reconPic->m_picBufY + startpad;
	fpelPlaneOrig = origPic->m_picBufY + startpad;
	int stride_ds = reconPic->m_lumaMarginY / 2 + origPic->m_picWidth / 4;
	intptr_t startpad_ds = reconPic->m_lumaMarginY / 4 * stride_ds + reconPic->m_lumaMarginX / 4;
	fpelPlaneOrigDS = origPic->m_picBufYDS + startpad_ds;

	//create down sample pixel  add by hdl
	downSampleAndSum(fpelPlaneOrig, origPic->m_picWidth, origPic->m_picHeight, lumaStride, fpelPlaneOrigDS, stride_ds);
	//extend orig down sample pixel
	extendEdge(fpelPlaneOrigDS, reconPic->m_lumaMarginY / 4, reconPic->m_lumaMarginX / 4, stride_ds, origPic->m_picWidth / 4, origPic->m_picHeight / 4);
	
	isWeighted = false;

	if (w)
	{
		if (!m_weightBuffer)
		{
			size_t padheight = (reconPic->m_numCuInHeight * g_maxCUHeight) + reconPic->m_lumaMarginY * 2;
			m_weightBuffer = (pixel*)X265_MALLOC(pixel, lumaStride * padheight);
			if (!m_weightBuffer)
				return -1;
		}

		isWeighted = true;
		weight = w->inputWeight;
		offset = w->inputOffset * (1 << (X265_DEPTH - 8));
		shift = w->log2WeightDenom;
		round = (w->log2WeightDenom >= 1) ? (1 << (w->log2WeightDenom - 1)) : (0);
		m_numWeightedRows = 0;

		/* use our buffer which will have weighted pixels written to it */
		fpelPlane = m_weightBuffer + startpad;
	}

	return 0;
}

MotionReference::~MotionReference()
{
    X265_FREE(m_weightBuffer);
}

void MotionReference::applyWeight(int rows, int numRows)
{
    rows = X265_MIN(rows, numRows);
    if (m_numWeightedRows >= rows)
        return;
    int marginX = m_reconPic->m_lumaMarginX;
    int marginY = m_reconPic->m_lumaMarginY;
    pixel* src = (pixel*)m_reconPic->getLumaAddr() + (m_numWeightedRows * (int)g_maxCUHeight * lumaStride);
    pixel* dst = fpelPlane + ((m_numWeightedRows * (int)g_maxCUHeight) * lumaStride);
    int width = m_reconPic->getWidth();
    int height = ((rows - m_numWeightedRows) * g_maxCUHeight);
    if (rows == numRows)
        height = ((m_reconPic->getHeight() % g_maxCUHeight) ? (m_reconPic->getHeight() % g_maxCUHeight) : g_maxCUHeight);
    size_t dstStride = lumaStride;

    // Computing weighted CU rows
    int shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
    int local_shift = shift + shiftNum;
    int local_round = local_shift ? (1 << (local_shift - 1)) : 0;
    int padwidth = (width + 15) & ~15;  // weightp assembly needs even 16 byte widths
    primitives.weight_pp(src, dst, lumaStride, dstStride, padwidth, height, weight, local_round, local_shift, offset);

    // Extending Left & Right
    primitives.extendRowBorder(dst, dstStride, width, height, marginX);

    // Extending Above
    if (m_numWeightedRows == 0)
    {
        pixel *pixY = fpelPlane - marginX;
        for (int y = 0; y < marginY; y++)
        {
            memcpy(pixY - (y + 1) * dstStride, pixY, dstStride * sizeof(pixel));
        }
    }

    // Extending Bottom
    if (rows == numRows)
    {
        pixel *pixY = fpelPlane - marginX + (m_reconPic->getHeight() - 1) * dstStride;
        for (int y = 0; y < marginY; y++)
        {
            memcpy(pixY + (y + 1) * dstStride, pixY, dstStride * sizeof(pixel));
        }
    }
    m_numWeightedRows = rows;
}
