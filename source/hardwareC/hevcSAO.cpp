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

/**
 \file     hevcSAO.cpp
 \brief       estimation part of sample adaptive offset class
 */
#include "hevcSAO.h"
#include <string.h>
#include <cstdlib>
#include <stdio.h>
#include <math.h>
#include "rk_define.h"

using namespace x265;

//! \ingroup TLibEncoder
//! \{
#define  HWC_SAO_RDO_MODULE 1
#define  SAO_TEMP 1


#define  HWC_SAO_RDO_CTU_ROW_LEVEL 0
#define  HWC_SAO_RDO_CTU_LEVEL 0
#define  HWC_SAO_RDO_GLOBAL_PAPRAM 0

hevcSAO::hevcSAO()
    : m_entropyCoder(NULL)
    , m_rdSbacCoders(NULL)
    , m_rdGoOnSbacCoder(NULL)
    , m_binCoderCABAC(NULL)
    , m_count(NULL)
    , m_offset(NULL)
    , m_offsetOrg(NULL)
    , m_countPreDblk(NULL)
    , m_offsetOrgPreDblk(NULL)
    , m_rate(NULL)
    , m_dist(NULL)
    , m_cost(NULL)
    , m_costPartBest(NULL)
    , m_distOrg(NULL)
    , m_typePartBest(NULL)
    , lumaLambda(0.)
    , chromaLambda(0.)
    , depth(0)
{
    m_depthSaoRate[0][0] = 0;
    m_depthSaoRate[0][1] = 0;
    m_depthSaoRate[0][2] = 0;
    m_depthSaoRate[0][3] = 0;
    m_depthSaoRate[1][0] = 0;
    m_depthSaoRate[1][1] = 0;
    m_depthSaoRate[1][2] = 0;
    m_depthSaoRate[1][3] = 0;

    m_saoBitIncreaseY = X265_MAX(X265_DEPTH - 10, 0);
    m_saoBitIncreaseC = X265_MAX(X265_DEPTH - 10, 0);
    m_offsetThY = 1 << X265_MIN(X265_DEPTH - 5, 5);
    m_offsetThC = 1 << X265_MIN(X265_DEPTH - 5, 5);

	creatHWC();
#if SAO_LOG_IN_X265
	m_fpSaoLogX265 = fopen(PATH_NAME("SAO_LOG_X265.txt"), "w+");
	if (m_fpSaoLogX265 == NULL)
		printf("open sao_log_x265 file failed!\n");
#endif

#if SAO_LOG_IN_HWC
	m_fpSaoLogHWC = fopen(PATH_NAME("SAO_LOG_HWC.txt"), "w+");
	if (m_fpSaoLogHWC == NULL)
		printf("open SAO_LOG_HWC file failed!\n");
#endif
}

hevcSAO::~hevcSAO()
{
	destoryHWC();
#if SAO_LOG_IN_X265
	fclose(m_fpSaoLogX265); m_fpSaoLogX265 = NULL;
#endif
#if SAO_LOG_IN_HWC
	fclose(m_fpSaoLogHWC);  m_fpSaoLogHWC = NULL;
#endif
}

// ====================================================================================================================
// Static or Global values
// ====================================================================================================================

// ====================================================================================================================
// Constants
// ====================================================================================================================

// ====================================================================================================================
// Tables
// ====================================================================================================================

#if HIGH_BIT_DEPTH
inline double xRoundIbdi2(double x)
{
    return ((x) > 0) ? (int)(((int)(x) + (1 << (X265_DEPTH - 8 - 1))) / (1 << (X265_DEPTH - 8))) : ((int)(((int)(x) - (1 << (X265_DEPTH - 8 - 1))) / (1 << (X265_DEPTH - 8))));
}

#endif

/** rounding with IBDI
 * \param  x
 */
inline double xRoundIbdi(double x)
{
#if HIGH_BIT_DEPTH
    return X265_DEPTH > 8 ? xRoundIbdi2(x) : ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)));
#else
    return (x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5));
#endif
}

/** process SAO for one partition
 * \param  *psQTPart, partIdx, lambda
 */
void hevcSAO::rdoSaoOnePart(SAOQTPart *psQTPart, int partIdx, double lambda, int yCbCr)
{
    int typeIdx;
    int numTotalType = MAX_NUM_SAO_TYPE;
    SAOQTPart* onePart = &(psQTPart[partIdx]);

    int64_t estDist;
    int classIdx;
    int shift = 2 * DISTORTION_PRECISION_ADJUSTMENT(X265_DEPTH - 8);

    m_distOrg[partIdx] =  0;

    double bestRDCostTableBo = MAX_DOUBLE;
    int    bestClassTableBo  = 0;
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];
    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];

    int allowMergeLeft;
    int allowMergeUp;
    SaoLcuParam saoLcuParamRdo;

    for (typeIdx = -1; typeIdx < numTotalType; typeIdx++)
    {
        m_rdGoOnSbacCoder->load(m_rdSbacCoders[onePart->partLevel][CI_CURR_BEST]);
        m_rdGoOnSbacCoder->resetBits();

        estDist = 0;

        if (typeIdx == -1)
        {
            for (int ry = onePart->startCUY; ry <= onePart->endCUY; ry++)
            {
                for (int rx = onePart->startCUX; rx <= onePart->endCUX; rx++)
                {
                    // get bits for iTypeIdx = -1
                    allowMergeLeft = 1;
                    allowMergeUp   = 1;

                    // reset
                    resetSaoUnit(&saoLcuParamRdo);

                    // set merge flag
                    saoLcuParamRdo.mergeUpFlag   = 1;
                    saoLcuParamRdo.mergeLeftFlag = 1;

                    if (ry == onePart->startCUY)
                    {
                        saoLcuParamRdo.mergeUpFlag = 0;
                    }

                    if (rx == onePart->startCUX)
                    {
                        saoLcuParamRdo.mergeLeftFlag = 0;
                    }

                    m_entropyCoder->encodeSaoUnitInterleaving(yCbCr, 1, rx, ry,  &saoLcuParamRdo, 1,  1,  allowMergeLeft, allowMergeUp);
                }
            }
        }

        if (typeIdx >= 0)
        {
            estDist = estSaoTypeDist(partIdx, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
            if (typeIdx == SAO_BO)
            {
                // Estimate Best Position
                double currentRDCost = 0.0;

                for (int i = 0; i < SAO_MAX_BO_CLASSES - SAO_BO_LEN + 1; i++)
                {
                    currentRDCost = 0.0;
                    for (uint32_t uj = i; uj < i + SAO_BO_LEN; uj++)
                    {
                        currentRDCost += currentRdCostTableBo[uj];
                    }

                    if (currentRDCost < bestRDCostTableBo)
                    {
                        bestRDCostTableBo = currentRDCost;
                        bestClassTableBo  = i;
                    }
                }

                // Re code all Offsets
                // Code Center
                for (classIdx = bestClassTableBo; classIdx < bestClassTableBo + SAO_BO_LEN; classIdx++)
                {
                    estDist += currentDistortionTableBo[classIdx];
                }
            }

            for (int ry = onePart->startCUY; ry <= onePart->endCUY; ry++)
            {
                for (int rx = onePart->startCUX; rx <= onePart->endCUX; rx++)
                {
                    // get bits for iTypeIdx = -1
                    allowMergeLeft = 1;
                    allowMergeUp   = 1;

                    // reset
                    resetSaoUnit(&saoLcuParamRdo);

                    // set merge flag
                    saoLcuParamRdo.mergeUpFlag   = 1;
                    saoLcuParamRdo.mergeLeftFlag = 1;

                    if (ry == onePart->startCUY)
                    {
                        saoLcuParamRdo.mergeUpFlag = 0;
                    }

                    if (rx == onePart->startCUX)
                    {
                        saoLcuParamRdo.mergeLeftFlag = 0;
                    }

                    // set type and offsets
                    saoLcuParamRdo.typeIdx = typeIdx;
                    saoLcuParamRdo.subTypeIdx = (typeIdx == SAO_BO) ? bestClassTableBo : 0;
                    saoLcuParamRdo.length = m_numClass[typeIdx];
                    for (classIdx = 0; classIdx < saoLcuParamRdo.length; classIdx++)
                    {
                        saoLcuParamRdo.offset[classIdx] = (int)m_offset[partIdx][typeIdx][classIdx + saoLcuParamRdo.subTypeIdx + 1];
                    }

                    m_entropyCoder->encodeSaoUnitInterleaving(yCbCr, 1, rx, ry,  &saoLcuParamRdo, 1,  1,  allowMergeLeft, allowMergeUp);
                }
            }

            m_dist[partIdx][typeIdx] = estDist;
            m_rate[partIdx][typeIdx] = m_entropyCoder->getNumberOfWrittenBits();

            m_cost[partIdx][typeIdx] = (double)((double)m_dist[partIdx][typeIdx] + lambda * (double)m_rate[partIdx][typeIdx]);

            if (m_cost[partIdx][typeIdx] < m_costPartBest[partIdx])
            {
                m_distOrg[partIdx] = 0;
                m_costPartBest[partIdx] = m_cost[partIdx][typeIdx];
                m_typePartBest[partIdx] = typeIdx;
                m_rdGoOnSbacCoder->store(m_rdSbacCoders[onePart->partLevel][CI_TEMP_BEST]);
            }
        }
        else
        {
            if (m_distOrg[partIdx] < m_costPartBest[partIdx])
            {
                m_costPartBest[partIdx] = (double)m_distOrg[partIdx] + m_entropyCoder->getNumberOfWrittenBits() * lambda;
                m_typePartBest[partIdx] = -1;
                m_rdGoOnSbacCoder->store(m_rdSbacCoders[onePart->partLevel][CI_TEMP_BEST]);
            }
        }
    }

    onePart->bProcessed = true;
    onePart->bSplit    = false;
    onePart->minDist   =       m_typePartBest[partIdx] >= 0 ? m_dist[partIdx][m_typePartBest[partIdx]] : m_distOrg[partIdx];
    onePart->minRate   = (int)(m_typePartBest[partIdx] >= 0 ? m_rate[partIdx][m_typePartBest[partIdx]] : 0);
    onePart->minCost   = onePart->minDist + lambda * onePart->minRate;
    onePart->bestType  = m_typePartBest[partIdx];
    if (onePart->bestType != -1)
    {
        // pOnePart->bEnableFlag =  1;
        onePart->length = m_numClass[onePart->bestType];
        int minIndex = 0;
        if (onePart->bestType == SAO_BO)
        {
            onePart->subTypeIdx = bestClassTableBo;
            minIndex = onePart->subTypeIdx;
        }
        for (int i = 0; i < onePart->length; i++)
        {
            onePart->offset[i] = (int)m_offset[partIdx][onePart->bestType][minIndex + i + 1];
        }
    }
    else
    {
        // pOnePart->bEnableFlag = 0;
        onePart->length     = 0;
    }
}

/** Run partition tree disable
 */
void hevcSAO::disablePartTree(SAOQTPart *psQTPart, int partIdx)
{
    SAOQTPart* pOnePart = &(psQTPart[partIdx]);

    pOnePart->bSplit     = false;
    pOnePart->length     =  0;
    pOnePart->bestType   = -1;

    if (pOnePart->partLevel < m_maxSplitLevel)
    {
        for (int i = 0; i < NUM_DOWN_PART; i++)
        {
            disablePartTree(psQTPart, pOnePart->downPartsIdx[i]);
        }
    }
}

/** Run quadtree decision function
 * \param  partIdx, pcPicOrg, pcPicDec, pcPicRest, &costFinal
 */
void hevcSAO::runQuadTreeDecision(SAOQTPart *qtPart, int partIdx, double &costFinal, int maxLevel, double lambda, int yCbCr)
{
    SAOQTPart* onePart = &(qtPart[partIdx]);

    uint32_t nextDepth = onePart->partLevel + 1;

    if (partIdx == 0)
    {
        costFinal = 0;
    }

    // SAO for this part
    if (!onePart->bProcessed)
    {
        rdoSaoOnePart(qtPart, partIdx, lambda, yCbCr);
    }

    // SAO for sub 4 parts
    if (onePart->partLevel < maxLevel)
    {
        double costNotSplit = lambda + onePart->minCost;
        double costSplit    = lambda;

        for (int i = 0; i < NUM_DOWN_PART; i++)
        {
            if (0 == i) //initialize RD with previous depth buffer
            {
                m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[onePart->partLevel][CI_CURR_BEST]);
            }
            else
            {
                m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[nextDepth][CI_NEXT_BEST]);
            }
            runQuadTreeDecision(qtPart, onePart->downPartsIdx[i], costFinal, maxLevel, lambda, yCbCr);
            costSplit += costFinal;
            m_rdSbacCoders[nextDepth][CI_NEXT_BEST]->load(m_rdSbacCoders[nextDepth][CI_TEMP_BEST]);
        }

        if (costSplit < costNotSplit)
        {
            costFinal = costSplit;
            onePart->bSplit   = true;
            onePart->length   =  0;
            onePart->bestType = -1;
            m_rdSbacCoders[onePart->partLevel][CI_NEXT_BEST]->load(m_rdSbacCoders[nextDepth][CI_NEXT_BEST]);
        }
        else
        {
            costFinal = costNotSplit;
            onePart->bSplit = false;
            for (int i = 0; i < NUM_DOWN_PART; i++)
            {
                disablePartTree(qtPart, onePart->downPartsIdx[i]);
            }

            m_rdSbacCoders[onePart->partLevel][CI_NEXT_BEST]->load(m_rdSbacCoders[onePart->partLevel][CI_TEMP_BEST]);
        }
    }
    else
    {
        costFinal = onePart->minCost;
    }
}

/** delete allocated memory of hevcSAO class.
 */
void hevcSAO::destroyEncBuffer()
{
    for (int i = 0; i < m_numTotalParts; i++)
    {
        for (int j = 0; j < MAX_NUM_SAO_TYPE; j++)
        {
            delete [] m_count[i][j];
            delete [] m_offset[i][j];
            delete [] m_offsetOrg[i][j];
        }

        delete [] m_rate[i];
        delete [] m_dist[i];
        delete [] m_cost[i];
        delete [] m_count[i];
        delete [] m_offset[i];
        delete [] m_offsetOrg[i];
    }

    delete [] m_distOrg;
    m_distOrg = NULL;
    delete [] m_costPartBest;
    m_costPartBest = NULL;
    delete [] m_typePartBest;
    m_typePartBest = NULL;
    delete [] m_rate;
    m_rate = NULL;
    delete [] m_dist;
    m_dist = NULL;
    delete [] m_cost;
    m_cost = NULL;
    delete [] m_count;
    m_count = NULL;
    delete [] m_offset;
    m_offset = NULL;
    delete [] m_offsetOrg;
    m_offsetOrg = NULL;

    delete[] m_countPreDblk;
    m_countPreDblk = NULL;

    delete[] m_offsetOrgPreDblk;
    m_offsetOrgPreDblk = NULL;

    int maxDepth = 4;
    for (int d = 0; d < maxDepth + 1; d++)
    {
        for (int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx++)
        {
            delete m_rdSbacCoders[d][iCIIdx];
            delete m_binCoderCABAC[d][iCIIdx];
        }
    }

    for (int d = 0; d < maxDepth + 1; d++)
    {
        delete [] m_rdSbacCoders[d];
        delete [] m_binCoderCABAC[d];
    }

    delete [] m_rdSbacCoders;
    delete [] m_binCoderCABAC;
}

