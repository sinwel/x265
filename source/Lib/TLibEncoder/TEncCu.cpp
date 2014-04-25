/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncCu.cpp
    \brief    Coding Unit (CU) encoder class
*/

#include "TEncCu.h"

#include "primitives.h"
#include "encoder.h"
#include "common.h"

#include "PPA/ppa.h"

#include <cmath>
#include <stdio.h>

#include "../../hardwareC/rk_define.h"
#include "hardwareC/inter.h"
#include "hardwareC/level_mode_calc.h"

#include "macro.h"

#include "RkIntraPred.h"
extern FILE* g_fp_result_x265;


using namespace x265;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncCu::TEncCu()
{
    m_search          = NULL;
    m_trQuant         = NULL;
    m_rdCost          = NULL;
    m_bitCounter      = NULL;
    m_entropyCoder    = NULL;
    m_rdSbacCoders    = NULL;
    m_rdGoOnSbacCoder = NULL;
}

/**
 \param    uiTotalDepth  total number of allowable depth
 \param    uiMaxWidth    largest CU width
 \param    uiMaxHeight   largest CU height
 */
void TEncCu::create(UChar totalDepth, uint32_t maxWidth)
{
    m_totalDepth   = totalDepth + 1;
    m_interCU_2Nx2N  = new TComDataCU*[m_totalDepth - 1];
    m_interCU_2NxN   = new TComDataCU*[m_totalDepth - 1];
    m_interCU_Nx2N   = new TComDataCU*[m_totalDepth - 1];
    m_intraInInterCU = new TComDataCU*[m_totalDepth - 1];
    m_mergeCU        = new TComDataCU*[m_totalDepth - 1];
    m_bestMergeCU    = new TComDataCU*[m_totalDepth - 1];
    m_bestCU      = new TComDataCU*[m_totalDepth - 1];
    m_tempCU      = new TComDataCU*[m_totalDepth - 1];

    m_bestPredYuv = new TComYuv*[m_totalDepth - 1];
    m_bestResiYuv = new TShortYUV*[m_totalDepth - 1];
    m_bestRecoYuv = new TComYuv*[m_totalDepth - 1];

    m_tmpPredYuv = new TComYuv*[m_totalDepth - 1];

    m_modePredYuv[0] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[1] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[2] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[3] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[4] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[5] = new TComYuv*[m_totalDepth - 1];

    m_tmpResiYuv = new TShortYUV*[m_totalDepth - 1];
    m_tmpRecoYuv = new TComYuv*[m_totalDepth - 1];

    m_bestMergeRecoYuv = new TComYuv*[m_totalDepth - 1];

    m_origYuv = new TComYuv*[m_totalDepth - 1];

    int csp = m_cfg->param.internalCsp;

    for (int i = 0; i < m_totalDepth - 1; i++)
    {
        uint32_t numPartitions = 1 << ((m_totalDepth - i - 1) << 1);
        uint32_t width  = maxWidth  >> i;
        uint32_t height = maxWidth >> i;

        m_bestCU[i] = new TComDataCU;
        m_bestCU[i]->create(numPartitions, width, height, maxWidth >> (m_totalDepth - 1), csp);

        m_tempCU[i] = new TComDataCU;
        m_tempCU[i]->create(numPartitions, width, height, maxWidth >> (m_totalDepth - 1), csp);

        m_interCU_2Nx2N[i] = new TComDataCU;
        m_interCU_2Nx2N[i]->create(numPartitions, width, height, maxWidth >> (m_totalDepth - 1), csp);

        m_interCU_2NxN[i] = new TComDataCU;
        m_interCU_2NxN[i]->create(numPartitions, width, height, maxWidth >> (m_totalDepth - 1), csp);

        m_interCU_Nx2N[i] = new TComDataCU;
        m_interCU_Nx2N[i]->create(numPartitions, width, height, maxWidth >> (m_totalDepth - 1), csp);

        m_intraInInterCU[i] = new TComDataCU;
        m_intraInInterCU[i]->create(numPartitions, width, height, maxWidth >> (m_totalDepth - 1), csp);

        m_mergeCU[i] = new TComDataCU;
        m_mergeCU[i]->create(numPartitions, width, height, maxWidth >> (m_totalDepth - 1), csp);

        m_bestMergeCU[i] = new TComDataCU;
        m_bestMergeCU[i]->create(numPartitions, width, height, maxWidth >> (m_totalDepth - 1), csp);

        m_bestPredYuv[i] = new TComYuv;
        m_bestPredYuv[i]->create(width, height, csp);

        m_bestResiYuv[i] = new TShortYUV;
        m_bestResiYuv[i]->create(width, height, csp);

        m_bestRecoYuv[i] = new TComYuv;
        m_bestRecoYuv[i]->create(width, height, csp);

        m_tmpPredYuv[i] = new TComYuv;
        m_tmpPredYuv[i]->create(width, height, csp);

        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            m_modePredYuv[j][i] = new TComYuv;
            m_modePredYuv[j][i]->create(width, height, csp);
        }

        m_tmpResiYuv[i] = new TShortYUV;
        m_tmpResiYuv[i]->create(width, height, csp);

        m_tmpRecoYuv[i] = new TComYuv;
        m_tmpRecoYuv[i]->create(width, height, csp);

        m_bestMergeRecoYuv[i] = new TComYuv;
        m_bestMergeRecoYuv[i]->create(width, height, csp);

        m_origYuv[i] = new TComYuv;
        m_origYuv[i]->create(width, height, csp);
    }

    m_bEncodeDQP = false;
    m_LCUPredictionSAD = 0;
    m_addSADDepth      = 0;
    m_temporalSAD      = 0;
}

void TEncCu::destroy()
{
    for (int i = 0; i < m_totalDepth - 1; i++)
    {
        if (m_interCU_2Nx2N[i])
        {
            m_interCU_2Nx2N[i]->destroy();
            delete m_interCU_2Nx2N[i];
            m_interCU_2Nx2N[i] = NULL;
        }
        if (m_interCU_2NxN[i])
        {
            m_interCU_2NxN[i]->destroy();
            delete m_interCU_2NxN[i];
            m_interCU_2NxN[i] = NULL;
        }
        if (m_interCU_Nx2N[i])
        {
            m_interCU_Nx2N[i]->destroy();
            delete m_interCU_Nx2N[i];
            m_interCU_Nx2N[i] = NULL;
        }
        if (m_intraInInterCU[i])
        {
            m_intraInInterCU[i]->destroy();
            delete m_intraInInterCU[i];
            m_intraInInterCU[i] = NULL;
        }
        if (m_mergeCU[i])
        {
            m_mergeCU[i]->destroy();
            delete m_mergeCU[i];
            m_mergeCU[i] = NULL;
        }
        if (m_bestMergeCU[i])
        {
            m_bestMergeCU[i]->destroy();
            delete m_bestMergeCU[i];
            m_bestMergeCU[i] = NULL;
        }
        if (m_bestCU[i])
        {
            m_bestCU[i]->destroy();
            delete m_bestCU[i];
            m_bestCU[i] = NULL;
        }
        if (m_tempCU[i])
        {
            m_tempCU[i]->destroy();
            delete m_tempCU[i];
            m_tempCU[i] = NULL;
        }

        if (m_bestPredYuv[i])
        {
            m_bestPredYuv[i]->destroy();
            delete m_bestPredYuv[i];
            m_bestPredYuv[i] = NULL;
        }
        if (m_bestResiYuv[i])
        {
            m_bestResiYuv[i]->destroy();
            delete m_bestResiYuv[i];
            m_bestResiYuv[i] = NULL;
        }
        if (m_bestRecoYuv[i])
        {
            m_bestRecoYuv[i]->destroy();
            delete m_bestRecoYuv[i];
            m_bestRecoYuv[i] = NULL;
        }

        if (m_tmpPredYuv[i])
        {
            m_tmpPredYuv[i]->destroy();
            delete m_tmpPredYuv[i];
            m_tmpPredYuv[i] = NULL;
        }
        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            if (m_modePredYuv[j][i])
            {
                m_modePredYuv[j][i]->destroy();
                delete m_modePredYuv[j][i];
                m_modePredYuv[j][i] = NULL;
            }
        }

        if (m_tmpResiYuv[i])
        {
            m_tmpResiYuv[i]->destroy();
            delete m_tmpResiYuv[i];
            m_tmpResiYuv[i] = NULL;
        }
        if (m_tmpRecoYuv[i])
        {
            m_tmpRecoYuv[i]->destroy();
            delete m_tmpRecoYuv[i];
            m_tmpRecoYuv[i] = NULL;
        }
        if (m_bestMergeRecoYuv[i])
        {
            m_bestMergeRecoYuv[i]->destroy();
            delete m_bestMergeRecoYuv[i];
            m_bestMergeRecoYuv[i] = NULL;
        }

        if (m_origYuv[i])
        {
            m_origYuv[i]->destroy();
            delete m_origYuv[i];
            m_origYuv[i] = NULL;
        }
    }

    delete [] m_interCU_2Nx2N;
    m_interCU_2Nx2N = NULL;
    delete [] m_interCU_2NxN;
    m_interCU_2NxN = NULL;
    delete [] m_interCU_Nx2N;
    m_interCU_Nx2N = NULL;
    delete [] m_intraInInterCU;
    m_intraInInterCU = NULL;
    delete [] m_mergeCU;
    m_mergeCU = NULL;
    delete [] m_bestMergeCU;
    m_bestMergeCU = NULL;
    delete [] m_bestCU;
    m_bestCU = NULL;
    delete [] m_tempCU;
    m_tempCU = NULL;

    delete [] m_bestPredYuv;
    m_bestPredYuv = NULL;
    delete [] m_bestResiYuv;
    m_bestResiYuv = NULL;
    delete [] m_bestRecoYuv;
    m_bestRecoYuv = NULL;

    delete [] m_bestMergeRecoYuv;
    m_bestMergeRecoYuv = NULL;
    delete [] m_tmpPredYuv;
    m_tmpPredYuv = NULL;

    for (int i = 0; i < MAX_PRED_TYPES; i++)
    {
        delete [] m_modePredYuv[i];
        m_modePredYuv[i] = NULL;
    }

    delete [] m_tmpResiYuv;
    m_tmpResiYuv = NULL;
    delete [] m_tmpRecoYuv;
    m_tmpRecoYuv = NULL;
    delete [] m_origYuv;
    m_origYuv = NULL;
}

/** \param    pcEncTop      pointer of encoder class
 */
