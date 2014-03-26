#ifndef _QT_H_
#define _QT_H_

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComDataCU.h"

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
#define CLIP(a, min, max)			(((a) < min) ? min : (((a) > max) ? max : (a)))



namespace x265 {
	// private namespace

	//! \ingroup TLibCommon
	//! \{

	// ====================================================================================================================
	// Constants
	// ====================================================================================================================

#define QP_BITS 15

#define HWC_DEBUG_MODE 1

	// ====================================================================================================================
	// QT module info definition
	// ====================================================================================================================
	typedef struct
	{
		// input 
		int16_t *inResi;
		uint32_t size;
		PredMode predMode;			// inter or intra
		SliceType sliceType;
		TextType textType;			// Y, U+V, U, V
		bool transformSkip;

		int qp;
		int qpBdOffset;
		int chromaQPOffset;

		// debug info
		int qpscaled;
		int qpPer;
		int qpRem;
		int qpBits;
		int quantScale;
		int dequantScale;
		int32_t *coeffT;
		int32_t *coeffTQIQ;

		// output
		int32_t *coeffTQ;
		uint32_t absSum;
		int		 lastPos;
		int16_t *outResi; // Input Residual, before QT and IQ/IT
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
		TextType	textType;
		SliceType	sliceType;
		PredMode	predMode;

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

		uint32_t size;
		PredMode predMode;
		int qp;
		int qpBdOffset;
		int chromaQPOffset;

		SliceType sliceType;
		TextType textType;			// Y, U+V, U, V

		uint32_t cuStride;			// cu stride(store residual)
		bool transformSkip;
		uint32_t absPartIdx;		// TU position in CTU
		
		//debug info
		int		 lastPos; // last position of  coeff after T and Q		
		int		 qpscaled;
		int		 qpPer;
		int		 qpRem;
		int		 qpBits;
		int		 mode;	// inter: REG_DCT; intra: LUMA? predDirection: REG_DCT. 

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
		void creat();
		void destroy();
		void proc();
		void procTandQ();
		void procIQandIT();
		void clearOutResi(); // inter, when absSum==0, clear outResi to zero
		void setQPforQ(int qpy, TextType ttype, int qpBdOffset, int chromaQPOffset);
		void fillResi(short* resi, int val, uint32_t resiStride, uint32_t tuSize); // to be debugged
		
		/************************************************************************/
		/*                       debug in HWC                                  */
		/************************************************************************/
		// set info by other modules, called in other modules
		void setFromIntra(InfoQTandIntra* infoQTandIntra);
		void setFromInter();

		// set info
		void set2Cabac(InfoQTandCabac* infoQTandCabac);
		void set2Intra(InfoQTandIntra* infoQTandIntra);
		void set2Recon();

		// info for QT module
		infoForQT*		  m_infoForQT;
		/************************************************************************/
		/*                       debug in x265                                   */
		/************************************************************************/
		// get input from x265
		void hevcQT::getInputFromX265(int16_t *inResi, TComDataCU* cu,
			uint32_t absPartIdx, TextType ttype, uint32_t cuStride, uint32_t tuWidth); 
		void hevcQT::getFromX265();
	
		// get output from x265
		void getOutputFromX265(int16_t *outResi); // intra
		void getOutputFromX265(int sw, int16_t *outResi); // inter

		// get debug info from x265
		void getDebugInfoFromX265_0(int lastPos, int absSum,
			int qpScaled, int qpPer, int qpRem, int qpBits, int32_t* quantCoef, int32_t* tmpCoeff, int32_t* coeff); //get info after T and Q	
		void getDebugInfoFromX265_1(int dequantScale, int shift, int32_t* coeffTQIQ);//get info after IQ
		
		// compare results with x265
		int compareX265andHWC();

		// info from x265
		infoFromX265*     m_infoFromX265;
	
	protected:

	
	private:
		//tools
		void transpose(short* inArray, int size);
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
		void idct(short* out, int* in, uint32_t tuWidth, bool isIntra, TextType textType);


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
		void dct(int* out, short* in, uint32_t tuWidth, bool isIntra, TextType textType);	

		uint32_t quant(int* coeffTQ, int* coeffT, int quantScale, int qBits, int add, int numCoeff, int32_t* lastPos);//return absSum
		void dequant(int* coeffTQIQ, int* coeffTQ, int dequantScale, int num, int shift);
	};
}
//! \}

#endif