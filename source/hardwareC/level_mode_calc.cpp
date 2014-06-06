#include <stdio.h>
#include "level_mode_calc.h"
#include "hardwareC.h"
#include "inter.h"

#ifdef RK_INTRA_4x4_PRED
extern uint32_t g_rk_raster2zscan_depth_4[256];
extern FILE* g_fp_result_rk;
extern FILE* g_fp_4x4_params_rk;
#endif

CU_LEVEL_CALC::CU_LEVEL_CALC()
{
}

CU_LEVEL_CALC::~CU_LEVEL_CALC()
{
}

void CU_LEVEL_CALC::init(uint8_t size)
{
    m_size = size;
    cu_w = size;
#if TQ_RUN_IN_HWC_INTRA
	m_hevcQT = new hevcQT;
#endif

#ifdef RK_INTRA_PRED
	m_rkIntraPred = new Rk_IntraPred;
	inf_intra_proc.size = m_size;
	inf_intra.bNeighborFlags = (bool *)malloc(33*sizeof(bool));

	inf_intra4x4.bNeighborFlags = (bool*)X265_MALLOC(bool, 5);
	inf_intra4x4.fenc 			= (uint8_t*)X265_MALLOC(uint8_t, 4*4);
	inf_intra4x4.reconEdgePixel = (uint8_t*)X265_MALLOC(uint8_t, 8 + 8 + 1);

	inf_intra4x4.pred			= (uint8_t*)X265_MALLOC(uint8_t, 4*4);
	inf_intra4x4.resi			= (int16_t*)X265_MALLOC(int16_t, 4*4);

#endif

#if TQ_RUN_IN_HWC_INTRA //added by lks
	for (int k=0; k<3; k++)
	{
		int tmpSize = k==0 ? m_size: (m_size/2);
		inf_tq_total[k].resi = (int16_t *)malloc(tmpSize*tmpSize * 2);
		inf_tq_total[k].oriResi = (int16_t *)malloc(tmpSize*tmpSize * 2);
		inf_tq_total[k].outResi = (int16_t *)malloc(tmpSize*tmpSize * 2);
		inf_recon_total[k].Recon = (uint8_t*)malloc(tmpSize*tmpSize * 1);
	}

	for(int i=0; i<6; i++)
	{
		//[0~3]: Y,  [4]: U, [5]: V
		coeff4x4[i] = (int16_t*)malloc(4*4 * 2);
		resi4x4[i] =  (int16_t*)malloc(4*4 * 2);
		recon4x4[i] = (uint8_t*)malloc(4*4 * 1);
	}
#endif

    inf_intra.pred = (uint8_t *)malloc(m_size*m_size);
    inf_intra.resi = (int16_t *)malloc(m_size*m_size*2);

    inf_tq.outResi = (int16_t *)malloc(m_size*m_size*2);
    inf_tq.oriResi = (int16_t *)malloc(m_size*m_size*2);

    inf_me.predY = (uint8_t *)malloc(m_size*m_size);
    inf_me.predU = (uint8_t *)malloc(m_size*m_size/4);
    inf_me.predV = (uint8_t *)malloc(m_size*m_size/4);

    inf_me.resiY = (int16_t *)malloc(m_size*m_size*2);
    inf_me.resiU = (int16_t *)malloc(m_size*m_size*2/4);
    inf_me.resiV = (int16_t *)malloc(m_size*m_size*2/4);

    cost_inter.recon_y = (uint8_t *)malloc(m_size*m_size);
    cost_inter.recon_u = (uint8_t *)malloc(m_size*m_size/4);
    cost_inter.recon_v = (uint8_t *)malloc(m_size*m_size/4);

    cost_inter.resi_y = (int16_t *)malloc(m_size*m_size*2);
    cost_inter.resi_u = (int16_t *)malloc(m_size*m_size*2/4);
    cost_inter.resi_v = (int16_t *)malloc(m_size*m_size*2/4);

    cost_intra.recon_y = (uint8_t *)malloc(m_size*m_size);
    cost_intra.recon_u = (uint8_t *)malloc(m_size*m_size/4);
    cost_intra.recon_v = (uint8_t *)malloc(m_size*m_size/4);

    cost_intra.resi_y = (int16_t *)malloc(m_size*m_size*2);
    cost_intra.resi_u = (int16_t *)malloc(m_size*m_size*2/4);
    cost_intra.resi_v = (int16_t *)malloc(m_size*m_size*2/4);

    cost_temp.recon_y = (uint8_t *)malloc(m_size*m_size);
    cost_temp.recon_u = (uint8_t *)malloc(m_size*m_size/4);
    cost_temp.recon_v = (uint8_t *)malloc(m_size*m_size/4);

    cost_temp.resi_y = (int16_t *)malloc(m_size*m_size*2);
    cost_temp.resi_u = (int16_t *)malloc(m_size*m_size*2/4);
    cost_temp.resi_v = (int16_t *)malloc(m_size*m_size*2/4);
    cu_matrix_data = (cuData *)malloc(4*sizeof(cuData));
	for(int i=0; i<((cu_w == 64) ? 1 : 4); i++)
		cu_matrix_data[i].init(cu_w);
}

void CU_LEVEL_CALC::deinit()
{
#ifdef RK_INTRA_PRED
	delete m_rkIntraPred;
	free(inf_intra.bNeighborFlags);
	X265_FREE(inf_intra4x4.bNeighborFlags);
	X265_FREE(inf_intra4x4.fenc);
	X265_FREE(inf_intra4x4.reconEdgePixel);
	X265_FREE(inf_intra4x4.pred);
	X265_FREE(inf_intra4x4.resi);
#endif
#if TQ_RUN_IN_HWC_INTRA
	delete m_hevcQT;
#endif

#if TQ_RUN_IN_HWC_INTRA //added by lks
	for (int k=0; k<3; k++)
	{
		free(inf_tq_total[k].resi);		inf_tq_total[k].resi = NULL;
		free(inf_tq_total[k].oriResi);  inf_tq_total[k].oriResi = NULL;
		free(inf_tq_total[k].outResi);  inf_tq_total[k].outResi = NULL;
		free(inf_recon_total[k].Recon); inf_recon_total[k].Recon = NULL;
	}

	for(int i=0; i<6; i++)
	{
		//[0~3]: Y,  [4]: U, [5]: V
		free(coeff4x4[i]); 	coeff4x4[i] = NULL;
		free(resi4x4[i]); 	resi4x4[i] = NULL;
		free(recon4x4[i]); 	recon4x4[i] = NULL;
	}
#endif

    uint8_t i;
    free(inf_intra.pred);
    free(inf_intra.resi);

    free(inf_tq.outResi);
    free(inf_tq.oriResi);

    free(inf_me.predY);
    free(inf_me.predU);
    free(inf_me.predV);

    free(inf_me.resiY);
    free(inf_me.resiU);
    free(inf_me.resiV);

    free(cost_inter.recon_y);
    free(cost_inter.recon_u);
    free(cost_inter.recon_v);

    free(cost_inter.resi_y);
    free(cost_inter.resi_u);
    free(cost_inter.resi_v);

    free(cost_intra.recon_y);
    free(cost_intra.recon_u);
    free(cost_intra.recon_v);

    free(cost_intra.resi_y);
    free(cost_intra.resi_u);
    free(cost_intra.resi_v);

    free(cost_temp.recon_y);
    free(cost_temp.recon_u);
    free(cost_temp.recon_v);

    free(cost_temp.resi_y);
    free(cost_temp.resi_u);
    free(cost_temp.resi_v);

	for(i=0; i<((cu_w == 64)? 1 : 4); i++)
        cu_matrix_data[i].deinit();
	free(cu_matrix_data);

}