/** create Encoder Buffer for SAO
 * \param
 */
void hevcSAO::createEncBuffer()
{
    m_distOrg = new int64_t[m_numTotalParts];
    m_costPartBest = new double[m_numTotalParts];
    m_typePartBest = new int[m_numTotalParts];

    m_rate = new int64_t*[m_numTotalParts];
    m_dist = new int64_t*[m_numTotalParts];
    m_cost = new double*[m_numTotalParts];

    m_count  = new int64_t * *[m_numTotalParts];
    m_offset = new int64_t * *[m_numTotalParts];
    m_offsetOrg = new int64_t * *[m_numTotalParts];

    for (int i = 0; i < m_numTotalParts; i++)
    {
        m_rate[i] = new int64_t[MAX_NUM_SAO_TYPE];
        m_dist[i] = new int64_t[MAX_NUM_SAO_TYPE];
        m_cost[i] = new double[MAX_NUM_SAO_TYPE];

        m_count[i] = new int64_t *[MAX_NUM_SAO_TYPE];
        m_offset[i] = new int64_t *[MAX_NUM_SAO_TYPE];
        m_offsetOrg[i] = new int64_t *[MAX_NUM_SAO_TYPE];

        for (int j = 0; j < MAX_NUM_SAO_TYPE; j++)
        {
            m_count[i][j]   = new int64_t[MAX_NUM_SAO_CLASS];
            m_offset[i][j]   = new int64_t[MAX_NUM_SAO_CLASS];
            m_offsetOrg[i][j] = new int64_t[MAX_NUM_SAO_CLASS];
        }
    }

    int numLcu = m_numCuInWidth * m_numCuInHeight;
    if (m_countPreDblk == NULL)
    {
        assert(m_offsetOrgPreDblk == NULL);

        m_countPreDblk  = new int64_t[numLcu][3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
        m_offsetOrgPreDblk = new int64_t[numLcu][3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
    }

    int maxDepth = 4;
    m_rdSbacCoders = new TEncSbac * *[maxDepth + 1];
    m_binCoderCABAC = new TEncBinCABAC * *[maxDepth + 1];

    for (int d = 0; d < maxDepth + 1; d++)
    {
        m_rdSbacCoders[d] = new TEncSbac*[CI_NUM];
        m_binCoderCABAC[d] = new TEncBinCABAC*[CI_NUM];
        for (int ciIdx = 0; ciIdx < CI_NUM; ciIdx++)
        {
            m_rdSbacCoders[d][ciIdx] = new TEncSbac;
            m_binCoderCABAC[d][ciIdx] = new TEncBinCABAC(true);
            m_rdSbacCoders[d][ciIdx]->init(m_binCoderCABAC[d][ciIdx]);
        }
    }
}

/** Start SAO encoder
 * \param pic, entropyCoder, rdSbacCoder, rdGoOnSbacCoder
 */
void hevcSAO::startSaoEnc(TComPic* pic, TEncEntropy* entropyCoder, TEncSbac* rdGoOnSbacCoder)
{
    m_pic = pic;
    m_entropyCoder = entropyCoder;

    m_rdGoOnSbacCoder = rdGoOnSbacCoder;
    m_entropyCoder->setEntropyCoder(m_rdGoOnSbacCoder, pic->getSlice());
    m_entropyCoder->resetEntropy();
    m_entropyCoder->resetBits();

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[0][CI_NEXT_BEST]);
    m_rdSbacCoders[0][CI_CURR_BEST]->load(m_rdSbacCoders[0][CI_NEXT_BEST]);
}

/** End SAO encoder
 */
void hevcSAO::endSaoEnc()
{
    m_pic = NULL;
    m_entropyCoder = NULL;
}

inline int xSign(int x)
{
    return (x >> 31) | ((int)((((uint32_t)-x)) >> 31));
}

/** Calculate SAO statistics for non-cross-slice or non-cross-tile processing
 * \param  recStart to-be-filtered block buffer pointer
 * \param  orgStart original block buffer pointer
 * \param  stride picture buffer stride
 * \param  ppStat statistics buffer
 * \param  counts counter buffer
 * \param  width block width
 * \param  height block height
 * \param  bBorderAvail availabilities of block border pixels
 */

/************************************************************************/
/*                       X265 DEBUG START                               */
/************************************************************************/

void hevcSAO::creatHWC()
{
	m_infoFromX265 = new InfoFromX265;
	m_infoForSAO = new InfoForSAO;
	m_saoRdoParam = new SaoRdoParam;
	
	m_saoParam = new SaoParam;
	m_saoParam->saoLcuParam[0] = new SaoLcuParam[MAX_NUM_HOR_CTU*MAX_NUM_VER_CTU];
	m_saoParam->saoLcuParam[1] = new SaoLcuParam[MAX_NUM_HOR_CTU*MAX_NUM_VER_CTU];
	m_saoParam->saoLcuParam[2] = new SaoLcuParam[MAX_NUM_HOR_CTU*MAX_NUM_VER_CTU];

		

	//just for debug
	m_saoParam->saoPart[0] = new SAOQTPart[341];
	m_saoParam->saoPart[1] = new SAOQTPart[341];
	m_saoParam->saoPart[2] = new SAOQTPart[341];
}

void hevcSAO::destoryHWC()
{
	delete m_infoForSAO;						m_infoForSAO = NULL;
	delete m_infoFromX265;						m_infoFromX265 = NULL;
	delete m_saoRdoParam;						m_saoRdoParam = NULL;

	delete[] m_saoParam->saoLcuParam[0];		m_saoParam->saoLcuParam[0] = NULL;
	delete[] m_saoParam->saoLcuParam[1];		m_saoParam->saoLcuParam[1] = NULL;
	delete[] m_saoParam->saoLcuParam[2];		m_saoParam->saoLcuParam[2] = NULL;
	
	//just for debug
	delete[] m_saoParam->saoPart[0]; 			m_saoParam->saoPart[0] = NULL;
	delete[] m_saoParam->saoPart[1]; 			m_saoParam->saoPart[1] = NULL;
	delete[] m_saoParam->saoPart[2];			m_saoParam->saoPart[2] = NULL;

	delete[] m_saoParam;						m_saoParam = NULL;
}

void hevcSAO::getSaoParamFromX265(SAOParam* saoParam)
{
	m_saoParam->bSaoFlag[0] = saoParam->bSaoFlag[0];
	m_saoParam->bSaoFlag[1] = saoParam->bSaoFlag[1];
	m_saoParam->maxSplitLevel = saoParam->maxSplitLevel;
	m_saoParam->oneUnitFlag[0] = saoParam->oneUnitFlag[0];
	m_saoParam->oneUnitFlag[1] = saoParam->oneUnitFlag[1];
	m_saoParam->oneUnitFlag[2] = saoParam->oneUnitFlag[2];
	m_saoParam->numCuInWidth = saoParam->numCuInWidth;
	m_saoParam->numCuInHeight = saoParam->numCuInHeight;


	for (int k = 0; k < saoParam->numCuInWidth*saoParam->numCuInHeight; k++)
	{
		copySaoUnit(&m_saoParam->saoLcuParam[0][k], &saoParam->saoLcuParam[0][k]);
		copySaoUnit(&m_saoParam->saoLcuParam[1][k], &saoParam->saoLcuParam[1][k]);
		copySaoUnit(&m_saoParam->saoLcuParam[2][k], &saoParam->saoLcuParam[2][k]);
	}

	//just for debug
	m_saoParam->maxSplitLevel = saoParam->maxSplitLevel;
	for (int k = 0; k < m_numCulPartsLevel[saoParam->maxSplitLevel];k++)
	{
		m_saoParam->saoPart[0][k] = saoParam->saoPart[0][k];
		m_saoParam->saoPart[1][k] = saoParam->saoPart[1][k];
		m_saoParam->saoPart[2][k] = saoParam->saoPart[2][k];
	}
	m_saoParam->oneUnitFlag[0] = saoParam->oneUnitFlag[0];
	m_saoParam->oneUnitFlag[1] = saoParam->oneUnitFlag[1];
	m_saoParam->oneUnitFlag[2] = saoParam->oneUnitFlag[2];
}

void hevcSAO::setSaoParamToX265(SAOParam * saoParam, int addr)
{
#if 0
	saoParam->bSaoFlag[0] = m_saoParam->bSaoFlag[0];
	saoParam->bSaoFlag[1] = m_saoParam->bSaoFlag[1];
	saoParam->maxSplitLevel = m_saoParam->maxSplitLevel;
	saoParam->oneUnitFlag[0] = m_saoParam->oneUnitFlag[0];
	saoParam->oneUnitFlag[1] = m_saoParam->oneUnitFlag[1];
	saoParam->oneUnitFlag[2] = m_saoParam->oneUnitFlag[2];
	saoParam->numCuInWidth = m_saoParam->numCuInWidth;
	saoParam->numCuInHeight = m_saoParam->numCuInHeight;
#endif
	SaoCtuParam* saoCtuParam = m_infoForSAO->saoCtuParam;
	for(int k=0;k<3;k++)
	{
		SaoLcuParam* saoLcuParam = &saoParam->saoLcuParam[k][addr];
		saoLcuParam->mergeLeftFlag = saoCtuParam[k].mergeLeftFlag;
		saoLcuParam->mergeUpFlag = saoCtuParam[k].mergeUpFlag;
		saoLcuParam->typeIdx = saoCtuParam[k].typeIdx;
	    saoLcuParam->length        = 4;

		if (0 <= saoCtuParam[k].typeIdx <= 3) // EO
		{
			saoLcuParam->subTypeIdx = saoCtuParam[k].bandIdx;
		}
		else // BO, noSAO
		{
			saoLcuParam->subTypeIdx = 0;
		}
		
	    for (int i = 0; i < 4; i++)
	    {
			saoLcuParam->offset[i] = saoCtuParam[k].offset[i];
	    }
	}

#if 0
	copySaoUnit(&saoParam->saoLcuParam[0][addr], &m_saoCtuParam[0]);
	copySaoUnit(&saoParam->saoLcuParam[1][addr], &m_saoCtuParam[1]);
	copySaoUnit(&saoParam->saoLcuParam[2][addr], &m_saoCtuParam[2]);
#endif
#if 0
	//just for debug
	saoParam->maxSplitLevel = m_saoParam->maxSplitLevel;
	for (int k = 0; k < m_numCulPartsLevel[saoParam->maxSplitLevel];k++)
	{
		saoParam->saoPart[0][k] = m_saoParam->saoPart[0][k];
		saoParam->saoPart[1][k] = m_saoParam->saoPart[1][k];
		saoParam->saoPart[2][k] = m_saoParam->saoPart[2][k];
	}
	saoParam->oneUnitFlag[0] = m_saoParam->oneUnitFlag[0];
	saoParam->oneUnitFlag[1] = m_saoParam->oneUnitFlag[1];
	saoParam->oneUnitFlag[2] = m_saoParam->oneUnitFlag[2];
#endif	
}
void hevcSAO::getInfoFromX265_0(int colIdx, int rowIdx, double lumaLambda, double chromaLambda, int ctuWidth,
	uint32_t lPelx, uint32_t tPely, int picWidth, int picHeight, uint8_t* inRecPixelY, uint8_t* orgPixelY, int lumaStride,
	uint8_t* inRecPixelU, uint8_t* orgPixelU, uint8_t* inRecPixelV, uint8_t* orgPixelV, int chromaStride,
	uint8_t* inRecPixelLeftY, uint8_t* inRecPixelLeftU, uint8_t* inRecPixelLeftV, uint8_t* inRecPixelUpY, uint8_t* inRecPixelUpU, uint8_t* inRecPixelUpV,
	double compDist[3], SaoLcuParam* saoLcuParamY, SaoLcuParam* saoLcuParamU, SaoLcuParam* saoLcuParamV)
{
	uint8_t k;
	// ===============  get input from X265 ==================
	m_infoFromX265->ctuPosY = (uint8_t)rowIdx;
	m_infoFromX265->ctuPosX = (uint8_t)colIdx;
	m_infoFromX265->lumaLambda = lumaLambda;
	m_infoFromX265->chromaLambda = chromaLambda;
	m_infoFromX265->size = (uint8_t)ctuWidth;
	uint8_t size = m_infoFromX265->size;

	SaoCtuParam* saoCtuParam = m_infoFromX265->saoCtuParam[colIdx];
	
	uint8_t validWidth = size;
	uint8_t validHeight = size;
	uint32_t rPelx = lPelx + validWidth;
	uint32_t bPely = tPely + validHeight;
	rPelx = rPelx > picWidth ? picWidth : rPelx;
	bPely = bPely > picHeight ? picHeight : bPely;
	validWidth = rPelx - lPelx;
	validHeight = bPely - tPely;
	m_infoFromX265->validWidth = validWidth;
	m_infoFromX265->validHeight = validHeight;
	m_infoFromX265->bLastCol = (rPelx == picWidth);
	m_infoFromX265->bLastRow = (bPely == picHeight);


	// Y component
	for (k = 0; k < size; k++)
	{
		memcpy(&m_infoFromX265->inRecPixelY[k*size], &inRecPixelY[k*lumaStride], size*sizeof(uint8_t));
		memcpy(&m_infoFromX265->orgPixelY[k*size], &orgPixelY[k*lumaStride], size*sizeof(uint8_t));
		
		// last col in left CTU 
		if (m_infoFromX265->ctuPosX>0)
		{
			m_infoFromX265->inRecPixelLeftY[k] = inRecPixelLeftY[size-1+k*lumaStride];
		}	
	}
	// U, V components
	for (k = 0; k < (size / 2); k++)
	{
		memcpy(&m_infoFromX265->inRecPixelU[k*size / 2], &inRecPixelU[k*chromaStride], size / 2 * sizeof(uint8_t));
		memcpy(&m_infoFromX265->orgPixelU[k*size / 2], &orgPixelU[k*chromaStride], size / 2 * sizeof(uint8_t));
		memcpy(&m_infoFromX265->inRecPixelV[k*size / 2], &inRecPixelV[k*chromaStride], size / 2 * sizeof(uint8_t));
		memcpy(&m_infoFromX265->orgPixelV[k*size / 2], &orgPixelV[k*chromaStride], size / 2 * sizeof(uint8_t));

		// last col in left CTU 
		if (m_infoFromX265->ctuPosX>0)
		{
			m_infoFromX265->inRecPixelLeftU[k] = inRecPixelLeftU[size/2 - 1 + k*chromaStride];
			m_infoFromX265->inRecPixelLeftV[k] = inRecPixelLeftV[size/2 - 1 + k*chromaStride];
		}
	}

	
	if (m_infoFromX265->ctuPosY>0)
	{
		// last row in Up CTU
		memcpy(&m_infoFromX265->inRecPixelUpY[0], inRecPixelUpY + (size - 1)*lumaStride, size*sizeof(uint8_t));
		memcpy(&m_infoFromX265->inRecPixelUpU[0], inRecPixelUpU + (size / 2 - 1)*chromaStride, size / 2 * sizeof(uint8_t));
		memcpy(&m_infoFromX265->inRecPixelUpV[0], inRecPixelUpV + (size / 2 - 1)*chromaStride, size / 2 * sizeof(uint8_t));
	

		// extra needed pixels (right_top)
		if (!m_infoFromX265->bLastCol)
		{
			m_infoFromX265->inRecPixelExtraY[1] = inRecPixelY[-lumaStride + size];
			m_infoFromX265->inRecPixelExtraU[1] = inRecPixelU[-chromaStride + size / 2];
			m_infoFromX265->inRecPixelExtraV[1] = inRecPixelV[-chromaStride + size / 2];
		}

		// extra needed pixels (left_top)
		if (m_infoFromX265->ctuPosX>0)
		{
			m_infoFromX265->inRecPixelExtraY[0] = inRecPixelY[-lumaStride-1];
			m_infoFromX265->inRecPixelExtraU[0] = inRecPixelU[-chromaStride-1];
			m_infoFromX265->inRecPixelExtraV[0] = inRecPixelV[-chromaStride-1];
		}

	}

	// ===============  get debugInfo from X265 ==================
	
	m_infoFromX265->compDist[colIdx][0] = compDist[0]; //notMerge
	m_infoFromX265->compDist[colIdx][1] = compDist[1]; //mergeLeft
	m_infoFromX265->compDist[colIdx][2] = compDist[2]; //mergeUp

	// Y component
	saoCtuParam[0].typeIdx = saoLcuParamY->typeIdx;
	saoCtuParam[0].bandIdx = saoLcuParamY->subTypeIdx;
	saoCtuParam[0].mergeLeftFlag= saoLcuParamY->mergeLeftFlag;
	saoCtuParam[0].mergeUpFlag = saoLcuParamY->mergeUpFlag;
	for (int k = 0; k < 4;k++)
	{
		saoCtuParam[0].offset[k] = saoLcuParamY->offset[k];
	}

	// U component
	saoCtuParam[1].typeIdx = saoLcuParamU->typeIdx;
	saoCtuParam[1].bandIdx = saoLcuParamU->subTypeIdx;
	saoCtuParam[1].mergeLeftFlag = saoLcuParamU->mergeLeftFlag;
	saoCtuParam[1].mergeUpFlag = saoLcuParamU->mergeUpFlag;

	for (int k = 0; k < 4; k++)
	{
		saoCtuParam[1].offset[k] = saoLcuParamU->offset[k];
	}

	// V component
	saoCtuParam[2].typeIdx = saoLcuParamV->typeIdx;
	saoCtuParam[2].bandIdx = saoLcuParamV->subTypeIdx;
	saoCtuParam[2].mergeLeftFlag = saoLcuParamV->mergeLeftFlag;
	saoCtuParam[2].mergeUpFlag = saoLcuParamV->mergeUpFlag;

	for (int k = 0; k < 4; k++)
	{
		saoCtuParam[2].offset[k] = saoLcuParamV->offset[k];
	}

}

void hevcSAO::getFromX265()
{
	m_infoForSAO->ctuPosY = m_infoFromX265->ctuPosY;
	m_infoForSAO->ctuPosX = m_infoFromX265->ctuPosX;
	m_infoForSAO->lumaLambda = m_infoFromX265->lumaLambda;
	m_infoForSAO->chromaLambda = m_infoFromX265->chromaLambda;
	m_infoForSAO->size = m_infoFromX265->size;
	m_infoForSAO->validWidth = m_infoFromX265->validWidth;
	m_infoForSAO->validHeight = m_infoFromX265->validHeight;
	m_infoForSAO->bLastCol = m_infoFromX265->bLastCol;
	m_infoForSAO->bLastRow = m_infoFromX265->bLastRow;

	memcpy(&m_infoForSAO->inRecPixelY[0], &m_infoFromX265->inRecPixelY[0], m_infoForSAO->size*m_infoForSAO->size*sizeof(uint8_t));
	memcpy(&m_infoForSAO->inRecPixelU[0], &m_infoFromX265->inRecPixelU[0], m_infoForSAO->size*m_infoForSAO->size/4*sizeof(uint8_t));
	memcpy(&m_infoForSAO->inRecPixelV[0], &m_infoFromX265->inRecPixelV[0], m_infoForSAO->size*m_infoForSAO->size/4*sizeof(uint8_t));
	
	memcpy(&m_infoForSAO->orgPixelY[0], &m_infoFromX265->orgPixelY[0], m_infoForSAO->size*m_infoForSAO->size*sizeof(uint8_t));
	memcpy(&m_infoForSAO->orgPixelU[0], &m_infoFromX265->orgPixelU[0], m_infoForSAO->size*m_infoForSAO->size/4*sizeof(uint8_t));
	memcpy(&m_infoForSAO->orgPixelV[0], &m_infoFromX265->orgPixelV[0], m_infoForSAO->size*m_infoForSAO->size/4*sizeof(uint8_t));

}

/////////////////////////////////////////////////////////////



////////////////////////// Global Param operations ///////////////////////////
void hevcSAO::setRdoParam(int colIdx, int addr)
{
	uint8_t size = m_infoForSAO->size;
	uint8_t* inRecPixelY = m_infoForSAO->inRecPixelY;
	uint8_t* inRecPixelU = m_infoForSAO->inRecPixelU;
	uint8_t* inRecPixelV = m_infoForSAO->inRecPixelV;
	SaoCtuParam* saoCtuParam = m_infoForSAO->saoCtuParam;

	for (int j = 0; j < 3; j++) // components idx(Y, Cb, Cr)
	{
		for (int k = 0; k < 4; k++) // offset idx
		{
			m_saoRdoParam->frameLineParam[j][colIdx].offset[k] = saoCtuParam[j].offset[k];
			m_saoRdoParam->ctuParam[j].offset[k] = saoCtuParam[j].offset[k];
		}
		m_saoRdoParam->frameLineParam[j][colIdx].bandIdx = saoCtuParam[j].bandIdx;
		m_saoRdoParam->frameLineParam[j][colIdx].typeIdx = saoCtuParam[j].typeIdx;
		m_saoRdoParam->ctuParam[j].bandIdx = saoCtuParam[j].bandIdx;
		m_saoRdoParam->ctuParam[j].typeIdx = saoCtuParam[j].typeIdx;
	}

	//update frameLine buffer (store last Row of current CTU)
	//NOTE: temply doned after deblocking

	//update left CTU line buffer (store last Column of current CTU)
	for (uint8_t k = 0; k < size;k++)
	{
		m_saoRdoParam->leftCtuLineBuffer[0][k] = inRecPixelY[k*size+  (size   - 1)]; //Y
		m_saoRdoParam->leftCtuLineBuffer[1][k] = inRecPixelU[k*size/2+(size/2 - 1)]; //Cb
		m_saoRdoParam->leftCtuLineBuffer[2][k] = inRecPixelV[k*size/2+(size/2 - 1)]; //Cr
	}
}


/* input: inRecAfterDblk --- input rec pixel after deblocking, pointer to current CTU Row start addr
*		  	
*        
*/
void hevcSAO::setRdoParam(uint8_t* inRecAfterDblkY, uint8_t* inRecAfterDblkU, uint8_t*  inRecAfterDblkV)
{
	memcpy(m_saoRdoParam->frameLineBuffer[0], inRecAfterDblkY, m_picWidth*sizeof(uint8_t));
	memcpy(m_saoRdoParam->frameLineBuffer[1], inRecAfterDblkU, m_picWidth*sizeof(uint8_t));
	memcpy(m_saoRdoParam->frameLineBuffer[2], inRecAfterDblkV, m_picWidth*sizeof(uint8_t));
}
void hevcSAO::resetRdoParam(int colIdx)
{

}

void hevcSAO::resetSaoCtuParam(SaoCtuParam* saoCtuParam)
{
	saoCtuParam->typeIdx = -1; // default: noSAO
	saoCtuParam->bandIdx = 0;
	saoCtuParam->mergeLeftFlag = 0;
	saoCtuParam->mergeUpFlag = 0;
	for (int k = 0; k < 4; k++)
	{
		saoCtuParam->offset[k] = 0;
	}
}

void hevcSAO::copySaoCtuParam(SaoCtuParam* dst, SaoCtuParam* src)
{
	dst->typeIdx = src->typeIdx;
	dst->bandIdx = src->bandIdx;
	dst->mergeLeftFlag = src->mergeLeftFlag;
	dst->mergeUpFlag = src->mergeUpFlag;
	for(int k=0; k<4; k++)
	{
		dst->offset[k] = src->offset[k];
	}
}

void hevcSAO::copySaoCtuParam(SaoCtuParam* dst, SaoGlobalCtuParam* src)
{
	dst->typeIdx = src->typeIdx;
	dst->bandIdx = src->bandIdx;
	for(int k=0; k<4; k++)
	{
		dst->offset[k] = src->offset[k];
	}
}

#if 0
void hevcSAO::initGlobalParam()
{
	g_saoGlobalParam = new SaoGlobalParam;
	g_saoGlobalParam->cnt = -1; //check failure
}

void hevcSAO::resetGlobalParam()
{
	delete g_saoGlobalParam;
	g_saoGlobalParam = NULL;
}

void hevcSAO::setGlobalParam(int colIdx, int addr)
{
	// update count
	if (colIdx == 0)
	{
		g_saoGlobalParam->cnt = 0;
	}
	else
	{
		g_saoGlobalParam->cnt++;
	}

	int cnt = g_saoGlobalParam->cnt;
	
	// update Up and Left CTU param
	for (int j = 0; j < 3;j++)
	{
		for (int k = 0; k < 4; k++)
		{
			g_saoGlobalParam->frameLineParam[j][cnt].offset[k] = m_saoParam->saoLcuParam[j][addr].offset[k];
			g_saoGlobalParam->ctuParam[j].offset[k] = m_saoParam->saoLcuParam[j][addr].offset[k];
		}
		g_saoGlobalParam->frameLineParam[j][cnt].bandIdx = m_saoParam->saoLcuParam[j][addr].subTypeIdx;
		g_saoGlobalParam->ctuParam[j].bandIdx = m_saoParam->saoLcuParam[j][addr].subTypeIdx;
#if SAO_DBG
// 		assert(g_saoGlobalParam->frameLineParam[j][cnt].bandIdx != (-842150451));
// 		assert(g_saoGlobalParam->ctuParam[j].bandIdx != (-842150451));
#endif
	}	
}
#endif
// void hevcSAO::getGlobalParam(SaoLcuParam* saoLcuParamLeft, SaoLcuParam* saoLcuParamUp)
// {
// 	int cnt = g_saoGlobalParam->cnt;
// 
// 	// get Up and Left param
// 	for (int k = 0; k < 4; k++)
// 	{
// 		saoLcuParamLeft->offset[k] = g_saoGlobalParam->frameLineParam[cnt].offset[k];
// 		saoLcuParamUp->offset[k] = g_saoGlobalParam->ctuParam.offset[k];
// 	}
// 
// }

//////////////////////////// main process functions ///////////////////////////////
void hevcSAO::RDO(SaoParam* saoParam, int addr)
{
	uint8_t ctuPosX = m_infoForSAO->ctuPosX; // colIdx
	uint8_t ctuPosY = m_infoForSAO->ctuPosY; // rowIdx
	//double lumaLambda = m_infoForSAO->lumaLambda;
	//double lumaLambda = m_infoForSAO->chromaLambda;
	SaoCtuParam* saoCtuParam = &m_infoForSAO->saoCtuParam[0];

	int j, k;
	int compIdx = 0;
	double compDistortion[3];

	SaoCtuParam mergeCtuParam[3][2];

	uint32_t rate;
	double bestCost, mergeCost;
#if HWC_SAO_RDO_MODULE
	bool allowMergeLeft = (ctuPosX != 0);
	bool allowMergeUp = (ctuPosY != 0);
#endif

	// init
	compDistortion[0] = 0;
	compDistortion[1] = 0;
	compDistortion[2] = 0;
	m_rdGoOnSbacCoder->load(m_rdSbacCoders[0][CI_CURR_BEST]);
	if (allowMergeLeft)
	{
		m_entropyCoder->m_entropyCoderIf->codeSaoMerge(0);
	}
	if (allowMergeUp)
	{
		m_entropyCoder->m_entropyCoderIf->codeSaoMerge(0);
	}
	m_rdGoOnSbacCoder->store(m_rdSbacCoders[0][CI_TEMP_BEST]);
	// reset stats Y, Cb, Cr
	for (compIdx = 0; compIdx < 3; compIdx++)
	{
		for (j = 0; j < MAX_NUM_SAO_TYPE; j++)
		{
			for (k = 0; k < MAX_NUM_SAO_CLASS; k++)
			{
				m_offset[compIdx][j][k] = 0;
				if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
				{
					m_count[compIdx][j][k] = m_countPreDblk[addr][compIdx][j][k];
					m_offsetOrg[compIdx][j][k] = m_offsetOrgPreDblk[addr][compIdx][j][k];
				}
				else
				{
					m_count[compIdx][j][k] = 0;
					m_offsetOrg[compIdx][j][k] = 0;
				}
			}
		}

		calcCtuSaoStats(compIdx);

	}

	resetSaoCtuParam(&saoCtuParam[0]); // Y
	resetSaoCtuParam(&saoCtuParam[1]); // U
	resetSaoCtuParam(&saoCtuParam[2]); // V

	calcLumaDist(allowMergeLeft, allowMergeUp, m_infoForSAO->lumaLambda, &mergeCtuParam[0][0], &compDistortion[0]);
	calcChromaDist(allowMergeLeft, allowMergeUp, m_infoForSAO->chromaLambda, &mergeCtuParam[1][0], &mergeCtuParam[2][0], &compDistortion[0]);

	m_infoForSAO->compDist[0] = compDistortion[0];
	m_infoForSAO->compDist[1] = compDistortion[1];
	m_infoForSAO->compDist[2] = compDistortion[2];

	// Cost of new SAO_params
	m_rdGoOnSbacCoder->load(m_rdSbacCoders[0][CI_CURR_BEST]);
	m_rdGoOnSbacCoder->resetBits();
	if (allowMergeLeft)
	{
		m_entropyCoder->m_entropyCoderIf->codeSaoMerge(0);
	}
	if (allowMergeUp)
	{
		m_entropyCoder->m_entropyCoderIf->codeSaoMerge(0);
	}
	for (compIdx = 0; compIdx < 3; compIdx++)
	{
		m_entropyCoder->encodeSaoOffset(&saoCtuParam[compIdx], compIdx);
	}

	rate = m_entropyCoder->getNumberOfWrittenBits();
#if SAO_DBG
	assert(rate == m_infoFromX265->rate[m_infoFromX265->ctuPosX]);
#endif
	bestCost = compDistortion[0] + (double)rate;
	m_rdGoOnSbacCoder->store(m_rdSbacCoders[0][CI_TEMP_BEST]);
#if HWC_SAO_RDO_MODULE
	//getInfoFromHWC_1(bestCost);
	m_infoForSAO->notMergeBestCost = bestCost;
#if SAO_DBG
	assert(bestCost == m_infoFromX265->notMergeBestCost[m_infoFromX265->ctuPosX]);
	assert(m_infoForSAO->notMergeBestCost == m_infoFromX265->notMergeBestCost[m_infoFromX265->ctuPosX]);
#endif
#endif
	// Cost of Merge
	for (int mergeUp = 0; mergeUp < 2; ++mergeUp)
	{
		if ((allowMergeLeft && (mergeUp == 0)) || (allowMergeUp && (mergeUp == 1)))
		{
			m_rdGoOnSbacCoder->load(m_rdSbacCoders[0][CI_CURR_BEST]);
			m_rdGoOnSbacCoder->resetBits();
			if (allowMergeLeft)
			{
				m_entropyCoder->m_entropyCoderIf->codeSaoMerge(1 - mergeUp);
			}
			if (allowMergeUp && (mergeUp == 1))
			{
				m_entropyCoder->m_entropyCoderIf->codeSaoMerge(1);
			}

			rate = m_entropyCoder->getNumberOfWrittenBits();
			mergeCost = compDistortion[mergeUp + 1] + (double)rate;
#if HWC_SAO_RDO_MODULE
			//getInfoFromHWC_1(mergeCost, mergeUp);
			if (mergeUp == 0) //mergeLeft
			{
				m_infoForSAO->mergeLeftCost = mergeCost;
			}
			else //mergeUp
			{
				m_infoForSAO->mergeUpCost = mergeCost;
			}
#endif
			if (mergeCost < bestCost)
			{
				bestCost = mergeCost;
				m_rdGoOnSbacCoder->store(m_rdSbacCoders[0][CI_TEMP_BEST]);
				for (compIdx = 0; compIdx < 3; compIdx++)
				{
					mergeCtuParam[compIdx][mergeUp].mergeLeftFlag = 1 - mergeUp;
					mergeCtuParam[compIdx][mergeUp].mergeUpFlag = mergeUp;

					copySaoCtuParam(&saoCtuParam[compIdx], &mergeCtuParam[compIdx][mergeUp]);

				}
			}
		}
	}

	if (saoCtuParam[0].typeIdx == -1)
	{
		numNoSao[0]++;
	}
	if (saoCtuParam[1].typeIdx == -1)
	{
		numNoSao[1] += 2;
	}
	m_rdGoOnSbacCoder->load(m_rdSbacCoders[0][CI_TEMP_BEST]);
	m_rdGoOnSbacCoder->store(m_rdSbacCoders[0][CI_CURR_BEST]);

#if HWC_SAO_RDO_MODULE
	//getInfoFromHWC_1(lumaLambda, chromaLambda, &compDistortion[0],
	//	&saoParam->saoLcuParam[0][addr], &saoParam->saoLcuParam[1][addr], &saoParam->saoLcuParam[2][addr]);
#if SAO_DBG	
	assert(!(m_infoForSAO->compDist[0] != m_infoFromX265->compDist[m_infoFromX265->ctuPosX][0]));
	assert(!(m_infoForSAO->compDist[1] != m_infoFromX265->compDist[m_infoFromX265->ctuPosX][1]));
	assert(!(m_infoForSAO->compDist[2] != m_infoFromX265->compDist[m_infoFromX265->ctuPosX][2]));
#endif

#endif
}

void hevcSAO::printRecon(FILE* fp, uint8_t* rec, uint8_t width, uint8_t height)
{
	fprintf(fp, "-------------- print rec begin --------------\n");
	for (int k = 0; k < height;k++)
	{
		for (int j = 0; j < width; j++)
		{
			fprintf(fp, "%02x ", rec[k*width + j] & 0xff);
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "-------------- print rec end --------------\n\n");
}

void hevcSAO::copyGlobalSaoCtuParam(SaoGlobalCtuParam* dst, SaoGlobalCtuParam* src)
{

	dst->typeIdx = src->typeIdx;
	dst->bandIdx = src->bandIdx;
	for(uint8_t k=0;k<4;k++)
	{
		dst->offset[k] = src->offset[k];
	}
}

void hevcSAO::copyGlobalSaoCtuParam(SaoLcuParam* dst, SaoGlobalCtuParam* src)
{
	dst->typeIdx = src->typeIdx;
	dst->subTypeIdx = src->bandIdx;
	for(uint8_t k=0;k<4;k++)
	{
		dst->offset[k] = src->offset[k];
	}
	dst->length = 4; // number of class, fix to 4.
}


////////////////////////////////////////////////////////////////////////
#if HWC_SAO_RDO_CTU_ROW_LEVEL 
int hevcSAO::compareX265andHWC()
{
	int flag = 0;

	flag += (m_infoForSAO->rowIdx != m_infoFromX265->rowIdx); assert(!flag);
	flag += (m_infoForSAO->colIdx != m_infoFromX265->colIdx); assert(!flag);
	flag += (m_infoForSAO->lumaLambda != m_infoFromX265->lumaLambda); assert(!flag);
	flag += (m_infoForSAO->chromaLambda != m_infoFromX265->chromaLambda); assert(!flag);

	for (int k = 0; k < MAX_NUM_HOR_CTU;k++)
	{
		for (int j = 0; j < 3;j++)
		{
			flag += (m_infoForSAO->compDist[k][j] != m_infoFromX265->compDist[k][j]); assert(!flag);
			flag += (m_infoForSAO->saoType[k][j] != m_infoFromX265->saoType[k][j]); assert(!flag);
			flag += (m_infoForSAO->bestBandIdx[k][j] != m_infoFromX265->bestBandIdx[k][j]); assert(!flag);
			flag += (m_infoForSAO->mergeLeftFlag[k][j] != m_infoFromX265->mergeLeftFlag[k][j]); assert(!flag);
			flag += (m_infoForSAO->mergeUpFlag[k][j] != m_infoFromX265->mergeUpFlag[k][j]); assert(!flag);
			for (int i = 0; i < 4;i++)
			{
				flag += (m_infoForSAO->bestOffset[k][j][i] != m_infoFromX265->bestOffset[k][j][i]); assert(!flag);
			}		
		}
		flag += (m_infoForSAO->notMergeBestCost[k] != m_infoFromX265->notMergeBestCost[k]); assert(!flag);
		flag += (m_infoForSAO->mergeLeftCost[k] != m_infoFromX265->mergeLeftCost[k]); assert(!flag);
		flag += (m_infoForSAO->mergeUpCost[k] != m_infoFromX265->mergeUpCost[k]); assert(!flag);
	}
	return flag;
}
#endif


int hevcSAO::compareX265andHWC(int colIdx)
{
	int flag = 0;
	SaoCtuParam* hwcSaoCtuParam = m_infoForSAO->saoCtuParam;
	// compare input
	flag += (m_infoForSAO->lumaLambda != m_infoFromX265->lumaLambda); assert(!flag);
	flag += (m_infoForSAO->chromaLambda != m_infoFromX265->chromaLambda); assert(!flag);

	// compare debug info
	flag += (m_infoForSAO->notMergeBestCost != m_infoFromX265->notMergeBestCost[colIdx]); assert(!flag);		
	if (hwcSaoCtuParam[0].mergeLeftFlag)
	{
		flag += (m_infoForSAO->mergeLeftCost != m_infoFromX265->mergeLeftCost[colIdx]); assert(!flag);
	}
	if (hwcSaoCtuParam[0].mergeUpFlag)
	{
		flag += (m_infoForSAO->mergeUpCost != m_infoFromX265->mergeUpCost[colIdx]); assert(!flag);
	}
	
	// compare output
	for (int j = 0; j < 3; j++) // Y, Cb, Cr
	{
		flag += (m_infoForSAO->compDist[j] != m_infoFromX265->compDist[colIdx][j]); assert(!flag);
		flag += (hwcSaoCtuParam[j].typeIdx != m_infoFromX265->saoCtuParam[colIdx][j].typeIdx); assert(!flag);
		flag += (hwcSaoCtuParam[j].bandIdx != m_infoFromX265->saoCtuParam[colIdx][j].bandIdx); assert(!flag);
		flag += (hwcSaoCtuParam[j].mergeLeftFlag != m_infoFromX265->saoCtuParam[colIdx][j].mergeLeftFlag); assert(!flag);
		flag += (hwcSaoCtuParam[j].mergeUpFlag != m_infoFromX265->saoCtuParam[colIdx][j].mergeUpFlag); assert(!flag);
		for (int i = 0; i < 4; i++)
		{
			flag += (hwcSaoCtuParam[j].offset[i] != m_infoFromX265->saoCtuParam[colIdx][j].offset[i]); assert(!flag);
		}
	}
	

	
	return flag;
}


/************************************************************************/
/*                       X265 DEBUG END                                 */
/************************************************************************/
void hevcSAO::calcSaoStatsBlock(Pel* recStart, Pel* orgStart, int stride, int64_t** stats, int64_t** counts, uint32_t width, uint32_t height, bool* bBorderAvail, int yCbCr)
{
    int64_t *stat, *count;
    int classIdx, posShift, startX, endX, startY, endY, signLeft, signRight, signDown, signDown1;
    Pel *fenc, *pRec;
    uint32_t edgeType;
    int x, y;
    Pel *pTableBo = (yCbCr == 0) ? m_lumaTableBo : m_chromaTableBo;
    int32_t *tmp_swap;

    //--------- Band offset-----------//
    stat = stats[SAO_BO];
    count = counts[SAO_BO];
    fenc  = orgStart;
    pRec  = recStart;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            classIdx = pTableBo[pRec[x]];
            if (classIdx)
            {
                stat[classIdx] += (fenc[x] - pRec[x]);
                count[classIdx]++;
            }
        }

        fenc += stride;
        pRec += stride;
    }

    //---------- Edge offset 0--------------//
    stat = stats[SAO_EO_0];
    count = counts[SAO_EO_0];
    fenc  = orgStart;
    pRec  = recStart;

    startX = (bBorderAvail[SGU_L]) ? 0 : 1;
    endX   = (bBorderAvail[SGU_R]) ? width : (width - 1);
    for (y = 0; y < height; y++)
    {
        signLeft = xSign(pRec[startX] - pRec[startX - 1]);
        for (x = startX; x < endX; x++)
        {
            signRight = xSign(pRec[x] - pRec[x + 1]);
            edgeType = signRight + signLeft + 2;
            signLeft = -signRight;

            stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
            count[m_eoTable[edgeType]]++;
        }

        pRec  += stride;
        fenc += stride;
    }

    //---------- Edge offset 1--------------//
    stat = stats[SAO_EO_1];
    count = counts[SAO_EO_1];
    fenc  = orgStart;
    pRec  = recStart;

    startY = (bBorderAvail[SGU_T]) ? 0 : 1;
    endY   = (bBorderAvail[SGU_B]) ? height : height - 1;
    if (!bBorderAvail[SGU_T])
    {
        pRec += stride;
        fenc += stride;
    }

    for (x = 0; x < width; x++)
    {
        m_upBuff1[x] = xSign(pRec[x] - pRec[x - stride]);
    }

    for (y = startY; y < endY; y++)
    {
        for (x = 0; x < width; x++)
        {
            signDown = xSign(pRec[x] - pRec[x + stride]);
            edgeType = signDown + m_upBuff1[x] + 2;
            m_upBuff1[x] = -signDown;

            stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
            count[m_eoTable[edgeType]]++;
        }

        fenc += stride;
        pRec += stride;
    }

    //---------- Edge offset 2--------------//
    stat = stats[SAO_EO_2];
    count = counts[SAO_EO_2];
    fenc   = orgStart;
    pRec   = recStart;

    posShift = stride + 1;

    startX = (bBorderAvail[SGU_L]) ? 0 : 1;
    endX   = (bBorderAvail[SGU_R]) ? width : (width - 1);

    //prepare 2nd line upper sign
    pRec += stride;
    for (x = startX; x < endX + 1; x++)
    {
        m_upBuff1[x] = xSign(pRec[x] - pRec[x - posShift]);
    }

    //1st line
    pRec -= stride;
    if (bBorderAvail[SGU_TL])
    {
        x = 0;
        edgeType = xSign(pRec[x] - pRec[x - posShift]) - m_upBuff1[x + 1] + 2;
        stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
        count[m_eoTable[edgeType]]++;
    }
    if (bBorderAvail[SGU_T])
    {
        for (x = 1; x < endX; x++)
        {
            edgeType = xSign(pRec[x] - pRec[x - posShift]) - m_upBuff1[x + 1] + 2;
            stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
            count[m_eoTable[edgeType]]++;
        }
    }
    pRec += stride;
    fenc += stride;

    //middle lines
    for (y = 1; y < height - 1; y++)
    {
        for (x = startX; x < endX; x++)
        {
            signDown1 = xSign(pRec[x] - pRec[x + posShift]);
            edgeType = signDown1 + m_upBuff1[x] + 2;
            stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
            count[m_eoTable[edgeType]]++;

            m_upBufft[x + 1] = -signDown1;
        }

        m_upBufft[startX] = xSign(pRec[stride + startX] - pRec[startX - 1]);

        tmp_swap  = m_upBuff1;
        m_upBuff1 = m_upBufft;
        m_upBufft = tmp_swap;

        pRec  += stride;
        fenc  += stride;
    }

    //last line
    if (bBorderAvail[SGU_B])
    {
        for (x = startX; x < width - 1; x++)
        {
            edgeType = xSign(pRec[x] - pRec[x + posShift]) + m_upBuff1[x] + 2;
            stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
            count[m_eoTable[edgeType]]++;
        }
    }
    if (bBorderAvail[SGU_BR])
    {
        x = width - 1;
        edgeType = xSign(pRec[x] - pRec[x + posShift]) + m_upBuff1[x] + 2;
        stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
        count[m_eoTable[edgeType]]++;
    }

    //---------- Edge offset 3--------------//

    stat = stats[SAO_EO_3];
    count = counts[SAO_EO_3];
    fenc  = orgStart;
    pRec  = recStart;

    posShift = stride - 1;
    startX = (bBorderAvail[SGU_L]) ? 0 : 1;
    endX   = (bBorderAvail[SGU_R]) ? width : (width - 1);

    //prepare 2nd line upper sign
    pRec += stride;
    for (x = startX - 1; x < endX; x++)
    {
        m_upBuff1[x] = xSign(pRec[x] - pRec[x - posShift]);
    }

    //first line
    pRec -= stride;
    if (bBorderAvail[SGU_T])
    {
        for (x = startX; x < width - 1; x++)
        {
            edgeType = xSign(pRec[x] - pRec[x - posShift]) - m_upBuff1[x - 1] + 2;
            stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
            count[m_eoTable[edgeType]]++;
        }
    }
    if (bBorderAvail[SGU_TR])
    {
        x = width - 1;
        edgeType = xSign(pRec[x] - pRec[x - posShift]) - m_upBuff1[x - 1] + 2;
        stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
        count[m_eoTable[edgeType]]++;
    }
    pRec += stride;
    fenc += stride;

    //middle lines
    for (y = 1; y < height - 1; y++)
    {
        for (x = startX; x < endX; x++)
        {
            signDown1 = xSign(pRec[x] - pRec[x + posShift]);
            edgeType = signDown1 + m_upBuff1[x] + 2;

            stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
            count[m_eoTable[edgeType]]++;
            m_upBuff1[x - 1] = -signDown1;
        }

        m_upBuff1[endX - 1] = xSign(pRec[endX - 1 + stride] - pRec[endX]);

        pRec  += stride;
        fenc  += stride;
    }

    //last line
    if (bBorderAvail[SGU_BL])
    {
        x = 0;
        edgeType = xSign(pRec[x] - pRec[x + posShift]) + m_upBuff1[x] + 2;
        stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
        count[m_eoTable[edgeType]]++;
    }
    if (bBorderAvail[SGU_B])
    {
        for (x = 1; x < endX; x++)
        {
            edgeType = xSign(pRec[x] - pRec[x + posShift]) + m_upBuff1[x] + 2;
            stat[m_eoTable[edgeType]] += (fenc[x] - pRec[x]);
            count[m_eoTable[edgeType]]++;
        }
    }
}

