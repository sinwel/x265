#ifndef __HARDWAREC_H__
#define __HARDWAREC_H__

#include "../Lib/TLibCommon/CommonDef.h"
#include "level_mode_calc.h"
#include "ctu_calc.h"

struct ENC_CONFIG
{
    //read from config file
};

//TODO
struct HW_CONFIG
{
    uint32_t pic_width;
    uint32_t pic_height;

    uint8_t ctu_size;       //64 32
    uint8_t max_cu_size;
    uint8_t max_cu_depth;
    uint8_t min_tu_size;
    uint8_t max_tu_depth;
    uint8_t intra_tu_4x4_enable;

    uint32_t ctu_num_of_slice;      //按照CTU num进行slice长度设定
    uint32_t ctu_num_tile_width;    //仅支持固定大小的Tile
    uint32_t ctu_num_tile_height;   //仅支持固定大小的Tile

    uint8_t wpp_enable;
    uint8_t rect_enable;
    uint8_t merge_range;
    uint8_t strong_intra_smoothing_enable;
    uint8_t constrained_intra_enable;

    uint8_t QP;
    int8_t  QP_cb_offset;
    int8_t  QP_cr_offset;
    uint8_t AQ_mode_enable;

    uint8_t Deblocking_enable;
    uint8_t SAO_enable;
};

class hardwareC
{
    public:

    struct HW_CONFIG hw_cfg;

    //class PREPEOCESS preProcess
    class CTU_CALC ctu_calc;
    //class BS bs;
    //class SAO sao;
    //class DBLK deblocking;

    unsigned int pic_w;
    unsigned int pic_h;

    unsigned int ctu_w;
    unsigned int ctu_w_log2;

    unsigned int ctu_x;
    unsigned int ctu_y;

    unsigned int cu_x;
    unsigned int cu_y;

	hardwareC(void);
	~hardwareC(void);

    void init();
    void ConfigFiles(FILE *fp);
    void proc();
};

extern hardwareC G_hardwareC;

#endif