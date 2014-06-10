

#include "deblock.h"


extern unsigned char Bslastresult[256];
extern unsigned char Bslastresultfifo[4][64];
extern unsigned int ctucnt;

#define 	input					int
#define 	output					int

// line zero
#define P3 pix[-4*xstride]//pix[-4*xstride]
#define P2 pix[-3*xstride]//pix[-3*xstride]
#define P1 pix[-2*xstride]//pix[-2*xstride]
#define P0 pix[-xstride]//pix[-xstride]
#define Q0 pix[0]
#define Q1 pix[xstride]
#define Q2 pix[2*xstride]
#define Q3 pix[3*xstride]

// line three. used only for deblocking decision
#define TP3 pix[-4*xstride+3*ystride]//pix[-4*xstride+3*ystride]
#define TP2 pix[-3*xstride+3*ystride]//pix[-3*xstride+3*ystride]
#define TP1 pix[-2*xstride+3*ystride]//pix[-2*xstride+3*ystride]
#define TP0 pix[-xstride+3*ystride]//pix[-xstride+3*ystride]
#define TQ0 pix[3*ystride]
#define TQ1 pix[xstride+3*ystride]
#define TQ2 pix[2*xstride+3*ystride]
#define TQ3 pix[3*xstride+3*ystride]


const unsigned char tctable[54] =
{
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, // QP  0...18
	 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, // QP 19...37
	 5, 5, 6, 6, 7, 8, 9,10,11,13,14,16,18,20,22,24 		  // QP 38...53
};

const unsigned char betatable[52] =
{
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 7, 8, // QP 0...18
	 9,10,11,12,13,14,15,16,17,18,20,22,24,26,28,30,32,34,36, // QP 19...37
	38,40,42,44,46,48,50,52,54,56,58,60,62,64				  // QP 38...51
};

const unsigned char aucChromaScale[58]=
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,
    17,18,19,20,21,22,23,24,25,26,27,28,29,29,30,31,32,
    33,33,34,34,35,35,36,36,37,37,38,39,40,41,42,43,44,
    45,46,47,48,49,50,51
};


LoopFilter::LoopFilter()
{
	//malloc frame buffer
	outputpicaddr = NULL;
	outputpicaddr = (char *)malloc(HEVC_MAX_WIDTH*HEVC_MAX_HEIGHT*3);

	deblock_dectrl_in_cmd_num = 0;
	deblock_bs_in_cmd_num = 0;
	deblock_cmd_line_num = 1;
	deb_sao_ctu_cnt = 0;
	deb_ctu_cnt = 0;
	
    hevc_filterd_init();

}

LoopFilter::~LoopFilter()
{
	//free frame buffer
	if(outputpicaddr)
		free(outputpicaddr);

}

void  LoopFilter::hevc_filterd_init()
{
	int 	i;

	//recon ram
	for(i=0;i<CTU_IN_NUM;i++)	//CTU_IN_NUM=4
	{
		ctu_y_in_buff[i].init(192, 512/*4*68*/);		//192 bitwid	depth=4*(64+4); y的input的ctu buff
		ctu_uv_in_buff[i].init(192, 512/*4*34*/);		//192 bitwid	depth=4*(32+2); uv的input ctu buff
	}

	//deblock ram
	for(i=0;i<4;i++)
		sao_data_ctu_right_col_luma[i].init(32, 256);	//12bit bitwid, depth 68, luma
	for(i=0;i<4;i++)
		sao_data_ctu_right_col_chr[i].init(64, 256);	//24bit bitwid, depth 34, chr
	
	//output ram
	for(i=0;i<CTU_OUT_NUM;i++)
	{
		ctu_y_out_buff[i].init(128, 1024/*(4+64)+12*(4+64)*/);	//128 bitwid,	depth=(4+64)+12*(4+64)=884, ctu y output buff
		ctu_uv_out_buff[i].init(128, 512/* (2+32)+12*(2+32)*/);	//128 bitwid,	depth=(2+32)+12*(2+32)=442, ctu uv output buff
	}

	return ;
}

//the function must init every frame decoding.
void  LoopFilter::hevc_filterd_frame_init()
{
	//reg initial
	ctu_buf_cnt_deblock_pre = 0;
	ctu_buf_cnt_deblock_pre2 = 0;
	ctu_buf_cnt_deblock = 0;
	deblock_bs_qp_pcm_buff_idx = 0;
	ctu_out_buff_cnt = 0;
	ctu_num_in_outbuff = 0;
	memset(deblocking_buf_rdy, 0, sizeof(deblocking_buf_rdy));
	
    return ;
}

int  LoopFilter::hevc_filterd_get_bs()
{
	return 0;
}

void  LoopFilter::hevc_deblock_calc_ctu()
{
	//to do deblocking calc
	int x, y;
	input pic_height_in_luma_samples;
	input ctu_x=0, ctu_y=0, picr=0, picb=0;

	//need to calc case for bottom of picture
	input height = picb ? (pic_height_in_luma_samples-ctu_y*ctb_size_filter+4) : (ctu_y ? (ctb_size_filter+4):ctb_size_filter);
	input heightC = picb ? (pic_height_in_luma_samples-ctu_y*ctb_size_filter)/2 : ctb_size_filter/2;;


	//luma vertical filtering and horizontal filtering
	for (x = 0; x < (picr ? ctb_size_filter:ctb_size_filter-4); (x = x ? (x+16):(x+12)))
	{
		for (y = 0; y < height; (y = (y||ctu_y) ? (y+8):(y+4)))
		{
			//read 16x8 pixel
			hevc_deblock_rw16x8p_luma(x, y, READ);

			//calc 4 times vert edge and 4 times hori edge
			hevc_deblock_calc16x8p_luma(x, y);

			//write 16x8 pixel
			hevc_deblock_rw16x8p_luma(x, y, WRITE);
		}
	}

	//chroma vertical filtering and horizontal filtering
	for (x = 0; x < ctb_size_filter/2; x+=8)
	{
		for (y = 0; y < heightC; y+=8)
		{
			//vertical chroma deblocking
			//read 8x8 pixel
			hevc_deblock_rw8x8p_vert_chr(x, y, READ);

			//calc 2times vert edge
			hevc_deblock_calc16x8p_chr(x, y, VERT);

			//write 8x8 pixel
			hevc_deblock_rw8x8p_vert_chr(x, y, WRITE);

			//horizontal chroma deblocking
			//read 16x4 pixel
			hevc_deblock_rw16x4p_horz_chr(x, y, READ);

			//calc 2 times hori edge
			hevc_deblock_calc16x8p_chr(x, y, HORZ);

			//write 16x4 pixel
			hevc_deblock_rw16x4p_horz_chr(x, y, WRITE);
		}
	}

	for (y = 0; y < heightC; y+=8)
	{
		x = ctb_size_filter/2 - 8;
		if(picr)	//at right bord of picture
		{
			//read 8x4 pixel
			hevc_deblock_rw16x4p_horz_chr(x+4, y, READ);

			//calc 1 times hori edge
			hevc_deblock_calc16x8p_chr(x+4, y, HORZ);

			//write 8x4 pixel
			hevc_deblock_rw16x4p_horz_chr(x+4, y, WRITE);
		}
	}

	return ;
}

