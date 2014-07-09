#ifndef __RK_INTRAPRED_H__
#define __RK_INTRAPRED_H__


// Include files
#include <stdint.h>
#include <math.h>

#include "module_interface.h"
#include "macro.h"
#include "qt.h"
#include "rk_define.h"
#include "CABAC.h"

#include <stdio.h>
#include <assert.h>
#include <algorithm>


extern const int rk_lambdaMotionSSE_tab_I[MAX_QP + 1];
extern const int rk_lambdaMotionSSE_tab_non_I[MAX_QP + 1];



//! \ingroup TLibEncoder
//! \{

#define FAST_MODE_STEP 1

//namespace RK_HEVC {
// private namespace

typedef enum CAND_4X4_NUMs   // Routines can be indexed using log2n(width)
{
    PART_0_RIGHT,
    PART_0_BOTTOM,
    PART_1_BOTTOM,
    PART_2_RIGHT,
    RECON_EDGE_4X4_NUM
}CAND_4X4_NUMs;

extern uint32_t g_intra_pu_lumaDir_bits[5][256][35];
extern uint32_t g_intra_depth_total_bits[5][256];

extern uint8_t g_4x4_single_reconEdge[RECON_EDGE_4X4_NUM][4];
// ÿ�� 8x8 ��block �� 4�� 4���صı�
// ��һά�ķ����±�Ϊ zorder/4
extern uint8_t g_4x4_total_reconEdge[64][RECON_EDGE_4X4_NUM][4];

extern  INTERFACE_INTRA g_previous_8x8_intra;


typedef struct RkIntraSmooth
{
    // smoothing [32x32 16x16 8x8],4x4����Ҫ�˲�
    // ģ����������Ч�ı߽�����,������˲���ı߽�����
    // buffer������32x32Ϊ�����洢�ռ�Ϊ 64+64+1 ��line buf��
    // �����x265�и�������ǣ��������64x64�����룬�ϲ�һ���Ữ�ֳ�32x32

    uint8_t rk_refAbove[65];
    uint8_t rk_refLeft[65];
    uint8_t rk_refAboveFiltered[X265_AND_RK][65];
    uint8_t rk_refLeftFiltered[X265_AND_RK][65];

} RkIntraSmooth;


typedef struct RkIntraPred_35
{
    // PU [32x32 16x16 8x8 4x4]
	//ALIGN_VAR_32(uint8_t, rk_predSample[33 * 32 * 32]);
	//ALIGN_VAR_32(uint8_t, rk_predSampleOrg[33 * 32 * 32]);

    uint8_t rk_predSample[32*32];
    uint8_t rk_predSampleOrg[32*32];
    uint8_t rk_predSampleTmp[35][32*32];
    uint8_t rk_predSampleCb[16*16];
    uint8_t rk_predSampleCr[16*16];
} RkIntraPred_35;