/** Calculate SAO statistics for current LCU without non-crossing slice
 * \param  addr,  partIdx,  yCbCr
 */
void hevcSAO::calcCtuSaoStats(int yCbCr)
{
	uint8_t size = m_infoForSAO->size;
	uint8_t validWidth = m_infoForSAO->validWidth;
	uint8_t validHeight = m_infoForSAO->validHeight;
	bool bLastCol = m_infoForSAO->bLastCol;
	bool bLastRow = m_infoForSAO->bLastRow;
	bool bFirstCol = (m_infoForSAO->ctuPosX == 0);
	bool bFirstRow = (m_infoForSAO->ctuPosY == 0);

	uint8_t* inRecPixel = yCbCr == 0 ? m_infoForSAO->inRecPixelY : (yCbCr == 1 ? m_infoForSAO->inRecPixelU : m_infoForSAO->inRecPixelV);
	uint8_t* orgPixel = yCbCr == 0 ? m_infoForSAO->orgPixelY : (yCbCr == 1 ? m_infoForSAO->orgPixelU : m_infoForSAO->orgPixelV);
	uint8_t* inRecPixelLeft = m_saoRdoParam->leftCtuLineBuffer[yCbCr];
   
	//uint8_t* inRecPixelUp = yCbCr == 0 ? m_infoFromX265->inRecPixelUpY : (yCbCr == 1 ? m_infoFromX265->inRecPixelUpU : m_infoFromX265->inRecPixelUpV);

uint8_t* inRecPixelExtra = yCbCr == 0 ? m_infoFromX265->inRecPixelExtraY : (yCbCr == 1 ? m_infoFromX265->inRecPixelExtraU : m_infoFromX265->inRecPixelExtraV);

    int x, y;

    uint8_t* fenc;
    uint8_t* pRec;
	uint8_t* pRecLeft;
	uint8_t* pRecUp;
	uint8_t* pRecExtra = inRecPixelExtra;

	int iIsChroma = (yCbCr != 0) ? 1 : 0;
	int ctuWidth = validWidth >> iIsChroma;
	int ctuHeight = validHeight >> iIsChroma;
	int stride = iIsChroma ? (size / 2) : size;
	uint8_t* inRecPixelUp = &m_saoRdoParam->frameLineBuffer[yCbCr][m_infoForSAO->ctuPosX*stride];

    int64_t* iStats;
    int64_t* iCount;
    int iClassIdx;

    int iStartX;
    int iStartY;
    int iEndX;
    int iEndY;
    Pel* pTableBo = (yCbCr == 0) ? m_lumaTableBo : m_chromaTableBo;
    int32_t *tmp_swap;

	int numSkipLine = iIsChroma ? 2 : 4;

	if (m_saoLcuBasedOptimization == 0)
	{
		numSkipLine = 0;
	}

	int numSkipLineRight = iIsChroma ? 3 : 5;
	if (m_saoLcuBasedOptimization == 0)
	{
		numSkipLineRight = 0;
	}


//if(iSaoType == BO_0 || iSaoType == BO_1)
    {
        if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
        {
            numSkipLine = iIsChroma ? 1 : 3;
            numSkipLineRight = iIsChroma ? 2 : 4;
        }
        iStats = m_offsetOrg[yCbCr][SAO_BO];
        iCount = m_count[yCbCr][SAO_BO];

		fenc = orgPixel;
		pRec = inRecPixel;

		iEndX   = (bLastCol) ? ctuWidth : ctuWidth - numSkipLineRight;
		iEndY   = (bLastRow) ? ctuHeight : ctuHeight - numSkipLine;

		for (y = 0; y < iEndY; y++)
        {
            for (x = 0; x < iEndX; x++)
            {
                iClassIdx = pTableBo[pRec[x]];
                if (iClassIdx)
                {
                    iStats[iClassIdx] += (fenc[x] - pRec[x]);
                    iCount[iClassIdx]++;
                }
            }
			fenc += stride;
			pRec += stride;
        }
    }
    int iSignLeft;
    int iSignRight;
    int iSignDown;
    int iSignDown1;
    int iSignDown2;

    uint32_t uiEdgeType;

//if (iSaoType == EO_0  || iSaoType == EO_1 || iSaoType == EO_2 || iSaoType == EO_3)
    {
        //if (iSaoType == EO_0)
        {
            if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
            {
                numSkipLine = iIsChroma ? 1 : 3;
                numSkipLineRight = iIsChroma ? 3 : 5;
            }
            iStats = m_offsetOrg[yCbCr][SAO_EO_0];
            iCount = m_count[yCbCr][SAO_EO_0];

			fenc = orgPixel;
			pRec = inRecPixel;
			pRecLeft = inRecPixelLeft;

			iStartX = (bFirstCol) ? 1 : 0;
			iEndX   = (bLastCol) ? ctuWidth - 1 : ctuWidth - numSkipLineRight;
 
			for (y = 0; y < ctuHeight - numSkipLine; y++)
            {
				if (iStartX == 0) // need Left CTU pixels
				{
					iSignLeft = xSign(pRec[iStartX] - pRecLeft[y]);
				}
				else
				{
					iSignLeft = xSign(pRec[iStartX] - pRec[iStartX - 1]);
				}                

                for (x = iStartX; x < iEndX; x++)
                {
                    iSignRight =  xSign(pRec[x] - pRec[x + 1]);
                    uiEdgeType =  iSignRight + iSignLeft + 2;
                    iSignLeft  = -iSignRight;

                    iStats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
                    iCount[m_eoTable[uiEdgeType]]++;
                }
				fenc += stride;
				pRec += stride;
     
			}
        }

        //if (iSaoType == EO_1)
        {
            if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
            {
                numSkipLine = iIsChroma ? 2 : 4;
                numSkipLineRight = iIsChroma ? 2 : 4;
            }
            iStats = m_offsetOrg[yCbCr][SAO_EO_1];
            iCount = m_count[yCbCr][SAO_EO_1];

			fenc = orgPixel;
			pRec = inRecPixel;
			pRecUp = inRecPixelUp;

			iStartY = (bFirstRow) ? 1 : 0;
			iEndX   = (bLastCol) ? ctuWidth : ctuWidth - numSkipLineRight;
			iEndY   = (bLastRow) ? ctuHeight - 1 : ctuHeight - numSkipLine;

			if (bFirstRow)
			{
				fenc += stride;
				pRec += stride;
			}

			for (x = 0; x < ctuWidth; x++)
			{
				if (iStartY==0) // need Up CTU pixels
				{
					m_upBuff1[x] = xSign(pRec[x] - pRecUp[x]);
				}
				else
				{
					m_upBuff1[x] = xSign(pRec[x] - pRec[x - stride]);
				}
			}
	
			for (y = iStartY; y < iEndY; y++)
			{
				for (x = 0; x < iEndX; x++)
				{
					iSignDown = xSign(pRec[x] - pRec[x + stride]);
					uiEdgeType    =  iSignDown + m_upBuff1[x] + 2;
					m_upBuff1[x] = -iSignDown;

					iStats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
					iCount[m_eoTable[uiEdgeType]]++;
				}

				fenc += stride;
				pRec += stride;
			}
	}

        //if (iSaoType == EO_2) 135 degree
        {
            if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
            {
                numSkipLine = iIsChroma ? 2 : 4;
                numSkipLineRight = iIsChroma ? 3 : 5;
            }
            iStats = m_offsetOrg[yCbCr][SAO_EO_2];
            iCount = m_count[yCbCr][SAO_EO_2];

			fenc = orgPixel;
			pRec = inRecPixel;
			pRecUp = inRecPixelUp;

			iStartX = (bFirstCol) ? 1 : 0;
			iEndX = (bLastCol) ? ctuWidth - 1 : ctuWidth - numSkipLineRight;

			iStartY = (bFirstRow) ? 1 : 0;
			iEndY   = (bLastRow) ? ctuHeight - 1 : ctuHeight - numSkipLine;
			if (bFirstRow)
			{
				fenc += stride;
				pRec += stride;

			}
  
            for (x = iStartX; x < iEndX; x++)
            {

				if (iStartY == 0)
				{
					if (x == 0) // need extra pixel(left_top)
					{
						m_upBuff1[0] = xSign(pRec[0] - pRecExtra[0]);
					}
					else // need Up CTU pixels
					{
						m_upBuff1[x] = xSign(pRec[x] - pRecUp[x-1]);
					}
				}
				else // bFirstRow, begin with 2nd row
				{
					if(x==0) // need Left CTU pixel
					{
						m_upBuff1[0] = xSign(pRec[0] - pRecLeft[0]);
					}
					else
					{
						m_upBuff1[x] = xSign(pRec[x] - pRec[x - stride - 1]);
					}
				}
			}

            for (y = iStartY; y < iEndY; y++)
            {

				if (iStartX==0)
				{
					iSignDown2 = xSign(pRec[stride + iStartX] - pRecLeft[y]);
				}
				else
				{
					iSignDown2 = xSign(pRec[stride + iStartX] - pRec[iStartX - 1]);
				}
				
				for (x = iStartX; x < iEndX; x++)
                {
					iSignDown1 = xSign(pRec[x] - pRec[x + stride + 1]);
                    uiEdgeType      =  iSignDown1 + m_upBuff1[x] + 2;
                    m_upBufft[x + 1] = -iSignDown1;
                    iStats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
                    iCount[m_eoTable[uiEdgeType]]++;
                }

                m_upBufft[iStartX] = iSignDown2;
                tmp_swap  = m_upBuff1;
                m_upBuff1 = m_upBufft;
                m_upBufft = tmp_swap;

				pRec += stride;
				fenc += stride;


			}
        }
        //if (iSaoType == EO_3  )
        {
            if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
            {
                numSkipLine = iIsChroma ? 2 : 4;
                numSkipLineRight = iIsChroma ? 3 : 5;
            }
            iStats = m_offsetOrg[yCbCr][SAO_EO_3];
            iCount = m_count[yCbCr][SAO_EO_3];

			fenc = orgPixel;
			pRec = inRecPixel;

			iStartX = (bFirstCol) ? 1 : 0;
			iEndX = (bLastCol) ? ctuWidth - 1 : ctuWidth - numSkipLineRight;

			iStartY = (bFirstRow) ? 1 : 0;
			iEndY   = (bLastRow) ? ctuHeight - 1 : ctuHeight - numSkipLine;
     
			if (iStartY == 1) // when bFirstRow, the first row in currCTU  isn't calculate
            {
				fenc += stride;
				pRec += stride;

            }


			for (x = iStartX; x < iEndX; x++)
			{
				// 1th row, xSign(current - right)
				if (iStartY == 0)
				{
					//NOTE: the lastCol in currCTU is not calculated, so no extra pixels are needed
					m_upBuff1[x] = xSign(pRec[x] - pRecUp[x+1]);// need UP CTU pixels
				}
				else
				{
					m_upBuff1[x] = xSign(pRec[x] - pRec[x - stride + 1]);
				}
			}

			for (y = iStartY; y < iEndY; y++)
			{
				for (x = iStartX; x < iEndX; x++)
				{

					//xSign(current - left)
					if (x == 0) //need Left CTU pixels
					{
						iSignDown1 = xSign(pRec[0] - pRecLeft[y + 1]);
					}
					else
					{
						iSignDown1 = xSign(pRec[x] - pRec[x + stride - 1]);
					}

					uiEdgeType = iSignDown1 + m_upBuff1[x] + 2;
					m_upBuff1[x - 1] = -iSignDown1;
					iStats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
					iCount[m_eoTable[uiEdgeType]]++;
				}

				m_upBuff1[iEndX - 1] = xSign(pRec[iEndX - 1 + stride] - pRec[iEndX]);

				pRec += stride;
				fenc += stride;
			}
        }
    }
}



