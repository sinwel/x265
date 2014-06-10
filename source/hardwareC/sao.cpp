

#include <string.h>
#include "sao.h"


#define 	input					int
#define 	output					int


void  SaoModule::hevc_sao_hrd()
{
	input ctu_x=0, ctu_y=0, picr=0, picb=0;
	
	hevc_sao_ctrl();

	hevc_sao_output();	//ctu buff output

	hevc_sao_rev_flag();	//ctu_buf_cnt_sao increase at this function

	return ;
}

void  SaoModule::hevc_sao_ctrl()
{
	//to do sao
	input ctu_x=0, ctu_y=0;
	
	sao_next_state = ST_IDLE;
	
	while(1)
	{
		sao_curr_state = sao_next_state;

		switch(sao_curr_state)
		{
			case ST_IDLE:
				if(ctu_y == 0)
					sao_next_state = ST_MID_LOWER;
				else
					sao_next_state = ST_MID_UPPER;
				break;
				
			case ST_MID_LOWER:
				hevc_sao_para_fetch();
				hevc_sao_calc_mid_lower();
				hevc_sao_save_mid_lower_edge();
				
				sao_next_state = ST_CTU_END;
				break;
				
			case ST_MID_UPPER:
				hevc_sao_para_fetch();
				hevc_sao_calc_mid_upper();
				
				sao_next_state = ST_MID_LOWER;
				break;

			case ST_CTU_END:
				return ;
			
			default:
				assert(0);
				break;
		}
	}

	return ;
}

/*read_line_width:read line的宽度,单位是象素*/
void  SaoModule::hevc_read_ctu_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
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
void  SaoModule::hevc_write_ctu_ram(RAM *in_buff, reg *tmp_buff, int ram_start_addr, int ram_ctu_stride,
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

void  SaoModule::hevc_sao_para_fetch()
{
	//to get sao para
	int	saoparunit[3];
	
	input ctu_x=0, ctu_y=0, merge=0, dir=0;
	
	input	sps_sample_adaptive_offset_enabled_flag;// = ((LOOPFILT_SLICE_CMD*)&loop_filt_slice_cmd[ctu_buf_cnt_sao])->saoe;
	input	slice_sao_luma_flag;// = ((LOOPFILT_SLICE_CMD*)&loop_filt_slice_cmd[ctu_buf_cnt_sao])->saol;
	input	slice_sao_chroma_flag;// = ((LOOPFILT_SLICE_CMD*)&loop_filt_slice_cmd[ctu_buf_cnt_sao])->saoc;
	
	//get param
	memset(saoparunit, 0, sizeof(saoparunit));
	
	switch(sao_curr_state)
	{
		case ST_MID_UPPER:
			if(ctu_y)
				saoparamrow.read(ctu_x, (char *)saoparunit);
			break;

		case ST_MID_LOWER:	//直接返回,不读ram param
			if(sps_sample_adaptive_offset_enabled_flag && (slice_sao_luma_flag||slice_sao_chroma_flag))
			{
				if(merge)
			    {
		        	if(dir)	// 1: up, 0:left
		        	{
						//copy up param
						saoparamrow.read(ctu_x, (char *)saoparunit);
						if(slice_sao_luma_flag)
							sao_para_luma[ctu_buf_cnt_sao] = saoparunit[0];
						if(slice_sao_chroma_flag)
						{
							sao_para_U[ctu_buf_cnt_sao] = saoparunit[1];
							sao_para_V[ctu_buf_cnt_sao] = saoparunit[2];
						}
		        	}else
	        		{
						//copy left param
						{
							if(slice_sao_luma_flag)
								sao_para_luma[ctu_buf_cnt_sao] = sao_para_luma[ctu_buf_cnt_sao_pre];
							if(slice_sao_chroma_flag)
							{
								sao_para_U[ctu_buf_cnt_sao] = sao_para_U[ctu_buf_cnt_sao_pre];
								sao_para_V[ctu_buf_cnt_sao] = sao_para_V[ctu_buf_cnt_sao_pre];
							}
						}
					}
				}
			}
			return ;
			
		default:
			assert(0);
			break;
	}

	sao_para_luma_up = saoparunit[0];
	sao_para_U_up = saoparunit[1];
	sao_para_V_up = saoparunit[2];
	
	return ;
}

void  SaoModule::hevc_sao_save_mid_lower_edge()
{
	int	i, saoparunit[3] = {0};
	reg		datain[16];
	
	input 	ctu_x=0, ctu_y=0, picr=0, picb=0;
	
	input 	ctbw   = (CTU_MAX_SIZE>>4);
	input 	ctbsize   = (ctb_size_filter>>4);
	input 	height = (ctb_size_filter-1);
	input 	heightC = (ctb_size_filter/2-1);

	
	//save sao param
	if(!picb)
	{
		saoparunit[0] = sao_para_luma[ctu_buf_cnt_sao];
		saoparunit[1] = sao_para_U[ctu_buf_cnt_sao];
		saoparunit[2] = sao_para_V[ctu_buf_cnt_sao];
		saoparamrow.write(ctu_x, (char *)saoparunit);
	}

	//save sao ctu 59th line
	if(!picb)
	{	
		//luma save info
		for(i=0;i<ctbsize-1;i++)
		{
			hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], datain, (height*ctbw+i), ctbw,
									1, 0, CTU_IN_MEM_PIXEL_WID, bit_depthY_pic_cmd);
			hevc_write_ctu_ram(&sao_y_row_buff, datain, (ctu_x*ctbsize+i), ctbsize,
									1, 0, CTU_IN_MEM_PIXEL_WID, bit_depthY_pic_cmd);
		}

		{
			hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], datain, (height*ctbw+i), ctbw,
									1, 0, CTU_IN_MEM_PIXEL_WID, bit_depthY_pic_cmd);
			hevc_write_ctu_ram(&sao_y_row_buff, datain, (ctu_x*ctbsize+i), ctbsize,
									1, 0, CTU_IN_MEM_PIXEL_WID, bit_depthY_pic_cmd);
		}
		
		//chroma save info
		for(i=0;i<ctbsize-1;i++)
		{
			hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], datain, (heightC*ctbw+i), ctbw,
									1, 0, CTU_IN_MEM_PIXEL_WID, bit_depthC_pic_cmd);
			hevc_write_ctu_ram(&sao_uv_row_buff, datain, (ctu_x*ctbsize+i), ctbsize,
									1, 0, CTU_IN_MEM_PIXEL_WID, bit_depthC_pic_cmd);
		}

		{
			hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], datain, (heightC*ctbw+i), ctbw,
									1, 0, CTU_IN_MEM_PIXEL_WID, bit_depthC_pic_cmd);
			hevc_write_ctu_ram(&sao_uv_row_buff, datain, (ctu_x*ctbsize+i), ctbsize,
									1, 0, CTU_IN_MEM_PIXEL_WID, bit_depthC_pic_cmd);
		}
	}
	
	//save avail_l and avail_t flag
	{
		char avail_l = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_l;
		char avail_t = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_t;
		sao_avail_l_flag.write(ctu_x, &avail_l);
		sao_avail_t_flag.write(ctu_y, &avail_t);
	}
	
	return ;
}

void  SaoModule::hevc_sao_rev_flag()
{
	int		ctu_out_max_size;

	input 	ctu_x=0, picr=0;
	
	input	ctu_st_pos = (ctu_x-ctu_num_in_outbuff);

	//clear deblocking_buf_rdy
	if(deblocking_buf_rdy[ctu_buf_cnt_sao])
		deblocking_buf_rdy[ctu_buf_cnt_sao] = 0;

	ctu_buf_cnt_sao_pre = ctu_buf_cnt_sao;
	ctu_buf_cnt_sao++;
	if(ctu_buf_cnt_sao >= 4)
		ctu_buf_cnt_sao = 0;


	ctu_out_max_size = ((ctb_size_filter==64)?(CTU_MAX_SIZE):(CTU_MAX_SIZE*2));
	
	ctu_num_in_outbuff++;
	if(picr || ((ctu_num_in_outbuff+1)*ctb_size_filter > ctu_out_max_size))
	{
		if(sao_out_tilel_flag[ctu_out_buff_cnt])
			sao_out_tilel_flag[ctu_out_buff_cnt] = 0;
		ctu_num_in_outbuff = 0;
		ctu_out_buff_cnt ^= 1;
	}
	
	return ;
}

void  SaoModule::hevc_sao_calc_luma_upper(int width, reg *data_out, int x, int y)
{
	//to do sao
	char flag;
	int tmp, shift, widthsrc = width;//width, height, 
	int i, diff0, diff1, offset_val, passidx;
	int	type_id, subtype_id, offset0, offset1, offset2, offset3;
	int	edge_idx[] = { 1, 2, 0, 3, 4 };
	reg *data_in, *data_in_up, *data_in_down;

	input 	ctu_x=0, ctu_y=0, picr=0, pic_width_in_luma_samples=0;
	
	input	widthr  = picr ? (pic_width_in_luma_samples-ctu_x*ctb_size_filter) : ctb_size_filter;
	input	lastcol_in_ctu = ((widthr+CTU_IN_MEM_PIXEL_WID-1)/CTU_IN_MEM_PIXEL_WID-1);
	input	islastcol = (x==lastcol_in_ctu)&&((x*CTU_IN_MEM_PIXEL_WID+width)>=widthr);
	
	input	pic_rightcol = (picr && (x*CTU_IN_MEM_PIXEL_WID+width)>=widthr);
	input	avail_t, avail_tl, avail_tr, passflag;
	input	ctu_avail_l, ctu_avail_t, ctu_avail_tl, ctu_avail_r, ctu_avail_tr;


	pic_rightcol = (sao_curr_state==ST_HEAD_UPPER) ? 0 : pic_rightcol;
	islastcol = (sao_curr_state==ST_HEAD_UPPER) ? 1 : islastcol;
	passflag = (sao_curr_state==ST_HEAD_UPPER) ? 0 : 1;
	
	switch(sao_curr_state)
	{
		case ST_MID_UPPER:
			//its mean mid upper's bottom left point
			avail_tl = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_tr;
			ctu_avail_tl = (!avail_tl && (y==3) && (x==0));
			avail_tr = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_deblock])->avail_tl;
			ctu_avail_tr = (!avail_tr && (y==3) && islastcol);
			//its mean mid upper's bottom edge
			sao_avail_l_flag.read(ctu_x, &flag);
			ctu_avail_l = (!flag && (x==0));
			if(!picr)
			{
				sao_avail_l_flag.read(ctu_x+1, &flag);
				ctu_avail_r = !flag && islastcol;
			}else
				ctu_avail_r = islastcol;
			//its mean mid upper last row
			avail_t = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_t;
			ctu_avail_t = (!avail_t && (y==3));
			break;
			
		default:
			assert(0);
			break;
	}
	
	
	//get in data
	data_in_up = &sao_data_in_luma[sao_luma_idx_up][1];
	data_in = &sao_data_in_luma[sao_luma_idx_cur][1];
	data_in_down = &sao_data_in_luma[sao_luma_idx_down][1];

	//buff idx circle
	tmp = sao_luma_idx_up;
	sao_luma_idx_up = sao_luma_idx_cur;
	sao_luma_idx_cur = sao_luma_idx_down;
	sao_luma_idx_down = tmp;

	type_id = ((SAO_PARAMETER *)&sao_para_luma_up)->type_id;
    subtype_id = ((SAO_PARAMETER *)&sao_para_luma_up)->subtype_id;
    offset0 = ((SAO_PARAMETER *)&sao_para_luma_up)->offset0;
    offset1 = ((SAO_PARAMETER *)&sao_para_luma_up)->offset1;
    offset2 = ((SAO_PARAMETER *)&sao_para_luma_up)->offset2;
    offset3 = ((SAO_PARAMETER *)&sao_para_luma_up)->offset3;
	
	memset(sao_offset_val, 0, sizeof(sao_offset_val));
	memset(sao_band_table, 0, sizeof(sao_band_table));

	
	switch(type_id)
	{
		case SAO_EDGE:
			
			sao_offset_val[0] = 0;
			sao_offset_val[1] = offset0;
			sao_offset_val[2] = offset1;
			sao_offset_val[3] = offset2;
			sao_offset_val[4] = offset3;

			switch(subtype_id)
			{
				case SAO_EO_HORIZ:
					if(ctu_avail_l)data_out[0] = data_in[0];
					i = (ctu_avail_l) ? 1 : 0;
					if(ctu_avail_r || pic_rightcol)
					{
						data_out[width-1] = data_in[width-1];
						width -= 1;
					}
					
					for (; i < width; i++)
					{
						passidx = (y/8)*(8+1)+(1+x*2+i/8)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i], data_in[i-1]);
							diff1 = CMP(data_in[i], data_in[i+1]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i] = av_clip_pixel(data_in[i] + sao_offset_val[offset_val], LUMA);
						}else
							data_out[i] = data_in[i];
					}
					
				break;

				case SAO_EO_VERT:
					if(ctu_avail_t)	//copy data
					{
						memcpy(data_out, data_in, width*sizeof(reg));
						break;
					}
					
					for (i = 0; i < width; i++)
					{
						passidx = (y/8)*(8+1)+(1+x*2+i/8)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i], data_in_up[i]);
							diff1 = CMP(data_in[i], data_in_down[i]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i] = av_clip_pixel(data_in[i] + sao_offset_val[offset_val], LUMA);
						}else
							data_out[i] = data_in[i];
					}
					
				break;

				case SAO_EO_135D:
					for (i=0; i < width; i++)
					{
						passidx = (y/8)*(8+1)+(1+x*2+i/8)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i], data_in_up[i-1]);
							diff1 = CMP(data_in[i], data_in_down[i+1]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i] = av_clip_pixel(data_in[i] + sao_offset_val[offset_val], LUMA);
						}else
							data_out[i] = data_in[i];
					}

					if(y==3)	//do last line pixel
					{
						if(((x==0)&&(ctu_avail_l || ctu_avail_t)) || ((x!=0)&&(ctu_avail_t)))
							data_out[0] = data_in[0];
						if(ctu_avail_t)
							memcpy(&data_out[1], &data_in[1], (width-2)*sizeof(reg));
						if(islastcol&&(ctu_avail_tr || pic_rightcol))
							data_out[width-1] = data_in[width-1];
						if(!islastcol&&ctu_avail_t)
							data_out[width-1] = data_in[width-1];
					}else
					{
						if(ctu_avail_l)data_out[0] = data_in[0];
						if(ctu_avail_r || pic_rightcol)data_out[width-1] = data_in[width-1];
					}
					
				break;

				case SAO_EO_45D:
					for (i=0; i < width; i++)
					{
						passidx = (y/8)*(8+1)+(1+x*2+i/8)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i], data_in_up[i+1]);
							diff1 = CMP(data_in[i], data_in_down[i-1]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i] = av_clip_pixel(data_in[i] + sao_offset_val[offset_val], LUMA);
						}else
							data_out[i] = data_in[i];
					}

					if(y==3)	//do last line pixel
					{
						if(((x==0)&&ctu_avail_tl) || ((x!=0)&&ctu_avail_t))
							data_out[0] = data_in[0];
						if(ctu_avail_t)
							memcpy(&data_out[1], &data_in[1], (width-2)*sizeof(reg));
						if(islastcol&&(ctu_avail_r || ctu_avail_t || pic_rightcol))
							data_out[width-1] = data_in[width-1];
						if(!islastcol&&ctu_avail_t)
							data_out[width-1] = data_in[width-1];
					}else
					{
						if(ctu_avail_l)data_out[0] = data_in[0];
						if(ctu_avail_r || pic_rightcol)data_out[width-1] = data_in[width-1];
					}
					
				break;

				default:
					break;
			}
			
			break;
			
		case SAO_BAND:	
			shift = bit_depthY_pic_cmd - 5;
			sao_band_position = subtype_id;
			sao_offset_val[0] = 0;
			sao_offset_val[1] = offset0;
			sao_offset_val[2] = offset1;
			sao_offset_val[3] = offset2;
			sao_offset_val[4] = offset3;
			/*if(offset0&0x20)
				sao_offset_val[1] = -(offset0&0x1f);
			if(offset1&0x20)
				sao_offset_val[2] = -(offset1&0x1f);
			if(offset2&0x20)
				sao_offset_val[3] = -(offset2&0x1f);
			if(offset3&0x20)
				sao_offset_val[4] = -(offset3&0x1f);*/

			for (i = 0; i < 4; i++)
		        sao_band_table[(i + sao_band_position) & 31] = i + 1;

			/*if(ctu_x==0 && x==0)data_out[0] = data_in[0];
			i = (ctu_x==0 && x==0) ? 1 : 0;*/
					
			for (i=0; i < width; i++)
			{
				passidx = (y/8)*(8+1)+(1+x*2+i/8)*passflag;
				if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
				{
					data_out[i] = av_clip_pixel(data_in[i] + sao_offset_val[sao_band_table[data_in[i] >> shift]], LUMA);
				}else
					data_out[i] = data_in[i];
			}
			break;

		case SAO_NOT_APPLIED:
			memcpy(data_out, data_in, width*sizeof(reg));
			break;
			
		default:	//type_id == SAO_NOT_APPLIED
			break;
	}

	return ;
}

