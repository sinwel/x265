#include "inter.h"
#include "math.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"
#if defined(__GNUC__) || defined(__clang__)    // use inline assembly, Gnu/AT&T syntax
#include <stdlib.h>
#endif

Mv g_leftPMV;
Mv g_leftPmv;
Mv g_RimeMv[85];
Mv g_FmeMv[85];
Mv g_mvAmvp[85][2];
Mv g_mvMerge[85][3];
Mv g_Mvmin[nNeightMv + 1];
Mv g_Mvmax[nNeightMv + 1];
unsigned char g_FmeResiY[4][64 * 64]; //add this for verify FME procedure
unsigned char g_FmeResiU[4][32 * 32];
unsigned char g_FmeResiV[4][32 * 32];

unsigned char g_MergeResiY[4][64 * 64];
unsigned char g_MergeResiU[4][32 * 32];
unsigned char g_MergeResiV[4][32 * 32];

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

double Lambda[QP_MAX] =
{
	0.05231, 0.060686, 0.07646, 0.096333333, 0.151715667, 0.15292, 0.192666667, 0.242745, 0.382299,
	0.385333333, 0.485489333, 0.611678333, 0.963333333, 0.970979, 1.223356667, 1.541333333, 2.427447667,
	2.446714, 3.082666667, 3.883916667, 6.116785, 6.165333333, 7.767833333, 9.786856667, 15.41333333,
	16.57137733, 22.183542, 29.5936, 39.357022, 52.19656867, 69.05173333, 91.14257667, 150.0651347,
	157.8325333, 207.1422197, 271.4221573, 443.904, 447.4271947, 563.722941, 710.2464, 1118.567987,
	1127.445883, 1420.4928, 1789.70878, 2818.614706, 2840.9856, 3579.41756, 4509.78353, 7102.464,
	7158.83512, 9019.56706, 11363.9424
};

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

//class CIME
unsigned short CoarseIntMotionEstimation::m_pMvCost[QP_MAX][MV_MAX * 2];

class CoarseIntMotionEstimation Cime;

CoarseIntMotionEstimation::CoarseIntMotionEstimation()
{

}

CoarseIntMotionEstimation::~CoarseIntMotionEstimation()
{

}

void CoarseIntMotionEstimation::Create(int nCimeSwWidth, int nCimeSwHeight, int nCtuSize, unsigned int nDownSampDist)
{
	m_nCtuSize = nCtuSize;
	m_nDownSampDist = nDownSampDist;
	m_nCimeSwWidth = nCimeSwWidth;
	m_nCimeSwHeight = nCimeSwHeight;
	m_pCimeSW = new short[nCimeSwWidth*nCimeSwHeight];
	assert(RK_NULL != m_pCimeSW);
	m_pCurrCtu = new unsigned char[nCtuSize*nCtuSize];
	assert(RK_NULL != m_pCurrCtu);
	m_pCurrCtuDS = new short[nCtuSize / m_nDownSampDist * nCtuSize / m_nDownSampDist];
	assert(RK_NULL != m_pCurrCtuDS);
	m_pRefPicDS = new short[(m_nPicWidth/m_nDownSampDist)*(m_nPicHeight/m_nDownSampDist)];
	assert(RK_NULL != m_pRefPicDS);
	m_pOrigPic = new unsigned char[m_nPicWidth*m_nPicHeight];
	assert(RK_NULL != m_pOrigPic);
}

void CoarseIntMotionEstimation::Destroy()
{
	if (RK_NULL != m_pCimeSW)
	{
		delete[] m_pCimeSW;
		m_pCimeSW = RK_NULL;
	}

	if (RK_NULL != m_pCurrCtu)
	{
		delete[] m_pCurrCtu;
		m_pCurrCtu = RK_NULL;
	}

	if (RK_NULL != m_pCurrCtuDS)
	{
		delete[] m_pCurrCtuDS;
		m_pCurrCtuDS = RK_NULL;
	}

	if (RK_NULL != m_pCurrCtuDS)
	{
		delete[] m_pCurrCtuDS;
		m_pCurrCtuDS = RK_NULL;
	}

	if (RK_NULL != m_pOrigPic)
	{
		delete[] m_pOrigPic;
		m_pOrigPic = RK_NULL;
	}
}

void CoarseIntMotionEstimation::Prefetch(unsigned char *pCurrPic, short *pRefPic) //pRefPic is a down sampled picture
{
	bool PicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	bool PicHeightNotDivCtu = m_nPicHeight / m_nCtuSize*m_nCtuSize < m_nPicHeight;
	int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (PicWidthNotDivCtu ? 1 : 0);
	int numCtuInPicHeight = m_nPicHeight / m_nCtuSize + (PicHeightNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % numCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / numCtuInPicWidth;
	int nCtuOffsFromCurrPic = (m_nCtuSize*nCtuPosHeight)*m_nPicWidth + (m_nCtuSize*nCtuPosWidth);
	int nCtuValidWidth = m_nCtuSize;
	int nCtuValidHeight = m_nCtuSize;

	//fill current CTU
	bool isCtuInRightEdge = (nCtuPosWidth == numCtuInPicWidth - 1);
	bool isCtuInBottomEdge = (nCtuPosHeight == numCtuInPicHeight - 1);
	if (isCtuInRightEdge && PicWidthNotDivCtu)
	{
		nCtuValidWidth = m_nPicWidth - (m_nPicWidth / m_nCtuSize*m_nCtuSize);
	}
	if (isCtuInBottomEdge && PicHeightNotDivCtu)
	{
		nCtuValidHeight = m_nPicHeight - (m_nPicHeight / m_nCtuSize*m_nCtuSize);
	}

	for (int i = 0; i < nCtuValidHeight; i ++)
	{
		for (int j = 0; j < nCtuValidWidth; j ++)
		{
			m_pCurrCtu[j + i*m_nCtuSize] = pCurrPic[nCtuOffsFromCurrPic + j + i*m_nPicWidth];
		}
	}

	if (nCtuValidWidth < m_nCtuSize)
	{
		for (int i = 0; i < m_nCtuSize; i ++)
		{
			for (int j = nCtuValidWidth; j < m_nCtuSize; j ++)
			{
				m_pCurrCtu[j + i*m_nCtuSize] = m_pCurrCtu[nCtuValidWidth-1+i*m_nCtuSize];
			}
		}
	}

	if (nCtuValidHeight < m_nCtuSize)
	{
		for (int i = nCtuValidHeight; i < m_nCtuSize; i ++)
		{
			for (int j = 0; j < m_nCtuSize; j ++)
			{
				m_pCurrCtu[j + i*m_nCtuSize] = m_pCurrCtu[j + (nCtuValidHeight - 1)*m_nCtuSize];
			}
		}
	}

	//fill current down sampled CTU
	for (int i = 0; i < m_nCtuSize/m_nDownSampDist; i++)
	{
		for (int j = 0; j < m_nCtuSize / m_nDownSampDist; j++)
		{
			short sum = 0;
			for (int m = 0; m < m_nDownSampDist; m ++)
			{
				for (int n = 0; n < m_nDownSampDist; n ++)
				{
					sum += m_pCurrCtu[(j*m_nDownSampDist + n) + (i*m_nDownSampDist + m)*m_nCtuSize];
				}
			}
			m_pCurrCtuDS[j + i*m_nCtuSize / m_nDownSampDist] = sum;
		}
	}

	//fill reference picture  8 edge position and 1 inside the reference picture
	unsigned int nCimeSwWidth = m_nCimeSwWidth - m_nCtuSize / m_nDownSampDist;
	unsigned int nCimeSwHeight = m_nCimeSwHeight - m_nCtuSize / m_nDownSampDist;
	unsigned int nCtuSize = m_nCtuSize / m_nDownSampDist;
	unsigned int nPicWidth = m_nPicWidth / m_nDownSampDist;
	unsigned int nPicHeight = m_nPicHeight / m_nDownSampDist;
	bool isOutTopEdge = (nCtuSize * nCtuPosHeight) < (nCimeSwHeight / 2);
	bool isOutLeftEdge = (nCtuSize * nCtuPosWidth) < (nCimeSwWidth / 2);
	bool isOutBottomEdge = (nCtuSize * nCtuPosHeight + (nCimeSwHeight / 2 - 1) + nCtuSize) > nPicHeight;
	bool isOutRightEdge = (nCtuSize * nCtuPosWidth + (nCimeSwWidth / 2 - 1) + nCtuSize) > nPicWidth;
	//1. down sampled search window is out of the top left corner
	if (isOutTopEdge && isOutLeftEdge)
	{
		int offset_x = (nCimeSwWidth / 2 - nCtuSize * nCtuPosWidth);
		int offset_y = (nCimeSwHeight / 2 - nCtuSize * nCtuPosHeight);
		short *pCurrCSW = m_pCimeSW + offset_x + offset_y*m_nCimeSwWidth;
		short *pCurrRefPic = pRefPic;
		int width = m_nCimeSwWidth - offset_x;
		int height = m_nCimeSwHeight - offset_y;
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrRefPic[j + i*nPicWidth];
			}
		}
		for (int i = 1; i <= offset_y; i ++)
		{
			for (int j = 0; j < width; j++)
			{
				int idx = j - i*m_nCimeSwWidth;
				pCurrCSW[idx] = pCurrCSW[j];
				//pCurrCSW[(j - i*m_nCimeSwWidth)] = pCurrCSW[j];
			}
		}
		pCurrCSW -= offset_y*m_nCimeSwWidth;
		for (int i = 0; i < m_nCimeSwHeight; i ++)
		{
			for (int j = 1; j <= offset_x; j ++)
			{
				int idx = -j + i*m_nCimeSwWidth;
				pCurrCSW[idx] = pCurrCSW[i*m_nCimeSwWidth];
				//pCurrCSW[-j + i*m_nCimeSwWidth] = pCurrCSW[i*m_nCimeSwWidth];
			}
		}
	}

	//2. down sampled search window is out of the top right corner
	if (isOutTopEdge && isOutRightEdge)
	{
		int offset_x = (nCimeSwWidth/2 + nCtuSize) - (nPicWidth - nCtuPosWidth*nCtuSize);
		int offset_y = (nCimeSwHeight / 2 - nCtuSize*nCtuPosHeight);
		short *pCurrCSW = m_pCimeSW + offset_y*m_nCimeSwWidth;
		short *pCurrRefPic = pRefPic + (nCtuPosWidth*nCtuSize) + (nCtuPosHeight*nCtuSize)*nPicWidth;
		pCurrRefPic -= (nCimeSwWidth / 2 + (nCtuSize*nCtuPosHeight)*nPicWidth);
		int width = m_nCimeSwWidth - offset_x;
		int height = m_nCimeSwHeight - offset_y;
		for (int i = 0; i < height; i ++)
		{
			for (int j = 0; j < width; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrRefPic[j + i*nPicWidth];
			}
		}
		pCurrCSW += (width-1);
		for (int i = 0; i < height; i ++)
		{
			for (int j = 1; j <= offset_x; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrCSW[i*m_nCimeSwWidth];
			}
		}
		pCurrCSW += offset_x;
		for (int i = 1; i <= offset_y; i ++)
		{
			for (int j = 0; j < m_nCimeSwWidth; j ++)
			{
				int idx = -j - i*m_nCimeSwWidth;
				pCurrCSW[idx] = pCurrCSW[-j];
				//pCurrCSW[-j - i*m_nCimeSwWidth] = pCurrCSW[-j];
			}
		}
	}

	//3. down sampled search window is out of the bottom left corner
	if (isOutLeftEdge && isOutBottomEdge)
	{
		int offset_x = (nCimeSwWidth / 2 - nCtuSize * nCtuPosWidth);
		int offset_y = (nCimeSwHeight / 2 + nCtuSize) - (nPicHeight - nCtuSize*nCtuPosHeight);
		short *pCurrCSW = m_pCimeSW + offset_x;
		short *pCurrRefPic = pRefPic + (nPicHeight - (m_nCimeSwHeight - offset_y))*nPicWidth;
		int width = m_nCimeSwWidth - offset_x;
		int height = m_nCimeSwHeight - offset_y;
		for (int i = 0; i < height; i ++)
		{
			for (int j = 0; j < width; j++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrRefPic[j + i*nPicWidth];
			}
		}
		pCurrCSW += (height - 1)*m_nCimeSwWidth;
		for (int i = 1; i <= offset_y; i ++)
		{
			for (int j = 0; j < width; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrCSW[j];
			}
		}
		pCurrCSW += offset_y*m_nCimeSwWidth;
		for (int i = 0; i < m_nCimeSwHeight; i ++)
		{
			for (int j = 1; j <= offset_x; j ++)
			{
				int idx1 = -j - i*m_nCimeSwWidth;
				int idx2 = -i*m_nCimeSwWidth;
				pCurrCSW[idx1] = pCurrCSW[idx2];
				//pCurrCSW[-j - i*m_nCimeSwWidth] = pCurrCSW[-i*m_nCimeSwWidth];
			}
		}
	}

	//4. down sampled search window is out of the bottom right corner
	if (isOutBottomEdge && isOutRightEdge)
	{
		int offset_x = (nCimeSwWidth / 2 + nCtuSize) - (nPicWidth - nCtuSize*nCtuPosWidth);
		int offset_y = (nCimeSwHeight / 2 + nCtuSize) - (nPicHeight - nCtuSize*nCtuPosHeight);
		int width = m_nCimeSwWidth - offset_x;
		int height = m_nCimeSwHeight - offset_y;
		short *pCurrRefPic = pRefPic + (nPicWidth*nPicHeight - 1);
		pCurrRefPic -= (width - 1) + (height - 1)*nPicWidth;
		short *pCurrCSW = m_pCimeSW;
		for (int i = 0; i < height; i ++)
		{
			for (int j = 0; j < width; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrRefPic[j + i*nPicWidth];
			}
		}
		pCurrCSW += (width - 1);
		for (int i = 0; i < height; i ++)
		{
			for (int j = 1; j <= offset_x; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrCSW[i*m_nCimeSwWidth];
			}
		}
		pCurrCSW += (height - 1)*m_nCimeSwWidth + offset_x;
		for (int i = 1; i <= offset_y; i ++)
		{
			for (int j = 0; j < m_nCimeSwWidth; j ++)
			{
				int idx = -j + i*m_nCimeSwWidth;
				pCurrCSW[idx] = pCurrCSW[-j];
				//pCurrCSW[-j + i*m_nCimeSwWidth] = pCurrCSW[-j];
			}
		}
	}

	//5. down sampled search window is out of the picture top edge
	if ((!isOutLeftEdge) && (!isOutRightEdge) && isOutTopEdge)
	{
		int offset_x = nCtuPosWidth*nCtuSize - nCimeSwWidth / 2;
		int offset_y = nCimeSwHeight / 2 - nCtuSize*nCtuPosHeight;
		short *pCurrRefPic = pRefPic + offset_x;
		short *pCurrCSW = m_pCimeSW + offset_y*m_nCimeSwWidth;
		int width = m_nCimeSwWidth;
		int height = m_nCimeSwHeight - offset_y;
		for (int i = 0; i < height; i ++)
		{
			for (int j = 0; j < width; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrRefPic[j + i*nPicWidth];
			}
		}
		for (int i = 1; i <= offset_y; i ++)
		{
			for (int j = 0; j < width; j ++)
			{
				int idx = j - i*m_nCimeSwWidth;
				pCurrCSW[idx] = pCurrRefPic[j];
				//pCurrCSW[j - i*m_nCimeSwWidth] = pCurrRefPic[j];
			}
		}
	}

	//6. down sampled search window is out of the picture left edge
	if ((!isOutBottomEdge) && (!isOutTopEdge) && isOutLeftEdge)
	{
		int offset_x = (nCimeSwWidth / 2 - nCtuSize * nCtuPosWidth);
		int offset_y = nCtuPosHeight*nCtuSize - nCimeSwHeight / 2;
		short *pCurrRefPic = pRefPic + offset_y*nPicWidth;
		short *pCurrCSW = m_pCimeSW + offset_x;
		int width = m_nCimeSwWidth - offset_x;
		int height = m_nCimeSwHeight;
		for (int i = 0; i < height; i ++)
		{
			for (int j = 0; j < width; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrRefPic[j + i*nPicWidth];
			}
		}
		for (int i = 0; i < height; i ++)
		{
			for (int j = 1; j <= offset_x; j ++)
			{
				int idx = -j + i*m_nCimeSwWidth;
				pCurrCSW[idx] = pCurrCSW[i*m_nCimeSwWidth];
				//pCurrCSW[-j + i*m_nCimeSwWidth] = pCurrCSW[i*m_nCimeSwWidth];
			}
		}
	}

	//7. down sampled search window is out of the picture bottom edge
	if ((!isOutLeftEdge) && (!isOutRightEdge) && isOutBottomEdge)
	{
		int offset_x = nCtuPosWidth*nCtuSize - nCimeSwWidth / 2;
		int offset_y = (nCimeSwHeight / 2 + nCtuSize) - (nPicHeight - nCtuSize*nCtuPosHeight);
		short *pCurrRefPic = pRefPic + offset_x + (nPicHeight - 1)*nPicWidth;
		pCurrRefPic -= (m_nCimeSwHeight - offset_y - 1)*nPicWidth;
		short *pCurrCSW = m_pCimeSW;
		int width = m_nCimeSwWidth;
		int height = m_nCimeSwHeight - offset_y;
		for (int i = 0; i < height; i ++)
		{
			for (int j = 0; j < width; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrRefPic[j + i*nPicWidth];
			}
		}
		pCurrCSW += (m_nCimeSwHeight - offset_y - 1)*m_nCimeSwWidth;
		for (int i = 1; i <= offset_y; i ++)
		{
			for (int j = 0; j < width; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrCSW[j];
			}
		}
	}

	//8. down sampled search window is out of the picture right edge
	if ((!isOutBottomEdge) && (!isOutTopEdge) && isOutRightEdge)
	{
		int offset_x = (nCimeSwWidth / 2 + nCtuSize) - (nPicWidth - nCtuSize*nCtuPosWidth);
		int offset_y = nCtuPosHeight*nCtuSize - nCimeSwHeight / 2;
		short *pCurrRefPic = pRefPic + (nPicWidth - 1) + offset_y*nPicWidth;
		pCurrRefPic -= (m_nCimeSwWidth - offset_x - 1);
		short *pCurrCSW = m_pCimeSW;
		int width = m_nCimeSwWidth - offset_x;
		int height = m_nCimeSwHeight;
		for (int i = 0; i < height; i ++)
		{
			for (int j = 0; j < width; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrRefPic[j + i*nPicWidth];
			}
		}
		pCurrCSW += (m_nCimeSwWidth - offset_x - 1);
		for (int i = 0; i < height; i ++)
		{
			for (int j = 1; j <= offset_x; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrCSW[i*m_nCimeSwWidth];
			}
		}
	}

	//9. down sampled search window is in the picture
	if (!(isOutTopEdge || isOutLeftEdge || isOutBottomEdge || isOutRightEdge))
	{
		short *pCurrRefPic = pRefPic + nCtuPosWidth*nCtuSize + (nCtuPosHeight*nCtuSize)*nPicWidth;
		pCurrRefPic -= (nCimeSwWidth / 2 + (nCimeSwHeight / 2)*nPicWidth);
		short *pCurrCSW = m_pCimeSW;
		for (int i = 0; i < m_nCimeSwHeight; i ++)
		{
			for (int j = 0; j < m_nCimeSwWidth; j ++)
			{
				pCurrCSW[j + i*m_nCimeSwWidth] = pCurrRefPic[j + i*nPicWidth];
			}
		}
	}
}

