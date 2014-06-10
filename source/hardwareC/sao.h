

#ifndef	_DEB_SAO_HRD_H
#define	_DEB_SAO_HRD_H

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "ram.h"


#define		LOOPFILETER_HW_MODULE_DEBUG

//#define		LOOPFILETER_HW_CREAT_PATTERN_DEB
//#define		LOOPFILETER_HW_CREAT_PATTERN_SAO
//#define		LOOPFILETER_HW_CREAT_PATTERN_BUSFD

//#define		LOOPFILETER_HW_CREAT_LOG
//#define		LOOPFILETER_HW_COL_LOG
//#define		LOOPFILETER_HW_TILE_INFO_LOG

#define		PATTERN_START_FRAME_NUM		0

#define		PATTERN_FRAME_NUM		16


#define 	reg						int
#define 	wire					int

#define 	CTU_IN_NUM				4
#define 	CTU_OUT_NUM				2
#define 	CTU_MAX_SIZE			64

#define 	HEVC_MAX_WIDTH			4096
#define 	HEVC_MAX_HEIGHT			2304
#define		HEVC_MAX_BIT_DEP		10
#define   	DEFAULT_INTRA_TC_OFFSET 2

#define		LUMA					0
#define		CHROMA					1
#define		CB						1
#define		CR						2

#define		CTU_IN_MEM_PIXEL_WID	16
#define		CTU_OUT_BIT_WID			128

#define		READ					0
#define		WRITE					1
#define		VERT					0
#define		HORZ					1

#define		CTUDOWN4LINE		8
#define		RIGHTBORD			4
#define		TOPLEFT				3
#define		LEFT				2
#define		RIGHT				1
#define		DOWNRIGHT			0

//sao fsm
#define		ST_IDLE				0
#define		ST_MID_LOWER		1
#define		ST_MID_UPPER		2
#define		ST_HEAD_LOWER		3
#define		ST_HEAD_UPPER		4
#define		ST_HEAD_PARA_FETCH	5
#define		ST_END_LOWER		6
#define		ST_END_UPPER		7
#define		ST_CTU_END			8

//#define    LOOPFILT_SLICE_CMD_ID             0xe
//#define    DBSAO_CU_CMD_ID                   0xf
#define    DBSAO_SAO_CMD_ID     0x80

//for debug mocro
#define   EDGE_VER    0
#define   EDGE_HOR    1

#define   MAX_QP      51

#define   CMP(a, b) ((a) > (b) ? 1 : ((a) == (b) ? 0 : -1))


typedef struct SAOParams {
    unsigned char type_idx[3]; ///< sao_type_idx

    int offset_abs[3][4]; ///< sao_offset_abs
    int offset_sign[3][4]; ///< sao_offset_sign

    int band_position[3]; ///< sao_band_position

    int eo_class[3]; ///< sao_eo_class

    // Inferred parameters
    int offset_val[3][5]; ///<SaoOffsetVal
} SAOParams;

typedef struct
{
    unsigned long    reserve                :1;
    unsigned long    type_id                :2;
      signed long    offset0                :6;
      signed long    offset1                :6;
      signed long    offset2                :6;
      signed long    offset3                :6;
    unsigned long    subtype_id             :5;
}SAO_PARAMETER;

enum SAOTypes {
    SAO_NOT_APPLIED = 0,
    SAO_BAND,
    SAO_EDGE
};

enum SAOEOClasss {
    SAO_EO_HORIZ = 0,
    SAO_EO_VERT,
    SAO_EO_135D,
    SAO_EO_45D
};


class SaoModule
{
public:

	
	//recon ram
	RAM 	ctu_y_in_buff[CTU_IN_NUM];		//y的input的ctu buff
	RAM 	ctu_uv_in_buff[CTU_IN_NUM];		//uv的input ctu buff
	reg		ctu_buff_status[CTU_IN_NUM];	//[1:0]	//Buffer_access_ctrl模块输出信号
	reg		ctu_buf_cnt_recon;				//[1:0]	recon的ctu buff的计数
	wire	recon_buf_end;					//recon写ctu buff完成的信号

	
	//sao ram
	RAM 	sao_y_row_buff;			//sao save y row pixel
	RAM 	sao_uv_row_buff;		//sao save uv row pixel
	RAM 	saoparamrow;			//sao row parameter buff
	RAM		sao_avail_l_flag;		//sao save avail flag
	RAM		sao_avail_t_flag;		//sao save avail flag

	reg		deblock_qt_pcmflag[4][9*9];		//delbock qt pcm flag
	reg    	deblocking_buf_rdy[4];			//deblock ready signal

	RAM		sao_data_ctu_right_col_luma[4];	//luma deblock save a col pixel for sao
	RAM		sao_data_ctu_right_col_chr[4];	//chroma deblock save a col pixel for sao
	reg 	sao_upper_bl_point_luma;
	reg 	sao_upper_bl_point_chr[2];
	reg 	sao_upper_tr_point_luma;
	reg 	sao_upper_tr_point_chr[2];
	reg 	sao_lower_br_point_luma;
	reg 	sao_lower_br_point_chr[2];
	reg 	sao_lower_tr_point_luma;
	reg 	sao_lower_tr_point_chr[2];
	
	reg		ctu_buf_cnt_sao;		//[1:0] sao的ctu buff的计数
	reg		ctu_buf_cnt_sao_pre;	//[1:0] sao的ctu buff的前一个计数
	reg		sao_out_tilel_flag[2];	//sao ctu output is tilel flag
	reg    	sao_avail_flag[4];		//sao save avail flag
	reg		sao_para_luma[4];
	reg		sao_para_U[4];
	reg		sao_para_V[4];
	reg		sao_para_luma_up;
	reg		sao_para_U_up;
	reg		sao_para_V_up;

