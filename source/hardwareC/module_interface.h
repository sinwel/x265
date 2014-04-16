#ifndef __MODULE_INTERFACE_H__
#define __MODULE_INTERFACE_H__

#include "../Lib/TLibCommon/CommonDef.h"
#include "rk_define.h"
#include "inter.h"
struct INTERFACE_INTRA_PROC
{
    /* input */
    uint8_t *fencY;
    uint8_t *fencU;
    uint8_t *fencV;
    uint8_t *reconEdgePixelY; // 指向左上角
    uint8_t *reconEdgePixelU;
    uint8_t *reconEdgePixelV;
    uint8_t *bNeighborFlags;  // 指向左上角
    uint8_t  numIntraNeighbor;
    uint8_t  size;
    uint8_t  useStrongIntraSmoothing;

    /* output */
    uint32_t Distortion;
    uint32_t Bits;
    uint32_t totalCost;
    int16_t  *ResiY;
    int16_t  *ResiU;
    int16_t  *ResiV;
    uint8_t  *ReconY;
    uint8_t  *ReconU;
    uint8_t  *ReconV;
    uint8_t   partSize;
    uint8_t   predModeIntra[4]; // if partSize == NxN, predMode exist 4 direction.
};

struct INTERFACE_INTER_PROC
{
    /* input */

    /* output */
    uint32_t Distortion;
    uint32_t Bits;
    uint32_t totalCost;
    int16_t *ResiY;
    int16_t *ResiU;
    int16_t *ResiV;
    uint8_t  *ReconY;
    uint8_t  *ReconU;
    uint8_t  *ReconV;
    uint8_t   partSize;
    uint8_t   predMode;
    uint8_t   skipFlag;
    uint8_t   mergeFlag;
    uint8_t   interDir;
    uint8_t   refIdx;
    MV_INFO   mv;
    uint32_t  mvd;
    uint8_t   mvpIndex;
    uint8_t   **cbf;
};

struct INTERFACE_TQ
{
    /* input */
    int16_t *resi;
    uint8_t  textType;
    int      QP;
    bool     TransFormSkip;
    uint8_t  Size;
    uint8_t  sliceType;
    uint8_t  predMode;
    int      qpBdOffset;
    int      chromaQpOffset;

    /* output */
    int16_t *outResi;
    int16_t *oriResi;
    uint8_t  absSum;
    uint8_t  lastPosX;
    uint8_t  lastPosY;
};

struct INTERFACE_INTRA
{
    /* input */
    uint8_t*    fenc;
    uint8_t*    reconEdgePixel;         // 指向左下角，不是指向中间
    uint8_t     NeighborFlags[17];      // ZSQ
    bool*       bNeighborFlags;         // 指向左下角，不是指向中间
    uint8_t     numintraNeighbor;
    uint8_t     size;
    bool	    useStrongIntraSmoothing;	//LEVEL	cu->getSlice()->getSPS()->getUseStrongIntraSmoothing()
    uint8_t     cidx;           // didx = 0 is luma(Y), cidx = 1 is U, cidx = 2 is V
    uint8_t     lumaDirMode;    // chroma direct derivated from luma

    /* output */
    uint8_t*    pred;
    int16_t*    resi;
    uint8_t     DirMode;
};


struct INTERFACE_RECON
{
    /* input */
    uint8_t  *pred;
    int16_t  *resi;
    uint8_t  *ori;
    uint8_t   size;

    /* output */
    uint32_t  SSE;
    uint8_t  *Recon;
};

struct INTERFACE_RDO
{
    /* input */
    uint32_t  SSE;
    uint32_t  QP;
    uint32_t  Bits;

    /* output */
    uint32_t  Cost;

};

struct INTERFACE_ME
{
    /* output */
    uint8_t  *predY;
    uint8_t  *predU;
    uint8_t  *predV;
    int16_t  *resiY;
    int16_t  *resiU;
    int16_t  *resiV;
	int32_t   predModes; //inter或intra的模式标识
    uint8_t   partSize; //pu的划分模式
	bool       skipFlag;
	uint32_t  interDir;
	int32_t    refPicIdx; //参考图像的标识,只有在多参考帧的时候有效
    uint8_t   mergeFlag;
    uint8_t   mergeIdx;
    Mv         mvd;
    uint32_t  mvdIdx;
    Mv         mv;
};

struct INTERFACE_CABAC
{
};


struct INTERFACE_PREPROCESS
{
    uint8_t  *input_src_y;
    uint8_t  *input_src_u;
    uint8_t  *input_src_v;
    uint8_t   format;
    uint8_t   cscMode;
    uint8_t   endian_mode;
    uint8_t   mirror_mode;
    uint8_t   rotate_mode;
    uint8_t   denoise_mode;
    uint8_t   aq_mode;
    uint8_t   aq_strenth;
    uint8_t   pic_width;
    uint8_t   pic_height;

    uint8_t  *output_y;
    uint8_t  *output_u;
    uint8_t  *output_v;
    uint8_t  *qp_aq_offset;
};

struct INTERFACE_IME
{
    /* input */
    uint16_t ImeSearchRangeWidth;
    uint16_t ImeSearchRangeHeight;
    bool     isValidCu[85];
    uint8_t  *pImeSearchRange;
    uint8_t  *pCurrCtu;

    /* output */
    struct IME_MV ***ime_mv;
};

struct INTERFACE_CTU_CALC
{
    /* input */
    uint8_t  *input_curr_y;
    uint8_t  *input_curr_u;
    uint8_t  *input_curr_v;
    uint8_t  *ref_y;
    uint8_t  *ref_u;
    uint8_t  *ref_v;

    IME_MV ***ime_mv;

    uint8_t QP;
    uint8_t QP_cb_offset;
    uint8_t QP_cr_offset;

    /* output */
    uint8_t *ctu_recon_y;
    uint8_t *ctu_recon_uv;
};

struct INTERFACE_DBLK
{

};

struct INTERFACE_SAO_CALC
{
    /* input */
    uint8_t *inRecPixelY;
    uint8_t *inRecPixelU;
    uint8_t *inRecPixelV;
    uint8_t *orgPixelY;
    uint8_t *orgPixelU;
    uint8_t *orgPixelV;
    uint8_t  size;
    uint8_t  lumaLamba;
    uint8_t  chromaLambda;
    uint8_t  ctuPosX;
    uint8_t  ctuPosY;
    uint8_t  validWidth;
    uint8_t  validHeight;

    /* output */
    uint8_t *outRecPixelY;
    uint8_t *outRecPixelU;
    uint8_t *outRecPixelV;
    int      offset[4];
    uint8_t  bandIdx;
    bool     mergeLeftFlag;
    bool     mergeUpFlag;

};

struct INTERFACE_SAO_FILTER
{
    /* input */
    uint8_t *inRecPixelY;
    uint8_t *inRecPixelU;
    uint8_t *inRecPixelV;
    uint8_t *orgPixelY;
    uint8_t *orgPixelU;
    uint8_t *orgPixelV;
    uint8_t  size;
    uint8_t  lumaLamba;
    uint8_t  chromaLambda;
    uint8_t  ctuPosX;
    uint8_t  ctuPosY;
    uint8_t  validWidth;
    uint8_t  validHeight;

    /* output */
    uint8_t *outRecPixelY;
    uint8_t *outRecPixelU;
    uint8_t *outRecPixelV;
    int      offset[4];
    uint8_t  bandIdx;
    bool     mergeLeftFlag;
    bool     mergeUpFlag;

};


#endif
