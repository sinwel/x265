#ifndef _CABAC_H
#define _CABAC_H
extern unsigned char state_table[3][52][165];

#include <fstream>
#include "../Lib/TLibCommon/CommonDef.h"
//#include "../Lib/TLibCommon/TComSlice.h"
#include "../Lib/TLibCommon/TComPic.h"
#include "../Lib/TLibCommon/TComSampleAdaptiveOffset.h"


#define  RDO_MAX_DEPTH 5
#define	CONTEXT_NUM			165

#define  INTRA_PU_4x4_MODIFY 1
#define  INTRA_SPLIT_FLAG_MODIFY 1
#define CABAC_INT_MODIFY 1
#define RK_CABAC_H 1
#define RK_CABAC_TEST 0
#define CABAC_EST_Accuracy_CutDown 0

#define MAX(X, Y)           ((X)>(Y)?(X):(Y))
typedef int Int;
typedef uint32_t UInt;

extern __int64 g_intra_est_bit_luma_pred_mode_all_case[RDO_MAX_DEPTH][256][35];

extern int g_intra_cu_best_mode[RDO_MAX_DEPTH][256];
extern __int64 g_SplitFlag_bit[RDO_MAX_DEPTH];

extern __int64 g_intra_est_bit_coded_sub_block_flag[3][RDO_MAX_DEPTH][256] ;
extern __int64 g_intra_est_sig_coeff_flag[3][RDO_MAX_DEPTH][256];
extern __int64 g_intra_est_bit_coeff_abs_level_greater1_flag[3][RDO_MAX_DEPTH][256];
extern __int64 g_intra_est_bit_coeff_abs_level_greater2_flag[3][RDO_MAX_DEPTH][256];
extern __int64 g_intra_est_bit_coeff_sign_flag[3][RDO_MAX_DEPTH][256] ;
extern __int64 g_intra_est_bit_coeff_abs_level_remaining[3][RDO_MAX_DEPTH][256] ;
extern __int64 g_intra_est_bit_last_sig_coeff_xy[3][RDO_MAX_DEPTH][256];

extern __int64 g_intra_est_bit_cbf[3][RDO_MAX_DEPTH][256] ;

extern __int64 g_intra_est_bit_luma_pred_mode[RDO_MAX_DEPTH][256] ;
extern __int64 g_intra_est_bit_chroma_pred_mode[RDO_MAX_DEPTH][256] ;
extern __int64 g_intra_est_bit_part_mode[RDO_MAX_DEPTH][256] ;

extern __int64 g_est_bit_tu[2][RDO_MAX_DEPTH][256] ;
extern __int64 g_est_bit_tu_luma_NoCbf[2][RDO_MAX_DEPTH][256];
extern __int64 g_est_bit_tu_cb_NoCbf[2][RDO_MAX_DEPTH][256];
extern __int64 g_est_bit_tu_cr_NoCbf[2][RDO_MAX_DEPTH][256];

extern __int64 g_est_bit_cu[2][RDO_MAX_DEPTH][256];

extern __int64 g_est_bit_cu_split_flag[2][RDO_MAX_DEPTH][256][2];
//Enc_est
typedef struct
{
	int val_len_r;                //[?:0]
	int binval0;                          //[0]
	int binval1;                          //[0]
	int binval2;                          //[0]
	int binval3;                          //[0]
	unsigned int est_bit_in;            //
}Enc_input_est;
typedef struct
{
	unsigned int est_bit_out;       //
}Enc_output_est;

//decision_est
typedef struct
{
	unsigned int est_bit_in;            //
	int binval_in;                     //[0]
	int valmps_in;                   //[0]
	int pstateidx_in;                //[5:0]
}Decision_unit_input_est;
typedef struct
{
	unsigned int est_bit_out;       //
	int pstateidx_out;               //[5:0]
	int valmps_out;                  //[0]
}Decision_unit_output_est;


