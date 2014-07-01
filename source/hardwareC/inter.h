#ifndef INTER_H
#define INTER_H

#define QP_MAX 70
#define MV_MAX 32768
#define INT_MAX_MINE 2147483647  ///< max. value of signed 32-bit integer
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
#define REF_PIC_LIST0 0
#define REF_PIC_LIST1 1
const int nRectSize = 5; //refine search window size
const int nRimeWidth = 8;
const int nRimeHeight = 6;
const int nNeightMv = 2; //plus 1 is number of refine search windows
const int nAllNeighMv = 28;
const int g_nSearchRangeWidth = 208;
const int g_nSearchRangeHeight = 208;
const int nCtuSize = 64;
#define MAX_MRG_NUM_CANDS 5
#define MAX_AMVP_NUM_CANDS 2
#define INVALID -1
#define MIN_MINE(a, b) ((a) < (b) ? (a) : (b))
#define MAX_MINE(a, b) ((a) > (b) ? (a) : (b))
#define COPY_IF_LT(x, y, a, b, c, d) \
if ((y) < (x)) \
{ \
	(x) = (y); \
	(a) = (b); \
	(c) = (d); \
}
template<typename T>
inline T Clip(T x) { return MIN_MINE(T((1 << DEPTH) - 1), MAX_MINE(T(0), x)); }
struct Mv 
{
	short x;
	short y;
	int nSadCost;
	Mv() :x(0), y(0), nSadCost(INT_MAX_MINE){}
};

enum Part_Size
{
	size_2Nx2N,         ///< symmetric motion partition,  2Nx2N
	size_2NxN,          ///< symmetric motion partition,  2Nx N
	size_Nx2N,          ///< symmetric motion partition,   Nx2N
	size_NxN,           ///< symmetric motion partition,   Nx N
	size_2NxnU,         ///< asymmetric motion partition, 2Nx( N/2) + 2Nx(3N/2)
	size_2NxnD,         ///< asymmetric motion partition, 2Nx(3N/2) + 2Nx( N/2)
	size_nLx2N,         ///< asymmetric motion partition, ( N/2)x2N + (3N/2)x2N
	size_nRx2N,         ///< asymmetric motion partition, (3N/2)x2N + ( N/2)x2N
	size_NONE = 15
};
enum Slice_Type
{
	b_slice,
	p_slice,
	i_slice
};
enum AMVP_DIR
{
	AMVP_LEFT = 0,        ///< MVP of left block
	AMVP_ABOVE,           ///< MVP of above block
	AMVP_ABOVE_RIGHT,     ///< MVP of above right block
	AMVP_BELOW_LEFT,      ///< MVP of below left block
	AMVP_ABOVE_LEFT       ///< MVP of above left block
};
struct TEMPORAL_MV
{
	unsigned int        pred_l0 : 1;
	unsigned int        pred_l1 : 1;
	unsigned int        long_term_l0 : 1; //无效
	unsigned int        long_term_l1 : 1; //无效
	short                   delta_poc_l0 : 16;        // -32768 ~ 32767
	short                   delta_poc_l1 : 16;        // -32768 ~ 32767
	short                   mv_x_l0 : 16;
	short                   mv_y_l0 : 16;
	short                   mv_x_l1 : 16;
	short                   mv_y_l1 : 16;
} ;  //COL_MV;
struct SPATIAL_MV
{
	unsigned char     valid : 1;
	unsigned char     pred_flag[2];   // 1bit each
	char                    ref_idx[2];     // 4bit each
	//char                    pic_idx[2];     // 4bit each  for calculate current reference picture poc
	short                   delta_poc[2];
	Mv                      mv[2];          // 16bit each

	//// debug
	//UInt64      ctu_count;
	//UInt64      cu_count;
	//UInt64      pu_count;
};  //MV_INFO;
struct AmvpInfo
{
	Mv m_mvCand[MAX_AMVP_NUM_CANDS + 1];  ///< array of motion vector predictor candidates
	int      m_num;                             ///< number of motion vector predictor candidates
};

struct MvField
{
	Mv      mv;
	int       refIdx;
	MvField() : refIdx(INVALID) {}
	void setMvField(const Mv & _mv, int _refIdx)
	{
		mv.x = _mv.x;
		mv.y = _mv.y;
		refIdx = _refIdx;
	}
};

struct MergeInfoForCabac 
{
	bool                         m_bSkipFlag;
	bool                         m_bMergeFlags;
	char                         m_mergeIdx;
	char                         m_predModes;
	unsigned char          m_cbfY;
	unsigned char          m_cbfU;
	unsigned char          m_cbfV;
	Part_Size                  m_partSize;	
	MergeInfoForCabac() : m_bMergeFlags(true), m_predModes(0), m_partSize(size_2Nx2N), m_cbfY(1), m_cbfU(1), m_cbfV(1){}
};

