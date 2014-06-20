#include "inter.h"
#include "math.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"
#include <algorithm>

Mv g_leftPMV[nMaxRefPic];
Mv g_leftPmv[nMaxRefPic];
Mv g_RimeMv[nMaxRefPic][85];
Mv g_FmeMv[nMaxRefPic][85];
Mv g_mvAmvp[nMaxRefPic][85][2];
MvField g_mvMerge[85][3*2];
Mv g_Mvmin[nMaxRefPic][nNeightMv + 1];
Mv g_Mvmax[nMaxRefPic][nNeightMv + 1];
short g_FmeResiY[4][64 * 64]; //add this for verify FME procedure
short g_FmeResiU[4][32 * 32];
short g_FmeResiV[4][32 * 32];
short g_MergeResiY[4][64 * 64];
short g_MergeResiU[4][32 * 32];
short g_MergeResiV[4][32 * 32];

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

unsigned int RasterScan[85] =
{
	0, 4, 16, 20, 64, 68, 80, 84,
	8, 12, 24, 28, 72, 76, 88, 92,
	32, 36, 48, 52, 96, 100, 112, 116,
	40, 44, 56, 60, 104, 108, 120, 124,
	128, 132, 144, 148, 192, 196, 208, 212,
	136, 140, 152, 156, 200, 204, 216, 220,
	160, 164, 176, 180, 224, 228, 240, 244,
	168, 172, 184, 188, 232, 236, 248, 252,
	0, 16, 64, 80, 32, 48, 96, 112,
	128, 144, 192, 208, 160, 176, 224, 240,
	0, 64, 128, 192, 0
};

unsigned int RasterToZscan[256] = 
{
	0, 1, 4, 5, 16, 17, 20, 21, 64, 65, 68, 69, 80, 81, 84, 85,
	2, 3, 6, 7, 18, 19, 22, 23, 66, 67, 70, 71, 82, 83, 86, 87,
	8, 9, 12, 13, 24, 25, 28, 29, 72, 73, 76, 77, 88, 89, 92, 93,
	10, 11, 14, 15, 26, 27, 30, 31, 74, 75, 78, 79, 90, 91, 94, 95,
	32, 33, 36, 37, 48, 49, 52, 53, 96, 97, 100, 101, 112, 113, 116, 117,
	34, 35, 38, 39, 50, 51, 54, 55, 98, 99, 102, 103, 114, 115, 118, 119,
	40, 41, 44, 45, 56, 57, 60, 61, 104, 105, 108, 109, 120, 121, 124, 125,
	42, 43, 46, 47, 58, 59, 62, 63, 106, 107, 110, 111, 122, 123, 126, 127,
	128, 129, 132, 133, 144, 145, 148, 149, 192, 193, 196, 197, 208, 209, 212, 213,
	130, 131, 134, 135, 146, 147, 150, 151, 194, 195, 198, 199, 210, 211, 214, 215,
	136, 137, 140, 141, 152, 153, 156, 157, 200, 201, 204, 205, 216, 217, 220, 221,
	138, 139, 142, 143, 154, 155, 158, 159, 202, 203, 206, 207, 218, 219, 222, 223,
	160, 161, 164, 165, 176, 177, 180, 181, 224, 225, 228, 229, 240, 241, 244, 245,
	162, 163, 166, 167, 178, 179, 182, 183, 226, 227, 230, 231, 242, 243, 246, 247,
	168, 169, 172, 173, 184, 185, 188, 189, 232, 233, 236, 237, 248, 249, 252, 253,
	170, 171, 174, 175, 186, 187, 190, 191, 234, 235, 238, 239, 250, 251, 254, 255
};

unsigned int ZscanToRaster[256] =
{
	0, 1, 16, 17, 2, 3, 18, 19, 32, 33, 48, 49, 34, 35, 50, 51,
	4, 5, 20, 21, 6, 7, 22, 23, 36, 37, 52, 53, 38, 39, 54, 55,
	64, 65, 80, 81, 66, 67, 82, 83, 96, 97, 112, 113, 98, 99, 114, 115,
	68, 69, 84, 85, 70, 71, 86, 87, 100, 101, 116, 117, 102, 103, 118, 119,
	8, 9, 24, 25, 10, 11, 26, 27, 40, 41, 56, 57, 42, 43, 58, 59,
	12, 13, 28, 29, 14, 15, 30, 31, 44, 45, 60, 61, 46, 47, 62, 63,
	72, 73, 88, 89, 74, 75, 90, 91, 104, 105, 120, 121, 106, 107, 122, 123,
	76, 77, 92, 93, 78, 79, 94, 95, 108, 109, 124, 125, 110, 111, 126, 127,
	128, 129, 144, 145, 130, 131, 146, 147, 160, 161, 176, 177, 162, 163, 178, 179,
	132, 133, 148, 149, 134, 135, 150, 151, 164, 165, 180, 181, 166, 167, 182, 183,
	192, 193, 208, 209, 194, 195, 210, 211, 224, 225, 240, 241, 226, 227, 242, 243,
	196, 197, 212, 213, 198, 199, 214, 215, 228, 229, 244, 245, 230, 231, 246, 247,
	136, 137, 152, 153, 138, 139, 154, 155, 168, 169, 184, 185, 170, 171, 186, 187,
	140, 141, 156, 157, 142, 143, 158, 159, 172, 173, 188, 189, 174, 175, 190, 191,
	200, 201, 216, 217, 202, 203, 218, 219, 232, 233, 248, 249, 234, 235, 250, 251,
	204, 205, 220, 221, 206, 207, 222, 223, 236, 237, 252, 253, 238, 239, 254, 255
};

unsigned int RasterToPelX[256] = 
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60
};

unsigned int RasterToPelY[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
	40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
	44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
	48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
	52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
	56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
	60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60
};

double Lambda[QP_MAX] =
{
	0.05231, 0.060686, 0.07646, 0.096333333, 0.151715667, 0.15292, 0.192666667, 0.242745, 0.382299,
	0.385333333, 0.485489333, 0.611678333, 0.963333333, 0.970979, 1.223356667, 1.541333333, 2.427447667,
	2.446714, 3.082666667, 3.883916667, 6.116785, 6.165333333, 7.767833333, 9.786856667, 15.41333333,
	16.57137733, 22.183542, 29.5936, 39.357022, 52.19656867, 69.05173333, 91.14257667, 150.0651347,
	157.8325333, 207.1422197, 271.4221573, 443.904, 447.4271947, 563.722941, 710.2464, 1118.567987,
	1127.445883, 1420.4928, 1789.70878, 2818.614706, 2840.9856, 3579.41756, 4509.78353, 7102.464,
	7158.83512, 9019.56706, 11363.9424, 14317.66967, 18039.13269, 22727.8821, 28635.33594, 36078.2611,
	45455.7588, 57270.66508, 72156.51362, 90911.5068, 114541.3166, 144313.0101, 181822.992, 229082.6059,
	288625.9859, 363645.9408, 458165.1574, 577251.9032, 727291.7952
};

int LambdaMotionSAD_tab_non_I[QP_MAX] =
{
	65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
	65536, 65536, 65536, 65536, 72486, 81363, 102106, 102511, 115064, 129156,
	162084, 162726, 182654, 205022, 257293, 266783, 308670, 356515, 411141, 473479,
	544586, 625663, 802823, 823338, 943222, 1079698, 1380779, 1386248, 1556011, 1746563,
	2191851, 2200532, 2470014, 2772497, 3479347, 3493127, 3920903, 4401064, 5523119, 5544994,
	6224045, 6986255
};

short LumaFilter[4][LUMA_NTAPS] =
{
	{ 0, 0, 0, 64, 0, 0, 0, 0 },
	{ -1, 4, -10, 58, 17, -5, 1, 0 },
	{ -1, 4, -11, 40, 40, -11, 4, -1 },
	{ 0, 1, -5, 17, 58, -10, 4, -1 }
};

short ChromaFilter[8][CHROMA_NTAPS] =
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

template<typename T>
inline T Clip3(T minVal, T maxVal, T a) { return std::min<T>(std::max<T>(minVal, a), maxVal); } ///< general min/max clip

//class CIME
unsigned short CoarseIntMotionEstimation::m_pMvCost[QP_MAX][MV_MAX * 2];

class CoarseIntMotionEstimation Cime;

CoarseIntMotionEstimation::CoarseIntMotionEstimation()
{
	m_nCtuSize = 64;
	m_nDownSampDist = 4;
	m_nRefPic[0] = 1;
	m_nRefPic[1] = 0;
}

CoarseIntMotionEstimation::~CoarseIntMotionEstimation()
{

}

void CoarseIntMotionEstimation::Create(int nCimeSwWidth, int nCimeSwHeight, int nCtu, unsigned int nDownSampDist)
{
	m_nCtuSize = nCtu;
	m_nDownSampDist = nDownSampDist;
	m_nCimeSwWidth = nCimeSwWidth;
	m_nCimeSwHeight = nCimeSwHeight;
	m_pCimeSW = new short[nCimeSwWidth*nCimeSwHeight];
	assert(RK_NULL != m_pCimeSW);
	m_pCurrCtu = new unsigned char[nCtu*nCtu];
	assert(RK_NULL != m_pCurrCtu);
	m_pCurrCtuDS = new short[nCtu / m_nDownSampDist * nCtu / m_nDownSampDist];
	assert(RK_NULL != m_pCurrCtuDS);
	m_pRefPicDS = new short *[m_nRefPic[0]+m_nRefPic[1]];
	assert(RK_NULL != m_pRefPicDS);
	for (int nRefPic = 0; nRefPic < m_nRefPic[0] + m_nRefPic[1]; nRefPic++)
	{
		m_pRefPicDS[nRefPic] = new short[(m_nPicWidth / m_nDownSampDist)*(m_nPicHeight / m_nDownSampDist)];
		assert(RK_NULL != m_pRefPicDS[nRefPic]);
	}
	m_mvNeighMv = new Mv*[m_nRefPic[0] + m_nRefPic[1]];
	assert(RK_NULL != m_mvNeighMv);
	for (int nRefPic = 0; nRefPic < m_nRefPic[0] + m_nRefPic[1]; nRefPic++)
	{
		m_mvNeighMv[nRefPic] = new Mv[nAllNeighMv];
		assert(RK_NULL != m_mvNeighMv[nRefPic]);
	}
	m_mvCimeOut = new Mv[m_nRefPic[0] + m_nRefPic[1]];
	assert(RK_NULL != m_mvCimeOut);
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

	for (int nRefPic = 0; nRefPic < m_nRefPic[0] + m_nRefPic[1]; nRefPic++)
	{
		if (RK_NULL != m_pRefPicDS[nRefPic])
		{
			delete[] m_pRefPicDS[nRefPic];
			m_pRefPicDS[nRefPic] = RK_NULL;
		}
	}
	delete[] m_pRefPicDS;
	m_pRefPicDS = RK_NULL;

	for (int nRefPic = 0; nRefPic < m_nRefPic[0] + m_nRefPic[1]; nRefPic++)
	{
		if (RK_NULL != m_mvNeighMv[nRefPic])
		{
			delete[] m_mvNeighMv[nRefPic];
			m_mvNeighMv[nRefPic] = RK_NULL;
		}
	}
	if (RK_NULL != m_mvNeighMv)
	{
		delete[] m_mvNeighMv;
		m_mvNeighMv = RK_NULL;
	}

	if (RK_NULL != m_mvCimeOut)
	{
		delete[] m_mvCimeOut;
		m_mvCimeOut = RK_NULL;
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
	unsigned int nCtu = m_nCtuSize / m_nDownSampDist;
	unsigned int nPicWidth = m_nPicWidth / m_nDownSampDist;
	unsigned int nPicHeight = m_nPicHeight / m_nDownSampDist;
	bool isOutTopEdge = (nCtu * nCtuPosHeight) < (nCimeSwHeight / 2);
	bool isOutLeftEdge = (nCtu * nCtuPosWidth) < (nCimeSwWidth / 2);
	bool isOutBottomEdge = (nCtu * nCtuPosHeight + (nCimeSwHeight / 2 - 1) + nCtu) > nPicHeight;
	bool isOutRightEdge = (nCtu * nCtuPosWidth + (nCimeSwWidth / 2 - 1) + nCtu) > nPicWidth;
	//1. down sampled search window is out of the top left corner
	if (isOutTopEdge && isOutLeftEdge)
	{
		int offset_x = (nCimeSwWidth / 2 - nCtu * nCtuPosWidth);
		int offset_y = (nCimeSwHeight / 2 - nCtu * nCtuPosHeight);
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
		int offset_x = (nCimeSwWidth/2 + nCtu) - (nPicWidth - nCtuPosWidth*nCtu);
		int offset_y = (nCimeSwHeight / 2 - nCtu*nCtuPosHeight);
		short *pCurrCSW = m_pCimeSW + offset_y*m_nCimeSwWidth;
		short *pCurrRefPic = pRefPic + (nCtuPosWidth*nCtu) + (nCtuPosHeight*nCtu)*nPicWidth;
		pCurrRefPic -= (nCimeSwWidth / 2 + (nCtu*nCtuPosHeight)*nPicWidth);
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
		int offset_x = (nCimeSwWidth / 2 - nCtu * nCtuPosWidth);
		int offset_y = (nCimeSwHeight / 2 + nCtu) - (nPicHeight - nCtu*nCtuPosHeight);
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
		int offset_x = (nCimeSwWidth / 2 + nCtu) - (nPicWidth - nCtu*nCtuPosWidth);
		int offset_y = (nCimeSwHeight / 2 + nCtu) - (nPicHeight - nCtu*nCtuPosHeight);
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
		int offset_x = nCtuPosWidth*nCtu - nCimeSwWidth / 2;
		int offset_y = nCimeSwHeight / 2 - nCtu*nCtuPosHeight;
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
		int offset_x = (nCimeSwWidth / 2 - nCtu * nCtuPosWidth);
		int offset_y = nCtuPosHeight*nCtu - nCimeSwHeight / 2;
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
		int offset_x = nCtuPosWidth*nCtu - nCimeSwWidth / 2;
		int offset_y = (nCimeSwHeight / 2 + nCtu) - (nPicHeight - nCtu*nCtuPosHeight);
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
		int offset_x = (nCimeSwWidth / 2 + nCtu) - (nPicWidth - nCtu*nCtuPosWidth);
		int offset_y = nCtuPosHeight*nCtu - nCimeSwHeight / 2;
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
		short *pCurrRefPic = pRefPic + nCtuPosWidth*nCtu + (nCtuPosHeight*nCtu)*nPicWidth;
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

void CoarseIntMotionEstimation::setDownSamplePic(unsigned char *pRefPic, int stride, int idx)
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
			m_pRefPicDS[idx][j / m_nDownSampDist + i / m_nDownSampDist*m_nPicWidth / m_nDownSampDist] = sum;
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

void CoarseIntMotionEstimation::CIME(unsigned char *pCurrPic, short *pRefPic, int nRefPicIdx)
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
		
	int bcost = INT_MAX_MINE;
	Mv mvp[3];
	setMvp(mvp, nRefPicIdx);
	for (tmv.y = mvmin.y; tmv.y <= mvmax.y; tmv.y++)
	{
		for (tmv.x = mvmin.x; tmv.x <= mvmax.x; tmv.x++)
		{
			int tmpCost = INT_MAX_MINE;
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
	m_mvCimeOut[nRefPicIdx].x = bmv.x; m_mvCimeOut[nRefPicIdx].y = bmv.y; m_mvCimeOut[nRefPicIdx].nSadCost = bcost;
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
	unsigned short *pMvCost = m_pMvCost[(int)m_nQP] + MV_MAX;
	return  (pMvCost[mv.x - mvp.x] + pMvCost[mv.y - mvp.y]);
}

void CoarseIntMotionEstimation::setMvp(Mv *mvp, int nRefPicIdx)
{
	const int nMvNeigh = nAllNeighMv;
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
	}
	if (!isValidTop)
	{
		isValid[24] = false; isValid[17] = false;
		isValid[23] = false; isValid[5] = false; isValid[25] = false;
		isValid[14] = false; isValid[20] = false; isValid[2] = false;
	}

	int idxFirst = INT_MAX_MINE;
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
			if (abs(m_mvNeighMv[nRefPicIdx][idx].x - m_mvNeighMv[nRefPicIdx][idy].x) <= nRectSize && abs(m_mvNeighMv[nRefPicIdx][idx].y - m_mvNeighMv[nRefPicIdx][idy].y) <= nRectSize)
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
			mvp[tmpCand].x = m_mvNeighMv[nRefPicIdx][i].x;
			mvp[tmpCand++].y = m_mvNeighMv[nRefPicIdx][i].y;
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

unsigned char RefineIntMotionEstimation::m_pCurrCtuRefPic[nMaxRefPic][3][(nCtuSize + 2 * (nRimeWidth + 4))*(nCtuSize + 2 * (nRimeHeight + 4))*3/2];

class RefineIntMotionEstimation Rime;

RefineIntMotionEstimation::RefineIntMotionEstimation()
{

}

RefineIntMotionEstimation::~RefineIntMotionEstimation()
{

}

void RefineIntMotionEstimation::setPmv(Mv pMvNeigh[nAllNeighMv], Mv mvCpmv, int nRefPicIdx)
{
	for (int i = 0; i < nAllNeighMv; i++)
	{
		m_mvNeigh[nRefPicIdx][i].x = pMvNeigh[i].x;
		m_mvNeigh[nRefPicIdx][i].y = pMvNeigh[i].y;
	}

	//get coarse search PMV
	m_mvRefine[nRefPicIdx][0].x = mvCpmv.x;
	m_mvRefine[nRefPicIdx][0].y = mvCpmv.y;

	/*get 2 neighbor PMV  begin*/
	const int nMvNeigh = nAllNeighMv;
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
	}
	if (!isValidTop)
	{
        isValid[24] = false; isValid[17] = false;
		isValid[23] = false; isValid[5] = false; isValid[25] = false;
		isValid[14] = false; isValid[20] = false; isValid[2] = false;
	}

	int idxFirst = INT_MAX_MINE;
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
			m_mvRefine[nRefPicIdx][tmpCand + 1].x = pMvNeigh[i].x;
			m_mvRefine[nRefPicIdx][tmpCand + 1].y = pMvNeigh[i].y;
			tmpCand++;
		}
	}
	if (tmpCand < nNeightMv)
	{
		for (int i = tmpCand+1; i < nNeightMv+1; i++) //if not full, the remaining set the same as first neighbor mv
		{
			m_mvRefine[nRefPicIdx][i].x = m_mvRefine[nRefPicIdx][1].x;
			m_mvRefine[nRefPicIdx][i].y = m_mvRefine[nRefPicIdx][1].y;
		}
	}
	/*get 2 neighbor PMV  end*/
	for (int i = 0; i < 3; i ++) //fetch even MV, for chroma search windows
	{
		m_mvRefine[nRefPicIdx][i].x = m_mvRefine[nRefPicIdx][i].x / 2 * 2;
		m_mvRefine[nRefPicIdx][i].y = m_mvRefine[nRefPicIdx][i].y / 2 * 2;
	}
	
}

void RefineIntMotionEstimation::setMvp(Mv *mvp, Mv pMvNeigh[nAllNeighMv])
{
	const int nMvNeigh = nAllNeighMv;
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
	}
	if (!isValidTop)
	{
		isValid[24] = false; isValid[17] = false;
		isValid[23] = false; isValid[5] = false; isValid[25] = false;
		isValid[14] = false; isValid[20] = false; isValid[2] = false;
	}

	int idxFirst = INT_MAX_MINE;
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
	unsigned short *pMvCost = m_pMvCost[(int)m_nQP] + MV_MAX;
	return  (pMvCost[mv.x - mvp.x] + pMvCost[mv.y - mvp.y]);
}

