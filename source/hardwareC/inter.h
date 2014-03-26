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
		//unsigned short      nMeRangeX; //me��x����
		//unsigned short      nMeRangeY; //me��y����
		unsigned short      ImeSearchRangeWidth; //IME�������
		unsigned short      ImeSearchRangeHeight; //IME�����߶�
		//unsigned int          picWidth; //ͼ����
		//unsigned int          picHeight; //ͼ��߶�
		//unsigned char       nCuOffsFromCtu[85]; //cu�����ctu��ƫ��
		bool                      isValidCu[85];
		unsigned char       *pImeSearchRange; //IME������
		unsigned char       *pCurrCtu; //��ǰCTU
	}imeinput;
	struct ImeOutput
	{
		short                     ImeMvX[85]; //IME���mv��x����,��ΪFME����
		short                     ImeMvY[85]; //IME���mv��y����,��ΪFME����
		unsigned int          nSadCost[85]; //IME���mv��Ӧ�Ĵ���,��ΪFME����
		//unsigned char       *pFmeSearchRange[85]; //FME��merge���õ�������
	}imeoutput;
	struct FmeInput
	{
		short                     MergeMvX[85][2][3]; //merge��ѡ����˳����д3��candidate��mv, x����, 2��ʾB֡��ǰ����
		short                     MergeMvY[85][2][3]; //merge��ѡ����˳����д3��candidate��mv, y����, 2��ʾB֡��ǰ����
		short                     FmeMvX[85][2][2]; //amvp��ѡ����˳����д2��candidate��mv, x����, 2��ʾB֡��ǰ����
		short                     FmeMvY[85][2][2]; //amvp��ѡ����˳����д2��candidate��mv, y����, 2��ʾB֡��ǰ����
		unsigned short      FmeSearchRangeWidth; //FME�������
		unsigned short      FmeSearchRangeHeight; //FME�����߶�
		bool                      isValidCu[85];
		unsigned char       *pFmeSearchRange; //FME��������
		ImeOutput            imeoutput; //IME�������Ϊ����
	}fmeinput;
	struct FmeOutput 
	{
		short                     FmeMvX[85]; //����fme�����mv, x����
		short                     FmeMvY[85]; //����fme�����mv, y����
		unsigned int          nSatdFme[85]; //����fme�����satd����ֵ
		unsigned int          nSatdMerge[85];
		unsigned int          nMergeIdx[85];
		unsigned char       *pResidual; //FME��������Ĳв�
	}fmeoutput;
	struct MvpInput
	{

	}mvpinput;
	struct MvpOutput
	{
		short                     MergeMvX[5]; //merge��ѡ����˳����д5��candidate��mv, x����
		short                     MergeMvY[5]; //merge��ѡ����˳����д5��candidate��mv, y����
		short                     FmeMvX[2]; //amvp��ѡ����˳����д2��candidate��mv, x����
		short                     FmeMvY[2]; //amvp��ѡ����˳����д2��candidate��mv, y����
	}mvpoutput;
};
extern char  nCtuSize; //CTU�ߴ磬������
extern char  nSplitDepth; //��������
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