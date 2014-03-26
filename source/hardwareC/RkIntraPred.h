#ifndef __RK_INTRAPRED_H__
#define __RK_INTRAPRED_H__


// Include files
#include <stdint.h>
#include <math.h>
#include "module_interface.h"

#include "../Lib/TLibCommon/TypeDef.h"
#include "x265.h" 
#include <stdio.h>
#include "qt.h"

//! \ingroup TLibEncoder
//! \{


namespace x265 {
// private namespace
extern unsigned char IntraFilterType[][35];

typedef enum X265_AND_RKs   // Routines can be indexed using log2n(width)
{
    X265_COMPENT,
    RK_COMPENT,
    X265_AND_RK
};

#define   SIZE_4x4     4
#define   SIZE_8x8     8
#define   SIZE_16x16   16
#define   SIZE_32x32   32

#define   TEST_LUMA_DEPTH       0
#define   TEST_LUMA_SIZE        SIZE_32x32
#define   TEST_CHROMA_DEPTH     0
#define   TEST_CHROMA_SIZE      SIZE_16x16

typedef struct RkIntraSmooth
{  
    // smoothing [32x32 16x16 8x8],4x4不需要滤波
    // 模块输入是有效的边界数据,输出是滤波后的边界数据
    // buffer以最大的32x32为例，存储空间为 64+64+1 的line buf中
    // 这里和x265有个区别就是，不会存在64x64的输入，上层一定会划分成32x32
	
    Pel rk_refAbove[65];
    Pel rk_refLeft[65];
    Pel rk_refAboveFiltered[X265_AND_RK][65];
    Pel rk_refLeftFiltered[X265_AND_RK][65];

} RkIntraSmooth;


typedef struct RkIntraPred_35
{  
    // PU [32x32 16x16 8x8 4x4]
	//ALIGN_VAR_32(Pel, rk_predSample[33 * 32 * 32]);
	//ALIGN_VAR_32(Pel, rk_predSampleOrg[33 * 32 * 32]);

    Pel rk_predSample[32*32];
    Pel rk_predSampleOrg[32*32];
    Pel rk_predSampleTmp[35][32*32];
    Pel rk_predSampleCb[16*16];
    Pel rk_predSampleCr[16*16];    
} RkIntraPred_35;

class Rk_IntraPred 
{
#ifdef CONNECT_QT
public:
    hevcQT*         rk_hevcqt;
    InfoQTandIntra  SendInfo2QtByIntra;
#endif
public:
    FILE *rk_logIntraPred[20];

    Pel             rk_LineBufTmp[129]; // 数据靠前存储 左下到左上到右上，corner点在中间

    Pel             rk_LineBuf[129];    // 数据靠前存储 左下到左上到右上，corner点在中间
    Pel             rk_LineBufCb[65];    // 数据靠前存储 左下到左上到右上，corner点在中间
    Pel             rk_LineBufCr[65];    // 数据靠前存储 左下到左上到右上，corner点在中间

	RkIntraSmooth   rk_IntraSmoothIn;
    bool            rk_bUseStrongIntraSmoothing;
    int32_t         rk_puHeight;
    int32_t         rk_puWidth;

    RkIntraPred_35  rk_IntraPred_35;
    int32_t         rk_puwidth_35;
    int32_t         rk_dirMode;

    // 记录角度预测中deltaFract系数，4层，32x32的最多有32个系数，35种模式
    int			    rk_verdeltaFract[32];
    int			    rk_hordeltaFract[32];
    int			    rk_verdeltaInt[32];
    int			    rk_hordeltaInt[32];
    bool            rk_bFlag32;
    bool            rk_bFlag16;
    bool            rk_bFlag8;
    bool            rk_bFlag4;
    
	Rk_IntraPred(void);
	~Rk_IntraPred(void);
// ------------------------------------------------------------------------------
/*
** 功能性模块
** 验证单个模块的正确性，数据接口通常是和x265一样，保证读取数据方便
*/
// ------------------------------------------------------------------------------

	void RkIntraSmoothing(Pel* refLeft,
								Pel* refAbove, 
								Pel* refLeftFlt, 
								Pel* refAboveFlt);
    void RkIntraSmoothingCheck();

