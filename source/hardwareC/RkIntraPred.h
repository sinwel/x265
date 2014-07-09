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
// 每个 8x8 的block 存 4条 4像素的边
// 第一维的访问下标为 zorder/4
extern uint8_t g_4x4_total_reconEdge[64][RECON_EDGE_4X4_NUM][4];

extern  INTERFACE_INTRA g_previous_8x8_intra;


typedef struct RkIntraSmooth
{
    // smoothing [32x32 16x16 8x8],4x4不需要滤波
    // 模块输入是有效的边界数据,输出是滤波后的边界数据
    // buffer以最大的32x32为例，存储空间为 64+64+1 的line buf中
    // 这里和x265有个区别就是，不会存在64x64的输入，上层一定会划分成32x32

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
    INTRA_4_ORI_PIXEL         , //按照y cb y y cr y的顺序打印
    INTRA_4_REF_PIXEL         , //按照y cb y y cr y的顺序进行打印
    INTRA_4_SAD               , //按照y y y y的顺序进行打印 
    INTRA_4_CABAC_MODE_BIT    , //按照y y y y的顺序进行打印 
    INTRA_4_ORI_PIXEL_CU_LU   , // 8x8 CU 按照64个像素一行的顺序打印，打印顺序p[63]p[62]p[61]...p[1]p[0]
    INTRA_4_ORI_PIXEL_CU_CB   , // 8x8 CU 按照16个像素一行的顺序打印，打印顺序p[15]p[14]p[13]...p[1]p[0]
    INTRA_4_ORI_PIXEL_CU_CR   , // 8x8 CU 按照16个像素一行的顺序打印，打印顺序p[15]p[14]p[13]...p[1]p[0]
    INTRA_4_REF_PIXEL_CU_LU   , //8x8 CU的ref pixel lu像素 33个像素一行 打印顺序p[32]p[31]p[23]...p[1]p[0]
    INTRA_4_REF_PIXEL_CU_CB   , //8x8 CU的ref pixel cb像素 17个像素一行 打印顺序p[16]p[15]p[14]...p[1]p[0]
    INTRA_4_REF_PIXEL_CU_CR   , //8x8 CU的ref pixel cr像素 17个像素一行 打印顺序p[16]p[15]p[14]...p[1]p[0]
    INTRA_4_REF_CU_VALID      , //8x8 CU的ref cu valid信号 按照5bit一行的顺序打印[4][3][2][1][0]
    INTRA_4_RESI_BEFORE       , //按照16个像素一行的顺序打印，每个像素位宽为16bit，打印顺序p[15]p[14]p[13]...p[1]p[0];按照y cb y y cr y的顺序打印
    INTRA_4_RESI_AFTER        ,
    INTRA_4_PRED              , //按照16个像素一行的顺序打印，每个像素位宽为8bit，打印顺序p[15],p[14],p[13]...p[1],p[0];按照y cb y y cr y 的顺序打印
    INTRA_4_RECON             , //按照16个像素一行的顺序打印，每个像素位宽为8bit，打印顺序p[15],p[14],p[13]...p[1],p[0];按照y cb y y cr y 的顺序打印
    INTRA_4_BEST_MODE         , //按照8bit一行进行打印, 按照y cb y y cr y的顺序打印
    INTRA_4_TU_CABAC_BITS   	, //按照一个tu 残差，32 bit一行的顺序进行打印按照y cb y y cr y的顺序打印 32 bit x 6
    INTRA_4_PU_PRED_MODE_BITS   , //按照 32 bit一行的数据进行打印,按照y cb y y cr y的顺序进行打印
    INTRA_4_CU_TOTAL_BITS       ,
    INTRA_4_CU_TOTAL_DISTORTION ,
    INTRA_4_CU_TOTAL_COST       ,
    INTRA_4_TU_COST_BITS        ,
    INTRA_4_TU_CBF_BITS         , //按照 24 bit打印 ，y cb y y cr y的顺序打印 一个TU 一行
    INTRA_4_PU_PART_MODE_BITS	, //按照 24 bit一行的顺序进行打印,一个PU一行    
    // ------ TQ file ----------- //
    INTRA_4_T_INPUT4X4DATA         ,
    INTRA_4_QIT_OUTPUT4X4DATA      ,
    
    INTRA_4_FILE_NUM
} Intra4HardwareFile;