void CU_LEVEL_CALC::intra_neighbour_flag_descion()
{
    if(cu_w == 64)
        return;

    uint8_t constrain_intra = pHardWare->hw_cfg.constrained_intra_enable;

    /* intra */
    unsigned char valid_flag_bl = (!x_pos) ? 1 : pHardWare->ctu_calc.cu_data_valid[((y_pos + cu_w) >> 3) * 8 + (x_pos >> 3) - 1];
    unsigned char valid_flag_tr = (!y_pos) ? 1 : pHardWare->ctu_calc.cu_data_valid[((y_pos >> 3) - 1) * 8 + ((x_pos + cu_w) >> 3)];

    int cand_top_left = pHardWare->ctu_calc.valid_left_top || x_pos || y_pos;

    int left_valid = pHardWare->ctu_calc.valid_left || x_pos;
    int top_valid  = pHardWare->ctu_calc.valid_top || y_pos;
    int top_left_valid = left_valid && top_valid && cand_top_left;

    int cand_up_right = ((x_pos + cu_w) == (pHardWare->ctu_calc.ctu_w)) ? pHardWare->ctu_calc.valid_top_right && !y_pos && y_in_pic : top_valid;

    int bottom_left_valid = left_valid && !((y_pos + cu_w) == pHardWare->ctu_calc.ctu_w) && ((y_in_pic + cu_w) < pHardWare->ctu_calc.pic_h) && valid_flag_bl;
    int top_right_valid = cand_up_right && ((x_in_pic + cu_w) < pHardWare->ctu_calc.pic_w) && valid_flag_tr;

    uint8_t m;

    int bottom_left_len = CLIP(pHardWare->ctu_calc.pic_h - (y_in_pic + cu_w), 0, cu_w);
    int top_right_len = CLIP(pHardWare->ctu_calc.pic_w - (x_in_pic + cu_w), 0, cu_w);

    memset(inf_intra.NeighborFlags, 0x0, 17);

    /* bottom_left */
    if(bottom_left_valid) {
        bottom_left_valid = 0;
        for (m=0; m<bottom_left_len/8; m++) {
            if(constrain_intra) {
                inf_intra.NeighborFlags[8 - (cu_w/8) - m - 1] = pHardWare->ctu_calc.L_cu_type[(x_pos/8 - 1) - (y_pos/8 + cu_w/8 + m) + 8];
                bottom_left_valid += inf_intra.NeighborFlags[8 - cu_w/8 - m - 1];
            }
            else {
                inf_intra.NeighborFlags[8 - (cu_w/8) - m - 1] = 1;
                bottom_left_valid += 1;
            }
        }
    }
    else {
        for (m=0; m<cu_w/8; m++)
            inf_intra.NeighborFlags[8 - (cu_w/8) - m - 1] = 0;
    }

    /* left */
    if(left_valid) {
        left_valid = 0;
        if(constrain_intra) {
            for (m=0; m<cu_w/8; m++) {
                inf_intra.NeighborFlags[8 - m - 1] = pHardWare->ctu_calc.L_cu_type[(x_pos/8 - 1) - (y_pos/8 + m) + 8];
                left_valid += inf_intra.NeighborFlags[8 - m - 1];
            }
        }
        else {
            for (m=0; m<cu_w/8; m++) {
                inf_intra.NeighborFlags[8 - m - 1] = 1;
                left_valid += 1;
            }
        }
    }
    else {
        for (m=0; m<cu_w/8; m++)
            inf_intra.NeighborFlags[8 - m - 1] = 0;
    }


    /* top_left */
    if(top_left_valid) {
        top_left_valid = 0;
        if(constrain_intra) {
            inf_intra.NeighborFlags[8] = pHardWare->ctu_calc.L_cu_type[(x_pos/8) - (y_pos/8) + 8];
            top_left_valid += inf_intra.NeighborFlags[8];
        }
        else {
            inf_intra.NeighborFlags[8] = 1;
            top_left_valid += 1;
        }
    }
    else {
        inf_intra.NeighborFlags[8] = 0;
    }


    /* top */
    if(top_valid) {
        top_valid = 0;
        if(constrain_intra) {
            for(m=0; m<cu_w/8; m++) {
                inf_intra.NeighborFlags[9 + m] = pHardWare->ctu_calc.L_cu_type[(x_pos/8 + m) - (y_pos/8 - 1) + 8];
                top_valid += inf_intra.NeighborFlags[8 + m + 1];
            }
        }
        else {
            for(m=0; m<cu_w/8; m++) {
                inf_intra.NeighborFlags[9 + m] = 1;
                top_valid += 1;
            }
        }
    }
    else {
        for(m=0; m<cu_w/8; m++)
            inf_intra.NeighborFlags[9 + m] = 0;
    }

    /* top_right */
    if(top_right_valid) {
        top_right_valid = 0;
        for(m=0; m<top_right_len/8; m++) {
            if(constrain_intra) {
                inf_intra.NeighborFlags[9 + m + cu_w/8] = pHardWare->ctu_calc.L_cu_type[(x_pos/8 + cu_w/8 + m) - (y_pos/8 - 1) + 8];
                top_right_valid += inf_intra.NeighborFlags[8 + m + 1 + cu_w/8];
            }
            else {
                inf_intra.NeighborFlags[9 + m + cu_w/8] = 1;
                top_right_valid += 1;
            }
        }
    }
    else {
        for(m=0; m<cu_w/8; m++)
            inf_intra.NeighborFlags[9 + m + cu_w/8] = 0;
    }

    inf_intra.numintraNeighbor = (uint8_t)(top_valid + top_right_valid + top_left_valid + left_valid + bottom_left_valid);

}
void CU_LEVEL_CALC::begin()
{
    uint8_t m;
    uint32_t ctu_w = pHardWare->ctu_calc.ctu_w;
    uint32_t pos;

    x_in_pic = pHardWare->ctu_calc.ctu_x * pHardWare->ctu_calc.ctu_w + x_pos;
    y_in_pic = pHardWare->ctu_calc.ctu_y * pHardWare->ctu_calc.ctu_w + y_pos;

    /* copy ori data */
    pos = x_pos + y_pos*64;
    for(m=0; m<cu_w; m++)
        memcpy(curr_cu_y + m*cu_w, pHardWare->ctu_calc.input_curr_y + pos + m*64, cu_w);
    pos = x_pos/2 + (y_pos/2)*32;
    for(m=0; m<cu_w/2; m++) {
        memcpy(curr_cu_u + m*cu_w/2, pHardWare->ctu_calc.input_curr_u + pos + m*32, cu_w/2);
        memcpy(curr_cu_v + m*cu_w/2, pHardWare->ctu_calc.input_curr_v + pos + m*32, cu_w/2);
    }

    /* copy L buf data  */
    memcpy(intra_buf_y + 64 - cu_w,   pHardWare->ctu_calc.L_intra_buf_y + 64 + x_pos   - (y_pos + cu_w),   2*cu_w + 1);
    memcpy(intra_buf_u + 32 - cu_w/2, pHardWare->ctu_calc.L_intra_buf_u + 32 + x_pos/2 - (y_pos + cu_w)/2, 2*cu_w/2 + 1);
    memcpy(intra_buf_v + 32 - cu_w/2, pHardWare->ctu_calc.L_intra_buf_v + 32 + x_pos/2 - (y_pos + cu_w)/2, 2*cu_w/2 + 1);
    memcpy(cu_type + 8 - cu_w/8,      pHardWare->ctu_calc.L_cu_type + 8 + x_pos/8 - (y_pos + cu_w)/8, 2*cu_w/8 + 1);
    if((!(x_pos + cu_w >= pHardWare->ctu_calc.ctu_w)) || (cu_w != 64)) {
        memcpy(intra_buf_y + 65 + cu_w,   pHardWare->ctu_calc.L_intra_buf_y + 64 + (x_pos + cu_w)   + 1 - (y_pos),   cu_w);
        memcpy(intra_buf_u + 33 + cu_w/2, pHardWare->ctu_calc.L_intra_buf_u + 32 + (x_pos + cu_w)/2 + 1 - (y_pos/2), cu_w/2);
        memcpy(intra_buf_v + 33 + cu_w/2, pHardWare->ctu_calc.L_intra_buf_v + 32 + (x_pos + cu_w)/2 + 1 - (y_pos/2), cu_w/2);
        memcpy(cu_type + 9 + cu_w/8,      pHardWare->ctu_calc.L_cu_type + 8 + (x_pos + cu_w)/8 + 1 - (y_pos/8), cu_w/8);
    }
    if(!(y_pos + cu_w >= pHardWare->ctu_calc.ctu_w)) {
        memcpy(intra_buf_y + 64 - cu_w*2, pHardWare->ctu_calc.L_intra_buf_y + 64 + x_pos   - (y_pos + 2*cu_w),   cu_w);
        memcpy(intra_buf_u + 32 - cu_w,   pHardWare->ctu_calc.L_intra_buf_u + 32 + x_pos/2 - (y_pos + 2*cu_w)/2, cu_w/2);
        memcpy(intra_buf_v + 32 - cu_w,   pHardWare->ctu_calc.L_intra_buf_v + 32 + x_pos/2 - (y_pos + 2*cu_w)/2, cu_w/2);
        memcpy(cu_type + 8 - 2*cu_w/8,    pHardWare->ctu_calc.L_cu_type + 8 + (x_pos + cu_w)/8 + 1 - (y_pos/8), cu_w/8);
    }

    intra_neighbour_flag_descion();

    /* intra_proc interface */
    inf_intra_proc.reconEdgePixelY = intra_buf_y + 64;
    inf_intra_proc.reconEdgePixelU = intra_buf_u + 32;
    inf_intra_proc.reconEdgePixelV = intra_buf_v + 32;
    inf_intra_proc.bNeighborFlags  = inf_intra.NeighborFlags + 8;

    inf_intra_proc.fencY = curr_cu_y;
    inf_intra_proc.fencU = curr_cu_u;
    inf_intra_proc.fencV = curr_cu_v;

    inf_intra_proc.numIntraNeighbor = inf_intra.numintraNeighbor;
    inf_intra_proc.size = cu_w;
    inf_intra_proc.useStrongIntraSmoothing = pHardWare->hw_cfg.strong_intra_smoothing_enable;
}