void TEncCu::init(Encoder* top)
{
    m_cfg = top;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param  rpcCU pointer of CU data class
 */
bool mergeFlag = 0;

void TEncCu::compressCU(TComDataCU* cu)
{
    // initialize CU data
    m_bestCU[0]->initCU(cu->getPic(), cu->getAddr());
    m_tempCU[0]->initCU(cu->getPic(), cu->getAddr());

    m_addSADDepth      = 0;
    m_LCUPredictionSAD = 0;
    m_temporalSAD      = 0;

    this->pHardWare = &G_hardwareC;


    #if RK_CTU_CALC_PROC_ENABLE
    pHardWare->ctu_x = pHardWare->ctu_calc.ctu_x = cu->m_cuPelX/cu->m_slice->m_sps->m_maxCUWidth;
    pHardWare->ctu_y = pHardWare->ctu_calc.ctu_y = cu->m_cuPelY/cu->m_slice->m_sps->m_maxCUWidth;
    pHardWare->pic_w = pHardWare->ctu_calc.pic_w = cu->m_slice->m_sps->m_picWidthInLumaSamples;
    pHardWare->pic_h = pHardWare->ctu_calc.pic_h = cu->m_slice->m_sps->m_picHeightInLumaSamples;
    pHardWare->ctu_w = pHardWare->ctu_calc.ctu_w = cu->m_slice->m_sps->m_maxCUWidth;
    //this->hardware.ctu_w_log2 = cu->m_slice->m_sps->m_maxCUWidth;

    if(pHardWare->ctu_y == 16)
       pHardWare->ctu_y = pHardWare->ctu_y;

    pHardWare->init();
    pHardWare->ctu_calc.init();
    pHardWare->ctu_calc.slice_type = m_bestCU[0]->getSlice()->getSliceType();
    pHardWare->hw_cfg.constrained_intra_enable = m_bestCU[0]->getSlice()->getPPS()->getConstrainedIntraPred();
    pHardWare->hw_cfg.TMVP_enable = m_bestCU[0]->getSlice()->getEnableTMVPFlag();
    pHardWare->hw_cfg.strong_intra_smoothing_enable = m_bestCU[0]->getSlice()->getSPS()->getUseStrongIntraSmoothing();
    pHardWare->ctu_calc.QP_lu = cu->m_qp[0];
    pHardWare->ctu_calc.QP_cb = cu->m_qp[0] + m_bestCU[0]->getSlice()->getPPS()->getChromaCbQpOffset();
    pHardWare->ctu_calc.QP_cr = cu->m_qp[0] + m_bestCU[0]->getSlice()->getPPS()->getChromaCrQpOffset();
    #endif

    // analysis of CU
#if LOG_CU_STATISTICS
    int numPartition = cu->getTotalNumPart();
#endif

    if (m_bestCU[0]->getSlice()->getSliceType() == I_SLICE)
    {
        xCompressIntraCU(m_bestCU[0], m_tempCU[0], 0);
#if LOG_CU_STATISTICS
        int i = 0, part;
        do
        {
            m_log->totalCu++;
            part = cu->getDepth(i);
            int next = numPartition >> (part * 2);
            if (part == g_maxCUDepth - 1 && cu->getPartitionSize(i) != SIZE_2Nx2N)
            {
                m_log->cntIntraNxN++;
            }
            else
            {
                m_log->cntIntra[part]++;
                if (cu->getLumaIntraDir()[i] > 1)
                    m_log->cuIntraDistribution[part][ANGULAR_MODE_ID]++;
                else
                    m_log->cuIntraDistribution[part][cu->getLumaIntraDir()[i]]++;
            }
            i += next;
        }
        while (i < numPartition);
#endif
    }
    else
    {
        if (m_cfg->param.rdLevel < 5)
        {
            TComDataCU* outBestCU = NULL;

            /* At the start of analysis, the best CU is a null pointer
            On return, it points to the CU encode with best chosen mode*/
            xCompressInterCU(outBestCU, m_tempCU[0], cu, 0, 0, 4);
        }
        else
		{
#if RK_INTER_CALC_RDO_TWICE
#if INTER_IME_TEST
			nCtuSize = cu->getSlice()->getSPS()->getMaxCUWidth();
			for (int i = 0; i < 85; i++)
			{
				interinfo.imeinput.isValidCu[i] = false; //先设为0
				interinfo.fmeinput.isValidCu[i] = false;
			}
			xCompressCU(m_bestCU[0], m_tempCU[0], 0, true);
			delete[] pSearchRange; //在motionEstimate函数里开辟内存空间
			delete[] pCurrCtu; //在motionEstimate函数里开辟内存空间
#else
			xCompressCU(m_bestCU[0], m_tempCU[0], 0, true);
#endif //end INTER_IME_TEST
#else
			xCompressCU(m_bestCU[0], m_tempCU[0], 0);
#endif //end RK_INTER_CALC_RDO_TWICE
		}
#if LOG_CU_STATISTICS
        int i = 0, part;
        do
        {
            part = cu->getDepth(i);
            m_log->cntTotalCu[part]++;
            int next = numPartition >> (part * 2);
            if (cu->isSkipped(i))
            {
                m_log->cntSkipCu[part]++;
            }
            else
            {
                m_log->totalCu++;
                if (cu->getPredictionMode(0) == MODE_INTER)
                {
                    m_log->cntInter[part]++;
                    if (cu->getPartitionSize(0) < AMP_ID)
                        m_log->cuInterDistribution[part][cu->getPartitionSize(0)]++;
                    else
                        m_log->cuInterDistribution[part][AMP_ID]++;
                }
                else if (cu->getPredictionMode(0) == MODE_INTRA)
                {
                    if (part == g_maxCUDepth - 1 && cu->getPartitionSize(0) == SIZE_NxN)
                    {
                        m_log->cntIntraNxN++;
                    }
                    else
                    {
                        m_log->cntIntra[part]++;
                        if (cu->getLumaIntraDir()[0] > 1)
                            m_log->cuIntraDistribution[part][ANGULAR_MODE_ID]++;
                        else
                            m_log->cuIntraDistribution[part][cu->getLumaIntraDir()[0]]++;
                    }
                }
            }
            i = i + next;
        }
        while (i < numPartition);
#endif
    }

    #if RK_CTU_CALC_PROC_ENABLE
    this->pHardWare->ctu_calc.proc();
    #endif
}

/** \param  cu  pointer of CU data class
 */
void TEncCu::encodeCU(TComDataCU* cu)
{
    if (cu->getSlice()->getPPS()->getUseDQP())
    {
        setdQPFlag(true);
    }

    // Encode CU data
    xEncodeCU(cu, 0, 0);
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/** Derive small set of test modes for AMP encoder speed-up
 *\param   outBestCU
 *\param   parentSize
 *\param   bTestAMP_Hor
 *\param   bTestAMP_Ver
 *\param   bTestMergeAMP_Hor
 *\param   bTestMergeAMP_Ver
 *\returns void
*/
void TEncCu::deriveTestModeAMP(TComDataCU* outBestCU, PartSize parentSize, bool &bTestAMP_Hor, bool &bTestAMP_Ver,
                               bool &bTestMergeAMP_Hor, bool &bTestMergeAMP_Ver)
{
    if (outBestCU->getPartitionSize(0) == SIZE_2NxN)
    {
        bTestAMP_Hor = true;
    }
    else if (outBestCU->getPartitionSize(0) == SIZE_Nx2N)
    {
        bTestAMP_Ver = true;
    }
    else if (outBestCU->getPartitionSize(0) == SIZE_2Nx2N && outBestCU->getMergeFlag(0) == false &&
             outBestCU->isSkipped(0) == false)
    {
        bTestAMP_Hor = true;
        bTestAMP_Ver = true;
    }

    //! Utilizing the partition size of parent PU
    if (parentSize >= SIZE_2NxnU && parentSize <= SIZE_nRx2N)
    {
        bTestMergeAMP_Hor = true;
        bTestMergeAMP_Ver = true;
    }

    if (parentSize == SIZE_NONE) //! if parent is intra
    {
        if (outBestCU->getPartitionSize(0) == SIZE_2NxN)
        {
            bTestMergeAMP_Hor = true;
        }
        else if (outBestCU->getPartitionSize(0) == SIZE_Nx2N)
        {
            bTestMergeAMP_Ver = true;
        }
    }

    if (outBestCU->getPartitionSize(0) == SIZE_2Nx2N && outBestCU->isSkipped(0) == false)
    {
        bTestMergeAMP_Hor = true;
        bTestMergeAMP_Ver = true;
    }

    if (outBestCU->getWidth(0) == 64)
    {
        bTestAMP_Hor = false;
        bTestAMP_Ver = false;
    }
}


#include "ctu_calc.h"
void copy_cu_data(cuData *p, TComDataCU *CU)
{
    uint32_t m;
    uint32_t cu_w = CU->m_width[0];
    p->totalDistortion = CU->m_totalDistortion;
    p->totalBits       = CU->m_totalBits;
    p->totalCost       = CU->m_totalCost;
    p->cu_x = 0;
    p->cu_y = 0;
    p->width = CU->m_width[0];
    //for(m=0; m<4; m++)
    //{
    //    p->Cbf[0][m] = CU->m_cbf[0][m];
    //    p->Cbf[1][m] = CU->m_cbf[1][m];
    //    p->Cbf[2][m] = CU->m_cbf[2][m];
    //}

    if(CU->m_partSizes[0] == SIZE_NxN)
        p->tu_split_flag = 1;
    else
        p->tu_split_flag = 0;

    p->cuPredMode[0]    = CU->m_predModes[0];
    p->cuSkipFlag[0]    = CU->m_skipFlag[0];
    p->MergeFlag        = CU->m_bMergeFlags[0];
    p->MergeIndex       = CU->m_mergeIndex[0];
    p->cuPartSize[0]      = CU->m_partSizes[0];

    for(m=0; m<cu_w*cu_w; m++) {
        p->CoeffY[m] = CU->m_trCoeffY[m];
    }
    for(m=0; m<cu_w*cu_w/4; m++) {
        p->CoeffU[m] = CU->m_trCoeffCb[m];
        p->CoeffV[m] = CU->m_trCoeffCr[m];
    }
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/** Compress a CU block recursively with enabling sub-LCU-level delta QP
 *\param   outBestCU
 *\param   outTempCU
 *\param   depth
 *\returns void
 *
 *- for loop of QP value to compress the current CU with all possible QP
*/

#if RK_OUTPUT_FILE
FILE *fp;
int ctu_num = 0;
#endif

void TEncCu::xCompressIntraCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth)
{
    //PPAScopeEvent(TEncCu_xCompressIntraCU + depth);

    #if RK_OUTPUT_FILE
	if(!fp)
      fp = fopen("cu_spilt_data", "wb+");
    #endif

    m_abortFlag = false;
    TComPic* pic = outBestCU->getPic();

    // get Original YUV data from picture
    // 将当前CU层的YUV数据拷贝至oriYuv结构体中
    m_origYuv[depth]->copyFromPicYuv(pic->getPicYuvOrg(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU());
    #if RK_CTU_CALC_PROC_ENABLE
    if(depth == 0) {
        memcpy(pHardWare->ctu_calc.input_curr_y, m_origYuv[depth]->m_bufY, pHardWare->ctu_calc.ctu_w*pHardWare->ctu_calc.ctu_w);
        memcpy(pHardWare->ctu_calc.input_curr_u, m_origYuv[depth]->m_bufU, pHardWare->ctu_calc.ctu_w*pHardWare->ctu_calc.ctu_w/4);
        memcpy(pHardWare->ctu_calc.input_curr_v, m_origYuv[depth]->m_bufV, pHardWare->ctu_calc.ctu_w*pHardWare->ctu_calc.ctu_w/4);
    }//add by zsq
    #endif

    // variables for fast encoder decision
    bool bTrySplit = true;

    // variable for Early CU determination
    bool bSubBranch = true;

    // variable for Cbf fast mode PU decision
    bool bTrySplitDQP = true;
    bool bBoundary = false;

    uint32_t lpelx = outBestCU->getCUPelX();
    uint32_t rpelx = lpelx + outBestCU->getWidth(0)  - 1;
    uint32_t tpelx = outBestCU->getCUPelY();
    uint32_t bpely = tpelx + outBestCU->getHeight(0) - 1;
    int qp = outTempCU->getQP(0);

    // If slice start or slice end is within this cu...
    TComSlice * slice = outTempCU->getPic()->getSlice();
    bool bSliceEnd = (slice->getSliceCurEndCUAddr() > outTempCU->getSCUAddr() && slice->getSliceCurEndCUAddr() < outTempCU->getSCUAddr() + outTempCU->getTotalNumPart());
    bool bInsidePicture = (rpelx < outBestCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (bpely < outBestCU->getSlice()->getSPS()->getPicHeightInLumaSamples());

    //Data for splitting
    UChar nextDepth = depth + 1;
    uint32_t partUnitIdx = 0;
    TComDataCU* subBestPartCU[4], *subTempPartCU[4];

    //We need to split; so dont try these modes
    if (!bSliceEnd && bInsidePicture)
    {
        // variables for fast encoder decision
        bTrySplit = true;

        outTempCU->initEstData(depth, qp);

        bTrySplitDQP = bTrySplit;

        if (depth <= m_addSADDepth)
        {
            m_LCUPredictionSAD += m_temporalSAD;
            m_addSADDepth = depth;
        }

        xCheckRDCostIntra(outBestCU, outTempCU, SIZE_2Nx2N);
        outTempCU->initEstData(depth, qp);

        if (depth == g_maxCUDepth - g_addCUDepth)
        {
            if (outTempCU->getWidth(0) > (1 << outTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize()))
            {
                xCheckRDCostIntra(outBestCU, outTempCU, SIZE_NxN);
            }
        }

        m_entropyCoder->resetBits();
        m_entropyCoder->encodeSplitFlag(outBestCU, 0, depth, true);
        outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        outBestCU->m_totalCost  = m_rdCost->calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);

        //curr level CU Cost、Bits、Distortion
        #if RK_CTU_CALC_PROC_ENABLE
        uint8_t pos;
        uint32_t m;
        uint8_t cu_w = outBestCU->m_width[0];
        switch(depth)
        {
            case 0:
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[0][0], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[0][0]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[0][0]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[0][0]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                break;
            case 1:
                pos = pHardWare->ctu_calc.best_pos[1];
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[1][pos], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[1][pos]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[1][pos]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[1][pos]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                break;
            case 2:
                pos = pHardWare->ctu_calc.best_pos[2];
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[2][pos], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[2][pos]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[2][pos]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[2][pos]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                break;
            case 3:
                pos = pHardWare->ctu_calc.best_pos[3];
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[3][pos], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[3][pos]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[3][pos]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[3][pos]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                break;
            default:
                break;
        }

        pHardWare->ctu_calc.best_pos[depth]++;
        #endif

        // Early CU determination
        if (outBestCU->isSkipped(0))
            bSubBranch = false;
        else
            bSubBranch = true;
    }
    else if (!(bSliceEnd && bInsidePicture))
    {
        bBoundary = true;
        m_addSADDepth++;
    }

    outTempCU->initEstData(depth, qp);

    // further split
    if (bSubBranch && bTrySplitDQP && depth < g_maxCUDepth - g_addCUDepth)
    {
        for (; partUnitIdx < 4; partUnitIdx++)
        {
            subBestPartCU[partUnitIdx]     = m_bestCU[nextDepth];
            subTempPartCU[partUnitIdx]     = m_tempCU[nextDepth];

            subBestPartCU[partUnitIdx]->initSubCU(outTempCU, partUnitIdx, nextDepth, qp);     // clear sub partition datas or init.
            subTempPartCU[partUnitIdx]->initSubCU(outTempCU, partUnitIdx, nextDepth, qp);     // clear sub partition datas or init.

            bool bInSlice = subBestPartCU[partUnitIdx]->getSCUAddr() < slice->getSliceCurEndCUAddr();
            if (bInSlice && (subBestPartCU[partUnitIdx]->getCUPelX() < slice->getSPS()->getPicWidthInLumaSamples()) && (subBestPartCU[partUnitIdx]->getCUPelY() < slice->getSPS()->getPicHeightInLumaSamples()))
            {
                if (0 == partUnitIdx) //initialize RD with previous depth buffer
                {
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
                }
                else
                {
                    //至底向上，需要更新采用最佳的CU划分的CABAC状态表用来更新当前CU开始时状态
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[nextDepth][CI_NEXT_BEST]);
                }

                xCompressIntraCU(subBestPartCU[partUnitIdx], subTempPartCU[partUnitIdx], nextDepth);
                outTempCU->copyPartFrom(subBestPartCU[partUnitIdx], partUnitIdx, nextDepth); // Keep best part data to current temporary data.
                xCopyYuv2Tmp(subBestPartCU[partUnitIdx]->getTotalNumPart() * partUnitIdx, nextDepth);
            }
            else if (bInSlice)
            {
                subBestPartCU[partUnitIdx]->copyToPic(nextDepth);
                outTempCU->copyPartFrom(subBestPartCU[partUnitIdx], partUnitIdx, nextDepth);
            }
        }

        if (!bBoundary)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeSplitFlag(outTempCU, 0, depth, true);

            outTempCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); //split bits
        }
        outTempCU->m_totalCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

        //curr sublevel CU Cost、Bits、Distortion
        #if RK_CTU_CALC_PROC_ENABLE
        uint8_t pos;
        switch(depth)
        {
            case 0:
                pHardWare->ctu_calc.intra_temp_64.m_totalDistortion = outTempCU->m_totalDistortion;
                pHardWare->ctu_calc.intra_temp_64.m_totalBits       = outTempCU->m_totalBits;
                pHardWare->ctu_calc.intra_temp_64.m_totalCost       = outTempCU->m_totalCost;
                break;
            case 1:
                pos = pHardWare->ctu_calc.temp_pos[1];
                pHardWare->ctu_calc.intra_temp_32[pos].m_totalDistortion = outTempCU->m_totalDistortion;
                pHardWare->ctu_calc.intra_temp_32[pos].m_totalBits       = outTempCU->m_totalBits;
                pHardWare->ctu_calc.intra_temp_32[pos].m_totalCost       = outTempCU->m_totalCost;
                pHardWare->ctu_calc.temp_pos[1]++;
                break;
            case 2:
                pos = pHardWare->ctu_calc.temp_pos[2];
                pHardWare->ctu_calc.intra_temp_16[pos].m_totalDistortion = outTempCU->m_totalDistortion;
                pHardWare->ctu_calc.intra_temp_16[pos].m_totalBits       = outTempCU->m_totalBits;
                pHardWare->ctu_calc.intra_temp_16[pos].m_totalCost       = outTempCU->m_totalCost;
                pHardWare->ctu_calc.temp_pos[2]++;
                break;
            case 3:
                break;
            default:
                break;
        }
        #endif

        if ((g_maxCUWidth >> depth) == outTempCU->getSlice()->getPPS()->getMinCuDQPSize() && outTempCU->getSlice()->getPPS()->getUseDQP())
        {
            bool hasResidual = false;
            for (uint32_t uiBlkIdx = 0; uiBlkIdx < outTempCU->getTotalNumPart(); uiBlkIdx++)
            {
                if (outTempCU->getCbf(uiBlkIdx, TEXT_LUMA) || outTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_U) |
                    outTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_V))
                {
                    hasResidual = true;
                    break;
                }
            }

            uint32_t targetPartIdx = 0;
            if (hasResidual)
            {
                bool foundNonZeroCbf = false;
                outTempCU->setQPSubCUs(outTempCU->getRefQP(targetPartIdx), outTempCU, 0, depth, foundNonZeroCbf);
                assert(foundNonZeroCbf);
            }
            else
            {
                outTempCU->setQPSubParts(outTempCU->getRefQP(targetPartIdx), 0, depth); // set QP to default QP
            }
        }

        #if RK_CTU_CALC_PROC_ENABLE
        switch(depth)
        {
            case 0:
                if (outTempCU->m_totalCost < outBestCU->m_totalCost)
                    pHardWare->ctu_calc.ori_cu_split_flag[0] = 1;
                break;
            case 1:
                if (outTempCU->m_totalCost < outBestCU->m_totalCost)
                    pHardWare->ctu_calc.ori_cu_split_flag[pHardWare->ctu_calc.best_pos[1] - 1 + 1] = 1;
                break;
            case 2:
                if (outTempCU->m_totalCost < outBestCU->m_totalCost)
                    pHardWare->ctu_calc.ori_cu_split_flag[pHardWare->ctu_calc.best_pos[2] - 1 + 5] = 1;
                break;
            case 3:
                break;
            default:
                break;
        }
        #endif

        #if RK_OUTPUT_FILE
        fprintf(fp, "depth = %d, bestCU_cost = %.8x, tempCU_cost = %.8x\n", depth, outBestCU->m_totalCost, outTempCU->m_totalCost);
        if(depth == 0){
            fprintf(fp, "ctu_num = %d\n", ctu_num++);
            fprintf(fp, "\n");
        }
        #endif

        m_rdSbacCoders[nextDepth][CI_NEXT_BEST]->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);
        xCheckBestMode(outBestCU, outTempCU, depth); // RD compare current prediction with split prediction.
    }

    outBestCU->copyToPic(depth); // Copy Best data to Picture for next partition prediction.

    // Copy Yuv data to picture Yuv
    xCopyYuv2Pic(outBestCU->getPic(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth, depth, outBestCU, lpelx, tpelx);

    #if RK_CTU_CALC_PROC_ENABLE
    if(depth == 0)
    {
        uint32_t m;
        memcpy(pHardWare->ctu_calc.ori_recon_y, m_bestRecoYuv[0]->m_bufY, 64*64);
        memcpy(pHardWare->ctu_calc.ori_recon_u, m_bestRecoYuv[0]->m_bufU, 32*32);
        memcpy(pHardWare->ctu_calc.ori_recon_v, m_bestRecoYuv[0]->m_bufV, 32*32);

        for(m=0; m<64*64; m++)
            pHardWare->ctu_calc.ori_resi_y[m] = outBestCU->m_trCoeffY[m];

        for(m=0; m<32*32; m++) {
            pHardWare->ctu_calc.ori_resi_u[m] = outBestCU->m_trCoeffCb[m];
            pHardWare->ctu_calc.ori_resi_v[m] = outBestCU->m_trCoeffCr[m];
        }
    }
    #endif

    if (bBoundary || (bSliceEnd && bInsidePicture)) return;

    // Assert if Best prediction mode is NONE
    // Selected mode's RD-cost must be not MAX_DOUBLE.
    assert(outBestCU->getPartitionSize(0) != SIZE_NONE);
    assert(outBestCU->getPredictionMode(0) != MODE_NONE);
    assert(outBestCU->m_totalCost != MAX_DOUBLE);
}