typedef enum Intra8HardwareFile
{
    INTRA_8_SAD               , //按照18个一行 y 的顺序进行打印 0 sad[0], 1 sad[1], 2 sad[2], 4 sad[4], ... 34 sad[34]
    INTRA_8_CABAC_MODE_BIT    , //按照18个一行 y 的顺序进行打印 [bit34][bit32]...[bit2][bit1][bit0] 
    INTRA_8_ORI_PIXEL_CU_LU   , // 8x8 CU 按照64个像素一行的顺序打印，打印顺序p[63]p[62]p[61]...p[1]p[0]
    INTRA_8_ORI_PIXEL_CU_CB   , // 8x8 CU 按照16个像素一行的顺序打印，打印顺序p[15]p[14]p[13]...p[1]p[0]
    INTRA_8_ORI_PIXEL_CU_CR   , // 8x8 CU 按照16个像素一行的顺序打印，打印顺序p[15]p[14]p[13]...p[1]p[0]
    INTRA_8_REF_PIXEL_CU_LU   , //8x8 CU的ref pixel lu像素 33个像素一行 打印顺序p[32]p[31]p[23]...p[1]p[0]
    INTRA_8_REF_PIXEL_CU_CB   , //8x8 CU的ref pixel cb像素 17个像素一行 打印顺序p[16]p[15]p[14]...p[1]p[0]
    INTRA_8_REF_PIXEL_CU_CR   , //8x8 CU的ref pixel cr像素 17个像素一行 打印顺序p[16]p[15]p[14]...p[1]p[0]
    INTRA_8_REF_CU_VALID      , //8x8 CU的ref cu valid信号 按照5bit一行的顺序打印[4][3][2][1][0]
    INTRA_8_RESI_BEFORE       , //按照16个像素一行的顺序打印，每个像素位宽为16bit，打印顺序p[15]p[14]p[13]...p[1]p[0];按照 y cb cr 的顺序打印
    INTRA_8_RESI_AFTER        , //按照16个像素一行的顺序打印，每个像素位宽为16bit，打印顺序p[15]p[14]p[13]...p[1]p[0];按照 y cb cr 的顺序打印 
    INTRA_8_PRED              , //按照16个像素一行的顺序打印，每个像素位宽为8bit，打印顺序p[15],p[14],p[13]...p[1],p[0];按照y cb cr的顺序打印 
    INTRA_8_RECON             , //按照16个像素一行的顺序打印，每个像素位宽为8bit，打印顺序p[15],p[14],p[13]...p[1],p[0];按照y cb cr的顺序打印 
    INTRA_8_BEST_MODE         , //按照8bit一行进行打印;按照y cb cr的顺序打印 
    INTRA_8_TU_CABAC_BITS   	, //按照一个tu 残差 32 bit一行的顺序进行打印;按照y cb cr 的顺序打印 
    INTRA_8_PU_PRED_MODE_BITS   , //按照 32 bit一行的数据进行打印,按照y cb&cr的顺序进行打印
    INTRA_8_CU_TOTAL_BITS       , //按照24bit一行的顺序进行打印
    INTRA_8_CU_TOTAL_DISTORTION , //按照32bit一行的顺序进行打印
    INTRA_8_CU_TOTAL_COST       , //按照32bit一行的顺序进行打印
    INTRA_8_TU_COST_BITS		, //按照一个Y分量的TU的(sad + sad_lambda * bit)打印;顺序0 cost[0], 1 cost[1], 2 cost[2], 4 cost[4], ... 34 cost[34];cost按32bit打印
    INTRA_8_TU_CBF_BITS         , //按照 24 bit打印 ，y cb cr 的顺序打印 一个TU 一行
    INTRA_8_PU_PART_MODE_BITS	, //按照 24 bit一行的顺序进行打印,一个PU一行
    INTRA_8_REF_PIXEL_CU_LU_FILTER  ,//8x8 CU的ref pixel lu像素 filter后像素 33个像素一行 打印顺序p[32]p[31]p[23]...p[1]p[0]
    INTRA_8_FILE_NUM
} Intra8HardwareFile;