typedef enum Intra4HardwareFile
{
    INTRA_4_ORI_PIXEL         , //����y cb y y cr y��˳���ӡ
    INTRA_4_REF_PIXEL         , //����y cb y y cr y��˳����д�ӡ
    INTRA_4_SAD               , //����y y y y��˳����д�ӡ 
    INTRA_4_CABAC_MODE_BIT    , //����y y y y��˳����д�ӡ 
    INTRA_4_ORI_PIXEL_CU_LU   , // 8x8 CU ����64������һ�е�˳���ӡ����ӡ˳��p[63]p[62]p[61]...p[1]p[0]
    INTRA_4_ORI_PIXEL_CU_CB   , // 8x8 CU ����16������һ�е�˳���ӡ����ӡ˳��p[15]p[14]p[13]...p[1]p[0]
    INTRA_4_ORI_PIXEL_CU_CR   , // 8x8 CU ����16������һ�е�˳���ӡ����ӡ˳��p[15]p[14]p[13]...p[1]p[0]
    INTRA_4_REF_PIXEL_CU_LU   , //8x8 CU��ref pixel lu���� 33������һ�� ��ӡ˳��p[32]p[31]p[23]...p[1]p[0]
    INTRA_4_REF_PIXEL_CU_CB   , //8x8 CU��ref pixel cb���� 17������һ�� ��ӡ˳��p[16]p[15]p[14]...p[1]p[0]
    INTRA_4_REF_PIXEL_CU_CR   , //8x8 CU��ref pixel cr���� 17������һ�� ��ӡ˳��p[16]p[15]p[14]...p[1]p[0]
    INTRA_4_REF_CU_VALID      , //8x8 CU��ref cu valid�ź� ����5bitһ�е�˳���ӡ[4][3][2][1][0]
    INTRA_4_RESI_BEFORE       , //����16������һ�е�˳���ӡ��ÿ������λ��Ϊ16bit����ӡ˳��p[15]p[14]p[13]...p[1]p[0];����y cb y y cr y��˳���ӡ
    INTRA_4_RESI_AFTER        ,
    INTRA_4_PRED              , //����16������һ�е�˳���ӡ��ÿ������λ��Ϊ8bit����ӡ˳��p[15],p[14],p[13]...p[1],p[0];����y cb y y cr y ��˳���ӡ
    INTRA_4_RECON             , //����16������һ�е�˳���ӡ��ÿ������λ��Ϊ8bit����ӡ˳��p[15],p[14],p[13]...p[1],p[0];����y cb y y cr y ��˳���ӡ
    INTRA_4_BEST_MODE         , //����8bitһ�н��д�ӡ, ����y cb y y cr y��˳���ӡ
    INTRA_4_TU_CABAC_BITS   	, //����һ��tu �в32 bitһ�е�˳����д�ӡ����y cb y y cr y��˳���ӡ 32 bit x 6
    INTRA_4_PU_PRED_MODE_BITS   , //���� 32 bitһ�е����ݽ��д�ӡ,����y cb y y cr y��˳����д�ӡ
    INTRA_4_CU_TOTAL_BITS       ,
    INTRA_4_CU_TOTAL_DISTORTION ,
    INTRA_4_CU_TOTAL_COST       ,
    INTRA_4_TU_COST_BITS        ,
    INTRA_4_TU_CBF_BITS         , //���� 24 bit��ӡ ��y cb y y cr y��˳���ӡ һ��TU һ��
    INTRA_4_PU_PART_MODE_BITS	, //���� 24 bitһ�е�˳����д�ӡ,һ��PUһ��    
    // ------ TQ file ----------- //
    INTRA_4_T_INPUT4X4DATA         ,
    INTRA_4_QIT_OUTPUT4X4DATA      ,
    
    INTRA_4_FILE_NUM
} Intra4HardwareFile;