struct FmeInfoForCabac
{
	bool                         m_bSkipFlag;
	bool                         m_bMergeFlags;
	char                         m_predModes;
	char                         m_refIdx[2];
	char                         m_mvpIdx[2];
	unsigned char          m_interDir;
	unsigned char          m_cbfY;
	unsigned char          m_cbfU;
	unsigned char          m_cbfV;
	short                        m_mvdx[2]; //x of mvd
	short                        m_mvdy[2]; //y of mvd
	Part_Size                  m_partSize;
	FmeInfoForCabac() :m_bSkipFlag(false), m_bMergeFlags(false), m_predModes(0), m_partSize(size_2Nx2N), m_cbfY(1), m_cbfU(1), m_cbfV(1){}
};
const int nMaxRefPic = 5*2; //ref picture number of list0 + ref picture number of list
extern Mv g_leftPMV[nMaxRefPic];
extern Mv g_leftPmv[nMaxRefPic];
extern Mv g_RimeMv[nMaxRefPic][85];
extern Mv g_FmeMv[nMaxRefPic][85];
extern Mv g_mvAmvp[nMaxRefPic][85][2];
extern MvField g_mvMerge[85][3*2];
extern Mv g_Mvmin[nMaxRefPic][nNeightMv + 1];
extern Mv g_Mvmax[nMaxRefPic][nNeightMv + 1];
extern short g_FmeResiY[4][64 * 64];
extern short g_FmeResiU[4][32 * 32];
extern short g_FmeResiV[4][32 * 32];
extern short g_MergeResiY[4][64 * 64];
extern short g_MergeResiU[4][32 * 32];
extern short g_MergeResiV[4][32 * 32];
extern class CoarseIntMotionEstimation Cime;
extern class RefineIntMotionEstimation Rime;
extern class FractionMotionEstimation Fme;
extern class MergeProc Merge;
extern class MergeMvpCand MergeCand;
extern class AmvpCand Amvp;
extern short OffsFromCtu64[85][2];
extern short OffsFromCtu32[21][2];
extern short OffsFromCtu16[5][2];
extern unsigned int RasterScan[85];
extern unsigned int RasterToZscan[256];
extern unsigned int ZscanToRaster[256];
extern unsigned int RasterToPelY[256];
extern unsigned int RasterToPelX[256];
extern double Lambda[QP_MAX];
extern int LambdaMotionSAD_tab_non_I[QP_MAX];
#define RK_NULL 0

extern short g_mergeResiY[4][64*64];
extern short g_mergeResiU[4][32*32];    
extern short g_mergeResiV[4][32*32];
extern short g_mergeCoeffY[4][64*64];
extern short g_mergeCoeffU[4][32*32];
extern short g_mergeCoeffV[4][32*32];
extern short g_fmeResiY[4][64*64];
extern short g_fmeResiU[4][32*32];    
extern short g_fmeResiV[4][32*32];
extern short g_fmeCoeffY[4][64*64];
extern short g_fmeCoeffU[4][32*32];
extern short g_fmeCoeffV[4][32*32];
extern bool  g_fme;
extern bool	 g_merge;
extern unsigned int  g_mergeBits[85];
extern unsigned int  g_fmeBits[85];

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
	int                                               m_nRefPic[2];
	Slice_Type                                    m_nSliceType;
	struct Mv**                                  m_mvNeighMv; //list have 2 directions


	//CIME output
	struct Mv*                                   m_mvCimeOut; //mv for current CTU

	//CIME not for input or output, only for compare
	short                                           **m_pRefPicDS; //down sampled reference picture, for test
	unsigned char                             *m_pOrigPic; //current original picture, for fetching current CTU pixel

	//static member array
	static unsigned short                   m_pMvCost[QP_MAX][MV_MAX * 2];

