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
 \file     hevcSAO.h
 \brief    estimation part of sample adaptive offset class (header)
 */

#ifndef _hevcSAO_H
#define _hevcSAO_H

#include "TLibCommon/TComSampleAdaptiveOffset.h"
#include "TLibCommon/TComPic.h"

#include "TLibEncoder/TEncEntropy.h"
#include "TLibEncoder/TEncSbac.h"
#include "TLibCommon/TComBitCounter.h"

//! \ingroup TLibEncoder
//! \{



// ====================================================================================================================
// Class definition
// ====================================================================================================================

namespace x265 {
// private namespace

#define  SAO_DBG		1 // debug operations both in hevcSAO and TEncSampleAdaptiveOffset



/* constant definitions*/
#define  MAX_NUM_HOR_CTU 30
#define  MAX_NUM_VER_CTU 30
#define  SAO_CTU_SIZE    64 //support 32 and 64



	/* Global */
	typedef struct
	{
        int typeIdx; // (-1)noSAO, (0)EO_0, (1)EO_1, (2)EO_2, (3)EO_3, (4)BO
		int offset[4];
		int bandIdx;
	}SaoGlobalCtuParam;

	typedef struct
	{
		uint8_t 			frameLineBuffer[3][MAX_NUM_HOR_CTU * 64];
		uint8_t 			ctuLineBuffer[3][64];
		SaoGlobalCtuParam 	frameLineParam[3][MAX_NUM_HOR_CTU];
		SaoGlobalCtuParam 	ctuParam[3];
		int 				cnt; //for mergeLeft case
	}SaoGlobalParam;

	typedef struct  
	{
		SaoGlobalCtuParam	frameLineParam[3][MAX_NUM_HOR_CTU];
		SaoGlobalCtuParam	ctuParam[3];

		uint8_t 			frameLineBuffer[3][MAX_NUM_HOR_CTU * 64]; //last row in up CTU, for merge up
		uint8_t				upCtuLineBuffer[3][65]; //current CTU needed buffer, the [64]th is extra pixel(left_top)
		uint8_t				leftCtuLineBuffer[3][64]; //last col in left CTU, for merge left 
	}SaoRdoParam;

	typedef struct
	{
		bool			bSaoFlag[2];
		SAOQTPart*		saoPart[3];
		int        		maxSplitLevel;
		bool         	oneUnitFlag[3];
		SaoLcuParam* 	saoLcuParam[3];
		int          	numCuInHeight;
		int          	numCuInWidth;
	}SaoParam;

#if 0
    typedef struct
    {
        int				typeIdx[3]; //luma, chroma
		int				offset[3][4];
		int				bandIdx[3];
		bool 			mergeLeftFlag[3];
		bool			mergeUpFlag[3];
    }SaoCtuParam;
#endif   

	typedef struct
	{
		// input
		uint8_t			inRecPixelY[SAO_CTU_SIZE*SAO_CTU_SIZE];   //Y
		uint8_t			inRecPixelU[SAO_CTU_SIZE*SAO_CTU_SIZE/4]; //U
		uint8_t			inRecPixelV[SAO_CTU_SIZE*SAO_CTU_SIZE/4]; //V
		uint8_t			orgPixelY[SAO_CTU_SIZE*SAO_CTU_SIZE];     //Y
		uint8_t			orgPixelU[SAO_CTU_SIZE*SAO_CTU_SIZE/4];   //U
		uint8_t			orgPixelV[SAO_CTU_SIZE*SAO_CTU_SIZE/4];   //V
		uint8_t			ctuPosX; //CTU X coordinate in PIC (colIdx)
		uint8_t			ctuPosY; //CTU Y coordinate in PIC (rowIdx)
		double			lumaLambda; //getFrom rdoSaoUnitRow()
		double			chromaLambda; //getFrom rdoSaoUnitRow()
		uint8_t			size;		// CTU size, support 32 and 64
		uint8_t			validWidth; // valid width of current CTU
		uint8_t			validHeight; // valid height of current CTU

		//temp input
		bool			bLastCol; // current CTU belong to LastCol in PIC
		bool			bLastRow; // current CTU belong to LastRow in PIC

		//output
		uint8_t			outRecPixelY[SAO_CTU_SIZE*SAO_CTU_SIZE];
		uint8_t			outRecPixelU[SAO_CTU_SIZE*SAO_CTU_SIZE / 4];
		uint8_t			outRecPixelV[SAO_CTU_SIZE*SAO_CTU_SIZE / 4];       
        SaoCtuParam     saoCtuParam[3];

		//debugInfo
		double 			compDist[3];
		double			notMergeBestCost;
		double			mergeLeftCost;
		double			mergeUpCost;
	}InfoForSAO;