void RefineIntMotionEstimation::CreatePicInfo()
{
	m_pOrigPic = new unsigned char[(m_nPicWidth * 3 / 2)*(m_nPicHeight * 3 / 2)];
	assert(RK_NULL != m_pOrigPic);
	for (int nRefPicIdx = 0; nRefPicIdx < nMaxRefPic; nRefPicIdx ++)
	{
		m_pRefPic[nRefPicIdx] = new unsigned char[(m_nPicWidth * 3 / 2)*(m_nPicHeight * 3 / 2)];
		assert(RK_NULL != m_pRefPic[nRefPicIdx]);
	}
}

void RefineIntMotionEstimation::DestroyPicInfo()
{
	if (RK_NULL != m_pOrigPic)
	{
		delete[] m_pOrigPic;
		m_pOrigPic = RK_NULL;
	}

	for (int nRefPicIdx = 0; nRefPicIdx < nMaxRefPic; nRefPicIdx++)
	{
		if (RK_NULL != m_pRefPic[nRefPicIdx])
		{
			delete[] m_pRefPic[nRefPicIdx];
			m_pRefPic[nRefPicIdx] = RK_NULL;
		}
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
	for (int nRefPicIdx = 0; nRefPicIdx < nMaxRefPic; nRefPicIdx ++)
	{
		m_pCuForFmeInterp[nRefPicIdx] = new unsigned char[(m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2) * 3 / 2];
		assert(RK_NULL != m_pCuForFmeInterp);
	}
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

	for (int nRefPicIdx = 0; nRefPicIdx < nMaxRefPic; nRefPicIdx++)
	{
		if (RK_NULL != m_pCuForFmeInterp[nRefPicIdx])
		{
			delete[] m_pCuForFmeInterp[nRefPicIdx];
			m_pCuForFmeInterp[nRefPicIdx] = RK_NULL;
		}
	}

}

void RefineIntMotionEstimation::setOrigPic(unsigned char *pOrigPicY, int stride_Y, unsigned char *pOrigPicU, int stride_U, unsigned char *pOrigPicV, int stride_V)
{//store YUV pixels as Y,Y,Y,Y,...,UV,UV...
	for (int i = 0; i < m_nPicHeight; i++)
	{
		for (int j = 0; j < m_nPicWidth; j++)
		{
			m_pOrigPic[j + i*m_nPicWidth] = pOrigPicY[j + i*stride_Y];
		}
	}
	unsigned char *pOrigPic = m_pOrigPic + m_nPicWidth*m_nPicHeight;
	for (int i = 0; i < m_nPicHeight / 2; i++)
	{
		for (int j = 0; j < m_nPicWidth / 2; j++)
		{
			pOrigPic[(j * 2 + 0) + i*m_nPicWidth] = pOrigPicU[j + i*stride_U];
			pOrigPic[(j * 2 + 1) + i*m_nPicWidth] = pOrigPicV[j + i*stride_V];
		}
	}

}

void RefineIntMotionEstimation::setRefPic(unsigned char *pRefPicY, int stride_Y, 	unsigned char *pRefPicU, int stride_U, 
	unsigned char *pRefPicV, int stride_V, int nRefPicIdx)
{//store YUV pixels as Y,Y,Y,Y,...,UV,UV...
	for (int i = 0; i < m_nPicHeight; i++)
	{
		for (int j = 0; j < m_nPicWidth; j++)
		{
			m_pRefPic[nRefPicIdx][j + i*m_nPicWidth] = pRefPicY[j + i*stride_Y];
		}
	}
	unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + m_nPicWidth*m_nPicHeight;
	for (int i = 0; i < m_nPicHeight / 2; i ++)
	{
		for (int j = 0; j < m_nPicWidth / 2; j ++)
		{
			pRefPic[(j * 2 + 0) + i*m_nPicWidth] = pRefPicU[j + i*stride_U];
			pRefPic[(j * 2 + 1) + i*m_nPicWidth] = pRefPicV[j + i*stride_V];
		}
	}
}

void RefineIntMotionEstimation::PrefetchCtuLumaInfo(int nRefPicIdx)
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
		int pmv_x = m_nCtuSize*nCtuPosWidth + offs_x + m_mvRefine[nRefPicIdx][pmvIdx].x - nRimeWidth - 4; //top left of refine search window
		int pmv_y = m_nCtuSize*nCtuPosHeight + offs_y + m_mvRefine[nRefPicIdx][pmvIdx].y - nRimeHeight - 4;
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset_x + offset_y*nRSWidth;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = m_pRefPic[nRefPicIdx][j + i*m_nPicWidth];
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < offset_x; j++)
					{
						pRimeSW[j + i*nRSWidth] = m_pRefPic[nRefPicIdx][0];
					}
				}
				pRimeSW += offset_x;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j + i*nRSWidth] = m_pRefPic[nRefPicIdx][j];
					}
				}
			}
			else if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
				for (int i = 0; i < offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = m_pRefPic[nRefPicIdx][0];
					}
				}
				pRimeSW += offset_y*nRSWidth;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j + i*nRSWidth] = m_pRefPic[nRefPicIdx][i*m_nPicWidth];
					}
				}
			}
			else
			{
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						m_pCurrCtuRefPic[nRefPicIdx][pmvIdx][j + i*nRSWidth] = m_pRefPic[nRefPicIdx][0];
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicWidth - width);
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset_y*nRSWidth;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicWidth - width);
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicWidth - 1);
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicWidth - 1);
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicHeight - height)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset_x;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicHeight - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicHeight - height)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicHeight - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicHeight*m_nPicWidth - 1) - (width - 1) - (height - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
				pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + (height - 1)*nRSWidth;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + m_nPicWidth*m_nPicHeight - 1; //bottom right
				pRefPic -= (width - 1);
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + m_nPicWidth*m_nPicHeight - 1; //bottom right
				pRefPic -= (height - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + m_nPicWidth*m_nPicHeight - 1; //bottom right
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
			unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + pmv_x;
			if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
			unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + pmv_y*m_nPicWidth;
			if (width)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
			unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicHeight - 1)*m_nPicWidth + pmv_x;
			if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
			unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicWidth - 1) + pmv_y*m_nPicWidth;
			if (width)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + width - 1;
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx];
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
			unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + pmv_x + pmv_y*m_nPicWidth;
			for (int i = 0; i < nRSHeight; i++)
			{
				for (int j = 0; j < nRSWidth; j++)
				{
					m_pCurrCtuRefPic[nRefPicIdx][pmvIdx][j + i*nRSWidth] = pRefPic[j + i*m_nPicWidth];
				}
			}
		}
	}
}

void RefineIntMotionEstimation::PrefetchCtuChromaInfo(int nRefPicIdx)
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

	int nCtu = nCtuSize;
	int offset = (nCtu + 2 * (nRimeWidth + 4))*(nCtu + 2 * (nRimeHeight + 4));
	//fetch 3 PMV search window
	for (int pmvIdx = 0; pmvIdx < 3; pmvIdx++)
	{//m_mvRefine is always even
		int nRSWidth = m_nCtuSize + 2 * (nRimeWidth + 4);
		int nRSHeight = m_nCtuSize + 2 * (nRimeHeight + 4);
		int pmv_x = m_nCtuSize*nCtuPosWidth + m_mvRefine[nRefPicIdx][pmvIdx].x - nRimeWidth - 4; //top left of refine search window
		int pmv_y = m_nCtuSize*nCtuPosHeight + m_mvRefine[nRefPicIdx][pmvIdx].y - nRimeHeight - 4;
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset_x * 2 + offset_y*nRSWidth * 2 + offset;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][j * 2 + 0 + i*m_nPicWidth + m_nPicWidth*m_nPicHeight];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][j * 2 + 1 + i*m_nPicWidth + m_nPicWidth*m_nPicHeight];
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < offset_x; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][0 + m_nPicWidth*m_nPicHeight];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][1 + m_nPicWidth*m_nPicHeight];
					}
				}
				pRimeSW += offset_x * 2;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < width; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][j * 2 + 0 + m_nPicWidth*m_nPicHeight];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][j * 2 + 1 + m_nPicWidth*m_nPicHeight];
					}
				}
			}
			else if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
				for (int i = 0; i < offset_y; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][0 + m_nPicWidth*m_nPicHeight];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][1 + m_nPicWidth*m_nPicHeight];
					}
				}
				pRimeSW += offset_y*nRSWidth * 2;
				for (int i = 0; i < height; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][0 + i*m_nPicWidth + m_nPicWidth*m_nPicHeight];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][1 + i*m_nPicWidth + m_nPicWidth*m_nPicHeight];
					}
				}
			}
			else
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
				for (int i = 0; i < nRSHeight; i++)
				{
					for (int j = 0; j < nRSWidth; j++)
					{
						pRimeSW[j * 2 + 0 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][0 + m_nPicWidth*m_nPicHeight];
						pRimeSW[j * 2 + 1 + i*nRSWidth * 2] = m_pRefPic[nRefPicIdx][1 + m_nPicWidth*m_nPicHeight];
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicWidth - width * 2) + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset_y*nRSWidth * 2 + offset;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicWidth - width * 2) + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicWidth - 2) + m_nPicWidth*m_nPicHeight;
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicWidth - 2) + m_nPicWidth*m_nPicHeight;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicHeight / 2 - height)*m_nPicWidth + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset_x * 2 + offset;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicHeight / 2 - 1)*m_nPicWidth + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicHeight / 2 - height)*m_nPicWidth + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicHeight / 2 - 1)*m_nPicWidth + m_nPicWidth*m_nPicHeight;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + m_nPicHeight*m_nPicWidth;
				pRefPic += (m_nPicHeight / 2 * m_nPicWidth - 2) - (width - 1) * 2 - (height - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
				pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + (height - 1)*nRSWidth * 2 + offset;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + m_nPicHeight*m_nPicWidth + m_nPicWidth*m_nPicHeight / 2 - 2; //bottom right
				pRefPic -= (width - 1) * 2;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + m_nPicHeight*m_nPicWidth + m_nPicWidth*m_nPicHeight / 2 - 2; //bottom right
				pRefPic -= (height - 1)*m_nPicWidth;
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
				unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + m_nPicHeight*m_nPicWidth + m_nPicWidth*m_nPicHeight / 2 - 2; //bottom right
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
			unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + m_nPicHeight*m_nPicWidth + pmv_x * 2;
			if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
			unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + pmv_y*m_nPicWidth + m_nPicHeight*m_nPicWidth;
			if (width)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
			unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicHeight / 2 - 1)*m_nPicWidth + pmv_x * 2 + m_nPicHeight*m_nPicWidth;
			if (height)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
			unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + (m_nPicWidth - 2) + pmv_y*m_nPicWidth + m_nPicHeight*m_nPicWidth;
			if (width)
			{
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + (width - 1) * 2 + offset;
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
				unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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
			unsigned char *pRefPic = m_pRefPic[nRefPicIdx] + pmv_x * 2 + pmv_y*m_nPicWidth + m_nPicHeight*m_nPicWidth;
			unsigned char *pRimeSW = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset;
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

bool RefineIntMotionEstimation::PrefetchCuInfo(int nRefPicIdx)
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
		unsigned char *pCurrCuRefPic = m_pCurrCtuRefPic[nRefPicIdx][pmvIdx] + offset_x + offset_y*(nCtuRefPicWidth);
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

void RefineIntMotionEstimation::RimeAndFme(int nRefPicIdx)
{
	static bool isFirst = true;
	if (isFirst)
	{
		setMvpCost();
		isFirst = false;
	}
	int bcost = INT_MAX_MINE;
	Mv bmv; bmv.x = 0; bmv.y = 0;
	Mv mvp[3];
	setMvp(mvp, m_mvNeigh[nRefPicIdx]);
	int pmvBestIdx = 0;
	int pmvBestCost = INT_MAX_MINE;
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
					Mv Tmv; Tmv.x = (tmv.x + m_mvRefine[nRefPicIdx][idxPmv].x); Tmv.y = (tmv.y + m_mvRefine[nRefPicIdx][idxPmv].y);
					if (tmpCost > mvcost(mvp[idx], Tmv))
						tmpCost = mvcost(mvp[idx], Tmv);
				}
				int idx = tmv.x + tmv.y*m_nRimeSwWidth;
				int cost = SAD(pCuRimeSW + idx, m_nRimeSwWidth, m_pCurrCuPel, m_nCuSize, m_nCuSize)+static_cast<int>(tmpCost);
				COPY_IF_LT(bcost, cost, bmv.x, (tmv.x + m_mvRefine[nRefPicIdx][idxPmv].x), bmv.y, (tmv.y + m_mvRefine[nRefPicIdx][idxPmv].y));
			}
		}
		if (bcost < pmvBestCost)
		{
			pmvBestCost = bcost;
			pmvBestIdx = idxPmv;
		}
	}
	m_mvRimeOut[nRefPicIdx].x = bmv.x; m_mvRimeOut[nRefPicIdx].y = bmv.y; m_mvRimeOut[nRefPicIdx].nSadCost = bcost;

	//FME calculate procedure
	bcost = INT_MAX_MINE;
	Mv tmv; tmv.x = bmv.x - m_mvRefine[nRefPicIdx][pmvBestIdx].x; tmv.y = bmv.y - m_mvRefine[nRefPicIdx][pmvBestIdx].y;

	unsigned char *pCuRimeSW = m_pCuRimeSW[pmvBestIdx] + (4 + nRimeWidth) + (4 + nRimeHeight)*m_nRimeSwWidth;
	for (short i = -3; i <= 3; i++)
	{
		for (short j = -3; j <= 3; j++)
		{
			Mv qmv; qmv.x = tmv.x*4 + i; qmv.y = tmv.y*4 + j;
			unsigned char *fref = pCuRimeSW + (qmv.x >> 2) - 4 + ((qmv.y >> 2)-4)* m_nRimeSwWidth;
			unsigned short tmpCost = USHORT_MAX;
			for (int idx = 0; idx < 3; idx++)
			{
				Mv Tmv; 
				Tmv.x = qmv.x + m_mvRefine[nRefPicIdx][pmvBestIdx].x * 4;
				Tmv.y = qmv.y + m_mvRefine[nRefPicIdx][pmvBestIdx].y * 4;
				if (tmpCost > mvcost(mvp[idx], Tmv))
					tmpCost = mvcost(mvp[idx], Tmv);
			}
			int cost = subpelCompare(fref, m_nRimeSwWidth, m_pCurrCuPel, m_nCuSize, qmv);
			cost += tmpCost;
			COPY_IF_LT(bcost, cost, bmv.x, qmv.x, bmv.y, qmv.y);
		}
	}
	m_mvFmeOut[nRefPicIdx].x = bmv.x + m_mvRefine[nRefPicIdx][pmvBestIdx].x * 4; 
	m_mvFmeOut[nRefPicIdx].y = bmv.y + m_mvRefine[nRefPicIdx][pmvBestIdx].y * 4; 
	m_mvFmeOut[nRefPicIdx].nSadCost = bcost;
	
	int offset_x = OffsFromCtu64[m_nCuPosInCtu][0];
	int offset_y = OffsFromCtu64[m_nCuPosInCtu][1];
	if (32 == m_nCtuSize)
	{
		offset_x = OffsFromCtu32[m_nCuPosInCtu][0];
		offset_y = OffsFromCtu32[m_nCuPosInCtu][1];
	}
	int nCtuRefPicWidth = m_nCtuSize + 2 * (nRimeWidth + 4);
	unsigned char *fref = m_pCurrCtuRefPic[nRefPicIdx][pmvBestIdx] + offset_x + offset_y*(nCtuRefPicWidth)+
		((bmv.x >> 2) + nRimeWidth) + (nRimeHeight + (bmv.y >> 2))* nCtuRefPicWidth;
	
	for (int i = 0; i < m_nCuSize + 4 * 2; i ++)
	{
		for (int j = 0; j < m_nCuSize + 4 * 2; j ++)
		{
			m_pCuForFmeInterp[nRefPicIdx][j + i*(m_nCuSize + 8)] = fref[j + i*nCtuRefPicWidth];
		}
	}
	int nCtu = nCtuSize;
	unsigned char *pCuForFmeInterp = m_pCuForFmeInterp[nRefPicIdx] + (m_nCuSize + 8)*(m_nCuSize + 8);
	fref = m_pCurrCtuRefPic[nRefPicIdx][pmvBestIdx] + (nCtu + 2 * (nRimeWidth + 4))*(nCtu + 2 * (nRimeHeight + 4));
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

unsigned short FractionMotionEstimation::m_pMvCost[QP_MAX][MV_MAX * 2];
float FractionMotionEstimation::m_dBitsizes[MV_MAX];

