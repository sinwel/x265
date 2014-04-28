#ifndef INTER_H
#define INTER_H

#define QP_MAX 52
#define MV_MAX 1024
#define INT_MAX 2147483647  ///< max. value of signed 32-bit integer
#define USHORT_MAX 32767

//for FME calculate
#define FILTER_PREC 6
#define DEPTH 8
#define LUMA_NTAPS 8
#define CHROMA_NTAPS 4
#define INTERNAL_PREC 14
#define INTERNAL_OFFS (1 << (INTERNAL_PREC - 1)) 
#define BITS_PER_VALUE (8 * sizeof(unsigned short))
#define HADAMARD4(d0, d1, d2, d3, s0, s1, s2, s3) { \
	unsigned int t0 = s0 + s1; \
	unsigned int t1 = s0 - s1; \
	unsigned int t2 = s2 + s3; \
	unsigned int t3 = s2 - s3; \
	d0 = t0 + t2; \
	d2 = t0 - t2; \
	d1 = t1 + t3; \
	d3 = t1 - t3; \
}
#define CTU_STRIDE 64

const int nRectSize = 5; //refine search window size
const int nRimeWidth = 8;
const int nRimeHeight = 6;
const int nNeightMv = 2; //plus 1 is number of refine search windows
const int g_nSearchRangeWidth = 128;
const int g_nSearchRangeHeight = 128;
const int nCtuSize = 64;
#define MIN_MINE(a, b) ((a) < (b) ? (a) : (b))
#define COPY_IF_LT(x, y, a, b, c, d) \
if ((y) < (x)) \
{ \
	(x) = (y); \
	(a) = (b); \
	(c) = (d); \
}

struct Mv 
{
	short x;
	short y;
	int nSadCost;
};

extern Mv g_leftPMV;
extern Mv g_leftPmv;
extern Mv g_RimeMv[85];
extern Mv g_FmeMv[85];
extern Mv g_mvAmvp[85][2];
extern Mv g_mvMerge[85][3];
extern Mv g_Mvmin[nNeightMv + 1];
extern Mv g_Mvmax[nNeightMv + 1];
extern unsigned char g_FmeResiY[4][64 * 64];
extern unsigned char g_FmeResiU[4][32 * 32];
extern unsigned char g_FmeResiV[4][32 * 32];
extern unsigned char g_MergeResiY[4][64 * 64];
extern unsigned char g_MergeResiU[4][32 * 32];
extern unsigned char g_MergeResiV[4][32 * 32];
extern class CoarseIntMotionEstimation Cime;
extern class RefineIntMotionEstimation Rime;
extern class FractionMotionEstimation Fme;
extern class MergeProc Merge;
extern short OffsFromCtu64[85][2];
extern short OffsFromCtu32[21][2];
extern short OffsFromCtu16[5][2];
extern double Lambda[QP_MAX];
#define RK_INTER_ME_TEST 1
#define RK_NULL 0

class CoarseIntMotionEstimation
{
private:
	//CIME input
	short                                          *m_pCimeSW; //CIME search window
	int                                              m_nCimeSwWidth;
	int                                              m_nCimeSwHeight;
	char                                           m_nQP; //One CTU have only one QP

	//CIME and RIME input
	unsigned char                             *m_pCurrCtu; 
	short                                           *m_pCurrCtuDS; //current down sampled CTU
	int                                               m_nCtuSize;
	int                                               m_nDownSampDist;
	int                                               m_nCtuPosInPic;  //CTU z scan position in the picture
	int                                               m_nPicWidth;
	int                                               m_nPicHeight;
	struct Mv                                     m_mvNeighMv[36];


	//CIME output
	struct Mv                                    m_mvCimeOut; //mv for current CTU

	//CIME not for input or output, only for compare
	short                                           *m_pRefPicDS; //down sampled reference picture, for test
	unsigned char                             *m_pOrigPic; //current original picture, for fetching current CTU pixel

	//static member array
	static unsigned short                   m_pMvCost[QP_MAX][MV_MAX * 2];

public:
	CoarseIntMotionEstimation();
	~CoarseIntMotionEstimation();

	void Create(int nCimeSwWidth, int nCimeSwHeight, int nCtuSize, unsigned int nDownSampDist = 4);
	void Destroy();
	void Prefetch(unsigned char *pCurrPic, short *pRefPic); //pRefPic is downscale reference picture
	void setDownSamplePic(unsigned char *pRefPic, int stride); //only for test
	int SAD(short *src_1, short *src_2, int width, int height, int stride);
	void CIME(unsigned char *pCurrPic, short *pRefPic); //coarse IME procedure

