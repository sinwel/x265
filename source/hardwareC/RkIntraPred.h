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
    // smoothing [32x32 16x16 8x8],4x4����Ҫ�˲�
    // ģ����������Ч�ı߽�����,������˲���ı߽�����
    // buffer������32x32Ϊ�����洢�ռ�Ϊ 64+64+1 ��line buf��
    // �����x265�и�������ǣ��������64x64�����룬�ϲ�һ���Ữ�ֳ�32x32
	
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

    Pel             rk_LineBufTmp[129]; // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�

    Pel             rk_LineBuf[129];    // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�
    Pel             rk_LineBufCb[65];    // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�
    Pel             rk_LineBufCr[65];    // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�

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
** �����֧�֣���֤�ӿڼ�����
**
*/
// ------------------------------------------------------------------------------

	void RkIntra_proc(INTERFACE_INTRA* pInterface_Intra, uint32_t partOffset);
    // ����L��buf�����L��buf
    void RK_IntraFillReferenceSamples(Pel* reconEdgePixel, 
							bool* bNeighborFlags,
							Pel* pLineBuf,
							int numIntraNeighbor, 
							int unitSize, 
							int totalUnits, 
							int width,
							int height);
    // ����L��buf�����left/above�ķֿ��洢
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
** ��֤���������
*/
// ------------------------------------------------------------------------------

    int         rk_bits[35];
    uint64_t    rk_lambdaMotionSAD;
    int         rk_bestMode[X265_AND_RK][4];//һ�������ͨ��һ������8x8��ʱ�������4������
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