void TEncCu::xCompressCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth, PartSize parentSize)
{
    //PPAScopeEvent(TEncCu_xCompressCU + depth);

    TComPic* pic = outBestCU->getPic();
    m_abortFlag = false;

    // get Original YUV data from picture
    m_origYuv[depth]->copyFromPicYuv(pic->getPicYuvOrg(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU());
    #if RK_CTU_CALC_PROC_ENABLE
    if(depth == 0) {
        memcpy(pHardWare->ctu_calc.input_curr_y, m_origYuv[depth]->m_bufY, pHardWare->ctu_calc.ctu_w*pHardWare->ctu_calc.ctu_w);
        memcpy(pHardWare->ctu_calc.input_curr_u, m_origYuv[depth]->m_bufU, pHardWare->ctu_calc.ctu_w*pHardWare->ctu_calc.ctu_w/4);
        memcpy(pHardWare->ctu_calc.input_curr_v, m_origYuv[depth]->m_bufV, pHardWare->ctu_calc.ctu_w*pHardWare->ctu_calc.ctu_w/4);

    }//add by zsq
    pHardWare->ctu_calc.slice_type = outBestCU->getSlice()->getSliceType();
    #endif

    // variables for fast encoder decision
    bool bTrySplit = true;

    // variable for Early CU determination
    bool bSubBranch = true;

    // variable for Cbf fast mode PU decision
    bool doNotBlockPu = true;
    bool earlyDetectionSkipMode = false;

    bool bTrySplitDQP = true;

    bool bBoundary = false;
    uint32_t lpelx = outBestCU->getCUPelX();
    uint32_t rpelx = lpelx + outBestCU->getWidth(0)  - 1;
    uint32_t tpely = outBestCU->getCUPelY();
    uint32_t bpely = tpely + outBestCU->getHeight(0) - 1;

    int qp = outTempCU->getQP(0);

    // If slice start or slice end is within this cu...
    TComSlice* slice = outTempCU->getPic()->getSlice();
    bool bSliceEnd = (slice->getSliceCurEndCUAddr() > outTempCU->getSCUAddr() &&
                      slice->getSliceCurEndCUAddr() < outTempCU->getSCUAddr() + outTempCU->getTotalNumPart());
    bool bInsidePicture = (rpelx < outBestCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) &&
        (bpely < outBestCU->getSlice()->getSPS()->getPicHeightInLumaSamples());

    // We need to split, so don't try these modes.
    if (!bSliceEnd && bInsidePicture)
    {
        // variables for fast encoder decision
        bTrySplit    = true;

        outTempCU->initEstData(depth, qp);

        // do inter modes, SKIP and 2Nx2N
        if (outBestCU->getSlice()->getSliceType() != I_SLICE)
        {
            // 2Nx2N
            if (m_cfg->param.bEnableEarlySkip)
            {
                xCheckRDCostInter(outBestCU, outTempCU, SIZE_2Nx2N);
                outTempCU->initEstData(depth, qp); // by competition for inter_2Nx2N
            }
			mergeFlag = 1;
            // by Merge for inter_2Nx2N
            xCheckRDCostMerge2Nx2N(outBestCU, outTempCU, &earlyDetectionSkipMode, m_bestPredYuv[depth], m_bestRecoYuv[depth]);
			mergeFlag = 0;

            outTempCU->initEstData(depth, qp);

            if (!m_cfg->param.bEnableEarlySkip)
            {
                // 2Nx2N, NxN
                xCheckRDCostInter(outBestCU, outTempCU, SIZE_2Nx2N);
                outTempCU->initEstData(depth, qp);
                if (m_cfg->param.bEnableCbfFastMode)
                {
                    doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                }
            }
        }

        bTrySplitDQP = bTrySplit;

        if (depth <= m_addSADDepth)
        {
            m_LCUPredictionSAD += m_temporalSAD;
            m_addSADDepth = depth;
        }

        if (!earlyDetectionSkipMode)
        {
            outTempCU->initEstData(depth, qp);

            // do inter modes, NxN, 2NxN, and Nx2N
            if (outBestCU->getSlice()->getSliceType() != I_SLICE)
            {
                // 2Nx2N, NxN
                if (!((outBestCU->getWidth(0) == 8) && (outBestCU->getHeight(0) == 8)))
                {
                    if (depth == g_maxCUDepth - g_addCUDepth && doNotBlockPu)
                    {
                        xCheckRDCostInter(outBestCU, outTempCU, SIZE_NxN);
                        outTempCU->initEstData(depth, qp);
                    }
                }

                if (m_cfg->param.bEnableRectInter)
                {
                    // 2NxN, Nx2N
                    if (doNotBlockPu)
                    {
                        xCheckRDCostInter(outBestCU, outTempCU, SIZE_Nx2N);
                        outTempCU->initEstData(depth, qp);
                        if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_Nx2N)
                        {
                            doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                    }
                    if (doNotBlockPu)
                    {
                        xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxN);
                        outTempCU->initEstData(depth, qp);
                        if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxN)
                        {
                            doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                    }
                }

                // Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
                if (pic->getSlice()->getSPS()->getAMPAcc(depth))
                {
                    bool bTestAMP_Hor = false, bTestAMP_Ver = false;
                    bool bTestMergeAMP_Hor = false, bTestMergeAMP_Ver = false;

                    deriveTestModeAMP(outBestCU, parentSize, bTestAMP_Hor, bTestAMP_Ver, bTestMergeAMP_Hor, bTestMergeAMP_Ver);

                    // Do horizontal AMP
                    if (bTestAMP_Hor)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnU);
                            outTempCU->initEstData(depth, qp);
                            if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnU)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnD);
                            outTempCU->initEstData(depth, qp);
                            if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnD)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                    }
                    else if (bTestMergeAMP_Hor)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnU, true);
                            outTempCU->initEstData(depth, qp);
                            if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnU)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnD, true);
                            outTempCU->initEstData(depth, qp);
                            if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnD)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                    }

                    // Do horizontal AMP
                    if (bTestAMP_Ver)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nLx2N);
                            outTempCU->initEstData(depth, qp);
                            if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_nLx2N)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nRx2N);
                            outTempCU->initEstData(depth, qp);
                        }
                    }
                    else if (bTestMergeAMP_Ver)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nLx2N, true);
                            outTempCU->initEstData(depth, qp);
                            if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_nLx2N)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nRx2N, true);
                            outTempCU->initEstData(depth, qp);
                        }
                    }
                }
            }

            // do normal intra modes
            // speedup for inter frames
            #if !RK_CTU_CALC_PROC_ENABLE
            if (outBestCU->getSlice()->getSliceType() == I_SLICE ||
                outBestCU->getCbf(0, TEXT_LUMA) != 0   ||
                outBestCU->getCbf(0, TEXT_CHROMA_U) != 0   ||
                outBestCU->getCbf(0, TEXT_CHROMA_V) != 0) // avoid very complex intra if it is unlikely
            #endif
            {
                xCheckRDCostIntraInInter(outBestCU, outTempCU, SIZE_2Nx2N);
                outTempCU->initEstData(depth, qp);

                if (depth == g_maxCUDepth - g_addCUDepth)
                {
                    if (outTempCU->getWidth(0) > (1 << outTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize()))
                    {
                        xCheckRDCostIntraInInter(outBestCU, outTempCU, SIZE_NxN);
                        outTempCU->initEstData(depth, qp);
                    }
                }
            }
            // test PCM
            if (pic->getSlice()->getSPS()->getUsePCM()
                && outTempCU->getWidth(0) <= (1 << pic->getSlice()->getSPS()->getPCMLog2MaxSize())
                && outTempCU->getWidth(0) >= (1 << pic->getSlice()->getSPS()->getPCMLog2MinSize()))
            {
                uint32_t rawbits = (2 * X265_DEPTH + X265_DEPTH) * outBestCU->getWidth(0) * outBestCU->getHeight(0) / 2;
                uint32_t bestbits = outBestCU->m_totalBits;
                if ((bestbits > rawbits) || (outBestCU->m_totalCost > m_rdCost->calcRdCost(0, rawbits)))
                {
                    xCheckIntraPCM(outBestCU, outTempCU);
                }
            }
        }

        m_entropyCoder->resetBits();
        m_entropyCoder->encodeSplitFlag(outBestCU, 0, depth, true);
        outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        outBestCU->m_totalCost  = m_rdCost->calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);

        //curr level CU Cost、Bits、Distortion
        #if RK_CTU_CALC_PROC_ENABLE
        uint8_t pos;
        uint32_t m;
        uint8_t cu_w = outBestCU->m_width[0];
        switch(depth)
        {
            case 0:
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[0][0], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[0][0]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[0][0]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[0][0]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                break;
            case 1:
                pos = pHardWare->ctu_calc.best_pos[1];
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[1][pos], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[1][pos]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[1][pos]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[1][pos]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                pHardWare->ctu_calc.best_pos[1]++;
                break;
            case 2:
                pos = pHardWare->ctu_calc.best_pos[2];
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[2][pos], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[2][pos]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[2][pos]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[2][pos]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                pHardWare->ctu_calc.best_pos[2]++;
                break;
            case 3:
                pos = pHardWare->ctu_calc.best_pos[3];
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[3][pos], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[3][pos]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[3][pos]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[3][pos]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                pHardWare->ctu_calc.best_pos[3]++;
                break;
            default:
                break;
        }
        #endif

        #if !RK_CTU_CALC_PROC_ENABLE
        // Early CU determination
        if (outBestCU->isSkipped(0))
            bSubBranch = false;
        else
            bSubBranch = true;
        #endif
    }
    else if (!(bSliceEnd && bInsidePicture))
    {
        bBoundary = true;
        m_addSADDepth++;
    }

    // copy original YUV samples to PCM buffer
    if (outBestCU->isLosslessCoded(0) && (outBestCU->getIPCMFlag(0) == false))
    {
        xFillPCMBuffer(outBestCU, m_origYuv[depth]);
    }

    outTempCU->initEstData(depth, qp);

    // further split
    if (bSubBranch && bTrySplitDQP && depth < g_maxCUDepth - g_addCUDepth)
    {
        UChar       nextDepth     = depth + 1;
        TComDataCU* subBestPartCU = m_bestCU[nextDepth];
        TComDataCU* subTempPartCU = m_tempCU[nextDepth];
        uint32_t partUnitIdx = 0;
        for (; partUnitIdx < 4; partUnitIdx++)
        {
            subBestPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.
            subTempPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.

            bool bInSlice = subBestPartCU->getSCUAddr() < slice->getSliceCurEndCUAddr();
            if (bInSlice && (subBestPartCU->getCUPelX() < slice->getSPS()->getPicWidthInLumaSamples()) &&
                (subBestPartCU->getCUPelY() < slice->getSPS()->getPicHeightInLumaSamples()))
            {
                if (0 == partUnitIdx) //initialize RD with previous depth buffer
                {
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
                }
                else
                {
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[nextDepth][CI_NEXT_BEST]);
                }

                xCompressCU(subBestPartCU, subTempPartCU, nextDepth);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth); // Keep best part data to current temporary data.
                xCopyYuv2Tmp(subBestPartCU->getTotalNumPart() * partUnitIdx, nextDepth);
            }
            else if (bInSlice)
            {
                subBestPartCU->copyToPic(nextDepth);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth);
            }
        }

        if (!bBoundary)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeSplitFlag(outTempCU, 0, depth, true);

            outTempCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        }
        outTempCU->m_totalCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

        #if RK_OUTPUT_FILE
        if(depth == 0){
            fprintf(fp, "ctu_num = %d\n", ctu_num++);
        }
        fprintf(fp, "depth = %d, bestCU_cost = %.8x, tempCU_cost = %.8x\n", depth, outBestCU->m_totalCost, outTempCU->m_totalCost);
        if(depth == 0){
            fprintf(fp, "\n");
        }
        #endif

        #if RK_CTU_CALC_PROC_ENABLE
        uint8_t pos;
        switch(depth)
        {
            case 0:
                pHardWare->ctu_calc.intra_temp_64.m_totalDistortion = outTempCU->m_totalDistortion;
                pHardWare->ctu_calc.intra_temp_64.m_totalBits       = outTempCU->m_totalBits;
                pHardWare->ctu_calc.intra_temp_64.m_totalCost       = outTempCU->m_totalCost;
                break;
            case 1:
                pos = pHardWare->ctu_calc.temp_pos[1];
                pHardWare->ctu_calc.intra_temp_32[pos].m_totalDistortion = outTempCU->m_totalDistortion;
                pHardWare->ctu_calc.intra_temp_32[pos].m_totalBits       = outTempCU->m_totalBits;
                pHardWare->ctu_calc.intra_temp_32[pos].m_totalCost       = outTempCU->m_totalCost;
                pHardWare->ctu_calc.temp_pos[1]++;
                break;
            case 2:
                pos = pHardWare->ctu_calc.temp_pos[2];
                pHardWare->ctu_calc.intra_temp_16[pos].m_totalDistortion = outTempCU->m_totalDistortion;
                pHardWare->ctu_calc.intra_temp_16[pos].m_totalBits       = outTempCU->m_totalBits;
                pHardWare->ctu_calc.intra_temp_16[pos].m_totalCost       = outTempCU->m_totalCost;
                pHardWare->ctu_calc.temp_pos[2]++;
                break;
            case 3:
                break;
            default:
                break;
        }
        #endif

        if ((g_maxCUWidth >> depth) == outTempCU->getSlice()->getPPS()->getMinCuDQPSize() && outTempCU->getSlice()->getPPS()->getUseDQP())
        {
            bool hasResidual = false;
            for (uint32_t blkIdx = 0; blkIdx < outTempCU->getTotalNumPart(); blkIdx++)
            {
                if (outTempCU->getCbf(blkIdx, TEXT_LUMA) || outTempCU->getCbf(blkIdx, TEXT_CHROMA_U) ||
                    outTempCU->getCbf(blkIdx, TEXT_CHROMA_V))
                {
                    hasResidual = true;
                    break;
                }
            }

            uint32_t targetPartIdx;
            targetPartIdx = 0;
            if (hasResidual)
            {
                bool foundNonZeroCbf = false;
                outTempCU->setQPSubCUs(outTempCU->getRefQP(targetPartIdx), outTempCU, 0, depth, foundNonZeroCbf);
                assert(foundNonZeroCbf);
            }
            else
            {
                outTempCU->setQPSubParts(outTempCU->getRefQP(targetPartIdx), 0, depth); // set QP to default QP
            }
        }

        #if RK_CTU_CALC_PROC_ENABLE
        switch(depth)
        {
            case 0:
                if (outTempCU->m_totalCost < outBestCU->m_totalCost)
                    pHardWare->ctu_calc.ori_cu_split_flag[0] = 1;
                break;
            case 1:
                if (outTempCU->m_totalCost < outBestCU->m_totalCost)
                    pHardWare->ctu_calc.ori_cu_split_flag[pHardWare->ctu_calc.best_pos[1] - 1 + 1] = 1;
                break;
            case 2:
                if (outTempCU->m_totalCost < outBestCU->m_totalCost)
                    pHardWare->ctu_calc.ori_cu_split_flag[pHardWare->ctu_calc.best_pos[2] - 1 + 5] = 1;
                break;
            case 3:
                break;
            default:
                break;
        }
        #endif

        m_rdSbacCoders[nextDepth][CI_NEXT_BEST]->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);
        xCheckBestMode(outBestCU, outTempCU, depth); // RD compare current CU against split
    }
    outBestCU->copyToPic(depth); // Copy Best data to Picture for next partition prediction.
    // Copy Yuv data to picture Yuv
    xCopyYuv2Pic(outBestCU->getPic(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth, depth, outBestCU, lpelx, tpely);

    #if RK_CTU_CALC_PROC_ENABLE
    if(depth == 0)
    {
        uint32_t m;
        memcpy(pHardWare->ctu_calc.ori_recon_y, m_bestRecoYuv[0]->m_bufY, 64*64);
        memcpy(pHardWare->ctu_calc.ori_recon_u, m_bestRecoYuv[0]->m_bufU, 32*32);
        memcpy(pHardWare->ctu_calc.ori_recon_v, m_bestRecoYuv[0]->m_bufV, 32*32);

        for(m=0; m<64*64; m++)
            pHardWare->ctu_calc.ori_resi_y[m] = outBestCU->m_trCoeffY[m];

        for(m=0; m<32*32; m++) {
            pHardWare->ctu_calc.ori_resi_u[m] = outBestCU->m_trCoeffCb[m];
            pHardWare->ctu_calc.ori_resi_v[m] = outBestCU->m_trCoeffCr[m];
        }
    }
    #endif

    if (bBoundary || (bSliceEnd && bInsidePicture)) return;

    // Assert if Best prediction mode is NONE
    // Selected mode's RD-cost must be not MAX_DOUBLE.
    assert(outBestCU->getPartitionSize(0) != SIZE_NONE);
    assert(outBestCU->getPredictionMode(0) != MODE_NONE);
    assert(outBestCU->m_totalCost != MAX_DOUBLE);
}

