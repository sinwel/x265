#include "ctu_calc.h"
#include "hardwareC.h"
#include "macro.h"



CTU_CALC::CTU_CALC()
{
    uint8_t i;

    input_curr_y = (uint8_t *)malloc(64*64);
    input_curr_u = (uint8_t *)malloc(32*32);
    input_curr_v = (uint8_t *)malloc(32*32);

    output_recon_y = (uint8_t *)malloc(64*64);
    output_recon_u = (uint8_t *)malloc(32*32);
    output_recon_v = (uint8_t *)malloc(32*32);

    output_resi_y = (int16_t *)malloc(64*64*2);
    output_resi_u = (int16_t *)malloc(32*32*2);
    output_resi_v = (int16_t *)malloc(32*32*2);

    cbfY = (uint8_t *)malloc(256);
    cbfU = (uint8_t *)malloc(64);
    cbfV = (uint8_t *)malloc(64);

    cu_level_calc[0].init(64);
    cu_level_calc[1].init(32);
    cu_level_calc[2].init(16);
    cu_level_calc[3].init(8);
#ifdef X265_LOG_FILE_ROCKCHIP
	fp_rk_intra_params = fopen(PATH_NAME("rk_intra_params.txt"), "w+" );
	if ( fp_rk_intra_params == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}

#endif

    cu_ori_data = new cuData**[4];

    cu_ori_data[0] = new cuData*[1];
	cu_ori_data[0][0] = new cuData;
    cu_ori_data[0][0]->init(64);

	cu_ori_data[1] = new cuData*[4];
    for(i=0; i<4; i++) {
        cu_ori_data[1][i] = new cuData;
        cu_ori_data[1][i]->init(32);
    }

	cu_ori_data[2] = new cuData*[16];
    for(i=0; i<16; i++) {
        cu_ori_data[2][i] = new cuData;
        cu_ori_data[2][i]->init(16);
    }

	cu_ori_data[3] = new cuData*[64];
    for(i=0; i<64; i++) {
        cu_ori_data[3][i] = new cuData;
        cu_ori_data[3][i]->init(8);
    }
}

CTU_CALC::~CTU_CALC()
{
    uint8_t i;

    cu_level_calc[0].deinit();
    cu_level_calc[1].deinit();
    cu_level_calc[2].deinit();
    cu_level_calc[3].deinit();
#ifdef X265_LOG_FILE_ROCKCHIP
	fclose(fp_rk_intra_params);
#endif
    free(input_curr_y);
    free(input_curr_u);
    free(input_curr_v);

    free(output_recon_y);
    free(output_recon_u);
    free(output_recon_v);

    free(output_resi_y);
    free(output_resi_u);
    free(output_resi_v);

    free(cbfY);
    free(cbfU);
    free(cbfV);

    cu_ori_data[0][0]->deinit();
    delete cu_ori_data[0][0];
    delete [] cu_ori_data[0];

    for(i=0; i<4; i++) {
        cu_ori_data[1][i]->deinit();
        delete cu_ori_data[1][i];
    }
    delete [] cu_ori_data[1];

    for(i=0; i<16; i++) {
        cu_ori_data[2][i]->deinit();
        delete cu_ori_data[2][i];
    }
    delete [] cu_ori_data[2];

    for(i=0; i<64; i++) {
		cu_ori_data[3][i]->deinit();
        delete cu_ori_data[3][i];
    }
    delete [] cu_ori_data[3];
    delete [] cu_ori_data;
}

void CTU_CALC::init()
{
    uint8_t m;
    for(m=0; m<4; m++) {
        best_pos[m] = 0;
        temp_pos[m] = 0;
    }

    cu_level_calc[0].pHardWare = pHardWare;
    cu_level_calc[1].pHardWare = pHardWare;
    cu_level_calc[2].pHardWare = pHardWare;
    cu_level_calc[3].pHardWare = pHardWare;
}


void CTU_CALC::deinit()
{
}

void CTU_CALC::begin()
{
    uint32_t pos;
    uint32_t len;
    uint8_t *buf_y, *buf_u, *buf_v, *buf_t;
    struct MV_INFO *buf_mv;

    /* intra */
    pos = ctu_x*ctu_w;
    buf_y = line_intra_buf_y + (pos);
    buf_u = line_intra_buf_u + (pos >> 1);
    buf_v = line_intra_buf_v + (pos >> 1);
    buf_t = line_cu_type + (pos / 8);
    buf_mv= line_mv_buf + (pos / 8);

    memcpy(L_intra_buf_y + 65, buf_y, ctu_w);
    memcpy(L_intra_buf_u + 33, buf_u, ctu_w >> 1);
    memcpy(L_intra_buf_v + 33, buf_v, ctu_w >> 1);
    memcpy(L_cu_type + 9,      buf_t, ctu_w >> 3);

    len = ctu_w;
    if(!(pos + ctu_w >= pHardWare->pic_w)) {
        memcpy(L_intra_buf_y + 65 + (ctu_w),      buf_y + (ctu_w),      32);
        memcpy(L_intra_buf_u + 33 + (ctu_w >> 1), buf_u + (ctu_w >> 1), 32 >> 1);
        memcpy(L_intra_buf_v + 33 + (ctu_w >> 1), buf_v + (ctu_w >> 1), 32 >> 1);
        memcpy(L_cu_type + 9 + (ctu_w >> 3), buf_t + (ctu_w >> 3), 32 >> 3);
    }

    memset(L_intra_mpm + 16, 0x1, 17); //MODE_DC
    if(ctu_x == 0)
        memset(L_intra_mpm, 0x1, 16); //MODE_DC
    /* inter */
    if(pos) {
        memcpy(L_mv_buf + 8, buf_mv - 1, sizeof(MV_INFO));
    }

    memcpy(L_mv_buf + 9, buf_mv, (ctu_w / 8)*sizeof(MV_INFO));

    if(!(pos + ctu_w >= pHardWare->pic_w)) {
        memcpy(L_mv_buf + 9 + (ctu_w / 8), buf_mv + (ctu_w / 8), (ctu_w / 8)*sizeof(MV_INFO));
    }

    /* ctu_valid_flag must be replaced by ENC_CTRL module in ctu_cmd */

    valid_left = valid_left_top = valid_top = valid_top_right = 1;

    if(ctu_y == 0) {
        valid_left_top = 0;
        valid_top = 0;
        valid_top_right = 0;
    }

    if(ctu_x == 0) {
        valid_left = 0;
        valid_left_top = 0;
    }

    if((ctu_x + 1)*ctu_w > pic_w) {
        valid_top_right = 0;
    }

    cu_level_calc[0].ori_pos = 0;
    cu_level_calc[1].ori_pos = 0;
    cu_level_calc[2].ori_pos = 0;
    cu_level_calc[3].ori_pos = 0;

    cu_level_calc[0].cu_pos = 0;
    cu_level_calc[1].cu_pos = 0;
    cu_level_calc[2].cu_pos = 0;
    cu_level_calc[3].cu_pos = 0;

    cu_level_calc[0].temp_pos = 0;
    cu_level_calc[1].temp_pos = 0;
    cu_level_calc[2].temp_pos = 0;
    cu_level_calc[3].temp_pos = 0;

    /* CTU开始前，将cu_data_valid清0 */
    memset(cu_data_valid, 0, 8*8);
}

void CTU_CALC::end()
{
    uint32_t pos;
    uint8_t *buf_y, *buf_u, *buf_v, *buf_t;
    uint8_t i;
    struct MV_INFO *buf_mv;

    /* intra */

    pos = ctu_x * ctu_w;
    buf_y = line_intra_buf_y + (pos);
    buf_u = line_intra_buf_u + (pos >> 1);
    buf_v = line_intra_buf_v + (pos >> 1);
    buf_t = line_cu_type + (pos / 8);

    /* ver to hor */
    for(i=0; i<ctu_w; i++){
        buf_y[i] = L_intra_buf_y[64 - ctu_w + i + 1];
    }

    for(i=0; i<ctu_w>>1; i++){
        buf_u[i] = L_intra_buf_u[32 - (ctu_w>>1) + i + 1];
        buf_v[i] = L_intra_buf_v[32 - (ctu_w>>1) + i + 1];
    }

    for(i=0; i<ctu_w/8; i++){
        buf_t[i] = L_cu_type[8 - ctu_w/8 + i + 1];// | (inter_cbf[16 - ctu_w/4 + i*2 + 1] << 1) | (inter_cbf[16 - ctu_w/4 + i*2 + 2] << 2);
    }

    /* hor to ver */
	for(i=0; i<(ctu_w/8 + 1); i++){
		L_cu_type[i] = L_cu_type[8 + i];
	}
    for(i=0; i<(ctu_w/4 + 1); i++){
		L_intra_mpm[i] = L_intra_mpm[16 + i];
	}

    for(i=0; i<ctu_w + 1; i++){
        L_intra_buf_y[i] = L_intra_buf_y[64 + i];
    }

    for(i=0; i<(ctu_w>>1) + 1; i++){
        L_intra_buf_u[i] = L_intra_buf_u[32 + i];
        L_intra_buf_v[i] = L_intra_buf_v[32 + i];
    }

	/* inter */
    pos = ctu_x * ctu_w;

    /* ver to hor */
    buf_mv = line_mv_buf + (pos/8);

    for(i=0; i<(ctu_w/8); i++){
        buf_mv[i] = L_mv_buf[i + 1];
    }

    /* hor to ver */
    for(i=0; i<(ctu_w/8); i++){
        L_mv_buf[i] = L_mv_buf[8 + i];
    }
    memcpy(pHardWare->inf_dblk.recon_y[ctu_x], output_recon_y, 64*64);
    memcpy(pHardWare->inf_dblk.recon_u[ctu_x], output_recon_y, 32*32);
    memcpy(pHardWare->inf_dblk.recon_v[ctu_x], output_recon_y, 32*32);
    memcpy(pHardWare->inf_dblk.cu_depth[ctu_x], cu_depth, 64);
    memcpy(pHardWare->inf_dblk.tu_depth[ctu_x], tu_depth, 256);
    memcpy(pHardWare->inf_dblk.pu_depth[ctu_x], pu_depth, 256);
    memcpy(pHardWare->inf_dblk.intra_bs_flag[ctu_x], intra_bs_flag, 256);
    memcpy(pHardWare->inf_dblk.qp[ctu_x], qp, 256);
}