void  LoopFilter::hevc_deblock_rw16x8p_luma(int x, int y, int rd_wr_en)
{
	//ctu's pos in picture
	input ctu_x, ctu_y, picr, picb;
	input ctbw = (CTU_MAX_SIZE>>4);
	input ctbsize = (ctb_size_filter>>4);
	input picw = (HEVC_MAX_WIDTH/16);
	input ctu_in_pic = (((ctu_x==0)<<1) + (ctu_y==0));
	input height = (ctu_y ? (ctb_size_filter+4):ctb_size_filter);
	input ctu_down_ylines = (y == (height - 4)) ? 4:8;
	input block_in_ctu = (picr && ((x+4)>=ctb_size_filter)) ? RIGHTBORD : (((x==0)<<1) + (y==0));
	input in_start_addr = ctbw*4;

	switch(ctu_in_pic)
	{
		case TOPLEFT:
			switch(block_in_ctu)
			{
				case TOPLEFT:	//12x4
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[16*4+4], in_start_addr, ctbw,
									4, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;

				case LEFT:	//12x8
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], in_start_addr+ctbw*y, ctbw,
									ctu_down_ylines, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;

				case RIGHT: //16x4
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[16*4], in_start_addr+(x+4)/16-1, ctbw,
									4, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[16*4+4], in_start_addr+(x+4)/16, ctbw,
									4, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;

				case DOWNRIGHT: //16x8
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[0], in_start_addr+ctbw*y+(x+4)/16-1, ctbw,
									ctu_down_ylines, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], in_start_addr+ctbw*y+(x+4)/16, ctbw,
									ctu_down_ylines, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;
				case RIGHTBORD:
					if(y)
					{
						hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[0], in_start_addr+ctbw*y+((x+4)>>4)-1, ctbw,
										ctu_down_ylines, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					}
					break;
			}
			break;


		case RIGHT:
			switch(block_in_ctu)
			{
				case TOPLEFT:
					{
						hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock_pre], &deblock_y_tmp_buff[16*4], in_start_addr+ctbsize-1, ctbw,
									4, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
						hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[16*4+4], in_start_addr, ctbw,
									4, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					}
					break;

				case LEFT:
					{
						hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock_pre], &deblock_y_tmp_buff[0], in_start_addr+ctbw*y+ctbsize-1, ctbw,
										ctu_down_ylines, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
						hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], in_start_addr+ctbw*y, ctbw,
										ctu_down_ylines, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					}
					break;

				case RIGHT:
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[16*4], in_start_addr+((x+4)>>4)-1, ctbw,
									4, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[16*4+4], in_start_addr+((x+4)>>4), ctbw,
									4, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;

				case DOWNRIGHT:
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[0], in_start_addr+ctbw*y+((x+4)>>4)-1, ctbw,
									ctu_down_ylines, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], in_start_addr+ctbw*y+((x+4)>>4), ctbw,
									ctu_down_ylines, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;

				case RIGHTBORD:
					if(y)
					{
						hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[0], in_start_addr+ctbw*y+((x+4)>>4)-1, ctbw,
										ctu_down_ylines, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					}
					break;
			}
			break;


		case LEFT:
			switch(block_in_ctu)
			{
				case TOPLEFT:
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], 0, ctbw,
									8, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;

				case LEFT:
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], ctbw*y, ctbw,
							ctu_down_ylines, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;

				case RIGHT:
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[0], (x+4)/16-1, ctbw,
									8, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], (x+4)/16, ctbw,
									8, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;

				case DOWNRIGHT:
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[0], ctbw*y+(x+4)/16-1, ctbw,
									ctu_down_ylines, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], ctbw*y+(x+4)/16, ctbw,
									ctu_down_ylines, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;
				case RIGHTBORD:
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[0], ctbw*y+((x+4)>>4)-1, ctbw,
									ctu_down_ylines, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					break;
			}
			break;


		case DOWNRIGHT:
			switch(block_in_ctu)
			{
				case TOPLEFT:
					{
						hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock_pre], &deblock_y_tmp_buff[0], ctbsize-1, ctbw,
									8, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
						hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], 0, ctbw,
									8, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					}
					break;

				case LEFT:
					{
						hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock_pre], &deblock_y_tmp_buff[0], ctbw*y+ctbsize-1, ctbw,
										ctu_down_ylines, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
						hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], ctbw*y, ctbw,
										ctu_down_ylines, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					}
					break;

				case RIGHT:
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[0], (x+4)/16-1, ctbw,
									8, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], (x+4)/16, ctbw,
									8, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;

				case DOWNRIGHT:
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[0], ctbw*y+(x+4)/16-1, ctbw,
									ctu_down_ylines, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[4], ctbw*y+(x+4)/16, ctbw,
									ctu_down_ylines, 0, 12, bit_depthY_pic_cmd, rd_wr_en);
					break;

				case RIGHTBORD:	//start from first row in ctu buff
					hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], &deblock_y_tmp_buff[0], ctbw*y+((x+4)>>4)-1, ctbw,
									ctu_down_ylines, 12, 4, bit_depthY_pic_cmd, rd_wr_en);
					break;
			}
			break;
	}

	return ;
}

void  LoopFilter::hevc_deblock_rw8x8p_vert_chr(int x, int y, int rd_wr_en)
{
	//ctu's pos in picture
	input ctu_x, ctu_y, picr, picb;
	input ctbsize  = (ctb_size_filter>>4);
	input ctbw  = (CTU_MAX_SIZE>>4);
	input ctu_in_pic = (((ctu_x==0)<<1) + (ctu_y==0));
	input block_in_ctu = ((x==0)<<1) + (y==0);
	input in_start_addr = ctbw*2;

	switch(ctu_in_pic)
	{
		case TOPLEFT:
		    switch(block_in_ctu)
        	{
        		case TOPLEFT:
				case LEFT:
        		    //at left of picture, so do nothing
        			break;

        		case RIGHT:
				case DOWNRIGHT:
        			hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], in_start_addr+ctbw*y+x/8-1, ctbw,
									8, 12, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[4], in_start_addr+ctbw*y+x/8, ctbw,
									8, 0, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
        			break;
        	}
			break;


		case RIGHT:
			switch(block_in_ctu)
        	{
        		case TOPLEFT:
				case LEFT:
					{
						hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock_pre], &deblock_uv_tmp_buff[0], in_start_addr+ctbw*y+ctbsize-1, ctbw,
										8, 12, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
						hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[4], in_start_addr+ctbw*y, ctbw,
										8, 0, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
					}
        			break;

        		case RIGHT:
				case DOWNRIGHT:
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], in_start_addr+ctbw*y+x/8-1, ctbw,
									8, 12, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[4], in_start_addr+ctbw*y+x/8, ctbw,
									8, 0, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
        			break;
        	}
			break;


		case DOWNRIGHT:
			switch(block_in_ctu)
        	{
        		case TOPLEFT:
				case LEFT:
					{
						hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock_pre], &deblock_uv_tmp_buff[0], in_start_addr+ctbw*y+ctbsize-1, ctbw,
										8, 12, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
						hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[4], in_start_addr+ctbw*y, ctbw,
										8, 0, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
					}
        			break;

        		case RIGHT:
				case DOWNRIGHT:
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], in_start_addr+ctbw*y+x/8-1, ctbw,
									8, 12, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[4], in_start_addr+ctbw*y+x/8, ctbw,
									8, 0, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
        			break;
        	}
			break;


		case LEFT:
			switch(block_in_ctu)
        	{
        		case TOPLEFT:
        		case LEFT:
					//at left of picture, so do nothing
        			break;

        		case RIGHT:
        		case DOWNRIGHT:
        			hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], in_start_addr+ctbw*y+x/8-1, ctbw,
									8, 12, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[4], in_start_addr+ctbw*y+x/8, ctbw,
									8, 0, 4, bit_depthC_pic_cmd, 8, rd_wr_en);
        			break;
        	}
			break;

		default:
			assert(0);
			break;
	}

	return ;
}