#if RK_INTER_CALC_RDO_TWICE
void TEncCu::xCheckRDCostMerge2Nx2N(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComYuv*& outBestPredYuv, TComYuv*& rpcYuvReconBest)
{
	assert(outTempCU->getSlice()->getSliceType() != I_SLICE);
	TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
	UChar interDirNeighbours[MRG_MAX_NUM_CANDS];
	int numValidMergeCand = 0;

	for (uint32_t i = 0; i < outTempCU->getSlice()->getMaxNumMergeCand(); ++i)
	{
		interDirNeighbours[i] = 0;
	}

	UChar depth = outTempCU->getDepth(0);
	outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
	outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);
	outTempCU->getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours, numValidMergeCand);

	int mergeCandBuffer[MRG_MAX_NUM_CANDS];
	for (uint32_t i = 0; i < numValidMergeCand; ++i)
	{
		mergeCandBuffer[i] = 0;
	}
	uint32_t Distortion = MAX_INT; uint32_t mergeIdx = 0;
	for (uint32_t mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand)
	{
		// set MC parameters
		outTempCU->setPredModeSubParts(MODE_INTER, 0, depth); // interprets depth relative to LCU level
		outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(),     0, depth);
		outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
		outTempCU->setMergeFlagSubParts(true, 0, 0, depth); // interprets depth relative to LCU level
		outTempCU->setMergeIndexSubParts(mergeCand, 0, 0, depth); // interprets depth relative to LCU level
		outTempCU->setInterDirSubParts(interDirNeighbours[mergeCand], 0, 0, depth); // interprets depth relative to LCU level
		outTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mvFieldNeighbours[0 + 2 * mergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level
		outTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mvFieldNeighbours[1 + 2 * mergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level
		outTempCU->setSkipFlagSubParts(false, 0, depth);

		m_search->motionCompensation(outTempCU, m_tmpPredYuv[depth], REF_PIC_LIST_X, -1, true, false);
		int part = partitionFromSizes(outTempCU->getWidth(0), outTempCU->getHeight(0));
		m_rdGoOnSbacCoder->load(m_rdSbacCoders[outTempCU->getDepth(0)][CI_CURR_BEST]);
		outTempCU->m_totalBits = m_search->xSymbolBitsInter(outTempCU, true);
		uint32_t distortion; //已经跟踪过了,此处的当前CU的Y分量信息和ME的Y分量信息一致,同时相应参考帧里的参考Y分量也一致
		if (JUDGE_SATD == outTempCU->getSlice()->getSPS()->getJudgeStand()) //SATD
		{
			distortion = primitives.satd[part](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
			 m_tmpPredYuv[depth]->getLumaAddr(), m_tmpPredYuv[depth]->getStride());
			outTempCU->m_totalCost = m_rdCost->calcRdSADCost(distortion, outTempCU->m_totalBits); //无satd的lambda值
		}
		else if (JUDGE_SAD == outTempCU->getSlice()->getSPS()->getJudgeStand()) //SAD
		{
			distortion = primitives.sad[part](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
			 m_tmpPredYuv[depth]->getLumaAddr(), m_tmpPredYuv[depth]->getStride());
			outTempCU->m_totalCost = m_rdCost->calcRdSADCost(distortion, outTempCU->m_totalBits);
		}
		else //SSE
		{
			distortion = primitives.sse_pp[part](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
			 m_tmpPredYuv[depth]->getLumaAddr(), m_tmpPredYuv[depth]->getStride());
			outTempCU->m_totalCost = m_rdCost->calcRdCost(distortion, outTempCU->m_totalBits);
		}
		int origQP = outTempCU->getQP(0);
		xCheckDQP(outTempCU);
		if (outTempCU->m_totalCost < outBestCU->m_totalCost)
		{
			TComDataCU* tmp = outTempCU;
			outTempCU = outBestCU;
			outBestCU = tmp;

			// Change Prediction data
			TComYuv* yuv = NULL;
			yuv = outBestPredYuv;
			outBestPredYuv = m_tmpPredYuv[depth];
			m_tmpPredYuv[depth] = yuv;

			yuv = rpcYuvReconBest;
			rpcYuvReconBest = m_tmpRecoYuv[depth];
			m_tmpRecoYuv[depth] = yuv;
		}
		outTempCU->initEstData(depth, origQP);
#if INTER_MERGE_TEST
		if (distortion < Distortion)
		{
			Distortion = distortion;
			mergeIdx = mergeCand;
		}
#endif
	}
#if INTER_MERGE_TEST
	assert(3 == outTempCU->getSlice()->getMaxNumMergeCand()); //测试的时候只用3个merge candidate
	assert(JUDGE_SATD == outTempCU->getSlice()->getSPS()->getJudgeStand());
	uint32_t offsIdx = getOffsetIdx(nCtuSize, outTempCU->getCUPelX(), outTempCU->getCUPelY(), outTempCU->getWidth(0));
	for (uint32_t mergeCand = 0; mergeCand < 3; ++mergeCand)
	{
		interinfo.fmeinput.MergeMvX[offsIdx][0][mergeCand] = mvFieldNeighbours[0 + mergeCand * 2].mv.x;
		interinfo.fmeinput.MergeMvY[offsIdx][0][mergeCand] = mvFieldNeighbours[0 + mergeCand * 2].mv.y;
		interinfo.fmeinput.MergeMvX[offsIdx][1][mergeCand] = mvFieldNeighbours[1 + mergeCand * 2].mv.x;
		interinfo.fmeinput.MergeMvY[offsIdx][1][mergeCand] = mvFieldNeighbours[1 + mergeCand * 2].mv.y;
	}
	fmeoutput.nSatdMerge[offsIdx] = Distortion;
	fmeoutput.nMergeIdx[offsIdx] = mergeIdx;
#endif
}
void TEncCu::xCheckRDCostInter(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize, bool isCalcRDOTwice, bool bUseMRG)
{
	if (!isCalcRDOTwice)
		return;
	UChar depth = outTempCU->getDepth(0);

	outTempCU->setDepthSubParts(depth, 0);
	outTempCU->setSkipFlagSubParts(false, 0, depth);
	outTempCU->setPartSizeSubParts(partSize, 0, depth);
	outTempCU->setPredModeSubParts(MODE_INTER, 0, depth);
	outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

	outTempCU->setMergeAMP(true);
	m_tmpRecoYuv[depth]->clear();
	m_tmpResiYuv[depth]->clear();
	m_search->predInterSearch(outTempCU, m_tmpPredYuv[depth], bUseMRG, true, false);

	m_rdGoOnSbacCoder->load(m_rdSbacCoders[outTempCU->getDepth(0)][CI_CURR_BEST]);
	int part = partitionFromSizes(outTempCU->getWidth(0), outTempCU->getHeight(0));
	if (JUDGE_SATD == outTempCU->getSlice()->getSPS()->getJudgeStand()) //satd
	{
		uint32_t distortion = primitives.satd[part](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
			m_tmpPredYuv[depth]->getLumaAddr(), m_tmpPredYuv[depth]->getStride()); //sse_pp求出残差平方的和 add by hdl
		outTempCU->m_totalBits = m_search->xSymbolBitsInter(outTempCU,true);
		outTempCU->m_totalCost = m_rdCost->calcRdSADCost(distortion, outTempCU->m_totalBits);
	}
	else if (JUDGE_SAD == outTempCU->getSlice()->getSPS()->getJudgeStand()) //sad
	{
		uint32_t distortion = primitives.sad[part](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
			m_tmpPredYuv[depth]->getLumaAddr(), m_tmpPredYuv[depth]->getStride()); //sse_pp求出残差平方的和 add by hdl
		outTempCU->m_totalBits = m_search->xSymbolBitsInter(outTempCU,true);
		outTempCU->m_totalCost = m_rdCost->calcRdSADCost(distortion, outTempCU->m_totalBits);
	}
	else //sse_pp
	{
		uint32_t distortion = primitives.sse_pp[part](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
			m_tmpPredYuv[depth]->getLumaAddr(), m_tmpPredYuv[depth]->getStride()); //sse_pp求出残差平方的和 add by hdl
		outTempCU->m_totalBits = m_search->xSymbolBitsInter(outTempCU,true);
		outTempCU->m_totalCost = m_rdCost->calcRdCost(distortion, outTempCU->m_totalBits);
	}

	xCheckBestMode(outBestCU, outTempCU, depth, true);
}
void TEncCu::xCheckBestMode(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth, bool isCalcRDOTwice)
{
	if (outTempCU->m_totalCost < outBestCU->m_totalCost && isCalcRDOTwice)
	{
		TComYuv* yuv;
		// Change Information data
		TComDataCU* cu = outBestCU;
		outBestCU = outTempCU;
		outTempCU = cu;

		// Change Prediction data
		yuv = m_bestPredYuv[depth];
		m_bestPredYuv[depth] = m_tmpPredYuv[depth];
		m_tmpPredYuv[depth] = yuv;
	}
}
void TEncCu::xCompressCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth, bool isCalcRDOTwice, PartSize parentSize)
{
	if (!isCalcRDOTwice)
		assert(false);

	//PPAScopeEvent(TEncCu_xCompressCU + depth);
	TComPic* pic = outBestCU->getPic();
	m_abortFlag = false;

	// get Original YUV data from picture
	m_origYuv[depth]->copyFromPicYuv(pic->getPicYuvOrg(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU());
    #if RK_CTU_CALC_PROC_ENABLE
    if(depth == 0) {
        memcpy(pHardWare->ctu_calc.input_curr_y, m_origYuv[depth]->m_bufY, pHardWare->ctu_calc.ctu_w*pHardWare->ctu_calc.ctu_w);
        memcpy(pHardWare->ctu_calc.input_curr_u, m_origYuv[depth]->m_bufU, pHardWare->ctu_calc.ctu_w*pHardWare->ctu_calc.ctu_w/4);
        memcpy(pHardWare->ctu_calc.input_curr_v, m_origYuv[depth]->m_bufV, pHardWare->ctu_calc.ctu_w*pHardWare->ctu_calc.ctu_w/4);
    }//add by zsq
    #endif

	// variables for fast encoder decision
	bool bTrySplit = true;

	// variable for Early CU determination
	bool bSubBranch = true;
	// variable for Cbf fast mode PU decision
	bool doNotBlockPu = true;
	bool bTrySplitDQP = true;

	bool bBoundary = false;
	uint32_t lpelx = outBestCU->getCUPelX();
	uint32_t rpelx = lpelx + outBestCU->getWidth(0)  - 1;
	uint32_t tpely = outBestCU->getCUPelY();
	uint32_t bpely = tpely + outBestCU->getHeight(0) - 1;

	int qp = outTempCU->getQP(0);

	// If slice start or slice end is within this cu...
	TComSlice* slice = outTempCU->getPic()->getSlice();
	bool bSliceEnd = (slice->getSliceCurEndCUAddr() > outTempCU->getSCUAddr() &&
		slice->getSliceCurEndCUAddr() < outTempCU->getSCUAddr() + outTempCU->getTotalNumPart());
	bool bInsidePicture = (rpelx < outBestCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) &&
		(bpely < outBestCU->getSlice()->getSPS()->getPicHeightInLumaSamples());

	// We need to split, so don't try these modes.
	if (!bSliceEnd && bInsidePicture)
	{
		// variables for fast encoder decision
		bTrySplit    = true;

		outTempCU->initEstData(depth, qp);

		// do inter modes, SKIP and 2Nx2N
		if (outBestCU->getSlice()->getSliceType() != I_SLICE)
		{
			xCheckRDCostMerge2Nx2N(outBestCU, outTempCU, m_bestPredYuv[depth], m_bestRecoYuv[depth]);
			outTempCU->initEstData(depth, qp);
#if INTER_IME_VS_MERGE
			//用来统计merge的5个candidate的mv方向与ME出来的mv的距离  暂时只统计P帧   add by hdl
			const int PixelDist = 10;
			static uint64_t cout_8x8[PixelDist],     cout_All_8x8[PixelDist][2];
			static uint64_t cout_16x16[PixelDist], cout_All_16x16[PixelDist][2];
			static uint64_t cout_32x32[PixelDist], cout_All_32x32[PixelDist][2];
			static uint64_t cout_64x64[PixelDist], cout_All_64x64[PixelDist][2];
			TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
			UChar interDirNeighbours[MRG_MAX_NUM_CANDS];
			int numValidMergeCand = 0;
			uint32_t Depth = outTempCU->getDepth(0);
			outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, Depth); // interprets depth relative to LCU level
			outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, Depth);
			outTempCU->getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours, numValidMergeCand);
			for (int i = 0; i < 5; i++)
				mvFieldNeighbours[2 * i].mv = mvFieldNeighbours[2 * i].mv >> 2;
			outTempCU->initEstData(Depth, qp);
			uint64_t tmpBestCost = outBestCU->m_totalCost; //保存merge的代价
			xCheckRDCostInter(outBestCU, outTempCU, SIZE_2Nx2N, true, false);
			MV mv;
			if (tmpBestCost == outBestCU->m_totalCost) //说明最优解为merge的2Nx2N  add by hdl
				mv = *(outTempCU->getCUMvField(0)->m_mv);
			else //说明最优解为非merge的2Nx2N  add by hdl
				mv = *(outBestCU->getCUMvField(0)->m_mv);
			mv = mv >> 2;

			if (tmpBestCost == outBestCU->m_totalCost) //merge的情况下,统计merge总数,以及merge的mv在2Nx2N某个像素范围内的总数
			{
				for (int i = 0; i < PixelDist; i ++)
				{
					switch (Depth)
					{
					case 0: cout_All_64x64[i][0] ++; break;
					case 1: cout_All_32x32[i][0] ++; break;
					case 2: cout_All_16x16[i][0] ++; break;
					case 3: cout_All_8x8[i][0] ++;    break;
					default: assert(false);
					}
					bool tmp = false;
					for (int j = 0; j < 5; j++)
					{
						//tmp |= mv.word == mvFieldNeighbours[2*i].mv.word;
						tmp |= (abs(mv.x - mvFieldNeighbours[2 * j].mv.x) <= i) && (abs(mv.y - mvFieldNeighbours[2 * j].mv.y) <= i);
					}
					if (tmp)
					{
						switch (Depth)
						{
						case 0: cout_64x64[i] ++; break;
						case 1: cout_32x32[i] ++; break;
						case 2: cout_16x16[i] ++; break;
						case 3: cout_8x8[i] ++;    break;
						default: assert(false);
						}
					}
				}

			}

			for (int i = 0; i < PixelDist; i++)
			{
				switch (Depth)
				{
				case 0: cout_All_64x64[i][1] ++; break;
				case 1: cout_All_32x32[i][1] ++; break;
				case 2: cout_All_16x16[i][1] ++; break;
				case 3: cout_All_8x8[i][1] ++;    break;
				default: assert(false);
				}
			}
			uint32_t cuAddr = outTempCU->getAddr();
			uint32_t nFrameWidthInCtu = outTempCU->getPic()->getFrameWidthInCU();
			uint32_t nFrameHeightInCtu = outTempCU->getPic()->getFrameHeightInCU();
			if (cuAddr == nFrameHeightInCtu*nFrameWidthInCtu - 1)
			{
				FILE *fp = fopen("count_value.txt", "wb");
				for (int i = 0; i < PixelDist; i++)
				{
					double Rate_64x64, Rate_32x32, Rate_16x16, Rate_8x8;
					Rate_64x64 = (double)cout_64x64[i] / (double)cout_All_64x64[i][0];
					Rate_32x32 = (double)cout_32x32[i] / (double)cout_All_32x32[i][0];
					Rate_16x16 = (double)cout_16x16[i] / (double)cout_All_16x16[i][0];
					Rate_8x8 = (double)cout_8x8[i] / (double)cout_All_8x8[i][0];
					fprintf(fp, "\nPixel Distant is: %d\n", i);
					fprintf(fp, "CU size is 64x64: All, %10lld,  All merge, %10lld, Hit mv of 2Nx2N, %10lld, Rate, %10lf\n",
						cout_All_64x64[i][1], cout_All_64x64[i][0], cout_64x64[i], Rate_64x64);
					fprintf(fp, "CU size is 32x32: All, %10lld,  All merge, %10lld, Hit mv of 2Nx2N, %10lld, Rate, %10lf\n",
						cout_All_32x32[i][1], cout_All_32x32[i][0], cout_32x32[i], Rate_32x32);
					fprintf(fp, "CU size is 16x16: All, %10lld,  All merge, %10lld, Hit mv of 2Nx2N, %10lld, Rate, %10lf\n",
						cout_All_16x16[i][1], cout_All_16x16[i][0], cout_16x16[i], Rate_16x16);
					fprintf(fp, "CU size is 8x8:   All, %10lld,  All merge, %10lld, Hit mv of 2Nx2N, %10lld, Rate, %10lf\n",
						cout_All_8x8[i][1], cout_All_8x8[i][0], cout_8x8[i], Rate_8x8);
				}
				fclose(fp);
			}
			outTempCU->initEstData(depth, qp);
			//用来统计merge的5个candidate的mv方向与ME出来的mv的距离  暂时只统计P帧   add by hdl
#else
			xCheckRDCostInter(outBestCU, outTempCU, SIZE_2Nx2N, true, false);
			outTempCU->initEstData(depth, qp);
#endif
			if (m_cfg->param.bEnableCbfFastMode)
			{
				doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
			}
		}

		bTrySplitDQP = bTrySplit;

		if (depth <= m_addSADDepth)
		{
			m_LCUPredictionSAD += m_temporalSAD;
			m_addSADDepth = depth;
		}

		outTempCU->initEstData(depth, qp);

		// do inter modes, NxN, 2NxN, and Nx2N
		if (outBestCU->getSlice()->getSliceType() != I_SLICE)
		{
			// 2Nx2N, NxN
			if (!((outBestCU->getWidth(0) == 8) && (outBestCU->getHeight(0) == 8)))
			{
				if (depth == g_maxCUDepth - g_addCUDepth && doNotBlockPu)
				{
					xCheckRDCostInter(outBestCU, outTempCU, SIZE_NxN, true, false);
					outTempCU->initEstData(depth, qp);
				}
			}

			if (m_cfg->param.bEnableRectInter)
			{
				// 2NxN, Nx2N
				if (doNotBlockPu)
				{
					xCheckRDCostInter(outBestCU, outTempCU, SIZE_Nx2N, true, false);
					outTempCU->initEstData(depth, qp);
					if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_Nx2N)
					{
						doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
					}
				}
				if (doNotBlockPu)
				{
					xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxN, true, false);
					outTempCU->initEstData(depth, qp);
					if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxN)
					{
						doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
					}
				}
			}

			// Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
			if (pic->getSlice()->getSPS()->getAMPAcc(depth))
			{
				bool bTestAMP_Hor = false, bTestAMP_Ver = false;
				bool bTestMergeAMP_Hor = false, bTestMergeAMP_Ver = false;

				deriveTestModeAMP(outBestCU, parentSize, bTestAMP_Hor, bTestAMP_Ver, bTestMergeAMP_Hor, bTestMergeAMP_Ver);

				// Do horizontal AMP
				if (bTestAMP_Hor)
				{
					if (doNotBlockPu)
					{
						xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnU, true, false);
						outTempCU->initEstData(depth, qp);
						if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnU)
						{
							doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
						}
					}
					if (doNotBlockPu)
					{
						xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnD, true, false);
						outTempCU->initEstData(depth, qp);
						if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnD)
						{
							doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
						}
					}
				}
				else if (bTestMergeAMP_Hor)
				{
					if (doNotBlockPu)
					{
						xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnU, true, true);
						outTempCU->initEstData(depth, qp);
						if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnU)
						{
							doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
						}
					}
					if (doNotBlockPu)
					{
						xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnD, true, true);
						outTempCU->initEstData(depth, qp);
						if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnD)
						{
							doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
						}
					}
				}

				// Do horizontal AMP
				if (bTestAMP_Ver)
				{
					if (doNotBlockPu)
					{
						xCheckRDCostInter(outBestCU, outTempCU, SIZE_nLx2N, true, false);
						outTempCU->initEstData(depth, qp);
						if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_nLx2N)
						{
							doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
						}
					}
					if (doNotBlockPu)
					{
						xCheckRDCostInter(outBestCU, outTempCU, SIZE_nRx2N, true, false);
						outTempCU->initEstData(depth, qp);
					}
				}
				else if (bTestMergeAMP_Ver)
				{
					if (doNotBlockPu)
					{
						xCheckRDCostInter(outBestCU, outTempCU, SIZE_nLx2N, true, true);
						outTempCU->initEstData(depth, qp);
						if (m_cfg->param.bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_nLx2N)
						{
							doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
						}
					}
					if (doNotBlockPu)
					{
						xCheckRDCostInter(outBestCU, outTempCU, SIZE_nRx2N, true, true);
						outTempCU->initEstData(depth, qp);
					}
				}
			}
		}

		int numPart = outBestCU->getNumPartInter();
		for (int partIdx = 0; partIdx < numPart; partIdx++)
		{//运动补偿根据mv值得到预测值  add by hdl
			m_search->motionCompensation(outBestCU, m_bestPredYuv[depth], REF_PIC_LIST_X, partIdx, false, true);
		}
		m_search->encodeResAndCalcRdInterCU(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth], m_bestResiYuv[depth], m_bestRecoYuv[depth], false);

		// do normal intra modes
		// speedup for inter frames
		#if 0
		if (outBestCU->getSlice()->getSliceType() == I_SLICE ||
			outBestCU->getCbf(0, TEXT_LUMA) != 0   ||
			outBestCU->getCbf(0, TEXT_CHROMA_U) != 0   ||
			outBestCU->getCbf(0, TEXT_CHROMA_V) != 0) // avoid very complex intra if it is unlikely
		#endif
		{
			xCheckRDCostIntraInInter(outBestCU, outTempCU, SIZE_2Nx2N);
			outTempCU->initEstData(depth, qp);

			if (depth == g_maxCUDepth - g_addCUDepth)
			{
				if (outTempCU->getWidth(0) > (1 << outTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize()))
				{
					xCheckRDCostIntraInInter(outBestCU, outTempCU, SIZE_NxN);
					outTempCU->initEstData(depth, qp);
				}
			}
		}
		// test PCM
		if (pic->getSlice()->getSPS()->getUsePCM()
			&& outTempCU->getWidth(0) <= (1 << pic->getSlice()->getSPS()->getPCMLog2MaxSize())
			&& outTempCU->getWidth(0) >= (1 << pic->getSlice()->getSPS()->getPCMLog2MinSize()))
		{
			uint32_t rawbits = (2 * X265_DEPTH + X265_DEPTH) * outBestCU->getWidth(0) * outBestCU->getHeight(0) / 2;
			uint32_t bestbits = outBestCU->m_totalBits;
			if ((bestbits > rawbits) || (outBestCU->m_totalCost > m_rdCost->calcRdCost(0, rawbits)))
			{
				xCheckIntraPCM(outBestCU, outTempCU);
			}
		}

		m_entropyCoder->resetBits();
		m_entropyCoder->encodeSplitFlag(outBestCU, 0, depth, true);
		outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
		outBestCU->m_totalCost  = m_rdCost->calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);

        //curr level CU Cost、Bits、Distortion
        #if RK_CTU_CALC_PROC_ENABLE
        uint8_t pos;
        uint32_t m;
        uint8_t cu_w = outBestCU->m_width[0];
        switch(depth)
        {
            case 0:
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[0][0], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[0][0]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[0][0]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[0][0]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                break;
            case 1:
                pos = pHardWare->ctu_calc.best_pos[1];
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[1][pos], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[1][pos]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[1][pos]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[1][pos]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                pHardWare->ctu_calc.best_pos[1]++;
                break;
            case 2:
                pos = pHardWare->ctu_calc.best_pos[2];
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[2][pos], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[2][pos]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[2][pos]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[2][pos]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                pHardWare->ctu_calc.best_pos[2]++;
                break;
            case 3:
                pos = pHardWare->ctu_calc.best_pos[3];
                copy_cu_data(pHardWare->ctu_calc.cu_ori_data[3][pos], outBestCU);
                for(m=0; m<cu_w*cu_w;   m++) pHardWare->ctu_calc.cu_ori_data[3][pos]->ReconY[m] = m_bestRecoYuv[depth]->m_bufY[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[3][pos]->ReconU[m] = m_bestRecoYuv[depth]->m_bufU[m];
                for(m=0; m<cu_w*cu_w/4; m++) pHardWare->ctu_calc.cu_ori_data[3][pos]->ReconV[m] = m_bestRecoYuv[depth]->m_bufV[m];
                pHardWare->ctu_calc.best_pos[3]++;
                break;
            default:
                break;
        }
        #endif

        #if !RK_CTU_CALC_PROC_ENABLE
		// Early CU determination
#if !INTER_MERGE_TEST
		if (outBestCU->isSkipped(0))
			bSubBranch = false;
		else
			bSubBranch = true;
#endif
        #endif
	}
	else if (!(bSliceEnd && bInsidePicture))
	{
		bBoundary = true;
		m_addSADDepth++;
	}

	// copy original YUV samples to PCM buffer
	if (outBestCU->isLosslessCoded(0) && (outBestCU->getIPCMFlag(0) == false))
	{
		xFillPCMBuffer(outBestCU, m_origYuv[depth]);
	}

	outTempCU->initEstData(depth, qp);

	// further split
	if (bSubBranch && bTrySplitDQP && depth < g_maxCUDepth - g_addCUDepth)
	{
		UChar       nextDepth     = depth + 1;
		TComDataCU* subBestPartCU = m_bestCU[nextDepth];
		TComDataCU* subTempPartCU = m_tempCU[nextDepth];
		uint32_t partUnitIdx = 0;
		for (; partUnitIdx < 4; partUnitIdx++)
		{
			subBestPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.
			subTempPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.

			bool bInSlice = subBestPartCU->getSCUAddr() < slice->getSliceCurEndCUAddr();
			if (bInSlice && (subBestPartCU->getCUPelX() < slice->getSPS()->getPicWidthInLumaSamples()) &&
				(subBestPartCU->getCUPelY() < slice->getSPS()->getPicHeightInLumaSamples()))
			{
				if (0 == partUnitIdx) //initialize RD with previous depth buffer
				{
					m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
				}
				else
				{
					m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[nextDepth][CI_NEXT_BEST]);
				}

				xCompressCU(subBestPartCU, subTempPartCU, nextDepth, isCalcRDOTwice);
				outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth); // Keep best part data to current temporary data.
				xCopyYuv2Tmp(subBestPartCU->getTotalNumPart() * partUnitIdx, nextDepth);
			}
			else if (bInSlice)
			{
				subBestPartCU->copyToPic(nextDepth);
				outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth);
			}
		}

		if (!bBoundary)
		{
			m_entropyCoder->resetBits();
			m_entropyCoder->encodeSplitFlag(outTempCU, 0, depth, true);

			outTempCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
		}
		outTempCU->m_totalCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

		if ((g_maxCUWidth >> depth) == outTempCU->getSlice()->getPPS()->getMinCuDQPSize() && outTempCU->getSlice()->getPPS()->getUseDQP())
		{
			bool hasResidual = false;
			for (uint32_t blkIdx = 0; blkIdx < outTempCU->getTotalNumPart(); blkIdx++)
			{
				if (outTempCU->getCbf(blkIdx, TEXT_LUMA) || outTempCU->getCbf(blkIdx, TEXT_CHROMA_U) ||
					outTempCU->getCbf(blkIdx, TEXT_CHROMA_V))
				{
					hasResidual = true;
					break;
				}
			}

			uint32_t targetPartIdx;
			targetPartIdx = 0;
			if (hasResidual)
			{
				bool foundNonZeroCbf = false;
				outTempCU->setQPSubCUs(outTempCU->getRefQP(targetPartIdx), outTempCU, 0, depth, foundNonZeroCbf);
				assert(foundNonZeroCbf);
			}
			else
			{
				outTempCU->setQPSubParts(outTempCU->getRefQP(targetPartIdx), 0, depth); // set QP to default QP
			}
		}

        #if RK_OUTPUT_FILE
        fprintf(fp, "depth = %d, currCU_cost = %.8x, PredMode = %d subCU_cost = %.8x\n",
        depth, outBestCU->m_totalCost, outBestCU->m_predModes[0], outTempCU->m_totalCost);
        if(depth == 0){
            fprintf(fp, "ctu_num = %d\n", ctu_num++);
            fprintf(fp, "\n");
        }
        #endif

        #if RK_CTU_CALC_PROC_ENABLE
        uint8_t pos;
        switch(depth)
        {
            case 0:
                pHardWare->ctu_calc.intra_temp_64.m_totalDistortion = outTempCU->m_totalDistortion;
                pHardWare->ctu_calc.intra_temp_64.m_totalBits       = outTempCU->m_totalBits;
                pHardWare->ctu_calc.intra_temp_64.m_totalCost       = outTempCU->m_totalCost;
                break;
            case 1:
                pos = pHardWare->ctu_calc.temp_pos[1];
                pHardWare->ctu_calc.intra_temp_32[pos].m_totalDistortion = outTempCU->m_totalDistortion;
                pHardWare->ctu_calc.intra_temp_32[pos].m_totalBits       = outTempCU->m_totalBits;
                pHardWare->ctu_calc.intra_temp_32[pos].m_totalCost       = outTempCU->m_totalCost;
                pHardWare->ctu_calc.temp_pos[1]++;
                break;
            case 2:
                pos = pHardWare->ctu_calc.temp_pos[2];
                pHardWare->ctu_calc.intra_temp_16[pos].m_totalDistortion = outTempCU->m_totalDistortion;
                pHardWare->ctu_calc.intra_temp_16[pos].m_totalBits       = outTempCU->m_totalBits;
                pHardWare->ctu_calc.intra_temp_16[pos].m_totalCost       = outTempCU->m_totalCost;
                pHardWare->ctu_calc.temp_pos[2]++;
                break;
            case 3:
                break;
            default:
                break;
        }
        #endif

         #if RK_CTU_CALC_PROC_ENABLE
        switch(depth)
        {
            case 0:
                if (outTempCU->m_totalCost < outBestCU->m_totalCost)
                    pHardWare->ctu_calc.ori_cu_split_flag[0] = 1;
                break;
            case 1:
                if (outTempCU->m_totalCost < outBestCU->m_totalCost)
                    pHardWare->ctu_calc.ori_cu_split_flag[pHardWare->ctu_calc.best_pos[1] - 1 + 1] = 1;
                break;
            case 2:
                if (outTempCU->m_totalCost < outBestCU->m_totalCost)
                    pHardWare->ctu_calc.ori_cu_split_flag[pHardWare->ctu_calc.best_pos[2] - 1 + 5] = 1;
                break;
            case 3:
                break;
            default:
                break;
        }
        #endif

		m_rdSbacCoders[nextDepth][CI_NEXT_BEST]->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);
		xCheckBestMode(outBestCU, outTempCU, depth); // RD compare current CU against split
	}
	outBestCU->copyToPic(depth); // Copy Best data to Picture for next partition prediction.
	// Copy Yuv data to picture Yuv
	xCopyYuv2Pic(outBestCU->getPic(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth, depth, outBestCU, lpelx, tpely);

    #if RK_CTU_CALC_PROC_ENABLE
    if(depth == 0)
    {
        uint32_t m;
        memcpy(pHardWare->ctu_calc.ori_recon_y, m_bestRecoYuv[0]->m_bufY, 64*64);
        memcpy(pHardWare->ctu_calc.ori_recon_u, m_bestRecoYuv[0]->m_bufU, 32*32);
        memcpy(pHardWare->ctu_calc.ori_recon_v, m_bestRecoYuv[0]->m_bufV, 32*32);

        for(m=0; m<64*64; m++)
            pHardWare->ctu_calc.ori_resi_y[m] = outBestCU->m_trCoeffY[m];

        for(m=0; m<32*32; m++) {
            pHardWare->ctu_calc.ori_resi_u[m] = outBestCU->m_trCoeffCb[m];
            pHardWare->ctu_calc.ori_resi_v[m] = outBestCU->m_trCoeffCr[m];
        }
    }
    #endif

	if (bBoundary || (bSliceEnd && bInsidePicture)) return;

	// Assert if Best prediction mode is NONE
	// Selected mode's RD-cost must be not MAX_DOUBLE.
	assert(outBestCU->getPartitionSize(0) != SIZE_NONE);
	assert(outBestCU->getPredictionMode(0) != MODE_NONE);
	assert(outBestCU->m_totalCost != MAX_DOUBLE);
}
#endif

