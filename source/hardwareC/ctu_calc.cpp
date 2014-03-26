#include "ctu_calc.h"
#include "hardwareC.h"



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
    cbfY = (uint8_t *)malloc(64);

    cu_level_calc[0].init(64);
    cu_level_calc[1].init(32);
    cu_level_calc[2].init(16);
    cu_level_calc[3].init(8);

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



    cu_temp_data = new cuData**[4];

    cu_temp_data[0] = new cuData*[1];
	cu_temp_data[0][0] = new cuData;
    cu_temp_data[0][0]->init(64);

	cu_temp_data[1] = new cuData*[4];
    for(i=0; i<4; i++) {
        cu_temp_data[1][i] = new cuData;
        cu_temp_data[1][i]->init(32);
    }

	cu_temp_data[2] = new cuData*[4];
    for(i=0; i<4; i++) {
        cu_temp_data[2][i] = new cuData;
        cu_temp_data[2][i]->init(16);
    }

	cu_temp_data[3] = new cuData*[4];
    for(i=0; i<4; i++) {
        cu_temp_data[3][i] = new cuData;
        cu_temp_data[3][i]->init(8);
    }
}

CTU_CALC::~CTU_CALC()
{
    uint8_t i;

    cu_level_calc[0].deinit();
    cu_level_calc[1].deinit();
    cu_level_calc[2].deinit();
    cu_level_calc[3].deinit();

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




    cu_temp_data[0][0]->deinit();
    delete cu_temp_data[0][0];
    delete [] cu_temp_data[0];

    for(i=0; i<4; i++) {
        cu_temp_data[1][i]->deinit();
        delete cu_temp_data[1][i];
    }
    delete [] cu_temp_data[1];

    for(i=0; i<4; i++) {
        cu_temp_data[2][i]->deinit();
        delete cu_temp_data[2][i];
    }
    delete [] cu_temp_data[2];

    for(i=0; i<4; i++) {
		cu_temp_data[3][i]->deinit();
        delete cu_temp_data[3][i];
    }
    delete [] cu_temp_data[3];
    delete [] cu_temp_data;
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
    uint32_t i;
    uint32_t pos;
    uint32_t len;
    uint8_t *buf_y, *buf_u, *buf_v, *buf_t;
    struct MV *buf_mv;

    /* intra */
    pos = ctu_x;
    buf_y = line_intra_buf_y + (pos);
    buf_u = line_intra_buf_u + (pos >> 1);
    buf_v = line_intra_buf_v + (pos >> 1);
    buf_t = line_intra_cu_type + (pos >> 8);
    buf_mv= line_mv_buf + (pos >> 8);

    if(pos) {
        memcpy(intra_buf_y + 64, buf_y - 1, 1);
        memcpy(intra_buf_u + 32, buf_u - 1, 1);
        memcpy(intra_buf_v + 32, buf_v - 1, 1);
        memcpy(intra_buf_cu_type + 8, buf_t - 1, 1);
    }

    memcpy(intra_buf_y + 65, buf_y, ctu_w);
    memcpy(intra_buf_u + 33, buf_u, ctu_w >> 1);
    memcpy(intra_buf_v + 33, buf_v, ctu_w >> 1);
    memcpy(intra_buf_cu_type + 9, buf_t, ctu_w >> 8);

    for(i=0; i<ctu_w/8; i++)
    {
        inter_cbf[17 + i*2 + 0] = (intra_buf_cu_type[9 + i] >> 1) & 1;
        inter_cbf[17 + i*2 + 1] = (intra_buf_cu_type[9 + i] >> 2) & 1;
    }


    len = ctu_w;
    if(pos + 2*ctu_w > pHardWare->pic_w) {
        len = pHardWare->pic_w - (pos + ctu_w);
    }

    memcpy(intra_buf_y + 65 + (ctu_w),      buf_y + (ctu_w),      len);
    memcpy(intra_buf_u + 33 + (ctu_w >> 1), buf_u + (ctu_w >> 1), len >> 1);
    memcpy(intra_buf_v + 33 + (ctu_w >> 1), buf_v + (ctu_w >> 1), len >> 1);
    memcpy(intra_buf_cu_type + 9 + (ctu_w >> 8), buf_t + (ctu_w >> 8), len >> 8);

    /* inter */
    if(pos) {
        memcpy(mv_buf + 8, buf_mv - 1, sizeof(MV));
    }

    memcpy(mv_buf + 9, buf_mv, (ctu_w >> 8)*sizeof(MV));

    memcpy(mv_buf + 9 + (ctu_w >> 8), buf_mv + (ctu_w >> 8), (len >> 8)*sizeof(MV));

    valid_left = 1;
    valid_left_top = 1;
    valid_top = 1;
    valid_top_right = 1;

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

    /* CTU开始前，将cu_data_valid清0 */
    memset(cu_data_valid, 0, 8*8);
}