void CU_LEVEL_CALC::end()
{
    uint8_t m, n;
    cuData  *cu_src;

    cu_src = &cu_matrix_data[matrix_pos];
    /* Y */
    for(m=0; m<cu_w; m++){
        pHardWare->ctu_calc.L_intra_buf_y[(x_pos + cu_w - 1) - (y_pos + m) + 64] = cu_src->ReconY[m*cu_w + cu_w - 1];
        pHardWare->ctu_calc.L_intra_buf_y[(x_pos + m) - (y_pos + cu_w - 1) + 64] = cu_src->ReconY[(cu_w - 1)*cu_w + m];
     }

    /* U */
    for(m=0; m<cu_w/2; m++){
        pHardWare->ctu_calc.L_intra_buf_u[(x_pos/2 + cu_w/2 - 1) - (y_pos/2 + m) + 32] = cu_src->ReconU[m*cu_w/2 + cu_w/2 - 1];
        pHardWare->ctu_calc.L_intra_buf_u[(x_pos/2 + m) - (y_pos/2 + cu_w/2 - 1) + 32] = cu_src->ReconU[(cu_w/2 - 1)*cu_w/2 + m];
    }

    /* V */
    for(m=0; m<cu_w/2; m++){
        pHardWare->ctu_calc.L_intra_buf_v[(x_pos/2 + cu_w/2 - 1) - (y_pos/2 + m) + 32] = cu_src->ReconV[m*cu_w/2 + cu_w/2 - 1];
        pHardWare->ctu_calc.L_intra_buf_v[(x_pos/2 + m) - (y_pos/2 + cu_w/2 - 1) + 32] = cu_src->ReconV[(cu_w/2 - 1)*cu_w/2 + m];
    }

    /* cu_type */
    for(m=0; m<cu_w/8; m++) {
        pHardWare->ctu_calc.L_cu_type[(x_pos/8 + cu_w/8 - 1) - (y_pos/8 + m) + 8] = cu_src->cuPredMode[m*cu_w/8 + cu_w/8 - 1];
        pHardWare->ctu_calc.L_cu_type[(x_pos/8 + m) - (y_pos/8 + cu_w/8 - 1) + 8] = cu_src->cuPredMode[(cu_w/8 - 1)*cu_w/8 + m];
    }

    /* cu_data_valid flag */
    for(m=0; m<cu_w/8; m++) {
        for(n=0; n<cu_w/8; n++) {
            pHardWare->ctu_calc.cu_data_valid[(y_pos/8 + m)*8 + x_pos/8 + n] = 1;
        }
    }

    /* TODO inter relate */
}