public:
	CoarseIntMotionEstimation();
	~CoarseIntMotionEstimation();

	void Create(int nCimeSwWidth, int nCimeSwHeight, int nSize = 64, unsigned int nDownSampDist = 4);
	void Destroy();
	void Prefetch(unsigned char *pCurrPic, short *pRefPic); //pRefPic is downscale reference picture
	void setDownSamplePic(unsigned char *pRefPic, int stride, int nRefPicIdx); //only for test
	int SAD(short *src_1, short *src_2, int width, int height, int stride);
	void CIME(unsigned char *pCurrPic, short *pRefPic, int nRefPicIdx = 0); //coarse IME procedure

	//get or set function of member variable
	void setCimeSwWidth(int nCimeSwWidth){ m_nCimeSwWidth = nCimeSwWidth; }
	void setCimeSwHeight(int nCimeSwHeight){ m_nCimeSwHeight = nCimeSwHeight; }
	void setPicWidth(int nPicWidth){ m_nPicWidth = nPicWidth; }
	void setPicHeight(int nPicHeight){ m_nPicHeight = nPicHeight; }
	void setCtuSize(int nSize){ m_nCtuSize = nSize; }
	void setDownSampDist(int nDownSampDist){ m_nDownSampDist = nDownSampDist; }
	void setCtuPosInPic(int nCtuPosInPic){ m_nCtuPosInPic = nCtuPosInPic; }
	void setOrigPic(unsigned char *pCurrOrigPic, int stride);
	void setSliceType(Slice_Type nSliceType){ m_nSliceType = nSliceType; }
	Slice_Type getSliceType(){ return m_nSliceType; }
	void setRefPicNum(int nRefPic, int list){ m_nRefPic[list] = nRefPic; }
	int getRefPicNum(int list){ return m_nRefPic[list]; }
	unsigned char *getOrigPic(){ return m_pOrigPic; }
	short *getRefPicDS(int nRefPicIdx){ return m_pRefPicDS[nRefPicIdx]; }
	void setNeighMv(Mv pMv, int nRefPicIdx, int idx){ m_mvNeighMv[nRefPicIdx][idx].x = pMv.x; m_mvNeighMv[nRefPicIdx][idx].y = pMv.y; }
	Mv getNeighMv(int nRefPicIdx, int idx){ return m_mvNeighMv[nRefPicIdx][idx]; }
	Mv getCimeMv(int nRefPicIdx){ return m_mvCimeOut[nRefPicIdx]; }

	//calculate mv cost
	void setQP(char qp){ m_nQP = qp; }
	char getQP(){ return m_nQP; }
	unsigned short mvcost(Mv mvp, Mv mv);
	void setMvp(Mv *mvp, int nRefPicIdx);
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
	int                                              m_nRefPic[2];
	Slice_Type                                   m_nSliceType;
	struct Mv                                    m_mvRefine[nMaxRefPic][3]; //0: CIME output PMV; 1. first neighbor PMV; 2. second neighbor PMV
	struct Mv                                    m_mvNeigh[nMaxRefPic][nAllNeighMv];
	unsigned char                             *m_pOrigPic; //current original picture, for fetching current CTU pixel
	unsigned char                             *m_pRefPic[nMaxRefPic]; //reference picture

	//RIME output
	struct Mv                                    m_mvRimeOut[nMaxRefPic]; //RIME output mv, send to FME

	//FME output
	struct Mv                                    m_mvFmeOut[nMaxRefPic]; //FME calculate output MV and cost
	unsigned char                             *m_pCuForFmeInterp[nMaxRefPic]; //current CU extend 4 pixels at 4 directions with best SATD corresponding MV

	//static member array
	static unsigned short                   m_pMvCost[QP_MAX][MV_MAX * 2];
	static unsigned char                    m_pCurrCtuPel[nCtuSize*nCtuSize*3/2];
	static unsigned char                    m_pCurrCtuRefPic[nMaxRefPic][3][(nCtuSize + 2*(nRimeWidth + 4))*(nCtuSize + 2*(nRimeHeight + 4))*3/2];