void  SaoModule::hevc_sao_calc_luma_lower(int width, reg *data_out, int x, int y)
{
	//to do sao
	char flag;
	int tmp, shift, widthsrc = width;//width, height, 
	int i, diff0, diff1, offset_val, passidx;
	int	type_id, subtype_id, offset0, offset1, offset2, offset3;
	int	edge_idx[] = { 1, 2, 0, 3, 4 };
	reg *data_in, *data_in_up, *data_in_down;

	input 	ctu_x=0, ctu_y=0, picr=0, picb=0;
	input	pic_width_in_luma_samples = 0, pic_height_in_luma_samples = 0;
	
	input	widthr  = picr ? (pic_width_in_luma_samples-ctu_x*ctb_size_filter) : ctb_size_filter;
	input	height = picb ? (pic_height_in_luma_samples-ctu_y*ctb_size_filter) : (ctb_size_filter-4);
	input	lastcol_in_ctu = ((widthr+CTU_IN_MEM_PIXEL_WID-1)/CTU_IN_MEM_PIXEL_WID-1);
	input	islastcol = (x==lastcol_in_ctu)&&((x*CTU_IN_MEM_PIXEL_WID+width)>=widthr);
	
	input	pic_lastrow = picb && (y==(height-1));
	input	pic_rightcol = (picr && (x*CTU_IN_MEM_PIXEL_WID+width)>=widthr);
	input	avail_l, avail_t, avail_tl, avail_r, avail_tr, passflag;
	input	ctu_avail_l, ctu_avail_t, ctu_avail_tl, ctu_avail_r, ctu_avail_tr;

	//head lower must be last col
	pic_rightcol = (sao_curr_state==ST_HEAD_LOWER) ? 0 : pic_rightcol;
	islastcol = (sao_curr_state==ST_HEAD_LOWER) ? 1 : islastcol;
	passflag = (sao_curr_state==ST_HEAD_LOWER) ? 0 : 1;
	
	switch(sao_curr_state)
	{
		case ST_MID_LOWER:	
			avail_l =   0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_l;
			avail_t = 	0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_t;
			avail_tl = 	0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_tl;
			avail_r = 	0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_deblock])->avail_l;
			avail_tr = 	0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_deblock])->avail_tr;
			ctu_avail_l = (!avail_l && (x==0));
			ctu_avail_r = (!avail_r && islastcol);
			ctu_avail_t = (!avail_t && (y==0));
			ctu_avail_tl = ((!avail_tl) && (x==0) && (y==0));
			ctu_avail_tr = ((!avail_tr) && islastcol && (y==0));
			break;
			
		case ST_HEAD_LOWER:
			avail_l =   0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_l;
			ctu_avail_r = (!avail_l && (x==0));
			sao_avail_t_flag.read(ctu_y, &flag);
			ctu_avail_t = (!flag && (y==0));
			avail_tr = 	0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_tr;
			ctu_avail_tr = (!avail_tr && (x==0) && (y==0));
			ctu_avail_l = 0;
			ctu_avail_tl = ctu_avail_t;
			break;
			
		default:
			assert(0);
			break;
	}
	
	
	//get in data
	data_in_up = &sao_data_in_luma[sao_luma_idx_up][1];
	data_in = &sao_data_in_luma[sao_luma_idx_cur][1];
	data_in_down = &sao_data_in_luma[sao_luma_idx_down][1];

	//buff idx circle
	tmp = sao_luma_idx_up;
	sao_luma_idx_up = sao_luma_idx_cur;
	sao_luma_idx_cur = sao_luma_idx_down;
	sao_luma_idx_down = tmp;

	if(sao_curr_state == ST_MID_LOWER)
	{
		type_id = ((SAO_PARAMETER *)&sao_para_luma[ctu_buf_cnt_sao])->type_id;
	    subtype_id = ((SAO_PARAMETER *)&sao_para_luma[ctu_buf_cnt_sao])->subtype_id;
	    offset0 = ((SAO_PARAMETER *)&sao_para_luma[ctu_buf_cnt_sao])->offset0;
	    offset1 = ((SAO_PARAMETER *)&sao_para_luma[ctu_buf_cnt_sao])->offset1;
	    offset2 = ((SAO_PARAMETER *)&sao_para_luma[ctu_buf_cnt_sao])->offset2;
	    offset3 = ((SAO_PARAMETER *)&sao_para_luma[ctu_buf_cnt_sao])->offset3;
	}else
	{
		type_id = ((SAO_PARAMETER *)&sao_para_luma_up)->type_id;
	    subtype_id = ((SAO_PARAMETER *)&sao_para_luma_up)->subtype_id;
	    offset0 = ((SAO_PARAMETER *)&sao_para_luma_up)->offset0;
	    offset1 = ((SAO_PARAMETER *)&sao_para_luma_up)->offset1;
	    offset2 = ((SAO_PARAMETER *)&sao_para_luma_up)->offset2;
	    offset3 = ((SAO_PARAMETER *)&sao_para_luma_up)->offset3;
	}
		
	memset(sao_offset_val, 0, sizeof(sao_offset_val));
	memset(sao_band_table, 0, sizeof(sao_band_table));

	
	switch(type_id)
	{
		case SAO_EDGE:
			
			sao_offset_val[0] = 0;
			sao_offset_val[1] = offset0;
			sao_offset_val[2] = offset1;
			sao_offset_val[3] = offset2;
			sao_offset_val[4] = offset3;

			switch(subtype_id)
			{
				case SAO_EO_HORIZ:
					if(ctu_avail_l)data_out[0] = data_in[0];
					i = (ctu_avail_l) ? 1 : 0;
					if(ctu_avail_r || pic_rightcol)
					{
						data_out[width-1] = data_in[width-1];
						width -= 1;
					}
					
					for (; i < width; i++)
					{
						passidx = (y/8+1)*(8+1)+(1+x*2+i/8)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i], data_in[i-1]);
							diff1 = CMP(data_in[i], data_in[i+1]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i] = av_clip_pixel(data_in[i] + sao_offset_val[offset_val], LUMA);
						}else
							data_out[i] = data_in[i];
					}
					
				break;

				case SAO_EO_VERT:
					if(ctu_avail_t || pic_lastrow)	//直接copy数据
					{
						memcpy(data_out, data_in, width*sizeof(reg));
						break;
					}
					
					for (i = 0; i < width; i++)
					{
						passidx = (y/8+1)*(8+1)+(1+x*2+i/8)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i], data_in_up[i]);
							diff1 = CMP(data_in[i], data_in_down[i]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i] = av_clip_pixel(data_in[i] + sao_offset_val[offset_val], LUMA);
						}else
							data_out[i] = data_in[i];
					}
					
				break;

				case SAO_EO_135D:
					//do all pixel
					for (i=0; i < width; i++)
					{
						passidx = (y/8+1)*(8+1)+(1+x*2+i/8)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i], data_in_up[i-1]);
							diff1 = CMP(data_in[i], data_in_down[i+1]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i] = av_clip_pixel(data_in[i] + sao_offset_val[offset_val], LUMA);
						}else
							data_out[i] = data_in[i];
					}
					
					//first line sao
					if(y==0)
					{
						//top left
						if(((x==0) && ctu_avail_tl) || ((x!=0) && ctu_avail_t))
							data_out[0] = data_in[0];
						//mid  copy data
						if(ctu_avail_t)
							memcpy(&data_out[1], &data_in[1], (width-2)*sizeof(reg));
						//top right
						if(islastcol&&(ctu_avail_r || ctu_avail_t || pic_rightcol))	//no avail tr
							data_out[width-1] = data_in[width-1];
						if(!islastcol&&ctu_avail_t)
							data_out[width-1] = data_in[width-1];
					}else
					{
						if(ctu_avail_l)data_out[0] = data_in[0];
						if(ctu_avail_r  || pic_rightcol)data_out[width-1] = data_in[width-1];
						if(pic_lastrow)
							memcpy(data_out, data_in, width*sizeof(reg));
					}
					
				break;

				case SAO_EO_45D:
					//do all pixel
					for (i=0; i < width; i++)
					{
						passidx = (y/8+1)*(8+1)+(1+x*2+i/8)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i], data_in_up[i+1]);
							diff1 = CMP(data_in[i], data_in_down[i-1]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i] = av_clip_pixel(data_in[i] + sao_offset_val[offset_val], LUMA);
						}else
							data_out[i] = data_in[i];
					}
					
					//first line sao
					if(y==0)
					{
						//top left
						if(((x==0)&&(ctu_avail_l || ctu_avail_t)) || ((x!=0)&&(ctu_avail_t)))
							data_out[0] = data_in[0];
						//mid  copy data
						if(ctu_avail_t)
							memcpy(&data_out[1], &data_in[1], (width-2)*sizeof(reg));
						//top right
						if(islastcol&&(ctu_avail_tr || pic_rightcol))	//no avail tr
							data_out[width-1] = data_in[width-1];
						if(!islastcol&&ctu_avail_t)
							data_out[width-1] = data_in[width-1];
					}else
					{
						if(ctu_avail_l)data_out[0] = data_in[0];
						if(ctu_avail_r || pic_rightcol)data_out[width-1] = data_in[width-1];
						if(pic_lastrow)
							memcpy(data_out, data_in, width*sizeof(reg));
					}

				break;

				default:
					break;
			}
			
			break;
			
		case SAO_BAND:	
			shift = bit_depthY_pic_cmd - 5;
			sao_band_position = subtype_id;
			sao_offset_val[0] = 0;
			sao_offset_val[1] = offset0;
			sao_offset_val[2] = offset1;
			sao_offset_val[3] = offset2;
			sao_offset_val[4] = offset3;
			/*if(offset0&0x20)
				sao_offset_val[1] = -(offset0&0x1f);
			if(offset1&0x20)
				sao_offset_val[2] = -(offset1&0x1f);
			if(offset2&0x20)
				sao_offset_val[3] = -(offset2&0x1f);
			if(offset3&0x20)
				sao_offset_val[4] = -(offset3&0x1f);*/

			for (i = 0; i < 4; i++)
		        sao_band_table[(i + sao_band_position) & 31] = i + 1;

			/*if(ctu_x==0 && x==0)data_out[0] = data_in[0];
			i = (ctu_x==0 && x==0) ? 1 : 0;*/
					
			for (i=0; i < width; i++)
			{
				passidx = (y/8+1)*(8+1)+(1+x*2+i/8)*passflag;
				if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
				{
					data_out[i] = av_clip_pixel(data_in[i] + sao_offset_val[sao_band_table[data_in[i] >> shift]], LUMA);
				}else
					data_out[i] = data_in[i];
			}
			break;

		case SAO_NOT_APPLIED:
			memcpy(data_out, data_in, width*sizeof(reg));
			break;
			
		default:	//type_id == SAO_NOT_APPLIED
			break;
	}
	
	return ;
}

