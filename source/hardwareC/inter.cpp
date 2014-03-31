#include "inter.h"
#include <assert.h>

#if defined(__GNUC__) || defined(__clang__)    
#include <stdlib.h>
#endif


char                     nCtuSize = 64; //CTU尺寸，可配置
char                     nSplitDepth = 3; //搜索层数, 0：只搜索64x64；1：只搜索64x64，32x32；2：搜索64x64，32x32，16x16；3：全搜
char                     nSampDist = 4;
InterInfo              interinfo; //inter处理的输入输出信息保存
InterInfo::ImeOutput imeoutput; //用来实现每个CTU信息的对比
InterInfo::FmeOutput fmeoutput; //保存fme部分 用于比较
unsigned char *pSearchRange;
unsigned char *pCurrCtu;
unsigned char pixRefBuf[32*32];
unsigned char pixCurrBuf[32*32];

const short LumaFilter[4][LUMA_NTAPS] =
{
	{ 0, 0, 0, 64, 0, 0, 0, 0 },
	{ -1, 4, -10, 58, 17, -5, 1, 0 },
	{ -1, 4, -11, 40, 40, -11, 4, -1 },
	{ 0, 1, -5, 17, 58, -10, 4, -1 }
};

const short ChromaFilter[8][CHROMA_NTAPS] =
{
	{ 0, 64, 0, 0 },
	{ -2, 58, 10, -2 },
	{ -4, 54, 16, -2 },
	{ -6, 46, 28, -4 },
	{ -4, 36, 36, -4 },
	{ -4, 28, 46, -6 },
	{ -2, 16, 54, -4 },
	{ -2, 10, 58, -2 }
};

