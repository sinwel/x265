/** \file     hevcQT.cpp
\brief    transform and quantization class
*/
#include "qt.h"
#include <assert.h>
#include <cstdlib>
#include <math.h>
#include <memory.h>


extern FILE* g_fp_TQ_LOG_HWC_INTRA;
extern FILE* g_fp_TQ_LOG_HWC_ME;
//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Macros
// ====================================================================================================================

// debug used

#define T_LOG_EN_4 0
#define T_LOG_EN_16 0
#define T_LOG_EN_32 0
#define IT_LOG_EN_4 0

// ====================================================================================================================
// tables definition
// ====================================================================================================================
int hevcQuantScales[6] =
{
	26214, 23302, 20560, 18396, 16384, 14564
};
int hevcInvQuantScales[6] =
{
	40, 45, 51, 57, 64, 72
};
int hevcSizeConvertToBit[65] =
{
	-1, -1, -1, -1, 0,											   //size=4, bit=0
	-1, -1, -1, 1,												   //size=8, bit=1
	-1, -1, -1, -1, -1, -1, -1, 2,								   //size=16,bit=2
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3,//size=32,bit=3
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4 //size=64,bit=4
};

uint8_t ChromaScale[70] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 29, 30, 31, 32,
	33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 39, 40, 41, 42, 43, 44,
	45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
	62, 63
};
// ====================================================================================================================
// hevcQT class member functions
// ====================================================================================================================

hevcQT::hevcQT()
{
	// malloc for m_infoForQT

	m_infoForQT = new infoForQT;
	m_infoForQT->inResi = new int16_t[CTU_SIZE*CTU_SIZE];
	m_infoForQT->outResi = new int16_t[CTU_SIZE*CTU_SIZE];
	m_infoForQT->coeffT = new int32_t[CTU_SIZE*CTU_SIZE];
	m_infoForQT->coeffTQ = new int32_t[CTU_SIZE*CTU_SIZE];
	m_infoForQT->coeffTQIQ = new int32_t[CTU_SIZE*CTU_SIZE];

#if TQ_LOG_IN_X265_INTRA
	m_fp_TQ_LOG_X265_INTRA= fopen(PATH_NAME("TQ_LOG_X265_INTRA.txt"), "w+");
	if (m_fp_TQ_LOG_X265_INTRA == NULL)
	{
		RK_HEVC_PRINT("creat TQ_LOG_X265_INTRA file failed.\n");
	}	
#endif	

#if TQ_LOG_IN_X265_ME
	m_fp_TQ_LOG_X265_ME= fopen(PATH_NAME("TQ_LOG_X265_ME.txt"), "w+");
	if (m_fp_TQ_LOG_X265_ME == NULL)
	{
		RK_HEVC_PRINT("creat TQ_LOG_X265_ME file failed.\n");
	}	
#endif
	m_infoFromX265 = new infoFromX265;
	m_infoFromX265->coeffT = new int32_t[CTU_SIZE*CTU_SIZE];
	m_infoFromX265->inResi = new int16_t[CTU_SIZE*CTU_SIZE];
	m_infoFromX265->outResi = new int16_t[CTU_SIZE*CTU_SIZE];
	m_infoFromX265->coeffTQ = new int32_t[CTU_SIZE*CTU_SIZE];
}

hevcQT::~hevcQT()
{
	/*NOTE: delete member pointers first*/
	// free for m_infoForQT
	delete[] m_infoForQT->coeffT;				m_infoForQT->coeffT = NULL;
	delete[] m_infoForQT->coeffTQ;				m_infoForQT->coeffTQ = NULL;
	delete[] m_infoForQT->coeffTQIQ;			m_infoForQT->coeffTQIQ = NULL;
	delete[] m_infoForQT->inResi;				m_infoForQT->inResi = NULL;
	delete[] m_infoForQT->outResi;				m_infoForQT->outResi = NULL;
	delete   m_infoForQT;						m_infoForQT = NULL;

#if TQ_LOG_IN_X265_INTRA
	if (m_fp_TQ_LOG_X265_INTRA)
	{
		fclose(m_fp_TQ_LOG_X265_INTRA); m_fp_TQ_LOG_X265_INTRA = NULL;
	}
#endif

#if TQ_LOG_IN_X265_ME
	if (m_fp_TQ_LOG_X265_ME)
	{
		fclose(m_fp_TQ_LOG_X265_ME); m_fp_TQ_LOG_X265_ME = NULL;
	}
#endif

	delete[] m_infoFromX265->coeffT;			m_infoFromX265->coeffT = NULL;
	delete[] m_infoFromX265->coeffTQ;			m_infoFromX265->coeffTQ = NULL;
	delete[] m_infoFromX265->inResi;			m_infoFromX265->inResi = NULL;
	delete[] m_infoFromX265->outResi;			m_infoFromX265->outResi = NULL;
	delete   m_infoFromX265;					m_infoFromX265 = NULL;
}


/* tool functions */
// transpose, only  for array width=height
void hevcQT::transpose(short* inArray, int size)
{
	short* temp = (short*)malloc(size*size*sizeof(short));
	for (int j = 0; j < size; j++)
	{
		for (int i = 0; i < size; i++)
		{
			temp[j * size + i] = inArray[i * size + j];
		}
	}
	memcpy(inArray, temp, size * size * sizeof(short));
	free(temp);

}

void hevcQT::fillResi(short* resi, int val, uint32_t resiStride, uint32_t tuSize)
{
	if (val == 0) // clear resi
	{
		for (uint32_t k = 0; k < tuSize; k++)
		{
			memset(&(resi[k*resiStride]), 0, tuSize*sizeof(short));
		}
	}
	else // dc val
	{
		for (uint32_t k = 0; k < tuSize; k++)
		{
			for (uint32_t j = 0; j < tuSize; j++)
			{
				resi[k*resiStride + j] = (short)val;
			}
		}
	}
}

// inverse transform 4x4 block
void hevcQT::it4(short *out, short *in, int shift, int trType, int dim)
{
	int k;
	int y0, y1, y2, y3;
	int(*table)[4];
	assert((trType == 0) || (trType == 1));
	assert((dim == 1) || (dim == 2));

	if (trType == 0)	/*IDCT*/
	{
		table = idct_4x4_coeff;
	}
	else	/*IDST*/
	{
		table = idst_4x4_coeff;
	}

	for (k = 0; k<4; k++)
	{
		// in: [k]th row
		y0 = table[0][0] * in[4 * k + 0] + table[0][1] * in[4 * k + 1] + table[0][2] * in[4 * k + 2] + table[0][3] * in[4 * k + 3];	// in[k][0~3]
		y1 = table[1][0] * in[4 * k + 0] + table[1][1] * in[4 * k + 1] + table[1][2] * in[4 * k + 2] + table[1][3] * in[4 * k + 3];	// in[k][0~3]
		y2 = table[2][0] * in[4 * k + 0] + table[2][1] * in[4 * k + 1] + table[2][2] * in[4 * k + 2] + table[2][3] * in[4 * k + 3];	// in[k][0~3]
		y3 = table[3][0] * in[4 * k + 0] + table[3][1] * in[4 * k + 1] + table[3][2] * in[4 * k + 2] + table[3][3] * in[4 * k + 3]; // in[k][0~3]

		if (dim == 1)
		{
			// out: [k]th col
			out[0 * 4 + k] = (short)CLIP((y0 + (1 << (shift - 1))) >> shift, -32768, 32767); // out[0][k]
			out[1 * 4 + k] = (short)CLIP((y1 + (1 << (shift - 1))) >> shift, -32768, 32767); // out[1][k]
			out[2 * 4 + k] = (short)CLIP((y2 + (1 << (shift - 1))) >> shift, -32768, 32767); // out[2][k]	
			out[3 * 4 + k] = (short)CLIP((y3 + (1 << (shift - 1))) >> shift, -32768, 32767); // out[3][k]	
		}
		else /* dim==2 */
		{
			// out: [k]th row
			out[4 * k + 0] = (short)CLIP((y0 + (1 << (shift - 1))) >> shift, -32768, 32767); // out[k][0]
			out[4 * k + 1] = (short)CLIP((y1 + (1 << (shift - 1))) >> shift, -32768, 32767); // out[k][1]
			out[4 * k + 2] = (short)CLIP((y2 + (1 << (shift - 1))) >> shift, -32768, 32767); // out[k][2]	
			out[4 * k + 3] = (short)CLIP((y3 + (1 << (shift - 1))) >> shift, -32768, 32767); // out[k][3]	
		}

	}
#if IT_LOG_EN_4
#define N 4
if(trType==0)
{
	if (trType == 0)
	{
		printf("-------- QT Module Start IDCT ------\n");
	}
	else
	{
		printf("-------- QT Module Start IDST ------\n");
	}

	printf("in:\n");
	for (int j = 0; j < N; j++)
	{
		for (int i = 0; i < N; i++)
		{

			printf("%04x, ", in[j*N + i]&0xffff); //%d
		}
		printf("\n");
	}

	printf("out:\n");
	for (int j = 0; j < N; j++)
	{
		for (int i = 0; i < N; i++)
		{
			printf("%04x, ", out[j*N + i]&0xffff); //%d
		}
		printf("\n");
	}
	printf("-------- QT Module END ------\n");
	printf("\n\n\n");
}	
#undef N	
#endif
}

// inverse transform 4x4 main function
void hevcQT::idct4(short *outResi, int *coeffTQIQ, int trType)
{

	short in[16];
	short out[16];
	short temp[16];

	int shift1D = 7;
	int shift2D = 12 - (RK_BIT_DEPTH - 8);


	//memcpy(in, coeffTQIQ, 4*4*sizeof(short));
	//debug used, convert [int] to [short]
	for (int k = 0; k < 16; k++)
	{
		in[k] = (short)coeffTQIQ[k];
	}

	//transpose, behavior of hevc decoder,	debug used
	transpose(in, 4);


	it4(temp, in, shift1D, trType, 1);// 1D idct4x4, row in, row out
	it4(out, temp, shift2D, trType, 2);//2D idct4x4, row in, col out

	// copy [out] to [ourResi], size in 4x4
	for (int k = 0; k < 4; k++)
	{
		memcpy(&(outResi[k*CTU_SIZE]), &(out[k * 4]), 4 * sizeof(short));
	}
}