void CU_LEVEL_CALC::BS_calc()
{
}

void CU_LEVEL_CALC::recon()
{

    uint32_t len = inf_recon.size*inf_recon.size;
    uint32_t j;
    uint32_t SSE = 0;
    int16_t  pixel;

    for(j=0; j<len; j++) {
        inf_recon.Recon[j] = (uint8_t)CLIP(inf_recon.resi[j] + inf_recon.pred[j], 0, 255);
        pixel = inf_recon.Recon[j] - inf_recon.ori[j];
        SSE += pixel * pixel;
    }

    inf_recon.SSE = SSE;
}

void CU_LEVEL_CALC::inter_proc()
{

    inf_recon.resi = inf_tq.outResi;
    //ME()

    /*Y*/
    //TQ();
    //CABAC();

    inf_recon.pred = inf_me.predY;
    inf_recon.size = m_size;
    inf_recon.Recon = cost_intra.recon_y;
    //Recon();
    //RDOCostCalc(inf_recon.SSE, QP, CABAC.Bits);

    /*U*/
    //TQ();
    //CABAC();

    inf_recon.pred = inf_me.predU;
    inf_recon.size = m_size/2;
    inf_recon.Recon = cost_intra.recon_u;
    //Recon();
    //RDOCostCalc(inf_recon.SSE, QP, CABAC.Bits);

    /*V*/
    //TQ();
    //CABAC();

    inf_recon.pred = inf_me.predV;
    inf_recon.Recon = cost_intra.recon_v;
    //Recon();
    //RDOCostCalc(inf_recon.SSE, QP, CABAC.Bits);
}

void CU_LEVEL_CALC::intra_proc()
{
    if(cu_w == 64)
        return;

	uint8_t qpY = pHardWare->ctu_calc.QP_lu;
	uint8_t qpU = pHardWare->ctu_calc.QP_cb;
	uint8_t qpV = pHardWare->ctu_calc.QP_cr;
	uint8_t sliceType = pHardWare->ctu_calc.slice_type;

#ifdef RK_CABAC
	uint32_t zscan = g_rk_raster2zscan_depth_4[x_pos/4 + y_pos*4];
	uint32_t totalBitsDepth = g_intra_depth_total_bits[depth][zscan];
	uint32_t totalBits8x8 = g_intra_depth_total_bits[3][zscan];
	uint32_t totalBits4x4 = g_intra_depth_total_bits[4][zscan];
#else
	uint32_t totalBitsDepth = 0;
	uint32_t totalBits8x8 = 0;
	uint32_t totalBits4x4 = 0;
#endif

#define OUT_8X8	1
	//=====================================================================================//
    /* -------------- Y ----------------*/
    inf_recon.size = m_size;
    inf_recon.Recon = cost_intra.recon_y;
    inf_intra.fenc = curr_cu_y;
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.L_intra_buf_y + x_pos - y_pos + 64;
    inf_recon.ori = curr_cu_y;
    inf_recon.size = cu_w;
	inf_intra_proc.Distortion = 0; // ensure the Distortion to be single CU.
#ifdef RK_INTRA_PRED

	// add by zxy for setLambda
	m_rkIntraPred->setLambda(qpY, sliceType);

	// --- covert uint8_t to bool ---//
	// RK flag map to 8 pel. x265 flag map to 4 pel[Luma]
	// RK flag map to 4 pel. x265 flag map to 2 pel[Chroma]
	uint8_t flag_map_pel = 8;
	uint8_t* bNeighbour = inf_intra_proc.bNeighborFlags - (2*inf_intra_proc.size/flag_map_pel);
	uint8_t k = 0;
	for (uint8_t i = 0 ; i < 2*inf_intra_proc.size/flag_map_pel ; i++ )
	{
		inf_intra.bNeighborFlags[k++] = *bNeighbour   == 1 ? true : false;
		inf_intra.bNeighborFlags[k++] = *bNeighbour++ == 1 ? true : false;
	}

	inf_intra.bNeighborFlags[k++] = *bNeighbour++ == 1 ? true : false;

	for (uint8_t i = 0 ; i < 2*inf_intra_proc.size/flag_map_pel ; i++ )
	{
		inf_intra.bNeighborFlags[k++] = *bNeighbour   == 1 ? true : false;
		inf_intra.bNeighborFlags[k++] = *bNeighbour++ == 1 ? true : false;
	}
	// ---------------------------//

	// ---- 1. TODO intra ----
 	inf_intra.size				= inf_intra_proc.size;
	inf_intra.cidx 				= 0;
	inf_intra.fenc				= inf_intra_proc.fencY;
	inf_intra.reconEdgePixel	= inf_intra_proc.reconEdgePixelY - 2*inf_intra_proc.size ;
	inf_intra.numintraNeighbor 	= inf_intra_proc.numIntraNeighbor;
	inf_intra.lumaDirMode		= 35; // inf_intra_proc.predMode;
	inf_intra.useStrongIntraSmoothing = inf_intra_proc.useStrongIntraSmoothing;

	m_rkIntraPred->Intra_Proc(&inf_intra,
								0,
								depth,
								x_pos,
								y_pos);

#endif
    // ---- 2. TODO TQ() ----
#if TQ_RUN_IN_HWC_INTRA
	m_hevcQT->proc(&inf_tq, &inf_intra, 0, qpY, sliceType); // Y
	memcpy(inf_tq_total[0].oriResi, inf_tq.oriResi, inf_tq.Size*inf_tq.Size * 2);

	/* Pointer connect */
    inf_recon.pred = inf_intra.pred; // intra pred connect to "recon" and "qt"
    inf_recon.resi = inf_tq.outResi; // tq outResi connect to "recon"

	// ---- 3. TODO reconstruction() ----
	//memcpy(inf_recon.pred)
	recon();
	memcpy(inf_recon_total[0].Recon, inf_recon.Recon, inf_tq.Size*inf_tq.Size); // for comparing in ctu_calc.cpp proc()

	// calc dist & update the CU Distortion for current depth
	inf_intra_proc.Distortion += RdoDistCalc(inf_recon.SSE, 0);
#endif
#if OUT_8X8
	// Y
	if (m_size == 8)
	{
		for ( uint8_t  j = 0 ; j < 8*8 ; j++ )
		{
			FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RESI_AFTER],"%04x",(uint16_t)inf_recon.resi[63 - j]);			    
			FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RECON],"%02x",inf_recon.Recon[63 - j]);			    
		}
		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RESI_AFTER],"\n");			    
		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RECON],"\n");	
	}
