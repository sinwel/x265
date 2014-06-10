/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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

#ifndef X265_MOTIONESTIMATE_H
#define X265_MOTIONESTIMATE_H

#include "primitives.h"
#include "reference.h"
#include "mv.h"
#include "bitcost.h"

namespace x265 {
// private x265 namespace

class MotionEstimate : public BitCost
{
protected:

    /* Aligned copy of original pixels, extra room for manual alignment */
    pixel *fencplane;
    intptr_t fencLumaStride;

    pixelcmp_t sad;
    pixelcmp_t satd;
    pixelcmp_t sa8d;
    pixelcmp_x3_t sad_x3;
    pixelcmp_x4_t sad_x4;

    intptr_t blockOffset;
    int partEnum;
    int searchMethod;
    int subpelRefine;

    /* subpel generation buffers */
    int blockwidth;
    int blockheight;

    MotionEstimate& operator =(const MotionEstimate&);

public:

    static const int COST_MAX = 1 << 28;

    pixel *fenc;

    MotionEstimate();

    virtual ~MotionEstimate();

    void setSearchMethod(int i) { searchMethod = i; }

    void setSubpelRefine(int i) { subpelRefine = i; }

    /* Methods called at slice setup */

    void setSourcePlane(pixel *Y, intptr_t luma)
    {
        fencplane = Y;
        fencLumaStride = luma;
    }

    void setSourcePU(int offset, int pwidth, int pheight);

    /* buf*() and motionEstimate() methods all use cached fenc pixels and thus
     * require setSourcePU() to be called prior. */

    inline int bufSAD(pixel *fref, intptr_t stride)  { return sad(fenc, FENC_STRIDE, fref, stride); }

    inline int bufSA8D(pixel *fref, intptr_t stride) { return sa8d(fenc, FENC_STRIDE, fref, stride); }

    inline int bufSATD(pixel *fref, intptr_t stride) { return satd(fenc, FENC_STRIDE, fref, stride); }

    int motionEstimate(ReferencePlanes *ref, const MV & mvmin, const MV & mvmax, const MV & qmvp, int numCandidates, const MV * mvc, int merange, MV & outQMv);
#if RK_INTER_METEST
	int motionEstimate(ReferencePlanes *ref, const MV & mvmin, const MV & mvmax, MV & outQMv);
	void motionEstimate(ReferencePlanes *ref, MV mvmin, MV mvmax, MV & outQMv, bool);
	int SAD(short *src_1, short *src_2, int width, int height, int stride);
	void DownSample(short *dst, pixel *src_2, int nCtuSize, int nCtuSize_ds);
	int motionEstimate(ReferencePlanes *ref, const MV & mvmin, const MV & mvmax, MV & outQMv, MV *mvNeightBor,
		int nMvNeightBor, bool isHaveMvd, bool isSavePmv, uint32_t offsIdx, uint32_t depth, intptr_t blockOffset_ds, int stride, int, int, int);
	int sad_ud(pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2, char BitWidth, char sampDist, bool isDirectSamp, int lx, int ly);
	int subpelCompare(pixel *pfref, int stride_1, pixel *pfenc, int stride_2, const MV& qmv);
	void InterpHoriz(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int N, int width, int height);
	void InterpVert(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int N, int width, int height);
	void FilterHorizontal(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int width, int height, int16_t const *coeff, int N);
	void FilterVertical(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int N, int width, int height);
#endif
    int subpelCompare(ReferencePlanes * ref, const MV &qmv, pixelcmp_t);
protected:

    inline void StarPatternSearch(ReferencePlanes *ref,
                                  const MV &       mvmin,
                                  const MV &       mvmax,
                                  MV &             bmv,
                                  int &            bcost,
                                  int &            bPointNr,
                                  int &            bDistance,
                                  int              earlyExitIters,
                                  int              merange);
};
}

#endif // ifndef X265_MOTIONESTIMATE_H