	typedef struct
	{
		// input
		uint8_t			inRecPixelY[SAO_CTU_SIZE*SAO_CTU_SIZE];   //Y
		uint8_t			inRecPixelU[SAO_CTU_SIZE*SAO_CTU_SIZE/4]; //U
		uint8_t			inRecPixelV[SAO_CTU_SIZE*SAO_CTU_SIZE/4]; //V
		uint8_t			orgPixelY[SAO_CTU_SIZE*SAO_CTU_SIZE];     //Y
		uint8_t			orgPixelU[SAO_CTU_SIZE*SAO_CTU_SIZE/4];   //U
		uint8_t			orgPixelV[SAO_CTU_SIZE*SAO_CTU_SIZE/4];   //V
		uint8_t			ctuPosX; //CTU X coordinate in PIC (colIdx)
		uint8_t			ctuPosY; //CTU Y coordinate in PIC (rowIdx)
		double			lumaLambda; //getFrom rdoSaoUnitRow()
		double			chromaLambda; //getFrom rdoSaoUnitRow()
		uint8_t			size;
		uint8_t			validWidth; // valid width of current CTU
		uint8_t			validHeight; // valid height of current CTU

		bool			bLastCol; // current CTU belong to LastCol in PIC
		bool			bLastRow; // current CTU belong to LastRow in PIC
		uint8_t			inRecPixelLeftY[SAO_CTU_SIZE]; // last col in left CTU, Y component
		uint8_t			inRecPixelLeftU[SAO_CTU_SIZE/4]; // last col in left CTU, U component
		uint8_t			inRecPixelLeftV[SAO_CTU_SIZE/4]; // last col in left CTU, V component
		uint8_t			inRecPixelUpY[SAO_CTU_SIZE]; // last row in up CTU, Y component
		uint8_t			inRecPixelUpU[SAO_CTU_SIZE]; // last row in up CTU, U component
		uint8_t			inRecPixelUpV[SAO_CTU_SIZE]; // last row in up CTU, V component
		uint8_t			inRecPixelExtraY[2];			// extra needed pixels, [0]: left_top, [1]:left_below
		uint8_t			inRecPixelExtraU[2];			// extra needed pixels, [0]: left_top, [1]:left_below
		uint8_t			inRecPixelExtraV[2];			// extra needed pixels, [0]: left_top, [1]:left_below

		//output
		uint8_t			outRecPixelY[SAO_CTU_SIZE*SAO_CTU_SIZE];
		uint8_t			outRecPixelU[SAO_CTU_SIZE*SAO_CTU_SIZE/4];
		uint8_t			outRecPixelV[SAO_CTU_SIZE*SAO_CTU_SIZE/4];
        SaoCtuParam     saoCtuParam[MAX_NUM_HOR_CTU][3];

		//debugInfo
		double 			compDist[MAX_NUM_HOR_CTU][3];
		double			notMergeBestCost[MAX_NUM_HOR_CTU];
		double			mergeLeftCost[MAX_NUM_HOR_CTU];
		double			mergeUpCost[MAX_NUM_HOR_CTU];
        
		//temp DebugInfo, aren't get by get functions
		int64_t			eoEstDist[MAX_NUM_HOR_CTU][4];
		int64_t			boEstDist[MAX_NUM_HOR_CTU];
		double			estRate[MAX_NUM_HOR_CTU][5]; //estBits of 5 types (notMerge)
		double			dist[MAX_NUM_HOR_CTU][3][2]; // [3]: notMerge,mergeLeft, mergeUp;  [2]: Luma, Chroma
        uint32_t        rate[MAX_NUM_HOR_CTU];  // tmp used

	}InfoFromX265;