#endif
    //CABAC();

    // store [ Y output] to [inf_intra_proc]
    inf_intra_proc.ReconY 	= inf_recon.Recon;
    inf_intra_proc.ResiY 	= inf_recon.resi;

	//=====================================================================================//



	//=====================================================================================//
    /* -------------- U ----------------*/
    inf_recon.size = m_size/2;
    inf_recon.Recon = cost_intra.recon_u;
    inf_intra.fenc = curr_cu_u;
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.L_intra_buf_u + x_pos/2 - y_pos/2 + 32;
    inf_intra.lumaDirMode = inf_intra.DirMode;
    inf_recon.ori = curr_cu_u;
    inf_recon.size = cu_w/2;

#ifdef RK_INTRA_PRED
	// ---- 1. TODO intra ----
 	inf_intra.size				= inf_intra_proc.size >> 1;
	inf_intra.cidx 				= 1;
	inf_intra.fenc				= inf_intra_proc.fencU;
	inf_intra.reconEdgePixel	= inf_intra_proc.reconEdgePixelU - inf_intra_proc.size ;
	inf_intra.numintraNeighbor 	= inf_intra_proc.numIntraNeighbor;
	inf_intra.lumaDirMode		= inf_intra.DirMode;
	inf_intra.useStrongIntraSmoothing = 0;

	m_rkIntraPred->Intra_Proc(&inf_intra,
								0,
								depth,
								x_pos,
								y_pos);

#endif

    // ---- 2. TODO TQ() ----
#if TQ_RUN_IN_HWC_INTRA
	m_hevcQT->proc(&inf_tq, &inf_intra, 1, qpU, sliceType); // Cb
	memcpy(inf_tq_total[1].oriResi, inf_tq.oriResi, inf_tq.Size*inf_tq.Size * 2);


	/* Pointer connect */
    inf_recon.pred = inf_intra.pred; // intra pred connect to "recon" and "qt"
    inf_recon.resi = inf_tq.outResi; // tq outResi connect to "recon"

	// ---- 3. TODO reconstruction() ----
	recon();
	memcpy(inf_recon_total[1].Recon, inf_recon.Recon, inf_tq.Size*inf_tq.Size); // for comparing in ctu_calc.cpp proc()

	// calc dist & update the CU Distortion for current depth
	inf_intra_proc.Distortion += RdoDistCalc(inf_recon.SSE, 1);
#endif
    //CABAC();
#if OUT_8X8
	// cb
	if (m_size == 8)
	{
		for ( uint8_t  j = 0 ; j < 4*4 ; j++ )
		{
			FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RESI_AFTER],"%04x",(uint16_t)inf_recon.resi[15 - j]);			    
			FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RECON],"%02x",inf_recon.Recon[15 - j]);			    
		}
		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RESI_AFTER],"\n");			    
		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RECON],"\n");	
	}
#endif
    // store [ U output] to [inf_intra_proc]
    inf_intra_proc.ReconU 	= inf_recon.Recon;
    inf_intra_proc.ResiU 	= inf_recon.resi;
	//=====================================================================================//

	//=====================================================================================//
	/* -------------- V ----------------*/
    inf_recon.Recon = cost_intra.recon_v;
    inf_intra.fenc = curr_cu_v;
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.L_intra_buf_v + x_pos/2 - y_pos/2 + 32;
    inf_recon.ori = curr_cu_v;
    inf_recon.size = cu_w/2;
#ifdef RK_INTRA_PRED
	// ---- 1. TODO intra ----

 	inf_intra.size				= inf_intra_proc.size >> 1;
	inf_intra.cidx 				= 2;
	inf_intra.fenc				= inf_intra_proc.fencV;
	inf_intra.reconEdgePixel	= inf_intra_proc.reconEdgePixelV - inf_intra_proc.size ;
	inf_intra.numintraNeighbor 	= inf_intra_proc.numIntraNeighbor;
	inf_intra.lumaDirMode		= inf_intra.DirMode;
	inf_intra.useStrongIntraSmoothing = 0;

	m_rkIntraPred->Intra_Proc(&inf_intra,
								0,
								depth,
								x_pos,
								y_pos);

#endif

    // ---- 2. TODO TQ() ----
#if TQ_RUN_IN_HWC_INTRA
	m_hevcQT->proc(&inf_tq, &inf_intra, 2, qpV, sliceType); // Cr
	memcpy(inf_tq_total[2].oriResi, inf_tq.oriResi, inf_tq.Size*inf_tq.Size *2);

	/* Pointer connect */
    inf_recon.pred = inf_intra.pred; // intra pred connect to "recon" and "qt"
    inf_recon.resi = inf_tq.outResi; // tq outResi connect to "recon"

	// ---- 3. TODO reconstruction() ----
	recon();
	memcpy(inf_recon_total[2].Recon, inf_recon.Recon, inf_tq.Size*inf_tq.Size);// for comparing in ctu_calc.cpp proc()

	// calc dist & update the CU Distortion for current depth
	inf_intra_proc.Distortion += RdoDistCalc(inf_recon.SSE, 2);
#endif

#if OUT_8X8
	// cr
	if (m_size == 8)
	{
		for ( uint8_t  j = 0 ; j < 4*4 ; j++ )
		{
			FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RESI_AFTER],"%04x",(uint16_t)inf_recon.resi[15 - j]);			    
			FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RECON],"%02x",inf_recon.Recon[15 - j]);			    
		}
		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RESI_AFTER],"\n");			    
		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_RECON],"\n");	
	}
#endif


    //CABAC();
	// store [ V output] to [inf_intra_proc]
    inf_intra_proc.ReconV 	= inf_recon.Recon;
    inf_intra_proc.ResiV 	= inf_recon.resi;

	// ---- 4. TODO RDOCostCalc for (Y + U + V) ----
	// cost = distortion + lambda * bits
	inf_intra_proc.Bits		 = totalBitsDepth;
	inf_intra_proc.totalCost = RdoCostCalc(inf_intra_proc.Distortion, totalBitsDepth, inf_tq.QP);
	//=====================================================================================//

	inf_intra_proc.partSize 		= 0; // 2Nx2N
	inf_intra_proc.predModeIntra[0] = inf_intra.DirMode;
	inf_intra_proc.predModeIntra[1] = 35;  // invalid mode
	inf_intra_proc.predModeIntra[2] = 35;  // invalid mode
	inf_intra_proc.predModeIntra[3] = 35;  // invalid mode