public:
	RefineIntMotionEstimation();
	~RefineIntMotionEstimation();
	void CreatePicInfo();
	void DestroyPicInfo();
	void CreateCuInfo(int nRimeSwWidth, int nRimeSwHeight, int nSize);
	void DestroyCuInfo();
	void PrefetchCtuLumaInfo(int nRefPicIdx);
	void PrefetchCtuChromaInfo(int nRefPicIdx);
	bool PrefetchCuInfo(int nRefPicIdx);
	int SAD(unsigned char *pCuRimeSW, int stride, unsigned char *pCurrCu, int width, int height);
	void RimeAndFme(int nRefPicIdx);

	//get or set function of member variable
	void setRimeSwWidth(int nCimeSwWidth){ m_nRimeSwWidth = nCimeSwWidth; }
	void setRimeSwHeight(int nCimeSwHeight){ m_nRimeSwHeight = nCimeSwHeight; }
	void setPicWidth(int nPicWidth){ m_nPicWidth = nPicWidth; }
	void setPicHeight(int nPicHeight){ m_nPicHeight = nPicHeight; }
	void setCtuSize(int nSize){ m_nCtuSize = nSize; }
	void setCuSize(int nCuSize){ m_nCuSize = nCuSize; }
	void setCtuPosInPic(int nCtuPosInPic){ m_nCtuPosInPic = nCtuPosInPic; }
	void setCuPosInCtu(int nCuPosInCtu){ m_nCuPosInCtu = nCuPosInCtu; }
	void setRefPicNum(int nRefPic, int list){ m_nRefPic[list] = nRefPic; }
	int getRefPicNum(int list){ return m_nRefPic[list]; }
	void setSliceType(Slice_Type nSliceType){ m_nSliceType = nSliceType; }
	Slice_Type getSliceType(){ return m_nSliceType; }
	void setRefPic(unsigned char *pRefPicY, int stride_Y, unsigned char *pRefPicU, int stride_U, unsigned char *pRefPicV, int stride_V, int nRefPicIdx);
	void setOrigPic(unsigned char *pOrigPicY, int stride_Y, unsigned char *pOrigPicU,int stride_U, unsigned char *pOrigPicV, int stride_V);
	unsigned char *getOrigPic(){ return m_pOrigPic; }
	unsigned char *getCurrCuPel(){ return m_pCurrCuPel; }
	unsigned char *getCuForFmeInterp(int nRefPicIdx){ return m_pCuForFmeInterp[nRefPicIdx]; }
	void setPmv(Mv pMvNeigh[nAllNeighMv], Mv mvCpmv, int nRefPicIdx);
	void getPmv(Mv &tmpMv, int nRefPicIdx, int idx){ tmpMv.x = m_mvRefine[nRefPicIdx][idx].x; tmpMv.y = m_mvRefine[nRefPicIdx][idx].y; }
	Mv getRimeMv(int nRefPicIdx){ return m_mvRimeOut[nRefPicIdx]; }
	Mv getFmeMv(int nRefPicIdx){ return m_mvFmeOut[nRefPicIdx]; }

	//static function
	static unsigned char *getCurrCtuRefPic(int nRefPicIdx, int idx){ return m_pCurrCtuRefPic[nRefPicIdx][idx]; }

	//calculate mv cost
	void setQP(char qp){ m_nQP = qp; }
	char getQP(){ return m_nQP; }
	unsigned short mvcost(Mv mvp, Mv mv);
	void setMvp(Mv *mvp, Mv pMvNeigh[nAllNeighMv]);
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
	struct Mv                                    m_mvAmvp[nMaxRefPic][2];
	struct Mv                                    m_mvFmeInput[nMaxRefPic];
	unsigned char                             *m_pFmeInterpPel[nMaxRefPic];
	unsigned char                             *m_pCurrCuPel;
	int                                               m_nPicWidth;
	int                                               m_nPicHeight;
	int                                               m_nCuSize;
	int                                               m_nCtuSize;
	int                                               m_nCtuPosInPic;  //CTU z scan position in the picture
	int                                               m_nCuPosInCtu;
	int                                               m_nRefPic[2];
	char                                            m_nQP;
	Slice_Type                                    m_nSliceType;
	FmeInfoForCabac                        m_fmeInfoForCabac;

	//FME output
	short                                           *m_pCurrCuResi[nMaxRefPic];
	unsigned char                             *m_pCurrCuPred[nMaxRefPic];
	short                                           *m_pBestResi;
	unsigned char                             *m_pBestPred;
	int                                               m_nMvpIdx[nMaxRefPic];
	struct Mv                                     m_mvMvd[nMaxRefPic];

	//static member
	static unsigned short                   m_pMvCost[QP_MAX][MV_MAX * 2];
	static float                                   m_dBitsizes[MV_MAX];

