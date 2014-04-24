


#include "RkIntraPred.h"
#include <cstring>
#include <assert.h>
#include <cmath>
#include "../Lib/TLibCommon/CommonDef.h"

//using namespace RK_HEVC;
extern FILE* g_fp_result_rk;
extern FILE* g_fp_result_x265;
extern FILE* g_fp_4x4_params_rk;
extern FILE* g_fp_4x4_params_x265;
static char rk_g_convertToBit[RK_MAX_CU_SIZE + 1];

// luma direct mode entropy need bits.
uint32_t g_intra_pu_lumaDir_bits[5][256][35];
// total bits [Luma + chroma] for depth = 0,1,2,3,4 
uint32_t g_intra_depth_total_bits[5][256];

uint8_t g_4x4_single_reconEdge[RECON_EDGE_4X4_NUM][4];
// 每个 8x8 的block 存 4条 4像素的边
// 第一维的访问下标为 zorder/4
uint8_t g_4x4_total_reconEdge[64][RECON_EDGE_4X4_NUM][4];

INTERFACE_INTRA g_previous_8x8_intra;
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

uint32_t g_rk_raster2zscan_depth_4[256] =
{
    0       ,     1       ,   4           ,5          , 16       , 17          ,  20       ,     21     ,    64     ,    65     ,   68      ,    69       , 80      ,  81       ,  84      ,  85  		  ,
	2       ,     3       ,   6       ,   7       ,    18    ,    19       ,  22       ,     23     ,    66     ,    67     ,   70      ,    71       , 82      ,  83       ,  86      ,  87  		  ,
	8       ,     9       ,  12      ,  13       ,   24     ,    25       ,   28      ,      29    ,     72   ,     73     ,     76    ,      77     ,   88    ,     89    ,      92  ,      93  	 ,
	10     ,     11      ,  14      ,   15     ,     26   ,     27      ,   30      ,      31    ,     74    ,     75    ,     78    ,     79      ,  90     ,    91     ,   94     ,   95  	   ,
	32     ,     33      ,  36      ,   37     ,     48   ,     49      ,   52      ,      53    ,     96    ,     97    ,    100   ,    101      , 112    ,   113    ,  116    ,  117		  ,
	34     ,     35      ,  38      ,   39     ,     50   ,     51      ,   54      ,      55    ,     98    ,     99    ,    102   ,    103      , 114    ,   115    ,  118    ,  119  	  ,
	40     ,     41      ,  44      ,   45     ,     56   ,     57      ,   60      ,      61    ,    104   ,     105   ,     108  ,      109   ,    120 ,      121  ,     124 ,      125  	 ,
	42     ,     43      ,  46      ,   47     ,     58   ,     59      ,   62      ,      63    ,    106   ,     107   ,     110  ,      111   ,    122 ,      123  ,     126 ,      127  	 ,
	128    ,    129    ,   132    ,    133  ,     144 ,      145    ,   148    ,    149    ,    192   ,     193   ,     196  ,      197   ,    208 ,      209  ,     212 ,      213    ,
	130    ,    131    ,   134    ,    135  ,     146 ,      147    ,   150    ,    151    ,    194   ,     195   ,     198  ,      199   ,    210 ,      211  ,     214 ,      215    ,
	136    ,    137    ,   140    ,    141  ,     152 ,      153    ,   156    ,    157    ,    200   ,     201   ,     204  ,      205   ,    216 ,      217  ,     220 ,      221    ,
	138    ,    139    ,   142    ,    143  ,     154 ,      155    ,   158    ,    159    ,    202   ,     203   ,     206  ,      207   ,    218 ,      219  ,     222 ,      223    ,
	160    ,    161    ,   164    ,    165  ,     176 ,      177    ,   180    ,    181    ,    224   ,     225   ,     228  ,      229   ,    240 ,      241  ,     244 ,      245    ,
	162    ,    163    ,   166    ,    167  ,     178 ,      179    ,   182    ,    183    ,    226   ,     227   ,     230  ,      231   ,    242 ,      243  ,     246 ,      247    ,
	168    ,    169    ,   172    ,    173  ,     184 ,      185    ,   188    ,    189    ,    232   ,     233   ,     236  ,      237   ,    248 ,      249  ,     252 ,      253    ,
	170    ,    171    ,   174    ,    175  ,     186 ,      187    ,   190    ,    191    ,    234   ,     235   ,     238  ,      239   ,    250 ,      251  ,     254 ,      255 
};

const int rk_lambdaMotionSSE_tab_I[MAX_QP + 1] =
{
	65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
	65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 74122, 93388,
	117662, 148245, 186777, 235325, 296490, 373555, 509870, 691812, 933888, 1255066,
	1680115, 2241331, 2980783, 3953212, 5229772, 6902867, 9092389, 11953766, 15060801, 18975421,
	23907532, 30121603, 37950842, 47815065, 60243207, 75901685, 95630131, 120486415, 151803370, 191260262,
	240972830, 303606741
};

const int rk_lambdaMotionSAD_tab_I[MAX_QP + 1] =
{
	65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
	65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 69697, 78232,
	87813, 98566, 110637, 124186, 139394, 156465, 182797, 212928, 247392, 286796,
	331825, 383259, 441982, 508996, 585438, 672596, 771931, 885100, 993491, 1115156,
	1251720, 1405008, 1577068, 1770200, 1986982, 2230312, 2503440, 2810017, 3154137, 3540400,
	3973964, 4460624
};

const int rk_lambdaMotionSSE_tab_non_I[MAX_QP + 1] =
{
	65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
	65536, 65536, 65536, 65536, 80173, 101012, 159085, 160347, 202025, 254536,
	400869, 404051, 509072, 641391, 1010128, 1086021, 1453820, 1939446, 2579301, 3420754,
	4525374, 5973119, 9834668, 10343712, 13575272, 17787922, 29091692, 29322588, 36944146, 46546708,
	73306471, 73888293, 93093416, 117290354, 184720733, 186186832, 234580709, 295553173, 465467080, 469161418,
	591106346, 744747329
};

const int rk_lambdaMotionSAD_tab_non_I[MAX_QP + 1] =
{
	65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536,
	65536, 65536, 65536, 65536, 72486, 81363, 102106, 102511, 115064, 129156,
	162084, 162726, 182654, 205022, 257293, 266783, 308670, 356515, 411141, 473479,
	544586, 625663, 802823, 823338, 943222, 1079698, 1380779, 1386248, 1556011, 1746563,
	2191851, 2200532, 2470014, 2772497, 3479347, 3493127, 3920903, 4401064, 5523119, 5544994,
	6224045, 6986255
};

void Rk_IntraPred::setLambda(int qp, uint8_t slicetype)
{
	if (2 == slicetype) // I Slice
	{
		m_rklambdaMotionSAD = rk_lambdaMotionSAD_tab_I[qp];
		m_rklambdaMotionSSE = rk_lambdaMotionSSE_tab_I[qp];
	}
	else
	{
		m_rklambdaMotionSAD = rk_lambdaMotionSAD_tab_non_I[qp];
		m_rklambdaMotionSSE = rk_lambdaMotionSSE_tab_non_I[qp];
	}
}

