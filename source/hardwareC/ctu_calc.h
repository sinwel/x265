#ifndef __CTU_CALC_H__
#define __CTU_CALC_H__

#include "../Lib/TLibCommon/CommonDef.h"
#include "../Lib/TLibCommon/TComDataCU.h"

#include "rk_define.h"
#include "level_mode_calc.h"
#include "inter.h"
#include "CABAC.h"

class hardwareC;

class cuCost
{
public:
    uint64_t m_totalCost;
    uint32_t m_totalDistortion;
    uint32_t m_totalBits;
};


class cuData
{
public:
    uint64_t totalCost;
    uint32_t totalDistortion;
    uint32_t totalBits;

    uint8_t  cu_x;
    uint8_t  cu_y;

    uint8_t  width;
    uint8_t  height;
    uint8_t  depth;

    uint8_t  *cuPartSize;    // Nx2N NxN
    uint8_t  *cuSkipFlag;    // CU Skip
    uint8_t  *cuPredMode;    // P:0 I:1

    uint8_t  MergeFlag;      // CU Merge
    uint8_t  MergeIndex;     // CU MergeIndex
    struct MV_INFO *mv;      // TODO inter relation
    uint32_t mvd;            // TODO
    uint8_t  mvdIndex;       // TODO

    uint8_t  intraDirMode[4];// intraLumaDirMode
    uint8_t  interDirMode;   // interDirmode

    uint8_t  *cbfY;      //
    uint8_t  *cbfU;      //
    //uint8_t  m_TransformSkipV;

    uint8_t  *cbfV;      //

    int16_t  *CoeffY;
    int16_t  *CoeffU;
    int16_t  *CoeffV;

    uint8_t  *ReconY;
    uint8_t  *ReconU;
    uint8_t  *ReconV;

    uint8_t  cu_split_flag;
    uint8_t  tu_split_flag;

    void init(uint8_t w);
    void deinit();

};



class CTU_CALC
{
public:

#if RK_CABAC_H
	CABAC_RDO  m_cabac_rdo;
#endif

    /* Input */
    uint8_t *input_curr_y;              //64*64  //inPut
    uint8_t *input_curr_u;              //32*32  //input
    uint8_t *input_curr_v;              //32*32  //input

    uint8_t  input_curr_ref_y[256*128];
    uint8_t  input_curr_ref_u[128*64];
    uint8_t  input_curr_ref_v[128*64];
    uint8_t  QP_lu;
    uint8_t  QP_cb;
    uint8_t  QP_cr;
	uint8_t  slice_type; //0: B, 1: P, 2: I

    /* output */
    uint8_t *output_recon_y;            //64*64
    uint8_t *output_recon_u;            //32*32
    uint8_t *output_recon_v;            //32*32

    int16_t *output_resi_y;             //64*64
    int16_t *output_resi_u;             //32*32
    int16_t *output_resi_v;             //32*32

    uint8_t  cu_split_flag[21]; //1
    uint8_t  tu_split_flag[64];

    uint8_t  cu_part_size[64];           // 0:2Nx2N 1:NxN
    uint8_t  cu_prediction_mode[64];
    uint8_t  cu_intra_dir_mode[64];

    uint8_t  cu_merge_flag[64];
    uint8_t  cu_merge_index[64];

    uint8_t  cu_valid_flag[4][64];

    uint8_t *cbfY;                  //256
    uint8_t *cbfU;                  //64
    uint8_t *cbfV;                  //64

    uint32_t MV[64];
    uint32_t MVD[64];
    uint32_t MVD_index[64];

    uint8_t  BS_horizontal_flag[16][8];       //BS horizontal flag for Deblocking
    uint8_t  BS_vertical_flag[16][8];         //BS vertical   flag for Deblocking


    /* inline */
    hardwareC *pHardWare;

    unsigned int pic_w;
    unsigned int pic_h;

    unsigned int ctu_w;
    unsigned int ctu_w_log2;

    uint8_t ctu_x;
    uint8_t ctu_y;

    uint8_t  valid_left;
    uint8_t  valid_left_top;
    uint8_t  valid_top;
    uint8_t  valid_top_right;