void CoarseIntMotionEstimation::setDownSamplePic(unsigned char *pRefPic, int stride)
{
	for (int i = 0; i < m_nPicHeight; i += m_nDownSampDist)
	{
		for (int j = 0; j < m_nPicWidth; j += m_nDownSampDist)
		{
			short sum = 0;
			for (int m = 0; m < m_nDownSampDist; m ++)
			{
				for (int n = 0; n < m_nDownSampDist; n ++)
				{
					sum += pRefPic[(j + n) + (i + m)*stride];
				}
			}
			m_pRefPicDS[j / m_nDownSampDist + i / m_nDownSampDist*m_nPicWidth / m_nDownSampDist] = sum;
		}
	}
}

void CoarseIntMotionEstimation::setOrigPic(unsigned char *pCurrOrigPic, int stride)
{
	for (int i = 0; i < m_nPicHeight; i ++)
	{
		for (int j = 0; j < m_nPicWidth; j ++)
		{
			m_pOrigPic[j + i*m_nPicWidth] = pCurrOrigPic[j + i*stride];
		}
	}
}

void CoarseIntMotionEstimation::CIME(unsigned char *pCurrPic, short *pRefPic)
{//CIME procedure for a CTU
	Prefetch(pCurrPic, pRefPic);
	Mv mvmin, mvmax, tmv, bmv;  bmv.x = 0; bmv.y = 0;
	mvmin.x = static_cast<short>(-(m_nCimeSwWidth - m_nCtuSize / m_nDownSampDist) / 2);
	mvmin.y = static_cast<short>(-(m_nCimeSwHeight - m_nCtuSize / m_nDownSampDist) / 2);
	mvmax.x = -1 - mvmin.x; mvmax.y = -1 - mvmin.y;
	int nOffsFromLT = -mvmin.x - mvmin.y * m_nCimeSwWidth;
	short *pCSW = m_pCimeSW + nOffsFromLT; //CIME search window center
	int nValidCtuWidth = m_nCtuSize;
	int nValidCtuHeight = m_nCtuSize;
	bool isPicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	bool isPicHeightNotDivCtu = m_nPicHeight / m_nCtuSize*m_nCtuSize < m_nPicHeight;
	int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (isPicWidthNotDivCtu ? 1 : 0);
	int numCtuInPicHeight = m_nPicHeight / m_nCtuSize + (isPicHeightNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % numCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / numCtuInPicWidth;

	if (isPicHeightNotDivCtu && nCtuPosHeight == numCtuInPicHeight - 1)
	{
		nValidCtuHeight = m_nPicHeight - m_nPicHeight / m_nCtuSize*m_nCtuSize;
	}
	if (isPicWidthNotDivCtu && nCtuPosWidth == numCtuInPicWidth - 1)
	{
		nValidCtuWidth = m_nPicWidth - m_nPicWidth / m_nCtuSize*m_nCtuSize;
	}

	static bool isFirst = true;
	if (isFirst)
	{
		setMvpCost();
		isFirst = false;
	}
		
	int bcost = INT_MAX;
	Mv mvp[3];
	setMvp(mvp);
	for (tmv.y = mvmin.y; tmv.y <= mvmax.y; tmv.y++)
	{
		for (tmv.x = mvmin.x; tmv.x <= mvmax.x; tmv.x++)
		{
			int tmpCost = INT_MAX;
			for (int idx = 0; idx < 3; idx++)
			{
				unsigned short mvCost = mvcost(mvp[idx], tmv);
				if (tmpCost > mvCost)
				tmpCost = mvCost;
			}
			int nOffsFromCenter = tmv.x + tmv.y*m_nCimeSwWidth;
			int cost = SAD(pCSW + nOffsFromCenter, m_pCurrCtuDS, nValidCtuWidth / 4, nValidCtuHeight / 4, m_nCimeSwWidth);
			cost += tmpCost;
			COPY_IF_LT(bcost, cost, bmv.x, 4*tmv.x, bmv.y, 4*tmv.y);
		}
	}
	m_mvCimeOut.x = bmv.x; m_mvCimeOut.y = bmv.y; m_mvCimeOut.nSadCost = bcost;
}

int CoarseIntMotionEstimation::SAD(short *src_1, short *src_2, int width, int height, int stride)
{//src_1: down sample CIME search window  src_2 down sample CTU
	int sum = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			sum += abs(src_1[j + i*stride] - src_2[j + i*width]);
		}
	}
	return sum;
}

unsigned short CoarseIntMotionEstimation::mvcost(Mv mvp, Mv mv)
{
	unsigned short *pMvCost = m_pMvCost[m_nQP] + MV_MAX;
	return  (pMvCost[mv.x - mvp.x] + pMvCost[mv.y - mvp.y]);
}

void CoarseIntMotionEstimation::setMvp(Mv *mvp)
{
	const int nMvNeigh = 36;
	bool isValid[nMvNeigh];
	memset(isValid, 1, nMvNeigh);
	bool PicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	//bool PicHeightNotDivCtu = m_nPicHeight / m_nCtuSize*m_nCtuSize < m_nPicHeight;
	int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (PicWidthNotDivCtu ? 1 : 0);
	//int numCtuInPicHeight = m_nPicHeight / m_nCtuSize + (PicHeightNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % numCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / numCtuInPicWidth;
	bool isValidLeftTop = true, isValidTop = true, isValidRightTop = true, isValidLeft = true;
	if (0 == nCtuPosWidth)
	{
		isValidLeft = false; 
		isValidLeftTop = false;
	}
	if (0 == nCtuPosHeight)
	{
		isValidLeftTop = false;
		isValidTop = false;
		isValidRightTop = false;
	}
	if (numCtuInPicWidth-1 == nCtuPosWidth)
	{
		isValidRightTop = false;
	}
	//check 36 neighbor positions valid or not
	if (64 == m_nCtuSize)
	{
		if (!isValidLeftTop)
		{
			isValid[11] = false;
		}
		if (!isValidRightTop)
		{
			isValid[8] = false;
		}
		if (!isValidLeft)
		{
			isValid[26] = false;
			isValid[27] = false;
			isValid[29] = false;
			isValid[31] = false;
			isValid[33] = false;
		}
		if (!isValidTop)
		{
			isValid[34] = false; isValid[30] = false; isValid[32] = false;
			isValid[28] = false; isValid[24] = false; isValid[17] = false;
			isValid[23] = false; isValid[5] = false; isValid[25] = false;
			isValid[14] = false; isValid[20] = false; isValid[2] = false;
		}
	}
	else if (32 == m_nCtuSize)
	{
		if (!isValidLeftTop)
		{
			isValid[7] = false;
		}
		if (!isValidRightTop)
		{
			isValid[6] = false;
		}
		if (!isValidLeft)
		{
			isValid[11] = false;
			isValid[13] = false;
			isValid[14] = false;
		}
		if (!isValidTop)
		{
			isValid[2] = false; isValid[4] = false;   isValid[8] = false;
			isValid[9] = false; isValid[10] = false; isValid[12] = false;
		}
		for (int i = 16; i < 36; i++)
			isValid[i] = false;
	}

	int idxFirst = INT_MAX;
	for (int idy = 0; idy < nMvNeigh; idy++)
	{
		if (isValid[idy])
		{
			idxFirst = idy;
			break;
		}
	}

	for (int idy = idxFirst; idy < nMvNeigh - 1; idy++)
	{
		if (!isValid[idy])
			continue;
		for (int idx = idy + 1; idx < nMvNeigh; idx++)
		{
			if (abs(m_mvNeighMv[idx].x - m_mvNeighMv[idy].x) <= nRectSize && abs(m_mvNeighMv[idx].y - m_mvNeighMv[idy].y) <= nRectSize)
			{
				isValid[idx] = false;
			}
		}
	}

	int tmpCand = 0;
	for (int i = 0; i < nMvNeigh; i++)
	{
		if (isValid[i] && tmpCand < 3) //replace real mvp
		{
			mvp[tmpCand].x = m_mvNeighMv[i].x;
			mvp[tmpCand++].y = m_mvNeighMv[i].y;
		}
	}
	if (tmpCand < 3)
	{
		for (int i = tmpCand; i < 3; i++) 
		{
			mvp[i].x = mvp[0].x;
			mvp[i].y = mvp[0].y;
		}
	}
}

void CoarseIntMotionEstimation::setMvpCost()
{
	float log2_2 = 2.0f / log(2.0f);  // 2 x 1/log(2)
	for (int qp = 0; qp < QP_MAX; qp ++)
	{
		double lambda = sqrt(Lambda[qp]);
		unsigned short *pMvCost = m_pMvCost[qp] + MV_MAX;
		for (int i = 0; i < MV_MAX; i++)
		{
			float bitsizes = 0.718f;
			if (i>0)
				bitsizes = log((float)(i + 1)) * log2_2 + 1.718f;

			pMvCost[i] = pMvCost[-i] = (unsigned short)MIN_MINE(bitsizes * lambda + 0.5f, (1 << 16) - 1);
		}
	}
}


//class RIME
unsigned short RefineIntMotionEstimation::m_pMvCost[QP_MAX][MV_MAX * 2];

unsigned char RefineIntMotionEstimation::m_pCurrCtuPel[nCtuSize*nCtuSize*3/2];