FractionMotionEstimation::FractionMotionEstimation()
{
	m_nCtuSize = 64;
}

FractionMotionEstimation::~FractionMotionEstimation()
{

}

void FractionMotionEstimation::Create()
{
	m_pCurrCuPel = new unsigned char[m_nCuSize*m_nCuSize*3/2];
	assert(RK_NULL != m_pCurrCuPel);

	m_pBestPred = new unsigned char[m_nCuSize*m_nCuSize * 3 / 2];
	assert(RK_NULL != m_pBestPred);

	m_pBestResi = new short[m_nCuSize*m_nCuSize * 3 / 2];
	assert(RK_NULL != m_pBestResi);

	for (int nRefPicIdx = 0; nRefPicIdx < nMaxRefPic; nRefPicIdx ++)
	{
		m_pCurrCuPred[nRefPicIdx] = new unsigned char[m_nCuSize*m_nCuSize * 3 / 2];
		assert(RK_NULL != m_pCurrCuPred[nRefPicIdx]);

		m_pCurrCuResi[nRefPicIdx] = new short[m_nCuSize*m_nCuSize * 3 / 2];
		assert(RK_NULL != m_pCurrCuResi[nRefPicIdx]);

		m_pFmeInterpPel[nRefPicIdx] = new unsigned char[(m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2) * 3 / 2];
		assert(RK_NULL != m_pFmeInterpPel[nRefPicIdx]);
	}
}

void FractionMotionEstimation::Destroy()
{
	if (RK_NULL != m_pCurrCuPel)
	{
		delete[] m_pCurrCuPel;
		m_pCurrCuPel = RK_NULL;
	}

	if (RK_NULL != m_pBestPred)
	{
		delete[] m_pBestPred;
		m_pBestPred = RK_NULL;
	}

	if (RK_NULL != m_pBestResi)
	{
		delete[] m_pBestResi;
		m_pBestResi = RK_NULL;
	}

	for (int nRefPicIdx = 0; nRefPicIdx < nMaxRefPic; nRefPicIdx ++)
	{
		if (RK_NULL != m_pCurrCuPred[nRefPicIdx])
		{
			delete[] m_pCurrCuPred[nRefPicIdx];
			m_pCurrCuPred[nRefPicIdx] = RK_NULL;
		}

		if (RK_NULL != m_pCurrCuResi[nRefPicIdx])
		{
			delete[] m_pCurrCuResi[nRefPicIdx];
			m_pCurrCuResi[nRefPicIdx] = RK_NULL;
		}

		if (RK_NULL != m_pFmeInterpPel[nRefPicIdx])
		{
			delete[] m_pFmeInterpPel[nRefPicIdx];
			m_pFmeInterpPel[nRefPicIdx] = RK_NULL;
		}
	}	

}

unsigned short FractionMotionEstimation::mvcost(Mv mvp, Mv mv)
{
	unsigned short *pMvCost = m_pMvCost[(int)m_nQP] + MV_MAX;
	return  (pMvCost[mv.x - mvp.x] + pMvCost[mv.y - mvp.y]);
}

void FractionMotionEstimation::setMvpCost()
{
	static bool isFirst = true;
	if (isFirst) isFirst = false;
	else return;
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

void FractionMotionEstimation::CalculateLogs()
{
	static bool isFirst = true;
	if (isFirst) isFirst = false;
	else return;
	
	m_dBitsizes[0] = 0.718f;
	float log2_2 = 2.0f / log(2.0f);  // 2 x 1/log(2)
	for (int i = 1; i < MV_MAX; i++)
	{
		m_dBitsizes[i] = log((float)(i + 1)) * log2_2 + 1.718f;
	}
}

unsigned short FractionMotionEstimation::bitcost(Mv mvp, Mv mv)
{
	return (unsigned short)(m_dBitsizes[(abs(mv.x - mvp.x) << 1) + !!(mv.x < mvp.x)] +
		m_dBitsizes[(abs(mv.y - mvp.y) << 1) + !!(mv.y < mvp.y)] + 0.5f);
}

void FractionMotionEstimation::setFmeInterpPel(unsigned char *src, int src_stride, int nRefPicIdx)
{
	int nSize = m_nCuSize + 2 * 4;
	for (int i = 0; i < nSize; i ++)
	{
		for (int j = 0; j < nSize; j ++)
		{
			m_pFmeInterpPel[nRefPicIdx][j + i*nSize] = src[j + i*src_stride];
		}
	}
	unsigned char *pSrc = src + nSize*nSize;
	unsigned char *pFmeInterpPel = m_pFmeInterpPel[nRefPicIdx] + nSize*nSize;
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
	setMvpCost();
	CalculateLogs();
	int nBestPrevIdx = INT_MAX_MINE;
	int nBestPostIdx = INT_MAX_MINE;
	int nBestPrevCost = INT_MAX_MINE;
	int nBestPostCost = INT_MAX_MINE;
	int nBestPrevBits = INT_MAX_MINE;
	int nBestPostBits = INT_MAX_MINE;
	if (b_slice == m_nSliceType)
	{
		for (int nRefPicIdx = 0; nRefPicIdx < m_nRefPic[REF_PIC_LIST0]; nRefPicIdx++)
		{
			int bitsTemp = 3;
			if (m_nRefPic[REF_PIC_LIST0] > 1) //number of forward reference pictures is more than 1
			{
				bitsTemp += nRefPicIdx + 1;
				if (nRefPicIdx == m_nRefPic[REF_PIC_LIST0] - 1) bitsTemp--;
			}
			xCalcResiAndMvd(nRefPicIdx, false, false, true);
			bitsTemp++;
			bitsTemp += (int)bitcost(m_mvAmvp[nRefPicIdx][m_nMvpIdx[nRefPicIdx]], m_mvFmeInput[nRefPicIdx]);
			int costTemp = m_mvFmeInput[nRefPicIdx].nSadCost + getCost(bitsTemp);
			if (costTemp < nBestPrevCost)
			{
				nBestPrevIdx = nRefPicIdx;
				nBestPrevCost = costTemp;
				nBestPrevBits = bitsTemp;
			}
		}
		xCalcResiAndMvd(nBestPrevIdx, true, false, false);

		for (int idx = 0; idx < m_nRefPic[REF_PIC_LIST1]; idx++)
		{
			int nRefPicIdx = nMaxRefPic / 2 + idx;
			int bitsTemp = 3;
			if (m_nRefPic[REF_PIC_LIST1] > 1) //number of forward reference pictures is more than 1
			{
				bitsTemp += idx + 1;
				if (idx == m_nRefPic[REF_PIC_LIST1] - 1) bitsTemp--;
			}
			xCalcResiAndMvd(nRefPicIdx, false, false, true);
			bitsTemp++;
			bitsTemp += (int)bitcost(m_mvAmvp[nRefPicIdx][m_nMvpIdx[nRefPicIdx]], m_mvFmeInput[nRefPicIdx]);
			int costTemp = m_mvFmeInput[nRefPicIdx].nSadCost + getCost(bitsTemp);
			if (costTemp < nBestPostCost)
			{
				nBestPostIdx = nRefPicIdx;
				nBestPostCost = costTemp;
				nBestPostBits = bitsTemp;
			}
		}
		xCalcResiAndMvd(nBestPostIdx, true, false, false);

		//processing prev and post 
		unsigned char *pAvg = new unsigned char[m_nCuSize*m_nCuSize];
		Avg(pAvg, m_pCurrCuPred[nBestPrevIdx], m_pCurrCuPred[nBestPostIdx]);
		int nBestAvgCost = satd_8x8(m_pCurrCuPel, pAvg);
		int nBestAvgBits = nBestPrevBits + nBestPostBits - 3 - 3 + 5;
		nBestAvgCost += getCost(nBestAvgBits);
		delete[] pAvg;

		if (nBestAvgCost <= nBestPrevCost&&nBestAvgCost <= nBestPostCost)
		{
			PredInterBi(nBestPrevIdx);
			PredInterBi(nBestPostIdx);
			addAvg(nBestPrevIdx, nBestPostIdx, true, true);
			m_fmeInfoForCabac.m_interDir = 3;
			m_fmeInfoForCabac.m_mvdx[REF_PIC_LIST0] = m_mvFmeInput[nBestPrevIdx].x;
			m_fmeInfoForCabac.m_mvdy[REF_PIC_LIST0] = m_mvFmeInput[nBestPrevIdx].y;
			m_fmeInfoForCabac.m_mvdx[REF_PIC_LIST1] = m_mvFmeInput[nBestPostIdx].x;
			m_fmeInfoForCabac.m_mvdy[REF_PIC_LIST1] = m_mvFmeInput[nBestPostIdx].y;
			m_fmeInfoForCabac.m_refIdx[REF_PIC_LIST0] = nBestPrevIdx;
			m_fmeInfoForCabac.m_refIdx[REF_PIC_LIST1] = nBestPostIdx - nMaxRefPic / 2;
			m_fmeInfoForCabac.m_mvpIdx[REF_PIC_LIST0] = m_nMvpIdx[REF_PIC_LIST0];
			m_fmeInfoForCabac.m_mvpIdx[REF_PIC_LIST1] = m_nMvpIdx[REF_PIC_LIST1];
		}
		else if (nBestPrevCost <= nBestPostCost)
		{
			xCalcResiAndMvd(nBestPrevIdx, true, true, false);
			CopyToBest(m_pCurrCuPred[nBestPrevIdx]);
			m_fmeInfoForCabac.m_interDir = 1;
			m_fmeInfoForCabac.m_mvdx[REF_PIC_LIST0] = m_mvFmeInput[nBestPrevIdx].x;
			m_fmeInfoForCabac.m_mvdy[REF_PIC_LIST0] = m_mvFmeInput[nBestPrevIdx].y;
			m_fmeInfoForCabac.m_refIdx[REF_PIC_LIST0] = nBestPrevIdx;
			m_fmeInfoForCabac.m_refIdx[REF_PIC_LIST0] = -1;
			m_fmeInfoForCabac.m_mvpIdx[REF_PIC_LIST0] = m_nMvpIdx[REF_PIC_LIST0];
		}
		else
		{
			xCalcResiAndMvd(nBestPostIdx, true, true, false);
			CopyToBest(m_pCurrCuPred[nBestPostIdx]);
			m_fmeInfoForCabac.m_interDir = 2;
			m_fmeInfoForCabac.m_mvdx[REF_PIC_LIST1] = m_mvFmeInput[nBestPostIdx].x;
			m_fmeInfoForCabac.m_mvdy[REF_PIC_LIST1] = m_mvFmeInput[nBestPostIdx].y;
			m_fmeInfoForCabac.m_refIdx[REF_PIC_LIST0] = -1;
			m_fmeInfoForCabac.m_refIdx[REF_PIC_LIST1] = nBestPostIdx - nMaxRefPic / 2;
			m_fmeInfoForCabac.m_mvpIdx[REF_PIC_LIST1] = m_nMvpIdx[REF_PIC_LIST1];
		}
	}
	else
	{
		for (int nRefPicIdx = 0; nRefPicIdx < m_nRefPic[REF_PIC_LIST0]; nRefPicIdx++)
		{
			int bitsTemp = 1;
			if (m_nRefPic[REF_PIC_LIST0] > 1) //number of forward reference pictures is more than 1
			{
				bitsTemp += nRefPicIdx + 1;
				if (nRefPicIdx == m_nRefPic[REF_PIC_LIST0] - 1) bitsTemp--;
			}
			xCalcResiAndMvd(nRefPicIdx, false, false, true);
			bitsTemp++;
			bitsTemp += (int)bitcost(m_mvAmvp[nRefPicIdx][m_nMvpIdx[nRefPicIdx]], m_mvFmeInput[nRefPicIdx]);
			int costTemp = m_mvFmeInput[nRefPicIdx].nSadCost + getCost(bitsTemp);
			if (costTemp < nBestPrevCost)
			{
				nBestPrevIdx = nRefPicIdx;
				nBestPrevCost = costTemp;
			}
		}
		xCalcResiAndMvd(nBestPrevIdx, true, true, false);
		CopyToBest(m_pCurrCuPred[nBestPrevIdx]);
		m_fmeInfoForCabac.m_interDir = 1;
		m_fmeInfoForCabac.m_mvdx[REF_PIC_LIST0] = m_mvFmeInput[nBestPrevIdx].x;
		m_fmeInfoForCabac.m_mvdy[REF_PIC_LIST0] = m_mvFmeInput[nBestPrevIdx].y;
		m_fmeInfoForCabac.m_refIdx[REF_PIC_LIST0] = nBestPrevIdx;
		m_fmeInfoForCabac.m_refIdx[REF_PIC_LIST0] = -1;
		m_fmeInfoForCabac.m_mvpIdx[REF_PIC_LIST0] = m_nMvpIdx[REF_PIC_LIST0];
	}

}

void FractionMotionEstimation::addAvg(int nBestPrevIdx, int nBestPostIdx, bool bLuma, bool bChroma)
{
	int x, y;
	unsigned int src0Stride, src1Stride, dststride;
	int shiftNum, offset;

	short* SrcY0 = new short[m_nCuSize*m_nCuSize]; short *srcY0 = SrcY0;
	short* SrcU0 = new short[m_nCuSize*m_nCuSize / 4]; short* srcU0 = SrcU0;
	short* SrcV0 = new short[m_nCuSize*m_nCuSize / 4]; short* srcV0 = SrcV0;

	short* SrcY1 = new short[m_nCuSize*m_nCuSize]; short *srcY1 = SrcY1;
	short* SrcU1 = new short[m_nCuSize*m_nCuSize / 4]; short *srcU1 = SrcU1;
	short* SrcV1 = new short[m_nCuSize*m_nCuSize / 4]; short *srcV1 = SrcV1;

	unsigned char* DstY = new unsigned char[m_nCuSize*m_nCuSize]; unsigned char *dstY = DstY;
	unsigned char* DstU = new unsigned char[m_nCuSize*m_nCuSize / 4]; unsigned char *dstU = DstU;
	unsigned char* DstV = new unsigned char[m_nCuSize*m_nCuSize / 4]; unsigned char *dstV = DstV;

	for (int i = 0; i < m_nCuSize; i++)
	{
		for (int j = 0; j < m_nCuSize; j++)
		{
			srcY0[j + i*m_nCuSize] = m_pCurrCuResi[nBestPrevIdx][j + i*m_nCuSize];
			srcY1[j + i*m_nCuSize] = m_pCurrCuResi[nBestPostIdx][j + i*m_nCuSize];
		}
	}
	for (int i = 0; i < m_nCuSize / 2; i ++)
	{
		for (int j = 0; j < m_nCuSize / 2; j ++)
		{
			srcU0[j + i*m_nCuSize / 2] = m_pCurrCuResi[nBestPrevIdx][j + i*m_nCuSize/2 + m_nCuSize*m_nCuSize];
			srcU1[j + i*m_nCuSize / 2] = m_pCurrCuResi[nBestPostIdx][j + i*m_nCuSize/2 + m_nCuSize*m_nCuSize];
			srcV0[j + i*m_nCuSize / 2] = m_pCurrCuResi[nBestPrevIdx][j + i*m_nCuSize/2 + m_nCuSize*m_nCuSize * 5 / 4];
			srcV1[j + i*m_nCuSize / 2] = m_pCurrCuResi[nBestPostIdx][j + i*m_nCuSize/2 + m_nCuSize*m_nCuSize * 5 / 4];
		}
	}

	if (bLuma)
	{
		src0Stride = src1Stride = dststride = m_nCuSize;
		shiftNum = INTERNAL_PREC + 1 - DEPTH;
		offset = (1 << (shiftNum - 1)) + 2 * INTERNAL_OFFS;

		for (y = 0; y < m_nCuSize; y++)
		{
			for (x = 0; x < m_nCuSize; x += 4)
			{
				dstY[x + 0] = Clip((srcY0[x + 0] + srcY1[x + 0] + offset) >> shiftNum);
				dstY[x + 1] = Clip((srcY0[x + 1] + srcY1[x + 1] + offset) >> shiftNum);
				dstY[x + 2] = Clip((srcY0[x + 2] + srcY1[x + 2] + offset) >> shiftNum);
				dstY[x + 3] = Clip((srcY0[x + 3] + srcY1[x + 3] + offset) >> shiftNum);
			}

			srcY0 += src0Stride;
			srcY1 += src1Stride;
			dstY += dststride;
		}
	}
	if (bChroma)
	{
		shiftNum = INTERNAL_PREC + 1 - DEPTH;
		offset = (1 << (shiftNum - 1)) + 2 * INTERNAL_OFFS;
		src0Stride = src1Stride = dststride = m_nCuSize / 2;

		for (y = m_nCuSize/2 - 1; y >= 0; y--)
		{
			for (x = m_nCuSize/2 - 1; x >= 0;)
			{
				// note: chroma min width is 2
				dstU[x] = Clip((srcU0[x] + srcU1[x] + offset) >> shiftNum);
				dstV[x] = Clip((srcV0[x] + srcV1[x] + offset) >> shiftNum);
				x--;
				dstU[x] = Clip((srcU0[x] + srcU1[x] + offset) >> shiftNum);
				dstV[x] = Clip((srcV0[x] + srcV1[x] + offset) >> shiftNum);
				x--;
			}

			srcU0 += src0Stride;
			srcU1 += src1Stride;
			srcV0 += src0Stride;
			srcV1 += src1Stride;
			dstU += dststride;
			dstV += dststride;
		}
	}

	for (int i = 0; i < m_nCuSize; i ++)
	{
		for (int j = 0; j < m_nCuSize; j ++)
		{
			m_pBestPred[j + i*m_nCuSize] = DstY[j + i*m_nCuSize];
			m_pBestResi[j + i*m_nCuSize] = (short)(m_pCurrCuPel[j + i*m_nCuSize] - DstY[j + i*m_nCuSize]);
		}
	}
	int Offset = m_nCuSize*m_nCuSize;
	for (int i = 0; i < m_nCuSize/2; i++)
	{
		for (int j = 0; j < m_nCuSize/2; j++)
		{
			m_pBestPred[j + i*m_nCuSize/2 + Offset] = DstU[j + i*m_nCuSize / 2];
			m_pBestPred[j + i*m_nCuSize/2 + Offset*5/4] = DstV[j + i*m_nCuSize / 2];
			m_pBestResi[j + i*m_nCuSize/2 + Offset] = (short)(m_pCurrCuPel[j * 2 + 0 + i*m_nCuSize + Offset] - DstU[j + i*m_nCuSize / 2]);
			m_pBestResi[j + i*m_nCuSize/2 + Offset * 5 / 4] = (short)(m_pCurrCuPel[j * 2 + 1 + i*m_nCuSize + Offset] - DstV[j + i*m_nCuSize / 2]);
		}
	}

	delete[] SrcY0;
	delete[] SrcY1;
	delete[] SrcU0;
	delete[] SrcU1;
	delete[] SrcV0;
	delete[] SrcV1;
	delete[] DstY;
	delete[] DstU;
	delete[] DstV;
}

void FractionMotionEstimation::Avg(unsigned char *pAvg, unsigned char *pRef0, unsigned char *pRef1)
{
	for (int i = 0; i < m_nCuSize; i ++)
	{
		for (int j = 0; j < m_nCuSize; j ++)
		{
			pAvg[j + i*m_nCuSize] = (pRef0[j + i*m_nCuSize] + pRef1[j + i*m_nCuSize] + 1) >> 1;
		}
	}
}

void FractionMotionEstimation::CopyToBest(unsigned char *pCurrCuPred)
{
	for (int i = 0; i < m_nCuSize; i++)
	{
		for (int j = 0; j < m_nCuSize; j++)
		{
			m_pBestPred[j + i*m_nCuSize] = pCurrCuPred[j + i*m_nCuSize];
			m_pBestResi[j + i*m_nCuSize] = (short)(m_pCurrCuPel[j + i*m_nCuSize] - pCurrCuPred[j + i*m_nCuSize]);
		}
	}
	int offset = m_nCuSize*m_nCuSize;
	for (int i = 0; i < m_nCuSize / 2; i++)
	{
		for (int j = 0; j < m_nCuSize/2; j++)
		{
			m_pBestPred[j + i*m_nCuSize/2 + offset] = pCurrCuPred[j + i*m_nCuSize/2 + offset];
			m_pBestResi[j + i*m_nCuSize/2 + offset] = (short)(m_pCurrCuPel[j * 2 + 0 + i*m_nCuSize + offset] - pCurrCuPred[j + i*m_nCuSize/2 + offset]);
			m_pBestPred[j + i*m_nCuSize/2 + offset * 5 / 4] = pCurrCuPred[j + i*m_nCuSize/2 + offset * 5 / 4];
			m_pBestResi[j + i*m_nCuSize/2 + offset * 5 / 4] = (short)(m_pCurrCuPel[j * 2 + 1 + i*m_nCuSize + offset] - pCurrCuPred[j + i*m_nCuSize/2 + offset * 5 / 4]);
		}
	}
}

int FractionMotionEstimation::satd_8x8(unsigned char *pix1, unsigned char *pix2)
{
	int satd = 0;
	int stride_pix1 = m_nCuSize;
	int stride_pix2 = m_nCuSize;
	for (int row = 0; row < m_nCuSize; row += 4)
	{
		for (int col = 0; col < m_nCuSize; col += 8)
		{
			satd += satd_8x4(pix1 + row * stride_pix1 + col, stride_pix1,
				pix2 + row * stride_pix2 + col, stride_pix2);
		}
	}

	return satd;
}

int FractionMotionEstimation::satd_8x4(unsigned char *pix1, int stride_pix1, unsigned char *pix2, int stride_pix2)
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

void FractionMotionEstimation::xCalcResiAndMvd(int nRefPicIdx, bool isLuma, bool isChroma, bool isCalcMvd)
{
	if (isLuma)
	{
		unsigned char *pCurrCuPredLuma = new unsigned char[m_nCuSize*m_nCuSize];
		InterpLuma(pCurrCuPredLuma, m_nCuSize, m_mvFmeInput[nRefPicIdx], 8, nRefPicIdx);
		for (int i = 0; i < m_nCuSize; i++)
		{
			for (int j = 0; j < m_nCuSize; j++)
			{
				m_pCurrCuResi[nRefPicIdx][j + i*m_nCuSize] = static_cast<short>(m_pCurrCuPel[j + i*m_nCuSize] - pCurrCuPredLuma[j + i*m_nCuSize]);
				m_pCurrCuPred[nRefPicIdx][j + i*m_nCuSize] = pCurrCuPredLuma[j + i*m_nCuSize];
			}
		}
		delete[] pCurrCuPredLuma;
	}

	if (isCalcMvd)
	{
		int bCost = INT_MAX_MINE;
		for (int i = 0; i < 2; i++)
		{
			int cost = abs(m_mvFmeInput[nRefPicIdx].x - m_mvAmvp[nRefPicIdx][i].x) + abs(m_mvFmeInput[nRefPicIdx].y - m_mvAmvp[nRefPicIdx][i].y);
			if (cost < bCost)
			{
				m_nMvpIdx[nRefPicIdx] = i;
				bCost = cost;
				m_mvMvd[nRefPicIdx].x = m_mvFmeInput[nRefPicIdx].x - m_mvAmvp[nRefPicIdx][i].x;
				m_mvMvd[nRefPicIdx].y = m_mvFmeInput[nRefPicIdx].y - m_mvAmvp[nRefPicIdx][i].y;
			}
		}
	}

	if (isChroma)
	{
		unsigned char *pCurrCuPredChroma = new unsigned char[m_nCuSize*m_nCuSize / 4];
		unsigned char *pCurrCuPel = m_pCurrCuPel + m_nCuSize*m_nCuSize;
		short *pCurrCuResi = m_pCurrCuResi[nRefPicIdx] + m_nCuSize*m_nCuSize;
		unsigned char *pCurrCuPred = m_pCurrCuPred[nRefPicIdx] + m_nCuSize*m_nCuSize;
		InterpChroma(pCurrCuPredChroma, m_nCuSize / 2, m_mvFmeInput[nRefPicIdx], true, 4, nRefPicIdx);
		for (int i = 0; i < m_nCuSize / 2; i++)
		{
			for (int j = 0; j < m_nCuSize / 2; j++)
			{
				pCurrCuResi[j + i*m_nCuSize / 2] = static_cast<short>(pCurrCuPel[j * 2 + 0 + i*m_nCuSize] - pCurrCuPredChroma[j + i*m_nCuSize / 2]);
				pCurrCuPred[j + i*m_nCuSize / 2] = pCurrCuPredChroma[j + i*m_nCuSize / 2];
			}
		}

		pCurrCuResi += m_nCuSize*m_nCuSize / 4;
		pCurrCuPred += m_nCuSize*m_nCuSize / 4;
		InterpChroma(pCurrCuPredChroma, m_nCuSize / 2, m_mvFmeInput[nRefPicIdx], false, 4, nRefPicIdx);
		for (int i = 0; i < m_nCuSize / 2; i++)
		{
			for (int j = 0; j < m_nCuSize / 2; j++)
			{
				pCurrCuResi[j + i*m_nCuSize / 2] = static_cast<short>(pCurrCuPel[j * 2 + 1 + i*m_nCuSize] - pCurrCuPredChroma[j + i*m_nCuSize / 2]);
				pCurrCuPred[j + i*m_nCuSize / 2] = pCurrCuPredChroma[j + i*m_nCuSize / 2];
			}
		}
		delete[] pCurrCuPredChroma;
	}
}

void FractionMotionEstimation::PredInterBi(int nRefPicIdx, bool isLuma, bool isChroma)
{
	if (isLuma)
	{
		xPredInterBi(nRefPicIdx, true);
	}

	if (isChroma)
	{
		xPredInterBi(nRefPicIdx, false, true);
		xPredInterBi(nRefPicIdx, false, false);
	}
}

void FractionMotionEstimation::xPredInterBi(int nRefPicIdx, bool isLuma, bool isCr)
{
	int refStride = m_nCuSize + 8;
	int xFrac = m_mvFmeInput[nRefPicIdx].x & 0x3;
	int yFrac = m_mvFmeInput[nRefPicIdx].y & 0x3;
	unsigned char *pCurrCuForInterp;
	int N = 8;
	int offset = 0;
	int nCuSize = m_nCuSize;
	if (!isLuma)
	{
		N /= 2;
		refStride /= 2;
		int add = 0;
		nCuSize /= 2;
		pCurrCuForInterp = new unsigned char[(m_nCuSize + 8)*(m_nCuSize + 8) / 4];
		xFrac = m_mvFmeInput[nRefPicIdx].x & 0x7;
		yFrac = m_mvFmeInput[nRefPicIdx].y & 0x7;
		if (isCr)
		{
			offset = m_nCuSize*m_nCuSize;
		}
		else
		{
			add = 1;
			offset = m_nCuSize*m_nCuSize * 5 / 4;
		}
		int Offset = (m_nCuSize + 8)*(m_nCuSize + 8);
		for (int i = 0; i < (m_nCuSize + 8) / 2; i ++)
		{
			for (int j = 0; j < (m_nCuSize + 8) / 2; j++)
			{
				pCurrCuForInterp[j + i*(m_nCuSize + 8) / 2] = m_pFmeInterpPel[nRefPicIdx][Offset + j * 2 + add + i*(m_nCuSize + 8)];
			}
		}
	}
	else
	{
		pCurrCuForInterp = new unsigned char[(m_nCuSize + 8)*(m_nCuSize + 8)];
		for (int i = 0; i < (m_nCuSize + 8); i++)
		{
			for (int j = 0; j < (m_nCuSize + 8); j++)
			{
				pCurrCuForInterp[j + i*(m_nCuSize + 8)] = m_pFmeInterpPel[nRefPicIdx][j + i*(m_nCuSize + 8)];
			}
		}
	}

	pCurrCuForInterp += N / 2 + N / 2 * refStride;
	if ((yFrac | xFrac) == 0)
	{
		ConvertPelToShortBi(pCurrCuForInterp, refStride, m_pCurrCuResi[nRefPicIdx] + offset, nCuSize, nCuSize, nCuSize);
		//primitives.luma_p2s(ref, refStride, dst, width, height);
	}
	else if (yFrac == 0)
	{
		InterpHorizonBi(pCurrCuForInterp, refStride, m_pCurrCuResi[nRefPicIdx] + offset, nCuSize, xFrac, 0, N, nCuSize, nCuSize);
		//primitives.luma_hps[partEnum](ref, refStride, dst, dstStride, xFrac, 0);
	}
	else if (xFrac == 0)
	{
		short *pFilter = LumaFilter[yFrac];
		if (!isLuma)
			pFilter = ChromaFilter[yFrac];
		InterpVerticalBi(pCurrCuForInterp, refStride, m_pCurrCuResi[nRefPicIdx] + offset, nCuSize, nCuSize, nCuSize, pFilter, N);
		//primitives.ipfilter_ps[FILTER_V_P_S_8](ref, refStride, dst, dstStride, width, height, g_lumaFilter[yFrac]);
	}
	else
	{
		int tmpStride = nCuSize;
		int filterSize = LUMA_NTAPS;
		if (!isLuma) filterSize = CHROMA_NTAPS;
		int halfFilterSize = (filterSize >> 1);
		short immedVals[64 * (64 + LUMA_NTAPS - 1)];
		InterpHorizonBi(pCurrCuForInterp, refStride, immedVals, tmpStride, xFrac, 1, N, nCuSize, nCuSize);
		FilterVerticalBi(immedVals + (halfFilterSize - 1) * tmpStride, tmpStride, m_pCurrCuResi[nRefPicIdx] + offset, nCuSize, nCuSize, nCuSize, yFrac, N);
		//primitives.luma_hps[partEnum](ref, refStride, m_immedVals, tmpStride, xFrac, 1);
		//primitives.ipfilter_ss[FILTER_V_S_S_8](m_immedVals + (halfFilterSize - 1) * tmpStride, tmpStride, dst, dstStride, width, height, yFrac);
	}
	pCurrCuForInterp -= N / 2 + N / 2 * refStride;
	delete[] pCurrCuForInterp;
}

void FractionMotionEstimation::ConvertPelToShortBi(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height)
{//x = 0, y = 0
	int shift = INTERNAL_PREC - DEPTH;
	int row, col;

	for (row = 0; row < height; row++)
	{
		for (col = 0; col < width; col++)
		{
			short val = src[col] << shift;
			dst[col] = val - (short)INTERNAL_OFFS;
		}

		src += srcStride;
		dst += dstStride;
	}
}

void FractionMotionEstimation::InterpHorizonBi(unsigned char *src, int srcStride, short *dst, short dstStride, int coeffIdx, int isRowExt, int N, int width, int height)
{//x !=0 y = 0  or  x !=0 y != 0
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

void FractionMotionEstimation::InterpVerticalBi(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *c, int N)
{//x = 0, y != 0
	int headRoom = INTERNAL_PREC - DEPTH;
	int shift = FILTER_PREC - headRoom;
	int offset = -INTERNAL_OFFS << shift;

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
			dst[col] = val;
		}

		src += srcStride;
		dst += dstStride;
	}
}