public:
	FractionMotionEstimation();
	~FractionMotionEstimation();
	//create or destroy member variable memory
	void Create();
	void Destroy();

	//get or set member variable
	unsigned short mvcost(Mv mvp, Mv mv);
	void setMvpCost();
	void CalculateLogs();
	unsigned short bitcost(Mv mvp, Mv mv);
	void setMvFmeInput(Mv mvFmeInput, int nRefPicIdx)
	{ 
		m_mvFmeInput[nRefPicIdx].x = mvFmeInput.x; 
		m_mvFmeInput[nRefPicIdx].y = mvFmeInput.y;
		m_mvFmeInput[nRefPicIdx].nSadCost = mvFmeInput.nSadCost;
	}
	Mv getMvFme(int idx){ return m_mvFmeInput[idx]; }
	Mv *getAmvp(int nRefPicIdx) { return m_mvAmvp[nRefPicIdx]; }
	void setAmvp(Mv mvAmvp[2], int nRefPicIdx)
	{ 
		m_mvAmvp[nRefPicIdx][0].x = mvAmvp[0].x; m_mvAmvp[nRefPicIdx][0].y = mvAmvp[0].y;
		m_mvAmvp[nRefPicIdx][1].x = mvAmvp[1].x; m_mvAmvp[nRefPicIdx][1].y = mvAmvp[1].y;
	}
	Mv getMvd(int nRefPicIdx){ return m_mvMvd[nRefPicIdx]; }
	int getMvpIdx(int nRefPicIdx){ return m_nMvpIdx[nRefPicIdx]; }
	Slice_Type getSliceType(){ return m_nSliceType; }
	void setSliceType(Slice_Type nSliceType){ m_nSliceType = nSliceType; }
	int getRefPicNum(int list){ return m_nRefPic[list]; }
	void setRefPicNum(int nRefPic, int list){ m_nRefPic[list] = nRefPic; }
	short *getCurrCuResi(){ return m_pBestResi; }
	FmeInfoForCabac* getFmeInfoForCabac(){ return &m_fmeInfoForCabac; }
	unsigned char *getCurrCuPred(){ return m_pBestPred; }
	void setFmeInterpPel(unsigned char *src, int src_stride, int nRefPicIdx);
	void setCurrCuPel(unsigned char *src, int src_stride);
	void setQP(char qp){ m_nQP = qp; }
	char getQP(){ return m_nQP; }
	void setPicWidth(int nPicWidth){ m_nPicWidth = nPicWidth; }
	void setPicHeight(int nPicHeight){ m_nPicHeight = nPicHeight; }
	void setCtuSize(int nSize){ m_nCtuSize = nSize; }
	void setCuSize(int nCuSize){ m_nCuSize = nCuSize; }
	void setCtuPosInPic(int nCtuPosInPic){ m_nCtuPosInPic = nCtuPosInPic; }
	void setCuPosInCtu(int nCuPosInCtu){ m_nCuPosInCtu = nCuPosInCtu; }

	//FME interpolate
	void CalcResiAndMvd();
	void xCalcResiAndMvd(int nRefPicIdx, bool isLuma = true, bool isChroma = true, bool isCalcMvd = true);
	void CopyToBest(unsigned char *pCurrCuPred);
	int satd_8x8(unsigned char *pix1, unsigned char *pix2);
	int satd_8x4(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2);
	unsigned int abs2(unsigned int a)
	{
		unsigned int s = ((a >> (BITS_PER_VALUE - 1)) & (((unsigned int)1 << BITS_PER_VALUE) + 1)) * ((unsigned short)-1);
		return (a + s) ^ s;
	}
	void Avg(unsigned char *pAvg, unsigned char *pRef0, unsigned char *pRef1);
	void addAvg(int nBestPrevIdx, int nBestPostIdx, bool bLuma, bool bChroma);
	void PredInterBi(int nRefPicIdx, bool isLuma = true, bool isChroma = true);
	void xPredInterBi(int nRefPicIdx, bool isLuma = true, bool isCr = false);
	int getCost(int bits){ return ((bits * LambdaMotionSAD_tab_non_I[(int)m_nQP] + 32768) >> 16); }
	void ConvertPelToShortBi(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height);
	void InterpHorizonBi(unsigned char *src, int srcStride, short *dst, short dstStride, int coeffIdx, int isRowExt, int N, int width, int height);
	void InterpVerticalBi(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *c, int N);
	void FilterVerticalBi(short *src, int srcStride, short *dst, int dstStride, int width, int height, const int coefIdx, int N);
	void InterpLuma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, int N, int nRefPicIdx);
	void InterpHorizLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpVertLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void FilterHorizonLuma(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff, int N);
	void FilterVertLuma(short *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpChroma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, bool isCb, int N, int nRefPicIdx);
	void InterpHorizChroma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpVertChroma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void FilterHorizonChroma(unsigned char *src, int srcStride, short *dst, int dstStride, int coeffIdx, int isRowExt, int N, int width, int height);
	void FilterVertChroma(short *src, int srcStride, unsigned char *dst, int dstStride, int width, int height, int coeffIdx, int N);
};