typedef enum Intra8HardwareFile
{
    INTRA_8_SAD               , //����18��һ�� y ��˳����д�ӡ 0 sad[0], 1 sad[1], 2 sad[2], 4 sad[4], ... 34 sad[34]
    INTRA_8_CABAC_MODE_BIT    , //����18��һ�� y ��˳����д�ӡ [bit34][bit32]...[bit2][bit1][bit0] 
    INTRA_8_ORI_PIXEL_CU_LU   , // 8x8 CU ����64������һ�е�˳���ӡ����ӡ˳��p[63]p[62]p[61]...p[1]p[0]
    INTRA_8_ORI_PIXEL_CU_CB   , // 8x8 CU ����16������һ�е�˳���ӡ����ӡ˳��p[15]p[14]p[13]...p[1]p[0]
    INTRA_8_ORI_PIXEL_CU_CR   , // 8x8 CU ����16������һ�е�˳���ӡ����ӡ˳��p[15]p[14]p[13]...p[1]p[0]
    INTRA_8_REF_PIXEL_CU_LU   , //8x8 CU��ref pixel lu���� 33������һ�� ��ӡ˳��p[32]p[31]p[23]...p[1]p[0]
    INTRA_8_REF_PIXEL_CU_CB   , //8x8 CU��ref pixel cb���� 17������һ�� ��ӡ˳��p[16]p[15]p[14]...p[1]p[0]
    INTRA_8_REF_PIXEL_CU_CR   , //8x8 CU��ref pixel cr���� 17������һ�� ��ӡ˳��p[16]p[15]p[14]...p[1]p[0]
    INTRA_8_REF_CU_VALID      , //8x8 CU��ref cu valid�ź� ����5bitһ�е�˳���ӡ[4][3][2][1][0]
    INTRA_8_RESI_BEFORE       , //����16������һ�е�˳���ӡ��ÿ������λ��Ϊ16bit����ӡ˳��p[15]p[14]p[13]...p[1]p[0];���� y cb cr ��˳���ӡ
    INTRA_8_RESI_AFTER        , //����16������һ�е�˳���ӡ��ÿ������λ��Ϊ16bit����ӡ˳��p[15]p[14]p[13]...p[1]p[0];���� y cb cr ��˳���ӡ 
    INTRA_8_PRED              , //����16������һ�е�˳���ӡ��ÿ������λ��Ϊ8bit����ӡ˳��p[15],p[14],p[13]...p[1],p[0];����y cb cr��˳���ӡ 
    INTRA_8_RECON             , //����16������һ�е�˳���ӡ��ÿ������λ��Ϊ8bit����ӡ˳��p[15],p[14],p[13]...p[1],p[0];����y cb cr��˳���ӡ 
    INTRA_8_BEST_MODE         , //����8bitһ�н��д�ӡ;����y cb cr��˳���ӡ 
    INTRA_8_TU_CABAC_BITS   	, //����һ��tu �в� 32 bitһ�е�˳����д�ӡ;����y cb cr ��˳���ӡ 
    INTRA_8_PU_PRED_MODE_BITS   , //���� 32 bitһ�е����ݽ��д�ӡ,����y cb&cr��˳����д�ӡ
    INTRA_8_CU_TOTAL_BITS       , //����24bitһ�е�˳����д�ӡ
    INTRA_8_CU_TOTAL_DISTORTION , //����32bitһ�е�˳����д�ӡ
    INTRA_8_CU_TOTAL_COST       , //����32bitһ�е�˳����д�ӡ
    INTRA_8_TU_COST_BITS		, //����һ��Y������TU��(sad + sad_lambda * bit)��ӡ;˳��0 cost[0], 1 cost[1], 2 cost[2], 4 cost[4], ... 34 cost[34];cost��32bit��ӡ
    INTRA_8_TU_CBF_BITS         , //���� 24 bit��ӡ ��y cb cr ��˳���ӡ һ��TU һ��
    INTRA_8_PU_PART_MODE_BITS	, //���� 24 bitһ�е�˳����д�ӡ,һ��PUһ��
    INTRA_8_REF_PIXEL_CU_LU_FILTER  ,//8x8 CU��ref pixel lu���� filter������ 33������һ�� ��ӡ˳��p[32]p[31]p[23]...p[1]p[0]
    INTRA_8_FILE_NUM
} Intra8HardwareFile;