void FractionMotionEstimation::FilterVerticalBi(short *src, int srcStride, short *dst, int dstStride, int width, int height, const int coefIdx, int N)
{//x !=0 y != 0
	const short *const c = (N == 8 ? LumaFilter[coefIdx] : ChromaFilter[coefIdx]);
	int shift = FILTER_PREC;
	int row, col;

	src -= (N / 2 - 1) * srcStride;
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

			short val = (short)((sum) >> shift);
			dst[col] = val;
		}

		src += srcStride;
		dst += dstStride;
	}
}

void FractionMotionEstimation::InterpLuma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, int N, int nRefPicIdx)
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
			pFmeInterpPel[j + i*stride_1] = m_pFmeInterpPel[nRefPicIdx][j + i*stride_1];
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

void FractionMotionEstimation::InterpChroma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, bool isCb, int N, int nRefPicIdx)
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
			pFmeInterpPel[j + i*stride_1] = m_pFmeInterpPel[nRefPicIdx][j * 2 + offs + i*stride_1 * 2 + (m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2)];
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

//class MergeMvpCand
class MergeMvpCand MergeCand;

MergeMvpCand::MergeMvpCand()
{
	m_bTMVPFlag = true;
	m_nCtuSize = 64;
	m_nMergeCand = 2;
	m_nMergeLevel = 2;
}

MergeMvpCand::~MergeMvpCand()
{

}

void MergeMvpCand::setMvSpatial(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatial[idx], &mvSpatial, sizeof(SPATIAL_MV)); 
}

void MergeMvpCand::setMvSpatialForCtu(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatialForCtu[idx], &mvSpatial, sizeof(SPATIAL_MV));
}

void MergeMvpCand::setMvSpatialForCu64(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatialForCu64[idx], &mvSpatial, sizeof(SPATIAL_MV));
}

void MergeMvpCand::setMvSpatialForCu32(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatialForCu32[idx], &mvSpatial, sizeof(SPATIAL_MV));
}

void MergeMvpCand::setMvSpatialForCu16(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatialForCu16[idx], &mvSpatial, sizeof(SPATIAL_MV));
}

void MergeMvpCand::setMvSpatialForCu8(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatialForCu8[idx], &mvSpatial, sizeof(SPATIAL_MV));
}

void MergeMvpCand::getMvSpatialForCtu(SPATIAL_MV &mvSpatial, int idx)
{
	memcpy(&mvSpatial, &m_mvSpatialForCtu[idx], sizeof(SPATIAL_MV));
}

void MergeMvpCand::getMvSpatialForCu64(SPATIAL_MV &mvSpatial, int idx)
{
	memcpy(&mvSpatial, &m_mvSpatialForCu64[idx], sizeof(SPATIAL_MV));
}
void MergeMvpCand::getMvSpatialForCu32(SPATIAL_MV &mvSpatial, int idx)
{
	memcpy(&mvSpatial, &m_mvSpatialForCu32[idx], sizeof(SPATIAL_MV));
}
void MergeMvpCand::getMvSpatialForCu16(SPATIAL_MV &mvSpatial, int idx)
{
	memcpy(&mvSpatial, &m_mvSpatialForCu16[idx], sizeof(SPATIAL_MV));
}
void MergeMvpCand::getMvSpatialForCu8(SPATIAL_MV &mvSpatial, int idx)
{
	memcpy(&mvSpatial, &m_mvSpatialForCu8[idx], sizeof(SPATIAL_MV));
}

void MergeMvpCand::setMvTemporal(TEMPORAL_MV mvTemporal, int cuIdx, int nPos)
{
	memcpy(&m_mvTemporal[cuIdx][nPos], &mvTemporal, sizeof(TEMPORAL_MV));
}