unsigned char RefineIntMotionEstimation::m_pCurrCtuRefPic[3][(nCtuSize + 2 * (nRimeWidth + 4))*(nCtuSize + 2 * (nRimeHeight + 4))*3/2];

class RefineIntMotionEstimation Rime;

RefineIntMotionEstimation::RefineIntMotionEstimation()
{

}

RefineIntMotionEstimation::~RefineIntMotionEstimation()
{

}

void RefineIntMotionEstimation::setPmv(Mv pMvNeigh[36], Mv mvCpmv)
{
	//get coarse search PMV
	m_mvRefine[0].x = mvCpmv.x;
	m_mvRefine[0].y = mvCpmv.y;

	/*get 2 neighbor PMV  begin*/
	const int nMvNeigh = 36;
	bool isValid[nMvNeigh];
	memset(isValid, 1, nMvNeigh);
	bool PicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	//bool PicHeightNotDivCtu = m_nPicHeight / m_nCtuSize*m_nCtuSize < m_nPicHeight;
	int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (PicWidthNotDivCtu ? 1 : 0);
	//int numCtuInPicHeight = m_nPicHeight / m_nCtuSize + (PicHeightNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % numCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / numCtuInPicWidth;
	bool isValidLeftTop = true, isValidTop = true, isValidRightTop = true, isValidLeft = true;
	if (0 == nCtuPosWidth)
	{
		isValidLeft = false;
		isValidLeftTop = false;
	}
	if (0 == nCtuPosHeight)
	{
		isValidLeftTop = false;
		isValidTop = false;
		isValidRightTop = false;
	}
	if (numCtuInPicWidth - 1 == nCtuPosWidth)
	{
		isValidRightTop = false;
	}
	//check 36 neighbor positions valid or not

	if (64 == m_nCtuSize)
	{
		if (!isValidLeftTop)
		{
			isValid[11] = false;
		}
		if (!isValidRightTop)
		{
			isValid[8] = false;
		}
		if (!isValidLeft)
		{
			isValid[26] = false;
			isValid[27] = false;
			isValid[29] = false;
			isValid[31] = false;
			isValid[33] = false;
		}
		if (!isValidTop)
		{
			isValid[34] = false; isValid[30] = false; isValid[32] = false;
			isValid[28] = false; isValid[24] = false; isValid[17] = false;
			isValid[23] = false; isValid[5] = false; isValid[25] = false;
			isValid[14] = false; isValid[20] = false; isValid[2] = false;
		}
	}
	else if (32 == m_nCtuSize)
	{
		if (!isValidLeftTop)
		{
			isValid[7] = false;
		}
		if (!isValidRightTop)
		{
			isValid[6] = false;
		}
		if (!isValidLeft)
		{
			isValid[11] = false;
			isValid[13] = false;
			isValid[14] = false;
		}
		if (!isValidTop)
		{
			isValid[2] = false; isValid[4] = false;  isValid[8] = false;
			isValid[9] = false; isValid[10] = false; isValid[12] = false;
		}
		for (int i = 16; i < 36; i++)
			isValid[i] = false;
	}

	int idxFirst = INT_MAX;
	for (int idy = 0; idy < nMvNeigh; idy++)
	{
		if (isValid[idy])
		{
			idxFirst = idy;
			break;
		}
	}

	for (int idy = idxFirst; idy < nMvNeigh - 1; idy++)
	{
		if (!isValid[idy])
			continue;
		for (int idx = idy + 1; idx < nMvNeigh; idx++)
		{
			if (abs(pMvNeigh[idx].x - pMvNeigh[idy].x) <= nRectSize && abs(pMvNeigh[idx].y - pMvNeigh[idy].y) <= nRectSize)
			{
				isValid[idx] = false;
			}
		}
	}

	int tmpCand = 0;
	for (int i = 0; i < nMvNeigh; i++)
	{
		if (isValid[i] && tmpCand < nNeightMv) //replace real mvp
		{
			m_mvRefine[tmpCand + 1].x = pMvNeigh[i].x;
			m_mvRefine[tmpCand + 1].y = pMvNeigh[i].y;
			tmpCand++;
		}
	}
	if (tmpCand < nNeightMv)
	{
		for (int i = tmpCand+1; i < nNeightMv+1; i++) //if not full, the remaining set the same as first neighbor mv
		{
			m_mvRefine[i].x = m_mvRefine[1].x;
			m_mvRefine[i].y = m_mvRefine[1].y;
		}
	}
	/*get 2 neighbor PMV  end*/
	for (int i = 0; i < 3; i ++) //fetch even MV, for chroma search windows
	{
		m_mvRefine[i].x = m_mvRefine[i].x / 2 * 2;
		m_mvRefine[i].y = m_mvRefine[i].y / 2 * 2;
	}
	
}

void RefineIntMotionEstimation::setMvp(Mv *mvp, Mv pMvNeigh[36])
{
	const int nMvNeigh = 36;
	bool isValid[nMvNeigh];
	memset(isValid, 1, nMvNeigh);
	bool PicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	//bool PicHeightNotDivCtu = m_nPicHeight / m_nCtuSize*m_nCtuSize < m_nPicHeight;
	int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (PicWidthNotDivCtu ? 1 : 0);
	//int numCtuInPicHeight = m_nPicHeight / m_nCtuSize + (PicHeightNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % numCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / numCtuInPicWidth;
	bool isValidLeftTop = true, isValidTop = true, isValidRightTop = true, isValidLeft = true;
	if (0 == nCtuPosWidth)
	{
		isValidLeft = false;
		isValidLeftTop = false;
	}
	if (0 == nCtuPosHeight)
	{
		isValidLeftTop = false;
		isValidTop = false;
		isValidRightTop = false;
	}
	if (numCtuInPicWidth - 1 == nCtuPosWidth)
	{
		isValidRightTop = false;
	}
	//check 36 neighbor positions valid or not

	if (64 == m_nCtuSize)
	{
		if (!isValidLeftTop)
		{
			isValid[11] = false;
		}
		if (!isValidRightTop)
		{
			isValid[8] = false;
		}
		if (!isValidLeft)
		{
			isValid[26] = false;
			isValid[27] = false;
			isValid[29] = false;
			isValid[31] = false;
			isValid[33] = false;
		}
		if (!isValidTop)
		{
			isValid[34] = false; isValid[30] = false; isValid[32] = false;
			isValid[28] = false; isValid[24] = false; isValid[17] = false;
			isValid[23] = false; isValid[5] = false; isValid[25] = false;
			isValid[14] = false; isValid[20] = false; isValid[2] = false;
		}
	}
	else if (32 == m_nCtuSize)
	{
		if (!isValidLeftTop)
		{
			isValid[7] = false;
		}
		if (!isValidRightTop)
		{
			isValid[6] = false;
		}
		if (!isValidLeft)
		{
			isValid[11] = false;
			isValid[13] = false;
			isValid[14] = false;
		}
		if (!isValidTop)
		{
			isValid[2] = false; isValid[4] = false;   isValid[8] = false;
			isValid[9] = false; isValid[10] = false; isValid[12] = false;
		}
		for (int i = 16; i < 36; i++)
			isValid[i] = false;
	}

	int idxFirst = INT_MAX;
	for (int idy = 0; idy < nMvNeigh; idy++)
	{
		if (isValid[idy])
		{
			idxFirst = idy;
			break;
		}
	}

	for (int idy = idxFirst; idy < nMvNeigh - 1; idy++)
	{
		if (!isValid[idy])
			continue;
		for (int idx = idy + 1; idx < nMvNeigh; idx++)
		{
			if (abs(pMvNeigh[idx].x - pMvNeigh[idy].x) <= nRectSize && abs(pMvNeigh[idx].y - pMvNeigh[idy].y) <= nRectSize)
			{
				isValid[idx] = false;
			}
		}
	}

	int tmpCand = 0;
	for (int i = 0; i < nMvNeigh; i++)
	{
		if (isValid[i] && tmpCand < 3) //replace real mvp
		{
			mvp[tmpCand].x = pMvNeigh[i].x;
			mvp[tmpCand++].y = pMvNeigh[i].y;
		}
	}
	if (tmpCand < 3)
	{
		for (int i = tmpCand; i < 3; i++)
		{
			mvp[i].x = mvp[0].x;
			mvp[i].y = mvp[0].y;
		}
	}
}

void RefineIntMotionEstimation::setMvpCost()
{
	float log2_2 = 2.0f / log(2.0f);  // 2 x 1/log(2)
	for (int qp = 0; qp < QP_MAX; qp++)
	{
		double lambda = sqrt(Lambda[qp]);
		unsigned short *pMvCost = m_pMvCost[qp] + MV_MAX;
		for (int i = 0; i < MV_MAX; i++)
		{
			float bitsizes = 0.718f;
			if (i>0)
				bitsizes = log((float)(i + 1)) * log2_2 + 1.718f;

			pMvCost[i] = pMvCost[-i] = (unsigned short)MIN_MINE(bitsizes * lambda + 0.5f, (1 << 16) - 1);
		}
	}
}

unsigned short RefineIntMotionEstimation::mvcost(Mv mvp, Mv mv)
{
	unsigned short *pMvCost = m_pMvCost[m_nQP] + MV_MAX;
	return  (pMvCost[mv.x - mvp.x] + pMvCost[mv.y - mvp.y]);
}

void RefineIntMotionEstimation::CreatePicInfo()
{
	m_pOrigPic = new unsigned char[(m_nPicWidth * 3 / 2)*(m_nPicHeight * 3 / 2)];
	assert(RK_NULL != m_pOrigPic);
	m_pRefPic = new unsigned char[(m_nPicWidth*3/2)*(m_nPicHeight*3/2)];
	assert(RK_NULL != m_pRefPic);
}

void RefineIntMotionEstimation::DestroyPicInfo()
{
	if (RK_NULL != m_pOrigPic)
	{
		delete[] m_pOrigPic;
		m_pOrigPic = RK_NULL;
	}

	if (RK_NULL != m_pRefPic)
	{
		delete[] m_pRefPic;
		m_pRefPic = RK_NULL;
	}
}

void RefineIntMotionEstimation::CreateCuInfo(int nRimeSwWidth, int nRimeSwHeight, int nCuSize)
{
	m_nCuSize = nCuSize;
	m_nRimeSwWidth = nRimeSwWidth;
	m_nRimeSwHeight = nRimeSwHeight;
	m_pCuRimeSW[0] = new unsigned char[nRimeSwWidth*nRimeSwHeight];
	assert(RK_NULL != m_pCuRimeSW[0]);
	m_pCuRimeSW[1] = new unsigned char[nRimeSwWidth*nRimeSwHeight];
	assert(RK_NULL != m_pCuRimeSW[1]);
	m_pCuRimeSW[2] = new unsigned char[nRimeSwWidth*nRimeSwHeight];
	assert(RK_NULL != m_pCuRimeSW[2]);
	m_pCurrCuPel = new unsigned char[nCuSize*nCuSize*3/2];
	assert(RK_NULL != m_pCurrCuPel);
	m_pCuForFmeInterp = new unsigned char[(m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2)*3/2];
	assert(RK_NULL != m_pCuForFmeInterp);
}

void RefineIntMotionEstimation::DestroyCuInfo()
{
	for (int i = 0; i < 3; i ++)
	{
		if (RK_NULL != m_pCuRimeSW[i])
		{
			delete[] m_pCuRimeSW[i];
			m_pCuRimeSW[i] = RK_NULL;
		}
	}
	
	if (RK_NULL != m_pCurrCuPel)
	{
		delete[] m_pCurrCuPel;
		m_pCurrCuPel = RK_NULL;
	}
	
	if (RK_NULL != m_pCuForFmeInterp)
	{
		delete[] m_pCuForFmeInterp;
		m_pCuForFmeInterp = RK_NULL;
	}

}

void RefineIntMotionEstimation::setOrigAndRefPic(unsigned char *pOrigPicY, unsigned char *pRefPicY, int stride_Y, unsigned char *pOrigPicU,
	unsigned char *pRefPicU, int stride_U, unsigned char *pOrigPicV, unsigned char *pRefPicV, int stride_V)
{//store YUV pixels as Y,Y,Y,Y,...,UV,UV...
	for (int i = 0; i < m_nPicHeight; i++)
	{
		for (int j = 0; j < m_nPicWidth; j++)
		{
			m_pOrigPic[j + i*m_nPicWidth] = pOrigPicY[j + i*stride_Y];
			m_pRefPic[j + i*m_nPicWidth] = pRefPicY[j + i*stride_Y];
		}
	}
	unsigned char *pOrigPic = m_pOrigPic + m_nPicWidth*m_nPicHeight;
	unsigned char *pRefPic = m_pRefPic + m_nPicWidth*m_nPicHeight;
	for (int i = 0; i < m_nPicHeight / 2; i ++)
	{
		for (int j = 0; j < m_nPicWidth / 2; j ++)
		{
			pOrigPic[(j * 2 + 0) + i*m_nPicWidth] = pOrigPicU[j + i*stride_U];
			pOrigPic[(j * 2 + 1) + i*m_nPicWidth] = pOrigPicV[j + i*stride_V];
			pRefPic[(j * 2 + 0) + i*m_nPicWidth] = pRefPicU[j + i*stride_U];
			pRefPic[(j * 2 + 1) + i*m_nPicWidth] = pRefPicV[j + i*stride_V];
		}
	}
}