#if OUT_8X8
	if (m_size == 8)
	{
		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_CU_TOTAL_BITS],"%08x",totalBits8x8);	
		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_CU_TOTAL_BITS],"\n");	

		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_CU_TOTAL_DISTORTION],"%08x",inf_intra_proc.Distortion);	
		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_CU_TOTAL_DISTORTION],"\n");	

		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_CU_TOTAL_COST],"%08x",inf_intra_proc.totalCost);			    
		FPRINT(m_rkIntraPred->fp_intra_8x8[INTRA_8_CU_TOTAL_COST],"\n");			    
	}
#endif

	



	if(m_size == 8)
    {
		/*
		** use 8x8 info to derivate 4x4 info
		*/
		uint8_t lu_cb_cr_order[6] = {0, 4, 1, 2, 5, 3};// y cb y y cr y
		uint8_t predModeLocal4x4[4] = {35,35,35,35};
		choose4x4split = false; // default: not split to 4x4
	    // TODO TU_SIZE == 4
	#ifdef RK_INTRA_4x4_PRED
		//*********************************************************************************//
		// =========== Y =============

		inf_intra.fenc				= inf_intra_proc.fencY;
		inf_intra.reconEdgePixel	= inf_intra_proc.reconEdgePixelY - 2*inf_intra_proc.size ;
		inf_intra.cidx				= 0;
		inf_intra.size				= 8;

		uint32_t zscan_idx = g_rk_raster2zscan_depth_4[x_pos/4 + y_pos*4]/4;
		uint8_t partIdx, lumaDir = 35;
		uint8_t single_reconEdge[RECON_EDGE_4X4_NUM][4];
		uint32_t totalCost4x4 = 0x7fffffff;
		uint32_t totalDist4x4 = 0;
		for (partIdx = 0 ; partIdx < 4 ; partIdx++ )
		{
			// assert RK recon data same to g_4x4_total_reconEdge
			if ( partIdx == 1 )
			{
			    // check partIdx=0 recon
			    for ( uint8_t i = 0 ; i < 4 ; i++ )
			    {
			        assert(g_4x4_total_reconEdge[zscan_idx][PART_0_RIGHT][i]  == single_reconEdge[PART_0_RIGHT][i]);
			        assert(g_4x4_total_reconEdge[zscan_idx][PART_0_BOTTOM][i] == single_reconEdge[PART_0_BOTTOM][i]);
				}
			}
			else if ( partIdx == 2 )
			{
			    // check partIdx=1 recon
			    for ( uint8_t i = 0 ; i < 4 ; i++ )
			    {
			        assert(g_4x4_total_reconEdge[zscan_idx][PART_1_BOTTOM][i]  == single_reconEdge[PART_1_BOTTOM][i]);
				}
			}
			else if ( partIdx == 3 )
			{
			    // check partIdx=2 recon
			    for ( uint8_t i = 0 ; i < 4 ; i++ )
			    {
			        assert(g_4x4_total_reconEdge[zscan_idx][PART_2_RIGHT][i]  == single_reconEdge[PART_2_RIGHT][i]);
				}

			}
			//m_rkIntraPred->splitTo4x4By8x8(&inf_intra, &inf_intra4x4, &g_4x4_total_reconEdge[zscan_idx][0][0], partIdx);
			m_rkIntraPred->splitTo4x4By8x8(&inf_intra, &inf_intra4x4, &single_reconEdge[0][0], partIdx);
		    // ---- 1. TODO intra() ----
			m_rkIntraPred->Intra_Proc(&inf_intra4x4,
									partIdx,
									4,
									x_pos,
									y_pos);

			predModeLocal4x4[partIdx] = inf_intra4x4.DirMode;
		#ifdef INTRA_RESULT_STORE_FILE
			m_rkIntraPred->Store4x4ReconInfo(g_fp_4x4_params_rk, inf_intra4x4, partIdx);
		#endif
			// set chroma 4x4 dirMode by use the first part.
			if ( partIdx == 0 )
			{
	    		lumaDir = inf_intra4x4.DirMode;
			}

#if TQ_RUN_IN_HWC_INTRA
		    // ---- 2. TODO TQ() ----
			m_hevcQT->proc(&inf_tq, &inf_intra4x4, 0, qpY, sliceType); // Y, I slice
			memcpy(coeff4x4[partIdx], inf_tq.oriResi,  inf_tq.Size*inf_tq.Size*2); // for comparing coeff(after T and Q)

			uint8_t  fenc4x4[16];
			m_rkIntraPred->Convert8x8To4x4(fenc4x4, curr_cu_y, partIdx);
			inf_recon.ori = fenc4x4;

			/* Pointer connect */
			inf_recon.pred = inf_intra4x4.pred; // intra4x4 pred connect to "recon" and "qt"
			inf_recon.resi = inf_tq.outResi; // tq outResi connect to "recon"


			// ---- 3. TODO recon() ----
			recon();

			// copy for compare Cost, 4 parts of 4x4 (Y)
			memcpy(resi4x4[partIdx], inf_recon.resi, inf_tq.Size*inf_tq.Size*2);
			memcpy(recon4x4[partIdx], inf_recon.Recon, inf_tq.Size*inf_tq.Size);

			// calc dist
			totalDist4x4 += RdoDistCalc(inf_recon.SSE, 0);
#endif

			//CABAC();

			m_rkIntraPred->RK_store4x4Recon2Ref(&single_reconEdge[0][0], recon4x4[partIdx], partIdx);

		}

		assert((lumaDir >= 0) && (lumaDir < 35));

		//*********************************************************************************//
		// =========== U =============
		inf_intra.fenc				= inf_intra_proc.fencU;
		inf_intra.reconEdgePixel	= inf_intra_proc.reconEdgePixelU - inf_intra_proc.size ;
		inf_intra.cidx				= 1;
	 	inf_intra.size				= 4;
		inf_intra.lumaDirMode		= lumaDir;
		// use the partIdx = 0 lumaDir do 4x4 again.
		m_rkIntraPred->Intra_Proc(&inf_intra,
									0,
									4,
									x_pos,
									y_pos);
#if TQ_RUN_IN_HWC_INTRA
		m_hevcQT->proc(&inf_tq, &inf_intra, 1, qpU, sliceType); // Cb
		memcpy(coeff4x4[4], inf_tq.oriResi,  inf_tq.Size*inf_tq.Size*2); // for comparing coeff(after T and Q)


		inf_recon.ori = curr_cu_u;

		/* Pointer connect */
	    inf_recon.pred = inf_intra.pred; // intra pred connect to "recon" and "qt"
	    inf_recon.resi = inf_tq.outResi; // tq outResi connect to "recon"

		recon();

		// copy for compare Cost, 4x4(U)
		memcpy(resi4x4[4], inf_recon.resi, inf_tq.Size*inf_tq.Size*2);
		memcpy(recon4x4[4], inf_recon.Recon, inf_tq.Size*inf_tq.Size);

		// calc dist
		totalDist4x4 += RdoDistCalc(inf_recon.SSE, 1);
#endif
		//CABAC();
		//*********************************************************************************//
		// =========== V =============
		inf_intra.fenc				= inf_intra_proc.fencV;
		inf_intra.reconEdgePixel	= inf_intra_proc.reconEdgePixelV - inf_intra_proc.size ;
		inf_intra.cidx				= 2;
	 	inf_intra.size				= 4;
		inf_intra.lumaDirMode		= lumaDir;

		// use the partIdx = 0 lumaDir do 4x4 again.
	    // ---- 1. TODO intra() ----
		m_rkIntraPred->Intra_Proc(&inf_intra,
									0,
									4,
									x_pos,
									y_pos);
#if TQ_RUN_IN_HWC_INTRA
	    // ---- 2. TODO TQ() ----
		m_hevcQT->proc(&inf_tq, &inf_intra, 2, qpV, sliceType); // Cr
		memcpy(coeff4x4[5], inf_tq.oriResi,  inf_tq.Size*inf_tq.Size*2); // for comparing coeff(after T and Q)

		inf_recon.ori = curr_cu_v;
		/* Pointer connect */
	    inf_recon.pred = inf_intra.pred; // intra pred connect to "recon" and "qt"
	    inf_recon.resi = inf_tq.outResi; // tq outResi connect to "recon"

	    // ---- 3. TODO recon() ----
		recon();

		// copy for compare Cost, 4x4(V)
		memcpy(resi4x4[5], inf_recon.resi, inf_tq.Size*inf_tq.Size*2);
		memcpy(recon4x4[5], inf_recon.Recon, inf_tq.Size*inf_tq.Size);

		// calc dist
		totalDist4x4 += RdoDistCalc(inf_recon.SSE, 2);
#endif
		//CABAC();
		//*********************************************************************************//

		// ---- 4. TODO RDOCostCalc for (Y + U + V) ----
		// cost = distortion + lambda * bits
		totalCost4x4 = RdoCostCalc(totalDist4x4, totalBits4x4, inf_tq.QP); // 4x4

		#if 0//def INTRA_RESULT_STORE_FILE
		    RK_HEVC_FPRINT(g_fp_result_rk,
				"bits4x4 = %d dist4x4 = %d cost4x4 = %d \n bits8x8 = %d dist8x8 = %d cost8x8 = %d \n\n",
				totalBits4x4,
				totalDist4x4,
				totalCost4x4,
				totalBits8x8,
				inf_intra_proc.Distortion,
				inf_intra_proc.totalCost
			);
		#endif
		// compare 8x8 cost vs four 4x4 cost,choose the best.
		if ( inf_intra_proc.totalCost > totalCost4x4 )
		{
			choose4x4split = true; // flag, used to decide whether convet8x8HWCtoX265 in ctu_calc.cpp for comparing

			// used 4x4 output to replace 8x8
		    inf_intra_proc.totalCost	= totalCost4x4;
			inf_intra_proc.Distortion 	= totalDist4x4;
			inf_intra_proc.Bits			= totalBits4x4;

			// store [Y U V output] to [inf_intra_proc]

			// pack Y from recon4x4[0~3]
			m_rkIntraPred->Convert4x4To8x8(inf_intra_proc.ReconY, recon4x4);
			m_rkIntraPred->Convert4x4To8x8(inf_intra_proc.ResiY, resi4x4);

			::memcpy(inf_intra_proc.ReconU, recon4x4[4], 16) ;
			::memcpy(inf_intra_proc.ResiU,  resi4x4[4],  32) ;
			::memcpy(inf_intra_proc.ReconV, recon4x4[5], 16) ;
			::memcpy(inf_intra_proc.ResiV,  resi4x4[5],  32) ;

			::memcpy(inf_recon_total[0].Recon, inf_intra_proc.ReconY, 64);
			::memcpy(inf_recon_total[1].Recon, inf_intra_proc.ReconU, 64>>2);
			::memcpy(inf_recon_total[2].Recon, inf_intra_proc.ReconV, 64>>2);

			m_rkIntraPred->Convert4x4To8x8(inf_tq_total[0].oriResi, coeff4x4); // Y
			::memcpy(inf_tq_total[1].oriResi, coeff4x4[4], 64*2>>2); // Cb
			::memcpy(inf_tq_total[2].oriResi, coeff4x4[5], 64*2>>2); // Cr

			// store partSize
			inf_intra_proc.partSize = 1; // NxN
			// store predModeIntra[4]
			inf_intra_proc.predModeIntra[0] = predModeLocal4x4[0];
			inf_intra_proc.predModeIntra[1] = predModeLocal4x4[1];
			inf_intra_proc.predModeIntra[2] = predModeLocal4x4[2];
			inf_intra_proc.predModeIntra[3] = predModeLocal4x4[3];
		}

	#endif

	#if 1
		for ( uint8_t  i = 0 ; i < 6 ; i++ )
		{
			for ( uint8_t  j = 0 ; j < inf_tq.Size*inf_tq.Size ; j++ )
			{
				FPRINT(m_rkIntraPred->fp_intra_4x4[INTRA_4_RESI_AFTER],"%04x",(uint16_t)resi4x4[lu_cb_cr_order[i]][inf_tq.Size*inf_tq.Size - 1 - j]);			    
				FPRINT(m_rkIntraPred->fp_intra_4x4[INTRA_4_RECON],"%02x",recon4x4[lu_cb_cr_order[i]][inf_tq.Size*inf_tq.Size - 1 - j]);			    
			}
			FPRINT(m_rkIntraPred->fp_intra_4x4[INTRA_4_RESI_AFTER],"\n");			    
			FPRINT(m_rkIntraPred->fp_intra_4x4[INTRA_4_RECON],"\n");			    
		}

		FPRINT(m_rkIntraPred->fp_intra_4x4[INTRA_4_CU_TOTAL_BITS],"%08x",totalBits4x4);	
		FPRINT(m_rkIntraPred->fp_intra_4x4[INTRA_4_CU_TOTAL_BITS],"\n");	
		
		FPRINT(m_rkIntraPred->fp_intra_4x4[INTRA_4_CU_TOTAL_DISTORTION],"%08x",totalDist4x4);	
		FPRINT(m_rkIntraPred->fp_intra_4x4[INTRA_4_CU_TOTAL_DISTORTION],"\n");	
		
		FPRINT(m_rkIntraPred->fp_intra_4x4[INTRA_4_CU_TOTAL_COST],"%08x",totalCost4x4);			    
		FPRINT(m_rkIntraPred->fp_intra_4x4[INTRA_4_CU_TOTAL_COST],"\n");			    
	#endif
    }