void MergeMvpCand::getMergeCandidates()
{
	unsigned int absPartAddr = RasterScan[m_nCuPosInCtu];
	bool abCandIsInter[MAX_MRG_NUM_CANDS];
	for (int i = 0; i < m_nMergeCand; ++i)
	{
		abCandIsInter[i] = false;
		m_mvFieldNeighbours[(i << 1)].refIdx = INVALID;
		m_mvFieldNeighbours[(i << 1) + 1].refIdx = INVALID;
	}
	int count = 0;
	int xP = static_cast<int>(OffsFromCtu64[m_nCuPosInCtu][0]);
	int yP = static_cast<int>(OffsFromCtu64[m_nCuPosInCtu][1]);
	int nPSW = m_nCuSize;
	int nPSH = m_nCuSize;

	bool PicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	int nCtuInPicWidth = m_nPicWidth / m_nCtuSize + (PicWidthNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % nCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / nCtuInPicWidth;
	bool isValid[5] = { 0 };
	for (int i = 0; i < 5; i++)
		isValid[i] = true;
	
	if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0) //A1
		isValid[0] = false;

	if (nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0) //B1
		isValid[1] = false;

	if (nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0 ||
		nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] + m_nCuSize + 4 >= m_nPicWidth) //B0
		isValid[2] = false;

	if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0 ||
		nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] + m_nCuSize + 4 >= m_nPicHeight) //A0
		isValid[3] = false;

	if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0 ||
		nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0) //B2
		isValid[4] = false;

	//order is A1,B1,B0,A0,B2  
	bool isAvailableA1 = m_mvSpatial[0].valid && isValid[0]
		                            && (m_mvSpatial[0].pred_flag[0] | m_mvSpatial[0].pred_flag[1]) 
									&& isDiffMER(xP - 1, yP + nPSH - 1, xP, yP);
	if (isAvailableA1) //A1
	{
		abCandIsInter[count] = true;
		m_nInterDirNeighbours[count] = m_mvSpatial[0].pred_flag[0] | (m_mvSpatial[0].pred_flag[1] << 1);
		m_mvFieldNeighbours[count << 1].mv.x = m_mvSpatial[0].mv[0].x;
		m_mvFieldNeighbours[count << 1].mv.y = m_mvSpatial[0].mv[0].y;
		m_mvFieldNeighbours[count << 1].refIdx = m_mvSpatial[0].ref_idx[0];

		if (b_slice == m_nSliceType)
		{
			m_mvFieldNeighbours[(count << 1) + 1].mv.x = m_mvSpatial[0].mv[1].x;
			m_mvFieldNeighbours[(count << 1) + 1].mv.y = m_mvSpatial[0].mv[1].y;
			m_mvFieldNeighbours[(count << 1) + 1].refIdx = m_mvSpatial[0].ref_idx[1];
		}
		count++;
	}
	if (m_nMergeCand == count)
		return;

	bool isAvailableB1 = m_mvSpatial[1].valid && isValid[1]
		&& (m_mvSpatial[1].pred_flag[0] | m_mvSpatial[1].pred_flag[1])
		&& isDiffMER(xP + nPSW - 1, yP - 1, xP, yP);
	if (isAvailableB1 && !(isAvailableA1 && hasEqualMotion(m_mvSpatial[1], m_mvSpatial[0])))
	{
		abCandIsInter[count] = true;
		m_nInterDirNeighbours[count] = m_mvSpatial[1].pred_flag[0] | (m_mvSpatial[1].pred_flag[1] << 1);
		m_mvFieldNeighbours[count << 1].mv.x = m_mvSpatial[1].mv[0].x;
		m_mvFieldNeighbours[count << 1].mv.y = m_mvSpatial[1].mv[0].y;
		m_mvFieldNeighbours[count << 1].refIdx = m_mvSpatial[1].ref_idx[0];

		if (b_slice == m_nSliceType)
		{
			m_mvFieldNeighbours[(count << 1) + 1].mv.x = m_mvSpatial[1].mv[1].x;
			m_mvFieldNeighbours[(count << 1) + 1].mv.y = m_mvSpatial[1].mv[1].y;
			m_mvFieldNeighbours[(count << 1) + 1].refIdx = m_mvSpatial[1].ref_idx[1];
		}
		count++;
	}
	if (m_nMergeCand == count)
		return;

	bool isAvailableB0 = m_mvSpatial[2].valid && isValid[2] //position is valid or not
		&& (m_mvSpatial[2].pred_flag[0] | m_mvSpatial[2].pred_flag[1]) //intra or not
		&& isDiffMER(xP + nPSW, yP - 1, xP, yP); //in the same merge level or not
	if (isAvailableB0 && !(isAvailableB1 && hasEqualMotion(m_mvSpatial[2], m_mvSpatial[1])))
	{
		abCandIsInter[count] = true;
		m_nInterDirNeighbours[count] = m_mvSpatial[2].pred_flag[0] | (m_mvSpatial[2].pred_flag[1] << 1);
		m_mvFieldNeighbours[count << 1].mv.x = m_mvSpatial[2].mv[0].x;
		m_mvFieldNeighbours[count << 1].mv.y = m_mvSpatial[2].mv[0].y;
		m_mvFieldNeighbours[count << 1].refIdx = m_mvSpatial[2].ref_idx[0];

		if (b_slice == m_nSliceType)
		{
			m_mvFieldNeighbours[(count << 1) + 1].mv.x = m_mvSpatial[2].mv[1].x;
			m_mvFieldNeighbours[(count << 1) + 1].mv.y = m_mvSpatial[2].mv[1].y;
			m_mvFieldNeighbours[(count << 1) + 1].refIdx = m_mvSpatial[2].ref_idx[1];
		}
		count++;
	}
	if (m_nMergeCand == count)
		return;

	bool isAvailableA0 = m_mvSpatial[3].valid && isValid[3] //position is valid or not
		&& (m_mvSpatial[3].pred_flag[0] | m_mvSpatial[3].pred_flag[1]) //intra or not
		&& isDiffMER(xP - 1, yP + nPSH, xP, yP); //in the same merge level or not
	if (isAvailableA0 && !(isAvailableA1 && hasEqualMotion(m_mvSpatial[3], m_mvSpatial[0])))
	{
		abCandIsInter[count] = true;
		m_nInterDirNeighbours[count] = m_mvSpatial[3].pred_flag[0] | (m_mvSpatial[3].pred_flag[1] << 1);
		m_mvFieldNeighbours[count << 1].mv.x = m_mvSpatial[3].mv[0].x;
		m_mvFieldNeighbours[count << 1].mv.y = m_mvSpatial[3].mv[0].y;
		m_mvFieldNeighbours[count << 1].refIdx = m_mvSpatial[3].ref_idx[0];

		if (b_slice == m_nSliceType)
		{
			m_mvFieldNeighbours[(count << 1) + 1].mv.x = m_mvSpatial[3].mv[1].x;
			m_mvFieldNeighbours[(count << 1) + 1].mv.y = m_mvSpatial[3].mv[1].y;
			m_mvFieldNeighbours[(count << 1) + 1].refIdx = m_mvSpatial[3].ref_idx[1];
		}
		count++;
	}
	if (m_nMergeCand == count)
		return;

	if (count<4)
	{
		bool isAvailableB2 = m_mvSpatial[4].valid && isValid[4] //position is valid or not
			&& (m_mvSpatial[4].pred_flag[0] | m_mvSpatial[4].pred_flag[1]) //intra or not
			&& isDiffMER(xP - 1, yP - 1, xP, yP); //in the same merge level or not
		if (isAvailableB2 && !(isAvailableA1 && hasEqualMotion(m_mvSpatial[4], m_mvSpatial[0]))&& !(isAvailableB1 && hasEqualMotion(m_mvSpatial[4], m_mvSpatial[1])))
		{
			abCandIsInter[count] = true;
			m_nInterDirNeighbours[count] = m_mvSpatial[4].pred_flag[0] | (m_mvSpatial[4].pred_flag[1] << 1);
			m_mvFieldNeighbours[count << 1].mv.x = m_mvSpatial[4].mv[0].x;
			m_mvFieldNeighbours[count << 1].mv.y = m_mvSpatial[4].mv[0].y;
			m_mvFieldNeighbours[count << 1].refIdx = m_mvSpatial[4].ref_idx[0];

			if (b_slice == m_nSliceType)
			{
				m_mvFieldNeighbours[(count << 1) + 1].mv.x = m_mvSpatial[4].mv[1].x;
				m_mvFieldNeighbours[(count << 1) + 1].mv.y = m_mvSpatial[4].mv[1].y;
				m_mvFieldNeighbours[(count << 1) + 1].refIdx = m_mvSpatial[4].ref_idx[1];
			}
			count++;
		}
	}
	if (m_nMergeCand == count)
		return;

	if (m_bTMVPFlag) //time domain
	{
		unsigned int partIdxRB;
		int lcuIdx = m_nCtuSize;
		deriveRightBottomIdx(partIdxRB);
		unsigned int uiAbsPartIdxTmp = ZscanToRaster[partIdxRB];
		unsigned int numPartInCuWidth = 16;
		unsigned int numPartInCuHeight = 16;
		bool isPicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
		int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (isPicWidthNotDivCtu ? 1 : 0);
		int nCtuPicX = (m_nCtuPosInPic % numCtuInPicWidth)*m_nCtuSize;
		int nCtuPicY = (m_nCtuPosInPic / numCtuInPicWidth)*m_nCtuSize;

		if ((nCtuPicX + (int)RasterToPelX[uiAbsPartIdxTmp] + 4) >= m_nPicWidth)  // image boundary check
		{
			lcuIdx = -1;
		}
		else if ((nCtuPicY + (int)RasterToPelY[uiAbsPartIdxTmp] + 4) >= m_nPicHeight)
		{
			lcuIdx = -1;
		}
		else
		{
			if ((uiAbsPartIdxTmp % numPartInCuWidth < numPartInCuWidth - 1) &&        // is not at the last column of LCU
				(uiAbsPartIdxTmp / numPartInCuWidth < numPartInCuHeight - 1)) // is not at the last row    of LCU
			{
				absPartAddr = RasterToZscan[uiAbsPartIdxTmp + numPartInCuWidth + 1];
				lcuIdx = m_nCtuPosInPic;
			}
			else if (uiAbsPartIdxTmp % numPartInCuWidth < numPartInCuWidth - 1)       // is not at the last column of LCU But is last row of LCU
			{
				absPartAddr = RasterToZscan[(uiAbsPartIdxTmp + numPartInCuWidth + 1) % 256];
				lcuIdx = -1;
			}
			else if (uiAbsPartIdxTmp / numPartInCuWidth < numPartInCuHeight - 1) // is not at the last row of LCU But is last column of LCU
			{
				absPartAddr = RasterToZscan[uiAbsPartIdxTmp + 1];
				lcuIdx = m_nCtuPosInPic + 1;
			}
			else //is the right bottom corner of LCU
			{
				absPartAddr = 0;
				lcuIdx = -1;
			}
		}

		Mv colmv;
		int refIdx = 0;

		bool bExistMV = false;
		unsigned int partIdxCenter;
		int curLCUIdx = m_nCtuPosInPic;
		int dir = 0;
		int arrayAddr = count;
		xDeriveCenterIdx(partIdxCenter);
		bExistMV = lcuIdx >= 0 && xGetColMVP(REF_PIC_LIST0, lcuIdx, absPartAddr, colmv, m_bCheckLDC, m_nFromL0Flag);
		if (!bExistMV)
		{
			bExistMV = xGetColMVP(REF_PIC_LIST0, curLCUIdx, partIdxCenter, colmv, m_bCheckLDC, m_nFromL0Flag);
		}
		if (bExistMV)
		{
			dir |= 1;
			m_mvFieldNeighbours[2 * arrayAddr].setMvField(colmv, refIdx);
		}

		if (b_slice == m_nSliceType)
		{
			bExistMV = lcuIdx >= 0 && xGetColMVP(REF_PIC_LIST1, lcuIdx, absPartAddr, colmv, m_bCheckLDC, m_nFromL0Flag);
			if (!bExistMV)
			{
				bExistMV = xGetColMVP(REF_PIC_LIST1, curLCUIdx, partIdxCenter, colmv, m_bCheckLDC, m_nFromL0Flag);
			}
			if (bExistMV)
			{
				dir |= 2;
				m_mvFieldNeighbours[2 * arrayAddr + 1].setMvField(colmv, refIdx);
			}
		}

		if (dir != 0)
		{
			m_nInterDirNeighbours[arrayAddr] = dir;
			abCandIsInter[arrayAddr] = true;
			count++;
		}
	}
	if (m_nMergeCand == count)
		return;

	int arrayAddr = count;
	int cutoff = arrayAddr;

	if (b_slice == m_nSliceType)
	{
		// TODO: TComRom??
		unsigned int priorityList0[12] = { 0, 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3 };
		unsigned int priorityList1[12] = { 1, 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2 };

		for (int idx = 0; idx < cutoff * (cutoff - 1) && arrayAddr != m_nMergeCand; idx++)
		{
			int i = priorityList0[idx];
			int j = priorityList1[idx];
			if (abCandIsInter[i] && abCandIsInter[j] && (m_nInterDirNeighbours[i] & 0x1) && (m_nInterDirNeighbours[j] & 0x2))
			{
				abCandIsInter[arrayAddr] = true;
				m_nInterDirNeighbours[arrayAddr] = 3;

				// get Mv from cand[i] and cand[j]
				m_mvFieldNeighbours[arrayAddr << 1].setMvField(m_mvFieldNeighbours[i << 1].mv, m_mvFieldNeighbours[i << 1].refIdx);
				m_mvFieldNeighbours[(arrayAddr << 1) + 1].setMvField(m_mvFieldNeighbours[(j << 1) + 1].mv, m_mvFieldNeighbours[(j << 1) + 1].refIdx);

				int refIdx0 = m_mvFieldNeighbours[(arrayAddr << 1)].refIdx;
				int refIdx1 = m_mvFieldNeighbours[(arrayAddr << 1) + 1].refIdx;
				if (refIdx0 == refIdx1 && 
					m_mvFieldNeighbours[(arrayAddr << 1)].mv.x == m_mvFieldNeighbours[(arrayAddr << 1) + 1].mv.x &&
					m_mvFieldNeighbours[(arrayAddr << 1)].mv.y == m_mvFieldNeighbours[(arrayAddr << 1) + 1].mv.y)
				{
					abCandIsInter[arrayAddr] = false;
				}
				else
				{
					arrayAddr++;
				}
			}
		}
	}

	if ((int)arrayAddr == m_nMergeCand)
	{
		return;
	}

	int numRefIdx = 1;
	if (p_slice == m_nSliceType)
	{
		numRefIdx = m_nRefPic[0];
	}
	
	int r = 0;
	int refcnt = 0;
	while (arrayAddr < m_nMergeCand)
	{
		abCandIsInter[arrayAddr] = true;
		m_nInterDirNeighbours[arrayAddr] = 1;
		Mv tmpMv; tmpMv.x = 0; tmpMv.y = 0;
		m_mvFieldNeighbours[arrayAddr << 1].setMvField(tmpMv, r);

		if (b_slice == m_nSliceType)
		{
			m_nInterDirNeighbours[arrayAddr] = 3;
			m_mvFieldNeighbours[(arrayAddr << 1) + 1].setMvField(tmpMv, r);
		}
		arrayAddr++;
		if (refcnt == numRefIdx - 1)
		{
			r = 0;
		}
		else
		{
			++r;
			++refcnt;
		}
	}

}

void MergeMvpCand::deriveRightBottomIdx(unsigned int& outPartIdxRB)
{
	outPartIdxRB = RasterToZscan[ZscanToRaster[RasterScan[m_nCuPosInCtu]] + (((m_nCuSize / 4) >> 1) - 1) * 16 + m_nCuSize / 4 - 1];
	unsigned int numPartitions = m_nCuSize*m_nCuSize / 16;
	outPartIdxRB += numPartitions >> 1;
}

void MergeMvpCand::xDeriveCenterIdx(unsigned int& outPartIdxCenter)
{
	outPartIdxCenter = RasterScan[m_nCuPosInCtu]; // partition origin.
	outPartIdxCenter = RasterToZscan[ZscanToRaster[outPartIdxCenter] + (m_nCuSize / 4) / 2 * 16 + (m_nCuSize / 4) / 2];
}

bool MergeMvpCand::isDiffMER(int xN, int yN, int xP, int yP)
{
	int plevel = m_nMergeLevel;

	if ((xN >> plevel) != (xP >> plevel))
	{
		return true;
	}
	if ((yN >> plevel) != (yP >> plevel))
	{
		return true;
	}
	return false;
}

bool MergeMvpCand::hasEqualMotion(SPATIAL_MV mvCurrent, SPATIAL_MV mvCompare)
{
	if (mvCurrent.pred_flag[0] != mvCompare.pred_flag[0] || mvCurrent.pred_flag[1] != mvCompare.pred_flag[1])
	{
		return false;
	}

	for (int refListIdx = 0; refListIdx < 2; refListIdx++)
	{
		if (mvCurrent.pred_flag[refListIdx])
		{
			if (mvCurrent.mv[refListIdx].x != mvCompare.mv[refListIdx].x || mvCurrent.mv[refListIdx].y != mvCompare.mv[refListIdx].y
				|| mvCurrent.ref_idx[refListIdx] != mvCompare.ref_idx[refListIdx])
			{
				return false;
			}
		}
	}

	return true;
}

int MergeMvpCand::zscanToPos(unsigned int zscan)
{
	int nPos = 0;
	switch (zscan)
	{
	case 0: nPos = 0; break;
	case 16: nPos = 1; break;
	case 64: nPos = 2; break;
	case 80: nPos = 3; break;
	case 32: nPos = 4; break;
	case 48: nPos = 5; break;
	case 96: nPos = 6; break;
	case 112: nPos = 7; break;
	case 128: nPos = 8; break;
	case 144: nPos = 9; break;
	case 192: nPos = 10; break;
	case 208: nPos = 11; break;
	case 160: nPos = 12; break;
	case 176: nPos = 13; break;
	case 224: nPos = 14; break;
	case 240: nPos = 15; break;
	default: assert(0);
	}
	return nPos;
}

bool MergeMvpCand::xGetColMVP(int picList, int cuAddr, unsigned int partUnitIdx, Mv& outMV, bool isCheckLDC, int colFromL0Flag)
{
	unsigned int absPartAddr = partUnitIdx & 0xFFFFFFF0;
	int colRefPicList;
	int curPOC, curRefPOC, scale;
	Mv colmv;

	int idx = (m_nCtuPosInPic < cuAddr) ? 1 : 0;
	int nPos = zscanToPos(absPartAddr);
	
	if (!(m_mvTemporal[idx][nPos].pred_l0 || m_mvTemporal[idx][nPos].pred_l1)) 
	{
		return false; //current CTU of collaborate picture is intra
	}
	
	curPOC = m_nCurrPicPoc;
	curRefPOC = m_nCurrRefPicPoc[picList];

	colRefPicList = isCheckLDC ? picList : colFromL0Flag;
	bool isListValid = colRefPicList ? m_mvTemporal[idx][nPos].pred_l1 : m_mvTemporal[idx][nPos].pred_l0;
	if (!isListValid)
	{
		colRefPicList = 1 - colRefPicList;
		isListValid = colRefPicList ? m_mvTemporal[idx][nPos].pred_l1 : m_mvTemporal[idx][nPos].pred_l0;
		if (!isListValid)
		{
			return false;
		}
	}
	colmv.x = colRefPicList ? m_mvTemporal[idx][nPos].mv_x_l1 : m_mvTemporal[idx][nPos].mv_x_l0;
	colmv.y = colRefPicList ? m_mvTemporal[idx][nPos].mv_y_l1 : m_mvTemporal[idx][nPos].mv_y_l0;
	int nDiffColPicPoc = colRefPicList ? m_mvTemporal[idx][nPos].delta_poc_l1 : m_mvTemporal[idx][nPos].delta_poc_l0;
	int nDiffCurrPicPoc = curPOC - curRefPOC;
	scale = xGetDistScaleFactor(nDiffCurrPicPoc, nDiffColPicPoc);
	if (scale == 4096)
	{
		outMV = colmv;
	}
	else
	{
		outMV = scaleMv(colmv, scale);
	}
	return true;
}