/** Calculate SAO statistics for current LCU without non-crossing slice
 * \param  addr,  partIdx,  yCbCr
 */
void hevcSAO::calcCtuSaoStatsAll(int yCbCr)
{
	uint8_t size = m_infoForSAO->size;
	uint8_t validWidth = m_infoForSAO->validWidth;
	uint8_t validHeight = m_infoForSAO->validHeight;
	bool bLastCol = m_infoForSAO->bLastCol;
	bool bLastRow = m_infoForSAO->bLastRow;
	bool bFirstCol = (m_infoForSAO->ctuPosX == 0);
	bool bFirstRow = (m_infoForSAO->ctuPosY == 0);

	uint8_t* inRecPixel = yCbCr == 0 ? m_infoForSAO->inRecPixelY : (yCbCr == 1 ? m_infoForSAO->inRecPixelU : m_infoForSAO->inRecPixelV);
	uint8_t* orgPixel = yCbCr == 0 ? m_infoForSAO->orgPixelY : (yCbCr == 1 ? m_infoForSAO->orgPixelU : m_infoForSAO->orgPixelV);
	uint8_t* inRecPixelLeft = m_saoRdoParam->leftCtuLineBuffer[yCbCr];
   
	//uint8_t* inRecPixelUp = yCbCr == 0 ? m_infoFromX265->inRecPixelUpY : (yCbCr == 1 ? m_infoFromX265->inRecPixelUpU : m_infoFromX265->inRecPixelUpV);

uint8_t* inRecPixelExtra = yCbCr == 0 ? m_infoFromX265->inRecPixelExtraY : (yCbCr == 1 ? m_infoFromX265->inRecPixelExtraU : m_infoFromX265->inRecPixelExtraV);

    int x, y;

    uint8_t* fenc;
    uint8_t* pRec;
	uint8_t* pRecLeft;
	uint8_t* pRecUp;
	uint8_t* pRecExtra = inRecPixelExtra;

	int iIsChroma = (yCbCr != 0) ? 1 : 0;
	int ctuWidth = validWidth >> iIsChroma;
	int ctuHeight = validHeight >> iIsChroma;
	int stride = iIsChroma ? (size / 2) : size;
	uint8_t* inRecPixelUp = &m_saoRdoParam->frameLineBuffer[yCbCr][m_infoForSAO->ctuPosX*stride];

    int64_t* iStats;
    int64_t* iCount;
    int iClassIdx;

    int iStartX;
    int iStartY;
    int iEndX;
    int iEndY;
    Pel* pTableBo = (yCbCr == 0) ? m_lumaTableBo : m_chromaTableBo;
    int32_t *tmp_swap;

	int numSkipLine = iIsChroma ? 2 : 4;

	if (m_saoLcuBasedOptimization == 0)
	{
		numSkipLine = 0;
	}

	int numSkipLineRight = iIsChroma ? 3 : 5;
	if (m_saoLcuBasedOptimization == 0)
	{
		numSkipLineRight = 0;
	}


	/* BO */
    {
        iStats = m_offsetOrg[yCbCr][SAO_BO];
        iCount = m_count[yCbCr][SAO_BO];

		fenc = orgPixel;
		pRec = inRecPixel;

		iEndX   = (bLastCol) ? ctuWidth : ctuWidth - 1;
		iEndY   = (bLastRow) ? ctuHeight : ctuHeight - 1;

		for (y = 0; y < iEndY; y++)
        {
            for (x = 0; x < iEndX; x++)
            {
                iClassIdx = pTableBo[pRec[x]];
                if (iClassIdx)
                {
                    iStats[iClassIdx] += (fenc[x] - pRec[x]);
                    iCount[iClassIdx]++;
                }
            }
			fenc += stride;
			pRec += stride;
        }
    }

    int iSignLeft;
    int iSignRight;
    int iSignDown;
    int iSignDown1;
    int iSignDown2;

    uint32_t uiEdgeType;


	/* EO_0 */
    {
        iStats = m_offsetOrg[yCbCr][SAO_EO_0];
        iCount = m_count[yCbCr][SAO_EO_0];

		fenc = orgPixel;
		pRec = inRecPixel;
		pRecLeft = inRecPixelLeft;

		iStartX = (bFirstCol) ? 1 : 0;
		iEndX   = (bLastCol) ? ctuWidth - 1 : ctuWidth;
 
		for (y = 0; y < ctuHeight - numSkipLine; y++)
        {
			if (iStartX == 0) // need Left CTU pixels
			{
				iSignLeft = xSign(pRec[iStartX] - pRecLeft[y]);
			}
			else
			{
				iSignLeft = xSign(pRec[iStartX] - pRec[iStartX - 1]);
			}                

            for (x = iStartX; x < iEndX; x++)
            {
                iSignRight =  xSign(pRec[x] - pRec[x + 1]);
                uiEdgeType =  iSignRight + iSignLeft + 2;
                iSignLeft  = -iSignRight;

                iStats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
                iCount[m_eoTable[uiEdgeType]]++;
            }
			fenc += stride;
			pRec += stride;
     
		}
    }

	/* EO_1 */
    {
        if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
        {
            numSkipLine = iIsChroma ? 2 : 4;
            numSkipLineRight = iIsChroma ? 2 : 4;
        }
        iStats = m_offsetOrg[yCbCr][SAO_EO_1];
        iCount = m_count[yCbCr][SAO_EO_1];

		fenc = orgPixel;
		pRec = inRecPixel;
		pRecUp = inRecPixelUp;

		iStartY = (bFirstRow) ? 1 : 0;
		iEndX   = (bLastCol) ? ctuWidth : ctuWidth - numSkipLineRight;
		iEndY   = (bLastRow) ? ctuHeight - 1 : ctuHeight - numSkipLine;

		if (bFirstRow)
		{
			fenc += stride;
			pRec += stride;
		}

		for (x = 0; x < ctuWidth; x++)
		{
			if (iStartY==0) // need Up CTU pixels
			{
				m_upBuff1[x] = xSign(pRec[x] - pRecUp[x]);
			}
			else
			{
				m_upBuff1[x] = xSign(pRec[x] - pRec[x - stride]);
			}
		}
	
		for (y = iStartY; y < iEndY; y++)
		{
			for (x = 0; x < iEndX; x++)
			{
				iSignDown = xSign(pRec[x] - pRec[x + stride]);
				uiEdgeType    =  iSignDown + m_upBuff1[x] + 2;
				m_upBuff1[x] = -iSignDown;

				iStats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
				iCount[m_eoTable[uiEdgeType]]++;
			}

			fenc += stride;
			pRec += stride;
		}
	}

	/* EO_2  135 degree*/
    {
        if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
        {
            numSkipLine = iIsChroma ? 2 : 4;
            numSkipLineRight = iIsChroma ? 3 : 5;
        }
        iStats = m_offsetOrg[yCbCr][SAO_EO_2];
        iCount = m_count[yCbCr][SAO_EO_2];

		fenc = orgPixel;
		pRec = inRecPixel;
		pRecUp = inRecPixelUp;

		iStartX = (bFirstCol) ? 1 : 0;
		iEndX = (bLastCol) ? ctuWidth - 1 : ctuWidth - numSkipLineRight;

		iStartY = (bFirstRow) ? 1 : 0;
		iEndY   = (bLastRow) ? ctuHeight - 1 : ctuHeight - numSkipLine;
		if (bFirstRow)
		{
			fenc += stride;
			pRec += stride;

		}
  
        for (x = iStartX; x < iEndX; x++)
        {

			if (iStartY == 0)
			{
				if (x == 0) // need extra pixel(left_top)
				{
					m_upBuff1[0] = xSign(pRec[0] - pRecExtra[0]);
				}
				else // need Up CTU pixels
				{
					m_upBuff1[x] = xSign(pRec[x] - pRecUp[x-1]);
				}
			}
			else // bFirstRow, begin with 2nd row
			{
				if(x==0) // need Left CTU pixel
				{
					m_upBuff1[0] = xSign(pRec[0] - pRecLeft[0]);
				}
				else
				{
					m_upBuff1[x] = xSign(pRec[x] - pRec[x - stride - 1]);
				}
			}
		}

        for (y = iStartY; y < iEndY; y++)
        {

			if (iStartX==0)
			{
				iSignDown2 = xSign(pRec[stride + iStartX] - pRecLeft[y]);
			}
			else
			{
				iSignDown2 = xSign(pRec[stride + iStartX] - pRec[iStartX - 1]);
			}
				
			for (x = iStartX; x < iEndX; x++)
            {
				iSignDown1 = xSign(pRec[x] - pRec[x + stride + 1]);
                uiEdgeType      =  iSignDown1 + m_upBuff1[x] + 2;
                m_upBufft[x + 1] = -iSignDown1;
                iStats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
                iCount[m_eoTable[uiEdgeType]]++;
            }

            m_upBufft[iStartX] = iSignDown2;
            tmp_swap  = m_upBuff1;
            m_upBuff1 = m_upBufft;
            m_upBufft = tmp_swap;

			pRec += stride;
			fenc += stride;


		}
    }


    /* EO_3  45 degree */
    {
        if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
        {
            numSkipLine = iIsChroma ? 2 : 4;
            numSkipLineRight = iIsChroma ? 3 : 5;
        }
        iStats = m_offsetOrg[yCbCr][SAO_EO_3];
        iCount = m_count[yCbCr][SAO_EO_3];

		fenc = orgPixel;
		pRec = inRecPixel;

		iStartX = (bFirstCol) ? 1 : 0;
		iEndX = (bLastCol) ? ctuWidth - 1 : ctuWidth - numSkipLineRight;

		iStartY = (bFirstRow) ? 1 : 0;
		iEndY   = (bLastRow) ? ctuHeight - 1 : ctuHeight - numSkipLine;
     
		if (iStartY == 1) // when bFirstRow, the first row in currCTU  isn't calculate
        {
			fenc += stride;
			pRec += stride;

        }


		for (x = iStartX; x < iEndX; x++)
		{
			// 1th row, xSign(current - right)
			if (iStartY == 0)
			{
				//NOTE: the lastCol in currCTU is not calculated, so no extra pixels are needed
				m_upBuff1[x] = xSign(pRec[x] - pRecUp[x+1]);// need UP CTU pixels
			}
			else
			{
				m_upBuff1[x] = xSign(pRec[x] - pRec[x - stride + 1]);
			}
		}

		for (y = iStartY; y < iEndY; y++)
		{
			for (x = iStartX; x < iEndX; x++)
			{

				//xSign(current - left)
				if (x == 0) //need Left CTU pixels
				{
					iSignDown1 = xSign(pRec[0] - pRecLeft[y + 1]);
				}
				else
				{
					iSignDown1 = xSign(pRec[x] - pRec[x + stride - 1]);
				}

				uiEdgeType = iSignDown1 + m_upBuff1[x] + 2;
				m_upBuff1[x - 1] = -iSignDown1;
				iStats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
				iCount[m_eoTable[uiEdgeType]]++;
			}

			m_upBuff1[iEndX - 1] = xSign(pRec[iEndX - 1 + stride] - pRec[iEndX]);

			pRec += stride;
			fenc += stride;
		}
	}
}

