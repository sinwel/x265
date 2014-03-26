#ifndef INTER_H
#define INTER_H
//Information for IME/FME/MERGE flow
#include <math.h>
#include <stdio.h>
#include <string.h>
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define INTER_FME_TEST_1                                                 0
#define INTER_FME_TEST_2                                                 1
#define INTER_IME_TEST                                                     1
#define INTER_IME_DOWNSCALE_VS_NOT_DOWNSCALE    0
#define INTER_IME_VS_MERGE                                            0
#define INTER_MERGE_TEST                                                1
#define LOW_DEFINITION_CTU_SIZE                                   32
#define MAX_SHORT                                                           32767
#define MAX_INT_MINE                                                      2147483647
#define FILTER_PREC                                                           6
#define PIXEL_DEPTH                                                          8
#define LUMA_NTAPS                                                         8
#define CHROMA_NTAPS                                                    4
#define INTERNAL_PREC                                                     14
#define INTERNAL_OFFS                                                     (1 << (INTERNAL_PREC - 1))
#define BITS_PER_SUM                                                       (8 * sizeof(unsigned short))
#define NULL                                                                      0
#define MVP_IDX_BITS                                                        1
#define HADAMARDTRANS4(d0, d1, d2, d3, s0, s1, s2, s3) { \
	unsigned int t0 = s0 + s1; \
	unsigned int t1 = s0 - s1; \
	unsigned int t2 = s2 + s3; \
	unsigned int t3 = s2 - s3; \
	d0 = t0 + t2; \
	d2 = t0 - t2; \
	d1 = t1 + t3; \
	d3 = t1 - t3; \
}

enum BuffType
{
	BT_BOOL, //bool
	BT_CHAR,  //char
	BT_UCHAR, //unsigned char
	BT_SHORT, //short
	BT_USHORT, //unsigned short
	BT_INT, //int 
	BT_UINT, //unsigned int
	BT_LONG, //long
	BT_ULONG, //unsigned long
	BT_LONGLONG, //long long
	BT_ULONGLONG, //unsigned long long
	BT_FLOAT, //float
	BT_DOUBLE //double
};

struct Mv
{
	short   x;
	short   y;
};

struct InterInfo
{
	struct ImeInput
	{
		//unsigned short      nMeRangeX; //me的x分量
		//unsigned short      nMeRangeY; //me的y分量
		unsigned short      ImeSearchRangeWidth; //IME搜索宽度
		unsigned short      ImeSearchRangeHeight; //IME搜索高度
		//unsigned int          picWidth; //图像宽度
		//unsigned int          picHeight; //图像高度
		//unsigned char       nCuOffsFromCtu[85]; //cu相对于ctu的偏移
		bool                      isValidCu[85];
		unsigned char       *pImeSearchRange; //IME搜索窗
		unsigned char       *pCurrCtu; //当前CTU
	}imeinput;
	struct ImeOutput
	{
		short                     ImeMvX[85]; //IME输出mv的x分量,作为FME输入
		short                     ImeMvY[85]; //IME输出mv的y分量,作为FME输入
		unsigned int          nSadCost[85]; //IME输出mv对应的代价,作为FME输入
		//unsigned char       *pFmeSearchRange[85]; //FME和merge公用的搜索窗
	}imeoutput;
	struct FmeInput
	{
		short                     MergeMvX[85][2][3]; //merge按选项表的顺序填写3个candidate的mv, x分量, 2表示B帧的前后向
		short                     MergeMvY[85][2][3]; //merge按选项表的顺序填写3个candidate的mv, y分量, 2表示B帧的前后向
		short                     FmeMvX[85][2][2]; //amvp按选项表的顺序填写2个candidate的mv, x分量, 2表示B帧的前后向
		short                     FmeMvY[85][2][2]; //amvp按选项表的顺序填写2个candidate的mv, y分量, 2表示B帧的前后向
		unsigned short      FmeSearchRangeWidth; //FME搜索宽度
		unsigned short      FmeSearchRangeHeight; //FME搜索高度
		bool                      isValidCu[85];
		unsigned char       *pFmeSearchRange; //FME的搜索窗
		ImeOutput            imeoutput; //IME的输出作为输入
	}fmeinput;
	struct FmeOutput 
	{
		short                     FmeMvX[85]; //经过fme的输出mv, x分量
		short                     FmeMvY[85]; //经过fme的输出mv, y分量
		unsigned int          nSatdFme[85]; //经过fme输出的satd代价值
		unsigned int          nSatdMerge[85];
		unsigned int          nMergeIdx[85];
		unsigned char       *pResidual; //FME最终输出的残差
	}fmeoutput;
	struct MvpInput
	{

	}mvpinput;
	struct MvpOutput
	{
		short                     MergeMvX[5]; //merge按选项表的顺序填写5个candidate的mv, x分量
		short                     MergeMvY[5]; //merge按选项表的顺序填写5个candidate的mv, y分量
		short                     FmeMvX[2]; //amvp按选项表的顺序填写2个candidate的mv, x分量
		short                     FmeMvY[2]; //amvp按选项表的顺序填写2个candidate的mv, y分量
	}mvpoutput;
};
extern char  nCtuSize; //CTU尺寸，可配置
extern char  nSplitDepth; //搜索层数
extern char  nSampDist;
extern InterInfo interinfo;
extern short OffsFromCtu64[85][2];
extern short OffsFromCtu32[21][2];
extern short OffsFromCtu16[5][2];
extern unsigned char *pSearchRange;
extern unsigned char *pCurrCtu;
extern InterInfo::ImeOutput imeoutput;
extern InterInfo::FmeOutput fmeoutput;
extern unsigned char pixRefBuf[32*32];
extern unsigned char pixCurrBuf[32*32];
extern const short LumaFilter[4][LUMA_NTAPS];
extern const short ChromaFilter[8][CHROMA_NTAPS];

