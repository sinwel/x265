#include <stdio.h>
#include "macro.h"
#include "hardwareC.h"

hardwareC G_hardwareC;

hardwareC::hardwareC()
{
    uint8_t i;
    inf_dblk.recon_y = new uint8_t*[MAX_NUM_CTU_W];
    inf_dblk.recon_u = new uint8_t*[MAX_NUM_CTU_W];
    inf_dblk.recon_v = new uint8_t*[MAX_NUM_CTU_W];
    inf_dblk.cu_depth = new uint8_t*[MAX_NUM_CTU_W];
    inf_dblk.pu_depth = new uint8_t*[MAX_NUM_CTU_W];
    inf_dblk.tu_depth = new uint8_t*[MAX_NUM_CTU_W];
    inf_dblk.mv_info = new MV_INFO*[MAX_NUM_CTU_W];
    inf_dblk.intra_bs_flag = new uint8_t*[MAX_NUM_CTU_W];
    inf_dblk.qp = new uint8_t*[MAX_NUM_CTU_W];
    for(i=0; i<MAX_NUM_CTU_W; i++) {
        inf_dblk.recon_y[i] = new uint8_t[64*64];
        inf_dblk.recon_u[i] = new uint8_t[32*32];
        inf_dblk.recon_v[i] = new uint8_t[32*32];
        inf_dblk.cu_depth[i] = new uint8_t[64];
        inf_dblk.pu_depth[i] = new uint8_t[256];
        inf_dblk.tu_depth[i] = new uint8_t[256];
        inf_dblk.mv_info[i] = new MV_INFO[64];
        inf_dblk.intra_bs_flag[i] = new uint8_t[256];
        inf_dblk.qp[i] = new uint8_t[256];
    }
}

hardwareC::~hardwareC()
{
    uint8_t i;
    for(i=0; i<MAX_NUM_CTU_W; i++) {
        delete []inf_dblk.recon_y[i];
        delete []inf_dblk.recon_u[i];
        delete []inf_dblk.recon_v[i];
        delete [] inf_dblk.cu_depth[i];
        delete [] inf_dblk.pu_depth[i];
        delete [] inf_dblk.tu_depth[i];
        delete [] inf_dblk.mv_info[i];
        delete [] inf_dblk.intra_bs_flag[i];
        delete [] inf_dblk.qp[i];
    }
    delete [] inf_dblk.recon_y;
    delete [] inf_dblk.recon_u;
    delete [] inf_dblk.recon_v;
    delete [] inf_dblk.cu_depth;
    delete [] inf_dblk.pu_depth;
    delete [] inf_dblk.tu_depth;
    delete [] inf_dblk.mv_info;
    delete [] inf_dblk.intra_bs_flag;
    delete [] inf_dblk.qp;
}

void hardwareC::init()
{
    ctu_calc.ctu_w = ctu_w;
    ctu_calc.pHardWare = this;

    inf_ctu_calc.input_curr_y = ctu_calc.input_curr_y;
    inf_ctu_calc.input_curr_u = ctu_calc.input_curr_u;
    inf_ctu_calc.input_curr_v = ctu_calc.input_curr_v;
}