typedef struct
{
	//SPS
	unsigned long	video_parameter_set_id:4;
	unsigned long	seq_parameter_set_id:4;
	unsigned long	chroma_format_idc:2;                    //dec ctr
	unsigned long	pic_width_in_luma_samples:13;            //dec ctr
	unsigned long	pic_height_in_luma_samples:13;           //dec ctr
	unsigned long	bit_depth_luma:4;                         //dec ctr
	unsigned long	bit_depth_chroma:4;                     //dec ctr
	unsigned long	log2_max_pic_order_cnt_lsb:5;           		//cabac only
	unsigned long	log2_max_luma_coding_block_size:3;      //dec ctr       Log2CtbSizeY =  log2_max_luma_coding_block_size  + log2_maxa_coding_block_depth
	unsigned long	log2_maxa_coding_block_depth:2;          //dec ctr
	unsigned long	log2_max_transform_block_size:3;         		//cabac only
	unsigned long	log2_max_transform_block_depth:2;   			//cabac only
	unsigned long  max_transform_hierarchy_depth_inter 	:3; 		//cabac only
	unsigned long  max_transform_hierarchy_depth_intra	:3;			//cabac only
	unsigned long  scaling_list_enabled_flag:1;				 //dec ctr
	unsigned long  amp_enabled_flag:1;                     			//cabac only
	unsigned long  sample_adaptive_offset_enabled_flag:1;  	 //dec ctr
	unsigned long  pcm_enabled_flag:1;						 //dec ctr
	unsigned long  pcm_sample_bit_depth_luma:4;				 //dec ctr
	unsigned long  pcm_sample_bit_depth_chroma :4;;        	 //dec ctr
	unsigned long  pcm_loop_filter_disabled_flag :1;		  //dec ctr
	unsigned long  log2_max_pcm_luma_coding_block_size:3;    			//cabac only
	unsigned long  log2_max_pcm_luma_coding_block_depth :2;  			//cabac only
	unsigned long  num_short_term_ref_pic_sets :7;   					//cabac only
	unsigned long  long_term_ref_pics_present_flag:1;					//cabac only
	unsigned long  num_long_term_ref_pics_sps :6;						//cabac only

	// 	unsigned long	lt_ref_pic_poc_lsb_sps[33];
	// 	bool			used_by_curr_pic_lt_sps_flag[33];

	unsigned long  sps_temporal_mvp_enabled_flag:1;  					//cabac only
	unsigned long  sps_strong_intra_smoothing_enabled_flag:1; //dec ctr

	unsigned long  transform_skip_rotation_enabled_flag:1;
	unsigned long  transform_skip_context_enabled_flag:1;
	unsigned long  intra_block_copy_enabled_flag:1;
	unsigned long  residual_dpcm_intra_enabled_flag:1;
	unsigned long  residual_dpcm_inter_enabled_flag:1;
	unsigned long  extended_precision_processing_flag:1;
	unsigned long  intra_smoothing_disabled_flag:1;

	///**///
	unsigned long MaxCUWidth;
	unsigned long MinCUWidth;
	unsigned long mExisted;
	unsigned long Sps_length;

	unsigned long  log2_min_pcm_luma_coding_block_size:3;
	unsigned long  log2_min_luma_coding_block_size:3;
	unsigned long  log2_min_transform_block_size:3;


	unsigned long				log2_min_coding_block_depth_bt_cu_tu:3;
	unsigned long				log2_max_coding_block_depth_bt_cu_tu:3;
	unsigned long				log2_min_coding_block_depth_in_pcm:3;
	unsigned long				log2_max_coding_block_depth_in_pcm:3;
	unsigned long				log2_min_tu_size;
	unsigned long				log2_min_cu_size;
	unsigned long				log2_max_coding_depth_for_dec;

	unsigned long				cu_in_ctu_per_line;
	unsigned long				tu_in_ctu_per_line;
	unsigned long				ctu_width_in_pic;
	unsigned long				ctu_height_in_pic;
	unsigned long				ctu_num_in_pic;
	unsigned long				Max_cu_in_ctu;
	unsigned long				Max_tu_in_ctu;
	unsigned long				tu_x_num;

	long						CoeffMin_y;
	long						CoeffMin_c;
	long						CoeffMax_y;
	long						CoeffMax_c;
	long						coeff_max_len_y;
	long						coeff_max_len_c;
	long						QpBdOffsetY;
	long						QpBdOffsetC;
	long						RawCtuBits ;


	//PPS
	unsigned long pps_pic_parameter_set_id:6;
	unsigned long pps_seq_parameter_set_id:4;
	unsigned long dependent_slice_segments_enabled_flag:1; 		//cabac dec
	unsigned long output_flag_present_flag :1;   				//cabac dec
	unsigned long num_extra_slice_header_bits :3;             	//cabac dec
	unsigned long sign_data_hiding_flag :1;        				//cabac dec
	unsigned long cabac_init_present_flag :1;   				//cabac dec
	unsigned long constrained_intra_pred_flag :1;     						//dec ctr
	unsigned long num_ref_idx_l0_default_active:4;  			//cabac dec
	unsigned long num_ref_idx_l1_default_active:4;  			//cabac dec
	long  init_qp:7;  									//cabac dec	//dec ctr
	unsigned long transform_skip_enabled_flag :1;				//cabac dec
	unsigned long cu_qp_delta_enabled_flag:1;					//cabac dec
	unsigned long  Log2MinCuQpDeltaSize:3;						//cabac dec	//dec ctr
	long  pps_cb_qp_offset:5; 												//dec ctr
	long  pps_cr_qp_offset :5;												//dec ctr
	unsigned long pps_slice_chroma_qp_offsets_present_flag:1;	//cabac dec
	unsigned long  weighted_pred_flag :1;									//dec ctr
	unsigned long  weighted_bipred_flag :1; 								//dec ctr
	unsigned long transquant_bypass_enabled_flag :1;			//cabac dec
  	unsigned long  tiles_enabled_flag :1;						//cabac dec	//dec ctr
	unsigned long  entropy_coding_sync_enabled_flag :1;			//cabac dec	//dec ctr
	unsigned long pps_loop_filter_across_slices_enabled_flag :1;//cabac dec
	unsigned long loop_filter_across_tiles_enabled_flag :1;					//dec ctr
	unsigned long deblocking_filter_override_enabled_flag :1;	//cabac dec
	unsigned long pps_deblocking_filter_disabled_flag	 :1; 	//cabac dec
	long pps_beta_offset_div2 :4;								//cabac dec	dec ctr
	long pps_tc_offset_div2 :4;
	unsigned long lists_modification_present_flag :1;			//cabac dec

	unsigned long  log2_parallel_merge_level :3; //Log2ParMrgLevel 			//dec ctr
	unsigned long slice_segment_header_extension_present_flag:1;	//cabac dec
	unsigned long log2_transform_skip_max_size;					//cabac dec
	 long num_tile_columns:6;							//cabac dec
	 long num_tile_rows:6;								//cabac dec
	unsigned char column_width[20];								//cabac dec
	unsigned char row_height[22];								//cabac dec

}Sps_Pps_struct;
typedef struct
{
unsigned long First_slice_segment_in_pic_flag	:1																					;
unsigned long No_output_of_prior_pics_flag	:1																					   ;
unsigned long Slice_pic_parameter_set_id	:10																						   ;
unsigned long Dependent_slice_segment_flag	:1																				   ;
unsigned long Slice_segment_address	:14																							   ;
unsigned long slice_type	:2																												   ;
unsigned long Pic_output_flag	:1																										   ;
unsigned long Colour_plane_id	:2																									   ;
unsigned long Rps_length:11;//length of rps data,this part maybe not be encoded by hardware 	11			 
unsigned long Slice_pic_order_cnt_lsb	:16																							   ;
unsigned long Short_term_ref_pic_set_sps_flag	:1																				   ;
unsigned long inter_ref_pic_set_prediction_flag	:1																				   ;
unsigned long delta_idx_minus1:6;// stRpsIdx == num_short_term_ref_pic_sets  in slice	6						   
unsigned long Delta_rps_sign	:1																										   ;
unsigned long Abs_delta_rps_minus1	:15																							   ;
unsigned long Used_by_curr_pic_flag[16];//	:1																							   ;
unsigned long Use_delta_flag[16];//	:1																										   ;
unsigned long Num_negative_pics	:4;
unsigned long Num_positive_pics	:4;
unsigned long   delta_poc_s0_minus1[16]	/*:15*/																						   ;
unsigned long used_by_curr_pic_s0_flag[16]	/*:1*/																					   ;
unsigned long   delta_poc_s1_minus1[16]	/*:15*/																						   ;
unsigned long used_by_curr_pic_s1_flag[16]/*	:1*/																					   ;
unsigned long   Short_term_ref_pic_set_idx:	6																					   ;
unsigned long Num_long_term_sps	:6																								   ;
unsigned long Num_long_term_pics	:6																								   ;
unsigned long lt_idx_sps[ 128] 	/*:5*/																										   ;
unsigned long     poc_lsb_lt[128 ]  	/*:15*/																									   ;
unsigned long     used_by_curr_pic_lt_flag[128 ]/*:	1*/																				   ;
unsigned long delta_poc_msb_present_flag[128 ]  	/*:1*/																			   ;
unsigned long        delta_poc_msb_cycle_lt[ 128 ]	;//16
unsigned long Slice_temporal_mvp_enabled_flag	:1																			   ;
unsigned long slice_sao_luma_flag	:1																								   ;
unsigned long slice_sao_chroma_flag	:1																							   ;
unsigned long Num_ref_idx_active_override_flag:	1																			   ;
unsigned long num_ref_idx_l0_active_minus1	:4																					   ;
unsigned long num_ref_idx_l1_active_minus1	:4																					   ;
unsigned long ref_pic_list_modification_flag_l0	:1																				   ;
unsigned long list_entry_l0[15+1]/*:	3*/															   ;
unsigned long ref_pic_list_modification_flag_l1	:1																				   ;
unsigned long list_entry_l1[15+1]/*:	3*/															   ;
unsigned long Mvd_l1_zero_flag	:1																									   ;
unsigned long Cabac_init_flag	:1																										   ;
unsigned long collocated_from_l0_flag	:1																							   ;
unsigned long collocated_ref_idx	:4																									   ;
unsigned long luma_log2_weight_denom:	3																						   ;
unsigned long chroma_log2_weight_denom	:3																					   ;
unsigned long     luma_weight_l0_flag[16]	/*:1*/ ;//* num_ref_idx_l0_active							 ;
unsigned long     Chroma_weight_l0_flag[16]	/*:1*/ ;//* num_ref_idx_l0_active						 ;
			 long delta_luma_weight_l0[16]/*:	8*/																							   ;
			 long luma_offset_l0[16]	/*:8*/																									   ;
			 long delta_Cb_weight_l0[16]/*:	8*/																								   ;
			 long delta_Cb_offset_l0[16]	/*:10*/																								  ;
			 long delta_Cr_weight_l0[16]	/*:8*/																								   ;
			 long delta_Cr_offset_l0[16]	/*:10*/																								  ;
unsigned long     luma_weight_l1_flag[16]	/*:1*/ ;//* num_ref_idx_l1_active							 ;
unsigned long     Chroma_weight_l1_flag[16]/*:	1*/;// * num_ref_idx_l1_active						 ;
			 long delta_luma_weight_l1[16]	/*:8*/																							   ;
			 long luma_offset_l1[16]	/*:8*/																									   ;
			 long delta_Cb_weight_l1[16]	/*:8*/																								   ;
			 long delta_Cb_offset_l1[16]	/*:10*/																								  ;
			 long delta_Cr_weight_l1[16]	/*:8*/																								   ;
			 long delta_Cr_offset_l1[16]	/*:10*/																								  ;
unsigned long Five_minus_max_num_merge_cand	:3																			   ;
			 long Slice_qp_delta	:7																										   ;
			 long slice_cb_qp_offset	:5																									   ;
			 long slice_cr_qp_offset	:5																									   ;
unsigned long Deblocking_filter_overide_flag	:1																				   ;
unsigned long Slice_deblocking_filter_disabled_flag	:1																		   ;
			 long Slice_beta_offset_div2	:4																							   ;
			 long Slice_tc_offset_div2:	4																								   ;
unsigned long Slice_loop_filter_across_slices_enabled_flag:	1																   ;
unsigned long Num_entry_point_offsets	:9																						   ;
unsigned long offset_len_minus1:5;// waiting for the end of the slice,update every tile or line	5				   
unsigned long Entry_point_offset_minus1[31+1]/*:6*/;//* Num_entry_point_offsets;//wait for the end of the slice,SLIE or the end of  the picture	6* Num_entry_point_offsets
/*unsigned long Maskbits (for align 32bit)	nbit;*/
}Slice_header_struct;