void  SaoModule::hevc_sao_calc_chr_upper(int width, reg *data_out, int x, int y)
{
	//to do sao
	char flag;
	int tmp, shift, widthsrc = width;//width, height, 
	int i, diff0, diff1, offset_val, passidx;
	int type_id, subtype_id, subtype_idV;
	int Uoffset0, Uoffset1, Uoffset2, Uoffset3;
	int Voffset0, Voffset1, Voffset2, Voffset3;
	int edge_idx[] = { 1, 2, 0, 3, 4 };
	
	reg sao_offset_val_u[5] = {0};
	reg sao_offset_val_v[5] = {0};
	reg band_table_u[32] = {0};
	reg band_table_v[32] = {0};
	reg band_position_u, band_position_v;
	reg *data_in, *data_in_up, *data_in_down;

	input 	ctu_x=0, ctu_y=0, picr=0;
	input	pic_width_in_luma_samples = 0;
	
	input	widthr  = picr ? (pic_width_in_luma_samples - ctu_x*ctb_size_filter) : ctb_size_filter;
	input	lastcol_in_ctu = ((widthr+CTU_IN_MEM_PIXEL_WID-1)/CTU_IN_MEM_PIXEL_WID-1);
	input	islastcol = (x==lastcol_in_ctu)&&((x*CTU_IN_MEM_PIXEL_WID+width)>=widthr);
	
	input	pic_rightcol = (picr && (x*CTU_IN_MEM_PIXEL_WID+width)>=widthr);
	input	avail_t, avail_tl, avail_tr, passflag;
	input	ctu_avail_l, ctu_avail_t, ctu_avail_tl, ctu_avail_r, ctu_avail_tr;

	pic_rightcol = (sao_curr_state==ST_HEAD_UPPER) ? 0 : pic_rightcol;
	islastcol = (sao_curr_state==ST_HEAD_UPPER) ? 1 : islastcol;
	passflag = (sao_curr_state==ST_HEAD_UPPER) ? 0 : 1;
	
	switch(sao_curr_state)
	{
		case ST_MID_UPPER:
			//its mean mid upper's bottom left point
			avail_tl = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_tr;
			ctu_avail_tl = (!avail_tl && (y==1) && (x==0));
			avail_tr = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_deblock])->avail_tl;
			ctu_avail_tr = (!avail_tr && (y==1) && islastcol);
			//its mean mid upper's bottom edge
			sao_avail_l_flag.read(ctu_x, &flag);
			ctu_avail_l = (!flag)&&(x==0);
			if(!picr)
			{
				sao_avail_l_flag.read(ctu_x+1, &flag);
				ctu_avail_r = !flag && islastcol;
			}else
				ctu_avail_r = islastcol;
			//its mean mid upper last row
			avail_t = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_t;
			ctu_avail_t = !avail_t && (y==1);
			break;
			
		case ST_HEAD_UPPER:
			sao_avail_l_flag.read(ctu_x, &flag);
			ctu_avail_r = !flag && (x==0);
			sao_avail_t_flag.read(ctu_y, &flag);
			ctu_avail_t = (!flag && (y==1));
			avail_tl = 	0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_tl;
			ctu_avail_tr = (!avail_tl && (y==1) && (x==0));
			ctu_avail_l = 0;
			ctu_avail_tl = ctu_avail_t;
			break;
			
		default:
			assert(0);
			break;
	}


	//get in data
	data_in_up = &sao_data_in_chr[sao_chr_idx_up][2];
	data_in = &sao_data_in_chr[sao_chr_idx_cur][2];
	data_in_down = &sao_data_in_chr[sao_chr_idx_down][2];

	//buff idx circle
	tmp = sao_chr_idx_up;
	sao_chr_idx_up = sao_chr_idx_cur;
	sao_chr_idx_cur = sao_chr_idx_down;
	sao_chr_idx_down = tmp;

	type_id = ((SAO_PARAMETER *)&sao_para_U_up)->type_id;
	subtype_id =  ((SAO_PARAMETER *)&sao_para_U_up)->subtype_id;
	subtype_idV = ((SAO_PARAMETER *)&sao_para_V_up)->subtype_id;
	Uoffset0 = ((SAO_PARAMETER *)&sao_para_U_up)->offset0;
	Uoffset1 = ((SAO_PARAMETER *)&sao_para_U_up)->offset1;
	Uoffset2 = ((SAO_PARAMETER *)&sao_para_U_up)->offset2;
	Uoffset3 = ((SAO_PARAMETER *)&sao_para_U_up)->offset3;
	Voffset0 = ((SAO_PARAMETER *)&sao_para_V_up)->offset0;
	Voffset1 = ((SAO_PARAMETER *)&sao_para_V_up)->offset1;
	Voffset2 = ((SAO_PARAMETER *)&sao_para_V_up)->offset2;
	Voffset3 = ((SAO_PARAMETER *)&sao_para_V_up)->offset3;
	

	memset(sao_offset_val_u, 0, sizeof(sao_offset_val_u));
	memset(sao_offset_val_v, 0, sizeof(sao_offset_val_v));
	memset(band_table_u, 0, sizeof(band_table_u));
	memset(band_table_v, 0, sizeof(band_table_v));
	
	switch(type_id)
	{
		case SAO_EDGE:
			
			sao_offset_val_u[0] = 0;
			sao_offset_val_u[1] = Uoffset0;
			sao_offset_val_u[2] = Uoffset1;
			sao_offset_val_u[3] = Uoffset2;
			sao_offset_val_u[4] = Uoffset3;
			sao_offset_val_v[0] = 0;
			sao_offset_val_v[1] = Voffset0;
			sao_offset_val_v[2] = Voffset1;
			sao_offset_val_v[3] = Voffset2;
			sao_offset_val_v[4] = Voffset3;

			switch(subtype_id)
			{
				case SAO_EO_HORIZ:
					if(ctu_avail_l)
					{
						data_out[0] = data_in[0];
						data_out[1] = data_in[1];
					}
					i = ctu_avail_l ? 1 : 0;
					if(ctu_avail_r || pic_rightcol)
					{
						data_out[width-2] = data_in[width-2];
						data_out[width-1] = data_in[width-1];
						width -= 2;
					}
					
					for (; i < width/2; i++)
					{
						passidx = (y/4)*(8+1)+(1+x*2+i/4)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i*2], data_in[i*2-2]);
							diff1 = CMP(data_in[i*2], data_in[i*2+2]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2] = av_clip_pixel(data_in[i*2] + sao_offset_val_u[offset_val], CHROMA);

							diff0 = CMP(data_in[i*2+1], data_in[i*2-1]);
							diff1 = CMP(data_in[i*2+1], data_in[i*2+3]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2+1] = av_clip_pixel(data_in[i*2+1] + sao_offset_val_v[offset_val], CHROMA);
						}else
						{
							data_out[i*2] = data_in[i*2];
							data_out[i*2+1] = data_in[i*2+1];
						}
					}
										
				break;

				case SAO_EO_VERT:
					if(ctu_avail_t)	//直接copy数据
					{
						memcpy(data_out, data_in, width*sizeof(reg));
						break;
					}
					
					for (i=0; i < width/2; i++)
					{
						passidx = (y/4)*(8+1)+(1+x*2+i/4)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i*2], data_in_up[i*2]);
							diff1 = CMP(data_in[i*2], data_in_down[i*2]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2] = av_clip_pixel(data_in[i*2] + sao_offset_val_u[offset_val], CHROMA);

							diff0 = CMP(data_in[i*2+1], data_in_up[i*2+1]);
							diff1 = CMP(data_in[i*2+1], data_in_down[i*2+1]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2+1] = av_clip_pixel(data_in[i*2+1] + sao_offset_val_v[offset_val], CHROMA);
						}else
						{
							data_out[i*2] = data_in[i*2];
							data_out[i*2+1] = data_in[i*2+1];
						}
					}
				break;

				case SAO_EO_135D:
					for (i=0; i < width/2; i++)
					{
						passidx = (y/4)*(8+1)+(1+x*2+i/4)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i*2], data_in_up[i*2-2]);
							diff1 = CMP(data_in[i*2], data_in_down[i*2+2]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2] = av_clip_pixel(data_in[i*2] + sao_offset_val_u[offset_val], CHROMA);

							diff0 = CMP(data_in[i*2+1], data_in_up[i*2-1]);
							diff1 = CMP(data_in[i*2+1], data_in_down[i*2+3]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2+1] = av_clip_pixel(data_in[i*2+1] + sao_offset_val_v[offset_val], CHROMA);
						}else
						{
							data_out[i*2] = data_in[i*2];
							data_out[i*2+1] = data_in[i*2+1];
						}
					}

					if(y==1)
					{
						if(((x==0)&&(ctu_avail_l || ctu_avail_t)) || ((x!=0)&&ctu_avail_t))
						{
							data_out[0] = data_in[0];
							data_out[1] = data_in[1];
						}
						if(ctu_avail_t)	//COPY
							memcpy(&data_out[2], &data_in[2], (width-4)*sizeof(reg));
						if(islastcol&&(ctu_avail_tr || pic_rightcol))
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}
						if(!islastcol&&ctu_avail_t)
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}
					}else
					{
						if(ctu_avail_l)
						{
							data_out[0] = data_in[0];
							data_out[1] = data_in[1];
						}
						if(ctu_avail_r || pic_rightcol)
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}
					}
					
				break;

				case SAO_EO_45D:
					for (i=0; i < width/2; i++)
					{
						passidx = (y/4)*(8+1)+(1+x*2+i/4)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i*2], data_in_up[i*2+2]);
							diff1 = CMP(data_in[i*2], data_in_down[i*2-2]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2] = av_clip_pixel(data_in[i*2] + sao_offset_val_u[offset_val], CHROMA);

							diff0 = CMP(data_in[i*2+1], data_in_up[i*2+3]);
							diff1 = CMP(data_in[i*2+1], data_in_down[i*2-1]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2+1] = av_clip_pixel(data_in[i*2+1] + sao_offset_val_v[offset_val], CHROMA);
						}else
						{
							data_out[i*2] = data_in[i*2];
							data_out[i*2+1] = data_in[i*2+1];
						}
					}

					if(y==1)
					{
						if(((x==0)&&ctu_avail_tl) || ((x!=0)&&ctu_avail_t))
						{
							data_out[0] = data_in[0];
							data_out[1] = data_in[1];
						}
						if(ctu_avail_t)	//COPY
							memcpy(&data_out[2], &data_in[2], (width-4)*sizeof(reg));
						if(islastcol&&(ctu_avail_r || ctu_avail_t || pic_rightcol))
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}
						if(!islastcol&&ctu_avail_t)
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}
					}else
					{
						if(ctu_avail_l)
						{
							data_out[0] = data_in[0];
							data_out[1] = data_in[1];
						}
						if(ctu_avail_r || pic_rightcol)
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}
					}
					
				break;

				default:
					break;
			}
			
			break;
			
		case SAO_BAND:	
			shift = bit_depthC_pic_cmd - 5;
			band_position_u = subtype_id;
			band_position_v = subtype_idV;

			sao_offset_val_u[0] = 0;
			sao_offset_val_u[1] = Uoffset0;
			sao_offset_val_u[2] = Uoffset1;
			sao_offset_val_u[3] = Uoffset2;
			sao_offset_val_u[4] = Uoffset3;
			
			sao_offset_val_v[0] = 0;
			sao_offset_val_v[1] = Voffset0;
			sao_offset_val_v[2] = Voffset1;
			sao_offset_val_v[3] = Voffset2;
			sao_offset_val_v[4] = Voffset3;

			for (i = 0; i < 4; i++)
				band_table_u[(i + band_position_u) & 31] = i + 1;
			
			for (i = 0; i < 4; i++)
				band_table_v[(i + band_position_v) & 31] = i + 1;

			/*if(ctu_x==0 && x==0)
			{
				data_out[0] = data_in[0];
				data_out[1] = data_in[1];
			}
			i = (ctu_x==0 && x==0) ? 1 : 0;*/

			for (i=0; i < width/2; i++)
			{
				passidx = (y/4)*(8+1)+(1+x*2+i/4)*passflag;
				if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
				{
					data_out[i*2] = av_clip_pixel(data_in[i*2] + sao_offset_val_u[band_table_u[data_in[i*2] >> shift]], CHROMA);
					data_out[i*2+1] = av_clip_pixel(data_in[i*2+1] + sao_offset_val_v[band_table_v[data_in[i*2+1] >> shift]], CHROMA);
				}else
				{
					data_out[i*2] = data_in[i*2];
					data_out[i*2+1] = data_in[i*2+1];
				}
			}
			break;

		case SAO_NOT_APPLIED:
			memcpy(data_out, data_in, width*sizeof(reg));
			break;
			
		default:	//type_id == SAO_NOT_APPLIED
			break;
	}
	
	return ;
}

