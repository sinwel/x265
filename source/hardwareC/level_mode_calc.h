#ifndef __LEVEL_MODE_CALC_H__
#define __LEVEL_MODE_CALC_H__

#include "../Lib/TLibCommon/CommonDef.h"
#include "module_interface.h"
#include "rk_define.h"

#include "RkIntraPred.h"
#include "qt.h"
#include "inter.h"
#include "CABAC.h"
//using namespace RK_HEVC;
//#include "hardwareC.h"

class cuData;
class hardwareC;

struct COST_DATA{
    uint32_t TotalCost;
    uint32_t Distortion;
    uint32_t Bits;
    uint32_t predMode;
    uint32_t partSize;
    uint8_t  skipFlag;
    uint8_t  mergeFlag;
    uint8_t  interDir;
    uint8_t  intraDir[4];
    uint8_t  refIdx;
    MV_INFO  mv;
    int32_t  mvd;
    uint8_t  mvpindex;
    uint8_t  cbf[4][3];    //[0][0] cbf_y, [0][1] cbf_u, [0][2] cbf_v else...

    uint8_t *recon_y;
    uint8_t *recon_u;
    uint8_t *recon_v;

    int16_t *resi_y;
    int16_t *resi_u;
    int16_t *resi_v;
};

class CU_LEVEL_CALC
{
#ifdef RK_INTRA_PRED
public:
    Rk_IntraPred*    m_rkIntraPred;
#endif
#if TQ_RUN_IN_HWC_INTRA || TQ_RUN_IN_HWC_ME
public:
	hevcQT*			 m_hevcQT;
#endif
#if RK_CABAC_H||RK_CABAC_FUNCTIONAL_TYPE||1
public:
	CABAC_RDO*  m_cabac_rdo;
#endif

public:

	/*inter FME MERGE input and output add by HDL*/
	class FractionMotionEstimation Fme;
	class MergeProc Merge;
	static class AmvpCand m_Amvp;
	static class MergeMvpCand m_MergeCand;

    struct INTERFACE_INTRA_PROC inf_intra_proc;
    struct INTERFACE_INTER_FME_PROC     inf_inter_fme_proc;
    struct INTERFACE_INTER_MERGE_PROC   inf_inter_merge_proc;
    struct INTERFACE_TQ     inf_tq;
    struct INTERFACE_INTRA  inf_intra;
    struct INTERFACE_INTRA  inf_intra4x4; // only used when m_size = 8, add by zxy
    struct INTERFACE_RECON  inf_recon;
    struct INTERFACE_RDO    inf_rdo;
    struct INTERFACE_ME     inf_me;
    struct INTERFACE_CABAC  inf_cabac;

	// for debug: TQ vs Recon
	struct INTERFACE_TQ     inf_tq_total[3]; //Y+Cb+Cr, for comparing oriResi(after T and Q), added by lks
 	struct INTERFACE_RECON	inf_recon_total[3]; //Y+Cb+Cr, for comparing Recon(after T,Q,IQ,IT + predPixel), added by lks
    int16_t*                coeff4x4[6];//coeff after T,Q, Y+Y+Y+Y+Cb+Cr, only for intra 4x4, added by lks
    int16_t*                resi4x4[6]; //resi after T,Q,IQ,IT, Y+Y+Y+Y+Cb+Cr, only for intra 4x4, added by lks
 	uint8_t*                recon4x4[6]; //reon, Y+Y+Y+Y+Cb+Cr, only for intra 4x4, added by lks
    bool                    choose4x4split; // flag, only used when size==8, 1: choose 4x4 split, added by lks

    // for copy resi/recon in ME, added by lks
	uint8_t*  reconFme[3];
	uint8_t*  reconMerge[3];
	int16_t*  coeffFme[3];
	int16_t*  coeffMerge[3];
    
	hardwareC *pHardWare;
    uint32_t TotalCost;

    uint8_t m_size;
    uint8_t cu_w;
    uint8_t x_pos;
    uint8_t y_pos;
    uint32_t x_in_pic;
    uint32_t y_in_pic;
    uint8_t valid; // 0:unvalid 1:valid //当前CU块是否有效

    uint8_t curr_cu_y[64*64];
    uint8_t curr_cu_u[32*32];
    uint8_t curr_cu_v[32*32];

    uint8_t cuPredMode; // P:0 I:1
    uint8_t cuPartMode; // 0 2Nx2N 1 NxN

    uint8_t mergeFlag;
    uint8_t mergeIndex;
    uint8_t MVDIndex;        //TODO
    struct MV_INFO MV;       //TODO
    uint8_t inter_cbfY[4];
    uint8_t inter_cbfU;
    uint8_t inter_cbfV;
    uint8_t inter_tuSplitFlag;

    uint8_t intra_cbfY[4];
    uint8_t intra_cbfU;
    uint8_t intra_cbfV;
    uint8_t intra_tuSplitFlag;
    uint8_t intra_dirMode[4];
    uint8_t intra_buf_y[32*5+1];
    uint8_t intra_buf_u[16*5+1];
    uint8_t intra_buf_v[16*5+1];
    uint8_t cu_type[4*5 + 1];
    uint8_t intra_mpm[4 + 1];
    uint8_t tu_cbfY_flag[16*16];
    struct MV_INFO mv_info[8*5+1];
    struct COST_DATA  cost_inter;
    struct COST_DATA  cost_intra;
    struct COST_DATA  cost_intra_4;
    struct COST_DATA *cost_best;
    struct COST_DATA  cost_temp;

    class cuData *cu_matrix_data;   //buf 4 for instead
    unsigned int  matrix_pos;       //buf 4 pos for instead
    unsigned int depth;
    unsigned int ori_pos;

    unsigned int cu_pos;
    unsigned int temp_pos;

public:
	unsigned int proc(unsigned int level, unsigned int pos_x, unsigned int pos_y);

    CU_LEVEL_CALC(void);
    ~CU_LEVEL_CALC(void);

    void init(uint8_t size);
    void deinit();
    void intra_proc();
    void intra_4_proc();
    void inter_qt(uint8_t nPart, uint8_t mode, uint8_t textType); // textType: Y(0), U(1), V(2); mode: fme(0), merge(1). added by lks
    void inter_proc(int offsIdx);
    uint32_t RdoDistCalc(uint32_t SSE, uint8_t yCbCr);
    uint32_t RdoCostCalc(uint32_t dist, uint32_t bits, int qp);
    void InterRdCompare();
    void RDOCompare();
    void begin();
    void end();

    void intra_neighbour_flag_descion();
    void recon();

    void BS_calc();
};
#endif