void hevcSAO::calcSaoStatsRowCus_BeforeDblk(TComPic* pic, int idxY)
{
    int addr, yCbCr;
    int x, y;
    TComSPS *pTmpSPS =  pic->getSlice()->getSPS();

    Pel* fenc;
    Pel* pRec;
    int stride;
    int lcuHeight = pTmpSPS->getMaxCUHeight();
    int lcuWidth  = pTmpSPS->getMaxCUWidth();
    uint32_t rPelX;
    uint32_t bPelY;
    int64_t* stats;
    int64_t* count;
    int classIdx;
    int picWidthTmp = 0;
    int picHeightTmp = 0;
    int startX;
    int startY;
    int endX;
    int endY;
    int firstX, firstY;

    int idxX;
    int frameWidthInCU  = m_numCuInWidth;

    int isChroma;
    int numSkipLine, numSkipLineRight;

    uint32_t lPelX, tPelY;
    TComDataCU *pTmpCu;
    Pel* pTableBo;
    int32_t *tmp_swap;

    {
        for (idxX = 0; idxX < frameWidthInCU; idxX++)
        {
            lcuHeight = pTmpSPS->getMaxCUHeight();
            lcuWidth  = pTmpSPS->getMaxCUWidth();
            addr     = idxX  + frameWidthInCU * idxY;
            pTmpCu = pic->getCU(addr);
            lPelX   = pTmpCu->getCUPelX();
            tPelY   = pTmpCu->getCUPelY();

            memset(m_countPreDblk[addr], 0, 3 * MAX_NUM_SAO_TYPE * MAX_NUM_SAO_CLASS * sizeof(int64_t));
            memset(m_offsetOrgPreDblk[addr], 0, 3 * MAX_NUM_SAO_TYPE * MAX_NUM_SAO_CLASS * sizeof(int64_t));
            for (yCbCr = 0; yCbCr < 3; yCbCr++)
            {
                isChroma = (yCbCr != 0) ? 1 : 0;

                if (yCbCr == 0)
                {
                    picWidthTmp  = m_picWidth;
                    picHeightTmp = m_picHeight;
                }
                else if (yCbCr == 1)
                {
                    picWidthTmp  = m_picWidth  >> isChroma;
                    picHeightTmp = m_picHeight >> isChroma;
                    lcuWidth     = lcuWidth    >> isChroma;
                    lcuHeight    = lcuHeight   >> isChroma;
                    lPelX       = lPelX      >> isChroma;
                    tPelY       = tPelY      >> isChroma;
                }
                rPelX       = lPelX + lcuWidth;
                bPelY       = tPelY + lcuHeight;
                rPelX       = rPelX > picWidthTmp  ? picWidthTmp  : rPelX;
                bPelY       = bPelY > picHeightTmp ? picHeightTmp : bPelY;
                lcuWidth     = rPelX - lPelX;
                lcuHeight    = bPelY - tPelY;

                stride    =  (yCbCr == 0) ? pic->getStride() : pic->getCStride();
                pTableBo = (yCbCr == 0) ? m_lumaTableBo : m_chromaTableBo;

                //if(iSaoType == BO)

                numSkipLine = isChroma ? 1 : 3;
                numSkipLineRight = isChroma ? 2 : 4;

                stats = m_offsetOrgPreDblk[addr][yCbCr][SAO_BO];
                count = m_countPreDblk[addr][yCbCr][SAO_BO];

                fenc = getPicYuvAddr(pic->getPicYuvOrg(), yCbCr, addr);
                pRec = getPicYuvAddr(pic->getPicYuvRec(), yCbCr, addr);

                startX   = (rPelX == picWidthTmp) ? lcuWidth : lcuWidth - numSkipLineRight;
                startY   = (bPelY == picHeightTmp) ? lcuHeight : lcuHeight - numSkipLine;

                for (y = 0; y < lcuHeight; y++)
                {
                    for (x = 0; x < lcuWidth; x++)
                    {
                        if (x < startX && y < startY)
                            continue;

                        classIdx = pTableBo[pRec[x]];
                        if (classIdx)
                        {
                            stats[classIdx] += (fenc[x] - pRec[x]);
                            count[classIdx]++;
                        }
                    }

                    fenc += stride;
                    pRec += stride;
                }

                int signLeft;
                int signRight;
                int signDown;
                int signDown1;
                int signDown2;

                uint32_t uiEdgeType;

                //if (iSaoType == EO_0)

                numSkipLine = isChroma ? 1 : 3;
                numSkipLineRight = isChroma ? 3 : 5;

                stats = m_offsetOrgPreDblk[addr][yCbCr][SAO_EO_0];
                count = m_countPreDblk[addr][yCbCr][SAO_EO_0];

                fenc = getPicYuvAddr(pic->getPicYuvOrg(), yCbCr, addr);
                pRec = getPicYuvAddr(pic->getPicYuvRec(), yCbCr, addr);

                startX   = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth - numSkipLineRight;
                startY   = (bPelY == picHeightTmp) ? lcuHeight : lcuHeight - numSkipLine;
                firstX   = (lPelX == 0) ? 1 : 0;
                endX   = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth;

                for (y = 0; y < lcuHeight; y++)
                {
                    signLeft = xSign(pRec[firstX] - pRec[firstX - 1]);
                    for (x = firstX; x < endX; x++)
                    {
                        signRight =  xSign(pRec[x] - pRec[x + 1]);
                        uiEdgeType =  signRight + signLeft + 2;
                        signLeft  = -signRight;

                        if (x < startX && y < startY)
                            continue;

                        stats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
                        count[m_eoTable[uiEdgeType]]++;
                    }

                    fenc += stride;
                    pRec += stride;
                }

                //if (iSaoType == EO_1)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 2 : 4;

                stats = m_offsetOrgPreDblk[addr][yCbCr][SAO_EO_1];
                count = m_countPreDblk[addr][yCbCr][SAO_EO_1];

                fenc = getPicYuvAddr(pic->getPicYuvOrg(), yCbCr, addr);
                pRec = getPicYuvAddr(pic->getPicYuvRec(), yCbCr, addr);

                startX   = (rPelX == picWidthTmp) ? lcuWidth : lcuWidth - numSkipLineRight;
                startY   = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight - numSkipLine;
                firstY = (tPelY == 0) ? 1 : 0;
                endY   = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight;
                if (firstY == 1)
                {
                    fenc += stride;
                    pRec += stride;
                }

                for (x = 0; x < lcuWidth; x++)
                {
                    m_upBuff1[x] = xSign(pRec[x] - pRec[x - stride]);
                }

                for (y = firstY; y < endY; y++)
                {
                    for (x = 0; x < lcuWidth; x++)
                    {
                        signDown     =  xSign(pRec[x] - pRec[x + stride]);
                        uiEdgeType    =  signDown + m_upBuff1[x] + 2;
                        m_upBuff1[x] = -signDown;

                        if (x < startX && y < startY)
                            continue;

                        stats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
                        count[m_eoTable[uiEdgeType]]++;
                    }

                    fenc += stride;
                    pRec += stride;
                }

                //if (iSaoType == EO_2)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 3 : 5;

                stats = m_offsetOrgPreDblk[addr][yCbCr][SAO_EO_2];
                count = m_countPreDblk[addr][yCbCr][SAO_EO_2];

                fenc = getPicYuvAddr(pic->getPicYuvOrg(), yCbCr, addr);
                pRec = getPicYuvAddr(pic->getPicYuvRec(), yCbCr, addr);

                startX   = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth - numSkipLineRight;
                startY   = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight - numSkipLine;
                firstX   = (lPelX == 0) ? 1 : 0;
                firstY = (tPelY == 0) ? 1 : 0;
                endX   = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth;
                endY   = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight;
                if (firstY == 1)
                {
                    fenc += stride;
                    pRec += stride;
                }

                for (x = firstX; x < endX; x++)
                {
                    m_upBuff1[x] = xSign(pRec[x] - pRec[x - stride - 1]);
                }

                for (y = firstY; y < endY; y++)
                {
                    signDown2 = xSign(pRec[stride + startX] - pRec[startX - 1]);
                    for (x = firstX; x < endX; x++)
                    {
                        signDown1      =  xSign(pRec[x] - pRec[x + stride + 1]);
                        uiEdgeType      =  signDown1 + m_upBuff1[x] + 2;
                        m_upBufft[x + 1] = -signDown1;

                        if (x < startX && y < startY)
                            continue;

                        stats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
                        count[m_eoTable[uiEdgeType]]++;
                    }

                    m_upBufft[firstX] = signDown2;
                    tmp_swap  = m_upBuff1;
                    m_upBuff1 = m_upBufft;
                    m_upBufft = tmp_swap;

                    pRec += stride;
                    fenc += stride;
                }

                //if (iSaoType == EO_3)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 3 : 5;

                stats = m_offsetOrgPreDblk[addr][yCbCr][SAO_EO_3];
                count = m_countPreDblk[addr][yCbCr][SAO_EO_3];

                fenc = getPicYuvAddr(pic->getPicYuvOrg(), yCbCr, addr);
                pRec = getPicYuvAddr(pic->getPicYuvRec(), yCbCr, addr);

                startX   = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth - numSkipLineRight;
                startY   = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight - numSkipLine;
                firstX   = (lPelX == 0) ? 1 : 0;
                firstY = (tPelY == 0) ? 1 : 0;
                endX   = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth;
                endY   = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight;
                if (firstY == 1)
                {
                    fenc += stride;
                    pRec += stride;
                }

                for (x = firstX - 1; x < endX; x++)
                {
                    m_upBuff1[x] = xSign(pRec[x] - pRec[x - stride + 1]);
                }

                for (y = firstY; y < endY; y++)
                {
                    for (x = firstX; x < endX; x++)
                    {
                        signDown1      =  xSign(pRec[x] - pRec[x + stride - 1]);
                        uiEdgeType      =  signDown1 + m_upBuff1[x] + 2;
                        m_upBuff1[x - 1] = -signDown1;

                        if (x < startX && y < startY)
                            continue;

                        stats[m_eoTable[uiEdgeType]] += (fenc[x] - pRec[x]);
                        count[m_eoTable[uiEdgeType]]++;
                    }

                    m_upBuff1[endX - 1] = xSign(pRec[endX - 1 + stride] - pRec[endX]);

                    pRec += stride;
                    fenc += stride;
                }
            }
        }
    }
}