Rk_IntraPred::Rk_IntraPred()
{
#ifdef X265_LOG_FILE_ROCKCHIP
	rk_logIntraPred[0] = fopen(PATH_NAME("x265_intra_params.txt"), "w+" );
	if ( rk_logIntraPred[0] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	rk_logIntraPred[1] = fopen(PATH_NAME("fastMode_log.txt"), "w+" );
	if ( rk_logIntraPred[1] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	rk_logIntraPred[2] = fopen(PATH_NAME("refMain_Expand.txt"), "w+" );
	if ( rk_logIntraPred[2] == NULL )
	{
	    RK_HEVC_PRINT("creat file failed.\n");
	}
	rk_logIntraPred[3] = fopen(PATH_NAME("intra_ang_tab.txt"), "w+" );
	if ( rk_logIntraPred[3] == NULL )
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

	initRk_g_convertToBit();
	
}

Rk_IntraPred::~Rk_IntraPred()
{
#ifdef X265_LOG_FILE_ROCKCHIP
	for (int i = 0 ; i < 4 ; i++ )
	{
		fclose(rk_logIntraPred[i]);
	}
#endif

}

/* 封装35中预测函数
** params@ in
**		refAbove 					- above 参考边
**		refLeft 					- left 参考边
**		stride						- 通常和width相等，但是4x4的时候stride可以等于8[允许PU下划]
**		log2_size 					- left 参考边
**		c_idx 						- =0表示 luma分量
**		dirMode 					- 35中预测方向中的一种

** params@ out  
**		predSample 					- 要填充的预测像素指针
*/
void Rk_IntraPred::RkIntraPredAll(uint8_t *predSample, 
            	uint8_t *refAbove, 
            	uint8_t *refLeft,
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
**		above 						- above 参考边 length: width + 1
**		left 						- left 参考边 length: heigth + 1 
**		stride						- 通常和width相等，但是4x4的时候stride可以等于8[允许PU下划]
**		log2_size 					- left 参考边

** params@ out 
**		predSample 					- 要填充的预测像素指针
*/
void Rk_IntraPred::RkIntraPred_PLANAR(uint8_t *predSample, 
				uint8_t 	*above, 
				uint8_t 	*left, 
				int 	stride, 
				int 	log2_size)
{
    int x, y;
    int nTbS = (1 << log2_size);
	//** 参考协议中，左上角的点不计入运算,above 和 left 需要地址+1
	above++;
	left++;
	for (y = 0; y < nTbS; y++) 
	{
        for (x = 0; x < nTbS; x++) 
		{
            predSample[y*stride + x] = (uint8_t)(( (nTbS - 1 - x) * left[y]  	+ (x + 1) * above[nTbS] +
                         				 (nTbS - 1 - y) * above[x] 	+ (y + 1) * left[nTbS] + nTbS) >>
                        					(log2_size + 1));
        }
    }
}






/* ** DC[predmode=1],from size 4x4 8x8 16x16 32x32
** params@ in
**		above 						- above 参考边 length: width + 1 前向也有 width - 1的空间
**		left 						- left 参考边 length: heigth + 1 前向也有 heigth - 1的空间
**		stride						- 通常和width相等，但是4x4的时候stride可以等于8[允许PU下划]
**		log2_size 					- left 参考边
**		c_idx 						- =0表示 luma分量

** params@ out  
**		predSample 					- 要填充的预测像素指针
*/
void Rk_IntraPred::RkIntraPred_DC(uint8_t *predSample, 
			uint8_t 	*above, 
			uint8_t 	*left, 
			int 	stride, 
			int 	log2_size, 
			int 	c_idx)
{
    int i, j, x, y;
    int nTbS = (1 << log2_size);
	//** 参考协议中，左上角的点不计入运算,above 和 left 需要地址+1
	above++;
	left++;
    int dc = nTbS;
    for (i = 0; i < nTbS; i++)
	{
        dc += left[i] + above[i]; 
	}
    dc >>= (log2_size + 1);

	// 先都赋值为DC，条件修改
	for (i = 0; i < nTbS; i++)
	{
    	for (j = 0; j < nTbS; j++)
		{
    		predSample[j*stride + i] = (uint8_t) dc;	
		}
	}

	// luma分量且nTbS < 32
    if (c_idx == 0 && nTbS < 32) 
	{
        predSample[0] = (uint8_t)((left[0] + 2 * dc  + above[0] + 2) >> 2);
		
        for (x = 1; x < nTbS; x++)
    	{
            predSample[x] = (uint8_t)((above[x] + 3 * dc + 2) >> 2);
    	}
        for (y = 1; y < nTbS; y++)
    	{
            predSample[y*stride] = (uint8_t)((left[y] + 3 * dc + 2) >> 2);
        }
    }

}

/* ** angular[predmode=1],from size 4x4 8x8 16x16 32x32
** params@ in
**		above 						- above 参考边 length: width + 1 前向也有 width - 1的空间
**		left 						- left 参考边 length: heigth + 1 前向也有 heigth - 1的空间
**		stride						- 通常和width相等，但是4x4的时候stride可以等于8[允许PU下划]
**		log2_size 					- left 参考边
**		c_idx 						- =0表示 luma分量
**		dirMode 					- 35中预测方向中的一种

** params@ out  
**		predSample 					- 要填充的预测像素指针
*/

void Rk_IntraPred::RkIntraPred_angular(uint8_t *predSample, 
            	uint8_t *refAbove, 
            	uint8_t *refLeft,
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
		
		// 以above为主，从left中[下标从1开始，跳过左上角的点]选取一些追加到refMain的上面
		if (intraPredAngle < 0)
        {
            for (k = -1; k > ext_valid_idx; k--)
            {                
                refMain[k] = refSide[(abs(invAngle*k) + 128) >> 8];
            }			
        }

        if (intraPredAngle == 0)
        {
        	// 垂直或水平方向，直接一个像素点填充一整行和一整列
        	// 注意 refMain中包含左上角的点，所以下标要比 predSample 的下标 + 1
            for (k = 0; k < width; k++)
            {
                for (l = 0; l < width; l++)
                {
                    predSample[k * stride + l] = refMain[l + 1];
                }
            }
			// 只有width < 32，是luma，且预测角度为 0 或者90度(这里有旋转过，只有0度)
            if ((width < 32) && isLuma)
            {
                for (k = 0; k < width; k++)
                {
					predSample[k * stride] = (pixel)ClipY(refMain[1]  + ((refSide[k + 1] - refSide[0]) >> 1));
                }
            }
   
	    }
        else // intraPredAngle != 0[根据映射关系进行]
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
	int8_t mapIdxInrefSide[32];
	if(modeVer)
	{
        uint8_t* refMain;
        uint8_t* refSide;
		int x,y;
		int ext_valid_idx = width * intraPredAngle >> 5;

        refMain = refAbove; // + (width - 1);
        refSide = refLeft; // + (width - 1);
		
		// 以above为主，从left中[下标从1开始，跳过左上角的点]选取一些追加到refMain的上面
		RK_HEVC_FPRINT(rk_logIntraPred[2], "--- %dx%d, Mode = %2d, map left to above[ y --> x] --- \n",width, width, dirMode);
		RK_HEVC_FPRINT(rk_logIntraPred[3], "--- %dx%d, Mode = %2d, [ y --> x] --- \n",width, width, dirMode);
		if (intraPredAngle < 0)
        {
            for ( x = -1; x > ext_valid_idx; x--)
            {                
                refMain[x] = refSide[(abs(invAngle*x) + 128) >> 8];
				// log expand index	
				int8_t refSide_idx = (abs(invAngle*x) + 128) >> 8;
				RK_HEVC_FPRINT(rk_logIntraPred[2], "refLeft[ %2d ] map to refMain[ %2d ]\n", refSide_idx, x);
				// 存储映射前 原来的位置(~2*width ~ 2*width)
				mapIdxInrefSide[-x] = -refSide_idx;
		
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
				// Int + 1, Int + 2是实际取数下标
				// Int + 2*widht 就是 linebuf 的下标, 此时取数变成 (linebufIdx + 1) (linebufIdx + 2)
				RK_HEVC_FPRINT(rk_logIntraPred[2], "y:[%2d] -> [Int = %2d (refMain) %2d (linebuf),used %2d, %2d,Fract = %2d] \n", y , deltaInt, deltaInt + 2*width,
				deltaInt + 2*width + 1, deltaInt + 2*width + 2,deltaFract);

				int8_t pos1 =  (deltaInt + 1) < 0 ? mapIdxInrefSide[-(deltaInt + 1)] : deltaInt + 1;
				int8_t pos2 =  (deltaInt + 2) < 0 ? mapIdxInrefSide[-(deltaInt + 2)] : deltaInt + 2;
				RK_HEVC_FPRINT(rk_logIntraPred[3], "y = %2d , %2d , %2d / %2d , %2d , %2d / %2d \n",
											y , 32 - deltaFract, pos1, pos1 + 2*width,
												 	 deltaFract, pos2, pos2 + 2*width);													


#ifdef X265_LOG_FILE_ROCKCHIP
				// 对某一角度的一个y,不同x = y*tan(角度)
				rk_verdeltaFract[y] = deltaFract;
				rk_verdeltaInt[y] = deltaInt;
#endif
                if (deltaFract)
                {
                    // 双线性插值
                    for (x = 0; x < width; x++)
                    {
                        refMainIndex = x + deltaInt + 1;
                        predSample[y * stride + x] = (uint8_t)((
							(32 - deltaFract) * refMain[refMainIndex] + 
								   deltaFract * refMain[refMainIndex + 1] 
								   + 16) >> 5);
                    }
                }
                else
                {
                    // 直接赋值
                    for (x = 0; x < width; x++)
                    {
                        predSample[y * stride + x] = refMain[x + deltaInt + 1];
                    }
                }
            }
        }
		else // intraPredAngle == 0
		{
        	// 垂直直接一个像素点填充一整列
        	// 注意 refMain中包含左上角的点，所以下标要比 predSample 的下标 + 1
            for (y = 0; y < width; y++)
            {
				RK_HEVC_FPRINT(rk_logIntraPred[3], "y = %2d , %2d , %2d / %2d , %2d , %2d / %2d \n",
								y , 32, 1, 1 + 2*width,
									0,  2, 2 + 2*width);													
           
                for (x = 0; x < width; x++)
                {
                    predSample[y * stride + x] = refMain[x + 1];
                }
            }
			// 只有width < 32，是luma，且预测角度为 0  
            if ((width < 32) && isLuma)
            {
                for (y = 0; y < width; y++)
                {
					predSample[y * stride] = (uint8_t)RK_ClipY(refMain[1]  + ((refSide[y + 1] - refSide[0]) >> 1));
                }
            }
   
	    }

    }
	
	if(modeHor)
	{
        uint8_t* refMain;
        uint8_t* refSide;
		int x,y;
		int ext_valid_idx = width * intraPredAngle >> 5;

        refMain = refLeft; // + (width - 1);
        refSide = refAbove; // + (width - 1);
		
		// 以left为主，从above中[下标从1开始，跳过左上角的点]选取一些追加到refMain的上面
		RK_HEVC_FPRINT(rk_logIntraPred[2], "--- %dx%d, Mode = %0d, map above to left[ x --> y] ---  \n",width, width, dirMode);
		RK_HEVC_FPRINT(rk_logIntraPred[3], "--- %dx%d, Mode = %0d, [ x --> y] ---  \n",width, width, dirMode);
		if (intraPredAngle < 0)
        {
            for (y = -1; y > ext_valid_idx; y--)
            {                
                refMain[y] = refSide[(abs(invAngle*y) + 128) >> 8];
				// log expand index				
				RK_HEVC_FPRINT(rk_logIntraPred[2], "refAbove[ %2d ] map to refMain[ %2d ]\n", (abs(invAngle*y) + 128) >> 8, y);
				// 存储映射前 原来的位置
				int8_t refSide_idx = (abs(invAngle*y) + 128) >> 8;
				mapIdxInrefSide[-y] = refSide_idx;

			}			
        }


        if( intraPredAngle != 0)//[根据映射关系进行]
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
				// Int + 1, Int + 2是实际取数下标
				// Int + 2*widht 就是 linebuf 的下标, 此时取数变成 (linebufIdx - 1) (linebufIdx - 2)
				RK_HEVC_FPRINT(rk_logIntraPred[2], "x:[%2d] -> [Int = %2d (refMain) %2d (linebuf),used %2d, %2d,Fract = %2d] \n", x , deltaInt, abs(deltaInt) + 2*width,  abs(deltaInt) + 2*width - 2,
				 abs(deltaInt) + 2*width - 1, deltaFract);

				int8_t pos1 =  (deltaInt + 1) < 0 ? mapIdxInrefSide[-(deltaInt + 1)] : -(deltaInt + 1);
				int8_t pos2 =  (deltaInt + 2) < 0 ? mapIdxInrefSide[-(deltaInt + 2)] : -(deltaInt + 2);
				RK_HEVC_FPRINT(rk_logIntraPred[3], "x = %2d , %2d , %2d / %2d , %2d , %2d / %2d \n",
											x , 32 - deltaFract, pos1, pos1 + 2*width,
												 	 deltaFract, pos2, pos2 + 2*width);													
		
#ifdef X265_LOG_FILE_ROCKCHIP
				// 对某一角度的一个x,不同y = x*ctan(角度)
				rk_hordeltaFract[x] = deltaFract;
				rk_hordeltaInt[x] = deltaInt;
#endif
                if (deltaFract)
                {
                    // 双线性插值
                    for (y = 0; y < width; y++)
                    {
                        refMainIndex = y + deltaInt + 1;
                        predSample[y * stride + x] = (uint8_t)(
							((32 - deltaFract) * refMain[refMainIndex] + 
							        deltaFract * refMain[refMainIndex + 1] 
							        + 16) >> 5);
                    }
                }
                else
                {
                    // 直接赋值
                    for (y = 0; y < width; y++)
                    {
                        predSample[y * stride + x] = refMain[y + deltaInt + 1];
                    }
                }
            }
        }
		else // (intraPredAngle == 0)
        {

		
        	// 水平方向，直接一个像素点填充一整行
        	// 注意 refMain中包含左上角的点，所以下标要比 predSample 的下标 + 1
            for (x = 0; x < width; x++)
            {
 				RK_HEVC_FPRINT(rk_logIntraPred[3], "x = %2d , %2d , %2d / %2d , %2d , %2d / %2d \n",
								x , 32 - 0,   -1, -1 + 2*width,
									     0,   -2, -2 + 2*width);													
          
                for (y = 0; y < width; y++)
                {
                    predSample[y * stride + x] = refMain[y + 1];
                }
            }
			// 只有width < 32，是luma，且预测角度为 0 
            if ((width < 32) && isLuma)
            {
                for (x = 0; x < width; x++)
                {
					predSample[0 * stride + x] = (uint8_t)RK_ClipY(refMain[1]  + ((refSide[x + 1] - refSide[0]) >> 1));
                }
            }
   
	    }
    }
#endif
    
}



#if 0
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
#endif

void Rk_IntraPred::RkIntraPred_angularCheck()
{
	int i,j;
	// 只有有效数据进行 compare
	//RK_HEVC_PRINT("dirMode = %d \r", rk_dirMode);
	if ( rk_dirMode >= 0)	
	{
		for ( i = 0 ; i < rk_puwidth_35 ; i++ )
		{
			for ( j = 0 ; j < rk_puwidth_35 ; j++ )
			{
				if(rk_IntraPred_35.rk_predSample[i*rk_puwidth_35 + j] != rk_IntraPred_35.rk_predSampleOrg[i*rk_puwidth_35 + j])
		        {
		        	RK_HEVC_PRINT("check falied %s \n",__FUNCTION__);
				}
			}
		}
	}
}



void Rk_IntraPred::RkIntraFillRefSamplesCheck(uint8_t* above, uint8_t* left, int32_t width, int32_t heigth)
{
	// 数据靠前存储
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

void Rk_IntraPred::RkIntraFillChromaCheck(uint8_t* pLineBuf, uint8_t* above, uint8_t* left, uint32_t width, uint32_t heigth)
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
	// 数据靠前存储
   	for ( int32_t idx = 0 ; idx < 2*rk_puHeight + 1 ; idx++ )
    {
        if((rk_IntraSmoothIn.rk_refAboveFiltered[X265_COMPENT][idx] != rk_IntraSmoothIn.rk_refAboveFiltered[RK_COMPENT][idx]) || 
        (rk_IntraSmoothIn.rk_refLeftFiltered[X265_COMPENT][idx] != rk_IntraSmoothIn.rk_refLeftFiltered[RK_COMPENT][idx]))
        {
            RK_HEVC_PRINT("%s Error happen\n",__FUNCTION__);			
        }
    }
}  


void Rk_IntraPred::fillReferenceSamples(uint8_t* roiOrigin, 
							uint8_t* adiTemp, 
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
    uint8_t* piRoiTemp;
    int  i, j;
    uint8_t  iDCValue = 1 << (RK_DEPTH - 1);

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
        uint8_t  piAdiLine[5 * RK_MAX_CU_SIZE];
        uint8_t  *piAdiLineTemp;
        bool *pbNeighborFlags;
        int  iNext, iCurr;
        uint8_t  piRef = 0;

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

void Rk_IntraPred::RkIntrafillRefSamples(uint8_t* roiOrigin, 
							bool* bNeighborFlags, 
							int numIntraNeighbor, 
							int unitSize, 
							int numUnitsInCU, 
							int totalUnits, 
							int width,
							int height,
							int picStride,
							uint8_t* pLineBuf)
{
    uint8_t* piRoiTemp;
    int  i, j;
    uint8_t  iDCValue = 1 << (RK_DEPTH - 1);
	
		
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
    	// roiOrigin 指向pic的初始左上角位置
    	
    	// corner 元素就是 roiOrigin - stride -1    	
        // Fill top-left border with rec. samples
        piRoiTemp = roiOrigin - picStride - 1;
        pLineBuf[2 * height] = *piRoiTemp;

    	// left 元素就是 roiOrigin - 1
        // Fill left border with rec. samples
        // Fill below left border with rec. samples
        piRoiTemp = roiOrigin - 1;

        for (i = 0; i < 2 * height; i++)
        {
			pLineBuf[2*height - 1 - i] = *piRoiTemp;
            piRoiTemp += picStride;
        }

    	// above 元素就是 roiOrigin - stride
        // Fill top border with rec. samples
        // Fill top right border with rec. samples
        piRoiTemp = roiOrigin - picStride;
        memcpy(&pLineBuf[1 + 2 * height ], piRoiTemp, 2 * width * sizeof(pLineBuf[0]));
    }
    else // reference samples are partially available
    {

//        bool bNeighborFlags[4 * MAX_NUM_SPU_W + 1];
// #define MAX_NUM_SPU_W           (RK_MAX_CU_SIZE / MIN_PU_SIZE) // maximum number of SPU in horizontal line
// x265最小的PU是到4x4，固定8个像素点为一个组合
// pbNeighborFlags 需要按照 L 型buf 传进来
//    	int  unitSize = 8; 
//		int  numUnitsInCU = rk_puHeight / unitSize;
        int  iNumUnits2 = numUnitsInCU << 1;
        int  iTotalSamples = totalUnits * unitSize;
// 参考x265的做法，把corner那个点扩充到 unitSize 个大小
// 这样填充的时候循环次数可以减少8倍，否则需要逐一判断，增加程序复杂度
        uint8_t  lineBufTmp[5 * RK_MAX_CU_SIZE];
        uint8_t  *pLineTemp = lineBufTmp;
        bool *pbNeighborFlags;
        int  iNext, iCurr;
        uint8_t  piRef = 0;

        // Initialize
        for (i = 0; i < iTotalSamples; i++)
        {
            lineBufTmp[i] = iDCValue;
        }

		// 将corner填充到line buf的中间,下标为 0，且占 unitSize 个空间大小
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
		// 填充line buf的上半段，从roiTemp中的 " ↑" 左下元素开始 往上填充
		// 实现 L buf的排列
        // Fill left & below-left samples
        piRoiTemp += picStride;
		// 指向存储left的地址
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

		// 填充line buf的下半段，从roiTemp中的 above 方向为 "->" 
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
		// rk_LineBuf 已经是配置好的 L buf
        pLineTemp = lineBufTmp;
        while (iCurr < totalUnits)
        {
            if (!bNeighborFlags[iCurr])
            {
            	// 是否是 左下角的那个点，line buf中的第一个点
                if (iCurr == 0)
                {
                    while (iNext < totalUnits && !bNeighborFlags[iNext])
                    {
                        iNext++;
                    }

                    piRef = pLineTemp[iNext * unitSize];
					// 填充左下角的点
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
                else // 如果不是左下角的那个点，按 L buf 顺序依次填充
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
        // 拷贝下半段数据，1 + 2*CU_width，注意包括 corner点，
        // 但是这里corner占用 unitSize 个空间,只需要选一个
        memcpy(pLineBuf,lineBufTmp, (2*height + 1)*sizeof(lineBufTmp[0]));
        memcpy(pLineBuf+2*height + 1,&lineBufTmp[2 * height + unitSize], (2*width)*sizeof(lineBufTmp[0]));
    }
}

void Rk_IntraPred::RkIntraSmoothing(uint8_t* refLeft,
									uint8_t* refAbove, 
									uint8_t* refLeftFlt, 
									uint8_t* refAboveFlt)
{

    // generate filtered intra prediction samples
    // left and left above border + above and above right border + top left corner = length of 3. filter buffer
	int32_t cuHeight2 = rk_puHeight * 2;
	int32_t cuWidth2  = rk_puWidth * 2;
	int32_t bufSize = cuHeight2 + cuWidth2 + 1;

	// L型 buffer构建 左下到左上，然后上到上右
	
    uint8_t refBuf[129];  // non-filtered
    uint8_t filteredBuf[129]; // filtered
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
			// 双线性插值
            for (int i = 1; i < cuHeight2; i++)
            {
                filteredBuf[i] = (uint8_t)(((cuHeight2 - i) * bottomLeft + i * topLeft + rk_puHeight) >> shift);
            }

            for (int i = 1; i < cuWidth2; i++)
            {
                filteredBuf[cuHeight2 + i] = (uint8_t)(((cuWidth2 - i) * topLeft + i * topRight + rk_puWidth) >> shift);
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

	// 输出给35中方向做预测，要求L型 buf 分开为 above 和 left
	// 注意 above 和 left 在前面要 预留 width - 1，由外层调用控制
	
	// " ↓"
	for ( int idx = 0 ; idx < 2*rk_puHeight + 1 ; idx++ )
	{
	     *refLeft++ = refBuf[2*rk_puHeight - idx];
		
	}
	// " -> "
	for ( int idx = 0 ; idx < 2*rk_puWidth + 1 ; idx++ )
	{
	     *refAbove++ = refBuf[2*rk_puHeight+idx];
	}

	// " ↓"
	for ( int idx = 0 ; idx < 2*rk_puHeight + 1 ; idx++ )
	{
	     *refLeftFlt++ = filteredBuf[2*rk_puHeight - idx];
		
	}
	// " -> "
	for ( int idx = 0 ; idx < 2*rk_puWidth + 1 ; idx++ )
	{
	     *refAboveFlt++ = filteredBuf[2*rk_puHeight+idx];
	}



	// 存储左上角的点，然后到左下角
	for ( int idx = 0 ; idx < 2*rk_puHeight + 1 ; idx++ )
	{
	     rk_IntraSmoothIn.rk_refLeftFiltered[RK_COMPENT][idx] = filteredBuf[2*rk_puHeight - idx];
		
	}
	// 存储左上角的点，然后到右上角
	for ( int idx = 0 ; idx < 2*rk_puWidth + 1 ; idx++ )
	{
	     rk_IntraSmoothIn.rk_refAboveFiltered[RK_COMPENT][idx] = filteredBuf[2*rk_puHeight+idx];
	}
}



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


#ifdef RK_INTRA_MODE_CHOOSE
int CacluVariance_45(uint8_t* block, int width)
{
	uint16_t i,j,dir;
	uint16_t sumdir[16];
	uint16_t countdir[16];
	uint16_t elementdir[16][16];
	uint16_t direct_num;
	
	// 45° 
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
				// 45°角的关系
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
	// 求sum(|mean-a| + |mean-b| + ...)
	for ( dir = 0 ; dir < direct_num; dir++ )
	{
		uint16_t avg = sumdir[dir]/countdir[dir];
		uint16_t absDiffSumDirect = 0;
		// 一个方向所有元素累加
		for ( count = 0 ; count < countdir[dir] ; count++ )
		{
			absDiffSumDirect += abs(avg - elementdir[dir][count] );
		}
		// 所有方向累加
		absDiffSumTotal += absDiffSumDirect;
	}

	return 	absDiffSumTotal;
}
int CacluVariance_180(uint8_t* block, int width)
{
	uint16_t sumdir[16];
	uint16_t direct_num;
	
	// 180°一行累加
	direct_num = width;
	// 数组访问是行优先，第一维对应 pic的Y方向，二维才是PIC的X方向
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
void symmetryForYaxis(uint8_t* block, int width)
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
void transpose(uint8_t* src, int blockSize)
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
int CacluVariance_135(uint8_t* block, int width)
{
	// 135°与 45°关于 Y 轴对称

	symmetryForYaxis(block,width);
	int ret = CacluVariance_45(block,width);
	return ret;

}

int CacluVariance_90(uint8_t* block, int width)
{
	// 180°与 90°转置关系
	transpose(block, width);
	int ret = CacluVariance_180(block,width);
	return ret;

}

 
void Rk_IntraPred::RkIntraPriorModeChoose(int rdModeCandidate[], uint32_t rdModeCost[], bool bsort )
{
	if ( !bsort )
	{
	    // 根据 rdModeCost[35] 选择 2 10 18 26 34 进行排序
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
		// 如果最优和次优相邻，取夹在中间的方向，外加另一方向的各4种
		if ( abs(index[0] - index[1]) == 1)
		{
			int k = 2;
			int min_idx = std::min<int>(Mode[0], Mode[1]) - 4;
			int max_idx = std::max<int>(Mode[0], Mode[1]) + 4;
			// 限定方向在 2~34之间		
			min_idx = min_idx < 2 ? 2 : min_idx;
			max_idx = max_idx > 34 ? 34 : max_idx;
			// 16种方向
		    for ( i = min_idx ; i < max_idx ; i += FAST_MODE_STEP  )
		    {
		        rdModeCandidate[k++] = i;
		    }
		}
		else
		// 如果最优和次优相邻，取各自自己左右方向的4种
		{
			int k = 2;
			int min_idx = Mode[0] - 4;
			int max_idx = Mode[0] + 4;
			// 限定方向在 2~34之间		
			min_idx = min_idx < 2 ? 2 : min_idx;
			max_idx = max_idx > 34 ? 34 : max_idx;
			// 16种方向
		    for ( i = min_idx ; i < max_idx ; i += FAST_MODE_STEP  )
		    {
		        rdModeCandidate[k++] = i;
		    }
			min_idx = Mode[1] - 4;
			max_idx = Mode[1] + 3;
			// 限定方向在 2~34之间		
			min_idx = min_idx < 2 ? 2 : min_idx;
			max_idx = max_idx > 34 ? 34 : max_idx;
			// 16种方向
		    for ( i = min_idx ; i < max_idx ; i += FAST_MODE_STEP )
		    {
		        rdModeCandidate[k++] = i;
		    }

		}
	}
	else
	// 根据 rdModeCost 最小选择最优
	{
		int i,index[18];
		uint32_t cost[18];
		int valid_len = 0;
		// 统计 rdModeCandidate 不等于-1的个数
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

 


// ------------------------------------------------------------------------------
/*
**  
**
*/
// ------------------------------------------------------------------------------

void RK_CheckLineBuf(uint8_t* LineBuf1, uint8_t* LineBuf2, uint32_t size)
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
void RK_CheckSmoothing(uint8_t* refLeft1, uint8_t* refAbove1, uint8_t* refLeftFlt1, uint8_t* refAboveFlt1,
								uint8_t* refLeft2, uint8_t* refAbove2, uint8_t* refLeftFlt2, uint8_t* refAboveFlt2,
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
void RK_CheckPredSamples(uint8_t* pred1, uint8_t* pred2, uint32_t width, uint32_t height)
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
	/* 这里stride等于width，即使是width=4的情况，就分开4次比较 */
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


void Rk_IntraPred::Convert8x8To4x4(uint8_t* pdst, uint8_t* psrc, uint8_t partIdx)
{
	uint8_t  offset[4] = {0, 4, 32, 36};
	psrc += offset[partIdx];
	for (uint8_t i = 0; i < 4; i++)
	{
		::memcpy(pdst, psrc, 4*sizeof(uint8_t));
		pdst += 4;
		psrc += 8;
	}	
}

void Rk_IntraPred::Convert4x4To8x8(uint8_t* pdst, uint8_t *psrcList[4])
{
	uint8_t  offset[4] = {0, 4, 32, 36};
	
	for (uint8_t j = 0 ; j < 4 ; j++ )
	{
	    uint8_t* pdstTmp 	= pdst + offset[j];
		uint8_t* psrc 		= psrcList[j];
		for (uint8_t i = 0; i < 4; i++)
		{
			::memcpy(pdstTmp, psrc, 4*sizeof(uint8_t));
			pdstTmp += 8;
			psrc 	+= 4;
		}	
	}	
}

void Rk_IntraPred::Convert4x4To8x8(int16_t* pdst, int16_t *psrcList[4])
{
	uint8_t  offset[4] = {0, 4, 32, 36};
	
	for (uint8_t j = 0 ; j < 4 ; j++ )
	{
	    int16_t* pdstTmp 	= pdst + offset[j];
		int16_t* psrc 		= psrcList[j];
		for (uint8_t i = 0; i < 4; i++)
		{
			::memcpy(pdstTmp, psrc, 4*sizeof(int16_t));
			pdstTmp += 8;
			psrc 	+= 4;
		}	
	}	
}

/*
** 填充 4个 4x4 的参考边集合
*/
void Rk_IntraPred::RK_store4x4Recon2Ref(uint8_t* pEdge, uint8_t* pRecon, uint32_t partOffset)
{
	uint8_t right_line[4];
	uint8_t bottom_line[4];

	// L buf 顺序
	right_line[0] = pRecon[15];
	right_line[1] = pRecon[11];
	right_line[2] = pRecon[7];
	right_line[3] = pRecon[3];
	
	bottom_line[0] = pRecon[12];
	bottom_line[1] = pRecon[13];
	bottom_line[2] = pRecon[14];
	bottom_line[3] = pRecon[15];


	if ( partOffset == 0)
	{
		::memcpy(pEdge, right_line, 4);		//    PART_0_RIGHT,
		::memcpy(pEdge + 4,bottom_line, 4); //    PART_0_BOTTOM,
	}
	else if ( partOffset == 1 )
	{
		::memcpy(pEdge + 8,bottom_line, 4); //    PART_1_BOTTOM,
	}
	else if ( partOffset == 2 )
	{
		::memcpy(pEdge + 12,right_line, 4); //     PART_2_RIGHT,
	}
}


void Rk_IntraPred::splitTo4x4By8x8(INTERFACE_INTRA* intra8x8, 
							INTERFACE_INTRA* intra4x4, 
							uint8_t* p4x4_reconEdge, 
							uint32_t partOffset)
{

	uint8_t i, j;
	uint8_t* src;
	uint8_t* dst;
	// fenc
	src = intra8x8->fenc;
	dst = intra4x4->fenc;
	if ( partOffset == 0 )
	{
		src += 0;
	}
	else if ( partOffset == 1 )
	{
		src += 4;
	}
	else if ( partOffset == 2 )
	{
		src += 32;
	}
	else if ( partOffset == 3 )
	{
		src += 36;
	}

	for ( i = 0 ; i < 4 ; i++ )
	{
	    for ( j = 0 ; j < 4 ; j++ )
	    {
	        dst[j] = src[j];
	    }
		dst += 4;
		src += 8;
	}

	// reconEdgePixel
	// split the 8x8 reconEdgePixel to 9 group;
	uint8_t L_recon[8][4], Mid;
	uint8_t* pRecon8x8 = intra8x8->reconEdgePixel;
	for ( i = 0 ; i < 4 ; i++ )
	{
		for ( j = 0 ; j < 4 ; j++ )
		{
		    L_recon[i][j] = *pRecon8x8++;
		}
	}
    Mid = *pRecon8x8++;
	for ( i = 4 ; i < 8 ; i++ )
	{
		for ( j = 0 ; j < 4 ; j++ )
		{
		    L_recon[i][j] = *pRecon8x8++;
		}
	}

	// bNeighborFlags
	// [	flag1, flag1,flag2, flag2, ]
	//  	flag3,
	// [	flag4, flag4, flag5, flag5 ]
	if ( partOffset == 0 )
	{
		//List_4x4_0 = [L2, L3, Mid, L4, L5	] 	
		::memcpy(intra4x4->reconEdgePixel, intra8x8->reconEdgePixel + 8, 17);

	 	// [flag2,	flag2,	flag3,	flag4,	flag4]
		::memcpy(intra4x4->bNeighborFlags, intra8x8->bNeighborFlags + 2, 5);

	}
	else if ( partOffset == 1 )
	{
		//List_4x4_1 = [X, PART_0_RIGHT, L4_last, L5, L6] 	
		::memcpy(intra4x4->reconEdgePixel + 4, p4x4_reconEdge, 4);
		intra4x4->reconEdgePixel[8] = L_recon[4][3];
		::memcpy(intra4x4->reconEdgePixel + 9,  &L_recon[5][0], 4);
		::memcpy(intra4x4->reconEdgePixel + 13, &L_recon[6][0], 4);

	 	// [0,   	1,   	flag4,	flag4, 	flag5]
		intra4x4->bNeighborFlags[0] = 0;
		intra4x4->bNeighborFlags[1] = 1;
		intra4x4->bNeighborFlags[2] = intra8x8->bNeighborFlags[6];
		intra4x4->bNeighborFlags[3] = intra8x8->bNeighborFlags[6];
		intra4x4->bNeighborFlags[4] = intra8x8->bNeighborFlags[7];
		
	}
	else if ( partOffset == 2 )
	{
		//List_4x4_2 = [L1, L2, L3_frist, PART_0_BOTTOM, PART_1_BOTTOM] 	
		::memcpy(intra4x4->reconEdgePixel 	 , &L_recon[1][0], 4);
		::memcpy(intra4x4->reconEdgePixel + 4 , &L_recon[2][0], 4);
		intra4x4->reconEdgePixel[8] = L_recon[3][0];
		::memcpy(intra4x4->reconEdgePixel + 9 , p4x4_reconEdge + 4, 4);
		::memcpy(intra4x4->reconEdgePixel + 13, p4x4_reconEdge + 8, 4);

		// [flag1,	flag2	flag2,	1,		1   ]
		intra4x4->bNeighborFlags[0] = intra8x8->bNeighborFlags[1];
		intra4x4->bNeighborFlags[1] = intra8x8->bNeighborFlags[2];
		intra4x4->bNeighborFlags[2] = intra8x8->bNeighborFlags[3];
		intra4x4->bNeighborFlags[3] = 1;
		intra4x4->bNeighborFlags[4] = 1;
	}
	//List_4x4_3 = [X, PART_2_RIGHT, PART_0_BOTTOM_last, PART_1_BOTTOM, X ] 	
	else if ( partOffset == 3 )
	{
		::memcpy(intra4x4->reconEdgePixel + 4 , p4x4_reconEdge + 12, 4);
		intra4x4->reconEdgePixel[8] = *(p4x4_reconEdge + 7);
		::memcpy(intra4x4->reconEdgePixel + 9 , p4x4_reconEdge + 8, 4);
	 	// [0,   	1,   	1,   	1,  	0	]
		intra4x4->bNeighborFlags[0] = 0;
		intra4x4->bNeighborFlags[1] = 1;
		intra4x4->bNeighborFlags[2] = 1;
		intra4x4->bNeighborFlags[3] = 1;
		intra4x4->bNeighborFlags[4] = 0;

	}

 	intra4x4->size = intra8x8->size >> 1;
	intra4x4->cidx = intra8x8->cidx;
	// 统计 numintraNeighbor
	intra4x4->numintraNeighbor = 0;
	for ( i = 0 ; i < 5 ; i++ )
	{
		if( intra4x4->bNeighborFlags[i] == true )
	    	intra4x4->numintraNeighbor++;
	}
}



void StoreLineBuf(FILE* fp, uint8_t* LineBuf, uint32_t size)
{

	RK_HEVC_FPRINT(fp, "LineBuf:\n");	
	for (uint32_t i = 0 ; i < size ; i++ )
	{
		RK_HEVC_FPRINT(fp, "0x%02x ",*LineBuf++);
	}
	RK_HEVC_FPRINT(fp, "\n\n");	
}
void StoreSmoothing(FILE* fp,
						uint8_t* refLeft, 
						uint8_t* refAbove, 
						uint8_t* refLeftFlt, 
						uint8_t* refAboveFlt,
						uint32_t width, 
						uint32_t height)
{

	RK_HEVC_FPRINT(fp, "Above:\n");	
	uint32_t i;
	for ( i = 0 ; i < 2*width + 1 ; i++ )
	{
		RK_HEVC_FPRINT(fp,"0x%02x ",refAbove[i]);
	}

	RK_HEVC_FPRINT(fp, "\nAboveflt:\n");	
	for ( i = 0 ; i < 2*width + 1 ; i++ )
	{
		RK_HEVC_FPRINT(fp,"0x%02x ",refAboveFlt[i]);
	}	 

	RK_HEVC_FPRINT(fp, "\nLeft:\n");	
	for ( i = 0 ; i < 2*height + 1 ; i++ )
	{
		RK_HEVC_FPRINT(fp,"0x%02x ",refLeft[i]);
	}

	RK_HEVC_FPRINT(fp, "\nLeftflt:\n");	
	for ( i = 0 ; i < 2*height + 1 ; i++ )
	{
		RK_HEVC_FPRINT(fp,"0x%02x ",refLeftFlt[i]);
	}
	
	RK_HEVC_FPRINT(fp, "\n\n");	
}

void StorePredSamples(FILE* fp,uint8_t* pred, uint32_t width, uint32_t height)
{
	uint32_t i,j;
	RK_HEVC_FPRINT(fp, "pred:\n");	
	for ( i = 0 ; i < height ; i++ )
	{
		for ( j = 0 ; j < width; j++ )
		{
    		RK_HEVC_FPRINT(fp,"0x%02x ",pred[i * width + j]);
		}
		RK_HEVC_FPRINT(fp, "\n");	
	}
	RK_HEVC_FPRINT(fp, "\n\n");	
}


void StoreSad(FILE* fp,int* cost)
{
	uint32_t i;
	RK_HEVC_FPRINT(fp, "COST:\n");	

	for ( i = 0 ; i < 35 ; i++ )
	{
		RK_HEVC_FPRINT(fp, "%d  ",cost[i]);
	}
	RK_HEVC_FPRINT(fp, "\n\n");	
}

void StoreCost(FILE* fp,uint64_t* cost)
{
	uint32_t i;
	RK_HEVC_FPRINT(fp, "COST:\n");	

	for ( i = 0 ; i < 35 ; i++ )
	{
		RK_HEVC_FPRINT(fp, "%lld  ",cost[i]);
	}
	RK_HEVC_FPRINT(fp, "\n\n");	
}
void StoreResidual(FILE* fp, int16_t* resi, uint32_t stride, uint32_t width, uint32_t height)
{
	uint32_t i,j;
	RK_HEVC_FPRINT(fp, "Resi:\n");	
	/* 这里stride等于width，即使是width=4的情况，就分开4次比较 */
	for ( i = 0 ; i < height ; i++ )
	{
		for ( j = 0 ; j < width; j++ )
		{
    		RK_HEVC_FPRINT(fp, "0x%04x  ", (uint16_t)resi[j]);
		}
		resi += stride;
		RK_HEVC_FPRINT(fp, "\n");	
	}
	RK_HEVC_FPRINT(fp, "\n\n");	
}

void Rk_IntraPred::Store4x4ReconInfo(FILE* fp,  INTERFACE_INTRA inf_intra4x4, uint32_t partOffset)
{
	RK_HEVC_FPRINT(fp,"[partOffset = %d]\n",partOffset);
	RK_HEVC_FPRINT(fp,"fenc: \n");
	uint8_t* pfenc0 = inf_intra4x4.fenc;
	for (uint8_t i = 0 ; i < 4; i++ )
	{
	    for (uint8_t j = 0 ; j < 4 ; j++ )
	    {
	    	RK_HEVC_FPRINT(fp,"0x%02x ",pfenc0[j]);
		}
		pfenc0 += 4;
		RK_HEVC_FPRINT(fp,"\n");		
	}
	RK_HEVC_FPRINT(fp,"\n \n");
	
	RK_HEVC_FPRINT(fp,"bNeighborFlags: \n");
	for (uint8_t i = 0 ; i < 5; i++ )
	{
    	RK_HEVC_FPRINT(fp,"%0d ",inf_intra4x4.bNeighborFlags[i] == true ? 1 : 0);
	}
	RK_HEVC_FPRINT(fp,"\n \n");

	
	RK_HEVC_FPRINT(fp,"recon: \n");
	uint8_t* pReconTmp0 = inf_intra4x4.reconEdgePixel;
	for (uint8_t i = 0 ; i < 2; i++ )
	{
	    for (uint8_t j = 0 ; j < 4 ; j++ )
	    {
			if ( inf_intra4x4.bNeighborFlags[i] == true )
			{
			    RK_HEVC_FPRINT(fp,"0x%02x ",pReconTmp0[j] );
			}
			else
			{
			    RK_HEVC_FPRINT(fp,"xxxx " );
			}
		}
		pReconTmp0 += 4;	 
	}
	if ( inf_intra4x4.bNeighborFlags[2] == true )
	{
	    RK_HEVC_FPRINT(fp,"\n 0x%02x \n",*pReconTmp0++ );
	}
	else
	{
	    RK_HEVC_FPRINT(fp,"\n xxxx \n" );
		pReconTmp0++;		
	}
	
	for (uint8_t i = 3 ; i < 5; i++ )
	{
	    for (uint8_t j = 0 ; j < 4 ; j++ )
	    {
			if ( inf_intra4x4.bNeighborFlags[i] == true )
			{
			    RK_HEVC_FPRINT(fp,"0x%02x ",pReconTmp0[j] );
			}
			else
			{
			    RK_HEVC_FPRINT(fp,"xxxx " );
			}
		}
		pReconTmp0 += 4;	
	}
	RK_HEVC_FPRINT(fp,"\n \n");

}
void Rk_IntraPred::Intra_Proc(INTERFACE_INTRA* pInterface_Intra, 
									uint32_t partOffset,
									uint32_t cur_depth,
									uint32_t cur_x_in_cu,
									uint32_t cur_y_in_cu)
{
	uint8_t* fenc 	= pInterface_Intra->fenc;
	int32_t width 	= pInterface_Intra->size;
	int32_t height	= pInterface_Intra->size;
	uint8_t LineBuf[129];
	// unitSize 只有4x4的时候是 4，其他case都是8
	// x265中固定为 4 
    int unitSize      = pInterface_Intra->cidx == 0 ? 4 : 2;
    int numUnitsInCU  = width / unitSize;
    int totalUnits    = (numUnitsInCU << 2) + 1;

	
// ----------------- luma ------------------------//
	if ( pInterface_Intra->cidx == 0)
	{
		/* step 1 */
		// fill
		//uint8_t LineBuf[129];
		RK_IntraFillReferenceSamples( 
								pInterface_Intra->reconEdgePixel,
								pInterface_Intra->bNeighborFlags, 
								LineBuf,// 可以和 rk_LineBuf 比较
								pInterface_Intra->numintraNeighbor, 
								unitSize, 
								totalUnits, 
								width,
								height);
#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_rk,"Y:\n");
		StoreLineBuf(g_fp_result_rk, LineBuf, 2*width + 2*height + 1);
#endif
		// step 2 //
		
		// smoothing
		uint8_t refLeft[65 + 32 - 1];
		uint8_t refAbove[65 + 32 - 1];
		uint8_t refLeftFlt[65 + 32 - 1];
		uint8_t refAboveFlt[65 + 32 - 1];
		bool bUseStrongIntraSmoothing = pInterface_Intra->useStrongIntraSmoothing;
		RK_IntraSmoothing(LineBuf, 
						width, 
						height, 
						bUseStrongIntraSmoothing,
						refLeft + width - 1,
						refAbove + width - 1,
						refLeftFlt + width - 1,
						refAboveFlt + width - 1);
#ifdef INTRA_RESULT_STORE_FILE
		// width - 1 是为了在做pred扩展用的
		StoreSmoothing(g_fp_result_rk,
						refLeft + width - 1, 
						refAbove + width - 1, 
						refLeftFlt + width - 1, 
						refAboveFlt + width - 1, 
						width, 
						height);
#endif	

		// step 3 //
		
		// predition
		//uint8_t* predSample = (uint8_t*)X265_MALLOC(uint8_t, width*height);
		uint8_t predSample[32*32];


		uint8_t RK_intraFilterThreshold[5] =
		{
		    10, //4x4
		    7,  //8x8
		    1,  //16x16
		    0,  //32x32
		    10, //64x64
		};
		int log2BlkSize = rk_g_convertToBit[width] + 2;
		// X265外层 puStride 大小都等于 width,只有4x4时的NxN, puStride = 8 //
		// 在RK_ENCODER中，数据都是紧凑存储，没有stride = 8的情况
		int puStride = width;
		int dirMode;
		int costSad[35];
		uint64_t costTotal[35];
		for ( dirMode = 0 ; dirMode < 35 ; dirMode++ )
		{
	    	int 	diff		= std::min<int>(abs((int)dirMode - HOR_IDX), abs((int)dirMode - VER_IDX));
			uint8_t filterIdx 	= diff > RK_intraFilterThreshold[log2BlkSize - 2] ? 1 : 0;
			
			uint8_t* refAboveDecide = refAbove + width - 1;
			uint8_t* refLeftDecide = refLeft + width - 1;

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

#ifdef INTRA_RESULT_STORE_FILE_
					
			StorePredSamples(g_fp_result_rk, predSample, width, height);
			
#endif

			// step 4 //
			// caclu SAD
			costSad[dirMode] = RK_IntraSad(fenc, 
					puStride, 
					predSample, 
					puStride, 
					width);
			// 
		#ifdef RK_CABAC
			uint32_t zscan_idx = g_rk_raster2zscan_depth_4[cur_x_in_cu/4 + cur_y_in_cu*4];
			uint32_t bits = g_intra_pu_lumaDir_bits[cur_depth][zscan_idx + partOffset][dirMode];
		#else
			uint32_t bits = 0;
		#endif

			//setLambda(30, 2);
			costTotal[dirMode] =  costSad[dirMode] + ((bits * m_rklambdaMotionSAD + 32768) >> 16); 
		}

#ifdef INTRA_RESULT_STORE_FILE
		StoreSad(g_fp_result_rk, costSad);
		StoreCost(g_fp_result_rk, costTotal);
#endif

		// step 5 //
		// get minnum costSad + lambad*bits,decide the dirMode
		uint32_t index[35];
		for (int i = 0 ; i < 35 ; i++ )
		{
		    index[i] = i;
		}
		BubbleSort(index,costTotal, 35);

		int bestMode = index[0];

		pInterface_Intra->DirMode = (uint8_t)bestMode;

		// step 6 //
		int 	diff		= std::min<int>(abs((int)bestMode - HOR_IDX), abs((int)bestMode - VER_IDX));
		uint8_t filterIdx 	= diff > RK_intraFilterThreshold[log2BlkSize - 2] ? 1 : 0;
		uint8_t* refAboveDecide = refAbove + width - 1;
		uint8_t* refLeftDecide 	= refLeft + width - 1; 
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

#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_rk,"Y:\n");
		StorePredSamples(g_fp_result_rk, rk_pred[RK_COMPENT], width, height);
		StoreResidual(g_fp_result_rk, rk_residual[RK_COMPENT], puStride, width, height);
#endif
		::memcpy(pInterface_Intra->pred, rk_pred[RK_COMPENT], width*height*sizeof(uint8_t) ); 
		::memcpy(pInterface_Intra->resi, rk_residual[RK_COMPENT], width*height*sizeof(int16_t) ); 

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
#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_rk,"U:\n");
		StoreLineBuf(g_fp_result_rk, LineBuf, 2*width + 2*height + 1);
#endif		
		// chroma 不需要 smoothing操作
		// 需要将 lineBuf 变为 refLeft 和 refAbove
		uint8_t 	refLeft[33 + 16 - 1];
		uint8_t 	refAbove[33 + 16 - 1];
		uint8_t* 	refLeftDecide = refLeft + width - 1;
		uint8_t* 	refAboveDecide = refAbove + width - 1;
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
		//uint8_t predSampleCb[16*16];
		int puStride = width;
		int log2BlkSize = rk_g_convertToBit[width] + 2;		
		RkIntraPredAll(rk_predCb[RK_COMPENT], 
				refAbove + width - 1, 
				refLeft + width - 1,
				puStride, 
				log2BlkSize,
				pInterface_Intra->cidx,  
				pInterface_Intra->lumaDirMode);
#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_rk,"dir: %d\n" ,pInterface_Intra->lumaDirMode);
		RK_HEVC_FPRINT(g_fp_result_rk,"U:\n");
		StorePredSamples(g_fp_result_rk, rk_predCb[RK_COMPENT], width, height);
#endif		
		// step 3 //
		// caclu resi
		//int16_t *residual = (int16_t*)X265_MALLOC(int16_t, width*height);
		//int16_t residual[16*16];
		RK_IntraCalcuResidual(fenc, 
							rk_predCb[RK_COMPENT], 
							rk_residualCb[RK_COMPENT],
							puStride,
							width);
#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_rk,"U:\n");
		StoreResidual(g_fp_result_rk, rk_residualCb[RK_COMPENT], puStride, width, height);
#endif
		::memcpy(pInterface_Intra->pred, rk_predCb[RK_COMPENT], width*height*sizeof(uint8_t) ); 
		::memcpy(pInterface_Intra->resi, rk_residualCb[RK_COMPENT], width*height*sizeof(int16_t) ); 


	    
	}
	else if ( pInterface_Intra->cidx == 2 )
	{
	// ----------------- chroma V------------------------//
		// step 1 //
		// fill
		RK_IntraFillReferenceSamples( 
								pInterface_Intra->reconEdgePixel,
								pInterface_Intra->bNeighborFlags, 
								LineBuf,// 可以和 rk_LineBuf 比较
								pInterface_Intra->numintraNeighbor, 
								unitSize, 
								totalUnits, 
								width,
								height);

#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_rk,"V:\n");
		StoreLineBuf(g_fp_result_rk, LineBuf, 2*width + 2*height + 1);
#endif
		// chroma 不需要 smoothing操作
		// 需要将 lineBuf 变为 refLeft 和 refAbove
		uint8_t 	refLeft[33 + 16 - 1];
		uint8_t 	refAbove[33 + 16 - 1];
		uint8_t* 	refLeftDecide = refLeft + width - 1;
		uint8_t* 	refAboveDecide = refAbove + width - 1;
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
		//uint8_t predSampleCr[16*16];
		int puStride = width;
		int log2BlkSize = rk_g_convertToBit[width] + 2;		
		RkIntraPredAll(rk_predCr[RK_COMPENT], 
				refAbove + width - 1, 
				refLeft + width - 1,
				puStride, 
				log2BlkSize,
				pInterface_Intra->cidx,  
				pInterface_Intra->lumaDirMode);

#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_rk,"dir: %d\n" ,pInterface_Intra->lumaDirMode);
		RK_HEVC_FPRINT(g_fp_result_rk,"V:\n");
		StorePredSamples(g_fp_result_rk, rk_predCr[RK_COMPENT], width, height);
#endif		
		
		// step 3 //
		// caclu resi
		//int16_t *residual = (int16_t*)X265_MALLOC(int16_t, width*height);
		//int16_t residual[16*16];
		RK_IntraCalcuResidual(fenc, 
							rk_predCr[RK_COMPENT], 
							rk_residualCr[RK_COMPENT],
							puStride,
							width);

#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_rk,"V:\n");
		StoreResidual(g_fp_result_rk, rk_residualCr[RK_COMPENT], puStride, width, height);
#endif
		::memcpy(pInterface_Intra->pred, rk_predCr[RK_COMPENT], width*height*sizeof(uint8_t) ); 
		::memcpy(pInterface_Intra->resi, rk_residualCr[RK_COMPENT], width*height*sizeof(int16_t) ); 
	}
	else
	{
		RK_HEVC_PRINT("%s Error case Happen.\n",__FUNCTION__);
	}
	
}






void Rk_IntraPred::RkIntra_proc(INTERFACE_INTRA* pInterface_Intra, 
									uint32_t partOffset,
									uint32_t cur_depth,
									uint32_t cur_x_in_cu,
									uint32_t cur_y_in_cu)
{
	uint8_t* 	fenc 	= pInterface_Intra->fenc;
	int32_t width 	= pInterface_Intra->size;
	int32_t height	= pInterface_Intra->size;
	uint8_t LineBuf[129];
	// unitSize 只有4x4的时候是 4，其他case都是8
	// x265中固定为 4 
    int unitSize      = pInterface_Intra->cidx == 0 ? 4 : 2;
    int numUnitsInCU  = width / unitSize;
    int totalUnits    = (numUnitsInCU << 2) + 1;

	
// ----------------- luma ------------------------//
	if ( pInterface_Intra->cidx == 0)
	{
		/* step 1 */
		// fill
		//uint8_t LineBuf[129];
		RK_IntraFillReferenceSamples( 
								pInterface_Intra->reconEdgePixel,
								pInterface_Intra->bNeighborFlags, 
								LineBuf,// 可以和 rk_LineBuf 比较
								pInterface_Intra->numintraNeighbor, 
								unitSize, 
								totalUnits, 
								width,
								height);

		RK_CheckLineBuf(LineBuf, rk_LineBuf, 2*width + 2*height + 1);
#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_x265,"Y:\n");
		StoreLineBuf(g_fp_result_x265, LineBuf, 2*width + 2*height + 1);
#endif
		// step 2 //
		
		// smoothing
		uint8_t refLeft[65 + 32 - 1];
		uint8_t refAbove[65 + 32 - 1];
		uint8_t refLeftFlt[65 + 32 - 1];
		uint8_t refAboveFlt[65 + 32 - 1];
		bool bUseStrongIntraSmoothing = rk_bUseStrongIntraSmoothing;
		RK_IntraSmoothing(LineBuf, 
						width, 
						height, 
						bUseStrongIntraSmoothing,
						refLeft + width - 1,
						refAbove + width - 1,
						refLeftFlt + width - 1,
						refAboveFlt + width - 1);
		// width - 1 是为了在做pred扩展用的
		RK_CheckSmoothing(refLeft + width - 1, refAbove + width - 1, refLeftFlt + width - 1, refAboveFlt + width - 1, 
			rk_IntraSmoothIn.rk_refLeft, rk_IntraSmoothIn.rk_refAbove, 
			rk_IntraSmoothIn.rk_refLeftFiltered[X265_COMPENT], rk_IntraSmoothIn.rk_refAboveFiltered[X265_COMPENT],
			width, height);
#ifdef INTRA_RESULT_STORE_FILE
		// width - 1 是为了在做pred扩展用的
		StoreSmoothing(g_fp_result_x265,
						refLeft + width - 1, 
						refAbove + width - 1, 
						refLeftFlt + width - 1, 
						refAboveFlt + width - 1, 
						width, 
						height);
#endif

		// step 3 //
		
		// predition
		//uint8_t* predSample = (uint8_t*)X265_MALLOC(uint8_t, width*height);
		uint8_t predSample[32*32];


		uint8_t RK_intraFilterThreshold[5] =
		{
		    10, //4x4
		    7,  //8x8
		    1,  //16x16
		    0,  //32x32
		    10, //64x64
		};
		int log2BlkSize = rk_g_convertToBit[width] + 2;
		// X265外层 puStride 大小都等于 width,只有4x4时的NxN, puStride = 8 //
		// 在RK_ENCODER中，数据都是紧凑存储，没有stride = 8的情况
		int puStride = width;
		int dirMode;
		int costSad[35];
		uint64_t costTotal[35];
		for ( dirMode = 0 ; dirMode < 35 ; dirMode++ )
		{
	    	int 	diff		= std::min<int>(abs((int)dirMode - HOR_IDX), abs((int)dirMode - VER_IDX));
			uint8_t filterIdx 	= diff > RK_intraFilterThreshold[log2BlkSize - 2] ? 1 : 0;
			
			uint8_t* refAboveDecide = refAbove + width - 1;
			uint8_t* refLeftDecide = refLeft + width - 1;

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
					
			RK_CheckPredSamples(
				rk_IntraPred_35.rk_predSampleTmp[dirMode], 
				predSample, width, height);
#ifdef INTRA_RESULT_STORE_FILE_
			StorePredSamples(g_fp_result_x265, predSample, width, height);
#endif

			// step 4 //
			// caclu SAD
			costSad[dirMode] = RK_IntraSad(fenc, 
					puStride, 
					predSample, 
					puStride, 
					width);

			costTotal[dirMode] =  costSad[dirMode] + ((rk_bits[partOffset][dirMode] * rk_lambdaMotionSAD + 32768) >> 16); 
		#ifdef RK_CABAC
			uint32_t zscan_idx = g_rk_raster2zscan_depth_4[cur_x_in_cu/4 + cur_y_in_cu*4];
			if ( g_intra_pu_lumaDir_bits[cur_depth][zscan_idx + partOffset][dirMode] != rk_bits[partOffset][dirMode] )
			{
			    RK_HEVC_PRINT("%d ",g_intra_pu_lumaDir_bits[cur_depth][zscan_idx + partOffset][dirMode]);
				RK_HEVC_PRINT("%d \n",rk_bits[partOffset][dirMode]);
			}
			assert(g_intra_pu_lumaDir_bits[cur_depth][zscan_idx + partOffset][dirMode] == rk_bits[partOffset][dirMode]);
		#endif
		}

		RK_CheckSad(costTotal, rk_modeCostsSadAndCabacCorrect);
#ifdef INTRA_RESULT_STORE_FILE
		StoreSad(g_fp_result_x265, costSad);
		StoreCost(g_fp_result_x265, costTotal);
#endif

		// step 5 //
		// get minnum costSad + lambad*bits,decide the dirMode
		uint32_t index[35];
		for (int i = 0 ; i < 35 ; i++ )
		{
		    index[i] = i;
		}
		BubbleSort(index,costTotal, 35);

		int bestMode = index[0];
	
		// 快速模式如果miss最优，这个assert 会 failed

		assert(bestMode == rk_bestMode[X265_COMPENT][partOffset]);
	
		// step 6 //
		int 	diff		= std::min<int>(abs((int)bestMode - HOR_IDX), abs((int)bestMode - VER_IDX));
		uint8_t filterIdx 	= diff > RK_intraFilterThreshold[log2BlkSize - 2] ? 1 : 0;
		uint8_t* refAboveDecide = refAbove + width - 1;
		uint8_t* refLeftDecide 	= refLeft + width - 1; 
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
#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_x265,"Y:\n");
		StorePredSamples(g_fp_result_x265, rk_pred[RK_COMPENT], width, height);
		StoreResidual(g_fp_result_x265, rk_residual[RK_COMPENT], puStride, width, height);
#endif
	#ifdef INTRA_4x4_DEBUG
		if ( width == 4)
		{
			RK_store4x4Recon2Ref(&rk_4x4RefCandidate[0][0], rk_recon[X265_COMPENT], partOffset);

			// 利用 4x4 划分前那个8x8的信息进行 模拟拆分
			/*
			** 根据 8x8 的信息填充 4个 4x4 的信息
			*/
			// 参考 设计文档 4.8.	更新参考边[4x4的PU]
			INTERFACE_INTRA inf_intra4x4;
			inf_intra4x4.bNeighborFlags = (bool*)X265_MALLOC(bool, 5);
			inf_intra4x4.fenc 			= (uint8_t*)X265_MALLOC(uint8_t, 4*4);
			inf_intra4x4.reconEdgePixel = (uint8_t*)X265_MALLOC(uint8_t, 8 + 8 + 1);

			uint32_t zscan_idx = g_rk_raster2zscan_depth_4[cur_x_in_cu/4 + cur_y_in_cu*4]/4;	

			splitTo4x4By8x8(&g_previous_8x8_intra, &inf_intra4x4, &g_4x4_total_reconEdge[zscan_idx][0][0], partOffset);
		#ifdef INTRA_RESULT_STORE_FILE
			Store4x4ReconInfo(g_fp_4x4_params_x265, inf_intra4x4, partOffset );
		#endif
		#if 0
			// compare fenc/bNeighborFlags/reconEdgePixel
			uint8_t* pReconTmp0 = inf_intra4x4.reconEdgePixel;
			uint8_t* pReconTmp1 = pInterface_Intra->reconEdgePixel;
			for (uint8_t i = 0 ; i < 5; i++ )
			{
			    if( inf_intra4x4.bNeighborFlags[i] != pInterface_Intra->bNeighborFlags[i])
		    	   	RK_HEVC_PRINT("check falied %s \n",__FUNCTION__);
				
				
				// corner
				if ( i == 2 )
				{
					if((true == inf_intra4x4.bNeighborFlags[i])	&& ( pReconTmp0[0] != pReconTmp1[0]))
			        	RK_HEVC_PRINT("check falied %s \n",__FUNCTION__);	
				
					pReconTmp0 ++;
					pReconTmp1 ++;
				}
				else
				{
					if(true == inf_intra4x4.bNeighborFlags[i])
					{
					    for (uint8_t j = 0 ; j < 4 ; j++ )
					    {
					        if( pReconTmp0[j] != pReconTmp1[j])
					        	RK_HEVC_PRINT("check falied %s \n",__FUNCTION__);
						}
					}
					pReconTmp0 += 4;
					pReconTmp1 += 4;						
				}
				

			}
			for (uint8_t i = 0 ; i < 16; i++ )
			{
			    if( inf_intra4x4.fenc[i] != pInterface_Intra->fenc[i])
		        	RK_HEVC_PRINT("check falied %s \n",__FUNCTION__);
			}
		#endif

			X265_FREE(inf_intra4x4.bNeighborFlags);
			X265_FREE(inf_intra4x4.fenc);
			X265_FREE(inf_intra4x4.reconEdgePixel);

			
		}

		if ( width == 8 )
		{
			::memcpy(g_previous_8x8_intra.bNeighborFlags, pInterface_Intra->bNeighborFlags, 9);
			::memcpy(g_previous_8x8_intra.fenc, pInterface_Intra->fenc, 8*8);
			::memcpy(g_previous_8x8_intra.reconEdgePixel, pInterface_Intra->reconEdgePixel, 33);
			g_previous_8x8_intra.numintraNeighbor = pInterface_Intra->numintraNeighbor;
			g_previous_8x8_intra.cidx = 0;
			g_previous_8x8_intra.size = 8;
			
		}
	#endif
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
#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_x265,"U:\n");
		StoreLineBuf(g_fp_result_x265, LineBuf, 2*width + 2*height + 1);
#endif	
		
		// chroma 不需要 smoothing操作
		// 需要将 lineBuf 变为 refLeft 和 refAbove
		uint8_t 	refLeft[33 + 16 - 1];
		uint8_t 	refAbove[33 + 16 - 1];
		uint8_t* 	refLeftDecide = refLeft + width - 1;
		uint8_t* 	refAboveDecide = refAbove + width - 1;
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
		//uint8_t predSampleCb[16*16];
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

#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_x265,"dir: %d\n" ,pInterface_Intra->lumaDirMode);
		RK_HEVC_FPRINT(g_fp_result_x265,"U:\n");
		StorePredSamples(g_fp_result_x265, rk_predCb[RK_COMPENT], width, height);
#endif		
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

#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_x265,"U:\n");
		StoreResidual(g_fp_result_x265, rk_residualCb[RK_COMPENT], puStride, width, height);
#endif

	    
	}
	else if ( pInterface_Intra->cidx == 2 )
	{
	// ----------------- chroma V------------------------//
		// step 1 //
		// fill
		RK_IntraFillReferenceSamples( 
								pInterface_Intra->reconEdgePixel,
								pInterface_Intra->bNeighborFlags, 
								LineBuf,// 可以和 rk_LineBuf 比较
								pInterface_Intra->numintraNeighbor, 
								unitSize, 
								totalUnits, 
								width,
								height);

		RK_CheckLineBuf(LineBuf, rk_LineBufCr, 2*width + 2*height + 1);

#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_x265,"V:\n");
		StoreLineBuf(g_fp_result_x265, LineBuf, 2*width + 2*height + 1);
#endif	
		// chroma 不需要 smoothing操作
		// 需要将 lineBuf 变为 refLeft 和 refAbove
		uint8_t 	refLeft[33 + 16 - 1];
		uint8_t 	refAbove[33 + 16 - 1];
		uint8_t* 	refLeftDecide = refLeft + width - 1;
		uint8_t* 	refAboveDecide = refAbove + width - 1;
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
		//uint8_t predSampleCr[16*16];
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
#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_x265,"dir: %d\n" ,pInterface_Intra->lumaDirMode);
		RK_HEVC_FPRINT(g_fp_result_x265,"V:\n");
		StorePredSamples(g_fp_result_x265, rk_predCr[RK_COMPENT], width, height);
#endif		
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
#ifdef INTRA_RESULT_STORE_FILE
		RK_HEVC_FPRINT(g_fp_result_x265,"V:\n");
		StoreResidual(g_fp_result_x265, rk_residualCr[RK_COMPENT], puStride, width, height);
#endif
	}
	else
	{
		RK_HEVC_PRINT("%s Error case Happen.\n",__FUNCTION__);
	}
	
}

/*
** params@ in
**		reconEdgePixel 	- 指向左上角参考点的指针，前后都有数据，L型buf
**		bNeighborFlags 	- 参考像素点有效性flags，左上角1对应1，其他的地方1对应unitSize
**		numIntraNeighbor - 有效性的个数
**		unitSize 		- 最小连续有效边的个数，通常为8，4 由intra模块内部控制
**		totalUnits 		- 所有边flags的总个数
**		width 			- PU宽度
**		height 			- PU高度
** params@ out
**		pLineBuf		- 填充好的LineBuf
*/
void Rk_IntraPred::RK_IntraFillReferenceSamples(uint8_t* reconEdgePixel, 
											bool* bNeighborFlags, 
											uint8_t* pLineBuf,
											int numIntraNeighbor, 
											int unitSize, 
											int totalUnits, 
											int width,
											int height)
{
    uint8_t* piRoiTemp;
    int  i;
    uint8_t  iDCValue = 1 << (RK_DEPTH - 1);
		
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
        memcpy(pLineBuf, reconEdgePixel, (2*width + 2*height + 1) * sizeof(uint8_t));
    }
    else // reference samples are partially available
    {

//        bool bNeighborFlags[4 * MAX_NUM_SPU_W + 1];
// #define MAX_NUM_SPU_W           (RK_MAX_CU_SIZE / MIN_PU_SIZE) // maximum number of SPU in horizontal line
// x265最小的PU是到4x4，固定8个像素点为一个组合
// pbNeighborFlags 需要按照 L 型buf 传进来
		int  numUnitsInCU = height / unitSize;
	    int  iNumUnits2 = numUnitsInCU << 1;
        int  iTotalSamples = totalUnits * unitSize;
// 参考x265的做法，把corner那个点扩充到 unitSize 个大小
// 这样填充的时候循环次数可以减少8倍，否则需要逐一判断，增加程序复杂度
        uint8_t  lineBufTmp[5 * RK_MAX_CU_SIZE];
        uint8_t  *pLineTemp = lineBufTmp;
        bool *pbNeighborFlags = bNeighborFlags + iNumUnits2;// 指向corner
        int  iNext, iCurr;
        uint8_t  piRef = 0;

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
		// 将 corner pixel 扩展填充到 unitSize 个空间大小
		// 便于循环处理
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
            	// 是否是 左下角的那个点，line buf中的第一个点
                if (iCurr == 0)
                {
                    while (iNext < totalUnits && !bNeighborFlags[iNext])
                    {
                        iNext++;
                    }

                    piRef = pLineTemp[iNext * unitSize];
					// 填充左下角的点
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
                else // 如果不是左下角的那个点，按 L buf 顺序依次填充
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
        // 拷贝下半段数据，1 + 2*CU_width，注意包括 corner点，
        // 但是这里corner占用 unitSize 个空间,只需要选一个
        memcpy(pLineBuf,lineBufTmp, (2*height + 1)*sizeof(lineBufTmp[0]));
        memcpy(pLineBuf+2*height + 1,&lineBufTmp[2 * height + unitSize], (2*width)*sizeof(lineBufTmp[0]));
    }
}			



/*
** params@ in
**		pLineBuf 					- 填充好的LineBuf
**		width 						- PU宽度
**		height 						- PU高度
**		bUseStrongIntraSmoothing	- 是否做强滤波标志

** params@ out 输出分成 left / above 输出
**		refLeft			 
**		refAbove		 
**		refLeftFlt		 
**		refAboveFlt		 
*/
void Rk_IntraPred::RK_IntraSmoothing(uint8_t* pLineBuf,
							int width,
						    int height,
						    bool bUseStrongIntraSmoothing,
                            uint8_t* refLeft,
							uint8_t* refAbove, 
							uint8_t* refLeftFlt, 
							uint8_t* refAboveFlt)
{

    // generate filtered intra prediction samples
    // left and left above border + above and above right border + top left corner = length of 3. filter buffer
	int cuHeight2 	= height * 2;
	int cuWidth2  	= width * 2;
	int bufSize 	= cuHeight2 + cuWidth2 + 1;

	// L型 buffer构建 左下到左上，然后上到上右
	
    uint8_t refBuf[129];  // non-filtered
    uint8_t filteredBuf[129]; // filtered

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
			// 双线性插值
            for (int i = 1; i < cuHeight2; i++)
            {
                filteredBuf[i] = ((cuHeight2 - i) * bottomLeft + i * topLeft + height) >> shift;
            }

            for (int i = 1; i < cuWidth2; i++)
            {
                filteredBuf[cuHeight2 + i] = ((cuWidth2 - i) * topLeft + i * topRight + width) >> shift;
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

	// 输出给35中方向做预测，要求L型 buf 分开为 above 和 left
	// 注意 above 和 left 在前面要 预留 width - 1，由外层调用控制
	
	// " ↓"
	for ( int idx = 0 ; idx < 2*height + 1 ; idx++ )
	{
	     *refLeft++ = refBuf[2*height - idx];
		
	}
	// " -> "
	for ( int idx = 0 ; idx < 2*width + 1 ; idx++ )
	{
	     *refAbove++ = refBuf[2*height+idx];
	}

	// " ↓"
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
**		dirMode 					- 35中预测方向中的一种

** params@ out  
**		return SAD 					 
*/

int Rk_IntraPred::RK_IntraSad(uint8_t *pix1, intptr_t stride_pix1, uint8_t *pix2, intptr_t stride_pix2, uint32_t size)
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
**		org 						- 原始像素			 
**		pred 				 		- 预测像素
**		stride 						- 图像stride 
**		blockSize 					- 残差块大小 [4.8.16.32]

** params@ out  
**		residual 					- 残差块指针	 
*/
void Rk_IntraPred::RK_IntraCalcuResidual(uint8_t*	org, 
				uint8_t *pred, 
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



