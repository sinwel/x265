#include <stdio.h>
#include "level_mode_calc.h"
#include "hardwareC.h"
#include "inter.h"
#include "CABAC.h"

#ifdef RK_INTRA_4x4_PRED
extern uint32_t g_rk_raster2zscan_depth_4[256];
extern FILE* g_fp_result_rk;
extern FILE* g_fp_4x4_params_rk;
#endif

// for debug: TQ with ME
int16_t           g_mergeResiY[4][64*64];
int16_t           g_mergeResiU[4][32*32];
int16_t           g_mergeResiV[4][32*32];
int16_t			  g_mergeCoeffY[4][64*64];
int16_t			  g_mergeCoeffU[4][32*32];
int16_t			  g_mergeCoeffV[4][32*32];
int16_t           g_fmeResiY[4][64*64];
int16_t           g_fmeResiU[4][32*32];
int16_t           g_fmeResiV[4][32*32];
int16_t			  g_fmeCoeffY[4][64*64];
int16_t			  g_fmeCoeffU[4][32*32];
int16_t			  g_fmeCoeffV[4][32*32];
bool			  g_fme;
bool			  g_merge;
unsigned int  g_mergeBits[85];
unsigned int  g_fmeBits[85];


// table to locate pos, used in inter_proc, added by lks
static int lumaPart2pos[4] = {0, 32, 32*64, 32*64+32};
static int chromaPart2pos[4] = {0, 16, 16*32, 16*32+16};

CU_LEVEL_CALC::CU_LEVEL_CALC()
{
}

CU_LEVEL_CALC::~CU_LEVEL_CALC()
{
}

class AmvpCand CU_LEVEL_CALC::m_Amvp;
class MergeMvpCand CU_LEVEL_CALC::m_MergeCand;

