#ifndef __RK_INTRAPRED_H__
#define __RK_INTRAPRED_H__


// Include files
#include <stdint.h>
#include <math.h>
#include <stdio.h>

#include "module_interface.h"
#include "macro.h" 
#include "qt.h"


//#define RK_INTRA_MODE_CHOOSE


//! \ingroup TLibEncoder
//! \{


namespace RK_HEVC {
// private namespace
extern unsigned char IntraFilterType[][35];

typedef enum X265_AND_RKs   // Routines can be indexed using log2n(width)
{
    X265_COMPENT,
    RK_COMPENT,
    X265_AND_RK
};

#define SIZE_4x4                4
#define SIZE_8x8                8
#define SIZE_16x16              16
#define SIZE_32x32              32
#define SIZE_64x64              64

#define TEST_LUMA_DEPTH         0
#define TEST_LUMA_SIZE          SIZE_32x32
#define TEST_CHROMA_DEPTH       0
#define TEST_CHROMA_SIZE        SIZE_16x16

#define RK_MAX_CU_DEPTH         6                           // log2(LCUSize)
#define RK_MAX_CU_SIZE          (1 << (RK_MAX_CU_DEPTH))       // maximum allowable size of CU
#define RK_MIN_PU_SIZE          4
#define RK_MAX_NUM_SPU_W        (RK_MAX_CU_SIZE / RK_MIN_PU_SIZE) // maximum number of SPU in horizontal line
#define RK_ADI_BUF_STRIDE       (2 * RK_MAX_CU_SIZE + 1 + 15)  // alignment to 16 bytes

typedef uint8_t RK_Pel;          // 16-bit pixel type
#define RK_DEPTH                8

/** clip x, such that 0 <= x <= #g_maxLumaVal */
template<typename T>
inline T RK_ClipY(T x) { return std::min<T>(T((1 << RK_DEPTH) - 1), std::max<T>(T(0), x)); }

template<typename T>
inline T RK_ClipC(T x) { return std::min<T>(T((1 << RK_DEPTH) - 1), std::max<T>(T(0), x)); }

/** clip a, such that minVal <= a <= maxVal */
template<typename T>
inline T RK_Clip3(T minVal, T maxVal, T a) { return std::min<T>(std::max<T>(minVal, a), maxVal); } ///< general min/max clip


typedef struct RkIntraSmooth
{  
    // smoothing [32x32 16x16 8x8],4x4����Ҫ�˲�
    // ģ����������Ч�ı߽�����,������˲���ı߽�����
    // buffer������32x32Ϊ�����洢�ռ�Ϊ 64+64+1 ��line buf��
    // �����x265�и�������ǣ��������64x64�����룬�ϲ�һ���Ữ�ֳ�32x32
	
    RK_Pel rk_refAbove[65];
    RK_Pel rk_refLeft[65];
    RK_Pel rk_refAboveFiltered[X265_AND_RK][65];
    RK_Pel rk_refLeftFiltered[X265_AND_RK][65];

} RkIntraSmooth;


typedef struct RkIntraPred_35
{  
    // PU [32x32 16x16 8x8 4x4]
	//ALIGN_VAR_32(RK_Pel, rk_predSample[33 * 32 * 32]);
	//ALIGN_VAR_32(RK_Pel, rk_predSampleOrg[33 * 32 * 32]);

    RK_Pel rk_predSample[32*32];
    RK_Pel rk_predSampleOrg[32*32];
    RK_Pel rk_predSampleTmp[35][32*32];
    RK_Pel rk_predSampleCb[16*16];
    RK_Pel rk_predSampleCr[16*16];    
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

    RK_Pel             rk_LineBufTmp[129]; // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�

    RK_Pel             rk_LineBuf[129];    // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�
    RK_Pel             rk_LineBufCb[65];    // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�
    RK_Pel             rk_LineBufCr[65];    // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�

	RkIntraSmooth   rk_IntraSmoothIn;
    bool            rk_bUseStrongIntraSmoothing;
    int32_t         rk_puHeight;
    int32_t         rk_puWidth;

    RkIntraPred_35  rk_IntraPred_35;
    int32_t         rk_puwidth_35;
    int32_t         rk_dirMode;

    // ��¼�Ƕ�Ԥ����deltaFractϵ����4�㣬32x32�������32��ϵ����35��ģʽ
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
** ������ģ��
** ��֤����ģ�����ȷ�ԣ����ݽӿ�ͨ���Ǻ�x265һ������֤��ȡ���ݷ���
*/
// ------------------------------------------------------------------------------

	void RkIntraSmoothing(RK_Pel* refLeft,
								RK_Pel* refAbove, 
								RK_Pel* refLeftFlt, 
								RK_Pel* refAboveFlt);
    void RkIntraSmoothingCheck();

	void RkIntraPred_PLANAR(RK_Pel *predSample, 
				RK_Pel 	*above, 
				RK_Pel 	*left, 
				int 	stride, 
				int 	log2_size);
	void RkIntraPred_DC(RK_Pel *predSample, 
			RK_Pel 	*above, 
			RK_Pel 	*left, 
			int 	stride, 
			int 	log2_size, 
			int 	c_idx);
    void RkIntraPred_angular(RK_Pel *predSample, 
            	RK_Pel *refAbove, 
            	RK_Pel *refLeft,
            	int stride,            	
            	int log2_size,
            	int c_idx,
            	int dirMode);

    void RkIntraPredAll(RK_Pel *predSample, 
            	RK_Pel *refAbove, 
            	RK_Pel *refLeft,
            	int stride,            	
            	int log2_size,
            	int c_idx,
            	int dirMode);
    
    void RkIntraPred_angularCheck();

    void RkIntrafillRefSamples(RK_Pel* roiOrigin, 
							bool* bNeighborFlags, 
							int numIntraNeighbor, 
							int unitSize, 
							int numUnitsInCU, 
							int totalUnits, 
							int width,
							int height,
							int picStride,
							RK_Pel* pLineBuf);

    void fillReferenceSamples(RK_Pel* roiOrigin, 
							RK_Pel* adiTemp, 
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

    void RkIntraFillRefSamplesCheck(RK_Pel* above, RK_Pel* left, int32_t width, int32_t heigth);
    void RkIntraFillChromaCheck(RK_Pel* pLineBuf, RK_Pel* above, RK_Pel* left, uint32_t width, uint32_t heigth);
    void LogdeltaFractForAngluar();


    //void RkIntraPriorModeChoose(RK_Pel* orgYuv, int width,	 intptr_t stride, int rdModeList[], int aboveMode, int leftMode);
    void RkIntraPriorModeChoose(int rdModeList[], uint32_t rdModeCost[], bool bsort);


// ------------------------------------------------------------------------------
/*
** �����֧�֣���֤�ӿڼ�����
**
*/
// ------------------------------------------------------------------------------

	void RkIntra_proc(INTERFACE_INTRA* pInterface_Intra, uint32_t partOffset);
    // ����L��buf�����L��buf
    void RK_IntraFillReferenceSamples(RK_Pel* reconEdgePixel, 
							bool* bNeighborFlags,
							RK_Pel* pLineBuf,
							int numIntraNeighbor, 
							int unitSize, 
							int totalUnits, 
							int width,
							int height);
    // ����L��buf�����left/above�ķֿ��洢
	void RK_IntraSmoothing(RK_Pel* pLineBuf,
							int width,
						    int height,
						    bool bUseStrongIntraSmoothing,
                            RK_Pel* refLeft,
							RK_Pel* refAbove, 
							RK_Pel* refLeftFlt, 
							RK_Pel* refAboveFlt);

    int RK_IntraSad(RK_Pel *pix1, 
                intptr_t stride_pix1, 
                RK_Pel *pix2, 
                intptr_t stride_pix2, 
                uint32_t size);
    
	void RK_IntraCalcuResidual(RK_Pel*org, 
				RK_Pel *pred, 
				int16_t *residual, 
				intptr_t stride,
				uint32_t blockSize);

// ------------------------------------------------------------------------------
/*
** ��֤���������
*/
// ------------------------------------------------------------------------------

    int         rk_bits[35];
    uint64_t    rk_lambdaMotionSAD;
    int         rk_bestMode[X265_AND_RK][4];//һ�������ͨ��һ������8x8��ʱ�������4������
    RK_Pel         rk_pred[X265_AND_RK][32*32];
    RK_Pel         rk_predCb[X265_AND_RK][32*32];
    RK_Pel         rk_predCr[X265_AND_RK][32*32];
    
    int16_t     rk_residual[X265_AND_RK][32*32];
    int16_t     rk_residualCb[X265_AND_RK][16*16];
    int16_t     rk_residualCr[X265_AND_RK][16*16];

    uint64_t    rk_modeCostsSadAndCabac[35];
    uint64_t    rk_modeCostsSadAndCabacCorrect[35];

};
   

}
//! \}

#endif