void CTU_CALC::cu_level_compare(uint32_t bestCU_cost, uint32_t tempCU_cost, uint8_t depth)
{
    class cuData *src_cu, *dst_cu;
    uint8_t m, n;
    uint8_t cu_w = cu_level_calc[depth + 1].cu_w;
    uint32_t len = cu_w * cu_w;
    uint32_t pos;
    uint8_t i, j;

    pHardWare->ctu_calc.cu_split_flag[depth*4 + cu_level_calc[depth].ori_pos] = 0;

    dst_cu = &cu_level_calc[depth].cu_matrix_data[cu_level_calc[depth].matrix_pos];
    if((bestCU_cost > tempCU_cost) || (cu_valid_flag[depth][cu_level_calc[depth].ori_pos] == 0))
    {
        for(m=0; m<4; m++)
        {
            src_cu = &cu_level_calc[depth+1].cu_matrix_data[m];
            memcpy(dst_cu->CoeffY + len*m,   src_cu->CoeffY, len*2);
            memcpy(dst_cu->CoeffU + len*m/4, src_cu->CoeffU, len/2);
            memcpy(dst_cu->CoeffV + len*m/4, src_cu->CoeffV, len/2);

            pos = (m & 1)*cu_w + (m >> 1)*cu_w*cu_w*2;
            for(n=0; n<cu_w; n++)
                memcpy(dst_cu->ReconY + pos + n*cu_w*2, src_cu->ReconY + n*cu_w, cu_w);

            pos = (m & 1)*cu_w/2 + (m >> 1)*cu_w*cu_w/2;
            for(n=0; n<cu_w/2; n++) {
                memcpy(dst_cu->ReconU + pos + n*cu_w, src_cu->ReconU + n*cu_w/2, cu_w/2);
                memcpy(dst_cu->ReconV + pos + n*cu_w, src_cu->ReconV + n*cu_w/2, cu_w/2);
            }
            pos = (m & 1)*cu_w/8 + (m >> 1)*(cu_w/4)*(cu_w/8);
            for(n=0; n<cu_w/8; n++) {
                memcpy(dst_cu->cuPredMode + pos + n*cu_w/4, src_cu->cuPredMode + n*cu_w/8, cu_w/8);
                memcpy(dst_cu->mv + pos + n*cu_w/4, src_cu->mv + n*cu_w/8, sizeof(MV_INFO)*cu_w/8);
            }
            pos = (m & 1)*cu_w/4 + (m >> 1)*(cu_w/2)*(cu_w/4);
            for(n=0; n<cu_w/4; n++) {
                memcpy(dst_cu->cbfY + pos + n*cu_w/2, src_cu->cbfY + n*cu_w/4, cu_w/4);
            }
        }

        pHardWare->ctu_calc.cu_split_flag[(depth>>1)*5 + (depth&1) + cu_level_calc[depth].ori_pos] = 1;
    }
    else {
        uint8_t cu_x_pos = cu_level_calc[depth].x_pos/8;
        uint8_t cu_y_pos = cu_level_calc[depth].y_pos/8;
        for(j=0; j<cu_level_calc[depth].cu_w/8; j++) {
            for(i=0; i<cu_level_calc[depth].cu_w/8; i++) {
                pHardWare->inf_dblk.cu_depth[pHardWare->ctu_x][cu_y_pos*8 + cu_x_pos] = depth;
                pHardWare->inf_dblk.mv_info[pHardWare->ctu_x][cu_y_pos*8 + cu_x_pos]  = cu_level_calc[depth].MV;
                if (cu_level_calc[depth].cuPredMode == 0) {
                    for(m=0; m<4; m++) {
                        pHardWare->inf_dblk.pu_depth[pHardWare->ctu_x][(cu_y_pos*2+ (m>>1))*16 + cu_x_pos*2 + (m&1)] = cu_level_calc[depth].cuPartMode;
                        pHardWare->inf_dblk.tu_depth[pHardWare->ctu_x][(cu_y_pos*2+ (m>>1))*16 + cu_x_pos*2 + (m&1)] = cu_level_calc[depth].cuPartMode;
                        pHardWare->inf_dblk.intra_bs_flag[pHardWare->ctu_x][(cu_y_pos*2+ (m>>1))*16 + cu_x_pos*2 + (m&1)] = cu_level_calc[depth].intra_cbfY[m];
                    }
                }
                else {
                    for(m=0; m<4; m++) {
                        pHardWare->inf_dblk.pu_depth[pHardWare->ctu_x][(cu_y_pos*2+ (m>>1))*16 + cu_x_pos*2 + (m&1)] = 0;
                        pHardWare->inf_dblk.tu_depth[pHardWare->ctu_x][(cu_y_pos*2+ (m>>1))*16 + cu_x_pos*2 + (m&1)] = 0;
                        pHardWare->inf_dblk.intra_bs_flag[pHardWare->ctu_x][(cu_y_pos*2+ (m>>1))*16 + cu_x_pos*2 + (m&1)] = cu_level_calc[depth].inter_cbfY[0];
                    }
                }
            }
        }
    }
}

void CTU_CALC::UpdateMvpInfo(int offsIdx, bool isSplit)
{
	SPATIAL_MV mvSpatial;
	int level = 0;
	if (offsIdx < 64)
		level = 3;
	else if (offsIdx < 80)
		level = 2;
	else if (offsIdx < 84)
		level = 1;
	else
		level = 0;
	MvField mvField;
	if (cu_level_calc[level].cost_best->predMode == 0)
	{
		if (cu_level_calc[level].cost_best->mergeFlag == 1) //merge
		{
			int nMergeIdx = cu_level_calc[level].Merge.getMergeIdx();
			mvSpatial.valid = 1;
			for (int i = 0; i < 2; i++)
			{
				mvField = cu_level_calc[level].Merge.getMergeCand(nMergeIdx, 0 != i);
				if (mvField.refIdx >= 0)
				{
					mvSpatial.pred_flag[i] = 1;
					mvSpatial.mv[i].x = mvField.mv.x;
					mvSpatial.mv[i].y = mvField.mv.y;
					mvSpatial.delta_poc[i] = cu_level_calc[level].m_MergeCand.getCurrPicPoc() -
						cu_level_calc[level].m_MergeCand.getCurrRefPicPoc(i, mvField.refIdx);
				}
				else
					mvSpatial.pred_flag[i] = 0;
				mvSpatial.ref_idx[i] = mvField.refIdx;
			}
		}
		else //fme
		{
			mvSpatial.valid = 1;
			for (int i = 0; i < 2; i++)
			{
				int refIdx = cu_level_calc[level].Fme.getFmeInfoForCabac()->m_refIdx[i];
				Mv tmpMv;
				if (refIdx >= 0)
				{
					mvSpatial.pred_flag[i] = 1;
					int idx = i == 0 ? refIdx : refIdx + nMaxRefPic / 2;
					mvSpatial.mv[i].x = cu_level_calc[level].Fme.getMvFme(idx).x;
					mvSpatial.mv[i].y = cu_level_calc[level].Fme.getMvFme(idx).y;
					mvSpatial.delta_poc[i] = cu_level_calc[level].m_Amvp.getCurrPicPoc() - cu_level_calc[level].m_Amvp.getCurrRefPicPoc(i, refIdx);
				}
				else
					mvSpatial.pred_flag[i] = 0;
				mvSpatial.ref_idx[i] = refIdx;
			}
		}
	}
	else
	{
		mvSpatial.pred_flag[0] = 0;
		mvSpatial.pred_flag[1] = 0;
		//isSplit = false;
	}
	cu_level_calc[level].m_Amvp.UpdateMvpInfo(offsIdx, mvSpatial, isSplit);
	cu_level_calc[level].m_MergeCand.UpdateMvpInfo(offsIdx, mvSpatial, isSplit);
}

