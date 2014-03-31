#ifndef __RK_DEFINE_H__
#define __RK_DEFINE_H__

#define RK_CTU_CALC_PROC_ENABLE 0
#define RK_OUTPUT_FILE 0


struct RK_MV
{
    uint32_t mv_x        : 10; //8.2
    uint32_t mv_y        : 10; //8.2
    uint32_t ref_idx     : 4;  //
    uint32_t pic_idx     : 4;
    uint32_t pred_flag   : 1;
    uint32_t reserve     : 3;
};

struct MV_INFO
{
    struct RK_MV mv[2];
};

struct IME_MV
{
    int16_t mv_x;
    int16_t mv_y;
    int16_t SAD_cost;
};

#endif