void CTU_CALC::end()
{
    uint32_t pos;
    uint8_t *buf_y, *buf_u, *buf_v, *buf_t;
    uint8_t i;
    struct MV *buf_mv;

    /* intra */

    pos = ctu_x;
    buf_y = line_intra_buf_y + (pos);
    buf_u = line_intra_buf_u + (pos >> 1);
    buf_v = line_intra_buf_v + (pos >> 1);
    buf_t = line_intra_cu_type + (pos >> 8);

    /* ver to hor */
    for(i=0; i<ctu_w; i++){
        buf_y[i] = intra_buf_y[64 - ctu_w + i + 1];
    }

    for(i=0; i<ctu_w>>1; i++){
        buf_u[i] = intra_buf_u[32 - (ctu_w>>1) + i + 1];
        buf_v[i] = intra_buf_v[32 - (ctu_w>>1) + i + 1];
    }

    for(i=0; i<ctu_w>>8; i++){
        buf_t[i] = intra_buf_cu_type[8 - ctu_w/8 + i + 1] | (inter_cbf[16 - ctu_w/4 + i*2 + 1] << 1) | (inter_cbf[16 - ctu_w/4 + i*2 + 2] << 2);
    }

    /* hor to ver */
    for(i=0; i<ctu_w; i++){
        intra_buf_y[i] = intra_buf_y[64 + i];
    }

    for(i=0; i<ctu_w>>1; i++){
        intra_buf_u[i] = intra_buf_u[32 + i];
        intra_buf_v[i] = intra_buf_v[32 + i];
    }

    for(i=0; i<ctu_w>>8; i++){
        intra_buf_cu_type[i] = intra_buf_cu_type[8 + i];

        inter_cbf[i*2 + 0] = inter_cbf[16 + i*2 + 0];
        inter_cbf[i*2 + 1] = inter_cbf[16 + i*2 + 1];
    }


	/* inter */
    pos = ctu_x * ctu_w;

    /* ver to hor */
    buf_mv = line_mv_buf + (pos>>8);

    for(i=0; i<(ctu_w>>8); i++){
        buf_mv[i] = mv_buf[8 - ctu_w/8 + i];
    }

    /* hor to ver */
    for(i=0; i<(ctu_w>>8); i++){
        mv_buf[i] = mv_buf[8 + i];
    }
}


