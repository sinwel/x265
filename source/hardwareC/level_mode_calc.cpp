#include <stdio.h>
#include "level_mode_calc.h"
#include "hardwareC.h"

#ifndef CLIP
#define CLIP(a, min, max)			(((a) < min) ? min : (((a) > max) ? max : (a)))
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
}

void CU_LEVEL_CALC::deinit()
{
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
}


void CU_LEVEL_CALC::intra_neighbour_flag_descion()
{
    if(cu_w == 64)
        return;

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

    /* bottom_left */
    if(bottom_left_valid) {
        bottom_left_valid = 0;
        if(0) { //constrain_intra
            for (m=0; m<cu_w/8; m++) {
                inf_intra.NeighborFlags[cu_w/8 - m - 1] = pHardWare->ctu_calc.intra_buf_cu_type[(x_pos/8 - 1) - (y_pos/8 + m + cu_w/8) + 8];
                bottom_left_valid |= inf_intra.NeighborFlags[cu_w/8 - m - 1];
            }
        }
        else {
            for (m=0; m<cu_w/8; m++)
                inf_intra.NeighborFlags[m] = 1;
            bottom_left_valid |= 1;
        }
    }
    else {
        for (m=0; m<cu_w/8; m++)
            inf_intra.NeighborFlags[m] = 0;
    }

    /* left */
    if(left_valid) {
        left_valid = 0;
        if(0) { //constrain_intra
            for (m=0; m<cu_w/8; m++) {
                inf_intra.NeighborFlags[2*cu_w/8 - m - 1] = pHardWare->ctu_calc.intra_buf_cu_type[(x_pos/8 - 1) - (y_pos/8 + m) + 8];
                left_valid |= inf_intra.NeighborFlags[2*cu_w/8 - m - 1];
            }
        }
        else {
            for (m=0; m<cu_w/8; m++)
                inf_intra.NeighborFlags[m + cu_w/8] = 1;
            left_valid |= 1;
        }
    }
    else {
        for (m=0; m<cu_w/8; m++)
            inf_intra.NeighborFlags[m + cu_w/8] = 0;
    }


    /* top_left */
    if(top_left_valid) {
        top_left_valid = 0;
        if(0) { //constrain_intra
            inf_intra.NeighborFlags[8] = pHardWare->ctu_calc.intra_buf_cu_type[(x_pos/8) - (y_pos/8) + 8];
            top_left_valid |= inf_intra.NeighborFlags[8];
        }
        else {
            inf_intra.NeighborFlags[8] = 1;
            inf_intra.NeighborFlags[8] |= 1;
        }
    }
    else {
        inf_intra.NeighborFlags[8] = 0;
    }


    /* top */
    if(top_valid) {
        top_valid = 0;
        if(0) { //constrain_intra
            for(m=0; m<cu_w/8; m++) {
                inf_intra.NeighborFlags[8 + m + 1] = pHardWare->ctu_calc.intra_buf_cu_type[(x_pos/8 + m + 1) - (y_pos/8) + 8];
                top_valid |= inf_intra.NeighborFlags[8 + m + 1];
            }
        }
        else {
            for(m=0; m<cu_w/8; m++)
                inf_intra.NeighborFlags[8 + m + 1] = 1;
            top_valid |= 1;
        }
    }
    else {
        for(m=0; m<cu_w/8; m++)
            inf_intra.NeighborFlags[8 + m + 1] = 0;
    }

    /* top_right */
    if(top_right_valid) {
        top_right_valid = 0;
        if(0) { //constrain_intra
            for(m=0; m<cu_w/8; m++) {
                inf_intra.NeighborFlags[8 + m + 1 + cu_w/8] = pHardWare->ctu_calc.intra_buf_cu_type[((x_pos + cu_w)/8 + m + 1) - (y_pos/8) + 8];
                top_right_valid |= inf_intra.NeighborFlags[8 + m + 1 + cu_w/8];
            }
        }
        else {
            for(m=0; m<cu_w/8; m++)
                inf_intra.NeighborFlags[8 + m + 1 + cu_w/8] = 1;
            top_right_valid |= 1;
        }
    }
    else {
        for(m=0; m<cu_w/8; m++)
            inf_intra.NeighborFlags[8 + m + 1 + cu_w/8] = 0;
    }

    inf_intra.numintraNeighbor = (uint8_t)(top_valid + top_right_valid + top_left_valid + left_valid + bottom_left_valid);

}
void CU_LEVEL_CALC::begin()
{
    uint8_t m;
    uint32_t pos = x_pos + y_pos*64;
    for(m=0; m<cu_w; m++)
        memcpy(curr_cu_y, pHardWare->ctu_calc.input_curr_y + pos, cu_w);
    pos = x_pos/2 + (y_pos/2)*32;
    for(m=0; m<cu_w/2; m++) {
        memcpy(curr_cu_u, pHardWare->ctu_calc.input_curr_u + pos, cu_w/2);
        memcpy(curr_cu_v, pHardWare->ctu_calc.input_curr_v + pos, cu_w/2);
    }

    intra_neighbour_flag_descion();
}

