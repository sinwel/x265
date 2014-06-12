#ifndef _QT_H_
#define _QT_H_

#define X265_TEST_MODE 0

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "module_interface.h"
#include <macro.h>
#include "rk_define.h"

extern int idst_4x4_coeff[4][4];
extern int idct_4x4_coeff[4][4];
extern int idct_8x8_coeff[8][8];
extern int idct_8x8_coeff_even[4][4];
extern int idct_8x8_coeff_odd[4][4];
extern int idct_16x16_coeff[16][16];

extern int idct_16x16_coeff_even[8][8];
extern int idct_16x16_coeff_even_part0[8][4];
extern int idct_16x16_coeff_even_part1[8][4];
extern int idct_16x16_coeff_odd[8][8];
extern int idct_16x16_coeff_odd_part0[8][4];
extern int idct_16x16_coeff_odd_part1[8][4];

extern int idct_32x32_coeff[32][32];
extern int idct_32x32_coeff_even[16][16];
extern int idct_32x32_coeff_even_part0[16][4];
extern int idct_32x32_coeff_even_part1[16][4];
extern int idct_32x32_coeff_even_part2[16][4];
extern int idct_32x32_coeff_even_part3[16][4];

extern int idct_32x32_coeff_odd[16][16];
extern int idct_32x32_coeff_odd_part0[16][4];
extern int idct_32x32_coeff_odd_part1[16][4];
extern int idct_32x32_coeff_odd_part2[16][4];
extern int idct_32x32_coeff_odd_part3[16][4];

extern int dst_4x4_coeff[4][4];
extern int dct_4x4_coeff[4][4];
extern int dct_8x8_coeff[8][8];

extern int dct_16x16_coeff_odd[8][8];
extern int dct_16x16_coeff_even[8][4];
extern int dct_16x16_coeff[16][16]; // not used actually
extern int dct_32x32_coeff_odd[16][16];
extern int dct_32x32_coeff_even[16][8];
extern int dct_32x32_coeff[32][32]; // not used actually

// SWC coeff
extern int t4_coeff[4][4];
// private namespace

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define GET_DBG_INFO_FOR_RTL      0
/* iDCT(8x8-32x32)  in:after transpose,    out:normal            (4x4) in:normal   out:normal
   DCT(8x8-32x32)   in:normal,             out:after transpose   (4x4) in:normal   out:normal
 */
 
#define GET_TQ_INFO_FOR_RTL       0
/* data form: (qp)(slice_type)(text_type)(tran_type)[data]
   input:   before T, every row or all elements(only for 4x4)
   output:  after Q and after IT, every row or all elements(only for 4x4)
 */

#define RK_QP_BITS 15
#define RK_BIT_DEPTH			  8
#define RK_MAX_TR_DYNAMIC_RANGE  15 // Maximum transform dynamic range (excluding sign bit)
#define RK_QUANT_SHIFT           14 // Q(4) = 2^14
#define RK_QUANT_IQUANT_SHIFT    20 // Q(QP%6) * IQ(QP%6) = 2^20
// ====================================================================================================================
// Data Definitions
// ====================================================================================================================

#define CTU_SIZE 64

// ====================================================================================================================
// QT module info definition
// ====================================================================================================================
typedef struct
{
	// input 
	int16_t		*inResi;
	uint8_t	    size;

	uint8_t		predMode;	// 0:intra, 1: inter
	uint8_t		sliceType;  // 0:B, 1:P, 2:I
	uint8_t		textType;	// 0: luma, 1: chroma


	bool		transformSkip;

	int			qp;
	int			qpBdOffset;
	int			chromaQPOffset;

	// debug info
	int			qpscaled;
	int			qpPer;
	int			qpRem;
	int			qpBits;
	int			quantScale;
	int			dequantScale;
	int32_t*	coeffT;
	int32_t*	coeffTQIQ;
	int			ctuWidth;

	// output
	int32_t*	coeffTQ;
	uint32_t	absSum;
	int			lastPos;
	int16_t*	outResi; // Input Residual, before QT and IQ/IT
}infoForQT;