void  LoopFilter::hevc_deblock_rw16x4p_horz_chr(int x, int y, int rd_wr_en)
{
	//ctu's pos in picture
	input ctu_x, ctu_y, picr, picb;
	input picw = (HEVC_MAX_WIDTH/16);
	input ctbw  = (CTU_MAX_SIZE>>4);
	input ctbsize  = (ctb_size_filter>>4);
	input ctu_in_pic = (((ctu_x==0)<<1) + (ctu_y==0));
	input block_in_ctu = (picr && ((x+4)>=ctb_size_filter/2)) ? RIGHTBORD : (((x==0)<<1) + (y==0));
	input in_start_addr = ctbw*2;

	
	switch(ctu_in_pic)
	{
		case TOPLEFT:
			switch(block_in_ctu)
			{
				case TOPLEFT:
				case RIGHT:
					//at left of picture, so do nothing
					break;

				case LEFT:
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[8], in_start_addr+ctbw*(y-2)+x/8, ctbw,
									4, 0, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					break;

				case DOWNRIGHT:
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], in_start_addr+ctbw*(y-2)+x/8-1, ctbw,
									4, 8, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[8], in_start_addr+ctbw*(y-2)+x/8, ctbw,
									4, 0, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					break;
				case RIGHTBORD:
					if(y)
						hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], in_start_addr+ctbw*(y-2)+x/8, ctbw,
										4, 8, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					break;
			}
			break;

		case RIGHT:
			switch(block_in_ctu)
			{
				case TOPLEFT:
				case RIGHT:
					//at left of picture, so do nothing
					break;

				case LEFT:
					{
						hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock_pre], &deblock_uv_tmp_buff[0], in_start_addr+ctbw*(y-2)+ctbsize-1, ctbw,
									4, 8, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
						hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[8], in_start_addr+ctbw*(y-2), ctbw,
									4, 0, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					}
					break;

				case DOWNRIGHT:
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], in_start_addr+ctbw*(y-2)+x/8-1, ctbw,
									4, 8, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[8], in_start_addr+ctbw*(y-2)+x/8, ctbw,
									4, 0, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					break;

				case RIGHTBORD:
					if(y)
						hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], in_start_addr+ctbw*(y-2)+x/8, ctbw,
										4, 8, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					break;
			}
			break;

		case LEFT:
			switch(block_in_ctu)
			{
				case TOPLEFT:
				case LEFT:
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[8], ctbw*y+x/8, ctbw,
									4, 0, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					break;

				case RIGHT:
				case DOWNRIGHT:
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], ctbw*y+x/8-1, ctbw,
									4, 8, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[8], ctbw*y+x/8, ctbw,
									4, 0, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					break;
				case RIGHTBORD:
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], ctbw*y+x/8, ctbw,
									4, 8, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					break;
			}
			break;

		case DOWNRIGHT:
			switch(block_in_ctu)
			{
				case TOPLEFT:
				case LEFT:
					{
						hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock_pre], &deblock_uv_tmp_buff[0], ctbw*y+ctbsize-1, ctbw,
									4, 8, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
						hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[8], ctbw*y, ctbw,
									4, 0, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					}
					break;

				case RIGHT:
				case DOWNRIGHT:
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], ctbw*y+x/8-1, ctbw,
									4, 8, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[8], ctbw*y+x/8, ctbw,
									4, 0, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					break;

				case RIGHTBORD:
					hevc_read_write_ctu_uv_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], &deblock_uv_tmp_buff[0], ctbw*y+x/8, ctbw,
									4, 8, 8, bit_depthC_pic_cmd, 16, rd_wr_en);
					break;
			}
			break;

		default:
			assert(0);
			break;
	}

	return ;
}

void  LoopFilter::hevc_deblock_get_edge_pixel()
{
	int i;
	reg data_in[16*4];
	char tmp[8];
	
	input ctu_x, ctu_y, picr, picb;
	input picw = (HEVC_MAX_WIDTH/16);
	input ctbsize = (ctb_size_filter>>4);
	input ctbw  = (CTU_MAX_SIZE>>4);


	//get edge param
	if(ctu_x)
	{
		{
			//get last qp right col to curr qp data
			for(i=0;i<ctb_size_filter/8+1;i++)
			{
				deblock_qp[deblock_bs_qp_pcm_buff_idx][(8+1)*i] =
						deblock_qp[deblock_bs_qp_pcm_buff_idx^1][(8+1)*i+ctb_size_filter/8];
			}
			//get pcm and qtbypass right col to next qp data
			for(i=0;i<ctb_size_filter/8+1;i++)
			{
				deblock_qt_pcmflag[ctu_buf_cnt_deblock][(8+1)*i] =
						deblock_qt_pcmflag[ctu_buf_cnt_deblock_pre][(8+1)*i+ctb_size_filter/8];
			}
		}
	}
	
	//luma get edge param and pixel
	if(ctu_y)
	{
		//get qp top row of curr ctu
		if(ctu_x)	//not tilel, because tilel top left qp is saved in qp col buff.
		{
			for(i=0;i<ctb_size_filter/16;i++)
				deblock_qp_row_buff.read((ctu_x-1)*(ctb_size_filter/16)+i, (char *)&tmp[i*2]);
			deblock_qp[deblock_bs_qp_pcm_buff_idx][0] = tmp[ctb_size_filter/8-1];
		}
		for(i=0;i<ctb_size_filter/16;i++)
			deblock_qp_row_buff.read(ctu_x*(ctb_size_filter/16)+i, (char *)&tmp[i*2]);
		for(i=0;i<ctb_size_filter/8;i++)
			deblock_qp[deblock_bs_qp_pcm_buff_idx][1+i] = tmp[i];
		//get pcm and qtbypass flag
		if(ctu_x)	//not tilel, because tilel top left pcm flag is saved in pcm col buff.
		{
			for(i=0;i<ctb_size_filter/16;i++)
				deb_sao_qt_pcmflag_row_buff.read((ctu_x-1)*(ctb_size_filter/16)+i, (char *)&tmp[i*2]);
			deblock_qt_pcmflag[ctu_buf_cnt_deblock][0] = tmp[ctb_size_filter/8-1];
		}
		for(i=0;i<ctb_size_filter/16;i++)
			deb_sao_qt_pcmflag_row_buff.read(ctu_x*(ctb_size_filter/16)+i, (char *)&tmp[i*2]);
		for(i=0;i<ctb_size_filter/8;i++)
			deblock_qt_pcmflag[ctu_buf_cnt_deblock][1+i] = tmp[i];
	}
	
	//luma get edge param and pixel
	if(ctu_y)
	{
		//read row data to ctu buff from row buff
		for(i=0;i<ctbsize;i++)
		{
			hevc_read_write_ctu_ram(&deblock_y_row_buff, data_in, ctbsize*ctu_x+i, picw,
								4, 0, 16, bit_depthY_pic_cmd, READ);
			hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], data_in, i, ctbw,
								4, 0, 16, bit_depthY_pic_cmd, WRITE);
		}
	}

	//chroma get edge pixel
	if(ctu_y)
	{
		//read row data to ctu buff from row buff
		for(i=0;i<ctbsize;i++)
		{
			hevc_read_write_ctu_ram(&deblock_uv_row_buff, data_in, ctbsize*ctu_x+i, picw,
								2, 0, 16, bit_depthC_pic_cmd, 0);
			hevc_read_write_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], data_in, i, ctbw,
								2, 0, 16, bit_depthC_pic_cmd, 1);
		}
	}

	return ;
}