/** finish encoding a cu and handle end-of-slice conditions
 * \param cu
 * \param absPartIdx
 * \param depth
 * \returns void
 */
void TEncCu::finishCU(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth)
{
    TComPic* pic = cu->getPic();
    TComSlice* slice = cu->getPic()->getSlice();

    //Calculate end address
    uint32_t cuAddr = cu->getSCUAddr() + absPartIdx;

    uint32_t internalAddress = (slice->getSliceCurEndCUAddr() - 1) % pic->getNumPartInCU();
    uint32_t externalAddress = (slice->getSliceCurEndCUAddr() - 1) / pic->getNumPartInCU();
    uint32_t posx = (externalAddress % pic->getFrameWidthInCU()) * g_maxCUWidth + g_rasterToPelX[g_zscanToRaster[internalAddress]];
    uint32_t posy = (externalAddress / pic->getFrameWidthInCU()) * g_maxCUHeight + g_rasterToPelY[g_zscanToRaster[internalAddress]];
    uint32_t width = slice->getSPS()->getPicWidthInLumaSamples();
    uint32_t height = slice->getSPS()->getPicHeightInLumaSamples();

    while (posx >= width || posy >= height)
    {
        internalAddress--;
        posx = (externalAddress % pic->getFrameWidthInCU()) * g_maxCUWidth + g_rasterToPelX[g_zscanToRaster[internalAddress]];
        posy = (externalAddress / pic->getFrameWidthInCU()) * g_maxCUHeight + g_rasterToPelY[g_zscanToRaster[internalAddress]];
    }

    internalAddress++;
    if (internalAddress == cu->getPic()->getNumPartInCU())
    {
        internalAddress = 0;
        externalAddress = (externalAddress + 1);
    }
    uint32_t realEndAddress = (externalAddress * pic->getNumPartInCU() + internalAddress);

    // Encode slice finish
    bool bTerminateSlice = false;
    if (cuAddr + (cu->getPic()->getNumPartInCU() >> (depth << 1)) == realEndAddress)
    {
        bTerminateSlice = true;
    }
    uint32_t uiGranularityWidth = g_maxCUWidth;
    posx = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
    posy = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
    bool granularityBoundary = ((posx + cu->getWidth(absPartIdx)) % uiGranularityWidth == 0 || (posx + cu->getWidth(absPartIdx) == width))
        && ((posy + cu->getHeight(absPartIdx)) % uiGranularityWidth == 0 || (posy + cu->getHeight(absPartIdx) == height));

    if (granularityBoundary)
    {
        // The 1-terminating bit is added to all streams, so don't add it here when it's 1.
        if (!bTerminateSlice)
            m_entropyCoder->encodeTerminatingBit(bTerminateSlice ? 1 : 0);
    }

    int numberOfWrittenBits = 0;
    if (m_bitCounter)
    {
        numberOfWrittenBits = m_entropyCoder->getNumberOfWrittenBits();
    }

    // Calculate slice end IF this CU puts us over slice bit size.
    uint32_t granularitySize = cu->getPic()->getNumPartInCU();
    int granularityEnd = ((cu->getSCUAddr() + absPartIdx) / granularitySize) * granularitySize;
    if (granularityEnd <= 0)
    {
        granularityEnd += X265_MAX(granularitySize, (cu->getPic()->getNumPartInCU() >> (depth << 1)));
    }
    if (granularityBoundary)
    {
        slice->setSliceBits((uint32_t)(slice->getSliceBits() + numberOfWrittenBits));
        slice->setSliceSegmentBits(slice->getSliceSegmentBits() + numberOfWrittenBits);
        if (m_bitCounter)
        {
            m_entropyCoder->resetBits();
        }
    }
}