/** get SAO statistics
 * \param  *psQTPart,  yCbCr
 */

void hevcSAO::getSaoStats(SAOQTPart *psQTPart, int yCbCr)
{
}

/** reset offset statistics
 * \param
 */
void hevcSAO::resetStats()
{
    for (int i = 0; i < m_numTotalParts; i++)
    {
        m_costPartBest[i] = MAX_DOUBLE;
        m_typePartBest[i] = -1;
        m_distOrg[i] = 0;
        for (int j = 0; j < MAX_NUM_SAO_TYPE; j++)
        {
            m_dist[i][j] = 0;
            m_rate[i][j] = 0;
            m_cost[i][j] = 0;
            for (int k = 0; k < MAX_NUM_SAO_CLASS; k++)
            {
                m_count[i][j][k] = 0;
                m_offset[i][j][k] = 0;
                m_offsetOrg[i][j][k] = 0;
            }
        }
    }
}

/** Sample adaptive offset process
 * \param saoParam
 * \param dLambdaLuma
 * \param lambdaChroma
 */
void hevcSAO::SAOProcess(SAOParam *saoParam)
{
}

/** Check merge SAO unit
 * \param saoUnitCurr current SAO unit
 * \param saoUnitCheck SAO unit tobe check
 * \param dir direction
 */