int MergeMvpCand::xGetDistScaleFactor(int diffPocB, int diffPocD)
{
	if (diffPocD == diffPocB)
	{
		return 4096;
	}
	else
	{
		int tdb = Clip3(-128, 127, diffPocB);
		int tdd = Clip3(-128, 127, diffPocD);
		int x = (0x4000 + abs(tdd / 2)) / tdd;
		int scale = Clip3(-4096, 4095, (tdb * x + 32) >> 6);
		return scale;
	}
}

Mv MergeMvpCand::scaleMv(Mv mv, int scale)
{
	int mvx = Clip3(-32768, 32767, (scale * mv.x + 127 + (scale * mv.x < 0)) >> 8);
	int mvy = Clip3(-32768, 32767, (scale * mv.y + 127 + (scale * mv.y < 0)) >> 8);
	Mv tmpMv; tmpMv.x = static_cast<short>(mvx);  tmpMv.y = static_cast<short>(mvy);
	return tmpMv;
}

//class AmvpCand
class AmvpCand Amvp;

AmvpCand::AmvpCand()
{
	m_bTMVPFlag = true;
	m_nCtuSize = 64;
}

AmvpCand::~AmvpCand()
{

}

void AmvpCand::setMvSpatial(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatial[idx], &mvSpatial, sizeof(SPATIAL_MV));
}
void AmvpCand::setMvSpatialForCtu(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatialForCtu[idx], &mvSpatial, sizeof(SPATIAL_MV));
}
void AmvpCand::setMvSpatialForCu64(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatialForCu64[idx], &mvSpatial, sizeof(SPATIAL_MV));
}
void AmvpCand::setMvSpatialForCu32(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatialForCu32[idx], &mvSpatial, sizeof(SPATIAL_MV));
}
void AmvpCand::setMvSpatialForCu16(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatialForCu16[idx], &mvSpatial, sizeof(SPATIAL_MV));
}
void AmvpCand::setMvSpatialForCu8(SPATIAL_MV mvSpatial, int idx)
{
	memcpy(&m_mvSpatialForCu8[idx], &mvSpatial, sizeof(SPATIAL_MV));
}
void AmvpCand::getMvSpatialForCtu(SPATIAL_MV &mvSpatial, int idx)
{
	memcpy(&mvSpatial, &m_mvSpatialForCtu[idx], sizeof(SPATIAL_MV));
}
void AmvpCand::getMvSpatialForCu64(SPATIAL_MV &mvSpatial, int idx)
{
	memcpy(&mvSpatial, &m_mvSpatialForCu64[idx], sizeof(SPATIAL_MV));
}
void AmvpCand::getMvSpatialForCu32(SPATIAL_MV &mvSpatial, int idx)
{
	memcpy(&mvSpatial, &m_mvSpatialForCu32[idx], sizeof(SPATIAL_MV));
}
void AmvpCand::getMvSpatialForCu16(SPATIAL_MV &mvSpatial, int idx)
{
	memcpy(&mvSpatial, &m_mvSpatialForCu16[idx], sizeof(SPATIAL_MV));
}
void AmvpCand::getMvSpatialForCu8(SPATIAL_MV &mvSpatial, int idx)
{
	memcpy(&mvSpatial, &m_mvSpatialForCu8[idx], sizeof(SPATIAL_MV));
}
void AmvpCand::setMvTemporal(TEMPORAL_MV mvTemporal, int cuIdx, int nPos)
{
	memcpy(&m_mvTemporal[cuIdx][nPos], &mvTemporal, sizeof(TEMPORAL_MV));
}

void AmvpCand::setCurrRefPicPoc(int nCurrRefPicPoc[nMaxRefPic/2], int list)
{
	for (int i = 0; i < nMaxRefPic / 2; i++)
	{
		m_nCurrRefPicPoc[list][i] = nCurrRefPicPoc[i];
	}
}

void AmvpCand::fillMvpCand(int picList, int refIdx, AmvpInfo* info)
{
	bool bAddedSmvp = false;

	info->m_num = 0;
	if (refIdx < 0)
	{
		return;
	}

	bool PicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	int nCtuInPicWidth = m_nPicWidth / m_nCtuSize + (PicWidthNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % nCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / nCtuInPicWidth;
	bool isValid[5] = { 0 };
	for (int i = 0; i < 5; i++)
		isValid[i] = true;

	if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0) //A1
		isValid[0] = false;

	if (nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0) //B1
		isValid[1] = false;

	if (nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0 ||
		nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] + m_nCuSize + 4 >= m_nPicWidth) //B0
		isValid[2] = false;

	if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0 ||
		nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] + m_nCuSize + 4 >= m_nPicHeight) //A0
		isValid[3] = false;

	if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0 ||
		nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0) //B2
		isValid[4] = false;

	//-- Get Spatial MV
	bool bAdded = false;
	bAddedSmvp = m_mvSpatial[3].valid && isValid[3] && (m_mvSpatial[3].pred_flag[0] | m_mvSpatial[3].pred_flag[1]);
	if (!bAddedSmvp)
	{
		bAddedSmvp = m_mvSpatial[0].valid && isValid[0] && (m_mvSpatial[0].pred_flag[0] | m_mvSpatial[0].pred_flag[1]);
	}

	// Left predictor search
	bAdded = xAddMVPCand(info, picList, refIdx, AMVP_BELOW_LEFT);
	if (!bAdded)
	{
		bAdded = xAddMVPCand(info, picList, refIdx, AMVP_LEFT);
	}

	if (!bAdded)
	{
		bAdded = xAddMVPCandOrder(info, picList, refIdx, AMVP_BELOW_LEFT);
		if (!bAdded)
		{
			bAdded = xAddMVPCandOrder(info, picList, refIdx, AMVP_LEFT);
		}
	}
	// Above predictor search
	bAdded = xAddMVPCand(info, picList, refIdx, AMVP_ABOVE_RIGHT);

	if (!bAdded)
	{
		bAdded = xAddMVPCand(info, picList, refIdx, AMVP_ABOVE);
	}

	if (!bAdded)
	{
		bAdded = xAddMVPCand(info, picList, refIdx, AMVP_ABOVE_LEFT);
	}
	bAdded = bAddedSmvp;
	if (info->m_num == 2) bAdded = true;

	if (!bAdded)
	{
		bAdded = xAddMVPCandOrder(info, picList, refIdx, AMVP_ABOVE_RIGHT);
		if (!bAdded)
		{
			bAdded = xAddMVPCandOrder(info, picList, refIdx, AMVP_ABOVE);
		}

		if (!bAdded)
		{
			bAdded = xAddMVPCandOrder(info, picList, refIdx, AMVP_ABOVE_LEFT);
		}
	}

	if (info->m_num == 2)
	{
		if (info->m_mvCand[0].x == info->m_mvCand[1].x && info->m_mvCand[0].y == info->m_mvCand[1].y)
		{
			info->m_num = 1;
		}
	}

	if (m_bTMVPFlag)
	{
		Mv colmv;
		unsigned int partIdxRB;
		unsigned int absPartAddr = 0;
		int lcuIdx = m_nCtuSize;
		deriveRightBottomIdx(partIdxRB);
		unsigned int uiAbsPartIdxTmp = ZscanToRaster[partIdxRB];
		unsigned int numPartInCuWidth = 16;
		unsigned int numPartInCuHeight = 16;
		bool isPicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
		int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (isPicWidthNotDivCtu ? 1 : 0);
		int nCtuPicX = (m_nCtuPosInPic % numCtuInPicWidth)*m_nCtuSize;
		int nCtuPicY = (m_nCtuPosInPic / numCtuInPicWidth)*m_nCtuSize;

		if ((nCtuPicX + (int)RasterToPelX[uiAbsPartIdxTmp] + 4) >= m_nPicWidth)  // image boundary check
		{
			lcuIdx = -1;
		}
		else if ((nCtuPicY + (int)RasterToPelY[uiAbsPartIdxTmp] + 4) >= m_nPicHeight)
		{
			lcuIdx = -1;
		}
		else
		{
			if ((uiAbsPartIdxTmp % numPartInCuWidth < numPartInCuWidth - 1) &&        // is not at the last column of LCU
				(uiAbsPartIdxTmp / numPartInCuWidth < numPartInCuHeight - 1)) // is not at the last row    of LCU
			{
				absPartAddr = RasterToZscan[uiAbsPartIdxTmp + numPartInCuWidth + 1];
				lcuIdx = m_nCtuPosInPic;
			}
			else if (uiAbsPartIdxTmp % numPartInCuWidth < numPartInCuWidth - 1)       // is not at the last column of LCU But is last row of LCU
			{
				absPartAddr = RasterToZscan[(uiAbsPartIdxTmp + numPartInCuWidth + 1) % 256];
				lcuIdx = -1;
			}
			else if (uiAbsPartIdxTmp / numPartInCuWidth < numPartInCuHeight - 1) // is not at the last row of LCU But is last column of LCU
			{
				absPartAddr = RasterToZscan[uiAbsPartIdxTmp + 1];
				lcuIdx = m_nCtuPosInPic + 1;
			}
			else //is the right bottom corner of LCU
			{
				absPartAddr = 0;
				lcuIdx = -1;
			}
		}

		if (lcuIdx >= 0 && xGetColMVP(picList, lcuIdx, absPartAddr, colmv, refIdx, m_bCheckLDC, m_nFromL0Flag))
		{
			info->m_mvCand[info->m_num++] = colmv;
		}
		else
		{
			unsigned int partIdxCenter;
			unsigned int curLCUIdx = m_nCtuPosInPic;
			xDeriveCenterIdx(partIdxCenter);
			if (xGetColMVP(picList, curLCUIdx, partIdxCenter, colmv, refIdx, m_bCheckLDC, m_nFromL0Flag))
			{
				info->m_mvCand[info->m_num++] = colmv;
			}
		}
		//----  co-located RightBottom Temporal Predictor  ---//
	}

	if (info->m_num > MAX_AMVP_NUM_CANDS)
	{
		info->m_num = MAX_AMVP_NUM_CANDS;
	}
	while (info->m_num < MAX_AMVP_NUM_CANDS)
	{
		info->m_mvCand[info->m_num].x = 0;
		info->m_mvCand[info->m_num].y = 0;
		info->m_num++;
	}
}

bool AmvpCand::xAddMVPCand(AmvpInfo* info, int picList, int refIdx, AMVP_DIR dir)
{
	bool PicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (PicWidthNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % numCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / numCtuInPicWidth;
	bool isOutPic = false;
	int idx = 0;
	switch (dir)
	{
	case AMVP_LEFT:
		if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0)
			isOutPic = true;		
		idx = 0; break;
	case AMVP_ABOVE:
		if (nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0)
			isOutPic = true;
		idx = 1; break;
	case AMVP_ABOVE_RIGHT:
		if (nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0 || 
			nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0]+m_nCuSize+4>=m_nPicWidth)
			isOutPic = true;
		idx = 2; break;
	case AMVP_BELOW_LEFT:
		if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0 ||
			nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] + m_nCuSize + 4 >= m_nPicHeight)
			isOutPic = true;
		idx = 3; break;
	case AMVP_ABOVE_LEFT:
		if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0 ||
			nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0)
			isOutPic = true;
		idx = 4; break;
	default:  assert(0);
	}

	if (!m_mvSpatial[idx].valid || isOutPic)
	{
		return false;
	}

	int currDeltaPoc = m_nCurrPicPoc - m_nCurrRefPicPoc[picList][refIdx];
	if ((m_mvSpatial[idx].ref_idx[picList] >= 0 && m_mvSpatial[idx].pred_flag[picList]) && currDeltaPoc == m_mvSpatial[idx].delta_poc[picList])
	{
		Mv mvp = m_mvSpatial[idx].mv[picList];
		info->m_mvCand[info->m_num++] = mvp;
		return true;
	}

	int refPicList2nd = REF_PIC_LIST0;
	if (picList == REF_PIC_LIST0)
	{
		refPicList2nd = REF_PIC_LIST1;
	}
	else if (picList == REF_PIC_LIST1)
	{
		refPicList2nd = REF_PIC_LIST0;
	}

	if (m_mvSpatial[idx].ref_idx[refPicList2nd] >= 0 && m_mvSpatial[idx].pred_flag[refPicList2nd])
	{
		int neibDeltaPOC = m_mvSpatial[idx].delta_poc[refPicList2nd];
		if (neibDeltaPOC == currDeltaPoc)
		{
			Mv mvp = m_mvSpatial[idx].mv[refPicList2nd];
			info->m_mvCand[info->m_num++] = mvp;
			return true;
		}
	}
	return false;
}

bool AmvpCand::xAddMVPCandOrder(AmvpInfo* info, int picList, int refIdx, AMVP_DIR dir)
{
	bool PicWidthNotDivCtu = m_nPicWidth / m_nCtuSize*m_nCtuSize < m_nPicWidth;
	int numCtuInPicWidth = m_nPicWidth / m_nCtuSize + (PicWidthNotDivCtu ? 1 : 0);
	int nCtuPosWidth = m_nCtuPosInPic % numCtuInPicWidth;
	int nCtuPosHeight = m_nCtuPosInPic / numCtuInPicWidth;
	bool isOutPic = false;
	int idx = 0;
	switch (dir)
	{
	case AMVP_LEFT:
		if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0)
			isOutPic = true;
		idx = 0; break;
	case AMVP_ABOVE:
		if (nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0)
			isOutPic = true;
		idx = 1; break;
	case AMVP_ABOVE_RIGHT:
		if (nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0 ||
			nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] + m_nCuSize + 4 >= m_nPicWidth)
			isOutPic = true;
		idx = 2; break;
	case AMVP_BELOW_LEFT:
		if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0 ||
			nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] + m_nCuSize + 4 >= m_nPicHeight)
			isOutPic = true;
		idx = 3; break;
	case AMVP_ABOVE_LEFT:
		if (nCtuPosWidth*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][0] - 4 < 0 ||
			nCtuPosHeight*m_nCtuSize + OffsFromCtu64[m_nCuPosInCtu][1] - 4 < 0)
			isOutPic = true;
		idx = 4; break;
	default:  assert(0);
	}

	if (!m_mvSpatial[idx].valid || isOutPic)
	{
		return false;
	}

	int refPicList2nd = REF_PIC_LIST0;
	if (picList == REF_PIC_LIST0)
	{
		refPicList2nd = REF_PIC_LIST1;
	}
	else if (picList == REF_PIC_LIST1)
	{
		refPicList2nd = REF_PIC_LIST0;
	}

	int currDeltaPoc = m_nCurrPicPoc - m_nCurrRefPicPoc[picList][refIdx];
	bool bIsCurrRefLongTerm = false;
	bool bIsNeibRefLongTerm = false;

	//---------------  V1 (END) ------------------//
	if (m_mvSpatial[idx].ref_idx[picList] >= 0 && m_mvSpatial[idx].pred_flag[picList])
	{
		int neibDeltaPoc = (int)m_mvSpatial[idx].delta_poc[picList];
		Mv mvp = m_mvSpatial[idx].mv[picList];
		Mv outMV;
		if (bIsCurrRefLongTerm == bIsNeibRefLongTerm)
		{
			if (bIsCurrRefLongTerm || bIsNeibRefLongTerm)
			{
				outMV = mvp;
			}
			else
			{
				int iScale = xGetDistScaleFactor(currDeltaPoc, neibDeltaPoc);
				if (iScale == 4096)
				{
					outMV = mvp;
				}
				else
				{
					outMV = scaleMv(mvp, iScale);
				}
			}
			info->m_mvCand[info->m_num++] = outMV;
			return true;
		}
	}

	//---------------------- V2(END) --------------------//
	if (m_mvSpatial[idx].ref_idx[refPicList2nd] >= 0 && m_mvSpatial[idx].pred_flag[refPicList2nd])
	{
		int neibDeltaPoc = (int)m_mvSpatial[idx].delta_poc[refPicList2nd];
		Mv mvp = m_mvSpatial[idx].mv[refPicList2nd];
		Mv outMV;

		if (bIsCurrRefLongTerm == bIsNeibRefLongTerm)
		{
			if (bIsCurrRefLongTerm || bIsNeibRefLongTerm)
			{
				outMV = mvp;
			}
			else
			{
				int iScale = xGetDistScaleFactor(currDeltaPoc, neibDeltaPoc);
				if (iScale == 4096)
				{
					outMV = mvp;
				}
				else
				{
					outMV = scaleMv(mvp, iScale);
				}
			}
			info->m_mvCand[info->m_num++] = outMV;
			return true;
		}
	}
	//---------------------- V3(END) --------------------//
	return false;
}

void AmvpCand::deriveRightBottomIdx(unsigned int& outPartIdxRB)
{
	outPartIdxRB = RasterToZscan[ZscanToRaster[RasterScan[m_nCuPosInCtu]] + (((m_nCuSize / 4) >> 1) - 1) * 16 + m_nCuSize / 4 - 1];
	unsigned int numPartitions = m_nCuSize*m_nCuSize / 16;
	outPartIdxRB += numPartitions >> 1;
}

void AmvpCand::xDeriveCenterIdx(unsigned int& outPartIdxCenter)
{
	outPartIdxCenter = RasterScan[m_nCuPosInCtu]; // partition origin.
	outPartIdxCenter = RasterToZscan[ZscanToRaster[outPartIdxCenter] + (m_nCuSize / 4) / 2 * 16 + (m_nCuSize / 4) / 2];
}