using namespace x265;

enum CTUSTATUS
{
	CTU_EST=0,
	CU_EST,
	PU_EST_WAIT,
	PU_EST,
	TU_EST_WAIT,
	TU_EST,
	CU_EST_WAIT,
};

typedef struct 
{
	bool       mergeUpFlag;
	bool       mergeLeftFlag;
	int        typeIdx;
//	int        subTypeIdx;                ///< indicates EO class or BO band position
	int        offset[4];
} SaoParam_CABAC;

#define	SAO_MERGE_FLAG_OFFSET									0X0              																						          				//0
#define  SAO_TYPE_IDX_OFFSET						SAO_MERGE_FLAG_OFFSET						+		   0X1                                									//1
#define  SPLIT_CU_FLAG_OFFSET						SAO_TYPE_IDX_OFFSET							+		   0X1                                  								    //2
#define  CU_TRANSQUANT_BYPASS_FLAG_OFFSET			SPLIT_CU_FLAG_OFFSET						+		   0X3                                  					    //5
#define  SKIP_FLAG_OFFSET							CU_TRANSQUANT_BYPASS_FLAG_OFFSET			+		   0X1                                  						    //6
#define  PRED_MODE_OFFSET							SKIP_FLAG_OFFSET							+		   0X3                                  										    //9
#define  PART_MODE_OFFSET							PRED_MODE_OFFSET							+		   0X1    //帧间最后一个bin用于2nxn nx2n在hm里叫AMP  //10
#define  PREV_INTRA_LUMA_PRED_MODE					PART_MODE_OFFSET							+		   0X4                                							//14
#define  INTRA_CHROMA_PRED_MODE_OFFSET				PREV_INTRA_LUMA_PRED_MODE					+		   0X1   //相对标准多了一个                  //15
#define  RQT_ROOT_CBF_OFFSET						INTRA_CHROMA_PRED_MODE_OFFSET				+		   0X2                                 					//17
#define  MERGE_FLAG_OFFSET							RQT_ROOT_CBF_OFFSET						    +		   0X1                                								//18
#define  MERGE_IDX_OFFSET							MERGE_FLAG_OFFSET							+		   0X1                                  									    //19
#define	INTER_PRED_IDC_OFFSET						MERGE_IDX_OFFSET							+		   0X1                                  								    //20
#define	REF_IDX_L0_OFFSET							INTER_PRED_IDC_OFFSET						+		   0X5   	                              									//25
#define  MVP_FLAG_OFFSET                       		REF_IDX_L0_OFFSET							+		   0X2                                										//27
#define  SPLIT_TRANSFORM_FLAG_OFFSET           		MVP_FLAG_OFFSET                         	+		   0X1                                  						    //28
#define  CBF_LUMA_OFFSET                       		SPLIT_TRANSFORM_FLAG_OFFSET             	+		   0X3                                  							    //31
#define  CBF_CHROMA_OFFSET                     		CBF_LUMA_OFFSET                         	+		   0X2    //代码里少了一个                               			//33
#define  ABS_MVD_GREATER0_FLAG_OFFSET				CBF_CHROMA_OFFSET                      		+		   0X4                                 						//37
#define  ABS_MVD_GREATER1_FLAG_OFFSET				ABS_MVD_GREATER0_FLAG_OFFSET				+		   0X1                                 					//38
#define  CU_QP_DELTA_OFFSET							ABS_MVD_GREATER1_FLAG_OFFSET				+		   0X1  //相对标准多了一个								//39
#define	TRANSFORM_SKIP_FLAG_OFFSET					CU_QP_DELTA_OFFSET							+		   0X3 															//42
#define  LAST_SIGNIFICANT_COEFF_X_PREFIX_OFFSET		TRANSFORM_SKIP_FLAG_OFFSET					+		   0X2                                  			    //44
#define  LAST_SIGNIFICANT_COEFF_Y_PREFIX_OFFSET		LAST_SIGNIFICANT_COEFF_X_PREFIX_OFFSET  	+		   0X12                                  		//62
#define  SIGNIFICANT_COEFF_GROUP_FLAG_OFFSET		LAST_SIGNIFICANT_COEFF_Y_PREFIX_OFFSET		+		   0X12                               			//80
#define  SIGNIFICANT_COEFF_FLAG_OFFSET				SIGNIFICANT_COEFF_GROUP_FLAG_OFFSET			+		   0X4                               				    //84
#define  COEFF_ABS_LEVEL_GREATER1_FLAG_OFFSET		SIGNIFICANT_COEFF_FLAG_OFFSET				+		   0X2C//2A+2                                  	//128
#define	COEFF_ABS_LEVEL_GREATER2_FLAG_OFFSET		COEFF_ABS_LEVEL_GREATER1_FLAG_OFFSET	    +		   0X18       										//152
#define	 COEFF_INIT_BC_FLAG							COEFF_ABS_LEVEL_GREATER2_FLAG_OFFSET	    +		   0X6     														//158
#define	 COEFF_INTER_RDPCM_FLAG						COEFF_INIT_BC_FLAG	    					+		   0X3      														    //161
#define	 COEFF_INTER_RDPCM_DIR_FLAG					COEFF_INTER_RDPCM_FLAG	    				+		   0X2      												    //163