// ====================================================================================================================
// HWC interface definition
// ====================================================================================================================
/* info between Intra and TQ*/
typedef struct
{
	// input
	short*		inResi;
	int			qp;
	int			size; //TU size
	uint8_t		textType; // 0: luma,  1: chroma
	uint8_t		predMode; // 0: intra, 1: inter
	uint8_t		sliceType;// 0:B, 1:P, 2:I

	int			qpBdOffset; // init in sps
	int			chromaQpoffset; // init in sps

	bool		tranformSkip;

	//output
	short*	    outResi; // only for 4x4
}InfoQTandIntra;

/* info between CABAC and TQ */
typedef struct
{
	//output
	short*	    oriResi; // after T and Q
	bool		cbf;	 // 0: zero resi, 1:not zero resi
	uint8_t		lastPosX;
	uint8_t		lastPosY;
}InfoQTandCabac;

/* info between Recon and TQ */
typedef struct
{
	//output
	short*	    outResi; // only for 4x4
}InfoQTandRecon;



// ====================================================================================================================
// x265  info definition
// ====================================================================================================================
/* Input Param */
typedef struct
{
	//input
	int16_t *inResi; // Input Residual, before QT and IQ/IT

	uint8_t size;
	uint8_t textType;			//0: luma, 1:Chroma
	uint8_t predMode;
	uint8_t sliceType;

	int qp;
	int qpBdOffset;
	int chromaQPOffset;


	uint32_t cuStride;			// cu stride(store residual)
	bool	 transformSkip;
	uint32_t absPartIdx;		// TU position in CTU

	//debug info
	int		 lastPos; // last position of  coeff after T and Q		
	int		 qpscaled;
	int		 qpPer;
	int		 qpRem;
	int		 qpBits;
	int		 mode;	// inter: REG_DCT; intra: LUMA? predDirection: REG_DCT. 
	int		 ctuWidth;

	int		 quantScale;
	int		 dequantScale;
	//int32_t* quantCoef;     ///< array of quantization matrix coefficient 4x4

	int		 shift; //input for IQ
	int32_t* coeffT; // coeff after T
	int32_t* coeffTQ;   // coeff after T and Q
	int32_t* coeffTQIQ; // coeff after T, Q, IT	

	uint32_t absSum;// absSum of coeff after T and Q, use for setting cbf
	int16_t *outResi; // Output Residual, after QT and IQ/IT
	//output
}infoFromX265;


// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// transform and quantization class
class hevcQT
{
public:

	hevcQT();
	~hevcQT();

	// main process
	void creat(); //actually not used
	void destroy(); //actually not used
	void proc(int predMode); // for X265, predMode=0, means Intra; preMode=1, means Inter.
	void proc(INTERFACE_TQ* inf_tq, INTERFACE_INTRA* inf_intra, uint8_t textType, uint8_t qp, uint8_t sliceType); // for intra
	void proc(INTERFACE_TQ* inf_tq, INTERFACE_ME* inf_me, uint8_t textType, uint8_t qp, uint8_t sliceType); //for inter
	void procTandQ();
	void procIQandIT();
	void setQPforQ(int qpy, uint8_t ttype);
	void fillResi(short* resi, int val, uint32_t resiStride, uint32_t tuSize); // to be debugged
	void printInputLog(FILE* fp);
	void printOutputLog(FILE* fp);
	/************************************************************************/
	/*                       debug in HWC                                  */
	/************************************************************************/
	// set info by other modules, called in other modules
	void getFromIntra(INTERFACE_INTRA* inf_intra, uint8_t textType, uint8_t qp, uint8_t sliceType);
	void getFromInter(INTERFACE_ME* inf_me, uint8_t textType, uint8_t qp, uint8_t sliceType);
	void setForHWC(INTERFACE_TQ* inf_tq, INTERFACE_INTRA* inf_intra);
    void setForHWC(INTERFACE_TQ* inf_tq, INTERFACE_ME* inf_me);
	// set info
	void set2Cabac(InfoQTandCabac* infoQTandCabac);
	void set2Intra(InfoQTandIntra* infoQTandIntra);
	void set2Recon(INTERFACE_RECON* inf_recon);

	// info for QT module
	infoForQT*		  m_infoForQT;
	FILE*			  m_fp_TQ_LOG_X265_INTRA;
	FILE*			  m_fp_TQ_LOG_X265_ME;