	//get or set function of member variable
	void setCimeSwWidth(int nCimeSwWidth){ m_nCimeSwWidth = nCimeSwWidth; }
	void setCimeSwHeight(int nCimeSwHeight){ m_nCimeSwHeight = nCimeSwHeight; }
	void setPicWidth(int nPicWidth){ m_nPicWidth = nPicWidth; }
	void setPicHeight(int nPicHeight){ m_nPicHeight = nPicHeight; }
	void setCtuSize(int nCtuSize){ m_nCtuSize = nCtuSize; }
	void setDownSampDist(int nDownSampDist){ m_nDownSampDist = nDownSampDist; }
	void setCtuPosInPic(int nCtuPosInPic){ m_nCtuPosInPic = nCtuPosInPic; }
	void setOrigPic(unsigned char *pCurrOrigPic, int stride);
	unsigned char *getOrigPic(){ return m_pOrigPic; }
	short *getRefPicDS(){ return m_pRefPicDS; }
	void setNeighMv(Mv pMv, int idx){ m_mvNeighMv[idx].x = pMv.x; m_mvNeighMv[idx].y = pMv.y; }
	Mv *getNeighMv(){ return m_mvNeighMv; }
	Mv getNeighMv(int idx){ return m_mvNeighMv[idx]; }
	Mv getCimeMv(){ return m_mvCimeOut; }

	//calculate mv cost
	void setQP(char qp){ m_nQP = qp; }
	char getQP(){ return m_nQP; }
	unsigned short mvcost(Mv mvp, Mv mv);
	void setMvp(Mv *mvp);
	void setMvpCost();
};

class RefineIntMotionEstimation
{
private:
	//RIME input
	unsigned char                             *m_pCuRimeSW[3]; //RIME 3 search window
	int                                              m_nRimeSwWidth;
	int                                              m_nRimeSwHeight;
	char                                            m_nQP; //One CTU have only one QP
	unsigned char                             *m_pCurrCuPel;
	int                                              m_nCtuSize;
	int                                              m_nCuSize;
	int                                              m_nCtuPosInPic;  //CTU z scan position in the picture
	int                                              m_nCuPosInCtu;
	int                                              m_nPicWidth;
	int                                              m_nPicHeight;
	struct Mv                                    m_mvRefine[3]; //0: CIME output PMV; 1. first neighbor PMV; 2. second neighbor PMV
	unsigned char                             *m_pOrigPic; //current original picture, for fetching current CTU pixel
	unsigned char                             *m_pRefPic; //reference picture

	//RIME output
	struct Mv                                    m_mvRimeOut; //RIME output mv, send to FME

	//FME output
	struct Mv                                    m_mvFmeOut; //FME calculate output MV and cost
	unsigned char                             *m_pCuForFmeInterp; //current CU extend 4 pixels at 4 directions with best SATD corresponding MV

	//static member array
	static unsigned short                   m_pMvCost[QP_MAX][MV_MAX * 2];
	static unsigned char                    m_pCurrCtuPel[nCtuSize*nCtuSize*3/2];
	static unsigned char                    m_pCurrCtuRefPic[3][(nCtuSize + 2*(nRimeWidth + 4))*(nCtuSize + 2*(nRimeHeight + 4))*3/2];

public:
	RefineIntMotionEstimation();
	~RefineIntMotionEstimation();
	void CreatePicInfo();
	void DestroyPicInfo();
	void CreateCuInfo(int nRimeSwWidth, int nRimeSwHeight, int nCtuSize);
	void DestroyCuInfo();
	void PrefetchCtuLumaInfo();
	void PrefetchCtuChromaInfo();
	bool PrefetchCuInfo();
	int SAD(unsigned char *pCuRimeSW, int stride, unsigned char *pCurrCu, int width, int height);
	void RimeAndFme(Mv mvNeigh[36]);

