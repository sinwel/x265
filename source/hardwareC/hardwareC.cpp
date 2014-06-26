#include <stdio.h>
#include "macro.h"
#include "hardwareC.h"
#include "rk_define.h"

hardwareC G_hardwareC;

hardwareC::hardwareC()
{
}

hardwareC::~hardwareC()
{
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
	int nRefPic = Cime.getRefPicNum();
	if (Cime.getSliceType() == b_slice)
	{
		for (int nRefPicIdx = 0; nRefPicIdx < nRefPic - 1; nRefPicIdx++)
		{
			Cime.CIME(Cime.getOrigPic(), Cime.getRefPicDS(nRefPicIdx), nRefPicIdx);
			assert(Cime.getCimeMv(nRefPicIdx).x == g_leftPMV[nRefPicIdx].x);
			assert(Cime.getCimeMv(nRefPicIdx).y == g_leftPMV[nRefPicIdx].y);
			assert(Cime.getCimeMv(nRefPicIdx).nSadCost == g_leftPMV[nRefPicIdx].nSadCost);
		}
		Cime.CIME(Cime.getOrigPic(), Cime.getRefPicDS(nRefPic - 1), nRefPic - 1);
		assert(Cime.getCimeMv(nRefPic - 1).x == g_leftPMV[nMaxRefPic - 1].x);
		assert(Cime.getCimeMv(nRefPic - 1).y == g_leftPMV[nMaxRefPic - 1].y);
		assert(Cime.getCimeMv(nRefPic - 1).nSadCost == g_leftPMV[nMaxRefPic - 1].nSadCost);
	} 
	else
	{
		for (int nRefPicIdx = 0; nRefPicIdx < nRefPic; nRefPicIdx++)
		{
			Cime.CIME(Cime.getOrigPic(), Cime.getRefPicDS(nRefPicIdx), nRefPicIdx);
			assert(Cime.getCimeMv(nRefPicIdx).x == g_leftPMV[nRefPicIdx].x);
			assert(Cime.getCimeMv(nRefPicIdx).y == g_leftPMV[nRefPicIdx].y);
			assert(Cime.getCimeMv(nRefPicIdx).nSadCost == g_leftPMV[nRefPicIdx].nSadCost);
		}
	}
	
	for (int i = 0; i < 85; i++)
	{
		nRefPic = Rime[i].getRefPicNum();
		if (b_slice == Rime[i].getSliceType())
		{
			for (int nRefPicIdx = 0; nRefPicIdx < nRefPic-1; nRefPicIdx++)
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
			if (Rime[i].PrefetchCuInfo(nMaxRefPic - 1))
			{
				Rime[i].RimeAndFme(nMaxRefPic - 1);
				//RIME compare
				assert(Rime[i].getRimeMv(nMaxRefPic - 1).x == g_RimeMv[nMaxRefPic - 1][i].x);
				assert(Rime[i].getRimeMv(nMaxRefPic - 1).y == g_RimeMv[nMaxRefPic - 1][i].y);
				assert(Rime[i].getRimeMv(nMaxRefPic - 1).nSadCost == g_RimeMv[nMaxRefPic - 1][i].nSadCost);
				//FME compare
				assert(Rime[i].getFmeMv(nMaxRefPic - 1).x == g_FmeMv[nMaxRefPic - 1][i].x);
				assert(Rime[i].getFmeMv(nMaxRefPic - 1).y == g_FmeMv[nMaxRefPic - 1][i].y);
				assert(Rime[i].getFmeMv(nMaxRefPic - 1).nSadCost == g_FmeMv[nMaxRefPic - 1][i].nSadCost);
			}
		}
		else
		{
			for (int nRefPicIdx = 0; nRefPicIdx < nRefPic; nRefPicIdx++)
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

    char  cmdbuff[1024];
	char  namebuff[512];
	int   num = 0;
    if(!fp)
      return;
    fseek(fp, 0, SEEK_SET);
    while(fgets(cmdbuff, 1024, fp))
    {
        int i;

        for(i=0; i<1024; i++)
        {
            if ((cmdbuff[i] == 0x0d)||(cmdbuff[i] == 0x0a)||(cmdbuff[i] == 0x00)) /* »»ÐÐ »Ø³µ ¿Õ×Ö·û */
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

		if (( cmdbuff[0] == 59 ) || ( cmdbuff[0] == 0 )) // ";" means skip
		{
		    continue;
		}

		int idx = 0;
        switch (current_cfg)
        {
            case CFG_FOR_PREPROCESS:
                break;
            case CFG_FOR_INTRA:										
				/* cut "space" and "tab" char */
				
				while (cmdbuff[idx])
				{	
					if ((cmdbuff[idx] == 0x20)||(cmdbuff[idx] == 0x9)) // 32 is space , 9 is tab
					{				
						cmdbuff[idx] = 0; 
						break;
					}
					idx++;
				}
				
				strcpy( namebuff, CFG_FILE);
				
				
				/*open output file */
				if ( num >= INTRA_4_FILE_NUM + INTRA_8_FILE_NUM)
				{
					strcat( namebuff, "/16x16/");
					strcat( namebuff, cmdbuff);
					ctu_calc.cu_level_calc[2].m_rkIntraPred->fp_intra_16x16[num - INTRA_4_FILE_NUM - INTRA_8_FILE_NUM] = fopen(namebuff, "w");
					RK_HEVC_PRINT("the %d file name is: %s \n",num - INTRA_4_FILE_NUM - INTRA_8_FILE_NUM,namebuff);
				}
				else if ( num >= INTRA_4_FILE_NUM)
				{
					strcat( namebuff, "/8x8/");
					strcat( namebuff, cmdbuff);
					ctu_calc.cu_level_calc[3].m_rkIntraPred->fp_intra_8x8[num - INTRA_4_FILE_NUM] = fopen(namebuff, "w");
					RK_HEVC_PRINT("the %d file name is: %s \n",num - INTRA_4_FILE_NUM, namebuff);
				}
				else if ( num < INTRA_4_FILE_NUM)
				{
					strcat( namebuff, "/4x4/");
					strcat( namebuff, cmdbuff);
					ctu_calc.cu_level_calc[3].m_rkIntraPred->fp_intra_4x4[num] = fopen(namebuff, "w");
					RK_HEVC_PRINT("the %d file name is: %s \n",num ,namebuff);
				}
				num++;

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
                //ctu_calc.model_cfg(cmdbuff);
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