//读入SPS  PPS参数
extern Sps_Pps_struct sps_pps_struct;
void read_Sps_Pps_to_sps_struct(TComSPS *m_sps ,TComPPS  *m_pps , Sps_Pps_struct &sps_pps_struct);

//读入Slice_header参数
extern Slice_header_struct slice_header_struct;
void CABAC_codeSliceHeader(TComSlice* slice , Slice_header_struct &slice_header_struct);
void CABAC_xCodePredWeightTable(TComSlice* slice , Slice_header_struct &slice_header_struct);
bool CABAC_findMatchingLTRP(TComSlice* slice, uint32_t *ltrpsIndex, int ltrpPOC, bool usedFlag);


class ContextModel_hm
{
public:
	ContextModel_hm  ()                        { m_ucState = 0; m_binsCoded = 0; }
	~ContextModel_hm ()                        {}

	UChar getState  ()                { return ( m_ucState >> 1 ); }                    ///< get current state
	UChar getMps    ()                { return ( m_ucState  & 1 ); }                    ///< get curret MPS
	UChar get_m_ucState()                { return ( m_ucState  ); }
	void set_m_ucState(UChar i)                 {m_ucState = i;}
//	void  setStateAndMps( UChar ucState, UChar ucMPS) { m_ucState = (ucState << 1) + ucMPS; } ///< set state and MPS

//	void init_L_buffer ( int qp, int initValue );   ///< initialize state with initial probability