void hevcSAO::checkMerge(SaoLcuParam * saoUnitCurr, SaoLcuParam * saoUnitCheck, int dir)
{
    int i;
    int countDiff = 0;

    if (saoUnitCurr->partIdx != saoUnitCheck->partIdx)
    {
        if (saoUnitCurr->typeIdx != -1)
        {
            if (saoUnitCurr->typeIdx == saoUnitCheck->typeIdx)
            {
                for (i = 0; i < saoUnitCurr->length; i++)
                {
                    countDiff += (saoUnitCurr->offset[i] != saoUnitCheck->offset[i]);
                }

                countDiff += (saoUnitCurr->subTypeIdx != saoUnitCheck->subTypeIdx);
                if (countDiff == 0)
                {
                    saoUnitCurr->partIdx = saoUnitCheck->partIdx;
                    if (dir == 1)
                    {
                        saoUnitCurr->mergeUpFlag = 1;
                        saoUnitCurr->mergeLeftFlag = 0;
                    }
                    else
                    {
                        saoUnitCurr->mergeUpFlag = 0;
                        saoUnitCurr->mergeLeftFlag = 1;
                    }
                }
            }
        }
        else
        {
            if (saoUnitCurr->typeIdx == saoUnitCheck->typeIdx)
            {
                saoUnitCurr->partIdx = saoUnitCheck->partIdx;
                if (dir == 1)
                {
                    saoUnitCurr->mergeUpFlag = 1;
                    saoUnitCurr->mergeLeftFlag = 0;
                }
                else
                {
                    saoUnitCurr->mergeUpFlag = 0;
                    saoUnitCurr->mergeLeftFlag = 1;
                }
            }
        }
    }
}

/** Assign SAO unit syntax from picture-based algorithm
 * \param saoLcuParam SAO LCU parameters
 * \param saoPart SAO part
 * \param oneUnitFlag SAO one unit flag
 * \param yCbCr color component Index
 */
void hevcSAO::assignSaoUnitSyntax(SaoLcuParam* saoLcuParam,  SAOQTPart* saoPart, bool &oneUnitFlag)
{
    if (saoPart->bSplit == 0)
    {
        oneUnitFlag = 1;
    }
    else
    {
        int i, j, addr, addrUp, addrLeft,  idx, idxUp, idxLeft,  idxCount;

        oneUnitFlag = 0;

        idxCount = -1;
        saoLcuParam[0].mergeUpFlag = 0;
        saoLcuParam[0].mergeLeftFlag = 0;

        for (j = 0; j < m_numCuInHeight; j++)
        {
            for (i = 0; i < m_numCuInWidth; i++)
            {
                addr     = i + j * m_numCuInWidth;
                addrLeft = (addr % m_numCuInWidth == 0) ? -1 : addr - 1;
                addrUp   = (addr < m_numCuInWidth)      ? -1 : addr - m_numCuInWidth;
                idx      = saoLcuParam[addr].partIdxTmp;
                idxLeft  = (addrLeft == -1) ? -1 : saoLcuParam[addrLeft].partIdxTmp;
                idxUp    = (addrUp == -1)   ? -1 : saoLcuParam[addrUp].partIdxTmp;

                if (idx != idxLeft && idx != idxUp)
                {
                    saoLcuParam[addr].mergeUpFlag   = 0;
                    idxCount++;
                    saoLcuParam[addr].mergeLeftFlag = 0;
                    saoLcuParam[addr].partIdx = idxCount;
                }
                else if (idx == idxLeft)
                {
                    saoLcuParam[addr].mergeUpFlag   = 1;
                    saoLcuParam[addr].mergeLeftFlag = 1;
                    saoLcuParam[addr].partIdx = saoLcuParam[addrLeft].partIdx;
                }
                else if (idx == idxUp)
                {
                    saoLcuParam[addr].mergeUpFlag   = 1;
                    saoLcuParam[addr].mergeLeftFlag = 0;
                    saoLcuParam[addr].partIdx = saoLcuParam[addrUp].partIdx;
                }
                if (addrUp != -1)
                {
                    checkMerge(&saoLcuParam[addr], &saoLcuParam[addrUp], 1);
                }
                if (addrLeft != -1)
                {
                    checkMerge(&saoLcuParam[addr], &saoLcuParam[addrLeft], 0);
                }
            }
        }
    }
}

void hevcSAO::rdoSaoUnitRowInit(SAOParam *saoParam)
{
    saoParam->bSaoFlag[0] = true;
    saoParam->bSaoFlag[1] = true;
    saoParam->oneUnitFlag[0] = false;
    saoParam->oneUnitFlag[1] = false;
    saoParam->oneUnitFlag[2] = false;

    numNoSao[0] = 0; // Luma
    numNoSao[1] = 0; // Chroma
#if 0
    if (depth > 0 && m_depthSaoRate[0][depth - 1] > SAO_ENCODING_RATE)
    {
        saoParam->bSaoFlag[0] = false;
    }
    if (depth > 0 && m_depthSaoRate[1][depth - 1] > SAO_ENCODING_RATE_CHROMA)
    {
        saoParam->bSaoFlag[1] = false;
    }
#endif
}

void hevcSAO::rdoSaoUnitRowEnd(SAOParam *saoParam, int numlcus)
{
    if (!saoParam->bSaoFlag[0])
    {
        m_depthSaoRate[0][depth] = 1.0;
    }
    else
    {
        m_depthSaoRate[0][depth] = numNoSao[0] / ((double)numlcus);
    }
    if (!saoParam->bSaoFlag[1])
    {
        m_depthSaoRate[1][depth] = 1.0;
    }
    else
    {
        m_depthSaoRate[1][depth] = numNoSao[1] / ((double)numlcus * 2);
    }
}


/** rate distortion optimization of SAO unit
 * \param saoParam SAO parameters
 * \param addr address
 * \param addrUp above address
 * \param addrLeft left address
 * \param yCbCr color component index
 * \param lambda
 */
inline int64_t hevcSAO::estSaoTypeDist(int compIdx, int typeIdx, int shift, double lambda, int32_t *currentDistortionTableBo, double *currentRdCostTableBo)
{
    int64_t estDist = 0;
    int classIdx;
    int saoBitIncrease = (compIdx == 0) ? m_saoBitIncreaseY : m_saoBitIncreaseC;
    int saoOffsetTh = (compIdx == 0) ? m_offsetThY : m_offsetThC;

    for (classIdx = 1; classIdx < ((typeIdx < SAO_BO) ?  m_numClass[typeIdx] + 1 : SAO_MAX_BO_CLASSES + 1); classIdx++)
    {
        if (typeIdx == SAO_BO)
        {
            currentDistortionTableBo[classIdx - 1] = 0;
            currentRdCostTableBo[classIdx - 1] = lambda;
        }
        if (m_count[compIdx][typeIdx][classIdx])
        {
            m_offset[compIdx][typeIdx][classIdx] = (int64_t)xRoundIbdi((double)(m_offsetOrg[compIdx][typeIdx][classIdx] << (X265_DEPTH - 8)) / (double)(m_count[compIdx][typeIdx][classIdx] << saoBitIncrease));
            m_offset[compIdx][typeIdx][classIdx] = Clip3(-saoOffsetTh + 1, saoOffsetTh - 1, (int)m_offset[compIdx][typeIdx][classIdx]);
            if (typeIdx < 4)
            {
                if (m_offset[compIdx][typeIdx][classIdx] < 0 && classIdx < 3)
                {
                    m_offset[compIdx][typeIdx][classIdx] = 0;
                }
                if (m_offset[compIdx][typeIdx][classIdx] > 0 && classIdx >= 3)
                {
                    m_offset[compIdx][typeIdx][classIdx] = 0;
                }
            }
            m_offset[compIdx][typeIdx][classIdx] = estIterOffset(typeIdx, classIdx, lambda, m_offset[compIdx][typeIdx][classIdx], m_count[compIdx][typeIdx][classIdx], m_offsetOrg[compIdx][typeIdx][classIdx], shift, saoBitIncrease, currentDistortionTableBo, currentRdCostTableBo, saoOffsetTh);
        }
        else
        {
            m_offsetOrg[compIdx][typeIdx][classIdx] = 0;
            m_offset[compIdx][typeIdx][classIdx] = 0;
        }
        if (typeIdx != SAO_BO)
        {
            estDist += estSaoDist(m_count[compIdx][typeIdx][classIdx], m_offset[compIdx][typeIdx][classIdx] << saoBitIncrease, m_offsetOrg[compIdx][typeIdx][classIdx], shift);
        }
    }

    return estDist;
}

inline int64_t hevcSAO::estSaoDist(int64_t count, int64_t offset, int64_t offsetOrg, int shift)
{
    return (count * offset * offset - offsetOrg * offset * 2) >> shift;
}

inline int64_t hevcSAO::estIterOffset(int typeIdx, int classIdx, double lambda, int64_t offsetInput, int64_t count, int64_t offsetOrg, int shift, int bitIncrease, int32_t *currentDistortionTableBo, double *currentRdCostTableBo, int offsetTh)
{
    //Clean up, best_q_offset.
    int64_t iterOffset, tempOffset;
    int64_t tempDist, tempRate;
    double tempCost, tempMinCost;
    int64_t offsetOutput = 0;

    iterOffset = offsetInput;
    // Assuming sending quantized value 0 results in zero offset and sending the value zero needs 1 bit. entropy coder can be used to measure the exact rate here.
    tempMinCost = lambda;
    while (iterOffset != 0)
    {
        // Calculate the bits required for signalling the offset
        tempRate = (typeIdx == SAO_BO) ? (abs((int)iterOffset) + 2) : (abs((int)iterOffset) + 1);
        if (abs((int)iterOffset) == offsetTh - 1)
        {
            tempRate--;
        }
        // Do the dequntization before distorion calculation
        tempOffset  = iterOffset << bitIncrease;
        tempDist    = estSaoDist(count, tempOffset, offsetOrg, shift);
        tempCost    = ((double)tempDist + lambda * (double)tempRate);
        if (tempCost < tempMinCost)
        {
            tempMinCost = tempCost;
            offsetOutput = iterOffset;
            if (typeIdx == SAO_BO)
            {
                currentDistortionTableBo[classIdx - 1] = (int)tempDist;
                currentRdCostTableBo[classIdx - 1] = tempCost;
            }
        }
        iterOffset = (iterOffset > 0) ? (iterOffset - 1) : (iterOffset + 1);
    }

    return offsetOutput;
}

