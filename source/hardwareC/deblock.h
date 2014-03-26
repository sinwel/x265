

#ifndef	_DEB_SAO_HRD_H
#define	_DEB_SAO_HRD_H

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "ram.h"


#define		LOOPFILETER_HW_MODULE_DEBUG


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

//for debug mocro
#define   EDGE_VER    0
#define   EDGE_HOR    1

#define   MAX_QP      51

#define   CMP(a, b) ((a) > (b) ? 1 : ((a) == (b) ? 0 : -1))


class LoopFilter
{
public:

	//FIFO	*cmdin;
	
	//recon ram
	RAM 	ctu_y_in_buff[CTU_IN_NUM];		//y的input的ctu buff
	RAM 	ctu_uv_in_buff[CTU_IN_NUM];		//uv的input ctu buff
	reg		ctu_buff_status[CTU_IN_NUM];	//[1:0]	//Buffer_access_ctrl模块输出信号
	reg		ctu_buf_cnt_recon;				//[1:0]	recon的ctu buff的计数
	wire	recon_buf_end;					//recon写ctu buff完成的信号

	//deblock ram
	RAM		deblock_y_row_buff;				//delbock save y row pixel, 192X1024
	RAM		deblock_uv_row_buff;			//delbock save uv row pixel
	RAM		deblock_qp_row_buff;			//delbock qp row buff
	RAM		deb_sao_qt_pcmflag_row_buff;	//delbock pcm row buff
	RAM		sao_data_ctu_right_col_luma[4];	//luma deblock save a col pixel for sao
	RAM		sao_data_ctu_right_col_chr[4];	//chroma deblock save a col pixel for sao
	reg		ctu_buf_cnt_deblock;			//[1:0] deblock的ctu buff的计数
	reg		ctu_buf_cnt_deblock_pre;		//[1:0] deblock的ctu buff的上一个计数
	reg		ctu_buf_cnt_deblock_pre2;		//[1:0] deblock的ctu buff的上上一个计数
	reg		deblock_bs[4][64+2];			//delbock bs
	reg		deblock_bs_y_right_col[8];		//deblock bs luma right col
	reg		deblock_bs_chr_right[4];		//delbock bs chroma right col for most right 4 cols of curr ctu, do it at next ctu
	reg		deblock_bs_cnt;					//deblock ver bs counter
	reg		deblock_qp[2][9*9];				//delbock qp
	reg		deblock_qt_pcmflag[4][9*9];		//delbock qt pcm flag
	reg		deblock_y_tmp_buff[16*8];		//[11:0] every pixel use, 16x8 pixel y buff
	reg		deblock_uv_tmp_buff[16*4];		//[11:0] every pixel use, 16x4(8x8) pixel y buff
	reg		deblock_bs_qp_pcm_buff_idx;		//0 or 1
	reg    	deblocking_buf_rdy[4];			//deblock ready signal
	reg		cmd_curr_state;
	reg		cmd_next_state;

	//output ram
	RAM 	ctu_y_out_buff[CTU_OUT_NUM];	//ctu y output buff
	RAM 	ctu_uv_out_buff[CTU_OUT_NUM];	//ctu uv output buff
	reg		ctu_out_buff_cnt;
	reg		ctu_num_in_outbuff;

	//global reg
	reg		sw_out_cbcr_swap;	//1'b0:uvuv,  1'b1:vuvu
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

	//define para for debug
	//debug mem
	char	ctu_bs_fifo_out[4][64];
	char	*p_iqit_bs_fifo_out;
	//UInt64 	*p_mvd_bs_fifo_out;
	//UInt64	mvd_bs_fifo_out[4];
	char	*outputpicaddr;
	int		busfd_wdata_cnt;
	int		deb_ctu_cnt;
	
	//define for creat pattern
	int		deblock_dectrl_in_cmd_num;
	int		deblock_bs_in_cmd_num;
	int		deblock_cmd_line_num;
	int		deb_sao_ctu_cnt;
	int		busfd_cmd_cnt;

	
public:
	void  hevc_filterd_init();
	void  hevc_filterd_frame_init();
	int   hevc_filterd_parse_cmd();
	void  hevc_deblock_calc_ctu();
	void  hevc_deblock_read16x8p_luma(int x, int y);
	void  hevc_deblock_read16x8p_chr(int x, int y);
	void  hevc_deblock_rw16x8p_luma(int x, int y, int rd_wr_en);
	void  hevc_deblock_rw8x8p_vert_chr(int x, int y, int rd_wr_en);
	void  hevc_deblock_rw16x4p_horz_chr(int x, int y, int rd_wr_en);
	void  hevc_deblock_calc16x8p_luma(int x0, int y0);
	void  hevc_deblock_calc16x8p_chr(int x0, int y0, int dir);

	void  hevc_deblock_save_pixel_and_flag();
	void  hevc_deblock_get_edge_pixel();
	void  hevc_filter_reverse_flag();

	void  hevc_read_ctu_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
									int read_line_nums, int read_start_pos, int read_line_width_pixel, int bitdepth);
	void  hevc_write_ctu_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
									int write_line_nums, int write_start_pos, int write_line_width_pixel, int bitdepth);
	void  hevc_read_write_ctu_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
									int read_line_nums, int read_start_pos, int read_line_width_pixel, int bitdepth, int rd_wr_en);
	void  hevc_read_write_ctu_uv_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
									int read_line_nums, int read_start_pos, int read_line_width_pixel, int bitdepth, int regstride, int rd_wr_en);
	
	void  hevc_pixel_unpack(unsigned char *src, reg *dst, int offset, int pixelnum, int bit_depth);
	void  hevc_pixel_pack(unsigned char *dst, reg *src, int offset, int pixelnum, int bit_depth);

	int 	chroma_tc(int iQpY, int c_idx, int tc);
	int 	av_clip_c(int a, int amin, int amax);
	int 	av_clip_pixel(int a, int cidx);
	int		abs(int a);
	
	void hevc_deblock_calc_unit_luma(reg *_pix, int xstride, int ystride,
									int no_p, int no_q, int _beta, int _tc, int x0, int y0, int bit_depth);
	void  hevc_deblock_calc_unit_chroma(reg *_pix, int xstride, int ystride, int datalen,
											int no_p, int no_q, int _tc_cb, int _tc_cr, int bit_depth);
	void  hevc_deblock_hrd();
	void  hevc_sao_hrd();
	void  hevc_filterd_hrd();
	void  hevc_filterd_setend();
	int   hevc_filterd_get_bs();
	
public:
	LoopFilter();
    virtual ~LoopFilter();

};

#endif