void hardwareC::proc()
{
	int nRefPicList0 = Cime.getRefPicNum(REF_PIC_LIST0);
	int nRefPicList1 = Cime.getRefPicNum(REF_PIC_LIST1);
	if (Cime.getSliceType() == b_slice)
	{
		for (int nRefPicIdx = 0; nRefPicIdx < nRefPicList0; nRefPicIdx++)
		{
			Cime.CIME(Cime.getOrigPic(), Cime.getRefPicDS(nRefPicIdx), nRefPicIdx);
			assert(Cime.getCimeMv(nRefPicIdx).x == g_leftPMV[nRefPicIdx].x);
			assert(Cime.getCimeMv(nRefPicIdx).y == g_leftPMV[nRefPicIdx].y);
			assert(Cime.getCimeMv(nRefPicIdx).nSadCost == g_leftPMV[nRefPicIdx].nSadCost);
		}
		for (int nRefPicIdx = 0; nRefPicIdx < nRefPicList1; nRefPicIdx++)
		{
			Cime.CIME(Cime.getOrigPic(), Cime.getRefPicDS(nRefPicIdx + nRefPicList0), nRefPicIdx + nRefPicList0);
			assert(Cime.getCimeMv(nRefPicIdx + nRefPicList0).x == g_leftPMV[nMaxRefPic / 2 + nRefPicIdx].x);
			assert(Cime.getCimeMv(nRefPicIdx + nRefPicList0).y == g_leftPMV[nMaxRefPic / 2 + nRefPicIdx].y);
			assert(Cime.getCimeMv(nRefPicIdx + nRefPicList0).nSadCost == g_leftPMV[nMaxRefPic / 2 + nRefPicIdx].nSadCost);
		}
	}
	else
	{
		for (int nRefPicIdx = 0; nRefPicIdx < nRefPicList0; nRefPicIdx++)
		{
			Cime.CIME(Cime.getOrigPic(), Cime.getRefPicDS(nRefPicIdx), nRefPicIdx);
			assert(Cime.getCimeMv(nRefPicIdx).x == g_leftPMV[nRefPicIdx].x);
			assert(Cime.getCimeMv(nRefPicIdx).y == g_leftPMV[nRefPicIdx].y);
			assert(Cime.getCimeMv(nRefPicIdx).nSadCost == g_leftPMV[nRefPicIdx].nSadCost);
		}
	}

	for (int i = 0; i < 85; i++)
	{
		if (b_slice == Rime[i].getSliceType())
		{
			for (int nRefPicIdx = 0; nRefPicIdx < nRefPicList0; nRefPicIdx++)
			{
				if (Rime[i].PrefetchCuInfo(nRefPicIdx))
				{
					Rime[i].RimeAndFme(nRefPicIdx);
					//RIME compare
					assert(Rime[i].getRimeMv(nRefPicIdx).x == g_RimeMv[nRefPicIdx][i].x);
					assert(Rime[i].getRimeMv(nRefPicIdx).y == g_RimeMv[nRefPicIdx][i].y);
					assert(Rime[i].getRimeMv(nRefPicIdx).nSadCost == g_RimeMv[nRefPicIdx][i].nSadCost);
					//FME compare
					assert(Rime[i].getFmeMv(nRefPicIdx).x == g_FmeMv[nRefPicIdx][i].x);
					assert(Rime[i].getFmeMv(nRefPicIdx).y == g_FmeMv[nRefPicIdx][i].y);
					assert(Rime[i].getFmeMv(nRefPicIdx).nSadCost == g_FmeMv[nRefPicIdx][i].nSadCost);
				}
				//Rime[i].DestroyCuInfo();
			}
			for (int nRefPicIdx = 0; nRefPicIdx < nRefPicList1; nRefPicIdx++)
			{
				int idx = nMaxRefPic / 2 + nRefPicIdx;
				if (Rime[i].PrefetchCuInfo(idx))
				{
					Rime[i].RimeAndFme(idx);
					//RIME compare
					assert(Rime[i].getRimeMv(idx).x == g_RimeMv[idx][i].x);
					assert(Rime[i].getRimeMv(idx).y == g_RimeMv[idx][i].y);
					assert(Rime[i].getRimeMv(idx).nSadCost == g_RimeMv[idx][i].nSadCost);
					//FME compare
					assert(Rime[i].getFmeMv(idx).x == g_FmeMv[idx][i].x);
					assert(Rime[i].getFmeMv(idx).y == g_FmeMv[idx][i].y);
					assert(Rime[i].getFmeMv(idx).nSadCost == g_FmeMv[idx][i].nSadCost);
				}
			}
		}
		else
		{
			for (int nRefPicIdx = 0; nRefPicIdx < nRefPicList0; nRefPicIdx++)
			{
				if (Rime[i].PrefetchCuInfo(nRefPicIdx))
				{
					Rime[i].RimeAndFme(nRefPicIdx);
					//RIME compare
					assert(Rime[i].getRimeMv(nRefPicIdx).x == g_RimeMv[nRefPicIdx][i].x);
					assert(Rime[i].getRimeMv(nRefPicIdx).y == g_RimeMv[nRefPicIdx][i].y);
					assert(Rime[i].getRimeMv(nRefPicIdx).nSadCost == g_RimeMv[nRefPicIdx][i].nSadCost);
					//FME compare
					assert(Rime[i].getFmeMv(nRefPicIdx).x == g_FmeMv[nRefPicIdx][i].x);
					assert(Rime[i].getFmeMv(nRefPicIdx).y == g_FmeMv[nRefPicIdx][i].y);
					assert(Rime[i].getFmeMv(nRefPicIdx).nSadCost == g_FmeMv[nRefPicIdx][i].nSadCost);
				}
				//Rime[i].DestroyCuInfo();
			}
		}
	}

    /*
	stream_buf;
	recon_buf;
    while(enc_ctrl())
    {
        //preProcess();
        //preFetch();
        //IME();
        //CTU_CALC();
        //RC();
        //DBLK();
        //SAO_CALC();
        //SAO_FILTER();
        //CABAC();
    }
    */
}