typedef enum Intra16HardwareFile
{
    INTRA_16_SAD               , //����35��һ�� y ��˳����д�ӡ 0 sad[0], 1 sad[1], 2 sad[2], 3 sad[3], ... 34 sad[34]
    INTRA_16_CABAC_MODE_BIT    , //����35��һ�� y ��˳����д�ӡ [bit34][bit33]...[bit2][bit1][bit0] 
    INTRA_16_ORI_PIXEL_CU_LU   , // 16x16 CU ����16 x 16������һ�е�˳���ӡ����ӡ˳��p[63]p[62]p[61]...p[1]p[0]
    INTRA_16_ORI_PIXEL_CU_CB   , // 16x16 CU ����8 x 8������һ�е�˳���ӡ����ӡ˳��p[15]p[14]p[13]...p[1]p[0]
    INTRA_16_ORI_PIXEL_CU_CR   , // 16x16 CU ����8 x 8������һ�е�˳���ӡ����ӡ˳��p[15]p[14]p[13]...p[1]p[0]
    INTRA_16_REF_PIXEL_CU_LU   , // 16x16 CU��ref pixel lu���� 65������һ�� ��ӡ˳��p[64]p[63]p[62]...p[1]p[0]
    INTRA_16_REF_PIXEL_CU_CB   , // 16x16 CU��ref pixel cb���� 33������һ�� ��ӡ˳��p[32]p[31]p[30]...p[1]p[0]
    INTRA_16_REF_PIXEL_CU_CR   , // 16x16 CU��ref pixel cr���� 33������һ�� ��ӡ˳��p[32]p[31]p[30]...p[1]p[0]
    INTRA_16_REF_CU_VALID      , // 16x16 CU��ref cu valid�ź� ����9bitһ�е�˳���ӡ[8][7][6][5][4][3][2][1][0]
    INTRA_16_RESI_BEFORE       , // Ԥ������в����� ����16������һ�е�˳���ӡ��ÿ������λ��Ϊ16bit����ӡ˳��p[15]p[14]p[13]...p[1]p[0];
								 //	cb����ƴ��1��	cr����ƴ��1�� 	����y cb cr��˳���ӡ
    INTRA_16_RESI_AFTER        , // ����IT֮��Ĳв����� ����16������һ�е�˳���ӡ��ÿ������λ��Ϊ16bit����ӡ˳��p[15]p[14]p[13]...p[1]p[0];
								 // cb����ƴ��1�� 	cr����ƴ��1�� 	����y cb cr��˳���ӡ
    INTRA_16_PRED              , // ����IT֮��Ĳв����� ����16������һ�е�˳���ӡ��ÿ������λ��Ϊ8bit����ӡ˳��p[15]p[14]p[13]...p[1]p[0];
								 // cb����ƴ��1�� 	cr����ƴ��1�� 	����y cb cr��˳���ӡ
    INTRA_16_RECON             , // ����IT֮��Ĳв����� ����16������һ�е�˳���ӡ��ÿ������λ��Ϊ8bit����ӡ˳��p[15]p[14]p[13]...p[1]p[0];
								 // cb����ƴ��1�� 	cr����ƴ��1�� 	����y cb cr��˳���ӡ
    INTRA_16_BEST_MODE         , // ����8bitһ�н��д�ӡ;����y cb cr��˳���ӡ 
    INTRA_16_TU_CABAC_BITS   	, //����һ��tu �в� 24 bitһ�е�˳����д�ӡ;����y cb cr ��˳���ӡ 
    INTRA_16_PU_PRED_MODE_BITS   , //���� 32 bitһ�е����ݽ��д�ӡ,����y cb cr��˳����д�ӡ
    INTRA_16_CU_TOTAL_BITS       , //����24bitһ�е�˳����д�ӡ
    INTRA_16_CU_TOTAL_DISTORTION , //����32bitһ�е�˳����д�ӡ
    INTRA_16_CU_TOTAL_COST       , //����32bitһ�е�˳����д�ӡ
    INTRA_16_TU_COST_BITS		, //����һ��Y������TU��(sad + sad_lambda * bit)��ӡ;˳��0 cost[0], 1 cost[1], 2 cost[2], 4 cost[4], ... 34 cost[34];cost��32bit��ӡ
    INTRA_16_TU_CBF_BITS         , //���� 24 bit��ӡ ��y cb cr ��˳���ӡ һ��TU һ��
    INTRA_16_PU_PART_MODE_BITS	, //���� 24 bitһ�е�˳����д�ӡ,һ��PUһ��
    INTRA_16_REF_PIXEL_CU_LU_FILTER  ,//16x16 CU��ref pixel lu���� �����˲����lu ref���� 65������һ�� ��ӡ˳��p[64]p[63]p[62]...p[1]p[0]

    INTRA_16_FILE_NUM
} Intra16HardwareFile;