void CTU_CALC::compare()
{
    uint32_t i, j;
    uint32_t w, h;

    w = ctu_w;
    h = ctu_w;
    if (ctu_w*(ctu_x + 1) > pic_w)
        w = pic_w - ctu_w*ctu_x;

    if (ctu_w*(ctu_y + 1) > pic_h)
        h = pic_h - ctu_w*ctu_y;


    for(j=0; j<h; j++) {
        for(i=0; i<w; i++) {
            if(cu_level_calc[0].cu_matrix_data[0].ReconY[j*ctu_w + i]!= (ori_recon_y[j*ctu_w + i])) {
                printf("ERROR RECON Y is diff Y %d X %d\n", j, i);
                assert(0);
            }
        }
    }

    for(j=0; j<h/2; j++)
    {
        for(i=0; i<w/2; i++)
        {
            if(cu_level_calc[0].cu_matrix_data[0].ReconU[j*ctu_w/2 + i] != (ori_recon_u[j*ctu_w/2 + i])) {
                printf("ERROR RECON u is diff Y %d X %d\n", j, i);
                assert(0);
            }
        }
    }

    for(j=0; j<h/2; j++)
    {
        for(i=0; i<w/2; i++)
        {
            if (cu_level_calc[0].cu_matrix_data[0].ReconV[j*ctu_w/2 + i] != (ori_recon_v[j*ctu_w/2 + i])) {
                printf("ERROR RECON v is diff Y %d X %d\n", j, i);
                assert(0);
            }
        }
    }
}

void static LogRef2File(FILE* fp_rk_intra_params, INTERFACE_INTRA_PROC inf_intra_proc, uint8_t* pRef, int8_t idx)
{
	uint8_t flag_map_pel = 8;
	//uint8_t* bNeighbour = inf_intra_proc.bNeighborFlags - (inf_intra_proc.size/4);

	uint8_t* flags = inf_intra_proc.bNeighborFlags - (inf_intra_proc.size/4);// 指向左上角 转换为 指向左下角

	bool Flags[4*RK_MAX_NUM_SPU_W + 1];
	uint8_t k = 0;
	for (uint8_t i = 0 ; i < 2*inf_intra_proc.size/flag_map_pel ; i++ )
	{
		Flags[k++] = *flags   == 1 ? true : false;
		Flags[k++] = *flags++ == 1 ? true : false;
	}

	Flags[k++] = *flags++ == 1 ? true : false;

	for (uint8_t i = 0 ; i < 2*inf_intra_proc.size/flag_map_pel ; i++ )
	{
		Flags[k++] = *flags   == 1 ? true : false;
		Flags[k++] = *flags++ == 1 ? true : false;
	}

#if 0
	if ( idx != 0 )
	{
	    inf_intra_proc.size >>= 1;
		flag_map_pel = 4;
	}

	for (uint8_t i = 0 ; i < 2*inf_intra_proc.size/flag_map_pel; i++ )
	{
		if ( *bNeighbour++ == 1 )
		{
			for (uint8_t j = 0 ; j < flag_map_pel ; j++ )
			{
			    RK_HEVC_FPRINT(fp_rk_intra_params,"0x%02x ",*pRef++ );
			}
		}
		else
		{
			for (uint8_t j = 0 ; j < flag_map_pel ; j++ )
			{
			    RK_HEVC_FPRINT(fp_rk_intra_params,"xxxx ");
				pRef++;
			}
		}
	}

	if ( *bNeighbour++ == 1 )
	{
	    RK_HEVC_FPRINT(fp_rk_intra_params,"\n0x%02x \n",*pRef++ );
	}
	else
	{
	    RK_HEVC_FPRINT(fp_rk_intra_params,"\nxxxx \n" );
		pRef++;
	}

	for (uint8_t i = 0 ; i < 2*inf_intra_proc.size/flag_map_pel; i++ )
	{
		if ( *bNeighbour++ == 1 )
		{
			for (uint8_t j = 0 ; j < flag_map_pel ; j++ )
			{
			    RK_HEVC_FPRINT(fp_rk_intra_params,"0x%02x ",*pRef++ );
			}
		}
		else
		{
			for (uint8_t j = 0 ; j < flag_map_pel ; j++ )
			{
			    RK_HEVC_FPRINT(fp_rk_intra_params,"xxxx ");
				pRef++;
			}
		}
	}
#else
	flag_map_pel = 4;
	if ( idx != 0 )
	{
	    inf_intra_proc.size >>= 1;
		flag_map_pel = 2;
	}
	bool* pflag = Flags;
	for (uint8_t i = 0 ; i < 2*inf_intra_proc.size/flag_map_pel; i++ )
	{
		if ( *pflag++ == 1 )
		{
			for (uint8_t j = 0 ; j < flag_map_pel ; j++ )
			{
			    RK_HEVC_FPRINT(fp_rk_intra_params,"0x%02x ",*pRef++ );
			}
		}
		else
		{
			for (uint8_t j = 0 ; j < flag_map_pel ; j++ )
			{
			    RK_HEVC_FPRINT(fp_rk_intra_params,"xxxx ");
				pRef++;
			}
		}
	}

	if ( *pflag++ == 1 )
	{
	    RK_HEVC_FPRINT(fp_rk_intra_params,"\n0x%02x \n",*pRef++ );
	}
	else
	{
	    RK_HEVC_FPRINT(fp_rk_intra_params,"\nxxxx \n" );
		pRef++;
	}

	for (uint8_t i = 0 ; i < 2*inf_intra_proc.size/flag_map_pel; i++ )
	{
		if ( *pflag++ == 1 )
		{
			for (uint8_t j = 0 ; j < flag_map_pel ; j++ )
			{
			    RK_HEVC_FPRINT(fp_rk_intra_params,"0x%02x ",*pRef++ );
			}
		}
		else
		{
			for (uint8_t j = 0 ; j < flag_map_pel ; j++ )
			{
			    RK_HEVC_FPRINT(fp_rk_intra_params,"xxxx ");
				pRef++;
			}
		}
	}

#endif
	RK_HEVC_FPRINT(fp_rk_intra_params,"\n\n");

}
void static LogFenc2File(FILE* fp_rk_intra_params, INTERFACE_INTRA_PROC inf_intra_proc, uint8_t* fenc,  int8_t idx)
{
	if ( idx != 0 )
	{
	    inf_intra_proc.size >>= 1;
	}

	// rk_Interface_Intra.fencY/U/V
	for (uint8_t i = 0 ; i < inf_intra_proc.size; i++ )
	{
		for (uint8_t j = 0 ; j < inf_intra_proc.size ; j++ )
		{
			RK_HEVC_FPRINT(fp_rk_intra_params,"0x%02x ",fenc[i * inf_intra_proc.size + j]);
		}
		RK_HEVC_FPRINT(fp_rk_intra_params,"\n");
	}
	RK_HEVC_FPRINT(fp_rk_intra_params,"\n\n");

	if ( idx == 0)
	{
		RK_HEVC_FPRINT(fp_rk_intra_params,"Luma StrongFlag = %d\n\n", inf_intra_proc.useStrongIntraSmoothing);
	}
	else
	{
		RK_HEVC_FPRINT(fp_rk_intra_params,"chroma haven't StrongFlag\n\n");
	}

}
void CTU_CALC::LogFile(INTERFACE_TQ* inf_tq)
{
#if 0
#if 0
	if (inf_tq->Size == 4 || inf_tq->textType != 0)
	{
		return;
	}
	fprintf(fp_TQ_Log_HWC, "==================== TQ input ====================\n");
	fprintf(fp_TQ_Log_HWC, "---------------- TU begin ----------------\n");
	fprintf(fp_TQ_Log_HWC, "sliceType = %d,  predMode = %d,  textType = %d,  size = %d\n",
		inf_tq->sliceType, inf_tq->predMode,
		inf_tq->textType, inf_tq->Size);
	fprintf(fp_TQ_Log_HWC, "qp = %d,  qpBdOffset = %d,  chromaQPOffset = %d,  transformSkip = %d\n",
		inf_tq->QP, inf_tq->qpBdOffset,
		inf_tq->chromaQpOffset, inf_tq->TransFormSkip);
	fprintf(fp_TQ_Log_HWC, "Resi:\n");
	for (uint32_t k = 0; k < inf_tq->Size; k++)
	{
		for (int j = 0; j < inf_tq->Size; j++)
		{
			fprintf(fp_TQ_Log_HWC, "0x%04x ", inf_tq->resi[k*inf_tq->Size + j]&0xffff);
		}
		fprintf(fp_TQ_Log_HWC, "\n");
	}
	fprintf(fp_TQ_Log_HWC, "---------------- TU end ----------------\n\n\n");
#else
	uint32_t i, j;
	RK_HEVC_FPRINT(fp_TQ_Log_HWC, "Resi:\n");
	/* 这里stride等于width，即使是width=4的情况，就分开4次比较 */
	for (i = 0; i < inf_tq->Size; i++)
	{
		for (j = 0; j < inf_tq->Size; j++)
		{
			RK_HEVC_FPRINT(fp_TQ_Log_HWC, "0x%04x  ", (uint16_t)inf_tq->resi[j]);
		}
		inf_tq->resi += inf_tq->Size;
		RK_HEVC_FPRINT(fp_TQ_Log_HWC, "\n");
	}
	RK_HEVC_FPRINT(fp_TQ_Log_HWC, "\n\n");
#endif
#endif
}
void CTU_CALC::convert8x8HWCtoX265Luma(int16_t* coeff)
{
	int16_t temp[8*8];
	int16_t* src = coeff;
	int offset[4] = {0, 4, 32, 36};
	int i, j;
	for(i=0; i<4; i++)
	{
		src = coeff + offset[i];
		for(j=0; j<4; j++)
		{
			memcpy(&temp[16*i+j*4], &src[j*8], 4*sizeof(int16_t));
		}
	}
	memcpy(coeff, &temp[0], 8*8*sizeof(int16_t));
}