class MergeMvpCand
{
private:
	//merge candidate input
	TEMPORAL_MV                   m_mvTemporal[2][16]; //store temporal information of 2 CTU
	SPATIAL_MV                        m_mvSpatial[5]; //store A1,B1,B0,A0,B2
	SPATIAL_MV                        m_mvSpatialForCtu[18];
	SPATIAL_MV                        m_mvSpatialForCu64[4];
	SPATIAL_MV                        m_mvSpatialForCu32[9];
	SPATIAL_MV                        m_mvSpatialForCu16[10];
	SPATIAL_MV                        m_mvSpatialForCu8[7];
	int                                       m_nRefPic[2];
	int                                       m_nMergeCand;
	int                                       m_nCuSize;
	int                                       m_nCtuSize;
	int                                       m_nCuPosInCtu;
	int                                       m_nCtuPosInPic;
	int                                       m_nPicWidth;
	int                                       m_nPicHeight;
	int                                       m_nMergeLevel;
	int                                       m_nCurrPicPoc;
	int                                       m_nCurrRefPicPoc[2][nMaxRefPic / 2];
	int                                       m_nFromL0Flag;
	bool                                    m_bCheckLDC;
	bool                                    m_bTMVPFlag;
	Slice_Type                           m_nSliceType;

	//merge candidate output
	MvField                m_mvFieldNeighbours[5*2]; //two directions
	unsigned char       m_nInterDirNeighbours[5];

public:
	MergeMvpCand();
	~MergeMvpCand();

	//set or get function
	void setCuSize(int nCuSize){ m_nCuSize = nCuSize; }
	void setCtuSize(int nSize){ m_nCtuSize = nSize; }
	void setCuPosInCtu(int nCuPosInCtu){ m_nCuPosInCtu = nCuPosInCtu; }
	void setCtuPosInPic(int nCtuPosInPic){ m_nCtuPosInPic = nCtuPosInPic; }
	void setPicWidth(int nPicWidth){ m_nPicWidth = nPicWidth; }
	void setPicHeight(int nPicHeight){ m_nPicHeight = nPicHeight; }
	void setMergeLevel(int nMergeLevel){ m_nMergeLevel = nMergeLevel; }
	void setCurrPicPoc(int nCurrPicPoc){ m_nCurrPicPoc = nCurrPicPoc; }
	void setCurrRefPicPoc(int nCurrRefPicPoc[2], int list);
	void setCheckLDC(bool isCheckLDC){ m_bCheckLDC = isCheckLDC; }
	void setFromL0Flag(bool isFromL0Flag){ m_nFromL0Flag = isFromL0Flag; }
	void setTMVPFlag(bool isTMVP = true){ m_bTMVPFlag = isTMVP; }
	void setSliceType(Slice_Type nSliceType){ m_nSliceType = nSliceType; }
	void setMvTemporal(TEMPORAL_MV mvTemporal, int cuIdx, int nPos);
	void setMvSpatial(SPATIAL_MV mvSpatial, int idx);
	void setMvSpatialForCtu(SPATIAL_MV mvSpatial, int idx);
	void setMvSpatialForCu64(SPATIAL_MV mvSpatial, int idx);
	void setMvSpatialForCu32(SPATIAL_MV mvSpatial, int idx);
	void setMvSpatialForCu16(SPATIAL_MV mvSpatial, int idx);
	void setMvSpatialForCu8(SPATIAL_MV mvSpatial, int idx);
	void setRefPicNum(int nRefPicNum[2]){ m_nRefPic[0] = nRefPicNum[0]; m_nRefPic[1] = nRefPicNum[1]; }
	void setMergeCandNum(int nMergeCand){ m_nMergeCand = nMergeCand; }
	int getMergeCandNum(){ return m_nMergeCand; }
	void getMvSpatialForCtu(SPATIAL_MV &mvSpatial, int idx);
	void getMvSpatialForCu64(SPATIAL_MV &mvSpatial, int idx);
	void getMvSpatialForCu32(SPATIAL_MV &mvSpatial, int idx);
	void getMvSpatialForCu16(SPATIAL_MV &mvSpatial, int idx);
	void getMvSpatialForCu8(SPATIAL_MV &mvSpatial, int idx);
	MvField *getMvFieldNeighbours(){ return m_mvFieldNeighbours; }
	unsigned char *getInterDirNeighbours(){ return m_nInterDirNeighbours; }
	int getCurrPicPoc(){ return m_nCurrPicPoc; }
	int getCurrRefPicPoc(int list, int refIdx){ return m_nCurrRefPicPoc[list][refIdx]; }