bool AmvpCand::hasEqualMotion(SPATIAL_MV mvCurrent, SPATIAL_MV mvCompare)
{
	if (mvCurrent.pred_flag[0] != mvCompare.pred_flag[0] || mvCurrent.pred_flag[1] != mvCompare.pred_flag[1])
	{
		return false;
	}

	for (int refListIdx = 0; refListIdx < 2; refListIdx++)
	{
		if (mvCurrent.pred_flag[refListIdx])
		{
			if (mvCurrent.mv[refListIdx].x != mvCompare.mv[refListIdx].x || mvCurrent.mv[refListIdx].y != mvCompare.mv[refListIdx].y
				|| mvCurrent.ref_idx[refListIdx] != mvCompare.ref_idx[refListIdx])
			{
				return false;
			}
		}
	}

	return true;
}

int AmvpCand::zscanToPos(unsigned int zscan)
{
	int nPos = 0;
	switch (zscan)
	{
	case 0: nPos = 0; break;
	case 16: nPos = 1; break;
	case 64: nPos = 2; break;
	case 80: nPos = 3; break;
	case 32: nPos = 4; break;
	case 48: nPos = 5; break;
	case 96: nPos = 6; break;
	case 112: nPos = 7; break;
	case 128: nPos = 8; break;
	case 144: nPos = 9; break;
	case 192: nPos = 10; break;
	case 208: nPos = 11; break;
	case 160: nPos = 12; break;
	case 176: nPos = 13; break;
	case 224: nPos = 14; break;
	case 240: nPos = 15; break;
	default: assert(0);
	}
	return nPos;
}

bool AmvpCand::xGetColMVP(int picList, int cuAddr, unsigned int partUnitIdx, Mv& outMV, int refIdx, bool isCheckLDC, int colFromL0Flag)
{
	unsigned int absPartAddr = partUnitIdx & 0xFFFFFFF0;
	int colRefPicList;
	int curPOC, curRefPOC, scale;
	Mv colmv;

	int idx = (m_nCtuPosInPic < cuAddr) ? 1 : 0;
	int nPos = zscanToPos(absPartAddr);

	if (!(m_mvTemporal[idx][nPos].pred_l0 || m_mvTemporal[idx][nPos].pred_l1))
	{
		return false; //current CTU of collaborate picture is intra
	}

	curPOC = m_nCurrPicPoc;
	curRefPOC = m_nCurrRefPicPoc[picList][refIdx];

	colRefPicList = isCheckLDC ? picList : colFromL0Flag;
	bool isListValid = colRefPicList ? m_mvTemporal[idx][nPos].pred_l1 : m_mvTemporal[idx][nPos].pred_l0;
	if (!isListValid)
	{
		colRefPicList = 1 - colRefPicList;
		isListValid = colRefPicList ? m_mvTemporal[idx][nPos].pred_l1 : m_mvTemporal[idx][nPos].pred_l0;
		if (!isListValid)
		{
			return false;
		}
	}
	colmv.x = colRefPicList ? m_mvTemporal[idx][nPos].mv_x_l1 : m_mvTemporal[idx][nPos].mv_x_l0;
	colmv.y = colRefPicList ? m_mvTemporal[idx][nPos].mv_y_l1 : m_mvTemporal[idx][nPos].mv_y_l0;
	int nDiffColPicPoc = colRefPicList ? m_mvTemporal[idx][nPos].delta_poc_l1 : m_mvTemporal[idx][nPos].delta_poc_l0;
	int nDiffCurrPicPoc = curPOC - curRefPOC;

	scale = xGetDistScaleFactor(nDiffCurrPicPoc, nDiffColPicPoc);
	if (scale == 4096)
	{
		outMV = colmv;
	}
	else
	{
		outMV = scaleMv(colmv, scale);
	}
	return true;
}

int AmvpCand::xGetDistScaleFactor(int diffPocB, int diffPocD)
{
	if (diffPocD == diffPocB)
	{
		return 4096;
	}
	else
	{
		int tdb = Clip3(-128, 127, diffPocB);
		int tdd = Clip3(-128, 127, diffPocD);
		int x = (0x4000 + abs(tdd / 2)) / tdd;
		int scale = Clip3(-4096, 4095, (tdb * x + 32) >> 6);
		return scale;
	}
}

Mv AmvpCand::scaleMv(Mv mv, int scale)
{
	int mvx = Clip3(-32768, 32767, (scale * mv.x + 127 + (scale * mv.x < 0)) >> 8);
	int mvy = Clip3(-32768, 32767, (scale * mv.y + 127 + (scale * mv.y < 0)) >> 8);
	Mv tmpMv; tmpMv.x = static_cast<short>(mvx);  tmpMv.y = static_cast<short>(mvy);
	return tmpMv;
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
	for (int i = 0; i < nMaxRefPic; i ++)
	{
		m_pCurrCuResi[i] = new short[m_nCuSize*m_nCuSize * 3 / 2];
		assert(RK_NULL != m_pCurrCuResi);
	}

	for (int i = 0; i < 3*2; i++)
	{
		m_pMergeSW[i] = new unsigned char[(m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2) * 3 / 2];
		assert(RK_NULL != m_pMergeSW[i]);
	}

	m_pMergePred = new unsigned char[m_nCuSize*m_nCuSize * 3 / 2];
	assert(RK_NULL != m_pMergePred);

	m_pMergeResi = new short[m_nCuSize*m_nCuSize*3/2];
	assert(RK_NULL != m_pMergeResi);

	m_pCurrCuPel = new unsigned char[m_nCuSize*m_nCuSize*3/2];
	assert(RK_NULL != m_pCurrCuPel);
}