void  LoopFilter::hevc_deblock_save_pixel_and_flag()
{
	int i, qpstride = 8+1;
	int	height, right_col_start, ctu_x_tmp;
	short tmps[2];
	char tmp[8];
	reg	data_in[16*4], datatmp[16*4];
	input ctbw  = (CTU_MAX_SIZE>>4);
	input ctbsize  = (ctb_size_filter>>4);
	input ctu_x, ctu_y, picr, picb;
	input picw = (HEVC_MAX_WIDTH/16);
	input in_buff_addr;
	input tc_offset_div2;
	input beta_offset_div2;

	
	//deblock save left pixel of ctu for sao, include luma and chroma
	if(ctu_x)
	{
		in_buff_addr = ctu_y ? 0 : (ctbw*4);
		height = ctu_y ? (ctb_size_filter+4):ctb_size_filter;
		right_col_start = ctu_y ? 0:4;
		for(i=0;i<height;i++)	//save at ctu_buf_cnt_deblock_pre unit, for sao read
		{
			hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], datatmp, in_buff_addr+i*ctbw, ctbw,
											1, 0, 2, bit_depthY_pic_cmd, READ);
			sao_data_ctu_right_col_luma[ctu_buf_cnt_deblock_pre].write((i+right_col_start), (char *)&datatmp[0]);
		}
		for(i=0;i<height/2;i++)
		{
			hevc_read_write_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], datatmp, in_buff_addr/2+i*ctbw, ctbw,
											1, 0, 2, bit_depthC_pic_cmd, READ);
			sao_data_ctu_right_col_chr[ctu_buf_cnt_deblock_pre].write((i+right_col_start/2), (char *)datatmp);
		}
	}


	//set deblocking ready signal
	deblocking_buf_rdy[ctu_buf_cnt_deblock] = 1;


	//note: something must before picb, some after.
	if(picb)	//at bottom of picture , save nothing.
		return ;

	
	//save qp row, pcm , qtpass row
	if(ctu_x || picr)
	{
		if(ctu_x)
		{
			for(i=0;i<ctb_size_filter/8;i++)
				tmp[i] = deblock_qp[deblock_bs_qp_pcm_buff_idx^1][qpstride*(ctb_size_filter/8)+1+i];
			ctu_x_tmp = ctu_x;
			for(i=0;i<ctb_size_filter/16;i++)
				deblock_qp_row_buff.write(ctu_x_tmp*(ctb_size_filter/16)+i, (char *)&tmp[i*2]);
			//save pcm and qtbypass row
			for(i=0;i<ctb_size_filter/8;i++)
				tmp[i] = deblock_qt_pcmflag[ctu_buf_cnt_deblock_pre][qpstride*(ctb_size_filter/8)+1+i];
			for(i=0;i<ctb_size_filter/16;i++)
				deb_sao_qt_pcmflag_row_buff.write(ctu_x_tmp*(ctb_size_filter/16)+i, (char *)&tmp[i*2]);
		}
		
		if(picr)	//在图像边界时,存取当前的ctu最低行;
		{
			for(i=0;i<ctb_size_filter/8;i++)
				tmp[i] = deblock_qp[deblock_bs_qp_pcm_buff_idx][qpstride*(ctb_size_filter/8)+1+i];
			ctu_x_tmp = ctu_x;
			for(i=0;i<ctb_size_filter/16;i++)
				deblock_qp_row_buff.write(ctu_x_tmp*(ctb_size_filter/16)+i, (char *)&tmp[i*2]);
			//save pcm and qtbypass row
			for(i=0;i<ctb_size_filter/8;i++)
				tmp[i] = deblock_qt_pcmflag[ctu_buf_cnt_deblock][qpstride*(ctb_size_filter/8)+1+i];
			for(i=0;i<ctb_size_filter/16;i++)
				deb_sao_qt_pcmflag_row_buff.write(ctu_x_tmp*(ctb_size_filter/16)+i, (char *)&tmp[i*2]);
		}
	}

	
	//luma save ctu down 4 lines
	in_buff_addr = ctbw*ctb_size_filter;
	if(ctu_x || (ctu_x==0&&picr))
	{
		//left ctu is tilel, so tile left board need save 5 col down 4 rows
		if(ctu_x)	//if not tilel, save ctu_buf_cnt_deblock_pre down 4 lines ctb_size col
		{
			for(i=0;i<ctbsize;i++)
			{
				hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock_pre], data_in, in_buff_addr+i, ctbw,
												4, 0, 16, bit_depthY_pic_cmd, READ);
				hevc_read_write_ctu_ram(&deblock_y_row_buff, data_in, ctbsize*(ctu_x-1)+i, picw,
												4, 0, 16, bit_depthY_pic_cmd, WRITE);
			}
		}
		if(picr)	//if tiler, save ctu_buf_cnt_deblock down 4 lines ctb_size-4 col
		{
			for(i=0;i<ctbsize-1;i++)
			{
				hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], data_in, in_buff_addr+i, ctbw,
												4, 0, 16, bit_depthY_pic_cmd, READ);
				hevc_read_write_ctu_ram(&deblock_y_row_buff, data_in, ctbsize*ctu_x+i, picw,
												4, 0, 16, bit_depthY_pic_cmd, WRITE);
			}
			if(picr)	//at picr , row buff save all
			{
				hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], data_in, in_buff_addr+i, ctbw,
												4, 0, 16, bit_depthY_pic_cmd, READ);
				hevc_read_write_ctu_ram(&deblock_y_row_buff, data_in, ctbsize*ctu_x+i, picw,
												4, 0, 16, bit_depthY_pic_cmd, WRITE);
			}else	//row buff save ctb_size-4 col data
			{
				hevc_read_write_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_deblock], data_in, in_buff_addr+i, ctbw,
												4, 0, 12, bit_depthY_pic_cmd, READ);
				hevc_read_write_ctu_ram(&deblock_y_row_buff, data_in, ctbsize*ctu_x+i, picw,
												4, 0, 12, bit_depthY_pic_cmd, WRITE);
			}
		}
	}
	
	//chroma save ctu down 2 lines
	in_buff_addr = ctbw*ctb_size_filter/2;
	if(ctu_x || (ctu_x==0&&picr))	//save pre ctu uv row right 2 col uv pixel
	{
		//left ctu , tile left board need save 5 col down 2 lines
		if(ctu_x)
		{
			for(i=0;i<ctbsize;i++)
			{
				hevc_read_write_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock_pre], data_in, in_buff_addr+i, ctbw,
												2, 0, 16, bit_depthC_pic_cmd, READ);
				hevc_read_write_ctu_ram(&deblock_uv_row_buff, data_in, ctbsize*(ctu_x-1)+i, picw,
												2, 0, 16, bit_depthC_pic_cmd, WRITE);
			}
		}
		if(picr)
		{
			for(i=0;i<ctbsize-1;i++)
			{
				hevc_read_write_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], data_in, in_buff_addr+i, ctbw,
												2, 0, 16, bit_depthC_pic_cmd, READ);
				hevc_read_write_ctu_ram(&deblock_uv_row_buff, data_in, ctbsize*ctu_x+i, picw,
												2, 0, 16, bit_depthC_pic_cmd, WRITE);
			}
			if(picr)	//at picr , row buff save all
			{
				hevc_read_write_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], data_in, in_buff_addr+i, ctbw,
												2, 0, 16, bit_depthC_pic_cmd, READ);
				hevc_read_write_ctu_ram(&deblock_uv_row_buff, data_in, ctbsize*ctu_x+i, picw,
												2, 0, 16, bit_depthC_pic_cmd, WRITE);
			}else
			{
				hevc_read_write_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_deblock], data_in, in_buff_addr+i, ctbw,
												2, 0, 12, bit_depthC_pic_cmd, READ);
				hevc_read_write_ctu_ram(&deblock_uv_row_buff, data_in, ctbsize*ctu_x+i, picw,
												2, 0, 12, bit_depthC_pic_cmd, WRITE);
			}
		}
	}
	
	return ;
}

