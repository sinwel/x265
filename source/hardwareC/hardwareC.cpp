#include <stdio.h>
#include "macro.h"
#include "hardwareC.h"

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

void hardwareC::proc()
{
    /*
	stream_buf;
	recon_buf;
    while(enc_ctrl())
    {
		//preFetch();
        //preProcess();
        //IME();
        //CTU_CALC();
        //DBLK();
        //SAO();
        //CABAC();
    }
    */
}



void hardwareC::ConfigFiles(FILE *fp)
{
#define CFG_FOR_INTRA       0
#define CFG_FOR_INTER       1
#define CFG_FOR_MC			2
#define CFG_FOR_IQIT        3
#define CFG_FOR_LOOPFILT    4
#define CFG_FOR_DEC_CTL     5
#define CFG_FOR_DEBUG       6
#if MODULE_INIT_OPEN

    int   current_cfg = -1;
    char  intra_cfg[] = "config for intra";
    char  inter_cfg[] = "config for inter";
    char  mc_cfg[]    = "config for mc";
    char  iqit_cfg[]  = "config for iqit";
    char  filter_cfg[]= "config for loopfilter";
    char  dec_cfg[]   = "config for dec control";
    char  debug_cfg[]  = "config for debug";
    char  cmdbuff[512];

    if(!fp)
      return;
    fseek(fp, 0, SEEK_SET);
    while(fgets(cmdbuff, 512, fp))
    {
        int     i;

        for(i=0; i<512; i++)
        {
            if ((cmdbuff[i] == 0x0d)||(cmdbuff[i] == 0x0a)||(cmdbuff[i] == 0x00))
                break;
        }
        cmdbuff[i] = 0;

        if (!strncmp(intra_cfg,cmdbuff,sizeof(intra_cfg)-1))
        {
            current_cfg = CFG_FOR_INTRA;
            continue;
        }
        else if(!strncmp(inter_cfg,cmdbuff,sizeof(inter_cfg)-1))
        {
            current_cfg = CFG_FOR_INTER;
            continue;
        }
        else if(!strncmp(iqit_cfg,cmdbuff,sizeof(iqit_cfg)-1))
        {
            current_cfg = CFG_FOR_IQIT;
            continue;
        }
        else if(!strncmp(filter_cfg,cmdbuff,sizeof(filter_cfg)-1))
        {
            current_cfg = CFG_FOR_LOOPFILT;
            continue;
        }
        else if(!strncmp(dec_cfg,cmdbuff,sizeof(dec_cfg)-1))
        {
            current_cfg = CFG_FOR_DEC_CTL;
            continue;
        }
        else if(!strncmp(debug_cfg,cmdbuff,sizeof(debug_cfg)-1))
        {
            current_cfg = CFG_FOR_DEBUG;
            continue;
        }
        else if(!strncmp(mc_cfg,cmdbuff,sizeof(mc_cfg)-1))
        {
            current_cfg = CFG_FOR_MC;
            continue;
        }

        switch (current_cfg)
        {
            case CFG_FOR_INTRA:
                intra_recon_g.model_cfg(cmdbuff);
                break;

            case CFG_FOR_INTER:
                //inter.model_cfg(cmdbuff);
                prefetch_cache_g.model_cfg(cmdbuff);
                break;

            case CFG_FOR_IQIT:
                iqit.model_cfg(cmdbuff);
                break;

            case CFG_FOR_LOOPFILT:
                loopfilterhw.model_cfg(cmdbuff);
                break;

            case CFG_FOR_DEC_CTL:
                hevc_dec.model_cfg(cmdbuff);
                break;

            case CFG_FOR_MC:
                pMc->model_cfg(cmdbuff);
                break;

            case CFG_FOR_DEBUG:
                model_cfg(cmdbuff);
                break;

            default:
                //printf("%s\n",cmdbuff);
                break;
        }
    }
#endif
}