void  SaoModule::hevc_sao_calc_chr_lower(int width, reg *data_out, int x, int y)
{
	//to do sao
	char flag;
	int tmp, shift, widthsrc = width;//width, height, 
	int i, diff0, diff1, offset_val, passidx;
	int type_id, subtype_id, subtype_idV;
	int Uoffset0, Uoffset1, Uoffset2, Uoffset3;
	int Voffset0, Voffset1, Voffset2, Voffset3;
	int edge_idx[] = { 1, 2, 0, 3, 4 };
	
	reg sao_offset_val_u[5] = {0};
	reg sao_offset_val_v[5] = {0};
	reg band_table_u[32] = {0};
	reg band_table_v[32] = {0};
	reg band_position_u, band_position_v;
	reg *data_in, *data_in_up, *data_in_down;

	input 	ctu_x=0, ctu_y=0, picr=0, picb=0;
	input	pic_width_in_luma_samples = 0, pic_height_in_luma_samples = 0;
	
	input	widthr  = picr ? (pic_width_in_luma_samples-ctu_x*ctb_size_filter) : ctb_size_filter;
	input	height  = picb ? (pic_height_in_luma_samples-ctu_y*ctb_size_filter)/2 : (ctb_size_filter-4)/2;
	input	lastcol_in_ctu = ((widthr+CTU_IN_MEM_PIXEL_WID-1)/CTU_IN_MEM_PIXEL_WID-1);	
	input	islastcol = (x==lastcol_in_ctu)&&((x*CTU_IN_MEM_PIXEL_WID+width)>=widthr);
	
	input	pic_lastrow = picb && (y==(height-1));
	input	pic_rightcol = (picr && (x*CTU_IN_MEM_PIXEL_WID+width)>=widthr);
	input	avail_l, avail_t, avail_tl, avail_r, avail_tr, passflag;
	input	ctu_avail_l, ctu_avail_t, ctu_avail_tl, ctu_avail_r, ctu_avail_tr;

	//head lower must be last col
	pic_rightcol = (sao_curr_state==ST_HEAD_LOWER) ? 0 : pic_rightcol;
	islastcol = (sao_curr_state==ST_HEAD_LOWER) ? 1 : islastcol;
	passflag = (sao_curr_state==ST_HEAD_LOWER) ? 0 : 1;
	
	switch(sao_curr_state)
	{
		case ST_MID_LOWER:	
			avail_l =   0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_l;
			avail_t = 	0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_t;
			avail_tl = 	0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_tl;
			avail_r = 	0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_deblock])->avail_l;
			avail_tr = 	0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_deblock])->avail_tr;
			ctu_avail_l = (!avail_l && (x==0));
			ctu_avail_r = (!avail_r && islastcol);
			ctu_avail_t = (!avail_t && (y==0));
			ctu_avail_tl = ((!avail_tl) && (x==0) && (y==0));
			ctu_avail_tr = ((!avail_tr) && islastcol && (y==0));
			break;
			
		case ST_HEAD_LOWER:
			avail_l =   0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_l;
			ctu_avail_r = (!avail_l && (x==0));
			sao_avail_t_flag.read(ctu_y, &flag);
			ctu_avail_t = (!flag && (y==0));
			avail_tr = 	0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->avail_tr;
			ctu_avail_tr = (!avail_tr && (x==0) && (y==0));
			ctu_avail_l = 0;
			ctu_avail_tl = ctu_avail_t;
			break;
			
		default:
			assert(0);
			break;
	}

	
	//get in data
	data_in_up = &sao_data_in_chr[sao_chr_idx_up][2];
	data_in = &sao_data_in_chr[sao_chr_idx_cur][2];
	data_in_down = &sao_data_in_chr[sao_chr_idx_down][2];

	//buff idx circle
	tmp = sao_chr_idx_up;
	sao_chr_idx_up = sao_chr_idx_cur;
	sao_chr_idx_cur = sao_chr_idx_down;
	sao_chr_idx_down = tmp;

	if(sao_curr_state == ST_MID_LOWER)
	{
		type_id = ((SAO_PARAMETER *)&sao_para_U[ctu_buf_cnt_sao])->type_id;
		subtype_id =  ((SAO_PARAMETER *)&sao_para_U[ctu_buf_cnt_sao])->subtype_id;
		subtype_idV = ((SAO_PARAMETER *)&sao_para_V[ctu_buf_cnt_sao])->subtype_id;
		Uoffset0 = ((SAO_PARAMETER *)&sao_para_U[ctu_buf_cnt_sao])->offset0;
		Uoffset1 = ((SAO_PARAMETER *)&sao_para_U[ctu_buf_cnt_sao])->offset1;
		Uoffset2 = ((SAO_PARAMETER *)&sao_para_U[ctu_buf_cnt_sao])->offset2;
		Uoffset3 = ((SAO_PARAMETER *)&sao_para_U[ctu_buf_cnt_sao])->offset3;
		Voffset0 = ((SAO_PARAMETER *)&sao_para_V[ctu_buf_cnt_sao])->offset0;
		Voffset1 = ((SAO_PARAMETER *)&sao_para_V[ctu_buf_cnt_sao])->offset1;
		Voffset2 = ((SAO_PARAMETER *)&sao_para_V[ctu_buf_cnt_sao])->offset2;
		Voffset3 = ((SAO_PARAMETER *)&sao_para_V[ctu_buf_cnt_sao])->offset3;
	}else
	{
		type_id = ((SAO_PARAMETER *)&sao_para_U_up)->type_id;
		subtype_id =  ((SAO_PARAMETER *)&sao_para_U_up)->subtype_id;
		subtype_idV = ((SAO_PARAMETER *)&sao_para_V_up)->subtype_id;
		Uoffset0 = ((SAO_PARAMETER *)&sao_para_U_up)->offset0;
		Uoffset1 = ((SAO_PARAMETER *)&sao_para_U_up)->offset1;
		Uoffset2 = ((SAO_PARAMETER *)&sao_para_U_up)->offset2;
		Uoffset3 = ((SAO_PARAMETER *)&sao_para_U_up)->offset3;
		Voffset0 = ((SAO_PARAMETER *)&sao_para_V_up)->offset0;
		Voffset1 = ((SAO_PARAMETER *)&sao_para_V_up)->offset1;
		Voffset2 = ((SAO_PARAMETER *)&sao_para_V_up)->offset2;
		Voffset3 = ((SAO_PARAMETER *)&sao_para_V_up)->offset3;
	}

	memset(sao_offset_val_u, 0, sizeof(sao_offset_val_u));
	memset(sao_offset_val_v, 0, sizeof(sao_offset_val_v));
	memset(band_table_u, 0, sizeof(band_table_u));
	memset(band_table_v, 0, sizeof(band_table_v));
	
	switch(type_id)
	{
		case SAO_EDGE:
			
			sao_offset_val_u[0] = 0;
			sao_offset_val_u[1] = Uoffset0;
			sao_offset_val_u[2] = Uoffset1;
			sao_offset_val_u[3] = Uoffset2;
			sao_offset_val_u[4] = Uoffset3;
			sao_offset_val_v[0] = 0;
			sao_offset_val_v[1] = Voffset0;
			sao_offset_val_v[2] = Voffset1;
			sao_offset_val_v[3] = Voffset2;
			sao_offset_val_v[4] = Voffset3;

			switch(subtype_id)
			{
				case SAO_EO_HORIZ:
					if(ctu_avail_l)
					{
						data_out[0] = data_in[0];
						data_out[1] = data_in[1];
					}
					i = ctu_avail_l ? 1 : 0;
					if(ctu_avail_r || pic_rightcol)
					{
						data_out[width-2] = data_in[width-2];
						data_out[width-1] = data_in[width-1];
						width -= 2;
					}
					
					for (; i < width/2; i++)
					{
						passidx = (y/4+1)*(8+1)+(1+x*2+i/4)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i*2], data_in[i*2-2]);
							diff1 = CMP(data_in[i*2], data_in[i*2+2]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2] = av_clip_pixel(data_in[i*2] + sao_offset_val_u[offset_val], CHROMA);

							diff0 = CMP(data_in[i*2+1], data_in[i*2-1]);
							diff1 = CMP(data_in[i*2+1], data_in[i*2+3]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2+1] = av_clip_pixel(data_in[i*2+1] + sao_offset_val_v[offset_val], CHROMA);
						}else
						{
							data_out[i*2] = data_in[i*2];
							data_out[i*2+1] = data_in[i*2+1];
						}
					}
										
				break;

				case SAO_EO_VERT:
					if(ctu_avail_t || pic_lastrow)	//直接copy数据
					{
						memcpy(data_out, data_in, width*sizeof(reg));
						break;
					}
					
					for (i=0; i < width/2; i++)
					{
						passidx = (y/4+1)*(8+1)+(1+x*2+i/4)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i*2], data_in_up[i*2]);
							diff1 = CMP(data_in[i*2], data_in_down[i*2]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2] = av_clip_pixel(data_in[i*2] + sao_offset_val_u[offset_val], CHROMA);

							diff0 = CMP(data_in[i*2+1], data_in_up[i*2+1]);
							diff1 = CMP(data_in[i*2+1], data_in_down[i*2+1]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2+1] = av_clip_pixel(data_in[i*2+1] + sao_offset_val_v[offset_val], CHROMA);
						}else
						{
							data_out[i*2] = data_in[i*2];
							data_out[i*2+1] = data_in[i*2+1];
						}
					}
				break;

				case SAO_EO_135D:
					for (i=0; i < width/2; i++)
					{
						passidx = (y/4+1)*(8+1)+(1+x*2+i/4)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i*2], data_in_up[i*2-2]);
							diff1 = CMP(data_in[i*2], data_in_down[i*2+2]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2] = av_clip_pixel(data_in[i*2] + sao_offset_val_u[offset_val], CHROMA);

							diff0 = CMP(data_in[i*2+1], data_in_up[i*2-1]);
							diff1 = CMP(data_in[i*2+1], data_in_down[i*2+3]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2+1] = av_clip_pixel(data_in[i*2+1] + sao_offset_val_v[offset_val], CHROMA);
						}else
						{
							data_out[i*2] = data_in[i*2];
							data_out[i*2+1] = data_in[i*2+1];
						}
					}

					//first line sao
					if(y==0)
					{
						//top left
						if(((x==0)&&ctu_avail_tl) || ((x!=0)&&ctu_avail_t))
						{
							data_out[0] = data_in[0];
							data_out[1] = data_in[1];
						}
						//mid  copy data
						if(ctu_avail_t)
							memcpy(&data_out[2], &data_in[2], (width-4)*sizeof(reg));
						//top right
						if(islastcol&&(ctu_avail_r || ctu_avail_t || pic_rightcol))	//no avail tr
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}
						if(!islastcol&&ctu_avail_t)
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}
					}else
					{
						if(ctu_avail_l)
						{
							data_out[0] = data_in[0];
							data_out[1] = data_in[1];
						}
						if(ctu_avail_r  || pic_rightcol)
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}	
						if(pic_lastrow)
							memcpy(data_out, data_in, width*sizeof(reg));
					}
					
				break;

				case SAO_EO_45D:
					for (i=0; i < width/2; i++)
					{
						passidx = (y/4+1)*(8+1)+(1+x*2+i/4)*passflag;
						if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
						{
							diff0 = CMP(data_in[i*2], data_in_up[i*2+2]);
							diff1 = CMP(data_in[i*2], data_in_down[i*2-2]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2] = av_clip_pixel(data_in[i*2] + sao_offset_val_u[offset_val], CHROMA);

							diff0 = CMP(data_in[i*2+1], data_in_up[i*2+3]);
							diff1 = CMP(data_in[i*2+1], data_in_down[i*2-1]);
							offset_val = edge_idx[2 + diff0 + diff1];
							data_out[i*2+1] = av_clip_pixel(data_in[i*2+1] + sao_offset_val_v[offset_val], CHROMA);
						}else
						{
							data_out[i*2] = data_in[i*2];
							data_out[i*2+1] = data_in[i*2+1];
						}
					}

					//first line sao
					if(y==0)
					{
						//top left
						if(((x==0)&&(ctu_avail_l || ctu_avail_t)) || ((x!=0)&&ctu_avail_t))
						{
							data_out[0] = data_in[0];
							data_out[1] = data_in[1];
						}
						//mid  copy data
						if(ctu_avail_t)
							memcpy(&data_out[2], &data_in[2], (width-4)*sizeof(reg));
						//top right
						if(islastcol&&(ctu_avail_tr || pic_rightcol))	//no avail tr
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}
						if(!islastcol&&ctu_avail_t)	//no avail tr
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}
					}else
					{
						if(ctu_avail_l)
						{
							data_out[0] = data_in[0];
							data_out[1] = data_in[1];
						}
						if(ctu_avail_r || pic_rightcol)
						{
							data_out[width-2] = data_in[width-2];
							data_out[width-1] = data_in[width-1];
						}	
						if(pic_lastrow)
							memcpy(data_out, data_in, width*sizeof(reg));
					}
					
				break;

				default:
					break;
			}
			
			break;
			
		case SAO_BAND:	
			shift = bit_depthC_pic_cmd - 5;
			band_position_u = subtype_id;
			band_position_v = subtype_idV;

			sao_offset_val_u[0] = 0;
			sao_offset_val_u[1] = Uoffset0;
			sao_offset_val_u[2] = Uoffset1;
			sao_offset_val_u[3] = Uoffset2;
			sao_offset_val_u[4] = Uoffset3;
			
			sao_offset_val_v[0] = 0;
			sao_offset_val_v[1] = Voffset0;
			sao_offset_val_v[2] = Voffset1;
			sao_offset_val_v[3] = Voffset2;
			sao_offset_val_v[4] = Voffset3;

			for (i = 0; i < 4; i++)
				band_table_u[(i + band_position_u) & 31] = i + 1;
			
			for (i = 0; i < 4; i++)
				band_table_v[(i + band_position_v) & 31] = i + 1;

			/*if(ctu_x==0 && x==0)
			{
				data_out[0] = data_in[0];
				data_out[1] = data_in[1];
			}
			i = (ctu_x==0 && x==0) ? 1 : 0;*/

			for (i=0; i < width/2; i++)
			{
				passidx = (y/4+1)*(8+1)+(1+x*2+i/4)*passflag;
				if(!deblock_qt_pcmflag[ctu_buf_cnt_sao][passidx])
				{
					data_out[i*2] = av_clip_pixel(data_in[i*2] + sao_offset_val_u[band_table_u[data_in[i*2] >> shift]], CHROMA);
					data_out[i*2+1] = av_clip_pixel(data_in[i*2+1] + sao_offset_val_v[band_table_v[data_in[i*2+1] >> shift]], CHROMA);
				}else
				{
					data_out[i*2] = data_in[i*2];
					data_out[i*2+1] = data_in[i*2+1];
				}
			}
			break;

		case SAO_NOT_APPLIED:
			memcpy(data_out, data_in, width*sizeof(reg));
			break;
			
		default:	//type_id == SAO_NOT_APPLIED
			break;
	}
	
	return ;
}