void RefineIntMotionEstimation::PrefetchCtuLumaInfo()
{
	bool PicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	bool PicHeightNotDivCtu = m_nPicHeight / m_nCtuSize*m_nCtuSize < m_nPicHeight;
	int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (PicWidthNotDivCtu ? 1 : 0);
	int numCtuInPicHeight = m_nPicHeight / m_nCtuSize + (PicHeightNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % numCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / numCtuInPicWidth;
	int offs_x = 0;
	int offs_y = 0;
	//if ((m_nCtuSize*nCtuPosWidth + m_nCtuSize) > m_nPicWidth || (m_nCtuSize*nCtuPosHeight + m_nCtuSize) > m_nPicHeight)
	//	return false;

	int nCtuOffsFromCurrPic = (m_nCtuSize*nCtuPosHeight)*m_nPicWidth + (m_nCtuSize*nCtuPosWidth);

	//fetch CTU pixel
	unsigned char *pCurrCtu = m_pOrigPic + nCtuOffsFromCurrPic;
	int nCtuValidWidth = m_nCtuSize;
	int nCtuValidHeight = m_nCtuSize;
	if (PicWidthNotDivCtu && (numCtuInPicWidth - 1) == nCtuPosWidth)
		nCtuValidWidth = m_nPicWidth - (m_nPicWidth / m_nCtuSize)*m_nCtuSize;
	if (PicHeightNotDivCtu && (numCtuInPicHeight - 1) == nCtuPosHeight)
		nCtuValidHeight = m_nPicHeight - (m_nPicHeight / m_nCtuSize)*m_nCtuSize;

	for (int i = 0; i < nCtuValidHeight; i++)
	{
		for (int j = 0; j < nCtuValidWidth; j++)
		{
			m_pCurrCtuPel[j + i*m_nCtuSize] = pCurrCtu[j + i*m_nPicWidth];
		}
	}
	if (nCtuValidWidth < m_nCtuSize)
	{
		for (int i = 0; i < m_nCtuSize; i++)
		{
			for (int j = nCtuValidWidth; j < m_nCtuSize; j++)
			{
				m_pCurrCtuPel[j + i*m_nCtuSize] = m_pCurrCtuPel[nCtuValidWidth - 1 + i*m_nCtuSize];
			}
		}
	}
	if (nCtuValidHeight < m_nCtuSize)
	{
		for (int i = nCtuValidHeight; i < m_nCtuSize; i++)
		{
			for (int j = 0; j < m_nCtuSize; j++)
			{
				m_pCurrCtuPel[j + i*m_nCtuSize] = m_pCurrCtuPel[j + (nCtuValidHeight - 1)*m_nCtuSize];
			}
		}
	}

	//fetch 3 PMV search window
	for (int pmvIdx = 0; pmvIdx < 3; pmvIdx++)
	{
		int nRSWidth = m_nCtuSize + 2 * (nRimeWidth + 4);
		int nRSHeight = m_nCtuSize + 2 * (nRimeHeight + 4);
		int pmv_x = m_nCtuSize*nCtuPosWidth + offs_x + m_mvRefine[pmvIdx].x - nRimeWidth - 4; //top left of refine search window
		int pmv_y = m_nCtuSize*nCtuPosHeight + offs_y + m_mvRefine[pmvIdx].y - nRimeHeight - 4;
		bool isOutTopEdge = pmv_y < 0;
		bool isOutLeftEdge = pmv_x < 0;
		bool isOutBottomEdge = pmv_y + nRSHeight > m_nPicHeight - 1;
		bool isOutRightEdge = pmv_x + nRSWidth > m_nPicWidth - 1;

		//1. reference pixels of current CU is out of top left edge
		if (isOutTopEdge && isOutLeftEdge)
		{
			int width = (nRSWidth + pmv_x);
			int height = (nRSHeight + pmv_y);
			if (width < 0) width = 0;
			if (height < 0) height = 0;
			int offset_x = nRSWidth - width;
			int offset_y = nRSHeight - height;
			if (width && height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset_x + offset_y*nRSWidth;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = m_pRefPic[j + i*m_nPicWidth];
					}
				}
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j - i*nRSWidth] = pRimeSW[j];
					}
				}
				pRimeSW -= offset_y*nRSWidth;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[-j + i*nRSWidth] = pRimeSW[i*nRSWidth];
					}
				}
			}
			else if (width)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < offset_x; j++)
					{
						pRimeSW[j + i*nRSWidth] = m_pRefPic[0];
					}
				}
				pRimeSW += offset_x;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = m_pRefPic[j];
					}
				}
			}
			else if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = m_pRefPic[0];
					}
				}
				pRimeSW += offset_y*nRSWidth;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = m_pRefPic[i*m_nPicWidth];
					}
				}
			}
			else
			{
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						m_pCurrCtuRefPic[pmvIdx][j + i*nRSWidth] = m_pRefPic[0];
					}
				}
			}
		}

		//2. reference pixels of current CU is out of top right edge
		if (isOutTopEdge && isOutRightEdge)
		{
			int width = (m_nPicWidth - pmv_x);
			int height = (nRSHeight + pmv_y);
			if (width < 0) width = 0;
			if (height < 0) height = 0;
			int offset_x = nRSWidth - width;
			int offset_y = nRSHeight - height;
			if (width&&height)
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicWidth - width);
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset_y*nRSWidth;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j + i*m_nPicWidth];
					}
				}
				pRimeSW += (width - 1);
				for (int i = 0; i < height; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRimeSW[i*nRSWidth];
					}
				}
				pRimeSW -= (width - 1);
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j - i*nRSWidth] = pRimeSW[j];
					}
				}
			}
			else if (width)
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicWidth - width);
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j];
					}
				}
				pRimeSW += (width - 1);
				pRefPic += (width - 1);
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < offset_x; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[0];
					}
				}
			}
			else if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				unsigned char *pRefPic = m_pRefPic + (m_nPicWidth - 1);
				for (int i = 0; i < offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[0];
					}
				}
				pRimeSW += offset_y*nRSWidth;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[i*m_nPicWidth];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				unsigned char *pRefPic = m_pRefPic + (m_nPicWidth - 1);
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[0];
					}
				}
			}
		}

		//3. reference pixels of current CU is out of bottom left edge
		if (isOutLeftEdge&&isOutBottomEdge)
		{
			int width = (nRSWidth + pmv_x);
			if (width < 0) width = 0;
			int height = (m_nPicHeight - pmv_y);
			if (height < 0) height = 0;
			int offset_x = nRSWidth - width;
			int offset_y = nRSHeight - height;

			if (width&&height)
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicHeight - height)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset_x;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j + i*m_nPicWidth];
					}
				}
				pRimeSW += (height - 1)*nRSWidth;
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRimeSW[j];
					}
				}
				pRimeSW -= (height - 1)*nRSWidth;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[-j + i*nRSWidth] = pRimeSW[i*nRSWidth];
					}
				}
			}
			else if (width)
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicHeight - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < offset_x; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[0];
					}
				}
				pRimeSW += offset_x;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j];
					}
				}
			}
			else if (height)
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicHeight - height)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[i*m_nPicWidth];
					}
				}
				pRimeSW += (height - 1)*nRSWidth;
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRimeSW[j];
					}
				}
			}
			else
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicHeight - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[0];
					}
				}
			}
		}

		//4. reference pixels of current CU is out of bottom right edge
		if (isOutBottomEdge&&isOutRightEdge)
		{
			int width = (m_nPicWidth - pmv_x);
			if (width < 0) width = 0;
			int height = (m_nPicHeight - pmv_y);
			if (height < 0) height = 0;
			int offset_x = nRSWidth - width;
			int offset_y = nRSHeight - height;

			if (width&&height)
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicHeight*m_nPicWidth - 1) - (width - 1) - (height - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j + i*m_nPicWidth];
					}
				}
				pRimeSW += (width - 1);
				for (int i = 0; i < height; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRimeSW[i*nRSWidth];
					}
				}
				pRimeSW = m_pCurrCtuRefPic[pmvIdx] + (height - 1)*nRSWidth;
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRimeSW[j];
					}
				}
			}
			else if (width)
			{
				unsigned char *pRefPic = m_pRefPic + m_nPicWidth*m_nPicHeight - 1; //bottom right
				pRefPic -= (width - 1);
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j];
					}
				}
				pRimeSW += (width - 1);
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRimeSW[i*nRSWidth];
					}
				}
			}
			else if (height)
			{
				unsigned char *pRefPic = m_pRefPic + m_nPicWidth*m_nPicHeight - 1; //bottom right
				pRefPic -= (height - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[i*m_nPicWidth];
					}
				}
				pRimeSW += (height - 1)*nRSWidth;
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRimeSW[j];
					}
				}
			}
			else
			{
				unsigned char *pRefPic = m_pRefPic + m_nPicWidth*m_nPicHeight - 1; //bottom right
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[0];
					}
				}
			}
		}

		//5. reference pixels of current CU is out of top edge
		if ((!isOutLeftEdge) && (!isOutRightEdge) && (isOutTopEdge))
		{
			int height = (nRSHeight + pmv_y);
			if (height < 0) height = 0;
			int offset_y = nRSHeight - height;
			unsigned char *pRefPic = m_pRefPic + pmv_x;
			if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j];
					}
				}
				pRimeSW += offset_y*nRSWidth;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j + i*m_nPicWidth];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j];
					}
				}
			}
		}

		//6. reference pixels of current CU is out of left edge
		if ((!isOutBottomEdge) && (!isOutTopEdge) && isOutLeftEdge)
		{
			int width = (nRSWidth + pmv_x);
			if (width < 0) width = 0;
			int offset_x = nRSWidth - width;
			unsigned char *pRefPic = m_pRefPic + pmv_y*m_nPicWidth;
			if (width)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < offset_x; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[i*m_nPicWidth];
					}
				}
				pRimeSW += offset_x;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j + i*m_nPicWidth];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[i*m_nPicWidth];
					}
				}
			}
		}

		//7. reference pixels of current CU is out of bottom edge
		if ((!isOutLeftEdge) && (!isOutRightEdge) && isOutBottomEdge)
		{
			int height = (m_nPicHeight - pmv_y);
			if (height < 0) height = 0;
			int offset_y = nRSHeight - height;
			unsigned char *pRefPic = m_pRefPic + (m_nPicHeight - 1)*m_nPicWidth + pmv_x;
			if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				pRefPic -= (height - 1)*m_nPicWidth;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j + i*m_nPicWidth];
					}
				}
				pRimeSW += (height - 1)*nRSWidth;
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRimeSW[j];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j];
					}
				}
			}
		}

		//8. reference pixels of current CU is out of right edge
		if ((!isOutTopEdge) && (!isOutBottomEdge) && isOutRightEdge)
		{
			int width = (m_nPicWidth - pmv_x);
			if (width < 0) width = 0;
			int offset_x = nRSWidth - width;
			unsigned char *pRefPic = m_pRefPic + (m_nPicWidth - 1) + pmv_y*m_nPicWidth;
			if (width)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + width - 1;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[i*m_nPicWidth];
					}
				}
				pRefPic -= (width - 1);
				pRimeSW -= (width - 1);
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[j + i*m_nPicWidth];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = pRefPic[i*m_nPicWidth];
					}
				}
			}
		}

		//9. reference pixels of current CU is inside the picture
		if ((!isOutTopEdge) && (!isOutLeftEdge) && (!isOutBottomEdge) && (!isOutRightEdge))
		{
			unsigned char *pRefPic = m_pRefPic + pmv_x + pmv_y*m_nPicWidth;
			for (int i = 0; i < nRSHeight; i++)
			{
				for (int j = 0; j < nRSWidth; j++)
				{
					m_pCurrCtuRefPic[pmvIdx][j + i*nRSWidth] = pRefPic[j + i*m_nPicWidth];
				}
			}
		}

		//FILE *fp = fopen("e:\\mine.txt", "ab");
		//for (int i = 0; i < nRSHeight; i++)
		//{
		//	for (int j = 0; j < nRSWidth; j++)
		//	{
		//		fprintf(fp, "%4d", m_pCurrCtuRefPic[pmvIdx][j + i*nRSWidth]);
		//	}
		//	fprintf(fp, "\n");
		//}
		//fclose(fp);

	}
}