void hardwareC::ConfigFiles(FILE *fp)
{
#define CFG_FOR_PREPROCESS  0
#define CFG_FOR_INTRA       1
#define CFG_FOR_IME         2
#define CFG_FOR_FME         3
#define CFG_FOR_QT          4
#define CFG_FOR_DBLK        5
#define CFG_FOR_SAO_CALC    6
#define CFG_FOR_SAO_FILTER  7
#define CFG_FOR_CTU_CALC    8
#define CFG_FOR_CABAC       9
#define CFG_FOR_RC          10
#define CFG_FOR_ENC_CTRL    11

#if MODULE_INIT_OPEN

    int   current_cfg = -1;
    char  preprocess_cfg[]  = "config for preprocess";
    char  intra_cfg[]       = "config for intra";
    char  ime_cfg[]         = "config for ime";
    char  fme_cfg[]         = "config for fme";
    char  qt_cfg[]          = "config for qt";
    char  dblk_cfg[]        = "config for dblk";
    char  sao_calc_cfg[]    = "config for sao_calc";
    char  sao_filter_cfg[]  = "config for sao_filter";
    char  ctu_ctrl_cfg[]    = "config for ctu_calc";
    char  cabac_cfg[]       = "config for cabac";
    char  rc_cfg[]          = "config for rc";
    char  enc_ctrl_cfg[]    = "config for enc_ctrl";

    char  cmdbuff[512];

    if(!fp)
      return;
    fseek(fp, 0, SEEK_SET);
    while(fgets(cmdbuff, 512, fp))
    {
        int i;

        for(i=0; i<512; i++)
        {
            if ((cmdbuff[i] == 0x0d)||(cmdbuff[i] == 0x0a)||(cmdbuff[i] == 0x00))
                break;
        }
        cmdbuff[i] = 0;

        if (0) ;
        OPT(cmdbuff, preprocess_cfg) {
            current_cfg = CFG_FOR_PREPROCESS;
            continue;
        }
        OPT(cmdbuff, intra_cfg) {
            current_cfg = CFG_FOR_INTRA;
            continue;
        }
        OPT(cmdbuff, ime_cfg) {
            current_cfg = CFG_FOR_IME;
            continue;
        }
        OPT(cmdbuff, fme_cfg) {
            current_cfg = CFG_FOR_FME;
            continue;
        }
        OPT(cmdbuff, qt_cfg) {
            current_cfg = CFG_FOR_QT;
            continue;
        }
        OPT(cmdbuff, dblk_cfg) {
            current_cfg = CFG_FOR_DBLK;
            continue;
        }
        OPT(cmdbuff, sao_calc_cfg) {
            current_cfg = CFG_FOR_SAO_CALC;
            continue;
        }
        OPT(cmdbuff, sao_filter_cfg) {
            current_cfg = CFG_FOR_SAO_FILTER;
            continue;
        }
        OPT(cmdbuff, ctu_ctrl_cfg) {
            current_cfg = CFG_FOR_CTU_CALC;
            continue;
        }
        OPT(cmdbuff, cabac_cfg) {
            current_cfg = CFG_FOR_CABAC;
            continue;
        }
        OPT(cmdbuff, rc_cfg) {
            current_cfg = CFG_FOR_RC;
            continue;
        }
        OPT(cmdbuff, enc_ctrl_cfg) {
            current_cfg = CFG_FOR_ENC_CTRL;
            continue;
        }
        else {
            current_cfg = current_cfg;
        }

        switch (current_cfg)
        {
            case CFG_FOR_PREPROCESS:
                break;
            case CFG_FOR_INTRA:
                //Rk_IntraPred.model_cfg(cmdbuff);
                break;
            case CFG_FOR_IME:
                break;
            case CFG_FOR_FME:
                break;
            case CFG_FOR_QT:
                break;
            case CFG_FOR_DBLK:
                break;
            case CFG_FOR_SAO_CALC:
                break;
            case CFG_FOR_SAO_FILTER:
                break;
            case CFG_FOR_CTU_CALC:
                ctu_calc.model_cfg(cmdbuff);
                break;
            case CFG_FOR_CABAC:
                break;
            case CFG_FOR_RC:
                break;
            case CFG_FOR_ENC_CTRL:
                break;
            default:
                //printf("%s\n",cmdbuff);
                break;
        }
    }
#endif
}