	void updateLPS ()
	{
		m_ucState = m_aucNextStateLPS[ m_ucState ];
	}

	void updateMPS ()
	{
		m_ucState = m_aucNextStateMPS[ m_ucState ];
	}

	int getEntropyBits(short val) { return m_entropyBits[m_ucState ^ val]; }

// #if FAST_BIT_EST
// 	Void update( Int binVal )
// 	{
// 		m_ucState = m_nextState[m_ucState][binVal];
// 	}
// 	static Void buildNextStateTable();
// 	static Int getEntropyBitsTrm( Int val ) { return m_entropyBits[126 ^ val]; }
// #endif
	void setBinsCoded(UINT val)   { m_binsCoded = val;  }
	UINT getBinsCoded()           { return m_binsCoded;   }

	UChar         m_ucState;                                                                  ///< internal state variable
private:
	static const  UChar m_aucNextStateMPS[ 128 ];
	static const  UChar m_aucNextStateLPS[ 128 ];
	static const int m_entropyBits[ 128 ];
// #if FAST_BIT_EST
// 	static UChar m_nextState[128][2];
// #endif
	UINT          m_binsCoded;

};


/*
class CABAC_RDO
{
private:
public:
	char			avail_left_up[IsIntra][depth][32];//分2 * tu_len + 1，-tu_len 到tu_len，其中tu_len点是用不到的，因为 左上角点用不到
	unsigned char	intra_luma_pred_mode_left_up[IsIntra][depth][32];//只有帧内块才会设置，存储的时经过帧内模式预测的值,没有就不更新，因为要用到的时候会判断是否avail且是否诶帧内块
	//只要是帧内块默认是dc_idx，也就是说pcm也要设置为dc_idx
	unsigned char	depth_left_up[IsIntra][depth][16];//解析处以及拆分前设置  cu块更新就可以了
	char			skip_flag_left_up[IsIntra][depth][16];//解析处设置	0，1都需要设置，否则不覆盖可能出现错误判断
	char			pred_mode_left_up[IsIntra][depth][16];//intra  ipcm inter empty都不一样skip为inter，part_size在tu前，所以能保证全部覆盖  每个块都必须有他的predmode，所以必须更新
	void init_L_buffer();
	uint32_t cu_to_tu_idx( uint32_t uiAbsPartIdx);
	uint32_t tu_to_cu_idx( uint32_t uiAbsPartIdx);
	template<typename T>
	T getLeftpart_in_tu(T* puhBaseLCU, UInt uiAbsPartIdx );
	template<typename T>
	T getUppart_in_tu(T* puhBaseLCU, UInt uiAbsPartIdx );
	char getLeftAvailSubParts( UInt uiAbsPartIdx );
	char getUpAvailSubParts( UInt uiAbsPartIdx );
	template<typename T>
	T getUppart_in_cu(  T* puhBaseLCU, UInt uiAbsPartIdx );
	template<typename T>
	T getLeftpart_in_cu( T* puhBaseLCU, UInt uiAbsPartIdx );
	char getLeftDepthSubParts( UInt uiAbsPartIdx );
	char getUpDepthSubParts( UInt uiAbsPartIdx );
	UInt getCtxSplitFlag( UInt uiAbsPartIdx, UInt depth );
	UInt getCtxSkipFlag( UInt uiAbsPartIdx );
	char getLeftSkip_flagSubParts( UInt uiAbsPartIdx );
	char getUpSkip_flagSubParts( UInt uiAbsPartIdx );
	Int getIntraDirLumaPredictor( UInt uiAbsPartIdx, Int* uiIntraDirPred,int partIdx );
	unsigned char getLeftIntra_pred_modeSubParts( UInt uiAbsPartIdx );
	unsigned char getUpIntra_pred_modeSubParts( UInt uiAbsPartIdx );
	char getLeftPred_modeSubParts( UInt uiAbsPartIdx );
	char getUpPred_modeSubParts( UInt uiAbsPartIdx );
	template<typename T>
	void setpart_in_tu( T uiParameter, T* puhBaseLCU, UInt uiAbsPartIdx, UInt depth );
	void setLumaIntraDirSubParts( UInt uiDir, UInt uiAbsPartIdx, UInt depth);
	template<typename T>
	void setpart_in_cu( T uiParameter, T* puhBaseLCU, UInt uiAbsPartIdx, UInt depth );
	void setPredModeSubParts( PredMode eMode, UInt uiAbsPartIdx, UInt depth );
	void setSkipFlagSubParts( bool skip, UInt absPartIdx, UInt depth );//cu level
	void setDepthSubParts( UInt depth, UInt uiAbsPartIdx );//cu level
	void setAvailSubParts_map( UInt availC, UInt uiAbsPartIdx,UInt depth  );//cu level
};

*/