void RefineIntMotionEstimation::PrefetchCtuChromaInfo()
{
	bool PicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	bool PicHeightNotDivCtu = m_nPicHeight / m_nCtuSize*m_nCtuSize < m_nPicHeight;
	int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (PicWidthNotDivCtu ? 1 : 0);
	int numCtuInPicHeight = m_nPicHeight / m_nCtuSize + (PicHeightNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % numCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / numCtuInPicWidth;
	int nCtuOffsFromCurrPic = m_nPicWidth*m_nPicHeight + (m_nCtuSize/2*nCtuPosHeight)*m_nPicWidth + (m_nCtuSize*nCtuPosWidth);

	//fetch CTU pixel
	int nCtuValidWidth = m_nCtuSize;
	int nCtuValidHeight = m_nCtuSize;
	if (PicWidthNotDivCtu && (numCtuInPicWidth - 1) == nCtuPosWidth)
		nCtuValidWidth = m_nPicWidth - (m_nPicWidth / m_nCtuSize)*m_nCtuSize;
	if (PicHeightNotDivCtu && (numCtuInPicHeight - 1) == nCtuPosHeight)
		nCtuValidHeight = m_nPicHeight - (m_nPicHeight / m_nCtuSize)*m_nCtuSize;

	//nCtuValidWidth /= 2; 
	nCtuValidHeight /= 2;
	unsigned char *pOrigPic = m_pOrigPic + nCtuOffsFromCurrPic;
	unsigned char *pCurrCtuPel = m_pCurrCtuPel + m_nCtuSize*m_nCtuSize;
	for (int i = 0; i < nCtuValidHeight; i++)
	{
		for (int j = 0; j < nCtuValidWidth; j++)
		{
			pCurrCtuPel[j + i*m_nCtuSize] = pOrigPic[j + i*m_nPicWidth];
		}
	}

	int offset = (nCtuSize + 2 * (nRimeWidth + 4))*(nCtuSize + 2 * (nRimeHeight + 4));
	//fetch 3 PMV search window
	for (int pmvIdx = 0; pmvIdx < 3; pmvIdx++)
	{//m_mvRefine is always even
		int nRSWidth = m_nCtuSize + 2 * (nRimeWidth + 4);
		int nRSHeight = m_nCtuSize + 2 * (nRimeHeight + 4);
		int pmv_x = m_nCtuSize*nCtuPosWidth + m_mvRefine[pmvIdx].x - nRimeWidth - 4; //top left of refine search window
		int pmv_y = m_nCtuSize*nCtuPosHeight + m_mvRefine[pmvIdx].y - nRimeHeight - 4;
		bool isOutTopEdge = pmv_y < 0;
		bool isOutLeftEdge = pmv_x < 0;
		bool isOutBottomEdge = pmv_y + nRSHeight > m_nPicHeight - 1;
		bool isOutRightEdge = pmv_x + nRSWidth > m_nPicWidth - 1;
		nRSWidth /= 2; nRSHeight /= 2;
		pmv_x /= 2; pmv_y /= 2;

		//1. reference pixels of current CU is out of top left edge
		if (isOutTopEdge && isOutLeftEdge)
		{
			int width = (nRSWidth + pmv_x);
			int height = (nRSHeight + pmv_y);
			if (width < 0) width = 0;
			if (height < 0) height = 0;
			int offset_x = nRSWidth - width;
			int offset_y = nRSHeight - height;
			if (width && height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset_x*2 + offset_y*nRSWidth*2 + offset;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j*2 + 0 + i*nRSWidth*2] = m_pRefPic[j*2 + 0 + i*m_nPicWidth + m_nPicWidth*m_nPicHeight];
						pRimeSW[j*2 + 1 + i*nRSWidth*2] = m_pRefPic[j*2 + 1 + i*m_nPicWidth + m_nPicWidth*m_nPicHeight];
					}
				}
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j*2 + 0 - i*nRSWidth*2] = pRimeSW[j*2 + 0];
						pRimeSW[j*2 + 1 - i*nRSWidth*2] = pRimeSW[j*2 + 1];
					}
				}
				pRimeSW -= offset_y*nRSWidth*2;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[-j * 2 + 0 + i*nRSWidth * 2] = pRimeSW[0 + i*nRSWidth * 2];
						pRimeSW[-j * 2 + 1 + i*nRSWidth * 2] = pRimeSW[1 + i*nRSWidth * 2];
					}
				}
			}
			else if (width)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < offset_x; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = m_pRefPic[0 + m_nPicWidth*m_nPicHeight];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = m_pRefPic[1 + m_nPicWidth*m_nPicHeight];
					}
				}
				pRimeSW += offset_x * 2;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = m_pRefPic[j * 2 + 0 + m_nPicWidth*m_nPicHeight];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = m_pRefPic[j * 2 + 1 + m_nPicWidth*m_nPicHeight];
					}
				}
			}
			else if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = m_pRefPic[0 + m_nPicWidth*m_nPicHeight];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = m_pRefPic[1 + m_nPicWidth*m_nPicHeight];
					}
				}
				pRimeSW += offset_y*nRSWidth * 2;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = m_pRefPic[0 + i*m_nPicWidth + m_nPicWidth*m_nPicHeight];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = m_pRefPic[1 + i*m_nPicWidth + m_nPicWidth*m_nPicHeight];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = m_pRefPic[0 + m_nPicWidth*m_nPicHeight];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = m_pRefPic[1 + m_nPicWidth*m_nPicHeight];
					}
				}
			}
		}

		//2. reference pixels of current CU is out of top right edge
		if (isOutTopEdge && isOutRightEdge)
		{
			int width = (m_nPicWidth/2 - pmv_x);
			int height = (nRSHeight + pmv_y);
			if (width < 0) width = 0;
			if (height < 0) height = 0;
			int offset_x = nRSWidth - width;
			int offset_y = nRSHeight - height;
			if (width&&height)
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicWidth - width * 2) + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset_y*nRSWidth * 2 + offset;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1 + i*m_nPicWidth];
					}
				}
				pRimeSW += (width - 1) * 2;
				for (int i = 0; i < height; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRimeSW[0 + i*nRSWidth * 2];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRimeSW[1 + i*nRSWidth * 2];
					}
				}
				pRimeSW -= (width - 1) * 2;
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 - i*nRSWidth * 2] = pRimeSW[j * 2 + 0];
						pRimeSW[j * 2 + 1 - i*nRSWidth * 2] = pRimeSW[j * 2 + 1];
					}
				}
			}
			else if (width)
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicWidth - width * 2) + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1];
					}
				}
				pRimeSW += (width - 1) * 2;
				pRefPic += (width - 1) * 2;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < offset_x; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1];
					}
				}
			}
			else if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				unsigned char *pRefPic = m_pRefPic + (m_nPicWidth - 2) + m_nPicWidth*m_nPicHeight;
				for (int i = 0; i < offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1];
					}
				}
				pRimeSW += offset_y*nRSWidth*2;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1 + i*m_nPicWidth];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				unsigned char *pRefPic = m_pRefPic + (m_nPicWidth - 2) + m_nPicWidth*m_nPicHeight;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1];
					}
				}
			}
		}

		//3. reference pixels of current CU is out of bottom left edge
		if (isOutLeftEdge&&isOutBottomEdge)
		{
			int width = (nRSWidth + pmv_x);
			if (width < 0) width = 0;
			int height = (m_nPicHeight/2 - pmv_y);
			if (height < 0) height = 0;
			int offset_x = nRSWidth - width;
			int offset_y = nRSHeight - height;

			if (width&&height)
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicHeight / 2 - height)*m_nPicWidth + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset_x * 2 + offset;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1 + i*m_nPicWidth];
					}
				}
				pRimeSW += (height - 1)*nRSWidth*2;
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRimeSW[j * 2 + 0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRimeSW[j * 2 + 1];
					}
				}
				pRimeSW -= (height - 1)*nRSWidth*2;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[-j * 2 + 0 + i*nRSWidth * 2] = pRimeSW[0 + i*nRSWidth * 2];
						pRimeSW[-j * 2 + 1 + i*nRSWidth * 2] = pRimeSW[1 + i*nRSWidth * 2];
					}
				}
			}
			else if (width)
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicHeight - 1)*m_nPicWidth + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < offset_x; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1];
					}
				}
				pRimeSW += offset_x*2;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1];
					}
				}
			}
			else if (height)
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicHeight - height)*m_nPicWidth + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1 + i*m_nPicWidth];
					}
				}
				pRimeSW += (height - 1)*nRSWidth * 2;
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRimeSW[j * 2 + 0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRimeSW[j * 2 + 1];
					}
				}
			}
			else
			{
				unsigned char *pRefPic = m_pRefPic + (m_nPicHeight - 1)*m_nPicWidth + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1];
					}
				}
			}
		}

		//4. reference pixels of current CU is out of bottom right edge
		if (isOutBottomEdge&&isOutRightEdge)
		{
			int width = (m_nPicWidth/2 - pmv_x);
			if (width < 0) width = 0;
			int height = (m_nPicHeight/2 - pmv_y);
			if (height < 0) height = 0;
			int offset_x = nRSWidth - width;
			int offset_y = nRSHeight - height;

			if (width&&height)
			{
				unsigned char *pRefPic = m_pRefPic + m_nPicHeight*m_nPicWidth;
				pRefPic += (m_nPicHeight / 2 * m_nPicWidth - 2) - (width - 1) * 2 - (height - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1 + i*m_nPicWidth];
					}
				}
				pRimeSW += (width - 1)*2;
				for (int i = 0; i < height; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRimeSW[0 + i*nRSWidth * 2];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRimeSW[1 + i*nRSWidth * 2];
					}
				}
				pRimeSW = m_pCurrCtuRefPic[pmvIdx] + (height - 1)*nRSWidth * 2 + offset;
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRimeSW[j * 2 + 0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRimeSW[j * 2 + 1];
					}
				}
			}
			else if (width)
			{
				unsigned char *pRefPic = m_pRefPic + m_nPicHeight*m_nPicWidth + m_nPicWidth*m_nPicHeight/2 - 2; //bottom right
				pRefPic -= (width - 1) * 2;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1];
					}
				}
				pRimeSW += (width - 1) * 2;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRimeSW[0 + i*nRSWidth * 2];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRimeSW[1 + i*nRSWidth * 2];
					}
				}
			}
			else if (height)
			{
				unsigned char *pRefPic = m_pRefPic + m_nPicHeight*m_nPicWidth + m_nPicWidth*m_nPicHeight / 2 - 2; //bottom right
				pRefPic -= (height - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1 + i*m_nPicWidth];
					}
				}
				pRimeSW += (height - 1)*nRSWidth*2;
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRimeSW[j * 2 + 0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRimeSW[j * 2 + 1];
					}
				}
			}
			else
			{
				unsigned char *pRefPic = m_pRefPic + m_nPicHeight*m_nPicWidth + m_nPicWidth*m_nPicHeight / 2 - 2; //bottom right
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1];
					}
				}
			}
		}

		//5. reference pixels of current CU is out of top edge
		if ((!isOutLeftEdge) && (!isOutRightEdge) && (isOutTopEdge))
		{
			int height = (nRSHeight + pmv_y);
			if (height < 0) height = 0;
			int offset_y = nRSHeight - height;
			unsigned char *pRefPic = m_pRefPic + m_nPicHeight*m_nPicWidth + pmv_x * 2;
			if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1];
					}
				}
				pRimeSW += offset_y*nRSWidth*2;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1 + i*m_nPicWidth];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1];
					}
				}
			}
		}

		//6. reference pixels of current CU is out of left edge
		if ((!isOutBottomEdge) && (!isOutTopEdge) && isOutLeftEdge)
		{
			int width = (nRSWidth + pmv_x);
			if (width < 0) width = 0;
			int offset_x = nRSWidth - width;
			unsigned char *pRefPic = m_pRefPic + pmv_y*m_nPicWidth + m_nPicHeight*m_nPicWidth;
			if (width)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < offset_x; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1 + i*m_nPicWidth];
					}
				}
				pRimeSW += offset_x * 2;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1 + i*m_nPicWidth];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1 + i*m_nPicWidth];
					}
				}
			}
		}

		//7. reference pixels of current CU is out of bottom edge
		if ((!isOutLeftEdge) && (!isOutRightEdge) && isOutBottomEdge)
		{
			int height = (m_nPicHeight/2 - pmv_y);
			if (height < 0) height = 0;
			int offset_y = nRSHeight - height;
			unsigned char *pRefPic = m_pRefPic + (m_nPicHeight / 2 - 1)*m_nPicWidth + pmv_x * 2 + m_nPicHeight*m_nPicWidth;
			if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				pRefPic -= (height - 1)*m_nPicWidth;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1 + i*m_nPicWidth];
					}
				}
				pRimeSW += (height - 1)*nRSWidth * 2;
				for (int i = 1; i <= offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRimeSW[j * 2 + 0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRimeSW[j * 2 + 1];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1];
					}
				}
			}
		}

		//8. reference pixels of current CU is out of right edge
		if ((!isOutTopEdge) && (!isOutBottomEdge) && isOutRightEdge)
		{
			int width = (m_nPicWidth / 2 - pmv_x);
			if (width < 0) width = 0;
			int offset_x = nRSWidth - width;
			unsigned char *pRefPic = m_pRefPic + (m_nPicWidth - 2) + pmv_y*m_nPicWidth + m_nPicHeight*m_nPicWidth;
			if (width)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + (width - 1) * 2 + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 1; j <= offset_x; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1 + i*m_nPicWidth];
					}
				}
				pRefPic -= (width - 1) * 2;
				pRimeSW -= (width - 1) * 2;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1 + i*m_nPicWidth];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[0 + i*m_nPicWidth];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[1 + i*m_nPicWidth];
					}
				}
			}
		}

		//9. reference pixels of current CU is inside the picture
		if ((!isOutTopEdge) && (!isOutLeftEdge) && (!isOutBottomEdge) && (!isOutRightEdge))
		{
			unsigned char *pRefPic = m_pRefPic + pmv_x * 2 + pmv_y*m_nPicWidth + m_nPicHeight*m_nPicWidth;
			unsigned char *pRimeSW = m_pCurrCtuRefPic[pmvIdx] + offset;
			for (int i = 0; i < nRSHeight; i++)
			{
				for (int j = 0; j < nRSWidth; j++)
				{
					pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = pRefPic[j * 2 + 0 + i*m_nPicWidth];
					pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = pRefPic[j * 2 + 1 + i*m_nPicWidth];
				}
			}
		}
	}
}

bool RefineIntMotionEstimation::PrefetchCuInfo()
{
	bool PicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	//bool PicHeightNotDivCtu = m_nPicHeight / m_nCtuSize*m_nCtuSize < m_nPicHeight;
	int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (PicWidthNotDivCtu ? 1 : 0);
	//int numCtuInPicHeight = m_nPicHeight / m_nCtuSize + (PicHeightNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % numCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / numCtuInPicWidth;
	int offset_x = OffsFromCtu64[m_nCuPosInCtu][0];
	int offset_y = OffsFromCtu64[m_nCuPosInCtu][1];
	if (32 == m_nCtuSize)
	{
		offset_x = OffsFromCtu32[m_nCuPosInCtu][0];
		offset_y = OffsFromCtu32[m_nCuPosInCtu][1];
	}
	if ((m_nCtuSize*nCtuPosWidth + offset_x + m_nCuSize) > m_nPicWidth || (m_nCtuSize*nCtuPosHeight + offset_y + m_nCuSize) > m_nPicHeight)
		return false;
	unsigned char *pCurrCu = m_pCurrCtuPel + offset_x + offset_y*m_nCtuSize;
	//fetch current CU pixel
	for (int i = 0; i < m_nCuSize; i ++)
	{
		for (int j = 0; j < m_nCuSize; j ++)
		{
			m_pCurrCuPel[j + i*m_nCuSize] = pCurrCu[j + i*m_nCtuSize];
		}
	}
	unsigned char *pCurrCuPel = m_pCurrCuPel + m_nCuSize*m_nCuSize;
	pCurrCu = m_pCurrCtuPel + m_nCtuSize*m_nCtuSize + offset_x + offset_y/2*m_nCtuSize;
	for (int i = 0; i < m_nCuSize/2; i++)
	{
		for (int j = 0; j < m_nCuSize; j++)
		{
			pCurrCuPel[j + i*m_nCuSize] = pCurrCu[j + i*m_nCtuSize];
		}
	}

	//fetch reference picture pixel of current CU
	int nCtuRefPicWidth = m_nCtuSize + 2 * (nRimeWidth + 4);
	int nCuRefPicWidth = m_nCuSize + 2 * (nRimeWidth + 4);
	int nCuRefPicHeight = m_nCuSize + 2 * (nRimeHeight + 4);
	for (int pmvIdx = 0; pmvIdx < 3; pmvIdx ++)
	{
		unsigned char *pCurrCuRefPic = m_pCurrCtuRefPic[pmvIdx] + offset_x + offset_y*(nCtuRefPicWidth);
		for (int i = 0; i < nCuRefPicHeight; i++)
		{
			for (int j = 0; j < nCuRefPicWidth; j++)
			{
				m_pCuRimeSW[pmvIdx][j + i*nCuRefPicWidth] = pCurrCuRefPic[j + i*nCtuRefPicWidth];
			}
		}
	}
	return true;
}

int RefineIntMotionEstimation::SAD(unsigned char *pCuRimeSW, int stride, unsigned char *pCurrCu, int width, int height)
{
	int sum = 0;
	for (int i = 0; i < height; i ++)
	{
		for (int j = 0; j < width; j ++)
		{
			sum += abs(pCuRimeSW[j + i*stride] - pCurrCu[j + i*width]);
		}
	}
	return sum;
}