/** Compute QP for each CU
 * \param cu Target CU
 * \returns quantization parameter
 */
int TEncCu::xComputeQP(TComDataCU* cu)
{
    int baseQP = cu->getSlice()->getSliceQp();

    return Clip3(-cu->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, baseQP);
}

/** encode a CU block recursively
 * \param cu
 * \param absPartIdx
 * \param depth
 * \returns void
 */
void TEncCu::xEncodeCU(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth)
{
    TComPic* pic = cu->getPic();

    bool bBoundary = false;
    uint32_t lpelx = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
    uint32_t rpelx = lpelx + (g_maxCUWidth >> depth)  - 1;
    uint32_t tpely = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
    uint32_t bpely = tpely + (g_maxCUHeight >> depth) - 1;

    TComSlice* slice = cu->getPic()->getSlice();

    // If slice start is within this cu...

    // We need to split, so don't try these modes.
    if ((rpelx < slice->getSPS()->getPicWidthInLumaSamples()) && (bpely < slice->getSPS()->getPicHeightInLumaSamples()))
    {
        m_entropyCoder->encodeSplitFlag(cu, absPartIdx, depth);
    }
    else
    {
        bBoundary = true;
    }

    if (((depth < cu->getDepth(absPartIdx)) && (depth < (g_maxCUDepth - g_addCUDepth))) || bBoundary)
    {
        uint32_t qNumParts = (pic->getNumPartInCU() >> (depth << 1)) >> 2;
        if ((g_maxCUWidth >> depth) == cu->getSlice()->getPPS()->getMinCuDQPSize() && cu->getSlice()->getPPS()->getUseDQP())
        {
            setdQPFlag(true);
        }
        for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++, absPartIdx += qNumParts)
        {
            lpelx = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
            tpely = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
            bool bInSlice = cu->getSCUAddr() + absPartIdx < slice->getSliceCurEndCUAddr();
            if (bInSlice && (lpelx < slice->getSPS()->getPicWidthInLumaSamples()) && (tpely < slice->getSPS()->getPicHeightInLumaSamples()))
            {
                xEncodeCU(cu, absPartIdx, depth + 1);
            }
        }

        return;
    }

    if ((g_maxCUWidth >> depth) >= cu->getSlice()->getPPS()->getMinCuDQPSize() && cu->getSlice()->getPPS()->getUseDQP())
    {
        setdQPFlag(true);
    }
    if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(cu, absPartIdx);
    }
    if (!cu->getSlice()->isIntra())
    {
        m_entropyCoder->encodeSkipFlag(cu, absPartIdx);
    }

    if (cu->isSkipped(absPartIdx))
    {
        m_entropyCoder->encodeMergeIndex(cu, absPartIdx);
        finishCU(cu, absPartIdx, depth);
        return;
    }
    m_entropyCoder->encodePredMode(cu, absPartIdx);

    m_entropyCoder->encodePartSize(cu, absPartIdx, depth);

    if (cu->isIntra(absPartIdx) && cu->getPartitionSize(absPartIdx) == SIZE_2Nx2N)
    {
        m_entropyCoder->encodeIPCMInfo(cu, absPartIdx);

        if (cu->getIPCMFlag(absPartIdx))
        {
            // Encode slice finish
            finishCU(cu, absPartIdx, depth);
            return;
        }
    }

    // prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
    m_entropyCoder->encodePredInfo(cu, absPartIdx);

    // Encode Coefficients
    bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(cu, absPartIdx, depth, cu->getWidth(absPartIdx), cu->getHeight(absPartIdx), bCodeDQP);
    setdQPFlag(bCodeDQP);

    // --- write terminating bit ---
    finishCU(cu, absPartIdx, depth);
}

