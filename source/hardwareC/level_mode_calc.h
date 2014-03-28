#ifndef __LEVEL_MODE_CALC_H__
#define __LEVEL_MODE_CALC_H__

#include "../Lib/TLibCommon/CommonDef.h"
#include "module_interface.h"

//#include "hardwareC.h"

//using namespace RK_HEVC;


class hardwareC;

struct COST_DATA{
    uint32_t TotalCost;
    uint32_t Distortion;
    uint32_t Bits;
    uint32_t predMode;

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
public:

    struct INTERFACE_TQ     inf_tq;
    struct INTERFACE_INTRA  inf_intra;
    struct INTERFACE_RECON  inf_recon;
    struct INTERFACE_RDO    inf_rdo;
    struct INTERFACE_ME     inf_me;
    struct INTERFACE_CABAC  inf_cabac;

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

    uint8_t cuPredMode; // 0:B 1:P 2:I
    uint8_t tuSplitFlag;
    uint8_t cbfY;
    uint8_t cbfU;
    uint8_t cbfV;
    uint8_t intraDirMode;
    uint8_t mergeFlag;
    uint8_t mergeIndex;
    uint32_t MVD;       //TODO
    uint32_t MVDIndex;  //TODO

    struct COST_DATA  cost_inter;
    struct COST_DATA  cost_intra;
    struct COST_DATA  cost_intra_4;
    struct COST_DATA *cost_best;
    struct COST_DATA  cost_temp;

    unsigned int depth;
    unsigned int ori_pos;
    unsigned int temp_pos;

    unsigned int cu_pos;

	unsigned int proc(unsigned int level, unsigned int pos_x, unsigned int pos_y);

    CU_LEVEL_CALC(void);
    ~CU_LEVEL_CALC(void);

    void init(uint8_t size);
    void deinit();
    void intra_proc();
    void intra_4_proc();
    void inter_proc();
    void RDOCompare();
    void begin();
    void end();

    void intra_neighbour_flag_descion();

    void recon();

    void BS_calc();
};
#endif