void CU_LEVEL_CALC::end()
{
    uint8_t m, n;
    cuData  *cu_src;

    cu_src = pHardWare->ctu_calc.cu_temp_data[depth][temp_pos];
    /* Y */
    for(m=0; m<cu_w; m++){
        pHardWare->ctu_calc.intra_buf_y[(x_pos + cu_w - 1) - (y_pos + m) + 64] = cu_src->ReconY[m*cu_w + cu_w - 1];
        pHardWare->ctu_calc.intra_buf_y[(x_pos + m) - (y_pos + cu_w - 1) + 64] = cu_src->ReconY[(cu_w - 1)*cu_w + m];
     }

    /* U */
    for(m=0; m<cu_w/2; m++){
        pHardWare->ctu_calc.intra_buf_u[(x_pos/2 + cu_w/2 - 1) - (y_pos/2 + m) + 32] = cu_src->ReconU[m*cu_w/2 + cu_w/2 - 1];
        pHardWare->ctu_calc.intra_buf_u[(x_pos/2 + m) - (y_pos/2 + cu_w/2 - 1) + 32] = cu_src->ReconU[(cu_w/2 - 1)*cu_w/2 + m];
    }

    /* V */
    for(m=0; m<cu_w/2; m++){
        pHardWare->ctu_calc.intra_buf_v[(x_pos/2 + cu_w/2 - 1) - (y_pos/2 + m) + 32] = cu_src->ReconV[m*cu_w/2 + cu_w/2 - 1];
        pHardWare->ctu_calc.intra_buf_v[(x_pos/2 + m) - (y_pos/2 + cu_w/2 - 1) + 32] = cu_src->ReconV[(cu_w/2 - 1)*cu_w/2 + m];
    }

    /* cu_type */
    for(m=0; m<cu_w/8; m++) {
        for(n=0; n<cu_w/8; n++) {
            pHardWare->ctu_calc.intra_buf_cu_type[(x_pos/8 + n) - (y_pos/8 + m) + 8] = (!cost_best->predMode);
        }
    }

    /* inter cbf for BS */
    for(m=0; m<cu_w/4; m++) {
        pHardWare->ctu_calc.inter_cbf[(x_pos/4 + cu_w/4 - 1) - (y_pos/4 + m) + 16] = cbfY;
        pHardWare->ctu_calc.inter_cbf[(x_pos/4 + m) - (y_pos/4 + cu_w/4 - 1) + 16] = cbfY;
    }
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

    inf_recon.pred = inf_intra.pred;
    inf_recon.resi = inf_tq.outResi;
    /*Y*/
    inf_recon.size = m_size;
    inf_recon.Recon = cost_intra.recon_y;
    inf_intra.fenc = curr_cu_y;
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.intra_buf_y + x_pos - y_pos + 64;
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
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.intra_buf_u + x_pos/2 - y_pos/2 + 32;
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
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.intra_buf_v + x_pos/2 - y_pos/2 + 32;
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
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.intra_buf_y + x_pos - y_pos + 64;
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
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.intra_buf_u + x_pos/2 - y_pos/2 + 32;
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
    inf_intra.reconEdgePixel = pHardWare->ctu_calc.intra_buf_v + x_pos/2 - y_pos/2 + 32;
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

    if((((p->ctu_x * p->ctu_w + pos_x + (1 << (6 - level))) > p->pic_w)) ||
         ((p->ctu_y * p->ctu_w + pos_y + (1 << (6 - level))) > p->pic_h)) {
        p->ctu_calc.cu_valid_flag[depth][ori_pos] = 0;
        return 0xffffffff;
    }

    begin();
    //inter_proc();
    //intra_proc();
    //intra_4_proc();

    //RDOCompare();

    cost_best = &cost_intra;


    //-------------------------------------------//
    // temp use for debug                        //
    //-------------------------------------------//
    src_cu = pHardWare->ctu_calc.cu_ori_data[depth][cu_pos++];
    dst_cu = pHardWare->ctu_calc.cu_temp_data[depth][temp_pos];

    memcpy(dst_cu->ReconY, src_cu->ReconY, cu_w*cu_w);
    memcpy(dst_cu->ReconU, src_cu->ReconU, cu_w*cu_w/4);
    memcpy(dst_cu->ReconV, src_cu->ReconV, cu_w*cu_w/4);

    memcpy(dst_cu->CoeffY, src_cu->CoeffY, cu_w*cu_w*2);
    memcpy(dst_cu->CoeffU, src_cu->CoeffU, cu_w*cu_w/2);
    memcpy(dst_cu->CoeffV, src_cu->CoeffV, cu_w*cu_w/2);

    cost = (uint32_t)src_cu->totalCost;

	return cost;
}