void CTU_CALC::cu_level_compare(uint32_t bestCU_cost, uint32_t tempCU_cost, uint8_t depth)
{
    class cuData *src_cu, *dst_cu;
    uint8_t m, n;
    uint8_t cu_w = cu_level_calc[depth + 1].cu_w;
    uint32_t len = cu_w * cu_w;
    uint32_t pos;

    pHardWare->ctu_calc.cu_split_flag[depth*4 + cu_level_calc[depth].ori_pos] = 0;

    dst_cu = cu_temp_data[depth][cu_level_calc[depth].temp_pos];
    if((bestCU_cost > tempCU_cost) || (cu_valid_flag[depth][cu_level_calc[depth].ori_pos] == 0))
    {
        for(m=0; m<4; m++)
        {
            src_cu = cu_temp_data[depth + 1][m];
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
        }

        pHardWare->ctu_calc.cu_split_flag[depth*4 + cu_level_calc[depth].ori_pos] = 1;
    }
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
            if(cu_temp_data[0][0]->ReconY[j*ctu_w + i]!= (ori_recon_y[j*ctu_w + i])) {
                printf("ERROR RECON y is diff y %d x %d\n", j, i);
                assert(0);
            }
        }
    }

    for(j=0; j<h/2; j++)
    {
        for(i=0; i<w/2; i++)
        {
            if(cu_temp_data[0][0]->ReconU[j*ctu_w/2 + i] != (ori_recon_u[j*ctu_w/2 + i])) {
                printf("ERROR RECON u is diff y %d x %d\n", j, i);
                assert(0);
            }
        }
    }

    for(j=0; j<h/2; j++)
    {
        for(i=0; i<w/2; i++)
        {
            if (cu_temp_data[0][0]->ReconV[j*ctu_w/2 + i] != (ori_recon_v[j*ctu_w/2 + i])) {
                printf("ERROR RECON v is diff y %d x %d\n", j, i);
                assert(0);
            }
        }
    }

    for(j=0; j<h; j++)
    {
        for(i=0; i<w; i++)
        {
            if(cu_temp_data[0][0]->CoeffY[j*ctu_w + i] != (ori_resi_y[j*ctu_w + i])) {
                printf("ERROR RESI y is diff y %d x %d\n", j, i);
                assert(0);
            }
        }
    }

    for(j=0; j<h/2; j++)
    {
        for(i=0; i<w/2; i++)
        {
            if(cu_temp_data[0][0]->CoeffU[j*ctu_w/2 + i] != (ori_resi_u[j*ctu_w/2 + i])) {
                printf("ERROR RESI u is diff y %d x %d\n", j, i);
                assert(0);
            }
        }
    }

    for(j=0; j<h/2; j++)
    {
        for(i=0; i<w/2; i++)
        {
            if(cu_temp_data[0][0]->CoeffV[j*ctu_w/2 + i] != (ori_resi_v[j*ctu_w/2 + i])) {
                printf("ERROR RESI v is diff y %d x %d\n", j, i);
                assert(0);
            }
        }
    }
}
void CTU_CALC::proc()
{
    unsigned int cost_0, cost_1, cost_2, cost_3;
    unsigned int cost_1_total = 0, cost_2_total = 0, cost_3_total = 0;
    unsigned int totalBits_0 = 0, totalBits_1 = 0, totalBits_2 = 0, totalBits_3 = 0;
    unsigned int totalDist_0 = 0, totalDist_1 = 0, totalDist_2 = 0, totalDist_3 = 0;
    unsigned int m, n, k;
    unsigned int cu_x_1 = 0, cu_y_1 = 0;
    unsigned int cu_x_2 = 0, cu_y_2 = 0;
    unsigned int cu_x_3 = 0, cu_y_3 = 0;

    //IME

    /*begin*/
    begin();

    cu_level_calc[0].depth = 0;
    cu_level_calc[0].TotalCost = 0;
    cost_0 = cu_level_calc[0].proc(0, 0, 0);

    totalBits_1 = 0;
    totalDist_1 = 0;
    for(m=0; m<4; m++)
    {
        cu_level_calc[1].depth = 1;
        cu_level_calc[1].temp_pos = m;
        cu_x_1 = (m & 1)*32;
        cu_y_1 = (m >>1)*32;

        cost_1 = cu_level_calc[1].proc(1, cu_x_1, cu_y_1);

        if (!(cost_1 & 0x80000000)) {
            totalBits_1 += cu_level_calc[1].cost_best->Bits;
            totalDist_1 += cu_level_calc[1].cost_best->Distortion;
        }

        totalBits_2 = 0;
        totalDist_2 = 0;
        for(n=0; n<4; n++)
        {
            cu_level_calc[2].depth = 2;
            cu_level_calc[2].temp_pos = n;
            cu_x_2 = cu_x_1 + (n & 1)*16;
            cu_y_2 = cu_y_1 + (n >>1)*16;

            cost_2 = cu_level_calc[2].proc(2, cu_x_2, cu_y_2);

            if (!(cost_2 & 0x80000000)) {
                totalBits_2 += cu_level_calc[2].cost_best->Bits;
                totalDist_2 += cu_level_calc[2].cost_best->Distortion;
            }

            totalBits_3 = 0;
            totalDist_3 = 0;
            for(k=0; k<4; k++)
            {
                cu_level_calc[3].depth = 3;
                cu_level_calc[3].temp_pos = k;
                cu_x_3 = cu_x_2 + (k & 1)*8;
                cu_y_3 = cu_y_2 + (k >>1)*8;

                cost_3 = cu_level_calc[3].proc(3, cu_x_3, cu_y_3);
                cu_level_calc[3].end();
                cu_level_calc[3].ori_pos++;

                if (!(cost_3 & 0x80000000)) {
                    totalBits_3 += cu_level_calc[3].cost_best->Bits;
                    totalDist_3 += cu_level_calc[3].cost_best->Distortion;
                }
            }
            //EncoderCuSplitFlag();
            //calcRDOCOST
            cost_3_total = (uint32_t)pHardWare->ctu_calc.intra_temp_16[cu_level_calc[2].ori_pos].m_totalCost;
            cu_level_compare(cost_2, cost_3_total, 2);
            cu_level_calc[2].end();
            cu_level_calc[2].ori_pos++;
        }
        //EncoderCuSplitFlag();
        //calcRDOCOST
        cost_2_total = (uint32_t)pHardWare->ctu_calc.intra_temp_32[cu_level_calc[1].ori_pos].m_totalCost;
        cu_level_compare(cost_1, cost_2_total, 1);
        cu_level_calc[1].end();
        cu_level_calc[1].ori_pos++;
    }
    //EncoderCuSplitFlag();
    //calcRDOCOST
    cost_1_total = (uint32_t)pHardWare->ctu_calc.intra_temp_64.m_totalCost;
    cu_level_compare(cost_0, cost_1_total, 0);
    cu_level_calc[0].end();

    /* end */
    end();


    /* for test */
    compare();
}