void  LoopFilter::hevc_filter_reverse_flag()
{
	ctu_buf_cnt_deblock_pre2 = ctu_buf_cnt_deblock_pre;
	ctu_buf_cnt_deblock_pre = ctu_buf_cnt_deblock;
	ctu_buf_cnt_deblock++;
	if(ctu_buf_cnt_deblock >= 4)
		ctu_buf_cnt_deblock = 0;

	deblock_bs_qp_pcm_buff_idx ^= 1;

	return ;
}

void  LoopFilter::hevc_deblock_calc16x8p_luma(int x0, int y0)
{
	int	x, y, bs;
	reg *src;
	input ctu_x, ctu_y, picr, picb;

	input	ctu_in_pic = ((ctu_x==0)<<1) + (ctu_y==0);
	input	block_in_ctu = (picr && ((x0+4)>=ctb_size_filter)) ? RIGHTBORD : (((x0==0)<<1) + (y0==0));
	input 	height = (ctu_y ? (ctb_size_filter+4):ctb_size_filter);
	input 	only_do_vert = (y0 == (height - 4));
	input 	tc_offset_div2;
	input 	beta_offset_div2;
	
	input	pcmidx = 0;
	input 	xinithor = 0;
	input 	horsize = 16;
	input 	vertsize = 8;


	if(y0==0)deblock_bs_cnt = 0;


	// vertical filtering
	for (y = 0; y < 8; y += 4)
	{
		for (x = 4; x < 16; x+=8)
		{
			//first 16x8 and last 16x8 olny do 2 bs; when at RIGHTBORD, all vert bs not do.
			if((only_do_vert && y==4) || (block_in_ctu==RIGHTBORD))
				bs = 0;
			else
				bs = deblock_bs[(x0+4)/16][deblock_bs_cnt++];	//offset must minus 2
			if (bs) {
				int	qpstride = 8+1;
				int	qpidx = (ctu_y==0)*qpstride + qpstride*((y0+y+(ctu_y!=0)*4)/8)+((x0+x)/8);
				int qp = (deblock_qp[deblock_bs_qp_pcm_buff_idx][qpidx]+deblock_qp[deblock_bs_qp_pcm_buff_idx][qpidx+1]+1)>>1;//(get_qPy(x - 1, y, sps) + get_qPy(x, y, sps) + 1) >> 1;
				int idxb = av_clip_c(qp + (beta_offset_div2 << 1), 0, MAX_QP);
				int beta = betatable[idxb];
				int no_p = 0;
				int no_q = 0;
				int idxt = av_clip_c(qp + DEFAULT_INTRA_TC_OFFSET * (bs - 1) + (tc_offset_div2 << 1), 0, MAX_QP + DEFAULT_INTRA_TC_OFFSET);
				int tc = tctable[idxt];

				if(1) {
					pcmidx = qpidx;
					if (deblock_qt_pcmflag[ctu_buf_cnt_deblock][pcmidx])
						no_p = 1;
					if (deblock_qt_pcmflag[ctu_buf_cnt_deblock][pcmidx+1])
						no_q = 1;
				}

				src = &deblock_y_tmp_buff[y*16+x];
				hevc_deblock_calc_unit_luma(src, 1, 16, no_p, no_q, beta, tc, x, y, bit_depthY_pic_cmd);
			}
		}
	}


	// horizontal filtering
	y = 4;
	for (x = 0; x < 16; x += 4)
	{
		if(only_do_vert)
			bs = 0;
		else
		{
			//at picr , last col do hor bs.
			if((x0+4)/16 == ctb_size_filter/16)
				bs = x==0 ? deblock_bs_y_right_col[(y0+4)/8] : 0;
			else
				bs = deblock_bs[(x0+4)/16][deblock_bs_cnt++];
		}
		if (bs) {
			int qpstride, qpidx, qp, idxb, beta, no_p, no_q, idxt, tc;
			short tmps[2];
			qpstride = 8+1;
			qpidx = ((y0+4)/8)*qpstride+((x0==0)?((x+4)/8):(1+(x0+x)/8));
			qp = (deblock_qp[deblock_bs_qp_pcm_buff_idx][qpidx]+deblock_qp[deblock_bs_qp_pcm_buff_idx][qpidx+qpstride]+1)>>1;//(get_qPy(x - 1, y, sps) + get_qPy(x, y, sps) + 1) >> 1;
			tc = tc_offset_div2;
			beta = beta_offset_div2;

			idxb = av_clip_c(qp + (beta << 1), 0, MAX_QP);
			beta = betatable[idxb];
			no_p = 0;
			no_q = 0;
			idxt = av_clip_c(qp + DEFAULT_INTRA_TC_OFFSET * (bs - 1) + (tc << 1), 0, MAX_QP + DEFAULT_INTRA_TC_OFFSET);
			tc = tctable[idxt];

			if(1) {
				pcmidx = qpidx;
				if (deblock_qt_pcmflag[ctu_buf_cnt_deblock][pcmidx])
					no_p = 1;
				if (deblock_qt_pcmflag[ctu_buf_cnt_deblock][pcmidx+qpstride])
					no_q = 1;
			}

			src = &deblock_y_tmp_buff[y*16+x];
			hevc_deblock_calc_unit_luma(src, 16, 1, no_p, no_q, beta, tc, x, 0, bit_depthY_pic_cmd);
		}
	}

}