	reg 	sao_data_in_luma[3][18];
	reg		sao_luma_idx_up;
	reg		sao_luma_idx_cur;
	reg		sao_luma_idx_down;
	reg 	sao_data_in_chr[3][20];
	reg		sao_chr_idx_up;
	reg		sao_chr_idx_cur;
	reg		sao_chr_idx_down;
	reg 	sao_data_out_luma[16];
	reg 	sao_data_out_chr[16];
	reg 	sao_offset_val[5];
	reg 	sao_band_table[32];
	reg 	sao_band_position;
	
	wire	sao_buf_end;			//sao写ctu buff完成的信号

	//output ram
	RAM 	ctu_y_out_buff[CTU_OUT_NUM];	//ctu y output buff
	RAM 	ctu_uv_out_buff[CTU_OUT_NUM];	//ctu uv output buff
	reg		ctu_out_buff_cnt;
	reg		ctu_num_in_outbuff;

	//global reg
	/*reg		sps_pcm_loop_filter_disabled_flag;
	reg		pps_loop_filter_across_tiles_enabled_flag;
	reg		slice_loop_filter_across_slices_enabled_flag;
	reg		slice_deblocking_filter_disabled_flag;
	reg		sps_sample_adaptive_offset_enabled_flag;
	reg		slice_sao_luma_flag;
	reg		slice_sao_chroma_flag;*/
	reg		loop_filt_slice_cmd_cur;
	reg		loop_filt_slice_cmd[4];
	
	//pic cmd reg
	reg		bit_depthY_pic_cmd;
	reg		bit_depthC_pic_cmd;
	reg		pps_cb_qp_offset_pic_cmd;
	reg		pps_cr_qp_offset_pic_cmd;
	reg		ctb_size_filter;

	//slice cmd reg
	/*reg		slice_cb_qp_offset;
	reg		slice_cr_qp_offset;
	reg		beta_offset_div2;
	reg		tc_offset_div2;*/

	//ctu cmd reg
	reg		ctu_start_cmd[4];

	//sao cmd data
	reg		sao_y_para_cmddata;
	reg		sao_cb_para_cmddata;
	reg		sao_cr_para_cmddata;

	//FSM	param
	//deblock
	//sao
	reg		sao_curr_state;
	reg		sao_next_state;
	char	*outputpicaddr;

	
public:
	
	void  hevc_pixel_unpack(unsigned char *src, reg *dst, int offset, int pixelnum, int bit_depth);
	void  hevc_pixel_pack(unsigned char *dst, reg *src, int offset, int pixelnum, int bit_depth);

	int 	chroma_tc(int iQpY, int c_idx, int tc);
	int 	av_clip_c(int a, int amin, int amax);
	int 	av_clip_pixel(int a, int cidx);
	int		abs(int a);

	void  hevc_sao_hrd();

	//SAO function
	void  hevc_sao_ctrl();
    void  hevc_sao_para_fetch();
	void  hevc_sao_save_mid_lower_edge();
	void  hevc_sao_save_head_lower_edge();
	void  hevc_sao_rev_flag();

    void  hevc_sao_calc_luma_upper(int width, reg *data_out, int x, int y);
    void  hevc_sao_calc_luma_lower(int width, reg *data_out, int x, int y);
    void  hevc_sao_calc_chr_upper(int width, reg *data_out, int x, int y);
    void  hevc_sao_calc_chr_lower(int width, reg *data_out, int x, int y);
	void  hevc_sao_rd_mid_lower_luma(int x, int y, int datalen, int savedatapos, int right_rd_en);
	void  hevc_sao_rd_mid_upper_luma(int x, int y, int datalen, int savedatapos, int right_rd_en);
	void  hevc_sao_rd_mid_lower_chr(int x, int y, int datalen, int savedatapos, int right_rd_en);
	void  hevc_sao_rd_mid_upper_chr(int x, int y, int datalen, int savedatapos, int right_rd_en);
	void  hevc_sao_read_right_pixel_luma(int x, int y, int ramstart, int idx, int right_rd_en);
	void  hevc_sao_read_right_pixel_chr(int x, int y, int ramstart, int idx, int right_rd_en);
	void  hevc_sao_wr_lower_luma(int x, int y, int datalen);
	void  hevc_sao_wr_upper_luma(int x, int y, int datalen);
	void  hevc_sao_wr_lower_chr(int x, int y, int datalen);
	void  hevc_sao_wr_upper_chr(int x, int y, int datalen);
	void  hevc_ctu_out_pack(unsigned char *dst, reg *src, int pixelnum, int bit_depth);
	void  hevc_ctu_out_unpack(unsigned char *src, unsigned short *dst, int pixelnum, int startbit, int bit_depth);
	
    void  hevc_sao_calc_mid_upper();
    void  hevc_sao_calc_mid_lower();
    void  hevc_sao_output();
	void  hevc_sao_output_bitdep8_Y(int ctu_y, int ctu_x, int tilel, int tiler, int picb, int picr);
	void  hevc_sao_output_bitdep8_C(int ctu_y, int ctu_x, int tilel, int tiler, int picb, int picr);
    void  hevc_write_output_ddr(unsigned char *data_tmp, int bytelen, int x, int y, int bytepos, int yuvmode, int bit_depth);

	void  hevc_read_ctu_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
									int read_line_nums, int read_start_pos, int read_line_width_pixel, int bitdepth);
	void  hevc_write_ctu_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
									int write_line_nums, int write_start_pos, int write_line_width_pixel, int bitdepth);
	
public:
	SaoModule();
    virtual ~SaoModule();

};

#endif