void CTU_CALC::compareCoeffandRecon(CU_LEVEL_CALC* hwc_data, int level, bool choose4x4split)
{
	assert(level <= 3);
	if(slice_type==2 && level==0)
		return;

	int pos=0;
	if(level==0) // cu_w==64, pos is always 0.
		pos=0;
	else		
		pos = hwc_data[level].cu_pos - 1;
	
	int sizeY = ctu_w >>  level;
	int sizeUV = ctu_w >> (level+1);

	int16_t* hwc_coeff[3];
	uint8_t* hwc_recon[3];
	
	if(hwc_data[level].cost_best->predMode==0) /* inter */
	{
		hwc_coeff[0] = hwc_data[level].cost_inter.resi_y;
		hwc_coeff[1] = hwc_data[level].cost_inter.resi_u;
		hwc_coeff[2] = hwc_data[level].cost_inter.resi_v;
		hwc_recon[0] = hwc_data[level].cost_inter.recon_y;
		hwc_recon[1] = hwc_data[level].cost_inter.recon_u;
		hwc_recon[2] = hwc_data[level].cost_inter.recon_v;	
	}
	else /* intra */
	{
		if(level==3 && choose4x4split) // only for intra 8x8
		{
			convert8x8HWCtoX265Luma(hwc_data[level].inf_tq_total[0].oriResi);//only Luma
		}	
		hwc_coeff[0] = hwc_data[level].inf_tq_total[0].oriResi;
		hwc_coeff[1] = hwc_data[level].inf_tq_total[1].oriResi;
		hwc_coeff[2] = hwc_data[level].inf_tq_total[2].oriResi;
		hwc_recon[0] = hwc_data[level].inf_recon_total[0].Recon;
		hwc_recon[1] = hwc_data[level].inf_recon_total[1].Recon;
		hwc_recon[2] = hwc_data[level].inf_recon_total[2].Recon;
	}

	/* compare oriResi(after T and Q) */
	if(level != 0) // cu_w < 64
	{
		assert(!memcmp(hwc_coeff[0], pHardWare->ctu_calc.cu_ori_data[level][pos]->CoeffY, sizeY*sizeY*sizeof(int16_t)));
		assert(!memcmp(hwc_coeff[1], pHardWare->ctu_calc.cu_ori_data[level][pos]->CoeffU, sizeUV*sizeUV*sizeof(int16_t)));
		assert(!memcmp(hwc_coeff[2], pHardWare->ctu_calc.cu_ori_data[level][pos]->CoeffV, sizeUV*sizeUV*sizeof(int16_t)));
	}
	else // cu_w==64
	//NOTE: when cu_w==64, coeff(after in T and Q) in x265 is stored unusually
	//1. stride 32(luma) or 16(chroma)
	//2. 4 coeff blocks are arranged one after another(raster scan).
	{
		int lumaPart2pos[4] = {0, 32, 32*64, 32*64+32};
		int chromaPart2pos[4] = {0, 16, 16*32, 16*32+16};
	    for(int i = 0; i < 4; i ++)
	    {
			short *pHwcCoeff = hwc_coeff[0] + lumaPart2pos[i];
			short *pX265Coeff = pHardWare->ctu_calc.cu_ori_data[level][pos]->CoeffY + i * 32 * 32;

			// Y
			for (int m = 0; m < 32; m++)
			{
				for (int n = 0; n < 32; n ++)
				{
					assert(pHwcCoeff[n + m * 64] == pX265Coeff[n + m * 32]);
				}
			}
			pHwcCoeff = hwc_coeff[1] + chromaPart2pos[i];
			pX265Coeff = pHardWare->ctu_calc.cu_ori_data[level][pos]->CoeffU + i * 16 * 16;

			// U
			for (int m = 0; m < 16; m++)
			{
				for (int n = 0; n < 16; n++)
				{
					assert(pHwcCoeff[n + m * 32] == pX265Coeff[n + m * 16]);
				}
			}
			pHwcCoeff = hwc_coeff[2] + chromaPart2pos[i];
			pX265Coeff = pHardWare->ctu_calc.cu_ori_data[level][pos]->CoeffV + i * 16 * 16;

			// V
			for (int m = 0; m < 16; m++)
			{
				for (int n = 0; n < 16; n++)
				{
					assert(pHwcCoeff[n + m * 32] == pX265Coeff[n + m * 16]);
				}
			}
			
	    }
	}

	/* compare Recon */
	assert(!memcmp(hwc_recon[0], pHardWare->ctu_calc.cu_ori_data[level][pos]->ReconY, sizeY*sizeY*sizeof(uint8_t)));
	assert(!memcmp(hwc_recon[1], pHardWare->ctu_calc.cu_ori_data[level][pos]->ReconU, sizeUV*sizeUV*sizeof(uint8_t)));
	assert(!memcmp(hwc_recon[2], pHardWare->ctu_calc.cu_ori_data[level][pos]->ReconV, sizeUV*sizeUV*sizeof(uint8_t)));

}

void CTU_CALC::LogIntraParams2File(INTERFACE_INTRA_PROC &inf_intra_proc, uint32_t x_pos, uint32_t y_pos)
{
	// log rk_Interface_Intra.bNeighborFlags
	if ( inf_intra_proc.size < 64)
	{
		// + offset 指向左下角
		bool bFlagCorner = (*inf_intra_proc.bNeighborFlags == 1) ? 1 : 0;
		uint8_t* bNeighbour = inf_intra_proc.bNeighborFlags - (inf_intra_proc.size/4);
        RK_HEVC_FPRINT(fp_rk_intra_params,"[cu_x = %d] [cu_y = %d]\n", ctu_x*ctu_w, ctu_y*ctu_w);
        RK_HEVC_FPRINT(fp_rk_intra_params,"[x_pos = %d] [y_pos = %d]\n", x_pos, y_pos);
		RK_HEVC_FPRINT(fp_rk_intra_params,"[num = %d]\n", (bFlagCorner == 1 ) ?
			inf_intra_proc.numIntraNeighbor * 2 - 1 : inf_intra_proc.numIntraNeighbor * 2);
		RK_HEVC_FPRINT(fp_rk_intra_params,"[size = %d]\n",inf_intra_proc.size);
		RK_HEVC_FPRINT(fp_rk_intra_params,"[initTrDepth = %d]\n",0);
		RK_HEVC_FPRINT(fp_rk_intra_params,"[part:%d]\n",0);
		// X265是4个pel一组，RK是8个PEL
		for (uint8_t i = 0 ; i < 2*inf_intra_proc.size/8; i++ )
		{
		    RK_HEVC_FPRINT(fp_rk_intra_params,"%d ",*bNeighbour );
		    RK_HEVC_FPRINT(fp_rk_intra_params,"%d ",*bNeighbour++ );
		}

		RK_HEVC_FPRINT(fp_rk_intra_params,"	 %d   ",*bNeighbour++ );

		for (uint8_t i = 0 ; i < 2*inf_intra_proc.size/8 ; i++ )
		{
		    RK_HEVC_FPRINT(fp_rk_intra_params,"%d ",*bNeighbour );
		    RK_HEVC_FPRINT(fp_rk_intra_params,"%d ",*bNeighbour++ );
		}
		RK_HEVC_FPRINT(fp_rk_intra_params,"\n\n");

		LogRef2File(fp_rk_intra_params, inf_intra_proc, inf_intra_proc.reconEdgePixelY - (2*inf_intra_proc.size), 0);
		LogFenc2File(fp_rk_intra_params, inf_intra_proc, inf_intra_proc.fencY, 0);

		LogRef2File(fp_rk_intra_params, inf_intra_proc, inf_intra_proc.reconEdgePixelU - inf_intra_proc.size, 1);
		LogFenc2File(fp_rk_intra_params, inf_intra_proc, inf_intra_proc.fencU, 1);

		LogRef2File(fp_rk_intra_params, inf_intra_proc, inf_intra_proc.reconEdgePixelV - inf_intra_proc.size, 2);
		LogFenc2File(fp_rk_intra_params, inf_intra_proc, inf_intra_proc.fencV, 2);

 	}
}