void  SaoModule::hevc_sao_rd_mid_lower_luma(int x, int y, int datalen, int savedatapos, int right_rd_en)
{
	input	type_id = ((SAO_PARAMETER *)&sao_para_luma[ctu_buf_cnt_sao])->type_id;
	input	subtype_id = ((SAO_PARAMETER *)&sao_para_luma[ctu_buf_cnt_sao])->subtype_id;
	input	ctu_x = 0, ctu_y = 0, picb = 0;
	input	pic_height_in_luma_samples = 0;
	
	input 	ctbw   = (CTU_MAX_SIZE>>4);
	input	height = picb ? (pic_height_in_luma_samples-ctu_y*ctb_size_filter) : (ctb_size_filter-4);
	input	is_last_line_lower = (y==(height-1));
	input	is_last_line_pic = picb && is_last_line_lower;
	input	in_buff_addr = (ctbw*4);

	switch(type_id)
	{
		case SAO_EDGE:
			switch(subtype_id)
			{
				case SAO_EO_HORIZ:
					if(y == 0)
					{
						sao_luma_idx_up = 0; sao_luma_idx_cur = 1; sao_luma_idx_down = 2;
					}
					
					//read first curr pixel and left right pixel
					//if(!(ctu_x == 0 && x == 0))
					//	hevc_sao_read_col_pixel_luma(ctu_y*ctb_size_filter+y, &sao_data_in_luma[sao_luma_idx_cur][0]);
					hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_cur][1], (in_buff_addr+y*ctbw+x), ctbw,
								1, 0, datalen, bit_depthY_pic_cmd);
					hevc_sao_read_right_pixel_luma(x, y+4, in_buff_addr+(y*ctbw+x)+1, sao_luma_idx_cur, right_rd_en);
					//hevc_sao_write_col_pixel_luma(ctu_y*ctb_size_filter+y, &sao_data_in_luma[sao_luma_idx_cur][savedatapos]);
					break;

				case SAO_EO_VERT:
					if(y == 0)
					{
						sao_luma_idx_up = 0; sao_luma_idx_cur = 1; sao_luma_idx_down = 2;
						
						if(ctu_y)//read up row pixel
							hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_up][1], (in_buff_addr-ctbw)+x, ctbw,
									1, 0, datalen, bit_depthY_pic_cmd);
						//read first curr pixel
						hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_cur][1], in_buff_addr+x, ctbw,
									1, 0, datalen, bit_depthY_pic_cmd);
						//hevc_sao_write_col_pixel_luma(ctu_y*ctb_size_filter, &sao_data_in_luma[sao_luma_idx_cur][savedatapos]);
					}

					if(!is_last_line_pic)
					{
						//read down pixel
						hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_down][1], in_buff_addr+((y+1)*ctbw+x), ctbw,
									1, 0, datalen, bit_depthY_pic_cmd);
						//if(!is_last_line_lower)	//last line at lower not write col data
						//	hevc_sao_write_col_pixel_luma(ctu_y*ctb_size_filter+y+1, &sao_data_in_luma[sao_luma_idx_down][savedatapos]);
					}
					break;
					
				case SAO_EO_135D:
				case SAO_EO_45D:
					if(y == 0)
					{
						sao_luma_idx_up = 0; sao_luma_idx_cur = 1; sao_luma_idx_down = 2;
						
						if(ctu_y)
						{
							//read up row pixel and left right pixel
							sao_data_in_luma[sao_luma_idx_up][0] = sao_upper_bl_point_luma;
							hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_up][1], (in_buff_addr-ctbw)+x, ctbw,
									1, 0, datalen, bit_depthY_pic_cmd);
							hevc_sao_read_right_pixel_luma(x, y+4-1, (in_buff_addr-ctbw)+x+1, sao_luma_idx_up, right_rd_en);
							sao_upper_bl_point_luma = sao_data_in_luma[sao_luma_idx_up][savedatapos];
						}

						//read first curr pixel and left right pixel
						//if(!(ctu_x == 0 && x == 0))
						//	hevc_sao_read_col_pixel_luma(ctu_y*ctb_size_filter, &sao_data_in_luma[sao_luma_idx_cur][0]);
						hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_cur][1], in_buff_addr+x, ctbw,
									1, 0, datalen, bit_depthY_pic_cmd);
						hevc_sao_read_right_pixel_luma(x, y+4, in_buff_addr+x+1, sao_luma_idx_cur, right_rd_en);
						//hevc_sao_write_col_pixel_luma(ctu_y*ctb_size_filter, &sao_data_in_luma[sao_luma_idx_cur][savedatapos]);
					}

					if(!is_last_line_pic)
					{
						if(!is_last_line_lower)
						{
							//read down pixel
							//if(!(ctu_x == 0 && x == 0))
							//	hevc_sao_read_col_pixel_luma(ctu_y*ctb_size_filter+y+1, &sao_data_in_luma[sao_luma_idx_down][0]);
							hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_down][1], in_buff_addr+((y+1)*ctbw+x), ctbw,
										1, 0, datalen, bit_depthY_pic_cmd);
							hevc_sao_read_right_pixel_luma(x, y+4+1, in_buff_addr+((y+1)*ctbw+x)+1, sao_luma_idx_down, right_rd_en);
							//hevc_sao_write_col_pixel_luma(ctu_y*ctb_size_filter+y+1, &sao_data_in_luma[sao_luma_idx_down][savedatapos]);
						}else
						{
							//read down pixel
							sao_data_in_luma[sao_luma_idx_down][0] = sao_lower_br_point_luma;
							hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_down][1], in_buff_addr+((y+1)*ctbw+x), ctbw,
										1, 0, datalen, bit_depthY_pic_cmd);
							hevc_sao_read_right_pixel_luma(x, y+4+1, in_buff_addr+((y+1)*ctbw+x)+1, sao_luma_idx_down, right_rd_en);
							sao_lower_br_point_luma = sao_data_in_luma[sao_luma_idx_down][datalen];
						}
					}
					break;
					
				default:
					assert(0);
					break;
			}
			break;

		case SAO_BAND:
			if(y == 0)
			{
				sao_luma_idx_up = 0; sao_luma_idx_cur = 1; sao_luma_idx_down = 2;
			}
			//read first curr pixel and left right pixel
			hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_cur][1], in_buff_addr+(y*ctbw+x), ctbw,
						1, 0, datalen, bit_depthY_pic_cmd);
			//hevc_sao_write_col_pixel_luma(ctu_y*ctb_size_filter+y, &sao_data_in_luma[sao_luma_idx_cur][savedatapos]);
			break;


		case SAO_NOT_APPLIED:
			if(y == 0)
			{
				sao_luma_idx_up = 0; sao_luma_idx_cur = 1; sao_luma_idx_down = 2;
			}
			//read first curr pixel and left right pixel
			hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_cur][1], in_buff_addr+(y*ctbw+x), ctbw,
						1, 0, datalen, bit_depthY_pic_cmd);
			//hevc_sao_write_col_pixel_luma(ctu_y*ctb_size_filter+y, &sao_data_in_luma[sao_luma_idx_cur][savedatapos]);
			break;

			
		default:
			assert(0);
			break;
	}
	
	return ;
}

void  SaoModule::hevc_sao_rd_mid_upper_luma(int x, int y, int datalen, int savedatapos, int right_rd_en)
{
	reg 	testtmp[2];
	
	input	type_id = ((SAO_PARAMETER *)&sao_para_luma_up)->type_id;
	input	subtype_id = ((SAO_PARAMETER *)&sao_para_luma_up)->subtype_id;
	
	input 	ctu_x=0, ctu_y=0, picr=0, picb=0;
	
	input 	ctbsize  = (ctb_size_filter>>4);
	input 	ctbw   = (CTU_MAX_SIZE>>4);
	input	height = 4;
	input	sao_col_pos = ctu_y*ctb_size_filter-4;
	
	
	switch(type_id)
	{
		case SAO_EDGE:
			switch(subtype_id)
			{
				case SAO_EO_HORIZ:
					if(y == 0)
					{
						sao_luma_idx_up = 0; sao_luma_idx_cur = 1; sao_luma_idx_down = 2;
					}
					
					//read first curr pixel and left right pixel
					//if(!(ctu_x == 0 && x == 0))
					//	hevc_sao_read_col_pixel_luma(sao_col_pos+y, &sao_data_in_luma[sao_luma_idx_cur][0]);
					hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_cur][1], (y*ctbw+x), ctbw,
								1, 0, datalen, bit_depthY_pic_cmd);
					hevc_sao_read_right_pixel_luma(x, y, (y*ctbw+x)+1, sao_luma_idx_cur, right_rd_en);
					//hevc_sao_write_col_pixel_luma(sao_col_pos+y, &sao_data_in_luma[sao_luma_idx_cur][savedatapos]);
					break;

				case SAO_EO_VERT:
					if(y == 0)
					{
						sao_luma_idx_up = 0; sao_luma_idx_cur = 1; sao_luma_idx_down = 2;

						hevc_read_ctu_ram(&sao_y_row_buff, &sao_data_in_luma[sao_luma_idx_up][1], (ctu_x*ctbsize+x), ctbsize,
									1, 0, CTU_IN_MEM_PIXEL_WID, bit_depthY_pic_cmd);
						//read first curr pixel
						hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_cur][1], x, ctbw,
									1, 0, datalen, bit_depthY_pic_cmd);
						//hevc_sao_write_col_pixel_luma(sao_col_pos, &sao_data_in_luma[sao_luma_idx_cur][savedatapos]);
					}

					if(y != (height-1))
					{
						//read down pixel
						hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_down][1], ((y+1)*ctbw+x), ctbw,
									1, 0, datalen, bit_depthY_pic_cmd);
						//hevc_sao_write_col_pixel_luma(sao_col_pos+y+1, &sao_data_in_luma[sao_luma_idx_down][savedatapos]);
					}else
					{
						//read down pixel
						hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_down][1], ((y+1)*ctbw+x), ctbw,
									1, 0, datalen, bit_depthY_pic_cmd);
					}
					break;
					
				case SAO_EO_135D:
				case SAO_EO_45D:
					if(y == 0)
					{
						sao_luma_idx_up = 0; sao_luma_idx_cur = 1; sao_luma_idx_down = 2;
						
						//read up row pixel and left right pixel
						sao_data_in_luma[sao_luma_idx_up][0] = sao_upper_tr_point_luma;
						hevc_read_ctu_ram(&sao_y_row_buff, &sao_data_in_luma[sao_luma_idx_up][1], (ctu_x*ctbsize+x), ctbsize,
										1, 0, datalen, bit_depthY_pic_cmd);
						if(right_rd_en)
						{
							hevc_read_ctu_ram(&sao_y_row_buff, testtmp, (ctu_x*ctbsize+x+1), ctbsize,
										1, 0, 2, bit_depthY_pic_cmd);
							sao_data_in_luma[sao_luma_idx_up][17] = testtmp[0];
						}
						sao_upper_tr_point_luma = sao_data_in_luma[sao_luma_idx_up][datalen];
							
						//read first curr pixel and left right pixel
						//if(!(ctu_x == 0 && x == 0))
						//	hevc_sao_read_col_pixel_luma(sao_col_pos, &sao_data_in_luma[sao_luma_idx_cur][0]);
						hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_cur][1], x, ctbw,
										1, 0, datalen, bit_depthY_pic_cmd);
						hevc_sao_read_right_pixel_luma(x, y, x+1, sao_luma_idx_cur, right_rd_en);
						//hevc_sao_write_col_pixel_luma(sao_col_pos, &sao_data_in_luma[sao_luma_idx_cur][savedatapos]);
					}

					if(y != (height-1))
					{
						//read down pixel
						//if(!(ctu_x == 0 && x == 0))
						//	hevc_sao_read_col_pixel_luma(sao_col_pos+y+1, &sao_data_in_luma[sao_luma_idx_down][0]);
						hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_down][1], ((y+1)*ctbw+x), ctbw,
									1, 0, datalen, bit_depthY_pic_cmd);
						hevc_sao_read_right_pixel_luma(x, y+1, ((y+1)*ctbw+x)+1, sao_luma_idx_down, right_rd_en);
						//hevc_sao_write_col_pixel_luma(sao_col_pos+y+1, &sao_data_in_luma[sao_luma_idx_down][savedatapos]);
					}else
					{
						//read down pixel
						sao_data_in_luma[sao_luma_idx_down][0] = sao_lower_tr_point_luma;
						hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_down][1], ((y+1)*ctbw+x), ctbw,
									1, 0, datalen, bit_depthY_pic_cmd);
						hevc_sao_read_right_pixel_luma(x, y+1, ((y+1)*ctbw+x)+1, sao_luma_idx_down, right_rd_en);
						sao_lower_tr_point_luma = sao_data_in_luma[sao_luma_idx_down][datalen];
					}
					break;

				default:
					assert(0);
					break;
			}
			break;

		case SAO_BAND:
			if(y == 0)
			{
				sao_luma_idx_up = 0; sao_luma_idx_cur = 1; sao_luma_idx_down = 2;
			}
			//read first curr pixel and left right pixel
			hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_cur][1], (y*ctbw+x), ctbw,
						1, 0, datalen, bit_depthY_pic_cmd);
			//hevc_sao_write_col_pixel_luma(sao_col_pos+y, &sao_data_in_luma[sao_luma_idx_cur][savedatapos]);
			break;


		case SAO_NOT_APPLIED:
			if(y == 0)
			{
				sao_luma_idx_up = 0; sao_luma_idx_cur = 1; sao_luma_idx_down = 2;
			}
			//read first curr pixel and left right pixel
			hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], &sao_data_in_luma[sao_luma_idx_cur][1], (y*ctbw+x), ctbw,
						1, 0, datalen, bit_depthY_pic_cmd);
			//hevc_sao_write_col_pixel_luma(sao_col_pos+y, &sao_data_in_luma[sao_luma_idx_cur][savedatapos]);
			break;

			
		default:
			assert(0);
			break;
	}

	return ;
}