	//get or set function of member variable
	void setRimeSwWidth(int nCimeSwWidth){ m_nRimeSwWidth = nCimeSwWidth; }
	void setRimeSwHeight(int nCimeSwHeight){ m_nRimeSwHeight = nCimeSwHeight; }
	void setPicWidth(int nPicWidth){ m_nPicWidth = nPicWidth; }
	void setPicHeight(int nPicHeight){ m_nPicHeight = nPicHeight; }
	void setCtuSize(int nCtuSize){ m_nCtuSize = nCtuSize; }
	void setCuSize(int nCuSize){ m_nCuSize = nCuSize; }
	void setCtuPosInPic(int nCtuPosInPic){ m_nCtuPosInPic = nCtuPosInPic; }
	void setCuPosInCtu(int nCuPosInCtu){ m_nCuPosInCtu = nCuPosInCtu; }
	void setOrigAndRefPic(unsigned char *pOrigPicY, unsigned char *pRefPicY, int stride_Y, unsigned char *pOrigPicU,
		unsigned char *pRefPicU, int stride_U, unsigned char *pOrigPicV, unsigned char *pRefPicV, int stride_V);
	unsigned char *getOrigPic(){ return m_pOrigPic; }
	unsigned char *getCurrCuPel(){ return m_pCurrCuPel; }
	unsigned char *getCuForFmeInterp(){ return m_pCuForFmeInterp; }
	void setPmv(Mv pMvNeigh[36], Mv mvCpmv);
	void getPmv(Mv &tmpMv, int idx){ tmpMv.x = m_mvRefine[idx].x; tmpMv.y = m_mvRefine[idx].y; }
	Mv getRimeMv(){ return m_mvRimeOut; }
	Mv getFmeMv(){ return m_mvFmeOut; }

	//static function
	static unsigned char *getCurrCtuRefPic(int idx){ return m_pCurrCtuRefPic[idx]; }

	//calculate mv cost
	void setQP(char qp){ m_nQP = qp; }
	char getQP(){ return m_nQP; }
	unsigned short mvcost(Mv mvp, Mv mv);
	void setMvp(Mv *mvp, Mv pMvNeigh[36]);
	void setMvpCost();

	//FME calculate
	int subpelCompare(unsigned char *pfref, int stride_1, unsigned char *pfenc, int stride_2, const Mv& qmv);
	void InterpHorizLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpVertLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void FilterHorizonLuma(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff, int N);
	void FilterVertLuma(short *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	int satd_8x8(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2, int w, int h);
	int satd_8x4(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2);
	inline unsigned int abs2(unsigned int a)
	{
		unsigned int s = ((a >> (BITS_PER_VALUE - 1)) & (((unsigned int)1 << BITS_PER_VALUE) + 1)) * ((unsigned short)-1);
		return (a + s) ^ s;
	}
};

class FractionMotionEstimation
{
private:
	//FME input
	struct Mv                                    m_mvAmvp[2];
	struct Mv                                    m_mvFmeInput;
	unsigned char                             *m_pFmeInterpPel;
	unsigned char                             *m_pCurrCuPel;
	int                                               m_nPicWidth;
	int                                               m_nPicHeight;
	int                                               m_nCuSize;
	int                                               m_nCtuSize;
	int                                               m_nCtuPosInPic;  //CTU z scan position in the picture
	int                                               m_nCuPosInCtu;

	//FME output
	unsigned char                             *m_pCurrCuResi;
	int                                               m_nMvpIdx;
	struct Mv                                     m_mvMvd;

public:
	FractionMotionEstimation();
	~FractionMotionEstimation();
	//create or destroy member variable memory
	void Create();
	void Destroy();

	//get or set member variable
	void setMvFmeInput(Mv mvFmeInput){ m_mvFmeInput.x = mvFmeInput.x; m_mvFmeInput.y = mvFmeInput.y; }
	Mv *getAmvp() { return m_mvAmvp; }
	void setAmvp(Mv mvAmvp[2])
	{ 
		m_mvAmvp[0].x = mvAmvp[0].x; m_mvAmvp[0].y = mvAmvp[0].y; 
	    m_mvAmvp[1].x = mvAmvp[1].x; m_mvAmvp[1].y = mvAmvp[1].y; 
	}
	Mv getMvd(){ return m_mvMvd; }
	int getMvpIdx(){ return m_nMvpIdx; }
	unsigned char *getCurrCuResi(){ return m_pCurrCuResi; }
	void setFmeInterpPel(unsigned char *src, int src_stride);
	void setCurrCuPel(unsigned char *src, int src_stride);
	void setPicWidth(int nPicWidth){ m_nPicWidth = nPicWidth; }
	void setPicHeight(int nPicHeight){ m_nPicHeight = nPicHeight; }
	void setCtuSize(int nCtuSize){ m_nCtuSize = nCtuSize; }
	void setCuSize(int nCuSize){ m_nCuSize = nCuSize; }
	void setCtuPosInPic(int nCtuPosInPic){ m_nCtuPosInPic = nCtuPosInPic; }
	void setCuPosInCtu(int nCuPosInCtu){ m_nCuPosInCtu = nCuPosInCtu; }

