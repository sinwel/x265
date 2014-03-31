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
    uint8_t *reconEdgePixelY;
    uint8_t *reconEdgePixelU;
    uint8_t *reconEdgePixelV;
    uint8_t *bNeighborFlags;
    uint8_t  numIntraNeighbor;
    uint8_t  size;
    uint8_t  useStrongIntraSmoothing;

    /* output */
    uint32_t Distortion;
    uint32_t Bits;
    uint32_t totalCost;
    uint16_t *ResiY;
    uint16_t *ResiU;
    uint16_t *ResiV;
    uint8_t  *ReconY;
    uint8_t  *ReconU;
    uint8_t  *ReconV;
    uint8_t   partSize;
    uint8_t   predMode;
};

struct INTERFACE_INTER_PROC
{
    /* input */

    /* output */
    uint32_t Distortion;
    uint32_t Bits;
    uint32_t totalCost;
    uint16_t *ResiY;
    uint16_t *ResiU;
    uint16_t *ResiV;
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

#endif