void RefineIntMotionEstimation::RimeAndFme(Mv mvNeigh[36])
{
	static bool isFirst = true;
	if (isFirst)
	{
		setMvpCost();
		isFirst = false;
	}
	int bcost = INT_MAX;
	Mv bmv; bmv.x = 0; bmv.y = 0;
	Mv mvp[3];
	setMvp(mvp, mvNeigh);
	int pmvBestIdx = 0;
	int pmvBestCost = INT_MAX;
	for (int idxPmv = 0; idxPmv < nNeightMv + 1; idxPmv++)
	{
		Mv tmv;
		unsigned char *pCuRimeSW = m_pCuRimeSW[idxPmv] + (4 + nRimeWidth) + (4 + nRimeHeight)*m_nRimeSwWidth;
		for (short i = -nRimeHeight; i <= nRimeHeight; i++)
		{
			for (short j = -nRimeWidth; j <= nRimeWidth; j++)
			{
				tmv.x =  j;  tmv.y =  i;
				unsigned short tmpCost = USHORT_MAX;
				for (int idx = 0; idx < 3; idx++)
				{
					Mv Tmv; Tmv.x = (tmv.x + m_mvRefine[idxPmv].x); Tmv.y = (tmv.y + m_mvRefine[idxPmv].y);
					if (tmpCost > mvcost(mvp[idx], Tmv))
						tmpCost = mvcost(mvp[idx], Tmv);
				}
				int idx = tmv.x + tmv.y*m_nRimeSwWidth;
				int cost = SAD(pCuRimeSW + idx, m_nRimeSwWidth, m_pCurrCuPel, m_nCuSize, m_nCuSize)+static_cast<int>(tmpCost);
				COPY_IF_LT(bcost, cost, bmv.x, (tmv.x + m_mvRefine[idxPmv].x), bmv.y, (tmv.y + m_mvRefine[idxPmv].y));
			}
		}
		if (bcost < pmvBestCost)
		{
			pmvBestCost = bcost;
			pmvBestIdx = idxPmv;
		}
	}
	m_mvRimeOut.x = bmv.x; m_mvRimeOut.y = bmv.y; m_mvRimeOut.nSadCost = bcost;

	//FME calculate procedure
	bcost = INT_MAX;
	Mv tmv; tmv.x = bmv.x - m_mvRefine[pmvBestIdx].x; tmv.y = bmv.y - m_mvRefine[pmvBestIdx].y;

	unsigned char *pCuRimeSW = m_pCuRimeSW[pmvBestIdx] + (4 + nRimeWidth) + (4 + nRimeHeight)*m_nRimeSwWidth;
	for (short i = -3; i <= 3; i++)
	{
		for (short j = -3; j <= 3; j++)
		{
			Mv qmv; qmv.x = tmv.x*4 + i; qmv.y = tmv.y*4 + j;
			unsigned char *fref = pCuRimeSW + (qmv.x >> 2) - 4 + ((qmv.y >> 2)-4)* m_nRimeSwWidth;
			int cost = subpelCompare(fref, m_nRimeSwWidth, m_pCurrCuPel, m_nCuSize, qmv);
			COPY_IF_LT(bcost, cost, bmv.x, qmv.x, bmv.y, qmv.y);
		}
	}
	m_mvFmeOut.x = bmv.x + m_mvRefine[pmvBestIdx].x * 4; m_mvFmeOut.y = bmv.y + m_mvRefine[pmvBestIdx].y * 4; m_mvFmeOut.nSadCost = bcost;
	
	int offset_x = OffsFromCtu64[m_nCuPosInCtu][0];
	int offset_y = OffsFromCtu64[m_nCuPosInCtu][1];
	if (32 == m_nCtuSize)
	{
		offset_x = OffsFromCtu32[m_nCuPosInCtu][0];
		offset_y = OffsFromCtu32[m_nCuPosInCtu][1];
	}
	int nCtuRefPicWidth = m_nCtuSize + 2 * (nRimeWidth + 4);
	unsigned char *fref = m_pCurrCtuRefPic[pmvBestIdx] + offset_x + offset_y*(nCtuRefPicWidth)+
		((bmv.x >> 2) + nRimeWidth) + (nRimeHeight + (bmv.y >> 2))* nCtuRefPicWidth;
	
	for (int i = 0; i < m_nCuSize + 4 * 2; i ++)
	{
		for (int j = 0; j < m_nCuSize + 4 * 2; j ++)
		{
			m_pCuForFmeInterp[j + i*(m_nCuSize + 8)] = fref[j + i*nCtuRefPicWidth];
		}
	}

	unsigned char *pCuForFmeInterp = m_pCuForFmeInterp + (m_nCuSize + 8)*(m_nCuSize + 8);
	fref = m_pCurrCtuRefPic[pmvBestIdx] + (nCtuSize + 2 * (nRimeWidth + 4))*(nCtuSize + 2 * (nRimeHeight + 4));
	fref += offset_x + offset_y/2*(nCtuRefPicWidth)+
		((bmv.x >> 3)*2 + nRimeWidth) + (nRimeHeight/2 + (bmv.y >> 3))* nCtuRefPicWidth;
	for (int i = 0; i < m_nCuSize/2 + 4; i++)
	{
		for (int j = 0; j < m_nCuSize/2 + 4; j++)
		{
			pCuForFmeInterp[j * 2 + 0 + i*(m_nCuSize + 8)] = fref[j * 2 + 0 + i*nCtuRefPicWidth];
			pCuForFmeInterp[j * 2 + 1 + i*(m_nCuSize + 8)] = fref[j * 2 + 1 + i*nCtuRefPicWidth];
		}
	}
}

int RefineIntMotionEstimation::subpelCompare(unsigned char *pfref, int stride_1, unsigned char *pfenc, int stride_2, const Mv& qmv)
{
	int xFrac = qmv.x & 0x3;
	int yFrac = qmv.y & 0x3;
	int N = 4;
	int multi = 1;
	if (4 == N)
		multi = 2;
	else
		multi = 1;

	if ((yFrac | xFrac) == 0)
	{
		return satd_8x8(pfenc, stride_2, pfref + 4 + 4 * stride_1, stride_1, m_nCuSize, m_nCuSize);
	}
	else
	{
		unsigned char subpelbuf[64 * 64];
		if (yFrac == 0)
		{
			InterpHorizLuma(pfref + 4 + 4 * stride_1, stride_1, subpelbuf, CTU_STRIDE, xFrac*multi, N, m_nCuSize, m_nCuSize);
		}
		else if (xFrac == 0)
		{
			InterpVertLuma(pfref + 4 + 4 * stride_1, stride_1, subpelbuf, CTU_STRIDE, yFrac*multi, N, m_nCuSize, m_nCuSize);
		}
		else
		{
			short immedVal[64 * (64 + 8)];
			int filterSize = N;
			int halfFilterSize = (filterSize >> 1);
			const short *pFilter;
			if (4 == N)
				pFilter = ChromaFilter[xFrac*multi];
			else
				pFilter = LumaFilter[xFrac];
			FilterHorizonLuma(pfref + 4 + 4 * stride_1 - (halfFilterSize - 1) * stride_1, stride_1, immedVal, m_nCuSize, m_nCuSize, m_nCuSize + filterSize - 1, pFilter, N);
			FilterVertLuma(immedVal + (halfFilterSize - 1) * m_nCuSize, m_nCuSize, subpelbuf, CTU_STRIDE, yFrac*multi, N, m_nCuSize, m_nCuSize);
		}
		return satd_8x8(pfenc, stride_2, subpelbuf, CTU_STRIDE, m_nCuSize, m_nCuSize);
	}
}

int RefineIntMotionEstimation::satd_8x8(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2, int w, int h)
{
	int satd = 0;

	for (int row = 0; row < h; row += 4)
	{
		for (int col = 0; col < w; col += 8)
		{
			satd += satd_8x4(pix1 + row * stride_pix1 + col, stride_pix1,
				pix2 + row * stride_pix2 + col, stride_pix2);
		}
	}

	return satd;
}

int RefineIntMotionEstimation::satd_8x4(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2)
{
	unsigned int tmp[4][4];
	unsigned int a0, a1, a2, a3;
	unsigned int sum = 0;

	for (int i = 0; i < 4; i++, pix1 += stride_pix1, pix2 += stride_pix2)
	{
		a0 = (pix1[0] - pix2[0]) + ((unsigned int)(pix1[4] - pix2[4]) << BITS_PER_VALUE);
		a1 = (pix1[1] - pix2[1]) + ((unsigned int)(pix1[5] - pix2[5]) << BITS_PER_VALUE);
		a2 = (pix1[2] - pix2[2]) + ((unsigned int)(pix1[6] - pix2[6]) << BITS_PER_VALUE);
		a3 = (pix1[3] - pix2[3]) + ((unsigned int)(pix1[7] - pix2[7]) << BITS_PER_VALUE);
		HADAMARD4(tmp[i][0], tmp[i][1], tmp[i][2], tmp[i][3], a0, a1, a2, a3);
	}

	for (int i = 0; i < 4; i++)
	{
		HADAMARD4(a0, a1, a2, a3, tmp[0][i], tmp[1][i], tmp[2][i], tmp[3][i]);
		sum += abs2(a0) + abs2(a1) + abs2(a2) + abs2(a3);
	}

	return (((unsigned short)sum) + (sum >> BITS_PER_VALUE)) >> 1;
}

void RefineIntMotionEstimation::InterpHorizLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	short const * coeff = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int headRoom = FILTER_PREC;
	int offset = (1 << (headRoom - 1));
	unsigned short maxVal = (1 << DEPTH) - 1;
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

void RefineIntMotionEstimation::InterpVertLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	short const * c = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int shift = FILTER_PREC;
	int offset = 1 << (shift - 1);
	unsigned short maxVal = (1 << DEPTH) - 1;

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

void RefineIntMotionEstimation::FilterHorizonLuma(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff, int N)
{
	int headRoom = INTERNAL_PREC - DEPTH;
	int shift = FILTER_PREC - headRoom;
	int offset = -INTERNAL_OFFS << shift;

	src -= N / 2 - 1;

	int row, col;
	for (row = 0; row < height; row++)
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

void RefineIntMotionEstimation::FilterVertLuma(short *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	int headRoom = INTERNAL_PREC - DEPTH;
	int shift = FILTER_PREC + headRoom;
	int offset = (1 << (shift - 1)) + (INTERNAL_OFFS << FILTER_PREC);
	unsigned short maxVal = (1 << DEPTH) - 1;
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

			short val = (short)((sum + offset) >> shift);

			val = (val < 0) ? 0 : val;
			val = (val > maxVal) ? maxVal : val;

			dst[col] = (unsigned char)val;
		}

		src += srcStride;
		dst += dstStride;
	}
}

//class FME
class FractionMotionEstimation Fme;

FractionMotionEstimation::FractionMotionEstimation()
{

}

FractionMotionEstimation::~FractionMotionEstimation()
{

}

void FractionMotionEstimation::Create()
{
	m_pCurrCuPel = new unsigned char[m_nCuSize*m_nCuSize*3/2];
	assert(RK_NULL != m_pCurrCuPel);

	m_pCurrCuResi = new unsigned char[m_nCuSize*m_nCuSize*3/2];
	assert(RK_NULL != m_pCurrCuResi);

	m_pFmeInterpPel = new unsigned char[(m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2)*3/2];
	assert(RK_NULL != m_pFmeInterpPel);
}

void FractionMotionEstimation::Destroy()
{
	if (RK_NULL != m_pCurrCuPel)
	{
		delete[] m_pCurrCuPel;
		m_pCurrCuPel = RK_NULL;
	}
	
	if (RK_NULL != m_pCurrCuResi)
	{
		delete[] m_pCurrCuResi;
		m_pCurrCuResi = RK_NULL;
	}
	
	if (RK_NULL != m_pFmeInterpPel)
	{
		delete[] m_pFmeInterpPel;
		m_pFmeInterpPel = RK_NULL;
	}
	
}

void FractionMotionEstimation::setFmeInterpPel(unsigned char *src, int src_stride)
{
	int nSize = m_nCuSize + 2 * 4;
	for (int i = 0; i < nSize; i ++)
	{
		for (int j = 0; j < nSize; j ++)
		{
			m_pFmeInterpPel[j + i*nSize] = src[j + i*src_stride];
		}
	}
	unsigned char *pSrc = src + nSize*nSize;
	unsigned char *pFmeInterpPel = m_pFmeInterpPel + nSize*nSize;
	for (int i = 0; i < nSize / 2; i ++)
	{
		for (int j = 0; j < nSize; j ++)
		{
			pFmeInterpPel[j + i*nSize] = pSrc[j + i*nSize];
		}
	}
}

void FractionMotionEstimation::setCurrCuPel(unsigned char *src, int src_stride)
{
	int nSize = m_nCuSize;
	for (int i = 0; i < nSize; i++)
	{
		for (int j = 0; j < nSize; j++)
		{
			m_pCurrCuPel[j + i*nSize] = src[j + i*src_stride];
		}
	}
	unsigned char *pCurrCuPel = m_pCurrCuPel + nSize*nSize;
	unsigned char *pSrc = src + nSize*nSize;
	for (int i = 0; i < nSize / 2; i ++)
	{
		for (int j = 0; j < nSize; j ++)
		{
			pCurrCuPel[j + i*nSize] = pSrc[j + i*nSize];
		}
	}
}