    uint8_t  line_intra_buf_y[1920];
    uint8_t  line_intra_buf_u[1920/2];
    uint8_t  line_intra_buf_v[1920/2];
    uint8_t  line_cu_type[1920/8];
    uint8_t  line_tu_cbfY_flag[1920/4];

    struct MV_INFO line_mv_buf[1920/8];

    uint8_t     L_intra_buf_y[32*5+1];                //相邻L型buf，用于存储重构像素
    uint8_t     L_intra_buf_u[16*5+1];                //相邻L型buf，用于存储重构像素
    uint8_t     L_intra_buf_v[16*5+1];                //相邻L型buf，用于存储重构像素
    uint8_t     L_cu_type[(32*5)/8 + 1];              //相邻L型buf，用于存储相邻CU块的属性，用于MV及constra_intra 以及BS计算
    struct MV_INFO   L_mv_buf[(32*5)/8 + 1];          //相邻L型buf,用于存储相邻CU块的MV信息

    uint8_t cu_data_valid[8*8];                 //cu data valid,用于判断相邻CU块是否有效。


    uint8_t inter_cbf[32+1];                    //相邻L型buf，用于存储相邻inter pu块的残差是否为0


    //TODO
    uint8_t  cu_depth[64];      //(0, 1, 2, 3)
    uint8_t  tu_depth[256];     //(0, 1, 2)
    uint8_t  pu_depth[256];     //(0, 1)
    uint8_t  intra_bs_flag[256]; //(2:intra, 1:cbf 0:none)
    MV_INFO  mv_info[64];
    uint8_t  qp[64];

    class CU_LEVEL_CALC cu_level_calc[4];

    FILE*    fp_rk_intra_params;


    /* ctu_status_calc */
    uint32_t sw_pic_total_bits;
    uint32_t sw_slice_bits;
    uint32_t sw_slice_ctu_num;
    uint32_t sw_slice_mode;         // 0 ctu_num mode / 1 slice_len mode
    uint8_t  last_ctu_qp;

    uint32_t cur_pic_total_bits;
    uint8_t  cur_ctu_qp;
    uint32_t cur_ctu_bits;
    uint32_t cur_slice_total_bits;
    uint32_t ctu_pred_bits;

    //TODO
    //MVP Relation data

    /* Data copy from x265*/
    uint8_t ori_recon_y[64*64];         // 重构Luma分量 64*64
    uint8_t ori_recon_u[32*32];         // 重构Cb分量   32*32
    uint8_t ori_recon_v[32*32];         // 重构Cr分量   32*32

    int16_t ori_resi_y[64*64];          // 残差Luma分量 64*64
    int16_t ori_resi_u[32*32];          // 残差Cb分量   32*32
    int16_t ori_resi_v[32*32];          // 残差Cr分量   32*32

    uint8_t best_pos[4];                // 按照Z序记录当前层CU的个数
    uint8_t temp_pos[4];                // 按照Z序记录当前sub层CU的个数

    class cuCost intra_temp_0;
    class cuCost intra_temp_1[4];
    class cuCost intra_temp_2[16];

    class cuData*** cu_ori_data;
    uint8_t ori_cu_split_flag[21];

    FILE *fp_ctu_calc_cmdf;
    FILE *fp_ctu_calc_recon_y;
    FILE *fp_ctu_calc_recon_uv;

    void proc();
    CTU_CALC();
    ~CTU_CALC();

    void init();
    void deinit();
    void begin();
    void end();
    void compare();
    void cu_level_compare(uint32_t bestCU_cost, uint32_t tempCU_cost, uint8_t depth);
	void model_cfg(char *cmd);
    void LogIntraParams2File(INTERFACE_INTRA_PROC &inf_intra_proc, uint32_t x_pos, uint32_t y_pos);
	void LogFile(INTERFACE_TQ* inf_tq);
    void convert8x8HWCtoX265Luma(int16_t* coeff);
    void compareCoeffandRecon(CU_LEVEL_CALC* hwc_data, int level, bool choose4x4split);
    void ctu_status_calc();
};

#endif