	//produce n merge candidates from 2 temporal MV and 5 spatial MV
	void getMergeCandidates();
	void PrefetchAmvpCandInfo(unsigned int offsIdx);
	void UpdateMvpInfo(unsigned int offsIdx, SPATIAL_MV mvSpatial, bool isSplit);
	void deriveRightBottomIdx(unsigned int& outPartIdxRB);
	void xDeriveCenterIdx(unsigned int& outPartIdxCenter);
	bool isDiffMER(int xN, int yN, int xP, int yP);
	bool hasEqualMotion(SPATIAL_MV mvFirst, SPATIAL_MV mvSecond);
	bool xGetColMVP(int picList, int cuAddr, unsigned int partUnitIdx, Mv& outMV, bool bCheckLDC, int colFromL0Flag);
	int zscanToPos(unsigned int zscan);
	int xGetDistScaleFactor(int diffPocB, int diffPocD);
	Mv scaleMv(Mv mv, int scale);
};

class AmvpCand
{
private:
	//merge candidate input
	TEMPORAL_MV    m_mvTemporal[2][16]; //store temporal information of 2 CTU
	SPATIAL_MV         m_mvSpatial[5]; //store A1,B1,B0,A0,B2
	SPATIAL_MV         m_mvSpatialForCtu[18];
	SPATIAL_MV         m_mvSpatialForCu64[4];
	SPATIAL_MV         m_mvSpatialForCu32[9];
	SPATIAL_MV         m_mvSpatialForCu16[10];
	SPATIAL_MV         m_mvSpatialForCu8[7];
	int                        m_nCuSize;
	int                        m_nCtuSize;
	int                        m_nCuPosInCtu;
	int                        m_nCtuPosInPic;
	int                        m_nPicWidth;
	int                        m_nPicHeight;
	int                        m_nCurrPicPoc;
	int                        m_nCurrRefPicPoc[2][nMaxRefPic/2]; //support 5 reference picture of each list
	int                        m_nFromL0Flag;
	bool                     m_bTMVPFlag;
	bool                     m_bCheckLDC;
	Slice_Type             m_nSliceType;

	//merge candidate output
	MvField                m_mvFieldNeighbours[5 * 2]; //two directions
	unsigned char       m_nInterDirNeighbours[5];

public:
	AmvpCand();
	~AmvpCand();

	//set or get function
	void setCuSize(int nCuSize){ m_nCuSize = nCuSize; }
	void setCtuSize(int nSize){ m_nCtuSize = nSize; }
	void setCuPosInCtu(int nCuPosInCtu){ m_nCuPosInCtu = nCuPosInCtu; }
	void setCtuPosInPic(int nCtuPosInPic){ m_nCtuPosInPic = nCtuPosInPic; }
	void setPicWidth(int nPicWidth){ m_nPicWidth = nPicWidth; }
	void setPicHeight(int nPicHeight){ m_nPicHeight = nPicHeight; }
	void setCurrPicPoc(int nCurrPicPoc){ m_nCurrPicPoc = nCurrPicPoc; }
	void setCurrRefPicPoc(int nCurrRefPicPoc[nMaxRefPic/2], int list);
	void setTMVPFlag(bool isTMVP = true){ m_bTMVPFlag = isTMVP; }
	void setCheckLDC(bool isCheckLDC){ m_bCheckLDC = isCheckLDC; }
	void setFromL0Flag(bool isFromL0Flag){ m_nFromL0Flag = isFromL0Flag; }
	void setSliceType(Slice_Type nSliceType){ m_nSliceType = nSliceType; }
	void setMvTemporal(TEMPORAL_MV mvTemporal, int cuIdx, int nPos);
	void setMvSpatial(SPATIAL_MV mvSpatial, int idx);
	void setMvSpatialForCtu(SPATIAL_MV mvSpatial, int idx);
	void setMvSpatialForCu64(SPATIAL_MV mvSpatial, int idx);
	void setMvSpatialForCu32(SPATIAL_MV mvSpatial, int idx);
	void setMvSpatialForCu16(SPATIAL_MV mvSpatial, int idx);
	void setMvSpatialForCu8(SPATIAL_MV mvSpatial, int idx);
	void getMvSpatialForCtu(SPATIAL_MV &mvSpatial, int idx);
	void getMvSpatialForCu64(SPATIAL_MV &mvSpatial, int idx);
	void getMvSpatialForCu32(SPATIAL_MV &mvSpatial, int idx);
	void getMvSpatialForCu16(SPATIAL_MV &mvSpatial, int idx);
	void getMvSpatialForCu8(SPATIAL_MV &mvSpatial, int idx);
	MvField *getMvFieldNeighbours(){ return m_mvFieldNeighbours; }
	unsigned char *getInterDirNeighbours(){ return m_nInterDirNeighbours; }
	int getCurrPicPoc(){ return m_nCurrPicPoc; }
	int getCurrRefPicPoc(int list, int refIdx){ return m_nCurrRefPicPoc[list][refIdx]; }