#define  PU_TU_MAX_MODE 3

class CABAC_RDO
{
private:
public:

		int current_status[2][RDO_MAX_DEPTH] ;
		int next_status[2][RDO_MAX_DEPTH] ;

		int x_ctu_left_up_in_pic;
		int y_ctu_left_up_in_pic;
		int x_cu_left_up_in_pic[2][RDO_MAX_DEPTH];
		int y_cu_left_up_in_pic[2][RDO_MAX_DEPTH];

		bool cu_in_pic_flag[2][RDO_MAX_DEPTH];

		uint32_t inx_in_cu[2][RDO_MAX_DEPTH] ;
		uint32_t inx_in_tu[2][RDO_MAX_DEPTH] ;
		int cur_mode[2][RDO_MAX_DEPTH];

		bool cu_ready[2][RDO_MAX_DEPTH];
		uint32_t cu_cnt[2][RDO_MAX_DEPTH];
		bool ctu_start_flag;
		int inx_in_cu_transfer_table[RDO_MAX_DEPTH];
		int cu_pixel_wide[RDO_MAX_DEPTH];

		int pu_cnt_table[PU_TU_MAX_MODE];//0表示PU没有划分 1表示一个CU划分成两个PU 2表示1个CU划分成4个PU  TU都和PU一样尺寸
		int tu_cnt_table[PU_TU_MAX_MODE];//指的是TU在PU的基础上划分成多少个TU
		int depth_to_cur_mode[2][RDO_MAX_DEPTH] ;
		bool pu_best_mode_flag[2][RDO_MAX_DEPTH] ; //某层PU最佳模式确定的标记
		int intra_pu_depth_luma_bestDir[RDO_MAX_DEPTH] ;//intra某层PU 亮度最佳的预测方向值
		int intra_pu_depth_chroma_bestDir[RDO_MAX_DEPTH] ;//intra某层PU 色度最佳的预测方向值

		bool pu_mode_bit_ready[2][RDO_MAX_DEPTH] ; //表示某层PU各种模式需要多少bit都已经准备好

		bool quant_finish_flag[2][RDO_MAX_DEPTH] ;
		uint32_t end_depth[2] ;
		uint32_t max_pu_cnt[2][RDO_MAX_DEPTH];
		uint32_t max_tu_cnt[2][RDO_MAX_DEPTH];
		uint32_t pu_cnt[2][RDO_MAX_DEPTH];
		uint32_t tu_cnt[2][RDO_MAX_DEPTH];

		uint32_t est_bits_total[2][RDO_MAX_DEPTH] ;//total bit
		uint32_t cu_est_bits[2][RDO_MAX_DEPTH] ;//cu total bit
		uint32_t pu_est_bits[2][RDO_MAX_DEPTH] ;//pu total bit
		uint32_t tu_est_bits[2][RDO_MAX_DEPTH] ;//tu total bit

		uint32_t intra_pu_depth_luma_bestDir_temp[4];//
		uint32_t cu_best_mode[RDO_MAX_DEPTH] ;//0表示当前层用inter  1表示intra   2表示不用这层，用这层往下划分的
		uint32_t cu_best_mode_flag[RDO_MAX_DEPTH] ;

		uint32_t est_bit_cu_spit_flag[2][RDO_MAX_DEPTH] ;
		uint32_t mstate_cu_spit_flag[2][RDO_MAX_DEPTH] ;

		uint32_t est_bit_cu_transquant_bypass_flag[2][RDO_MAX_DEPTH][2] ;
		uint32_t mstate_cu_transquant_bypass_flag[2][RDO_MAX_DEPTH][2] ;

		uint32_t est_bit_cu_skip_flag[2][RDO_MAX_DEPTH][2];
		uint32_t mstate_cu_skip_flag[2][RDO_MAX_DEPTH][2] ;

		uint32_t est_bit_pred_mode_flag[2][RDO_MAX_DEPTH] ;
		uint32_t mstate_pred_mode_flag[2][RDO_MAX_DEPTH] ;

		uint32_t est_bit_pu_luma_dir[RDO_MAX_DEPTH][35] ;
		uint32_t mstate_prev_intra_luma_pred_flag[RDO_MAX_DEPTH];