void  SaoModule::hevc_sao_rd_mid_lower_chr(int x, int y, int datalen, int savedatapos, int right_rd_en)
{
	input	type_id = ((SAO_PARAMETER *)&sao_para_U[ctu_buf_cnt_sao])->type_id;
	input	subtype_id = ((SAO_PARAMETER *)&sao_para_U[ctu_buf_cnt_sao])->subtype_id;
	input	ctu_x  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_x;
	input	ctu_y  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_y;
	input   picb  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_b;
	input	pic_height_in_luma_samples = 0;
	input 	ctbw   = (CTU_MAX_SIZE>>4);
	input	height = picb ? (pic_height_in_luma_samples-ctu_y*ctb_size_filter)/2 : (ctb_size_filter-4)/2;
	input	is_last_line_lower = (y==(height-1));
	input	is_last_line_pic = picb && is_last_line_lower;
	input	in_buff_addr = (ctbw*2);
	input	ctb_size_chr = ctb_size_filter/2;
	
	switch(type_id)
	{
		case SAO_EDGE:
			switch(subtype_id)
			{
				case SAO_EO_HORIZ:
					if(y == 0)
					{
						sao_chr_idx_up = 0; sao_chr_idx_cur = 1; sao_chr_idx_down = 2;
					}
					
					//read first curr pixel and left right pixel
					//if(!(ctu_x == 0 && x == 0))
					//	hevc_sao_read_col_pixel_chr(ctu_y*ctb_size_chr+y, &sao_data_in_chr[sao_chr_idx_cur][0]);
					hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_cur][2], (in_buff_addr+y*ctbw+x), ctbw,
								1, 0, datalen, bit_depthC_pic_cmd);
					hevc_sao_read_right_pixel_chr(x, y+2, in_buff_addr+(y*ctbw+x)+1, sao_chr_idx_cur, right_rd_en);
					//hevc_sao_write_col_pixel_chr(ctu_y*ctb_size_chr+y, &sao_data_in_chr[sao_chr_idx_cur][savedatapos]);
					break;

				case SAO_EO_VERT:
					if(y == 0)
					{
						sao_chr_idx_up = 0; sao_chr_idx_cur = 1; sao_chr_idx_down = 2;
						
						if(ctu_y)//read up row pixel
							hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_up][2], (in_buff_addr-ctbw)+x, ctbw,
									1, 0, datalen, bit_depthC_pic_cmd);
						//read first curr pixel
						hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_cur][2], in_buff_addr+x, ctbw,
									1, 0, datalen, bit_depthC_pic_cmd);
						//hevc_sao_write_col_pixel_chr(ctu_y*ctb_size_chr, &sao_data_in_chr[sao_chr_idx_cur][savedatapos]);
					}

					if(!is_last_line_pic)
					{
						//read down pixel
						hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_down][2], in_buff_addr+((y+1)*ctbw+x), ctbw,
									1, 0, datalen, bit_depthC_pic_cmd);
						//if(!is_last_line_lower)
						//	hevc_sao_write_col_pixel_chr(ctu_y*ctb_size_chr+y+1, &sao_data_in_chr[sao_chr_idx_down][savedatapos]);
					}
					break;
					
				case SAO_EO_135D:
				case SAO_EO_45D:
					if(y == 0)
					{
						sao_chr_idx_up = 0; sao_chr_idx_cur = 1; sao_chr_idx_down = 2;
						
						if(ctu_y)
						{
							//read up row pixel and left right pixel
							sao_data_in_chr[sao_chr_idx_up][0] = sao_upper_bl_point_chr[0];
							sao_data_in_chr[sao_chr_idx_up][1] = sao_upper_bl_point_chr[1];
							hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_up][2], (in_buff_addr-ctbw)+x, ctbw,
									1, 0, datalen, bit_depthC_pic_cmd);
							hevc_sao_read_right_pixel_chr(x, y+2-1, (in_buff_addr-ctbw)+x+1, sao_chr_idx_up, right_rd_en);
							sao_upper_bl_point_chr[0] = sao_data_in_chr[sao_chr_idx_up][savedatapos];
							sao_upper_bl_point_chr[1] = sao_data_in_chr[sao_chr_idx_up][savedatapos+1];
						}

						//read first curr pixel and left right pixel
						//if(!(ctu_x == 0 && x == 0))
						//	hevc_sao_read_col_pixel_chr(ctu_y*ctb_size_chr, &sao_data_in_chr[sao_chr_idx_cur][0]);
						hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_cur][2], in_buff_addr+x, ctbw,
									1, 0, datalen, bit_depthC_pic_cmd);
						hevc_sao_read_right_pixel_chr(x, y+2, in_buff_addr+x+1, sao_chr_idx_cur, right_rd_en);
						//hevc_sao_write_col_pixel_chr(ctu_y*ctb_size_chr, &sao_data_in_chr[sao_chr_idx_cur][savedatapos]);
					}

					if(!is_last_line_pic)
					{
						if(!is_last_line_lower)
						{
							//read down pixel
							//if(!(ctu_x == 0 && x == 0))
							//	hevc_sao_read_col_pixel_chr(ctu_y*ctb_size_chr+y+1, &sao_data_in_chr[sao_chr_idx_down][0]);
							hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_down][2], in_buff_addr+((y+1)*ctbw+x), ctbw,
										1, 0, datalen, bit_depthC_pic_cmd);
							hevc_sao_read_right_pixel_chr(x, y+2+1, in_buff_addr+((y+1)*ctbw+x)+1, sao_chr_idx_down, right_rd_en);
							//hevc_sao_write_col_pixel_chr(ctu_y*ctb_size_chr+y+1, &sao_data_in_chr[sao_chr_idx_down][savedatapos]);
						}else
						{
							//read down pixel
							sao_data_in_chr[sao_chr_idx_down][0] = sao_lower_br_point_chr[0];
							sao_data_in_chr[sao_chr_idx_down][1] = sao_lower_br_point_chr[1];
							hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_down][2], in_buff_addr+((y+1)*ctbw+x), ctbw,
										1, 0, datalen, bit_depthC_pic_cmd);
							hevc_sao_read_right_pixel_chr(x, y+2+1, in_buff_addr+((y+1)*ctbw+x)+1, sao_chr_idx_down, right_rd_en);
							sao_lower_br_point_chr[0] = sao_data_in_chr[sao_chr_idx_down][datalen];
							sao_lower_br_point_chr[1] = sao_data_in_chr[sao_chr_idx_down][datalen+1];
						}
					}
					break;
					
				default:
					assert(0);
					break;
			}
			break;

		case SAO_BAND:
			if(y == 0)
			{
				sao_chr_idx_up = 0; sao_chr_idx_cur = 1; sao_chr_idx_down = 2;
			}
			//read first curr pixel and left right pixel
			hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_cur][2], in_buff_addr+(y*ctbw+x), ctbw,
						1, 0, datalen, bit_depthC_pic_cmd);
			//hevc_sao_write_col_pixel_chr(ctu_y*ctb_size_chr+y, &sao_data_in_chr[sao_chr_idx_cur][savedatapos]);
			break;


		case SAO_NOT_APPLIED:
			if(y == 0)
			{
				sao_chr_idx_up = 0; sao_chr_idx_cur = 1; sao_chr_idx_down = 2;
			}
			//read first curr pixel and left right pixel
			hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_cur][2], in_buff_addr+(y*ctbw+x), ctbw,
						1, 0, datalen, bit_depthC_pic_cmd);
			//hevc_sao_write_col_pixel_chr(ctu_y*ctb_size_chr+y, &sao_data_in_chr[sao_chr_idx_cur][savedatapos]);
			break;

			
		default:
			assert(0);
			break;
	}

	return ;
}