void FractionMotionEstimation::CalcResiAndMvd()
{
	InterpLuma(m_pCurrCuResi, m_nCuSize, m_mvFmeInput, 8);
	for (int i = 0; i < m_nCuSize; i ++)
	{
		for (int j = 0; j < m_nCuSize; j ++)
		{
			m_pCurrCuResi[j + i*m_nCuSize] = static_cast<unsigned char>(abs(m_pCurrCuResi[j + i*m_nCuSize] - m_pCurrCuPel[j + i*m_nCuSize]));
		}
	}

	int bCost = INT_MAX;
	for (int i = 0; i < 2; i ++)
	{
		int cost = abs(m_mvFmeInput.x - m_mvAmvp[i].x) + abs(m_mvFmeInput.y - m_mvAmvp[i].y);
		if (cost < bCost)
		{
			m_nMvpIdx = i;
			m_mvMvd.x = m_mvFmeInput.x - m_mvAmvp[i].x;
			m_mvMvd.y = m_mvFmeInput.y - m_mvAmvp[i].y;
		}
	}

	unsigned char *pCurrCuPred = new unsigned char[m_nCuSize*m_nCuSize / 4];
	unsigned char *pCurrCuPel = m_pCurrCuPel + m_nCuSize*m_nCuSize;
	unsigned char *pCurrCuResi = m_pCurrCuResi + m_nCuSize*m_nCuSize;
	InterpChroma(pCurrCuPred, m_nCuSize / 2, m_mvFmeInput, true, 4);
	for (int i = 0; i < m_nCuSize / 2; i ++)
	{
		for (int j = 0; j < m_nCuSize / 2; j ++)
		{
			pCurrCuResi[j + i*m_nCuSize / 2] = static_cast<unsigned char>(abs(pCurrCuPred[j + i*m_nCuSize / 2] - pCurrCuPel[j * 2 + 0 + i*m_nCuSize]));
		}
	}

	pCurrCuResi += m_nCuSize*m_nCuSize / 4;
	InterpChroma(pCurrCuPred, m_nCuSize / 2, m_mvFmeInput, false, 4);
	for (int i = 0; i < m_nCuSize / 2; i++)
	{
		for (int j = 0; j < m_nCuSize / 2; j++)
		{
			pCurrCuResi[j + i*m_nCuSize / 2] = static_cast<unsigned char>(abs(pCurrCuPred[j + i*m_nCuSize / 2] - pCurrCuPel[j * 2 + 1 + i*m_nCuSize]));
		}
	}

	//FILE *fp = fopen("e:\\mine_pred.txt", "ab");
	//for (int i = 0; i < m_nCuSize / 2; i++)
	//{
	//	for (int j = 0; j < m_nCuSize / 2; j++)
	//	{
	//		fprintf(fp, "%4d", pCurrCuPred[j + i*m_nCuSize / 2]);
	//	}
	//	fprintf(fp, "\n");
	//}
	//fclose(fp);

	//fp = fopen("e:\\mine_fenc.txt", "ab");
	//for (int i = 0; i < m_nCuSize / 2; i++)
	//{
	//	for (int j = 0; j < m_nCuSize / 2; j++)
	//	{
	//		fprintf(fp, "%4d", pCurrCuPel[j * 2 + 0 + i*m_nCuSize]);
	//	}
	//	fprintf(fp, "\n");
	//}
	//fclose(fp);

	delete[] pCurrCuPred;
}

void FractionMotionEstimation::InterpLuma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, int N)
{
	int xFrac = qmv.x & 0x3;
	int yFrac = qmv.y & 0x3;
	int multi = 1;
	int stride_1 = m_nCuSize + 4 * 2;
	unsigned char *pFmeInterpPel = RK_NULL;
	pFmeInterpPel = new unsigned char[(m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2)];
	for (int i = 0; i < (m_nCuSize + 8); i++)
	{
		for (int j = 0; j < (m_nCuSize + 8); j++)
		{
			pFmeInterpPel[j + i*stride_1] = m_pFmeInterpPel[j + i*stride_1];
		}
	}

	if ((yFrac | xFrac) == 0)
	{
		unsigned char *pRef = pFmeInterpPel + N / 2 + N / 2 * stride_1;
		for (int i = 0; i < m_nCuSize; i ++)
		{
			for (int j = 0; j < m_nCuSize; j ++)
			{
				pFmeInterp[j + i*m_nCuSize] = pRef[j + i*stride_1];
			}
		}
	}
	else
	{
		//unsigned char subpelbuf[64 * 64];
		if (yFrac == 0)
		{
			InterpHorizLuma(pFmeInterpPel + N / 2 + N / 2 * stride_1, stride_1, pFmeInterp, stride_2, xFrac*multi, N, m_nCuSize, m_nCuSize);
		}
		else if (xFrac == 0)
		{
			InterpVertLuma(pFmeInterpPel + N / 2 + N / 2 * stride_1, stride_1, pFmeInterp, stride_2, yFrac*multi, N, m_nCuSize, m_nCuSize);
		}
		else
		{
			short immedVal[64 * (64 + 8)];
			int filterSize = N;
			int halfFilterSize = (filterSize >> 1);
			const short *pFilter;
			if (4 == N)
				pFilter = ChromaFilter[xFrac*multi];
			else
				pFilter = LumaFilter[xFrac];
			FilterHorizonLuma(pFmeInterpPel + N / 2 + N / 2 * stride_1 - (halfFilterSize - 1) * stride_1, stride_1, immedVal, m_nCuSize, m_nCuSize, m_nCuSize + filterSize - 1, pFilter, N);
			FilterVertLuma(immedVal + (halfFilterSize - 1) * m_nCuSize, m_nCuSize, pFmeInterp, stride_2, yFrac*multi, N, m_nCuSize, m_nCuSize);
		}
	}
	delete[] pFmeInterpPel;
}

void FractionMotionEstimation::InterpHorizLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	short const * coeff = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int headRoom = FILTER_PREC;
	int offset = (1 << (headRoom - 1));
	unsigned short maxVal = (1 << DEPTH) - 1;
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

void FractionMotionEstimation::InterpVertLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	short const * c = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int shift = FILTER_PREC;
	int offset = 1 << (shift - 1);
	unsigned short maxVal = (1 << DEPTH) - 1;

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

void FractionMotionEstimation::FilterHorizonLuma(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff, int N)
{
	int headRoom = INTERNAL_PREC - DEPTH;
	int shift = FILTER_PREC - headRoom;
	int offset = -INTERNAL_OFFS << shift;

	src -= N / 2 - 1;

	int row, col;
	for (row = 0; row < height; row++)
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

void FractionMotionEstimation::FilterVertLuma(short *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	int headRoom = INTERNAL_PREC - DEPTH;
	int shift = FILTER_PREC + headRoom;
	int offset = (1 << (shift - 1)) + (INTERNAL_OFFS << FILTER_PREC);
	unsigned short maxVal = (1 << DEPTH) - 1;
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

			short val = (short)((sum + offset) >> shift);

			val = (val < 0) ? 0 : val;
			val = (val > maxVal) ? maxVal : val;

			dst[col] = (unsigned char)val;
		}

		src += srcStride;
		dst += dstStride;
	}
}

void FractionMotionEstimation::InterpChroma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, bool isCb, int N)
{
	int xFrac = qmv.x & 0x7;
	int yFrac = qmv.y & 0x7;
	int stride_1 = (m_nCuSize + 4 * 2)/2;
	int nCuSize = m_nCuSize / 2;
	int offs = isCb ? 0 : 1;
	unsigned char *pFmeInterpPel = RK_NULL;
	pFmeInterpPel = new unsigned char[(m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2)/4];
	for (int i = 0; i < (m_nCuSize + 8)/2; i++)
	{
		for (int j = 0; j < (m_nCuSize + 8)/2; j++)
		{
			pFmeInterpPel[j + i*stride_1] = m_pFmeInterpPel[j * 2 + offs + i*stride_1 * 2 + (m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2)];
		}
	}

	if ((yFrac | xFrac) == 0)
	{
		unsigned char *pRef = pFmeInterpPel + N / 2 + N / 2 * stride_1;
		for (int i = 0; i < nCuSize; i++)
		{
			for (int j = 0; j < nCuSize; j++)
			{
				pFmeInterp[j + i*nCuSize] = pRef[j + i*stride_1];
			}
		}
	}
	else
	{
		//unsigned char subpelbuf[64 * 64];
		if (yFrac == 0)
		{
			InterpHorizChroma(pFmeInterpPel + N / 2 + N / 2 * stride_1, stride_1, pFmeInterp, stride_2, xFrac, N, nCuSize, nCuSize);
		}
		else if (xFrac == 0)
		{
			InterpVertChroma(pFmeInterpPel + N / 2 + N / 2 * stride_1, stride_1, pFmeInterp, stride_2, yFrac, N, nCuSize, nCuSize);
		}
		else
		{
			short immedVal[64 * (64 + 8)];
			int filterSize = N;
			int halfFilterSize = (filterSize >> 1);
			FilterHorizonChroma(pFmeInterpPel + N / 2 + N / 2 * stride_1, stride_1, immedVal, nCuSize, xFrac, 1, N, nCuSize, nCuSize);
			FilterVertChroma(immedVal + (halfFilterSize - 1) * nCuSize, nCuSize, pFmeInterp, stride_2, nCuSize, nCuSize, yFrac, N);
		}
	}
	delete[] pFmeInterpPel;
}

void FractionMotionEstimation::InterpHorizChroma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	short const * coeff = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int headRoom = FILTER_PREC;
	int offset = (1 << (headRoom - 1));
	unsigned short maxVal = (1 << DEPTH) - 1;
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

void FractionMotionEstimation::InterpVertChroma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	short const * c = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int shift = FILTER_PREC;
	int offset = 1 << (shift - 1);
	unsigned short maxVal = (1 << DEPTH) - 1;

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

void FractionMotionEstimation::FilterHorizonChroma(unsigned char *src, int srcStride, short *dst, int dstStride, int coeffIdx, int isRowExt, int N, int width, int height)
{
	short const * coeff = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int headRoom = INTERNAL_PREC - DEPTH;
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

void FractionMotionEstimation::FilterVertChroma(short *src, int srcStride, unsigned char *dst, int dstStride, int width, int height, int coeffIdx, int N)
{
	int headRoom = INTERNAL_PREC - DEPTH;
	int shift = FILTER_PREC + headRoom;
	int offset = (1 << (shift - 1)) + (INTERNAL_OFFS << FILTER_PREC);
	unsigned short maxVal = (1 << DEPTH) - 1;
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

			short val = (short)((sum + offset) >> shift);

			val = (val < 0) ? 0 : val;
			val = (val > maxVal) ? maxVal : val;

			dst[col] = (unsigned char)val;
		}

		src += srcStride;
		dst += dstStride;
	}
}

//class Merge
class MergeProc Merge;

MergeProc::MergeProc()
{

}

MergeProc::~MergeProc()
{

}

void MergeProc::Create()
{
	m_pMergeSW[0] = new unsigned char[(m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2)*3/2];
	assert(RK_NULL != m_pMergeSW[0]);

	m_pMergeSW[1] = new unsigned char[(m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2)*3/2];
	assert(RK_NULL != m_pMergeSW[1]);

	m_pMergeSW[2] = new unsigned char[(m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2)*3/2];
	assert(RK_NULL != m_pMergeSW[2]);

	m_pMergeResi = new unsigned char[m_nCuSize*m_nCuSize*3/2];
	assert(RK_NULL != m_pMergeResi);

	m_pCurrCuPel = new unsigned char[m_nCuSize*m_nCuSize*3/2];
	assert(RK_NULL != m_pCurrCuPel);
}

void MergeProc::Destroy()
{
	for (int i = 0; i < 3; i ++)
	{
		if (RK_NULL != m_pMergeSW[i])
		{
			delete[] m_pMergeSW[i];
			m_pMergeSW[i] = RK_NULL;
		}
	}
	
	if (RK_NULL!=m_pMergeResi)
	{
		delete[] m_pMergeResi;
		m_pMergeResi = RK_NULL;
	}
	
	if (RK_NULL!=m_pCurrCuPel)
	{
		delete[] m_pCurrCuPel;
		m_pCurrCuPel = RK_NULL;
	}
}

void MergeProc::PrefetchMergeSW(unsigned char *src_0, int stride_0, unsigned char *src_1, int stride_1)
{
	Mv mvmin[2], mvmax[2];
	short Width = static_cast<short>(nRimeWidth);
	short Height = static_cast<short>(nRimeHeight);
	short offsX = OffsFromCtu64[m_nCuPosInCtu][0];
	short offsY = OffsFromCtu64[m_nCuPosInCtu][1];
	if (32 == m_nCtuSize)
	{
		offsX = OffsFromCtu32[m_nCuPosInCtu][0];
		offsY = OffsFromCtu32[m_nCuPosInCtu][1];
	}

	for (int idx = 0; idx < 2; idx++)
	{
		mvmin[idx].x = m_mvNeighPmv[idx].x - offsX - Width;
		mvmin[idx].y = m_mvNeighPmv[idx].y - offsY - Height;
		mvmax[idx].x = m_mvNeighPmv[idx].x + nCtuSize + Width - offsX - static_cast<unsigned char>(m_nCuSize);
		mvmax[idx].y = m_mvNeighPmv[idx].y + nCtuSize + Height - offsY - static_cast<unsigned char>(m_nCuSize);
	}

	m_bMergeSwValid[0] = m_bMergeSwValid[1] = m_bMergeSwValid[2] = false;
	for (int nMergeIdx = 0; nMergeIdx < 3; nMergeIdx ++)
	{
		if (((m_mvMergeCand[nMergeIdx].x>>2) >= mvmin[0].x && (m_mvMergeCand[nMergeIdx].y >>2) >= mvmin[0].y) &&
			((m_mvMergeCand[nMergeIdx].x >>2) <= mvmax[0].x && (m_mvMergeCand[nMergeIdx].y>>2) <= mvmax[0].y))
		{
			short offset_x = (m_mvMergeCand[nMergeIdx].x >> 2) - m_mvNeighPmv[0].x + offsX + Width;
			short offset_y = (m_mvMergeCand[nMergeIdx].y >> 2) - m_mvNeighPmv[0].y + offsY + Height;
			unsigned char *pNeigbPmv0 = src_0 + offset_x + offset_y*stride_0;
			for (int i = 0; i < (m_nCuSize + 8); i ++)
			{
				for (int j = 0; j < (m_nCuSize+8); j ++)
				{
					m_pMergeSW[nMergeIdx][j + i*(m_nCuSize + 8)] = pNeigbPmv0[j + i*stride_0];
				}
			}
			int offset = (nCtuSize + 2 * (nRimeWidth + 4))*(nCtuSize + 2 * (nRimeHeight + 4));
			offset_x = (m_mvMergeCand[nMergeIdx].x >> 3) * 2 - m_mvNeighPmv[0].x + offsX + Width;
			offset_y = (m_mvMergeCand[nMergeIdx].y >> 3) - m_mvNeighPmv[0].y / 2 + offsY / 2 + Height / 2;
			pNeigbPmv0 = src_0 + offset;
		    pNeigbPmv0 += offset_x + offset_y*stride_0;
			unsigned char *pMergeSw = m_pMergeSW[nMergeIdx] + (m_nCuSize + 8)*(m_nCuSize + 8);
			for (int i = 0; i < (m_nCuSize + 8)/2; i++)
			{
				for (int j = 0; j < (m_nCuSize + 8); j++)
				{
					pMergeSw[j + i*(m_nCuSize + 8)] = pNeigbPmv0[j + i*stride_0];
				}
			}

			m_bMergeSwValid[nMergeIdx] = true;
			continue;
		}

		if (((m_mvMergeCand[nMergeIdx].x >> 2) >= mvmin[1].x && (m_mvMergeCand[nMergeIdx].y >> 2) >= mvmin[1].y) &&
			((m_mvMergeCand[nMergeIdx].x >> 2) <= mvmax[1].x && (m_mvMergeCand[nMergeIdx].y >> 2) <= mvmax[1].y))
		{
			short offset_x = (m_mvMergeCand[nMergeIdx].x >> 2) - m_mvNeighPmv[1].x + offsX + Width;
			short offset_y = (m_mvMergeCand[nMergeIdx].y >> 2) - m_mvNeighPmv[1].y + offsY + Height;
			unsigned char *pNeigbPmv1 = src_1 + offset_x + offset_y*stride_1;
			for (int i = 0; i < (m_nCuSize + 8); i++)
			{
				for (int j = 0; j < (m_nCuSize + 8); j++)
				{
					m_pMergeSW[nMergeIdx][j + i*(m_nCuSize + 8)] = pNeigbPmv1[j + i*stride_1];
				}
			}

			int offset = (nCtuSize + 2 * (nRimeWidth + 4))*(nCtuSize + 2 * (nRimeHeight + 4));
			offset_x = (m_mvMergeCand[nMergeIdx].x >> 3) * 2 - m_mvNeighPmv[1].x + offsX + Width;
			offset_y = (m_mvMergeCand[nMergeIdx].y >> 3) - m_mvNeighPmv[1].y / 2 + offsY / 2 + Height / 2;
			pNeigbPmv1 = src_1 + offset;
			pNeigbPmv1 += offset_x + offset_y*stride_1;
			unsigned char *pMergeSw = m_pMergeSW[nMergeIdx] + (m_nCuSize + 8)*(m_nCuSize + 8);
			for (int i = 0; i < (m_nCuSize + 8) / 2; i++)
			{
				for (int j = 0; j < (m_nCuSize + 8); j++)
				{
					pMergeSw[j + i*(m_nCuSize + 8)] = pNeigbPmv1[j + i*stride_1];
				}
			}

			m_bMergeSwValid[nMergeIdx] = true;
			continue;
		}
	}
}