		uint32_t est_bit_pu_chroma_dir[RDO_MAX_DEPTH] ;
		uint32_t mstate_pu_chroma_dir[RDO_MAX_DEPTH] ;

		uint32_t est_bit_pu_part_mode[2][RDO_MAX_DEPTH][2] ;
		uint32_t est_bit_pu_part_mode_sure[2][RDO_MAX_DEPTH] ;
		uint32_t mstate_pu_part_mode[2][RDO_MAX_DEPTH][2] ;

		uint32_t est_bit_merge_flag[RDO_MAX_DEPTH][2] ;
		uint32_t mstate_merge_flag[RDO_MAX_DEPTH][2] ;

		uint32_t est_bit_merge_idx[RDO_MAX_DEPTH][2] ;
		uint32_t mstate_merge_idx[RDO_MAX_DEPTH][2] ;

		uint32_t est_bit_mvp_l0_flag[RDO_MAX_DEPTH][2] ;
		uint32_t mstate_mvp_l0_flag[RDO_MAX_DEPTH][2];

		uint32_t est_bit_abs_mvd_greater0_flag[RDO_MAX_DEPTH][2] ;
		uint32_t mstate_abs_mvd_greater0_flag[RDO_MAX_DEPTH][2] ;

		uint32_t est_bit_abs_mvd_greater1_flag[RDO_MAX_DEPTH][2] ;
		uint32_t mstate_abs_mvd_greater1_flag[RDO_MAX_DEPTH][2] ;

		uint32_t est_bit_rqt_root_cbf[RDO_MAX_DEPTH][2];
		uint32_t mstate_rqt_root_cbf[RDO_MAX_DEPTH][2] ;

		uint32_t est_bit_cbf_luma[2][RDO_MAX_DEPTH][2] ;
		uint32_t mstate_cbf_luma[2][RDO_MAX_DEPTH][2] ;

		uint32_t est_bit_cbf_chroma[2][RDO_MAX_DEPTH][2][2] ;
		uint32_t mstate_cbf_chroma[2][RDO_MAX_DEPTH][2][2] ;

		UINT est_bit_sao_merge_left_flag[2];
		UINT est_bit_sao_merge_up_flag[2];//由于merge left和up使用同一个上下文，up标记都默认先经过一个输入值为0的状态跳转
		UINT est_bit_sao_type_idx_luma[2];
		UINT est_bit_sao_type_idx_chroma[2][2];//第一维表示Y分量type是否做SAO，type只有第一个bin是decision。

		UINT est_bit_tu[2][RDO_MAX_DEPTH];


		char			avail_left_up[2][RDO_MAX_DEPTH][32];//分2 * tu_len + 1，-tu_len 到tu_len，其中tu_len点是用不到的，因为 左上角点用不到 
		unsigned char	intra_luma_pred_mode_left_up[2][RDO_MAX_DEPTH][32];//只有帧内块才会设置，存储的时经过帧内模式预测的值,没有就不更新，因为要用到的时候会判断是否avail且是否诶帧内块
		//只要是帧内块默认是dc_idx，也就是说pcm也要设置为dc_idx
		unsigned char	depth_left_up[2][RDO_MAX_DEPTH][16];//解析处以及拆分前设置  cu块更新就可以了
		char			skip_flag_left_up[2][RDO_MAX_DEPTH][16];//解析处设置	0，1都需要设置，否则不覆盖可能出现错误判断
		char			pred_mode_left_up[2][RDO_MAX_DEPTH][16];//intra  ipcm inter empty都不一样skip为inter，part_size在tu前，所以能保证全部覆盖  每个块都必须有他的predmode，所以必须更新
	
	void init_L_buffer();//初始化L型buff
	void update_L_buffer_x();
	void update_L_buffer_y();
	uint32_t cu_to_tu_idx( uint32_t uiAbsPartIdx);
	uint32_t tu_to_cu_idx( uint32_t uiAbsPartIdx);
	template<typename T>
	T getLeftpart_in_tu(T* puhBaseLCU, UInt uiAbsPartIdx );
	template<typename T>
	T getUppart_in_tu(T* puhBaseLCU, UInt uiAbsPartIdx );
	char getLeftAvailSubParts( UInt uiAbsPartIdx, UInt depth , bool IsIntra  );
	char getUpAvailSubParts( UInt uiAbsPartIdx , UInt depth , bool IsIntra );
	template<typename T>
	T getUppart_in_cu(  T* puhBaseLCU, UInt uiAbsPartIdx );
	template<typename T>
	T getLeftpart_in_cu( T* puhBaseLCU, UInt uiAbsPartIdx );
	char getLeftDepthSubParts( UInt uiAbsPartIdx , UInt depth , bool IsIntra );
	char getUpDepthSubParts( UInt uiAbsPartIdx , UInt depth , bool IsIntra );
	UInt getCtxSplitFlag( UInt uiAbsPartIdx, UInt depth , bool IsIntra);
	UInt getCtxSkipFlag( UInt uiAbsPartIdx , UInt depth , bool IsIntra );
	char getLeftSkip_flagSubParts( UInt uiAbsPartIdx  , UInt depth , bool IsIntra);
	char getUpSkip_flagSubParts( UInt uiAbsPartIdx  , UInt depth , bool IsIntra);
	Int getIntraDirLumaPredictor( UInt uiAbsPartIdx, Int* uiIntraDirPred,int partIdx   , UInt depth , bool IsIntra);
	unsigned char getLeftIntra_pred_modeSubParts( UInt uiAbsPartIdx   , UInt depth , bool IsIntra);
	unsigned char getUpIntra_pred_modeSubParts( UInt uiAbsPartIdx  , UInt depth , bool IsIntra );
	char getLeftPred_modeSubParts( UInt uiAbsPartIdx   , UInt depth , bool IsIntra);
	char getUpPred_modeSubParts( UInt uiAbsPartIdx   , UInt depth , bool IsIntra);
	template<typename T>
	void setpart_in_tu( T uiParameter, T* puhBaseLCU, UInt uiAbsPartIdx, UInt depth );
	void setLumaIntraDirSubParts( UInt uiDir, UInt uiAbsPartIdx, UInt depth , bool IsIntra);
	template<typename T>
	void setpart_in_cu( T uiParameter, T* puhBaseLCU, UInt uiAbsPartIdx, UInt depth );
	void setPredModeSubParts( PredMode eMode, UInt uiAbsPartIdx, UInt depth , bool IsIntra );
	void setSkipFlagSubParts( bool skip, UInt absPartIdx, UInt depth , bool IsIntra );//cu level
	void setDepthSubParts( UInt depth, UInt uiAbsPartIdx  , bool IsIntra);//cu level
	void setAvailSubParts_map( UInt availC, UInt uiAbsPartIdx,UInt depth  , bool IsIntra );//cu level