void CU_LEVEL_CALC::init(uint8_t size)
{
    m_size = size;
    cu_w = size;
#if TQ_RUN_IN_HWC_INTRA || TQ_RUN_IN_HWC_ME
	m_hevcQT = new hevcQT; // reuse by intra and me
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

#if TQ_RUN_IN_HWC_ME //added by lks
	for(int k=0; k<3; k++)
	{
		int tmpSize = k==0 ? m_size: (m_size/2);
		reconFme[k] = (uint8_t*)malloc(tmpSize*tmpSize);
		reconMerge[k] = (uint8_t*)malloc(tmpSize*tmpSize);
		coeffFme[k] = (int16_t*)malloc(tmpSize*tmpSize*2);
		coeffMerge[k] = (int16_t*)malloc(tmpSize*tmpSize*2);
	}
#endif

    inf_intra.pred = (uint8_t *)malloc(m_size*m_size);
    inf_intra.resi = (int16_t *)malloc(m_size*m_size*2);

    inf_tq.outResi = (int16_t *)malloc(m_size*m_size*2);
    inf_tq.oriResi = (int16_t *)malloc(m_size*m_size*2);

    inf_me.pred = (uint8_t *)malloc(m_size*m_size);
    inf_me.resi = (int16_t *)malloc(m_size*m_size*2);

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
#if (TQ_RUN_IN_HWC_INTRA || TQ_RUN_IN_HWC_ME)
	delete m_hevcQT; // reuse by intra and me
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
#if TQ_RUN_IN_HWC_ME //added by lks

	for(int k=0; k<3; k++)
	{	
		free(reconFme[k]);		reconFme[k]=NULL;
		free(reconMerge[k]);	reconMerge[k]=NULL;
		free(coeffFme[k]); 		coeffFme[k]=NULL;
		free(coeffMerge[k]);	coeffMerge[k]=NULL;
	}
#endif

    uint8_t i;
    free(inf_intra.pred);
    free(inf_intra.resi);

    free(inf_tq.outResi);
    free(inf_tq.oriResi);

    free(inf_me.pred);
    free(inf_me.resi);

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
    memcpy(intra_mpm,   pHardWare->ctu_calc.L_intra_mpm + 16 + x_pos/4 - (y_pos + 8)/4, 2*8/4 + 1);
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
    for(m=0; m<cu_w/8; m++) {
        if(cu_src->cuPredMode[m*cu_w/8 + cu_w/8 - 1] == 0) {
            pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + cu_w/4 - 1) - (y_pos/4 + m) + 16] = 1; //MODE_DC
            pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + cu_w/4    ) - (y_pos/4 + m) + 16] = 1; //MODE_DC
        }
        else {
            if(cu_w == 8 && cu_src->cuPartSize[0] == 1) {
                pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + cu_w/4 - 1) - (y_pos/4 + m) + 16] = cu_src->intraDirMode[1];
                pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + cu_w/4    ) - (y_pos/4 + m) + 16] = cu_src->intraDirMode[3];
            }
            else {
                pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + cu_w/4 - 1) - (y_pos/4 + m) + 16] = cu_src->intraDirMode[1];
                pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + cu_w/4    ) - (y_pos/4 + m) + 16] = cu_src->intraDirMode[1];
            }
        }
        if (cu_src->cuPredMode[(cu_w/8 - 1)*cu_w/8 + m] == 0) {
            pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + m) - (y_pos/4 + cu_w/4 - 1) + 16] = 1;
            pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + 1 + m) - (y_pos/4 + cu_w/4 - 1) + 16] = 1;
        }
        else {
            if (cu_w == 8 && cu_src->cuPartSize[0] == 1) {
                pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + m) - (y_pos/4 + cu_w/4 - 1) + 16] = cu_src->intraDirMode[2];
                pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + 1 + m) - (y_pos/4 + cu_w/4 - 1) + 16] = cu_src->intraDirMode[3];
            }
            else {
                pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + m) - (y_pos/4 + cu_w/4 - 1) + 16] = cu_src->intraDirMode[1];
                pHardWare->ctu_calc.L_intra_mpm[(x_pos/4 + 1 + m) - (y_pos/4 + cu_w/4 - 1) + 16] = cu_src->intraDirMode[1];
            }
        }
    }

    /* cu_data_valid flag */
    for(m=0; m<cu_w/8; m++) {
        for(n=0; n<cu_w/8; n++) {
            pHardWare->ctu_calc.cu_data_valid[(y_pos/8 + m)*8 + x_pos/8 + n] = 1;
        }
    }

    /* inter */
    for(m=0; m<cu_w/8; m++){
        pHardWare->ctu_calc.L_mv_buf[(x_pos/8 + cu_w/8 - 1) - (y_pos/8 + m) + 8] = cu_src->mv[m*cu_w/8 + cu_w/8 - 1];
        pHardWare->ctu_calc.L_mv_buf[(x_pos/8 + m) - (y_pos/8 + cu_w/8 - 1) + 8] = cu_src->mv[(cu_w/8 - 1)*cu_w/8 + m];
    }


    if(cu_w == 8) {
        pHardWare->inf_dblk.cu_depth[pHardWare->ctu_x][(y_pos/8) * 8 + x_pos/8] = 3;
        if (cuPredMode == 1) {
            pHardWare->inf_dblk.pu_depth[pHardWare->ctu_x][(y_pos/8) * 8 + x_pos/8] = cuPartMode;
            pHardWare->inf_dblk.pu_depth[pHardWare->ctu_x][(y_pos/8) * 8 + x_pos/8+1] = cuPartMode;
            pHardWare->inf_dblk.pu_depth[pHardWare->ctu_x][(y_pos/8+1) * 8 + x_pos/8] = cuPartMode;
            pHardWare->inf_dblk.pu_depth[pHardWare->ctu_x][(y_pos/8+1) * 8 + x_pos/8+1] = cuPartMode;
            pHardWare->inf_dblk.tu_depth[pHardWare->ctu_x][(y_pos/8) * 8 + x_pos/8] = cuPartMode;
            pHardWare->inf_dblk.tu_depth[pHardWare->ctu_x][(y_pos/8) * 8 + x_pos/8+1] = cuPartMode;
            pHardWare->inf_dblk.tu_depth[pHardWare->ctu_x][(y_pos/8+1) * 8 + x_pos/8] = cuPartMode;
            pHardWare->inf_dblk.tu_depth[pHardWare->ctu_x][(y_pos/8+1) * 8 + x_pos/8+1] = cuPartMode;
            pHardWare->inf_dblk.intra_bs_flag[pHardWare->ctu_x][(y_pos/16) * 16 + x_pos/4] = 2;
            pHardWare->inf_dblk.intra_bs_flag[pHardWare->ctu_x][(y_pos/16) * 16 + x_pos/4+1] = 2;
            pHardWare->inf_dblk.intra_bs_flag[pHardWare->ctu_x][(y_pos/16+1) * 16 + x_pos/4] = 2;
            pHardWare->inf_dblk.intra_bs_flag[pHardWare->ctu_x][(y_pos/16+1) * 16 + x_pos/4+1] = 2;
        }
        else {
            pHardWare->inf_dblk.pu_depth[pHardWare->ctu_x][(y_pos/8) * 8 + x_pos/8] = 0;
            pHardWare->inf_dblk.pu_depth[pHardWare->ctu_x][(y_pos/8) * 8 + x_pos/8+1] = 0;
            pHardWare->inf_dblk.pu_depth[pHardWare->ctu_x][(y_pos/8+1) * 8 + x_pos/8] = 0;
            pHardWare->inf_dblk.pu_depth[pHardWare->ctu_x][(y_pos/8+1) * 8 + x_pos/8+1] = 0;
            pHardWare->inf_dblk.tu_depth[pHardWare->ctu_x][(y_pos/8) * 8 + x_pos/8] = 0;
            pHardWare->inf_dblk.tu_depth[pHardWare->ctu_x][(y_pos/8) * 8 + x_pos/8+1] = 0;
            pHardWare->inf_dblk.tu_depth[pHardWare->ctu_x][(y_pos/8+1) * 8 + x_pos/8] = 0;
            pHardWare->inf_dblk.tu_depth[pHardWare->ctu_x][(y_pos/8+1) * 8 + x_pos/8+1] = 0;
            pHardWare->inf_dblk.intra_bs_flag[pHardWare->ctu_x][(y_pos/16) * 16 + x_pos/4] = inter_cbfY[0];
            pHardWare->inf_dblk.intra_bs_flag[pHardWare->ctu_x][(y_pos/16) * 16 + x_pos/4+1] = inter_cbfY[0];
            pHardWare->inf_dblk.intra_bs_flag[pHardWare->ctu_x][(y_pos/16+1) * 16 + x_pos/4] = inter_cbfY[0];
            pHardWare->inf_dblk.intra_bs_flag[pHardWare->ctu_x][(y_pos/16+1) * 16 + x_pos/4+1] = inter_cbfY[0];
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

/* calc One TU, One textType
  1. proc QT 			---- output: inf_tq.outResi
  2. copy pred 			---- output: inf_me.pred
*/
void CU_LEVEL_CALC::inter_qt(uint8_t nPart, uint8_t mode, uint8_t textType)
{
	uint8_t sliceType = pHardWare->ctu_calc.slice_type;
	uint32_t ctuSize;
	uint8_t cuSize, qp;
	int16_t *resiX265;
	int  *part2pos;
	int  strideX265 = textType==0? 64 : 32;
	int16_t *resiHWC = mode == 0 ? Fme.getCurrCuResi() : Merge.getMergeResi(); // output from hwc
	uint8_t *predHWC = mode == 0 ? Fme.getCurrCuPred() : Merge.getMergePred(); // output from hwc
	int16_t *coeffHWC = mode==0 ? coeffFme[textType] : coeffMerge[textType];
	
	if(textType==0) // Y
	{
		ctuSize = pHardWare->ctu_w;
		cuSize = cu_w;
		part2pos = lumaPart2pos;
		qp = pHardWare->ctu_calc.QP_lu;
		resiX265 = mode==0? &g_fmeResiY[0][0] : &g_mergeResiY[0][0];
		resiX265 += 64*64*depth;
	}
	else if(textType==1) // U
	{
		ctuSize = pHardWare->ctu_w/2;
		cuSize = cu_w/2;
		part2pos = chromaPart2pos;
		qp = pHardWare->ctu_calc.QP_cb;
		resiHWC += cu_w*cu_w;
		predHWC += cu_w*cu_w;
		resiX265 = mode==0? &g_fmeResiU[0][0] : &g_mergeResiU[0][0];
		resiX265 += 32*32*depth;
	}
	else // V
	{
		ctuSize = pHardWare->ctu_w/2;
		cuSize = cu_w/2;
		part2pos = chromaPart2pos;
		qp = pHardWare->ctu_calc.QP_cr;
		resiHWC += cu_w*cu_w*5/4;
		predHWC += cu_w*cu_w*5/4;
		resiX265 = mode==0? &g_fmeResiV[0][0] : &g_mergeResiV[0][0];
		resiX265 += 32*32*depth;
	}


	int pos;
	uint8_t tuSize;
	if(cu_w==64)
	{
		tuSize = cuSize/2;
		inf_me.size = tuSize;
		pos = part2pos[nPart];
		for(int k=0; k<tuSize; k++)
		{
			assert(pos+k*cuSize>=0 || pos+k*cuSize<cuSize*cuSize);
			memcpy(inf_me.resi+k*tuSize, resiHWC+pos+k*cuSize, tuSize*2); // copy resi
			memcpy(inf_me.pred+k*tuSize, predHWC+pos+k*cuSize, tuSize); // copy pred
		}
		
		m_hevcQT->proc(&inf_tq, &inf_me, textType, qp, sliceType);

		for(int k=0; k<tuSize; k++)
		{
			memcpy(&coeffHWC[pos+k*strideX265], &inf_tq.oriResi[k*tuSize], tuSize*2); // copy coeff(after Q and T)
		}

		// compare
		for (int k = 0; k < tuSize; k++)
		{
			assert(!memcmp(inf_tq.outResi + k*tuSize, resiX265+pos+k*strideX265, tuSize*2));
		}

	}
	else
	{
		tuSize = cuSize;
		inf_me.size = tuSize;
		pos = textType==0? y_pos*ctuSize+x_pos: y_pos/2*ctuSize+x_pos/2;
		memcpy(inf_me.resi, resiHWC, tuSize*tuSize*2); // copy resi
		memcpy(inf_me.pred, predHWC, tuSize*tuSize); // copy pred

		m_hevcQT->proc(&inf_tq, &inf_me, textType, qp, sliceType); // Y
		memcpy(coeffHWC, inf_tq.oriResi, tuSize*tuSize*2); // copy coeff(after Q and T)
		
		// compare
		for(int k=0; k<tuSize; k++)
		{
			assert(!memcmp(inf_tq.outResi + k*tuSize, resiX265+pos+k*strideX265, tuSize*2));
		}
	}

}
void CU_LEVEL_CALC::inter_proc(int offsIdx)
{
	m_Amvp.PrefetchAmvpCandInfo(offsIdx);
	m_MergeCand.PrefetchAmvpCandInfo(offsIdx);
	int nPicWidth = pHardWare->pic_w;
	int nPicHeight = pHardWare->pic_h;
	int nCtu = pHardWare->ctu_w;
	int nCuSize = cu_w;
	bool PicWidthNotDivCtu = nPicWidth / nCtu*nCtu < nPicWidth;
	int numCtuInPicWidth = nPicWidth / nCtu + (PicWidthNotDivCtu ? 1 : 0);
	int nCtuPosInPic = pHardWare->ctu_x + pHardWare->ctu_y*numCtuInPicWidth;


	int offset_x = OffsFromCtu64[offsIdx][0];
	int offset_y = OffsFromCtu64[offsIdx][1];
	bool isCuOutPic = false;
	if ((nCtu*((int)pHardWare->ctu_x) + offset_x + (int)cu_w) > nPicWidth || (nCtu*((int)pHardWare->ctu_y) + offset_y + (int)cu_w) > nPicHeight)
		isCuOutPic = true;
	Fme.setCtuSize(pHardWare->ctu_w);
	Fme.setCtuPosInPic(nCtuPosInPic);
	Merge.setCtuSize(pHardWare->ctu_w);

	pHardWare->Rime[offsIdx].setCuPosInCtu(offsIdx);
	Fme.setCuPosInCtu(offsIdx);
	Fme.setCuSize(nCuSize);
	Fme.setSliceType(pHardWare->Rime[offsIdx].getSliceType());
	Fme.Create();

	Merge.setCuPosInCtu(offsIdx);
	Merge.setCuSize(nCuSize);
	m_MergeCand.getMergeCandidates();
	MvField *pMvField = m_MergeCand.getMvFieldNeighbours();
	for (int i = 0; i < m_MergeCand.getMergeCandNum(); i ++)
	{
		assert(g_mvMerge[offsIdx][i * 2].refIdx == pMvField[i * 2].refIdx);
		if (pMvField[i*2].refIdx >= 0)
		{
			assert(g_mvMerge[offsIdx][i * 2].mv.x == pMvField[i * 2].mv.x);
			assert(g_mvMerge[offsIdx][i * 2].mv.y == pMvField[i * 2].mv.y);
		}
		assert(g_mvMerge[offsIdx][i * 2 + 1].refIdx == pMvField[i * 2 + 1].refIdx);
		if (pMvField[i * 2 + 1].refIdx >= 0)
		{
			assert(g_mvMerge[offsIdx][i * 2 + 1].mv.x == pMvField[i * 2 + 1].mv.x);
			assert(g_mvMerge[offsIdx][i * 2 + 1].mv.y == pMvField[i * 2 + 1].mv.y);
		}
	}
	Merge.setMergeCand(g_mvMerge[offsIdx]);
	Mv tmpMv[nMaxRefPic*2];
	for (int nRefPicIdx = 0; nRefPicIdx < nMaxRefPic; nRefPicIdx ++)
	{
		pHardWare->Rime[offsIdx].getPmv(tmpMv[nRefPicIdx * 2 + 0], nRefPicIdx, 1);
		pHardWare->Rime[offsIdx].getPmv(tmpMv[nRefPicIdx * 2 + 1], nRefPicIdx, 2);
	}
	Merge.setNeighPmv(tmpMv);
	Merge.Create();

	inf_inter_fme_proc.Distortion = 0;
	inf_inter_fme_proc.Bits= g_fmeBits[offsIdx];// to do
	inf_inter_merge_proc.Distortion = 0;
	inf_inter_merge_proc.Bits= g_mergeBits[offsIdx];

	uint8_t ori_data[32*32];
	
	if (!isCuOutPic)
	{
		//Fme residual and MVD MVP index compare
		Fme.setRefPicNum(pHardWare->Rime[offsIdx].getRefPicNum(REF_PIC_LIST0), REF_PIC_LIST0);
		Fme.setRefPicNum(pHardWare->Rime[offsIdx].getRefPicNum(REF_PIC_LIST1), REF_PIC_LIST1);
		Fme.setQP(pHardWare->Rime[offsIdx].getQP());
		if (Fme.getSliceType() == b_slice)
		{
			Fme.setCurrCuPel(pHardWare->Rime[offsIdx].getCurrCuPel(), nCuSize);
			for (int nRefPicIdx = 0; nRefPicIdx < Fme.getRefPicNum(REF_PIC_LIST0); nRefPicIdx++)
			{
				AmvpInfo amvpMine;
				m_Amvp.fillMvpCand(REF_PIC_LIST0, nRefPicIdx, &amvpMine);
				for (int i = 0; i < amvpMine.m_num; i++)
				{
					assert(amvpMine.m_mvCand[i].x == g_mvAmvp[nRefPicIdx][offsIdx][i].x);
					assert(amvpMine.m_mvCand[i].y == g_mvAmvp[nRefPicIdx][offsIdx][i].y);
				}
				Fme.setAmvp(g_mvAmvp[nRefPicIdx][offsIdx], nRefPicIdx);
				Fme.setFmeInterpPel(pHardWare->Rime[offsIdx].getCuForFmeInterp(nRefPicIdx), (nCuSize + 4 * 2), nRefPicIdx);
				Fme.setMvFmeInput(pHardWare->Rime[offsIdx].getFmeMv(nRefPicIdx), nRefPicIdx);
			}
			for (int nRefPicIdx = 0; nRefPicIdx < Fme.getRefPicNum(REF_PIC_LIST1); nRefPicIdx++)
			{
				int idx = nMaxRefPic / 2 + nRefPicIdx;
				AmvpInfo amvpMine;
				m_Amvp.fillMvpCand(REF_PIC_LIST1, nRefPicIdx, &amvpMine);
				for (int i = 0; i < amvpMine.m_num; i++)
				{
					assert(amvpMine.m_mvCand[i].x == g_mvAmvp[idx][offsIdx][i].x);
					assert(amvpMine.m_mvCand[i].y == g_mvAmvp[idx][offsIdx][i].y);
				}
				Fme.setAmvp(g_mvAmvp[idx][offsIdx], idx);
				Fme.setFmeInterpPel(pHardWare->Rime[offsIdx].getCuForFmeInterp(idx), (nCuSize + 4 * 2), idx);
				Fme.setMvFmeInput(pHardWare->Rime[offsIdx].getFmeMv(idx), idx);
			}
			Fme.CalcResiAndMvd();
		}
		else
		{
			Fme.setCurrCuPel(pHardWare->Rime[offsIdx].getCurrCuPel(), nCuSize);
			for (int nRefPicIdx = 0; nRefPicIdx < Fme.getRefPicNum(REF_PIC_LIST0); nRefPicIdx++)
			{
				AmvpInfo amvpMine;
				m_Amvp.fillMvpCand(REF_PIC_LIST0, nRefPicIdx, &amvpMine);
				for (int i = 0; i < amvpMine.m_num; i++)
				{
					assert(amvpMine.m_mvCand[i].x == g_mvAmvp[nRefPicIdx][offsIdx][i].x);
					assert(amvpMine.m_mvCand[i].y == g_mvAmvp[nRefPicIdx][offsIdx][i].y);
				}
				Fme.setAmvp(g_mvAmvp[nRefPicIdx][offsIdx], nRefPicIdx);
				Fme.setFmeInterpPel(pHardWare->Rime[offsIdx].getCuForFmeInterp(nRefPicIdx), (nCuSize + 4 * 2), nRefPicIdx);
				Fme.setMvFmeInput(pHardWare->Rime[offsIdx].getFmeMv(nRefPicIdx), nRefPicIdx);
			}
			Fme.CalcResiAndMvd();
		}

		short Offset_x = 0; Offset_x = OffsFromCtu64[offsIdx][0];
		short Offset_y = 0; Offset_y = OffsFromCtu64[offsIdx][1];
		for (int m = 0; m < nCuSize; m++)
		{
			for (int n = 0; n < nCuSize; n++)
			{
				assert(g_FmeResiY[depth][(n + Offset_x) + (m + Offset_y) * 64] == Fme.getCurrCuResi()[n + m*nCuSize]);
			}
		}

		for (int m = 0; m < nCuSize / 2; m++)
		{
			for (int n = 0; n < nCuSize / 2; n++)
			{
				assert(g_FmeResiU[depth][(n + Offset_x / 2) + (m + Offset_y / 2) * 32] == Fme.getCurrCuResi()[n + m*nCuSize / 2 + nCuSize*nCuSize]);
				assert(g_FmeResiV[depth][(n + Offset_x / 2) + (m + Offset_y / 2) * 32] == Fme.getCurrCuResi()[n + m*nCuSize / 2 + nCuSize*nCuSize * 5 / 4]);
			}
		}

		uint8_t nPart = 0;
		
		//=================== TQ  for FME ===================
		unsigned char cbfY = 0;
		unsigned char cbfU = 0;
		unsigned char cbfV = 0;
#if TQ_RUN_IN_HWC_ME
		if(cu_w==64)
		{
			int k, pos;
			for(nPart=0; nPart<4; nPart++)
			{
				//-------- Y ---------
				// TQ proc
				inter_qt(nPart, 0, 0); 
				cbfY = (inf_tq.absSum == 0 ? 0:1);

				inf_recon.size = inf_me.size;
    			inf_recon.pred = inf_me.pred; 
    			inf_recon.resi = inf_tq.outResi;
    			inf_recon.Recon = cost_inter.recon_y;

#if RK_CABAC_FUNCTIONAL_TYPE
				m_cabac_rdo->IsIntra_tu = 0;
				m_cabac_rdo->depth_tu = 0;
				m_cabac_rdo->zscan_idx_tu = g_rk_raster2zscan_depth_4[Offset_x/4 + Offset_y*4] ;
				m_cabac_rdo->tu_luma_idx = nPart;
				m_cabac_rdo->cbf_luma_total[nPart] = cbfY;

				m_cabac_rdo->TU_chroma_size = 16;
				m_cabac_rdo->TU_Resi_luma = inf_tq.oriResi;
				m_cabac_rdo->cabac_est_functional_type_inter(0);
#endif


				pos = lumaPart2pos[nPart];
				for(k=0; k<32; k++)
				{
					memcpy(ori_data + k*32, curr_cu_y+pos+k*64, 32);
				}	
				inf_recon.ori = ori_data;

				// reconstruct
				recon();

				for(k=0; k<32; k++)
				{
					memcpy(reconFme[0]+pos+k*64, inf_recon.Recon+k*inf_recon.size, 32);				
				}

				inf_inter_fme_proc.Distortion += RdoDistCalc(inf_recon.SSE, 0);

				//-------- U ---------
				// TQ proc
				inter_qt(nPart, 0, 1); 	
				cbfU = (inf_tq.absSum == 0 ? 0:1);
				
#if RK_CABAC_FUNCTIONAL_TYPE
				m_cabac_rdo->cbf_chroma_u_total[nPart] = cbfU;
				m_cabac_rdo->TU_Resi_chroma_u = inf_tq.oriResi;
				m_cabac_rdo->cabac_est_functional_type_inter(1);
#endif

				inf_recon.size = inf_me.size;
				inf_recon.pred = inf_me.pred; 
	    		inf_recon.resi = inf_tq.outResi; 
				inf_recon.Recon = cost_inter.recon_u;

				pos = chromaPart2pos[nPart];
				for(k=0; k<16; k++)
				{
					memcpy(ori_data+k*16, curr_cu_u+pos+k*32, 16);
				}
				inf_recon.ori = ori_data;

				// reconstruct
				recon();

				for(k=0; k<16; k++)
				{		
					memcpy(reconFme[1]+pos+k*32, inf_recon.Recon+k*inf_recon.size, 16);
				}						

				// calc dist
				inf_inter_fme_proc.Distortion += RdoDistCalc(inf_recon.SSE, 1);

				//-------- V ---------
				// TQ proc
				inter_qt(nPart, 0, 2); 	
				cbfV = (inf_tq.absSum == 0 ? 0:1);

#if RK_CABAC_FUNCTIONAL_TYPE
				m_cabac_rdo->cbf_chroma_v_total[nPart] = cbfV;
				m_cabac_rdo->TU_Resi_chroma_v = inf_tq.oriResi;
				m_cabac_rdo->cabac_est_functional_type_inter(2);
#endif
				
				inf_recon.size = inf_me.size;
				inf_recon.pred = inf_me.pred; 
		    	inf_recon.resi = inf_tq.outResi;
		 		inf_recon.Recon = cost_inter.recon_v;

				for(k=0; k<16; k++)
				{
					memcpy(ori_data+k*16, curr_cu_v+pos+k*32, 16);
				}
				inf_recon.ori = ori_data;

				// reconstruct
				recon();
				
				for(k=0; k<16; k++)
				{		
					memcpy(reconFme[2]+pos+k*32, inf_recon.Recon+k*inf_recon.size, 16);
				}								

				// calc dist
				inf_inter_fme_proc.Distortion += RdoDistCalc(inf_recon.SSE, 2);					
			}
			
			inf_inter_fme_proc.ReconY = reconFme[0];
			inf_inter_fme_proc.ReconU = reconFme[1];
			inf_inter_fme_proc.ReconV = reconFme[2];
			
			inf_inter_fme_proc.ResiY = coeffFme[0];
			inf_inter_fme_proc.ResiU = coeffFme[1];
			inf_inter_fme_proc.ResiV = coeffFme[2];	
			
#if RK_CABAC_FUNCTIONAL_TYPE
			uint64_t cu_pu_bits = m_cabac_rdo->Inter_fme_est_bit_cu_pu(Fme.getFmeInfoForCabac());
			assert( (cu_pu_bits + m_cabac_rdo->tu_est_bits[m_cabac_rdo->IsIntra_tu][m_cabac_rdo->depth_tu] + 16384)>>15 == inf_inter_fme_proc.Bits );
#endif

			// Calc Cost
			inf_inter_fme_proc.totalCost = RdoCostCalc(inf_inter_fme_proc.Distortion, inf_inter_fme_proc.Bits, inf_tq.QP);
		}
		else // cu_w < 64
		{
			//-------- Y ---------
			// TQ proc
			inter_qt(0, 0, 0);
			Fme.getFmeInfoForCabac()->m_cbfY = cbfY;
			
#if RK_CABAC_FUNCTIONAL_TYPE
			m_cabac_rdo->IsIntra_tu = 0;
			m_cabac_rdo->depth_tu = cu_w==32 ? 1 : (cu_w == 16 ? 2 : (cu_w == 8 ? 3 : 8));
			m_cabac_rdo->zscan_idx_tu = g_rk_raster2zscan_depth_4[Offset_x/4 + Offset_y*4] ;
			m_cabac_rdo->tu_luma_idx = 0;
			m_cabac_rdo->cbf_luma_total[0] = (inf_tq.absSum == 0 ? 0:1);;

			m_cabac_rdo->TU_chroma_size = cu_w/2;
			m_cabac_rdo->TU_Resi_luma = inf_tq.oriResi;
			m_cabac_rdo->cabac_est_functional_type_inter(0);
#endif

			inf_recon.size = inf_me.size;
    		inf_recon.pred = inf_me.pred; 
    		inf_recon.resi = inf_tq.outResi;
			inf_recon.ori = curr_cu_y;
    		inf_recon.Recon = reconFme[0];

			// reconstruct
			recon();	
			
	  		inf_inter_fme_proc.ReconY = inf_recon.Recon;
			inf_inter_fme_proc.ResiY = coeffFme[0];

			// calc dist
			inf_inter_fme_proc.Distortion += RdoDistCalc(inf_recon.SSE, 0);

			//-------- U ---------
			// TQ proc
			inter_qt(0, 0, 1);
			Fme.getFmeInfoForCabac()->m_cbfU=cbfU;

#if RK_CABAC_FUNCTIONAL_TYPE
			m_cabac_rdo->cbf_chroma_u_total[0] = (inf_tq.absSum == 0 ? 0:1);;
			m_cabac_rdo->TU_Resi_chroma_u = inf_tq.oriResi;
			m_cabac_rdo->cabac_est_functional_type_inter(1);
#endif
			
			inf_recon.size = inf_me.size;
			inf_recon.pred = inf_me.pred; 
    		inf_recon.resi = inf_tq.outResi;
			inf_recon.ori = curr_cu_u;		
			inf_recon.Recon = reconFme[1];

			// reconstruct
			recon();
			
	    	inf_inter_fme_proc.ReconU 	= inf_recon.Recon;
			inf_inter_fme_proc.ResiU = coeffFme[1];

			// calc dist
			inf_inter_fme_proc.Distortion += RdoDistCalc(inf_recon.SSE, 1);

			//-------- V ---------
			// TQ proc
			inter_qt(0, 0, 2);
			Fme.getFmeInfoForCabac()->m_cbfV=cbfV;

#if RK_CABAC_FUNCTIONAL_TYPE
			m_cabac_rdo->cbf_chroma_v_total[0] = (inf_tq.absSum == 0 ? 0:1);;
			m_cabac_rdo->TU_Resi_chroma_v = inf_tq.oriResi;
			m_cabac_rdo->cabac_est_functional_type_inter(2);
#endif
			
			inf_recon.size = inf_me.size;
			inf_recon.pred = inf_me.pred; 
    		inf_recon.resi = inf_tq.outResi;
			inf_recon.ori = curr_cu_v;		
	 		inf_recon.Recon = reconFme[2];

			// reconstruct
			recon();
			
		    inf_inter_fme_proc.ReconV 	= inf_recon.Recon;
			inf_inter_fme_proc.ResiV = coeffFme[2];

			// calc dist
			inf_inter_fme_proc.Distortion += RdoDistCalc(inf_recon.SSE, 2);		
#if RK_CABAC_FUNCTIONAL_TYPE
			uint64_t cu_pu_bits = m_cabac_rdo->Inter_fme_est_bit_cu_pu(Fme.getFmeInfoForCabac());
			assert( (cu_pu_bits + m_cabac_rdo->tu_est_bits[m_cabac_rdo->IsIntra_tu][m_cabac_rdo->depth_tu] + 16384)>>15 == inf_inter_fme_proc.Bits );
#endif
			
			// Calc Cost
			inf_inter_fme_proc.totalCost = RdoCostCalc(inf_inter_fme_proc.Distortion, inf_inter_fme_proc.Bits, inf_tq.QP);
		}
#endif

		//Merge residual and MVD MVP index compare
		int stride = nCtu + 2 * (nRimeWidth + 4);
		for (int nMergeIdx = 0; nMergeIdx < 3; nMergeIdx ++)
		{
			int refIdx0 = Merge.getMergeCand(nMergeIdx, false).refIdx;
			int refIdx1 = Merge.getMergeCand(nMergeIdx, true).refIdx;
			Merge.PrefetchMergeSW(pHardWare->Rime[offsIdx].getCurrCtuRefPic(refIdx0, 1), stride,
				pHardWare->Rime[offsIdx].getCurrCtuRefPic(refIdx0, 2), stride,
				pHardWare->Rime[offsIdx].getCurrCtuRefPic((nMaxRefPic/2+refIdx1), 1), stride,
				pHardWare->Rime[offsIdx].getCurrCtuRefPic((nMaxRefPic/2+refIdx1), 2), stride, nMergeIdx); //get neighbor PMV pixel
		}

		//Merge.PrefetchMergeSW(pHardWare->Rime[offsIdx].getCurrCtuRefPic(0,1), stride, pHardWare->Rime[offsIdx].getCurrCtuRefPic(0,2), stride); //get neighbor PMV pixel
		Merge.setCurrCuPel(pHardWare->Rime[offsIdx].getCurrCuPel(), nCuSize);
		Merge.CalcMergeResi();
		if (Merge.getMergeCandValid()[0] || Merge.getMergeCandValid()[1] || Merge.getMergeCandValid()[2])
		{
			for (int m = 0; m < nCuSize; m++)
			{
				for (int n = 0; n < nCuSize; n++)
				{
					assert(g_MergeResiY[depth][(n + offset_x) + (m + offset_y) * 64] == Merge.getMergeResi()[n + m*nCuSize]);
				}
			}
			for (int m = 0; m < nCuSize / 2; m++)
			{
				for (int n = 0; n < nCuSize / 2; n++)
				{
					assert(g_MergeResiU[depth][(n + offset_x / 2) + (m + offset_y / 2) * 32] == Merge.getMergeResi()[n + m*nCuSize / 2 + nCuSize*nCuSize]);
					assert(g_MergeResiV[depth][(n + offset_x / 2) + (m + offset_y / 2) * 32] == Merge.getMergeResi()[n + m*nCuSize / 2 + nCuSize*nCuSize * 5 / 4]);
				}
			}
			
		//=================== TQ  for Merge ===================
#if TQ_RUN_IN_HWC_ME	
	    cbfY = cbfU = cbfV = false;
		if(cu_w==64)
		{
			int k, pos;
			for(nPart=0; nPart<4; nPart++)
			{
				//-------- Y ---------
				// TQ proc
				inter_qt(nPart, 1, 0); 	
				cbfY |= (inf_tq.absSum == 0 ? 0:1);
				
				inf_recon.size = inf_me.size;
    			inf_recon.pred = inf_me.pred; 
    			inf_recon.resi = inf_tq.outResi;
    			inf_recon.Recon = cost_inter.recon_y;
				
#if RK_CABAC_FUNCTIONAL_TYPE
				m_cabac_rdo->IsIntra_tu = 2;//2表示merge
				m_cabac_rdo->depth_tu = 0;
				m_cabac_rdo->zscan_idx_tu = g_rk_raster2zscan_depth_4[Offset_x/4 + Offset_y*4] ;
				m_cabac_rdo->tu_luma_idx = nPart;
				m_cabac_rdo->cbf_luma_total[nPart] = (inf_tq.absSum == 0 ? 0:1);

				m_cabac_rdo->TU_chroma_size = 16;
				m_cabac_rdo->TU_Resi_luma = inf_tq.oriResi;
				m_cabac_rdo->cabac_est_functional_type_inter(0);
#endif

				pos = lumaPart2pos[nPart];
				for(k=0; k<32; k++)
				{
					memcpy(ori_data + k*32, curr_cu_y+pos+k*64, 32);
				}	
				inf_recon.ori = ori_data;

				// reconstruct
				recon();
				
				for(k=0; k<32; k++)
				{
					memcpy(reconMerge[0]+pos+k*64, inf_recon.Recon+k*inf_recon.size, 32);				
				}	

				// calc dist
				inf_inter_merge_proc.Distortion += RdoDistCalc(inf_recon.SSE, 0);

				//-------- U ---------
				// TQ proc
				inter_qt(nPart, 1, 1); 	
				cbfU |= (inf_tq.absSum == 0 ? 0:1);
				
#if RK_CABAC_FUNCTIONAL_TYPE
				m_cabac_rdo->cbf_chroma_u_total[nPart] = (inf_tq.absSum == 0 ? 0:1);
				m_cabac_rdo->TU_Resi_chroma_u = inf_tq.oriResi;
				m_cabac_rdo->cabac_est_functional_type_inter(1);
#endif

				inf_recon.size = inf_me.size;
				inf_recon.pred = inf_me.pred; 
	    		inf_recon.resi = inf_tq.outResi; 
				inf_recon.Recon = cost_inter.recon_u;

				pos = chromaPart2pos[nPart];
				for(k=0; k<16; k++)
				{
					memcpy(ori_data+k*16, curr_cu_u+pos+k*32, 16);
				}
				inf_recon.ori = ori_data;

				// reconstruct
				recon();	

				for(k=0; k<16; k++)
				{		
					memcpy(reconMerge[1]+pos+k*32, inf_recon.Recon+k*inf_recon.size, 16);
				}	

				// calc dist
				inf_inter_merge_proc.Distortion += RdoDistCalc(inf_recon.SSE, 1);

				//-------- V ---------
				// TQ proc
				inter_qt(nPart, 1, 2); 	
				cbfV |= (inf_tq.absSum == 0 ? 0:1);

#if RK_CABAC_FUNCTIONAL_TYPE
				m_cabac_rdo->cbf_chroma_v_total[nPart] = (inf_tq.absSum == 0 ? 0:1);
				m_cabac_rdo->TU_Resi_chroma_v = inf_tq.oriResi;
				m_cabac_rdo->cabac_est_functional_type_inter(2);
#endif
				
				inf_recon.size = inf_me.size;
				inf_recon.pred = inf_me.pred; 
		    	inf_recon.resi = inf_tq.outResi;
		 		inf_recon.Recon = cost_inter.recon_v;

				for(k=0; k<16; k++)
				{
					memcpy(ori_data+k*16, curr_cu_v+pos+k*32, 16);
				}
				inf_recon.ori = ori_data;

				// reconstruct
				recon();
				
				for(k=0; k<16; k++)
				{		
					memcpy(reconMerge[2]+pos+k*32, inf_recon.Recon+k*inf_recon.size, 16);
				}				

				// calc dist
				inf_inter_merge_proc.Distortion += RdoDistCalc(inf_recon.SSE, 2);	
				
			}

			Merge.getMergeInfoForCabac()->m_cbfY = cbfY;
			Merge.getMergeInfoForCabac()->m_cbfU = cbfU;
			Merge.getMergeInfoForCabac()->m_cbfV = cbfV;
			Merge.getMergeInfoForCabac()->m_bSkipFlag = false;
			if (!(cbfY || cbfU || cbfV))
			{
				Merge.getMergeInfoForCabac()->m_bSkipFlag = true;
			}

#if RK_CABAC_FUNCTIONAL_TYPE
			uint64_t cu_pu_bits = m_cabac_rdo->Inter_merge_est_bit_cu_pu(Merge.getMergeInfoForCabac());
			assert( (cu_pu_bits + m_cabac_rdo->tu_est_bits[m_cabac_rdo->IsIntra_tu][m_cabac_rdo->depth_tu] + 16384)>>15 == inf_inter_merge_proc.Bits );
#endif

			inf_inter_merge_proc.ReconY = reconMerge[0];
			inf_inter_merge_proc.ReconU = reconMerge[1];
			inf_inter_merge_proc.ReconV = reconMerge[2];
			
			inf_inter_merge_proc.ResiY = coeffMerge[0];
			inf_inter_merge_proc.ResiU = coeffMerge[1];
			inf_inter_merge_proc.ResiV = coeffMerge[2];
			
			// Calc Cost
			inf_inter_merge_proc.totalCost = RdoCostCalc(inf_inter_merge_proc.Distortion, inf_inter_merge_proc.Bits, inf_tq.QP);

		}
		else // cu_w < 64
		{
			//-------- Y ---------
			// TQ proc
			inter_qt(0, 1, 0);
			cbfY = (inf_tq.absSum == 0 ? 0:1);

#if RK_CABAC_FUNCTIONAL_TYPE
			m_cabac_rdo->IsIntra_tu = 2;
			m_cabac_rdo->depth_tu = cu_w==32 ? 1 : (cu_w == 16 ? 2 : (cu_w == 8 ? 3 : 8));
			m_cabac_rdo->zscan_idx_tu = g_rk_raster2zscan_depth_4[Offset_x/4 + Offset_y*4] ;
			m_cabac_rdo->tu_luma_idx = 0;
			m_cabac_rdo->cbf_luma_total[0] = (inf_tq.absSum == 0 ? 0:1);

			m_cabac_rdo->TU_chroma_size = cu_w/2;
			m_cabac_rdo->TU_Resi_luma = inf_tq.oriResi;
			m_cabac_rdo->cabac_est_functional_type_inter(0);
#endif
			
			inf_recon.size = inf_me.size;
    		inf_recon.pred = inf_me.pred; 
    		inf_recon.resi = inf_tq.outResi;
			inf_recon.ori = curr_cu_y;
    		inf_recon.Recon = reconMerge[0];

			// reconstruct
			recon();
			
	  		inf_inter_merge_proc.ReconY = inf_recon.Recon;
   			inf_inter_merge_proc.ResiY 	= coeffMerge[0];		

			// calc dist
			inf_inter_merge_proc.Distortion += RdoDistCalc(inf_recon.SSE, 0);

			//-------- U ---------
			// TQ proc
			inter_qt(0, 1, 1);
			cbfU = (inf_tq.absSum == 0 ? 0:1);

#if RK_CABAC_FUNCTIONAL_TYPE
			m_cabac_rdo->cbf_chroma_u_total[0] = (inf_tq.absSum == 0 ? 0:1);;
			m_cabac_rdo->TU_Resi_chroma_u = inf_tq.oriResi;
			m_cabac_rdo->cabac_est_functional_type_inter(1);
#endif
			
			inf_recon.size = inf_me.size;
			inf_recon.pred = inf_me.pred;
    		inf_recon.resi = inf_tq.outResi;
			inf_recon.ori = curr_cu_u;
			inf_recon.Recon = cost_inter.recon_u;

			// reconstruct
			recon();
			
	    	inf_inter_merge_proc.ReconU = inf_recon.Recon;
			inf_inter_merge_proc.ResiU = coeffMerge[1];

			// calc dist
			inf_inter_merge_proc.Distortion += RdoDistCalc(inf_recon.SSE, 1);

			//-------- V ---------
			// TQ proc
			inter_qt(0, 1, 2);
			cbfV = (inf_tq.absSum == 0 ? 0:1);
			Merge.getMergeInfoForCabac()->m_cbfY = cbfY;
			Merge.getMergeInfoForCabac()->m_cbfU = cbfU;
			Merge.getMergeInfoForCabac()->m_cbfV = cbfV;
			Merge.getMergeInfoForCabac()->m_bSkipFlag = false;
			if (!(cbfY || cbfU || cbfV))
			{
				Merge.getMergeInfoForCabac()->m_bSkipFlag = true;
			}

#if RK_CABAC_FUNCTIONAL_TYPE
			m_cabac_rdo->cbf_chroma_v_total[0] = (inf_tq.absSum == 0 ? 0:1);;
			m_cabac_rdo->TU_Resi_chroma_v = inf_tq.oriResi;
			m_cabac_rdo->cabac_est_functional_type_inter(2);
#endif

			inf_recon.size = inf_me.size;
			inf_recon.pred = inf_me.pred; 
    		inf_recon.resi = inf_tq.outResi;
			inf_recon.ori = curr_cu_v;		
	 		inf_recon.Recon = reconMerge[2];

			// reconstruct
			recon();
			
		    inf_inter_merge_proc.ReconV = inf_recon.Recon;
			inf_inter_merge_proc.ResiV = coeffMerge[2];

			// calc dist
			inf_inter_merge_proc.Distortion += RdoDistCalc(inf_recon.SSE, 2);	

#if RK_CABAC_FUNCTIONAL_TYPE
			uint64_t cu_pu_bits = m_cabac_rdo->Inter_merge_est_bit_cu_pu(Merge.getMergeInfoForCabac());
			assert( (cu_pu_bits + m_cabac_rdo->tu_est_bits[m_cabac_rdo->IsIntra_tu][m_cabac_rdo->depth_tu] + 16384)>>15 == inf_inter_merge_proc.Bits );
#endif
			
			// Calc Cost
			inf_inter_merge_proc.totalCost = RdoCostCalc(inf_inter_merge_proc.Distortion, inf_inter_merge_proc.Bits, inf_tq.QP);
		}		
#endif

		}
		else
			inf_inter_merge_proc.totalCost = MAX_INT;
	}
	pHardWare->Rime[offsIdx].DestroyCuInfo();
	Fme.Destroy();
	Merge.Destroy();

}

void CU_LEVEL_CALC::intra_proc()
{
    if(cu_w == 64)
        return;

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
	uint8_t qpY = pHardWare->ctu_calc.QP_lu;
	uint8_t qpU = pHardWare->ctu_calc.QP_cb;
	uint8_t qpV = pHardWare->ctu_calc.QP_cr;
	uint8_t sliceType = pHardWare->ctu_calc.slice_type;
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

#if RK_CABAC_H||RK_CABAC_FUNCTIONAL_TYPE
	m_rkIntraPred->m_cabac_rdo = m_cabac_rdo;
#endif

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

#if RK_CABAC_H || RK_CABAC_FUNCTIONAL_TYPE
	m_cabac_rdo->cbf_luma_total[0] = inf_tq.absSum == 0 ? 0 : 1;
#endif
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

#if RK_CABAC_H || RK_CABAC_FUNCTIONAL_TYPE
	m_cabac_rdo->cbf_chroma_u_total[0] = inf_tq.absSum == 0 ? 0 : 1;
#endif
#endif
    //CABAC();

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

#if RK_CABAC_H || RK_CABAC_FUNCTIONAL_TYPE
	m_cabac_rdo->cbf_chroma_v_total[0] = inf_tq.absSum == 0 ? 0 : 1;
#endif
#endif

#if RK_CABAC_H || RK_CABAC_FUNCTIONAL_TYPE
	m_cabac_rdo->quant_finish_flag[1][depth] = 1;

	m_cabac_rdo->TU_Resi_luma = inf_tq_total[0].oriResi;
	m_cabac_rdo->TU_Resi_chroma_u = inf_tq_total[1].oriResi;
	m_cabac_rdo->TU_Resi_chroma_v = inf_tq_total[2].oriResi;

	m_cabac_rdo->TU_chroma_size = inf_tq.Size;
	m_cabac_rdo->IsIntra_tu = 1;
	m_cabac_rdo->depth_tu = depth;
	m_cabac_rdo->zscan_idx_tu = g_rk_raster2zscan_depth_4[x_pos/4 + y_pos*4];
	m_cabac_rdo->tu_luma_idx = 0;
	m_cabac_rdo->cu_qp_luma = pHardWare->ctu_calc.QP_lu;
	
#if RK_CABAC_FUNCTIONAL_TYPE
	uint64_t cu_pu_bits = m_cabac_rdo->Intra_est_bit_cu_pu();
	m_cabac_rdo->cabac_est_functional_type_intra();
#endif
	
#endif

    //CABAC();
	// store [ V output] to [inf_intra_proc]
    inf_intra_proc.ReconV 	= inf_recon.Recon;
    inf_intra_proc.ResiV 	= inf_recon.resi;

	// ---- 4. TODO RDOCostCalc for (Y + U + V) ----
	// cost = distortion + lambda * bits
#if RK_CABAC_H || RK_CABAC_FUNCTIONAL_TYPE
//	if (slice_header_struct.slice_type == 2)
	{
		assert(((m_cabac_rdo->tu_est_bits[1][depth] + cu_pu_bits)+16384)/32768 == totalBitsDepth);
	}

	inf_intra_proc.Bits		 = ((m_cabac_rdo->tu_est_bits[1][depth] + cu_pu_bits)+16384)/32768 ;
#else
	inf_intra_proc.Bits		 = totalBitsDepth;
#endif
	inf_intra_proc.totalCost = RdoCostCalc(inf_intra_proc.Distortion, totalBitsDepth, inf_tq.QP);
	//=====================================================================================//

	inf_intra_proc.partSize 		= 0; // 2Nx2N
	inf_intra_proc.predModeIntra[0] = inf_intra.DirMode;
	inf_intra_proc.predModeIntra[1] = 35;  // invalid mode
	inf_intra_proc.predModeIntra[2] = 35;  // invalid mode
	inf_intra_proc.predModeIntra[3] = 35;  // invalid mode

	if(m_size == 8)
    {
		/*
		** use 8x8 info to derivate 4x4 info
		*/
		uint8_t predModeLocal4x4[4] = {35,35,35,35};
		choose4x4split = false; // default: not split to 4x4
	    // TODO TU_chroma_size == 4
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
		uint64_t est_bits_cu_pu = 0;
		for (partIdx = 0 ; partIdx < 4 ; partIdx++ )
		{
#if RK_CABAC_H || RK_CABAC_FUNCTIONAL_TYPE
			m_cabac_rdo->est_bits_init();
#endif
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

#if RK_CABAC_FUNCTIONAL_TYPE
			m_cabac_rdo->quant_finish_flag[1][4] = 1;

			m_cabac_rdo->TU_Resi_luma = inf_tq.oriResi;
			m_cabac_rdo->TU_chroma_size = 2;
			m_cabac_rdo->cbf_luma_total[partIdx] = inf_tq.absSum == 0 ? 0 : 1;

			m_cabac_rdo->IsIntra_tu = 1;
			m_cabac_rdo->depth_tu = 4;
			m_cabac_rdo->zscan_idx_tu = g_rk_raster2zscan_depth_4[x_pos/4 + y_pos*4];

			m_cabac_rdo->tu_luma_idx = partIdx;
			est_bits_cu_pu += m_cabac_rdo->Intra_est_bit_cu_pu();
			m_cabac_rdo->cabac_est_functional_type_intra();

#endif
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
#if RK_CABAC_H || RK_CABAC_FUNCTIONAL_TYPE
		m_cabac_rdo->cbf_chroma_u_total[0] = inf_tq.absSum == 0 ? 0 : 1;
#endif
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
			if (sliceType == 2) 
			{
				RK_HEVC_FPRINT(g_fp_result_rk,"intra \n");
			}
			else
				RK_HEVC_FPRINT(g_fp_result_rk,"intraForInter \n");

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
#if RK_CABAC_FUNCTIONAL_TYPE
			m_cabac_rdo->TU_Resi_luma = coeff4x4[3];
			m_cabac_rdo->TU_Resi_chroma_u = coeff4x4[4];
			m_cabac_rdo->TU_Resi_chroma_v = coeff4x4[5];
			m_cabac_rdo->TU_chroma_size = 2;
			m_cabac_rdo->cbf_chroma_v_total[0] = inf_tq.absSum == 0 ? 0 : 1;

			m_cabac_rdo->IsIntra_tu = 1;
			m_cabac_rdo->depth_tu = 4;
			m_cabac_rdo->zscan_idx_tu = g_rk_raster2zscan_depth_4[x_pos/4 + y_pos*4];

			m_cabac_rdo->tu_luma_idx = partIdx;
			est_bits_cu_pu += m_cabac_rdo->Intra_est_bit_cu_pu();
			m_cabac_rdo->cabac_est_functional_type_intra();

			assert(((m_cabac_rdo->tu_est_bits[1][4] + est_bits_cu_pu)+16384)/32768 == totalBits4x4);


			if (inf_intra_proc.totalCost <= totalCost4x4)
				m_cabac_rdo->cu_best_mode[3] = 1;
			else 
				m_cabac_rdo->cu_best_mode[3] = 2;

			m_cabac_rdo->depth_tu = 3;
			m_cabac_rdo->Intra_cu_update();
#endif
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


    }
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
        //TODO TU_chroma_size == 4
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
		dist = (uint32_t)((SSE * pHardWare->hw_cfg.CbDistWeight + 128) >> 8);
	}
	else if(yCbCr==2) // Cr
	{
		dist = (uint32_t)((SSE * pHardWare->hw_cfg.CrDistWeight + 128) >> 8);
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
	cost = dist + (((uint64_t)bits * lambda + 32768) >> 16);

	return cost;
}

void CU_LEVEL_CALC::InterRdCompare()
{
	/* compare inter FME & Merge */
	if (inf_inter_fme_proc.totalCost <= inf_inter_merge_proc.totalCost) // choose FME
	{
		memcpy(cost_inter.recon_y, inf_inter_fme_proc.ReconY, m_size*m_size);
		memcpy(cost_inter.recon_u, inf_inter_fme_proc.ReconU, m_size*m_size / 4);
		memcpy(cost_inter.recon_v, inf_inter_fme_proc.ReconV, m_size*m_size / 4);
		memcpy(cost_inter.resi_y, inf_inter_fme_proc.ResiY, m_size*m_size * 2);
		memcpy(cost_inter.resi_u, inf_inter_fme_proc.ResiU, m_size*m_size / 2);
		memcpy(cost_inter.resi_v, inf_inter_fme_proc.ResiV, m_size*m_size / 2);
		cost_inter.TotalCost = inf_inter_fme_proc.totalCost;
		cost_inter.Distortion = inf_inter_fme_proc.Distortion;
		cost_inter.Bits = inf_inter_fme_proc.Bits;
		cost_inter.skipFlag = 0;
		cost_inter.mergeFlag = 0;
		cost_inter.partSize = inf_inter_fme_proc.partSize;
		cost_inter.predMode = inf_inter_fme_proc.predMode;
#if RK_CABAC_FUNCTIONAL_TYPE
		m_cabac_rdo->inter_cu_best_mode[m_cabac_rdo->depth_tu] = 0;
#endif
	}
	else // choose Merge
	{
		memcpy(cost_inter.recon_y, inf_inter_merge_proc.ReconY, m_size*m_size);
		memcpy(cost_inter.recon_u, inf_inter_merge_proc.ReconU, m_size*m_size / 4);
		memcpy(cost_inter.recon_v, inf_inter_merge_proc.ReconV, m_size*m_size / 4);
		memcpy(cost_inter.resi_y, inf_inter_merge_proc.ResiY, m_size*m_size * 2);
		memcpy(cost_inter.resi_u, inf_inter_merge_proc.ResiU, m_size*m_size / 2);
		memcpy(cost_inter.resi_v, inf_inter_merge_proc.ResiV, m_size*m_size / 2);
		cost_inter.TotalCost = inf_inter_merge_proc.totalCost;
		cost_inter.Distortion = inf_inter_merge_proc.Distortion;
		cost_inter.Bits = inf_inter_merge_proc.Bits;
		cost_inter.skipFlag = 0;
		cost_inter.mergeFlag = 1;
		cost_inter.partSize = inf_inter_merge_proc.partSize;
		cost_inter.predMode = inf_inter_merge_proc.predMode;
#if RK_CABAC_FUNCTIONAL_TYPE
		m_cabac_rdo->inter_cu_best_mode[m_cabac_rdo->depth_tu] = 1;
#endif
	}
}
void CU_LEVEL_CALC::RDOCompare()
{
	/*
    if ((cu_w == 8) && (cost_intra.TotalCost > cost_intra_4.TotalCost))
    {
        cost_intra = cost_intra_4;
    }
	*/
	if(2 == pHardWare->ctu_calc.slice_type) // only intra
	{
        cost_best = &cost_intra;
        cost_best->predMode = 1;
	}
	else if(cu_w==64) // only inter do 64x64
	{
		cost_best = &cost_inter;
        cost_best->predMode = 0;
	}
	else // normal case
	{
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
}

unsigned int CU_LEVEL_CALC::proc(unsigned int level, unsigned int pos_x, unsigned int pos_y)
{
    hardwareC *p = pHardWare;
    cuData *dst_cu, *src_cu;
    uint8_t i;
    MV_INFO mv_zero;
	uint32_t cost;

    memset(&mv_zero, 0x0, sizeof(MV_INFO));
    x_pos = (uint8_t)pos_x;
    y_pos = (uint8_t)pos_y;
    valid = 1;



    p->ctu_calc.cu_valid_flag[depth][ori_pos] = 1;

    if((((p->ctu_x * p->ctu_w + pos_x) < p->pic_w)) && ((p->ctu_y * p->ctu_w + pos_y) < p->pic_h)) {
        temp_pos++;
    }

    if((((p->ctu_x * p->ctu_w + pos_x + (1 << (6 - level))) > p->pic_w)) ||
         ((p->ctu_y * p->ctu_w + pos_y + (1 << (6 - level))) > p->pic_h)) {
        p->ctu_calc.cu_valid_flag[depth][ori_pos] = 0;
#if RK_CABAC_FUNCTIONAL_TYPE
		m_cabac_rdo->bits_split_cu_flag[depth] = 0;
#endif
        return 0xffffffff;
    }

	begin();

#if RK_CABAC_FUNCTIONAL_TYPE
	m_cabac_rdo->depth_tu = level;
	m_cabac_rdo->ctu_x = pHardWare->ctu_x;
	m_cabac_rdo->ctu_y = pHardWare->ctu_y;
	memcpy(m_cabac_rdo->mModels_of_CU_PU , m_cabac_rdo->mModels[1] , sizeof(m_cabac_rdo->mModels_of_CU_PU)) ;
#endif

#if RK_INTER_METEST
	if (2 != pHardWare->ctu_calc.slice_type)
	{
		int offsIdx = 84;
		if (1 == level)
			offsIdx = 80 + x_pos / 32 + y_pos / 32 * 2;
		else if (2 == level)
			offsIdx = 64 + x_pos / 16 + y_pos / 16 * 4;
		else if (3 == level)
			offsIdx = x_pos / 8 + y_pos / 8 * 8;
		
 		inter_proc(offsIdx);
		
		InterRdCompare();
	}
#endif
    intra_proc();
	
    cost_intra.predMode = 1;
    cost_intra.TotalCost = inf_intra_proc.totalCost;
    cost_intra.Distortion = inf_intra_proc.Distortion;
    cost_intra.Bits = inf_intra_proc.Bits;
	RDOCompare();
    //cost_best = &cost_intra;

#if RK_CABAC_FUNCTIONAL_TYPE
	m_cabac_rdo->cu_best_mode[level] = cost_best->predMode;
#endif
	
	
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
    cost_best->skipFlag = src_cu->cuSkipFlag[0];
    cost_best->intraDir[0] = inf_intra_proc.predModeIntra[0];
    cost_best->intraDir[1] = inf_intra_proc.predModeIntra[1];
    cost_best->intraDir[2] = inf_intra_proc.predModeIntra[2];
    cost_best->intraDir[3] = inf_intra_proc.predModeIntra[3];

    //------------------------------------------//

    memcpy(dst_cu->ReconY, cost_best->recon_y, cu_w*cu_w);
    memcpy(dst_cu->ReconU, cost_best->recon_u, cu_w*cu_w/4);
    memcpy(dst_cu->ReconV, cost_best->recon_v, cu_w*cu_w/4);

    if (cost_best->predMode == 1) {
        for(i=0; i<(cu_w/8)*(cu_w/8); i++) {
            dst_cu->mv[i] = mv_zero;
        }
    }
    else {
        for(i=0; i<(cu_w/8)*(cu_w/8); i++) {
            dst_cu->mv[i] = cost_best->mv;
        }
    }

    memset(cu_matrix_data[matrix_pos].cuPredMode, cost_best->predMode, (cu_w/8)*(cu_w/8));
    memset(cu_matrix_data[matrix_pos].cuPartSize, cost_best->partSize, (cu_w/8)*(cu_w/8));
    memset(cu_matrix_data[matrix_pos].cuSkipFlag, cost_best->skipFlag, (cu_w/8)*(cu_w/8));

    cost = (uint32_t)src_cu->totalCost;

#if RK_CABAC_FUNCTIONAL_TYPE
	uint32_t zscan = g_rk_raster2zscan_depth_4[x_pos/4 + y_pos*4];
	m_cabac_rdo->ctu_x = pHardWare->ctu_x;
	int bits_split_cu_flag = m_cabac_rdo->est_bits_split_cu_flag(depth  , zscan);
	if (cost_best->predMode == 1 && depth!=0)
	{
		int test = RdoCostCalc(inf_intra_proc.Distortion, inf_intra_proc.Bits + bits_split_cu_flag, inf_tq.QP);
		assert(cost == test);  
		cost_best->Bits = inf_intra_proc.Bits + bits_split_cu_flag;
		cost_best->Distortion = inf_intra_proc.Distortion;
	}
	else if (cost_best->predMode == 0)
	{
		int test = RdoCostCalc(cost_inter.Distortion, cost_inter.Bits + bits_split_cu_flag, inf_tq.QP);
		assert(cost == test);  
		cost_best->Bits = cost_inter.Bits + bits_split_cu_flag;
		cost_best->Distortion = cost_inter.Distortion;
	}

	
#endif


	return cost;
	//return cost_best->TotalCost;
}