	class hevcSAO : public TComSampleAdaptiveOffset
	{
	private:

	//     TEncEntropy*      m_entropyCoder;
	//     TEncSbac***       m_rdSbacCoders;            ///< for CABAC
	//     TEncSbac*         m_rdGoOnSbacCoder;
	//     TEncBinCABAC***   m_binCoderCABAC;          ///< temporal CABAC state storage for RD computation

		int64_t  ***m_count;    //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
		int64_t  ***m_offset;   //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
		int64_t  ***m_offsetOrg; //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
		int64_t(*m_countPreDblk)[3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];    //[LCU][YCbCr][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
		int64_t(*m_offsetOrgPreDblk)[3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]; //[LCU][YCbCr][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
		int64_t  **m_rate;      //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
		int64_t  **m_dist;      //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
		double **m_cost;      //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
		double *m_costPartBest; //[MAX_NUM_SAO_PART];
		int64_t  *m_distOrg;    //[MAX_NUM_SAO_PART];
		int    *m_typePartBest; //[MAX_NUM_SAO_PART];
		int     m_offsetThY;
		int     m_offsetThC;
		double  m_depthSaoRate[2][4];

	public:

		TEncEntropy*      m_entropyCoder;
		TEncSbac***       m_rdSbacCoders;            ///< for CABAC
		TEncSbac*         m_rdGoOnSbacCoder;
		TEncBinCABAC***   m_binCoderCABAC;          ///< temporal CABAC state storage for RD computation

		double  lumaLambda;
		double  chromaLambda;
		int     depth;
		int     numNoSao[2];

		hevcSAO();
		virtual ~hevcSAO();

        /* actually not used [begin]*/
        void getSaoStats(SAOQTPart *psQTPart, int yCbCr);
		void SAOProcess(SAOParam *saoParam);
        /* actually not used [end] */
        
		void startSaoEnc(TComPic* pic, TEncEntropy* entropyCoder, TEncSbac* rdGoOnSbacCoder);
		void endSaoEnc();
		void resetStats();


		void runQuadTreeDecision(SAOQTPart *psQTPart, int partIdx, double &costFinal, int maxLevel, double lambda, int yCbCr);
		void rdoSaoOnePart(SAOQTPart *psQTPart, int partIdx, double lambda, int yCbCr);

		void disablePartTree(SAOQTPart *psQTPart, int partIdx);
		void calcSaoStatsBlock(Pel* recStart, Pel* orgStart, int stride, int64_t** stats, int64_t** counts, uint32_t width, uint32_t height, bool* bBorderAvail, int yCbCr);
		void calcSaoStatsRowCus_BeforeDblk(TComPic* pic, int idxY);
		void destroyEncBuffer();
		void createEncBuffer();
		void assignSaoUnitSyntax(SaoLcuParam* saoLcuParam,  SAOQTPart* saoPart, bool &oneUnitFlag);
		void checkMerge(SaoLcuParam* lcuParamCurr, SaoLcuParam * lcuParamCheck, int dir);
		inline int64_t estSaoDist(int64_t count, int64_t offset, int64_t offsetOrg, int shift);
		inline int64_t estIterOffset(int typeIdx, int classIdx, double lambda, int64_t offsetInput, int64_t count, int64_t offsetOrg, int shift, int bitIncrease, int32_t *currentDistortionTableBo, double *currentRdCostTableBo, int offsetTh);
		inline int64_t estSaoTypeDist(int compIdx, int typeIdx, int shift, double lambda, int32_t *currentDistortionTableBo, double *currentRdCostTableBo);
		void setMaxNumOffsetsPerPic(int val) { m_maxNumOffsetsPerPic = val; }

