#ifndef __MODULE_INTERFACE_H__
#define __MODULE_INTERFACE_H__

#include "../Lib/TLibCommon/CommonDef.h"

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
    uint8_t   partSize;
    uint8_t   mergeFlag;
    uint8_t   mergeIdx;
    uint32_t  mvd;
    uint32_t  mvdIdx;
    uint32_t  mvp;
};

struct INTERFACE_CABAC
{
};

#endif