void hevcSAO::calcLumaDist(int allowMergeLeft, int allowMergeUp, double lambda, SaoCtuParam* mergeCtuParam, double *compDistortion)
{
	uint8_t ctuPosX = m_infoForSAO->ctuPosX;
	SaoGlobalCtuParam* leftCtuParam = &m_saoRdoParam->ctuParam[0];
	SaoGlobalCtuParam* upCtuParam = &m_saoRdoParam->frameLineParam[0][ctuPosX];
	SaoGlobalCtuParam* neighborCtuParam;
	SaoCtuParam* saoCtuParam = &m_infoForSAO->saoCtuParam[0]; // Y comp
	//uint8_t size = m_infoForSAO->size;
    int typeIdx;

    int64_t estDist;
    int classIdx;
    int shift = 2 * DISTORTION_PRECISION_ADJUSTMENT(X265_DEPTH - 8);
    int64_t bestDist;

	resetSaoCtuParam(saoCtuParam);
	resetSaoCtuParam(&mergeCtuParam[0]);
	resetSaoCtuParam(&mergeCtuParam[1]);	
	
    double dCostPartBest = MAX_DOUBLE;

    double  bestRDCostTableBo = MAX_DOUBLE;
    int     bestClassTableBo    = 0;
    int     currentDistortionTableBo[MAX_NUM_SAO_CLASS];
    double  currentRdCostTableBo[MAX_NUM_SAO_CLASS];

    SaoCtuParam   saoCtuParamRdo;
    double   estRate = 0;

	resetSaoCtuParam(&saoCtuParamRdo);

    m_rdGoOnSbacCoder->load(m_rdSbacCoders[0][CI_TEMP_BEST]);
    m_rdGoOnSbacCoder->resetBits();
	m_entropyCoder->encodeSaoOffset(&saoCtuParamRdo, 0);

    dCostPartBest = m_entropyCoder->getNumberOfWrittenBits() * lambda;
	copySaoCtuParam(saoCtuParam, &saoCtuParamRdo);
    bestDist = 0;

    for (typeIdx = 0; typeIdx < MAX_NUM_SAO_TYPE; typeIdx++)
    {
        estDist = estSaoTypeDist(0, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
#if SAO_DBG
		assert(estDist == m_infoFromX265->eoEstDist[m_infoFromX265->ctuPosX][typeIdx]);
#endif
        if (typeIdx == SAO_BO)
        {
            // Estimate Best Position
            double currentRDCost = 0.0;

            for (int i = 0; i < SAO_MAX_BO_CLASSES - SAO_BO_LEN + 1; i++)
            {
                currentRDCost = 0.0;
                for (uint32_t uj = i; uj < i + SAO_BO_LEN; uj++)
                {
                    currentRDCost += currentRdCostTableBo[uj];
                }

                if (currentRDCost < bestRDCostTableBo)
                {
                    bestRDCostTableBo = currentRDCost;
                    bestClassTableBo  = i;
                }
            }

            // Re code all Offsets
            // Code Center
            estDist = 0;
            for (classIdx = bestClassTableBo; classIdx < bestClassTableBo + SAO_BO_LEN; classIdx++)
            {
                estDist += currentDistortionTableBo[classIdx];
            }
#if SAO_DBG
			assert(estDist == m_infoFromX265->boEstDist[m_infoFromX265->ctuPosX]);
#endif
        }
        resetSaoCtuParam(&saoCtuParamRdo);
		//saoCtuParamRdo.length = 4;
		saoCtuParamRdo.typeIdx = typeIdx;
		saoCtuParamRdo.mergeLeftFlag = 0;
		saoCtuParamRdo.mergeUpFlag = 0;
		saoCtuParamRdo.bandIdx = (typeIdx == SAO_BO) ? bestClassTableBo : 0;
        for (classIdx = 0; classIdx < 4; classIdx++)
        {
			saoCtuParamRdo.offset[classIdx] = (int)m_offset[0][typeIdx][classIdx + saoCtuParamRdo.bandIdx + 1];
        }

        m_rdGoOnSbacCoder->load(m_rdSbacCoders[0][CI_TEMP_BEST]);
        m_rdGoOnSbacCoder->resetBits();
		m_entropyCoder->encodeSaoOffset(&saoCtuParamRdo, 0);

        estRate = m_entropyCoder->getNumberOfWrittenBits();
        m_cost[0][typeIdx] = (double)((double)estDist + lambda * (double)estRate);
#if SAO_DBG
		assert(estRate == m_infoFromX265->estRate[m_infoFromX265->ctuPosX][typeIdx]);
#endif
        if (m_cost[0][typeIdx] < dCostPartBest)
        {
            dCostPartBest = m_cost[0][typeIdx];
			copySaoCtuParam(saoCtuParam, &saoCtuParamRdo);
            bestDist = estDist;
        }
    }

    compDistortion[0] += ((double)bestDist / lambda);
#if SAO_DBG
	assert(lambda == m_infoFromX265->lumaLambda);
	assert(compDistortion[0] == m_infoFromX265->dist[m_infoFromX265->ctuPosX][0][0]);
	//assert(m_infoFromX265->compDist[m_infoFromX265->colIdx][0] != -9.5789767833552038);
#endif
    m_rdGoOnSbacCoder->load(m_rdSbacCoders[0][CI_TEMP_BEST]);
	m_entropyCoder->encodeSaoOffset(saoCtuParam, 0);
    m_rdGoOnSbacCoder->store(m_rdSbacCoders[0][CI_TEMP_BEST]);

    // merge left or merge up	
    for (int idxNeighbor = 0; idxNeighbor < 2; idxNeighbor++)
    {
        neighborCtuParam = NULL;
        if (allowMergeLeft  && idxNeighbor == 0) // merge Lefts
        {	
            neighborCtuParam = leftCtuParam;
        }
        else if (allowMergeUp && idxNeighbor == 1) // merge Up
        {
            neighborCtuParam = upCtuParam;
        }
        if (neighborCtuParam != NULL)
        {
            estDist = 0;
            typeIdx = neighborCtuParam->typeIdx;
            if (typeIdx >= 0)
            {
				int mergeBandPosition = (typeIdx == SAO_BO) ? neighborCtuParam->bandIdx : 0;					

				int   merge_iOffset;
                for (classIdx = 0; classIdx < m_numClass[typeIdx]; classIdx++)
                {
					merge_iOffset = neighborCtuParam->offset[classIdx];
                    estDist   += estSaoDist(m_count[0][typeIdx][classIdx + mergeBandPosition + 1], merge_iOffset, m_offsetOrg[0][typeIdx][classIdx + mergeBandPosition + 1],  shift);
                }
            }
            else
            {
                estDist = 0; // noSAO
            }

            copySaoCtuParam(&mergeCtuParam[idxNeighbor], neighborCtuParam);
			mergeCtuParam[idxNeighbor].mergeUpFlag = idxNeighbor;
			mergeCtuParam[idxNeighbor].mergeLeftFlag = !idxNeighbor;
			
            compDistortion[idxNeighbor + 1] += ((double)estDist / lambda);
        }
    }	
#if SAO_DBG
	assert(compDistortion[1] == m_infoFromX265->dist[m_infoFromX265->ctuPosX][1][0]);
	assert(compDistortion[2] == m_infoFromX265->dist[m_infoFromX265->ctuPosX][2][0]);
#endif
}

void hevcSAO::calcChromaDist(int allowMergeLeft, int allowMergeUp, double lambda, SaoCtuParam* crSaoParam, SaoCtuParam* cbSaoParam, double *distortion)
{

	uint8_t ctuPosX = m_infoForSAO->ctuPosX;
	SaoGlobalCtuParam* leftCtuParam[2];
	SaoGlobalCtuParam* upCtuParam[2];
	SaoGlobalCtuParam* neighborCtuParam[2];
	
	leftCtuParam[0] = &m_saoRdoParam->ctuParam[1]; // Cb
	leftCtuParam[1] = &m_saoRdoParam->ctuParam[2]; // Cr
	upCtuParam[0] = &m_saoRdoParam->frameLineParam[1][ctuPosX]; // Cb
	upCtuParam[1] = &m_saoRdoParam->frameLineParam[2][ctuPosX]; // Cr
	
	SaoCtuParam* saoCtuParam[2] = {&m_infoForSAO->saoCtuParam[1], &m_infoForSAO->saoCtuParam[2]}; // Cb, Cr
	
    int typeIdx;

    int64_t estDist[2];
    int classIdx;
    int shift = 2 * DISTORTION_PRECISION_ADJUSTMENT(X265_DEPTH - 8);
    int64_t bestDist = 0;

    SaoCtuParam*  saoMergeParam[2][2];

    saoMergeParam[0][0] = &crSaoParam[0];
    saoMergeParam[0][1] = &crSaoParam[1];
    saoMergeParam[1][0] = &cbSaoParam[0];
    saoMergeParam[1][1] = &cbSaoParam[1];

    resetSaoCtuParam(saoMergeParam[0][0]);
	resetSaoCtuParam(saoMergeParam[0][1]);
	resetSaoCtuParam(saoMergeParam[1][0]);
	resetSaoCtuParam(saoMergeParam[1][1]);

    double costPartBest = MAX_DOUBLE;

    double  bestRDCostTableBo;
    int     bestClassTableBo[2]    = { 0, 0 };
    int     currentDistortionTableBo[MAX_NUM_SAO_CLASS];
    double  currentRdCostTableBo[MAX_NUM_SAO_CLASS];

    SaoCtuParam   saoCtuParamRdo[2];
    double   estRate = 0;

	resetSaoCtuParam(&saoCtuParamRdo[0]);
	resetSaoCtuParam(&saoCtuParamRdo[1]);

    m_rdGoOnSbacCoder->load(m_rdSbacCoders[0][CI_TEMP_BEST]);
    m_rdGoOnSbacCoder->resetBits();
	m_entropyCoder->encodeSaoOffset(&saoCtuParamRdo[0], 1);
	m_entropyCoder->encodeSaoOffset(&saoCtuParamRdo[1], 2);

    costPartBest = m_entropyCoder->getNumberOfWrittenBits() * lambda;
	copySaoCtuParam(saoCtuParam[0], &saoCtuParamRdo[0]);
	copySaoCtuParam(saoCtuParam[1], &saoCtuParamRdo[1]);

    for (typeIdx = 0; typeIdx < MAX_NUM_SAO_TYPE; typeIdx++)
    {
        if (typeIdx == SAO_BO)
        {
            // Estimate Best Position
            for (int compIdx = 0; compIdx < 2; compIdx++)
            {
                double currentRDCost = 0.0;
                bestRDCostTableBo = MAX_DOUBLE;
                estDist[compIdx] = estSaoTypeDist(compIdx + 1, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
                for (int i = 0; i < SAO_MAX_BO_CLASSES - SAO_BO_LEN + 1; i++)
                {
                    currentRDCost = 0.0;
                    for (uint32_t uj = i; uj < i + SAO_BO_LEN; uj++)
                    {
                        currentRDCost += currentRdCostTableBo[uj];
                    }

                    if (currentRDCost < bestRDCostTableBo)
                    {
                        bestRDCostTableBo = currentRDCost;
                        bestClassTableBo[compIdx]  = i;
                    }
                }

                // Re code all Offsets
                // Code Center
                estDist[compIdx] = 0;
                for (classIdx = bestClassTableBo[compIdx]; classIdx < bestClassTableBo[compIdx] + SAO_BO_LEN; classIdx++)
                {
                    estDist[compIdx] += currentDistortionTableBo[classIdx];
                }
            }
        }
        else
        {
            estDist[0] = estSaoTypeDist(1, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
            estDist[1] = estSaoTypeDist(2, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
        }

        m_rdGoOnSbacCoder->load(m_rdSbacCoders[0][CI_TEMP_BEST]);
        m_rdGoOnSbacCoder->resetBits();

        for (int compIdx = 0; compIdx < 2; compIdx++)
        {
            resetSaoCtuParam(&saoCtuParamRdo[compIdx]);
			saoCtuParamRdo[compIdx].typeIdx = typeIdx;
			saoCtuParamRdo[compIdx].mergeLeftFlag = 0;
			saoCtuParamRdo[compIdx].mergeUpFlag = 0;
			saoCtuParamRdo[compIdx].bandIdx = (typeIdx == SAO_BO) ? bestClassTableBo[compIdx] : 0;
			for (classIdx = 0; classIdx < 4; classIdx++)
            {
				saoCtuParamRdo[compIdx].offset[classIdx] = (int)m_offset[compIdx + 1][typeIdx][classIdx + saoCtuParamRdo[compIdx].bandIdx + 1];
            }

			m_entropyCoder->encodeSaoOffset(&saoCtuParamRdo[compIdx], compIdx + 1);
        }

        estRate = m_entropyCoder->getNumberOfWrittenBits();
        m_cost[1][typeIdx] = (double)((double)(estDist[0] + estDist[1])  + lambda * (double)estRate);

        if (m_cost[1][typeIdx] < costPartBest)
        {
            costPartBest = m_cost[1][typeIdx];
			copySaoCtuParam(saoCtuParam[0], &saoCtuParamRdo[0]);
			copySaoCtuParam(saoCtuParam[1], &saoCtuParamRdo[1]);
            bestDist = (estDist[0] + estDist[1]);
        }
    }

    distortion[0] += ((double)bestDist / lambda);
#if SAO_DBG
	assert(distortion[0] == m_infoFromX265->dist[m_infoFromX265->ctuPosX][0][1]);
#endif
    m_rdGoOnSbacCoder->load(m_rdSbacCoders[0][CI_TEMP_BEST]);
	m_entropyCoder->encodeSaoOffset(saoCtuParam[0], 1);
	m_entropyCoder->encodeSaoOffset(saoCtuParam[1], 2);
    m_rdGoOnSbacCoder->store(m_rdSbacCoders[0][CI_TEMP_BEST]);

    // merge left or merge up

    for (int idxNeighbor = 0; idxNeighbor < 2; idxNeighbor++)
    {
        for (int compIdx = 0; compIdx < 2; compIdx++)
        {
            neighborCtuParam[compIdx] = NULL;
            if (allowMergeLeft && idxNeighbor == 0) // merge Left
            {
                neighborCtuParam[compIdx] = leftCtuParam[compIdx];
            }
            else if (allowMergeUp && idxNeighbor == 1) // merge Up
            {
				neighborCtuParam[compIdx] = upCtuParam[compIdx];
            }
            if (neighborCtuParam[compIdx] != NULL)
            {
                estDist[compIdx] = 0;
                typeIdx = neighborCtuParam[compIdx]->typeIdx;
                if (typeIdx >= 0)
                {
					int mergeBandPosition = (typeIdx == SAO_BO) ? neighborCtuParam[compIdx]->bandIdx : 0;
                    int merge_iOffset;
                    for (classIdx = 0; classIdx < m_numClass[typeIdx]; classIdx++)
                    {
                        merge_iOffset = neighborCtuParam[compIdx]->offset[classIdx];
                        estDist[compIdx]   += estSaoDist(m_count[compIdx + 1][typeIdx][classIdx + mergeBandPosition + 1], merge_iOffset, m_offsetOrg[compIdx + 1][typeIdx][classIdx + mergeBandPosition + 1],  shift);
                    }
                }
                else
                {
                    estDist[compIdx] = 0;
                }

                copySaoCtuParam(saoMergeParam[compIdx][idxNeighbor], neighborCtuParam[compIdx]);
                saoMergeParam[compIdx][idxNeighbor]->mergeUpFlag   = idxNeighbor;
                saoMergeParam[compIdx][idxNeighbor]->mergeLeftFlag = !idxNeighbor;
                distortion[idxNeighbor + 1] += ((double)estDist[compIdx] / lambda);
            }
        }
    }
#if SAO_DBG
	assert(distortion[1] == m_infoFromX265->dist[m_infoFromX265->ctuPosX][1][1]);
	assert(distortion[2] == m_infoFromX265->dist[m_infoFromX265->ctuPosX][2][1]);
#endif
}

//! \}