		int  getMaxNumOffsetsPerPic() { return m_maxNumOffsetsPerPic; }

		void rdoSaoUnitRowInit(SAOParam *saoParam);
		void rdoSaoUnitRowEnd(SAOParam *saoParam, int numlcus);
		/************************************************************************/
		/*                         HWC debug                                    */
		/************************************************************************/
		//main functions
		void creatHWC();
		void destoryHWC();
        void calcCtuSaoStats(int yCbCr);    
        void calcCtuSaoStatsAll(int yCbCr);
		void calcLumaDist(int allowMergeLeft, int allowMergeUp, double lambda, SaoCtuParam* mergeCtuParam, double *distortion);
		void calcChromaDist(int allowMergeLeft, int allowMergeUp,  double lambda, SaoCtuParam *crSaoParam, SaoCtuParam *cbSaoParam, double *distortion);
		void RDO(SaoParam* saoParam, int addr);

        void resetSaoCtuParam(SaoCtuParam* saoCtuParam);
        void copySaoCtuParam(SaoCtuParam* dst, SaoCtuParam* src);
		void copySaoCtuParam(SaoCtuParam* dst, SaoGlobalCtuParam* src);
		// tool function
		void printRecon(FILE* fp, uint8_t* rec, uint8_t width, uint8_t height);
        void copyGlobalSaoCtuParam(SaoGlobalCtuParam* dst, SaoGlobalCtuParam* src);
        void copyGlobalSaoCtuParam(SaoLcuParam* dst, SaoGlobalCtuParam* src);
        
		//data definitions
		InfoForSAO*		m_infoForSAO;
		InfoFromX265*	m_infoFromX265;
		SaoParam*		m_saoParam; 
		SaoRdoParam*	m_saoRdoParam;
		uint8_t			m_frameLineBuffer[3][1920]; //temp use for debug
		
		// get info from x265
		void getSaoParamFromX265(SAOParam* saoParam);
		void setSaoParamToX265(SAOParam * saoParam, int addr);
		void getInfoFromX265_0(int colIdx, int rowIdx, double lumaLambda, double chromaLambda, int ctuWidth,
			uint32_t lPelx, uint32_t tPely, int picWidth, int picHeight, uint8_t* inRecPixelY, uint8_t* orgPixelY, int lumaStride,
			uint8_t* inRecPixelU, uint8_t* orgPixelU, uint8_t* inRecPixelV, uint8_t* orgPixelV, int chromaStride,
			uint8_t* inRecPixelLeftY, uint8_t* inRecPixelLeftU, uint8_t* inRecPixelLeftV, uint8_t* inRecPixelUpY, uint8_t* inRecPixelUpU, uint8_t* inRecPixelUpV,
			double compDist[3], SaoLcuParam* saoLcuParamY, SaoLcuParam* saoLcuParamU, SaoLcuParam* saoLcuParamV);

		void getFromX265();

        // set info to x265, temp interfaces
        
		//get info from HWC
        void setSaoParamToX265(SAOParam* saoParam);
		
		// Global data maintain
		void initGlobalParam();
		void setGlobalParam(int colIdx, int addr);
		void resetGlobalParam();

		void resetRdoParam(int colIdx);
		void setRdoParam(int colIdx, int addr);
		void setRdoParam(uint8_t* inRecAfterDblkY, uint8_t* inRecAfterDblkU, uint8_t*  inRecAfterDblkV);

		//void getGlobalParam(SaoLcuParam* saoLcuParamLeft, SaoLcuParam* saoLcuParamUp);
		// compare Results
		int compareX265andHWC(); //CTU Row level
		int compareX265andHWC(int colIdx);//CTU level

		FILE* m_fpSaoLogX265;
		FILE* m_fpSaoLogHWC;
	};
}
//! \}

#endif // ifndef X265_hevcSAO_H