// idct 8x8
void hevcQT::it8(short *out, short *in, int shift)
{
	int(*tableEve)[4] = idct_8x8_coeff_even;
	int(*tableOdd)[4] = idct_8x8_coeff_odd;

	int y0, y1, y2, y3;
	int z0, z1, z2, z3;

	y0 = tableEve[0][0] * in[0] + tableEve[0][1] * in[2] + tableEve[0][2] * in[4] + tableEve[0][3] * in[6];
	z0 = tableOdd[0][0] * in[1] + tableOdd[0][1] * in[3] + tableOdd[0][2] * in[5] + tableOdd[0][3] * in[7];
	out[0] = (short)CLIP((y0 + z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
	out[7] = (short)CLIP((y0 - z0 + (1 << (shift - 1))) >> shift, -32768, 32767);

	y1 = tableEve[1][0] * in[0] + tableEve[1][1] * in[2] + tableEve[1][2] * in[4] + tableEve[1][3] * in[6];
	z1 = tableOdd[1][0] * in[1] + tableOdd[1][1] * in[3] + tableOdd[1][2] * in[5] + tableOdd[1][3] * in[7];
	out[1] = (short)CLIP((y1 + z1 + (1 << (shift - 1))) >> shift, -32768, 32767);
	out[6] = (short)CLIP((y1 - z1 + (1 << (shift - 1))) >> shift, -32768, 32767);

	y2 = tableEve[2][0] * in[0] + tableEve[2][1] * in[2] + tableEve[2][2] * in[4] + tableEve[2][3] * in[6];
	z2 = tableOdd[2][0] * in[1] + tableOdd[2][1] * in[3] + tableOdd[2][2] * in[5] + tableOdd[2][3] * in[7];
	out[2] = (short)CLIP((y2 + z2 + (1 << (shift - 1))) >> shift, -32768, 32767);
	out[5] = (short)CLIP((y2 - z2 + (1 << (shift - 1))) >> shift, -32768, 32767);

	y3 = tableEve[3][0] * in[0] + tableEve[3][1] * in[2] + tableEve[3][2] * in[4] + tableEve[3][3] * in[6];
	z3 = tableOdd[3][0] * in[1] + tableOdd[3][1] * in[3] + tableOdd[3][2] * in[5] + tableOdd[3][3] * in[7];
	out[3] = (short)CLIP((y3 + z3 + (1 << (shift - 1))) >> shift, -32768, 32767);
	out[4] = (short)CLIP((y3 - z3 + (1 << (shift - 1))) >> shift, -32768, 32767);
}

//inverse transform 8x8 main function
void hevcQT::idct8(short *outResi, int *coeffTQIQ)
{
	short in[64];
	short out[64];
	short temp[64];

	int shift1D = 7;
	int shift2D = 12 - (RK_BIT_DEPTH - 8);


	//memcpy(in, coeffTQIQ, 4*4*sizeof(short));
	//debug used, convert [int] to [short]
	for (int k = 0; k < 64; k++)
	{
		in[k] = (short)coeffTQIQ[k];
	}

	//transpose, behavior of hevc decoder,	debug used
	transpose(in, 8);

	for (int k = 0; k < 8; k++)
	{
		it8(&(temp[k * 8]), &(in[8 * k]), shift1D); //1D 1 1ine
	}

	//transpose, behavior of hevc decoder,	debug used
	transpose(temp, 8);


	for (int k = 0; k < 8; k++)
	{
		it8(&(out[k * 8]), &(temp[8 * k]), shift2D); //2D 1 line
	}

	// copy [out] to [ourResi], size in 8x8
	for (int k = 0; k < 8; k++)
	{
		memcpy(&(outResi[k*CTU_SIZE]), &(out[k * 8]), 8 * sizeof(short));
	}
}

// inverse transform 16x16, get out[0~3] and out[12~15]
void hevcQT::it16_0to3(short *out, short *in, int shift)
{
	int i;
	int(*pEve0)[4] = idct_16x16_coeff_even_part0;
	int(*pEve1)[4] = idct_16x16_coeff_even_part1;

	int(*pOdd0)[4] = idct_16x16_coeff_odd_part0;
	int(*pOdd1)[4] = idct_16x16_coeff_odd_part1;

	int y00, y01;
	int z00, z01;
	int y0;
	int z0;

	for (i = 0; i < 4; i++)
	{
		y00 = pEve0[i][0] * in[0] + pEve0[i][1] * in[2] + pEve0[i][2] * in[4] + pEve0[i][3] * in[6];
		y01 = pEve1[i][0] * in[8] + pEve1[i][1] * in[10] + pEve1[i][2] * in[12] + pEve1[i][3] * in[14];

		z00 = pOdd0[i][0] * in[1] + pOdd0[i][1] * in[3] + pOdd0[i][2] * in[5] + pOdd0[i][3] * in[7];
		z01 = pOdd1[i][0] * in[9] + pOdd1[i][1] * in[11] + pOdd1[i][2] * in[13] + pOdd1[i][3] * in[15];

		y0 = y00 + y01;
		z0 = z00 + z01;

		out[0 + i] = (short)CLIP((y0 + z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
		out[15 - i] = (short)CLIP((y0 - z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
	}

}

// inverse transform 16x16, get out[4~7] and out[8~11]
void hevcQT::it16_4to7(short *out, short *in, int shift)
{
	int i;
	int(*pEve0)[4] = idct_16x16_coeff_even_part0;
	int(*pEve1)[4] = idct_16x16_coeff_even_part1;

	int(*pOdd0)[4] = idct_16x16_coeff_odd_part0;
	int(*pOdd1)[4] = idct_16x16_coeff_odd_part1;

	int y00, y01;
	int z00, z01;
	int y0;
	int z0;

	for (i = 4; i < 8; i++)
	{
		y00 = pEve0[i][0] * in[0] + pEve0[i][1] * in[2] + pEve0[i][2] * in[4] + pEve0[i][3] * in[6];
		y01 = pEve1[i][0] * in[8] + pEve1[i][1] * in[10] + pEve1[i][2] * in[12] + pEve1[i][3] * in[14];

		z00 = pOdd0[i][0] * in[1] + pOdd0[i][1] * in[3] + pOdd0[i][2] * in[5] + pOdd0[i][3] * in[7];
		z01 = pOdd1[i][0] * in[9] + pOdd1[i][1] * in[11] + pOdd1[i][2] * in[13] + pOdd1[i][3] * in[15];

		y0 = y00 + y01;
		z0 = z00 + z01;

		out[0 + i] = (short)CLIP((y0 + z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
		out[15 - i] = (short)CLIP((y0 - z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
	}

}

// inverse transform 16x16, main function
void hevcQT::idct16(short *outResi, int *coeffTQIQ)
{
	short in[16 * 16];
	short out[16 * 16];
	short temp[16 * 16];

	int shift1D = 7;
	int shift2D = 12 - (RK_BIT_DEPTH - 8);

	for (int k = 0; k < 16 * 16; k++)
	{
		in[k] = (short)coeffTQIQ[k];
	}

	//transpose, behavior of hevc decoder,	debug used
	transpose(in, 16);

	for (int k = 0; k < 16; k++)
	{
		it16_0to3(&(temp[k * 16]), &(in[16 * k]), shift1D); //1D 1 1ine, get temp[0~3], temp[12~15] 
		it16_4to7(&(temp[k * 16]), &(in[16 * k]), shift1D); //1D 1 line, get temp[4~7], temp[ 8~11]
	}

	//transpose, behavior of hevc decoder,	debug used
	transpose(temp, 16);

	for (int k = 0; k < 16; k++)
	{
		it16_0to3(&(out[k * 16]), &(temp[16 * k]), shift2D); //2D 1 line, get out[0~3], out[12~15]
		it16_4to7(&(out[k * 16]), &(temp[16 * k]), shift2D); //2D 1 line, get out[4~7], out[ 8~11]
	}


	// copy [out] to [ourResi], size in 16x16
	for (int k = 0; k < 16; k++)
	{
		memcpy(&(outResi[k*CTU_SIZE]), &(out[k * 16]), 16 * sizeof(short));
	}
}

// inverse transform 32x32, get out[0~3], out[28~31]
void hevcQT::it32_0to3(short *out, short *in, int shift)
{
	int i;

	int(*pEve0)[4] = idct_32x32_coeff_even_part0;
	int(*pEve1)[4] = idct_32x32_coeff_even_part1;
	int(*pEve2)[4] = idct_32x32_coeff_even_part2;
	int(*pEve3)[4] = idct_32x32_coeff_even_part3;

	int(*pOdd0)[4] = idct_32x32_coeff_odd_part0;
	int(*pOdd1)[4] = idct_32x32_coeff_odd_part1;
	int(*pOdd2)[4] = idct_32x32_coeff_odd_part2;
	int(*pOdd3)[4] = idct_32x32_coeff_odd_part3;

	int y00, y01, y02, y03;
	int z00, z01, z02, z03;
	int y0;
	int z0;

	for (i = 0; i < 4; i++)
	{
		y00 = pEve0[i][0] * in[0] + pEve0[i][1] * in[2] + pEve0[i][2] * in[4] + pEve0[i][3] * in[6];
		y01 = pEve1[i][0] * in[8] + pEve1[i][1] * in[10] + pEve1[i][2] * in[12] + pEve1[i][3] * in[14];
		y02 = pEve2[i][0] * in[16] + pEve2[i][1] * in[18] + pEve2[i][2] * in[20] + pEve2[i][3] * in[22];
		y03 = pEve3[i][0] * in[24] + pEve3[i][1] * in[26] + pEve3[i][2] * in[28] + pEve3[i][3] * in[30];

		z00 = pOdd0[i][0] * in[1] + pOdd0[i][1] * in[3] + pOdd0[i][2] * in[5] + pOdd0[i][3] * in[7];
		z01 = pOdd1[i][0] * in[9] + pOdd1[i][1] * in[11] + pOdd1[i][2] * in[13] + pOdd1[i][3] * in[15];
		z02 = pOdd2[i][0] * in[17] + pOdd2[i][1] * in[19] + pOdd2[i][2] * in[21] + pOdd2[i][3] * in[23];
		z03 = pOdd3[i][0] * in[25] + pOdd3[i][1] * in[27] + pOdd3[i][2] * in[29] + pOdd3[i][3] * in[31];

		y0 = y00 + y01 + y02 + y03;
		z0 = z00 + z01 + z02 + z03;

		out[0 + i] = (short)CLIP((y0 + z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
		out[31 - i] = (short)CLIP((y0 - z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
	}
}

// inverse transform 32x32, get out[4~7], out[24~27]
void hevcQT::it32_4to7(short *out, short *in, int shift)
{
	int i;

	int(*pEve0)[4] = idct_32x32_coeff_even_part0;
	int(*pEve1)[4] = idct_32x32_coeff_even_part1;
	int(*pEve2)[4] = idct_32x32_coeff_even_part2;
	int(*pEve3)[4] = idct_32x32_coeff_even_part3;

	int(*pOdd0)[4] = idct_32x32_coeff_odd_part0;
	int(*pOdd1)[4] = idct_32x32_coeff_odd_part1;
	int(*pOdd2)[4] = idct_32x32_coeff_odd_part2;
	int(*pOdd3)[4] = idct_32x32_coeff_odd_part3;

	int y00, y01, y02, y03;
	int z00, z01, z02, z03;
	int y0;
	int z0;

	for (i = 4; i < 8; i++)
	{
		y00 = pEve0[i][0] * in[0] + pEve0[i][1] * in[2] + pEve0[i][2] * in[4] + pEve0[i][3] * in[6];
		y01 = pEve1[i][0] * in[8] + pEve1[i][1] * in[10] + pEve1[i][2] * in[12] + pEve1[i][3] * in[14];
		y02 = pEve2[i][0] * in[16] + pEve2[i][1] * in[18] + pEve2[i][2] * in[20] + pEve2[i][3] * in[22];
		y03 = pEve3[i][0] * in[24] + pEve3[i][1] * in[26] + pEve3[i][2] * in[28] + pEve3[i][3] * in[30];

		z00 = pOdd0[i][0] * in[1] + pOdd0[i][1] * in[3] + pOdd0[i][2] * in[5] + pOdd0[i][3] * in[7];
		z01 = pOdd1[i][0] * in[9] + pOdd1[i][1] * in[11] + pOdd1[i][2] * in[13] + pOdd1[i][3] * in[15];
		z02 = pOdd2[i][0] * in[17] + pOdd2[i][1] * in[19] + pOdd2[i][2] * in[21] + pOdd2[i][3] * in[23];
		z03 = pOdd3[i][0] * in[25] + pOdd3[i][1] * in[27] + pOdd3[i][2] * in[29] + pOdd3[i][3] * in[31];

		y0 = y00 + y01 + y02 + y03;
		z0 = z00 + z01 + z02 + z03;

		out[0 + i] = (short)CLIP((y0 + z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
		out[31 - i] = (short)CLIP((y0 - z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
	}
}

// inverse transform 32x32, get out[8~11], out[20~23]
void hevcQT::it32_8to11(short *out, short *in, int shift)
{
	int i;

	int(*pEve0)[4] = idct_32x32_coeff_even_part0;
	int(*pEve1)[4] = idct_32x32_coeff_even_part1;
	int(*pEve2)[4] = idct_32x32_coeff_even_part2;
	int(*pEve3)[4] = idct_32x32_coeff_even_part3;

	int(*pOdd0)[4] = idct_32x32_coeff_odd_part0;
	int(*pOdd1)[4] = idct_32x32_coeff_odd_part1;
	int(*pOdd2)[4] = idct_32x32_coeff_odd_part2;
	int(*pOdd3)[4] = idct_32x32_coeff_odd_part3;

	int y00, y01, y02, y03;
	int z00, z01, z02, z03;
	int y0;
	int z0;


	for (i = 8; i < 12; i++)
	{
		y00 = pEve0[i][0] * in[0] + pEve0[i][1] * in[2] + pEve0[i][2] * in[4] + pEve0[i][3] * in[6];
		y01 = pEve1[i][0] * in[8] + pEve1[i][1] * in[10] + pEve1[i][2] * in[12] + pEve1[i][3] * in[14];
		y02 = pEve2[i][0] * in[16] + pEve2[i][1] * in[18] + pEve2[i][2] * in[20] + pEve2[i][3] * in[22];
		y03 = pEve3[i][0] * in[24] + pEve3[i][1] * in[26] + pEve3[i][2] * in[28] + pEve3[i][3] * in[30];

		z00 = pOdd0[i][0] * in[1] + pOdd0[i][1] * in[3] + pOdd0[i][2] * in[5] + pOdd0[i][3] * in[7];
		z01 = pOdd1[i][0] * in[9] + pOdd1[i][1] * in[11] + pOdd1[i][2] * in[13] + pOdd1[i][3] * in[15];
		z02 = pOdd2[i][0] * in[17] + pOdd2[i][1] * in[19] + pOdd2[i][2] * in[21] + pOdd2[i][3] * in[23];
		z03 = pOdd3[i][0] * in[25] + pOdd3[i][1] * in[27] + pOdd3[i][2] * in[29] + pOdd3[i][3] * in[31];

		y0 = y00 + y01 + y02 + y03;
		z0 = z00 + z01 + z02 + z03;

		out[0 + i] = (short)CLIP((y0 + z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
		out[31 - i] = (short)CLIP((y0 - z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
	}
}

// inverse transform 32x32, get out[12~15], out[16~19]
void hevcQT::it32_12to15(short *out, short *in, int shift)
{
	int i;

	int(*pEve0)[4] = idct_32x32_coeff_even_part0;
	int(*pEve1)[4] = idct_32x32_coeff_even_part1;
	int(*pEve2)[4] = idct_32x32_coeff_even_part2;
	int(*pEve3)[4] = idct_32x32_coeff_even_part3;

	int(*pOdd0)[4] = idct_32x32_coeff_odd_part0;
	int(*pOdd1)[4] = idct_32x32_coeff_odd_part1;
	int(*pOdd2)[4] = idct_32x32_coeff_odd_part2;
	int(*pOdd3)[4] = idct_32x32_coeff_odd_part3;

	int y00, y01, y02, y03;
	int z00, z01, z02, z03;
	int y0;
	int z0;

	for (i = 12; i < 16; i++)
	{
		y00 = pEve0[i][0] * in[0] + pEve0[i][1] * in[2] + pEve0[i][2] * in[4] + pEve0[i][3] * in[6];
		y01 = pEve1[i][0] * in[8] + pEve1[i][1] * in[10] + pEve1[i][2] * in[12] + pEve1[i][3] * in[14];
		y02 = pEve2[i][0] * in[16] + pEve2[i][1] * in[18] + pEve2[i][2] * in[20] + pEve2[i][3] * in[22];
		y03 = pEve3[i][0] * in[24] + pEve3[i][1] * in[26] + pEve3[i][2] * in[28] + pEve3[i][3] * in[30];

		z00 = pOdd0[i][0] * in[1] + pOdd0[i][1] * in[3] + pOdd0[i][2] * in[5] + pOdd0[i][3] * in[7];
		z01 = pOdd1[i][0] * in[9] + pOdd1[i][1] * in[11] + pOdd1[i][2] * in[13] + pOdd1[i][3] * in[15];
		z02 = pOdd2[i][0] * in[17] + pOdd2[i][1] * in[19] + pOdd2[i][2] * in[21] + pOdd2[i][3] * in[23];
		z03 = pOdd3[i][0] * in[25] + pOdd3[i][1] * in[27] + pOdd3[i][2] * in[29] + pOdd3[i][3] * in[31];

		y0 = y00 + y01 + y02 + y03;
		z0 = z00 + z01 + z02 + z03;

		out[0 + i] = (short)CLIP((y0 + z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
		out[31 - i] = (short)CLIP((y0 - z0 + (1 << (shift - 1))) >> shift, -32768, 32767);
	}
}

// inverse transform 32x32, main function
void hevcQT::idct32(short *outResi, int *coeffTQIQ)
{
	short in[32 * 32];
	short out[32 * 32];
	short temp[32 * 32];

	int shift1D = 7;
	int shift2D = 12 - (RK_BIT_DEPTH - 8);

	for (int k = 0; k < 32 * 32; k++)
	{
		in[k] = (short)coeffTQIQ[k];
	}

	//transpose, behavior of hevc decoder,	debug used
	transpose(in, 32);

	for (int k = 0; k < 32; k++)
	{
		it32_0to3(&(temp[k * 32]), &(in[32 * k]), shift1D); //1D 1 1ine, get temp[0~3], temp[28~31] 
		it32_4to7(&(temp[k * 32]), &(in[32 * k]), shift1D); //1D 1 line, get temp[4~7], temp[24~27]
		it32_8to11(&(temp[k * 32]), &(in[32 * k]), shift1D); //1D 1 line, get temp[8~11], temp[20~23]
		it32_12to15(&(temp[k * 32]), &(in[32 * k]), shift1D); //1D 1 line, get temp[12~15], temp[16~19]
	}

	//transpose, behavior of hevc decoder,	debug used
	transpose(temp, 32);

	for (int k = 0; k < 32; k++)
	{
		it32_0to3(&(out[k * 32]), &(temp[32 * k]), shift2D); //2D 1 1ine, get out[0~3], out[28~31] 
		it32_4to7(&(out[k * 32]), &(temp[32 * k]), shift2D); //2D 1 line, get out[4~7], out[24~27]
		it32_8to11(&(out[k * 32]), &(temp[32 * k]), shift2D); //2D 1 line, get out[8~11], out[20~23]
		it32_12to15(&(out[k * 32]), &(temp[32 * k]), shift2D); //2D 1 line, get out[12~15], out[16~19]
	}


	// copy [out] to [ourResi], size in 32x32
	for (int k = 0; k < 32; k++)
	{
		memcpy(&(outResi[k*CTU_SIZE]), &(out[k * 32]), 32 * sizeof(short));
	}
}

void hevcQT::idct(short* outResi, int* coeffTQIQ, uint32_t tuWidth, bool isIntra, uint8_t textType)
{
	//isIntra: intra(1), inter(0), when is Intra, 4x4 luma do IDST

	int trType; // only for Intra 4x4, to decide to do IDST or IDCT

	trType = isIntra ? (textType == 0 ? 1 : 0) : 0; // LUMA & Intra, only for 4x4, 1--IDST, 0--IDCT

	switch (tuWidth)
	{
	case 4:	 idct4(outResi, coeffTQIQ, trType);//1--IDST, 0--IDCT
		break;
	case 8:  idct8(outResi, coeffTQIQ);
		break;
	case 16: idct16(outResi, coeffTQIQ);
		break;
	case 32: idct32(outResi, coeffTQIQ);
		break;
	default:
		break;
	}
}


void hevcQT::t4(short* out, short* in, int shift, int trType, int dim)
{
	int j;
	int E[2], O[2];
	int add = 1 << (shift - 1);
	int(*table)[4] = NULL;
	assert((trType == 0) || (trType == 1));
	assert((dim == 1) || (dim == 2));

	if (trType == 0)	/*DCT*/
	{
		table = dct_4x4_coeff;
	}
	else	/*DST*/
	{
		table = dst_4x4_coeff;
	}

	if (trType == 0) /*DCT*/
	{
		// in: row[0~3] ==> out: col[0~3]
		for (j = 0; j < 4; j++)
		{
			/* E and O */
			E[0] = in[j * 4 + 0] + in[j * 4 + 3];
			O[0] = in[j * 4 + 0] - in[j * 4 + 3];
			E[1] = in[j * 4 + 1] + in[j * 4 + 2];
			O[1] = in[j * 4 + 1] - in[j * 4 + 2];

			//original description in x265
			//out[0 * 4 + j] = (short)((coeff[0][0] * (in[j*4 + 0] + in[j*4 + 3]) + coeff[0][1] * (in[j*4 + 1] + in[j*4 + 2]) + add) >> shift);
			//out[1 * 4 + j] = (short)((coeff[1][0] * (in[j*4 + 0] - in[j*4 + 3]) + coeff[1][1] * (in[j*4 + 1] - in[j*4 + 2]) + add) >> shift);		
			//out[2 * 4 + j] = (short)((coeff[2][0] * (in[j*4 + 0] + in[j*4 + 3]) + coeff[2][1] * (in[j*4 + 1] + in[j*4 + 2]) + add) >> shift);
			//out[3 * 4 + j] = (short)((coeff[3][0] * (in[j*4 + 0] - in[j*4 + 3]) + coeff[3][1] * (in[j*4 + 1] - in[j*4 + 2]) + add) >> shift);

			out[0 * 4 + j] = (short)((table[0][0] * E[0] + table[0][1] * E[1] + add) >> shift); // out[0][j]
			out[1 * 4 + j] = (short)((table[1][0] * O[0] + table[1][1] * O[1] + add) >> shift); // out[1][j]
			out[2 * 4 + j] = (short)((table[2][0] * E[0] + table[2][1] * E[1] + add) >> shift); // out[2][j]
			out[3 * 4 + j] = (short)((table[3][0] * O[0] + table[3][1] * O[1] + add) >> shift); // out[3][j]

		}
	}
	else /*DST*/
	{
		// in: row[0~3] ==> out: col[0~3]
		for (j = 0; j < 4; j++)
		{
			/* E and O */
			out[0 * 4 + j] = (short)((table[0][0] * in[j * 4 + 0] + table[0][1] * in[j * 4 + 1] + table[0][2] * in[j * 4 + 2] + table[0][3] * in[j * 4 + 3] + add) >> shift);
			out[1 * 4 + j] = (short)((table[1][0] * in[j * 4 + 0] + table[1][1] * in[j * 4 + 1] + table[1][2] * in[j * 4 + 2] + table[1][3] * in[j * 4 + 3] + add) >> shift);
			out[2 * 4 + j] = (short)((table[2][0] * in[j * 4 + 0] + table[2][1] * in[j * 4 + 1] + table[2][2] * in[j * 4 + 2] + table[2][3] * in[j * 4 + 3] + add) >> shift);
			out[3 * 4 + j] = (short)((table[3][0] * in[j * 4 + 0] + table[3][1] * in[j * 4 + 1] + table[3][2] * in[j * 4 + 2] + table[3][3] * in[j * 4 + 3] + add) >> shift);
		}

	}

#if T_LOG_EN_4
#define N 4
if(trType==0) // only print dst
{
	if (trType == 0)
	{
		printf("-------- QT Module Start DCT ------\n");
	}
	else
	{
		printf("-------- QT Module Start DST ------\n");
	}

	printf("in:\n");
	for (int j = 0; j < N; j++)
	{
		for (int i = 0; i < N; i++)
		{

			printf("%04x, ", in[j*N + i]&0xffff); //%d
		}
		printf("\n");
	}

	printf("out:\n");
	for (int j = 0; j < N; j++)
	{
		for (int i = 0; i < N; i++)
		{
			printf("%04x, ", out[j*N + i]&0xffff);//%d
		}
		printf("\n");
	}
	printf("-------- QT Module END ------\n");
	printf("\n\n\n");
}	
#undef N	
#endif
}

void hevcQT::dct4(int *coeffT, short *inResi, int trType)
{
	short in[16];
	short out[16];
	short temp[16];

	const int shift1D = 1 + RK_BIT_DEPTH - 8;
	const int shift2D = 8;

	//temp interface, because [inResi] is stored in 64x64 definitely
	for (int k = 0; k < 4; k++)
	{
		memcpy(&(in[k * 4]), &(inResi[k*CTU_SIZE]), 4 * sizeof(short));
	}

	t4(temp, in, shift1D, trType, 1); // 1D dct4x4,	row in, col out
	t4(out, temp, shift2D, trType, 2);// 2D dct4x4,	row in, col out

	// copy [out] to [coeffT], size in 4x4
	for (int k = 0; k < 16; k++)
	{
		coeffT[k] = (int)out[k];
	}
}

void hevcQT::t8(short* out, short* in, int shift)
{
	int(*table)[8] = dct_8x8_coeff;
	int E[4], O[4];
	int add = 1 << (shift - 1);

	/*in: row[0~7] ==> out: col[0~7]*/
	for (int k = 0; k < 8; k++) // col[0]~col[7]
	{

		/* E and O*/
		E[0] = in[8 * k + 0] + in[8 * k + 7];
		E[1] = in[8 * k + 1] + in[8 * k + 6];
		E[2] = in[8 * k + 2] + in[8 * k + 5];
		E[3] = in[8 * k + 3] + in[8 * k + 4];
		O[0] = in[8 * k + 0] - in[8 * k + 7];
		O[1] = in[8 * k + 1] - in[8 * k + 6];
		O[2] = in[8 * k + 2] - in[8 * k + 5];
		O[3] = in[8 * k + 3] - in[8 * k + 4];

		out[0 * 8 + k] = (short)((table[0][0] * E[0] + table[0][1] * E[1] + table[0][1] * E[2] + table[0][0] * E[3] + add) >> shift); // out[0][k]
		out[2 * 8 + k] = (short)((table[2][0] * E[0] + table[2][1] * E[1] - table[2][1] * E[2] - table[2][0] * E[3] + add) >> shift); // out[2][k]
		out[4 * 8 + k] = (short)((table[4][0] * E[0] + table[4][1] * E[1] + table[4][1] * E[2] + table[4][0] * E[3] + add) >> shift); // out[4][k]
		out[6 * 8 + k] = (short)((table[6][0] * E[0] + table[6][1] * E[1] - table[6][1] * E[2] - table[6][0] * E[3] + add) >> shift); // out[6][k]

		out[1 * 8 + k] = (short)((table[1][0] * O[0] + table[1][1] * O[1] + table[1][2] * O[2] + table[1][3] * O[3] + add) >> shift); // out[1][k]
		out[3 * 8 + k] = (short)((table[3][0] * O[0] + table[3][1] * O[1] + table[3][2] * O[2] + table[3][3] * O[3] + add) >> shift); // out[3][k]
		out[5 * 8 + k] = (short)((table[5][0] * O[0] + table[5][1] * O[1] + table[5][2] * O[2] + table[5][3] * O[3] + add) >> shift); // out[5][k]
		out[7 * 8 + k] = (short)((table[7][0] * O[0] + table[7][1] * O[1] + table[7][2] * O[2] + table[7][3] * O[3] + add) >> shift); // out[7][k]
	}

}

void hevcQT::dct8(int *coeffT, short *inResi)
{
	short in[8 * 8];
	short out[8 * 8];
	short temp[8 * 8];

	const int shift1D = 2 + RK_BIT_DEPTH - 8;
	const int shift2D = 9;

	//temp interface, because [inResi] is stored in 64x64 definitely
	for (int k = 0; k < 8; k++)
	{
		memcpy(&(in[k * 8]), &(inResi[k*CTU_SIZE]), 8 * sizeof(short));
	}

	t8(temp, in, shift1D);//  1D dct8x8,	row in, col out
	t8(out, temp, shift2D);// 2D dct8x8,	row in, col out

	// copy [out] to [coeffT], size in 8x8
	for (int k = 0; k < 8 * 8; k++)
	{
		coeffT[k] = (int)out[k];
	}

}

void hevcQT::t16(short* out, short* in, int shift)
{
	int E[8], O[8];
	int EE[4], EO[4];
	int tmp0[8], tmp1[8];
	int(*tableEve)[4] = dct_16x16_coeff_even;
	int(*tableOdd)[8] = dct_16x16_coeff_odd;
	int add = 1 << (shift - 1);

	/*in: row[0~15] ==> out: col[0~15]*/
	for (int j = 0; j < 16; j++)
	{
		/* E and O */
		for (int k = 0; k < 8; k++)
		{
			E[k] = in[j * 16 + k] + in[j * 16 + (15 - k)];
			O[k] = in[j * 16 + k] - in[j * 16 + (15 - k)];
		}

		/* EE and EO */
		EE[0] = E[0] + E[7];
		EE[1] = E[1] + E[6];
		EE[2] = E[2] + E[5];
		EE[3] = E[3] + E[4];

		EO[0] = E[0] - E[7];
		EO[1] = E[1] - E[6];
		EO[2] = E[2] - E[5];
		EO[3] = E[3] - E[4];


		out[0 * 16 + j] = (short)((tableEve[0][0] * EE[0] + tableEve[0][1] * EE[1] + tableEve[0][2] * EE[2] + tableEve[0][3] * EE[3] + add) >> shift); // out[ 0][j]
		out[4 * 16 + j] = (short)((tableEve[2][0] * EE[0] + tableEve[2][1] * EE[1] + tableEve[2][2] * EE[2] + tableEve[2][3] * EE[3] + add) >> shift); // out[ 4][j]
		out[8 * 16 + j] = (short)((tableEve[4][0] * EE[0] + tableEve[4][1] * EE[1] + tableEve[4][2] * EE[2] + tableEve[4][3] * EE[3] + add) >> shift); // out[ 8][j] 
		out[12 * 16 + j] = (short)((tableEve[6][0] * EE[0] + tableEve[6][1] * EE[1] + tableEve[6][2] * EE[2] + tableEve[6][3] * EE[3] + add) >> shift); // out[12][j]

		out[2 * 16 + j] = (short)((tableEve[1][0] * EO[0] + tableEve[1][1] * EO[1] + tableEve[1][2] * EO[2] + tableEve[1][3] * EO[3] + add) >> shift); // out[ 2][j]
		out[6 * 16 + j] = (short)((tableEve[3][0] * EO[0] + tableEve[3][1] * EO[1] + tableEve[3][2] * EO[2] + tableEve[3][3] * EO[3] + add) >> shift); // out[ 6][j]
		out[10 * 16 + j] = (short)((tableEve[5][0] * EO[0] + tableEve[5][1] * EO[1] + tableEve[5][2] * EO[2] + tableEve[5][3] * EO[3] + add) >> shift); // out[10][j]
		out[14 * 16 + j] = (short)((tableEve[7][0] * EO[0] + tableEve[7][1] * EO[1] + tableEve[7][2] * EO[2] + tableEve[7][3] * EO[3] + add) >> shift); // out[14][j]

		tmp0[0] = tableOdd[0][0] * O[0] + tableOdd[0][1] * O[1] + tableOdd[0][2] * O[2] + tableOdd[0][3] * O[3];
		tmp0[1] = tableOdd[1][0] * O[0] + tableOdd[1][1] * O[1] + tableOdd[1][2] * O[2] + tableOdd[1][3] * O[3];
		tmp0[2] = tableOdd[2][0] * O[0] + tableOdd[2][1] * O[1] + tableOdd[2][2] * O[2] + tableOdd[2][3] * O[3];
		tmp0[3] = tableOdd[3][0] * O[0] + tableOdd[3][1] * O[1] + tableOdd[3][2] * O[2] + tableOdd[3][3] * O[3];
		tmp0[4] = tableOdd[4][0] * O[0] + tableOdd[4][1] * O[1] + tableOdd[4][2] * O[2] + tableOdd[4][3] * O[3];
		tmp0[5] = tableOdd[5][0] * O[0] + tableOdd[5][1] * O[1] + tableOdd[5][2] * O[2] + tableOdd[5][3] * O[3];
		tmp0[6] = tableOdd[6][0] * O[0] + tableOdd[6][1] * O[1] + tableOdd[6][2] * O[2] + tableOdd[6][3] * O[3];
		tmp0[7] = tableOdd[7][0] * O[0] + tableOdd[7][1] * O[1] + tableOdd[7][2] * O[2] + tableOdd[7][3] * O[3];

		tmp1[0] = tableOdd[0][4] * O[4] + tableOdd[0][5] * O[5] + tableOdd[0][6] * O[6] + tableOdd[0][7] * O[7];
		tmp1[1] = tableOdd[1][4] * O[4] + tableOdd[1][5] * O[5] + tableOdd[1][6] * O[6] + tableOdd[1][7] * O[7];
		tmp1[2] = tableOdd[2][4] * O[4] + tableOdd[2][5] * O[5] + tableOdd[2][6] * O[6] + tableOdd[2][7] * O[7];
		tmp1[3] = tableOdd[3][4] * O[4] + tableOdd[3][5] * O[5] + tableOdd[3][6] * O[6] + tableOdd[3][7] * O[7];
		tmp1[4] = tableOdd[4][4] * O[4] + tableOdd[4][5] * O[5] + tableOdd[4][6] * O[6] + tableOdd[4][7] * O[7];
		tmp1[5] = tableOdd[5][4] * O[4] + tableOdd[5][5] * O[5] + tableOdd[5][6] * O[6] + tableOdd[5][7] * O[7];
		tmp1[6] = tableOdd[6][4] * O[4] + tableOdd[6][5] * O[5] + tableOdd[6][6] * O[6] + tableOdd[6][7] * O[7];
		tmp1[7] = tableOdd[7][4] * O[4] + tableOdd[7][5] * O[5] + tableOdd[7][6] * O[6] + tableOdd[7][7] * O[7];

		out[1 * 16 + j] = (short)((tmp0[0] + tmp1[0] + add) >> shift);
		out[3 * 16 + j] = (short)((tmp0[1] + tmp1[1] + add) >> shift);
		out[5 * 16 + j] = (short)((tmp0[2] + tmp1[2] + add) >> shift);
		out[7 * 16 + j] = (short)((tmp0[3] + tmp1[3] + add) >> shift);
		out[9 * 16 + j] = (short)((tmp0[4] + tmp1[4] + add) >> shift);
		out[11 * 16 + j] = (short)((tmp0[5] + tmp1[5] + add) >> shift);
		out[13 * 16 + j] = (short)((tmp0[6] + tmp1[6] + add) >> shift);
		out[15 * 16 + j] = (short)((tmp0[7] + tmp1[7] + add) >> shift);

		// original description in x265
		// 		out[1 * 16 + j] = (short)((tableOdd[1][0] * O[0] + tableOdd[1][1] * O[1] + tableOdd[1][2] * O[2] + table[1][3] * O[3] + table[1][4] * O[4] + tableOdd[1][5] * O[5] + tableOdd[1][6] * O[6] + tableOdd[1][7] * O[7] + add) >> shift);
		// 		out[3 * 16 + j] = (short)((tableOdd[3][0] * O[0] + tableOdd[3][1] * O[1] + tableOdd[3][2] * O[2] + table[3][3] * O[3] + table[3][4] * O[4] + tableOdd[3][5] * O[5] + tableOdd[3][6] * O[6] + tableOdd[1][7] * O[7] + add) >> shift);
		// 		out[5 * 16 + j] = (short)((tableOdd[5][0] * O[0] + tableOdd[5][1] * O[1] + tableOdd[5][2] * O[2] + table[5][3] * O[3] + table[5][4] * O[4] + tableOdd[5][5] * O[5] + tableOdd[5][6] * O[6] + tableOdd[1][7] * O[7] + add) >> shift);
		// 		out[7 * 16 + j] = (short)((tableOdd[7][0] * O[0] + tableOdd[7][1] * O[1] + tableOdd[7][2] * O[2] + table[7][3] * O[3] + table[7][4] * O[4] + tableOdd[7][5] * O[5] + tableOdd[7][6] * O[6] + tableOdd[1][7] * O[7] + add) >> shift);
		// 		out[9 * 16 + j] = (short)((tableOdd[9][0] * O[0] + tableOdd[9][1] * O[1] + tableOdd[9][2] * O[2] + table[9][3] * O[3] + table[9][4] * O[4] + tableOdd[9][5] * O[5] + tableOdd[9][6] * O[6] + tableOdd[1][7] * O[7] + add) >> shift);
		// 		out[11 * 16 + j] = (short)((tableOdd[11][0] * O[0] + tableOdd[11][1] * O[1] + tableOdd[11][2] * O[2] + table[11][3] * O[3] + table[11][4] * O[4] + tableOdd[11][5] * O[5] + tableOdd[11][6] * O[6] + tableOdd[1][7] * O[7] + add) >> shift);
		// 		out[13 * 16 + j] = (short)((tableOdd[13][0] * O[0] + tableOdd[13][1] * O[1] + tableOdd[13][2] * O[2] + table[13][3] * O[3] + table[13][4] * O[4] + tableOdd[13][5] * O[5] + tableOdd[13][6] * O[6] + tableOdd[1][7] * O[7] + add) >> shift);
		// 		out[15 * 16 + j] = (short)((tableOdd[15][0] * O[0] + tableOdd[15][1] * O[1] + tableOdd[15][2] * O[2] + table[15][3] * O[3] + table[15][4] * O[4] + tableOdd[15][5] * O[5] + tableOdd[15][6] * O[6] + tableOdd[1][7] * O[7] + add) >> shift);
	}
#if QT_LOG_EN_16
#define N 16
	printf("-------- QT Module Start DCT ------\n");
	printf("in:\n");
	for (int j = 0; j < N; j++)
	{
		for (int i = 0; i < N; i++)
		{

			printf("%d, ", in[j*N + i]);
		}
		printf("\n");
	}

	printf("out:\n");
	for (int j = 0; j < N; j++)
	{
		for (int i = 0; i < N; i++)
		{
			printf("%d, ", out[j*N + i]);
		}
		printf("\n");
	}
	printf("-------- QT Module END ------\n");
	printf("\n\n\n");
#undef N	
#endif
}

void hevcQT::dct16(int *coeffT, short *inResi)
{
	short in[16 * 16];
	short out[16 * 16];
	short temp[16 * 16];

	const int shift1D = 3 + RK_BIT_DEPTH - 8;
	const int shift2D = 10;

	//temp interface, because [inResi] is stored in 64x64 definitely
	for (int k = 0; k < 16; k++)
	{
		memcpy(&(in[k * 16]), &(inResi[k*CTU_SIZE]), 16 * sizeof(short));
	}

	t16(temp, in, shift1D);//  1D dct16x16,	row in, col out
	t16(out, temp, shift2D);// 2D dct16x16,	row in, col out

	// copy [out] to [coeffT], size in 16x16
	for (int k = 0; k < 16 * 16; k++)
	{
		coeffT[k] = (int)out[k];
	}
}

void hevcQT::t32(short* out, short* in, int shift)
{
	int j, k;
	int tmpX0[8], tmpX1[8];
	int tmpY0[16], tmpY1[16], tmpY2[16], tmpY3[16];
	int E[16], O[16];
	int EE[8], EO[8];
	int EEE[4], EEO[4];
	int(*tableEve)[8] = dct_32x32_coeff_even;
	int(*tableOdd)[16] = dct_32x32_coeff_odd;
	int add = 1 << (shift - 1);

	for (j = 0; j < 32; j++)
	{
		for (k = 0; k < 16; k++)
		{
			E[k] = in[32 * j + k] + in[32 * j + (31 - k)];
			O[k] = in[32 * j + k] - in[32 * j + (31 - k)];
		}

		/* EE and EO */
		for (k = 0; k < 8; k++)
		{
			EE[k] = E[k] + E[15 - k];
			EO[k] = E[k] - E[15 - k];
		}

		/* EEE and EEO */
		for (k = 0; k < 4; k++)
		{
			EEE[k] = EE[k] + EE[7 - k];
			EEO[k] = EE[k] - EE[7 - k];
		}

		out[0 * 32 + j] = (short)((tableEve[0][0] * EEE[0] + tableEve[0][1] * EEE[1] + tableEve[0][2] * EEE[2] + tableEve[0][3] * EEE[3] + add) >> shift); //[ 0][0~1]
		out[8 * 32 + j] = (short)((tableEve[4][0] * EEE[0] + tableEve[4][1] * EEE[1] + tableEve[4][2] * EEE[2] + tableEve[4][3] * EEE[3] + add) >> shift); //[ 8][0~1]	
		out[16 * 32 + j] = (short)((tableEve[8][0] * EEE[0] + tableEve[8][1] * EEE[1] + tableEve[8][2] * EEE[2] + tableEve[8][3] * EEE[3] + add) >> shift); //[16][0~1]	
		out[24 * 32 + j] = (short)((tableEve[12][0] * EEE[0] + tableEve[12][1] * EEE[1] + tableEve[12][2] * EEE[2] + tableEve[12][3] * EEE[3] + add) >> shift); //[24][0~1]	

		out[4 * 32 + j] = (short)((tableEve[2][0] * EEO[0] + tableEve[2][1] * EEO[1] + tableEve[2][2] * EEO[2] + tableEve[2][3] * EEO[3] + add) >> shift); //[ 4][0~3]
		out[12 * 32 + j] = (short)((tableEve[6][0] * EEO[0] + tableEve[6][1] * EEO[1] + tableEve[6][2] * EEO[2] + tableEve[6][3] * EEO[3] + add) >> shift); //[12][0~3]
		out[20 * 32 + j] = (short)((tableEve[10][0] * EEO[0] + tableEve[10][1] * EEO[1] + tableEve[10][2] * EEO[2] + tableEve[10][3] * EEO[3] + add) >> shift); //[20][0~3]	
		out[28 * 32 + j] = (short)((tableEve[14][0] * EEO[0] + tableEve[14][1] * EEO[1] + tableEve[14][2] * EEO[2] + tableEve[14][3] * EEO[3] + add) >> shift); //[28][0~3]	


		tmpX0[0] = tableEve[1][0] * EO[0] + tableEve[1][1] * EO[1] + tableEve[1][2] * EO[2] + tableEve[1][3] * EO[3]; //[ 2][0~7]
		tmpX0[1] = tableEve[3][0] * EO[0] + tableEve[3][1] * EO[1] + tableEve[3][2] * EO[2] + tableEve[3][3] * EO[3]; //[ 6][0~7]
		tmpX0[2] = tableEve[5][0] * EO[0] + tableEve[5][1] * EO[1] + tableEve[5][2] * EO[2] + tableEve[5][3] * EO[3]; //[10][0~7]				
		tmpX0[3] = tableEve[7][0] * EO[0] + tableEve[7][1] * EO[1] + tableEve[7][2] * EO[2] + tableEve[7][3] * EO[3]; //[14][0~7]			
		tmpX0[4] = tableEve[9][0] * EO[0] + tableEve[9][1] * EO[1] + tableEve[9][2] * EO[2] + tableEve[9][3] * EO[3]; //[18][0~7]
		tmpX0[5] = tableEve[11][0] * EO[0] + tableEve[11][1] * EO[1] + tableEve[11][2] * EO[2] + tableEve[11][3] * EO[3]; //[22][0~7]
		tmpX0[6] = tableEve[13][0] * EO[0] + tableEve[13][1] * EO[1] + tableEve[13][2] * EO[2] + tableEve[13][3] * EO[3]; //[26][0~7]
		tmpX0[7] = tableEve[15][0] * EO[0] + tableEve[15][1] * EO[1] + tableEve[15][2] * EO[2] + tableEve[15][3] * EO[3]; //[30][0~7]

		tmpX1[0] = tableEve[1][4] * EO[4] + tableEve[1][5] * EO[5] + tableEve[1][6] * EO[6] + tableEve[1][7] * EO[7];
		tmpX1[1] = tableEve[3][4] * EO[4] + tableEve[3][5] * EO[5] + tableEve[3][6] * EO[6] + tableEve[3][7] * EO[7];
		tmpX1[2] = tableEve[5][4] * EO[4] + tableEve[5][5] * EO[5] + tableEve[5][6] * EO[6] + tableEve[5][7] * EO[7];
		tmpX1[3] = tableEve[7][4] * EO[4] + tableEve[7][5] * EO[5] + tableEve[7][6] * EO[6] + tableEve[7][7] * EO[7];
		tmpX1[4] = tableEve[9][4] * EO[4] + tableEve[9][5] * EO[5] + tableEve[9][6] * EO[6] + tableEve[9][7] * EO[7];
		tmpX1[5] = tableEve[11][4] * EO[4] + tableEve[11][5] * EO[5] + tableEve[11][6] * EO[6] + tableEve[11][7] * EO[7];
		tmpX1[6] = tableEve[13][4] * EO[4] + tableEve[13][5] * EO[5] + tableEve[13][6] * EO[6] + tableEve[13][7] * EO[7];
		tmpX1[7] = tableEve[15][4] * EO[4] + tableEve[15][5] * EO[5] + tableEve[15][6] * EO[6] + tableEve[15][7] * EO[7];

		out[2 * 32 + j] = (short)((tmpX0[0] + tmpX1[0] + add) >> shift);
		out[6 * 32 + j] = (short)((tmpX0[1] + tmpX1[1] + add) >> shift);
		out[10 * 32 + j] = (short)((tmpX0[2] + tmpX1[2] + add) >> shift);
		out[14 * 32 + j] = (short)((tmpX0[3] + tmpX1[3] + add) >> shift);
		out[18 * 32 + j] = (short)((tmpX0[4] + tmpX1[4] + add) >> shift);
		out[22 * 32 + j] = (short)((tmpX0[5] + tmpX1[5] + add) >> shift);
		out[26 * 32 + j] = (short)((tmpX0[6] + tmpX1[6] + add) >> shift);
		out[30 * 32 + j] = (short)((tmpX0[7] + tmpX1[7] + add) >> shift);
#if QT_LOG_EN_32
		printf("~~~~~~~~~~~~~~~ QT Module odd table start ~~~~~~~~~~~~~~~\n");
#endif
		for (k = 0; k<16; k++)
		{
			//[1~31][0~15]
			tmpY0[k] = tableOdd[k][0] * O[0] + tableOdd[k][1] * O[1] + tableOdd[k][2] * O[2] + tableOdd[k][3] * O[3];
			tmpY1[k] = tableOdd[k][4] * O[4] + tableOdd[k][5] * O[5] + tableOdd[k][6] * O[6] + tableOdd[k][7] * O[7];
			tmpY2[k] = tableOdd[k][8] * O[8] + tableOdd[k][9] * O[9] + tableOdd[k][10] * O[10] + tableOdd[k][11] * O[11];
			tmpY3[k] = tableOdd[k][12] * O[12] + tableOdd[k][13] * O[13] + tableOdd[k][14] * O[14] + tableOdd[k][15] * O[15];
#if QT_LOG_EN_32
			for (int i = 0; i < 16; i++)
			{
				printf("%d, ", tableOdd[k][i]);
			}
			printf("\n");
#endif
			//out[1*32+j]~out[31*32+j] all odd out
			out[(2 * k + 1) * 32 + j] = (short)((tmpY0[k] + tmpY1[k] + tmpY2[k] + tmpY3[k] + add) >> shift);
		}
	}
#if QT_LOG_EN_32
#define N 32
	printf("~~~~~~~~~~~~~~~ QT Module odd table end ~~~~~~~~~~~~~~~\n\n");

	printf("-------- QT Module Start DCT ------\n");
	printf("in:\n");
	for (int j = 0; j < N; j++)
	{
		for (int i = 0; i < N; i++)
		{

			printf("%d, ", in[j*N + i]);
		}
		printf("\n");
	}

	printf("out:\n");
	for (int j = 0; j < N; j++)
	{
		for (int i = 0; i < N; i++)
		{
			printf("%d, ", out[j*N + i]);
		}
		printf("\n");
	}

	printf("-------- QT Module END ------\n");
	printf("\n\n\n");
#undef N	
#endif
}

void hevcQT::dct32(int *coeffT, short *inResi)
{
	short in[32 * 32];
	short out[32 * 32];
	short temp[32 * 32];

	const int shift1D = 4 + RK_BIT_DEPTH - 8;
	const int shift2D = 11;

	//temp interface, because [inResi] is stored in 64x64 definitely
	for (int k = 0; k < 32; k++)
	{
		memcpy(&(in[k * 32]), &(inResi[k*CTU_SIZE]), 32 * sizeof(short));
	}

	t32(temp, in, shift1D);//  1D dct32x32,	row in, col out
	t32(out, temp, shift2D);// 2D dct32x32,	row in, col out

	// copy [out] to [coeffT], size in 32x32
	for (int k = 0; k < 32 * 32; k++)
	{
		coeffT[k] = (int)out[k];
	}
}

void hevcQT::dct(int* coeffT, short* inResi, uint32_t tuWidth, bool isIntra, uint8_t textType)
{
	//isIntra: intra(1), inter(0), when is Intra, 4x4 luma do IDST

	int trType; // only for Intra 4x4, to decide to do IDST or IDCT

	trType = isIntra ? (textType == 0 ? 1 : 0) : 0; // LUMA & Intra, only for 4x4, 1--IDST, 0--IDCT

	switch (tuWidth)
	{
	case 4:	 dct4(coeffT, inResi, trType);//1--IDST, 0--IDCT
		break;
	case 8:  dct8(coeffT, inResi);
		break;
	case 16: dct16(coeffT, inResi);
		break;
	case 32: dct32(coeffT, inResi);
		break;
	default:
		break;
	}
}

/** Set qP for Quantization.
* \param qpy QPy
* \param bLowpass
* \param sliceType
* \param ttype
* \param qpBdOffset
* \param chromaQPOffset
*
* return void
*/
void hevcQT::setQPforQ(int qp, uint8_t ttype)
{
	int qpScaled;

	if (ttype == 0) // LUMA
	{
		qpScaled = qp;
	}
	else //Chroma
	{
		qpScaled = ChromaScale[qp];
	}

	m_infoForQT->qpPer = qpScaled / 6;
	m_infoForQT->qpscaled = qpScaled;
	m_infoForQT->qpRem = qpScaled % 6;
	m_infoForQT->qpBits = RK_QP_BITS + m_infoForQT->qpPer;
}
uint32_t hevcQT::quant(int* out, int* in, int quantScale, int qBits, int add, int numCoeff, int32_t* lastPos)
{

	uint32_t absSum = 0;
	int temp0, temp1;
	int sign;

	for (int k = 0; k < numCoeff; k++)
	{
		sign = (in[k] < 0 ? -1 : 1);
		temp0 = abs(in[k]) * quantScale;
		temp1 = ((temp0 + add) >> qBits);
		if (temp1)
			*lastPos = k; // update lastPos
		absSum += temp1;
		temp1 *= sign;
		out[k] = CLIP(temp1, -32768, 32767);
	}
	return absSum;
}

void hevcQT::dequant(int* out, int* in, int dequantScale, int num, int shift)
{
	assert(num <= 32 * 32);
	// NOTE: maximum of scale is (72 * 256)
	assert(dequantScale < 32768);
	assert((num % 8) == 0);
	assert(shift <= 10);

	int temp0, temp1;
	int add = 1 << (shift - 1);

	for (int n = 0; n < num; n++)
	{
		temp0 = CLIP(in[n], -32768, 32767);
		temp1 = (temp0 * dequantScale + add) >> shift;
		out[n] = CLIP(temp1, -32768, 32767);
	}
}


void hevcQT::getFromX265()
{
	memcpy(m_infoForQT->inResi, m_infoFromX265->inResi, CTU_SIZE*CTU_SIZE*sizeof(int16_t));
	m_infoForQT->size = m_infoFromX265->size;
	assert(m_infoForQT->size <= 32);
	m_infoForQT->predMode = m_infoFromX265->predMode;
	m_infoForQT->sliceType = m_infoFromX265->sliceType;
	m_infoForQT->textType = m_infoFromX265->textType;
	m_infoForQT->qp = m_infoFromX265->qp;
	m_infoForQT->transformSkip = m_infoFromX265->transformSkip;
	m_infoForQT->qpBdOffset = m_infoFromX265->qpBdOffset;
	m_infoForQT->chromaQPOffset = m_infoFromX265->chromaQPOffset;

	// debug use
	m_infoForQT->ctuWidth = m_infoFromX265->ctuWidth;
}
// compare HEVC results with x265
void hevcQT::compareX265andHWC()
{
	/*compare info first, and then output.
	*tips: when assert happens, locate it,
	*comment the assertion, and set the conditional breakpoint next line
	*e.g. conditon: flagInfo==1
	*/
	uint32_t size = m_infoForQT->size;
	// compare debug info
	//assert(m_infoFromX265->qpscaled == m_infoForQT->qpscaled);
	//assert(m_infoFromX265->qpPer == m_infoForQT->qpPer); 
	//assert(m_infoFromX265->qpRem == m_infoForQT->qpRem);
	//assert(m_infoFromX265->qpBits == m_infoForQT->qpBits);
	//assert(m_infoFromX265->dequantScale == m_infoForQT->dequantScale);
	//assert(!memcmp(m_infoFromX265->quantCoef, m_infoForQT->quantCoef, tuWidth*tuHeight*sizeof(int32_t))); 
	//assert(!memcmp(m_infoFromX265->coeffT, m_infoForQT->coeffT, size*size*sizeof(int32_t))); 
	//assert(!memcmp(m_infoFromX265->coeffTQIQ, m_infoForQT->coeffTQIQ, size*size*sizeof(int32_t)));

	// compare output
	assert(m_infoFromX265->absSum == m_infoForQT->absSum);
	assert(m_infoFromX265->lastPos == m_infoForQT->lastPos);
	assert(!memcmp(m_infoFromX265->coeffTQ, m_infoForQT->coeffTQ, size*size*sizeof(int32_t)));	
	assert(!memcmp(m_infoFromX265->outResi, m_infoForQT->outResi, CTU_SIZE*CTU_SIZE*sizeof(int16_t)));
}


void hevcQT::getFromIntra(INTERFACE_INTRA* inf_intra, uint8_t textType, uint8_t qp, uint8_t sliceType)
{
	for (int k = 0; k < inf_intra->size; k++)
	{
		memcpy(&m_infoForQT->inResi[k*CTU_SIZE], &inf_intra->resi[k*inf_intra->size], inf_intra->size*sizeof(int16_t));
	}

	m_infoForQT->size = inf_intra->size;
	assert(m_infoForQT->size <= 32);
	m_infoForQT->predMode = 0; //intra
	m_infoForQT->textType = textType;
	m_infoForQT->qp = qp;
	m_infoForQT->sliceType = sliceType; //I
}

void hevcQT::getFromInter(INTERFACE_ME* inf_me, uint8_t textType)
{
#if 0
	for (int k = 0; k < inf_me->size; k++)
	{
		memcpy(&m_infoForQT->inResi[k*CTU_SIZE], &inf_intra->resi[k*inf_intra->size], inf_intra->size*sizeof(int16_t));
	}

	m_infoForQT->size = inf_intra->size;
	assert(m_infoForQT->size <= 32);
	m_infoForQT->predMode = 0; //intra
	m_infoForQT->sliceType = 2; //I
	m_infoForQT->textType = textType;
	m_infoForQT->qp = 30;
	m_infoForQT->transformSkip = 0;
	m_infoForQT->qpBdOffset = 0;
	m_infoForQT->chromaQPOffset = 0;
#endif
}


void hevcQT::setForHWC(INTERFACE_TQ* inf_tq, INTERFACE_INTRA* inf_intra)
{

	// output
	uint8_t	 size = m_infoForQT->size;
	inf_tq->absSum = m_infoForQT->absSum;
	inf_tq->lastPosX = m_infoForQT->lastPos % size;
	inf_tq->lastPosY = m_infoForQT->lastPos / size;
	for (uint8_t k = 0; k < size; k++)
	{
		memcpy(&inf_tq->outResi[k*size], &m_infoForQT->outResi[k*CTU_SIZE], size*sizeof(int16_t));
		for (uint8_t j = 0; j < size; j++)
		{
			inf_tq->oriResi[k*size + j] = (int16_t)m_infoForQT->coeffTQ[k*size + j];
		}
	}


	// input
	inf_tq->resi = inf_intra->resi; // NOTE: pointer assignment, not memcpy
	inf_tq->Size = m_infoForQT->size;
	inf_tq->predMode = m_infoForQT->predMode; //intra
	inf_tq->sliceType = m_infoForQT->sliceType; //I
	inf_tq->textType = m_infoForQT->textType;
	inf_tq->QP = m_infoForQT->qp;
	//inf_tq->qpBdOffset = m_infoForQT->qpBdOffset;
	//inf_tq->chromaQpOffset = m_infoForQT->chromaQPOffset;

}


void hevcQT::set2Cabac(InfoQTandCabac* infoQTandCabac)
{
	uint32_t size = m_infoForQT->size;

	// temp interface to write date from other module
	for (uint32_t k = 0; k<size; k++)
	{
		for (uint32_t j = 0; j < size; j++)
		{
			infoQTandCabac->oriResi[k*size + j] = (short)m_infoForQT->coeffTQ[k*size + j];
		}
	}
	assert(m_infoForQT->lastPos >= 0);
	infoQTandCabac->lastPosX = (uint8_t)(m_infoForQT->lastPos % size);
	infoQTandCabac->lastPosY = (uint8_t)(m_infoForQT->lastPos / size);
	infoQTandCabac->cbf = (m_infoForQT->absSum > 0);
}

// only for 4x4
void hevcQT::set2Intra(InfoQTandIntra* infoQTandIntra)
{
	uint32_t size = m_infoForQT->size;
	for (uint32_t k = 0; k < size; k++)
	{
		memcpy(&(infoQTandIntra->outResi[k*size]), &(m_infoForQT->outResi[k*CTU_SIZE]), size*sizeof(int16_t));
	}
}

void hevcQT::set2Recon(INTERFACE_RECON* inf_recon)
{
	uint32_t size = m_infoForQT->size;
	for (uint32_t k = 0; k < size; k++)
	{
		memcpy(&(inf_recon->resi[k*size]), &(m_infoForQT->outResi[k*CTU_SIZE]), size*sizeof(int16_t));
	}
}


void hevcQT::printInputLog(FILE* fp)
{
	if (//m_infoForQT->size == 4 ||
		m_infoForQT->ctuWidth == 64
		//|| m_infoForQT->textType != 0
		)
	{
		return;
	}
	fprintf(fp, "==================== TQ input ====================\n");
	fprintf(fp, "---------------- TU begin ----------------\n");
	fprintf(fp, "sliceType = %d,  predMode = %d,  textType = %d,  size = %d\n",
		m_infoForQT->sliceType, m_infoForQT->predMode,
		m_infoForQT->textType, m_infoForQT->size);

	fprintf(fp, "qp = %d,  qpBdOffset = %d,  chromaQPOffset = %d,  transformSkip = %d\n\n",
		m_infoForQT->qp, m_infoForQT->qpBdOffset,
		m_infoForQT->chromaQPOffset, m_infoForQT->transformSkip);

	// if print resi only, start here
	fprintf(fp, "Resi:\n");
	for (uint8_t k = 0; k < m_infoForQT->size; k++)
	{
		for (uint8_t j = 0; j < m_infoForQT->size; j++)
		{
			fprintf(fp, "0x%04x ", m_infoForQT->inResi[k*CTU_SIZE + j] & 0xffff);
		}
		fprintf(fp, "\n");
	}
	// if print resi only, end here
	fprintf(fp, "---------------- TU end ----------------\n\n\n");
}

void hevcQT::printOutputLog(FILE* fp)
{
	uint8_t size = m_infoForQT->size;
	if (//size == 4 || 
		m_infoForQT->ctuWidth == 64 
		//|| m_infoForQT->textType != 0
		)
	{
		return;
	}

	fprintf(fp, "==================== TQ output ====================\n");
	fprintf(fp, "---------------- TU begin ----------------\n");
	fprintf(fp, "absSum = %d,  lastPosX = %d,  lasPoxY = %d\n\n",
		m_infoForQT->absSum, m_infoForQT->lastPos%size, m_infoForQT->lastPos / size);

	fprintf(fp, "oriResi:\n");
	for (uint8_t k = 0; k < size; k++)
	{
		for (uint8_t j = 0; j < size; j++)
		{
			fprintf(fp, "0x%04x ", ((int16_t)m_infoForQT->coeffTQ[k*size + j]) & 0xffff);
		}
		fprintf(fp, "\n");
	}

	fprintf(fp, "\noutResi:\n");
	for (uint8_t k = 0; k < size; k++)
	{
		for (uint8_t j = 0; j < size; j++)
		{
			fprintf(fp, "0x%04x ", m_infoForQT->outResi[k*CTU_SIZE + j] & 0xffff);
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "---------------- TU end ----------------\n\n\n");
}


void hevcQT::procTandQ()
{

	int qp = m_infoForQT->qp;

	uint8_t textType = m_infoForQT->textType;

	int qpBdOffset = m_infoForQT->qpBdOffset;
	int chromaQPOffset = m_infoForQT->chromaQPOffset;
	uint8_t predMode = m_infoForQT->predMode;

	uint32_t size = m_infoForQT->size;
	int16_t* inResi = m_infoForQT->inResi;
	uint8_t sliceType = m_infoForQT->sliceType;

	int32_t* coeffT = m_infoForQT->coeffT;
	int32_t* coeffTQ = m_infoForQT->coeffTQ;

	uint32_t* absSum = &(m_infoForQT->absSum); // sum of coeffs after T and Q, changed later
	int* lastPos = &(m_infoForQT->lastPos); // last pos of coef not zero, changed later
	int* quantScale = &(m_infoForQT->quantScale);
	*lastPos = -1;
	*absSum = 0;

	// init some values
	const uint32_t log2BlockSize = hevcSizeConvertToBit[size];
	int transformShift = RK_MAX_TR_DYNAMIC_RANGE - RK_BIT_DEPTH - log2BlockSize - 2; // Represents scaling through forward transform

	// set values from qp
	setQPforQ(qp, textType); //set qpPer, qpRem, qpBits
	int qpPer = m_infoForQT->qpPer;
	int qpRem = m_infoForQT->qpRem;

	//set Quant Matrix and dequantScale for quantization 
	*quantScale = hevcQuantScales[qpRem];

	//NOTE: here inResi stride is [constant CTU_SIZE(64)], instead of [cuStride] in x265
	/* complete  Transform */
	bool isIntra = (predMode == 0);

	dct(coeffT, inResi, size, isIntra, textType); // 0 means inter, 1 means intra.
	/******	Quant ******/
	int add = 0;
	int qbits = RK_QUANT_SHIFT + qpPer + transformShift;
	add = (sliceType == 2 ? 171 : 85) << (qbits - 9); //I slice

	int numCoeff = size * size;
	*absSum += quant(coeffTQ, coeffT, *quantScale, qbits, add, numCoeff, lastPos);
}

void hevcQT::procIQandIT()
{
	uint32_t absSum = m_infoForQT->absSum; // sum of coeffs after T and Q
	uint8_t textType = m_infoForQT->textType;
	uint8_t predMode = m_infoForQT->predMode;

	uint32_t size = m_infoForQT->size;
	int qpPer = m_infoForQT->qpPer;
	int qpRem = m_infoForQT->qpRem;

	int32_t* coeffTQ = m_infoForQT->coeffTQ;
	int32_t* coeffTQIQ = m_infoForQT->coeffTQIQ;
	int* dequantScale = &(m_infoForQT->dequantScale); // dequant scale, changed later

	int16_t* outResi = m_infoForQT->outResi;

	int* lastPos = &(m_infoForQT->lastPos); // last pos of coef not zero, changed later

	bool isIntraLuma = (textType == 0 && predMode == 0); // LUMA & Intra, for dc only decision

	// init some values
	const uint32_t log2BlockSize = hevcSizeConvertToBit[size];
	int transformShift = RK_MAX_TR_DYNAMIC_RANGE - RK_BIT_DEPTH - log2BlockSize - 2; // Represents scaling through forward transform

	if (absSum) //there exist nonzero coeffs after T and Q 
	{
		/* complete Inverse Quantization */
		int shift = RK_QUANT_IQUANT_SHIFT - RK_QUANT_SHIFT - transformShift;
		*dequantScale = hevcInvQuantScales[qpRem] << qpPer;
		dequant(coeffTQIQ, coeffTQ, *dequantScale, size*size, shift);

		assert(*lastPos >= 0);// CHECK_ME: we can't here when no any coeff

		// DC only
		if (*lastPos == 0 && !((size == 4) && isIntraLuma)) //only_dc && (tuwidth != 4 || intra  
		{
			int dc_val = (((coeffTQIQ[0] * 64 + 64) >> 7) * 64 + 2048) >> 12;
			fillResi(outResi, dc_val, CTU_SIZE, size);
		}
		else
		{
			/* complete Inverse Transform */
			bool isIntra = (predMode == 0);

			idct(outResi, coeffTQIQ, size, isIntra, textType); // 0 means inter
		}
	}
	else
	{
		fillResi(outResi, 0, CTU_SIZE, size); //clear resi
	}
}

// for X265
void hevcQT::proc(int predMode)
{
	// get from x265
	getFromX265();

	// print input
	
#if TQ_LOG_IN_X265_INTRA
	if(predMode==0) // Intra
		printInputLog(m_fp_TQ_LOG_X265_INTRA);
#endif
#if TQ_LOG_IN_X265_ME
	if(predMode==1) // Inter
		printInputLog(m_fp_TQ_LOG_X265_ME);
#endif
	// T and Q
	procTandQ();

	// IQ and IT
	procIQandIT();

	// print output
#if TQ_LOG_IN_X265_INTRA	
	printOutputLog(m_fp_TQ_LOG_X265_INTRA);
#endif
#if TQ_LOG_IN_X265_ME
	printOutputLog(m_fp_TQ_LOG_X265_ME);
#endif

	// compare results
	compareX265andHWC();
}

// for Intra
void hevcQT::proc(INTERFACE_TQ* inf_tq, INTERFACE_INTRA* inf_intra, uint8_t textType, uint8_t qp, uint8_t sliceType)
{
	//get from intra
	getFromIntra(inf_intra, textType, qp, sliceType);

#if TQ_LOG_IN_HWC_INTRA
	//print input
	printInputLog(g_fp_TQ_LOG_HWC_INTRA);
#endif
	// T and Q
	procTandQ();

	// IQ and IT
	procIQandIT();

#if TQ_LOG_IN_HWC_INTRA
	// print output
	printOutputLog(g_fp_TQ_LOG_HWC_INTRA);
#endif

	//set info
	setForHWC(inf_tq, inf_intra);
}

// for inter
void hevcQT::proc(INTERFACE_TQ* inf_tq, INTERFACE_ME* inf_me, uint8_t textType)
{
	//get from intra
	getFromInter(inf_me, textType);

#if TQ_LOG_IN_HWC_ME
	//print input
	printInputLog(g_fp_TQ_LOG_HWC_ME);
#endif
	// T and Q
	procTandQ();

	// IQ and IT
	procIQandIT();

#if TQ_LOG_IN_HWC_ME
	// print output
	printOutputLog(g_fp_TQ_LOG_HWC_ME);
#endif

	//set info
	//setForHWC(inf_tq, inf_intra);
}



//! \}