void MergeProc::setCurrCuPel(unsigned char *src, int src_stride)
{
	int nSize = m_nCuSize;
	for (int i = 0; i < nSize; i++)
	{
		for (int j = 0; j < nSize; j++)
		{
			m_pCurrCuPel[j + i*nSize] = src[j + i*src_stride];
		}
	}
}

void MergeProc::CalcResiAndMvd()
{
	unsigned char *pMergeResi = new unsigned char[m_nCuSize*m_nCuSize];
	int bCost = INT_MAX;
	int nBestMergeCand = 0;
	for (int nMergeCand = 0; nMergeCand < 3; nMergeCand ++)
	{
		if (!m_bMergeSwValid[nMergeCand])
			continue;
		InterpLuma(m_pMergeSW[nMergeCand], m_nCuSize + 8, pMergeResi, m_nCuSize, m_mvMergeCand[nMergeCand]);
		int cost = SAD(m_pCurrCuPel, m_nCuSize, pMergeResi, m_nCuSize, m_nCuSize, m_nCuSize);
		if (cost < bCost)
		{
			nBestMergeCand = nMergeCand;
			bCost = cost;
			for (int i = 0; i < m_nCuSize; i ++)
			{
				for (int j = 0; j < m_nCuSize; j++)
				{
					m_pMergeResi[j + i*m_nCuSize] = static_cast<unsigned char>(abs(pMergeResi[j + i * m_nCuSize] - m_pCurrCuPel[j + i*m_nCuSize]));
				}
			}
		}
	}
	InterpChroma(pMergeResi, m_nCuSize, m_mvMergeCand[nBestMergeCand], true, nBestMergeCand, 4);
	for (int i = 0; i < m_nCuSize/2; i++)
	{
		for (int j = 0; j < m_nCuSize/2; j++)
		{
			m_pMergeResi[j + i*m_nCuSize/2 + m_nCuSize*m_nCuSize] = static_cast<unsigned char>(abs(pMergeResi[j + i * m_nCuSize] - m_pCurrCuPel[j + i*m_nCuSize]));
		}
	}
	InterpChroma(pMergeResi, m_nCuSize, m_mvMergeCand[nBestMergeCand], false, nBestMergeCand, 4);
	for (int i = 0; i < m_nCuSize / 2; i++)
	{
		for (int j = 0; j < m_nCuSize / 2; j++)
		{
			m_pMergeResi[j + i*m_nCuSize / 2 + m_nCuSize*m_nCuSize*5/4] = static_cast<unsigned char>(abs(pMergeResi[j + i * m_nCuSize] - m_pCurrCuPel[j + i*m_nCuSize]));
		}
	}

	delete[] pMergeResi;
}

void MergeProc::InterpLuma(unsigned char *pMergeForInterp, int stride_1, unsigned char *pMergePred, int stride_2, const Mv& qmv)
{
	int xFrac = qmv.x & 0x3;
	int yFrac = qmv.y & 0x3;
	int N = 8;
	int multi = 1;
	if (4 == N)
		multi = 2;
	else
		multi = 1;

	if ((yFrac | xFrac) == 0)
	{
		unsigned char *pRef = pMergeForInterp + 4 + 4 * stride_1;
		for (int i = 0; i < m_nCuSize; i++)
		{
			for (int j = 0; j < m_nCuSize; j++)
			{
				pMergePred[j + i*stride_2] = pRef[j + i*stride_1];
			}
		}
	}
	else
	{
		//unsigned char subpelbuf[64 * 64];
		if (yFrac == 0)
		{
			InterpHorizLuma(pMergeForInterp + 4 + 4 * stride_1, stride_1, pMergePred, stride_2, xFrac*multi, N, m_nCuSize, m_nCuSize);
		}
		else if (xFrac == 0)
		{
			InterpVertLuma(pMergeForInterp + 4 + 4 * stride_1, stride_1, pMergePred, stride_2, yFrac*multi, N, m_nCuSize, m_nCuSize);
		}
		else
		{
			short immedVal[64 * (64 + 8)];
			int filterSize = N;
			int halfFilterSize = (filterSize >> 1);
			const short *pFilter;
			if (4 == N)
				pFilter = ChromaFilter[xFrac*multi];
			else
				pFilter = LumaFilter[xFrac];
			FilterHorizonLuma(pMergeForInterp + 4 + 4 * stride_1 - (halfFilterSize - 1) * stride_1, stride_1, immedVal, m_nCuSize, m_nCuSize, m_nCuSize + filterSize - 1, pFilter, N);
			FilterVertLuma(immedVal + (halfFilterSize - 1) * m_nCuSize, m_nCuSize, pMergePred, stride_2, yFrac*multi, N, m_nCuSize, m_nCuSize);
		}
	}
}

void MergeProc::InterpHorizLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	short const * coeff = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int headRoom = FILTER_PREC;
	int offset = (1 << (headRoom - 1));
	unsigned short maxVal = (1 << DEPTH) - 1;
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

void MergeProc::InterpVertLuma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	short const * c = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int shift = FILTER_PREC;
	int offset = 1 << (shift - 1);
	unsigned short maxVal = (1 << DEPTH) - 1;

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

void MergeProc::FilterHorizonLuma(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff, int N)
{
	int headRoom = INTERNAL_PREC - DEPTH;
	int shift = FILTER_PREC - headRoom;
	int offset = -INTERNAL_OFFS << shift;

	src -= N / 2 - 1;

	int row, col;
	for (row = 0; row < height; row++)
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

void MergeProc::FilterVertLuma(short *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	int headRoom = INTERNAL_PREC - DEPTH;
	int shift = FILTER_PREC + headRoom;
	int offset = (1 << (shift - 1)) + (INTERNAL_OFFS << FILTER_PREC);
	unsigned short maxVal = (1 << DEPTH) - 1;
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

			short val = (short)((sum + offset) >> shift);

			val = (val < 0) ? 0 : val;
			val = (val > maxVal) ? maxVal : val;

			dst[col] = (unsigned char)val;
		}

		src += srcStride;
		dst += dstStride;
	}
}

void MergeProc::InterpChroma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, bool isCb, int nBestMerge, int N)
{
	int xFrac = qmv.x & 0x7;
	int yFrac = qmv.y & 0x7;
	int stride_1 = (m_nCuSize + 4 * 2) / 2;
	int nCuSize = m_nCuSize / 2;
	int offs = isCb ? 0 : 1;
	unsigned char *pMergeInterpPel = new unsigned char[(m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2) / 4];
	for (int i = 0; i < (m_nCuSize + 8) / 2; i++)
	{
		for (int j = 0; j < (m_nCuSize + 8) / 2; j++)
		{
			pMergeInterpPel[j + i*stride_1] = m_pMergeSW[nBestMerge][j * 2 + offs + i*stride_1 * 2 + (m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2)];
		}
	}

	if ((yFrac | xFrac) == 0)
	{
		unsigned char *pRef = pMergeInterpPel + N / 2 + N / 2 * stride_1;
		for (int i = 0; i < nCuSize; i++)
		{
			for (int j = 0; j < nCuSize; j++)
			{
				pFmeInterp[j + i*nCuSize] = pRef[j + i*stride_1];
			}
		}
	}
	else
	{
		//unsigned char subpelbuf[64 * 64];
		if (yFrac == 0)
		{
			InterpHorizChroma(pMergeInterpPel + N / 2 + N / 2 * stride_1, stride_1, pFmeInterp, stride_2, xFrac, N, nCuSize, nCuSize);
		}
		else if (xFrac == 0)
		{
			InterpVertChroma(pMergeInterpPel + N / 2 + N / 2 * stride_1, stride_1, pFmeInterp, stride_2, yFrac, N, nCuSize, nCuSize);
		}
		else
		{
			short immedVal[64 * (64 + 8)];
			int filterSize = N;
			int halfFilterSize = (filterSize >> 1);
			FilterHorizonChroma(pMergeInterpPel + N / 2 + N / 2 * stride_1, stride_1, immedVal, nCuSize, xFrac, 1, N, nCuSize, nCuSize);
			FilterVertChroma(immedVal + (halfFilterSize - 1) * nCuSize, nCuSize, pFmeInterp, stride_2, nCuSize, nCuSize, yFrac, N);
		}
	}
	delete[] pMergeInterpPel;
}

void MergeProc::InterpHorizChroma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	short const * coeff = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int headRoom = FILTER_PREC;
	int offset = (1 << (headRoom - 1));
	unsigned short maxVal = (1 << DEPTH) - 1;
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

void MergeProc::InterpVertChroma(unsigned char *src, int srcStride, unsigned char *dst, int dstStride, int coeffIdx, int N, int width, int height)
{
	short const * c = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int shift = FILTER_PREC;
	int offset = 1 << (shift - 1);
	unsigned short maxVal = (1 << DEPTH) - 1;

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

void MergeProc::FilterHorizonChroma(unsigned char *src, int srcStride, short *dst, int dstStride, int coeffIdx, int isRowExt, int N, int width, int height)
{
	short const * coeff = (N == 4) ? ChromaFilter[coeffIdx] : LumaFilter[coeffIdx];
	int headRoom = INTERNAL_PREC - DEPTH;
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

void MergeProc::FilterVertChroma(short *src, int srcStride, unsigned char *dst, int dstStride, int width, int height, int coeffIdx, int N)
{
	int headRoom = INTERNAL_PREC - DEPTH;
	int shift = FILTER_PREC + headRoom;
	int offset = (1 << (shift - 1)) + (INTERNAL_OFFS << FILTER_PREC);
	unsigned short maxVal = (1 << DEPTH) - 1;
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

			short val = (short)((sum + offset) >> shift);

			val = (val < 0) ? 0 : val;
			val = (val > maxVal) ? maxVal : val;

			dst[col] = (unsigned char)val;
		}

		src += srcStride;
		dst += dstStride;
	}
}

int MergeProc::SSE(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2, int lx, int ly)
{
	int sum = 0;
	int iTemp;

	for (int y = 0; y < ly; y++)
	{
		for (int x = 0; x < lx; x++)
		{
			iTemp = pix1[x] - pix2[x];
			sum += (iTemp * iTemp);
		}

		pix1 += stride_pix1;
		pix2 += stride_pix2;
	}

	return sum;
}

int MergeProc::SAD(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2, int lx, int ly)
{
	int sum = 0;

	for (int y = 0; y < ly; y++)
	{
		for (int x = 0; x < lx; x++)
		{
			sum += abs(pix1[x] - pix2[x]);
		}

		pix1 += stride_pix1;
		pix2 += stride_pix2;
	}

	return sum;
}

int MergeProc::SATD(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2, int w, int h)
{
	int satd = 0;

	for (int row = 0; row < h; row += 4)
	{
		for (int col = 0; col < w; col += 8)
		{
			satd += satd_8x4(pix1 + row * stride_pix1 + col, stride_pix1,
				pix2 + row * stride_pix2 + col, stride_pix2);
		}
	}

	return satd;
}

int MergeProc::satd_8x4(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2)
{
	unsigned int tmp[4][4];
	unsigned int a0, a1, a2, a3;
	unsigned int sum = 0;

	for (int i = 0; i < 4; i++, pix1 += stride_pix1, pix2 += stride_pix2)
	{
		a0 = (pix1[0] - pix2[0]) + ((unsigned int)(pix1[4] - pix2[4]) << BITS_PER_VALUE);
		a1 = (pix1[1] - pix2[1]) + ((unsigned int)(pix1[5] - pix2[5]) << BITS_PER_VALUE);
		a2 = (pix1[2] - pix2[2]) + ((unsigned int)(pix1[6] - pix2[6]) << BITS_PER_VALUE);
		a3 = (pix1[3] - pix2[3]) + ((unsigned int)(pix1[7] - pix2[7]) << BITS_PER_VALUE);
		HADAMARD4(tmp[i][0], tmp[i][1], tmp[i][2], tmp[i][3], a0, a1, a2, a3);
	}

	for (int i = 0; i < 4; i++)
	{
		HADAMARD4(a0, a1, a2, a3, tmp[0][i], tmp[1][i], tmp[2][i], tmp[3][i]);
		sum += abs2(a0) + abs2(a1) + abs2(a2) + abs2(a3);
	}

	return (((unsigned short)sum) + (sum >> BITS_PER_VALUE)) >> 1;
}