	//FME interpolate
	void CalcResiAndMvd();
	void InterpLuma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, int N);
	void InterpHorizLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpVertLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void FilterHorizonLuma(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff, int N);
	void FilterVertLuma(short *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpChroma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, bool isCb, int N);
	void InterpHorizChroma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpVertChroma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void FilterHorizonChroma(unsigned char *src, int srcStride, short *dst, int dstStride, int coeffIdx, int isRowExt, int N, int width, int height);
	void FilterVertChroma(short *src, int srcStride, unsigned char *dst, int dstStride, int width, int height, int coeffIdx, int N);
};

class MergeProc
{
private:
	//merge input
	struct Mv                                    m_mvMergeCand[3];
	struct Mv                                    m_mvNeighPmv[2]; //2 neighbor PMVs to determine merge search window
	unsigned char                             *m_pMergeSW[3]; //3 merge candidates fetch data form 2 neighbor search window,exclude CPMV search window
	unsigned char                             *m_pCurrCuPel;
	bool                                            m_bMergeSwValid[3];
	int                                               m_nCuSize;
	int                                               m_nCtuSize;
	int                                               m_nCuPosInCtu;

	//merge output
	unsigned char                              *m_pMergeResi;
	int                                                m_nMergeIdx;

public:
	MergeProc();
	~MergeProc();
	//create or destroy member variable memory
	void Create();
	void Destroy();

	//get or set member
	void setNeighPmv(Mv mvNeighPmv[2])
	{ 
		m_mvNeighPmv[0].x = mvNeighPmv[0].x; m_mvNeighPmv[0].y = mvNeighPmv[0].y;
		m_mvNeighPmv[1].x = mvNeighPmv[1].x; m_mvNeighPmv[1].y = mvNeighPmv[1].y;
	}
	void setMergeCand(Mv mvMergeCand[3])
	{
		m_mvMergeCand[0].x = mvMergeCand[0].x; m_mvMergeCand[0].y = mvMergeCand[0].y;
		m_mvMergeCand[1].x = mvMergeCand[1].x; m_mvMergeCand[1].y = mvMergeCand[1].y;
		m_mvMergeCand[2].x = mvMergeCand[2].x; m_mvMergeCand[2].y = mvMergeCand[2].y;
	}
	void setCuSize(int nCuSize){ m_nCuSize = nCuSize; }
	void setCtuSize(int nCtuSize){ m_nCtuSize = nCtuSize; }
	void setCuPosInCtu(int nCuPosInCtu){ m_nCuPosInCtu = nCuPosInCtu; }
	bool *getMergeCandValid(){ return m_bMergeSwValid; }
	void PrefetchMergeSW(unsigned char *src_1, int stride_1, unsigned char *src_2, int stride_2);
	void setCurrCuPel(unsigned char *src, int src_stride);
	unsigned char *getMergeResi(){ return m_pMergeResi; }

	//merge interpolate
	void CalcResiAndMvd();
	void InterpLuma(unsigned char *pMergeForInterp, int stride_1, unsigned char *pMergePred, int stride_2, const Mv& qmv);
	void InterpHorizLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpVertLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void FilterHorizonLuma(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff, int N);
	void FilterVertLuma(short *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpChroma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, bool isCb, int nBestMerge, int N);
	void InterpHorizChroma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpVertChroma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void FilterHorizonChroma(unsigned char *src, int srcStride, short *dst, int dstStride, int coeffIdx, int isRowExt, int N, int width, int height);
	void FilterVertChroma(short *src, int srcStride, unsigned char *dst, int dstStride, int width, int height, int coeffIdx, int N);

	//calculate cost
	int SSE(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2, int lx, int ly);
	int SAD(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2, int lx, int ly);
	int SATD(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2, int w, int h);
	int satd_8x4(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2);
	inline unsigned int abs2(unsigned int a)
	{
		unsigned int s = ((a >> (BITS_PER_VALUE - 1)) & (((unsigned int)1 << BITS_PER_VALUE) + 1)) * ((unsigned short)-1);
		return (a + s) ^ s;
	}
};
#endif