void  SaoModule::hevc_sao_rd_mid_upper_chr(int x, int y, int datalen, int savedatapos, int right_rd_en)
{
	input	type_id = ((SAO_PARAMETER *)&sao_para_U_up)->type_id;
	input	subtype_id = ((SAO_PARAMETER *)&sao_para_U_up)->subtype_id;
	input	ctu_x  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_x;
	input	ctu_y  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_y;
	input   picb  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_b;
	input 	ctbsize  = ctb_size_filter>>4;
	input 	ctbw   = (CTU_MAX_SIZE>>4);
	input	height = 2;
	input	sao_col_pos = ((ctu_y-1)*ctb_size_filter+ctb_size_filter-4)/2;
	
	
	switch(type_id)
	{
		case SAO_EDGE:
			switch(subtype_id)
			{
				case SAO_EO_HORIZ:
					if(y == 0)
					{
						sao_chr_idx_up = 0; sao_chr_idx_cur = 1; sao_chr_idx_down = 2;
					}
					
					//read first curr pixel and left right pixel
					//if(!(ctu_x == 0 && x == 0))
					//	hevc_sao_read_col_pixel_chr(sao_col_pos+y, &sao_data_in_chr[sao_chr_idx_cur][0]);
					hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_cur][2], (y*ctbw+x), ctbw,
								1, 0, datalen, bit_depthC_pic_cmd);
					hevc_sao_read_right_pixel_chr(x, y, (y*ctbw+x)+1, sao_chr_idx_cur, right_rd_en);
					//hevc_sao_write_col_pixel_chr(sao_col_pos+y, &sao_data_in_chr[sao_chr_idx_cur][savedatapos]);
					break;

				case SAO_EO_VERT:
					if(y == 0)
					{
						sao_chr_idx_up = 0; sao_chr_idx_cur = 1; sao_chr_idx_down = 2;

						hevc_read_ctu_ram(&sao_uv_row_buff, &sao_data_in_chr[sao_chr_idx_up][2], (ctu_x*ctbsize+x), ctbsize,
									1, 0, CTU_IN_MEM_PIXEL_WID, bit_depthC_pic_cmd);
						//read first curr pixel
						hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_cur][2], x, ctbw,
									1, 0, datalen, bit_depthC_pic_cmd);
						//hevc_sao_write_col_pixel_chr(sao_col_pos, &sao_data_in_chr[sao_chr_idx_cur][savedatapos]);
					}

					if(y != (height-1))
					{
						//read down pixel
						hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_down][2], ((y+1)*ctbw+x), ctbw,
									1, 0, datalen, bit_depthC_pic_cmd);
						//hevc_sao_write_col_pixel_chr(sao_col_pos+y+1, &sao_data_in_chr[sao_chr_idx_down][savedatapos]);
					}else
					{
						//read down pixel
						hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_down][2], ((y+1)*ctbw+x), ctbw,
									1, 0, datalen, bit_depthC_pic_cmd);
					}
					break;
					
				case SAO_EO_135D:
				case SAO_EO_45D:
					if(y == 0)
					{
						sao_chr_idx_up = 0; sao_chr_idx_cur = 1; sao_chr_idx_down = 2;
						
						//read up row pixel and left right pixel
						sao_data_in_chr[sao_chr_idx_up][0] = sao_upper_tr_point_chr[0];
						sao_data_in_chr[sao_chr_idx_up][1] = sao_upper_tr_point_chr[1];
						hevc_read_ctu_ram(&sao_uv_row_buff, &sao_data_in_chr[sao_chr_idx_up][2], (ctu_x*ctbsize+x), ctbsize,
									1, 0, datalen, bit_depthC_pic_cmd);
						if(right_rd_en)
						{
							hevc_read_ctu_ram(&sao_uv_row_buff, &sao_data_in_chr[sao_chr_idx_up][18], (ctu_x*ctbsize+x+1), ctbsize,
										1, 0, 2, bit_depthC_pic_cmd);
						}
						sao_upper_tr_point_chr[0] = sao_data_in_chr[sao_chr_idx_up][datalen];
						sao_upper_tr_point_chr[1] = sao_data_in_chr[sao_chr_idx_up][datalen+1];;
						
						//read first curr pixel and left right pixel
						//if(!(ctu_x == 0 && x == 0))
						//	hevc_sao_read_col_pixel_chr(sao_col_pos, &sao_data_in_chr[sao_chr_idx_cur][0]);
						hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_cur][2], x, ctbw,
									1, 0, datalen, bit_depthC_pic_cmd);
						hevc_sao_read_right_pixel_chr(x, y, x+1, sao_chr_idx_cur, right_rd_en);
						//hevc_sao_write_col_pixel_chr(sao_col_pos, &sao_data_in_chr[sao_chr_idx_cur][savedatapos]);
					}

					if(y != (height-1))
					{
						//read down pixel
						//if(!(ctu_x == 0 && x == 0))
						//	hevc_sao_read_col_pixel_chr(sao_col_pos+y+1, &sao_data_in_chr[sao_chr_idx_down][0]);
						hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_down][2], ((y+1)*ctbw+x), ctbw,
									1, 0, datalen, bit_depthC_pic_cmd);
						hevc_sao_read_right_pixel_chr(x, y+1, ((y+1)*ctbw+x)+1, sao_chr_idx_down, right_rd_en);
						//hevc_sao_write_col_pixel_chr(sao_col_pos+y+1, &sao_data_in_chr[sao_chr_idx_down][savedatapos]);
					}else
					{
						//read down pixel
						sao_data_in_chr[sao_chr_idx_down][0] = sao_lower_tr_point_chr[0];
						sao_data_in_chr[sao_chr_idx_down][1] = sao_lower_tr_point_chr[1];
						hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_down][2], ((y+1)*ctbw+x), ctbw,
									1, 0, datalen, bit_depthC_pic_cmd);
						hevc_sao_read_right_pixel_chr(x, y+1, ((y+1)*ctbw+x)+1, sao_chr_idx_down, right_rd_en);
						sao_lower_tr_point_chr[0] = sao_data_in_chr[sao_chr_idx_down][datalen];
						sao_lower_tr_point_chr[1] = sao_data_in_chr[sao_chr_idx_down][datalen+1];
					}
					break;

				default:
					assert(0);
					break;
			}
			break;

		case SAO_BAND:
			if(y == 0)
			{
				sao_chr_idx_up = 0; sao_chr_idx_cur = 1; sao_chr_idx_down = 2;
			}
			//read first curr pixel and left right pixel
			hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_cur][2], (y*ctbw+x), ctbw,
						1, 0, datalen, bit_depthC_pic_cmd);
			//hevc_sao_write_col_pixel_chr(sao_col_pos+y, &sao_data_in_chr[sao_chr_idx_cur][savedatapos]);
			break;


		case SAO_NOT_APPLIED:
			if(y == 0)
			{
				sao_chr_idx_up = 0; sao_chr_idx_cur = 1; sao_chr_idx_down = 2;
			}
			//read first curr pixel and left right pixel
			hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[sao_chr_idx_cur][2], (y*ctbw+x), ctbw,
						1, 0, datalen, bit_depthC_pic_cmd);
			//hevc_sao_write_col_pixel_chr(sao_col_pos+y, &sao_data_in_chr[sao_chr_idx_cur][savedatapos]);
			break;

			
		default:
			assert(0);
			break;
	}

	return ;
}

void  SaoModule::hevc_sao_read_right_pixel_luma(int x, int y, int ramstart, int idx, int right_rd_en)
{
	reg		testtmp[2];
	input   picr  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_r;
	input 	ctbw   = (CTU_MAX_SIZE>>4);
	input	width  = ctb_size_filter;
	
	if(right_rd_en)
	{
		if(x == (width/CTU_IN_MEM_PIXEL_WID-1))
			sao_data_ctu_right_col_luma[ctu_buf_cnt_sao].read(y, (char *)&sao_data_in_luma[idx][17]);
		else
		{
			hevc_read_ctu_ram(&ctu_y_in_buff[ctu_buf_cnt_sao], testtmp, ramstart, ctbw,
					1, 0, 2, bit_depthY_pic_cmd);
			sao_data_in_luma[idx][17] = testtmp[0];
		}
	}
}

void  SaoModule::hevc_sao_read_right_pixel_chr(int x, int y, int ramstart, int idx, int right_rd_en)
{
	input   tiler  = 0;
	input 	ctbw   = (CTU_MAX_SIZE>>4);
	input	width  = ctb_size_filter;
	
	if(right_rd_en)
	{
		if(x == (width/CTU_IN_MEM_PIXEL_WID-1))
		{
			sao_data_ctu_right_col_chr[ctu_buf_cnt_sao].read(y, (char *)&sao_data_in_chr[idx][18]);
		}else
		{
			hevc_read_ctu_ram(&ctu_uv_in_buff[ctu_buf_cnt_sao], &sao_data_in_chr[idx][18], ramstart, ctbw,
					1, 0, 2, bit_depthC_pic_cmd);
		}
	}
}