short OffsFromCtu64[85][2] =
{
	{ 0, 0 }, { 8, 0 }, { 16, 0 }, { 24, 0 }, { 32, 0 }, { 40, 0 }, { 48, 0 }, { 56, 0 }, //8x8
	{ 0, 8 }, { 8, 8 }, { 16, 8 }, { 24, 8 }, { 32, 8 }, { 40, 8 }, { 48, 8 }, { 56, 8 },
	{ 0, 16 }, { 8, 16 }, { 16, 16 }, { 24, 16 }, { 32, 16 }, { 40, 16 }, { 48, 16 }, { 56, 16 },
	{ 0, 24 }, { 8, 24 }, { 16, 24 }, { 24, 24 }, { 32, 24 }, { 40, 24 }, { 48, 24 }, { 56, 24 },
	{ 0, 32 }, { 8, 32 }, { 16, 32 }, { 24, 32 }, { 32, 32 }, { 40, 32 }, { 48, 32 }, { 56, 32 },
	{ 0, 40 }, { 8, 40 }, { 16, 40 }, { 24, 40 }, { 32, 40 }, { 40, 40 }, { 48, 40 }, { 56, 40 },
	{ 0, 48 }, { 8, 48 }, { 16, 48 }, { 24, 48 }, { 32, 48 }, { 40, 48 }, { 48, 48 }, { 56, 48 },
	{ 0, 56 }, { 8, 56 }, { 16, 56 }, { 24, 56 }, { 32, 56 }, { 40, 56 }, { 48, 56 }, { 56, 56 },
	{ 0, 0 }, { 16, 0 }, { 32, 0 }, { 48, 0 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, { 48, 16 }, //16x16
	{ 0, 32 }, { 16, 32 }, { 32, 32 }, { 48, 32 }, { 0, 48 }, { 16, 48 }, { 32, 48 }, { 48, 48 },
	{ 0, 0 }, { 32, 0 }, { 0, 32 }, { 32, 32 },  //32x32
	{ 0, 0 } //64x64
};

short OffsFromCtu32[21][2] =
{
	{ 0, 0 }, { 8, 0 }, { 16, 0 }, { 24, 0 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, { 24, 8 }, //8x8
	{ 0, 16 }, { 8, 16 }, { 16, 16 }, { 24, 16 }, { 0, 24 }, { 8, 24 }, { 16, 24 }, { 24, 24 },
	{ 0, 0 }, { 16, 0 }, { 0, 16 }, { 16, 16 }, //16x16
	{ 0, 0 } //32x32
};

short OffsFromCtu16[5][2] =
{
	{ 0, 0 }, { 8, 0 }, { 0, 8 }, { 8, 8 }, //8x8
	{ 0, 0 } //16x16
};

void motionEstimate(InterInfo *pInterInfo)
{
	assert((nCtuSize == 16) || (nCtuSize == 32) || (nCtuSize == 64));
	int EdgeMinusPelNum = 4;
	int width = pInterInfo->imeinput.ImeSearchRangeWidth;
	int height = pInterInfo->imeinput.ImeSearchRangeHeight;
	width -= EdgeMinusPelNum * 2;
	height -= EdgeMinusPelNum * 2;
	int *pImeSR = new int[(width - nSampDist + 1)*(height - nSampDist + 1)];
	int *pCurrCtu = new int[(nCtuSize/nSampDist)*(nCtuSize/nSampDist)];
	ImePrefetch(&pInterInfo->imeinput, EdgeMinusPelNum, pImeSR, width, height);
	for (int i = 0; i < nCtuSize / nSampDist; i++)
	{
		for (int j = 0; j < nCtuSize / nSampDist; j++)
		{
			int sum = 0;
			for (int x = 0; x < nSampDist; x++)
			{
				for (int y = 0; y < nSampDist; y++)
				{
					sum += (pInterInfo->imeinput.pCurrCtu[(j*nSampDist + y) + (i*nSampDist + x)*nCtuSize]);
				}
			}
			pCurrCtu[j + (nCtuSize / nSampDist)*i] = sum;
		}
	}
	ImeProc(&pInterInfo->imeinput, pImeSR, pCurrCtu, width, height, nCtuSize, &pInterInfo->imeoutput);
	memcpy(&pInterInfo->fmeinput.imeoutput, &pInterInfo->imeoutput, sizeof(InterInfo::ImeOutput));
	FmeProc(&pInterInfo->fmeinput, nCtuSize, nSplitDepth, pInterInfo->imeinput.pCurrCtu, &pInterInfo->fmeoutput);
	MergeProc(&pInterInfo->fmeinput, nCtuSize, nSplitDepth, pInterInfo->imeinput.pCurrCtu, &pInterInfo->fmeoutput);
	delete[] pImeSR;
	delete[] pCurrCtu;
}
bool isSplit(int nCtuSize, int nSplitDepth, int idx)
{
	bool tmp = false;
	if (64 == nCtuSize)
	{
		if (nSplitDepth <= 0)
		{
			if (84 == idx)
				tmp = true;
		}
		else if (nSplitDepth <= 1)
		{
			if (idx >= 80)
				tmp = true;
		}
		else if (nSplitDepth <= 2)
		{
			if (idx >= 64)
				tmp = true;
		}
		else
		{
			if (idx >= 0)
				tmp = true;
		}
	} 
	else if (32 == nCtuSize)
	{
		if (nSplitDepth <= 0)
		{
			if (20 == idx)
				tmp = true;
		}
		else if (nSplitDepth <= 1)
		{
			if (idx >= 16)
				tmp = true;
		}
		else
		{
			if (idx >= 0)
				tmp = true;
		}
	}
	else
	{
		if (nSplitDepth <= 0)
		{
			if (4 == idx)
				tmp = true;
		}
		else
		{
			if (idx >= 0)
				tmp = true;
		}
	}
	return tmp;
}
void ImePrefetch(InterInfo::ImeInput *pImeInput, int EdgeMinusPelNum, int *pImeSR, int width, int height)
{
	assert(EdgeMinusPelNum % 4 == 0);
	int RightMove = 0;
	int PelBit = 8;
	//pImeSR = (int *)Create(width-nSampDist, height-nSampDist, BT_INT); //保存中间(第二级IME)的搜索窗时用的数据类型为int
	unsigned char *pImeSearchRange = pImeInput->pImeSearchRange + EdgeMinusPelNum + (EdgeMinusPelNum)*pImeInput->ImeSearchRangeWidth;
	for (int i = 0; i <= height-nSampDist; i ++)
	{
		for (int j = 0; j <= width-nSampDist; j ++)
		{
			int sum = 0;
			for (int x = 0; x < nSampDist; x ++)
			{
				for (int y = 0; y < nSampDist; y ++)
				{
					sum += (pImeSearchRange[(j + y) + (i + x)*pImeInput->ImeSearchRangeWidth] >> (8 - PelBit));
				}
			}
			pImeSR[j + i*(width - nSampDist + 1)] = sum >> RightMove;
		}
	}
}
void ImeProc(InterInfo::ImeInput *pImeInput, int *pImeSR, int *pCurrCtu, int merangex, int merangey, int nCtuSize, InterInfo::ImeOutput *pImeOutput)
{
	int RightMove = 0;
	int PelBit = 8;
	int nCuInCtu;
	switch (nCtuSize)
	{
	case 64: nCuInCtu = 85; break;
	case 32: nCuInCtu = 21; break;
	case 16: nCuInCtu = 5;   break;
	default: nCuInCtu = 85;
	}
	//int *pCurrCtu = (int *)Create(nCtuSize/nSampDist, nCtuSize/nSampDist, BT_INT);//保存CTU下采样的点
	if (64 == nCtuSize)
	{
		for (int i = 0; i < nCuInCtu; i++)
		{
			if ((!pImeInput->isValidCu[i]) || !isSplit(nCtuSize, nSplitDepth, i))
				continue;
			if (i < 64)
				IntMotionEstimate(pImeSR, pCurrCtu, 8, nCtuSize, merangex, merangey, i, pImeOutput);
			else if (i < 80)
				IntMotionEstimate(pImeSR, pCurrCtu, 16, nCtuSize, merangex, merangey, i, pImeOutput);
			else if (i < 84)
				IntMotionEstimate(pImeSR, pCurrCtu, 32, nCtuSize, merangex, merangey, i, pImeOutput);
			else
				IntMotionEstimate(pImeSR, pCurrCtu, 64, nCtuSize, merangex, merangey, i, pImeOutput);
		}
	}
	else if (32 == nCtuSize)
	{
		for (int i = 0; i < nCuInCtu; i++)
		{
			if ((!pImeInput->isValidCu[i]) || !isSplit(nCtuSize, nSplitDepth, i))
				continue;
			if (i < 16)
				IntMotionEstimate(pImeSR, pCurrCtu, 8, nCtuSize, merangex, merangey, i, pImeOutput);
			else if (i < 20)
				IntMotionEstimate(pImeSR, pCurrCtu, 16, nCtuSize, merangex, merangey, i, pImeOutput);
			else
				IntMotionEstimate(pImeSR, pCurrCtu, 32, nCtuSize, merangex, merangey, i, pImeOutput);
		}
	}
	else
	{
		for (int i = 0; i < nCuInCtu; i++)
		{
			if ((!pImeInput->isValidCu[i]) || !isSplit(nCtuSize, nSplitDepth, i))
				continue;
			if (i < 4)
				IntMotionEstimate(pImeSR, pCurrCtu, 8, nCtuSize, merangex, merangey, i, pImeOutput);
			else
				IntMotionEstimate(pImeSR, pCurrCtu, 16, nCtuSize, merangex, merangey, i, pImeOutput);
		}
	}
}
void ImeProcCompare(InterInfo::ImeOutput *pImeOutput_x265, InterInfo::ImeOutput *pImeOutput_mine, const char nCtuSize, const char nSplitDepth)
{
	int nStartPosition;
	int nEndPosition;

	if (64 == nCtuSize)
	{
		nEndPosition = 84;
		if (3 <= nSplitDepth)
			nStartPosition = 0;
		else if (2 <= nSplitDepth)
			nStartPosition = 64;
		else if (1 <= nSplitDepth)
			nStartPosition = 80;
		else
			nStartPosition = 84;
	}
	else if (32 == nCtuSize)
	{
		nEndPosition = 20;
		if (2 <= nSplitDepth)
			nStartPosition = 0;
		else if (1 <= nSplitDepth)
			nStartPosition = 16;
		else
			nStartPosition = 20;
	}
	else
	{
		nEndPosition = 4;
		if (1 <= nSplitDepth)
			nStartPosition = 0;
		else
			nStartPosition = 4;
	}

	for (int i = nEndPosition; i >= nStartPosition ; i--)
	{
		assert(pImeOutput_mine->ImeMvX[i] == pImeOutput_x265->ImeMvX[i]);
		assert(pImeOutput_mine->ImeMvY[i] == pImeOutput_x265->ImeMvY[i]);
		assert(pImeOutput_mine->nSadCost[i] == pImeOutput_x265->nSadCost[i]);
	}
}
void FmeProcCompare(InterInfo::FmeOutput *pFmeOutput_x265, InterInfo::FmeOutput *pFmeOutput_mine, const char nCtuSize, const char nSplitDepth)
{
	int nStartPosition;
	int nEndPosition;

	if (64 == nCtuSize)
	{
		nEndPosition = 84;
		if (3 <= nSplitDepth)
			nStartPosition = 0;
		else if (2 <= nSplitDepth)
			nStartPosition = 64;
		else if (1 <= nSplitDepth)
			nStartPosition = 80;
		else
			nStartPosition = 84;
	}
	else if (32 == nCtuSize)
	{
		nEndPosition = 20;
		if (2 <= nSplitDepth)
			nStartPosition = 0;
		else if (1 <= nSplitDepth)
			nStartPosition = 16;
		else
			nStartPosition = 20;
	}
	else
	{
		nEndPosition = 4;
		if (1 <= nSplitDepth)
			nStartPosition = 0;
		else
			nStartPosition = 4;
	}

	for (int i = nEndPosition; i >= nStartPosition; i--)
	{
		assert(pFmeOutput_mine->FmeMvX[i] == pFmeOutput_x265->FmeMvX[i]);
		assert(pFmeOutput_mine->FmeMvY[i] == pFmeOutput_x265->FmeMvY[i]);
		assert(pFmeOutput_mine->nSatdFme[i] == pFmeOutput_x265->nSatdFme[i]);
		assert(pFmeOutput_mine->nMergeIdx[i] == pFmeOutput_x265->nMergeIdx[i]);
		assert(pFmeOutput_mine->nSatdMerge[i] == pFmeOutput_x265->nSatdMerge[i]);
	}
}
void IntMotionEstimate(int *pImeSearchRange, int *pCurrCtu, int nCuSize, int nCtuSize, int merangex, int merangey, int offsIdx, InterInfo::ImeOutput *pImeOutput)
{
	Mv bmv; bmv.x = 0; bmv.y = 0;
	Mv offset; 
	switch (nCtuSize)
	{
	case 64: offset.x = OffsFromCtu64[offsIdx][0]; offset.y = OffsFromCtu64[offsIdx][1]; break;
	case 32: offset.x = OffsFromCtu32[offsIdx][0]; offset.y = OffsFromCtu32[offsIdx][1]; break;
	case 16: offset.x = OffsFromCtu16[offsIdx][0]; offset.y = OffsFromCtu16[offsIdx][1]; break;
	default: offset.x = OffsFromCtu64[offsIdx][0]; offset.y = OffsFromCtu64[offsIdx][1];
	}
	Mv Center; Center.x = offset.x + merangex / 2 - nCtuSize / 2; Center.y = offset.y + merangey / 2 - nCtuSize / 2;
	Mv mvmin; mvmin.x = nCtuSize/2 - merangex / 2; mvmin.y = nCtuSize/2 - merangey / 2;
	Mv mvmax; mvmax.x = merangex / 2 - nCtuSize / 2; mvmax.y = merangey / 2 - nCtuSize / 2;
	int withSR = merangex - nSampDist + 1;
	int *pCurrCuSearchRangeCenter = pImeSearchRange + Center.x + Center.y*withSR;
	int bcost = Sad(pCurrCuSearchRangeCenter, withSR, pCurrCtu, offset, nCuSize, nCtuSize); //当前位置的代价
	Mv tmv;
	for (tmv.y = mvmin.y; tmv.y <= mvmax.y; tmv.y++)
	{
		for (tmv.x = mvmin.x; tmv.x <= mvmax.x; tmv.x++)
		{
			int cost = Sad(pCurrCuSearchRangeCenter + tmv.x + tmv.y*withSR, withSR, pCurrCtu, offset, nCuSize, nCtuSize);
			if (cost < bcost)
			{
				bcost = cost;
				bmv.x = tmv.x;
				bmv.y = tmv.y;
			}			
		}
	}
	pImeOutput->ImeMvX[offsIdx] = bmv.x;
	pImeOutput->ImeMvY[offsIdx] = bmv.y;
	pImeOutput->nSadCost[offsIdx] = bcost;
}
int Sad(int *pCurrCuSearchRange, int merangex, int *pCurrCtu, Mv offset, int nCuSize, int nCtuSize)
{
	int bSadCost = 0;
	int *pCurrCu = pCurrCtu + (offset.x/4) + (offset.y/4)*(nCtuSize/nSampDist);
	for (int i = 0; i < nCuSize/nSampDist; i++)
	{
		for (int j = 0; j < nCuSize/nSampDist; j ++)
		{
			bSadCost += abs(pCurrCu[j + i*nCtuSize/nSampDist] - pCurrCuSearchRange[j*nSampDist + i*nSampDist*merangex]);
		}
	}
	return bSadCost;
}
void ResetImeParam(InterInfo::ImeOutput *pImeOutput)
{
	for (int i = 0; i < 85; i ++)
	{
		pImeOutput->ImeMvX[i] = MAX_SHORT;
		pImeOutput->ImeMvY[i] = MAX_SHORT;
		pImeOutput->nSadCost[i] = MAX_INT_MINE;
		//pImeOutput->pFmeSearchRange[i] = 0;
	}
}
void ResetFmeParam(InterInfo::FmeOutput *pFmeOutput)
{
	for (int i = 0; i < 85; i ++)
	{
		pFmeOutput->FmeMvX[i] = MAX_SHORT;
		pFmeOutput->FmeMvY[i] = MAX_SHORT;
		pFmeOutput->nSatdFme[i] = MAX_INT_MINE;
		pFmeOutput->nSatdMerge[i] = MAX_INT_MINE;
		pFmeOutput->nMergeIdx[i] = MAX_INT_MINE;
	}
}
void InterpOnlyHoriz(unsigned char *src, long long srcStride, unsigned char *dst, long long dstStride, int coeffIdx, int width, int height, int N)
{
	short const * coeff = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int headRoom = FILTER_PREC;
	int offset = (1 << (headRoom - 1));
	unsigned char maxVal = (1 << PIXEL_DEPTH) - 1;
	int cStride = 1;

	src -= (N / 2 - 1) * cStride;

	int row, col;
	for (row = 0; row < height; row++)
	{
		for (col = 0; col < width; col++)
		{
			int sum;

			sum = src[col + 0 * cStride] * coeff[0];
			sum += src[col + 1 * cStride] * coeff[1];
			sum += src[col + 2 * cStride] * coeff[2];
			sum += src[col + 3 * cStride] * coeff[3];
			if (N == 8)
			{
				sum += src[col + 4 * cStride] * coeff[4];
				sum += src[col + 5 * cStride] * coeff[5];
				sum += src[col + 6 * cStride] * coeff[6];
				sum += src[col + 7 * cStride] * coeff[7];
			}
			short val = (short)((sum + offset) >> headRoom);

			if (val < 0) val = 0;
			if (val > maxVal) val = maxVal;
			dst[col] = (unsigned char)val;
		}

		src += srcStride;
		dst += dstStride;
	}
}
void InterpOnlyVert(unsigned char *src, long long srcStride, unsigned char *dst, long long dstStride, int coeffIdx, int width, int height, int N)
{
	short const * c = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int shift = FILTER_PREC;
	int offset = 1 << (shift - 1);
	unsigned short maxVal = (1 << PIXEL_DEPTH) - 1;

	src -= (N / 2 - 1) * srcStride;

	int row, col;
	for (row = 0; row < height; row++)
	{
		for (col = 0; col < width; col++)
		{
			int sum;

			sum = src[col + 0 * srcStride] * c[0];
			sum += src[col + 1 * srcStride] * c[1];
			sum += src[col + 2 * srcStride] * c[2];
			sum += src[col + 3 * srcStride] * c[3];
			if (N == 8)
			{
				sum += src[col + 4 * srcStride] * c[4];
				sum += src[col + 5 * srcStride] * c[5];
				sum += src[col + 6 * srcStride] * c[6];
				sum += src[col + 7 * srcStride] * c[7];
			}

			short val = (short)((sum + offset) >> shift);
			val = (val < 0) ? 0 : val;
			val = (val > maxVal) ? maxVal : val;

			dst[col] = (unsigned char)val;
		}

		src += srcStride;
		dst += dstStride;
	}
}
void InterpHoriz(unsigned char *src, long long srcStride, short *dst, long long dstStride, int coeffIdx, int isRowExt, int width, int height, int N)
{
	short const * coeff = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int headRoom = INTERNAL_PREC - PIXEL_DEPTH;
	int shift = FILTER_PREC - headRoom;
	int offset = -INTERNAL_OFFS << shift;
	int blkheight = height;

	src -= N / 2 - 1;

	if (isRowExt)
	{
		src -= (N / 2 - 1) * srcStride;
		blkheight += N - 1;
	}

	int row, col;
	for (row = 0; row < blkheight; row++)
	{
		for (col = 0; col < width; col++)
		{
			int sum;

			sum = src[col + 0] * coeff[0];
			sum += src[col + 1] * coeff[1];
			sum += src[col + 2] * coeff[2];
			sum += src[col + 3] * coeff[3];
			if (N == 8)
			{
				sum += src[col + 4] * coeff[4];
				sum += src[col + 5] * coeff[5];
				sum += src[col + 6] * coeff[6];
				sum += src[col + 7] * coeff[7];
			}

			short val = (short)((sum + offset) >> shift);
			dst[col] = val;
		}

		src += srcStride;
		dst += dstStride;
	}
}
void InterpVert(short *src, long long srcStride, unsigned char *dst, long long dstStride, int coeffIdx, int width, int height, int N)
{
	int headRoom = INTERNAL_PREC - PIXEL_DEPTH;
	int shift = FILTER_PREC + headRoom;
	int offset = (1 << (shift - 1)) + (INTERNAL_OFFS << FILTER_PREC);
	unsigned short maxVal = (1 << PIXEL_DEPTH) - 1;
	const short *coeff = (N == 8 ? LumaFilter[coeffIdx] : ChromaFilter[coeffIdx]);

	src -= (N / 2 - 1) * srcStride;

	int row, col;
	for (row = 0; row < height; row++)
	{
		for (col = 0; col < width; col++)
		{
			int sum;

			sum = src[col + 0 * srcStride] * coeff[0];
			sum += src[col + 1 * srcStride] * coeff[1];
			sum += src[col + 2 * srcStride] * coeff[2];
			sum += src[col + 3 * srcStride] * coeff[3];
			if (N == 8)
			{
				sum += src[col + 4 * srcStride] * coeff[4];
				sum += src[col + 5 * srcStride] * coeff[5];
				sum += src[col + 6 * srcStride] * coeff[6];
				sum += src[col + 7 * srcStride] * coeff[7];
			}

			int val = (int)((sum + offset) >> shift);

			val = (val < 0) ? 0 : val;
			val = (val > maxVal) ? maxVal : val;

			dst[col] = (unsigned char)val;
		}

		src += srcStride;
		dst += dstStride;
	}
}
int SATD_8x4(unsigned char *pix1, long long stride_pix1, unsigned char *pix2, long stride_pix2)
{
	unsigned int tmp[4][4];
	unsigned int a0, a1, a2, a3;
	unsigned int sum = 0;

	for (int i = 0; i < 4; i++, pix1 += stride_pix1, pix2 += stride_pix2)
	{
		a0 = (pix1[0] - pix2[0]) + ((unsigned int)(pix1[4] - pix2[4]) << BITS_PER_SUM);
		a1 = (pix1[1] - pix2[1]) + ((unsigned int)(pix1[5] - pix2[5]) << BITS_PER_SUM);
		a2 = (pix1[2] - pix2[2]) + ((unsigned int)(pix1[6] - pix2[6]) << BITS_PER_SUM);
		a3 = (pix1[3] - pix2[3]) + ((unsigned int)(pix1[7] - pix2[7]) << BITS_PER_SUM);
		HADAMARDTRANS4(tmp[i][0], tmp[i][1], tmp[i][2], tmp[i][3], a0, a1, a2, a3);
	}

	for (int i = 0; i < 4; i++)
	{
		HADAMARDTRANS4(a0, a1, a2, a3, tmp[0][i], tmp[1][i], tmp[2][i], tmp[3][i]);
		sum += abs2(a0) + abs2(a1) + abs2(a2) + abs2(a3);
	}

	return (((unsigned short)sum) + (sum >> BITS_PER_SUM)) >> 1;
}
unsigned int abs2(unsigned int a)
{
	unsigned int s = ((a >> (BITS_PER_SUM - 1)) & (((unsigned int)1 << BITS_PER_SUM) + 1)) * ((unsigned short)-1);
	return (a + s) ^ s;
}
int SATD8(unsigned char *pix1, long long stride_pix1, unsigned char *pix2, long long stride_pix2, int w, int h)
{
	int satd = 0;

	for (int row = 0; row < h; row += 4)
	{
		for (int col = 0; col < w; col += 8)
		{
			satd += SATD_8x4(pix1 + row * stride_pix1 + col, stride_pix1,
				pix2 + row * stride_pix2 + col, stride_pix2);
		}
	}

	return satd;
}
int FractMotionEstimate(unsigned char *pRef, int RefStride, unsigned char *pFenc, int FencStride, const Mv qmv, int blockwidth, int blockheight)
{
	int xFrac = qmv.x & 0x3;
	int yFrac = qmv.y & 0x3;
	unsigned char *fref = pRef;
	if ((yFrac | xFrac) == 0)
	{
		//unsigned char *fref = pRef + (qmv.x >> 2) + (qmv.y >> 2) * RefStride;
		return SATD8(pFenc, FencStride, fref, RefStride, blockwidth, blockheight);
	}
	else
	{
		unsigned char subpelbuf[64 * 64] = { 0 };
		//unsigned char *fref = pRef + (qmv.x >> 2) + (qmv.y >> 2) * RefStride;
		if (yFrac == 0)
		{
			InterpOnlyHoriz(fref, RefStride, subpelbuf, nCtuSize, xFrac, blockwidth, blockheight, 8);
		}
		else if (xFrac == 0)
		{
			InterpOnlyVert(fref, RefStride, subpelbuf, nCtuSize, yFrac, blockwidth, blockheight, 8);
		}
		else
		{
			short immed[64 * (64 + 8)] = { 0 };
			int filterSize = LUMA_NTAPS;
			int halfFilterSize = filterSize >> 1;
			InterpHoriz(fref, RefStride, immed, blockwidth, xFrac, 1, blockwidth, blockheight, 8);
			InterpVert(immed + (halfFilterSize - 1) * blockwidth, blockwidth, subpelbuf, nCtuSize, yFrac, blockwidth, blockheight, 8);
		}
		return SATD8(pFenc, nCtuSize, subpelbuf, nCtuSize, blockwidth, blockheight);
	}
}
void FmeProc(InterInfo::FmeInput *pFmeInput, char nCtuSize, char nSplitDepth, unsigned char *pCurrCtu, InterInfo::FmeOutput *pFmeOutput)
{
	int nCuInCtu;
	switch (nCtuSize)
	{
	case 64: nCuInCtu = 85; break;
	case 32: nCuInCtu = 21; break;
	case 16: nCuInCtu = 5;   break;
	default: nCuInCtu = 85;
	}
	Mv offset;
	for (int i = 0; i < nCuInCtu; i++)
	{
		if ((!pFmeInput->isValidCu[i]) || (!pFmeInput->imeoutput.nSadCost[i]) || (!isSplit(nCtuSize, nSplitDepth, i)))
			continue;
		int nBuffSize;
		SelectBuffSize(nCtuSize, i, nBuffSize);
		unsigned char *pBuff = new unsigned char[nBuffSize*nBuffSize];
		memset(pBuff, 0, nBuffSize*nBuffSize*sizeof(unsigned char));
		FmePrefetch(pFmeInput, pBuff, nBuffSize, nBuffSize, i, nBuffSize - 8, nCtuSize);
		Mv qmv; qmv.x = pFmeInput->imeoutput.ImeMvX[i] * 4; qmv.y = pFmeInput->imeoutput.ImeMvY[i] * 4;
		switch (nCtuSize)
		{
		case 64: offset.x = OffsFromCtu64[i][0]; offset.y = OffsFromCtu64[i][1]; break;
		case 32: offset.x = OffsFromCtu32[i][0]; offset.y = OffsFromCtu32[i][1]; break;
		case 16: offset.x = OffsFromCtu16[i][0]; offset.y = OffsFromCtu16[i][1]; break;
		}
		int BestCost = MAX_INT_MINE; Mv BestMv;
		for (short x = -3; x <= 3; x++)
		{
			for (short y = -3; y <= 3; y++)
			{
				Mv tmpMv; tmpMv.x = qmv.x + x; tmpMv.y = qmv.y + y;
				Mv offsMv; offsMv.x = (tmpMv.x >> 2) - (qmv.x >> 2) + 4; offsMv.y = (tmpMv.y >> 2) - (qmv.y >> 2) + 4;
				int satdcost = FractMotionEstimate(pBuff + offsMv.x + offsMv.y * nBuffSize, nBuffSize, pCurrCtu + offset.x + offset.y*nCtuSize, nCtuSize, tmpMv, nBuffSize - 8, nBuffSize - 8);
				if (BestCost > satdcost)
				{
					BestMv.x = tmpMv.x;
					BestMv.y = tmpMv.y;
					BestCost = satdcost;
				}
			}
		}
		pFmeOutput->FmeMvX[i] = BestMv.x; pFmeOutput->FmeMvY[i] = BestMv.y; pFmeOutput->nSatdFme[i] = BestCost;
		delete[] pBuff;
	}
}
void FmePrefetch(InterInfo::FmeInput *pFmeInput, unsigned char *pFmeInputBuff, int BuffWidth, int BuffHeight, int offsIdx, int nCuSize, int nCtuSize)
{
	Mv mv; mv.x = pFmeInput->imeoutput.ImeMvX[offsIdx]; mv.y = pFmeInput->imeoutput.ImeMvY[offsIdx];
	int Width = pFmeInput->FmeSearchRangeWidth;
	int Height = pFmeInput->FmeSearchRangeHeight;
	Mv Center;  Mv offset;
	switch (nCtuSize)
	{
	case 64: offset.x = OffsFromCtu64[offsIdx][0]; offset.y = OffsFromCtu64[offsIdx][1]; break;
	case 32: offset.x = OffsFromCtu32[offsIdx][0]; offset.y = OffsFromCtu32[offsIdx][1]; break;
	case 16: offset.x = OffsFromCtu16[offsIdx][0]; offset.y = OffsFromCtu16[offsIdx][1]; break;
	default:offset.x = OffsFromCtu64[offsIdx][0]; offset.y = OffsFromCtu64[offsIdx][1]; break;
	}
	Center.x = (Width - nCtuSize)/2 + offset.x; Center.y = (Height - nCtuSize)/2 + offset.y;

	unsigned char *pTemp = pFmeInput->pFmeSearchRange + (Center.x + mv.x - 4) + (Center.y + mv.y - 4)*Width; //找到当前CU的位置,取出需要的数据进行分数像素插值
	for (int i = 0; i < nCuSize+8; i ++)
	{
		for (int j = 0; j < nCuSize+8; j ++)
		{
			pFmeInputBuff[j + i*(nCuSize + 8)] = pTemp[j + i*Width];
		}
	}
}
void SelectBuffSize(int nCtuSize, int i, int &nBuffSize)
{
	if (64 == nCtuSize)
	{
		if (i < 64)
			nBuffSize = 16;
		else if (i < 80)
			nBuffSize = 24;
		else if (i < 84)
			nBuffSize = 40;
		else
			nBuffSize = 72;
	}
	else if (32 == nCtuSize)
	{
		if (i < 16)
			nBuffSize = 16;
		else if (i < 20)
			nBuffSize = 24;
		else
			nBuffSize = 40;
	}
	else
	{
		if (i < 4)
			nBuffSize = 16;
		else
			nBuffSize = 24;
	}
}
unsigned int getOffsetIdx(int nCtuSize, int nCuPelX, int nCuPelY, unsigned int width)
{
	unsigned int offsIdx;
	if (64 == nCtuSize)
	{
		if (width == 8)
			offsIdx = (nCuPelX % 64) / 8 + (nCuPelY % 64) / 8 * 8;
		else if (width == 16)
			offsIdx = 64 + (nCuPelX % 64) / 16 + (nCuPelY % 64) / 16 * 4;
		else if (width == 32)
			offsIdx = 80 + (nCuPelX % 64) / 32 + (nCuPelY % 64) / 32 * 2;
		else
			offsIdx = 84;
	}
	else if (32 == nCtuSize)
	{
		if (width == 8)
			offsIdx = (nCuPelX % 32) / 8 + (nCuPelY % 32) / 8 * 4;
		else if (width == 16)
			offsIdx = 16 + (nCuPelX % 32) / 16 + (nCuPelY % 32) / 16 * 2;
		else if (width == 32)
			offsIdx = 20;
	}
	else
	{
		if (width == 8)
			offsIdx = (nCuPelX % 16) / 8 + (nCuPelY % 16) / 8 * 2;
		else if (width == 16)
			offsIdx = 4;
	}
	return offsIdx;
}
void MergeProc(InterInfo::FmeInput *pFmeInput, char nCtuSize, char nSplitDepth, unsigned char *pCurrCtu, InterInfo::FmeOutput *pFmeOutput)
{
	int nCuInCtu;
	switch (nCtuSize)
	{
	case 64: nCuInCtu = 85; break;
	case 32: nCuInCtu = 21; break;
	case 16: nCuInCtu = 5;   break;
	default: nCuInCtu = 85;
	}
	Mv offset;
	for (int i = 0; i < nCuInCtu; i++)
	{
		if ((!pFmeInput->isValidCu[i]) || (!isSplit(nCtuSize, nSplitDepth, i)))
			continue;
		int nBuffSize;
		SelectBuffSize(nCtuSize, i, nBuffSize);
		unsigned char *pBuff = new unsigned char[nBuffSize*nBuffSize];
		memset(pBuff, 0, nBuffSize*nBuffSize*sizeof(unsigned char));
		Mv qmv[3];
		for (int j = 0; j < 3; j ++)
		{
			qmv[j].x = interinfo.fmeinput.MergeMvX[i][0][j];
			qmv[j].y = interinfo.fmeinput.MergeMvY[i][0][j];
		}
		switch (nCtuSize)
		{
		case 64: offset.x = OffsFromCtu64[i][0]; offset.y = OffsFromCtu64[i][1]; break;
		case 32: offset.x = OffsFromCtu32[i][0]; offset.y = OffsFromCtu32[i][1]; break;
		case 16: offset.x = OffsFromCtu16[i][0]; offset.y = OffsFromCtu16[i][1]; break;
		}
		int BestCost = MAX_INT_MINE;
		unsigned int mergeIdx = 0;
		for (int j = 0; j < 3; j++)
		{
			Mv tmpMv; tmpMv.x = qmv[j].x; tmpMv.y = qmv[j].y;
			Mv mergeMv; mergeMv.x = tmpMv.x >> 2; mergeMv.y = tmpMv.y >> 2;
			MergePrefetch(pFmeInput, pBuff, nBuffSize, nBuffSize, i, nBuffSize - 8, nCtuSize, mergeMv);
			int satdcost = FractMotionEstimate(pBuff + 4 + 4 * nBuffSize, nBuffSize, pCurrCtu + offset.x + offset.y*nCtuSize, nCtuSize, tmpMv, nBuffSize - 8, nBuffSize - 8);
			if (BestCost > satdcost)
			{
				mergeIdx = j;
				BestCost = satdcost;
			}
		}
		pFmeOutput->nSatdMerge[i] = BestCost; pFmeOutput->nMergeIdx[i] = mergeIdx;
		delete[] pBuff;
	}
}
void MergePrefetch(InterInfo::FmeInput *pFmeInput, unsigned char *pFmeInputBuff, int BuffWidth, int BuffHeight, int offsIdx, int nCuSize, int nCtuSize, Mv mv)
{
	int Width = pFmeInput->FmeSearchRangeWidth;
	int Height = pFmeInput->FmeSearchRangeHeight;
	Mv Center;  Mv offset;
	switch (nCtuSize)
	{
	case 64: offset.x = OffsFromCtu64[offsIdx][0]; offset.y = OffsFromCtu64[offsIdx][1]; break;
	case 32: offset.x = OffsFromCtu32[offsIdx][0]; offset.y = OffsFromCtu32[offsIdx][1]; break;
	case 16: offset.x = OffsFromCtu16[offsIdx][0]; offset.y = OffsFromCtu16[offsIdx][1]; break;
	default:offset.x = OffsFromCtu64[offsIdx][0]; offset.y = OffsFromCtu64[offsIdx][1]; break;
	}
	Mv mvmin; mvmin.x = nCtuSize / 2 - Width / 2 + 4; mvmin.y = nCtuSize / 2 - Height / 2 + 4;
	Center.x = 4 - mvmin.x + offset.x; Center.y = 4 - mvmin.y + offset.y;
	unsigned char *pTemp = pFmeInput->pFmeSearchRange + (Center.x + mv.x - 4) + (Center.y + mv.y - 4)*Width; //找到当前CU的位置,取出需要的数据进行分数像素插值
	for (int i = 0; i < nCuSize + 8; i++)
	{
		for (int j = 0; j < nCuSize + 8; j++)
		{
			pFmeInputBuff[j + i*(nCuSize + 8)] = pTemp[j + i*Width];
		}
	}
}