void  LoopFilter::hevc_deblock_calc16x8p_chr(int x0, int y0, int dir)
{
	int x=x0, y=y0;
	int bs, bsindex;
	reg *src;
	
	input ctu_x, ctu_y, picr, picb;
	
	input 	tc_offset_div2;
	input 	beta_offset_div2;
	input	ctu_last_col = (x0+4)==ctb_size_filter/2;
	input	pcmidx = 0;
	input 	xinithor = 0;
	input 	filterlen = 4;


	//get next col chr bs data
	if(y0==0 && x0)
	{
		deblock_bs_chr_right[0] = deblock_bs[(x0+4)/8-1][7];	//add 4 for picr or tiler
		deblock_bs_chr_right[1] = deblock_bs[(x0+4)/8-1][7+8*2];
		deblock_bs_chr_right[2] = deblock_bs[(x0+4)/8-1][7+8*4];
		deblock_bs_chr_right[3] = deblock_bs[(x0+4)/8-1][7+8*6];
	}
	
	if(dir == VERT)
	{
		if(y0==0)
			deblock_bs_cnt = 0;
		else
			deblock_bs_cnt += 2;

		// vertical filtering
		for (y = 0; y < 8; y += 4)
		{
			bsindex = deblock_bs_cnt*8 + (y/4)*8 + 2;
			bs = deblock_bs[x0/8][bsindex];
			if (bs == 2) {
				int qpstride = 8+1;
				int qpidx = qpstride + qpstride*((y0*2+y*2)/8)+((x0*2)/8);
				int qp = (deblock_qp[deblock_bs_qp_pcm_buff_idx][qpidx]+deblock_qp[deblock_bs_qp_pcm_buff_idx][qpidx+1]+1)>>1;//(get_qPy(x - 1, y, sps) + get_qPy(x, y, sps) + 1) >> 1;
				int no_p = 0;
				int no_q = 0;
				int tc_cb = chroma_tc(qp, CB, tc_offset_div2);
				int tc_cr = chroma_tc(qp, CR, tc_offset_div2);

				if(1) {
					pcmidx = qpidx;
					if (deblock_qt_pcmflag[ctu_buf_cnt_deblock][pcmidx])
						no_p = 1;
					if (deblock_qt_pcmflag[ctu_buf_cnt_deblock][pcmidx+1])
						no_q = 1;
				}

				src = &deblock_uv_tmp_buff[4+y*8];
				hevc_deblock_calc_unit_chroma(src, 2, 8, filterlen, no_p, no_q, tc_cb, tc_cr, bit_depthC_pic_cmd);
			}
		}
	}else if(dir == HORZ)
	{
		for (x = 0; x < 8; x += 4)
		{
			if(x == 0)
				bs = deblock_bs_chr_right[y0/8];
			else if(ctu_last_col && x==4)
				bs = 0;
			else
			{
				bsindex = deblock_bs_cnt*8 + 5;
				bs = deblock_bs[x0/8][bsindex];
			}
			if (bs == 2) {
				int qpstride, qpidx, qp, no_p, no_q, tc, tc_cb, tc_cr;
				short tmps[2];
				qpstride = 8+1;
				if(ctu_last_col)
					qpidx = ((y0*2)/8)*qpstride+(x0+x+4)/4;
				else
					qpidx = ((y0*2)/8)*qpstride+((x0==0)?((x*2)/8):(x0+x)/4);
				qp = (deblock_qp[deblock_bs_qp_pcm_buff_idx][qpidx]+deblock_qp[deblock_bs_qp_pcm_buff_idx][qpidx+qpstride]+1)>>1;
				no_p = 0;
				no_q = 0;
				tc = tc_offset_div2;

				tc_cb = chroma_tc(qp, CB, tc);
				tc_cr = chroma_tc(qp, CR, tc);

				if(1) {
					pcmidx = qpidx;
					if (deblock_qt_pcmflag[ctu_buf_cnt_deblock][pcmidx])
						no_p = 1;
					if (deblock_qt_pcmflag[ctu_buf_cnt_deblock][pcmidx+qpstride])
						no_q = 1;
				}

				src = &deblock_uv_tmp_buff[2*16+x*2];
				hevc_deblock_calc_unit_chroma(src, 16, 2, filterlen, no_p, no_q, tc_cb, tc_cr, bit_depthC_pic_cmd);
			}
		}

	}

	return ;
}

int LoopFilter::chroma_tc(int iQpY, int c_idx, int tc)
{
    int idxt, qp, offset;

    // slice qp offset is not used for deblocking
    if (c_idx == 1)
        offset = pps_cb_qp_offset_pic_cmd;
    else
        offset = pps_cr_qp_offset_pic_cmd;

	iQpY = iQpY + offset;

	qp = ( ((iQpY) < 0) ? (iQpY) : (((iQpY) > 57) ? ((iQpY)-6) : aucChromaScale[(iQpY)]) );

    idxt = av_clip_c(qp + DEFAULT_INTRA_TC_OFFSET + (tc<<1), 0, 53);
    return tctable[idxt];
}

/*read_line_width:read line的宽度,单位是象素*/
void  LoopFilter::hevc_read_ctu_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
									int read_line_nums, int read_start_pos, int read_line_width_pixel, int bitdepth)
{
	int i;
	unsigned char tmp[24];

	for(i=0;i<read_line_nums;i++)//read linenum lines pixels, only ram_len pixel
	{
		in_buff->read(ram_start_addr+i*ram_ctu_stride, (char *)tmp);
		hevc_pixel_unpack(tmp, tmp_buff+i*16, read_start_pos, read_line_width_pixel, bitdepth);
	}
}