    // debug info for RTL
    void printResi(int16_t *resi, uint8_t size);
    void printResiOneDim(int16_t *in, int16_t *outD1, int16_t *outD2,int size, bool isT, int trType);
    void printResiTQiQiT(int16_t *inT, int16_t *outT, int16_t *outQ, int16_t *outIQ, int16_t* outIT,
									 uint8_t trType);


#if GET_DBG_INFO_FOR_RTL
    void printResiForTandIT(FILE* fpIn, FILE* fpOut, int16_t *in, int16_t *out, uint8_t size, bool isT, uint8_t trType);
    FILE*             m_fp_input4x4data;
    FILE*             m_fp_output4x4data;
    FILE*             m_fp_input4x4data_8x8chr;   
    FILE*             m_fp_output4x4data_8x8chr;
    FILE*             m_fp_input8x8data;
    FILE*             m_fp_output8x8data;  
    FILE*             m_fp_input16x16data;
    FILE*             m_fp_output16x16data;      
    FILE*             m_fp_input32x32data;
    FILE*             m_fp_output32x32data;     
#endif
#if GET_TQ_INFO_FOR_RTL
    void             fprintResiForTQiT(FILE* fpIn, FILE* fpOut, int16_t *inT, int16_t *outQ, int16_t *outIT,
                                           uint8_t trType);
    FILE*             m_fp_t_input4x4data;
    FILE*             m_fp_qit_output4x4data;
    //FILE*             m_fp_t_input4x4data_8x8chr;
    //FILE*             m_fp_qit_output4x4data_8x8chr;
    FILE*             m_fp_t_input8x8data;
    FILE*             m_fp_qit_output8x8data;    
    FILE*             m_fp_t_input16x16data;
    FILE*             m_fp_qit_output16x16data;   
    FILE*             m_fp_t_input32x32data;
    FILE*             m_fp_qit_output32x32data;    
#endif
	/************************************************************************/
	/*                       debug in x265                                   */
	/************************************************************************/
	void getFromX265();

	// compare results with x265
	void compareX265andHWC();
	// info from x265

	infoFromX265*     m_infoFromX265;
    int               g_count_block;

	void getOutputFrom();
protected:


private:
	//tools
	void transpose(short* inArray, int size);

    //======================= SWC version =======================
    void _ist4(short *out, short *in, int shift);
    void _idst4(short *dst, int *src);
    void _ict4(short *out, short *in, int shift);
    void _idct4(short *dst, int *src);
    void _st4(short *dst, short *src, int shift);
    void _dst4(int *dst, short *src);
    void _ct4(short *dst, short *src, int shift);
    void _dct4(int *dst, short *src);


    //======================= HWC version =======================
	/* inverse transform*/
	void it4(short* out, short* in, int shift, int trType, int dim); //idct4x4, or idst4x4
	void it8(short* out, short* in, int shift);
	void it16_0to3(short *out, short* in, int shift);
	void it16_4to7(short *out, short *in, int shift);
	void it32_0to3(short *out, short *in, int shift);
	void it32_4to7(short *out, short *in, int shift);
	void it32_8to11(short *out, short *in, int shift);
	void it32_12to15(short *out, short *in, int shift);

	void idct4(short *out, int *in, int trType);
	void idct8(short *out, int *in);
	void idct16(short *out, int *in);
	void idct32(short *out, int *in);

	// isIntra: intra(1), inter(0), when is Intra, 4x4 luma do IDST
	// textType: is only for Intra 4x4, to decide to do IDST or IDCT
	void idct(short* out, int* in, uint32_t tuWidth, bool isIntra, uint8_t textType);


	/* transform */
	void t4(short* out, short* in, int shift, int trType, int dim);
	void t8(short* out, short* in, int shift);
	void t16(short* out, short* in, int shift);
	void t32(short* out, short* in, int shift);

	void dct4(int *out, short *in, int trType);
	void dct8(int *out, short *in);
	void dct16(int *out, short *in);
	void dct32(int *out, short *in);

	// isIntra: intra(1), inter(0), when is Intra, 4x4 luma do IDST
	// textType: is only for Intra 4x4, to decide to do IDST or IDCT
	void dct(int* out, short* in, uint32_t tuWidth, bool isIntra, uint8_t textType);

	uint32_t quant(int* coeffTQ, int* coeffT, int quantScale, int qBits, int add, int numCoeff, int32_t* lastPos);//return absSum
	void dequant(int* coeffTQIQ, int* coeffTQ, int dequantScale, int num, int shift);
};

//! \}

#endif
