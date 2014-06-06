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

		if (( cmdbuff[0] == 59 ) || ( cmdbuff[0] == 0 )) // ";" means skip
		{
		    continue;
		}


        switch (current_cfg)
        {
            case CFG_FOR_PREPROCESS:
                break;
            case CFG_FOR_INTRA:										
				
				
				strcpy( namebuff, CFG_FILE);
				strcat( namebuff, cmdbuff);
			
				/*open output file */
				if ( num >= INTRA_4_FILE_NUM)
					ctu_calc.cu_level_calc[3].m_rkIntraPred->fp_intra_8x8[num - INTRA_4_FILE_NUM] = fopen(namebuff, "w");
				else if ( num < INTRA_4_FILE_NUM)
					ctu_calc.cu_level_calc[3].m_rkIntraPred->fp_intra_4x4[num] = fopen(namebuff, "w");

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