/** check RD costs for a CU block encoded with merge
 * \param outBestCU
 * \param outTempCU
 * \returns void
 */
void TEncCu::xCheckRDCostMerge2Nx2N(TComDataCU*& outBestCU, TComDataCU*& outTempCU, bool *earlyDetectionSkipMode, TComYuv*& outBestPredYuv, TComYuv*& rpcYuvReconBest)
{
    assert(outTempCU->getSlice()->getSliceType() != I_SLICE);
    TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar interDirNeighbours[MRG_MAX_NUM_CANDS];
    int numValidMergeCand = 0;

    for (uint32_t i = 0; i < outTempCU->getSlice()->getMaxNumMergeCand(); ++i)
    {
        interDirNeighbours[i] = 0;
    }

    UChar depth = outTempCU->getDepth(0);
    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);
    outTempCU->getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours, numValidMergeCand);

    int mergeCandBuffer[MRG_MAX_NUM_CANDS];
    for (uint32_t i = 0; i < numValidMergeCand; ++i)
    {
        mergeCandBuffer[i] = 0;
    }

    bool bestIsSkip = false;

    uint32_t iteration;
    if (outTempCU->isLosslessCoded(0))
    {
        iteration = 1;
    }
    else
    {
        iteration = 2;
    }

    for (uint32_t noResidual = 0; noResidual < iteration; ++noResidual)
    {
        for (uint32_t mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand)
        {
            if (!(noResidual == 1 && mergeCandBuffer[mergeCand] == 1))
            {
                if (!(bestIsSkip && noResidual == 0))
                {
                    // set MC parameters
                    outTempCU->setPredModeSubParts(MODE_INTER, 0, depth); // interprets depth relative to LCU level
                    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(),     0, depth);
                    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
                    outTempCU->setMergeFlagSubParts(true, 0, 0, depth); // interprets depth relative to LCU level
                    outTempCU->setMergeIndexSubParts(mergeCand, 0, 0, depth); // interprets depth relative to LCU level
                    outTempCU->setInterDirSubParts(interDirNeighbours[mergeCand], 0, 0, depth); // interprets depth relative to LCU level
                    outTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mvFieldNeighbours[0 + 2 * mergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level
                    outTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mvFieldNeighbours[1 + 2 * mergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level

                    // do MC
                    m_search->motionCompensation(outTempCU, m_tmpPredYuv[depth]);
                    // estimate residual and encode everything
                    m_search->encodeResAndCalcRdInterCU(outTempCU,
                                                        m_origYuv[depth],
                                                        m_tmpPredYuv[depth],
                                                        m_tmpResiYuv[depth],
                                                        m_bestResiYuv[depth],
                                                        m_tmpRecoYuv[depth],
                                                        (noResidual ? true : false));

                    /* Todo: Fix the satd cost estimates. Why is merge being chosen in high motion areas: estimated distortion is too low? */
                    if (noResidual == 0)
                    {
                        if (outTempCU->getQtRootCbf(0) == 0)
                        {
                            mergeCandBuffer[mergeCand] = 1;
                        }
                    }

                    outTempCU->setSkipFlagSubParts(outTempCU->getQtRootCbf(0) == 0, 0, depth);
                    int origQP = outTempCU->getQP(0);
                    xCheckDQP(outTempCU);
                    if (outTempCU->m_totalCost < outBestCU->m_totalCost)
                    {
                        TComDataCU* tmp = outTempCU;
                        outTempCU = outBestCU;
                        outBestCU = tmp;

                        // Change Prediction data
                        TComYuv* yuv = NULL;
                        yuv = outBestPredYuv;
                        outBestPredYuv = m_tmpPredYuv[depth];
                        m_tmpPredYuv[depth] = yuv;

                        yuv = rpcYuvReconBest;
                        rpcYuvReconBest = m_tmpRecoYuv[depth];
                        m_tmpRecoYuv[depth] = yuv;
                    }
                    outTempCU->initEstData(depth, origQP);
                    if (!bestIsSkip)
                    {
                        bestIsSkip = outBestCU->getQtRootCbf(0) == 0;
                    }
                }
            }
        }

        if (noResidual == 0 && m_cfg->param.bEnableEarlySkip)
        {
            if (outBestCU->getQtRootCbf(0) == 0)
            {
                if (outBestCU->getMergeFlag(0))
                {
                    *earlyDetectionSkipMode = true;
                }
                else
                {
                    int mvsum = 0;
                    for (uint32_t refListIdx = 0; refListIdx < 2; refListIdx++)
                    {
                        if (outBestCU->getSlice()->getNumRefIdx(refListIdx) > 0)
                        {
                            TComCUMvField* pcCUMvField = outBestCU->getCUMvField(refListIdx);
                            int hor = abs(pcCUMvField->getMvd(0).x);
                            int ver = abs(pcCUMvField->getMvd(0).y);
                            mvsum += hor + ver;
                        }
                    }

                    if (mvsum == 0)
                    {
                        *earlyDetectionSkipMode = true;
                    }
                }
            }
        }
    }
}

void TEncCu::xCheckRDCostInter(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize, bool bUseMRG)
{
    UChar depth = outTempCU->getDepth(0);

    outTempCU->setDepthSubParts(depth, 0);
    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTER, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

    outTempCU->setMergeAMP(true);
    m_tmpRecoYuv[depth]->clear();
    m_tmpResiYuv[depth]->clear();
    m_search->predInterSearch(outTempCU, m_tmpPredYuv[depth], bUseMRG);
    m_search->encodeResAndCalcRdInterCU(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_bestResiYuv[depth], m_tmpRecoYuv[depth], false);

    xCheckDQP(outTempCU);

    xCheckBestMode(outBestCU, outTempCU, depth);
}