#undef OUT_8X8	
	
}


//4 means intra 4x4 pred
void CU_LEVEL_CALC::intra_4_proc()
{
    if(cu_w != 8)
        return;

    inf_recon.pred = inf_intra.pred;
    inf_recon.resi = inf_tq.outResi;
    /*Y*/
    inf_recon.size = m_size;
    inf_recon.Recon = cost_intra.recon_y;
    inf_intra.fenc = curr_cu_y;
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.L_intra_buf_y + x_pos - y_pos + 64;
    inf_recon.ori = curr_cu_y;
    inf_recon.size = cu_w;
    //Intra();
    //TQ();
    //CABAC();
    //recon();
    //RDOCostCalc(inf_recon.SSE, QP, CABAC.Bits);

    /*U*/
    inf_recon.size = m_size/2;
    inf_recon.Recon = cost_intra.recon_u;
    inf_intra.fenc = curr_cu_u;
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.L_intra_buf_u + x_pos/2 - y_pos/2 + 32;
    inf_intra.lumaDirMode = inf_intra.DirMode;
    inf_recon.ori = curr_cu_u;
    inf_recon.size = cu_w/2;
    //Intra();
    //TQ();
    //CABAC();
    //recon();
    //RDOCostCalc(inf_recon.SSE, QP, CABAC.Bits);

    /*V*/
    inf_recon.Recon = cost_intra.recon_v;
    inf_intra.fenc = curr_cu_v;
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.L_intra_buf_v + x_pos/2 - y_pos/2 + 32;
    inf_recon.ori = curr_cu_v;
    inf_recon.size = cu_w/2;
    //Intra();
    //TQ();
    //CABAC();
    //recon();
    //RDOCostCalc(inf_recon.SSE, QP, CABAC.Bits);

    if(m_size == 8)
    {
        //TODO TU_SIZE == 4
    }
}