typedef enum Intra16HardwareFile
{
    INTRA_16_SAD               , //按照35个一行 y 的顺序进行打印 0 sad[0], 1 sad[1], 2 sad[2], 3 sad[3], ... 34 sad[34]
    INTRA_16_CABAC_MODE_BIT    , //按照35个一行 y 的顺序进行打印 [bit34][bit33]...[bit2][bit1][bit0] 
    INTRA_16_ORI_PIXEL_CU_LU   , // 16x16 CU 按照16 x 16个像素一行的顺序打印，打印顺序p[63]p[62]p[61]...p[1]p[0]
    INTRA_16_ORI_PIXEL_CU_CB   , // 16x16 CU 按照8 x 8个像素一行的顺序打印，打印顺序p[15]p[14]p[13]...p[1]p[0]
    INTRA_16_ORI_PIXEL_CU_CR   , // 16x16 CU 按照8 x 8个像素一行的顺序打印，打印顺序p[15]p[14]p[13]...p[1]p[0]
    INTRA_16_REF_PIXEL_CU_LU   , // 16x16 CU的ref pixel lu像素 65个像素一行 打印顺序p[64]p[63]p[62]...p[1]p[0]
    INTRA_16_REF_PIXEL_CU_CB   , // 16x16 CU的ref pixel cb像素 33个像素一行 打印顺序p[32]p[31]p[30]...p[1]p[0]
    INTRA_16_REF_PIXEL_CU_CR   , // 16x16 CU的ref pixel cr像素 33个像素一行 打印顺序p[32]p[31]p[30]...p[1]p[0]
    INTRA_16_REF_CU_VALID      , // 16x16 CU的ref cu valid信号 按照9bit一行的顺序打印[8][7][6][5][4][3][2][1][0]
    INTRA_16_RESI_BEFORE       , // 预测出来残差像素 按照16个像素一行的顺序打印，每个像素位宽为16bit，打印顺序p[15]p[14]p[13]...p[1]p[0];
								 //	cb两行拼成1行	cr两行拼成1行 	按照y cb cr的顺序打印
    INTRA_16_RESI_AFTER        , // 经过IT之后的残差像素 按照16个像素一行的顺序打印，每个像素位宽为16bit，打印顺序p[15]p[14]p[13]...p[1]p[0];
								 // cb两行拼成1行 	cr两行拼成1行 	按照y cb cr的顺序打印
    INTRA_16_PRED              , // 经过IT之后的残差像素 按照16个像素一行的顺序打印，每个像素位宽为8bit，打印顺序p[15]p[14]p[13]...p[1]p[0];
								 // cb两行拼成1行 	cr两行拼成1行 	按照y cb cr的顺序打印
    INTRA_16_RECON             , // 经过IT之后的残差像素 按照16个像素一行的顺序打印，每个像素位宽为8bit，打印顺序p[15]p[14]p[13]...p[1]p[0];
								 // cb两行拼成1行 	cr两行拼成1行 	按照y cb cr的顺序打印
    INTRA_16_BEST_MODE         , // 按照8bit一行进行打印;按照y cb cr的顺序打印 
    INTRA_16_TU_CABAC_BITS   	, //按照一个tu 残差 24 bit一行的顺序进行打印;按照y cb cr 的顺序打印 
    INTRA_16_PU_PRED_MODE_BITS   , //按照 32 bit一行的数据进行打印,按照y cb cr的顺序进行打印
    INTRA_16_CU_TOTAL_BITS       , //按照24bit一行的顺序进行打印
    INTRA_16_CU_TOTAL_DISTORTION , //按照32bit一行的顺序进行打印
    INTRA_16_CU_TOTAL_COST       , //按照32bit一行的顺序进行打印
    INTRA_16_TU_COST_BITS		, //按照一个Y分量的TU的(sad + sad_lambda * bit)打印;顺序0 cost[0], 1 cost[1], 2 cost[2], 4 cost[4], ... 34 cost[34];cost按32bit打印
    INTRA_16_TU_CBF_BITS         , //按照 24 bit打印 ，y cb cr 的顺序打印 一个TU 一行
    INTRA_16_PU_PART_MODE_BITS	, //按照 24 bit一行的顺序进行打印,一个PU一行
    INTRA_16_REF_PIXEL_CU_LU_FILTER  ,//16x16 CU的ref pixel lu像素 经过滤波后的lu ref像素 65个像素一行 打印顺序p[64]p[63]p[62]...p[1]p[0]

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


    uint8_t             rk_LineBufTmp[129]; // 数据靠前存储 左下到左上到右上，corner点在中间

    uint8_t             rk_LineBuf[129];    // 数据靠前存储 左下到左上到右上，corner点在中间
    uint8_t             rk_LineBufCb[65];    // 数据靠前存储 左下到左上到右上，corner点在中间
    uint8_t             rk_LineBufCr[65];    // 数据靠前存储 左下到左上到右上，corner点在中间

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
** 对外层支持，保证接口兼容性
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
    // 输入L型buf，输出L型buf
    void RK_IntraFillReferenceSamples(uint8_t* reconEdgePixel,
							bool* bNeighborFlags,
							uint8_t* pLineBuf,
							int numIntraNeighbor,
							int unitSize,
							int totalUnits,
							int width,
							int height);
    // 输入L型buf，输出left/above的分开存储
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
** 验证数据流输出
*/
// ------------------------------------------------------------------------------

    uint32_t    rk_bits[4][35];
    uint64_t    rk_lambdaMotionSAD;
	uint64_t	m_rklambdaMotionSAD;
	uint64_t	m_rklambdaMotionSSE;

    
    int         rk_bestMode[X265_AND_RK][4];//一个输入块通常一个方向，8x8的时候可以有4个方向
    uint8_t      rk_pred[X265_AND_RK][32*32];
    uint8_t      rk_predCb[X265_AND_RK][16*16];
    uint8_t      rk_predCr[X265_AND_RK][16*16];

    int16_t     rk_residual[X265_AND_RK][32*32];
    int16_t     rk_residualCb[X265_AND_RK][16*16];
    int16_t     rk_residualCr[X265_AND_RK][16*16];

    uint8_t      rk_recon[X265_AND_RK][32*32];
    uint8_t      rk_reconCb[X265_AND_RK][16*16];
    uint8_t      rk_reconCr[X265_AND_RK][16*16];

    uint8_t      rk_4x4RefCandidate[RECON_EDGE_4X4_NUM][4];// 0-right, 0 - bottom, 1 - bottom, 2 - right 参考设计文档
   

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