/*read_line_width_bit:read line的宽度,单位是bit,但是必须保证byte对齐*/
/*read_start_bit	    :在位宽内的起始bit位置,但是必须保证byte对齐*/
void  LoopFilter::hevc_write_ctu_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
									int write_line_nums, int write_start_pos, int write_line_width_pixel, int bitdepth)
{
	int i;
	unsigned char tmp[24];
	//assert((write_line_width_bit&0x7)==0);

	for(i=0;i<write_line_nums;i++)//read linenum lines pixels, only ram_len pixel
	{
		in_buff->read(ram_start_addr+i*ram_ctu_stride, (char *)tmp);
		hevc_pixel_pack(tmp, tmp_buff+i*16, write_start_pos, write_line_width_pixel, bitdepth);
		in_buff->write(ram_start_addr+i*ram_ctu_stride, (char *)tmp);
	}
}

void  LoopFilter::hevc_read_write_ctu_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
									int read_line_nums, int read_start_pos, int read_line_width_pixel, int bitdepth, int rd_wr_en)
{
	if(rd_wr_en == READ)
	{
		hevc_read_ctu_ram(in_buff, tmp_buff, ram_start_addr, ram_ctu_stride,
									read_line_nums, read_start_pos, read_line_width_pixel, bitdepth);
	}else
	{
		hevc_write_ctu_ram(in_buff, tmp_buff, ram_start_addr, ram_ctu_stride,
									read_line_nums, read_start_pos, read_line_width_pixel, bitdepth);
	}
}

void  LoopFilter::hevc_read_write_ctu_uv_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
									int read_line_nums, int read_start_pos, int read_line_width_pixel, int bitdepth, int regstride, int rd_wr_en)
{
	int i;
	unsigned char tmp[24];

	if(rd_wr_en == READ)
	{
		for(i=0;i<read_line_nums;i++)
		{
			in_buff->read(ram_start_addr+i*ram_ctu_stride, (char *)tmp);
			hevc_pixel_unpack(tmp, tmp_buff+i*regstride, read_start_pos, read_line_width_pixel, bitdepth);
		}
	}else
	{
		for(i=0;i<read_line_nums;i++)
		{
			in_buff->read(ram_start_addr+i*ram_ctu_stride, (char *)tmp);
			hevc_pixel_pack(tmp, tmp_buff+i*regstride, read_start_pos, read_line_width_pixel, bitdepth);
			in_buff->write(ram_start_addr+i*ram_ctu_stride, (char *)tmp);
		}
	}
}

void  LoopFilter::hevc_pixel_unpack(unsigned char *src, reg *dst, int offset, int pixelnum, int bit_depth)
{
	int i, bitpos, startbit = 0;;
	assert((bit_depth >=8) && (bit_depth <=12));
	assert((offset&1)==0);
	assert((pixelnum&1)==0);

	//every 10 bit mean a pixel
	if(bit_depth == 8 || bit_depth == 9 || bit_depth == 10)
	{
		src = &src[(offset*10)/8];		
		if((offset*10)&7)
			startbit = (offset*10)&7;
		else
			startbit = 0;
		bitpos = startbit;
		for(i=0;i<pixelnum;i++)
		{
			dst[i] = (src[0]>>bitpos)&((1<<(8-bitpos))-1);
			dst[i] += ((src[1]<<(8-bitpos))&0x3ff);
			src++;
			bitpos = bitpos+2;
			if(bitpos>=8)
			{
				bitpos -= 8;
				src++;
			}
		}
	}else
		assert(0);
	
	#if 0	//8, 9, 10 pack
	if(bit_depth == 8)
	{
		src = &src[offset];
		for(i=0;i<pixelnum;i++)
		{
			dst[i] = src[i];
		}
	}
	else if(bit_depth == 9 || bit_depth == 10)
	{
		src = &src[(offset*10)/8];		
		if((offset*10)&7)
			startbit = (offset*10)&7;
		else
			startbit = 0;
		bitpos = startbit;
		for(i=0;i<pixelnum;i++)
		{
			dst[i] = (src[0]<<(bitpos+2))&0x3ff;
			dst[i] += (src[1]>>(8-(bitpos+2)));
			src++;
			bitpos = bitpos+2;
			if(bitpos>=8)
			{
				bitpos -= 8;
				src++;
			}
		}
	}else if(bit_depth == 12)
	{
		src = &src[(offset/2)*3];
		j = 0;
		for(i=0;i<pixelnum/2;i++)
		{
			dst[i*2] = src[j++]<<4;
			dst[i*2] += (src[j]>>4)&0xf;
			dst[i*2+1] = (src[j++]&0xf)<<8;
			dst[i*2+1] += src[j++];
		}
	}else
		assert(0);
	#endif
}

void  LoopFilter::hevc_pixel_pack(unsigned char *dst, reg *src, int offset, int pixelnum, int bit_depth)
{
	int i, j, totbits, bitpos, bitsleft;
	assert((bit_depth >=8) && (bit_depth <=12));
	assert((offset&1)==0);
	//assert((pixelnum&1)==0);

	//every 10 bit mean a pixel
	if(bit_depth == 8 || bit_depth == 9 || bit_depth == 10)
	{
		dst = &dst[(offset*10)/8];
		if((offset*10)&7)
		{
			j = (offset*10)&7;
			dst[0] = ((src[0]<<j)&0xff)+(dst[0]&((1<<j)-1));
			dst++;
			bitpos = 8-j;
			bitsleft = (10-(8-j));			//left bits
			totbits = pixelnum*10-(8-j);	//all bits
		}else
		{
			bitpos = 0;
			bitsleft = 10;			//left bits
			totbits = pixelnum*10;	//all bits
		}
		for(i=0;i<(totbits+7)/8;i++)
		{
			if(bitsleft>=8) //getbits(8);
				dst[i] = (src[0]>>bitpos)&0xff;
			else
				dst[i] = ((src[0]>>bitpos)&0xff)+((src[1]&((1<<(bitpos-2)))-1)<<(10-bitpos));
			bitpos += 8;
			bitsleft -= 8;
			if(bitpos>=10)
				bitpos -= 10;
			if(bitsleft<=0)
			{
				src++;
				bitsleft += 10;
			}
		}
	}else
		assert(0);

	#if 0	//8, 9, 10 pack
	if(bit_depth == 8)
	{
		dst = &dst[offset];
		for(i=0;i<pixelnum;i++)
		{
			dst[i] = src[i];
		}
	}else if(bit_depth == 9 || bit_depth == 10)
	{
		dst = &dst[(offset*10)/8];
		if((offset*10)&7)
		{
			j = (offset*10)&7;
			dst[0] = (src[0]>>(10-(8-j)))+((dst[0]>>(8-j))<<(8-j));
			dst++;
			bitpos = 8-j;
			bitsleft = (10-(8-j));			//left bits
			totbits = pixelnum*10-(8-j);	//all bits
		}else
		{
			bitpos = 0;
			bitsleft = 10;			//left bits
			totbits = pixelnum*10;	//all bits
		}
		for(i=0;i<(totbits+7)/8;i++)
		{
			if(bitsleft>=8) //getbits(8);
				dst[i] = (src[0]>>(2-bitpos))&0xff;
			else
				dst[i] = ((src[0]<<(bitpos-2))&0xff)+(src[1]>>(12-bitpos));
			bitpos += 8;
			bitsleft -= 8;
			if(bitpos>=10)
				bitpos -= 10;
			if(bitsleft<=0)
			{
				src++;
				bitsleft += 10;
			}
		}
	}else if(bit_depth == 12)
	{
		dst = &dst[(offset/2)*3];
		j = 0;
		for(i=0;i<pixelnum/2;i++)
		{
			dst[j++] = src[i*2]>>4;
			dst[j] = (src[i*2]&0x3f)<<4;
			dst[j++] += (src[i*2+1]>>8);
			dst[j++] = src[i*2+1]&0xff;
		}
	}else
		assert(0);
	#endif
}