void CTU_CALC::proc()
{
    unsigned int cost_0 = 0, cost_1 = 0, cost_2 = 0, cost_3 = 0;
    unsigned int cost_1_total = 0, cost_2_total = 0, cost_3_total = 0;
    unsigned int totalBits_1  = 0, totalBits_2  = 0, totalBits_3  = 0;
    unsigned int totalDist_1  = 0, totalDist_2  = 0, totalDist_3  = 0;
    unsigned int m=0, n=0, k=0;
    unsigned int cu_x_1 = 0, cu_y_1 = 0;
    unsigned int cu_x_2 = 0, cu_y_2 = 0;
    unsigned int cu_x_3 = 0, cu_y_3 = 0;

    //IME 0:B 1:P 2:I
	//if (slice_type != 2)  // B or P
		//Ime();
	if (pHardWare->ctu_y == 1 && pHardWare->ctu_x == 0)
		pHardWare->ctu_y = pHardWare->ctu_y;

    cu_level_calc[0].cu_w = ctu_w >> 0;
    cu_level_calc[1].cu_w = ctu_w >> 1;
    cu_level_calc[2].cu_w = ctu_w >> 2;
    cu_level_calc[3].cu_w = ctu_w >> 3;

    /*begin*/
    begin();

#ifndef	CTU_CALC_STATUS_ENABLE
	//==============================================================
	int offsIdx = 84;
	cu_level_calc[0].depth = 0;
    cu_level_calc[0].TotalCost = 0;

#if RK_CABAC_FUNCTIONAL_TYPE
	for (int i = 0 ; i < 4 ; i++)
	{
		cu_level_calc[i].m_cabac_rdo = &m_cabac_rdo;
	}
#endif
    cost_0 = cu_level_calc[0].proc(0, 0, 0);
	compareCoeffandRecon(cu_level_calc, 0, false);

    totalBits_1 = 0;
    totalDist_1 = 0;
    for(m=0; m<4; m++)
    {
        cu_level_calc[1].depth = 1;
        cu_level_calc[1].matrix_pos = m;
        cu_x_1 = (m & 1)*(ctu_w>>1);
        cu_y_1 = (m >>1)*(ctu_w>>1);

        cost_1 = cu_level_calc[1].proc(1, cu_x_1, cu_y_1);

        if (!(cost_1 & 0x80000000))
		{
//            totalBits_1 += cu_level_calc[1].cost_best->Bits;
//            totalDist_1 += cu_level_calc[1].cost_best->Distortion;
#ifdef LOG_INTRA_PARAMS_2_FILE
            LogIntraParams2File(cu_level_calc[1].inf_intra_proc, cu_x_1, cu_y_1);
#endif
#if TQ_RUN_IN_HWC_INTRA
			//compare coeff(after T and Q), and Recon (Y, U, V)
			compareCoeffandRecon(cu_level_calc, 1, false);
#endif
        }

        totalBits_2 = 0;
        totalDist_2 = 0;
        for(n=0; n<4; n++)
        {
            cu_level_calc[2].depth = 2;
            cu_level_calc[2].matrix_pos = n;
            cu_x_2 = cu_x_1 + (n & 1)*(ctu_w>>2);
            cu_y_2 = cu_y_1 + (n >>1)*(ctu_w>>2);

            cost_2 = cu_level_calc[2].proc(2, cu_x_2, cu_y_2);


            if (!(cost_2 & 0x80000000)) {
//                totalBits_2 += cu_level_calc[2].cost_best->Bits;
//                totalDist_2 += cu_level_calc[2].cost_best->Distortion;

			#ifdef LOG_INTRA_PARAMS_2_FILE
                LogIntraParams2File(cu_level_calc[2].inf_intra_proc, cu_x_2, cu_y_2);
			#endif
#if TQ_RUN_IN_HWC_INTRA
			    //compare coeff(after T and Q), and Recon (Y, U, V)
			    compareCoeffandRecon(cu_level_calc, 2, false);
#endif
            }

            totalBits_3 = 0;
            totalDist_3 = 0;
            for(k=0; k<4; k++)
            {
                cu_level_calc[3].depth = 3;
                cu_level_calc[3].matrix_pos = k;
                cu_x_3 = cu_x_2 + (k & 1)*(ctu_w>>3);
                cu_y_3 = cu_y_2 + (k >>1)*(ctu_w>>3);

                cost_3 = cu_level_calc[3].proc(3, cu_x_3, cu_y_3);
#if RK_CABAC_FUNCTIONAL_TYPE
				m_cabac_rdo.cu_update();
#endif

                if (!(cost_3 & 0x80000000)) {
                    totalBits_3 += cu_level_calc[3].cost_best->Bits;
                    totalDist_3 += cu_level_calc[3].cost_best->Distortion;

				#ifdef LOG_INTRA_PARAMS_2_FILE
                    LogIntraParams2File(cu_level_calc[3].inf_intra_proc, cu_x_3, cu_y_3);
				#endif
#if TQ_RUN_IN_HWC_INTRA //wait for 4x4 intra to be done, added by lks
				    //compare coeff(after T and Q), and Recon (Y, U, V)
				    compareCoeffandRecon(cu_level_calc, 3, cu_level_calc[3].choose4x4split);
#endif
                }

				if (2 != pHardWare->ctu_calc.slice_type)
				{
					offsIdx = cu_x_3 / 8 + cu_y_3 / 8 * 8;
					UpdateMvpInfo(offsIdx, false);
				}

				cu_level_calc[3].end();
				cu_level_calc[3].ori_pos++;

            }
            //EncoderCuSplitFlag();
            //calcRDOCOST
            cost_3_total = (uint32_t)pHardWare->ctu_calc.intra_temp_2[cu_level_calc[2].temp_pos-1].m_totalCost;
#if RK_CABAC_FUNCTIONAL_TYPE
			uint32_t zscan = RasterToZscan[cu_x_2/4 + cu_y_2*4];
			int bits_split_cu_flag = (m_cabac_rdo.bits_split_cu_flag[2] + (1<<14))>>15;
			totalBits_3+= bits_split_cu_flag;
			int temp = cu_level_calc[3].RdoCostCalc(totalDist_3, totalBits_3, pHardWare->ctu_calc.QP_lu);
			if (!(cost_2 & 0x80000000))
			{
				assert(cost_3_total == temp);
			}

			if (cost_2 <= cost_3_total)
			{
				//m_cabac_rdo.cu_best_mode[2] = 1;//0表示当前层用inter  1表示intra   2表示不用这层，用这层往下划分的
				if (!(cost_2 & 0x80000000)) {
					totalBits_2 += cu_level_calc[2].cost_best->Bits;
					totalDist_2 += cu_level_calc[2].cost_best->Distortion;
				}
			}
			else 
			{
				m_cabac_rdo.cu_best_mode[2] = 2;
				if (!(cost_3_total & 0x80000000)) {
					totalBits_2 += totalBits_3;
					totalDist_2 += totalDist_3;
				}
			}

			m_cabac_rdo.depth_tu = 2;
			m_cabac_rdo.cu_update();
#endif
			
			cu_level_compare(cost_2, cost_3_total, 2);

			if (2 != pHardWare->ctu_calc.slice_type)
			{
				offsIdx = 64 + cu_x_2 / 16 + cu_y_2 / 16 * 4;
				UpdateMvpInfo(offsIdx, cost_3_total < cost_2);
			}

            cu_level_calc[2].end();
            cu_level_calc[2].ori_pos++;
        }
        //EncoderCuSplitFlag();
        //calcRDOCOST
        cost_2_total = (uint32_t)pHardWare->ctu_calc.intra_temp_1[cu_level_calc[1].temp_pos-1].m_totalCost;
#if RK_CABAC_FUNCTIONAL_TYPE
		uint32_t zscan = RasterToZscan[cu_x_1/4 + cu_y_1*4];
		int bits_split_cu_flag = (m_cabac_rdo.bits_split_cu_flag[1] + (1<<14))>>15;
		totalBits_2+=bits_split_cu_flag;
		int temp = cu_level_calc[2].RdoCostCalc(totalDist_2, totalBits_2 , pHardWare->ctu_calc.QP_lu);
		if (!(cost_1 & 0x80000000))
		{
			assert(cost_2_total == temp);
		}

		if (cost_1 <= cost_2_total)
		{
			//m_cabac_rdo.cu_best_mode[1] = 1;//0表示当前层用inter  1表示intra   2表示不用这层，用这层往下划分的
			if (!(cost_1 & 0x80000000)) {
				totalBits_1 += cu_level_calc[1].cost_best->Bits;
				totalDist_1 += cu_level_calc[1].cost_best->Distortion;
			}
		}
		else 
		{
			m_cabac_rdo.cu_best_mode[1] = 2;
			if (!(cost_2_total & 0x80000000)) {
				totalBits_1 += totalBits_2;
				totalDist_1 += totalDist_2;
			}
		}

		m_cabac_rdo.depth_tu = 1;
		m_cabac_rdo.cu_update();
#endif

		cu_level_compare(cost_1, cost_2_total, 1);

		if (2 != pHardWare->ctu_calc.slice_type)
		{
			offsIdx = 80 + cu_x_1 / 32 + cu_y_1 / 32 * 2;
			UpdateMvpInfo(offsIdx, cost_2_total < cost_1);
		}

        cu_level_calc[1].end();
        cu_level_calc[1].ori_pos++;
    }
    //EncoderCuSplitFlag();
    //calcRDOCOST
    cost_1_total = (uint32_t)pHardWare->ctu_calc.intra_temp_0.m_totalCost;

#if RK_CABAC_FUNCTIONAL_TYPE
	uint32_t zscan = RasterToZscan[0];
	int bits_split_cu_flag = (m_cabac_rdo.bits_split_cu_flag[0] + (1<<14))>>15;
	totalBits_1+=bits_split_cu_flag;
	int temp = cu_level_calc[2].RdoCostCalc(totalDist_1, totalBits_1 , pHardWare->ctu_calc.QP_lu);
	if (!(cost_0 & 0x80000000))
	{
		assert(cost_1_total == temp);
	}

	if (cost_0 <= cost_1_total)
	{
		//m_cabac_rdo.cu_best_mode[0] = 1;//0表示当前层用inter  1表示intra   2表示不用这层，用这层往下划分的
		if (!(cost_0 & 0x80000000)) {
			totalBits_1 += cu_level_calc[0].cost_best->Bits;
			totalDist_1 += cu_level_calc[0].cost_best->Distortion;
		}
	}
	else 
	{
		m_cabac_rdo.cu_best_mode[0] = 2;
		if (!(cost_1_total & 0x80000000)) {
			totalBits_1 += totalBits_2;
			totalDist_1 += totalDist_2;
		}
	}

	m_cabac_rdo.depth_tu = 0;
	m_cabac_rdo.cu_update();
#endif

    cu_level_compare(cost_0, cost_1_total, 0);
    cu_level_calc[0].end();
	//========================================================================
#else  // CTU_CALC_STATUS discription

	enum __CTU_CALC_STATUS {  // 状态机的19个状态，32路选择器实现，其它状态默认进入idle
		CTU_CALC_IDLE           = 0,    // A
		CTU_CALC_BEGIN          = 1,    // B
		CTU_CALC_PREPARE        = 2,    // C
		CTU_CALC_64_START       = 3,    // D
		CTU_CALC_32_START       = 4,    // E
		CTU_CALC_16_START       = 5,    // F
		CTU_CALC_8_START        = 6,    // G
		CTU_CALC_8_ON           = 7,    // H
		CTU_CALC_8_END          = 8,    // I
		CTU_CALC_8_16_WAIT      = 9,    // J
		CTU_CALC_8_16_COMPARE   = 10,   // K
		CTU_CALC_16_END         = 11,   // L
		CTU_CALC_16_32_WAIT     = 12,   // M
		CTU_CALC_16_32_COMPARE  = 13,   // N
		CTU_CALC_32_END         = 14,   // O
		CTU_CALC_32_64_WAIT     = 15,   // P
		CTU_CALC_32_64_COMPARE  = 16,   // Q
		CTU_CALC_64_END         = 17,   // R
		CTU_CALC_END            = 18,   // S
	}ctu_cal_flag;

	struct __CTU_CALC_PCB {   // 状态跳转的控制参数块
		bool    CTU_CMD_valid          ;
		bool    ctu_calc_init_end      ;
		bool    IME_MV_ready           ;
		bool    ori_pixel_data_ready   ;
		int8_t  ctu_w                  ;
		bool    ctu_calc_64_start_flag ;
		bool    ctu_calc_32_start_flag ;
		bool    ctu_calc_16_start_flag ;
		bool    ctu_calc_8_start_flag  ;
		bool    cu_8_calc_ready        ;
		int8_t  cu_8_num               ;
		bool    cu_16_calc_ready       ;
		bool    cu_8_16_compare_ready  ;
		int8_t  cu_16_num              ;
		bool    cu_16_end_ready        ;
		bool    cu_32_calc_ready       ;
		bool    cu_16_32_compare_ready ;
		int8_t  cu_32_num              ;
		bool    cu_32_end_ready        ;
		bool    cu_64_calc_ready       ;
		bool    cu_32_64_compare_ready ;
		bool    cu_64_end_ready        ;
		bool    ctu_end_ready          ;

	}ctu_cal_pcb;

	// initialize ctu_cal_pcb parameters
	{
		ctu_cal_pcb.CTU_CMD_valid           = false;
		ctu_cal_pcb.ctu_calc_init_end       = false;
		ctu_cal_pcb.IME_MV_ready            = false;
		ctu_cal_pcb.ori_pixel_data_ready    = false;
		ctu_cal_pcb.ctu_w                   = ctu_w;
		ctu_cal_pcb.ctu_calc_64_start_flag  = false;
		ctu_cal_pcb.ctu_calc_32_start_flag  = false;
		ctu_cal_pcb.ctu_calc_16_start_flag  = false;
		ctu_cal_pcb.ctu_calc_8_start_flag   = false;
		ctu_cal_pcb.cu_8_calc_ready         = false;
		ctu_cal_pcb.cu_8_num                = 0;
		ctu_cal_pcb.cu_16_calc_ready        = false;
		ctu_cal_pcb.cu_8_16_compare_ready   = false;
		ctu_cal_pcb.cu_16_num               = 0;
		ctu_cal_pcb.cu_16_end_ready         = false;
		ctu_cal_pcb.cu_32_calc_ready        = false;
		ctu_cal_pcb.cu_16_32_compare_ready  = false;
		ctu_cal_pcb.cu_32_num               = 0;
		ctu_cal_pcb.cu_32_end_ready         = false;
		ctu_cal_pcb.cu_64_calc_ready        = false;
		ctu_cal_pcb.cu_32_64_compare_ready  = false;
		ctu_cal_pcb.cu_64_end_ready         = false;
		ctu_cal_pcb.ctu_end_ready           = false;
	}

#define  CTU_CALC_DEBUG


	ctu_cal_flag = CTU_CALC_IDLE;   // start point
	CTU_CALC_DEBUG("\n\n-------- CTU_CALC_DEBUG: BEGIN -------------\n\n");
	while(1)
	{

		switch(ctu_cal_flag)
		{
			case CTU_CALC_IDLE:
				//---- status todo ---



				//---- status flag ---

				ctu_cal_pcb.CTU_CMD_valid = true;
				//---- next status ---

				if (ctu_cal_pcb.CTU_CMD_valid)     // 1
				{
					ctu_cal_flag = CTU_CALC_BEGIN;
					CTU_CALC_DEBUG("1: A ==> B\n");
				}
				else
				{
					ctu_cal_flag = CTU_CALC_IDLE;
					CTU_CALC_DEBUG("   A ==> A\n");
				}
				//--------------------
				break;
			case CTU_CALC_BEGIN:
				//---- status todo ---



				//---- status flag ---

				ctu_cal_pcb.ctu_calc_init_end = true;
				//---- next status ---

				if (ctu_cal_pcb.ctu_calc_init_end)  // 2
				{
					ctu_cal_flag = CTU_CALC_PREPARE;
					CTU_CALC_DEBUG("2: B ==> C\n");
				}
				else
				{
					ctu_cal_flag = CTU_CALC_BEGIN;
					CTU_CALC_DEBUG("   B ==> B\n");
				}
				//--------------------
				break;
			case CTU_CALC_PREPARE:
				//---- status todo ---
				cu_level_calc[0].depth = 0;
				cu_level_calc[0].TotalCost = 0;


				//---- status flag ---

				ctu_cal_pcb.IME_MV_ready         = true;
				ctu_cal_pcb.ori_pixel_data_ready = true;
				//---- next status ---
				if (ctu_cal_pcb.IME_MV_ready && ctu_cal_pcb.ori_pixel_data_ready && (ctu_cal_pcb.ctu_w == 64))      //3
				{

					ctu_cal_flag = CTU_CALC_64_START;
					CTU_CALC_DEBUG("3: C ==> D\n");


				}
				else if (ctu_cal_pcb.IME_MV_ready && ctu_cal_pcb.ori_pixel_data_ready && (ctu_cal_pcb.ctu_w == 32)) //4
				{

					ctu_cal_flag = CTU_CALC_32_START;
					CTU_CALC_DEBUG("   C ==> E\n");

				}
				else
				{
					ctu_cal_flag = CTU_CALC_PREPARE;
					CTU_CALC_DEBUG("C ==> C\n");
				}
				//--------------------
				break;
			case CTU_CALC_64_START:
				//---- status todo ---

				cost_0 = cu_level_calc[0].proc(0, 0, 0);

				//---- status flag ---

				ctu_cal_pcb.ctu_calc_64_start_flag = true;
				//---- next status ---

				if (ctu_cal_pcb.ctu_calc_64_start_flag)  // 5
				{
					ctu_cal_flag = CTU_CALC_32_START;
					CTU_CALC_DEBUG("5: D ==> E\n");
					totalBits_1 = 0;
					totalDist_1 = 0;

				}
				else
				{
					ctu_cal_flag = CTU_CALC_64_START;
					CTU_CALC_DEBUG("   D ==> D\n");
				}

				//--------------------


				break;
			case CTU_CALC_32_START:
				//---- status todo ---
				CTU_CALC_DEBUG("CTU_CALC_32_START..... cu_32_num = %d\n", ctu_cal_pcb.cu_32_num);
				cu_level_calc[1].depth = 1;
				cu_level_calc[1].matrix_pos = ctu_cal_pcb.cu_32_num;
				cu_x_1 = (ctu_cal_pcb.cu_32_num & 1)*32;
				cu_y_1 = (ctu_cal_pcb.cu_32_num >>1)*32;
				cost_1 = cu_level_calc[1].proc(1, cu_x_1, cu_y_1);

				if (!(cost_1 & 0x80000000)) {
					totalBits_1 += cu_level_calc[1].cost_best->Bits;
					totalDist_1 += cu_level_calc[1].cost_best->Distortion;
#ifdef LOG_INTRA_PARAMS_2_FILE
					LogIntraParams2File(cu_level_calc[1].inf_intra_proc, cu_x_1, cu_y_1);
#endif
#if TQ_RUN_IN_HWC_INTRA
					//compare coeff(after T and Q), and Recon (Y, U, V)
					compareCoeffandRecon(cu_level_calc, 1);
#endif
				}



				//---- status flag ---

				ctu_cal_pcb.ctu_calc_32_start_flag = true;
				//---- next status ---

				if (ctu_cal_pcb.ctu_calc_32_start_flag)  // 6
				{
					ctu_cal_flag = CTU_CALC_16_START;
					CTU_CALC_DEBUG("6: E ==> F\n");
					totalBits_2 = 0;
					totalDist_2 = 0;

				}
				else
				{
					ctu_cal_flag = CTU_CALC_32_START;
					CTU_CALC_DEBUG("   E ==> E\n");
				}

				//--------------------


				break;
			case CTU_CALC_16_START:
				//---- status todo ---
				CTU_CALC_DEBUG("CTU_CALC_16_START..... cu_16_num = %d\n", ctu_cal_pcb.cu_16_num);
				cu_level_calc[2].depth = 2;
				cu_level_calc[2].matrix_pos = ctu_cal_pcb.cu_16_num;
				cu_x_2 = cu_x_1 + (ctu_cal_pcb.cu_16_num & 1)*16;
				cu_y_2 = cu_y_1 + (ctu_cal_pcb.cu_16_num >>1)*16;

				cost_2 = cu_level_calc[2].proc(2, cu_x_2, cu_y_2);

				if (!(cost_2 & 0x80000000)) {
					totalBits_2 += cu_level_calc[2].cost_best->Bits;
					totalDist_2 += cu_level_calc[2].cost_best->Distortion;

#ifdef LOG_INTRA_PARAMS_2_FILE
					LogIntraParams2File(cu_level_calc[2].inf_intra_proc, cu_x_2, cu_y_2);
#endif
#if TQ_RUN_IN_HWC_INTRA
					//compare coeff(after T and Q), and Recon (Y, U, V)
					compareCoeffandRecon(cu_level_calc, 2);
#endif
				}
				//---- status flag ---

				ctu_cal_pcb.ctu_calc_16_start_flag = true;
				//---- next status ---

				if (ctu_cal_pcb.ctu_calc_16_start_flag && (ctu_w != 32))  // 7
				{
					ctu_cal_flag = CTU_CALC_8_START;
					CTU_CALC_DEBUG("7: F ==> G\n");
					totalBits_3 = 0;
					totalDist_3 = 0;
				}
				else
				{
					ctu_cal_flag = CTU_CALC_16_START;
					CTU_CALC_DEBUG("   F ==> F\n");
				}
				//--------------------

				break;
			case CTU_CALC_8_START:
				//---- status todo ---
				CTU_CALC_DEBUG("CTU_CALC_8_START..... cu_8_num = %d\n", ctu_cal_pcb.cu_8_num);
				cu_level_calc[3].depth = 3;
				cu_level_calc[3].matrix_pos = ctu_cal_pcb.cu_8_num;
				cu_x_3 = cu_x_2 + (ctu_cal_pcb.cu_8_num & 1)*8;
				cu_y_3 = cu_y_2 + (ctu_cal_pcb.cu_8_num >>1)*8;

				cost_3 = cu_level_calc[3].proc(3, cu_x_3, cu_y_3);

				if (!(cost_3 & 0x80000000)) {
					totalBits_3 += cu_level_calc[3].cost_best->Bits;
					totalDist_3 += cu_level_calc[3].cost_best->Distortion;

#ifdef LOG_INTRA_PARAMS_2_FILE
					LogIntraParams2File(cu_level_calc[3].inf_intra_proc, cu_x_3, cu_y_3);
#endif
#if 1 //wait for 4x4 intra to be done, added by lks
					//compare coeff(after T and Q), and Recon (Y, U, V)
					compareCoeffandRecon8x8(cu_level_calc, cu_level_calc[3].choose4x4split);
#endif
				}
				//---- status flag ---

				ctu_cal_pcb.ctu_calc_8_start_flag = true;
				//---- next status ---

				if (ctu_cal_pcb.ctu_calc_8_start_flag)  // 8
				{
					ctu_cal_flag = CTU_CALC_8_ON;
					CTU_CALC_DEBUG("8: G ==> H\n");
				}
				else
				{
					ctu_cal_flag = CTU_CALC_8_START;
					CTU_CALC_DEBUG("   G ==> G\n");
				}

				//--------------------


				break;
			case CTU_CALC_8_ON:
				//---- status todo ---



				//---- status flag ---

				ctu_cal_pcb.cu_8_calc_ready = true;
				//---- next status ---
				if (ctu_cal_pcb.cu_8_calc_ready)  // 9
				{
					ctu_cal_flag = CTU_CALC_8_END;
					CTU_CALC_DEBUG("9: H ==> I\n");
				}
				else
				{
					ctu_cal_flag = CTU_CALC_8_ON;
					CTU_CALC_DEBUG("   H ==> H\n");
				}


				//--------------------


				break;
			case CTU_CALC_8_END:
				//---- status todo ---

				cu_level_calc[3].end();
				cu_level_calc[3].ori_pos++;

				//---- status flag ---


				//---- next status ---
				if (ctu_cal_pcb.cu_8_num != 3)       // 10
				{
					CTU_CALC_DEBUG("10: I ==> G\n");
					ctu_cal_flag = CTU_CALC_8_START;
					ctu_cal_pcb.cu_8_num++;

				}
				else if (ctu_cal_pcb.cu_8_num == 3)  // 11
				{
					CTU_CALC_DEBUG("11: I ==> J\n");
					ctu_cal_flag = CTU_CALC_8_16_WAIT;
					ctu_cal_pcb.cu_8_num = 0;

				}
				else
				{
					assert(ctu_cal_pcb.cu_8_num>=0 && ctu_cal_pcb.cu_8_num<=3);
					ctu_cal_flag = CTU_CALC_8_END;
				}

				//--------------------

				break;
			case CTU_CALC_8_16_WAIT:
				//---- status todo ---



				//---- status flag ---

				ctu_cal_pcb.cu_16_calc_ready = true;
				//---- next status ---

				if (ctu_cal_pcb.cu_16_calc_ready)   // 12
				{
					ctu_cal_flag = CTU_CALC_8_16_COMPARE;
					CTU_CALC_DEBUG("12: J ==> K\n");
				}
				else
				{

					ctu_cal_flag = CTU_CALC_8_16_WAIT;
					CTU_CALC_DEBUG("   J ==> J\n");
				}

				//--------------------


				break;
			case CTU_CALC_8_16_COMPARE:
				//---- status todo ---

				//EncoderCuSplitFlag();
				//calcRDOCOST
				cost_3_total = (uint32_t)pHardWare->ctu_calc.intra_temp_16[cu_level_calc[2].temp_pos-1].m_totalCost;
				cu_level_compare(cost_2, cost_3_total, 2);


				//---- status flag ---

				ctu_cal_pcb.cu_8_16_compare_ready = true;
				//---- next status ---
				if (ctu_cal_pcb.cu_8_16_compare_ready) // 13
				{
					ctu_cal_flag = CTU_CALC_16_END;
					CTU_CALC_DEBUG("13: K ==> L\n");
				}
				else
				{
					ctu_cal_flag = CTU_CALC_8_16_COMPARE;
					CTU_CALC_DEBUG("   K ==> K\n");
				}
				//--------------------

				break;
			case CTU_CALC_16_END:
				//---- status todo ---

				cu_level_calc[2].end();
				cu_level_calc[2].ori_pos++;

				//---- status flag ---

				ctu_cal_pcb.cu_16_end_ready = true;
				//---- next status ---
				if (ctu_cal_pcb.cu_16_end_ready && (ctu_cal_pcb.cu_16_num != 3))  // 14
				{
					CTU_CALC_DEBUG("14: L ==> F\n");
					ctu_cal_flag = CTU_CALC_16_START;
					ctu_cal_pcb.cu_16_num++;

				}
				else if((ctu_cal_pcb.cu_16_end_ready && (ctu_cal_pcb.cu_16_num == 3))) //15
				{
					CTU_CALC_DEBUG("15: L ==> M\n");
					ctu_cal_flag = CTU_CALC_16_32_WAIT;
					ctu_cal_pcb.cu_16_num = 0;
				}
				else
				{
					assert(ctu_cal_pcb.cu_16_num>=0 && ctu_cal_pcb.cu_16_num<=3);
					ctu_cal_flag = CTU_CALC_16_END;
					CTU_CALC_DEBUG("   L ==> L\n");
				}
				//--------------------
				break;
			case CTU_CALC_16_32_WAIT:
				//---- status todo ---



				//---- status flag ---

				ctu_cal_pcb.cu_32_calc_ready = true;
				//---- next status ---
				if (ctu_cal_pcb.cu_32_calc_ready)    // 16
				{

					ctu_cal_flag = CTU_CALC_16_32_COMPARE;
					CTU_CALC_DEBUG("16: M ==> N\n");
				}
				else
				{

					ctu_cal_flag = CTU_CALC_16_32_WAIT;
					CTU_CALC_DEBUG("   M ==> M\n");
				}
				//--------------------
				break;

			case CTU_CALC_16_32_COMPARE:
				//---- status todo ---

				//EncoderCuSplitFlag();
				//calcRDOCOST
				cost_2_total = (uint32_t)pHardWare->ctu_calc.intra_temp_32[cu_level_calc[1].temp_pos-1].m_totalCost;
				cu_level_compare(cost_1, cost_2_total, 1);

				//---- status flag ---

				ctu_cal_pcb.cu_16_32_compare_ready  = true;
				//---- next status ---
				if (ctu_cal_pcb.cu_16_32_compare_ready )  // 17
				{

					ctu_cal_flag = CTU_CALC_32_END;
					CTU_CALC_DEBUG("17: N ==> O\n");
				}
				else
				{
					ctu_cal_flag = CTU_CALC_16_32_COMPARE;
					CTU_CALC_DEBUG("   N ==> N\n");
				}
				//--------------------
				break;

			case CTU_CALC_32_END:
				//---- status todo ---

				cu_level_calc[1].end();
				cu_level_calc[1].ori_pos++;

				//---- status flag ---

				ctu_cal_pcb.cu_32_end_ready = true;
				//---- next status ---
				if (ctu_cal_pcb.cu_32_end_ready && (ctu_cal_pcb.cu_32_num != 3) &&(ctu_cal_pcb.ctu_w != 32))  // 18
				{
					CTU_CALC_DEBUG("18: O ==> E\n");
					ctu_cal_flag = CTU_CALC_32_START;
					ctu_cal_pcb.cu_32_num++;
				}
				else if(ctu_cal_pcb.cu_32_end_ready && (ctu_cal_pcb.cu_32_num == 3) &&(ctu_cal_pcb.ctu_w != 32))  // 19
				{
					CTU_CALC_DEBUG("19: O ==> P\n");
					ctu_cal_flag = CTU_CALC_32_64_WAIT;
					ctu_cal_pcb.cu_32_num = 0;
				}
				else if((ctu_cal_pcb.ctu_w == 32) && ctu_cal_pcb.cu_32_end_ready)  // 21
				{
					CTU_CALC_DEBUG("21:   O ==> S\n");
					ctu_cal_flag = CTU_CALC_END;
				}
				else
				{
					assert(ctu_cal_pcb.cu_32_num>=0 && ctu_cal_pcb.cu_32_num<=3);

					ctu_cal_flag = CTU_CALC_32_END;
					CTU_CALC_DEBUG("   O ==> O\n");
				}
				//--------------------
				break;

			case CTU_CALC_32_64_WAIT:
				//---- status todo ---



				//---- status flag ---

				ctu_cal_pcb.cu_64_calc_ready  = true;
				//---- next status ---
				if (ctu_cal_pcb.cu_64_calc_ready )  // 20
				{

					ctu_cal_flag = CTU_CALC_32_64_COMPARE;
					CTU_CALC_DEBUG("20: P ==> Q\n");
				}
				else
				{
					ctu_cal_flag = CTU_CALC_32_64_WAIT;
					CTU_CALC_DEBUG("   P ==> P\n");
				}
				//--------------------
				break;

			case CTU_CALC_32_64_COMPARE:
				//---- status todo ---



				//EncoderCuSplitFlag();
				//calcRDOCOST
				cost_1_total = (uint32_t)pHardWare->ctu_calc.intra_temp_64.m_totalCost;
				cu_level_compare(cost_0, cost_1_total, 0);


				//---- status flag ---

				ctu_cal_pcb.cu_32_64_compare_ready  = true;
				//---- next status ---
				if (ctu_cal_pcb.cu_32_64_compare_ready )  // 22
				{

					ctu_cal_flag = CTU_CALC_64_END;
					CTU_CALC_DEBUG("22: Q ==> R\n");
				}
				else
				{
					ctu_cal_flag = CTU_CALC_32_64_COMPARE;
					CTU_CALC_DEBUG("   Q ==> Q\n");
				}
				//--------------------
				break;

			case CTU_CALC_64_END:
				//---- status todo ---

				cu_level_calc[0].end();

				//---- status flag ---
				ctu_cal_pcb.cu_64_end_ready  = true;
				//---- next status ---
				if (ctu_cal_pcb.cu_64_end_ready )  // 23
				{

					ctu_cal_flag = CTU_CALC_END;
					CTU_CALC_DEBUG("23: R ==> S\n");
				}
				else
				{
					ctu_cal_flag = CTU_CALC_64_END;
					CTU_CALC_DEBUG("   R ==> R\n");
				}


				//--------------------
				break;

			case CTU_CALC_END:
				//---- status todo ---



				//---- status flag ---

				ctu_cal_pcb.ctu_end_ready  = true;
				//---- next status ---
				if (ctu_cal_pcb.ctu_end_ready )  // 23
				{
					ctu_cal_flag = CTU_CALC_IDLE;
					CTU_CALC_DEBUG("24: S ==> A\n");
					goto __ExitStatus;
				}
				else
				{
					ctu_cal_flag = CTU_CALC_END;
					CTU_CALC_DEBUG("   S ==> S\n");
				}
				//--------------------
				break;

			default:
				assert("This status is not exist");   // others states exit
				break;
		}
	}

__ExitStatus:
	CTU_CALC_DEBUG("\n-------- CTU_CALC_DEBUG: END -------------\n\n");
#endif
	//=======================================================================
    /* end */
    end();

    /* for test */
    compare();
}