class Rk_IntraPred
{
public:
#if RK_CABAC_H
	CABAC_RDO* m_cabac_rdo;
#endif
    FILE *rk_logIntraPred[20];
    FILE* fp_intra_4x4[INTRA_4_FILE_NUM];
    FILE* fp_intra_8x8[INTRA_8_FILE_NUM];
    FILE* fp_intra_16x16[INTRA_16_FILE_NUM];

    int     num_4x4fp;
    int     num_8x8fp;

	uint8_t             rk_OrgBufAll[6][129];
	uint8_t             rk_LineBufAll[6][129];
	int16_t             rk_resiAll[6][32*32]; 
	uint8_t             rk_predAll[6][32*32]; 
	int16_t             rk_resiOutAll[6][32*32]; 
	uint8_t             rk_reconAll[6][32*32]; 


    uint8_t             rk_LineBufTmp[129]; // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�

    uint8_t             rk_LineBuf[129];    // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�
    uint8_t             rk_LineBufCb[65];    // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�
    uint8_t             rk_LineBufCr[65];    // ���ݿ�ǰ�洢 ���µ����ϵ����ϣ�corner�����м�

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

	void RkIntraSmoothing(uint8_t* refLeft,
								uint8_t* refAbove,
								uint8_t* refLeftFlt,
								uint8_t* refAboveFlt);
    void RkIntraSmoothingCheck();

	void RkIntraPred_PLANAR(uint8_t *predSample,
				uint8_t 	*above,
				uint8_t 	*left,
				int 	stride,
				int 	log2_size);
	void RkIntraPred_DC(uint8_t *predSample,
			uint8_t 	*above,
			uint8_t 	*left,
			int 	stride,
			int 	log2_size,
			int 	c_idx);
    void RkIntraPred_angular(uint8_t *predSample,
            	uint8_t *refAbove,
            	uint8_t *refLeft,
            	int stride,
            	int log2_size,
            	int c_idx,
            	int dirMode);

    void RkIntraPredAll(uint8_t *predSample,
            	uint8_t *refAbove,
            	uint8_t *refLeft,
            	int stride,
            	int log2_size,
            	int c_idx,
            	int dirMode);

    void RkIntraPred_angularCheck();

    void RkIntrafillRefSamples(uint8_t* roiOrigin,
							bool* bNeighborFlags,
							int numIntraNeighbor,
							int unitSize,
							int numUnitsInCU,
							int totalUnits,
							int width,
							int height,
							int picStride,
							uint8_t* pLineBuf);