uint32_t CU_LEVEL_CALC::RdoDistCalc(uint32_t SSE, uint8_t yCbCr)
{
	uint32_t dist = 0;
	if(yCbCr==0) // Y
	{
		dist = SSE;
	}
	else if(yCbCr==1) // Cb
	{
		dist = (SSE * pHardWare->hw_cfg.CbDistWeight + 128) >> 8;
	}
	else if(yCbCr==2) // Cr
	{
		dist = (SSE * pHardWare->hw_cfg.CrDistWeight + 128) >> 8;
	}
	else
	{
	    RK_HEVC_PRINT("shouldn't happen.\n")
	}

	return dist;
}
uint32_t CU_LEVEL_CALC::RdoCostCalc(uint32_t dist, uint32_t bits, int qp)
{

	// RDO formula: cost = dist + lambda * bits
	int lambda;
	uint32_t cost;

	/* get Lambda */
	if (pHardWare->hw_cfg.slice_type == 2) // I Slice
	{
		lambda = rk_lambdaMotionSSE_tab_I[qp];
	}
	else
	{
		lambda = rk_lambdaMotionSSE_tab_non_I[qp];
	}

	/* calc  Cost */
	cost = dist + ((bits * lambda + 32768) >> 16);

	return cost;
}
void CU_LEVEL_CALC::RDOCompare()
{
    if ((cu_w == 8) && (cost_intra.TotalCost > cost_intra_4.TotalCost))
    {
        cost_intra = cost_intra_4;
    }

    if(cost_inter.TotalCost > cost_intra.TotalCost)
    {
        cost_best = &cost_intra;
        cost_best->predMode = 1;
    }
    else
    {
        cost_best = &cost_inter;
        cost_best->predMode = 0;
    }
}

unsigned int CU_LEVEL_CALC::proc(unsigned int level, unsigned int pos_x, unsigned int pos_y)
{
    hardwareC *p = pHardWare;
    cuData *dst_cu, *src_cu;

    x_pos = (uint8_t)pos_x;
    y_pos = (uint8_t)pos_y;
    valid = 1;

    uint32_t cost;

    p->ctu_calc.cu_valid_flag[depth][ori_pos] = 1;

    if((((p->ctu_x * p->ctu_w + pos_x) < p->pic_w)) && ((p->ctu_y * p->ctu_w + pos_y) < p->pic_h)) {
        temp_pos++;
    }

    if((((p->ctu_x * p->ctu_w + pos_x + (1 << (6 - level))) > p->pic_w)) ||
         ((p->ctu_y * p->ctu_w + pos_y + (1 << (6 - level))) > p->pic_h)) {
        p->ctu_calc.cu_valid_flag[depth][ori_pos] = 0;
        return 0xffffffff;
    }

    begin();
    //inter_proc();
    intra_proc();

    //RDOCompare();

    cost_intra.predMode = 1;
    cost_best = &cost_intra;



    //-------------------------------------------//
    // temp use for debug                        //
    //-------------------------------------------//
    src_cu = pHardWare->ctu_calc.cu_ori_data[depth][cu_pos++];
    dst_cu = &cu_matrix_data[matrix_pos];
    memcpy(cost_best->recon_y, src_cu->ReconY, cu_w*cu_w);
    memcpy(cost_best->recon_u, src_cu->ReconU, cu_w*cu_w/4);
    memcpy(cost_best->recon_v, src_cu->ReconV, cu_w*cu_w/4);

    cost_best->predMode = src_cu->cuPredMode[0];
    cost_best->partSize = src_cu->cuPartSize[0];

    //------------------------------------------//

    memcpy(dst_cu->ReconY, cost_best->recon_y, cu_w*cu_w);
    memcpy(dst_cu->ReconU, cost_best->recon_u, cu_w*cu_w/4);
    memcpy(dst_cu->ReconV, cost_best->recon_v, cu_w*cu_w/4);

    memset(cu_matrix_data[matrix_pos].cuPredMode, cost_best->predMode, (cu_w/8)*(cu_w/8));
    memset(cu_matrix_data[matrix_pos].cuPartSize, cost_best->partSize, (cu_w/8)*(cu_w/8));
    memset(cu_matrix_data[matrix_pos].cuSkipFlag, cost_best->skipFlag, (cu_w/8)*(cu_w/8));

    cost = (uint32_t)src_cu->totalCost;

	return cost;
}
