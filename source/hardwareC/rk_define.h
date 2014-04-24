#ifndef __RK_DEFINE_H__
#define __RK_DEFINE_H__


/* macros definitions */
#define CLIP(a, min, max)			(((a) < min) ? min : (((a) > max) ? max : (a)))

#define     FPRINT(fp, ...)  { if (fp) { fprintf(fp, ## __VA_ARGS__); fflush(fp);} }
#define     FWRITE(fp, ...)  { if (fp) { fwrite (fp, ## __VA_ARGS__); fflush(fp);} }

#define OPT(name0, name1) else if (!strncmp(name0, name1, sizeof(name1) - 1))

/* data definitions */
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

struct CTU_CMD
{
    uint32_t ctu_x_offset : 7;
    uint32_t ctu_y_offset : 7;
    uint32_t avail_l      : 1;
    uint32_t avail_t      : 1;
    uint32_t avail_tr     : 1;
    uint32_t avail_tl     : 1;
    uint32_t pic_r        : 1;
    uint32_t pic_b        : 1;
    uint32_t tile_l       : 1;
    uint32_t tile_r       : 1;
};
/* copy from RkIntraPred.h by zxy  */

#if defined(_MSC_VER)
	#define FILE_PATH "F:/HEVC/log_files/"
	#define PATH_NAME(name) (FILE_PATH ## name)
#elif  defined(__GNUC__)
	#define FILE_PATH "/home2/zxy/log_files/"
	#define PATH_NAME(name) ("/home2/zxy/log_files/"name)
#endif


#define SIZE_4x4                4
#define SIZE_8x8                8
#define SIZE_16x16              16
#define SIZE_32x32              32
#define SIZE_64x64              64

#define REG						int
#define REG32					int
#define REG16					short
#define REG8					unsigned char

#if 1
// all case
#define MATCH_CASE(size,partition) ( size != SIZE_64x64)
#else
//if (( cu->getWidth(0) != SIZE_64x64) && ( cu->getPartitionSize(0) == SIZE_2Nx2N )) // фа╠н4x4
#define MATCH_CASE(size,partition) ( (size != SIZE_64x64) && ( partition == 0))
#endif
#define RK_MAX_CU_DEPTH         6                           // log2(LCUSize)
#define RK_MAX_CU_SIZE          (1 << (RK_MAX_CU_DEPTH))       // maximum allowable size of CU
#define RK_MIN_PU_SIZE          4
#define RK_MAX_NUM_SPU_W        (RK_MAX_CU_SIZE / RK_MIN_PU_SIZE) // maximum number of SPU in horizontal line
#define RK_ADI_BUF_STRIDE       (2 * RK_MAX_CU_SIZE + 1 + 15)  // alignment to 16 bytes

#define RK_DEPTH                8

#define RK_CHOOSE               1   // enable flag for default cfg modificaiton(e.g. RDOQ, signhide), added by lks

#define RK_ClipY(x)			(((x) < 0) ? 0 : (((x) > ((1 << RK_DEPTH) - 1)) ? ((1 << RK_DEPTH) - 1) : (x)))
#define RK_ClipC(x)			(((x) < 0) ? 0 : (((x) > ((1 << RK_DEPTH) - 1)) ? ((1 << RK_DEPTH) - 1) : (x)))
#define RK_ClipC(x)			(((x) < 0) ? 0 : (((x) > ((1 << RK_DEPTH) - 1)) ? ((1 << RK_DEPTH) - 1) : (x)))
#define RK_Clip3(a, min, max)			(((a) < min) ? min : (((a) > max) ? max : (a)))

typedef enum X265_AND_RKs   // Routines can be indexed using log2n(width)
{
    X265_COMPENT,
    RK_COMPENT,
    X265_AND_RK
} X265_AND_RKs;



#endif