void MergeProc::Destroy()
{
	for (int i = 0; i < nMaxRefPic; i++)
	{
		if (RK_NULL != m_pCurrCuResi[i])
		{
			delete[] m_pCurrCuResi[i];
			m_pCurrCuResi[i] = RK_NULL;
		}
	}

	for (int i = 0; i < 3*2; i++)
	{
		if (RK_NULL != m_pMergeSW[i])
		{
			delete[] m_pMergeSW[i];
			m_pMergeSW[i] = RK_NULL;
		}
	}
	
	if (RK_NULL != m_pMergePred)
	{
		delete[] m_pMergePred;
		m_pMergePred = RK_NULL;
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

void MergeProc::setMergeCand(MvField mvMergeCand[6])
{
	for (int i = 0; i < 6; i ++)
	{
		m_mvMergeCand[i].mv.x = mvMergeCand[i].mv.x;
		m_mvMergeCand[i].mv.y = mvMergeCand[i].mv.y;
		m_mvMergeCand[i].refIdx = mvMergeCand[i].refIdx;
	}
}

void MergeProc::setNeighPmv(Mv mvNeighPmv[nMaxRefPic*2])
{
	for (int nRefPicIdx = 0; nRefPicIdx < nMaxRefPic; nRefPicIdx ++)
	{
		m_mvNeighPmv[nRefPicIdx * 2 + 0].x = mvNeighPmv[nRefPicIdx * 2 + 0].x;
		m_mvNeighPmv[nRefPicIdx * 2 + 0].y = mvNeighPmv[nRefPicIdx * 2 + 0].y;
		m_mvNeighPmv[nRefPicIdx * 2 + 1].x = mvNeighPmv[nRefPicIdx * 2 + 1].x;
		m_mvNeighPmv[nRefPicIdx * 2 + 1].y = mvNeighPmv[nRefPicIdx * 2 + 1].y;
	}
}

void MergeProc::PrefetchMergeSW(unsigned char *src_prev0, int stride_prev0, unsigned char *src_prev1, int stride_prev1,
	unsigned char *src_post0, int stride_post0, unsigned char *src_post1, int stride_post1, int nMergeIdx)
{
	int nCtu = nCtuSize;
	Mv mvmin[4], mvmax[4];
	short Width = static_cast<short>(nRimeWidth);
	short Height = static_cast<short>(nRimeHeight);
	short offsX = OffsFromCtu64[m_nCuPosInCtu][0];
	short offsY = OffsFromCtu64[m_nCuPosInCtu][1];

	int refIdx0 = m_mvMergeCand[nMergeIdx * 2].refIdx;
	int refIdx1 = m_mvMergeCand[nMergeIdx * 2 + 1].refIdx;
	int IdxList1 = nMaxRefPic / 2 + refIdx1;
	for (int idx = 0; idx < 2; idx++) 
	{
		mvmin[idx].x = m_mvNeighPmv[refIdx0*2+idx].x - offsX - Width;//two rime search windows of prev direction
		mvmin[idx].y = m_mvNeighPmv[refIdx0*2+idx].y - offsY - Height;
		mvmax[idx].x = m_mvNeighPmv[refIdx0*2+idx].x + nCtu + Width - offsX - static_cast<unsigned char>(m_nCuSize);
		mvmax[idx].y = m_mvNeighPmv[refIdx0*2+idx].y + nCtu + Height - offsY - static_cast<unsigned char>(m_nCuSize);
		if (refIdx1 >= 0)
		{
			mvmin[idx + 2].x = m_mvNeighPmv[IdxList1 * 2 + idx].x - offsX - Width;//two rime search windows of post direction
			mvmin[idx + 2].y = m_mvNeighPmv[IdxList1 * 2 + idx].y - offsY - Height;
			mvmax[idx + 2].y = m_mvNeighPmv[IdxList1 * 2 + idx].y + nCtu + Height - offsY - static_cast<unsigned char>(m_nCuSize);
			mvmax[idx + 2].x = m_mvNeighPmv[IdxList1 * 2 + idx].x + nCtu + Width - offsX - static_cast<unsigned char>(m_nCuSize);
		}
	}
	bool isAddPrev = false;
	//first rime search window of prev direction
	Mv tmpMv = m_mvMergeCand[nMergeIdx*2].mv; 
	if (refIdx0 >= 0 && ((tmpMv.x >> 2) >= mvmin[0].x && (tmpMv.y >> 2) >= mvmin[0].y) &&	
		((tmpMv.x >> 2) <= mvmax[0].x && (tmpMv.y >> 2) <= mvmax[0].y))
	{
		short offset_x = (tmpMv.x >> 2) - m_mvNeighPmv[refIdx0 * 2 + 0].x + offsX + Width;
		short offset_y = (tmpMv.y >> 2) - m_mvNeighPmv[refIdx0 * 2 + 0].y + offsY + Height;
		unsigned char *pNeigbPmv0 = src_prev0 + offset_x + offset_y*stride_prev0;
		for (int i = 0; i < (m_nCuSize + 8); i++)
		{
			for (int j = 0; j < (m_nCuSize + 8); j++)
			{
				m_pMergeSW[nMergeIdx*2+0][j + i*(m_nCuSize + 8)] = pNeigbPmv0[j + i*stride_prev0];
			}
		}
		int offset = (nCtu + 2 * (nRimeWidth + 4))*(nCtu + 2 * (nRimeHeight + 4));
		offset_x = (tmpMv.x >> 3) * 2 - m_mvNeighPmv[refIdx0 * 2 + 0].x + offsX + Width;
		offset_y = (tmpMv.y >> 3) - m_mvNeighPmv[refIdx0 * 2 + 0].y / 2 + offsY / 2 + Height / 2;
		pNeigbPmv0 = src_prev0 + offset;
		pNeigbPmv0 += offset_x + offset_y*stride_prev0;
		unsigned char *pMergeSw = m_pMergeSW[nMergeIdx*2+0] + (m_nCuSize + 8)*(m_nCuSize + 8);
		for (int i = 0; i < (m_nCuSize + 8) / 2; i++)
		{
			for (int j = 0; j < (m_nCuSize + 8); j++)
			{
				pMergeSw[j + i*(m_nCuSize + 8)] = pNeigbPmv0[j + i*stride_prev0];
			}
		}

		isAddPrev = true;
	}

	//second rime search window of prev direction
    tmpMv = m_mvMergeCand[nMergeIdx * 2].mv;
	if (!isAddPrev && refIdx0 >= 0 && ((tmpMv.x >> 2) >= mvmin[1].x && (tmpMv.y >> 2) >= mvmin[1].y) &&
		((tmpMv.x >> 2) <= mvmax[1].x && (tmpMv.y >> 2) <= mvmax[1].y))
	{
		short offset_x = (tmpMv.x >> 2) - m_mvNeighPmv[refIdx0 * 2 + 1].x + offsX + Width;
		short offset_y = (tmpMv.y >> 2) - m_mvNeighPmv[refIdx0 * 2 + 1].y + offsY + Height;
		unsigned char *pNeigbPmv1 = src_prev1 + offset_x + offset_y*stride_prev1;
		for (int i = 0; i < (m_nCuSize + 8); i++)
		{
			for (int j = 0; j < (m_nCuSize + 8); j++)
			{
				m_pMergeSW[nMergeIdx*2+0][j + i*(m_nCuSize + 8)] = pNeigbPmv1[j + i*stride_prev1];
			}
		}
		int offset = (nCtu + 2 * (nRimeWidth + 4))*(nCtu + 2 * (nRimeHeight + 4));
		offset_x = (tmpMv.x >> 3) * 2 - m_mvNeighPmv[refIdx0 * 2 + 1].x + offsX + Width;
		offset_y = (tmpMv.y >> 3) - m_mvNeighPmv[refIdx0 * 2 + 1].y / 2 + offsY / 2 + Height / 2;
		pNeigbPmv1 = src_prev1 + offset;
		pNeigbPmv1 += offset_x + offset_y*stride_prev1;
		unsigned char *pMergeSw = m_pMergeSW[nMergeIdx*2+0] + (m_nCuSize + 8)*(m_nCuSize + 8);
		for (int i = 0; i < (m_nCuSize + 8) / 2; i++)
		{
			for (int j = 0; j < (m_nCuSize + 8); j++)
			{
				pMergeSw[j + i*(m_nCuSize + 8)] = pNeigbPmv1[j + i*stride_prev0];
			}
		}

		isAddPrev = true;
	}

	//first rime search window of post direction
	bool isAddPost = false;
	tmpMv = m_mvMergeCand[nMergeIdx * 2 + 1].mv;
	if (refIdx1 >= 0 && ((tmpMv.x >> 2) >= mvmin[2].x && (tmpMv.y >> 2) >= mvmin[2].y) && 
		((tmpMv.x >> 2) <= mvmax[2].x && (tmpMv.y >> 2) <= mvmax[2].y))
	{
		short offset_x = (tmpMv.x >> 2) - m_mvNeighPmv[IdxList1 * 2 + 0].x + offsX + Width;
		short offset_y = (tmpMv.y >> 2) - m_mvNeighPmv[IdxList1 * 2 + 0].y + offsY + Height;
		unsigned char *pNeigbPmv0 = src_post0 + offset_x + offset_y*stride_post0;
		for (int i = 0; i < (m_nCuSize + 8); i++)
		{
			for (int j = 0; j < (m_nCuSize + 8); j++)
			{
				m_pMergeSW[nMergeIdx*2+1][j + i*(m_nCuSize + 8)] = pNeigbPmv0[j + i*stride_post0];
			}
		}
		int offset = (nCtu + 2 * (nRimeWidth + 4))*(nCtu + 2 * (nRimeHeight + 4));
		offset_x = (tmpMv.x >> 3) * 2 - m_mvNeighPmv[IdxList1 * 2 + 0].x + offsX + Width;
		offset_y = (tmpMv.y >> 3) - m_mvNeighPmv[IdxList1 * 2 + 0].y / 2 + offsY / 2 + Height / 2;
		pNeigbPmv0 = src_post0 + offset;
		pNeigbPmv0 += offset_x + offset_y*stride_post0;
		unsigned char *pMergeSw = m_pMergeSW[nMergeIdx*2+1] + (m_nCuSize + 8)*(m_nCuSize + 8);
		for (int i = 0; i < (m_nCuSize + 8) / 2; i++)
		{
			for (int j = 0; j < (m_nCuSize + 8); j++)
			{
				pMergeSw[j + i*(m_nCuSize + 8)] = pNeigbPmv0[j + i*stride_post0];
			}
		}

		isAddPost = true;
	}

	//second rime search window of post direction
	tmpMv = m_mvMergeCand[nMergeIdx * 2 + 1].mv;
	if (!isAddPost && refIdx1 >= 0 && ((tmpMv.x >> 2) >= mvmin[3].x && (tmpMv.y >> 2) >= mvmin[3].y) &&
		((tmpMv.x >> 2) <= mvmax[3].x && (tmpMv.y >> 2) <= mvmax[3].y))
	{
		short offset_x = (tmpMv.x >> 2) - m_mvNeighPmv[IdxList1 * 2 + 1].x + offsX + Width;
		short offset_y = (tmpMv.y >> 2) - m_mvNeighPmv[IdxList1 * 2 + 1].y + offsY + Height;
		unsigned char *pNeigbPmv1 = src_post1 + offset_x + offset_y*stride_post1;
		for (int i = 0; i < (m_nCuSize + 8); i++)
		{
			for (int j = 0; j < (m_nCuSize + 8); j++)
			{
				m_pMergeSW[nMergeIdx*2+1][j + i*(m_nCuSize + 8)] = pNeigbPmv1[j + i*stride_post1];
			}
		}
		int offset = (nCtu + 2 * (nRimeWidth + 4))*(nCtu + 2 * (nRimeHeight + 4));
		offset_x = (tmpMv.x >> 3) * 2 - m_mvNeighPmv[IdxList1 * 2 + 1].x + offsX + Width;
		offset_y = (tmpMv.y >> 3) - m_mvNeighPmv[IdxList1 * 2 + 1].y / 2 + offsY / 2 + Height / 2;
		pNeigbPmv1 = src_post1 + offset;
		pNeigbPmv1 += offset_x + offset_y*stride_post1;
		unsigned char *pMergeSw = m_pMergeSW[nMergeIdx*2+1] + (m_nCuSize + 8)*(m_nCuSize + 8);
		for (int i = 0; i < (m_nCuSize + 8) / 2; i++)
		{
			for (int j = 0; j < (m_nCuSize + 8); j++)
			{
				pMergeSw[j + i*(m_nCuSize + 8)] = pNeigbPmv1[j + i*stride_post1];
			}
		}

		isAddPost = true;
	}

	m_bMergeSwValid[nMergeIdx] = false;
	if (refIdx0 >= 0 && refIdx1 >= 0)
	{
		if (isAddPrev && isAddPost)
			m_bMergeSwValid[nMergeIdx] = true;
	}
	else if (refIdx0 >= 0)
	{
		if (isAddPrev)
			m_bMergeSwValid[nMergeIdx] = true;
	}
	else if (refIdx1 >= 0)
	{
		if (isAddPost)
			m_bMergeSwValid[nMergeIdx] = true;
	}
	else assert(false);
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
	unsigned char *pCurrCuPel = m_pCurrCuPel + nSize*nSize;
	unsigned char *pSrc = src + nSize*nSize;
	for (int i = 0; i < nSize / 2; i++)
	{
		for (int j = 0; j < nSize; j++)
		{
			pCurrCuPel[j + i*nSize] = pSrc[j + i*nSize];
		}
	}
}

void MergeProc::CalcMergeResi()
{
	unsigned char *pMergePred = new unsigned char[m_nCuSize*m_nCuSize];
	int bCost = INT_MAX_MINE;
	int nBestMergeCand = 0;
	for (int nMergeCand = 0; nMergeCand < 3; nMergeCand ++)
	{
		if (!m_bMergeSwValid[nMergeCand])
			continue;
		int refIdx0 = m_mvMergeCand[nMergeCand * 2].refIdx;
		int refIdx1 = m_mvMergeCand[nMergeCand * 2 + 1].refIdx;
		Mv mv0 = m_mvMergeCand[nMergeCand * 2].mv;
		Mv mv1 = m_mvMergeCand[nMergeCand * 2 + 1].mv;
		if (refIdx0 >= 0 && refIdx1 >= 0)
		{
			xPredInterBi(nMergeCand, 0, mv0, true, false);
			xPredInterBi(nMergeCand, 1, mv1, true, false);
			addAvg(pMergePred, nMergeCand, true, false);
		}
		else if (refIdx0 >= 0)
		{
			InterpLuma(m_pMergeSW[nMergeCand * 2 + 0], m_nCuSize + 8, pMergePred, m_nCuSize, mv0);
		}
		else if (refIdx1 >= 0)
		{
			InterpLuma(m_pMergeSW[nMergeCand * 2 + 1], m_nCuSize + 8, pMergePred, m_nCuSize, mv1);
		}
		else assert(false);

		int cost = SAD(m_pCurrCuPel, m_nCuSize, pMergePred, m_nCuSize, m_nCuSize, m_nCuSize);
		if (cost < bCost)
		{
			nBestMergeCand = nMergeCand;
			bCost = cost;
			for (int i = 0; i < m_nCuSize; i ++)
			{
				for (int j = 0; j < m_nCuSize; j++)
				{
					m_pMergeResi[j + i*m_nCuSize] = static_cast<short>(m_pCurrCuPel[j + i*m_nCuSize] - pMergePred[j + i * m_nCuSize]);
					m_pMergePred[j + i*m_nCuSize] = pMergePred[j + i * m_nCuSize];
				}
			}
		}
	}
	m_mergeInfoForCabac.m_mergeIdx = nBestMergeCand;
	m_mergeInfoForCabac.m_bSkipFlag = false;
	delete[] pMergePred; pMergePred = RK_NULL;
	if (!m_bMergeSwValid[nBestMergeCand])
		return;

	int refIdx0 = m_mvMergeCand[nBestMergeCand * 2].refIdx;
	int refIdx1 = m_mvMergeCand[nBestMergeCand * 2 + 1].refIdx;
	Mv mv0 = m_mvMergeCand[nBestMergeCand * 2].mv;
	Mv mv1 = m_mvMergeCand[nBestMergeCand * 2 + 1].mv;
	pMergePred = new unsigned char[m_nCuSize*m_nCuSize / 4];
	unsigned char *pCurrCuPel = m_pCurrCuPel + m_nCuSize*m_nCuSize;
	if (refIdx0 >= 0 && refIdx1 >= 0)
	{
		xPredInterBi(nBestMergeCand, 0, mv0, false, true);
		xPredInterBi(nBestMergeCand, 1, mv1, false, true);
		addAvg(pMergePred, nBestMergeCand, false, true);
	}
	else if (refIdx0 >= 0)
	{
		InterpChroma(pMergePred, m_nCuSize / 2, mv0, true, nBestMergeCand, 0);
	}
	else if (refIdx1 >= 0)
	{
		InterpChroma(pMergePred, m_nCuSize / 2, mv1, true, nBestMergeCand, 1);
	}
	else assert(false);
	//InterpChroma(pMergePred, m_nCuSize/2, m_mvMergeCand[nBestMergeCand], true, nBestMergeCand);
	for (int i = 0; i < m_nCuSize/2; i++)
	{
		for (int j = 0; j < m_nCuSize/2; j++)
		{
			m_pMergeResi[j + i*m_nCuSize / 2 + m_nCuSize*m_nCuSize] = static_cast<short>(pCurrCuPel[j * 2 + 0 + i*m_nCuSize] - pMergePred[j + i * m_nCuSize / 2]);
			m_pMergePred[j + i*m_nCuSize / 2 + m_nCuSize*m_nCuSize] = pMergePred[j + i * m_nCuSize / 2];
		}
	}
	pCurrCuPel = m_pCurrCuPel + m_nCuSize*m_nCuSize;
	if (refIdx0 >= 0 && refIdx1 >= 0)
	{
		xPredInterBi(nBestMergeCand, 0, mv0, false, false);
		xPredInterBi(nBestMergeCand, 1, mv1, false, false);
		addAvg(pMergePred, nBestMergeCand, false, false);
	}
	else if (refIdx0 >= 0)
	{
		InterpChroma(pMergePred, m_nCuSize / 2, mv0, false, nBestMergeCand, 0);
	}
	else if (refIdx1 >= 0)
	{
		InterpChroma(pMergePred, m_nCuSize / 2, mv1, false, nBestMergeCand, 1);
	}
	else assert(false);
	//InterpChroma(pMergePred, m_nCuSize/2, m_mvMergeCand[nBestMergeCand], false, nBestMergeCand);
	for (int i = 0; i < m_nCuSize / 2; i++)
	{
		for (int j = 0; j < m_nCuSize / 2; j++)
		{
			m_pMergeResi[j + i*m_nCuSize / 2 + m_nCuSize*m_nCuSize * 5 / 4] = static_cast<short>(pCurrCuPel[j * 2 + 1 + i*m_nCuSize] - pMergePred[j + i * m_nCuSize / 2]);
			m_pMergePred[j + i*m_nCuSize / 2 + m_nCuSize*m_nCuSize * 5 / 4] = pMergePred[j + i * m_nCuSize / 2];
		}
	}

	delete[] pMergePred; pMergePred = RK_NULL;
}

void MergeProc::addAvg(unsigned char*pBestPred, int nMergeIdx, bool bLuma, bool isCr)
{
	int x, y;
	unsigned int src0Stride, src1Stride, dststride;
	int shiftNum, offset;

	short* SrcY0 = new short[m_nCuSize*m_nCuSize]; short *srcY0 = SrcY0;
	short* SrcU0 = new short[m_nCuSize*m_nCuSize / 4]; short* srcU0 = SrcU0;
	short* SrcV0 = new short[m_nCuSize*m_nCuSize / 4]; short* srcV0 = SrcV0;

	short* SrcY1 = new short[m_nCuSize*m_nCuSize]; short *srcY1 = SrcY1;
	short* SrcU1 = new short[m_nCuSize*m_nCuSize / 4]; short *srcU1 = SrcU1;
	short* SrcV1 = new short[m_nCuSize*m_nCuSize / 4]; short *srcV1 = SrcV1;

	unsigned char* DstY = new unsigned char[m_nCuSize*m_nCuSize]; unsigned char *dstY = DstY;
	unsigned char* DstU = new unsigned char[m_nCuSize*m_nCuSize / 4]; unsigned char *dstU = DstU;
	unsigned char* DstV = new unsigned char[m_nCuSize*m_nCuSize / 4]; unsigned char *dstV = DstV;

	for (int i = 0; i < m_nCuSize; i++)
	{
		for (int j = 0; j < m_nCuSize; j++)
		{
			srcY0[j + i*m_nCuSize] = m_pCurrCuResi[nMergeIdx * 2 + 0][j + i*m_nCuSize];
			srcY1[j + i*m_nCuSize] = m_pCurrCuResi[nMergeIdx * 2 + 1][j + i*m_nCuSize];
		}
	}
	for (int i = 0; i < m_nCuSize / 2; i++)
	{
		for (int j = 0; j < m_nCuSize / 2; j++)
		{
			srcU0[j + i*m_nCuSize / 2] = m_pCurrCuResi[nMergeIdx * 2 + 0][j + i*m_nCuSize / 2 + m_nCuSize*m_nCuSize];
			srcU1[j + i*m_nCuSize / 2] = m_pCurrCuResi[nMergeIdx * 2 + 1][j + i*m_nCuSize / 2 + m_nCuSize*m_nCuSize];
			srcV0[j + i*m_nCuSize / 2] = m_pCurrCuResi[nMergeIdx * 2 + 0][j + i*m_nCuSize / 2 + m_nCuSize*m_nCuSize * 5 / 4];
			srcV1[j + i*m_nCuSize / 2] = m_pCurrCuResi[nMergeIdx * 2 + 1][j + i*m_nCuSize / 2 + m_nCuSize*m_nCuSize * 5 / 4];
		}
	}

	if (bLuma)
	{
		src0Stride = src1Stride = dststride = m_nCuSize;
		shiftNum = INTERNAL_PREC + 1 - DEPTH;
		offset = (1 << (shiftNum - 1)) + 2 * INTERNAL_OFFS;

		for (y = 0; y < m_nCuSize; y++)
		{
			for (x = 0; x < m_nCuSize; x += 4)
			{
				dstY[x + 0] = Clip((srcY0[x + 0] + srcY1[x + 0] + offset) >> shiftNum);
				dstY[x + 1] = Clip((srcY0[x + 1] + srcY1[x + 1] + offset) >> shiftNum);
				dstY[x + 2] = Clip((srcY0[x + 2] + srcY1[x + 2] + offset) >> shiftNum);
				dstY[x + 3] = Clip((srcY0[x + 3] + srcY1[x + 3] + offset) >> shiftNum);
			}

			srcY0 += src0Stride;
			srcY1 += src1Stride;
			dstY += dststride;
		}
	}
	else
	{
		shiftNum = INTERNAL_PREC + 1 - DEPTH;
		offset = (1 << (shiftNum - 1)) + 2 * INTERNAL_OFFS;
		src0Stride = src1Stride = dststride = m_nCuSize / 2;

		for (y = m_nCuSize / 2 - 1; y >= 0; y--)
		{
			for (x = m_nCuSize / 2 - 1; x >= 0;)
			{
				// note: chroma min width is 2
				dstU[x] = Clip((srcU0[x] + srcU1[x] + offset) >> shiftNum);
				dstV[x] = Clip((srcV0[x] + srcV1[x] + offset) >> shiftNum);
				x--;
				dstU[x] = Clip((srcU0[x] + srcU1[x] + offset) >> shiftNum);
				dstV[x] = Clip((srcV0[x] + srcV1[x] + offset) >> shiftNum);
				x--;
			}

			srcU0 += src0Stride;
			srcU1 += src1Stride;
			srcV0 += src0Stride;
			srcV1 += src1Stride;
			dstU += dststride;
			dstV += dststride;
		}
	}
	if (bLuma)
	{
		for (int i = 0; i < m_nCuSize; i++)
		{
			for (int j = 0; j < m_nCuSize; j++)
			{
				pBestPred[j + i*m_nCuSize] = DstY[j + i*m_nCuSize];
			}
		}
	}
	else
	{
		if (isCr)
			dstU = DstU;
		else
			dstU = DstV;
		for (int i = 0; i < m_nCuSize / 2; i++)
		{
			for (int j = 0; j < m_nCuSize / 2; j++)
			{
				pBestPred[j + i*m_nCuSize / 2] = dstU[j + i*m_nCuSize / 2];
			}
		}
	}

	delete[] SrcY0;
	delete[] SrcY1;
	delete[] SrcU0;
	delete[] SrcU1;
	delete[] SrcV0;
	delete[] SrcV1;
	delete[] DstY;
	delete[] DstU;
	delete[] DstV;
}

void MergeProc::xPredInterBi(int nMergeIdx, int add, Mv qmv, bool isLuma, bool isCr)
{
	int refStride = m_nCuSize + 8;
	int xFrac = qmv.x & 0x3;
	int yFrac = qmv.y & 0x3;
	int N = 8;
	int offset = 0;
	int nCuSize = m_nCuSize;
	unsigned char *pCurrCuForInterp = RK_NULL;
	if (!isLuma)
	{
		N /= 2;
		refStride /= 2;
		nCuSize /= 2;
		int offs = 0;
		xFrac = qmv.x & 0x7;
		yFrac = qmv.y & 0x7;
		if (isCr)
		{
			offset = m_nCuSize*m_nCuSize;
		}
		else
		{
			offs = 1;
			offset = m_nCuSize*m_nCuSize * 5 / 4;
		}
		int Offset = (m_nCuSize + 8)*(m_nCuSize + 8);
		pCurrCuForInterp = new unsigned char[(m_nCuSize + 8)*(m_nCuSize + 8) / 4];
		for (int i = 0; i < (m_nCuSize + 8) / 2; i++)
		{
			for (int j = 0; j < (m_nCuSize + 8) / 2; j++)
			{
				pCurrCuForInterp[j + i*(m_nCuSize + 8) / 2] = m_pMergeSW[nMergeIdx*2+add][Offset + j * 2 + offs + i*(m_nCuSize + 8)];
			}
		}
	}
	else
	{
		pCurrCuForInterp = new unsigned char[(m_nCuSize + 8)*(m_nCuSize + 8)];
		for (int i = 0; i < (m_nCuSize + 8); i++)
		{
			for (int j = 0; j < (m_nCuSize + 8); j++)
			{
				pCurrCuForInterp[j + i*(m_nCuSize + 8)] = m_pMergeSW[nMergeIdx*2+add][j + i*(m_nCuSize + 8)];
			}
		}
	}

	pCurrCuForInterp += N / 2 + N / 2 * refStride;
	if ((yFrac | xFrac) == 0)
	{
		ConvertPelToShortBi(pCurrCuForInterp, refStride, m_pCurrCuResi[nMergeIdx*2+add] + offset, nCuSize, nCuSize, nCuSize);
		//primitives.luma_p2s(ref, refStride, dst, width, height);
	}
	else if (yFrac == 0)
	{
		InterpHorizonBi(pCurrCuForInterp, refStride, m_pCurrCuResi[nMergeIdx * 2 + add] + offset, nCuSize, xFrac, 0, N, nCuSize, nCuSize);
		//primitives.luma_hps[partEnum](ref, refStride, dst, dstStride, xFrac, 0);
	}
	else if (xFrac == 0)
	{
		short *pFilter = LumaFilter[yFrac];
		if (!isLuma)
			pFilter = ChromaFilter[yFrac];
		InterpVerticalBi(pCurrCuForInterp, refStride, m_pCurrCuResi[nMergeIdx * 2 + add] + offset, nCuSize, nCuSize, nCuSize, pFilter, N);
		//primitives.ipfilter_ps[FILTER_V_P_S_8](ref, refStride, dst, dstStride, width, height, g_lumaFilter[yFrac]);
	}
	else
	{
		int tmpStride = nCuSize;
		int filterSize = LUMA_NTAPS;
		if (!isLuma) filterSize = CHROMA_NTAPS;
		int halfFilterSize = (filterSize >> 1);
		short immedVals[64 * (64 + LUMA_NTAPS - 1)];
		InterpHorizonBi(pCurrCuForInterp, refStride, immedVals, tmpStride, xFrac, 1, N, nCuSize, nCuSize);
		FilterVerticalBi(immedVals + (halfFilterSize - 1) * tmpStride, tmpStride, m_pCurrCuResi[nMergeIdx * 2 + add] + offset, nCuSize, nCuSize, nCuSize, yFrac, N);
		//primitives.luma_hps[partEnum](ref, refStride, m_immedVals, tmpStride, xFrac, 1);
		//primitives.ipfilter_ss[FILTER_V_S_S_8](m_immedVals + (halfFilterSize - 1) * tmpStride, tmpStride, dst, dstStride, width, height, yFrac);
	}
	pCurrCuForInterp -= N / 2 + N / 2 * refStride;
	delete[] pCurrCuForInterp;
}

void MergeProc::ConvertPelToShortBi(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height)
{//x = 0, y = 0
	int shift = INTERNAL_PREC - DEPTH;
	int row, col;

	for (row = 0; row < height; row++)
	{
		for (col = 0; col < width; col++)
		{
			short val = src[col] << shift;
			dst[col] = val - (short)INTERNAL_OFFS;
		}

		src += srcStride;
		dst += dstStride;
	}
}

void MergeProc::InterpHorizonBi(unsigned char *src, int srcStride, short *dst, short dstStride, int coeffIdx, int isRowExt, int N, int width, int height)
{//x !=0 y = 0  or  x !=0 y != 0
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

void MergeProc::InterpVerticalBi(unsigned char *src, int srcStride, short *dst, int dstStride, int width, int height, short const *c, int N)
{//x = 0, y != 0
	int headRoom = INTERNAL_PREC - DEPTH;
	int shift = FILTER_PREC - headRoom;
	int offset = -INTERNAL_OFFS << shift;

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
			dst[col] = val;
		}

		src += srcStride;
		dst += dstStride;
	}
}

void MergeProc::FilterVerticalBi(short *src, int srcStride, short *dst, int dstStride, int width, int height, const int coefIdx, int N)
{//x !=0 y != 0
	const short *const c = (N == 8 ? LumaFilter[coefIdx] : ChromaFilter[coefIdx]);
	int shift = FILTER_PREC;
	int row, col;

	src -= (N / 2 - 1) * srcStride;
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

			short val = (short)((sum) >> shift);
			dst[col] = val;
		}

		src += srcStride;
		dst += dstStride;
	}
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

void MergeProc::InterpChroma(unsigned char *pFmeInterp, int stride_2, const Mv& qmv, bool isCb, int nBestMerge, int dir, int N)
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
			pMergeInterpPel[j + i*stride_1] = m_pMergeSW[nBestMerge*2+dir][j * 2 + offs + i*stride_1 * 2 + (m_nCuSize + 4 * 2)*(m_nCuSize + 4 * 2)];
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