	void RkIntraPred_PLANAR(Pel *predSample, 
				Pel 	*above, 
				Pel 	*left, 
				int 	stride, 
				int 	log2_size);
	void RkIntraPred_DC(Pel *predSample, 
			Pel 	*above, 
			Pel 	*left, 
			int 	stride, 
			int 	log2_size, 
			int 	c_idx);
    void RkIntraPred_angular(Pel *predSample, 
            	Pel *refAbove, 
            	Pel *refLeft,
            	int stride,            	
            	int log2_size,
            	int c_idx,
            	int dirMode);

    void RkIntraPredAll(Pel *predSample, 
            	Pel *refAbove, 
            	Pel *refLeft,
            	int stride,            	
            	int log2_size,
            	int c_idx,
            	int dirMode);
    
    void RkIntraPred_angularCheck();

    void RkIntrafillRefSamples(Pel* roiOrigin, 
							bool* bNeighborFlags, 
							int numIntraNeighbor, 
							int unitSize, 
							int numUnitsInCU, 
							int totalUnits, 
							int width,
							int height,
							int picStride,
							Pel* pLineBuf);

    void fillReferenceSamples(Pel* roiOrigin, 
							Pel* adiTemp, 
							bool* bNeighborFlags, 
							int numIntraNeighbor, 
							int unitSize, 
							int numUnitsInCU, 
							int totalUnits, 
							uint32_t cuWidth, 
							uint32_t cuHeight, 
							uint32_t width, 
							uint32_t height, 
							int picStride,
							uint32_t trDepth);

    void RkIntraFillRefSamplesCheck(Pel* above, Pel* left, int32_t width, int32_t heigth);
    void RkIntraFillChromaCheck(Pel* pLineBuf, Pel* above, Pel* left, uint32_t width, uint32_t heigth);
    void LogdeltaFractForAngluar();


    //void RkIntraPriorModeChoose(Pel* orgYuv, int width,	 intptr_t stride, int rdModeList[], int aboveMode, int leftMode);
    void RkIntraPriorModeChoose(int rdModeList[], uint32_t rdModeCost[], bool bsort);


// ------------------------------------------------------------------------------
/*
** 对外层支持，保证接口兼容性
**
*/
// ------------------------------------------------------------------------------

	void RkIntra_proc(INTERFACE_INTRA* pInterface_Intra, uint32_t partOffset);
    // 输入L型buf，输出L型buf
    void RK_IntraFillReferenceSamples(Pel* reconEdgePixel, 
							bool* bNeighborFlags,
							Pel* pLineBuf,
							int numIntraNeighbor, 
							int unitSize, 
							int totalUnits, 
							int width,
							int height);
    // 输入L型buf，输出left/above的分开存储
	void RK_IntraSmoothing(Pel* pLineBuf,
							int width,
						    int height,
						    bool bUseStrongIntraSmoothing,
                            Pel* refLeft,
							Pel* refAbove, 
							Pel* refLeftFlt, 
							Pel* refAboveFlt);

    int RK_IntraSad(Pel *pix1, 
                intptr_t stride_pix1, 
                Pel *pix2, 
                intptr_t stride_pix2, 
                uint32_t size);
    
	void RK_IntraCalcuResidual(Pel*org, 
				Pel *pred, 
				int16_t *residual, 
				intptr_t stride,
				uint32_t blockSize);

// ------------------------------------------------------------------------------
/*
** 验证数据流输出
*/
// ------------------------------------------------------------------------------

    int         rk_bits[35];
    uint64_t    rk_lambdaMotionSAD;
    int         rk_bestMode[X265_AND_RK][4];//一个输入块通常一个方向，8x8的时候可以有4个方向
    Pel         rk_pred[X265_AND_RK][32*32];
    Pel         rk_predCb[X265_AND_RK][32*32];
    Pel         rk_predCr[X265_AND_RK][32*32];
    
    int16_t     rk_residual[X265_AND_RK][32*32];
    int16_t     rk_residualCb[X265_AND_RK][16*16];
    int16_t     rk_residualCr[X265_AND_RK][16*16];

    uint64_t    rk_modeCostsSadAndCabac[35];

};
   

}
//! \}

#endif