void ImePrefetch(InterInfo::ImeInput *pImeInput, int EdgeMinusPelNum, int *pImeSR, int width, int height);
void motionEstimate(InterInfo *pInterInfo);
void ImeProc(InterInfo::ImeInput *pImeInput, int *pImeSR, int *pCurrCtu, int merangex, int merangey, int nCtuSize, InterInfo::ImeOutput *pImeOutput);
void ImeProcCompare(InterInfo::ImeOutput *pImeOutput_x265, InterInfo::ImeOutput *pImeOutput_mine, const char, const char);
void FmeProcCompare(InterInfo::FmeOutput *pFmeOutput_x265, InterInfo::FmeOutput *pFmeOutput_mine, const char nCtuSize, const char nSplitDepth);
void IntMotionEstimate(int *pImeSearchRange, int *pCurrCtu, int nCuSize, int nCtuSize, int merangex, int merangey, int offsIdx, InterInfo::ImeOutput *pImeOutput);
int Sad(int *pCurrCuSearchRange, int merangex, int *pCurrCtu, Mv offset, int nCuSize, int nCtuSize);
void ResetImeParam(InterInfo::ImeOutput *pImeOutput);
void ResetFmeParam(InterInfo::FmeOutput *pFmeOutput);
void InterpOnlyHoriz(unsigned char *src, long long srcStride, unsigned char *dst, long long dstStride, int coeffIdx, int width, int height, int N = 8);
void InterpOnlyVert(unsigned char *src, long long srcStride, unsigned char *dst, long long dstStride, int coeffIdx, int width, int height, int N = 8);
void InterpHoriz(unsigned char *src, long long srcStride, short *dst, long long dstStride, int coeffIdx, int isRowExt, int width, int height, int N = 8);
void InterpVert(short *src, long long srcStride, unsigned char *dst, long long dstStride, int coeffIdx, int width, int height, int N = 8);
unsigned int abs2(unsigned int a);
int SATD_8x4(unsigned char *pix1, long long stride_pix1, unsigned char *pix2, long stride_pix2);
int SATD8(unsigned char *pix1, long long stride_pix1, unsigned char *pix2, long long stride_pix2, int w, int h);
int FractMotionEstimate(unsigned char *pRef, int RefStride, unsigned char *pFenc, int FencStride, const Mv qmv, int blockwidth, int blockheight);
void FmeProc(InterInfo::FmeInput *pFmeInput, char nCtuSize, char nSplitDepth, unsigned char *pCurrCtu, InterInfo::FmeOutput *pFmeOutput);
void FmePrefetch(InterInfo::FmeInput *pFmeInput, unsigned char *pFmeInputBuff, int BuffWidth, int BuffHeight, int offsIdx, int nCuSize, int nCtuSize);
void SelectBuffSize(int nCtuSize, int idx, int &nBuffSize);
bool isSplit(int nCtuSize, int nSplitDepth, int idx);
void MergeProc(InterInfo::FmeInput *pFmeInput, char nCtuSize, char nSplitDepth, unsigned char *pCurrCtu, InterInfo::FmeOutput *pFmeOutput);
void MergePrefetch(InterInfo::FmeInput *pFmeInput, unsigned char *pFmeInputBuff, int BuffWidth, int BuffHeight, int offsIdx, int nCuSize, int nCtuSize, Mv mv);
unsigned int getOffsetIdx(int nCtuSize, int nCuPelX, int nCuPelY, unsigned int width);
#endif