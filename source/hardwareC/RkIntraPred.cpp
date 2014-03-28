


#include "RkIntraPred.h"
#include <cstring>

using namespace RK_HEVC;

#if defined(_MSC_VER)
	#define FILE_PATH "F:/HEVC/log_files/"
	#define PATH_NAME(name) (FILE_PATH ## name)
#elif  defined(__GNUC__) 
	#define FILE_PATH "/home2/zxy/log_files/"
	#define PATH_NAME(name) ("/home2/zxy/log_files/"name)
#endif

char rk_g_convertToBit[RK_MAX_CU_SIZE + 1];

void initRk_g_convertToBit()
{
        int 	i;
		char 	c;

        // g_convertToBit[ x ]: log2(x/4), if x=4 -> 0, x=8 -> 1, x=16 -> 2, ...
        memset(rk_g_convertToBit, -1, sizeof(rk_g_convertToBit));
        c = 0;
        for (i = 4; i < RK_MAX_CU_SIZE; i *= 2)
        {
            rk_g_convertToBit[i] = c;
            c++;
        }

        rk_g_convertToBit[i] = c;
    }

Rk_IntraPred::Rk_IntraPred()
{
#ifdef X265_LOG_FILE_ROCKCHIP
	rk_logIntraPred[0] = fopen(PATH_NAME("intrapred.txt"), "w+" );
	if ( rk_logIntraPred[0] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	rk_logIntraPred[1] = fopen(PATH_NAME("intrapred_1.txt"), "w+" );
	if ( rk_logIntraPred[1] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	rk_logIntraPred[2] = fopen(PATH_NAME("deltaFract.txt"), "w+" );
	if ( rk_logIntraPred[2] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	rk_logIntraPred[3] = fopen(PATH_NAME("dirMode.txt"), "w+" );
	if ( rk_logIntraPred[3] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	rk_logIntraPred[4] = fopen(PATH_NAME("intra_input.txt"), "w+" );
	if ( rk_logIntraPred[4] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	rk_logIntraPred[5] = fopen(PATH_NAME("bNeigbours.txt"), "w+" );
	if ( rk_logIntraPred[5] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	rk_logIntraPred[6] = fopen(PATH_NAME("pred_input.txt"), "w+" );
	if ( rk_logIntraPred[6] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	rk_logIntraPred[7] = fopen(PATH_NAME("pred_input_1.txt"), "w+" );
	if ( rk_logIntraPred[7] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	rk_logIntraPred[8] = fopen(PATH_NAME("intra_flow.txt"), "w+" );
	if ( rk_logIntraPred[8] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	
#endif
	rk_bFlag32 = 1;
	rk_bFlag16 = 1;
	rk_bFlag8	= 1;
	rk_bFlag4	= 1;
	::memset(rk_verdeltaFract,0,sizeof(rk_verdeltaFract));
	::memset(rk_hordeltaFract,0,sizeof(rk_hordeltaFract));
#ifdef CONNECT_QT
	rk_hevcqt = new hevcQT;
#endif

	initRk_g_convertToBit();
	
}

Rk_IntraPred::~Rk_IntraPred()
{
#ifdef X265_LOG_FILE_ROCKCHIP
	for (int i = 0 ; i < 8 ; i++ )
	{
		fclose(rk_logIntraPred[i]);
	}
#endif
#ifdef CONNECT_QT
	delete rk_hevcqt;
#endif

}

/* ��װ35��Ԥ�⺯��
** params@ in
**		refAbove 					- above �ο���
**		refLeft 					- left �ο���
**		stride						- ͨ����width��ȣ�����4x4��ʱ��stride���Ե���8[����PU�»�]
**		log2_size 					- left �ο���
**		c_idx 						- =0��ʾ luma����
**		dirMode 					- 35��Ԥ�ⷽ���е�һ��

** params@ out  
**		predSample 					- Ҫ����Ԥ������ָ��
*/
void Rk_IntraPred::RkIntraPredAll(RK_Pel *predSample, 
            	RK_Pel *refAbove, 
            	RK_Pel *refLeft,
            	int stride,            	
            	int log2_size,
            	int c_idx,
            	int dirMode)
{
	assert(dirMode < 35);
	if ( dirMode == 0 )
	{
	    RkIntraPred_PLANAR(predSample,
						refAbove, 
						refLeft, 
				 		stride, 
					 	log2_size);
	}
	else if ( dirMode == 1)
	{
	    RkIntraPred_DC(predSample,
						refAbove, 
						refLeft, 
				 		stride, 
					 	log2_size,
					 	c_idx);
	}
	else
	{
		RkIntraPred_angular(predSample, 
            	refAbove, 
            	refLeft,
            	stride,            	
            	log2_size,
            	c_idx,
            	dirMode);
	}
}


/* planar[predmode=0],from size 4x4 8x8 16x16 32x32
** params@ in
**		above 						- above �ο��� length: width + 1
**		left 						- left �ο��� length: heigth + 1 
**		stride						- ͨ����width��ȣ�����4x4��ʱ��stride���Ե���8[����PU�»�]
**		log2_size 					- left �ο���

** params@ out 
**		predSample 					- Ҫ����Ԥ������ָ��
*/
void Rk_IntraPred::RkIntraPred_PLANAR(RK_Pel *predSample, 
				RK_Pel 	*above, 
				RK_Pel 	*left, 
				int 	stride, 
				int 	log2_size)
{
    int x, y;
    int nTbS = (1 << log2_size);
	//** �ο�Э���У����Ͻǵĵ㲻��������,above �� left ��Ҫ��ַ+1
	above++;
	left++;
	for (y = 0; y < nTbS; y++) 
	{
        for (x = 0; x < nTbS; x++) 
		{
            predSample[y*stride + x] = (RK_Pel)(( (nTbS - 1 - x) * left[y]  	+ (x + 1) * above[nTbS] +
                         				 (nTbS - 1 - y) * above[x] 	+ (y + 1) * left[nTbS] + nTbS) >>
                        					(log2_size + 1));
        }
    }
}






/* ** DC[predmode=1],from size 4x4 8x8 16x16 32x32
** params@ in
**		above 						- above �ο��� length: width + 1 ǰ��Ҳ�� width - 1�Ŀռ�
**		left 						- left �ο��� length: heigth + 1 ǰ��Ҳ�� heigth - 1�Ŀռ�
**		stride						- ͨ����width��ȣ�����4x4��ʱ��stride���Ե���8[����PU�»�]
**		log2_size 					- left �ο���
**		c_idx 						- =0��ʾ luma����

** params@ out  
**		predSample 					- Ҫ����Ԥ������ָ��
*/
void Rk_IntraPred::RkIntraPred_DC(RK_Pel *predSample, 
			RK_Pel 	*above, 
			RK_Pel 	*left, 
			int 	stride, 
			int 	log2_size, 
			int 	c_idx)
{
    int i, j, x, y;
    int nTbS = (1 << log2_size);
	//** �ο�Э���У����Ͻǵĵ㲻��������,above �� left ��Ҫ��ַ+1
	above++;
	left++;
    int dc = nTbS;
    for (i = 0; i < nTbS; i++)
	{
        dc += left[i] + above[i]; 
	}
    dc >>= (log2_size + 1);

	// �ȶ���ֵΪDC�������޸�
	for (i = 0; i < nTbS; i++)
	{
    	for (j = 0; j < nTbS; j++)
		{
    		predSample[j*stride + i] = (RK_Pel) dc;	
		}
	}

	// luma������nTbS < 32
    if (c_idx == 0 && nTbS < 32) 
	{
        predSample[0] = (RK_Pel)((left[0] + 2 * dc  + above[0] + 2) >> 2);
		
        for (x = 1; x < nTbS; x++)
    	{
            predSample[x] = (RK_Pel)((above[x] + 3 * dc + 2) >> 2);
    	}
        for (y = 1; y < nTbS; y++)
    	{
            predSample[y*stride] = (RK_Pel)((left[y] + 3 * dc + 2) >> 2);
        }
    }

}

/* ** angular[predmode=1],from size 4x4 8x8 16x16 32x32
** params@ in
**		above 						- above �ο��� length: width + 1 ǰ��Ҳ�� width - 1�Ŀռ�
**		left 						- left �ο��� length: heigth + 1 ǰ��Ҳ�� heigth - 1�Ŀռ�
**		stride						- ͨ����width��ȣ�����4x4��ʱ��stride���Ե���8[����PU�»�]
**		log2_size 					- left �ο���
**		c_idx 						- =0��ʾ luma����
**		dirMode 					- 35��Ԥ�ⷽ���е�һ��

** params@ out  
**		predSample 					- Ҫ����Ԥ������ָ��
*/

void Rk_IntraPred::RkIntraPred_angular(RK_Pel *predSample, 
            	RK_Pel *refAbove, 
            	RK_Pel *refLeft,
            	int stride,            	
            	int log2_size,
            	int c_idx,
            	int dirMode)
{
    bool modeHor       	= (dirMode < 18);
    bool modeVer       	= !modeHor;
	bool isLuma		   	= (c_idx == 0);
	int width 			= 1 << log2_size;
    int intraPredAngle 	= modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int absAng         	= abs(intraPredAngle);
    int signAng        	= intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];

    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;
	
#if 0
    int  k, l;

	// modeVer, mode >= 18
	{
        pixel* refMain;
        pixel* refSide;
		int ext_valid_idx = width * intraPredAngle >> 5;

        refMain = (modeVer ? refAbove : refLeft); // + (width - 1);
        refSide = (modeVer ? refLeft : refAbove); // + (width - 1);
		
		// ��aboveΪ������left��[�±��1��ʼ���������Ͻǵĵ�]ѡȡһЩ׷�ӵ�refMain������
		if (intraPredAngle < 0)
        {
            for (k = -1; k > ext_valid_idx; k--)
            {                
                refMain[k] = refSide[(abs(invAngle*k) + 128) >> 8];
            }			
        }

        if (intraPredAngle == 0)
        {
        	// ��ֱ��ˮƽ����ֱ��һ�����ص����һ���к�һ����
        	// ע�� refMain�а������Ͻǵĵ㣬�����±�Ҫ�� predSample ���±� + 1
            for (k = 0; k < width; k++)
            {
                for (l = 0; l < width; l++)
                {
                    predSample[k * stride + l] = refMain[l + 1];
                }
            }
			// ֻ��width < 32����luma����Ԥ��Ƕ�Ϊ 0 ����90��(��������ת����ֻ��0��)
            if ((width < 32) && isLuma)
            {
                for (k = 0; k < width; k++)
                {
					predSample[k * stride] = (pixel)ClipY(refMain[1]  + ((refSide[k + 1] - refSide[0]) >> 1));
                }
            }
   
	    }
        else // intraPredAngle != 0[����ӳ���ϵ����]
        {
            int deltaPos = 0;
            int deltaInt;
            int deltaFract;
            int refMainIndex;

            for (k = 0; k < width; k++)
            {
                deltaPos += intraPredAngle;
                deltaInt   = deltaPos >> 5;
                deltaFract = deltaPos & (32 - 1);

                if (deltaFract)
                {
                    // Do linear filtering
                    for (l = 0; l < width; l++)
                    {
                        refMainIndex = l + deltaInt + 1;
                        predSample[k * stride + l] = (pixel)(((32 - deltaFract) * refMain[refMainIndex] + deltaFract * refMain[refMainIndex + 1] + 16) >> 5);
                    }
                }
                else
                {
                    // Just copy the integer samples
                    for (l = 0; l < width; l++)
                    {
                        predSample[k * stride + l] = refMain[l + deltaInt + 1];
                    }
                }
            }
        }

        // Flip the block if this is the horizontal mode
        if (modeHor)
        {
            for (k = 0; k < width - 1; k++)
            {
                for (l = k + 1; l < width; l++)
                {
                    pixel tmp              		= predSample[k * stride + l];
                    predSample[k * stride + l] 	= predSample[l * stride + k];
                    predSample[l * stride + k] 	= tmp;
                }
            }
        }
    }
#else
	if(modeVer)
	{
        RK_Pel* refMain;
        RK_Pel* refSide;
		int x,y;
		int ext_valid_idx = width * intraPredAngle >> 5;

        refMain = refAbove; // + (width - 1);
        refSide = refLeft; // + (width - 1);
		
		// ��aboveΪ������left��[�±��1��ʼ���������Ͻǵĵ�]ѡȡһЩ׷�ӵ�refMain������
		if (intraPredAngle < 0)
        {
            for ( x = -1; x > ext_valid_idx; x--)
            {                
                refMain[x] = refSide[(abs(invAngle*x) + 128) >> 8];
            }			
        }

        if (intraPredAngle != 0)
        {
            int deltaPos = 0;
            int deltaInt;
            int deltaFract;
            int refMainIndex;

            for (y = 0; y < width; y++)
            {
                deltaPos += intraPredAngle;
                deltaInt   = deltaPos >> 5;
                deltaFract = deltaPos & (32 - 1);
#ifdef X265_LOG_FILE_ROCKCHIP
				// ��ĳһ�Ƕȵ�һ��y,��ͬx = y*tan(�Ƕ�)
				rk_verdeltaFract[y] = deltaFract;
				rk_verdeltaInt[y] = deltaInt;
#endif
                if (deltaFract)
                {
                    // ˫���Բ�ֵ
                    for (x = 0; x < width; x++)
                    {
                        refMainIndex = x + deltaInt + 1;
                        predSample[y * stride + x] = (RK_Pel)((
							(32 - deltaFract) * refMain[refMainIndex] + 
								   deltaFract * refMain[refMainIndex + 1] 
								   + 16) >> 5);
                    }
                }
                else
                {
                    // ֱ�Ӹ�ֵ
                    for (x = 0; x < width; x++)
                    {
                        predSample[y * stride + x] = refMain[x + deltaInt + 1];
                    }
                }
            }
        }
		else // intraPredAngle == 0
		{
        	// ��ֱֱ��һ�����ص����һ����
        	// ע�� refMain�а������Ͻǵĵ㣬�����±�Ҫ�� predSample ���±� + 1
            for (y = 0; y < width; y++)
            {
                for (x = 0; x < width; x++)
                {
                    predSample[y * stride + x] = refMain[x + 1];
                }
            }
			// ֻ��width < 32����luma����Ԥ��Ƕ�Ϊ 0  
            if ((width < 32) && isLuma)
            {
                for (y = 0; y < width; y++)
                {
					predSample[y * stride] = (RK_Pel)RK_ClipY(refMain[1]  + ((refSide[y + 1] - refSide[0]) >> 1));
                }
            }
   
	    }

    }
	
	if(modeHor)
	{
        RK_Pel* refMain;
        RK_Pel* refSide;
		int x,y;
		int ext_valid_idx = width * intraPredAngle >> 5;

        refMain = refLeft; // + (width - 1);
        refSide = refAbove; // + (width - 1);
		
		// ��leftΪ������above��[�±��1��ʼ���������Ͻǵĵ�]ѡȡһЩ׷�ӵ�refMain������
		if (intraPredAngle < 0)
        {
            for (y = -1; y > ext_valid_idx; y--)
            {                
                refMain[y] = refSide[(abs(invAngle*y) + 128) >> 8];
            }			
        }


        if( intraPredAngle != 0)//[����ӳ���ϵ����]
        {
            int deltaPos = 0;
            int deltaInt;
            int deltaFract;
            int refMainIndex;

            for (x = 0; x < width; x++)
            {
                deltaPos += intraPredAngle;
                deltaInt   = deltaPos >> 5;
                deltaFract = deltaPos & (32 - 1);
#ifdef X265_LOG_FILE_ROCKCHIP
				// ��ĳһ�Ƕȵ�һ��x,��ͬy = x*ctan(�Ƕ�)
				rk_hordeltaFract[x] = deltaFract;
				rk_hordeltaInt[x] = deltaInt;
#endif
                if (deltaFract)
                {
                    // ˫���Բ�ֵ
                    for (y = 0; y < width; y++)
                    {
                        refMainIndex = y + deltaInt + 1;
                        predSample[y * stride + x] = (RK_Pel)(
							((32 - deltaFract) * refMain[refMainIndex] + 
							        deltaFract * refMain[refMainIndex + 1] 
							        + 16) >> 5);
                    }
                }
                else
                {
                    // ֱ�Ӹ�ֵ
                    for (y = 0; y < width; y++)
                    {
                        predSample[y * stride + x] = refMain[y + deltaInt + 1];
                    }
                }
            }
        }
		else // (intraPredAngle == 0)
        {
        	// ˮƽ����ֱ��һ�����ص����һ����
        	// ע�� refMain�а������Ͻǵĵ㣬�����±�Ҫ�� predSample ���±� + 1
            for (x = 0; x < width; x++)
            {
                for (y = 0; y < width; y++)
                {
                    predSample[y * stride + x] = refMain[y + 1];
                }
            }
			// ֻ��width < 32����luma����Ԥ��Ƕ�Ϊ 0 
            if ((width < 32) && isLuma)
            {
                for (x = 0; x < width; x++)
                {
					predSample[0 * stride + x] = (RK_Pel)RK_ClipY(refMain[1]  + ((refSide[x + 1] - refSide[0]) >> 1));
                }
            }
   
	    }
    }
#endif
    
}





void Rk_IntraPred::LogdeltaFractForAngluar()
{
	int i,j;


	if(rk_dirMode > 1)
	{
		RK_HEVC_FPRINT(rk_logIntraPred[2],"rk_dirMode = %d, size is %d x %d \n",rk_dirMode,rk_puwidth_35,rk_puwidth_35);

		if ( rk_dirMode >= 18)
		{
			for ( i = 0 ; i < rk_puwidth_35 ; i++ )
			{
				RK_HEVC_FPRINT(rk_logIntraPred[2],"[y:] fact = %02d \t",rk_verdeltaFract[i]);

				for ( j = 0 ; j < rk_puwidth_35  ; j++ )
				{
					RK_HEVC_FPRINT(rk_logIntraPred[2]," (%2d,%2d) ",rk_verdeltaInt[i] + j + 1,rk_verdeltaInt[i] + j + 2);
				}				
				RK_HEVC_FPRINT(rk_logIntraPred[2],"\n");
			}
			
		}
		else
		{
			for ( i = 0 ; i < rk_puwidth_35 ; i++ )
			{
				RK_HEVC_FPRINT(rk_logIntraPred[2],"[x:] fact = %02d \t",rk_hordeltaFract[i]);
				for ( j = 0 ; j < rk_puwidth_35  ; j++ )
				{
				//	RK_HEVC_FPRINT(rk_logIntraPred[2]," %02d x %02d + %02d x %02d ", 32 - rk_hordeltaFract[i],rk_hordeltaInt[i] + j + 1
				//		,rk_hordeltaFract[i], rk_hordeltaInt[i] + j + 2);
					RK_HEVC_FPRINT(rk_logIntraPred[2]," (%2d,%2d) ", rk_hordeltaInt[i] + j + 1, rk_hordeltaInt[i] + j + 2);
				}
				RK_HEVC_FPRINT(rk_logIntraPred[2],"\n");
			}
		}
	}	
	else
	{
		RK_HEVC_PRINT("logging over\n");
	}
}
void Rk_IntraPred::RkIntraPred_angularCheck()
{
	int i,j;
	// ֻ����Ч���ݽ��� compare
	//RK_HEVC_PRINT("dirMode = %d \r", rk_dirMode);
	if ( rk_dirMode >= 0)	
	{

		//RK_HEVC_FPRINT(rk_logIntraPred[0]," size is %d x %d \n",rk_puwidth_35,rk_puwidth_35);
		//RK_HEVC_FPRINT(rk_logIntraPred[1]," size is %d x %d \n",rk_puwidth_35,rk_puwidth_35);

		for ( i = 0 ; i < rk_puwidth_35 ; i++ )
		{
			for ( j = 0 ; j < rk_puwidth_35 ; j++ )
			{
				//RK_HEVC_FPRINT(rk_logIntraPred[0],"0x%02x \t",rk_IntraPred_35.rk_predSample[i]);
				//RK_HEVC_FPRINT(rk_logIntraPred[1],"0x%02x \t",rk_IntraPred_35.rk_predSampleOrg[i]);
				if(rk_IntraPred_35.rk_predSample[i*rk_puwidth_35 + j] != rk_IntraPred_35.rk_predSampleOrg[i*rk_puwidth_35 + j])
		        {
		        	RK_HEVC_PRINT("check falied %s \n",__FUNCTION__);
				}
			}
			//RK_HEVC_FPRINT(rk_logIntraPred[0]," \n");
			//RK_HEVC_FPRINT(rk_logIntraPred[1]," \n");
		}
		//RK_HEVC_FPRINT(rk_logIntraPred[0]," \n");
		//RK_HEVC_FPRINT(rk_logIntraPred[1]," \n");
	}
}



void Rk_IntraPred::RkIntraFillRefSamplesCheck(RK_Pel* above, RK_Pel* left, int32_t width, int32_t heigth)
{
	// ���ݿ�ǰ�洢
	int32_t i ;
   	for (  i = 0 ; i < 2*heigth + 1 ; i++ )
    {
        if(rk_LineBufTmp[i] != left[2*heigth - i])
		{
            RK_HEVC_PRINT("%s Error happen\n",__FUNCTION__);			
        }
    }
   	for ( i = 0 ; i < 2*width; i++ )
    {
        if(rk_LineBufTmp[i+2*heigth+1] != above[i+1])
		{
            RK_HEVC_PRINT("%s Error happen\n",__FUNCTION__);			
        }
    }

	// rk_LineBufTmp vs rk_LineBuf
   	for (  i = 0 ; i < 2*heigth + 2*width + 1 ; i++ )
    {
        if(rk_LineBufTmp[i] != rk_LineBuf[i])
		{
            RK_HEVC_PRINT("%s Error happen\n",__FUNCTION__);			
        }
    }

	

}

void Rk_IntraPred::RkIntraFillChromaCheck(RK_Pel* pLineBuf, RK_Pel* above, RK_Pel* left, uint32_t width, uint32_t heigth)
{
	uint32_t i ;
   	for (  i = 0 ; i < 2*heigth + 1 ; i++ )
    {
        if(pLineBuf[i] != left[2*heigth - i])
		{
            RK_HEVC_PRINT("[%s] Error happen\n",__FUNCTION__);			
        }
    }
   	for ( i = 0 ; i < 2*width; i++ )
    {
        if(pLineBuf[i+2*heigth+1] != above[i+1])
		{
            RK_HEVC_PRINT("[%s] Error happen\n",__FUNCTION__);	
        }
    }
}

void Rk_IntraPred::RkIntraSmoothingCheck()
{
	// ���ݿ�ǰ�洢
   	for ( int32_t idx = 0 ; idx < 2*rk_puHeight + 1 ; idx++ )
    {
        if((rk_IntraSmoothIn.rk_refAboveFiltered[X265_COMPENT][idx] != rk_IntraSmoothIn.rk_refAboveFiltered[RK_COMPENT][idx]) || 
        (rk_IntraSmoothIn.rk_refLeftFiltered[X265_COMPENT][idx] != rk_IntraSmoothIn.rk_refLeftFiltered[RK_COMPENT][idx]))
        {
            RK_HEVC_PRINT("%s Error happen\n",__FUNCTION__);			
        }
    }
}  


void Rk_IntraPred::fillReferenceSamples(RK_Pel* roiOrigin, 
							RK_Pel* adiTemp, 
							bool* bNeighborFlags, 
							int numIntraNeighbor, 
							int unitSize, 
							int numUnitsInCU, 
							int totalUnits, 
							uint32_t cuWidth, 
							uint32_t cuHeight, 
							uint32_t width, 
							uint32_t height, 
							int picStride,
							uint32_t trDepth)
{
    RK_Pel* piRoiTemp;
    int  i, j;
    RK_Pel  iDCValue = 1 << (RK_DEPTH - 1);

    if (numIntraNeighbor == 0)
    {
        // Fill border with DC value
        for (i = 0; i < (int)width; i++)
        {
            adiTemp[i] = iDCValue;
        }

        for (i = 1; i < (int)height; i++)
        {
            adiTemp[i * RK_ADI_BUF_STRIDE] = iDCValue;
        }
		
    }
    else if (numIntraNeighbor == totalUnits)
    {
        // Fill top-left border with rec. samples
        piRoiTemp = roiOrigin - picStride - 1;
        adiTemp[0] = piRoiTemp[0];

        // Fill left border with rec. samples
        // Fill below left border with rec. samples
        piRoiTemp = roiOrigin - 1;

        for (i = 0; i < 2 * (int)cuHeight; i++)
        {
            adiTemp[(1 + i) * RK_ADI_BUF_STRIDE] = piRoiTemp[0];
            piRoiTemp += picStride;
        }

        // Fill top border with rec. samples
        // Fill top right border with rec. samples
        piRoiTemp = roiOrigin - picStride;
        memcpy(&adiTemp[1], piRoiTemp, 2 * cuWidth * sizeof(*adiTemp));

    }
    else // reference samples are partially available
    {
        int  iNumUnits2 = numUnitsInCU << 1;
        int  iTotalSamples = totalUnits * unitSize;
        RK_Pel  piAdiLine[5 * RK_MAX_CU_SIZE];
        RK_Pel  *piAdiLineTemp;
        bool *pbNeighborFlags;
        int  iNext, iCurr;
        RK_Pel  piRef = 0;

        // Initialize
        for (i = 0; i < iTotalSamples; i++)
        {
            piAdiLine[i] = iDCValue;
        }

        // Fill top-left sample
        piRoiTemp = roiOrigin - picStride - 1;
        piAdiLineTemp = piAdiLine + (iNumUnits2 * unitSize);
        pbNeighborFlags = bNeighborFlags + iNumUnits2;
        if (*pbNeighborFlags)
        {
            piAdiLineTemp[0] = piRoiTemp[0];
            for (i = 1; i < unitSize; i++)
            {
                piAdiLineTemp[i] = piAdiLineTemp[0];
            }
        }

        // Fill left & below-left samples
        piRoiTemp += picStride;
        piAdiLineTemp--;
        pbNeighborFlags--;
        for (j = 0; j < iNumUnits2; j++)
        {
            if (*pbNeighborFlags)
            {
                for (i = 0; i < unitSize; i++)
                {
                    piAdiLineTemp[-i] = piRoiTemp[i * picStride];
                }
            }
            piRoiTemp += unitSize * picStride;
            piAdiLineTemp -= unitSize;
            pbNeighborFlags--;
        }

        // Fill above & above-right samples
        piRoiTemp = roiOrigin - picStride;
        piAdiLineTemp = piAdiLine + ((iNumUnits2 + 1) * unitSize);
        pbNeighborFlags = bNeighborFlags + iNumUnits2 + 1;
        for (j = 0; j < iNumUnits2; j++)
        {
            if (*pbNeighborFlags)
            {
                memcpy(piAdiLineTemp, piRoiTemp, unitSize * sizeof(*adiTemp));
            }
            piRoiTemp += unitSize;
            piAdiLineTemp += unitSize;
            pbNeighborFlags++;
        }

        // Pad reference samples when necessary
        iCurr = 0;
        iNext = 1;
        piAdiLineTemp = piAdiLine;
        while (iCurr < totalUnits)
        {
            if (!bNeighborFlags[iCurr])
            {
                if (iCurr == 0)
                {
                    while (iNext < totalUnits && !bNeighborFlags[iNext])
                    {
                        iNext++;
                    }

                    piRef = piAdiLine[iNext * unitSize];
                    // Pad unavailable samples with new value
                    while (iCurr < iNext)
                    {
                        for (i = 0; i < unitSize; i++)
                        {
                            piAdiLineTemp[i] = piRef;
                        }

                        piAdiLineTemp += unitSize;
                        iCurr++;
                    }
                }
                else
                {
                    piRef = piAdiLine[iCurr * unitSize - 1];
                    for (i = 0; i < unitSize; i++)
                    {
                        piAdiLineTemp[i] = piRef;
                    }

                    piAdiLineTemp += unitSize;
                    iCurr++;
                }
            }
            else
            {
                piAdiLineTemp += unitSize;
                iCurr++;
            }
        }

        // Copy processed samples
        piAdiLineTemp = piAdiLine + height + unitSize - 2;
        memcpy(adiTemp, piAdiLineTemp, width * sizeof(*adiTemp));

        piAdiLineTemp = piAdiLine + height - 1;
        for (i = 1; i < (int)height; i++)
        {
            adiTemp[i * RK_ADI_BUF_STRIDE] = piAdiLineTemp[-i];
        }
	
    }
}

void Rk_IntraPred::RkIntrafillRefSamples(RK_Pel* roiOrigin, 
							bool* bNeighborFlags, 
							int numIntraNeighbor, 
							int unitSize, 
							int numUnitsInCU, 
							int totalUnits, 
							int width,
							int height,
							int picStride,
							RK_Pel* pLineBuf)
{
    RK_Pel* piRoiTemp;
    int  i, j;
    RK_Pel  iDCValue = 1 << (RK_DEPTH - 1);
	
		
    if (numIntraNeighbor == 0)
    {
        // Fill border with DC value
        for ( i = 0 ; i < 2*height + 1; i++ )
        {
			pLineBuf[i] = iDCValue;
        }
		for ( i = 0 ; i < 2*width; i++ )
        {
			pLineBuf[i+2*height + 1] = iDCValue;
        }
    }
    else if (numIntraNeighbor == totalUnits)
    {
    	// roiOrigin ָ��pic�ĳ�ʼ���Ͻ�λ��
    	
    	// corner Ԫ�ؾ��� roiOrigin - stride -1    	
        // Fill top-left border with rec. samples
        piRoiTemp = roiOrigin - picStride - 1;
        pLineBuf[2 * height] = *piRoiTemp;

    	// left Ԫ�ؾ��� roiOrigin - 1
        // Fill left border with rec. samples
        // Fill below left border with rec. samples
        piRoiTemp = roiOrigin - 1;

        for (i = 0; i < 2 * height; i++)
        {
			pLineBuf[2*height - 1 - i] = *piRoiTemp;
            piRoiTemp += picStride;
        }

    	// above Ԫ�ؾ��� roiOrigin - stride
        // Fill top border with rec. samples
        // Fill top right border with rec. samples
        piRoiTemp = roiOrigin - picStride;
        memcpy(&pLineBuf[1 + 2 * height ], piRoiTemp, 2 * width * sizeof(pLineBuf[0]));
    }
    else // reference samples are partially available
    {

//        bool bNeighborFlags[4 * MAX_NUM_SPU_W + 1];
// #define MAX_NUM_SPU_W           (RK_MAX_CU_SIZE / MIN_PU_SIZE) // maximum number of SPU in horizontal line
// x265��С��PU�ǵ�4x4���̶�8�����ص�Ϊһ�����
// pbNeighborFlags ��Ҫ���� L ��buf ������
//    	int  unitSize = 8; 
//		int  numUnitsInCU = rk_puHeight / unitSize;
        int  iNumUnits2 = numUnitsInCU << 1;
        int  iTotalSamples = totalUnits * unitSize;
// �ο�x265����������corner�Ǹ������䵽 unitSize ����С
// ��������ʱ��ѭ���������Լ���8����������Ҫ��һ�жϣ����ӳ����Ӷ�
        RK_Pel  lineBufTmp[5 * RK_MAX_CU_SIZE];
        RK_Pel  *pLineTemp = lineBufTmp;
        bool *pbNeighborFlags;
        int  iNext, iCurr;
        RK_Pel  piRef = 0;

        // Initialize
        for (i = 0; i < iTotalSamples; i++)
        {
            lineBufTmp[i] = iDCValue;
        }

		// ��corner��䵽line buf���м�,�±�Ϊ 0����ռ unitSize ���ռ��С
        // Fill top-left sample
        piRoiTemp = roiOrigin - picStride - 1;
        pbNeighborFlags = bNeighborFlags + iNumUnits2;
        if (*pbNeighborFlags)
        {
            for (i = 0; i < unitSize; i++)
            {        
        		lineBufTmp[2 * height + i] = *piRoiTemp;
            }
        }
		// ���line buf���ϰ�Σ���roiTemp�е� " ��" ����Ԫ�ؿ�ʼ �������
		// ʵ�� L buf������
        // Fill left & below-left samples
        piRoiTemp += picStride;
		// ָ��洢left�ĵ�ַ
        pLineTemp = &lineBufTmp[2 * height - 1];
        pbNeighborFlags--;
		
        for (j = 0; j < iNumUnits2; j++)
        {
            if (*pbNeighborFlags)
            {
                for (i = 0; i < unitSize; i++)
                {
        			pLineTemp[-i] = piRoiTemp[i * picStride];
                }
            }
            piRoiTemp += unitSize * picStride;
            pLineTemp -= unitSize;
            pbNeighborFlags--;
        }

		// ���line buf���°�Σ���roiTemp�е� above ����Ϊ "->" 
        // Fill above & above-right samples
        piRoiTemp = roiOrigin - picStride;
        pLineTemp = &lineBufTmp[2 * height + unitSize];
		
        pbNeighborFlags = bNeighborFlags + iNumUnits2 + 1;
        for (j = 0; j < iNumUnits2; j++)
        {
            if (*pbNeighborFlags)
            {
                memcpy(pLineTemp, piRoiTemp, unitSize * sizeof(lineBufTmp[0]));
            }
            piRoiTemp += unitSize;
            pLineTemp += unitSize;
            pbNeighborFlags++;
        }

        // Pad reference samples when necessary
        iCurr = 0;
        iNext = 1;
		// rk_LineBuf �Ѿ������úõ� L buf
        pLineTemp = lineBufTmp;
        while (iCurr < totalUnits)
        {
            if (!bNeighborFlags[iCurr])
            {
            	// �Ƿ��� ���½ǵ��Ǹ��㣬line buf�еĵ�һ����
                if (iCurr == 0)
                {
                    while (iNext < totalUnits && !bNeighborFlags[iNext])
                    {
                        iNext++;
                    }

                    piRef = pLineTemp[iNext * unitSize];
					// ������½ǵĵ�
                    // Pad unavailable samples with new value
                    //while (iCurr < iNext)
                    {
	                    for (i = 0; i < unitSize; i++)
	                    {
	                        pLineTemp[i] = piRef;
	                    }

		                pLineTemp += unitSize;
		                iCurr++;
                    }
                }
                else // ����������½ǵ��Ǹ��㣬�� L buf ˳���������
                {
                    piRef = lineBufTmp[iCurr * unitSize - 1];
                    for (i = 0; i < unitSize; i++)
                    {
                        pLineTemp[i] = piRef;
                    }

                    pLineTemp += unitSize;
                    iCurr++;
                }
            }
            else
            {
                pLineTemp += unitSize;
                iCurr++;
            }
        }

        // Copy processed samples
        // �����°�����ݣ�1 + 2*CU_width��ע����� corner�㣬
        // ��������cornerռ�� unitSize ���ռ�,ֻ��Ҫѡһ��
        memcpy(pLineBuf,lineBufTmp, (2*height + 1)*sizeof(lineBufTmp[0]));
        memcpy(pLineBuf+2*height + 1,&lineBufTmp[2 * height + unitSize], (2*width)*sizeof(lineBufTmp[0]));
    }
}

void Rk_IntraPred::RkIntraSmoothing(RK_Pel* refLeft,
									RK_Pel* refAbove, 
									RK_Pel* refLeftFlt, 
									RK_Pel* refAboveFlt)
{

    // generate filtered intra prediction samples
    // left and left above border + above and above right border + top left corner = length of 3. filter buffer
	int32_t cuHeight2 = rk_puHeight * 2;
	int32_t cuWidth2  = rk_puWidth * 2;
	int32_t bufSize = cuHeight2 + cuWidth2 + 1;

	// L�� buffer���� ���µ����ϣ�Ȼ���ϵ�����
	
    RK_Pel refBuf[129];  // non-filtered
    RK_Pel filteredBuf[129]; // filtered
#if 0    
	for ( int32_t idx = 0 ; idx < 2*rk_puHeight + 1 ; idx++ )
	{
	    refBuf[idx] = rk_IntraSmoothIn.rk_refLeft[2*rk_puHeight - idx];
		
	}
	for ( int32_t idx = 0 ; idx < 2*rk_puHeight ; idx++ )
	{
	    refBuf[idx+2*rk_puHeight+1] = rk_IntraSmoothIn.rk_refAbove[idx+1];
	}
#else
	// use RkIntraFillRefSamples Out
	for ( int32_t idx = 0 ; idx < cuHeight2 + cuWidth2 + 1 ; idx++ )
	{
	    refBuf[idx] = rk_LineBuf[idx];		
	}

#endif
    //int l = 0;
  
    if (rk_bUseStrongIntraSmoothing)
    {
        int blkSize = 32;
        int bottomLeft = refBuf[0];
        int topLeft = refBuf[cuHeight2];
        int topRight = refBuf[bufSize - 1];
        int threshold = 1 << (RK_DEPTH - 5);
        bool bilinearLeft = abs(bottomLeft + topLeft - 2 * refBuf[rk_puHeight]) < threshold;
        bool bilinearAbove  = abs(topLeft + topRight - 2 * refBuf[cuHeight2 + rk_puHeight]) < threshold;

        if (rk_puWidth >= blkSize && (bilinearLeft && bilinearAbove))
        {
            int shift = rk_g_convertToBit[rk_puWidth] + 3; // log2(uiCuHeight2)
            filteredBuf[0] = refBuf[0];
            filteredBuf[cuHeight2] = refBuf[cuHeight2];
            filteredBuf[bufSize - 1] = refBuf[bufSize - 1];
			// ˫���Բ�ֵ
            for (int i = 1; i < cuHeight2; i++)
            {
                filteredBuf[i] = (RK_Pel)(((cuHeight2 - i) * bottomLeft + i * topLeft + rk_puHeight) >> shift);
            }

            for (int i = 1; i < cuWidth2; i++)
            {
                filteredBuf[cuHeight2 + i] = (RK_Pel)(((cuWidth2 - i) * topLeft + i * topRight + rk_puWidth) >> shift);
            }
        }
        else
        {
            // 1. filtering with [1 2 1]
            filteredBuf[0] = refBuf[0];
            filteredBuf[bufSize - 1] = refBuf[bufSize - 1];
            for (int i = 1; i < bufSize - 1; i++)
            {
                filteredBuf[i] = (refBuf[i - 1] + 2 * refBuf[i] + refBuf[i + 1] + 2) >> 2;
            }
        }
    }
    else
    {
        // 1. filtering with [1 2 1]
        filteredBuf[0] = refBuf[0];
        filteredBuf[bufSize - 1] = refBuf[bufSize - 1];
        for (int i = 1; i < bufSize - 1; i++)
        {
            filteredBuf[i] = (refBuf[i - 1] + 2 * refBuf[i] + refBuf[i + 1] + 2) >> 2;
        }
    }

	// �����35�з�����Ԥ�⣬Ҫ��L�� buf �ֿ�Ϊ above �� left
	// ע�� above �� left ��ǰ��Ҫ Ԥ�� width - 1���������ÿ���
	
	// " ��"
	for ( int idx = 0 ; idx < 2*rk_puHeight + 1 ; idx++ )
	{
	     *refLeft++ = refBuf[2*rk_puHeight - idx];
		
	}
	// " -> "
	for ( int idx = 0 ; idx < 2*rk_puWidth + 1 ; idx++ )
	{
	     *refAbove++ = refBuf[2*rk_puHeight+idx];
	}

	// " ��"
	for ( int idx = 0 ; idx < 2*rk_puHeight + 1 ; idx++ )
	{
	     *refLeftFlt++ = filteredBuf[2*rk_puHeight - idx];
		
	}
	// " -> "
	for ( int idx = 0 ; idx < 2*rk_puWidth + 1 ; idx++ )
	{
	     *refAboveFlt++ = filteredBuf[2*rk_puHeight+idx];
	}



	// �洢���Ͻǵĵ㣬Ȼ�����½�
	for ( int idx = 0 ; idx < 2*rk_puHeight + 1 ; idx++ )
	{
	     rk_IntraSmoothIn.rk_refLeftFiltered[RK_COMPENT][idx] = filteredBuf[2*rk_puHeight - idx];
		
	}
	// �洢���Ͻǵĵ㣬Ȼ�����Ͻ�
	for ( int idx = 0 ; idx < 2*rk_puWidth + 1 ; idx++ )
	{
	     rk_IntraSmoothIn.rk_refAboveFiltered[RK_COMPENT][idx] = filteredBuf[2*rk_puHeight+idx];
	}
}





#ifdef RK_INTRA_MODE_CHOOSE
int CacluVariance_45(RK_Pel* block, int width)
{
	uint16_t i,j,dir;
	uint16_t sumdir[16];
	uint16_t countdir[16];
	uint16_t elementdir[16][16];
	uint16_t direct_num;
	
	// 45�� 
	::memset(sumdir,0,sizeof(sumdir));
	::memset(countdir,0,sizeof(countdir));
	::memset(elementdir,0,sizeof(elementdir));
	
	direct_num = 2*width - 1;
	for ( dir = 0 ; dir < direct_num; dir++ )
	{
		int k = 0;
	    for ( i = 0 ; i < width ; i++ )
		{
			for ( j = 0 ; j < width ; j++ )
			{
				// 45��ǵĹ�ϵ
				if ( (i + j) == dir )
				{
    			    sumdir[dir] += block[i * width + j];
					elementdir[dir][k] = block[i * width + j];
					k++;
					countdir[dir] = k;
				}
			}
		}
	}
	
	uint16_t count;
	uint16_t absDiffSumTotal = 0;
	// ��sum(|mean-a| + |mean-b| + ...)
	for ( dir = 0 ; dir < direct_num; dir++ )
	{
		uint16_t avg = sumdir[dir]/countdir[dir];
		uint16_t absDiffSumDirect = 0;
		// һ����������Ԫ���ۼ�
		for ( count = 0 ; count < countdir[dir] ; count++ )
		{
			absDiffSumDirect += abs(avg - elementdir[dir][count] );
		}
		// ���з����ۼ�
		absDiffSumTotal += absDiffSumDirect;
	}

	return 	absDiffSumTotal;
}
int CacluVariance_180(RK_Pel* block, int width)
{
	uint16_t sumdir[16];
	uint16_t direct_num;
	
	// 180��һ���ۼ�
	direct_num = width;
	// ��������������ȣ���һά��Ӧ pic��Y���򣬶�ά����PIC��X����
	int y,x;	
	int absDiffSumDirect = 0;
	::memset(sumdir,0,sizeof(sumdir));
	for ( x = 0 ; x < width ; x++ )
	{
		for ( y = 0 ; y < width ; y++ )
		{
		    sumdir[x] += block[x * width + y];
		}
		
		uint16_t avg = sumdir[x]/width;
		
		for ( y = 0 ; y < width ; y++ )
		{
		    absDiffSumDirect += abs(avg - block[x * width + y]);
		}
	}

	return absDiffSumDirect;
}
void symmetryForYaxis(RK_Pel* block, int width)
{
	uint16_t x,y,tmp;
	for ( y = 0 ; y < width/2 ; y++ )
	{
		for ( x = 0 ; x < width ; x++ )
		{
		    tmp 								= block[x * width + y];
			block[x * width + y] 				= block[x * width + width - 1 - y];
			block[x * width + width - 1 - y] 	= tmp;
		}
	}

}
void transpose(RK_Pel* src, int blockSize)
{
	uint16_t tmp;
    for (uint16_t k = 0; k < blockSize; k++)
    {
        for (uint16_t l = k+1; l < blockSize; l++)
        {
        	tmp 					= src[k * blockSize + l];
            src[k * blockSize + l] 	= src[l * blockSize + k];
			src[l * blockSize + k]	= tmp;
        }
    }
}
int CacluVariance_135(RK_Pel* block, int width)
{
	// 135���� 45����� Y ��Գ�

	symmetryForYaxis(block,width);
	int ret = CacluVariance_45(block,width);
	return ret;

}

int CacluVariance_90(RK_Pel* block, int width)
{
	// 180���� 90��ת�ù�ϵ
	transpose(block, width);
	int ret = CacluVariance_180(block,width);
	return ret;

}

 
void Rk_IntraPred::RkIntraPriorModeChoose(int rdModeCandidate[], uint32_t rdModeCost[], bool bsort )
{
	if ( !bsort )
	{
	    // ���� rdModeCost[35] ѡ�� 2 10 18 26 34 ��������
		int dirMode[5] = {2,10,18,26,34};
		uint32_t cost[5];
		int i, index[5],Mode[5];
		for ( i = 0 ; i < 5 ; i++ )
		{	
			index[i] = i;
			cost[i] = rdModeCost[dirMode[i]];
		}
		BubbleSort(index, cost, 5);
		for ( i = 0 ; i < 5 ; i++ )
		{
		    Mode[i] = dirMode[index[i]];
		}
		rdModeCandidate[0] = 0;
		rdModeCandidate[1] = 1;
		// ������źʹ������ڣ�ȡ�����м�ķ��������һ����ĸ�4��
		if ( abs(index[0] - index[1]) == 1)
		{
			int min_idx = std::min<int>(Mode[0], Mode[1]) - 4;
			int max_idx = std::max<int>(Mode[0], Mode[1]) + 4;
			// �޶������� 2~34֮��		
			min_idx = min_idx < 2 ? 2 : min_idx;
			max_idx = max_idx > 34 ? 34 : max_idx;
			
			// 16�ַ���
		    for ( i = min_idx ; i < max_idx ; i++ )
		    {
		        rdModeCandidate[2 + (i-min_idx)] = i;
		    }
		}
		else
		// ������źʹ������ڣ�ȡ�����Լ����ҷ����4��
		{
			int k = 2;
			int min_idx = Mode[0] - 4;
			int max_idx = Mode[0] + 4;
			// �޶������� 2~34֮��		
			min_idx = min_idx < 2 ? 2 : min_idx;
			max_idx = max_idx > 34 ? 34 : max_idx;
			// 16�ַ���
		    for ( i = min_idx ; i < max_idx ; i++ )
		    {
		        rdModeCandidate[k++] = i;
		    }
			min_idx = Mode[1] - 4;
			max_idx = Mode[1] + 3;
			// �޶������� 2~34֮��		
			min_idx = min_idx < 2 ? 2 : min_idx;
			max_idx = max_idx > 34 ? 34 : max_idx;
			// 16�ַ���
		    for ( i = min_idx ; i < max_idx ; i++ )
		    {
		        rdModeCandidate[k++] = i;
		    }

		}
	}
	else
	// ���� rdModeCost ��Сѡ������
	{
		int i,index[18];
		uint32_t cost[18];
		int valid_len = 0;
		// ͳ�� rdModeCandidate ������-1�ĸ���
		for ( i = 0 ; i < 18 ; i++ )
		{
			if(rdModeCandidate[i] != -1)
				valid_len++;
		}
		for ( i = 0 ; i < valid_len ; i++ )
		{	
			index[i] = i;
			cost[i] = rdModeCost[rdModeCandidate[i]];
		}
		BubbleSort(index, cost, valid_len);
		rdModeCandidate[0] = rdModeCandidate[index[0]];
	}
}
#endif

int BubbleSort(int Index[], uint32_t Value[], int len)
{
    int i, j;
    int flag;
    int temp;

    if (len <= 0)
    {
        return 0;
    }

    // bubble sort increase
	for(i = 0; i < len - 1; i++)
    {
        flag = 1;
        for(j = 0; j < len - i - 1; j++)
        {
            if (Value[Index[j]] > Value[Index[j+1]])
			{
                temp = Index[j];
                Index[j] = Index[j + 1];
                Index[j + 1] = temp;

                flag = 0;
            }
        }

        if(1 == flag)
        {   
            break;
        }
    }
	return 1;
}

int BubbleSort(uint32_t Index[], uint64_t Value[], uint32_t len)
{
    uint32_t i, j;
    uint32_t flag;
    uint32_t temp;

    if (len <= 0)
    {
        return 0;
    }

    // bubble sort low -> high
	for(i = 0; i < len - 1; i++)
    {
        flag = 1;
        for(j = 0; j < len - i - 1; j++)
        {
            if (Value[Index[j]] > Value[Index[j+1]])
			{
                temp = Index[j];
                Index[j] = Index[j + 1];
                Index[j + 1] = temp;

                flag = 0;
            }
        }

        if(1 == flag)
        {   
            break;
        }
    }
	return 1;
} 


// ------------------------------------------------------------------------------
/*
**  
**
*/
// ------------------------------------------------------------------------------

void RK_CheckLineBuf(RK_Pel* LineBuf1, RK_Pel* LineBuf2, uint32_t size)
{
	uint32_t i;
	for ( i = 0 ; i < size ; i++ )
	{
	    if(LineBuf1[i] != LineBuf2[i])
    	{
    		RK_HEVC_PRINT("%s: check failed\n",__FUNCTION__);
    	}
		
	}
}
void RK_CheckSmoothing(RK_Pel* refLeft1, RK_Pel* refAbove1, RK_Pel* refLeftFlt1, RK_Pel* refAboveFlt1,
								RK_Pel* refLeft2, RK_Pel* refAbove2, RK_Pel* refLeftFlt2, RK_Pel* refAboveFlt2,
								uint32_t width, uint32_t height)
{
	uint32_t i;
	for ( i = 0 ; i < 2*width + 1 ; i++ )
	{
	    if(refAbove1[i] != refAbove2[i])
    	{
    		RK_HEVC_PRINT("%s: check failed\n",__FUNCTION__);
    	}
		if(refAboveFlt1[i] != refAboveFlt2[i])
    	{
    		RK_HEVC_PRINT("%s: check failed\n",__FUNCTION__);
    	}		
	}
	for ( i = 0 ; i < 2*height + 1; i++ )
	{
	    if(refLeft1[i] != refLeft2[i])
    	{
    		RK_HEVC_PRINT("%s: check failed\n",__FUNCTION__);
    	}
		if(refLeftFlt1[i] != refLeftFlt2[i])
    	{
    		RK_HEVC_PRINT("%s: check failed\n",__FUNCTION__);
    	}		
	}
}
void RK_CheckPredSamples(RK_Pel* pred1, RK_Pel* pred2, uint32_t width, uint32_t height)
{
	uint32_t i,j;

	for ( i = 0 ; i < height ; i++ )
	{
		for ( j = 0 ; j < width; j++ )
		{
		    if(pred1[i * width + j] != pred2[i * width + j])
	    	{
	    		RK_HEVC_PRINT("%s: check failed\n",__FUNCTION__);
	    	}
		}
	}
	
}

void RK_CheckSad(uint64_t* cost1, uint64_t* cost2)
{
	uint32_t i;

	for ( i = 0 ; i < 35 ; i++ )
	{
	    if(cost1[i] != cost2[i])
		{
			RK_HEVC_PRINT("%s: check failed\n",__FUNCTION__);
		}
	}
}

void RK_CheckResidual(int16_t* resi1, int16_t* resi2, uint32_t stride, uint32_t width, uint32_t height)
{
	uint32_t i,j;
	/* ����stride����width����ʹ��width=4��������ͷֿ�4�αȽ� */
	for ( i = 0 ; i < height ; i++ )
	{
		for ( j = 0 ; j < width; j++ )
		{
		    if(resi1[j] != resi2[j])
	    	{
	    		RK_HEVC_PRINT("%s: check failed\n",__FUNCTION__);
	    	}
		}
		resi1 += stride;
		resi2 += stride;		
	}
}

void RK_CheckRefChroma(RK_Pel* refLeft1, RK_Pel* refAbove1,RK_Pel* refLeft2,RK_Pel* refAbove2, uint32_t size)
{
	uint32_t i;
	/* ����stride����width����ʹ��width=4��������ͷֿ�4�αȽ� */
	for ( i = 0 ; i < size ; i++ )
	{
	    if(refLeft1[i] != refLeft2[i])
    	{
    		RK_HEVC_PRINT("%s: check failed\n",__FUNCTION__);
    	}
		if(refAbove1[i] != refAbove2[i])
    	{
    		RK_HEVC_PRINT("%s: check failed\n",__FUNCTION__);
    	}	
	}
}

void Rk_IntraPred::RkIntra_proc(INTERFACE_INTRA* pInterface_Intra, uint32_t partOffset)
{
	RK_Pel* 	fenc 	= pInterface_Intra->fenc;
	int32_t width 	= pInterface_Intra->size;
	int32_t height	= pInterface_Intra->size;
	RK_Pel LineBuf[129];
	// unitSize ֻ��4x4��ʱ���� 4������case����8
	// x265�й̶�Ϊ 4 
    int unitSize      = pInterface_Intra->cidx == 0 ? 4 : 2;
    int numUnitsInCU  = width / unitSize;
    int totalUnits    = (numUnitsInCU << 2) + 1;

	
// ----------------- luma ------------------------//
	if ( pInterface_Intra->cidx == 0)
	{
		/* step 1 */
		// fill
		//RK_Pel LineBuf[129];
		RK_IntraFillReferenceSamples( 
								pInterface_Intra->reconEdgePixel,
								pInterface_Intra->bNeighborFlags, 
								LineBuf,// ���Ժ� rk_LineBuf �Ƚ�
								pInterface_Intra->numintraNeighbor, 
								unitSize, 
								totalUnits, 
								width,
								height);

		RK_CheckLineBuf(LineBuf, rk_LineBuf, 2*width + 2*height + 1);

		// step 2 //
		
		// smoothing
		RK_Pel refLeft[65 + 32 - 1];
		RK_Pel refAbove[65 + 32 - 1];
		RK_Pel refLeftFlt[65 + 32 - 1];
		RK_Pel refAboveFlt[65 + 32 - 1];
		bool bUseStrongIntraSmoothing = rk_bUseStrongIntraSmoothing;
		RK_IntraSmoothing(LineBuf, 
						width, 
						height, 
						bUseStrongIntraSmoothing,
						refLeft + width - 1,
						refAbove + width - 1,
						refLeftFlt + width - 1,
						refAboveFlt + width - 1);
		// width - 1 ��Ϊ������pred��չ�õ�
		RK_CheckSmoothing(refLeft + width - 1, refAbove + width - 1, refLeftFlt + width - 1, refAboveFlt + width - 1, 
			rk_IntraSmoothIn.rk_refLeft, rk_IntraSmoothIn.rk_refAbove, 
			rk_IntraSmoothIn.rk_refLeftFiltered[X265_COMPENT], rk_IntraSmoothIn.rk_refAboveFiltered[X265_COMPENT],
			width, height);

		// step 3 //
		
		// predition
		//RK_Pel* predSample = (RK_Pel*)X265_MALLOC(RK_Pel, width*height);
		RK_Pel predSample[32*32];


		uint8_t RK_intraFilterThreshold[5] =
		{
		    10, //4x4
		    7,  //8x8
		    1,  //16x16
		    0,  //32x32
		    10, //64x64
		};
		int log2BlkSize = rk_g_convertToBit[width] + 2;
		// X265��� puStride ��С������ width,ֻ��4x4ʱ��NxN, puStride = 8 //
		// ��RK_ENCODER�У����ݶ��ǽ��մ洢��û��stride = 8�����
		int puStride = width;
		int dirMode;
		int costSad[35];
		uint64_t costTotal[35];
		for ( dirMode = 0 ; dirMode < 35 ; dirMode++ )
		{
	    	int 	diff		= std::min<int>(abs((int)dirMode - HOR_IDX), abs((int)dirMode - VER_IDX));
			uint8_t filterIdx 	= diff > RK_intraFilterThreshold[log2BlkSize - 2] ? 1 : 0;
			
			RK_Pel* refAboveDecide = refAbove + width - 1;
			RK_Pel* refLeftDecide = refLeft + width - 1;

			if (dirMode == DC_IDX)
			{
				filterIdx = 0; //no smoothing for DC or LM chroma
			}
			assert(filterIdx <= 1);
			if ( filterIdx )
			{
				refAboveDecide = refAboveFlt + width - 1;
				refLeftDecide = refLeftFlt + width - 1;
			}
			
			RkIntraPredAll(predSample, 
							refAboveDecide, 
							refLeftDecide,
							puStride, 
							log2BlkSize,
							pInterface_Intra->cidx, // 0 is luma NOW only support luma
							dirMode);
#if 0
			for ( int i = 0 ; i <  2*width + 1 ; i++ )
			{
	    		RK_HEVC_FPRINT(rk_logIntraPred[6],"%d ",*refLeftDecide++);
			}
			RK_HEVC_FPRINT(rk_logIntraPred[6],"\n");
			for ( int i = 0 ; i <  2*width + 1 ; i++ )
			{
	    		RK_HEVC_FPRINT(rk_logIntraPred[6],"%d ",*refAboveDecide++);
			}
			RK_HEVC_FPRINT(rk_logIntraPred[6],"\n");
#endif
					
			RK_CheckPredSamples(
				rk_IntraPred_35.rk_predSampleTmp[dirMode], 
				predSample, width, height);
			


			// step 4 //
			// caclu SAD
			costSad[dirMode] = RK_IntraSad(fenc, 
					puStride, 
					predSample, 
					puStride, 
					width);

			costTotal[dirMode] =  costSad[dirMode] + ((rk_bits[dirMode] * rk_lambdaMotionSAD + 32768) >> 16); 

		}

		RK_CheckSad(costTotal, rk_modeCostsSadAndCabac);


		// step 5 //
		// get minnum costSad + lambad*bits,decide the dirMode
		uint32_t index[35];
		for (int i = 0 ; i < 35 ; i++ )
		{
		    index[i] = i;
		}
		BubbleSort(index,costTotal, 35);

		int bestMode = index[0];
		// TEST_LUMA_DEPTH = 0,rk_bestModeֻ��1������
		// TEST_LUMA_DEPTH = 0,rk_bestModeֻ��4������
		assert(bestMode == rk_bestMode[X265_COMPENT][partOffset]);

		// step 6 //
		int 	diff		= std::min<int>(abs((int)bestMode - HOR_IDX), abs((int)bestMode - VER_IDX));
		uint8_t filterIdx 	= diff > RK_intraFilterThreshold[log2BlkSize - 2] ? 1 : 0;
		RK_Pel* refAboveDecide = refAbove + width - 1;
		RK_Pel* refLeftDecide 	= refLeft + width - 1; 
		if (bestMode == DC_IDX)
		{
			filterIdx = 0; //no smoothing for DC or LM chroma
		}
		assert(filterIdx <= 1);
		if ( filterIdx )
		{
			refAboveDecide = refAboveFlt + width - 1;
			refLeftDecide = refLeftFlt + width - 1;
		}
		// calcu the bestMode pred
		RkIntraPredAll(rk_pred[RK_COMPENT], 
					refAboveDecide, 
					refLeftDecide,
					puStride, 
					log2BlkSize,
					0, // 0 is luma NOW only support luma
					bestMode);

		// step 7 //
		// caclu resi
		//int16_t *residual = (int16_t*)X265_MALLOC(int16_t, width*height);
		//int16_t residual[32*32];
		RK_IntraCalcuResidual(fenc, 
							rk_pred[RK_COMPENT], 
							rk_residual[RK_COMPENT],
							puStride,
							width);

		RK_CheckResidual(rk_residual[X265_COMPENT], rk_residual[RK_COMPENT], 
			puStride, width, height);
	}
	else if ( pInterface_Intra->cidx == 1 )
	{
	// ----------------- chroma U------------------------//
		// step 1 //
		// fill
		RK_IntraFillReferenceSamples( 
								pInterface_Intra->reconEdgePixel,
								pInterface_Intra->bNeighborFlags, 
								LineBuf, 
								pInterface_Intra->numintraNeighbor, 
								unitSize, 
								totalUnits, 
								width,
								height);

		RK_CheckLineBuf(LineBuf, rk_LineBufCb, 2*width + 2*height + 1);
		// chroma ����Ҫ smoothing����
		// ��Ҫ�� lineBuf ��Ϊ refLeft �� refAbove
		RK_Pel 	refLeft[33 + 16 - 1];
		RK_Pel 	refAbove[33 + 16 - 1];
		RK_Pel* 	refLeftDecide = refLeft + width - 1;
		RK_Pel* 	refAboveDecide = refAbove + width - 1;
		for ( int i = 0 ; i < 2*pInterface_Intra->size + 1; i++ )
		{
		    *refLeftDecide++ = LineBuf[2*pInterface_Intra->size - i];
		}

		for ( int i = 0 ; i < 2*pInterface_Intra->size + 1; i++ )
		{
		    *refAboveDecide++ = LineBuf[2*pInterface_Intra->size + i];
		}

		// step 2 //
		// pred with lumaDirMode
		//RK_Pel predSampleCb[16*16];
		int puStride = width;
		int log2BlkSize = rk_g_convertToBit[width] + 2;		
		RkIntraPredAll(rk_predCb[RK_COMPENT], 
				refAbove + width - 1, 
				refLeft + width - 1,
				puStride, 
				log2BlkSize,
				pInterface_Intra->cidx,  
				pInterface_Intra->lumaDirMode);

		RK_CheckPredSamples(rk_IntraPred_35.rk_predSampleCb, rk_predCb[RK_COMPENT], width, height);
		
		// step 3 //
		// caclu resi
		//int16_t *residual = (int16_t*)X265_MALLOC(int16_t, width*height);
		//int16_t residual[16*16];
		RK_IntraCalcuResidual(fenc, 
							rk_predCb[RK_COMPENT], 
							rk_residualCb[RK_COMPENT],
							puStride,
							width);

		RK_CheckResidual(rk_residualCb[X265_COMPENT], rk_residualCb[RK_COMPENT], puStride, width, height);
	    
	}
	else if ( pInterface_Intra->cidx == 2 )
	{
	// ----------------- chroma V------------------------//
		// step 1 //
		// fill
		RK_IntraFillReferenceSamples( 
								pInterface_Intra->reconEdgePixel,
								pInterface_Intra->bNeighborFlags, 
								LineBuf,// ���Ժ� rk_LineBuf �Ƚ�
								pInterface_Intra->numintraNeighbor, 
								unitSize, 
								totalUnits, 
								width,
								height);

		RK_CheckLineBuf(LineBuf, rk_LineBufCr, 2*width + 2*height + 1);

		// chroma ����Ҫ smoothing����
		// ��Ҫ�� lineBuf ��Ϊ refLeft �� refAbove
		RK_Pel 	refLeft[33 + 16 - 1];
		RK_Pel 	refAbove[33 + 16 - 1];
		RK_Pel* 	refLeftDecide = refLeft + width - 1;
		RK_Pel* 	refAboveDecide = refAbove + width - 1;
		for ( int i = 0 ; i < 2*pInterface_Intra->size + 1; i++ )
		{
		    *refLeftDecide++ = LineBuf[2*pInterface_Intra->size - i];
		}

		for ( int i = 0 ; i < 2*pInterface_Intra->size + 1; i++ )
		{
		    *refAboveDecide++ = LineBuf[2*pInterface_Intra->size + i];
		}

		// step 2 //
		// pred with lumaDirMode
		//RK_Pel predSampleCr[16*16];
		int puStride = width;
		int log2BlkSize = rk_g_convertToBit[width] + 2;		
		RkIntraPredAll(rk_predCr[RK_COMPENT], 
				refAbove + width - 1, 
				refLeft + width - 1,
				puStride, 
				log2BlkSize,
				pInterface_Intra->cidx,  
				pInterface_Intra->lumaDirMode);

		RK_CheckPredSamples(rk_IntraPred_35.rk_predSampleCr, rk_predCr[RK_COMPENT], width, height);
		
		// step 3 //
		// caclu resi
		//int16_t *residual = (int16_t*)X265_MALLOC(int16_t, width*height);
		//int16_t residual[16*16];
		RK_IntraCalcuResidual(fenc, 
							rk_predCr[RK_COMPENT], 
							rk_residualCr[RK_COMPENT],
							puStride,
							width);

		RK_CheckResidual(rk_residualCr[X265_COMPENT], rk_residualCr[RK_COMPENT], puStride, width, height);
	}
	else
	{
		RK_HEVC_PRINT("%s Error case Happen.\n",__FUNCTION__);
	}
	
}

/*
** params@ in
**		reconEdgePixel 	- ָ�����Ͻǲο����ָ�룬ǰ�������ݣ�L��buf
**		bNeighborFlags 	- �ο����ص���Ч��flags�����Ͻ�1��Ӧ1�������ĵط�1��ӦunitSize
**		numIntraNeighbor - ��Ч�Եĸ���
**		unitSize 		- ��С������Ч�ߵĸ�����ͨ��Ϊ8��4 ��intraģ���ڲ�����
**		totalUnits 		- ���б�flags���ܸ���
**		width 			- PU���
**		height 			- PU�߶�
** params@ out
**		pLineBuf		- ���õ�LineBuf
*/
void Rk_IntraPred::RK_IntraFillReferenceSamples(RK_Pel* reconEdgePixel, 
											bool* bNeighborFlags, 
											RK_Pel* pLineBuf,
											int numIntraNeighbor, 
											int unitSize, 
											int totalUnits, 
											int width,
											int height)
{
    RK_Pel* piRoiTemp;
    int  i;
    RK_Pel  iDCValue = 1 << (RK_DEPTH - 1);
		
    if (numIntraNeighbor == 0)
    {
        // Fill border with DC value
        for ( i = 0 ; i < 2*height + 1; i++ )
        {
			pLineBuf[i] = iDCValue;
        }
		for ( i = 0 ; i < 2*width; i++ )
        {
			pLineBuf[i+2*height + 1] = iDCValue;
        }
    }
    else if (numIntraNeighbor == totalUnits)
    {
        memcpy(pLineBuf, reconEdgePixel, (2*width + 2*height + 1) * sizeof(RK_Pel));
    }
    else // reference samples are partially available
    {

//        bool bNeighborFlags[4 * MAX_NUM_SPU_W + 1];
// #define MAX_NUM_SPU_W           (RK_MAX_CU_SIZE / MIN_PU_SIZE) // maximum number of SPU in horizontal line
// x265��С��PU�ǵ�4x4���̶�8�����ص�Ϊһ�����
// pbNeighborFlags ��Ҫ���� L ��buf ������
		int  numUnitsInCU = height / unitSize;
	    int  iNumUnits2 = numUnitsInCU << 1;
        int  iTotalSamples = totalUnits * unitSize;
// �ο�x265����������corner�Ǹ������䵽 unitSize ����С
// ��������ʱ��ѭ���������Լ���8����������Ҫ��һ�жϣ����ӳ����Ӷ�
        RK_Pel  lineBufTmp[5 * RK_MAX_CU_SIZE];
        RK_Pel  *pLineTemp = lineBufTmp;
        bool *pbNeighborFlags = bNeighborFlags + iNumUnits2;// ָ��corner
        int  iNext, iCurr;
        RK_Pel  piRef = 0;

        // Initialize
        for (i = 0; i < iTotalSamples; i++)
        {
            lineBufTmp[i] = iDCValue;
        }
		
		// copy L buf below-left
		piRoiTemp = reconEdgePixel;
        for (i = 0; i < (height << 1); i++)
        {
            *pLineTemp++ = *piRoiTemp++;
        }		
		// �� corner pixel ��չ��䵽 unitSize ���ռ��С
		// ����ѭ������
		if (*pbNeighborFlags)
        {
            for (i = 0; i < unitSize; i++)
            {        
        		*pLineTemp = *piRoiTemp;
            }
        }
		pLineTemp += unitSize ;
		piRoiTemp++;
		// copy L buf above-left
        for (i = 0; i < (width << 1); i++)
        {
            *pLineTemp++ = *piRoiTemp++;
        }
	   

        // Pad reference samples when necessary
        iCurr = 0;
        iNext = 1;
		// init pLineTemp
        pLineTemp = lineBufTmp;
        while (iCurr < totalUnits)
        {
            if (!bNeighborFlags[iCurr])
            {
            	// �Ƿ��� ���½ǵ��Ǹ��㣬line buf�еĵ�һ����
                if (iCurr == 0)
                {
                    while (iNext < totalUnits && !bNeighborFlags[iNext])
                    {
                        iNext++;
                    }

                    piRef = pLineTemp[iNext * unitSize];
					// ������½ǵĵ�
                    // Pad unavailable samples with new value
                    //while (iCurr < iNext)
                    {
	                    for (i = 0; i < unitSize; i++)
	                    {
	                        pLineTemp[i] = piRef;
	                    }

		                pLineTemp += unitSize;
		                iCurr++;
                    }
                }
                else // ����������½ǵ��Ǹ��㣬�� L buf ˳���������
                {
                    piRef = lineBufTmp[iCurr * unitSize - 1];
                    for (i = 0; i < unitSize; i++)
                    {
                        pLineTemp[i] = piRef;
                    }

                    pLineTemp += unitSize;
                    iCurr++;
                }
            }
            else
            {
                pLineTemp += unitSize;
                iCurr++;
            }
        }

        // Copy processed samples
        // �����°�����ݣ�1 + 2*CU_width��ע����� corner�㣬
        // ��������cornerռ�� unitSize ���ռ�,ֻ��Ҫѡһ��
        memcpy(pLineBuf,lineBufTmp, (2*height + 1)*sizeof(lineBufTmp[0]));
        memcpy(pLineBuf+2*height + 1,&lineBufTmp[2 * height + unitSize], (2*width)*sizeof(lineBufTmp[0]));
    }
}			



/*
** params@ in
**		pLineBuf 					- ���õ�LineBuf
**		width 						- PU���
**		height 						- PU�߶�
**		bUseStrongIntraSmoothing	- �Ƿ���ǿ�˲���־

** params@ out ����ֳ� left / above ���
**		refLeft			 
**		refAbove		 
**		refLeftFlt		 
**		refAboveFlt		 
*/
void Rk_IntraPred::RK_IntraSmoothing(RK_Pel* pLineBuf,
							int width,
						    int height,
						    bool bUseStrongIntraSmoothing,
                            RK_Pel* refLeft,
							RK_Pel* refAbove, 
							RK_Pel* refLeftFlt, 
							RK_Pel* refAboveFlt)
{

    // generate filtered intra prediction samples
    // left and left above border + above and above right border + top left corner = length of 3. filter buffer
	int cuHeight2 	= height * 2;
	int cuWidth2  	= width * 2;
	int bufSize 	= cuHeight2 + cuWidth2 + 1;

	// L�� buffer���� ���µ����ϣ�Ȼ���ϵ�����
	
    RK_Pel refBuf[129];  // non-filtered
    RK_Pel filteredBuf[129]; // filtered

	// use RkIntraFillRefSamples Out
	for ( int32_t idx = 0 ; idx < cuHeight2 + cuWidth2 + 1 ; idx++ )
	{
	    refBuf[idx] = pLineBuf[idx];		
	}

    //int l = 0;
  
    if (bUseStrongIntraSmoothing)
    {
        int blkSize = 32;
        int bottomLeft = refBuf[0];
        int topLeft = refBuf[cuHeight2];
        int topRight = refBuf[bufSize - 1];
        int threshold = 1 << (RK_DEPTH - 5);
        bool bilinearLeft = abs(bottomLeft + topLeft - 2 * refBuf[height]) < threshold;
        bool bilinearAbove  = abs(topLeft + topRight - 2 * refBuf[cuHeight2 + height]) < threshold;

        if (width >= blkSize && (bilinearLeft && bilinearAbove))
        {
            int shift = rk_g_convertToBit[width] + 3; // log2(uiCuHeight2)
            filteredBuf[0] = refBuf[0];
            filteredBuf[cuHeight2] = refBuf[cuHeight2];
            filteredBuf[bufSize - 1] = refBuf[bufSize - 1];
			// ˫���Բ�ֵ
            for (int i = 1; i < cuHeight2; i++)
            {
                filteredBuf[i] = (RK_Pel)(((cuHeight2 - i) * bottomLeft + i * topLeft + height) >> shift);
            }

            for (int i = 1; i < cuWidth2; i++)
            {
                filteredBuf[cuHeight2 + i] = (RK_Pel)(((cuWidth2 - i) * topLeft + i * topRight + width) >> shift);
            }
        }
        else
        {
            // 1. filtering with [1 2 1]
            filteredBuf[0] = refBuf[0];
            filteredBuf[bufSize - 1] = refBuf[bufSize - 1];
            for (int i = 1; i < bufSize - 1; i++)
            {
                filteredBuf[i] = (refBuf[i - 1] + 2 * refBuf[i] + refBuf[i + 1] + 2) >> 2;
            }
        }
    }
    else
    {
        // 1. filtering with [1 2 1]
        filteredBuf[0] = refBuf[0];
        filteredBuf[bufSize - 1] = refBuf[bufSize - 1];
        for (int i = 1; i < bufSize - 1; i++)
        {
            filteredBuf[i] = (refBuf[i - 1] + 2 * refBuf[i] + refBuf[i + 1] + 2) >> 2;
        }
    }

	// �����35�з�����Ԥ�⣬Ҫ��L�� buf �ֿ�Ϊ above �� left
	// ע�� above �� left ��ǰ��Ҫ Ԥ�� width - 1���������ÿ���
	
	// " ��"
	for ( int idx = 0 ; idx < 2*height + 1 ; idx++ )
	{
	     *refLeft++ = refBuf[2*height - idx];
		
	}
	// " -> "
	for ( int idx = 0 ; idx < 2*width + 1 ; idx++ )
	{
	     *refAbove++ = refBuf[2*height+idx];
	}

	// " ��"
	for ( int idx = 0 ; idx < 2*height + 1 ; idx++ )
	{
	     *refLeftFlt++ = filteredBuf[2*height - idx];
		
	}
	// " -> "
	for ( int idx = 0 ; idx < 2*width + 1 ; idx++ )
	{
	     *refAboveFlt++ = filteredBuf[2*height+idx];
	}

}


/*  
** SAD,from size 4x4 8x8 16x16 32x32
** params@ in
**		pix1 						 
**		stride_pix1 				 
**		pix2 						 
**		stride_pix2 				 
**		size 						-- input [4.8.16.32]
**		dirMode 					- 35��Ԥ�ⷽ���е�һ��

** params@ out  
**		return SAD 					 
*/

int Rk_IntraPred::RK_IntraSad(RK_Pel *pix1, intptr_t stride_pix1, RK_Pel *pix2, intptr_t stride_pix2, uint32_t size)
{
    int sum = 0;

    for (uint32_t y = 0; y < size; y++)
    {
        for (uint32_t x = 0; x < size; x += 4)
        {
            sum += abs(pix1[x + 0] - pix2[x + 0]);
            sum += abs(pix1[x + 1] - pix2[x + 1]);
            sum += abs(pix1[x + 2] - pix2[x + 2]);
            sum += abs(pix1[x + 3] - pix2[x + 3]);
        }

        pix1 += stride_pix1;
        pix2 += stride_pix2;
    }

    return sum;
}

/*  
** SAD,from size 4x4 8x8 16x16 32x32
** params@ in
**		org 						- ԭʼ����			 
**		pred 				 		- Ԥ������
**		stride 						- ͼ��stride 
**		blockSize 					- �в���С [4.8.16.32]

** params@ out  
**		residual 					- �в��ָ��	 
*/
void Rk_IntraPred::RK_IntraCalcuResidual(RK_Pel*	org, 
				RK_Pel *pred, 
				int16_t *residual, 
				intptr_t stride,
				uint32_t blockSize)
{
    for (uint32_t uiY = 0; uiY < blockSize; uiY++)
    {
        for (uint32_t uiX = 0; uiX < blockSize; uiX++)
        {
            residual[uiX] = (int16_t)(org[uiX]) - (int16_t)(pred[uiX]);
        }

        org += stride;
        residual += stride;
        pred += stride;
    }
}