void CTU_CALC::model_cfg(char *cmd)
{
    char    ctu_calc_cmdf[]        =  "ctu_calc_cmd_file = ";
    char    ctu_calc_recon_y[]     =  "ctu_calc_recon_y_file = ";
    char    ctu_calc_recon_uv[]    =  "ctu_calc_recon_uv_file = ";
    if (0);
    OPT(cmd, ctu_calc_cmdf)     { fp_ctu_calc_cmdf = fopen(cmd + sizeof(ctu_calc_cmdf)-1,"wb+");}
    OPT(cmd, ctu_calc_recon_y)  { fp_ctu_calc_recon_y = fopen(cmd + sizeof(ctu_calc_cmdf)-1,"wb+");}
    OPT(cmd, ctu_calc_recon_uv) { fp_ctu_calc_recon_uv = fopen(cmd + sizeof(ctu_calc_cmdf)-1,"wb+");}
    else
        ;

}

void CTU_CALC::ctu_status_calc()
{
}

void cuData::init(uint8_t w)
{
    CoeffY = (int16_t *)malloc(w*w*2);
    CoeffU = (int16_t *)malloc(w*w*2/4);
    CoeffV = (int16_t *)malloc(w*w*2/4);

    ReconY = (uint8_t *)malloc(w*w);
    ReconU = (uint8_t *)malloc(w*w/4);
    ReconV = (uint8_t *)malloc(w*w/4);
    cbfY = (uint8_t *)malloc((w/4)*(w/4));
    mv = (MV_INFO *)malloc(sizeof(MV_INFO)*(w/8)*(w/8));
    cuPartSize = (uint8_t *)malloc((w/8)*(w/8));
    cuSkipFlag = (uint8_t *)malloc((w/8)*(w/8));
    cuPredMode = (uint8_t *)malloc((w/8)*(w/8));
}

void cuData::deinit()
{
    free(CoeffY);
    free(CoeffU);
    free(CoeffV);

    free(ReconY);
    free(ReconU);
    free(ReconV);
    free(cbfY);
    free(mv);
    free(cuPartSize);
    free(cuSkipFlag);
    free(cuPredMode);
}