    void fillReferenceSamples(uint8_t* roiOrigin,
							uint8_t* adiTemp,
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

    void RkIntraFillRefSamplesCheck(uint8_t* above, uint8_t* left, int32_t width, int32_t heigth);
    void RkIntraFillChromaCheck(uint8_t* pLineBuf, uint8_t* above, uint8_t* left, uint32_t width, uint32_t heigth);
    void LogdeltaFractForAngluar();


    //void RkIntraPriorModeChoose(uint8_t* orgYuv, int width,	 intptr_t stride, int rdModeList[], int aboveMode, int leftMode);
    void RkIntraPriorModeChoose(int rdModeList[], uint32_t rdModeCost[], bool bsort);


// ------------------------------------------------------------------------------
/*
** �����֧�֣���֤�ӿڼ�����
**
*/
// ------------------------------------------------------------------------------
    void Convert8x8To4x4(uint8_t* pdst, uint8_t* psrc, uint8_t partIdx);
    void Convert4x4To8x8(uint8_t* pdst, uint8_t *psrcList[4]);
    void Convert4x4To8x8(int16_t* pdst, int16_t *psrcList[4]);
    void RK_store4x4Recon2Ref(uint8_t* pEdge, uint8_t* pRecon, uint32_t partOffset);
    void splitTo4x4By8x8(INTERFACE_INTRA* intra8x8, 
							INTERFACE_INTRA* intra4x4, 
							uint8_t* p4x4_reconEdge, 
							uint32_t partOffset);
    void Store4x4ReconInfo(FILE* fp,  INTERFACE_INTRA inf_intra4x4, uint32_t partOffset);

    void setLambda(int qp, uint8_t slicetype);

    void Intra_Proc(INTERFACE_INTRA* pInterface_Intra, 
									uint32_t partOffset,
									uint32_t cur_depth,
									uint32_t cur_x_in_cu,
									uint32_t cur_y_in_cu);    


	void RkIntra_proc(INTERFACE_INTRA* pInterface_Intra, 
                                    uint32_t partOffset,
									uint32_t cur_depth,
									uint32_t cur_x_in_cu,
									uint32_t cur_y_in_cu        );
    // ����L��buf�����L��buf
    void RK_IntraFillReferenceSamples(uint8_t* reconEdgePixel,
							bool* bNeighborFlags,
							uint8_t* pLineBuf,
							int numIntraNeighbor,
							int unitSize,
							int totalUnits,
							int width,
							int height);
    // ����L��buf�����left/above�ķֿ��洢
	void RK_IntraSmoothing(uint8_t* pLineBuf,
							int width,
						    int height,
						    bool bUseStrongIntraSmoothing,
                            uint8_t* refLeft,
							uint8_t* refAbove,
							uint8_t* refLeftFlt,
							uint8_t* refAboveFlt);

    int RK_IntraSad(uint8_t *pix1,
                intptr_t stride_pix1,
                uint8_t *pix2,
                intptr_t stride_pix2,
                uint32_t size);

	void RK_IntraCalcuResidual(uint8_t*org,
				uint8_t *pred,
				int16_t *residual,
				intptr_t stride,
				uint32_t blockSize);

// ------------------------------------------------------------------------------
/*
** ��֤���������
*/
// ------------------------------------------------------------------------------

    uint32_t    rk_bits[4][35];
    uint64_t    rk_lambdaMotionSAD;
	uint64_t	m_rklambdaMotionSAD;
	uint64_t	m_rklambdaMotionSSE;

    
    int         rk_bestMode[X265_AND_RK][4];//һ�������ͨ��һ������8x8��ʱ�������4������
    uint8_t      rk_pred[X265_AND_RK][32*32];
    uint8_t      rk_predCb[X265_AND_RK][16*16];
    uint8_t      rk_predCr[X265_AND_RK][16*16];

    int16_t     rk_residual[X265_AND_RK][32*32];
    int16_t     rk_residualCb[X265_AND_RK][16*16];
    int16_t     rk_residualCr[X265_AND_RK][16*16];

    uint8_t      rk_recon[X265_AND_RK][32*32];
    uint8_t      rk_reconCb[X265_AND_RK][16*16];
    uint8_t      rk_reconCr[X265_AND_RK][16*16];

    uint8_t      rk_4x4RefCandidate[RECON_EDGE_4X4_NUM][4];// 0-right, 0 - bottom, 1 - bottom, 2 - right �ο�����ĵ�
   

    uint64_t    rk_modeCostsSadAndCabac[35];
    uint64_t    rk_modeCostsSadAndCabacCorrect[35];

    uint32_t    rk_totalBits4x4;
    uint32_t    rk_totalDist4x4;    
    uint32_t    rk_totalCost4x4;
    
    uint32_t    rk_totalBits8x8;
    uint32_t    rk_totalDist8x8;    
    uint32_t    rk_totalCost8x8;  

  
    uint32_t    rk_totalBitsBest;
    uint32_t    rk_totalCostBest;  
};


//}
//! \}

#endif