	//CABAC接口函数
	UINT Est_bit_cu(UINT  depth , bool  IsIntra , bool  cu_skip_flag);//CU级别某层深度下bit估计
	UINT Est_bit_pu_luma_dir(UINT  depth , int  dir);//PU级别INTRA 35种方向luma bit估计
	UINT Est_bit_pu_merge(UINT  depth , UINT  merge_idx);//PU级别INTER  merge模式bit估计
	UINT Est_bit_pu_fme(UINT  depth , UINT  mvd , UINT  mvp_l0_flag);//PU级别INTER  FME模式bit估计
	UINT Est_bit_sao(SaoParam_CABAC  saoParam , int cIdx , int Y_type);//SAO级别模式bit估计
	UINT Est_bit_tu(UINT  depth , bool  IsIntra);//TU级别bit估计



	ContextModel_hm mModels[2][RDO_MAX_DEPTH][CONTEXT_NUM];
	
	void init_data();//初始化变量
	void resetEntropy_CABAC();//初始化8套上下文
	void cabac_rdo_status_test(uint32_t depth,bool IsIntra , bool end_of_cabac_rdo_status );//状态机用来和x265原始数据对比使用
	void cabac_rdo_status(uint32_t depth,bool IsIntra , bool end_of_cabac_rdo_status );//状态机  联调时使用

	//tu 级别相关
	int tu_luma_4x4_idx;
	int TU_type;//残差块的类型 0 表示是luma
	int TU_size;//残差块的像素宽
	int scanIdx;
	int cbf;
	int cbf_luma;
	int cbf_chroma_u;
	int cbf_chroma_v;
	int16_t *TU_Resi_luma;
	int16_t *TU_Resi_chroma_u;
	int16_t *TU_Resi_chroma_v;
	//int TU_type;//0表示luma 1表示chromaU  2表示chromaV
	bool IsIntra_tu;
	uint32_t depth_tu;

	uint32_t est_bit_last_sig_coeff_x_prefix[2][RDO_MAX_DEPTH];
	uint32_t est_bit_last_sig_coeff_x_suffix[2][RDO_MAX_DEPTH];

	uint32_t est_bit_last_sig_coeff_y_prefix[2][RDO_MAX_DEPTH];
	uint32_t est_bit_last_sig_coeff_y_suffix[2][RDO_MAX_DEPTH];

	uint32_t est_bit_sig_coeff_flag[2][RDO_MAX_DEPTH];

	uint32_t est_bit_coeff_sign_flag[2][RDO_MAX_DEPTH];
	
	uint32_t est_bit_coeff_abs_level_greater1_flag[2][RDO_MAX_DEPTH];

	uint32_t est_bit_coeff_abs_level_greater2_flag[2][RDO_MAX_DEPTH];

	uint32_t est_bit_coeff_abs_level_remaining[2][RDO_MAX_DEPTH];

	uint32_t est_bit_coded_sub_block_flag[2][RDO_MAX_DEPTH];

	void cabac_est(int TU_size,int16_t *TU_Resi , int TU_type);

	void	est_bins(int bin, int len , int valbin_ctxInc_r[4] , int ctx_offset);
	void	est_bypass( int len , int mode);
	void hevc_cabac_decision_enc_est(Enc_input_est   enc_input_est,  Enc_output_est   &enc_output_est , int valbin_ctxInc_r[4] , int ctx_offset );
	void hevc_enc_cabac_decision_unit_est(Decision_unit_input_est  decision_unit_input_est , Decision_unit_output_est   &decision_unit_output_est);
	void est_bin1(int  last_sig_coeff_x_prefix_r , int last_sig_coeff_y_prefix_r  , int ctx_offset_x , int ctx_offset_y);
	void est_bits_init();
#if RK_CABAC_TEST
	TComDataCU *cu_rdo;
#endif
	
};

extern void cabac_est_test(int size,int16_t *oriResi);

#endif