int LoopFilter::av_clip_c(int a, int amin, int amax)
{
    if      (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}

int LoopFilter::av_clip_pixel(int a, int cidx)
{
	int	bit_depth;
	
	if(cidx == LUMA)
		bit_depth = bit_depthY_pic_cmd;
	else if(cidx == CHROMA)
		bit_depth = bit_depthC_pic_cmd;
	else
		assert(0);
	
	if(a < 0)
		a = 0;
	if(a > ((1<<bit_depth)-1))
		a = ((1<<bit_depth)-1);
	
	return a;
}

int	LoopFilter::abs(int a)
{
	if(a < 0)
		a = -a;
	return a;
}
void LoopFilter::hevc_deblock_calc_unit_luma(reg *_pix, int xstride, int ystride,
									int no_p, int no_q, int _beta, int _tc, int x0, int y0, int bit_depth)
{
    int d;
    reg *pix = (reg*)_pix;
	int dp0, dq0, dp3, dq3, d0, d3, beta, tc;


    dp0 = abs(P2 - 2 * P1 +  P0);
    dq0 = abs(Q2 - 2 * Q1 +  Q0);
    dp3 = abs(TP2 - 2 * TP1 + TP0);
    dq3 = abs(TQ2 - 2 * TQ1 + TQ0);
    d0 = dp0 + dq0;
    d3 = dp3 + dq3;

    beta = _beta << (bit_depth - 8);
    tc = _tc << (bit_depth - 8);

    if (d0 + d3 < beta) {
        const int beta_3 = beta >> 3;
        const int beta_2 = beta >> 2;
        const int tc25 = ((tc * 5 + 1) >> 1);

        if(abs( P3 -  P0) + abs( Q3 -  Q0) < beta_3 && abs( P0 -  Q0) < tc25 &&
           abs(TP3 - TP0) + abs(TQ3 - TQ0) < beta_3 && abs(TP0 - TQ0) < tc25 &&
                                 (d0 << 1) < beta_2 &&      (d3 << 1) < beta_2) {
            // strong filtering
            const int tc2 = tc << 1;
            for(d = 0; d < 4; d++) {
                const int p3 = P3;
                const int p2 = P2;
                const int p1 = P1;
                const int p0 = P0;
                const int q0 = Q0;
                const int q1 = Q1;
                const int q2 = Q2;
                const int q3 = Q3;
                if(!no_p) {
                    P0 = av_clip_c(( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3, p0-tc2, p0+tc2);
                    P1 = av_clip_c(( p2 + p1 + p0 + q0 + 2 ) >> 2, p1-tc2, p1+tc2);
                    P2 = av_clip_c(( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3, p2-tc2, p2+tc2);
                }
                if(!no_q) {
                    Q0 = av_clip_c(( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3, q0-tc2, q0+tc2);
                    Q1 = av_clip_c(( p0 + q0 + q1 + q2 + 2 ) >> 2, q1-tc2, q1+tc2);
                    Q2 = av_clip_c(( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3, q2-tc2, q2+tc2);
                }
                pix += ystride;
            }
        } else { // normal filtering
            int nd_p = 1;
            int nd_q = 1;
            const int tc_2 = tc >> 1;
            if (dp0 + dp3 < ((beta+(beta>>1))>>3))
                nd_p = 2;
            if (dq0 + dq3 < ((beta+(beta>>1))>>3))
                nd_q = 2;

            for(d = 0; d < 4; d++) {
                const int p2 = P2;
                const int p1 = P1;
                const int p0 = P0;
                const int q0 = Q0;
                const int q1 = Q1;
                const int q2 = Q2;
                int delta0 = (9*(q0 - p0) - 3*(q1 - p1) + 8) >> 4;
                if (abs(delta0) < 10 * tc) {
                    delta0 = av_clip_c(delta0, -tc, tc);
                    if(!no_p)
                        P0 = av_clip_pixel(p0 + delta0, LUMA);
                    if(!no_q)
                        Q0 = av_clip_pixel(q0 - delta0, LUMA);
                    if(!no_p && nd_p > 1) {
                        const int deltap1 = av_clip_c((((p2 + p0 + 1) >> 1) - p1 + delta0) >> 1, -tc_2, tc_2);
                        P1 = av_clip_pixel(p1 + deltap1, LUMA);
                    }
                    if(!no_q && nd_q > 1) {
                        const int deltaq1 = av_clip_c((((q2 + q0 + 1) >> 1) - q1 - delta0) >> 1, -tc_2, tc_2);
                        Q1 = av_clip_pixel(q1 + deltaq1, LUMA);
                    }
                }
                pix += ystride;
            }
        }
    }
}

void  LoopFilter::hevc_deblock_calc_unit_chroma(reg *_pix, int xstride, int ystride, int datalen, 
											int no_p, int no_q, int _tc_cb, int _tc_cr, int bit_depth)
{
    int d;
    reg *pix = _pix;
    int tc = _tc_cb << (bit_depth - 8);

	//U deblocking
    for(d = 0; d < datalen; d++) {
        int delta0;
        const int p1 = P1;
        const int p0 = P0;
        const int q0 = Q0;
        const int q1 = Q1;
        delta0 = av_clip_c((((q0 - p0) << 2) + p1 - q1 + 4) >> 3, -tc, tc);
        if(!no_p)
            P0 = av_clip_pixel(p0 + delta0, CHROMA);
        if(!no_q)
            Q0 = av_clip_pixel(q0 - delta0, CHROMA);
        pix += ystride;
    }

	//V deblocking
	pix = _pix+1;
	tc = _tc_cr << (bit_depth - 8);
    for(d = 0; d < datalen; d++) {
        int delta0;
        const int p1 = P1;
        const int p0 = P0;
        const int q0 = Q0;
        const int q1 = Q1;
        delta0 = av_clip_c((((q0 - p0) << 2) + p1 - q1 + 4) >> 3, -tc, tc);
        if(!no_p)
            P0 = av_clip_pixel(p0 + delta0, CHROMA);
        if(!no_q)
            Q0 = av_clip_pixel(q0 - delta0, CHROMA);
        pix += ystride;
    }
}

void  LoopFilter::hevc_deblock_hrd()
{

	hevc_deblock_get_edge_pixel();

	hevc_deblock_calc_ctu();

	hevc_deblock_save_pixel_and_flag();

	return ;
}