void  SaoModule::hevc_ctu_out_pack(unsigned char *dst, reg *src, int pixelnum, int bit_depth)
{
	int i, totbits, bitpos, bitsleft;
	assert((bit_depth == 9) || (bit_depth == 10));
	
	if((bit_depth == 9) || (bit_depth == 10))
	{
		bitpos = 0;
		bitsleft = 10;			//left bits
		totbits = pixelnum*10;	//all bits
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
}

void  SaoModule::hevc_ctu_out_unpack(unsigned char *src, unsigned short *dst, int pixelnum, int startbit, int bit_depth)
{
	int i, bitpos;
	assert((bit_depth == 8) || (bit_depth == 9) || (bit_depth == 10));

	if(bit_depth == 8)
	{
		for(i=0;i<pixelnum;i++)
			dst[i] = src[i];
	}else if((bit_depth == 9) || (bit_depth == 10))
	{
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
}

void  SaoModule::hevc_sao_wr_lower_luma(int x, int y, int datalen)
{
	int i, totbytes, leftbits;
	unsigned char tmp[20]={0};
	unsigned char tmp1[16]={0};
	unsigned char tmp2[16]={0};
	
	input	ctu_x  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_x;
	input	ctu_y  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_y;
	input   tilel  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->tile_l && ctu_x;
	input   picb   = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_b;
	input 	ctbw   = (ctb_size_filter==64) ? (4*HEVC_MAX_BIT_DEP/8+1) : (4*HEVC_MAX_BIT_DEP*2/8+1);	//ctbw=6 or 11
	input	start_out_ram, curr_pos_byte;	//curr_pos_out_buff的单位时32bit
	input	ctu_st_pos = (ctu_x-ctu_num_in_outbuff);
	
    //calc start_out_ram and curr_pos_byte
	if(bit_depthY_pic_cmd == 8)
	{
		start_out_ram = ctu_y ? (ctbw*4) : 0;
		start_out_ram += (ctu_num_in_outbuff*ctb_size_filter/16+x);
		curr_pos_byte = 0;
	}else
		assert(0);
	
	//output pixel to ctu_y_out_buff
	if(bit_depthY_pic_cmd == 8)
	{
		if(sao_curr_state == ST_HEAD_LOWER)
		{
			for(i=0;i<datalen;i++)
				tmp[16-datalen+i] = sao_data_out_luma[i];
			
			ctu_y_out_buff[ctu_out_buff_cnt].write(((ctu_y!=0)*4*ctbw+y*ctbw), (char *)tmp);
		}else
		{
			if(sao_out_tilel_flag[ctu_out_buff_cnt] || tilel)
				start_out_ram += 1;
			
			for(i=0;i<datalen;i++)
				tmp[i] = sao_data_out_luma[i];

			ctu_y_out_buff[ctu_out_buff_cnt].write((start_out_ram+y*ctbw), (char *)tmp);
		}
	}else
		assert(0);
		
	return ;
}

void  SaoModule::hevc_sao_wr_upper_luma(int x, int y, int datalen)
{
	int i, totbytes, leftbits;
	unsigned char tmp[20]={0};
	unsigned char tmp1[16]={0};
	unsigned char tmp2[16]={0};
	
	input   ctu_y  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_y;
	input   ctu_x  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_x;
	input   picb   = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_b;	
	input   picr   = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_r;	
	input 	ctbw   = (ctb_size_filter==64) ? (4*HEVC_MAX_BIT_DEP/8+1) : (4*HEVC_MAX_BIT_DEP*2/8+1);	//ctbw=6 or 11
	input	start_out_ram, curr_pos_byte;
	input	ctu_st_pos = (ctu_x-ctu_num_in_outbuff);


	//calc start_out_ram and curr_pos_byte
	if(bit_depthY_pic_cmd == 8)
	{
		start_out_ram = (ctu_num_in_outbuff*ctb_size_filter/16+x);
		curr_pos_byte = 0;
	}else
		assert(0);


	if(bit_depthY_pic_cmd == 8)
	{
		for(i=0;i<datalen;i++)
			tmp[i] = sao_data_out_luma[i];

		ctu_y_out_buff[ctu_out_buff_cnt].write((start_out_ram+y*ctbw), (char *)tmp);
	}else
		assert(0);
}

void  SaoModule::hevc_sao_wr_lower_chr(int x, int y, int datalen)
{
	int i, totbytes, leftbits;
	unsigned char tmp[20]={0};
	unsigned char tmp1[16]={0};
	unsigned char tmp2[16]={0};
	
	input	ctu_x  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_x;
	input	ctu_y  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_y;
	input   tilel  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->tile_l && ctu_x;
	input   picb   = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_b;
	input 	ctbw   = (ctb_size_filter==64) ? (4*HEVC_MAX_BIT_DEP/8+1) : (4*HEVC_MAX_BIT_DEP*2/8+1);	//ctbw=6 or 11
	input	start_out_ram, curr_pos_byte;	//curr_pos_out_buff的单位时32bit
	input	ctu_st_pos = (ctu_x-ctu_num_in_outbuff);
	
	
	//calc start_out_ram and curr_pos_byte
	if(bit_depthC_pic_cmd == 8)
	{
		start_out_ram = ctu_y ? (ctbw*2) : 0;
		start_out_ram += (ctu_num_in_outbuff*ctb_size_filter/16+x);
		curr_pos_byte = 0;
	}else
		assert(0);


	if(bit_depthC_pic_cmd == 8)
	{
		for(i=0;i<datalen;i++)
			tmp[i] = sao_data_out_chr[i];

		ctu_uv_out_buff[ctu_out_buff_cnt].write((start_out_ram+y*ctbw), (char *)tmp);
	}else	
		assert(0);
	
	return ;
}

void  SaoModule::hevc_sao_wr_upper_chr(int x, int y, int datalen)
{
	int i, totbytes, leftbits;
	unsigned char tmp[20]={0};
	unsigned char tmp1[16]={0};
	unsigned char tmp2[16]={0};

	input	ctu_x  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_x;
	input	ctu_y  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_y;
	input   tilel  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->tile_l && ctu_x;
	input   picb   = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_b;	
	input 	ctbw   = (ctb_size_filter==64) ? (4*HEVC_MAX_BIT_DEP/8+1) : (4*HEVC_MAX_BIT_DEP*2/8+1);	//ctbw=6 or 11
	input	start_out_ram, curr_pos_byte;
	input	ctu_st_pos = (ctu_x-ctu_num_in_outbuff);
	
	
	//calc start_out_ram and curr_pos_byte
	if(bit_depthC_pic_cmd == 8)
	{
		start_out_ram = (ctu_num_in_outbuff*ctb_size_filter/16+x);
		curr_pos_byte = 0;
	}else
		assert(0);


	if(bit_depthC_pic_cmd == 8)
	{
		if(sao_out_tilel_flag[ctu_out_buff_cnt] || tilel)
			start_out_ram += 1;
		
		for(i=0;i<datalen;i++)
			tmp[i] = sao_data_out_chr[i];

		ctu_uv_out_buff[ctu_out_buff_cnt].write((start_out_ram+y*ctbw), (char *)tmp);
	}else
		assert(0);
	
	return ;
}

void  SaoModule::hevc_sao_calc_mid_upper()
{
	int		x=0, y=0, datalen;
	
	input	ctu_y = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_y;
	input	ctu_x = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_x;
	input   picr = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_r;
	input	pic_width_in_luma_samples = 0;
	input	width  = picr ? (pic_width_in_luma_samples - ctu_x*ctb_size_filter) : ctb_size_filter;
	input	widthr = picr ? ((width+16-1)/16-1)*16 : width;	// last width of picr and tiler do lonely.
	input	height = 4;
	

	//luma mid upper sao calc
	for (x = 0; x < widthr/CTU_IN_MEM_PIXEL_WID; x++)
	{
		for (y = 0; y < height; y++)
		{
			//horz read pixel
			hevc_sao_rd_mid_upper_luma(x, y, CTU_IN_MEM_PIXEL_WID, CTU_IN_MEM_PIXEL_WID, 1);
			
			//calcing, 16 pixel every two cycle in hardware, cmodel calc one time.
			hevc_sao_calc_luma_upper(CTU_IN_MEM_PIXEL_WID, sao_data_out_luma, x, y);
			
			//sao output write ctu out ram; i 表示在一个bitwid里占用的bits
			hevc_sao_wr_upper_luma(x, y, CTU_IN_MEM_PIXEL_WID);
		}
	}

	//luma, if tiler is true, so do last col sao, col width is (CTU_IN_MEM_PIXEL_WID-5)
	if(picr)
	{
		datalen = width-widthr;
		for (y = 0; y < height; y++)
		{
			//horz read pixel
			hevc_sao_rd_mid_upper_luma(x, y, datalen, datalen, 0);	//lenth is not 2 align, so read more 1 byte
			
			//calcing, 16 pixel every two cycle in hardware, cmodel calc one time.
			hevc_sao_calc_luma_upper(datalen, sao_data_out_luma, x, y);
			
			//sao output write ctu out ram; i 表示在一个bitwid里占用的bits
			hevc_sao_wr_upper_luma(x, y, datalen);
		}
	}
	

	//chroma mid upper sao calc
	for (x = 0; x < widthr/CTU_IN_MEM_PIXEL_WID; x++)
	{
		for (y = 0; y < height/2; y++)
		{
			//horz read pixel
			hevc_sao_rd_mid_upper_chr(x, y, CTU_IN_MEM_PIXEL_WID, CTU_IN_MEM_PIXEL_WID, 1);
			
			//calcing, 16 pixel every two cycle in hardware, cmodel calc one time.
			hevc_sao_calc_chr_upper(CTU_IN_MEM_PIXEL_WID, sao_data_out_chr, x, y);
			
			//sao output write ctu out ram; i 表示在一个bitwid里占用的bits
			hevc_sao_wr_upper_chr(x, y, CTU_IN_MEM_PIXEL_WID);
		}
	}

	//chroma, if tiler is true, so do last col sao, col width is (CTU_IN_MEM_PIXEL_WID-10)
	if(picr)
	{
		datalen = width-widthr;
		for (y = 0; y < height/2; y++)
		{
			//horz read pixel
			hevc_sao_rd_mid_upper_chr(x, y, datalen, datalen, 0);
			
			//calcing, 16 pixel every two cycle in hardware, cmodel calc one time.
			hevc_sao_calc_chr_upper(datalen, sao_data_out_chr, x, y);
			
			//sao output write ctu out ram; i 表示在一个bitwid里占用的bits
			hevc_sao_wr_upper_chr(x, y, datalen);
		}
	}
	
	return ;
}

void  SaoModule::hevc_sao_calc_mid_lower()
{
	int		x=0, y=0, datalen;

	input	ctu_x = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_x;
	input	ctu_y = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_y;
	input   picb  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_b;
	input   picr  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_r;
	input	pic_width_in_luma_samples = 0, pic_height_in_luma_samples = 0;
	input	width  = picr ? (pic_width_in_luma_samples - ctu_x*ctb_size_filter) : ctb_size_filter;
	input	height = picb ? (pic_height_in_luma_samples - ctu_y*ctb_size_filter) : (ctb_size_filter-4);
	

	//luma mid lower sao calc
	for (x = 0; x < width/CTU_IN_MEM_PIXEL_WID; x++)
	{
		for (y = 0; y < height; y++)
		{
			//horz read pixel
			hevc_sao_rd_mid_lower_luma(x, y, CTU_IN_MEM_PIXEL_WID, CTU_IN_MEM_PIXEL_WID, 1);
			
			//calcing, 16 pixel every two cycle in hardware, cmodel calc one time.
			hevc_sao_calc_luma_lower(CTU_IN_MEM_PIXEL_WID, sao_data_out_luma, x, y);
			
			//sao output write ctu out ram; i 表示在一个bitwid里占用的bits
			hevc_sao_wr_lower_luma(x, y, CTU_IN_MEM_PIXEL_WID);
		}
	}
	
	//luma, if tiler is true, so do last col sao, col width is (CTU_IN_MEM_PIXEL_WID-5)
	if(picr && (width&(CTU_IN_MEM_PIXEL_WID-1)))
	{
		datalen = width&(CTU_IN_MEM_PIXEL_WID-1);
		for (y = 0; y < height; y++)
		{
			//horz read pixel
			hevc_sao_rd_mid_lower_luma(x, y, datalen, datalen, 0);	//lenth is not 2 align, so read more 1 byte
			
			//calcing, 16 pixel every two cycle in hardware, cmodel calc one time.
			hevc_sao_calc_luma_lower(datalen, sao_data_out_luma, x, y);
			
			//sao output write ctu out ram; i 表示在一个bitwid里占用的bits
			hevc_sao_wr_lower_luma(x, y, datalen);
		}
	}

	
	//chroma mid lower sao calc
	for (x = 0; x < width/CTU_IN_MEM_PIXEL_WID; x++)
	{
		for (y = 0; y < height/2; y++)
		{
			//horz read pixel
			hevc_sao_rd_mid_lower_chr(x, y, CTU_IN_MEM_PIXEL_WID, CTU_IN_MEM_PIXEL_WID, 1);
			
			//calcing, 16 pixel every two cycle in hardware, cmodel calc one time.
			hevc_sao_calc_chr_lower(CTU_IN_MEM_PIXEL_WID, sao_data_out_chr, x, y);
			
			//sao output write ctu out ram; i 表示在一个bitwid里占用的bits
			hevc_sao_wr_lower_chr(x, y, CTU_IN_MEM_PIXEL_WID);
		}
	}

	//chroma, if tiler is true, so do last col sao, col width is (CTU_IN_MEM_PIXEL_WID-10)
	if(picr && (width&(CTU_IN_MEM_PIXEL_WID-1)))
	{
		datalen = width&(CTU_IN_MEM_PIXEL_WID-1);
		for (y = 0; y < height/2; y++)
		{
			//horz read pixel
			hevc_sao_rd_mid_lower_chr(x, y, datalen, datalen, 0);
			
			//calcing, 16 pixel every two cycle in hardware, cmodel calc one time.
			hevc_sao_calc_chr_lower(datalen, sao_data_out_chr, x, y);
			
			//sao output write ctu out ram; i 表示在一个bitwid里占用的bits
			hevc_sao_wr_lower_chr(x, y, datalen);
		}
	}
	
	return ;
}

void  SaoModule::hevc_sao_output()
{
	input	ctu_x = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_x;
	input	ctu_y = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_y;	
    input   picb   = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_b;    
    input   picr   = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_r;      
	input	ctu_out_max_size;


	ctu_out_max_size = ((ctb_size_filter==64)?(CTU_MAX_SIZE):(CTU_MAX_SIZE*2));

	
	//width promise 64, but if ctbsize is 16 or 32, so width must 128
	if(((ctu_num_in_outbuff+1)*ctb_size_filter < ctu_out_max_size) && !picr)
		return ;


	if(bit_depthY_pic_cmd == 8)
		hevc_sao_output_bitdep8_Y(ctu_y, ctu_x, 0, 0, picb, picr);
	else
		assert(0);
	
	if(bit_depthC_pic_cmd == 8)
		hevc_sao_output_bitdep8_C(ctu_y, ctu_x, 0, 0, picb, picr);
	else
		assert(0);
	
	return ;
}

void  SaoModule::hevc_sao_output_bitdep8_Y(int ctu_y, int ctu_x, int tilel, int tiler, int picb, int picr)
{
	int 	i, j, strideout, stridepix, start_out;
	unsigned char data_burst8_tmp[128]={0};

	input 	ctbw   = (ctb_size_filter==64) ? (4*HEVC_MAX_BIT_DEP/8+1) : (4*HEVC_MAX_BIT_DEP*2/8+1);
	input	luma_col_size = ctb_size_filter - (ctu_y == 0)*4 + (picb!=0)*4;

	
	//LUMA pixel output
	{
		strideout = ctbw;
		stridepix = ((ctu_num_in_outbuff+1)*ctb_size_filter);
		for(i=0;i<luma_col_size;i++)
		{
			for(j=0;j<stridepix/16;j++)
				ctu_y_out_buff[ctu_out_buff_cnt].read(strideout*i+j, (char *)&data_burst8_tmp[16*j]);
			hevc_write_output_ddr(data_burst8_tmp, stridepix, 0, i, 0, LUMA, bit_depthY_pic_cmd);			
		}	
	}

	return ;
}

void  SaoModule::hevc_sao_output_bitdep8_C(int ctu_y, int ctu_x, int tilel, int tiler, int picb, int picr)
{
	int 	i, j, strideout, stridepix, start_out;
	unsigned char data_burst8_tmp[128]={0};

	input 	ctbw   = (ctb_size_filter==64) ? (4*HEVC_MAX_BIT_DEP/8+1) : (4*HEVC_MAX_BIT_DEP*2/8+1);
	input	luma_col_size = ctb_size_filter - (ctu_y == 0)*4 + (picb!=0)*4;

	
	//chroma pixel output
	{
		strideout = ctbw;
		stridepix = ((ctu_num_in_outbuff+1)*ctb_size_filter);
		for(i=0;i<luma_col_size/2;i++)
		{
			for(j=0;j<stridepix/16;j++)
				ctu_uv_out_buff[ctu_out_buff_cnt].read(strideout*i+j, (char *)&data_burst8_tmp[16*j]);
			hevc_write_output_ddr(data_burst8_tmp, stridepix, 0, i, 0, CHROMA, bit_depthC_pic_cmd);			
		}
	}

	return ;
}

void  SaoModule::hevc_write_output_ddr(unsigned char *data_tmp, int bytelen, int x, int y, int bytepos, int yuvmode, int bit_depth)
{
	int bit_dep_out = (bit_depth==9) ? 10 : bit_depth;
	int	i, pixnum = bytelen*8/bit_dep_out;
	int pic_vir_stride = HEVC_MAX_WIDTH*12/8;//((pic_width_in_luma_samples+ctb_size_filter-1)&(~(ctb_size_filter-1)))*bit_depth;
	input	pic_height_in_luma_samples = 0, pic_width_in_luma_samples = 0;
	int pic_height=pic_height_in_luma_samples;

	unsigned char *outputaddrY = (unsigned char *)outputpicaddr;
	unsigned char *outputaddrUV = (unsigned char *)(outputpicaddr+pic_vir_stride*pic_height);
	unsigned char *dst, *dstUV;
	int width = pic_vir_stride;//pic_width_in_luma_samples;
	int height = pic_height;//pic_height_in_luma_samples;
		
	input	ctu_x = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_x;
	input	ctu_y = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->ctu_y;
	input   picr  = 0;//((CTU_START_CMD *)&ctu_start_cmd[ctu_buf_cnt_sao])->pic_r;  
	
	
	if((ctu_y*ctb_size_filter-(ctu_y!=0)*4+y) >= pic_height_in_luma_samples && (yuvmode==LUMA))	//out of pic board, don't write pixel
		return ;
	if((ctu_y*ctb_size_filter/2-(ctu_y!=0)*2+y) >= pic_height_in_luma_samples/2 && (yuvmode==CHROMA))	//out of pic board, don't write pixel
		return ;
	
	ctu_x = ctu_x - ctu_num_in_outbuff;
	
	if((ctu_x*ctb_size_filter+x+pixnum) >= pic_width_in_luma_samples)	//out of pic board, don't write pixel
		pixnum = pixnum - ((ctu_x*ctb_size_filter+x+pixnum) - pic_width_in_luma_samples);
	
	if(yuvmode == LUMA)	//bit_depth
	{
		dst = outputaddrY + (ctu_y*ctb_size_filter-(ctu_y!=0)*4+y)*width + (ctu_x*ctb_size_filter+x)*bit_dep_out/8+bytepos;
		for(i=0; i< bytelen; i++)
	        dst[i] = data_tmp[i];
		
	}else if(yuvmode == CHROMA)
	{
		dstUV = outputaddrUV + (ctu_y*ctb_size_filter/2-(ctu_y!=0)*2+y)*width + (ctu_x*ctb_size_filter+x)*bit_dep_out/8+bytepos;
		for(i=0; i< bytelen; i++)
	        dstUV[i] = data_tmp[i];
		
	}else
	{
		assert(0);
	}

	return ;
}