void TEncCu::xCheckRDCostIntra(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize)
{
    //PPAScopeEvent(TEncCU_xCheckRDCostIntra + depth);
    uint32_t depth = outTempCU->getDepth(0);
    uint32_t preCalcDistC = 0;

    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

    m_search->estIntraPredQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth], preCalcDistC, true);

    m_tmpRecoYuv[depth]->copyToPicLuma(outTempCU->getPic()->getPicYuvRec(), outTempCU->getAddr(), outTempCU->getZorderIdxInCU());

    m_search->estIntraPredChromaQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth], preCalcDistC);

    m_entropyCoder->resetBits();
    if (outTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(outTempCU, 0, true);
    }
    m_entropyCoder->encodeSkipFlag(outTempCU, 0, true);
    m_entropyCoder->encodePredMode(outTempCU, 0, true);
    m_entropyCoder->encodePartSize(outTempCU, 0, depth, true);
    m_entropyCoder->encodePredInfo(outTempCU, 0, true);
    m_entropyCoder->encodeIPCMInfo(outTempCU, 0, true);

    // Encode Coefficients
    bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(outTempCU, 0, depth, outTempCU->getWidth(0), outTempCU->getHeight(0), bCodeDQP);
    setdQPFlag(bCodeDQP);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    outTempCU->m_totalBits = m_entropyCoder->getNumberOfWrittenBits();
    outTempCU->m_totalCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);
#ifdef X265_INTRA_DEBUG
	if ( depth == 3 )
	{
		uint32_t *bits, *cost, *dist;
		bits = (SIZE_2Nx2N == partSize) ? &m_search->m_rkIntraPred->rk_totalBits8x8 : &m_search->m_rkIntraPred->rk_totalBits4x4;
		dist = (SIZE_2Nx2N == partSize) ? &m_search->m_rkIntraPred->rk_totalDist8x8 : &m_search->m_rkIntraPred->rk_totalDist4x4;
		cost = (SIZE_2Nx2N == partSize) ? &m_search->m_rkIntraPred->rk_totalCost8x8 : &m_search->m_rkIntraPred->rk_totalCost4x4;
		*bits = outTempCU->m_totalBits;
		*dist = outTempCU->m_totalDistortion;
		*cost = outTempCU->m_totalCost;

	#ifdef RK_CABAC
		uint8_t subDepth = (SIZE_2Nx2N == partSize) ? 0 : 1;
		// only care depth = 3 and 4
		g_intra_depth_total_bits[depth + subDepth][outTempCU->getZorderIdxInCU()] = outTempCU->m_totalBits;
	#endif
	}
#endif
    xCheckDQP(outTempCU);
    xCheckBestMode(outBestCU, outTempCU, depth);
#ifdef X265_INTRA_DEBUG
	if ( depth == 3 )
	{
		m_search->m_rkIntraPred->rk_totalBitsBest = outBestCU->m_totalBits;
		m_search->m_rkIntraPred->rk_totalCostBest = outBestCU->m_totalCost;
		// write to file
		if ( SIZE_NxN == partSize )
		{
		#if 0 //def INTRA_RESULT_STORE_FILE
		    RK_HEVC_FPRINT(g_fp_result_x265,
				"bits4x4 = %d dist4x4 = %d cost4x4 = %d \n bits8x8 = %d dist8x8 = %d cost8x8 = %d \n\n",
				m_search->m_rkIntraPred->rk_totalBits4x4,
				m_search->m_rkIntraPred->rk_totalDist4x4,
				m_search->m_rkIntraPred->rk_totalCost4x4,
				m_search->m_rkIntraPred->rk_totalBits8x8,
				m_search->m_rkIntraPred->rk_totalDist8x8,
				m_search->m_rkIntraPred->rk_totalCost8x8//,
				//m_search->m_rkIntraPred->rk_totalBitsBest,
				//m_search->m_rkIntraPred->rk_totalCostBest
			);
		#endif
		}
	}

	// mask 64x64 CU
#ifdef DISABLE_64x64_CU
	if ( depth == 0 )
	{
	    outBestCU->m_totalCost 			= MAX_INT64;
		outBestCU->m_totalDistortion 	= MAX_INT;
		outBestCU->m_totalBits			= MAX_INT;
	}
#endif

#endif

}

void TEncCu::xCheckRDCostIntraInInter(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize)
{
    uint32_t depth = outTempCU->getDepth(0);

    PPAScopeEvent(TEncCU_xCheckRDCostIntra + depth);

    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

    bool bSeparateLumaChroma = true; // choose estimation mode
    uint32_t preCalcDistC = 0;
    if (!bSeparateLumaChroma)
    {
        m_search->preestChromaPredMode(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth]);
    }
    m_search->estIntraPredQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth],
                             preCalcDistC, bSeparateLumaChroma);

    m_tmpRecoYuv[depth]->copyToPicLuma(outTempCU->getPic()->getPicYuvRec(), outTempCU->getAddr(), outTempCU->getZorderIdxInCU());

    m_search->estIntraPredChromaQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth], preCalcDistC);

    m_entropyCoder->resetBits();
    if (outTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(outTempCU, 0, true);
    }
    m_entropyCoder->encodeSkipFlag(outTempCU, 0, true);
    m_entropyCoder->encodePredMode(outTempCU, 0, true);
    m_entropyCoder->encodePartSize(outTempCU, 0, depth, true);
    m_entropyCoder->encodePredInfo(outTempCU, 0, true);
    m_entropyCoder->encodeIPCMInfo(outTempCU, 0, true);

    // Encode Coefficients
    bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(outTempCU, 0, depth, outTempCU->getWidth(0), outTempCU->getHeight(0), bCodeDQP);
    setdQPFlag(bCodeDQP);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    outTempCU->m_totalBits = m_entropyCoder->getNumberOfWrittenBits();
    outTempCU->m_totalCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

    xCheckDQP(outTempCU);
    xCheckBestMode(outBestCU, outTempCU, depth);
}

/** Check R-D costs for a CU with PCM mode.
 * \param outBestCU pointer to best mode CU data structure
 * \param outTempCU pointer to testing mode CU data structure
 * \returns void
 *
 * \note Current PCM implementation encodes sample values in a lossless way. The distortion of PCM mode CUs are zero. PCM mode is selected if the best mode yields bits greater than that of PCM mode.
 */
void TEncCu::xCheckIntraPCM(TComDataCU*& outBestCU, TComDataCU*& outTempCU)
{
    //PPAScopeEvent(TEncCU_xCheckIntraPCM);

    uint32_t depth = outTempCU->getDepth(0);

    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setIPCMFlag(0, true);
    outTempCU->setIPCMFlagSubParts(true, 0, outTempCU->getDepth(0));
    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    outTempCU->setTrIdxSubParts(0, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

    m_search->IPCMSearch(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth]);

    m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

    m_entropyCoder->resetBits();
    if (outTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(outTempCU, 0, true);
    }
    m_entropyCoder->encodeSkipFlag(outTempCU, 0, true);
    m_entropyCoder->encodePredMode(outTempCU, 0, true);
    m_entropyCoder->encodePartSize(outTempCU, 0, depth, true);
    m_entropyCoder->encodeIPCMInfo(outTempCU, 0, true);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    outTempCU->m_totalBits = m_entropyCoder->getNumberOfWrittenBits();
    outTempCU->m_totalCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

    xCheckDQP(outTempCU);
    xCheckBestMode(outBestCU, outTempCU, depth);
}

/** check whether current try is the best with identifying the depth of current try
 * \param outBestCU
 * \param outTempCU
 * \returns void
 */
void TEncCu::xCheckBestMode(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth)
{
    if (outTempCU->m_totalCost < outBestCU->m_totalCost)
    {
        TComYuv* yuv;
        // Change Information data
        TComDataCU* cu = outBestCU;
        outBestCU = outTempCU;
        outTempCU = cu;

        // Change Prediction data
        yuv = m_bestPredYuv[depth];
        m_bestPredYuv[depth] = m_tmpPredYuv[depth];
        m_tmpPredYuv[depth] = yuv;

        // Change Reconstruction data
        yuv = m_bestRecoYuv[depth];
        m_bestRecoYuv[depth] = m_tmpRecoYuv[depth];
        m_tmpRecoYuv[depth] = yuv;

        m_rdSbacCoders[depth][CI_TEMP_BEST]->store(m_rdSbacCoders[depth][CI_NEXT_BEST]);
    }
}

void TEncCu::xCheckDQP(TComDataCU* cu)
{
    uint32_t depth = cu->getDepth(0);

    if (cu->getSlice()->getPPS()->getUseDQP() && (g_maxCUWidth >> depth) >= cu->getSlice()->getPPS()->getMinCuDQPSize())
    {
        if (!cu->getCbf(0, TEXT_LUMA, 0) && !cu->getCbf(0, TEXT_CHROMA_U, 0) && !cu->getCbf(0, TEXT_CHROMA_V, 0))
            cu->setQPSubParts(cu->getRefQP(0), 0, depth); // set QP to default QP
    }
}

void TEncCu::xCopyAMVPInfo(AMVPInfo* src, AMVPInfo* dst)
{
    // TODO: SJB - there are multiple implementations of this function, it should be an AMVPInfo method
    dst->m_num = src->m_num;
    for (int i = 0; i < src->m_num; i++)
    {
        dst->m_mvCand[i] = src->m_mvCand[i];
    }
}

void TEncCu::xCopyYuv2Pic(TComPic* outPic, uint32_t cuAddr, uint32_t absPartIdx, uint32_t depth, uint32_t srcDepth, TComDataCU* cu, uint32_t lpelx, uint32_t tpely)
{
    uint32_t rpelx = lpelx + (g_maxCUWidth >> depth)  - 1;
    uint32_t bpely = tpely + (g_maxCUHeight >> depth) - 1;
    TComSlice* slice = cu->getPic()->getSlice();
    bool bSliceEnd = slice->getSliceCurEndCUAddr() > (cu->getAddr()) * cu->getPic()->getNumPartInCU() + absPartIdx &&
        slice->getSliceCurEndCUAddr() < (cu->getAddr()) * cu->getPic()->getNumPartInCU() + absPartIdx + (cu->getPic()->getNumPartInCU() >> (depth << 1));

    if (!bSliceEnd && (rpelx < slice->getSPS()->getPicWidthInLumaSamples()) && (bpely < slice->getSPS()->getPicHeightInLumaSamples()))
    {
        uint32_t absPartIdxInRaster = g_zscanToRaster[absPartIdx];
        uint32_t srcBlkWidth = outPic->getNumPartInWidth() >> (srcDepth);
        uint32_t blkWidth    = outPic->getNumPartInWidth() >> (depth);
        uint32_t partIdxX = ((absPartIdxInRaster % outPic->getNumPartInWidth()) % srcBlkWidth) / blkWidth;
        uint32_t partIdxY = ((absPartIdxInRaster / outPic->getNumPartInWidth()) % srcBlkWidth) / blkWidth;
        uint32_t partIdx = partIdxY * (srcBlkWidth / blkWidth) + partIdxX;
        m_bestRecoYuv[srcDepth]->copyToPicYuv(outPic->getPicYuvRec(), cuAddr, absPartIdx, depth - srcDepth, partIdx);
    }
    else
    {
        uint32_t qNumParts = (cu->getPic()->getNumPartInCU() >> (depth << 1)) >> 2;

        for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++, absPartIdx += qNumParts)
        {
            uint32_t subCULPelX = lpelx + (g_maxCUWidth >> (depth + 1)) * (partUnitIdx &  1);
            uint32_t subCUTPelY = tpely + (g_maxCUHeight >> (depth + 1)) * (partUnitIdx >> 1);

            bool bInSlice = cu->getAddr() * cu->getPic()->getNumPartInCU() + absPartIdx < slice->getSliceCurEndCUAddr();
            if (bInSlice && (subCULPelX < slice->getSPS()->getPicWidthInLumaSamples()) && (subCUTPelY < slice->getSPS()->getPicHeightInLumaSamples()))
            {
                xCopyYuv2Pic(outPic, cuAddr, absPartIdx, depth + 1, srcDepth, cu, subCULPelX, subCUTPelY); // Copy Yuv data to picture Yuv
            }
        }
    }
}

void TEncCu::xCopyYuv2Tmp(uint32_t partUnitIdx, uint32_t nextDepth)
{
    m_bestRecoYuv[nextDepth]->copyToPartYuv(m_tmpRecoYuv[nextDepth - 1], partUnitIdx);
}

/** Function for filling the PCM buffer of a CU using its original sample array
 * \param cu pointer to current CU
 * \param fencYuv pointer to original sample array
 * \returns void
 */
void TEncCu::xFillPCMBuffer(TComDataCU* cu, TComYuv* fencYuv)
{
    uint32_t width = cu->getWidth(0);
    uint32_t height = cu->getHeight(0);

    Pel* srcY = fencYuv->getLumaAddr(0, width);
    Pel* dstY = cu->getPCMSampleY();
    uint32_t srcStride = fencYuv->getStride();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            dstY[x] = srcY[x];
        }

        dstY += width;
        srcY += srcStride;
    }

    Pel* srcCb = fencYuv->getCbAddr();
    Pel* srcCr = fencYuv->getCrAddr();

    Pel* dstCb = cu->getPCMSampleCb();
    Pel* dstCr = cu->getPCMSampleCr();

    uint32_t srcStrideC = fencYuv->getCStride();
    uint32_t heightC = height >> 1;
    uint32_t widthC = width >> 1;

    for (int y = 0; y < heightC; y++)
    {
        for (int x = 0; x < widthC; x++)
        {
            dstCb[x] = srcCb[x];
            dstCr[x] = srcCr[x];
        }

        dstCb += widthC;
        dstCr += widthC;
        srcCb += srcStrideC;
        srcCr += srcStrideC;
    }
}

//! \}