	//produce n merge candidates from 2 temporal MV and 5 spatial MV
	void fillMvpCand(int picList, int refIdx, AmvpInfo* info);
	void PrefetchAmvpCandInfo(unsigned int offsIdx);
	void UpdateMvpInfo(unsigned int offsIdx, SPATIAL_MV mvSpatial, bool isSplit);
	bool xAddMVPCand(AmvpInfo* info, int picList, int refIdx, AMVP_DIR dir);
	bool xAddMVPCandOrder(AmvpInfo* info, int picList, int refIdx, AMVP_DIR dir);
	void deriveRightBottomIdx(unsigned int& outPartIdxRB);
	void xDeriveCenterIdx(unsigned int& outPartIdxCenter);
	bool hasEqualMotion(SPATIAL_MV mvFirst, SPATIAL_MV mvSecond);
	bool xGetColMVP(int picList, int cuAddr, unsigned int partUnitIdx, Mv& outMV, int refIdx, bool bCheckLDC, int colFromL0Flag);
	int zscanToPos(unsigned int zscan);
	int xGetDistScaleFactor(int diffPocB, int diffPocD);
	Mv scaleMv(Mv mv, int scale);
};

class MergeProc
{
private:
	//merge input
	struct MvField                             m_mvMergeCand[3*2];
	struct Mv                                    m_mvNeighPmv[nMaxRefPic*2]; //2 neighbor PMVs to determine merge search window
	unsigned char                             *m_pMergeSW[3*2]; //3 merge candidates fetch data form 2 neighbor search window,exclude CPMV search window
	unsigned char                             *m_pCurrCuPel;
	bool                                            m_bMergeSwValid[3];
	int                                               m_nCuSize;
	int                                               m_nCtuSize;
	int                                               m_nCuPosInCtu;
	MergeInfoForCabac                    m_mergeInfoForCabac;

	//merge output
	short                                            *m_pCurrCuResi[nMaxRefPic];
	short                                            *m_pMergeResi;
	unsigned char                              *m_pMergePred;
	int                                                m_nMergeIdx;

public:
	MergeProc();
	~MergeProc();
	//create or destroy member variable memory
	void Create();
	void Destroy();

	//get or set member
	void setNeighPmv(Mv mvNeighPmv[nMaxRefPic*2]);
	void setMergeCand(MvField mvMergeCand[6]);
	void setCuSize(int nCuSize){ m_nCuSize = nCuSize; }
	void setCtuSize(int nSize){ m_nCtuSize = nSize; }
	void setCuPosInCtu(int nCuPosInCtu){ m_nCuPosInCtu = nCuPosInCtu; }
	bool *getMergeCandValid(){ return m_bMergeSwValid; }
	void PrefetchMergeSW(unsigned char *src_prev0, int stride_prev0, unsigned char *src_prev1, int stride_prev1, 
		unsigned char *src_post0, int stride_post0, unsigned char *src_post1, int stride_post1, int nMergeIdx);
	void setCurrCuPel(unsigned char *src, int src_stride);
	MergeInfoForCabac *getMergeInfoForCabac(){ return &m_mergeInfoForCabac; }
	short *getMergeResi(){ return m_pMergeResi; }
    unsigned char *getMergePred(){return m_pMergePred;}
	MvField getMergeCand(int nMergeIdx, bool isPost){ return m_mvMergeCand[nMergeIdx * 2 + isPost]; }
	int getMergeIdx(){ return m_nMergeIdx; }
    
	//merge interpolate
	void CalcMergeResi();
	void addAvg(unsigned char*pBestPred, int nMergeIdx, bool isLuma = true, bool isCr = true);
	void xPredInterBi(int nMergeIdx, int add, Mv qmv, bool isLuma, bool isCr);
	void ConvertPelToShortBi(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height);
	void InterpHorizonBi(unsigned char *src, int srcStride, short *dst, short dstStride, int coeffIdx, int isRowExt, int N, int width, int height);
	void InterpVerticalBi(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *c, int N);
	void FilterVerticalBi(short *src, int srcStride, short *dst, int dstStride, int width, int height, const int coefIdx, int N);
	void InterpLuma(unsigned char *pMergeForInterp, int stride_1, unsigned char *pMergePred, int stride_2, const Mv& qmv);
	void InterpHorizLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpVertLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void FilterHorizonLuma(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff, int N);
	void FilterVertLuma(short *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height);
	void InterpChroma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, bool isCb, int nBestMerge, int dir, int N = 4);
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