#include "CABAC.h"
#include <string.h>

uint64_t g_tu_bits[2][5][256] = {0};
unsigned int pu_luma_dir_x265[5][256][35]={0};


CABAC_RDO *g_cabac_rdo_test = new CABAC_RDO;
int g_intra_cu_best_mode[RDO_MAX_DEPTH][256] = {0};//1表示采用当前深度下一层的intra CU
uint64_t g_SplitFlag_bit[RDO_MAX_DEPTH] = {0};


//TU
uint64_t g_intra_est_bit_last_sig_coeff_xy[3][RDO_MAX_DEPTH][256] = {0};
uint64_t g_intra_est_bit_coded_sub_block_flag[3][RDO_MAX_DEPTH][256] = {0};
uint64_t g_intra_est_sig_coeff_flag[3][RDO_MAX_DEPTH][256] = {0};
uint64_t g_intra_est_bit_coeff_abs_level_greater1_flag[3][RDO_MAX_DEPTH][256] = {0};
uint64_t g_intra_est_bit_coeff_abs_level_greater2_flag[3][RDO_MAX_DEPTH][256] = {0};
uint64_t g_intra_est_bit_coeff_sign_flag[3][RDO_MAX_DEPTH][256] = {0};
uint64_t g_intra_est_bit_coeff_abs_level_remaining[3][RDO_MAX_DEPTH][256] = {0};
//CBF
uint64_t g_intra_est_bit_cbf[3][RDO_MAX_DEPTH][256] = {0};

//PU
uint64_t g_intra_est_bit_luma_pred_mode_all_case[RDO_MAX_DEPTH][256][35] = {0};
uint64_t g_intra_est_bit_luma_pred_mode[RDO_MAX_DEPTH][256] = {0};
uint64_t g_intra_est_bit_chroma_pred_mode[RDO_MAX_DEPTH][256] = {0};
uint64_t g_intra_est_bit_part_mode[RDO_MAX_DEPTH][256] = {0};

//TU
uint64_t g_est_bit_tu[RDO_CU_MODE][RDO_MAX_DEPTH][256]= {0}; //第一维0表示INTER 1表示INTRA 2表示MERGE
uint64_t g_est_bit_tu_luma_NoCbf[RDO_CU_MODE][RDO_MAX_DEPTH][256]= {0};
uint64_t g_est_bit_tu_cb_NoCbf[RDO_CU_MODE][RDO_MAX_DEPTH][256]= {0};
uint64_t g_est_bit_tu_cr_NoCbf[RDO_CU_MODE][RDO_MAX_DEPTH][256]= {0};
//cu_delta_qp
int g_cu_qp_delta_abs[RDO_CU_MODE][RDO_MAX_DEPTH][256] = {0};

//CU
uint64_t g_est_bit_cu[RDO_CU_MODE][RDO_MAX_DEPTH][256]= {0};

uint64_t g_est_bit_cu_split_flag[RDO_CU_MODE][RDO_MAX_DEPTH][256][2]= {0};

#pragma region
const int CABAC_g_entropyBits[128] =
{
	// Corrected table, most notably for last state
	0x07b23, 0x085f9, 0x074a0, 0x08cbc, 0x06ee4, 0x09354, 0x067f4, 0x09c1b, 0x060b0, 0x0a62a, 0x05a9c, 0x0af5b, 0x0548d, 0x0b955, 0x04f56, 0x0c2a9,
	0x04a87, 0x0cbf7, 0x045d6, 0x0d5c3, 0x04144, 0x0e01b, 0x03d88, 0x0e937, 0x039e0, 0x0f2cd, 0x03663, 0x0fc9e, 0x03347, 0x10600, 0x03050, 0x10f95,
	0x02d4d, 0x11a02, 0x02ad3, 0x12333, 0x0286e, 0x12cad, 0x02604, 0x136df, 0x02425, 0x13f48, 0x021f4, 0x149c4, 0x0203e, 0x1527b, 0x01e4d, 0x15d00,
	0x01c99, 0x166de, 0x01b18, 0x17017, 0x019a5, 0x17988, 0x01841, 0x18327, 0x016df, 0x18d50, 0x015d9, 0x19547, 0x0147c, 0x1a083, 0x0138e, 0x1a8a3,
	0x01251, 0x1b418, 0x01166, 0x1bd27, 0x01068, 0x1c77b, 0x00f7f, 0x1d18e, 0x00eda, 0x1d91a, 0x00e19, 0x1e254, 0x00d4f, 0x1ec9a, 0x00c90, 0x1f6e0,
	0x00c01, 0x1fef8, 0x00b5f, 0x208b1, 0x00ab6, 0x21362, 0x00a15, 0x21e46, 0x00988, 0x2285d, 0x00934, 0x22ea8, 0x008a8, 0x239b2, 0x0081d, 0x24577,
	0x007c9, 0x24ce6, 0x00763, 0x25663, 0x00710, 0x25e8f, 0x006a0, 0x26a26, 0x00672, 0x26f23, 0x005e8, 0x27ef8, 0x005ba, 0x284b5, 0x0055e, 0x29057,
	0x0050c, 0x29bab, 0x004c1, 0x2a674, 0x004a7, 0x2aa5e, 0x0046f, 0x2b32f, 0x0041f, 0x2c0ad, 0x003e7, 0x2ca8d, 0x003ba, 0x2d323, 0x0010c, 0x3bfbb
};
const UChar aucNextState_0[128] =
{
	2        ,     0     ,     4      ,     1     ,       6     ,       3    ,       8    ,      5   ,      10  ,       5   ,      12   ,       9   ,     14    ,      9    ,     16    ,       11   ,
	18      ,     13    ,    20     ,    15    ,     22     ,      17   ,     24     ,   19    ,    26    ,    19    ,    28     ,    23     ,   30     ,   23      ,   32      ,    25   	 ,
	34      ,     27    ,    36     ,    27    ,     38     ,      31   ,     40     ,   31    ,    42    ,    33    ,    44     ,    33     ,   46     ,   37      ,   48      ,    37   	 ,
	50      ,     39    ,    52     ,    39    ,     54     ,      43   ,     56     ,   43    ,    58    ,    45    ,    60     ,    45     ,   62     ,   47      ,   64      ,    49   	 ,
	66      ,     49    ,    68     ,    51    ,     70     ,      53   ,     72     ,   53    ,    74    ,    55    ,    76     ,    55     ,   78     ,   57      ,   80      ,    59   	 ,
	82      ,     59    ,    84     ,    61    ,     86     ,      61   ,     88     ,   61    ,    90    ,    63    ,    92     ,    65     ,   94     ,   65      ,   96      ,    67   	 ,
	98      ,     67    ,   100    ,     67   ,     102    ,     69    ,    104    ,    69   ,    106  ,      71  ,     108   ,      71  ,     110  ,       71 ,      112  ,       73  ,
	114    ,    73     ,  116     ,    73    ,   118      ,   75      ,  120      ,  75     ,  122    ,    75    ,  124      ,   77     ,  124     ,    77     ,   126    ,    127
};
const int CABAC_g_entropyBits_0[128] = 
{
	0x07b23, 0x085f9, 0x074a0, 0x08cbc, 0x06ee4, 0x09354, 0x067f4, 0x09c1b, 0x060b0, 0x0a62a, 0x05a9c, 0x0af5b, 0x0548d, 0x0b955, 0x04f56, 0x0c2a9,
	0x04a87, 0x0cbf7, 0x045d6, 0x0d5c3, 0x04144, 0x0e01b, 0x03d88, 0x0e937, 0x039e0, 0x0f2cd, 0x03663, 0x0fc9e, 0x03347, 0x10600, 0x03050, 0x10f95,
	0x02d4d, 0x11a02, 0x02ad3, 0x12333, 0x0286e, 0x12cad, 0x02604, 0x136df, 0x02425, 0x13f48, 0x021f4, 0x149c4, 0x0203e, 0x1527b, 0x01e4d, 0x15d00,
	0x01c99, 0x166de, 0x01b18, 0x17017, 0x019a5, 0x17988, 0x01841, 0x18327, 0x016df, 0x18d50, 0x015d9, 0x19547, 0x0147c, 0x1a083, 0x0138e, 0x1a8a3,
	0x01251, 0x1b418, 0x01166, 0x1bd27, 0x01068, 0x1c77b, 0x00f7f, 0x1d18e, 0x00eda, 0x1d91a, 0x00e19, 0x1e254, 0x00d4f, 0x1ec9a, 0x00c90, 0x1f6e0,
	0x00c01, 0x1fef8, 0x00b5f, 0x208b1, 0x00ab6, 0x21362, 0x00a15, 0x21e46, 0x00988, 0x2285d, 0x00934, 0x22ea8, 0x008a8, 0x239b2, 0x0081d, 0x24577,
	0x007c9, 0x24ce6, 0x00763, 0x25663, 0x00710, 0x25e8f, 0x006a0, 0x26a26, 0x00672, 0x26f23, 0x005e8, 0x27ef8, 0x005ba, 0x284b5, 0x0055e, 0x29057,
	0x0050c, 0x29bab, 0x004c1, 0x2a674, 0x004a7, 0x2aa5e, 0x0046f, 0x2b32f, 0x0041f, 0x2c0ad, 0x003e7, 0x2ca8d, 0x003ba, 0x2d323, 0x0010c, 0x3bfbb
};

const UChar aucNextState_1[128] =
{
	1     ,      3       ,     0     ,      5      ,     2      ,     7     ,     4     ,     9        ,     4     ,     11     ,     8     ,    13      ,    8      ,    15     ,   10     ,    17  	  ,
	12   ,      19      ,    14    ,     21     ,     16    ,    23     ,    18    ,     25      ,     18   ,      27    ,     22    ,     29     ,    22    ,     31    ,     24   ,      33  	  ,
	26   ,      35      ,    26    ,     37     ,     30    ,    39     ,    30    ,     41      ,     32   ,      43    ,     32    ,     45     ,    36    ,     47    ,     36   ,      49  	  ,
	38   ,      51      ,    38    ,     53     ,     42    ,    55     ,    42    ,     57      ,     44   ,      59    ,     44    ,     61     ,    46    ,     63    ,     48   ,      65  	  ,
	48   ,      67      ,    50    ,     69     ,     52    ,    71     ,    52    ,     73      ,     54   ,      75    ,     54    ,     77     ,    56    ,     79    ,     58   ,      81  	  ,
	58   ,      83      ,    60    ,     85     ,     60    ,    87     ,    60    ,     89      ,     62   ,      91    ,     64    ,     93     ,    64    ,     95    ,     66   ,      97  	  ,
	66   ,      99      ,    66    ,    101    ,     68     ,   103   ,     68   ,    105      ,   70     ,   107     ,    70     ,   109     ,    70    ,    111   ,     72   ,    113  	 ,
	72   ,     115     ,    72    ,    117    ,     74     ,   119   ,     74    ,    121     ,    74    ,    123    ,    76     ,   125     ,    76    ,    125    ,    126  ,      127
};
const int CABAC_g_entropyBits_1[128] = 
{
	34297     ,   31523  ,   36028    ,   29856 ,    37716  ,    28388 ,     39963  ,    26612 ,     42538  ,    24752 ,     44891  ,    23196  ,    47445  ,    21645  ,   49833     ,   20310	  ,
	52215     ,   19079  ,   54723    ,  17878  ,   57371   ,   16708  ,    59703   ,   15752  ,    62157   ,   14816  ,    64670   ,   13923   ,   67072   ,   13127   ,   69525     ,   12368	  ,
	72194     ,   11597  ,   74547    ,  10963  ,   76973   ,   10350  ,    79583   ,    9732   ,   81736    ,   9253    ,   84420   ,    8692    , 86651     ,   8254     ,  89344      ,  7757  	  ,
	91870     ,    7321   ,  94231     ,  6936    , 96648    ,   6565    ,  99111     ,   6209    , 101712    ,   5855    ,   103751  ,     5593   ,  106627  ,     5244   ,    108707  ,    5006 	 ,
	111640   ,    4689   , 113959    ,   4454   , 116603   ,    4200   ,  119182   ,    3967   ,  121114   ,    3802   ,   123476  ,     3609   ,  126106  ,     3407   ,    128736  ,    3216	  ,
	130808   ,    3073   , 133297    ,   2911   , 136034   ,    2742   ,  138822   ,    2581   ,  141405   ,    2440   ,   143016  ,     2356   ,  145842  ,     2216   ,    148855  ,    2077	  ,
	150758   ,    1993   , 153187    ,   1891   , 155279   ,    1808   ,  158246   ,    1696   ,  159523   ,    1650   ,   163576  ,     1512   ,  165045  ,     1466   ,    168023  ,    1374	  ,
	170923   ,    1292   , 173684    ,   1217   , 174686   ,    1191    , 176943   ,    1135   ,  180397   ,    1055   ,   182925  ,      999    ,  185123  ,      954    ,     245691 ,       268  
};

const UChar aucNextState_00[128] =
{
	+4        ,    +2     ,    +6        ,     +0      ,    +8      ,    +1       ,      +10     ,     +3   ,      +12   ,       +3    ,     +14   ,       +5    ,      +16  ,        +5    ,      +18   ,       +9  ,
	+20      ,      +9    ,     +22    ,    +11     ,   +24      ,   +13      ,      +26     ,    +15   ,     +28    ,     +15    ,    +30   ,     +19     ,    +32    ,     +19     ,    +34     ,    +19    ,
	+36      ,     +23   ,     +38     ,    +23     ,    +40    ,     +25    ,     +42      ,  +25    ,    +44      ,   +27      ,  +46      ,   +27      ,  +48      ,   +31       , +50        ,+31  		 ,
	+52      ,     +31   ,     +54     ,    +31     ,    +56    ,     +33    ,     +58      ,  +33    ,    +60      ,   +37      ,  +62      ,   +37      ,  +64      ,   +37       , +66        ,+39   	 ,
	+68      ,     +39   ,     +70     ,    +39     ,    +72    ,     +43    ,     +74      ,  +43    ,    +76      ,   +43      ,  +78      ,   +43      ,  +80      ,   +45       , +82        ,+45  		 ,
	+84      ,     +45   ,     +86     ,    +47     ,    +88    ,     +47    ,      +90     ,   +47   ,     +92     ,    +49     ,   +94     ,    +49     ,   +96     ,    +49      ,  +98       , +51   	 ,
	+100    ,    +51    ,   +102     ,    +51     ,  +104     ,    +53     ,   +106      ,  +53    ,   +108     ,    +53     ,  +110    ,    +53      ,  +112    ,     +53     ,   +114    ,    +55    ,
	+116    ,    +55    ,   +118     ,    +55     ,  +120     ,    +55     ,   +122      ,  +55    ,   +124     ,    +55     ,  +124    ,    +57      ,  +124    ,     +57     ,   +126    ,   +127  
};


const int CABAC_g_entropyBits_00[128] = 
{
	+61379 ,    +65820   ,   +58244  ,    +70325    ,  +55000  ,   +73744    ,  +51364    ,  +77679     , +47948    ,  +80254     ,    +44841     , +87429       , +41955  ,    +89983   ,   +39389  ,    +94724   ,
	+36957 ,    +99660   ,   +34586  ,   +104556   ,   +32460  ,   +109586  ,    +30568  ,   +114426  ,    +28739 ,    +116880 ,     +27050 ,    +124373  ,    +25495  ,  +126775    ,  +23965  ,  +131682	  ,
	+22560 ,   +136864  ,    +21313 ,   +139217    ,  +20082  ,  +146498   ,   +18985   ,  +149108   ,   +17945  ,   +153930  ,    +16946  ,  +156614     , +16011     , +163624    ,  +15078  ,  +166317	 ,
	+14257 ,   +171453  ,    +13501 ,   +173814    ,  +12774  ,  +181068   ,   +12064   ,  +183531   ,   +11448  ,   +188363  ,    +10837  ,  +190402     , +10250     ,   +195971  ,     +9695 ,   +200577	  ,
	+9143   , +203510    ,   +8654    , +208190     ,  +8167    ,+213251     ,  +7769      ,  +215830   ,    +7411   ,  +220225   ,    +7016    , +222587     ,  +6623      ,  +227818   ,    +6289  ,  +232487    ,
	+5984   , +234559    ,   +5653    , +239924     ,  +5323    ,+242661     ,  +5021      ,  +245449   ,    +4796   ,  +250112   ,    +4572    , +254656     ,  +4293      ,  +257482   ,    +4070  ,  +262814    ,
	+3884   , +264717    ,   +3699    , +267146     ,  +3504    ,+271882     ,  +3346      ,  +274849   ,    +3162   ,  +278705   ,    +2978    , +282758     ,  +2840      ,  +284227   ,    +2666  ,  +289137    ,
	+2509   , +292037    ,   +2408    , +294798     ,  +2326    ,+298162     ,  +2190      ,  +300419   ,    +2054   ,  +303873   ,    +1953    , +309031     ,  +1908      ,  +311229   ,     +536   , +491382   
};


const UChar aucNextState_01[128] =
{
	+0      ,    +1       ,   +2       ,   +3       ,   +4        ,  +5         , +4        ,  +7         , +8        ,  +7          ,  +8       ,  +11      ,   +10      ,  +11       ,  +12      ,   +13     	 ,
	+14    ,     +15    ,    +16     ,    +17    ,     +18    ,     +19    ,    +18    ,     +21    ,     +22  ,       +21   ,     +22   ,      +25  ,       +24  ,      +25   ,      +26  ,       +27 	 ,
	+26    ,     +29    ,    +30     ,    +29    ,     +30    ,     +33    ,    +32    ,     +33    ,     +32  ,       +35   ,     +36   ,      +35  ,       +36  ,      +39   ,      +38  ,       +39   ,
	+38    ,     +41    ,    +42     ,    +41    ,     +42    ,     +45    ,    +44    ,     +45    ,     +44  ,       +47   ,     +46   ,      +47  ,       +48  ,      +49   ,      +48  ,       +51   ,
	+50    ,     +51    ,    +52     ,    +53    ,     +52    ,     +55    ,    +54    ,     +55    ,     +54  ,       +57   ,     +56   ,      +57  ,       +58  ,      +59   ,      +58  ,       +61   ,
	+60    ,     +61    ,    +60     ,    +63    ,     +60    ,     +63    ,    +62    ,     +63    ,     +64  ,       +65   ,     +64   ,      +67  ,       +66  ,      +67   ,      +66  ,       +69 	 ,
	+66    ,     +69    ,    +68     ,    +69    ,     +68    ,     +71    ,    +70    ,     +71    ,     +70  ,       +73   ,     +70   ,      +73  ,       +72  ,      +73   ,      +72  ,       +75   ,
	+72    ,     +75    ,    +74     ,    +75    ,     +74    ,     +77    ,    +74    ,     +77    ,     +76  ,       +77   ,     +76   ,      +79  ,       +76  ,      +79   ,     +126 ,       +127
};


const int CABAC_g_entropyBits_01[128] = 
{
	+67551    ,  +68594    ,  +67572   ,  +67551    ,  +68351     ,+67572      ,+69150     , +68351        , +69643     , +70926     , +70641    ,  +69643     , +71478       ,   +72197    ,  +72525   ,   +73029  	,
	+73802    ,  +73860    ,  +75249   ,  +75033    ,  +76411     ,+76450      ,+77909     , +77581        , +79486     , +80035     , +80995    ,  +80422     , +82652       ,   +82824    ,  +84562   ,   +84341  	,
	+86144    ,  +86117    ,  +87936   ,  +88470    ,  +89933     ,+89341      ,+91468     , +91951        , +93673     , +93333     , +95343    ,  +96017     , +97598       ,   +97001    ,  +99627   ,   +99694  	,
	+101552  ,   +101602 ,   +103584 ,   +103963 ,    +105676 ,   +105340 ,   +107921 ,    +107803   ,  +109606  ,   +109966 ,   +112220 ,    +112005 ,    +113951  ,   +114384  ,  +116646  ,   +116028 	,
	+118648  ,   +118961 ,   +121057 ,   +120895 ,    +123382 ,   +123168 ,   +125081 ,    +125747   ,  +127278  ,   +127323 ,   +129715 ,    +129685 ,    +132143  ,   +131961  ,  +134024  ,   +134329 	,
	+136370  ,   +136401 ,   +138945 ,   +138541 ,    +141564 ,   +141278 ,   +143986 ,    +144066   ,  +145456  ,   +146411 ,   +148198 ,    +147705 ,    +151071  ,   +150531  ,  +152835  ,   +153309 	,
	+155180  ,   +155212 ,   +157170 ,   +157641 ,    +160054 ,   +159479 ,   +161219 ,    +162446   ,  +165226  ,   +163490 ,   +166557 ,    +167543 ,    +169489  ,   +169012  ,  +172297  ,   +171825 	,
	+174976  ,   +174725 ,   +175903 ,   +177486 ,    +178134 ,   +178295 ,   +181532 ,    +180552   ,  +183980  ,   +184006 ,   +186122 ,    +186332 ,    +186077  ,   +188530  ,  +245959  ,   +245959 
};

const UChar aucNextState_10[128] =
{
	0      ,    1      ,   2      ,  3         , 4        , 5        , 6           ,       5     ,    6      ,    9      ,   10    ,    9     ,   10     ,    11       ,  12     ,   13  ,
	14    ,     15    ,     16   ,    17    ,     18  ,      19  ,       20   ,      19    ,    20    ,     23   ,     24   ,     23  ,      24  ,       25    ,     26  ,      27   ,
	28    ,     27    ,     28   ,    31    ,     32  ,      31  ,       32   ,      33    ,    34    ,     33   ,     34   ,     37  ,      38  ,       37    ,     38  ,      39   ,
	40    ,     39    ,     40   ,    43    ,     44  ,      43  ,       44   ,      45    ,    46    ,     45   ,     46   ,     47  ,      48  ,       49    ,     50  ,      49   ,
	50    ,     51    ,     52   ,    53    ,     54  ,      53  ,       54   ,      55    ,    56    ,     55   ,     56   ,     57  ,      58  ,       59    ,     60  ,      59 	 ,
	60    ,     61    ,     62   ,    61    ,     62  ,      61  ,       62   ,      63    ,    64    ,     65   ,     66   ,     65  ,      66  ,       67    ,     68  ,      67   ,
	68    ,     67    ,     68   ,    69    ,     70  ,      69  ,       70   ,      71    ,    72    ,     71   ,     72   ,     71  ,      72  ,       73    ,     74  ,      73   ,
	74    ,     73    ,     74   ,    75    ,     76  ,      75  ,       76   ,      75    ,    76    ,     77   ,     78   ,     77  ,      78  ,       77    ,    126 ,      127	
};


const int CABAC_g_entropyBits_10[128] = 
{
	68594    ,  67551      ,  67551   ,  67572   ,    67572  ,   68351  ,   68351   ,    69150  ,   70926   ,   69643     ,   69643  ,   70641   ,    72197   ,     71478 ,   73029  ,  72525  ,
	73860    ,  73802      ,  75033   ,  75249   ,    76450  ,   76411  ,   77581   ,    77909  ,   80035   ,   79486     ,   80422  ,   80995   ,    82824   ,     82652 ,   84341  ,  84562  ,
	86117    ,  86144      ,  88470   ,  87936   ,    89341  ,   89933  ,   91951   ,    91468  ,   93333   ,   93673     ,   96017  ,   95343   ,    97001   ,     97598 ,   99694  ,  99627  ,
	101602  ,   101552   , 103963   , 103584  ,  105340  ,  105676  ,   107803 ,   107921 ,   109966 ,    109606  ,  112005  ,  112220  ,   114384  ,   113951  ,  116028 ,  116646,
	118961  ,   118648   , 120895   , 121057  ,  123168  ,  123382  ,   125747 ,   125081 ,   127323 ,    127278  ,  129685  ,  129715  ,   131961  ,   132143  ,  134329 ,  134024,
	136401  ,   136370   , 138541   , 138945  ,  141278  ,  141564  ,   144066 ,   143986 ,   146411 ,    145456  ,  147705  ,  148198  ,   150531  ,   151071  ,  153309 ,  152835,
	155212  ,   155180   , 157641   , 157170  ,  159479  ,  160054  ,   162446 ,   161219 ,   163490 ,    165226  ,  167543  ,  166557  ,   169012  ,   169489  ,  171825 ,  172297,
	174725  ,   174976   , 177486   , 175903  ,  178295  ,  178134  ,   180552 ,   181532 ,   184006 ,    183980  ,  186332  ,  186122  ,   188530  ,   186077  ,  245959 ,  245959 
};

const UChar aucNextState_11[128] =
{
	+3     ,     +5       ,   +1       ,  +7        ,  +0         ,    +9      ,    +2       , +11        ,  +2        ,+13        ,+4           , +15       ,  +4      ,   +17      ,   +8      ,  +19   	   ,
	+8     ,    +21      ,  +10       , +23      ,  +12        ,    +25    ,     +14     ,    +27     ,    +14    ,    +29    ,   +18      ,  +31      ,  +18    ,     +33    ,    +18    ,    +35  	,
	+22   ,      +37     ,    +22    ,    +39    ,     +24    ,    +41    ,    +24      ,   +43      ,   +26     ,   +45     ,  +26       ,   +47     ,  +30     ,    +49     ,    +30   ,     +51     ,
	+30   ,      +53     ,    +30    ,    +55    ,     +32    ,    +57    ,    +32      ,   +59      ,   +36     ,   +61     ,  +36       ,   +63     ,  +36     ,    +65     ,    +38   ,     +67     ,
	+38   ,      +69     ,    +38    ,    +71    ,     +42    ,    +73    ,    +42      ,   +75      ,   +42     ,   +77     ,  +42       ,   +79     ,  +44     ,    +81     ,    +44   ,     +83     ,
	+44   ,      +85     ,    +46    ,    +87    ,     +46    ,    +89    ,    +46      ,   +91      ,   +48     ,   +93     ,  +48       ,   +95     ,  +48     ,    +97     ,    +50   ,     +99     ,
	+50   ,     +101    ,     +50   ,    +103  ,      +52    ,    +105  ,       +52   ,     +107  ,       +52 ,      +109,       +52   ,     +111 ,    +52  ,      +113 ,       +54 ,      +115  ,
	+54   ,     +117    ,     +54   ,    +119  ,      +54    ,    +121  ,       +54   ,     +123  ,       +54 ,      +125,       +56   ,     +125 ,    +56  ,      +125 ,      +126,       +127
};

const int CABAC_g_entropyBits_11[128] = 
{
	+65820    ,  +61379   ,  +70325    , +58244    ,  +73744     , +55000   ,   +77679    ,  +51364    ,  +80254      ,  +47948  ,    +87429    , +44841   ,   +89983     ,   +41955   ,   +94724    ,  +39389    ,
	+99660    ,  +36957   , +104556   ,  +34586   ,  +109586   ,  +32460   ,  +114426   ,   +30568   ,  +116880    ,  +28739  ,   +124373   ,  +27050  ,   +126775   ,   +25495    ,   +131682  ,    +23965  ,
	+136864  ,    +22560 ,   +139217 ,   +21313   ,  +146498   ,   +20082 ,    +149108 ,     +18985 ,    +153930  ,   +17945 ,    +156614   ,   +16946 ,    +163624  ,    +16011  ,   +166317  ,    +15078	,
	+171453  ,    +14257 ,   +173814 ,   +13501   ,  +181068   ,   +12774 ,    +183531 ,     +12064 ,    +188363  ,   +11448 ,    +190402   ,   +10837 ,    +195971  ,    +10250  ,   +200577  ,     +9695   ,
	+203510  ,     +9143  ,  +208190  ,    +8654   ,  +213251   ,    +8167  ,   +215830  ,     +7769   ,  +220225    ,   +7411   ,  +222587    ,  +7016    , +227818     ,    +6623    ,   +232487  ,     +6289   ,
	+234559  ,     +5984  ,  +239924  ,    +5653   ,  +242661   ,    +5323  ,   +245449  ,     +5021   ,  +250112    ,   +4796   ,  +254656    ,  +4572    , +257482     ,   +4293     ,   +262814  ,     +4070   ,
	+264717  ,     +3884  ,  +267146  ,    +3699   ,  +271882   ,    +3504  ,   +274849  ,     +3346   ,  +278705    ,   +3162   ,  +282758    ,  +2978    , +284227     ,    +2840    ,   +289137  ,     +2666   ,
	+292037  ,     +2509  ,  +294798  ,    +2408   ,  +298162   ,    +2326  ,   +300419  ,     +2190   ,  +303873    ,   +2054   ,  +309031    ,  +1953    , +311229     ,    +1908    ,   +491382  ,      +536  
};


const UChar aucNextState_110[128] =
{
	1      ,    3       ,     0      ,    5     ,       2    ,      5     ,     4      ,    9     ,     4       ,   9      ,     6      ,   11      ,  6     ,    13     ,    10     ,   15  	 ,
	10    ,     17    ,    12      ,   19    ,     14    ,     19     ,    16     ,    23   ,      16    ,    23   ,     20     ,    25     ,   20  ,      27   ,      20   ,     27  	,
	24    ,     31    ,    24      ,   31    ,     26    ,     33     ,    26     ,    33   ,      28    ,    37   ,     28     ,    37     ,   32  ,      39   ,      32   ,     39  	,
	32    ,     43    ,    32      ,   43    ,     34    ,     45     ,    34     ,    45   ,      38    ,    47   ,     38     ,    49     ,   38  ,      49   ,      40   ,     51  	,
	40    ,     53    ,    40      ,   53    ,     44    ,     55     ,    44     ,    55   ,      44    ,    57   ,     44     ,    59     ,   46  ,      59   ,      46   ,     61  	,
	46    ,     61    ,    48      ,   61    ,     48    ,     63     ,    48     ,    65   ,      50    ,    65   ,     50     ,    67     ,   50  ,      67   ,      52   ,     67  	,
	52    ,     69    ,    52      ,   69    ,     54    ,     71     ,    54     ,    71   ,      54    ,    71   ,     54     ,    73     ,   54  ,      73   ,      56   ,     73  	,
	56    ,     75    ,    56      ,   75    ,     56    ,     75     ,    56      ,   77    ,     56     ,   77    ,     58     ,    77     ,   58  ,      77   ,     126  ,     127
};

const int CABAC_g_entropyBits_110[128] = 
{
	101848  ,   99095   ,   104622  ,   98207   ,  105267  ,   97538    , 107535    ,  96255     ,  110110   ,   95393   ,  115817    ,   94674   ,  118371   ,  94170    , 119476   ,  94112  	 ,
	124412  ,   94328   ,   127752  ,   94289   ,  131231  ,   94617    , 134736    ,  95238     ,  137190   ,   95811   ,  142251    ,   96575   ,  144653   ,  97689    , 149560   ,  98512  	 ,
	152616  ,   99533   ,   154969  ,  100896  ,  161314  ,  101818    , 163924    , 103405    , 167853    ,   104596 ,    170537  ,   106290 ,   175992  ,  107881   , 178685   , 109309 	,
	183821  ,  110905  ,   186182  ,  112612   ,  192665  ,   114486  ,   195128  ,   115815  ,   198713  ,   118075 ,    200752  ,   119544  ,   206321  ,   121890 ,   210309 ,   123654 ,
	213242  ,  125746  ,   217922  ,  127836   ,  221943  ,   129281  ,   224522  ,   131245  ,   228917  ,   133517 ,    231279  ,   135752  ,   236072  ,   137431 ,   240741 ,   139586 ,
	242813  ,  142018  ,   247681  ,  144475   ,  250418  ,   146728  ,   253206  ,   148037  ,   257433  ,   150638 ,    261977  ,   153427  ,   264803  ,   155051 ,   269750 ,   157257 ,
	271653  ,  159163  ,   274082  ,  161945   ,  278447  ,   163027  ,   281414  ,   166922  ,   285270  ,   168207 ,    289323  ,   171001  ,   290792  ,   173763 ,   295346 ,   176350 ,
	298246  ,  177195  ,   301007  ,  179351   ,  304371  ,   182723  ,   306628  ,   185115  ,   310082  ,   187177 ,    314886  ,   187076  ,   317084  ,   187031 ,   491650 ,   246227 
};

const UChar aucNextState_111[128] =
{
	5     ,       7      ,    3      ,    9      ,     1      ,    11     ,     0     ,     13     ,    0      ,    15       ,    2      ,    17      ,    2      ,    19      ,    4      ,    21  	   ,
	4     ,      23     ,    8      ,   25      ,      8     ,     27    ,     10   ,      29    ,     10   ,      31     ,     14   ,      33    ,      14   ,      35    ,     14   ,        37  	  ,
	18   ,      39    ,     18   ,     41     ,     18    ,    43     ,    18    ,    45       ,    22    ,     47      ,    22    ,     49     ,     24   ,     51     ,    24     ,     53  		,
	24   ,      55    ,     24   ,     57     ,     26    ,    59     ,    26    ,    61       ,    30    ,     63      ,    30    ,     65     ,     30   ,     67     ,    30     ,     69  		,
	30   ,      71    ,     30   ,     73     ,     32    ,    75     ,    32    ,    77       ,    32    ,     79      ,    32    ,     81     ,     36   ,     83     ,    36     ,     85  		,
	36   ,      87    ,     36   ,     89     ,     36    ,    91     ,    36    ,    93       ,    38    ,     95      ,    38    ,     97     ,     38   ,     99     ,    38     ,     101 	   ,
	38   ,     103   ,      38  ,     105    ,     42    ,    107   ,      42  ,      109   ,      42  ,      111   ,      42  ,      113  ,      42   ,     115   ,      42  ,      117 	 ,
	42   ,     119   ,      42  ,     121    ,     42    ,    123   ,      42  ,      125   ,      42  ,      125   ,      44  ,      125  ,      44   ,     125   ,     126  ,      127
};

const int CABAC_g_entropyBits_111[128] = 
{
	95676     ,   89767  ,   101848   ,   84856  ,  108041   ,  79752    ,   113707  ,    74560  ,   116282  ,    69593  ,   125145   ,   65151   ,   127699  ,   61034   , 137262     ,   57267  	,
	142198   ,   53665  ,   149447   ,   50338   ,  157031   ,   47276  ,   164259  ,    44491  ,   166713  ,    41866  ,   179096   ,   39418   ,  181498    ,   37092  ,  186405    ,    34928   ,
	196567   ,   32910  ,   198920   ,   31045   ,  208655   ,   29335  ,   211265  ,    27677  ,   218600  ,    26199  ,   221284   ,   24703   ,  233149    ,   23332  ,  235842    ,    22014   ,
	240978   ,   20822  ,   243339   ,   19710   ,  253262   ,   18629  ,   255725  ,    17657  ,   265336  ,    16692  ,   267375   ,   15843   ,  272944    ,   14939  ,  280160    ,    14149   ,
	283093   ,   13343  ,   287773   ,   12621   ,  297671   ,   11969  ,   300250  ,    11378  ,   304645  ,    10818  ,   307007   ,   10232   ,  314469    ,   9696    ,  319138    ,     9200	 ,
	321210   ,    8726   ,  329268    ,   8234    , 332005    ,  7763     ,  334793   ,   7377     ,   341982  ,     7012   ,   346526   ,    6649    ,   349352  ,    6286    ,   357045  ,    5961  	 ,
	358948   ,    5692   ,  361377    ,   5395    , 368530    ,  5154     ,  371497   ,   4858     ,   375353  ,     4628   ,   379406   ,    4352    ,   380875  ,    4132    ,   388248  ,    3883  	 ,
	391148   ,    3700   ,  393909    ,   3543    , 397273    ,  3381     ,  399530   ,   3189     ,   402984  ,     3008   ,   410743   ,    2907    ,   412941  ,    2862    ,   737073  ,     804  
};

const int cu_ZscanToXpixel[64]=
{
	+0      ,    +8       ,  +0       ,   +8          ,    +16     ,    +24     ,    +16    ,     +24,
	+0      ,    +8       ,  +0       ,   +8          ,    +16     ,    +24     ,    +16    ,     +24,
	+32    ,     +40     ,    +32   ,      +40     ,    +48     ,    +56     ,    +48    ,     +56,
	+32    ,     +40     ,    +32   ,      +40     ,    +48     ,    +56     ,    +48    ,     +56,
	+0      ,    +8       ,  +0       ,   +8          ,    +16     ,    +24     ,    +16    ,     +24,
	+0      ,    +8       ,  +0       ,   +8          ,    +16     ,    +24     ,    +16    ,     +24,
	+32    ,     +40     ,    +32   ,      +40     ,    +48     ,    +56     ,    +48    ,     +56,
	+32    ,     +40     ,    +32   ,      +40     ,    +48     ,    +56     ,    +48    ,     +56,
};

const int cu_ZscanToYpixel[64]=
{
	0      ,    0        ,    8     ,     8     ,      0    ,       0    ,     8       ,    8  	 ,
	16    ,     16     ,   24     ,    24    ,     16   ,      16    ,     24     ,    24  ,
	0      ,    0        ,    8     ,     8     ,      0    ,       0    ,     8       ,    8  	 ,
	16    ,     16     ,   24     ,    24    ,     16   ,      16    ,     24     ,    24  ,
	32    ,     32     ,   40     ,    40    ,     32   ,      32    ,     40     ,    40  ,
	48    ,     48     ,   56     ,    56    ,     48   ,      48    ,     56     ,    56  ,
	32    ,     32     ,   40     ,    40    ,     32   ,      32    ,     40     ,    40  ,
	48    ,     48     ,   56     ,    56    ,     48   ,      48    ,     56     ,    56  ,
};

Int cu_ZscanToRaster[64];
Int tu_ZscanToRaster[256];
UInt zsantoraster_depth_4[256] =
{		   0,	   1,	  16,	  17,	   2,	   3,	  18,	  19,	  32,	  33,	  48,	  49,	  34,	  35,	  50,	  51,
4,	   5,	  20,	  21,	   6,	   7,	  22,	  23,	  36,	  37,	  52,	  53,	  38,	  39,	  54,	  55,
64,	  65,	  80,	  81,	  66,	  67,	  82,	  83,	  96,	  97,	 112,	 113,	  98,	  99,	 114,	 115,
68,	  69,	  84,	  85,	  70,	  71,	  86,	  87,	 100,	 101,	 116,	 117,	 102,	 103,	 118,	 119,
8,	   9,	  24,	  25,	  10,	  11,	  26,	  27,	  40,	  41,	  56,	  57,	  42,	  43,	  58,	  59,
12,	  13,	  28,	  29,	  14,	  15,	  30,	  31,	  44,	  45,	  60,	  61,	  46,	  47,	  62,	  63,
72,	  73,	  88,	  89,	  74,	  75,	  90,	  91,	 104,	 105,	 120,	 121,	 106,	 107,	 122,	 123,
76,	  77,	  92,	  93,	  78,	  79,	  94,	  95,	 108,	 109,	 124,	 125,	 110,	 111,	 126,	 127,
128,	 129,	 144,	 145,	 130,	 131,	 146,	 147,	 160,	 161,	 176,	 177,	 162,	 163,	 178,	 179,
132,	 133,	 148,	 149,	 134,	 135,	 150,	 151,	 164,	 165,	 180,	 181,	 166,	 167,	 182,	 183,
192,	 193,	 208,	 209,	 194,	 195,	 210,	 211,	 224,	 225,	 240,	 241,	 226,	 227,	 242,	 243,
196,	 197,	 212,	 213,	 198,	 199,	 214,	 215,	 228,	 229,	 244,	 245,	 230,	 231,	 246,	 247,
136,	 137,	 152,	 153,	 138,	 139,	 154,	 155,	 168,	 169,	 184,	 185,	 170,	 171,	 186,	 187,
140,	 141,	 156,	 157,	 142,	 143,	 158,	 159,	 172,	 173,	 188,	 189,	 174,	 175,	 190,	 191,
200,	 201,	 216,	 217,	 202,	 203,	 218,	 219,	 232,	 233,	 248,	 249,	 234,	 235,	 250,	 251,
204,	 205,	 220,	 221,	 206,	 207,	 222,	 223,	 236,	 237,	 252,	 253,	 238,	 239,	 254,	 255,
};

UInt zsantoraster_depth_3[64]=
{
	0,1,8,9,2,3,10,11,16,17,24,25,18,19,26,27,4,5,12,13,6,7,14,15,20,21,28,29,22,23,30,31,
	32,33,40,41,34,35,42,43,48,49,56,57,50,51,58,59,36,37,44,45,38,39,46,47,52,53,60,61,54,55,62,63
};
UInt zsantoraster_depth_2[16]=
{
	0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15
};
UInt zsantoraster_depth_1[4]=
{
	0,1,2,3
};
UInt zsantoraster_depth_0[1]=
{
	0
};

#pragma endregion

//decision_est
void CABAC_RDO::hevc_enc_cabac_decision_unit_est(Decision_unit_input_est  decision_unit_input_est , Decision_unit_output_est   &decision_unit_output_est)
{
	int binval_in = decision_unit_input_est.binval_in;                     //[0]
	int valmps_in = decision_unit_input_est.valmps_in;                  //[0]
	int pstateidx_in = decision_unit_input_est.pstateidx_in;            //[5:0]
	unsigned int  est_bit_in = decision_unit_input_est.est_bit_in;

	int    StateChangeFlag	;//[0]
	int    valmpsmps      	;//[0]
	int    valmpslps         	;//[0]
	int    mpsFlag         	;//[0]
	int    pStateIdxoutmps;//[5:0]
	int    pStateIdxoutlps 	;//[5:0]


	//MPS概率索引转移
	if (pstateidx_in<62)
	{
		pStateIdxoutmps = (pstateidx_in & 0x3f) + 1;
	} 
	else
	{
		pStateIdxoutmps = pstateidx_in & 0x3f;
	}

	//LPS概率索引转移
	switch (pstateidx_in)
	{
	case 0 : pStateIdxoutlps = 0 ; break;
	case 1 : pStateIdxoutlps = 0 ; break;
	case 2 : pStateIdxoutlps = 1 ; break;
	case 3 : pStateIdxoutlps = 2 ; break;
	case 4 : pStateIdxoutlps = 2 ; break;
	case 5 : pStateIdxoutlps = 4 ; break;
	case 6 : pStateIdxoutlps = 4 ; break;
	case 7 : pStateIdxoutlps = 5 ; break;
	case 8 : pStateIdxoutlps = 6 ; break;
	case 9 : pStateIdxoutlps = 7 ; break;
	case 10 : pStateIdxoutlps = 8 ; break;
	case 11 : pStateIdxoutlps = 9 ; break;
	case 12 : pStateIdxoutlps = 9 ; break;
	case 13 : pStateIdxoutlps = 11 ; break;
	case 14 : pStateIdxoutlps = 11 ; break;
	case 15 : pStateIdxoutlps = 12 ; break;
	case 16 : pStateIdxoutlps = 13 ; break;
	case 17 : pStateIdxoutlps = 13 ; break;
	case 18 : pStateIdxoutlps = 15 ; break;
	case 19 : pStateIdxoutlps = 15 ; break;
	case 20 : pStateIdxoutlps = 16 ; break;
	case 21 : pStateIdxoutlps = 16 ; break;
	case 22 : pStateIdxoutlps = 18 ; break;
	case 23 : pStateIdxoutlps = 18 ; break;
	case 24 : pStateIdxoutlps = 19 ; break;
	case 25 : pStateIdxoutlps = 19 ; break;
	case 26 : pStateIdxoutlps = 21 ; break;
	case 27 : pStateIdxoutlps = 21 ; break;
	case 28 : pStateIdxoutlps = 22 ; break;
	case 29 : pStateIdxoutlps = 22 ; break;
	case 30 : pStateIdxoutlps = 23 ; break;
	case 31 : pStateIdxoutlps = 24 ; break;
	case 32 : pStateIdxoutlps = 24 ; break;
	case 33 : pStateIdxoutlps = 25 ; break;
	case 34 : pStateIdxoutlps = 26 ; break;
	case 35 : pStateIdxoutlps = 26 ; break;
	case 36 : pStateIdxoutlps = 27 ; break;
	case 37 : pStateIdxoutlps = 27 ; break;
	case 38 : pStateIdxoutlps = 28 ; break;
	case 39 : pStateIdxoutlps = 29 ; break;
	case 40 : pStateIdxoutlps = 29 ; break;
	case 41 : pStateIdxoutlps = 30 ; break;
	case 42 : pStateIdxoutlps = 30 ; break;
	case 43 : pStateIdxoutlps = 30 ; break;
	case 44 : pStateIdxoutlps = 31 ; break;
	case 45 : pStateIdxoutlps = 32 ; break;
	case 46 : pStateIdxoutlps = 32 ; break;
	case 47 : pStateIdxoutlps = 33 ; break;
	case 48 : pStateIdxoutlps = 33 ; break;
	case 49 : pStateIdxoutlps = 33 ; break;
	case 50 : pStateIdxoutlps = 34 ; break;
	case 51 : pStateIdxoutlps = 34 ; break;
	case 52 : pStateIdxoutlps = 35 ; break;
	case 53 : pStateIdxoutlps = 35 ; break;
	case 54 : pStateIdxoutlps = 35 ; break;
	case 55 : pStateIdxoutlps = 36 ; break;
	case 56 : pStateIdxoutlps = 36 ; break;
	case 57 : pStateIdxoutlps = 36 ; break;
	case 58 : pStateIdxoutlps = 37 ; break;
	case 59 : pStateIdxoutlps = 37 ; break;
	case 60 : pStateIdxoutlps = 37 ; break;
	case 61 : pStateIdxoutlps = 38 ; break;
	case 62 : pStateIdxoutlps = 38 ; break;

	default: pStateIdxoutlps = 63 ; break;
	}

	//准备MPS,LPS需要的相关数据
	valmpsmps = valmps_in;

	StateChangeFlag = pstateidx_in==0 ? 1: 0;
	valmpslps = StateChangeFlag ? !valmps_in : valmps_in;

	//概率不同的概率符号的出现选择相应的数据输出
	mpsFlag = binval_in == valmps_in;
	decision_unit_output_est.est_bit_out = CABAC_g_entropyBits[(pstateidx_in << 1)+ (binval_in^valmps_in)] + est_bit_in;
	switch (mpsFlag)
	{
	case 0:
		decision_unit_output_est.valmps_out = valmpslps;
		decision_unit_output_est.pstateidx_out = pStateIdxoutlps & 0x3f;
		break;

	case 1:
		decision_unit_output_est.valmps_out = valmpsmps;
		decision_unit_output_est.pstateidx_out = pStateIdxoutmps & 0x3f;
		break;

	default:
		break;
	}
}
void CABAC_RDO::hevc_cabac_decision_enc_est(Enc_input_est   enc_input_est,  Enc_output_est   &enc_output_est, 	int valbin_ctxInc_r[4]  , int ctx_offset)
{
	int val_len_r           = enc_input_est.val_len_r;               //[1:0]
	UChar tlb2ctl_din0                 = mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[0]].get_m_ucState();                    //[6:0]上下文 最低位保存的为MPS的值，高6为保存的为概率值
	UChar tlb2ctl_din1                 = mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[1]].get_m_ucState();                    //[6:0]
	UChar tlb2ctl_din2                 = mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[2]].get_m_ucState();                    //[6:0]
	UChar tlb2ctl_din3                 = mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[3]].get_m_ucState();                    //[6:0]
	int binval0                       = enc_input_est.binval0;                          //[0]
	int binval1                       = enc_input_est.binval1;                          //[0]
	int binval2                       = enc_input_est.binval2;                          //[0]
	int binval3                       = enc_input_est.binval3;                          //[0]
	unsigned int est_bit_in      = enc_input_est.est_bit_in;                       //


	int		pstateidx_in0					;//[5:0]
	int		valmps_in0						;//[0]
	int		binval_in0						;//[0]
	unsigned int		est_bit_in0		;

	int		pstateidx_out0				;//[5:0]
	int		valmps_out0					;//[0]
	unsigned int est_bit_out0			;


	int		pstateidx_in1					;//[5:0]
	int		valmps_in1						;//[0]
	int		binval_in1						;//[0]
	unsigned int		est_bit_in1		;


	int		pstateidx_out1				;//[5:0]
	int		valmps_out1					;//[0]
	unsigned int est_bit_out1			;


	int		pstateidx_in2					;//[5:0]
	int		valmps_in2						;//[0]
	int		binval_in2						;//[0]
	unsigned int		est_bit_in2		;


	int		pstateidx_out2				;//[5:0]
	int		valmps_out2					;//[0]
	unsigned int est_bit_out2			;


	int		pstateidx_in3					;//[5:0]
	int		valmps_in3						;//[0]
	int		binval_in3						;//[0]
	unsigned int		est_bit_in3		;


	int		pstateidx_out3				;//[5:0]
	int		valmps_out3					;//[0]
	unsigned int est_bit_out3			;


	//bin0
	binval_in0					=	binval0;    
	pstateidx_in0             = tlb2ctl_din0 >>1;
	valmps_in0                = tlb2ctl_din0 & 0x1;
	est_bit_in0                 = est_bit_in;

	Decision_unit_input_est   decision_unit_input0_est;
	Decision_unit_output_est   decision_unit_output0_est;

	decision_unit_input0_est.binval_in = binval_in0;
	decision_unit_input0_est.pstateidx_in = pstateidx_in0;
	decision_unit_input0_est.valmps_in = valmps_in0;
	decision_unit_input0_est.est_bit_in = est_bit_in0;

	hevc_enc_cabac_decision_unit_est(decision_unit_input0_est , decision_unit_output0_est);
	
	est_bit_out0 = decision_unit_output0_est.est_bit_out;
	pstateidx_out0 = decision_unit_output0_est.pstateidx_out;
	valmps_out0 = decision_unit_output0_est.valmps_out;

	tlb2ctl_din0 = (pstateidx_out0<<1) + valmps_out0;

	//bin1
	if (valbin_ctxInc_r[1] == valbin_ctxInc_r[0])
	{
		tlb2ctl_din1 = tlb2ctl_din0;
	}

	binval_in1					=	binval1;    
	pstateidx_in1             = tlb2ctl_din1 >>1;
	valmps_in1                = tlb2ctl_din1 & 0x1;
	est_bit_in1                 = est_bit_out0;

	Decision_unit_input_est   decision_unit_input1_est;
	Decision_unit_output_est   decision_unit_output1_est;

	decision_unit_input1_est.binval_in = binval_in1;
	decision_unit_input1_est.pstateidx_in = pstateidx_in1;
	decision_unit_input1_est.valmps_in = valmps_in1;
	decision_unit_input1_est.est_bit_in = est_bit_in1;

	hevc_enc_cabac_decision_unit_est(decision_unit_input1_est , decision_unit_output1_est);

	est_bit_out1 = decision_unit_output1_est.est_bit_out;
	pstateidx_out1 = decision_unit_output1_est.pstateidx_out;
	valmps_out1 = decision_unit_output1_est.valmps_out;

	tlb2ctl_din1 = (pstateidx_out1<<1) + valmps_out1;

	//bin2
	if (valbin_ctxInc_r[2] == valbin_ctxInc_r[1])
	{
		tlb2ctl_din2 = tlb2ctl_din1;
	}
	else if (valbin_ctxInc_r[2] == valbin_ctxInc_r[0])
	{
		tlb2ctl_din2 = tlb2ctl_din0;
	}

	binval_in2					=	binval2;    
	pstateidx_in2             = tlb2ctl_din2 >>1;
	valmps_in2                = tlb2ctl_din2 & 0x1;
	est_bit_in2                 = est_bit_out1;

	Decision_unit_input_est   decision_unit_input2_est;
	Decision_unit_output_est   decision_unit_output2_est;

	decision_unit_input2_est.binval_in = binval_in2;
	decision_unit_input2_est.pstateidx_in = pstateidx_in2;
	decision_unit_input2_est.valmps_in = valmps_in2;
	decision_unit_input2_est.est_bit_in = est_bit_in2;

	hevc_enc_cabac_decision_unit_est(decision_unit_input2_est , decision_unit_output2_est);

	est_bit_out2 = decision_unit_output2_est.est_bit_out;
	pstateidx_out2 = decision_unit_output2_est.pstateidx_out;
	valmps_out2 = decision_unit_output2_est.valmps_out;

	tlb2ctl_din2 = (pstateidx_out2<<1) + valmps_out2;

	//bin3
	if (valbin_ctxInc_r[3] == valbin_ctxInc_r[2])
	{
		tlb2ctl_din3 = tlb2ctl_din2;
	}
	else if (valbin_ctxInc_r[3] == valbin_ctxInc_r[1])
	{
		tlb2ctl_din3 = tlb2ctl_din1;
	}
	else if (valbin_ctxInc_r[3] == valbin_ctxInc_r[0])
	{
		tlb2ctl_din3 = tlb2ctl_din0;
	}

	binval_in3					=	binval3;    
	pstateidx_in3             = tlb2ctl_din3 >>1;
	valmps_in3                = tlb2ctl_din3 & 0x1;
	est_bit_in3                 = est_bit_out2;

	Decision_unit_input_est   decision_unit_input3_est;
	Decision_unit_output_est   decision_unit_output3_est;

	decision_unit_input3_est.binval_in = binval_in3;
	decision_unit_input3_est.pstateidx_in = pstateidx_in3;
	decision_unit_input3_est.valmps_in = valmps_in3;
	decision_unit_input3_est.est_bit_in = est_bit_in3;

	hevc_enc_cabac_decision_unit_est(decision_unit_input3_est , decision_unit_output3_est);

	est_bit_out3 = decision_unit_output3_est.est_bit_out;
	pstateidx_out3 = decision_unit_output3_est.pstateidx_out;
	valmps_out3 = decision_unit_output3_est.valmps_out;

	tlb2ctl_din3 = (pstateidx_out3<<1) + valmps_out3;
	//bin 0 1 2 3 end
	switch (val_len_r)
	{
	case 1:
		enc_output_est.est_bit_out = est_bit_out0;
		mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[0]].set_m_ucState(tlb2ctl_din0);
		break;
	case 2:
		enc_output_est.est_bit_out = est_bit_out1;
		mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[0]].set_m_ucState(tlb2ctl_din0);
		mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[1]].set_m_ucState(tlb2ctl_din1);
		break;
	case 3:
		enc_output_est.est_bit_out = est_bit_out2;
		mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[0]].set_m_ucState(tlb2ctl_din0);
		mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[1]].set_m_ucState(tlb2ctl_din1);
		mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[2]].set_m_ucState(tlb2ctl_din2);
		break;
	case 4:
		enc_output_est.est_bit_out = est_bit_out3;
		mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[0]].set_m_ucState(tlb2ctl_din0);
		mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[1]].set_m_ucState(tlb2ctl_din1);
		mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[2]].set_m_ucState(tlb2ctl_din2);
		mModels[IsIntra_tu][depth_tu][ctx_offset + valbin_ctxInc_r[3]].set_m_ucState(tlb2ctl_din3);
		break;
	}
}


#pragma  region
const UChar aucNextStateMPS[ 128 ] =
{
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
	18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
	34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
	66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
	82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97,
	98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
	114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 124, 125, 126, 127
};

const UChar aucNextStateLPS[ 128 ] =
{
	1, 0, 0, 1, 2, 3, 4, 5, 4, 5, 8, 9, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17, 18, 19, 18, 19, 22, 23, 22, 23, 24, 25,
	26, 27, 26, 27, 30, 31, 30, 31, 32, 33, 32, 33, 36, 37, 36, 37,
	38, 39, 38, 39, 42, 43, 42, 43, 44, 45, 44, 45, 46, 47, 48, 49,
	48, 49, 50, 51, 52, 53, 52, 53, 54, 55, 54, 55, 56, 57, 58, 59,
	58, 59, 60, 61, 60, 61, 60, 61, 62, 63, 64, 65, 64, 65, 66, 67,
	66, 67, 66, 67, 68, 69, 68, 69, 70, 71, 70, 71, 70, 71, 72, 73,
	72, 73, 72, 73, 74, 75, 74, 75, 74, 75, 76, 77, 76, 77, 126, 127
};
const UChar ContextModel_hm::m_aucNextStateMPS[ 128 ] =
{
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
	18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
	34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
	66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
	82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97,
	98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
	114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 124, 125, 126, 127
};

const UChar ContextModel_hm::m_aucNextStateLPS[ 128 ] =
{
	1, 0, 0, 1, 2, 3, 4, 5, 4, 5, 8, 9, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17, 18, 19, 18, 19, 22, 23, 22, 23, 24, 25,
	26, 27, 26, 27, 30, 31, 30, 31, 32, 33, 32, 33, 36, 37, 36, 37,
	38, 39, 38, 39, 42, 43, 42, 43, 44, 45, 44, 45, 46, 47, 48, 49,
	48, 49, 50, 51, 52, 53, 52, 53, 54, 55, 54, 55, 56, 57, 58, 59,
	58, 59, 60, 61, 60, 61, 60, 61, 62, 63, 64, 65, 64, 65, 66, 67,
	66, 67, 66, 67, 68, 69, 68, 69, 70, 71, 70, 71, 70, 71, 72, 73,
	72, 73, 72, 73, 74, 75, 74, 75, 74, 75, 76, 77, 76, 77, 126, 127
};
unsigned char state_table[3][52][CONTEXT_NUM] =
{
	{
		{
			14,  30,  17,  49,  49,   1,   1,   1,   1,   1,
				30,   1,   1,   1,  30,  81,  17,   1,   1,   1,
				1,   1,   1,   1,   1,   1,   1,   1,  14,   1,
				1,  81,  49,  65,   1,  62,   1,   1,   1,   1,
				1,   1,  17,  17,  65,  65,  33,  49,  33,  14,
				49,  81,  33,  49,  81,  81,  81,  81,  81,  33,
				17,  81,  65,  65,  33,  49,  33,  14,  49,  81,
				33,  49,  81,  81,  81,  81,  81,  33,  17,  81,
				17,  17,  62,  49,  81,  81,  49,  65,  65,  65,
				33,  33,  33,  17,  49,  49, 110,  14,  49,  17,
				49,  49, 110,  14,  49,  17,  49,  49, 110,  14,
				49,  33,  17,  62,  62,  30,  30,  30,  30,  14,
				30,  17,  81,  30,  17,  81,   1,   1,  33,  33,
				14,   1,  33,  30,   1,  17,  14,   1,  78,  33,
				17,  17,   1,  30,  33, 110,  62,  62,  33, 110,
				1,  78,   1,  14,  30,  46,  30,  30,   1,   1,
				1,   1,   1,   1,   1
		},
		{
			14,  30,  15,  47,  49,   1,   1,   1,   1,   1,
				30,   1,   1,   1,  30,  77,  15,   1,   1,   1,
				1,   1,   1,   1,   1,   1,   1,   1,  14,   0,
				0,  79,  47,  61,   0,  62,   1,   1,   1,   1,
				1,   1,  15,  15,  63,  63,  31,  47,  31,  14,
				47,  79,  31,  47,  79,  79,  79,  79,  77,  31,
				15,  77,  63,  63,  31,  47,  31,  14,  47,  79,
				31,  47,  79,  79,  79,  79,  77,  31,  15,  77,
				13,  17,  64,  47,  79,  79,  47,  63,  63,  61,
				31,  31,  31,  15,  47,  47, 110,  14,  47,  15,
				47,  47, 110,  14,  47,  15,  47,  47, 110,  14,
				47,  31,  15,  62,  62,  30,  32,  30,  32,  14,
				32,  15,  79,  32,  15,  79,   1,   1,  31,  29,
				16,   0,  31,  30,   0,  15,  14,   2,  78,  29,
				15,  15,   0,  30,  31, 110,  62,  62,  31, 108,
				0,  78,   0,  14,  32,  46,  30,  30,   1,   1,
				1,   1,   1,   1,   1
			},
			{
				14,  28,  15,  47,  49,   1,   1,   1,   1,   1,
					28,   1,   1,   1,  28,  73,  15,   1,   1,   1,
					1,   1,   1,   1,   1,   1,   1,   1,  14,   0,
					0,  77,  47,  59,   0,  60,   1,   1,   1,   1,
					1,   1,  15,  15,  61,  61,  29,  45,  31,  14,
					45,  77,  31,  45,  77,  79,  77,  77,  73,  29,
					13,  73,  61,  61,  29,  45,  31,  14,  45,  77,
					31,  45,  77,  79,  77,  77,  73,  29,  13,  73,
					11,  17,  64,  47,  77,  77,  45,  61,  61,  59,
					29,  29,  29,  13,  45,  47, 108,  14,  45,  13,
					45,  47, 108,  14,  45,  13,  45,  47, 108,  14,
					45,  31,  15,  60,  60,  30,  32,  30,  32,  14,
					32,  15,  77,  32,  15,  77,   1,   1,  31,  27,
					16,   0,  31,  30,   0,  15,  14,   6,  78,  27,
					15,  13,   2,  30,  31, 108,  62,  60,  31, 104,
					2,  76,   0,  14,  32,  46,  30,  30,   1,   1,
					1,   1,   1,   1,   1
			},
			{
				14,  26,  15,  47,  49,   1,   1,   1,   1,   1,
					28,   1,   1,   1,  28,  69,  15,   1,   1,   1,
					1,   1,   1,   1,   1,   1,   1,   1,  14,   0,
					0,  75,  47,  57,   0,  60,   1,   1,   1,   1,
					1,   1,  15,  15,  59,  59,  29,  45,  31,  14,
					45,  77,  31,  43,  75,  79,  77,  75,  71,  27,
					13,  69,  59,  59,  29,  45,  31,  14,  45,  77,
					31,  43,  75,  79,  77,  75,  71,  27,  13,  69,
					9,  17,  64,  47,  75,  75,  45,  59,  59,  57,
					29,  27,  29,  11,  45,  47, 108,  14,  45,  11,
					45,  47, 108,  14,  45,  11,  45,  47, 108,  14,
					45,  31,  15,  60,  60,  30,  32,  30,  32,  14,
					32,  15,  75,  32,  15,  75,   1,   1,  31,  25,
					16,   0,  31,  30,   0,  15,  14,   8,  78,  25,
					15,  11,   2,  30,  31, 108,  62,  60,  31, 102,
					2,  74,   0,  14,  32,  46,  30,  30,   1,   1,
					1,   1,   1,   1,   1
				},
				{
					14,  24,  13,  45,  49,   1,   1,   1,   1,   1,
						26,   1,   1,   1,  26,  65,  13,   1,   1,   1,
						1,   1,   1,   1,   1,   1,   1,   1,  14,   2,
						2,  73,  45,  55,   2,  58,   1,   1,   1,   1,
						1,   1,  13,  13,  57,  57,  27,  43,  29,  14,
						43,  75,  29,  41,  73,  77,  75,  73,  67,  25,
						11,  65,  57,  57,  27,  43,  29,  14,  43,  75,
						29,  41,  73,  77,  75,  73,  67,  25,  11,  65,
						7,  19,  66,  45,  73,  73,  43,  57,  57,  55,
						27,  25,  27,   9,  43,  45, 106,  14,  43,   9,
						43,  45, 106,  14,  43,   9,  43,  45, 106,  14,
						43,  29,  13,  58,  58,  30,  34,  30,  34,  14,
						34,  13,  73,  34,  13,  73,   1,   1,  29,  23,
						18,   2,  29,  30,   2,  13,  14,  12,  78,  23,
						13,   9,   4,  30,  29, 106,  60,  58,  29,  98,
						4,  72,   2,  14,  34,  44,  30,  30,   1,   1,
						1,   1,   1,   1,   1
				},
				{
					14,  22,  13,  45,  49,   1,   1,   1,   1,   1,
						24,   1,   1,   1,  24,  61,  13,   1,   1,   1,
						1,   1,   1,   1,   1,   1,   1,   1,  14,   2,
						2,  71,  45,  51,   2,  56,   1,   1,   1,   1,
						1,   1,  13,  13,  55,  55,  25,  41,  29,  14,
						41,  73,  29,  39,  71,  77,  73,  71,  65,  23,
						9,  61,  55,  55,  25,  41,  29,  14,  41,  73,
						29,  39,  71,  77,  73,  71,  65,  23,   9,  61,
						3,  19,  66,  45,  71,  71,  41,  55,  55,  51,
						25,  23,  25,   7,  41,  45, 104,  14,  41,   7,
						41,  45, 104,  14,  41,   7,  41,  45, 104,  14,
						41,  29,  13,  56,  56,  30,  34,  30,  34,  14,
						34,  13,  71,  34,  13,  71,   1,   1,  29,  19,
						18,   2,  29,  30,   2,  13,  14,  14,  78,  19,
						13,   7,   6,  30,  29, 104,  60,  56,  29,  96,
						6,  70,   2,  14,  34,  44,  30,  30,   1,   1,
						1,   1,   1,   1,   1
					},
					{
						14,  20,  13,  45,  49,   1,   1,   1,   1,   1,
							24,   1,   1,   1,  24,  57,  13,   1,   1,   1,
							1,   1,   1,   1,   1,   1,   1,   1,  14,   2,
							2,  69,  45,  49,   2,  56,   1,   1,   1,   1,
							1,   1,  13,  13,  53,  53,  25,  41,  29,  14,
							41,  73,  29,  37,  69,  77,  73,  69,  61,  21,
							9,  57,  53,  53,  25,  41,  29,  14,  41,  73,
							29,  37,  69,  77,  73,  69,  61,  21,   9,  57,
							1,  19,  66,  45,  69,  69,  41,  53,  53,  49,
							25,  21,  25,   5,  41,  45, 104,  14,  41,   5,
							41,  45, 104,  14,  41,   5,  41,  45, 104,  14,
							41,  29,  13,  56,  56,  30,  34,  30,  34,  14,
							34,  13,  69,  34,  13,  69,   1,   1,  29,  17,
							18,   2,  29,  30,   2,  13,  14,  18,  78,  17,
							13,   5,   6,  30,  29, 104,  60,  56,  29,  92,
							6,  68,   2,  14,  34,  44,  30,  30,   1,   1,
							1,   1,   1,   1,   1
					},
					{
						14,  18,  11,  43,  49,   1,   1,   1,   1,   1,
							22,   1,   1,   1,  22,  53,  11,   1,   1,   1,
							1,   1,   1,   1,   1,   1,   1,   1,  14,   4,
							4,  67,  43,  47,   4,  54,   1,   1,   1,   1,
							1,   1,  11,  11,  51,  51,  23,  39,  27,  14,
							39,  71,  27,  35,  67,  75,  71,  67,  59,  19,
							7,  53,  51,  51,  23,  39,  27,  14,  39,  71,
							27,  35,  67,  75,  71,  67,  59,  19,   7,  53,
							0,  21,  68,  43,  67,  67,  39,  51,  51,  47,
							23,  19,  23,   3,  39,  43, 102,  14,  39,   3,
							39,  43, 102,  14,  39,   3,  39,  43, 102,  14,
							39,  27,  11,  54,  54,  30,  36,  30,  36,  14,
							36,  11,  67,  36,  11,  67,   1,   1,  27,  15,
							20,   4,  27,  30,   4,  11,  14,  20,  78,  15,
							11,   3,   8,  30,  27, 102,  58,  54,  27,  90,
							8,  66,   4,  14,  36,  42,  30,  30,   1,   1,
							1,   1,   1,   1,   1
						},
						{
							14,  16,  11,  43,  49,   1,   1,   1,   1,   1,
								20,   1,   1,   1,  20,  51,  11,   1,   1,   1,
								1,   1,   1,   1,   1,   1,   1,   1,  14,   4,
								4,  65,  43,  45,   4,  52,   1,   1,   1,   1,
								1,   1,  11,  11,  49,  49,  23,  39,  27,  14,
								39,  71,  27,  33,  65,  75,  71,  65,  55,  17,
								7,  51,  49,  49,  23,  39,  27,  14,  39,  71,
								27,  33,  65,  75,  71,  65,  55,  17,   7,  51,
								2,  21,  68,  43,  65,  65,  39,  49,  49,  45,
								23,  17,  23,   1,  39,  43, 100,  14,  39,   1,
								39,  43, 100,  14,  39,   1,  39,  43, 100,  14,
								39,  27,  11,  52,  52,  30,  36,  30,  36,  14,
								36,  11,  65,  36,  11,  65,   1,   1,  27,  13,
								20,   4,  27,  30,   4,  11,  14,  24,  78,  13,
								11,   1,   8,  30,  27, 100,  58,  52,  27,  86,
								8,  64,   4,  14,  36,  42,  30,  30,   1,   1,
								1,   1,   1,   1,   1
						},
						{
							14,  14,  11,  43,  49,   1,   1,   1,   1,   1,
								20,   1,   1,   1,  20,  47,  11,   1,   1,   1,
								1,   1,   1,   1,   1,   1,   1,   1,  14,   4,
								4,  63,  43,  41,   4,  52,   1,   1,   1,   1,
								1,   1,  11,  11,  47,  47,  21,  37,  27,  14,
								37,  69,  27,  31,  63,  75,  69,  63,  51,  15,
								5,  47,  47,  47,  21,  37,  27,  14,  37,  69,
								27,  31,  63,  75,  69,  63,  51,  15,   5,  47,
								6,  21,  68,  43,  63,  63,  37,  47,  47,  41,
								21,  15,  21,   0,  37,  43, 100,  14,  37,   0,
								37,  43, 100,  14,  37,   0,  37,  43, 100,  14,
								37,  27,  11,  52,  52,  30,  36,  30,  36,  14,
								36,  11,  63,  36,  11,  63,   1,   1,  27,   9,
								20,   4,  27,  30,   4,  11,  14,  28,  78,   9,
								11,   0,  10,  30,  27, 100,  58,  52,  27,  82,
								10,  62,   4,  14,  36,  42,  30,  30,   1,   1,
								1,   1,   1,   1,   1
							},
							{
								14,  12,   9,  41,  49,   1,   1,   1,   1,   1,
									18,   1,   1,   1,  18,  43,   9,   1,   1,   1,
									1,   1,   1,   1,   1,   1,   1,   1,  14,   6,
									6,  61,  41,  39,   6,  50,   1,   1,   1,   1,
									1,   1,   9,   9,  45,  45,  19,  35,  25,  14,
									35,  67,  25,  29,  61,  73,  67,  61,  49,  13,
									3,  43,  45,  45,  19,  35,  25,  14,  35,  67,
									25,  29,  61,  73,  67,  61,  49,  13,   3,  43,
									8,  23,  70,  41,  61,  61,  35,  45,  45,  39,
									19,  13,  19,   2,  35,  41,  98,  14,  35,   2,
									35,  41,  98,  14,  35,   2,  35,  41,  98,  14,
									35,  25,   9,  50,  50,  30,  38,  30,  38,  14,
									38,   9,  61,  38,   9,  61,   1,   1,  25,   7,
									22,   6,  25,  30,   6,   9,  14,  30,  78,   7,
									9,   2,  12,  30,  25,  98,  56,  50,  25,  80,
									12,  60,   6,  14,  38,  40,  30,  30,   1,   1,
									1,   1,   1,   1,   1
							},
							{
								14,  10,   9,  41,  49,   1,   1,   1,   1,   1,
									18,   1,   1,   1,  18,  39,   9,   1,   1,   1,
									1,   1,   1,   1,   1,   1,   1,   1,  14,   6,
									6,  59,  41,  37,   6,  50,   1,   1,   1,   1,
									1,   1,   9,   9,  43,  43,  19,  35,  25,  14,
									35,  67,  25,  27,  59,  73,  67,  59,  45,  11,
									3,  39,  43,  43,  19,  35,  25,  14,  35,  67,
									25,  27,  59,  73,  67,  59,  45,  11,   3,  39,
									10,  23,  70,  41,  59,  59,  35,  43,  43,  37,
									19,  11,  19,   4,  35,  41,  98,  14,  35,   4,
									35,  41,  98,  14,  35,   4,  35,  41,  98,  14,
									35,  25,   9,  50,  50,  30,  38,  30,  38,  14,
									38,   9,  59,  38,   9,  59,   1,   1,  25,   5,
									22,   6,  25,  30,   6,   9,  14,  34,  78,   5,
									9,   4,  12,  30,  25,  98,  56,  50,  25,  76,
									12,  58,   6,  14,  38,  40,  30,  30,   1,   1,
									1,   1,   1,   1,   1
								},
								{
									14,   8,   9,  41,  49,   1,   1,   1,   1,   1,
										16,   1,   1,   1,  16,  35,   9,   1,   1,   1,
										1,   1,   1,   1,   1,   1,   1,   1,  14,   6,
										6,  57,  41,  35,   6,  48,   1,   1,   1,   1,
										1,   1,   9,   9,  41,  41,  17,  33,  25,  14,
										33,  65,  25,  25,  57,  73,  65,  57,  43,   9,
										1,  35,  41,  41,  17,  33,  25,  14,  33,  65,
										25,  25,  57,  73,  65,  57,  43,   9,   1,  35,
										12,  23,  70,  41,  57,  57,  33,  41,  41,  35,
										17,   9,  17,   6,  33,  41,  96,  14,  33,   6,
										33,  41,  96,  14,  33,   6,  33,  41,  96,  14,
										33,  25,   9,  48,  48,  30,  38,  30,  38,  14,
										38,   9,  57,  38,   9,  57,   1,   1,  25,   3,
										22,   6,  25,  30,   6,   9,  14,  36,  78,   3,
										9,   6,  14,  30,  25,  96,  56,  48,  25,  74,
										14,  56,   6,  14,  38,  40,  30,  30,   1,   1,
										1,   1,   1,   1,   1
								},
								{
									14,   6,   7,  39,  49,   1,   1,   1,   1,   1,
										14,   1,   1,   1,  14,  31,   7,   1,   1,   1,
										1,   1,   1,   1,   1,   1,   1,   1,  14,   8,
										8,  55,  39,  31,   8,  46,   1,   1,   1,   1,
										1,   1,   7,   7,  39,  39,  15,  31,  23,  14,
										31,  63,  23,  23,  55,  71,  63,  55,  39,   7,
										0,  31,  39,  39,  15,  31,  23,  14,  31,  63,
										23,  23,  55,  71,  63,  55,  39,   7,   0,  31,
										16,  25,  72,  39,  55,  55,  31,  39,  39,  31,
										15,   7,  15,   8,  31,  39,  94,  14,  31,   8,
										31,  39,  94,  14,  31,   8,  31,  39,  94,  14,
										31,  23,   7,  46,  46,  30,  40,  30,  40,  14,
										40,   7,  55,  40,   7,  55,   1,   1,  23,   0,
										24,   8,  23,  30,   8,   7,  14,  40,  78,   0,
										7,   8,  16,  30,  23,  94,  54,  46,  23,  70,
										16,  54,   8,  14,  40,  38,  30,  30,   1,   1,
										1,   1,   1,   1,   1
									},
									{
										14,   4,   7,  39,  49,   1,   1,   1,   1,   1,
											14,   1,   1,   1,  14,  27,   7,   1,   1,   1,
											1,   1,   1,   1,   1,   1,   1,   1,  14,   8,
											8,  53,  39,  29,   8,  46,   1,   1,   1,   1,
											1,   1,   7,   7,  37,  37,  15,  31,  23,  14,
											31,  63,  23,  21,  53,  71,  63,  53,  37,   5,
											0,  27,  37,  37,  15,  31,  23,  14,  31,  63,
											23,  21,  53,  71,  63,  53,  37,   5,   0,  27,
											18,  25,  72,  39,  53,  53,  31,  37,  37,  29,
											15,   5,  15,  10,  31,  39,  94,  14,  31,  10,
											31,  39,  94,  14,  31,  10,  31,  39,  94,  14,
											31,  23,   7,  46,  46,  30,  40,  30,  40,  14,
											40,   7,  53,  40,   7,  53,   1,   1,  23,   2,
											24,   8,  23,  30,   8,   7,  14,  42,  78,   2,
											7,  10,  16,  30,  23,  94,  54,  46,  23,  68,
											16,  52,   8,  14,  40,  38,  30,  30,   1,   1,
											1,   1,   1,   1,   1
									},
									{
										14,   2,   7,  39,  49,   1,   1,   1,   1,   1,
											12,   1,   1,   1,  12,  23,   7,   1,   1,   1,
											1,   1,   1,   1,   1,   1,   1,   1,  14,   8,
											8,  51,  39,  27,   8,  44,   1,   1,   1,   1,
											1,   1,   7,   7,  35,  35,  13,  29,  23,  14,
											29,  61,  23,  19,  51,  71,  61,  51,  33,   3,
											2,  23,  35,  35,  13,  29,  23,  14,  29,  61,
											23,  19,  51,  71,  61,  51,  33,   3,   2,  23,
											20,  25,  72,  39,  51,  51,  29,  35,  35,  27,
											13,   3,  13,  12,  29,  39,  92,  14,  29,  12,
											29,  39,  92,  14,  29,  12,  29,  39,  92,  14,
											29,  23,   7,  44,  44,  30,  40,  30,  40,  14,
											40,   7,  51,  40,   7,  51,   1,   1,  23,   4,
											24,   8,  23,  30,   8,   7,  14,  46,  78,   4,
											7,  12,  18,  30,  23,  92,  54,  44,  23,  64,
											18,  50,   8,  14,  40,  38,  30,  30,   1,   1,
											1,   1,   1,   1,   1
										},
										{
											14,   0,   7,  39,  49,   1,   1,   1,   1,   1,
												10,   1,   1,   1,  10,  21,   7,   1,   1,   1,
												1,   1,   1,   1,   1,   1,   1,   1,  14,   8,
												8,  51,  39,  25,   8,  42,   1,   1,   1,   1,
												1,   1,   7,   7,  35,  35,  13,  29,  23,  14,
												29,  61,  23,  19,  51,  71,  61,  51,  31,   3,
												2,  21,  35,  35,  13,  29,  23,  14,  29,  61,
												23,  19,  51,  71,  61,  51,  31,   3,   2,  21,
												22,  27,  72,  39,  51,  51,  29,  35,  35,  25,
												13,   3,  13,  12,  29,  39,  90,  14,  29,  12,
												29,  39,  90,  14,  29,  12,  29,  39,  90,  14,
												29,  23,   7,  42,  42,  30,  40,  30,  40,  14,
												40,   7,  51,  40,   7,  51,   1,   1,  23,   6,
												24,   8,  23,  30,   8,   7,  14,  48,  78,   6,
												7,  12,  18,  30,  23,  90,  52,  42,  23,  60,
												18,  48,   8,  14,  40,  36,  30,  30,   1,   1,
												1,   1,   1,   1,   1
										},
										{
											14,   0,   5,  37,  49,   1,   1,   1,   1,   1,
												10,   1,   1,   1,  10,  17,   5,   1,   1,   1,
												1,   1,   1,   1,   1,   1,   1,   1,  14,  10,
												10,  49,  37,  21,  10,  42,   1,   1,   1,   1,
												1,   1,   5,   5,  33,  33,  11,  27,  21,  14,
												27,  59,  21,  17,  49,  69,  59,  49,  27,   1,
												4,  17,  33,  33,  11,  27,  21,  14,  27,  59,
												21,  17,  49,  69,  59,  49,  27,   1,   4,  17,
												26,  27,  74,  37,  49,  49,  27,  33,  33,  21,
												11,   1,  11,  14,  27,  37,  90,  14,  27,  14,
												27,  37,  90,  14,  27,  14,  27,  37,  90,  14,
												27,  21,   5,  42,  42,  30,  42,  30,  42,  14,
												42,   5,  49,  42,   5,  49,   1,   1,  21,  10,
												26,  10,  21,  30,  10,   5,  14,  52,  78,  10,
												5,  14,  20,  30,  21,  90,  52,  42,  21,  58,
												20,  48,  10,  14,  42,  36,  30,  30,   1,   1,
												1,   1,   1,   1,   1
											},
											{
												14,   1,   5,  37,  49,   1,   1,   1,   1,   1,
													8,   1,   1,   1,   8,  13,   5,   1,   1,   1,
													1,   1,   1,   1,   1,   1,   1,   1,  14,  10,
													10,  47,  37,  19,  10,  40,   1,   1,   1,   1,
													1,   1,   5,   5,  31,  31,   9,  25,  21,  14,
													25,  57,  21,  15,  47,  69,  57,  47,  23,   0,
													6,  13,  31,  31,   9,  25,  21,  14,  25,  57,
													21,  15,  47,  69,  57,  47,  23,   0,   6,  13,
													28,  27,  74,  37,  47,  47,  25,  31,  31,  19,
													9,   0,   9,  16,  25,  37,  88,  14,  25,  16,
													25,  37,  88,  14,  25,  16,  25,  37,  88,  14,
													25,  21,   5,  40,  40,  30,  42,  30,  42,  14,
													42,   5,  47,  42,   5,  47,   1,   1,  21,  12,
													26,  10,  21,  30,  10,   5,  14,  56,  78,  12,
													5,  16,  22,  30,  21,  88,  52,  40,  21,  54,
													22,  46,  10,  14,  42,  36,  30,  30,   1,   1,
													1,   1,   1,   1,   1
											},
											{
												14,   3,   5,  37,  49,   1,   1,   1,   1,   1,
													8,   1,   1,   1,   8,   9,   5,   1,   1,   1,
													1,   1,   1,   1,   1,   1,   1,   1,  14,  10,
													10,  45,  37,  17,  10,  40,   1,   1,   1,   1,
													1,   1,   5,   5,  29,  29,   9,  25,  21,  14,
													25,  57,  21,  13,  45,  69,  57,  45,  21,   2,
													6,   9,  29,  29,   9,  25,  21,  14,  25,  57,
													21,  13,  45,  69,  57,  45,  21,   2,   6,   9,
													30,  27,  74,  37,  45,  45,  25,  29,  29,  17,
													9,   2,   9,  18,  25,  37,  88,  14,  25,  18,
													25,  37,  88,  14,  25,  18,  25,  37,  88,  14,
													25,  21,   5,  40,  40,  30,  42,  30,  42,  14,
													42,   5,  45,  42,   5,  45,   1,   1,  21,  14,
													26,  10,  21,  30,  10,   5,  14,  58,  78,  14,
													5,  18,  22,  30,  21,  88,  52,  40,  21,  52,
													22,  44,  10,  14,  42,  36,  30,  30,   1,   1,
													1,   1,   1,   1,   1
												},
												{
													14,   5,   3,  35,  49,   1,   1,   1,   1,   1,
														6,   1,   1,   1,   6,   5,   3,   1,   1,   1,
														1,   1,   1,   1,   1,   1,   1,   1,  14,  12,
														12,  43,  35,  15,  12,  38,   1,   1,   1,   1,
														1,   1,   3,   3,  27,  27,   7,  23,  19,  14,
														23,  55,  19,  11,  43,  67,  55,  43,  17,   4,
														8,   5,  27,  27,   7,  23,  19,  14,  23,  55,
														19,  11,  43,  67,  55,  43,  17,   4,   8,   5,
														32,  29,  76,  35,  43,  43,  23,  27,  27,  15,
														7,   4,   7,  20,  23,  35,  86,  14,  23,  20,
														23,  35,  86,  14,  23,  20,  23,  35,  86,  14,
														23,  19,   3,  38,  38,  30,  44,  30,  44,  14,
														44,   3,  43,  44,   3,  43,   1,   1,  19,  16,
														28,  12,  19,  30,  12,   3,  14,  62,  78,  16,
														3,  20,  24,  30,  19,  86,  50,  38,  19,  48,
														24,  42,  12,  14,  44,  34,  30,  30,   1,   1,
														1,   1,   1,   1,   1
												},
												{
													14,   7,   3,  35,  49,   1,   1,   1,   1,   1,
														4,   1,   1,   1,   4,   1,   3,   1,   1,   1,
														1,   1,   1,   1,   1,   1,   1,   1,  14,  12,
														12,  41,  35,  11,  12,  36,   1,   1,   1,   1,
														1,   1,   3,   3,  25,  25,   5,  21,  19,  14,
														21,  53,  19,   9,  41,  67,  53,  41,  15,   6,
														10,   1,  25,  25,   5,  21,  19,  14,  21,  53,
														19,   9,  41,  67,  53,  41,  15,   6,  10,   1,
														36,  29,  76,  35,  41,  41,  21,  25,  25,  11,
														5,   6,   5,  22,  21,  35,  84,  14,  21,  22,
														21,  35,  84,  14,  21,  22,  21,  35,  84,  14,
														21,  19,   3,  36,  36,  30,  44,  30,  44,  14,
														44,   3,  41,  44,   3,  41,   1,   1,  19,  20,
														28,  12,  19,  30,  12,   3,  14,  64,  78,  20,
														3,  22,  26,  30,  19,  84,  50,  36,  19,  46,
														26,  40,  12,  14,  44,  34,  30,  30,   1,   1,
														1,   1,   1,   1,   1
													},
													{
														14,   9,   3,  35,  49,   1,   1,   1,   1,   1,
															4,   1,   1,   1,   4,   2,   3,   1,   1,   1,
															1,   1,   1,   1,   1,   1,   1,   1,  14,  12,
															12,  39,  35,   9,  12,  36,   1,   1,   1,   1,
															1,   1,   3,   3,  23,  23,   5,  21,  19,  14,
															21,  53,  19,   7,  39,  67,  53,  39,  11,   8,
															10,   2,  23,  23,   5,  21,  19,  14,  21,  53,
															19,   7,  39,  67,  53,  39,  11,   8,  10,   2,
															38,  29,  76,  35,  39,  39,  21,  23,  23,   9,
															5,   8,   5,  24,  21,  35,  84,  14,  21,  24,
															21,  35,  84,  14,  21,  24,  21,  35,  84,  14,
															21,  19,   3,  36,  36,  30,  44,  30,  44,  14,
															44,   3,  39,  44,   3,  39,   1,   1,  19,  22,
															28,  12,  19,  30,  12,   3,  14,  68,  78,  22,
															3,  24,  26,  30,  19,  84,  50,  36,  19,  42,
															26,  38,  12,  14,  44,  34,  30,  30,   1,   1,
															1,   1,   1,   1,   1
													},
													{
														14,  11,   1,  33,  49,   1,   1,   1,   1,   1,
															2,   1,   1,   1,   2,   6,   1,   1,   1,   1,
															1,   1,   1,   1,   1,   1,   1,   1,  14,  14,
															14,  37,  33,   7,  14,  34,   1,   1,   1,   1,
															1,   1,   1,   1,  21,  21,   3,  19,  17,  14,
															19,  51,  17,   5,  37,  65,  51,  37,   9,  10,
															12,   6,  21,  21,   3,  19,  17,  14,  19,  51,
															17,   5,  37,  65,  51,  37,   9,  10,  12,   6,
															40,  31,  78,  33,  37,  37,  19,  21,  21,   7,
															3,  10,   3,  26,  19,  33,  82,  14,  19,  26,
															19,  33,  82,  14,  19,  26,  19,  33,  82,  14,
															19,  17,   1,  34,  34,  30,  46,  30,  46,  14,
															46,   1,  37,  46,   1,  37,   1,   1,  17,  24,
															30,  14,  17,  30,  14,   1,  14,  70,  78,  24,
															1,  26,  28,  30,  17,  82,  48,  34,  17,  40,
															28,  36,  14,  14,  46,  32,  30,  30,   1,   1,
															1,   1,   1,   1,   1
														},
														{
															14,  13,   1,  33,  49,   1,   1,   1,   1,   1,
																0,   1,   1,   1,   0,   8,   1,   1,   1,   1,
																1,   1,   1,   1,   1,   1,   1,   1,  14,  14,
																14,  35,  33,   5,  14,  32,   1,   1,   1,   1,
																1,   1,   1,   1,  19,  19,   3,  19,  17,  14,
																19,  51,  17,   3,  35,  65,  51,  35,   5,  12,
																12,   8,  19,  19,   3,  19,  17,  14,  19,  51,
																17,   3,  35,  65,  51,  35,   5,  12,  12,   8,
																42,  31,  78,  33,  35,  35,  19,  19,  19,   5,
																3,  12,   3,  28,  19,  33,  80,  14,  19,  28,
																19,  33,  80,  14,  19,  28,  19,  33,  80,  14,
																19,  17,   1,  32,  32,  30,  46,  30,  46,  14,
																46,   1,  35,  46,   1,  35,   1,   1,  17,  26,
																30,  14,  17,  30,  14,   1,  14,  74,  78,  26,
																1,  28,  28,  30,  17,  80,  48,  32,  17,  36,
																28,  34,  14,  14,  46,  32,  30,  30,   1,   1,
																1,   1,   1,   1,   1
														},
														{
															14,  15,   1,  33,  49,   1,   1,   1,   1,   1,
																0,   1,   1,   1,   0,  12,   1,   1,   1,   1,
																1,   1,   1,   1,   1,   1,   1,   1,  14,  14,
																14,  33,  33,   1,  14,  32,   1,   1,   1,   1,
																1,   1,   1,   1,  17,  17,   1,  17,  17,  14,
																17,  49,  17,   1,  33,  65,  49,  33,   1,  14,
																14,  12,  17,  17,   1,  17,  17,  14,  17,  49,
																17,   1,  33,  65,  49,  33,   1,  14,  14,  12,
																46,  31,  78,  33,  33,  33,  17,  17,  17,   1,
																1,  14,   1,  30,  17,  33,  80,  14,  17,  30,
																17,  33,  80,  14,  17,  30,  17,  33,  80,  14,
																17,  17,   1,  32,  32,  30,  46,  30,  46,  14,
																46,   1,  33,  46,   1,  33,   1,   1,  17,  30,
																30,  14,  17,  30,  14,   1,  14,  78,  78,  30,
																1,  30,  30,  30,  17,  80,  48,  32,  17,  32,
																30,  32,  14,  14,  46,  32,  30,  30,   1,   1,
																1,   1,   1,   1,   1
															},
															{
																14,  17,   0,  31,  49,   1,   1,   1,   1,   1,
																	1,   1,   1,   1,   1,  16,   0,   1,   1,   1,
																	1,   1,   1,   1,   1,   1,   1,   1,  14,  16,
																	16,  31,  31,   0,  16,  30,   1,   1,   1,   1,
																	1,   1,   0,   0,  15,  15,   0,  15,  15,  14,
																	15,  47,  15,   0,  31,  63,  47,  31,   0,  16,
																	16,  16,  15,  15,   0,  15,  15,  14,  15,  47,
																	15,   0,  31,  63,  47,  31,   0,  16,  16,  16,
																	48,  33,  80,  31,  31,  31,  15,  15,  15,   0,
																	0,  16,   0,  32,  15,  31,  78,  14,  15,  32,
																	15,  31,  78,  14,  15,  32,  15,  31,  78,  14,
																	15,  15,   0,  30,  30,  30,  48,  30,  48,  14,
																	48,   0,  31,  48,   0,  31,   1,   1,  15,  32,
																	32,  16,  15,  30,  16,   0,  14,  80,  78,  32,
																	0,  32,  32,  30,  15,  78,  46,  30,  15,  30,
																	32,  30,  16,  14,  48,  30,  30,  30,   1,   1,
																	1,   1,   1,   1,   1
															},
															{
																14,  19,   0,  31,  49,   1,   1,   1,   1,   1,
																	1,   1,   1,   1,   1,  20,   0,   1,   1,   1,
																	1,   1,   1,   1,   1,   1,   1,   1,  14,  16,
																	16,  29,  31,   2,  16,  30,   1,   1,   1,   1,
																	1,   1,   0,   0,  13,  13,   0,  15,  15,  14,
																	15,  47,  15,   2,  29,  63,  47,  29,   4,  18,
																	16,  20,  13,  13,   0,  15,  15,  14,  15,  47,
																	15,   2,  29,  63,  47,  29,   4,  18,  16,  20,
																	50,  33,  80,  31,  29,  29,  15,  13,  13,   2,
																	0,  18,   0,  34,  15,  31,  78,  14,  15,  34,
																	15,  31,  78,  14,  15,  34,  15,  31,  78,  14,
																	15,  15,   0,  30,  30,  30,  48,  30,  48,  14,
																	48,   0,  29,  48,   0,  29,   1,   1,  15,  34,
																	32,  16,  15,  30,  16,   0,  14,  84,  78,  34,
																	0,  34,  32,  30,  15,  78,  46,  30,  15,  26,
																	32,  28,  16,  14,  48,  30,  30,  30,   1,   1,
																	1,   1,   1,   1,   1
																},
																{
																	14,  21,   0,  31,  49,   1,   1,   1,   1,   1,
																		3,   1,   1,   1,   3,  24,   0,   1,   1,   1,
																		1,   1,   1,   1,   1,   1,   1,   1,  14,  16,
																		16,  27,  31,   4,  16,  28,   1,   1,   1,   1,
																		1,   1,   0,   0,  11,  11,   2,  13,  15,  14,
																		13,  45,  15,   4,  27,  63,  45,  27,   6,  20,
																		18,  24,  11,  11,   2,  13,  15,  14,  13,  45,
																		15,   4,  27,  63,  45,  27,   6,  20,  18,  24,
																		52,  33,  80,  31,  27,  27,  13,  11,  11,   4,
																		2,  20,   2,  36,  13,  31,  76,  14,  13,  36,
																		13,  31,  76,  14,  13,  36,  13,  31,  76,  14,
																		13,  15,   0,  28,  28,  30,  48,  30,  48,  14,
																		48,   0,  27,  48,   0,  27,   1,   1,  15,  36,
																		32,  16,  15,  30,  16,   0,  14,  86,  78,  36,
																		0,  36,  34,  30,  15,  76,  46,  28,  15,  24,
																		34,  26,  16,  14,  48,  30,  30,  30,   1,   1,
																		1,   1,   1,   1,   1
																},
																{
																	14,  23,   2,  29,  49,   1,   1,   1,   1,   1,
																		5,   1,   1,   1,   5,  28,   2,   1,   1,   1,
																		1,   1,   1,   1,   1,   1,   1,   1,  14,  18,
																		18,  25,  29,   8,  18,  26,   1,   1,   1,   1,
																		1,   1,   2,   2,   9,   9,   4,  11,  13,  14,
																		11,  43,  13,   6,  25,  61,  43,  25,  10,  22,
																		20,  28,   9,   9,   4,  11,  13,  14,  11,  43,
																		13,   6,  25,  61,  43,  25,  10,  22,  20,  28,
																		56,  35,  82,  29,  25,  25,  11,   9,   9,   8,
																		4,  22,   4,  38,  11,  29,  74,  14,  11,  38,
																		11,  29,  74,  14,  11,  38,  11,  29,  74,  14,
																		11,  13,   2,  26,  26,  30,  50,  30,  50,  14,
																		50,   2,  25,  50,   2,  25,   1,   1,  13,  40,
																		34,  18,  13,  30,  18,   2,  14,  90,  78,  40,
																		2,  38,  36,  30,  13,  74,  44,  26,  13,  20,
																		36,  24,  18,  14,  50,  28,  30,  30,   1,   1,
																		1,   1,   1,   1,   1
																	},
																	{
																		14,  25,   2,  29,  49,   1,   1,   1,   1,   1,
																			5,   1,   1,   1,   5,  32,   2,   1,   1,   1,
																			1,   1,   1,   1,   1,   1,   1,   1,  14,  18,
																			18,  23,  29,  10,  18,  26,   1,   1,   1,   1,
																			1,   1,   2,   2,   7,   7,   4,  11,  13,  14,
																			11,  43,  13,   8,  23,  61,  43,  23,  12,  24,
																			20,  32,   7,   7,   4,  11,  13,  14,  11,  43,
																			13,   8,  23,  61,  43,  23,  12,  24,  20,  32,
																			58,  35,  82,  29,  23,  23,  11,   7,   7,  10,
																			4,  24,   4,  40,  11,  29,  74,  14,  11,  40,
																			11,  29,  74,  14,  11,  40,  11,  29,  74,  14,
																			11,  13,   2,  26,  26,  30,  50,  30,  50,  14,
																			50,   2,  23,  50,   2,  23,   1,   1,  13,  42,
																			34,  18,  13,  30,  18,   2,  14,  92,  78,  42,
																			2,  40,  36,  30,  13,  74,  44,  26,  13,  18,
																			36,  22,  18,  14,  50,  28,  30,  30,   1,   1,
																			1,   1,   1,   1,   1
																	},
																	{
																		14,  27,   2,  29,  49,   1,   1,   1,   1,   1,
																			7,   1,   1,   1,   7,  36,   2,   1,   1,   1,
																			1,   1,   1,   1,   1,   1,   1,   1,  14,  18,
																			18,  21,  29,  12,  18,  24,   1,   1,   1,   1,
																			1,   1,   2,   2,   5,   5,   6,   9,  13,  14,
																			9,  41,  13,  10,  21,  61,  41,  21,  16,  26,
																			22,  36,   5,   5,   6,   9,  13,  14,   9,  41,
																			13,  10,  21,  61,  41,  21,  16,  26,  22,  36,
																			60,  35,  82,  29,  21,  21,   9,   5,   5,  12,
																			6,  26,   6,  42,   9,  29,  72,  14,   9,  42,
																			9,  29,  72,  14,   9,  42,   9,  29,  72,  14,
																			9,  13,   2,  24,  24,  30,  50,  30,  50,  14,
																			50,   2,  21,  50,   2,  21,   1,   1,  13,  44,
																			34,  18,  13,  30,  18,   2,  14,  96,  78,  44,
																			2,  42,  38,  30,  13,  72,  44,  24,  13,  14,
																			38,  20,  18,  14,  50,  28,  30,  30,   1,   1,
																			1,   1,   1,   1,   1
																		},
																		{
																			14,  29,   2,  29,  49,   1,   1,   1,   1,   1,
																				9,   1,   1,   1,   9,  38,   2,   1,   1,   1,
																				1,   1,   1,   1,   1,   1,   1,   1,  14,  18,
																				18,  21,  29,  14,  18,  22,   1,   1,   1,   1,
																				1,   1,   2,   2,   5,   5,   6,   9,  13,  14,
																				9,  41,  13,  10,  21,  61,  41,  21,  18,  26,
																				22,  38,   5,   5,   6,   9,  13,  14,   9,  41,
																				13,  10,  21,  61,  41,  21,  18,  26,  22,  38,
																				62,  37,  82,  29,  21,  21,   9,   5,   5,  14,
																				6,  26,   6,  42,   9,  29,  70,  14,   9,  42,
																				9,  29,  70,  14,   9,  42,   9,  29,  70,  14,
																				9,  13,   2,  22,  22,  30,  50,  30,  50,  14,
																				50,   2,  21,  50,   2,  21,   1,   1,  13,  46,
																				34,  18,  13,  30,  18,   2,  14,  98,  78,  46,
																				2,  42,  38,  30,  13,  70,  42,  22,  13,  10,
																				38,  18,  18,  14,  50,  26,  30,  30,   1,   1,
																				1,   1,   1,   1,   1
																		},
																		{
																			14,  29,   4,  27,  49,   1,   1,   1,   1,   1,
																				9,   1,   1,   1,   9,  42,   4,   1,   1,   1,
																				1,   1,   1,   1,   1,   1,   1,   1,  14,  20,
																				20,  19,  27,  18,  20,  22,   1,   1,   1,   1,
																				1,   1,   4,   4,   3,   3,   8,   7,  11,  14,
																				7,  39,  11,  12,  19,  59,  39,  19,  22,  28,
																				24,  42,   3,   3,   8,   7,  11,  14,   7,  39,
																				11,  12,  19,  59,  39,  19,  22,  28,  24,  42,
																				66,  37,  84,  27,  19,  19,   7,   3,   3,  18,
																				8,  28,   8,  44,   7,  27,  70,  14,   7,  44,
																				7,  27,  70,  14,   7,  44,   7,  27,  70,  14,
																				7,  11,   4,  22,  22,  30,  52,  30,  52,  14,
																				52,   4,  19,  52,   4,  19,   1,   1,  11,  50,
																				36,  20,  11,  30,  20,   4,  14, 102,  78,  50,
																				4,  44,  40,  30,  11,  70,  42,  22,  11,   8,
																				40,  18,  20,  14,  52,  26,  30,  30,   1,   1,
																				1,   1,   1,   1,   1
																			},
																			{
																				14,  31,   4,  27,  49,   1,   1,   1,   1,   1,
																					11,   1,   1,   1,  11,  46,   4,   1,   1,   1,
																					1,   1,   1,   1,   1,   1,   1,   1,  14,  20,
																					20,  17,  27,  20,  20,  20,   1,   1,   1,   1,
																					1,   1,   4,   4,   1,   1,  10,   5,  11,  14,
																					5,  37,  11,  14,  17,  59,  37,  17,  26,  30,
																					26,  46,   1,   1,  10,   5,  11,  14,   5,  37,
																					11,  14,  17,  59,  37,  17,  26,  30,  26,  46,
																					68,  37,  84,  27,  17,  17,   5,   1,   1,  20,
																					10,  30,  10,  46,   5,  27,  68,  14,   5,  46,
																					5,  27,  68,  14,   5,  46,   5,  27,  68,  14,
																					5,  11,   4,  20,  20,  30,  52,  30,  52,  14,
																					52,   4,  17,  52,   4,  17,   1,   1,  11,  52,
																					36,  20,  11,  30,  20,   4,  14, 106,  78,  52,
																					4,  46,  42,  30,  11,  68,  42,  20,  11,   4,
																					42,  16,  20,  14,  52,  26,  30,  30,   1,   1,
																					1,   1,   1,   1,   1
																			},
																			{
																				14,  33,   4,  27,  49,   1,   1,   1,   1,   1,
																					11,   1,   1,   1,  11,  50,   4,   1,   1,   1,
																					1,   1,   1,   1,   1,   1,   1,   1,  14,  20,
																					20,  15,  27,  22,  20,  20,   1,   1,   1,   1,
																					1,   1,   4,   4,   0,   0,  10,   5,  11,  14,
																					5,  37,  11,  16,  15,  59,  37,  15,  28,  32,
																					26,  50,   0,   0,  10,   5,  11,  14,   5,  37,
																					11,  16,  15,  59,  37,  15,  28,  32,  26,  50,
																					70,  37,  84,  27,  15,  15,   5,   0,   0,  22,
																					10,  32,  10,  48,   5,  27,  68,  14,   5,  48,
																					5,  27,  68,  14,   5,  48,   5,  27,  68,  14,
																					5,  11,   4,  20,  20,  30,  52,  30,  52,  14,
																					52,   4,  15,  52,   4,  15,   1,   1,  11,  54,
																					36,  20,  11,  30,  20,   4,  14, 108,  78,  54,
																					4,  48,  42,  30,  11,  68,  42,  20,  11,   2,
																					42,  14,  20,  14,  52,  26,  30,  30,   1,   1,
																					1,   1,   1,   1,   1
																				},
																				{
																					14,  35,   6,  25,  49,   1,   1,   1,   1,   1,
																						13,   1,   1,   1,  13,  54,   6,   1,   1,   1,
																						1,   1,   1,   1,   1,   1,   1,   1,  14,  22,
																						22,  13,  25,  24,  22,  18,   1,   1,   1,   1,
																						1,   1,   6,   6,   2,   2,  12,   3,   9,  14,
																						3,  35,   9,  18,  13,  57,  35,  13,  32,  34,
																						28,  54,   2,   2,  12,   3,   9,  14,   3,  35,
																						9,  18,  13,  57,  35,  13,  32,  34,  28,  54,
																						72,  39,  86,  25,  13,  13,   3,   2,   2,  24,
																						12,  34,  12,  50,   3,  25,  66,  14,   3,  50,
																						3,  25,  66,  14,   3,  50,   3,  25,  66,  14,
																						3,   9,   6,  18,  18,  30,  54,  30,  54,  14,
																						54,   6,  13,  54,   6,  13,   1,   1,   9,  56,
																						38,  22,   9,  30,  22,   6,  14, 112,  78,  56,
																						6,  50,  44,  30,   9,  66,  40,  18,   9,   1,
																						44,  12,  22,  14,  54,  24,  30,  30,   1,   1,
																						1,   1,   1,   1,   1
																				},
																				{
																					14,  37,   6,  25,  49,   1,   1,   1,   1,   1,
																						15,   1,   1,   1,  15,  58,   6,   1,   1,   1,
																						1,   1,   1,   1,   1,   1,   1,   1,  14,  22,
																						22,  11,  25,  28,  22,  16,   1,   1,   1,   1,
																						1,   1,   6,   6,   4,   4,  14,   1,   9,  14,
																						1,  33,   9,  20,  11,  57,  33,  11,  34,  36,
																						30,  58,   4,   4,  14,   1,   9,  14,   1,  33,
																						9,  20,  11,  57,  33,  11,  34,  36,  30,  58,
																						76,  39,  86,  25,  11,  11,   1,   4,   4,  28,
																						14,  36,  14,  52,   1,  25,  64,  14,   1,  52,
																						1,  25,  64,  14,   1,  52,   1,  25,  64,  14,
																						1,   9,   6,  16,  16,  30,  54,  30,  54,  14,
																						54,   6,  11,  54,   6,  11,   1,   1,   9,  60,
																						38,  22,   9,  30,  22,   6,  14, 114,  78,  60,
																						6,  52,  46,  30,   9,  64,  40,  16,   9,   3,
																						46,  10,  22,  14,  54,  24,  30,  30,   1,   1,
																						1,   1,   1,   1,   1
																					},
																					{
																						14,  39,   6,  25,  49,   1,   1,   1,   1,   1,
																							15,   1,   1,   1,  15,  62,   6,   1,   1,   1,
																							1,   1,   1,   1,   1,   1,   1,   1,  14,  22,
																							22,   9,  25,  30,  22,  16,   1,   1,   1,   1,
																							1,   1,   6,   6,   6,   6,  14,   1,   9,  14,
																							1,  33,   9,  22,   9,  57,  33,   9,  38,  38,
																							30,  62,   6,   6,  14,   1,   9,  14,   1,  33,
																							9,  22,   9,  57,  33,   9,  38,  38,  30,  62,
																							78,  39,  86,  25,   9,   9,   1,   6,   6,  30,
																							14,  38,  14,  54,   1,  25,  64,  14,   1,  54,
																							1,  25,  64,  14,   1,  54,   1,  25,  64,  14,
																							1,   9,   6,  16,  16,  30,  54,  30,  54,  14,
																							54,   6,   9,  54,   6,   9,   1,   1,   9,  62,
																							38,  22,   9,  30,  22,   6,  14, 118,  78,  62,
																							6,  54,  46,  30,   9,  64,  40,  16,   9,   7,
																							46,   8,  22,  14,  54,  24,  30,  30,   1,   1,
																							1,   1,   1,   1,   1
																					},
																					{
																						14,  41,   8,  23,  49,   1,   1,   1,   1,   1,
																							17,   1,   1,   1,  17,  66,   8,   1,   1,   1,
																							1,   1,   1,   1,   1,   1,   1,   1,  14,  24,
																							24,   7,  23,  32,  24,  14,   1,   1,   1,   1,
																							1,   1,   8,   8,   8,   8,  16,   0,   7,  14,
																							0,  31,   7,  24,   7,  55,  31,   7,  40,  40,
																							32,  66,   8,   8,  16,   0,   7,  14,   0,  31,
																							7,  24,   7,  55,  31,   7,  40,  40,  32,  66,
																							80,  41,  88,  23,   7,   7,   0,   8,   8,  32,
																							16,  40,  16,  56,   0,  23,  62,  14,   0,  56,
																							0,  23,  62,  14,   0,  56,   0,  23,  62,  14,
																							0,   7,   8,  14,  14,  30,  56,  30,  56,  14,
																							56,   8,   7,  56,   8,   7,   1,   1,   7,  64,
																							40,  24,   7,  30,  24,   8,  14, 120,  78,  64,
																							8,  56,  48,  30,   7,  62,  38,  14,   7,   9,
																							48,   6,  24,  14,  56,  22,  30,  30,   1,   1,
																							1,   1,   1,   1,   1
																						},
																						{
																							14,  43,   8,  23,  49,   1,   1,   1,   1,   1,
																								19,   1,   1,   1,  19,  68,   8,   1,   1,   1,
																								1,   1,   1,   1,   1,   1,   1,   1,  14,  24,
																								24,   5,  23,  34,  24,  12,   1,   1,   1,   1,
																								1,   1,   8,   8,  10,  10,  16,   0,   7,  14,
																								0,  31,   7,  26,   5,  55,  31,   5,  44,  42,
																								32,  68,  10,  10,  16,   0,   7,  14,   0,  31,
																								7,  26,   5,  55,  31,   5,  44,  42,  32,  68,
																								82,  41,  88,  23,   5,   5,   0,  10,  10,  34,
																								16,  42,  16,  58,   0,  23,  60,  14,   0,  58,
																								0,  23,  60,  14,   0,  58,   0,  23,  60,  14,
																								0,   7,   8,  12,  12,  30,  56,  30,  56,  14,
																								56,   8,   5,  56,   8,   5,   1,   1,   7,  66,
																								40,  24,   7,  30,  24,   8,  14, 124,  78,  66,
																								8,  58,  48,  30,   7,  60,  38,  12,   7,  13,
																								48,   4,  24,  14,  56,  22,  30,  30,   1,   1,
																								1,   1,   1,   1,   1
																						},
																						{
																							14,  45,   8,  23,  49,   1,   1,   1,   1,   1,
																								19,   1,   1,   1,  19,  72,   8,   1,   1,   1,
																								1,   1,   1,   1,   1,   1,   1,   1,  14,  24,
																								24,   3,  23,  38,  24,  12,   1,   1,   1,   1,
																								1,   1,   8,   8,  12,  12,  18,   2,   7,  14,
																								2,  29,   7,  28,   3,  55,  29,   3,  48,  44,
																								34,  72,  12,  12,  18,   2,   7,  14,   2,  29,
																								7,  28,   3,  55,  29,   3,  48,  44,  34,  72,
																								86,  41,  88,  23,   3,   3,   2,  12,  12,  38,
																								18,  44,  18,  60,   2,  23,  60,  14,   2,  60,
																								2,  23,  60,  14,   2,  60,   2,  23,  60,  14,
																								2,   7,   8,  12,  12,  30,  56,  30,  56,  14,
																								56,   8,   3,  56,   8,   3,   1,   1,   7,  70,
																								40,  24,   7,  30,  24,   8,  14, 124,  78,  70,
																								8,  60,  50,  30,   7,  60,  38,  12,   7,  17,
																								50,   2,  24,  14,  56,  22,  30,  30,   1,   1,
																								1,   1,   1,   1,   1
																							},
																							{
																								14,  47,  10,  21,  49,   1,   1,   1,   1,   1,
																									21,   1,   1,   1,  21,  76,  10,   1,   1,   1,
																									1,   1,   1,   1,   1,   1,   1,   1,  14,  26,
																									26,   1,  21,  40,  26,  10,   1,   1,   1,   1,
																									1,   1,  10,  10,  14,  14,  20,   4,   5,  14,
																									4,  27,   5,  30,   1,  53,  27,   1,  50,  46,
																									36,  76,  14,  14,  20,   4,   5,  14,   4,  27,
																									5,  30,   1,  53,  27,   1,  50,  46,  36,  76,
																									88,  43,  90,  21,   1,   1,   4,  14,  14,  40,
																									20,  46,  20,  62,   4,  21,  58,  14,   4,  62,
																									4,  21,  58,  14,   4,  62,   4,  21,  58,  14,
																									4,   5,  10,  10,  10,  30,  58,  30,  58,  14,
																									58,  10,   1,  58,  10,   1,   1,   1,   5,  72,
																									42,  26,   5,  30,  26,  10,  14, 124,  78,  72,
																									10,  62,  52,  30,   5,  58,  36,  10,   5,  19,
																									52,   0,  26,  14,  58,  20,  30,  30,   1,   1,
																									1,   1,   1,   1,   1
																							},
																							{
																								14,  49,  10,  21,  49,   1,   1,   1,   1,   1,
																									21,   1,   1,   1,  21,  80,  10,   1,   1,   1,
																									1,   1,   1,   1,   1,   1,   1,   1,  14,  26,
																									26,   0,  21,  42,  26,  10,   1,   1,   1,   1,
																									1,   1,  10,  10,  16,  16,  20,   4,   5,  14,
																									4,  27,   5,  32,   0,  53,  27,   0,  54,  48,
																									36,  80,  16,  16,  20,   4,   5,  14,   4,  27,
																									5,  32,   0,  53,  27,   0,  54,  48,  36,  80,
																									90,  43,  90,  21,   0,   0,   4,  16,  16,  42,
																									20,  48,  20,  64,   4,  21,  58,  14,   4,  64,
																									4,  21,  58,  14,   4,  64,   4,  21,  58,  14,
																									4,   5,  10,  10,  10,  30,  58,  30,  58,  14,
																									58,  10,   0,  58,  10,   0,   1,   1,   5,  74,
																									42,  26,   5,  30,  26,  10,  14, 124,  78,  74,
																									10,  64,  52,  30,   5,  58,  36,  10,   5,  23,
																									52,   1,  26,  14,  58,  20,  30,  30,   1,   1,
																									1,   1,   1,   1,   1
																								},
																								{
																									14,  51,  10,  21,  49,   1,   1,   1,   1,   1,
																										23,   1,   1,   1,  23,  84,  10,   1,   1,   1,
																										1,   1,   1,   1,   1,   1,   1,   1,  14,  26,
																										26,   2,  21,  44,  26,   8,   1,   1,   1,   1,
																										1,   1,  10,  10,  18,  18,  22,   6,   5,  14,
																										6,  25,   5,  34,   2,  53,  25,   2,  56,  50,
																										38,  84,  18,  18,  22,   6,   5,  14,   6,  25,
																										5,  34,   2,  53,  25,   2,  56,  50,  38,  84,
																										92,  43,  90,  21,   2,   2,   6,  18,  18,  44,
																										22,  50,  22,  66,   6,  21,  56,  14,   6,  66,
																										6,  21,  56,  14,   6,  66,   6,  21,  56,  14,
																										6,   5,  10,   8,   8,  30,  58,  30,  58,  14,
																										58,  10,   2,  58,  10,   2,   1,   1,   5,  76,
																										42,  26,   5,  30,  26,  10,  14, 124,  78,  76,
																										10,  66,  54,  30,   5,  56,  36,   8,   5,  25,
																										54,   3,  26,  14,  58,  20,  30,  30,   1,   1,
																										1,   1,   1,   1,   1
																								},
																								{
																									14,  53,  12,  19,  49,   1,   1,   1,   1,   1,
																										25,   1,   1,   1,  25,  88,  12,   1,   1,   1,
																										1,   1,   1,   1,   1,   1,   1,   1,  14,  28,
																										28,   4,  19,  48,  28,   6,   1,   1,   1,   1,
																										1,   1,  12,  12,  20,  20,  24,   8,   3,  14,
																										8,  23,   3,  36,   4,  51,  23,   4,  60,  52,
																										40,  88,  20,  20,  24,   8,   3,  14,   8,  23,
																										3,  36,   4,  51,  23,   4,  60,  52,  40,  88,
																										96,  45,  92,  19,   4,   4,   8,  20,  20,  48,
																										24,  52,  24,  68,   8,  19,  54,  14,   8,  68,
																										8,  19,  54,  14,   8,  68,   8,  19,  54,  14,
																										8,   3,  12,   6,   6,  30,  60,  30,  60,  14,
																										60,  12,   4,  60,  12,   4,   1,   1,   3,  80,
																										44,  28,   3,  30,  28,  12,  14, 124,  78,  80,
																										12,  68,  56,  30,   3,  54,  34,   6,   3,  29,
																										56,   5,  28,  14,  60,  18,  30,  30,   1,   1,
																										1,   1,   1,   1,   1
																									},
																									{
																										14,  55,  12,  19,  49,   1,   1,   1,   1,   1,
																											25,   1,   1,   1,  25,  92,  12,   1,   1,   1,
																											1,   1,   1,   1,   1,   1,   1,   1,  14,  28,
																											28,   6,  19,  50,  28,   6,   1,   1,   1,   1,
																											1,   1,  12,  12,  22,  22,  24,   8,   3,  14,
																											8,  23,   3,  38,   6,  51,  23,   6,  62,  54,
																											40,  92,  22,  22,  24,   8,   3,  14,   8,  23,
																											3,  38,   6,  51,  23,   6,  62,  54,  40,  92,
																											98,  45,  92,  19,   6,   6,   8,  22,  22,  50,
																											24,  54,  24,  70,   8,  19,  54,  14,   8,  70,
																											8,  19,  54,  14,   8,  70,   8,  19,  54,  14,
																											8,   3,  12,   6,   6,  30,  60,  30,  60,  14,
																											60,  12,   6,  60,  12,   6,   1,   1,   3,  82,
																											44,  28,   3,  30,  28,  12,  14, 124,  78,  82,
																											12,  70,  56,  30,   3,  54,  34,   6,   3,  31,
																											56,   7,  28,  14,  60,  18,  30,  30,   1,   1,
																											1,   1,   1,   1,   1
																									},
																									{
																										14,  57,  12,  19,  49,   1,   1,   1,   1,   1,
																											27,   1,   1,   1,  27,  96,  12,   1,   1,   1,
																											1,   1,   1,   1,   1,   1,   1,   1,  14,  28,
																											28,   8,  19,  52,  28,   4,   1,   1,   1,   1,
																											1,   1,  12,  12,  24,  24,  26,  10,   3,  14,
																											10,  21,   3,  40,   8,  51,  21,   8,  66,  56,
																											42,  96,  24,  24,  26,  10,   3,  14,  10,  21,
																											3,  40,   8,  51,  21,   8,  66,  56,  42,  96,
																											100,  45,  92,  19,   8,   8,  10,  24,  24,  52,
																											26,  56,  26,  72,  10,  19,  52,  14,  10,  72,
																											10,  19,  52,  14,  10,  72,  10,  19,  52,  14,
																											10,   3,  12,   4,   4,  30,  60,  30,  60,  14,
																											60,  12,   8,  60,  12,   8,   1,   1,   3,  84,
																											44,  28,   3,  30,  28,  12,  14, 124,  78,  84,
																											12,  72,  58,  30,   3,  52,  34,   4,   3,  35,
																											58,   9,  28,  14,  60,  18,  30,  30,   1,   1,
																											1,   1,   1,   1,   1
																										},
																										{
																											14,  59,  12,  19,  49,   1,   1,   1,   1,   1,
																												29,   1,   1,   1,  29,  98,  12,   1,   1,   1,
																												1,   1,   1,   1,   1,   1,   1,   1,  14,  28,
																												28,   8,  19,  54,  28,   2,   1,   1,   1,   1,
																												1,   1,  12,  12,  24,  24,  26,  10,   3,  14,
																												10,  21,   3,  40,   8,  51,  21,   8,  68,  56,
																												42,  98,  24,  24,  26,  10,   3,  14,  10,  21,
																												3,  40,   8,  51,  21,   8,  68,  56,  42,  98,
																												102,  47,  92,  19,   8,   8,  10,  24,  24,  54,
																												26,  56,  26,  72,  10,  19,  50,  14,  10,  72,
																												10,  19,  50,  14,  10,  72,  10,  19,  50,  14,
																												10,   3,  12,   2,   2,  30,  60,  30,  60,  14,
																												60,  12,   8,  60,  12,   8,   1,   1,   3,  86,
																												44,  28,   3,  30,  28,  12,  14, 124,  78,  86,
																												12,  72,  58,  30,   3,  50,  32,   2,   3,  39,
																												58,  11,  28,  14,  60,  16,  30,  30,   1,   1,
																												1,   1,   1,   1,   1
																										},
																										{
																											14,  59,  14,  17,  49,   1,   1,   1,   1,   1,
																												29,   1,   1,   1,  29, 102,  14,   1,   1,   1,
																												1,   1,   1,   1,   1,   1,   1,   1,  14,  30,
																												30,  10,  17,  58,  30,   2,   1,   1,   1,   1,
																												1,   1,  14,  14,  26,  26,  28,  12,   1,  14,
																												12,  19,   1,  42,  10,  49,  19,  10,  72,  58,
																												44, 102,  26,  26,  28,  12,   1,  14,  12,  19,
																												1,  42,  10,  49,  19,  10,  72,  58,  44, 102,
																												106,  47,  94,  17,  10,  10,  12,  26,  26,  58,
																												28,  58,  28,  74,  12,  17,  50,  14,  12,  74,
																												12,  17,  50,  14,  12,  74,  12,  17,  50,  14,
																												12,   1,  14,   2,   2,  30,  62,  30,  62,  14,
																												62,  14,  10,  62,  14,  10,   1,   1,   1,  90,
																												46,  30,   1,  30,  30,  14,  14, 124,  78,  90,
																												14,  74,  60,  30,   1,  50,  32,   2,   1,  41,
																												60,  11,  30,  14,  62,  16,  30,  30,   1,   1,
																												1,   1,   1,   1,   1
																											},
																											{
																												14,  61,  14,  17,  49,   1,   1,   1,   1,   1,
																													31,   1,   1,   1,  31, 106,  14,   1,   1,   1,
																													1,   1,   1,   1,   1,   1,   1,   1,  14,  30,
																													30,  12,  17,  60,  30,   0,   1,   1,   1,   1,
																													1,   1,  14,  14,  28,  28,  30,  14,   1,  14,
																													14,  17,   1,  44,  12,  49,  17,  12,  76,  60,
																													46, 106,  28,  28,  30,  14,   1,  14,  14,  17,
																													1,  44,  12,  49,  17,  12,  76,  60,  46, 106,
																													108,  47,  94,  17,  12,  12,  14,  28,  28,  60,
																													30,  60,  30,  76,  14,  17,  48,  14,  14,  76,
																													14,  17,  48,  14,  14,  76,  14,  17,  48,  14,
																													14,   1,  14,   0,   0,  30,  62,  30,  62,  14,
																													62,  14,  12,  62,  14,  12,   1,   1,   1,  92,
																													46,  30,   1,  30,  30,  14,  14, 124,  78,  92,
																													14,  76,  62,  30,   1,  48,  32,   0,   1,  45,
																													62,  13,  30,  14,  62,  16,  30,  30,   1,   1,
																													1,   1,   1,   1,   1
																											},
																											{
																												14,  63,  14,  17,  49,   1,   1,   1,   1,   1,
																													31,   1,   1,   1,  31, 110,  14,   1,   1,   1,
																													1,   1,   1,   1,   1,   1,   1,   1,  14,  30,
																													30,  14,  17,  62,  30,   0,   1,   1,   1,   1,
																													1,   1,  14,  14,  30,  30,  30,  14,   1,  14,
																													14,  17,   1,  46,  14,  49,  17,  14,  78,  62,
																													46, 110,  30,  30,  30,  14,   1,  14,  14,  17,
																													1,  46,  14,  49,  17,  14,  78,  62,  46, 110,
																													110,  47,  94,  17,  14,  14,  14,  30,  30,  62,
																													30,  62,  30,  78,  14,  17,  48,  14,  14,  78,
																													14,  17,  48,  14,  14,  78,  14,  17,  48,  14,
																													14,   1,  14,   0,   0,  30,  62,  30,  62,  14,
																													62,  14,  14,  62,  14,  14,   1,   1,   1,  94,
																													46,  30,   1,  30,  30,  14,  14, 124,  78,  94,
																													14,  78,  62,  30,   1,  48,  32,   0,   1,  47,
																													62,  15,  30,  14,  62,  16,  30,  30,   1,   1,
																													1,   1,   1,   1,   1
																												},
	},
	{
		{
			14,  14,  17,  17,  65,   1,  78,  14,  14,  78,
				1,  17,   1,   1,   1,  30,  17,  81,  65,   1,
				81,  81,  81,  81,  81,  14,  14,  30,  33,   1,
				65,  14,  81,  78,  17,  46,   1,  33,  62,   1,
				1,   1,  17,  17,  49,  65,  65,  65,  81,  81,
				49,  81,  65,  65,  65,  81,  81,  81,  65,  33,
				17,  33,  49,  65,  65,  65,  81,  81,  49,  81,
				65,  65,  65,  81,  81,  81,  65,  33,  17,  33,
				14,  33,  49,   1,  17,   1,  17,  14,  17,  17,
				17,  81,  14,  62,  46,  33,  30,  14,   1,  62,
				46,  33,  30,  14,   1,  62,  46,  33,  30,  14,
				1,   1,  14,  17,  17,  17,  14,  17,  14,  46,
				46,  46,  33,  46,  46,  33,   1,   1,   1,  94,
				94,  46,   1,  30,  46,  62,  62,  62,  78,  30,
				14,  14,  30,  14,  14, 124,  62,  46,   1,  46,
				14,  62,  17,  46,  17,   1,  17,  46,   1,   1,
				1,   1,   1,   1,   1
		},
		{
			14,  14,  15,  15,  63,   1,  78,  14,  14,  78,
				1,  15,   1,   1,   1,  30,  15,  77,  63,   0,
				77,  77,  77,  75,  75,  14,  14,  30,  31,   0,
				61,  14,  79,  78,  15,  46,   1,  31,  62,   1,
				1,   1,  15,  15,  47,  63,  61,  63,  77,  77,
				47,  79,  63,  61,  63,  79,  79,  77,  61,  31,
				15,  31,  47,  63,  61,  63,  77,  77,  47,  79,
				63,  61,  63,  79,  79,  77,  61,  31,  15,  31,
				16,  31,  45,   1,  17,   1,  15,  14,  15,  15,
				15,  77,  14,  62,  46,  31,  32,  14,   1,  62,
				46,  31,  32,  14,   1,  62,  46,  31,  32,  14,
				1,   1,  14,  15,  15,  15,  16,  15,  16,  46,
				46,  46,  31,  46,  46,  31,   1,   1,   1,  94,
				94,  46,   1,  30,  46,  62,  62,  64,  78,  32,
				14,  16,  32,  16,  14, 124,  62,  46,   1,  46,
				16,  62,  15,  46,  13,   0,  15,  46,   1,   1,
				1,   1,   1,   1,   1
			},
			{
				14,  12,  13,  15,  61,   1,  76,  12,  12,  78,
					1,  15,   1,   1,   1,  30,  15,  73,  61,   2,
					75,  73,  73,  71,  71,  14,  14,  30,  29,   0,
					59,  14,  77,  78,  13,  46,   1,  31,  60,   1,
					1,   1,  15,  15,  45,  61,  59,  61,  75,  73,
					45,  77,  61,  57,  61,  77,  77,  75,  59,  29,
					13,  29,  45,  61,  59,  61,  75,  73,  45,  77,
					61,  57,  61,  77,  77,  75,  59,  29,  13,  29,
					18,  31,  41,   1,  17,   1,  15,  14,  15,  13,
					13,  73,  14,  62,  44,  31,  32,  14,   1,  62,
					44,  31,  32,  14,   1,  62,  44,  31,  32,  14,
					1,   1,  14,  13,  13,  13,  18,  13,  18,  46,
					46,  44,  31,  46,  44,  31,   1,   1,   1,  92,
					92,  46,   1,  30,  46,  60,  60,  64,  78,  32,
					14,  18,  32,  16,  14, 124,  62,  46,   1,  46,
					16,  60,  13,  46,  11,   2,  13,  46,   1,   1,
					1,   1,   1,   1,   1
			},
			{
				14,  12,  11,  15,  61,   1,  74,  12,  10,  78,
					1,  15,   1,   1,   1,  30,  15,  71,  59,   2,
					73,  71,  69,  65,  65,  14,  14,  30,  29,   0,
					57,  14,  75,  78,  11,  46,   1,  31,  58,   1,
					1,   1,  15,  15,  45,  59,  57,  59,  73,  71,
					45,  75,  59,  55,  59,  75,  75,  73,  57,  27,
					13,  27,  45,  59,  57,  59,  73,  71,  45,  75,
					59,  55,  59,  75,  75,  73,  57,  27,  13,  27,
					18,  31,  37,   1,  17,   1,  15,  14,  15,  13,
					13,  69,  14,  62,  44,  31,  32,  14,   1,  62,
					44,  31,  32,  14,   1,  62,  44,  31,  32,  14,
					1,   1,  14,  13,  13,  11,  18,  11,  18,  46,
					46,  44,  31,  46,  44,  31,   1,   1,   1,  90,
					90,  46,   1,  30,  46,  60,  60,  64,  78,  32,
					14,  18,  32,  16,  14, 122,  62,  46,   1,  46,
					16,  60,  11,  46,   9,   2,  11,  46,   1,   1,
					1,   1,   1,   1,   1
				},
				{
					14,  10,   9,  13,  59,   1,  72,  10,   8,  78,
						1,  13,   1,   1,   1,  30,  13,  67,  57,   4,
						71,  67,  65,  61,  61,  14,  14,  28,  27,   2,
						55,  14,  73,  78,   9,  44,   1,  29,  56,   1,
						1,   1,  13,  13,  43,  57,  55,  57,  71,  67,
						43,  73,  57,  51,  57,  73,  73,  71,  55,  25,
						11,  25,  43,  57,  55,  57,  71,  67,  43,  73,
						57,  51,  57,  73,  73,  71,  55,  25,  11,  25,
						20,  29,  33,   1,  17,   1,  13,  14,  13,  11,
						11,  65,  14,  60,  42,  29,  34,  14,   1,  60,
						42,  29,  34,  14,   1,  60,  42,  29,  34,  14,
						1,   3,  14,  11,  11,   9,  20,   9,  20,  44,
						46,  42,  29,  46,  42,  29,   1,   1,   1,  88,
						88,  44,   1,  30,  44,  58,  58,  66,  78,  34,
						14,  20,  34,  18,  12, 120,  60,  44,   1,  44,
						18,  58,   9,  44,   7,   4,   9,  44,   1,   1,
						1,   1,   1,   1,   1
				},
				{
					14,   8,   7,  13,  57,   1,  70,   8,   6,  78,
						1,  13,   1,   1,   1,  30,  13,  65,  55,   6,
						67,  65,  61,  55,  55,  14,  14,  28,  25,   2,
						51,  14,  71,  78,   7,  44,   1,  29,  54,   1,
						1,   1,  13,  13,  41,  55,  51,  55,  67,  65,
						41,  71,  55,  49,  55,  71,  71,  67,  51,  23,
						9,  23,  41,  55,  51,  55,  67,  65,  41,  71,
						55,  49,  55,  71,  71,  67,  51,  23,   9,  23,
						22,  29,  29,   1,  17,   1,  13,  14,  13,   9,
						9,  61,  14,  60,  40,  29,  34,  14,   1,  60,
						40,  29,  34,  14,   1,  60,  40,  29,  34,  14,
						1,   3,  14,   9,   9,   7,  22,   7,  22,  44,
						46,  40,  29,  46,  40,  29,   1,   1,   1,  86,
						86,  44,   1,  30,  44,  56,  56,  66,  78,  34,
						14,  22,  34,  18,  12, 118,  60,  44,   1,  44,
						18,  56,   7,  44,   3,   6,   7,  44,   1,   1,
						1,   1,   1,   1,   1
					},
					{
						14,   8,   5,  13,  57,   1,  68,   8,   4,  78,
							1,  13,   1,   1,   1,  30,  13,  61,  53,   6,
							65,  61,  57,  51,  51,  14,  14,  28,  25,   2,
							49,  14,  69,  78,   5,  44,   1,  29,  52,   1,
							1,   1,  13,  13,  41,  53,  49,  53,  65,  61,
							41,  69,  53,  45,  53,  69,  69,  65,  49,  21,
							9,  21,  41,  53,  49,  53,  65,  61,  41,  69,
							53,  45,  53,  69,  69,  65,  49,  21,   9,  21,
							22,  29,  25,   1,  17,   1,  13,  14,  13,   9,
							9,  57,  14,  60,  40,  29,  34,  14,   1,  60,
							40,  29,  34,  14,   1,  60,  40,  29,  34,  14,
							1,   3,  14,   9,   9,   5,  22,   5,  22,  44,
							46,  40,  29,  46,  40,  29,   1,   1,   1,  84,
							84,  44,   1,  30,  44,  56,  56,  66,  78,  34,
							14,  22,  34,  18,  12, 116,  60,  44,   1,  44,
							18,  56,   5,  44,   1,   6,   5,  44,   1,   1,
							1,   1,   1,   1,   1
					},
					{
						14,   6,   3,  11,  55,   1,  66,   6,   2,  78,
							1,  11,   1,   1,   1,  30,  11,  59,  51,   8,
							63,  59,  53,  45,  45,  14,  14,  26,  23,   4,
							47,  14,  67,  78,   3,  42,   1,  27,  50,   1,
							1,   1,  11,  11,  39,  51,  47,  51,  63,  59,
							39,  67,  51,  43,  51,  67,  67,  63,  47,  19,
							7,  19,  39,  51,  47,  51,  63,  59,  39,  67,
							51,  43,  51,  67,  67,  63,  47,  19,   7,  19,
							24,  27,  21,   1,  17,   1,  11,  14,  11,   7,
							7,  53,  14,  58,  38,  27,  36,  14,   1,  58,
							38,  27,  36,  14,   1,  58,  38,  27,  36,  14,
							1,   5,  14,   7,   7,   3,  24,   3,  24,  42,
							46,  38,  27,  46,  38,  27,   1,   1,   1,  82,
							82,  42,   1,  30,  42,  54,  54,  68,  78,  36,
							14,  24,  36,  20,  10, 114,  58,  42,   1,  42,
							20,  54,   3,  42,   0,   8,   3,  42,   1,   1,
							1,   1,   1,   1,   1
						},
						{
							14,   4,   1,  11,  55,   1,  64,   4,   0,  78,
								1,  11,   1,   1,   1,  30,  11,  55,  49,   8,
								61,  55,  51,  41,  41,  14,  14,  26,  23,   4,
								45,  14,  65,  78,   1,  42,   1,  27,  48,   1,
								1,   1,  11,  11,  39,  49,  45,  49,  61,  55,
								39,  65,  49,  39,  49,  65,  65,  61,  45,  17,
								7,  17,  39,  49,  45,  49,  61,  55,  39,  65,
								49,  39,  49,  65,  65,  61,  45,  17,   7,  17,
								24,  27,  19,   1,  17,   1,  11,  14,  11,   7,
								7,  51,  14,  58,  36,  27,  36,  14,   1,  58,
								36,  27,  36,  14,   1,  58,  36,  27,  36,  14,
								1,   5,  14,   7,   7,   1,  24,   1,  24,  42,
								46,  36,  27,  46,  36,  27,   1,   1,   1,  80,
								80,  42,   1,  30,  42,  52,  52,  68,  78,  36,
								14,  24,  36,  20,  10, 112,  58,  42,   1,  42,
								20,  52,   1,  42,   2,   8,   1,  42,   1,   1,
								1,   1,   1,   1,   1
						},
						{
							14,   4,   0,  11,  53,   1,  62,   4,   1,  78,
								1,  11,   1,   1,   1,  30,  11,  51,  47,  10,
								57,  51,  47,  35,  35,  14,  14,  26,  21,   4,
								41,  14,  63,  78,   0,  42,   1,  27,  46,   1,
								1,   1,  11,  11,  37,  47,  41,  47,  57,  51,
								37,  63,  47,  35,  47,  63,  63,  57,  41,  15,
								5,  15,  37,  47,  41,  47,  57,  51,  37,  63,
								47,  35,  47,  63,  63,  57,  41,  15,   5,  15,
								26,  27,  15,   1,  17,   1,  11,  14,  11,   5,
								5,  47,  14,  58,  36,  27,  36,  14,   1,  58,
								36,  27,  36,  14,   1,  58,  36,  27,  36,  14,
								1,   5,  14,   5,   5,   0,  26,   0,  26,  42,
								46,  36,  27,  46,  36,  27,   1,   1,   1,  78,
								78,  42,   1,  30,  42,  52,  52,  68,  78,  36,
								14,  26,  36,  20,  10, 110,  58,  42,   1,  42,
								20,  52,   0,  42,   6,  10,   0,  42,   1,   1,
								1,   1,   1,   1,   1
							},
							{
								14,   2,   2,   9,  51,   1,  60,   2,   3,  78,
									1,   9,   1,   1,   1,  30,   9,  49,  45,  12,
									55,  49,  43,  31,  31,  14,  14,  24,  19,   6,
									39,  14,  61,  78,   2,  40,   1,  25,  44,   1,
									1,   1,   9,   9,  35,  45,  39,  45,  55,  49,
									35,  61,  45,  33,  45,  61,  61,  55,  39,  13,
									3,  13,  35,  45,  39,  45,  55,  49,  35,  61,
									45,  33,  45,  61,  61,  55,  39,  13,   3,  13,
									28,  25,  11,   1,  17,   1,   9,  14,   9,   3,
									3,  43,  14,  56,  34,  25,  38,  14,   1,  56,
									34,  25,  38,  14,   1,  56,  34,  25,  38,  14,
									1,   7,  14,   3,   3,   2,  28,   2,  28,  40,
									46,  34,  25,  46,  34,  25,   1,   1,   1,  76,
									76,  40,   1,  30,  40,  50,  50,  70,  78,  38,
									14,  28,  38,  22,   8, 108,  56,  40,   1,  40,
									22,  50,   2,  40,   8,  12,   2,  40,   1,   1,
									1,   1,   1,   1,   1
							},
							{
								14,   2,   4,   9,  51,   1,  58,   2,   5,  78,
									1,   9,   1,   1,   1,  30,   9,  45,  43,  12,
									53,  45,  39,  25,  25,  14,  14,  24,  19,   6,
									37,  14,  59,  78,   4,  40,   1,  25,  42,   1,
									1,   1,   9,   9,  35,  43,  37,  43,  53,  45,
									35,  59,  43,  29,  43,  59,  59,  53,  37,  11,
									3,  11,  35,  43,  37,  43,  53,  45,  35,  59,
									43,  29,  43,  59,  59,  53,  37,  11,   3,  11,
									28,  25,   7,   1,  17,   1,   9,  14,   9,   3,
									3,  39,  14,  56,  34,  25,  38,  14,   1,  56,
									34,  25,  38,  14,   1,  56,  34,  25,  38,  14,
									1,   7,  14,   3,   3,   4,  28,   4,  28,  40,
									46,  34,  25,  46,  34,  25,   1,   1,   1,  74,
									74,  40,   1,  30,  40,  50,  50,  70,  78,  38,
									14,  28,  38,  22,   8, 106,  56,  40,   1,  40,
									22,  50,   4,  40,  10,  12,   4,  40,   1,   1,
									1,   1,   1,   1,   1
								},
								{
									14,   0,   6,   9,  49,   1,  56,   0,   7,  78,
										1,   9,   1,   1,   1,  30,   9,  43,  41,  14,
										51,  43,  35,  21,  21,  14,  14,  24,  17,   6,
										35,  14,  57,  78,   6,  40,   1,  25,  40,   1,
										1,   1,   9,   9,  33,  41,  35,  41,  51,  43,
										33,  57,  41,  27,  41,  57,  57,  51,  35,   9,
										1,   9,  33,  41,  35,  41,  51,  43,  33,  57,
										41,  27,  41,  57,  57,  51,  35,   9,   1,   9,
										30,  25,   3,   1,  17,   1,   9,  14,   9,   1,
										1,  35,  14,  56,  32,  25,  38,  14,   1,  56,
										32,  25,  38,  14,   1,  56,  32,  25,  38,  14,
										1,   7,  14,   1,   1,   6,  30,   6,  30,  40,
										46,  32,  25,  46,  32,  25,   1,   1,   1,  72,
										72,  40,   1,  30,  40,  48,  48,  70,  78,  38,
										14,  30,  38,  22,   8, 104,  56,  40,   1,  40,
										22,  48,   6,  40,  12,  14,   6,  40,   1,   1,
										1,   1,   1,   1,   1
								},
								{
									14,   1,   8,   7,  47,   1,  54,   1,   9,  78,
										1,   7,   1,   1,   1,  30,   7,  39,  39,  16,
										47,  39,  31,  15,  15,  14,  14,  22,  15,   8,
										31,  14,  55,  78,   8,  38,   1,  23,  38,   1,
										1,   1,   7,   7,  31,  39,  31,  39,  47,  39,
										31,  55,  39,  23,  39,  55,  55,  47,  31,   7,
										0,   7,  31,  39,  31,  39,  47,  39,  31,  55,
										39,  23,  39,  55,  55,  47,  31,   7,   0,   7,
										32,  23,   0,   1,  17,   1,   7,  14,   7,   0,
										0,  31,  14,  54,  30,  23,  40,  14,   1,  54,
										30,  23,  40,  14,   1,  54,  30,  23,  40,  14,
										1,   9,  14,   0,   0,   8,  32,   8,  32,  38,
										46,  30,  23,  46,  30,  23,   1,   1,   1,  70,
										70,  38,   1,  30,  38,  46,  46,  72,  78,  40,
										14,  32,  40,  24,   6, 102,  54,  38,   1,  38,
										24,  46,   8,  38,  16,  16,   8,  38,   1,   1,
										1,   1,   1,   1,   1
									},
									{
										14,   1,  10,   7,  47,   1,  52,   1,  11,  78,
											1,   7,   1,   1,   1,  30,   7,  37,  37,  16,
											45,  37,  27,  11,  11,  14,  14,  22,  15,   8,
											29,  14,  53,  78,  10,  38,   1,  23,  36,   1,
											1,   1,   7,   7,  31,  37,  29,  37,  45,  37,
											31,  53,  37,  21,  37,  53,  53,  45,  29,   5,
											0,   5,  31,  37,  29,  37,  45,  37,  31,  53,
											37,  21,  37,  53,  53,  45,  29,   5,   0,   5,
											32,  23,   4,   1,  17,   1,   7,  14,   7,   0,
											0,  27,  14,  54,  30,  23,  40,  14,   1,  54,
											30,  23,  40,  14,   1,  54,  30,  23,  40,  14,
											1,   9,  14,   0,   0,  10,  32,  10,  32,  38,
											46,  30,  23,  46,  30,  23,   1,   1,   1,  68,
											68,  38,   1,  30,  38,  46,  46,  72,  78,  40,
											14,  32,  40,  24,   6, 100,  54,  38,   1,  38,
											24,  46,  10,  38,  18,  16,  10,  38,   1,   1,
											1,   1,   1,   1,   1
									},
									{
										14,   3,  12,   7,  45,   1,  50,   3,  13,  78,
											1,   7,   1,   1,   1,  30,   7,  33,  35,  18,
											43,  33,  23,   5,   5,  14,  14,  22,  13,   8,
											27,  14,  51,  78,  12,  38,   1,  23,  34,   1,
											1,   1,   7,   7,  29,  35,  27,  35,  43,  33,
											29,  51,  35,  17,  35,  51,  51,  43,  27,   3,
											2,   3,  29,  35,  27,  35,  43,  33,  29,  51,
											35,  17,  35,  51,  51,  43,  27,   3,   2,   3,
											34,  23,   8,   1,  17,   1,   7,  14,   7,   2,
											2,  23,  14,  54,  28,  23,  40,  14,   1,  54,
											28,  23,  40,  14,   1,  54,  28,  23,  40,  14,
											1,   9,  14,   2,   2,  12,  34,  12,  34,  38,
											46,  28,  23,  46,  28,  23,   1,   1,   1,  66,
											66,  38,   1,  30,  38,  44,  44,  72,  78,  40,
											14,  34,  40,  24,   6,  98,  54,  38,   1,  38,
											24,  44,  12,  38,  20,  18,  12,  38,   1,   1,
											1,   1,   1,   1,   1
										},
										{
											14,   5,  12,   7,  45,   1,  48,   5,  15,  78,
												1,   7,   1,   1,   1,  30,   7,  31,  35,  18,
												41,  31,  21,   1,   1,  14,  14,  20,  13,   8,
												25,  14,  51,  78,  12,  36,   1,  23,  32,   1,
												1,   1,   7,   7,  29,  35,  25,  35,  41,  31,
												29,  51,  35,  15,  35,  51,  51,  41,  25,   3,
												2,   3,  29,  35,  25,  35,  41,  31,  29,  51,
												35,  15,  35,  51,  51,  41,  25,   3,   2,   3,
												34,  23,  10,   1,  17,   1,   7,  14,   7,   2,
												2,  21,  14,  52,  26,  23,  40,  14,   1,  52,
												26,  23,  40,  14,   1,  52,  26,  23,  40,  14,
												1,  11,  14,   2,   2,  12,  34,  12,  34,  36,
												46,  26,  23,  46,  26,  23,   1,   1,   1,  64,
												64,  36,   1,  30,  36,  42,  42,  72,  78,  40,
												14,  34,  40,  24,   4,  96,  52,  36,   1,  36,
												24,  42,  12,  36,  22,  18,  12,  36,   1,   1,
												1,   1,   1,   1,   1
										},
										{
											14,   5,  14,   5,  43,   1,  48,   5,  15,  78,
												1,   5,   1,   1,   1,  30,   5,  27,  33,  20,
												37,  27,  17,   4,   4,  14,  14,  20,  11,  10,
												21,  14,  49,  78,  14,  36,   1,  21,  32,   1,
												1,   1,   5,   5,  27,  33,  21,  33,  37,  27,
												27,  49,  33,  11,  33,  49,  49,  37,  21,   1,
												4,   1,  27,  33,  21,  33,  37,  27,  27,  49,
												33,  11,  33,  49,  49,  37,  21,   1,   4,   1,
												36,  21,  14,   1,  17,   1,   5,  14,   5,   4,
												4,  17,  14,  52,  26,  21,  42,  14,   1,  52,
												26,  21,  42,  14,   1,  52,  26,  21,  42,  14,
												1,  11,  14,   4,   4,  14,  36,  14,  36,  36,
												46,  26,  21,  46,  26,  21,   1,   1,   1,  64,
												64,  36,   1,  30,  36,  42,  42,  74,  78,  42,
												14,  36,  42,  26,   4,  96,  52,  36,   1,  36,
												26,  42,  14,  36,  26,  20,  14,  36,   1,   1,
												1,   1,   1,   1,   1
											},
											{
												14,   7,  16,   5,  41,   1,  46,   7,  17,  78,
													1,   5,   1,   1,   1,  30,   5,  23,  31,  22,
													35,  23,  13,   8,   8,  14,  14,  20,   9,  10,
													19,  14,  47,  78,  16,  36,   1,  21,  30,   1,
													1,   1,   5,   5,  25,  31,  19,  31,  35,  23,
													25,  47,  31,   7,  31,  47,  47,  35,  19,   0,
													6,   0,  25,  31,  19,  31,  35,  23,  25,  47,
													31,   7,  31,  47,  47,  35,  19,   0,   6,   0,
													38,  21,  18,   1,  17,   1,   5,  14,   5,   6,
													6,  13,  14,  52,  24,  21,  42,  14,   1,  52,
													24,  21,  42,  14,   1,  52,  24,  21,  42,  14,
													1,  11,  14,   6,   6,  16,  38,  16,  38,  36,
													46,  24,  21,  46,  24,  21,   1,   1,   1,  62,
													62,  36,   1,  30,  36,  40,  40,  74,  78,  42,
													14,  38,  42,  26,   4,  94,  52,  36,   1,  36,
													26,  40,  16,  36,  28,  22,  16,  36,   1,   1,
													1,   1,   1,   1,   1
											},
											{
												14,   7,  18,   5,  41,   1,  44,   7,  19,  78,
													1,   5,   1,   1,   1,  30,   5,  21,  29,  22,
													33,  21,   9,  14,  14,  14,  14,  20,   9,  10,
													17,  14,  45,  78,  18,  36,   1,  21,  28,   1,
													1,   1,   5,   5,  25,  29,  17,  29,  33,  21,
													25,  45,  29,   5,  29,  45,  45,  33,  17,   2,
													6,   2,  25,  29,  17,  29,  33,  21,  25,  45,
													29,   5,  29,  45,  45,  33,  17,   2,   6,   2,
													38,  21,  22,   1,  17,   1,   5,  14,   5,   6,
													6,   9,  14,  52,  24,  21,  42,  14,   1,  52,
													24,  21,  42,  14,   1,  52,  24,  21,  42,  14,
													1,  11,  14,   6,   6,  18,  38,  18,  38,  36,
													46,  24,  21,  46,  24,  21,   1,   1,   1,  60,
													60,  36,   1,  30,  36,  40,  40,  74,  78,  42,
													14,  38,  42,  26,   4,  92,  52,  36,   1,  36,
													26,  40,  18,  36,  30,  22,  18,  36,   1,   1,
													1,   1,   1,   1,   1
												},
												{
													14,   9,  20,   3,  39,   1,  42,   9,  21,  78,
														1,   3,   1,   1,   1,  30,   3,  17,  27,  24,
														31,  17,   5,  18,  18,  14,  14,  18,   7,  12,
														15,  14,  43,  78,  20,  34,   1,  19,  26,   1,
														1,   1,   3,   3,  23,  27,  15,  27,  31,  17,
														23,  43,  27,   1,  27,  43,  43,  31,  15,   4,
														8,   4,  23,  27,  15,  27,  31,  17,  23,  43,
														27,   1,  27,  43,  43,  31,  15,   4,   8,   4,
														40,  19,  26,   1,  17,   1,   3,  14,   3,   8,
														8,   5,  14,  50,  22,  19,  44,  14,   1,  50,
														22,  19,  44,  14,   1,  50,  22,  19,  44,  14,
														1,  13,  14,   8,   8,  20,  40,  20,  40,  34,
														46,  22,  19,  46,  22,  19,   1,   1,   1,  58,
														58,  34,   1,  30,  34,  38,  38,  76,  78,  44,
														14,  40,  44,  28,   2,  90,  50,  34,   1,  34,
														28,  38,  20,  34,  32,  24,  20,  34,   1,   1,
														1,   1,   1,   1,   1
												},
												{
													14,  11,  22,   3,  37,   1,  40,  11,  23,  78,
														1,   3,   1,   1,   1,  30,   3,  15,  25,  26,
														27,  15,   1,  24,  24,  14,  14,  18,   5,  12,
														11,  14,  41,  78,  22,  34,   1,  19,  24,   1,
														1,   1,   3,   3,  21,  25,  11,  25,  27,  15,
														21,  41,  25,   0,  25,  41,  41,  27,  11,   6,
														10,   6,  21,  25,  11,  25,  27,  15,  21,  41,
														25,   0,  25,  41,  41,  27,  11,   6,  10,   6,
														42,  19,  30,   1,  17,   1,   3,  14,   3,  10,
														10,   1,  14,  50,  20,  19,  44,  14,   1,  50,
														20,  19,  44,  14,   1,  50,  20,  19,  44,  14,
														1,  13,  14,  10,  10,  22,  42,  22,  42,  34,
														46,  20,  19,  46,  20,  19,   1,   1,   1,  56,
														56,  34,   1,  30,  34,  36,  36,  76,  78,  44,
														14,  42,  44,  28,   2,  88,  50,  34,   1,  34,
														28,  36,  22,  34,  36,  26,  22,  34,   1,   1,
														1,   1,   1,   1,   1
													},
													{
														14,  11,  24,   3,  37,   1,  38,  11,  25,  78,
															1,   3,   1,   1,   1,  30,   3,  11,  23,  26,
															25,  11,   2,  28,  28,  14,  14,  18,   5,  12,
															9,  14,  39,  78,  24,  34,   1,  19,  22,   1,
															1,   1,   3,   3,  21,  23,   9,  23,  25,  11,
															21,  39,  23,   4,  23,  39,  39,  25,   9,   8,
															10,   8,  21,  23,   9,  23,  25,  11,  21,  39,
															23,   4,  23,  39,  39,  25,   9,   8,  10,   8,
															42,  19,  34,   1,  17,   1,   3,  14,   3,  10,
															10,   2,  14,  50,  20,  19,  44,  14,   1,  50,
															20,  19,  44,  14,   1,  50,  20,  19,  44,  14,
															1,  13,  14,  10,  10,  24,  42,  24,  42,  34,
															46,  20,  19,  46,  20,  19,   1,   1,   1,  54,
															54,  34,   1,  30,  34,  36,  36,  76,  78,  44,
															14,  42,  44,  28,   2,  86,  50,  34,   1,  34,
															28,  36,  24,  34,  38,  26,  24,  34,   1,   1,
															1,   1,   1,   1,   1
													},
													{
														14,  13,  26,   1,  35,   1,  36,  13,  27,  78,
															1,   1,   1,   1,   1,  30,   1,   9,  21,  28,
															23,   9,   6,  34,  34,  14,  14,  16,   3,  14,
															7,  14,  37,  78,  26,  32,   1,  17,  20,   1,
															1,   1,   1,   1,  19,  21,   7,  21,  23,   9,
															19,  37,  21,   6,  21,  37,  37,  23,   7,  10,
															12,  10,  19,  21,   7,  21,  23,   9,  19,  37,
															21,   6,  21,  37,  37,  23,   7,  10,  12,  10,
															44,  17,  38,   1,  17,   1,   1,  14,   1,  12,
															12,   6,  14,  48,  18,  17,  46,  14,   1,  48,
															18,  17,  46,  14,   1,  48,  18,  17,  46,  14,
															1,  15,  14,  12,  12,  26,  44,  26,  44,  32,
															46,  18,  17,  46,  18,  17,   1,   1,   1,  52,
															52,  32,   1,  30,  32,  34,  34,  78,  78,  46,
															14,  44,  46,  30,   0,  84,  48,  32,   1,  32,
															30,  34,  26,  32,  40,  28,  26,  32,   1,   1,
															1,   1,   1,   1,   1
														},
														{
															14,  15,  28,   1,  35,   1,  34,  15,  29,  78,
																1,   1,   1,   1,   1,  30,   1,   5,  19,  28,
																21,   5,   8,  38,  38,  14,  14,  16,   3,  14,
																5,  14,  35,  78,  28,  32,   1,  17,  18,   1,
																1,   1,   1,   1,  19,  19,   5,  19,  21,   5,
																19,  35,  19,  10,  19,  35,  35,  21,   5,  12,
																12,  12,  19,  19,   5,  19,  21,   5,  19,  35,
																19,  10,  19,  35,  35,  21,   5,  12,  12,  12,
																44,  17,  40,   1,  17,   1,   1,  14,   1,  12,
																12,   8,  14,  48,  16,  17,  46,  14,   1,  48,
																16,  17,  46,  14,   1,  48,  16,  17,  46,  14,
																1,  15,  14,  12,  12,  28,  44,  28,  44,  32,
																46,  16,  17,  46,  16,  17,   1,   1,   1,  50,
																50,  32,   1,  30,  32,  32,  32,  78,  78,  46,
																14,  44,  46,  30,   0,  82,  48,  32,   1,  32,
																30,  32,  28,  32,  42,  28,  28,  32,   1,   1,
																1,   1,   1,   1,   1
														},
														{
															14,  15,  30,   1,  33,   1,  32,  15,  31,  78,
																1,   1,   1,   1,   1,  30,   1,   1,  17,  30,
																17,   1,  12,  44,  44,  14,  14,  16,   1,  14,
																1,  14,  33,  78,  30,  32,   1,  17,  16,   1,
																1,   1,   1,   1,  17,  17,   1,  17,  17,   1,
																17,  33,  17,  14,  17,  33,  33,  17,   1,  14,
																14,  14,  17,  17,   1,  17,  17,   1,  17,  33,
																17,  14,  17,  33,  33,  17,   1,  14,  14,  14,
																46,  17,  44,   1,  17,   1,   1,  14,   1,  14,
																14,  12,  14,  48,  16,  17,  46,  14,   1,  48,
																16,  17,  46,  14,   1,  48,  16,  17,  46,  14,
																1,  15,  14,  14,  14,  30,  46,  30,  46,  32,
																46,  16,  17,  46,  16,  17,   1,   1,   1,  48,
																48,  32,   1,  30,  32,  32,  32,  78,  78,  46,
																14,  46,  46,  30,   0,  80,  48,  32,   1,  32,
																30,  32,  30,  32,  46,  30,  30,  32,   1,   1,
																1,   1,   1,   1,   1
															},
															{
																14,  17,  32,   0,  31,   1,  30,  17,  33,  78,
																	1,   0,   1,   1,   1,  30,   0,   0,  15,  32,
																	15,   0,  16,  48,  48,  14,  14,  14,   0,  16,
																	0,  14,  31,  78,  32,  30,   1,  15,  14,   1,
																	1,   1,   0,   0,  15,  15,   0,  15,  15,   0,
																	15,  31,  15,  16,  15,  31,  31,  15,   0,  16,
																	16,  16,  15,  15,   0,  15,  15,   0,  15,  31,
																	15,  16,  15,  31,  31,  15,   0,  16,  16,  16,
																	48,  15,  48,   1,  17,   1,   0,  14,   0,  16,
																	16,  16,  14,  46,  14,  15,  48,  14,   1,  46,
																	14,  15,  48,  14,   1,  46,  14,  15,  48,  14,
																	1,  17,  14,  16,  16,  32,  48,  32,  48,  30,
																	46,  14,  15,  46,  14,  15,   1,   1,   1,  46,
																	46,  30,   1,  30,  30,  30,  30,  80,  78,  48,
																	14,  48,  48,  32,   1,  78,  46,  30,   1,  30,
																	32,  30,  32,  30,  48,  32,  32,  30,   1,   1,
																	1,   1,   1,   1,   1
															},
															{
																14,  17,  34,   0,  31,   1,  28,  17,  35,  78,
																	1,   0,   1,   1,   1,  30,   0,   4,  13,  32,
																	13,   4,  20,  54,  54,  14,  14,  14,   0,  16,
																	2,  14,  29,  78,  34,  30,   1,  15,  12,   1,
																	1,   1,   0,   0,  15,  13,   2,  13,  13,   4,
																	15,  29,  13,  20,  13,  29,  29,  13,   2,  18,
																	16,  18,  15,  13,   2,  13,  13,   4,  15,  29,
																	13,  20,  13,  29,  29,  13,   2,  18,  16,  18,
																	48,  15,  52,   1,  17,   1,   0,  14,   0,  16,
																	16,  20,  14,  46,  14,  15,  48,  14,   1,  46,
																	14,  15,  48,  14,   1,  46,  14,  15,  48,  14,
																	1,  17,  14,  16,  16,  34,  48,  34,  48,  30,
																	46,  14,  15,  46,  14,  15,   1,   1,   1,  44,
																	44,  30,   1,  30,  30,  30,  30,  80,  78,  48,
																	14,  48,  48,  32,   1,  76,  46,  30,   1,  30,
																	32,  30,  34,  30,  50,  32,  34,  30,   1,   1,
																	1,   1,   1,   1,   1
																},
																{
																	14,  19,  36,   0,  29,   1,  26,  19,  37,  78,
																		1,   0,   1,   1,   1,  30,   0,   6,  11,  34,
																		11,   6,  24,  58,  58,  14,  14,  14,   2,  16,
																		4,  14,  27,  78,  36,  30,   1,  15,  10,   1,
																		1,   1,   0,   0,  13,  11,   4,  11,  11,   6,
																		13,  27,  11,  22,  11,  27,  27,  11,   4,  20,
																		18,  20,  13,  11,   4,  11,  11,   6,  13,  27,
																		11,  22,  11,  27,  27,  11,   4,  20,  18,  20,
																		50,  15,  56,   1,  17,   1,   0,  14,   0,  18,
																		18,  24,  14,  46,  12,  15,  48,  14,   1,  46,
																		12,  15,  48,  14,   1,  46,  12,  15,  48,  14,
																		1,  17,  14,  18,  18,  36,  50,  36,  50,  30,
																		46,  12,  15,  46,  12,  15,   1,   1,   1,  42,
																		42,  30,   1,  30,  30,  28,  28,  80,  78,  48,
																		14,  50,  48,  32,   1,  74,  46,  30,   1,  30,
																		32,  28,  36,  30,  52,  34,  36,  30,   1,   1,
																		1,   1,   1,   1,   1
																},
																{
																	14,  21,  38,   2,  27,   1,  24,  21,  39,  78,
																		1,   2,   1,   1,   1,  30,   2,  10,   9,  36,
																		7,  10,  28,  64,  64,  14,  14,  12,   4,  18,
																		8,  14,  25,  78,  38,  28,   1,  13,   8,   1,
																		1,   1,   2,   2,  11,   9,   8,   9,   7,  10,
																		11,  25,   9,  26,   9,  25,  25,   7,   8,  22,
																		20,  22,  11,   9,   8,   9,   7,  10,  11,  25,
																		9,  26,   9,  25,  25,   7,   8,  22,  20,  22,
																		52,  13,  60,   1,  17,   1,   2,  14,   2,  20,
																		20,  28,  14,  44,  10,  13,  50,  14,   1,  44,
																		10,  13,  50,  14,   1,  44,  10,  13,  50,  14,
																		1,  19,  14,  20,  20,  38,  52,  38,  52,  28,
																		46,  10,  13,  46,  10,  13,   1,   1,   1,  40,
																		40,  28,   1,  30,  28,  26,  26,  82,  78,  50,
																		14,  52,  50,  34,   3,  72,  44,  28,   1,  28,
																		34,  26,  38,  28,  56,  36,  38,  28,   1,   1,
																		1,   1,   1,   1,   1
																	},
																	{
																		14,  21,  40,   2,  27,   1,  22,  21,  41,  78,
																			1,   2,   1,   1,   1,  30,   2,  12,   7,  36,
																			5,  12,  32,  68,  68,  14,  14,  12,   4,  18,
																			10,  14,  23,  78,  40,  28,   1,  13,   6,   1,
																			1,   1,   2,   2,  11,   7,  10,   7,   5,  12,
																			11,  23,   7,  28,   7,  23,  23,   5,  10,  24,
																			20,  24,  11,   7,  10,   7,   5,  12,  11,  23,
																			7,  28,   7,  23,  23,   5,  10,  24,  20,  24,
																			52,  13,  64,   1,  17,   1,   2,  14,   2,  20,
																			20,  32,  14,  44,  10,  13,  50,  14,   1,  44,
																			10,  13,  50,  14,   1,  44,  10,  13,  50,  14,
																			1,  19,  14,  20,  20,  40,  52,  40,  52,  28,
																			46,  10,  13,  46,  10,  13,   1,   1,   1,  38,
																			38,  28,   1,  30,  28,  26,  26,  82,  78,  50,
																			14,  52,  50,  34,   3,  70,  44,  28,   1,  28,
																			34,  26,  40,  28,  58,  36,  40,  28,   1,   1,
																			1,   1,   1,   1,   1
																	},
																	{
																		14,  23,  42,   2,  25,   1,  20,  23,  43,  78,
																			1,   2,   1,   1,   1,  30,   2,  16,   5,  38,
																			3,  16,  36,  74,  74,  14,  14,  12,   6,  18,
																			12,  14,  21,  78,  42,  28,   1,  13,   4,   1,
																			1,   1,   2,   2,   9,   5,  12,   5,   3,  16,
																			9,  21,   5,  32,   5,  21,  21,   3,  12,  26,
																			22,  26,   9,   5,  12,   5,   3,  16,   9,  21,
																			5,  32,   5,  21,  21,   3,  12,  26,  22,  26,
																			54,  13,  68,   1,  17,   1,   2,  14,   2,  22,
																			22,  36,  14,  44,   8,  13,  50,  14,   1,  44,
																			8,  13,  50,  14,   1,  44,   8,  13,  50,  14,
																			1,  19,  14,  22,  22,  42,  54,  42,  54,  28,
																			46,   8,  13,  46,   8,  13,   1,   1,   1,  36,
																			36,  28,   1,  30,  28,  24,  24,  82,  78,  50,
																			14,  54,  50,  34,   3,  68,  44,  28,   1,  28,
																			34,  24,  42,  28,  60,  38,  42,  28,   1,   1,
																			1,   1,   1,   1,   1
																		},
																		{
																			14,  25,  42,   2,  25,   1,  18,  25,  45,  78,
																				1,   2,   1,   1,   1,  30,   2,  18,   5,  38,
																				1,  18,  38,  78,  78,  14,  14,  10,   6,  18,
																				14,  14,  21,  78,  42,  26,   1,  13,   2,   1,
																				1,   1,   2,   2,   9,   5,  14,   5,   1,  18,
																				9,  21,   5,  34,   5,  21,  21,   1,  14,  26,
																				22,  26,   9,   5,  14,   5,   1,  18,   9,  21,
																				5,  34,   5,  21,  21,   1,  14,  26,  22,  26,
																				54,  13,  70,   1,  17,   1,   2,  14,   2,  22,
																				22,  38,  14,  42,   6,  13,  50,  14,   1,  42,
																				6,  13,  50,  14,   1,  42,   6,  13,  50,  14,
																				1,  21,  14,  22,  22,  42,  54,  42,  54,  26,
																				46,   6,  13,  46,   6,  13,   1,   1,   1,  34,
																				34,  26,   1,  30,  26,  22,  22,  82,  78,  50,
																				14,  54,  50,  34,   5,  66,  42,  26,   1,  26,
																				34,  22,  42,  26,  62,  38,  42,  26,   1,   1,
																				1,   1,   1,   1,   1
																		},
																		{
																			14,  25,  44,   4,  23,   1,  18,  25,  45,  78,
																				1,   4,   1,   1,   1,  30,   4,  22,   3,  40,
																				2,  22,  42,  84,  84,  14,  14,  10,   8,  20,
																				18,  14,  19,  78,  44,  26,   1,  11,   2,   1,
																				1,   1,   4,   4,   7,   3,  18,   3,   2,  22,
																				7,  19,   3,  38,   3,  19,  19,   2,  18,  28,
																				24,  28,   7,   3,  18,   3,   2,  22,   7,  19,
																				3,  38,   3,  19,  19,   2,  18,  28,  24,  28,
																				56,  11,  74,   1,  17,   1,   4,  14,   4,  24,
																				24,  42,  14,  42,   6,  11,  52,  14,   1,  42,
																				6,  11,  52,  14,   1,  42,   6,  11,  52,  14,
																				1,  21,  14,  24,  24,  44,  56,  44,  56,  26,
																				46,   6,  11,  46,   6,  11,   1,   1,   1,  34,
																				34,  26,   1,  30,  26,  22,  22,  84,  78,  52,
																				14,  56,  52,  36,   5,  66,  42,  26,   1,  26,
																				36,  22,  44,  26,  66,  40,  44,  26,   1,   1,
																				1,   1,   1,   1,   1
																			},
																			{
																				14,  27,  46,   4,  21,   1,  16,  27,  47,  78,
																					1,   4,   1,   1,   1,  30,   4,  26,   1,  42,
																					4,  26,  46,  88,  88,  14,  14,  10,  10,  20,
																					20,  14,  17,  78,  46,  26,   1,  11,   0,   1,
																					1,   1,   4,   4,   5,   1,  20,   1,   4,  26,
																					5,  17,   1,  42,   1,  17,  17,   4,  20,  30,
																					26,  30,   5,   1,  20,   1,   4,  26,   5,  17,
																					1,  42,   1,  17,  17,   4,  20,  30,  26,  30,
																					58,  11,  78,   1,  17,   1,   4,  14,   4,  26,
																					26,  46,  14,  42,   4,  11,  52,  14,   1,  42,
																					4,  11,  52,  14,   1,  42,   4,  11,  52,  14,
																					1,  21,  14,  26,  26,  46,  58,  46,  58,  26,
																					46,   4,  11,  46,   4,  11,   1,   1,   1,  32,
																					32,  26,   1,  30,  26,  20,  20,  84,  78,  52,
																					14,  58,  52,  36,   5,  64,  42,  26,   1,  26,
																					36,  20,  46,  26,  68,  42,  46,  26,   1,   1,
																					1,   1,   1,   1,   1
																			},
																			{
																				14,  27,  48,   4,  21,   1,  14,  27,  49,  78,
																					1,   4,   1,   1,   1,  30,   4,  28,   0,  42,
																					6,  28,  50,  94,  94,  14,  14,  10,  10,  20,
																					22,  14,  15,  78,  48,  26,   1,  11,   1,   1,
																					1,   1,   4,   4,   5,   0,  22,   0,   6,  28,
																					5,  15,   0,  44,   0,  15,  15,   6,  22,  32,
																					26,  32,   5,   0,  22,   0,   6,  28,   5,  15,
																					0,  44,   0,  15,  15,   6,  22,  32,  26,  32,
																					58,  11,  82,   1,  17,   1,   4,  14,   4,  26,
																					26,  50,  14,  42,   4,  11,  52,  14,   1,  42,
																					4,  11,  52,  14,   1,  42,   4,  11,  52,  14,
																					1,  21,  14,  26,  26,  48,  58,  48,  58,  26,
																					46,   4,  11,  46,   4,  11,   1,   1,   1,  30,
																					30,  26,   1,  30,  26,  20,  20,  84,  78,  52,
																					14,  58,  52,  36,   5,  62,  42,  26,   1,  26,
																					36,  20,  48,  26,  70,  42,  48,  26,   1,   1,
																					1,   1,   1,   1,   1
																				},
																				{
																					14,  29,  50,   6,  19,   1,  12,  29,  51,  78,
																						1,   6,   1,   1,   1,  30,   6,  32,   2,  44,
																						8,  32,  54,  98,  98,  14,  14,   8,  12,  22,
																						24,  14,  13,  78,  50,  24,   1,   9,   3,   1,
																						1,   1,   6,   6,   3,   2,  24,   2,   8,  32,
																						3,  13,   2,  48,   2,  13,  13,   8,  24,  34,
																						28,  34,   3,   2,  24,   2,   8,  32,   3,  13,
																						2,  48,   2,  13,  13,   8,  24,  34,  28,  34,
																						60,   9,  86,   1,  17,   1,   6,  14,   6,  28,
																						28,  54,  14,  40,   2,   9,  54,  14,   1,  40,
																						2,   9,  54,  14,   1,  40,   2,   9,  54,  14,
																						1,  23,  14,  28,  28,  50,  60,  50,  60,  24,
																						46,   2,   9,  46,   2,   9,   1,   1,   1,  28,
																						28,  24,   1,  30,  24,  18,  18,  86,  78,  54,
																						14,  60,  54,  38,   7,  60,  40,  24,   1,  24,
																						38,  18,  50,  24,  72,  44,  50,  24,   1,   1,
																						1,   1,   1,   1,   1
																				},
																				{
																					14,  31,  52,   6,  17,   1,  10,  31,  53,  78,
																						1,   6,   1,   1,   1,  30,   6,  34,   4,  46,
																						12,  34,  58, 104, 104,  14,  14,   8,  14,  22,
																						28,  14,  11,  78,  52,  24,   1,   9,   5,   1,
																						1,   1,   6,   6,   1,   4,  28,   4,  12,  34,
																						1,  11,   4,  50,   4,  11,  11,  12,  28,  36,
																						30,  36,   1,   4,  28,   4,  12,  34,   1,  11,
																						4,  50,   4,  11,  11,  12,  28,  36,  30,  36,
																						62,   9,  90,   1,  17,   1,   6,  14,   6,  30,
																						30,  58,  14,  40,   0,   9,  54,  14,   1,  40,
																						0,   9,  54,  14,   1,  40,   0,   9,  54,  14,
																						1,  23,  14,  30,  30,  52,  62,  52,  62,  24,
																						46,   0,   9,  46,   0,   9,   1,   1,   1,  26,
																						26,  24,   1,  30,  24,  16,  16,  86,  78,  54,
																						14,  62,  54,  38,   7,  58,  40,  24,   1,  24,
																						38,  16,  52,  24,  76,  46,  52,  24,   1,   1,
																						1,   1,   1,   1,   1
																					},
																					{
																						14,  31,  54,   6,  17,   1,   8,  31,  55,  78,
																							1,   6,   1,   1,   1,  30,   6,  38,   6,  46,
																							14,  38,  62, 108, 108,  14,  14,   8,  14,  22,
																							30,  14,   9,  78,  54,  24,   1,   9,   7,   1,
																							1,   1,   6,   6,   1,   6,  30,   6,  14,  38,
																							1,   9,   6,  54,   6,   9,   9,  14,  30,  38,
																							30,  38,   1,   6,  30,   6,  14,  38,   1,   9,
																							6,  54,   6,   9,   9,  14,  30,  38,  30,  38,
																							62,   9,  94,   1,  17,   1,   6,  14,   6,  30,
																							30,  62,  14,  40,   0,   9,  54,  14,   1,  40,
																							0,   9,  54,  14,   1,  40,   0,   9,  54,  14,
																							1,  23,  14,  30,  30,  54,  62,  54,  62,  24,
																							46,   0,   9,  46,   0,   9,   1,   1,   1,  24,
																							24,  24,   1,  30,  24,  16,  16,  86,  78,  54,
																							14,  62,  54,  38,   7,  56,  40,  24,   1,  24,
																							38,  16,  54,  24,  78,  46,  54,  24,   1,   1,
																							1,   1,   1,   1,   1
																					},
																					{
																						14,  33,  56,   8,  15,   1,   6,  33,  57,  78,
																							1,   8,   1,   1,   1,  30,   8,  40,   8,  48,
																							16,  40,  66, 114, 114,  14,  14,   6,  16,  24,
																							32,  14,   7,  78,  56,  22,   1,   7,   9,   1,
																							1,   1,   8,   8,   0,   8,  32,   8,  16,  40,
																							0,   7,   8,  56,   8,   7,   7,  16,  32,  40,
																							32,  40,   0,   8,  32,   8,  16,  40,   0,   7,
																							8,  56,   8,   7,   7,  16,  32,  40,  32,  40,
																							64,   7,  98,   1,  17,   1,   8,  14,   8,  32,
																							32,  66,  14,  38,   1,   7,  56,  14,   1,  38,
																							1,   7,  56,  14,   1,  38,   1,   7,  56,  14,
																							1,  25,  14,  32,  32,  56,  64,  56,  64,  22,
																							46,   1,   7,  46,   1,   7,   1,   1,   1,  22,
																							22,  22,   1,  30,  22,  14,  14,  88,  78,  56,
																							14,  64,  56,  40,   9,  54,  38,  22,   1,  22,
																							40,  14,  56,  22,  80,  48,  56,  22,   1,   1,
																							1,   1,   1,   1,   1
																						},
																						{
																							14,  35,  58,   8,  15,   1,   4,  35,  59,  78,
																								1,   8,   1,   1,   1,  30,   8,  44,  10,  48,
																								18,  44,  68, 118, 118,  14,  14,   6,  16,  24,
																								34,  14,   5,  78,  58,  22,   1,   7,  11,   1,
																								1,   1,   8,   8,   0,  10,  34,  10,  18,  44,
																								0,   5,  10,  60,  10,   5,   5,  18,  34,  42,
																								32,  42,   0,  10,  34,  10,  18,  44,   0,   5,
																								10,  60,  10,   5,   5,  18,  34,  42,  32,  42,
																								64,   7, 100,   1,  17,   1,   8,  14,   8,  32,
																								32,  68,  14,  38,   3,   7,  56,  14,   1,  38,
																								3,   7,  56,  14,   1,  38,   3,   7,  56,  14,
																								1,  25,  14,  32,  32,  58,  64,  58,  64,  22,
																								46,   3,   7,  46,   3,   7,   1,   1,   1,  20,
																								20,  22,   1,  30,  22,  12,  12,  88,  78,  56,
																								14,  64,  56,  40,   9,  52,  38,  22,   1,  22,
																								40,  12,  58,  22,  82,  48,  58,  22,   1,   1,
																								1,   1,   1,   1,   1
																						},
																						{
																							14,  35,  60,   8,  13,   1,   2,  35,  61,  78,
																								1,   8,   1,   1,   1,  30,   8,  48,  12,  50,
																								22,  48,  72, 124, 124,  14,  14,   6,  18,  24,
																								38,  14,   3,  78,  60,  22,   1,   7,  13,   1,
																								1,   1,   8,   8,   2,  12,  38,  12,  22,  48,
																								2,   3,  12,  64,  12,   3,   3,  22,  38,  44,
																								34,  44,   2,  12,  38,  12,  22,  48,   2,   3,
																								12,  64,  12,   3,   3,  22,  38,  44,  34,  44,
																								66,   7, 104,   1,  17,   1,   8,  14,   8,  34,
																								34,  72,  14,  38,   3,   7,  56,  14,   1,  38,
																								3,   7,  56,  14,   1,  38,   3,   7,  56,  14,
																								1,  25,  14,  34,  34,  60,  66,  60,  66,  22,
																								46,   3,   7,  46,   3,   7,   1,   1,   1,  18,
																								18,  22,   1,  30,  22,  12,  12,  88,  78,  56,
																								14,  66,  56,  40,   9,  50,  38,  22,   1,  22,
																								40,  12,  60,  22,  86,  50,  60,  22,   1,   1,
																								1,   1,   1,   1,   1
																							},
																							{
																								14,  37,  62,  10,  11,   1,   0,  37,  63,  78,
																									1,  10,   1,   1,   1,  30,  10,  50,  14,  52,
																									24,  50,  76, 124, 124,  14,  14,   4,  20,  26,
																									40,  14,   1,  78,  62,  20,   1,   5,  15,   1,
																									1,   1,  10,  10,   4,  14,  40,  14,  24,  50,
																									4,   1,  14,  66,  14,   1,   1,  24,  40,  46,
																									36,  46,   4,  14,  40,  14,  24,  50,   4,   1,
																									14,  66,  14,   1,   1,  24,  40,  46,  36,  46,
																									68,   5, 108,   1,  17,   1,  10,  14,  10,  36,
																									36,  76,  14,  36,   5,   5,  58,  14,   1,  36,
																									5,   5,  58,  14,   1,  36,   5,   5,  58,  14,
																									1,  27,  14,  36,  36,  62,  68,  62,  68,  20,
																									46,   5,   5,  46,   5,   5,   1,   1,   1,  16,
																									16,  20,   1,  30,  20,  10,  10,  90,  78,  58,
																									14,  68,  58,  42,  11,  48,  36,  20,   1,  20,
																									42,  10,  62,  20,  88,  52,  62,  20,   1,   1,
																									1,   1,   1,   1,   1
																							},
																							{
																								14,  37,  64,  10,  11,   1,   1,  37,  65,  78,
																									1,  10,   1,   1,   1,  30,  10,  54,  16,  52,
																									26,  54,  80, 124, 124,  14,  14,   4,  20,  26,
																									42,  14,   0,  78,  64,  20,   1,   5,  17,   1,
																									1,   1,  10,  10,   4,  16,  42,  16,  26,  54,
																									4,   0,  16,  70,  16,   0,   0,  26,  42,  48,
																									36,  48,   4,  16,  42,  16,  26,  54,   4,   0,
																									16,  70,  16,   0,   0,  26,  42,  48,  36,  48,
																									68,   5, 112,   1,  17,   1,  10,  14,  10,  36,
																									36,  80,  14,  36,   5,   5,  58,  14,   1,  36,
																									5,   5,  58,  14,   1,  36,   5,   5,  58,  14,
																									1,  27,  14,  36,  36,  64,  68,  64,  68,  20,
																									46,   5,   5,  46,   5,   5,   1,   1,   1,  14,
																									14,  20,   1,  30,  20,  10,  10,  90,  78,  58,
																									14,  68,  58,  42,  11,  46,  36,  20,   1,  20,
																									42,  10,  64,  20,  90,  52,  64,  20,   1,   1,
																									1,   1,   1,   1,   1
																								},
																								{
																									14,  39,  66,  10,   9,   1,   3,  39,  67,  78,
																										1,  10,   1,   1,   1,  30,  10,  56,  18,  54,
																										28,  56,  84, 124, 124,  14,  14,   4,  22,  26,
																										44,  14,   2,  78,  66,  20,   1,   5,  19,   1,
																										1,   1,  10,  10,   6,  18,  44,  18,  28,  56,
																										6,   2,  18,  72,  18,   2,   2,  28,  44,  50,
																										38,  50,   6,  18,  44,  18,  28,  56,   6,   2,
																										18,  72,  18,   2,   2,  28,  44,  50,  38,  50,
																										70,   5, 116,   1,  17,   1,  10,  14,  10,  38,
																										38,  84,  14,  36,   7,   5,  58,  14,   1,  36,
																										7,   5,  58,  14,   1,  36,   7,   5,  58,  14,
																										1,  27,  14,  38,  38,  66,  70,  66,  70,  20,
																										46,   7,   5,  46,   7,   5,   1,   1,   1,  12,
																										12,  20,   1,  30,  20,   8,   8,  90,  78,  58,
																										14,  70,  58,  42,  11,  44,  36,  20,   1,  20,
																										42,   8,  66,  20,  92,  54,  66,  20,   1,   1,
																										1,   1,   1,   1,   1
																								},
																								{
																									14,  41,  68,  12,   7,   1,   5,  41,  69,  78,
																										1,  12,   1,   1,   1,  30,  12,  60,  20,  56,
																										32,  60,  88, 124, 124,  14,  14,   2,  24,  28,
																										48,  14,   4,  78,  68,  18,   1,   3,  21,   1,
																										1,   1,  12,  12,   8,  20,  48,  20,  32,  60,
																										8,   4,  20,  76,  20,   4,   4,  32,  48,  52,
																										40,  52,   8,  20,  48,  20,  32,  60,   8,   4,
																										20,  76,  20,   4,   4,  32,  48,  52,  40,  52,
																										72,   3, 120,   1,  17,   1,  12,  14,  12,  40,
																										40,  88,  14,  34,   9,   3,  60,  14,   1,  34,
																										9,   3,  60,  14,   1,  34,   9,   3,  60,  14,
																										1,  29,  14,  40,  40,  68,  72,  68,  72,  18,
																										46,   9,   3,  46,   9,   3,   1,   1,   1,  10,
																										10,  18,   1,  30,  18,   6,   6,  92,  78,  60,
																										14,  72,  60,  44,  13,  42,  34,  18,   1,  18,
																										44,   6,  68,  18,  96,  56,  68,  18,   1,   1,
																										1,   1,   1,   1,   1
																									},
																									{
																										14,  41,  70,  12,   7,   1,   7,  41,  71,  78,
																											1,  12,   1,   1,   1,  30,  12,  62,  22,  56,
																											34,  62,  92, 124, 124,  14,  14,   2,  24,  28,
																											50,  14,   6,  78,  70,  18,   1,   3,  23,   1,
																											1,   1,  12,  12,   8,  22,  50,  22,  34,  62,
																											8,   6,  22,  78,  22,   6,   6,  34,  50,  54,
																											40,  54,   8,  22,  50,  22,  34,  62,   8,   6,
																											22,  78,  22,   6,   6,  34,  50,  54,  40,  54,
																											72,   3, 124,   1,  17,   1,  12,  14,  12,  40,
																											40,  92,  14,  34,   9,   3,  60,  14,   1,  34,
																											9,   3,  60,  14,   1,  34,   9,   3,  60,  14,
																											1,  29,  14,  40,  40,  70,  72,  70,  72,  18,
																											46,   9,   3,  46,   9,   3,   1,   1,   1,   8,
																											8,  18,   1,  30,  18,   6,   6,  92,  78,  60,
																											14,  72,  60,  44,  13,  40,  34,  18,   1,  18,
																											44,   6,  70,  18,  98,  56,  70,  18,   1,   1,
																											1,   1,   1,   1,   1
																									},
																									{
																										14,  43,  72,  12,   5,   1,   9,  43,  73,  78,
																											1,  12,   1,   1,   1,  30,  12,  66,  24,  58,
																											36,  66,  96, 124, 124,  14,  14,   2,  26,  28,
																											52,  14,   8,  78,  72,  18,   1,   3,  25,   1,
																											1,   1,  12,  12,  10,  24,  52,  24,  36,  66,
																											10,   8,  24,  82,  24,   8,   8,  36,  52,  56,
																											42,  56,  10,  24,  52,  24,  36,  66,  10,   8,
																											24,  82,  24,   8,   8,  36,  52,  56,  42,  56,
																											74,   3, 124,   1,  17,   1,  12,  14,  12,  42,
																											42,  96,  14,  34,  11,   3,  60,  14,   1,  34,
																											11,   3,  60,  14,   1,  34,  11,   3,  60,  14,
																											1,  29,  14,  42,  42,  72,  74,  72,  74,  18,
																											46,  11,   3,  46,  11,   3,   1,   1,   1,   6,
																											6,  18,   1,  30,  18,   4,   4,  92,  78,  60,
																											14,  74,  60,  44,  13,  38,  34,  18,   1,  18,
																											44,   4,  72,  18, 100,  58,  72,  18,   1,   1,
																											1,   1,   1,   1,   1
																										},
																										{
																											14,  45,  72,  12,   5,   1,  11,  45,  75,  78,
																												1,  12,   1,   1,   1,  30,  12,  68,  24,  58,
																												38,  68,  98, 124, 124,  14,  14,   0,  26,  28,
																												54,  14,   8,  78,  72,  16,   1,   3,  27,   1,
																												1,   1,  12,  12,  10,  24,  54,  24,  38,  68,
																												10,   8,  24,  84,  24,   8,   8,  38,  54,  56,
																												42,  56,  10,  24,  54,  24,  38,  68,  10,   8,
																												24,  84,  24,   8,   8,  38,  54,  56,  42,  56,
																												74,   3, 124,   1,  17,   1,  12,  14,  12,  42,
																												42,  98,  14,  32,  13,   3,  60,  14,   1,  32,
																												13,   3,  60,  14,   1,  32,  13,   3,  60,  14,
																												1,  31,  14,  42,  42,  72,  74,  72,  74,  16,
																												46,  13,   3,  46,  13,   3,   1,   1,   1,   4,
																												4,  16,   1,  30,  16,   2,   2,  92,  78,  60,
																												14,  74,  60,  44,  15,  36,  32,  16,   1,  16,
																												44,   2,  72,  16, 102,  58,  72,  16,   1,   1,
																												1,   1,   1,   1,   1
																										},
																										{
																											14,  45,  74,  14,   3,   1,  11,  45,  75,  78,
																												1,  14,   1,   1,   1,  30,  14,  72,  26,  60,
																												42,  72, 102, 124, 124,  14,  14,   0,  28,  30,
																												58,  14,  10,  78,  74,  16,   1,   1,  27,   1,
																												1,   1,  14,  14,  12,  26,  58,  26,  42,  72,
																												12,  10,  26,  88,  26,  10,  10,  42,  58,  58,
																												44,  58,  12,  26,  58,  26,  42,  72,  12,  10,
																												26,  88,  26,  10,  10,  42,  58,  58,  44,  58,
																												76,   1, 124,   1,  17,   1,  14,  14,  14,  44,
																												44, 102,  14,  32,  13,   1,  62,  14,   1,  32,
																												13,   1,  62,  14,   1,  32,  13,   1,  62,  14,
																												1,  31,  14,  44,  44,  74,  76,  74,  76,  16,
																												46,  13,   1,  46,  13,   1,   1,   1,   1,   4,
																												4,  16,   1,  30,  16,   2,   2,  94,  78,  62,
																												14,  76,  62,  46,  15,  36,  32,  16,   1,  16,
																												46,   2,  74,  16, 106,  60,  74,  16,   1,   1,
																												1,   1,   1,   1,   1
																											},
																											{
																												14,  47,  76,  14,   1,   1,  13,  47,  77,  78,
																													1,  14,   1,   1,   1,  30,  14,  76,  28,  62,
																													44,  76, 106, 124, 124,  14,  14,   0,  30,  30,
																													60,  14,  12,  78,  76,  16,   1,   1,  29,   1,
																													1,   1,  14,  14,  14,  28,  60,  28,  44,  76,
																													14,  12,  28,  92,  28,  12,  12,  44,  60,  60,
																													46,  60,  14,  28,  60,  28,  44,  76,  14,  12,
																													28,  92,  28,  12,  12,  44,  60,  60,  46,  60,
																													78,   1, 124,   1,  17,   1,  14,  14,  14,  46,
																													46, 106,  14,  32,  15,   1,  62,  14,   1,  32,
																													15,   1,  62,  14,   1,  32,  15,   1,  62,  14,
																													1,  31,  14,  46,  46,  76,  78,  76,  78,  16,
																													46,  15,   1,  46,  15,   1,   1,   1,   1,   2,
																													2,  16,   1,  30,  16,   0,   0,  94,  78,  62,
																													14,  78,  62,  46,  15,  34,  32,  16,   1,  16,
																													46,   0,  76,  16, 108,  62,  76,  16,   1,   1,
																													1,   1,   1,   1,   1
																											},
																											{
																												14,  47,  78,  14,   1,   1,  15,  47,  79,  78,
																													1,  14,   1,   1,   1,  30,  14,  78,  30,  62,
																													46,  78, 110, 124, 124,  14,  14,   0,  30,  30,
																													62,  14,  14,  78,  78,  16,   1,   1,  31,   1,
																													1,   1,  14,  14,  14,  30,  62,  30,  46,  78,
																													14,  14,  30,  94,  30,  14,  14,  46,  62,  62,
																													46,  62,  14,  30,  62,  30,  46,  78,  14,  14,
																													30,  94,  30,  14,  14,  46,  62,  62,  46,  62,
																													78,   1, 124,   1,  17,   1,  14,  14,  14,  46,
																													46, 110,  14,  32,  15,   1,  62,  14,   1,  32,
																													15,   1,  62,  14,   1,  32,  15,   1,  62,  14,
																													1,  31,  14,  46,  46,  78,  78,  78,  78,  16,
																													46,  15,   1,  46,  15,   1,   1,   1,   1,   0,
																													0,  16,   1,  30,  16,   0,   0,  94,  78,  62,
																													14,  78,  62,  46,  15,  32,  32,  16,   1,  16,
																													46,   0,  78,  16, 110,  62,  78,  16,   1,   1,
																													1,   1,   1,   1,   1
																												},
																												},
																												{
																													{
																														14, 124,  17,  17,  65,   1,  78,  14,  14,  62,
																															1,  17,   1,   1,  46,  30,  17,  81,   1,  14,
																															81,  81,  81,  81,  81,  14,  14,  30, 124,  46,
																															1,  14,  81,  78,  33,  46,   1,  14,  62,   1,
																															1,   1,  17,  17,  49,  65,  33,  65,  81,  65,
																															49,  81,  81,  81,  49,  65,  81,  81,  81,  33,
																															17,  49,  49,  65,  33,  65,  81,  65,  49,  81,
																															81,  81,  49,  65,  81,  81,  81,  33,  17,  49,
																															14,  33,  49,   1,   1,   1,  17,  14,  17,  17,
																															17,  81,  33,  62,  46,  33,  30,  14,   1,  62,
																															46,  33,  30,  14,   1,  62,  46,  33,  30,  14,
																															1,   1,  14,   1,   1,   1,  14,   1,  14,  46,
																															46,  46,  33,  46,  46,  33,   1,   1,   1,  94,
																															46,  46,   1,  30,  46,  62,  62,  62,  78,  30,
																															14,  14,  30,   1,  14, 124,  62,  46,   1,  30,
																															46,  62,  17,  46,  17,  17,  17,  46,   1,   1,
																															1,   1,   1,   1,   1
																													},
																													{
																														14, 124,  15,  15,  63,   1,  78,  14,  14,  64,
																															1,  15,   1,   1,  46,  30,  15,  77,   1,  16,
																															77,  77,  77,  75,  75,  14,  14,  30, 124,  46,
																															0,  14,  79,  78,  29,  46,   1,  14,  62,   1,
																															1,   1,  15,  15,  47,  63,  31,  63,  77,  61,
																															47,  79,  79,  77,  47,  63,  79,  79,  77,  31,
																															15,  45,  47,  63,  31,  63,  77,  61,  47,  79,
																															79,  77,  47,  63,  79,  79,  77,  31,  15,  45,
																															16,  31,  45,   1,   1,   1,  15,  14,  15,  15,
																															15,  77,  31,  62,  46,  31,  32,  14,   1,  62,
																															46,  31,  32,  14,   1,  62,  46,  31,  32,  14,
																															1,   1,  14,   0,   0,   0,  16,   0,  16,  46,
																															46,  46,  31,  46,  46,  31,   1,   1,   1,  94,
																															46,  46,   1,  30,  46,  62,  62,  64,  78,  32,
																															14,  16,  32,   0,  14, 124,  62,  46,   1,  30,
																															46,  62,  15,  46,  13,  15,  15,  46,   1,   1,
																															1,   1,   1,   1,   1
																														},
																														{
																															14, 124,  13,  15,  61,   1,  76,  12,  12,  64,
																																1,  15,   1,   1,  44,  30,  15,  73,   1,  16,
																																75,  73,  73,  71,  71,  14,  14,  30, 124,  46,
																																2,  14,  77,  78,  27,  46,   1,  14,  60,   1,
																																1,   1,  15,  15,  45,  61,  29,  61,  75,  59,
																																45,  77,  77,  73,  45,  61,  77,  77,  73,  29,
																																13,  43,  45,  61,  29,  61,  75,  59,  45,  77,
																																77,  73,  45,  61,  77,  77,  73,  29,  13,  43,
																																18,  31,  41,   1,   1,   1,  15,  14,  15,  13,
																																13,  73,  29,  62,  44,  31,  32,  14,   1,  62,
																																44,  31,  32,  14,   1,  62,  44,  31,  32,  14,
																																1,   1,  14,   0,   0,   2,  18,   2,  18,  46,
																																46,  44,  31,  46,  44,  31,   1,   1,   1,  92,
																																46,  46,   1,  30,  46,  60,  60,  64,  78,  32,
																																14,  18,  32,   2,  14, 124,  62,  46,   1,  30,
																																46,  60,  13,  46,  11,  13,  13,  46,   1,   1,
																																1,   1,   1,   1,   1
																														},
																														{
																															14, 124,  11,  15,  61,   1,  74,  12,  10,  64,
																																1,  15,   1,   1,  44,  30,  15,  71,   1,  16,
																																73,  71,  69,  65,  65,  14,  14,  30, 124,  46,
																																2,  14,  75,  78,  25,  46,   1,  14,  58,   1,
																																1,   1,  15,  15,  45,  59,  29,  59,  73,  57,
																																45,  75,  75,  71,  45,  61,  75,  75,  71,  27,
																																13,  41,  45,  59,  29,  59,  73,  57,  45,  75,
																																75,  71,  45,  61,  75,  75,  71,  27,  13,  41,
																																18,  31,  37,   1,   1,   1,  15,  14,  15,  13,
																																13,  69,  29,  62,  44,  31,  32,  14,   1,  62,
																																44,  31,  32,  14,   1,  62,  44,  31,  32,  14,
																																1,   1,  14,   0,   0,   2,  18,   2,  18,  46,
																																46,  44,  31,  46,  44,  31,   1,   1,   1,  90,
																																46,  46,   1,  30,  46,  60,  60,  64,  78,  32,
																																14,  18,  32,   2,  14, 124,  62,  46,   1,  30,
																																46,  60,  11,  46,   9,  11,  11,  46,   1,   1,
																																1,   1,   1,   1,   1
																															},
																															{
																																14, 124,   9,  13,  59,   1,  72,  10,   8,  66,
																																	1,  13,   1,   1,  42,  30,  13,  67,   1,  18,
																																	71,  67,  65,  61,  61,  14,  14,  28, 124,  44,
																																	4,  14,  73,  78,  23,  44,   1,  12,  56,   1,
																																	1,   1,  13,  13,  43,  57,  27,  57,  71,  55,
																																	43,  73,  73,  67,  43,  59,  73,  73,  67,  25,
																																	11,  39,  43,  57,  27,  57,  71,  55,  43,  73,
																																	73,  67,  43,  59,  73,  73,  67,  25,  11,  39,
																																	20,  29,  33,   1,   3,   1,  13,  14,  13,  11,
																																	11,  65,  27,  60,  42,  29,  34,  14,   1,  60,
																																	42,  29,  34,  14,   1,  60,  42,  29,  34,  14,
																																	1,   3,  14,   2,   2,   4,  20,   4,  20,  44,
																																	46,  42,  29,  46,  42,  29,   1,   1,   1,  88,
																																	44,  44,   1,  30,  44,  58,  58,  66,  78,  34,
																																	14,  20,  34,   4,  12, 124,  60,  44,   1,  30,
																																	44,  58,   9,  44,   7,   9,   9,  44,   1,   1,
																																	1,   1,   1,   1,   1
																															},
																															{
																																14, 124,   7,  13,  57,   1,  70,   8,   6,  66,
																																	1,  13,   1,   1,  40,  30,  13,  65,   1,  18,
																																	67,  65,  61,  55,  55,  14,  14,  28, 124,  44,
																																	6,  14,  71,  78,  19,  44,   1,  12,  54,   1,
																																	1,   1,  13,  13,  41,  55,  25,  55,  67,  51,
																																	41,  71,  71,  65,  41,  57,  71,  71,  65,  23,
																																	9,  35,  41,  55,  25,  55,  67,  51,  41,  71,
																																	71,  65,  41,  57,  71,  71,  65,  23,   9,  35,
																																	22,  29,  29,   1,   3,   1,  13,  14,  13,   9,
																																	9,  61,  25,  60,  40,  29,  34,  14,   1,  60,
																																	40,  29,  34,  14,   1,  60,  40,  29,  34,  14,
																																	1,   3,  14,   2,   2,   6,  22,   6,  22,  44,
																																	46,  40,  29,  46,  40,  29,   1,   1,   1,  86,
																																	44,  44,   1,  30,  44,  56,  56,  66,  78,  34,
																																	14,  22,  34,   6,  12, 124,  60,  44,   1,  30,
																																	44,  56,   7,  44,   3,   7,   7,  44,   1,   1,
																																	1,   1,   1,   1,   1
																																},
																																{
																																	14, 124,   5,  13,  57,   1,  68,   8,   4,  66,
																																		1,  13,   1,   1,  40,  30,  13,  61,   1,  18,
																																		65,  61,  57,  51,  51,  14,  14,  28, 124,  44,
																																		6,  14,  69,  78,  17,  44,   1,  12,  52,   1,
																																		1,   1,  13,  13,  41,  53,  25,  53,  65,  49,
																																		41,  69,  69,  61,  41,  57,  69,  69,  61,  21,
																																		9,  33,  41,  53,  25,  53,  65,  49,  41,  69,
																																		69,  61,  41,  57,  69,  69,  61,  21,   9,  33,
																																		22,  29,  25,   1,   3,   1,  13,  14,  13,   9,
																																		9,  57,  25,  60,  40,  29,  34,  14,   1,  60,
																																		40,  29,  34,  14,   1,  60,  40,  29,  34,  14,
																																		1,   3,  14,   2,   2,   6,  22,   6,  22,  44,
																																		46,  40,  29,  46,  40,  29,   1,   1,   1,  84,
																																		44,  44,   1,  30,  44,  56,  56,  66,  78,  34,
																																		14,  22,  34,   6,  12, 124,  60,  44,   1,  30,
																																		44,  56,   5,  44,   1,   5,   5,  44,   1,   1,
																																		1,   1,   1,   1,   1
																																},
																																{
																																	14, 124,   3,  11,  55,   1,  66,   6,   2,  68,
																																		1,  11,   1,   1,  38,  30,  11,  59,   1,  20,
																																		63,  59,  53,  45,  45,  14,  14,  26, 124,  42,
																																		8,  14,  67,  78,  15,  42,   1,  10,  50,   1,
																																		1,   1,  11,  11,  39,  51,  23,  51,  63,  47,
																																		39,  67,  67,  59,  39,  55,  67,  67,  59,  19,
																																		7,  31,  39,  51,  23,  51,  63,  47,  39,  67,
																																		67,  59,  39,  55,  67,  67,  59,  19,   7,  31,
																																		24,  27,  21,   1,   5,   1,  11,  14,  11,   7,
																																		7,  53,  23,  58,  38,  27,  36,  14,   1,  58,
																																		38,  27,  36,  14,   1,  58,  38,  27,  36,  14,
																																		1,   5,  14,   4,   4,   8,  24,   8,  24,  42,
																																		46,  38,  27,  46,  38,  27,   1,   1,   1,  82,
																																		42,  42,   1,  30,  42,  54,  54,  68,  78,  36,
																																		14,  24,  36,   8,  10, 124,  58,  42,   1,  30,
																																		42,  54,   3,  42,   0,   3,   3,  42,   1,   1,
																																		1,   1,   1,   1,   1
																																	},
																																	{
																																		14, 124,   1,  11,  55,   1,  64,   4,   0,  68,
																																			1,  11,   1,   1,  36,  30,  11,  55,   1,  20,
																																			61,  55,  51,  41,  41,  14,  14,  26, 124,  42,
																																			8,  14,  65,  78,  13,  42,   1,  10,  48,   1,
																																			1,   1,  11,  11,  39,  49,  23,  49,  61,  45,
																																			39,  65,  65,  55,  39,  55,  65,  65,  55,  17,
																																			7,  29,  39,  49,  23,  49,  61,  45,  39,  65,
																																			65,  55,  39,  55,  65,  65,  55,  17,   7,  29,
																																			24,  27,  19,   1,   5,   1,  11,  14,  11,   7,
																																			7,  51,  23,  58,  36,  27,  36,  14,   1,  58,
																																			36,  27,  36,  14,   1,  58,  36,  27,  36,  14,
																																			1,   5,  14,   4,   4,   8,  24,   8,  24,  42,
																																			46,  36,  27,  46,  36,  27,   1,   1,   1,  80,
																																			42,  42,   1,  30,  42,  52,  52,  68,  78,  36,
																																			14,  24,  36,   8,  10, 124,  58,  42,   1,  30,
																																			42,  52,   1,  42,   2,   1,   1,  42,   1,   1,
																																			1,   1,   1,   1,   1
																																	},
																																	{
																																		14, 124,   0,  11,  53,   1,  62,   4,   1,  68,
																																			1,  11,   1,   1,  36,  30,  11,  51,   1,  20,
																																			57,  51,  47,  35,  35,  14,  14,  26, 124,  42,
																																			10,  14,  63,  78,   9,  42,   1,  10,  46,   1,
																																			1,   1,  11,  11,  37,  47,  21,  47,  57,  41,
																																			37,  63,  63,  51,  37,  53,  63,  63,  51,  15,
																																			5,  25,  37,  47,  21,  47,  57,  41,  37,  63,
																																			63,  51,  37,  53,  63,  63,  51,  15,   5,  25,
																																			26,  27,  15,   1,   5,   1,  11,  14,  11,   5,
																																			5,  47,  21,  58,  36,  27,  36,  14,   1,  58,
																																			36,  27,  36,  14,   1,  58,  36,  27,  36,  14,
																																			1,   5,  14,   4,   4,  10,  26,  10,  26,  42,
																																			46,  36,  27,  46,  36,  27,   1,   1,   1,  78,
																																			42,  42,   1,  30,  42,  52,  52,  68,  78,  36,
																																			14,  26,  36,  10,  10, 124,  58,  42,   1,  30,
																																			42,  52,   0,  42,   6,   0,   0,  42,   1,   1,
																																			1,   1,   1,   1,   1
																																		},
																																		{
																																			14, 124,   2,   9,  51,   1,  60,   2,   3,  70,
																																				1,   9,   1,   1,  34,  30,   9,  49,   1,  22,
																																				55,  49,  43,  31,  31,  14,  14,  24, 124,  40,
																																				12,  14,  61,  78,   7,  40,   1,   8,  44,   1,
																																				1,   1,   9,   9,  35,  45,  19,  45,  55,  39,
																																				35,  61,  61,  49,  35,  51,  61,  61,  49,  13,
																																				3,  23,  35,  45,  19,  45,  55,  39,  35,  61,
																																				61,  49,  35,  51,  61,  61,  49,  13,   3,  23,
																																				28,  25,  11,   1,   7,   1,   9,  14,   9,   3,
																																				3,  43,  19,  56,  34,  25,  38,  14,   1,  56,
																																				34,  25,  38,  14,   1,  56,  34,  25,  38,  14,
																																				1,   7,  14,   6,   6,  12,  28,  12,  28,  40,
																																				46,  34,  25,  46,  34,  25,   1,   1,   1,  76,
																																				40,  40,   1,  30,  40,  50,  50,  70,  78,  38,
																																				14,  28,  38,  12,   8, 124,  56,  40,   1,  30,
																																				40,  50,   2,  40,   8,   2,   2,  40,   1,   1,
																																				1,   1,   1,   1,   1
																																		},
																																		{
																																			14, 124,   4,   9,  51,   1,  58,   2,   5,  70,
																																				1,   9,   1,   1,  34,  30,   9,  45,   1,  22,
																																				53,  45,  39,  25,  25,  14,  14,  24, 124,  40,
																																				12,  14,  59,  78,   5,  40,   1,   8,  42,   1,
																																				1,   1,   9,   9,  35,  43,  19,  43,  53,  37,
																																				35,  59,  59,  45,  35,  51,  59,  59,  45,  11,
																																				3,  21,  35,  43,  19,  43,  53,  37,  35,  59,
																																				59,  45,  35,  51,  59,  59,  45,  11,   3,  21,
																																				28,  25,   7,   1,   7,   1,   9,  14,   9,   3,
																																				3,  39,  19,  56,  34,  25,  38,  14,   1,  56,
																																				34,  25,  38,  14,   1,  56,  34,  25,  38,  14,
																																				1,   7,  14,   6,   6,  12,  28,  12,  28,  40,
																																				46,  34,  25,  46,  34,  25,   1,   1,   1,  74,
																																				40,  40,   1,  30,  40,  50,  50,  70,  78,  38,
																																				14,  28,  38,  12,   8, 124,  56,  40,   1,  30,
																																				40,  50,   4,  40,  10,   4,   4,  40,   1,   1,
																																				1,   1,   1,   1,   1
																																			},
																																			{
																																				14, 124,   6,   9,  49,   1,  56,   0,   7,  70,
																																					1,   9,   1,   1,  32,  30,   9,  43,   1,  22,
																																					51,  43,  35,  21,  21,  14,  14,  24, 122,  40,
																																					14,  14,  57,  78,   3,  40,   1,   8,  40,   1,
																																					1,   1,   9,   9,  33,  41,  17,  41,  51,  35,
																																					33,  57,  57,  43,  33,  49,  57,  57,  43,   9,
																																					1,  19,  33,  41,  17,  41,  51,  35,  33,  57,
																																					57,  43,  33,  49,  57,  57,  43,   9,   1,  19,
																																					30,  25,   3,   1,   7,   1,   9,  14,   9,   1,
																																					1,  35,  17,  56,  32,  25,  38,  14,   1,  56,
																																					32,  25,  38,  14,   1,  56,  32,  25,  38,  14,
																																					1,   7,  14,   6,   6,  14,  30,  14,  30,  40,
																																					46,  32,  25,  46,  32,  25,   1,   1,   1,  72,
																																					40,  40,   1,  30,  40,  48,  48,  70,  78,  38,
																																					14,  30,  38,  14,   8, 124,  56,  40,   1,  30,
																																					40,  48,   6,  40,  12,   6,   6,  40,   1,   1,
																																					1,   1,   1,   1,   1
																																			},
																																			{
																																				14, 124,   8,   7,  47,   1,  54,   1,   9,  72,
																																					1,   7,   1,   1,  30,  30,   7,  39,   1,  24,
																																					47,  39,  31,  15,  15,  14,  14,  22, 118,  38,
																																					16,  14,  55,  78,   0,  38,   1,   6,  38,   1,
																																					1,   1,   7,   7,  31,  39,  15,  39,  47,  31,
																																					31,  55,  55,  39,  31,  47,  55,  55,  39,   7,
																																					0,  15,  31,  39,  15,  39,  47,  31,  31,  55,
																																					55,  39,  31,  47,  55,  55,  39,   7,   0,  15,
																																					32,  23,   0,   1,   9,   1,   7,  14,   7,   0,
																																					0,  31,  15,  54,  30,  23,  40,  14,   1,  54,
																																					30,  23,  40,  14,   1,  54,  30,  23,  40,  14,
																																					1,   9,  14,   8,   8,  16,  32,  16,  32,  38,
																																					46,  30,  23,  46,  30,  23,   1,   1,   1,  70,
																																					38,  38,   1,  30,  38,  46,  46,  72,  78,  40,
																																					14,  32,  40,  16,   6, 124,  54,  38,   1,  30,
																																					38,  46,   8,  38,  16,   8,   8,  38,   1,   1,
																																					1,   1,   1,   1,   1
																																				},
																																				{
																																					14, 124,  10,   7,  47,   1,  52,   1,  11,  72,
																																						1,   7,   1,   1,  30,  30,   7,  37,   1,  24,
																																						45,  37,  27,  11,  11,  14,  14,  22, 116,  38,
																																						16,  14,  53,  78,   2,  38,   1,   6,  36,   1,
																																						1,   1,   7,   7,  31,  37,  15,  37,  45,  29,
																																						31,  53,  53,  37,  31,  47,  53,  53,  37,   5,
																																						0,  13,  31,  37,  15,  37,  45,  29,  31,  53,
																																						53,  37,  31,  47,  53,  53,  37,   5,   0,  13,
																																						32,  23,   4,   1,   9,   1,   7,  14,   7,   0,
																																						0,  27,  15,  54,  30,  23,  40,  14,   1,  54,
																																						30,  23,  40,  14,   1,  54,  30,  23,  40,  14,
																																						1,   9,  14,   8,   8,  16,  32,  16,  32,  38,
																																						46,  30,  23,  46,  30,  23,   1,   1,   1,  68,
																																						38,  38,   1,  30,  38,  46,  46,  72,  78,  40,
																																						14,  32,  40,  16,   6, 124,  54,  38,   1,  30,
																																						38,  46,  10,  38,  18,  10,  10,  38,   1,   1,
																																						1,   1,   1,   1,   1
																																				},
																																				{
																																					14, 124,  12,   7,  45,   1,  50,   3,  13,  72,
																																						1,   7,   1,   1,  28,  30,   7,  33,   1,  24,
																																						43,  33,  23,   5,   5,  14,  14,  22, 112,  38,
																																						18,  14,  51,  78,   4,  38,   1,   6,  34,   1,
																																						1,   1,   7,   7,  29,  35,  13,  35,  43,  27,
																																						29,  51,  51,  33,  29,  45,  51,  51,  33,   3,
																																						2,  11,  29,  35,  13,  35,  43,  27,  29,  51,
																																						51,  33,  29,  45,  51,  51,  33,   3,   2,  11,
																																						34,  23,   8,   1,   9,   1,   7,  14,   7,   2,
																																						2,  23,  13,  54,  28,  23,  40,  14,   1,  54,
																																						28,  23,  40,  14,   1,  54,  28,  23,  40,  14,
																																						1,   9,  14,   8,   8,  18,  34,  18,  34,  38,
																																						46,  28,  23,  46,  28,  23,   1,   1,   1,  66,
																																						38,  38,   1,  30,  38,  44,  44,  72,  78,  40,
																																						14,  34,  40,  18,   6, 122,  54,  38,   1,  30,
																																						38,  44,  12,  38,  20,  12,  12,  38,   1,   1,
																																						1,   1,   1,   1,   1
																																					},
																																					{
																																						14, 124,  12,   7,  45,   1,  48,   5,  15,  72,
																																							1,   7,   1,   1,  26,  30,   7,  31,   1,  24,
																																							41,  31,  21,   1,   1,  14,  14,  20, 108,  36,
																																							18,  14,  51,  78,   6,  36,   1,   4,  32,   1,
																																							1,   1,   7,   7,  29,  35,  13,  35,  41,  25,
																																							29,  51,  51,  31,  29,  45,  51,  51,  31,   3,
																																							2,   9,  29,  35,  13,  35,  41,  25,  29,  51,
																																							51,  31,  29,  45,  51,  51,  31,   3,   2,   9,
																																							34,  23,  10,   1,  11,   1,   7,  14,   7,   2,
																																							2,  21,  13,  52,  26,  23,  40,  14,   1,  52,
																																							26,  23,  40,  14,   1,  52,  26,  23,  40,  14,
																																							1,  11,  14,   8,   8,  18,  34,  18,  34,  36,
																																							46,  26,  23,  46,  26,  23,   1,   1,   1,  64,
																																							36,  36,   1,  30,  36,  42,  42,  72,  78,  40,
																																							14,  34,  40,  18,   4, 118,  52,  36,   1,  30,
																																							36,  42,  12,  36,  22,  12,  12,  36,   1,   1,
																																							1,   1,   1,   1,   1
																																					},
																																					{
																																						14, 124,  14,   5,  43,   1,  48,   5,  15,  74,
																																							1,   5,   1,   1,  26,  30,   5,  27,   1,  26,
																																							37,  27,  17,   4,   4,  14,  14,  20, 106,  36,
																																							20,  14,  49,  78,  10,  36,   1,   4,  32,   1,
																																							1,   1,   5,   5,  27,  33,  11,  33,  37,  21,
																																							27,  49,  49,  27,  27,  43,  49,  49,  27,   1,
																																							4,   5,  27,  33,  11,  33,  37,  21,  27,  49,
																																							49,  27,  27,  43,  49,  49,  27,   1,   4,   5,
																																							36,  21,  14,   1,  11,   1,   5,  14,   5,   4,
																																							4,  17,  11,  52,  26,  21,  42,  14,   1,  52,
																																							26,  21,  42,  14,   1,  52,  26,  21,  42,  14,
																																							1,  11,  14,  10,  10,  20,  36,  20,  36,  36,
																																							46,  26,  21,  46,  26,  21,   1,   1,   1,  64,
																																							36,  36,   1,  30,  36,  42,  42,  74,  78,  42,
																																							14,  36,  42,  20,   4, 116,  52,  36,   1,  30,
																																							36,  42,  14,  36,  26,  14,  14,  36,   1,   1,
																																							1,   1,   1,   1,   1
																																						},
																																						{
																																							14, 124,  16,   5,  41,   1,  46,   7,  17,  74,
																																								1,   5,   1,   1,  24,  30,   5,  23,   1,  26,
																																								35,  23,  13,   8,   8,  14,  14,  20, 102,  36,
																																								22,  14,  47,  78,  12,  36,   1,   4,  30,   1,
																																								1,   1,   5,   5,  25,  31,   9,  31,  35,  19,
																																								25,  47,  47,  23,  25,  41,  47,  47,  23,   0,
																																								6,   3,  25,  31,   9,  31,  35,  19,  25,  47,
																																								47,  23,  25,  41,  47,  47,  23,   0,   6,   3,
																																								38,  21,  18,   1,  11,   1,   5,  14,   5,   6,
																																								6,  13,   9,  52,  24,  21,  42,  14,   1,  52,
																																								24,  21,  42,  14,   1,  52,  24,  21,  42,  14,
																																								1,  11,  14,  10,  10,  22,  38,  22,  38,  36,
																																								46,  24,  21,  46,  24,  21,   1,   1,   1,  62,
																																								36,  36,   1,  30,  36,  40,  40,  74,  78,  42,
																																								14,  38,  42,  22,   4, 114,  52,  36,   1,  30,
																																								36,  40,  16,  36,  28,  16,  16,  36,   1,   1,
																																								1,   1,   1,   1,   1
																																						},
																																						{
																																							14, 124,  18,   5,  41,   1,  44,   7,  19,  74,
																																								1,   5,   1,   1,  24,  30,   5,  21,   1,  26,
																																								33,  21,   9,  14,  14,  14,  14,  20, 100,  36,
																																								22,  14,  45,  78,  14,  36,   1,   4,  28,   1,
																																								1,   1,   5,   5,  25,  29,   9,  29,  33,  17,
																																								25,  45,  45,  21,  25,  41,  45,  45,  21,   2,
																																								6,   1,  25,  29,   9,  29,  33,  17,  25,  45,
																																								45,  21,  25,  41,  45,  45,  21,   2,   6,   1,
																																								38,  21,  22,   1,  11,   1,   5,  14,   5,   6,
																																								6,   9,   9,  52,  24,  21,  42,  14,   1,  52,
																																								24,  21,  42,  14,   1,  52,  24,  21,  42,  14,
																																								1,  11,  14,  10,  10,  22,  38,  22,  38,  36,
																																								46,  24,  21,  46,  24,  21,   1,   1,   1,  60,
																																								36,  36,   1,  30,  36,  40,  40,  74,  78,  42,
																																								14,  38,  42,  22,   4, 112,  52,  36,   1,  30,
																																								36,  40,  18,  36,  30,  18,  18,  36,   1,   1,
																																								1,   1,   1,   1,   1
																																							},
																																							{
																																								14, 124,  20,   3,  39,   1,  42,   9,  21,  76,
																																									1,   3,   1,   1,  22,  30,   3,  17,   1,  28,
																																									31,  17,   5,  18,  18,  14,  14,  18,  96,  34,
																																									24,  14,  43,  78,  16,  34,   1,   2,  26,   1,
																																									1,   1,   3,   3,  23,  27,   7,  27,  31,  15,
																																									23,  43,  43,  17,  23,  39,  43,  43,  17,   4,
																																									8,   0,  23,  27,   7,  27,  31,  15,  23,  43,
																																									43,  17,  23,  39,  43,  43,  17,   4,   8,   0,
																																									40,  19,  26,   1,  13,   1,   3,  14,   3,   8,
																																									8,   5,   7,  50,  22,  19,  44,  14,   1,  50,
																																									22,  19,  44,  14,   1,  50,  22,  19,  44,  14,
																																									1,  13,  14,  12,  12,  24,  40,  24,  40,  34,
																																									46,  22,  19,  46,  22,  19,   1,   1,   1,  58,
																																									34,  34,   1,  30,  34,  38,  38,  76,  78,  44,
																																									14,  40,  44,  24,   2, 108,  50,  34,   1,  30,
																																									34,  38,  20,  34,  32,  20,  20,  34,   1,   1,
																																									1,   1,   1,   1,   1
																																							},
																																							{
																																								14, 124,  22,   3,  37,   1,  40,  11,  23,  76,
																																									1,   3,   1,   1,  20,  30,   3,  15,   1,  28,
																																									27,  15,   1,  24,  24,  14,  14,  18,  94,  34,
																																									26,  14,  41,  78,  20,  34,   1,   2,  24,   1,
																																									1,   1,   3,   3,  21,  25,   5,  25,  27,  11,
																																									21,  41,  41,  15,  21,  37,  41,  41,  15,   6,
																																									10,   4,  21,  25,   5,  25,  27,  11,  21,  41,
																																									41,  15,  21,  37,  41,  41,  15,   6,  10,   4,
																																									42,  19,  30,   1,  13,   1,   3,  14,   3,  10,
																																									10,   1,   5,  50,  20,  19,  44,  14,   1,  50,
																																									20,  19,  44,  14,   1,  50,  20,  19,  44,  14,
																																									1,  13,  14,  12,  12,  26,  42,  26,  42,  34,
																																									46,  20,  19,  46,  20,  19,   1,   1,   1,  56,
																																									34,  34,   1,  30,  34,  36,  36,  76,  78,  44,
																																									14,  42,  44,  26,   2, 106,  50,  34,   1,  30,
																																									34,  36,  22,  34,  36,  22,  22,  34,   1,   1,
																																									1,   1,   1,   1,   1
																																								},
																																								{
																																									14, 124,  24,   3,  37,   1,  38,  11,  25,  76,
																																										1,   3,   1,   1,  20,  30,   3,  11,   1,  28,
																																										25,  11,   2,  28,  28,  14,  14,  18,  90,  34,
																																										26,  14,  39,  78,  22,  34,   1,   2,  22,   1,
																																										1,   1,   3,   3,  21,  23,   5,  23,  25,   9,
																																										21,  39,  39,  11,  21,  37,  39,  39,  11,   8,
																																										10,   6,  21,  23,   5,  23,  25,   9,  21,  39,
																																										39,  11,  21,  37,  39,  39,  11,   8,  10,   6,
																																										42,  19,  34,   1,  13,   1,   3,  14,   3,  10,
																																										10,   2,   5,  50,  20,  19,  44,  14,   1,  50,
																																										20,  19,  44,  14,   1,  50,  20,  19,  44,  14,
																																										1,  13,  14,  12,  12,  26,  42,  26,  42,  34,
																																										46,  20,  19,  46,  20,  19,   1,   1,   1,  54,
																																										34,  34,   1,  30,  34,  36,  36,  76,  78,  44,
																																										14,  42,  44,  26,   2, 104,  50,  34,   1,  30,
																																										34,  36,  24,  34,  38,  24,  24,  34,   1,   1,
																																										1,   1,   1,   1,   1
																																								},
																																								{
																																									14, 124,  26,   1,  35,   1,  36,  13,  27,  78,
																																										1,   1,   1,   1,  18,  30,   1,   9,   1,  30,
																																										23,   9,   6,  34,  34,  14,  14,  16,  88,  32,
																																										28,  14,  37,  78,  24,  32,   1,   0,  20,   1,
																																										1,   1,   1,   1,  19,  21,   3,  21,  23,   7,
																																										19,  37,  37,   9,  19,  35,  37,  37,   9,  10,
																																										12,   8,  19,  21,   3,  21,  23,   7,  19,  37,
																																										37,   9,  19,  35,  37,  37,   9,  10,  12,   8,
																																										44,  17,  38,   1,  15,   1,   1,  14,   1,  12,
																																										12,   6,   3,  48,  18,  17,  46,  14,   1,  48,
																																										18,  17,  46,  14,   1,  48,  18,  17,  46,  14,
																																										1,  15,  14,  14,  14,  28,  44,  28,  44,  32,
																																										46,  18,  17,  46,  18,  17,   1,   1,   1,  52,
																																										32,  32,   1,  30,  32,  34,  34,  78,  78,  46,
																																										14,  44,  46,  28,   0, 102,  48,  32,   1,  30,
																																										32,  34,  26,  32,  40,  26,  26,  32,   1,   1,
																																										1,   1,   1,   1,   1
																																									},
																																									{
																																										14, 124,  28,   1,  35,   1,  34,  15,  29,  78,
																																											1,   1,   1,   1,  16,  30,   1,   5,   1,  30,
																																											21,   5,   8,  38,  38,  14,  14,  16,  84,  32,
																																											28,  14,  35,  78,  26,  32,   1,   0,  18,   1,
																																											1,   1,   1,   1,  19,  19,   3,  19,  21,   5,
																																											19,  35,  35,   5,  19,  35,  35,  35,   5,  12,
																																											12,  10,  19,  19,   3,  19,  21,   5,  19,  35,
																																											35,   5,  19,  35,  35,  35,   5,  12,  12,  10,
																																											44,  17,  40,   1,  15,   1,   1,  14,   1,  12,
																																											12,   8,   3,  48,  16,  17,  46,  14,   1,  48,
																																											16,  17,  46,  14,   1,  48,  16,  17,  46,  14,
																																											1,  15,  14,  14,  14,  28,  44,  28,  44,  32,
																																											46,  16,  17,  46,  16,  17,   1,   1,   1,  50,
																																											32,  32,   1,  30,  32,  32,  32,  78,  78,  46,
																																											14,  44,  46,  28,   0,  98,  48,  32,   1,  30,
																																											32,  32,  28,  32,  42,  28,  28,  32,   1,   1,
																																											1,   1,   1,   1,   1
																																									},
																																									{
																																										14, 124,  30,   1,  33,   1,  32,  15,  31,  78,
																																											1,   1,   1,   1,  16,  30,   1,   1,   1,  30,
																																											17,   1,  12,  44,  44,  14,  14,  16,  80,  32,
																																											30,  14,  33,  78,  30,  32,   1,   0,  16,   1,
																																											1,   1,   1,   1,  17,  17,   1,  17,  17,   1,
																																											17,  33,  33,   1,  17,  33,  33,  33,   1,  14,
																																											14,  14,  17,  17,   1,  17,  17,   1,  17,  33,
																																											33,   1,  17,  33,  33,  33,   1,  14,  14,  14,
																																											46,  17,  44,   1,  15,   1,   1,  14,   1,  14,
																																											14,  12,   1,  48,  16,  17,  46,  14,   1,  48,
																																											16,  17,  46,  14,   1,  48,  16,  17,  46,  14,
																																											1,  15,  14,  14,  14,  30,  46,  30,  46,  32,
																																											46,  16,  17,  46,  16,  17,   1,   1,   1,  48,
																																											32,  32,   1,  30,  32,  32,  32,  78,  78,  46,
																																											14,  46,  46,  30,   0,  96,  48,  32,   1,  30,
																																											32,  32,  30,  32,  46,  30,  30,  32,   1,   1,
																																											1,   1,   1,   1,   1
																																										},
																																										{
																																											14, 124,  32,   0,  31,   1,  30,  17,  33,  80,
																																												1,   0,   1,   1,  14,  30,   0,   0,   1,  32,
																																												15,   0,  16,  48,  48,  14,  14,  14,  78,  30,
																																												32,  14,  31,  78,  32,  30,   1,   1,  14,   1,
																																												1,   1,   0,   0,  15,  15,   0,  15,  15,   0,
																																												15,  31,  31,   0,  15,  31,  31,  31,   0,  16,
																																												16,  16,  15,  15,   0,  15,  15,   0,  15,  31,
																																												31,   0,  15,  31,  31,  31,   0,  16,  16,  16,
																																												48,  15,  48,   1,  17,   1,   0,  14,   0,  16,
																																												16,  16,   0,  46,  14,  15,  48,  14,   1,  46,
																																												14,  15,  48,  14,   1,  46,  14,  15,  48,  14,
																																												1,  17,  14,  16,  16,  32,  48,  32,  48,  30,
																																												46,  14,  15,  46,  14,  15,   1,   1,   1,  46,
																																												30,  30,   1,  30,  30,  30,  30,  80,  78,  48,
																																												14,  48,  48,  32,   1,  94,  46,  30,   1,  30,
																																												30,  30,  32,  30,  48,  32,  32,  30,   1,   1,
																																												1,   1,   1,   1,   1
																																										},
																																										{
																																											14, 124,  34,   0,  31,   1,  28,  17,  35,  80,
																																												1,   0,   1,   1,  14,  30,   0,   4,   1,  32,
																																												13,   4,  20,  54,  54,  14,  14,  14,  74,  30,
																																												32,  14,  29,  78,  34,  30,   1,   1,  12,   1,
																																												1,   1,   0,   0,  15,  13,   0,  13,  13,   2,
																																												15,  29,  29,   4,  15,  31,  29,  29,   4,  18,
																																												16,  18,  15,  13,   0,  13,  13,   2,  15,  29,
																																												29,   4,  15,  31,  29,  29,   4,  18,  16,  18,
																																												48,  15,  52,   1,  17,   1,   0,  14,   0,  16,
																																												16,  20,   0,  46,  14,  15,  48,  14,   1,  46,
																																												14,  15,  48,  14,   1,  46,  14,  15,  48,  14,
																																												1,  17,  14,  16,  16,  32,  48,  32,  48,  30,
																																												46,  14,  15,  46,  14,  15,   1,   1,   1,  44,
																																												30,  30,   1,  30,  30,  30,  30,  80,  78,  48,
																																												14,  48,  48,  32,   1,  92,  46,  30,   1,  30,
																																												30,  30,  34,  30,  50,  34,  34,  30,   1,   1,
																																												1,   1,   1,   1,   1
																																											},
																																											{
																																												14, 124,  36,   0,  29,   1,  26,  19,  37,  80,
																																													1,   0,   1,   1,  12,  30,   0,   6,   1,  32,
																																													11,   6,  24,  58,  58,  14,  14,  14,  72,  30,
																																													34,  14,  27,  78,  36,  30,   1,   1,  10,   1,
																																													1,   1,   0,   0,  13,  11,   2,  11,  11,   4,
																																													13,  27,  27,   6,  13,  29,  27,  27,   6,  20,
																																													18,  20,  13,  11,   2,  11,  11,   4,  13,  27,
																																													27,   6,  13,  29,  27,  27,   6,  20,  18,  20,
																																													50,  15,  56,   1,  17,   1,   0,  14,   0,  18,
																																													18,  24,   2,  46,  12,  15,  48,  14,   1,  46,
																																													12,  15,  48,  14,   1,  46,  12,  15,  48,  14,
																																													1,  17,  14,  16,  16,  34,  50,  34,  50,  30,
																																													46,  12,  15,  46,  12,  15,   1,   1,   1,  42,
																																													30,  30,   1,  30,  30,  28,  28,  80,  78,  48,
																																													14,  50,  48,  34,   1,  88,  46,  30,   1,  30,
																																													30,  28,  36,  30,  52,  36,  36,  30,   1,   1,
																																													1,   1,   1,   1,   1
																																											},
																																											{
																																												14, 124,  38,   2,  27,   1,  24,  21,  39,  82,
																																													1,   2,   1,   1,  10,  30,   2,  10,   1,  34,
																																													7,  10,  28,  64,  64,  14,  14,  12,  68,  28,
																																													36,  14,  25,  78,  40,  28,   1,   3,   8,   1,
																																													1,   1,   2,   2,  11,   9,   4,   9,   7,   8,
																																													11,  25,  25,  10,  11,  27,  25,  25,  10,  22,
																																													20,  24,  11,   9,   4,   9,   7,   8,  11,  25,
																																													25,  10,  11,  27,  25,  25,  10,  22,  20,  24,
																																													52,  13,  60,   1,  19,   1,   2,  14,   2,  20,
																																													20,  28,   4,  44,  10,  13,  50,  14,   1,  44,
																																													10,  13,  50,  14,   1,  44,  10,  13,  50,  14,
																																													1,  19,  14,  18,  18,  36,  52,  36,  52,  28,
																																													46,  10,  13,  46,  10,  13,   1,   1,   1,  40,
																																													28,  28,   1,  30,  28,  26,  26,  82,  78,  50,
																																													14,  52,  50,  36,   3,  86,  44,  28,   1,  30,
																																													28,  26,  38,  28,  56,  38,  38,  28,   1,   1,
																																													1,   1,   1,   1,   1
																																												},
																																												{
																																													14, 124,  40,   2,  27,   1,  22,  21,  41,  82,
																																														1,   2,   1,   1,  10,  30,   2,  12,   1,  34,
																																														5,  12,  32,  68,  68,  14,  14,  12,  66,  28,
																																														36,  14,  23,  78,  42,  28,   1,   3,   6,   1,
																																														1,   1,   2,   2,  11,   7,   4,   7,   5,  10,
																																														11,  23,  23,  12,  11,  27,  23,  23,  12,  24,
																																														20,  26,  11,   7,   4,   7,   5,  10,  11,  23,
																																														23,  12,  11,  27,  23,  23,  12,  24,  20,  26,
																																														52,  13,  64,   1,  19,   1,   2,  14,   2,  20,
																																														20,  32,   4,  44,  10,  13,  50,  14,   1,  44,
																																														10,  13,  50,  14,   1,  44,  10,  13,  50,  14,
																																														1,  19,  14,  18,  18,  36,  52,  36,  52,  28,
																																														46,  10,  13,  46,  10,  13,   1,   1,   1,  38,
																																														28,  28,   1,  30,  28,  26,  26,  82,  78,  50,
																																														14,  52,  50,  36,   3,  84,  44,  28,   1,  30,
																																														28,  26,  40,  28,  58,  40,  40,  28,   1,   1,
																																														1,   1,   1,   1,   1
																																												},
																																												{
																																													14, 124,  42,   2,  25,   1,  20,  23,  43,  82,
																																														1,   2,   1,   1,   8,  30,   2,  16,   1,  34,
																																														3,  16,  36,  74,  74,  14,  14,  12,  62,  28,
																																														38,  14,  21,  78,  44,  28,   1,   3,   4,   1,
																																														1,   1,   2,   2,   9,   5,   6,   5,   3,  12,
																																														9,  21,  21,  16,   9,  25,  21,  21,  16,  26,
																																														22,  28,   9,   5,   6,   5,   3,  12,   9,  21,
																																														21,  16,   9,  25,  21,  21,  16,  26,  22,  28,
																																														54,  13,  68,   1,  19,   1,   2,  14,   2,  22,
																																														22,  36,   6,  44,   8,  13,  50,  14,   1,  44,
																																														8,  13,  50,  14,   1,  44,   8,  13,  50,  14,
																																														1,  19,  14,  18,  18,  38,  54,  38,  54,  28,
																																														46,   8,  13,  46,   8,  13,   1,   1,   1,  36,
																																														28,  28,   1,  30,  28,  24,  24,  82,  78,  50,
																																														14,  54,  50,  38,   3,  82,  44,  28,   1,  30,
																																														28,  24,  42,  28,  60,  42,  42,  28,   1,   1,
																																														1,   1,   1,   1,   1
																																													},
																																													{
																																														14, 124,  42,   2,  25,   1,  18,  25,  45,  82,
																																															1,   2,   1,   1,   6,  30,   2,  18,   1,  34,
																																															1,  18,  38,  78,  78,  14,  14,  10,  58,  26,
																																															38,  14,  21,  78,  46,  26,   1,   5,   2,   1,
																																															1,   1,   2,   2,   9,   5,   6,   5,   1,  14,
																																															9,  21,  21,  18,   9,  25,  21,  21,  18,  26,
																																															22,  30,   9,   5,   6,   5,   1,  14,   9,  21,
																																															21,  18,   9,  25,  21,  21,  18,  26,  22,  30,
																																															54,  13,  70,   1,  21,   1,   2,  14,   2,  22,
																																															22,  38,   6,  42,   6,  13,  50,  14,   1,  42,
																																															6,  13,  50,  14,   1,  42,   6,  13,  50,  14,
																																															1,  21,  14,  18,  18,  38,  54,  38,  54,  26,
																																															46,   6,  13,  46,   6,  13,   1,   1,   1,  34,
																																															26,  26,   1,  30,  26,  22,  22,  82,  78,  50,
																																															14,  54,  50,  38,   5,  78,  42,  26,   1,  30,
																																															26,  22,  42,  26,  62,  42,  42,  26,   1,   1,
																																															1,   1,   1,   1,   1
																																													},
																																													{
																																														14, 124,  44,   4,  23,   1,  18,  25,  45,  84,
																																															1,   4,   1,   1,   6,  30,   4,  22,   1,  36,
																																															2,  22,  42,  84,  84,  14,  14,  10,  56,  26,
																																															40,  14,  19,  78,  50,  26,   1,   5,   2,   1,
																																															1,   1,   4,   4,   7,   3,   8,   3,   2,  18,
																																															7,  19,  19,  22,   7,  23,  19,  19,  22,  28,
																																															24,  34,   7,   3,   8,   3,   2,  18,   7,  19,
																																															19,  22,   7,  23,  19,  19,  22,  28,  24,  34,
																																															56,  11,  74,   1,  21,   1,   4,  14,   4,  24,
																																															24,  42,   8,  42,   6,  11,  52,  14,   1,  42,
																																															6,  11,  52,  14,   1,  42,   6,  11,  52,  14,
																																															1,  21,  14,  20,  20,  40,  56,  40,  56,  26,
																																															46,   6,  11,  46,   6,  11,   1,   1,   1,  34,
																																															26,  26,   1,  30,  26,  22,  22,  84,  78,  52,
																																															14,  56,  52,  40,   5,  76,  42,  26,   1,  30,
																																															26,  22,  44,  26,  66,  44,  44,  26,   1,   1,
																																															1,   1,   1,   1,   1
																																														},
																																														{
																																															14, 124,  46,   4,  21,   1,  16,  27,  47,  84,
																																																1,   4,   1,   1,   4,  30,   4,  26,   1,  36,
																																																4,  26,  46,  88,  88,  14,  14,  10,  52,  26,
																																																42,  14,  17,  78,  52,  26,   1,   5,   0,   1,
																																																1,   1,   4,   4,   5,   1,  10,   1,   4,  20,
																																																5,  17,  17,  26,   5,  21,  17,  17,  26,  30,
																																																26,  36,   5,   1,  10,   1,   4,  20,   5,  17,
																																																17,  26,   5,  21,  17,  17,  26,  30,  26,  36,
																																																58,  11,  78,   1,  21,   1,   4,  14,   4,  26,
																																																26,  46,  10,  42,   4,  11,  52,  14,   1,  42,
																																																4,  11,  52,  14,   1,  42,   4,  11,  52,  14,
																																																1,  21,  14,  20,  20,  42,  58,  42,  58,  26,
																																																46,   4,  11,  46,   4,  11,   1,   1,   1,  32,
																																																26,  26,   1,  30,  26,  20,  20,  84,  78,  52,
																																																14,  58,  52,  42,   5,  74,  42,  26,   1,  30,
																																																26,  20,  46,  26,  68,  46,  46,  26,   1,   1,
																																																1,   1,   1,   1,   1
																																														},
																																														{
																																															14, 124,  48,   4,  21,   1,  14,  27,  49,  84,
																																																1,   4,   1,   1,   4,  30,   4,  28,   1,  36,
																																																6,  28,  50,  94,  94,  14,  14,  10,  50,  26,
																																																42,  14,  15,  78,  54,  26,   1,   5,   1,   1,
																																																1,   1,   4,   4,   5,   0,  10,   0,   6,  22,
																																																5,  15,  15,  28,   5,  21,  15,  15,  28,  32,
																																																26,  38,   5,   0,  10,   0,   6,  22,   5,  15,
																																																15,  28,   5,  21,  15,  15,  28,  32,  26,  38,
																																																58,  11,  82,   1,  21,   1,   4,  14,   4,  26,
																																																26,  50,  10,  42,   4,  11,  52,  14,   1,  42,
																																																4,  11,  52,  14,   1,  42,   4,  11,  52,  14,
																																																1,  21,  14,  20,  20,  42,  58,  42,  58,  26,
																																																46,   4,  11,  46,   4,  11,   1,   1,   1,  30,
																																																26,  26,   1,  30,  26,  20,  20,  84,  78,  52,
																																																14,  58,  52,  42,   5,  72,  42,  26,   1,  30,
																																																26,  20,  48,  26,  70,  48,  48,  26,   1,   1,
																																																1,   1,   1,   1,   1
																																															},
																																															{
																																																14, 124,  50,   6,  19,   1,  12,  29,  51,  86,
																																																	1,   6,   1,   1,   2,  30,   6,  32,   1,  38,
																																																	8,  32,  54,  98,  98,  14,  14,   8,  46,  24,
																																																	44,  14,  13,  78,  56,  24,   1,   7,   3,   1,
																																																	1,   1,   6,   6,   3,   2,  12,   2,   8,  24,
																																																	3,  13,  13,  32,   3,  19,  13,  13,  32,  34,
																																																	28,  40,   3,   2,  12,   2,   8,  24,   3,  13,
																																																	13,  32,   3,  19,  13,  13,  32,  34,  28,  40,
																																																	60,   9,  86,   1,  23,   1,   6,  14,   6,  28,
																																																	28,  54,  12,  40,   2,   9,  54,  14,   1,  40,
																																																	2,   9,  54,  14,   1,  40,   2,   9,  54,  14,
																																																	1,  23,  14,  22,  22,  44,  60,  44,  60,  24,
																																																	46,   2,   9,  46,   2,   9,   1,   1,   1,  28,
																																																	24,  24,   1,  30,  24,  18,  18,  86,  78,  54,
																																																	14,  60,  54,  44,   7,  68,  40,  24,   1,  30,
																																																	24,  18,  50,  24,  72,  50,  50,  24,   1,   1,
																																																	1,   1,   1,   1,   1
																																															},
																																															{
																																																14, 124,  52,   6,  17,   1,  10,  31,  53,  86,
																																																	1,   6,   1,   1,   0,  30,   6,  34,   1,  38,
																																																	12,  34,  58, 104, 104,  14,  14,   8,  44,  24,
																																																	46,  14,  11,  78,  60,  24,   1,   7,   5,   1,
																																																	1,   1,   6,   6,   1,   4,  14,   4,  12,  28,
																																																	1,  11,  11,  34,   1,  17,  11,  11,  34,  36,
																																																	30,  44,   1,   4,  14,   4,  12,  28,   1,  11,
																																																	11,  34,   1,  17,  11,  11,  34,  36,  30,  44,
																																																	62,   9,  90,   1,  23,   1,   6,  14,   6,  30,
																																																	30,  58,  14,  40,   0,   9,  54,  14,   1,  40,
																																																	0,   9,  54,  14,   1,  40,   0,   9,  54,  14,
																																																	1,  23,  14,  22,  22,  46,  62,  46,  62,  24,
																																																	46,   0,   9,  46,   0,   9,   1,   1,   1,  26,
																																																	24,  24,   1,  30,  24,  16,  16,  86,  78,  54,
																																																	14,  62,  54,  46,   7,  66,  40,  24,   1,  30,
																																																	24,  16,  52,  24,  76,  52,  52,  24,   1,   1,
																																																	1,   1,   1,   1,   1
																																																},
																																																{
																																																	14, 124,  54,   6,  17,   1,   8,  31,  55,  86,
																																																		1,   6,   1,   1,   0,  30,   6,  38,   1,  38,
																																																		14,  38,  62, 108, 108,  14,  14,   8,  40,  24,
																																																		46,  14,   9,  78,  62,  24,   1,   7,   7,   1,
																																																		1,   1,   6,   6,   1,   6,  14,   6,  14,  30,
																																																		1,   9,   9,  38,   1,  17,   9,   9,  38,  38,
																																																		30,  46,   1,   6,  14,   6,  14,  30,   1,   9,
																																																		9,  38,   1,  17,   9,   9,  38,  38,  30,  46,
																																																		62,   9,  94,   1,  23,   1,   6,  14,   6,  30,
																																																		30,  62,  14,  40,   0,   9,  54,  14,   1,  40,
																																																		0,   9,  54,  14,   1,  40,   0,   9,  54,  14,
																																																		1,  23,  14,  22,  22,  46,  62,  46,  62,  24,
																																																		46,   0,   9,  46,   0,   9,   1,   1,   1,  24,
																																																		24,  24,   1,  30,  24,  16,  16,  86,  78,  54,
																																																		14,  62,  54,  46,   7,  64,  40,  24,   1,  30,
																																																		24,  16,  54,  24,  78,  54,  54,  24,   1,   1,
																																																		1,   1,   1,   1,   1
																																																},
																																																{
																																																	14, 124,  56,   8,  15,   1,   6,  33,  57,  88,
																																																		1,   8,   1,   1,   1,  30,   8,  40,   1,  40,
																																																		16,  40,  66, 114, 114,  14,  14,   6,  38,  22,
																																																		48,  14,   7,  78,  64,  22,   1,   9,   9,   1,
																																																		1,   1,   8,   8,   0,   8,  16,   8,  16,  32,
																																																		0,   7,   7,  40,   0,  15,   7,   7,  40,  40,
																																																		32,  48,   0,   8,  16,   8,  16,  32,   0,   7,
																																																		7,  40,   0,  15,   7,   7,  40,  40,  32,  48,
																																																		64,   7,  98,   1,  25,   1,   8,  14,   8,  32,
																																																		32,  66,  16,  38,   1,   7,  56,  14,   1,  38,
																																																		1,   7,  56,  14,   1,  38,   1,   7,  56,  14,
																																																		1,  25,  14,  24,  24,  48,  64,  48,  64,  22,
																																																		46,   1,   7,  46,   1,   7,   1,   1,   1,  22,
																																																		22,  22,   1,  30,  22,  14,  14,  88,  78,  56,
																																																		14,  64,  56,  48,   9,  62,  38,  22,   1,  30,
																																																		22,  14,  56,  22,  80,  56,  56,  22,   1,   1,
																																																		1,   1,   1,   1,   1
																																																	},
																																																	{
																																																		14, 124,  58,   8,  15,   1,   4,  35,  59,  88,
																																																			1,   8,   1,   1,   3,  30,   8,  44,   1,  40,
																																																			18,  44,  68, 118, 118,  14,  14,   6,  34,  22,
																																																			48,  14,   5,  78,  66,  22,   1,   9,  11,   1,
																																																			1,   1,   8,   8,   0,  10,  16,  10,  18,  34,
																																																			0,   5,   5,  44,   0,  15,   5,   5,  44,  42,
																																																			32,  50,   0,  10,  16,  10,  18,  34,   0,   5,
																																																			5,  44,   0,  15,   5,   5,  44,  42,  32,  50,
																																																			64,   7, 100,   1,  25,   1,   8,  14,   8,  32,
																																																			32,  68,  16,  38,   3,   7,  56,  14,   1,  38,
																																																			3,   7,  56,  14,   1,  38,   3,   7,  56,  14,
																																																			1,  25,  14,  24,  24,  48,  64,  48,  64,  22,
																																																			46,   3,   7,  46,   3,   7,   1,   1,   1,  20,
																																																			22,  22,   1,  30,  22,  12,  12,  88,  78,  56,
																																																			14,  64,  56,  48,   9,  58,  38,  22,   1,  30,
																																																			22,  12,  58,  22,  82,  58,  58,  22,   1,   1,
																																																			1,   1,   1,   1,   1
																																																	},
																																																	{
																																																		14, 124,  60,   8,  13,   1,   2,  35,  61,  88,
																																																			1,   8,   1,   1,   3,  30,   8,  48,   1,  40,
																																																			22,  48,  72, 124, 124,  14,  14,   6,  30,  22,
																																																			50,  14,   3,  78,  70,  22,   1,   9,  13,   1,
																																																			1,   1,   8,   8,   2,  12,  18,  12,  22,  38,
																																																			2,   3,   3,  48,   2,  13,   3,   3,  48,  44,
																																																			34,  54,   2,  12,  18,  12,  22,  38,   2,   3,
																																																			3,  48,   2,  13,   3,   3,  48,  44,  34,  54,
																																																			66,   7, 104,   1,  25,   1,   8,  14,   8,  34,
																																																			34,  72,  18,  38,   3,   7,  56,  14,   1,  38,
																																																			3,   7,  56,  14,   1,  38,   3,   7,  56,  14,
																																																			1,  25,  14,  24,  24,  50,  66,  50,  66,  22,
																																																			46,   3,   7,  46,   3,   7,   1,   1,   1,  18,
																																																			22,  22,   1,  30,  22,  12,  12,  88,  78,  56,
																																																			14,  66,  56,  50,   9,  56,  38,  22,   1,  30,
																																																			22,  12,  60,  22,  86,  60,  60,  22,   1,   1,
																																																			1,   1,   1,   1,   1
																																																		},
																																																		{
																																																			14, 124,  62,  10,  11,   1,   0,  37,  63,  90,
																																																				1,  10,   1,   1,   5,  30,  10,  50,   1,  42,
																																																				24,  50,  76, 124, 124,  14,  14,   4,  28,  20,
																																																				52,  14,   1,  78,  72,  20,   1,  11,  15,   1,
																																																				1,   1,  10,  10,   4,  14,  20,  14,  24,  40,
																																																				4,   1,   1,  50,   4,  11,   1,   1,  50,  46,
																																																				36,  56,   4,  14,  20,  14,  24,  40,   4,   1,
																																																				1,  50,   4,  11,   1,   1,  50,  46,  36,  56,
																																																				68,   5, 108,   1,  27,   1,  10,  14,  10,  36,
																																																				36,  76,  20,  36,   5,   5,  58,  14,   1,  36,
																																																				5,   5,  58,  14,   1,  36,   5,   5,  58,  14,
																																																				1,  27,  14,  26,  26,  52,  68,  52,  68,  20,
																																																				46,   5,   5,  46,   5,   5,   1,   1,   1,  16,
																																																				20,  20,   1,  30,  20,  10,  10,  90,  78,  58,
																																																				14,  68,  58,  52,  11,  54,  36,  20,   1,  30,
																																																				20,  10,  62,  20,  88,  62,  62,  20,   1,   1,
																																																				1,   1,   1,   1,   1
																																																		},
																																																		{
																																																			14, 124,  64,  10,  11,   1,   1,  37,  65,  90,
																																																				1,  10,   1,   1,   5,  30,  10,  54,   1,  42,
																																																				26,  54,  80, 124, 124,  14,  14,   4,  24,  20,
																																																				52,  14,   0,  78,  74,  20,   1,  11,  17,   1,
																																																				1,   1,  10,  10,   4,  16,  20,  16,  26,  42,
																																																				4,   0,   0,  54,   4,  11,   0,   0,  54,  48,
																																																				36,  58,   4,  16,  20,  16,  26,  42,   4,   0,
																																																				0,  54,   4,  11,   0,   0,  54,  48,  36,  58,
																																																				68,   5, 112,   1,  27,   1,  10,  14,  10,  36,
																																																				36,  80,  20,  36,   5,   5,  58,  14,   1,  36,
																																																				5,   5,  58,  14,   1,  36,   5,   5,  58,  14,
																																																				1,  27,  14,  26,  26,  52,  68,  52,  68,  20,
																																																				46,   5,   5,  46,   5,   5,   1,   1,   1,  14,
																																																				20,  20,   1,  30,  20,  10,  10,  90,  78,  58,
																																																				14,  68,  58,  52,  11,  52,  36,  20,   1,  30,
																																																				20,  10,  64,  20,  90,  64,  64,  20,   1,   1,
																																																				1,   1,   1,   1,   1
																																																			},
																																																			{
																																																				14, 124,  66,  10,   9,   1,   3,  39,  67,  90,
																																																					1,  10,   1,   1,   7,  30,  10,  56,   1,  42,
																																																					28,  56,  84, 124, 124,  14,  14,   4,  22,  20,
																																																					54,  14,   2,  78,  76,  20,   1,  11,  19,   1,
																																																					1,   1,  10,  10,   6,  18,  22,  18,  28,  44,
																																																					6,   2,   2,  56,   6,   9,   2,   2,  56,  50,
																																																					38,  60,   6,  18,  22,  18,  28,  44,   6,   2,
																																																					2,  56,   6,   9,   2,   2,  56,  50,  38,  60,
																																																					70,   5, 116,   1,  27,   1,  10,  14,  10,  38,
																																																					38,  84,  22,  36,   7,   5,  58,  14,   1,  36,
																																																					7,   5,  58,  14,   1,  36,   7,   5,  58,  14,
																																																					1,  27,  14,  26,  26,  54,  70,  54,  70,  20,
																																																					46,   7,   5,  46,   7,   5,   1,   1,   1,  12,
																																																					20,  20,   1,  30,  20,   8,   8,  90,  78,  58,
																																																					14,  70,  58,  54,  11,  48,  36,  20,   1,  30,
																																																					20,   8,  66,  20,  92,  66,  66,  20,   1,   1,
																																																					1,   1,   1,   1,   1
																																																			},
																																																			{
																																																				14, 124,  68,  12,   7,   1,   5,  41,  69,  92,
																																																					1,  12,   1,   1,   9,  30,  12,  60,   1,  44,
																																																					32,  60,  88, 124, 124,  14,  14,   2,  18,  18,
																																																					56,  14,   4,  78,  80,  18,   1,  13,  21,   1,
																																																					1,   1,  12,  12,   8,  20,  24,  20,  32,  48,
																																																					8,   4,   4,  60,   8,   7,   4,   4,  60,  52,
																																																					40,  64,   8,  20,  24,  20,  32,  48,   8,   4,
																																																					4,  60,   8,   7,   4,   4,  60,  52,  40,  64,
																																																					72,   3, 120,   1,  29,   1,  12,  14,  12,  40,
																																																					40,  88,  24,  34,   9,   3,  60,  14,   1,  34,
																																																					9,   3,  60,  14,   1,  34,   9,   3,  60,  14,
																																																					1,  29,  14,  28,  28,  56,  72,  56,  72,  18,
																																																					46,   9,   3,  46,   9,   3,   1,   1,   1,  10,
																																																					18,  18,   1,  30,  18,   6,   6,  92,  78,  60,
																																																					14,  72,  60,  56,  13,  46,  34,  18,   1,  30,
																																																					18,   6,  68,  18,  96,  68,  68,  18,   1,   1,
																																																					1,   1,   1,   1,   1
																																																				},
																																																				{
																																																					14, 124,  70,  12,   7,   1,   7,  41,  71,  92,
																																																						1,  12,   1,   1,   9,  30,  12,  62,   1,  44,
																																																						34,  62,  92, 124, 124,  14,  14,   2,  16,  18,
																																																						56,  14,   6,  78,  82,  18,   1,  13,  23,   1,
																																																						1,   1,  12,  12,   8,  22,  24,  22,  34,  50,
																																																						8,   6,   6,  62,   8,   7,   6,   6,  62,  54,
																																																						40,  66,   8,  22,  24,  22,  34,  50,   8,   6,
																																																						6,  62,   8,   7,   6,   6,  62,  54,  40,  66,
																																																						72,   3, 124,   1,  29,   1,  12,  14,  12,  40,
																																																						40,  92,  24,  34,   9,   3,  60,  14,   1,  34,
																																																						9,   3,  60,  14,   1,  34,   9,   3,  60,  14,
																																																						1,  29,  14,  28,  28,  56,  72,  56,  72,  18,
																																																						46,   9,   3,  46,   9,   3,   1,   1,   1,   8,
																																																						18,  18,   1,  30,  18,   6,   6,  92,  78,  60,
																																																						14,  72,  60,  56,  13,  44,  34,  18,   1,  30,
																																																						18,   6,  70,  18,  98,  70,  70,  18,   1,   1,
																																																						1,   1,   1,   1,   1
																																																				},
																																																				{
																																																					14, 124,  72,  12,   5,   1,   9,  43,  73,  92,
																																																						1,  12,   1,   1,  11,  30,  12,  66,   1,  44,
																																																						36,  66,  96, 124, 124,  14,  14,   2,  12,  18,
																																																						58,  14,   8,  78,  84,  18,   1,  13,  25,   1,
																																																						1,   1,  12,  12,  10,  24,  26,  24,  36,  52,
																																																						10,   8,   8,  66,  10,   5,   8,   8,  66,  56,
																																																						42,  68,  10,  24,  26,  24,  36,  52,  10,   8,
																																																						8,  66,  10,   5,   8,   8,  66,  56,  42,  68,
																																																						74,   3, 124,   1,  29,   1,  12,  14,  12,  42,
																																																						42,  96,  26,  34,  11,   3,  60,  14,   1,  34,
																																																						11,   3,  60,  14,   1,  34,  11,   3,  60,  14,
																																																						1,  29,  14,  28,  28,  58,  74,  58,  74,  18,
																																																						46,  11,   3,  46,  11,   3,   1,   1,   1,   6,
																																																						18,  18,   1,  30,  18,   4,   4,  92,  78,  60,
																																																						14,  74,  60,  58,  13,  42,  34,  18,   1,  30,
																																																						18,   4,  72,  18, 100,  72,  72,  18,   1,   1,
																																																						1,   1,   1,   1,   1
																																																					},
																																																					{
																																																						14, 124,  72,  12,   5,   1,  11,  45,  75,  92,
																																																							1,  12,   1,   1,  13,  30,  12,  68,   1,  44,
																																																							38,  68,  98, 124, 124,  14,  14,   0,   8,  16,
																																																							58,  14,   8,  78,  86,  16,   1,  15,  27,   1,
																																																							1,   1,  12,  12,  10,  24,  26,  24,  38,  54,
																																																							10,   8,   8,  68,  10,   5,   8,   8,  68,  56,
																																																							42,  70,  10,  24,  26,  24,  38,  54,  10,   8,
																																																							8,  68,  10,   5,   8,   8,  68,  56,  42,  70,
																																																							74,   3, 124,   1,  31,   1,  12,  14,  12,  42,
																																																							42,  98,  26,  32,  13,   3,  60,  14,   1,  32,
																																																							13,   3,  60,  14,   1,  32,  13,   3,  60,  14,
																																																							1,  31,  14,  28,  28,  58,  74,  58,  74,  16,
																																																							46,  13,   3,  46,  13,   3,   1,   1,   1,   4,
																																																							16,  16,   1,  30,  16,   2,   2,  92,  78,  60,
																																																							14,  74,  60,  58,  15,  38,  32,  16,   1,  30,
																																																							16,   2,  72,  16, 102,  72,  72,  16,   1,   1,
																																																							1,   1,   1,   1,   1
																																																					},
																																																					{
																																																						14, 124,  74,  14,   3,   1,  11,  45,  75,  94,
																																																							1,  14,   1,   1,  13,  30,  14,  72,   1,  46,
																																																							42,  72, 102, 124, 124,  14,  14,   0,   6,  16,
																																																							60,  14,  10,  78,  90,  16,   1,  15,  27,   1,
																																																							1,   1,  14,  14,  12,  26,  28,  26,  42,  58,
																																																							12,  10,  10,  72,  12,   3,  10,  10,  72,  58,
																																																							44,  74,  12,  26,  28,  26,  42,  58,  12,  10,
																																																							10,  72,  12,   3,  10,  10,  72,  58,  44,  74,
																																																							76,   1, 124,   1,  31,   1,  14,  14,  14,  44,
																																																							44, 102,  28,  32,  13,   1,  62,  14,   1,  32,
																																																							13,   1,  62,  14,   1,  32,  13,   1,  62,  14,
																																																							1,  31,  14,  30,  30,  60,  76,  60,  76,  16,
																																																							46,  13,   1,  46,  13,   1,   1,   1,   1,   4,
																																																							16,  16,   1,  30,  16,   2,   2,  94,  78,  62,
																																																							14,  76,  62,  60,  15,  36,  32,  16,   1,  30,
																																																							16,   2,  74,  16, 106,  74,  74,  16,   1,   1,
																																																							1,   1,   1,   1,   1
																																																						},
																																																						{
																																																							14, 124,  76,  14,   1,   1,  13,  47,  77,  94,
																																																								1,  14,   1,   1,  15,  30,  14,  76,   1,  46,
																																																								44,  76, 106, 124, 124,  14,  14,   0,   2,  16,
																																																								62,  14,  12,  78,  92,  16,   1,  15,  29,   1,
																																																								1,   1,  14,  14,  14,  28,  30,  28,  44,  60,
																																																								14,  12,  12,  76,  14,   1,  12,  12,  76,  60,
																																																								46,  76,  14,  28,  30,  28,  44,  60,  14,  12,
																																																								12,  76,  14,   1,  12,  12,  76,  60,  46,  76,
																																																								78,   1, 124,   1,  31,   1,  14,  14,  14,  46,
																																																								46, 106,  30,  32,  15,   1,  62,  14,   1,  32,
																																																								15,   1,  62,  14,   1,  32,  15,   1,  62,  14,
																																																								1,  31,  14,  30,  30,  62,  78,  62,  78,  16,
																																																								46,  15,   1,  46,  15,   1,   1,   1,   1,   2,
																																																								16,  16,   1,  30,  16,   0,   0,  94,  78,  62,
																																																								14,  78,  62,  62,  15,  34,  32,  16,   1,  30,
																																																								16,   0,  76,  16, 108,  76,  76,  16,   1,   1,
																																																								1,   1,   1,   1,   1
																																																						},
																																																						{
																																																							14, 124,  78,  14,   1,   1,  15,  47,  79,  94,
																																																								1,  14,   1,   1,  15,  30,  14,  78,   1,  46,
																																																								46,  78, 110, 124, 124,  14,  14,   0,   0,  16,
																																																								62,  14,  14,  78,  94,  16,   1,  15,  31,   1,
																																																								1,   1,  14,  14,  14,  30,  30,  30,  46,  62,
																																																								14,  14,  14,  78,  14,   1,  14,  14,  78,  62,
																																																								46,  78,  14,  30,  30,  30,  46,  62,  14,  14,
																																																								14,  78,  14,   1,  14,  14,  78,  62,  46,  78,
																																																								78,   1, 124,   1,  31,   1,  14,  14,  14,  46,
																																																								46, 110,  30,  32,  15,   1,  62,  14,   1,  32,
																																																								15,   1,  62,  14,   1,  32,  15,   1,  62,  14,
																																																								1,  31,  14,  30,  30,  62,  78,  62,  78,  16,
																																																								46,  15,   1,  46,  15,   1,   1,   1,   1,   0,
																																																								16,  16,   1,  30,  16,   0,   0,  94,  78,  62,
																																																								14,  78,  62,  62,  15,  32,  32,  16,   1,  30,
																																																								16,   0,  78,  16, 110,  78,  78,  16,   1,   1,
																																																								1,   1,   1,   1,   1
																																																							},
																												}
};
#pragma  endregion



//读入SPS  PPS参数
Sps_Pps_struct sps_pps_struct={0};
void read_Sps_Pps_to_sps_struct(TComSPS *m_sps , TComPPS  *m_pps , Sps_Pps_struct &sps_pps_struct)
{
	//SPS
	sps_pps_struct.video_parameter_set_id = m_sps->getVPSId();
	sps_pps_struct.seq_parameter_set_id = m_sps->getSPSId();
	sps_pps_struct.chroma_format_idc = m_sps->getChromaFormatIdc();//?  400,420,422,444  YUV格式选择
	sps_pps_struct.pic_width_in_luma_samples = m_sps->getPicWidthInLumaSamples();
	sps_pps_struct.pic_height_in_luma_samples = m_sps->getPicHeightInLumaSamples();
	sps_pps_struct.bit_depth_luma = m_sps->getBitDepthY();
	sps_pps_struct.bit_depth_chroma = m_sps->getBitDepthC();
	sps_pps_struct.log2_max_pic_order_cnt_lsb = m_sps->getBitsForPOC();//?
	sps_pps_struct.log2_max_luma_coding_block_size = m_sps->getLog2MinCodingBlockSize()+m_sps->getLog2DiffMaxMinCodingBlockSize();
	sps_pps_struct.log2_maxa_coding_block_depth = m_sps->getLog2DiffMaxMinCodingBlockSize();
	sps_pps_struct.log2_max_transform_block_size = m_sps->getQuadtreeTULog2MaxSize();
	sps_pps_struct.log2_max_transform_block_depth = m_sps->getQuadtreeTULog2MaxSize() - m_sps->getQuadtreeTULog2MinSize();
	sps_pps_struct.max_transform_hierarchy_depth_inter = m_sps->getQuadtreeTUMaxDepthInter();
	sps_pps_struct.max_transform_hierarchy_depth_intra = m_sps->getQuadtreeTUMaxDepthIntra();
	sps_pps_struct.amp_enabled_flag = m_sps->getUseAMP();
	sps_pps_struct.sample_adaptive_offset_enabled_flag = m_sps->getUseSAO();
	sps_pps_struct.sps_temporal_mvp_enabled_flag = m_sps->getTMVPFlagsPresent();
	sps_pps_struct.sps_strong_intra_smoothing_enabled_flag = m_sps->getUseStrongIntraSmoothing();
	sps_pps_struct.scaling_list_enabled_flag = m_sps->getScalingListFlag();
	sps_pps_struct.num_short_term_ref_pic_sets = m_sps->getRPSList()->getNumberOfReferencePictureSets();
	sps_pps_struct.long_term_ref_pics_present_flag = m_sps->getLongTermRefsPresent();
	sps_pps_struct.num_long_term_ref_pics_sps = m_sps->getNumLongTermRefPicSPS();
	sps_pps_struct.pcm_enabled_flag = m_sps->getUsePCM();
	sps_pps_struct.pcm_sample_bit_depth_luma = m_sps->getPCMBitDepthLuma();
	sps_pps_struct.pcm_sample_bit_depth_chroma = m_sps->getPCMBitDepthChroma();
	sps_pps_struct.log2_max_pcm_luma_coding_block_depth = m_sps->getPCMLog2MaxSize() - m_sps->getPCMLog2MinSize();
	sps_pps_struct.log2_max_pcm_luma_coding_block_size = m_sps->getPCMLog2MaxSize();
	sps_pps_struct.pcm_loop_filter_disabled_flag = m_sps->getPCMFilterDisableFlag();

	sps_pps_struct.transform_skip_rotation_enabled_flag = 0;
	sps_pps_struct.transform_skip_context_enabled_flag = 0;
	sps_pps_struct.intra_block_copy_enabled_flag = 0;
	sps_pps_struct.residual_dpcm_intra_enabled_flag = 0;
	sps_pps_struct.residual_dpcm_inter_enabled_flag = 0;
	sps_pps_struct.extended_precision_processing_flag = 0;
	sps_pps_struct.intra_smoothing_disabled_flag = 0;

	///***///
	sps_pps_struct.CoeffMin_y 	= -(1 << (sps_pps_struct.extended_precision_processing_flag ? MAX(15, sps_pps_struct.bit_depth_luma + 6) : 15));
	sps_pps_struct.CoeffMin_c 	= -(1 << (sps_pps_struct.extended_precision_processing_flag ? MAX(15, sps_pps_struct.bit_depth_chroma + 6) : 15));
	sps_pps_struct.CoeffMax_y 	= (1 << (sps_pps_struct.extended_precision_processing_flag ? MAX(15, sps_pps_struct.bit_depth_luma + 6) : 15)) - 1;
	sps_pps_struct.CoeffMax_c 	= (1 << (sps_pps_struct.extended_precision_processing_flag ? MAX(15, sps_pps_struct.bit_depth_chroma + 6) : 15)) - 1;


	sps_pps_struct.coeff_max_len_y	= (sps_pps_struct.extended_precision_processing_flag ? MAX(15, sps_pps_struct.bit_depth_luma + 6) : 15) + 1;
	sps_pps_struct.coeff_max_len_c	= (sps_pps_struct.extended_precision_processing_flag ? MAX(15, sps_pps_struct.bit_depth_chroma + 6) : 15) + 1;
	sps_pps_struct.QpBdOffsetY          = 6 * (sps_pps_struct.bit_depth_luma - 8);
	sps_pps_struct.QpBdOffsetC          = 6 * (sps_pps_struct.bit_depth_chroma - 8);

	sps_pps_struct.log2_min_pcm_luma_coding_block_size = sps_pps_struct.log2_max_pcm_luma_coding_block_size -sps_pps_struct.log2_max_pcm_luma_coding_block_depth;
	sps_pps_struct.log2_min_luma_coding_block_size =  sps_pps_struct.log2_max_luma_coding_block_size - sps_pps_struct.log2_maxa_coding_block_depth;
	sps_pps_struct.log2_min_transform_block_size = sps_pps_struct.log2_max_transform_block_size - sps_pps_struct.log2_max_transform_block_depth;

	sps_pps_struct.MaxCUWidth = 1 << (sps_pps_struct.log2_max_luma_coding_block_size);
#if 0//!USE_MIN_CU_TU_SIZE
	sps_pps_struct.log2_min_tu_size = sps_pps_struct.log2_min_transform_block_size;
	sps_pps_struct.log2_min_cu_size = sps_pps_struct.log2_min_luma_coding_block_size;
	sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu = sps_pps_struct.log2_max_luma_coding_block_size -
		(sps_pps_struct.log2_max_transform_block_size -log2_max_transform_block_depth) ;
	sps_pps_struct.log2_min_coding_block_depth_bt_cu_tu = sps_pps_struct.log2_max_luma_coding_block_size -
		sps_pps_struct.log2_maxa_coding_block_depth - (sps_pps_struct.log2_max_transform_block_size -sps_pps_struct.log2_max_transform_block_depth);
	sps_pps_struct.log2_max_coding_depth_for_dec = sps_pps_struct.log2_maxa_coding_block_depth;
#else
	sps_pps_struct.log2_min_tu_size = 2;
	sps_pps_struct.log2_min_cu_size = 3;
	sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu = sps_pps_struct.log2_max_luma_coding_block_size - sps_pps_struct.log2_min_tu_size ;
	sps_pps_struct.log2_min_coding_block_depth_bt_cu_tu =sps_pps_struct.log2_min_cu_size -sps_pps_struct.log2_min_tu_size;

	sps_pps_struct.log2_max_coding_depth_for_dec = sps_pps_struct.log2_max_luma_coding_block_size - sps_pps_struct.log2_min_cu_size;
#endif
	sps_pps_struct.Max_cu_in_ctu = 1 << (  sps_pps_struct.log2_max_coding_depth_for_dec << 1);




	sps_pps_struct.Max_tu_in_ctu = 1 << (  sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu << 1);


	sps_pps_struct.log2_max_coding_block_depth_in_pcm = sps_pps_struct.log2_max_luma_coding_block_size -
		(sps_pps_struct.log2_max_pcm_luma_coding_block_size -sps_pps_struct.log2_max_pcm_luma_coding_block_depth) ;
	sps_pps_struct.log2_min_coding_block_depth_in_pcm = sps_pps_struct.log2_max_luma_coding_block_size -
		sps_pps_struct.log2_maxa_coding_block_depth -
		(sps_pps_struct.log2_max_pcm_luma_coding_block_size -sps_pps_struct.log2_max_pcm_luma_coding_block_depth) ;

	sps_pps_struct.cu_in_ctu_per_line = 1 << sps_pps_struct.log2_max_coding_depth_for_dec;
	sps_pps_struct.tu_in_ctu_per_line = 1 << sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu;
	sps_pps_struct.ctu_width_in_pic = (sps_pps_struct.pic_width_in_luma_samples  + (1 << sps_pps_struct.log2_max_luma_coding_block_size) - 1) / (1 << sps_pps_struct.log2_max_luma_coding_block_size) ;
	sps_pps_struct.ctu_height_in_pic = (sps_pps_struct.pic_height_in_luma_samples  + (1 << sps_pps_struct.log2_max_luma_coding_block_size) - 1) / (1 << sps_pps_struct.log2_max_luma_coding_block_size);

	sps_pps_struct.ctu_num_in_pic = sps_pps_struct.ctu_width_in_pic * sps_pps_struct.ctu_height_in_pic;
	sps_pps_struct.RawCtuBits = sps_pps_struct.MaxCUWidth * sps_pps_struct.MaxCUWidth * sps_pps_struct.bit_depth_luma + sps_pps_struct.MaxCUWidth * sps_pps_struct.MaxCUWidth * sps_pps_struct.bit_depth_chroma /2 ;

	sps_pps_struct.mExisted = 1;
	sps_pps_struct.Sps_length = 0;

	if(sps_pps_struct.log2_max_coding_depth_for_dec == 3)
		memcpy(cu_ZscanToRaster,zsantoraster_depth_3,sizeof (zsantoraster_depth_3));
	else	if(sps_pps_struct.log2_max_coding_depth_for_dec == 2)
		memcpy(cu_ZscanToRaster,zsantoraster_depth_2,sizeof (zsantoraster_depth_2));
	else	if(sps_pps_struct.log2_max_coding_depth_for_dec == 1)
		memcpy(cu_ZscanToRaster,zsantoraster_depth_1,sizeof (zsantoraster_depth_1));
	else	if(sps_pps_struct.log2_max_coding_depth_for_dec == 0)
		memcpy(cu_ZscanToRaster,zsantoraster_depth_0,sizeof (zsantoraster_depth_0));


	if(sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu == 4)
		memcpy(tu_ZscanToRaster,zsantoraster_depth_4,sizeof (zsantoraster_depth_4));
	else	if(sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu == 3)
		memcpy(tu_ZscanToRaster,zsantoraster_depth_3,sizeof (zsantoraster_depth_3));
	else	if(sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu == 2)
		memcpy(tu_ZscanToRaster,zsantoraster_depth_2,sizeof (zsantoraster_depth_2));
	else	if(sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu == 1)
		memcpy(tu_ZscanToRaster,zsantoraster_depth_1,sizeof (zsantoraster_depth_1));
	else	if(sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu == 0)
		memcpy(tu_ZscanToRaster,zsantoraster_depth_0,sizeof (zsantoraster_depth_0));

	//PPS
	sps_pps_struct.pps_pic_parameter_set_id = m_pps->getPPSId();	
	sps_pps_struct.pps_seq_parameter_set_id = m_pps->getSPSId(); 				
	sps_pps_struct.dependent_slice_segments_enabled_flag = 0;						
	sps_pps_struct.output_flag_present_flag = m_pps->getOutputFlagPresentFlag();		//?
	sps_pps_struct.num_extra_slice_header_bits = m_pps->getNumExtraSliceHeaderBits();				//?  预留
	sps_pps_struct.sign_data_hiding_flag = m_pps->getSignHideFlag();					//?
	sps_pps_struct.cabac_init_present_flag = m_pps->getCabacInitPresentFlag();				//?  为1表示P帧和B帧初始上下文对换    出于什么考虑？
	sps_pps_struct.num_ref_idx_l0_default_active = m_pps->getNumRefIdxL0DefaultActive();				
	sps_pps_struct.num_ref_idx_l1_default_active = m_pps->getNumRefIdxL1DefaultActive();				
	sps_pps_struct.init_qp = m_pps->getPicInitQPMinus26() + 26;					
	sps_pps_struct.constrained_intra_pred_flag = m_pps->getConstrainedIntraPred();				//?  为1时做INTRA预测要求变高，如果周围块是inter的，则不用来生成预测像素
	sps_pps_struct.transform_skip_enabled_flag = m_pps->getUseTransformSkip();		
	sps_pps_struct.cu_qp_delta_enabled_flag = m_pps->getUseDQP();					
	sps_pps_struct.Log2MinCuQpDeltaSize = m_pps->getMinCuDQPSize();					
	sps_pps_struct.pps_cb_qp_offset = m_pps->getChromaCbQpOffset();				
	sps_pps_struct.pps_cr_qp_offset = m_pps->getChromaCrQpOffset();				
	sps_pps_struct.pps_slice_chroma_qp_offsets_present_flag = m_pps->getSliceChromaQpFlag();				
	sps_pps_struct.weighted_pred_flag = m_pps->getUseWP();					//?  前向加权预测是如何加权的？
	sps_pps_struct.weighted_bipred_flag = m_pps->getWPBiPred();				
	sps_pps_struct.transquant_bypass_enabled_flag = m_pps->getTransquantBypassEnableFlag();				
	sps_pps_struct.tiles_enabled_flag = 0;			
	sps_pps_struct.entropy_coding_sync_enabled_flag = m_pps->getEntropyCodingSyncEnabledFlag();			//?
	sps_pps_struct.pps_loop_filter_across_slices_enabled_flag = 1;				
	sps_pps_struct.loop_filter_across_tiles_enabled_flag = 1;			
	sps_pps_struct.deblocking_filter_override_enabled_flag = m_pps->getDeblockingFilterOverrideEnabledFlag();			//?
	sps_pps_struct.pps_deblocking_filter_disabled_flag = m_pps->getPicDisableDeblockingFilterFlag();				
	sps_pps_struct.pps_beta_offset_div2 = m_pps->getDeblockingFilterBetaOffsetDiv2();				
	sps_pps_struct.pps_tc_offset_div2 = m_pps->getDeblockingFilterTcOffsetDiv2();				
	sps_pps_struct.lists_modification_present_flag = m_pps->getListsModificationPresentFlag();			//?
	sps_pps_struct.log2_parallel_merge_level = m_pps->getLog2ParallelMergeLevelMinus2() + 2;			//?
	sps_pps_struct.slice_segment_header_extension_present_flag = m_pps->getSliceHeaderExtensionPresentFlag();	//?
	sps_pps_struct.log2_transform_skip_max_size = 0;				
	sps_pps_struct.num_tile_columns = 0;				
	sps_pps_struct.num_tile_rows = 0;				
}

//读入Slice_header参数
Slice_header_struct slice_header_struct = {0};
void CABAC_codeSliceHeader(TComSlice* slice , Slice_header_struct &slice_header_struct)
{

	//calculate number of bits required for slice address
	int maxSliceSegmentAddress = slice->getPic()->getNumCUsInFrame();
	int bitsSliceSegmentAddress = 0;
	while (maxSliceSegmentAddress > (1 << bitsSliceSegmentAddress))
	{
		bitsSliceSegmentAddress++;
	}

	//write slice address
	int sliceSegmentAddress = 0;

	//WRITE_FLAG(sliceSegmentAddress == 0, "first_slice_segment_in_pic_flag");
	slice_header_struct.First_slice_segment_in_pic_flag = sliceSegmentAddress == 0;

	if (slice->getRapPicFlag())
	{
		//WRITE_FLAG(0, "no_output_of_prior_pics_flag");
		slice_header_struct.No_output_of_prior_pics_flag = 0;
	}

	//WRITE_UVLC(slice->getPPS()->getPPSId(), "slice_pic_parameter_set_id");
	slice_header_struct.Slice_pic_parameter_set_id = slice->getPPS()->getPPSId();

	slice->setDependentSliceSegmentFlag(!slice->isNextSlice());
// 	if (sliceSegmentAddress > 0)
// 	{
// 		WRITE_CODE(sliceSegmentAddress, bitsSliceSegmentAddress, "slice_segment_address");
// 	}
	slice_header_struct.Dependent_slice_segment_flag = 0;
	slice_header_struct.Slice_segment_address = 0;


	if (!slice_header_struct.Dependent_slice_segment_flag)
	{
// 		for (int i = 0; i < slice->getPPS()->getNumExtraSliceHeaderBits(); i++)
// 		{
// 			assert(!!"slice_reserved_undetermined_flag[]");
// 			WRITE_FLAG(0, "slice_reserved_undetermined_flag[]");
// 		}

//		WRITE_UVLC(slice->getSliceType(),       "slice_type");
		slice_header_struct.slice_type = slice->getSliceType();

		if (slice->getPPS()->getOutputFlagPresentFlag())
		{
//			WRITE_FLAG(slice->getPicOutputFlag() ? 1 : 0, "pic_output_flag");
			slice_header_struct.Pic_output_flag = slice->getPicOutputFlag() ? 1 : 0 ;
		}

		// in the first version chroma_format_idc is equal to one, thus colour_plane_id will not be present
		assert(slice->getSPS()->getChromaFormatIdc() == 1);
		// if( separate_colour_plane_flag  ==  1 )
		//   colour_plane_id                                      u(2)
		slice_header_struct.Colour_plane_id = 0;

		if (!slice->getIdrPicFlag())
		{
			int picOrderCntLSB = (slice->getPOC() - slice->getLastIDR() + (1 << slice->getSPS()->getBitsForPOC())) % (1 << slice->getSPS()->getBitsForPOC());
			//WRITE_CODE(picOrderCntLSB, slice->getSPS()->getBitsForPOC(), "pic_order_cnt_lsb");
			slice_header_struct.Slice_pic_order_cnt_lsb = picOrderCntLSB;
			TComReferencePictureSet* rps = slice->getRPS();

			// check for bitstream restriction stating that:
			// If the current picture is a BLA or CRA picture, the value of NumPocTotalCurr shall be equal to 0.
			// Ideally this process should not be repeated for each slice in a picture
			if (slice->isIRAP())
			{
				for (int picIdx = 0; picIdx < rps->getNumberOfPictures(); picIdx++)
				{
					assert(!rps->getUsed(picIdx));
				}
			}

			if (slice->getRPSidx() < 0)
			{
//				WRITE_FLAG(0, "short_term_ref_pic_set_sps_flag");
				slice_header_struct.Short_term_ref_pic_set_sps_flag = 0;
//				codeShortTermRefPicSet(rps, true, slice->getSPS()->getRPSList()->getNumberOfReferencePictureSets());
				if (sps_pps_struct.num_short_term_ref_pic_sets!=0)
				{
					slice_header_struct.inter_ref_pic_set_prediction_flag = rps->getInterRPSPrediction();
				}
				if (slice_header_struct.inter_ref_pic_set_prediction_flag == 1)
				{
					slice_header_struct.delta_idx_minus1 = rps->getDeltaRIdxMinus1();
					slice_header_struct.Delta_rps_sign = (rps->getDeltaRPS())>=0?0:1;
					slice_header_struct.Abs_delta_rps_minus1 = abs(rps->getDeltaRPS()) - 1;
					for (int j = 0; j<rps->getNumRefIdc();j++)
					{
						slice_header_struct.Used_by_curr_pic_flag[j] = rps->getRefIdc(j) == 1?1:0;
						if (rps->getRefIdc(j) != 0)
						{
							slice_header_struct.Use_delta_flag[j]  = ((rps->getRefIdc(j))>>1);
						}
					}
				} 
				else
				{
					slice_header_struct.Num_negative_pics = rps->getNumberOfNegativePictures();
					slice_header_struct.Num_positive_pics = rps->getNumberOfPositivePictures();
					int prev = 0;
					for (uint32_t i=0 ; i <slice_header_struct.Num_negative_pics ;i++ )
					{
						slice_header_struct.delta_poc_s0_minus1[i] = prev - rps->getDeltaPOC(i) - 1;
						prev = rps->getDeltaPOC(i);
						slice_header_struct.used_by_curr_pic_s0_flag[i] = rps->getUsed(i);
					}
					 prev = 0;
					for (uint32_t i=slice_header_struct.Num_negative_pics ; i <(slice_header_struct.Num_positive_pics + slice_header_struct.Num_positive_pics) ;i++ )
					{
						slice_header_struct.delta_poc_s0_minus1[i] =  rps->getDeltaPOC(i) -prev - 1;
						prev = rps->getDeltaPOC(i);
						slice_header_struct.used_by_curr_pic_s0_flag[i] = rps->getUsed(i);
					}
				}
			}
			else
			{
//				WRITE_FLAG(1, "short_term_ref_pic_set_sps_flag");
				slice_header_struct.Short_term_ref_pic_set_sps_flag = 1;
				int numBits = 0;
				while ((1 << numBits) < slice->getSPS()->getRPSList()->getNumberOfReferencePictureSets())
				{
					numBits++;
				}

				if (numBits > 0)
				{
//					WRITE_CODE(slice->getRPSidx(), numBits, "short_term_ref_pic_set_idx");
					slice_header_struct.Short_term_ref_pic_set_sps_flag = slice->getRPSidx();
				}
			}
			if (slice->getSPS()->getLongTermRefsPresent())
			{
				int numLtrpInSH = rps->getNumberOfLongtermPictures();
				int ltrpInSPS[MAX_NUM_REF_PICS];
				int numLtrpInSPS = 0;
				uint32_t ltrpIndex;
				int counter = 0;
				for (int k = rps->getNumberOfPictures() - 1; k > rps->getNumberOfPictures() - rps->getNumberOfLongtermPictures() - 1; k--)
				{
					if (CABAC_findMatchingLTRP(slice, &ltrpIndex, rps->getPOC(k), rps->getUsed(k)))
					{
						ltrpInSPS[numLtrpInSPS] = ltrpIndex;
						numLtrpInSPS++;
					}
					else
					{
						counter++;
					}
				}

				numLtrpInSH -= numLtrpInSPS;

				int bitsForLtrpInSPS = 0;
				while (slice->getSPS()->getNumLongTermRefPicSPS() > (uint32_t)(1 << bitsForLtrpInSPS))
				{
					bitsForLtrpInSPS++;
				}

				if (slice->getSPS()->getNumLongTermRefPicSPS() > 0)
				{
//					WRITE_UVLC(numLtrpInSPS, "num_long_term_sps");
					slice_header_struct.Num_long_term_sps = numLtrpInSPS;
				}
				//WRITE_UVLC(numLtrpInSH, "num_long_term_pics");
				slice_header_struct.Num_long_term_pics = numLtrpInSH;
				// Note that the LSBs of the LT ref. pic. POCs must be sorted before.
				// Not sorted here because LT ref indices will be used in setRefPicList()
				int prevDeltaMSB = 0, prevLSB = 0;
				int offset = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures();
				for (int i = rps->getNumberOfPictures() - 1; i > offset - 1; i--)
				{
					if (counter < numLtrpInSPS)
					{
						if (bitsForLtrpInSPS > 0)
						{
//							WRITE_CODE(ltrpInSPS[counter], bitsForLtrpInSPS, "lt_idx_sps[i]");
							slice_header_struct.lt_idx_sps[i] = ltrpInSPS[counter];
						}
					}
					else
					{
//						WRITE_CODE(rps->getPocLSBLT(i), slice->getSPS()->getBitsForPOC(), "poc_lsb_lt");//[i]
						slice_header_struct.poc_lsb_lt[i] = rps->getPocLSBLT(i);
//						WRITE_FLAG(rps->getUsed(i), "used_by_curr_pic_lt_flag");//[i]
						slice_header_struct.used_by_curr_pic_lt_flag[i] = rps->getUsed(i);
					}
//					WRITE_FLAG(rps->getDeltaPocMSBPresentFlag(i), "delta_poc_msb_present_flag");//[i]
					slice_header_struct.delta_poc_msb_present_flag[i] = rps->getDeltaPocMSBPresentFlag(i);

					if (rps->getDeltaPocMSBPresentFlag(i))
					{
						bool deltaFlag = false;
						//  First LTRP from SPS                 ||  First LTRP from SH                              || curr LSB            != prev LSB
						if ((i == rps->getNumberOfPictures() - 1) || (i == rps->getNumberOfPictures() - 1 - numLtrpInSPS) || (rps->getPocLSBLT(i) != prevLSB))
						{
							deltaFlag = true;
						}
						if (deltaFlag)
						{
//							WRITE_UVLC(rps->getDeltaPocMSBCycleLT(i), "delta_poc_msb_cycle_lt[i]");
							slice_header_struct.delta_poc_msb_cycle_lt[ i ] = rps->getDeltaPocMSBCycleLT(i);
						}
						else
						{
							int differenceInDeltaMSB = rps->getDeltaPocMSBCycleLT(i) - prevDeltaMSB;
							assert(differenceInDeltaMSB >= 0);
//							WRITE_UVLC(differenceInDeltaMSB, "delta_poc_msb_cycle_lt[i]");
							slice_header_struct.delta_poc_msb_cycle_lt[ i ] = differenceInDeltaMSB;
						}
						prevLSB = rps->getPocLSBLT(i);
						prevDeltaMSB = rps->getDeltaPocMSBCycleLT(i);
					}
				}
			}
			if (slice->getSPS()->getTMVPFlagsPresent())
			{
//				WRITE_FLAG(slice->getEnableTMVPFlag() ? 1 : 0, "slice_temporal_mvp_enable_flag");
				slice_header_struct.Slice_temporal_mvp_enabled_flag = (slice->getEnableTMVPFlag() ? 1 : 0);
			}
		}
		if (slice->getSPS()->getUseSAO())
		{
			if (slice->getSPS()->getUseSAO())
			{
//				WRITE_FLAG(slice->getSaoEnabledFlag(), "slice_sao_luma_flag");
				slice_header_struct.slice_sao_luma_flag = slice->getSaoEnabledFlag();
				{
					SAOParam *saoParam = slice->getPic()->getPicSym()->getSaoParam();
//					WRITE_FLAG(saoParam->bSaoFlag[1], "slice_sao_chroma_flag");
					slice_header_struct.slice_sao_chroma_flag = saoParam->bSaoFlag[1];
				}
			}
		}

		//check if numrefidxes match the defaults. If not, override

		if (slice_header_struct.slice_type != 2)
		{
			bool overrideFlag = ((uint32_t)slice->getNumRefIdx(REF_PIC_LIST_0) != slice->getPPS()->getNumRefIdxL0DefaultActive() || (slice->isInterB() && (uint32_t)slice->getNumRefIdx(REF_PIC_LIST_1) != slice->getPPS()->getNumRefIdxL1DefaultActive()));
//			WRITE_FLAG(overrideFlag ? 1 : 0,                               "num_ref_idx_active_override_flag");
			slice_header_struct.Num_ref_idx_active_override_flag = overrideFlag ? 1 : 0;
			if (overrideFlag)
			{
//				WRITE_UVLC(slice->getNumRefIdx(REF_PIC_LIST_0) - 1,      "num_ref_idx_l0_active_minus1");
				slice_header_struct.num_ref_idx_l0_active_minus1 = slice->getNumRefIdx(REF_PIC_LIST_0) - 1;
				if (slice->isInterB())
				{
//					WRITE_UVLC(slice->getNumRefIdx(REF_PIC_LIST_1) - 1,    "num_ref_idx_l1_active_minus1");
				slice_header_struct.num_ref_idx_l1_active_minus1 = slice->getNumRefIdx(REF_PIC_LIST_1) - 1;
				}
				else
				{
					slice->setNumRefIdx(REF_PIC_LIST_1, 0);
					slice_header_struct.num_ref_idx_l1_active_minus1 = 0 ;
				}
			}
			else
			{
				slice_header_struct.num_ref_idx_l0_active_minus1 = slice->getPPS()->getNumRefIdxL0DefaultActive() -1;
				slice_header_struct.num_ref_idx_l1_active_minus1 = slice->getPPS()->getNumRefIdxL1DefaultActive() -1;
			}
		}
		else
		{
			slice->setNumRefIdx(REF_PIC_LIST_0, 0);
			slice->setNumRefIdx(REF_PIC_LIST_1, 0);
			slice_header_struct.num_ref_idx_l0_active_minus1 = 0 ;
			slice_header_struct.num_ref_idx_l1_active_minus1 = 0 ;
		}

//		if( lists_modification_present_flag && NumPocTotalCurr > 1 )  应该是不会成立
//		ref_pic_list_modification_flag_l0
//			list_entry_l0[num_ref_idx_l0_active_minus1+1]
//		ref_pic_list_modification_flag_l1
//			list_entry_l1[num_ref_idx_l1_active_minus1+1]


		if (slice->isInterB())
		{
//			WRITE_FLAG(slice->getMvdL1ZeroFlag() ? 1 : 0,   "mvd_l1_zero_flag");
			slice_header_struct.Mvd_l1_zero_flag = slice->getMvdL1ZeroFlag() ? 1 : 0;
		}

		if (!slice->isIntra())
		{
			if (!slice->isIntra() && slice->getPPS()->getCabacInitPresentFlag())
			{
				SliceType sliceType   = slice->getSliceType();
				int  encCABACTableIdx = slice->getPPS()->getEncCABACTableIdx();
				bool encCabacInitFlag = (sliceType != encCABACTableIdx && encCABACTableIdx != I_SLICE) ? true : false;
				slice->setCabacInitFlag(encCabacInitFlag);
//				WRITE_FLAG(encCabacInitFlag ? 1 : 0, "cabac_init_flag");
				slice_header_struct.Cabac_init_flag = encCabacInitFlag ? 1 : 0;
			}
		}

		if (slice->getEnableTMVPFlag())
		{
			if (slice->getSliceType() == B_SLICE)
			{
//				WRITE_FLAG(slice->getColFromL0Flag(), "collocated_from_l0_flag");
				slice_header_struct.collocated_from_l0_flag = slice->getColFromL0Flag();
			}

			if (slice->getSliceType() != I_SLICE &&
				((slice->getColFromL0Flag() == 1 && slice->getNumRefIdx(REF_PIC_LIST_0) > 1) ||
				(slice->getColFromL0Flag() == 0  && slice->getNumRefIdx(REF_PIC_LIST_1) > 1)))
			{
//				WRITE_UVLC(slice->getColRefIdx(), "collocated_ref_idx");
				slice_header_struct.collocated_ref_idx = slice->getColRefIdx();
			}
		}
		if ((slice->getPPS()->getUseWP() && slice->getSliceType() == P_SLICE) || (slice->getPPS()->getWPBiPred() && slice->getSliceType() == B_SLICE))
		{
			CABAC_xCodePredWeightTable(slice , slice_header_struct);
		}
		assert(slice->getMaxNumMergeCand() <= MRG_MAX_NUM_CANDS);
		if (!slice->isIntra())
		{
//			WRITE_UVLC(MRG_MAX_NUM_CANDS - slice->getMaxNumMergeCand(), "five_minus_max_num_merge_cand");
			slice_header_struct.Five_minus_max_num_merge_cand = MRG_MAX_NUM_CANDS - slice->getMaxNumMergeCand();
		}
		int code = slice->getSliceQp() - (slice->getPPS()->getPicInitQPMinus26() + 26);
//		WRITE_SVLC(code, "slice_qp_delta");
		slice_header_struct.Slice_qp_delta = code;

		if (slice->getPPS()->getSliceChromaQpFlag())
		{
			code = slice->getSliceQpDeltaCb();
//			WRITE_SVLC(code, "slice_qp_delta_cb");//slice_cb_qp_offset 协议命名
			slice_header_struct.slice_cb_qp_offset = code;
			code = slice->getSliceQpDeltaCr();
//			WRITE_SVLC(code, "slice_qp_delta_cr");//slice_cr_qp_offset 协议命名
			slice_header_struct.slice_cr_qp_offset = code;
		} 
		if (slice->getPPS()->getDeblockingFilterControlPresentFlag())
		{
			if (slice->getPPS()->getDeblockingFilterOverrideEnabledFlag())
			{
//				WRITE_FLAG(slice->getDeblockingFilterOverrideFlag(), "deblocking_filter_override_flag");
				slice_header_struct.Deblocking_filter_overide_flag = slice->getDeblockingFilterOverrideFlag();
			}
			if (slice->getDeblockingFilterOverrideFlag())
			{
//				WRITE_FLAG(slice->getDeblockingFilterDisable(), "slice_disable_deblocking_filter_flag");
				slice_header_struct.Slice_deblocking_filter_disabled_flag = slice->getDeblockingFilterDisable();
				if (!slice->getDeblockingFilterDisable())
				{
//					WRITE_SVLC(slice->getDeblockingFilterBetaOffsetDiv2(), "slice_beta_offset_div2");
					slice_header_struct.Slice_beta_offset_div2 = slice->getDeblockingFilterBetaOffsetDiv2();
//					WRITE_SVLC(slice->getDeblockingFilterTcOffsetDiv2(),   "slice_tc_offset_div2");
					slice_header_struct.Slice_tc_offset_div2 = slice->getDeblockingFilterTcOffsetDiv2();
				}
			}
		}

		bool isSAOEnabled = (!slice->getSPS()->getUseSAO()) ? (false) : (slice->getSaoEnabledFlag() || slice->getSaoEnabledFlagChroma());
		bool isDBFEnabled = (!slice->getDeblockingFilterDisable());

		if (isSAOEnabled || isDBFEnabled)
		{
//			WRITE_FLAG(1, "slice_loop_filter_across_slices_enabled_flag");
			slice_header_struct.Slice_loop_filter_across_slices_enabled_flag = 1;
		}
	}
	if (slice->getPPS()->getSliceHeaderExtensionPresentFlag())
	{
//		WRITE_UVLC(0, "slice_header_extension_length");
	}
}
void CABAC_xCodePredWeightTable(TComSlice* slice , Slice_header_struct &slice_header_struct)
{
	wpScalingParam  *wp;
	bool            bChroma      = true; // color always present in HEVC ?
	int             iNbRef       = (slice->getSliceType() == B_SLICE) ? (2) : (1);
	bool            bDenomCoded  = false;
	uint32_t            mode = 0;
	uint32_t            totalSignalledWeightFlags = 0;

	if ((slice->getSliceType() == P_SLICE && slice->getPPS()->getUseWP()) || (slice->getSliceType() == B_SLICE && slice->getPPS()->getWPBiPred()))
	{
		mode = 1; // explicit
	}
	if (mode == 1)
	{
		for (int picList = 0; picList < iNbRef; picList++)
		{
			for (int refIdx = 0; refIdx < slice->getNumRefIdx(picList); refIdx++)
			{
				slice->getWpScaling(picList, refIdx, wp);
				if (!bDenomCoded)
				{
					int iDeltaDenom;
//					WRITE_UVLC(wp[0].log2WeightDenom, "luma_log2_weight_denom"); // ue(v): luma_log2_weight_denom
					slice_header_struct.luma_log2_weight_denom = wp[0].log2WeightDenom;

					if (bChroma)
					{
						iDeltaDenom = (wp[1].log2WeightDenom - wp[0].log2WeightDenom);
//						WRITE_SVLC(iDeltaDenom, "delta_chroma_log2_weight_denom"); // se(v): delta_chroma_log2_weight_denom
						slice_header_struct.chroma_log2_weight_denom = iDeltaDenom + wp[1].log2WeightDenom;//注意delta的区别
					}
					bDenomCoded = true;
				}
//				WRITE_FLAG(wp[0].bPresentFlag, "luma_weight_lX_flag");         // u(1): luma_weight_lX_flag  //[i]
				picList==0? slice_header_struct.luma_weight_l0_flag[refIdx] = wp[0].bPresentFlag : slice_header_struct.luma_weight_l1_flag[refIdx] = wp[0].bPresentFlag ;
				
				totalSignalledWeightFlags += wp[0].bPresentFlag;
			}

			if (bChroma)
			{
				for (int refIdx = 0; refIdx < slice->getNumRefIdx(picList); refIdx++)
				{
					slice->getWpScaling(picList, refIdx, wp);
//					WRITE_FLAG(wp[1].bPresentFlag, "chroma_weight_lX_flag");   // u(1): chroma_weight_lX_flag //[i]
					picList==0? slice_header_struct.Chroma_weight_l0_flag[refIdx] = wp[1].bPresentFlag : slice_header_struct.Chroma_weight_l1_flag[refIdx] = wp[1].bPresentFlag;
					
					totalSignalledWeightFlags += 2 * wp[1].bPresentFlag;
				}
			}

			for (int refIdx = 0; refIdx < slice->getNumRefIdx(picList); refIdx++)
			{
				slice->getWpScaling(picList, refIdx, wp);
				if (wp[0].bPresentFlag)
				{
					int iDeltaWeight = (wp[0].inputWeight - (1 << wp[0].log2WeightDenom));
//					WRITE_SVLC(iDeltaWeight, "delta_luma_weight_lX");          // se(v): delta_luma_weight_lX
					picList==0? slice_header_struct.delta_luma_weight_l0[refIdx] = iDeltaWeight : slice_header_struct.delta_luma_weight_l1[refIdx] = iDeltaWeight;
					
//					WRITE_SVLC(wp[0].inputOffset, "luma_offset_lX");               // se(v): luma_offset_lX
					picList==0? slice_header_struct.luma_offset_l0[refIdx] : slice_header_struct.luma_offset_l1[refIdx];
				
				}

				if (bChroma)
				{
					if (wp[1].bPresentFlag)
					{
						for (int j = 1; j < 3; j++)
						{
							int iDeltaWeight = (wp[j].inputWeight - (1 << wp[1].log2WeightDenom));
//							WRITE_SVLC(iDeltaWeight, "delta_chroma_weight_lX"); // se(v): delta_chroma_weight_lX
							if (j == 1)
							{
								picList==0 ? slice_header_struct.delta_Cb_weight_l0[refIdx] = iDeltaWeight : slice_header_struct.delta_Cb_weight_l1[refIdx] = iDeltaWeight;
							} 
							else
							{
								picList==0 ? slice_header_struct.delta_Cr_weight_l0[refIdx] = iDeltaWeight : slice_header_struct.delta_Cr_weight_l1[refIdx] = iDeltaWeight;
							}

							int pred = (128 - ((128 * wp[j].inputWeight) >> (wp[j].log2WeightDenom)));
							int iDeltaChroma = (wp[j].inputOffset - pred);
//							WRITE_SVLC(iDeltaChroma, "delta_chroma_offset_lX"); // se(v): delta_chroma_offset_lX
							if (j == 1)
							{
								picList==0 ? slice_header_struct.delta_Cb_offset_l0[refIdx] = iDeltaChroma : slice_header_struct.delta_Cb_offset_l1[refIdx] = iDeltaChroma;
							} 
							else
							{
								picList==0 ? slice_header_struct.delta_Cr_offset_l0[refIdx] = iDeltaChroma : slice_header_struct.delta_Cr_offset_l1[refIdx] = iDeltaChroma;
							}
						
						}
					}
				}
			}
		}

		assert(totalSignalledWeightFlags <= 24);
	}
}
bool CABAC_findMatchingLTRP(TComSlice* slice, uint32_t *ltrpsIndex, int ltrpPOC, bool usedFlag)
{
	// bool state = true, state2 = false;
	int lsb = ltrpPOC % (1 << slice->getSPS()->getBitsForPOC());

	for (int k = 0; k < (int)slice->getSPS()->getNumLongTermRefPicSPS(); k++)
	{
		if ((lsb == (int)slice->getSPS()->getLtRefPicPocLsbSps(k)) && (usedFlag == slice->getSPS()->getUsedByCurrPicLtSPSFlag(k)))
		{
			*ltrpsIndex = k;
			return true;
		}
	}

	return false;
}

//初始化上下文
void CABAC_RDO::resetEntropy_CABAC()
{
	SliceType sliceType  			= (SliceType)slice_header_struct.slice_type;
	int       qp         			= Clip3(0, 51,(int)(sps_pps_struct.init_qp + slice_header_struct.Slice_qp_delta));//m_Slice->SliceQpY;

	if (sps_pps_struct.cabac_init_present_flag && slice_header_struct.Cabac_init_flag)//P和B的初始化表格是否交换的标记
	{
		switch (sliceType)
		{
		case P_SLICE:           // change initialization table to B_SLICE initialization
			sliceType = B_SLICE;
			break;
		case B_SLICE:           // change initialization table to P_SLICE initialization
			sliceType = P_SLICE;
			break;
		default     :           // should not occur
			break;
			//assert(0);
		}
	}
	
	for (int x = 0; x < RDO_CU_MODE ; x++)
	{
		for (int y = 0; y < RDO_MAX_DEPTH ; y++)
		{
			for(int i = 0; i < CONTEXT_NUM;i++)
			{
				mModels[x][y][i].set_m_ucState(state_table[2 - sliceType][qp][i]);//m_ucState = state_table[2 - sliceType][qp][i];//初始化165中上下文
				mModels[x][y][i].setBinsCoded(0);//目的是什么？
			}
		}
	}
	

//	m_uiLastDQpNonZero  = 0;//什么的标记？

	// new structure
//	m_uiLastQp          = qp;//目的是什么？

//	m_pcTDecBinIf->start();//初始化    预先读入2个字节的bit
}
//初始化变量
void CABAC_RDO::init_data()
{
	resetEntropy_CABAC();
	init_L_buffer();

	memset(current_status , CTU_EST , sizeof(current_status));
	memset(next_status , CTU_EST , sizeof(next_status));

	x_ctu_left_up_in_pic = 0;
	y_ctu_left_up_in_pic = 0;

	memset(x_cu_left_up_in_pic , 0 , sizeof(x_cu_left_up_in_pic));
	memset(y_cu_left_up_in_pic , 0 , sizeof(y_cu_left_up_in_pic));

	memset(cu_in_pic_flag , 0 , sizeof(cu_in_pic_flag));

	memset(inx_in_cu , 0 , sizeof(inx_in_cu));
	memset(inx_in_tu , 0 , sizeof(inx_in_tu));
	 ctu_start_flag = 1;

	 inx_in_cu_transfer_table[0] = 64;inx_in_cu_transfer_table[1] = 16;inx_in_cu_transfer_table[2] = 4;inx_in_cu_transfer_table[3] = 1;inx_in_cu_transfer_table[4] = 1;
	 cu_pixel_wide[0] = 64;cu_pixel_wide[1] = 32;cu_pixel_wide[2] = 16;cu_pixel_wide[3] = 8;cu_pixel_wide[4] = 8;

	   memset(quant_finish_flag , 0 , sizeof(quant_finish_flag));
	   memset(cu_ready , 0 , sizeof(cu_ready));
	   memset(cu_cnt , 0 , sizeof(cu_cnt));

		pu_cnt_table[0] = 1;pu_cnt_table[1] = 2;pu_cnt_table[2] = 4;//0表示PU没有划分 1表示一个CU划分成两个PU 2表示1个CU划分成4个PU  TU都和PU一样尺寸
	   tu_cnt_table[0] = 1;tu_cnt_table[1] = 1;tu_cnt_table[2] = 1;//指的是TU在PU的基础上划分成多少个TU
	   memset(depth_to_cur_mode , 0 , sizeof(depth_to_cur_mode));depth_to_cur_mode[1][4]=2;
	   memset(pu_best_mode_flag , 0 , sizeof(pu_best_mode_flag));//某层PU最佳模式确定的标记
	   memset(intra_pu_depth_luma_bestDir , 0 , sizeof(intra_pu_depth_luma_bestDir));//intra某层PU最佳的预测方向值
	   memset(pu_mode_bit_ready , 0 , sizeof(pu_mode_bit_ready));//表示某层PU各种模式需要多少bit都已经准备好

	   end_depth[0] =3 ; end_depth[1] =4;
	   memset(max_pu_cnt , 0 , sizeof(max_pu_cnt));
	   memset(max_tu_cnt , 0 , sizeof(max_tu_cnt));
	   memset(pu_cnt , 0 , sizeof(pu_cnt));
	   memset(tu_cnt , 0 , sizeof(tu_cnt));

	   memset(est_bits_total , 0 , sizeof(est_bits_total));
	   memset(cu_est_bits , 0 , sizeof(cu_est_bits));
	   memset(pu_est_bits , 0 , sizeof(pu_est_bits));
	   memset(tu_est_bits , 0 , sizeof(tu_est_bits));

	   memset(intra_pu_depth_luma_bestDir_temp , 0 , sizeof(intra_pu_depth_luma_bestDir_temp));
	   memset(cu_best_mode , 0 , sizeof(cu_best_mode));
	   memset(cu_best_mode_flag , 0 , sizeof(cu_best_mode_flag));

	   memset(est_bit_cu_spit_flag , 0 , sizeof(est_bit_cu_spit_flag));
	   memset(mstate_cu_spit_flag , 0 , sizeof(mstate_cu_spit_flag));

	   memset(est_bit_cu_transquant_bypass_flag , 0 , sizeof(est_bit_cu_transquant_bypass_flag));
	   memset(mstate_cu_transquant_bypass_flag , 0 , sizeof(mstate_cu_transquant_bypass_flag));

	   memset(est_bit_cu_skip_flag , 0 , sizeof(est_bit_cu_skip_flag));
	   memset(mstate_cu_skip_flag , 0 , sizeof(mstate_cu_skip_flag));

	   memset(est_bit_pred_mode_flag , 0 , sizeof(est_bit_pred_mode_flag));
	   memset(mstate_pred_mode_flag , 0 , sizeof(mstate_pred_mode_flag));

	   memset(est_bit_pu_luma_dir , 0 , sizeof(est_bit_pu_luma_dir));
	   memset(mstate_prev_intra_luma_pred_flag , 0 , sizeof(mstate_prev_intra_luma_pred_flag));

	   memset(est_bit_pu_chroma_dir , 0 , sizeof(est_bit_pu_chroma_dir));
	   memset(mstate_pu_chroma_dir , 0 , sizeof(mstate_pu_chroma_dir));

	   memset(est_bit_pu_part_mode , 0 , sizeof(est_bit_pu_part_mode));
	   memset(est_bit_pu_part_mode_sure , 0 , sizeof(est_bit_pu_part_mode_sure));
	   memset(mstate_pu_part_mode , 0 , sizeof(mstate_pu_part_mode));

	   memset(est_bit_merge_flag , 0 , sizeof(est_bit_merge_flag));
	   memset(mstate_merge_flag , 0 , sizeof(mstate_merge_flag));

	   memset(est_bit_merge_idx , 0 , sizeof(est_bit_merge_idx));
	   memset(mstate_merge_idx , 0 , sizeof(mstate_merge_idx));

	   memset(est_bit_mvp_l0_flag , 0 , sizeof(est_bit_mvp_l0_flag));
	   memset(mstate_mvp_l0_flag , 0 , sizeof(mstate_mvp_l0_flag));

	   memset(est_bit_abs_mvd_greater0_flag , 0 , sizeof(est_bit_abs_mvd_greater0_flag));
	   memset(mstate_abs_mvd_greater0_flag , 0 , sizeof(mstate_abs_mvd_greater0_flag));

	   memset(est_bit_abs_mvd_greater1_flag , 0 , sizeof(est_bit_abs_mvd_greater1_flag));
	   memset(mstate_abs_mvd_greater1_flag , 0 , sizeof(mstate_abs_mvd_greater1_flag));

	   memset(est_bit_rqt_root_cbf , 0 , sizeof(est_bit_rqt_root_cbf));
	   memset(mstate_rqt_root_cbf , 0 , sizeof(mstate_rqt_root_cbf));

	   memset(est_bit_cbf_luma , 0 , sizeof(est_bit_cbf_luma));
	   memset(mstate_cbf_luma , 0 , sizeof(mstate_cbf_luma));

	   memset(est_bit_sao_merge_left_flag , 0 , sizeof(est_bit_sao_merge_left_flag));
	   memset(est_bit_sao_merge_up_flag , 0 , sizeof(est_bit_sao_merge_up_flag));
	   memset(est_bit_sao_type_idx_luma , 0 , sizeof(est_bit_sao_type_idx_luma));
	   memset(est_bit_sao_type_idx_chroma , 0 , sizeof(est_bit_sao_type_idx_chroma));

	   memset(est_bit_tu , 0 , sizeof(est_bit_tu));
}

void CABAC_RDO::est_bits_init()
{
	memset(est_bit_last_sig_coeff_x_prefix, 0 , sizeof(est_bit_last_sig_coeff_x_prefix));
	memset(est_bit_last_sig_coeff_x_suffix, 0 , sizeof(est_bit_last_sig_coeff_x_suffix));
	memset(est_bit_last_sig_coeff_y_prefix, 0 , sizeof(est_bit_last_sig_coeff_y_prefix));
	memset(est_bit_last_sig_coeff_y_suffix, 0 , sizeof(est_bit_last_sig_coeff_y_suffix));
	memset(est_bit_sig_coeff_flag, 0 , sizeof(est_bit_sig_coeff_flag));
	memset(est_bit_coeff_sign_flag, 0 , sizeof(est_bit_coeff_sign_flag));
	memset(est_bit_coeff_abs_level_greater1_flag, 0 , sizeof(est_bit_coeff_abs_level_greater1_flag));
	memset(est_bit_coeff_abs_level_greater2_flag, 0 , sizeof(est_bit_coeff_abs_level_greater2_flag));
	memset(est_bit_coeff_abs_level_remaining, 0 , sizeof(est_bit_coeff_abs_level_remaining));
	memset(est_bit_coded_sub_block_flag, 0 , sizeof(est_bit_coded_sub_block_flag));

	if (depth_tu != 4)
	{
		memset(tu_est_bits, 0 , sizeof(tu_est_bits));
		memset(pu_est_bits, 0 , sizeof(pu_est_bits));
	}
	
}

//CABAC_RDO
void CABAC_RDO::init_L_buffer()
{
	memset(&avail_left_up,0,sizeof(avail_left_up));
	memset(&intra_luma_pred_mode_left_up,1,sizeof(intra_luma_pred_mode_left_up));
	memset(&depth_left_up,0,sizeof(depth_left_up));
	memset(&skip_flag_left_up,0,sizeof(skip_flag_left_up));
	memset(&pred_mode_left_up,0,sizeof(pred_mode_left_up));

}

void CABAC_RDO::update_L_buffer_x()
{
	for (int i = 0 ; i < RDO_CU_MODE ; i++)
	{
		for (int j = 0 ; j < (RDO_MAX_DEPTH) ; j++ )
		{
//			memset(&avail_left_up[i][j][16],0,sizeof(avail_left_up[i][j])/2);
			memset(&intra_luma_pred_mode_left_up[i][j][16],1,sizeof(intra_luma_pred_mode_left_up[i][j])/2);
//			memset(&depth_left_up[i][j][8],0,sizeof(depth_left_up[i][j])/2);
//			memset(&skip_flag_left_up[i][j][8],0,sizeof(skip_flag_left_up[i][j])/2);
			memset(&pred_mode_left_up[i][j][8],0,sizeof(pred_mode_left_up[i][j])/2);
		}
	}
	x_ctu_left_up_in_pic += 64;
}

void CABAC_RDO::update_L_buffer_y()
{
	for (int i = 0 ; i < RDO_CU_MODE ; i++)
	{
		for (int j = 0 ; j < (RDO_MAX_DEPTH) ; j++ )
		{
			memset(&avail_left_up[i][j][0],0 , 16);
			memset(&intra_luma_pred_mode_left_up[i][j][0],1, 16);
			memset(&depth_left_up[i][j][0],0, 8);
			memset(&skip_flag_left_up[i][j][0],0, 8);
			memset(&pred_mode_left_up[i][j][0],0, 8);
		}
	}
	x_ctu_left_up_in_pic = 0;
	y_ctu_left_up_in_pic += 64;
}

uint32_t CABAC_RDO::cu_to_tu_idx( uint32_t uiAbsPartIdx)//把最小CU的扫描序号转换为最小TU的扫描序号
{
	int	depth = sps_pps_struct.log2_min_coding_block_depth_bt_cu_tu;//最小CU和最小TU之间的深度差
	return uiAbsPartIdx << (depth * 2);
}
uint32_t CABAC_RDO::tu_to_cu_idx( uint32_t uiAbsPartIdx)
{
	int depth = sps_pps_struct.log2_min_coding_block_depth_bt_cu_tu;
	return uiAbsPartIdx >> (depth * 2);
}

template<typename T>
T CABAC_RDO::getLeftpart_in_tu(T* puhBaseLCU, UInt uiAbsPartIdx )
{
  int log2_max_coding_block_depth_bt_cu_tu = sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu;

  /*
  	input :
  	tu_ZscanToRaster							tu级的scan到raster序的表
  	log2_max_coding_block_depth_bt_cu_tu  		max depth between ctu and tu
  	uiAbsPartIdx								当前tu的index


  	输出:
  	y_pos				当前tu在vertical方向的offset
   */
  int	y_pos = ((Int)tu_ZscanToRaster[uiAbsPartIdx])  >> log2_max_coding_block_depth_bt_cu_tu;

    /*
  	input :
  	y_pos				当前tu在vertical方向的offset
  	puhBaseLCU			当前变量buffer的首地址


  	输出:
  	puhBaseLCU[y_pos] 		需要获取的当前变量左方变量的值
	*/
  return puhBaseLCU[y_pos];
	
}

template<typename T>
T CABAC_RDO::getUppart_in_tu(T* puhBaseLCU, UInt uiAbsPartIdx )
{
  int tu_in_ctu_per_line = sps_pps_struct.tu_in_ctu_per_line;
  int log2_max_coding_block_depth_bt_cu_tu = sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu;
  int    x_pos;
  assert( (tu_ZscanToRaster[uiAbsPartIdx]) % tu_in_ctu_per_line == 
  	((tu_ZscanToRaster[uiAbsPartIdx]) &  ((1 << sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu ) - 1)));
 	/*
  	input :
  	tu_ZscanToRaster						tu级的scan到raster序的表
  	log2_max_coding_block_depth_bt_cu_tu  	max depth between cut and tu
  	uiAbsPartIdx							当前tu的index
  	输出:
  	x_pos				当前cu在水平方向的offset
	*/
	{
		x_pos = (tu_ZscanToRaster[uiAbsPartIdx] ) &  ((1 << log2_max_coding_block_depth_bt_cu_tu) - 1);
  	}
	/*
  	input :
  	x_pos 								当前的cu在水平方向上的偏移
  	log2_max_coding_block_depth_bt_cu_tu  	max depth between cut and tu
  	puhBaseLCU							当前变量buffer的首地址
  	输出:
  	puhBaseLCU[(1 << log2_max_coding_block_depth_bt_cu_tu) + x_pos] 		需要获取的当前变量上方变量的值
	*/
  return puhBaseLCU[(1 << log2_max_coding_block_depth_bt_cu_tu) + x_pos +(L_buff_ctu_x<<4)];
}

char CABAC_RDO::getLeftAvailSubParts( UInt uiAbsPartIdx, UInt depth , int IsIntra  )
{
	//assert(getLeftpart_in_tu<char>(slice_avail_left_up, uiAbsPartIdx ) == getLeftpart_in_map_tu<char>(slice_avail_map,uiAbsPartIdx));

	return	getLeftpart_in_tu<char>(avail_left_up[IsIntra][depth], uiAbsPartIdx );
	//return	getLeftpart_in_map_tu<char>(slice_avail_map,uiAbsPartIdx);
}

char CABAC_RDO::getUpAvailSubParts( UInt uiAbsPartIdx , UInt depth , int IsIntra )
{
	L_buff_ctu_x = ctu_x;
	//assert(getUppart_in_tu<char>(slice_avail_left_up, uiAbsPartIdx ) == getUppart_in_map_tu<char>(slice_avail_map,uiAbsPartIdx));
	return	getUppart_in_tu<char>(avail_left_up[IsIntra][depth], uiAbsPartIdx );
	//return	getUppart_in_map_tu<char>(slice_avail_map,uiAbsPartIdx);
}
template<typename T>
T CABAC_RDO::getUppart_in_cu(  T* puhBaseLCU, UInt uiAbsPartIdx )
{
  int log2_max_coding_depth_for_dec = sps_pps_struct.log2_max_coding_depth_for_dec;
  int    x_pos;

 	/*
  	input :
  	cu_ZscanToRaster					cu级的scan到raster序的表
  	log2_maxa_coding_block_depth  	
  	uiAbsPartIdx						当前cu的index


  	输出:
  	x_pos				当前cu在水平方向的offset
	*/
	{
		x_pos = (cu_ZscanToRaster[uiAbsPartIdx] ) &  ((1 << log2_max_coding_depth_for_dec ) - 1);
  	}
	/*
  	input :
  	x_pos 						当前的cu在水平方向上的偏移
  	log2_maxa_coding_block_depth  	max cu depth
  	puhBaseLCU					当前变量buffer的首地址


  	输出:
  	puhBaseLCU[(1 << log2_maxa_coding_block_depth ) + x_pos] 		需要获取的当前变量上方变量的值
	*/
	
  	return puhBaseLCU[(1 << log2_max_coding_depth_for_dec )+ x_pos+(L_buff_ctu_x<<3)];
}
template<typename T>
T CABAC_RDO::getLeftpart_in_cu( T* puhBaseLCU, UInt uiAbsPartIdx )
{
  int log2_max_coding_depth_for_dec = sps_pps_struct.log2_max_coding_depth_for_dec;
  int	y_pos ;
  /*
  	input :
  	cu_ZscanToRaster					cu级的scan到raster序的表
  	log2_maxa_coding_block_depth  		max cu depth
  	uiAbsPartIdx						当前cu的index


  	输出:
  	y_pos				当前cu在vertical方向的offset
   */
  
  y_pos	= (Int)(cu_ZscanToRaster[uiAbsPartIdx]) >> log2_max_coding_depth_for_dec ;
  
  
  /*
  	input :
  	y_pos				当前cu在vertical方向的offset
  	puhBaseLCU		当前变量buffer的首地址


  	输出:
  	puhBaseLCU[y_pos] 		需要获取的当前变量左方变量的值
	*/
  return puhBaseLCU[ y_pos];
}

char CABAC_RDO::getLeftDepthSubParts( UInt uiAbsPartIdx , UInt depth , int IsIntra )
{
	//assert(getLeftpart_in_cu<UChar>(depth_left_up[IsIntra][depth], uiAbsPartIdx ) == getLeftpart_in_map_cu<UChar>(depth_map,uiAbsPartIdx));
	char temp;/*temp1*/

	temp = getLeftpart_in_cu<UChar>(depth_left_up[IsIntra][depth], uiAbsPartIdx );
//	temp1 = (get_global_left_6bit_func(uiAbsPartIdx) ) & 3;

//	UInt Idx_in_tu = cu_to_tu_idx(uiAbsPartIdx);
// 	if(getLeftAvailSubParts(Idx_in_tu))
// 		assert(temp==temp1);
	return getLeftpart_in_cu<UChar>(depth_left_up[IsIntra][depth], uiAbsPartIdx );

	//	return  getLeftpart_in_map_cu<UChar>(depth_map,uiAbsPartIdx);

}

char CABAC_RDO::getUpDepthSubParts( UInt uiAbsPartIdx , UInt depth , int IsIntra )
{
	//assert(getUppart_in_cu<UChar>(depth_left_up[IsIntra][depth], uiAbsPartIdx ) == getUppart_in_map_cu<UChar>(depth_map,uiAbsPartIdx));
//	UInt Idx_in_tu = cu_to_tu_idx(uiAbsPartIdx);
	char temp;/*temp1*/

	temp = getUppart_in_cu<UChar>(depth_left_up[IsIntra][depth], uiAbsPartIdx );
//	temp1 = (get_global_up_6bit_func(uiAbsPartIdx) ) & 3;
//	if(getUpAvailSubParts(Idx_in_tu))
//		assert(temp == temp1);
	L_buff_ctu_x = ctu_x;
	return getUppart_in_cu<UChar>(depth_left_up[IsIntra][depth], uiAbsPartIdx );


	//	return	getUppart_in_map_cu<UChar>(depth_map,uiAbsPartIdx);
}

UInt CABAC_RDO::getCtxSplitFlag( UInt uiAbsPartIdx, UInt depth , int IsIntra)
{
//	CABAC_RDO* pcTempCU;
//	UInt        uiTempPartIdx;
	UInt        uiCtx;
	// Get left split flag
	UInt Idx_in_tu = cu_to_tu_idx(uiAbsPartIdx);

//	assert((getLeftSliceAvailSubParts(Idx_in_tu) && getLeftTileAvailSubParts(Idx_in_tu) )== getLeftAvailSubParts(Idx_in_tu));
	uiCtx  = getLeftAvailSubParts(Idx_in_tu,depth , IsIntra)//(getLeftSliceAvailSubParts(Idx_in_tu) && getLeftTileAvailSubParts(Idx_in_tu))
		? (( getLeftDepthSubParts(uiAbsPartIdx,depth , IsIntra) > (UChar)depth)? 1 : 0 ) : 0;

	// Get above split flag

//	assert((getUpSliceAvailSubParts(Idx_in_tu) && getUpTileAvailSubParts(Idx_in_tu)) == getUpAvailSubParts(Idx_in_tu));
	uiCtx += getUpAvailSubParts(Idx_in_tu,depth , IsIntra)//(getUpSliceAvailSubParts(Idx_in_tu) && getUpTileAvailSubParts(Idx_in_tu))
		? (( getUpDepthSubParts(uiAbsPartIdx,depth , IsIntra) > (UChar)depth)? 1 : 0 ) : 0;

	return uiCtx;
}

UInt CABAC_RDO::getCtxSkipFlag( UInt uiAbsPartIdx , UInt depth , int IsIntra )
{
//	CABAC_RDO* pcTempCU;
//	UInt        uiTempPartIdx;
	UInt        uiCtx = 0;

	UInt Idx_in_tu = cu_to_tu_idx(uiAbsPartIdx);
	// Get BCBP of left PU
//	assert((getLeftSliceAvailSubParts(Idx_in_tu) && getLeftTileAvailSubParts(Idx_in_tu)) == getLeftAvailSubParts(Idx_in_tu));
	uiCtx  = getLeftAvailSubParts(Idx_in_tu,depth,IsIntra)//(getLeftSliceAvailSubParts(Idx_in_tu) && getLeftTileAvailSubParts(Idx_in_tu))  //?为什么要使用32大小的数组
		? (getLeftSkip_flagSubParts(uiAbsPartIdx,depth,IsIntra)? 1 : 0 ) : 0;


	// Get BCBP of above PU

//	assert((getUpSliceAvailSubParts(Idx_in_tu) && getUpTileAvailSubParts(Idx_in_tu)) == getUpAvailSubParts(Idx_in_tu));
	uiCtx  += getUpAvailSubParts(Idx_in_tu,depth,IsIntra)//(getUpSliceAvailSubParts(Idx_in_tu) && getUpTileAvailSubParts(Idx_in_tu))
		? (getUpSkip_flagSubParts(uiAbsPartIdx,depth,IsIntra)? 1 : 0 ) : 0;


	return uiCtx;
}

char CABAC_RDO::getLeftSkip_flagSubParts( UInt uiAbsPartIdx  , UInt depth , int IsIntra)
{
	//	assert(getLeftpart_in_cu<Char>(skip_flag_left_up[IsIntra][depth], uiAbsPartIdx ) == getLeftpart_in_map_cu<Char>(skip_flag_map,uiAbsPartIdx));
// 	UInt Idx_in_tu = cu_to_tu_idx(uiAbsPartIdx);
// 	if(getLeftAvailSubParts(Idx_in_tu))
// 		assert(getLeftpart_in_cu<Char>(skip_flag_left_up[IsIntra][depth], uiAbsPartIdx )  == 
// 		((get_global_left_6bit_func(uiAbsPartIdx) >> 2) & 1));
	return getLeftpart_in_cu<char>(skip_flag_left_up[IsIntra][depth], uiAbsPartIdx );

	//	return getLeftpart_in_cu<Char>(skip_flag_left_up[IsIntra][depth], uiAbsPartIdx );
	//return	getLeftpart_in_map_cu<Char>(skip_flag_map,uiAbsPartIdx);
}

char CABAC_RDO::getUpSkip_flagSubParts( UInt uiAbsPartIdx  , UInt depth , int IsIntra)
{
	//	assert(getUppart_in_cu<Char>(skip_flag_left_up[IsIntra][depth], uiAbsPartIdx ) == getUppart_in_map_cu<Char>(skip_flag_map,uiAbsPartIdx));
//	UInt Idx_in_tu = cu_to_tu_idx(uiAbsPartIdx);
// 	if(getUpAvailSubParts(Idx_in_tu))
// 		assert(getUppart_in_cu<Char>(skip_flag_left_up[IsIntra][depth], uiAbsPartIdx )  == 
// 		((get_global_up_6bit_func(uiAbsPartIdx) >> 2) & 1));
	L_buff_ctu_x = ctu_x;
	return getUppart_in_cu<char>(skip_flag_left_up[IsIntra][depth], uiAbsPartIdx );

	//return	getUppart_in_cu<Char>(skip_flag_left_up[IsIntra][depth], uiAbsPartIdx ) ;
	//	return	getUppart_in_map_cu<Char>(skip_flag_map,uiAbsPartIdx);

}

Int CABAC_RDO::getIntraDirLumaPredictor( UInt uiAbsPartIdx, Int* uiIntraDirPred,int partIdx   , UInt depth , int IsIntra)
{
	Int         iLeftIntraDir, iAboveIntraDir;
	Int         uiPredNum = 0;

	// Get intra direction of left PU
	Int		  inx_in_cu_temp = tu_to_cu_idx(uiAbsPartIdx);
	Int		  left_intra_mode; 
	Int		  up_intra_mode; 
	Int		  left_avail; 
	Int		  up_avail; 
	Int		  left_pred_mode; 
	Int		  up_pred_mode; 

	if(partIdx % 2 == 0)
	{
		left_intra_mode = getLeftIntra_pred_modeSubParts(uiAbsPartIdx,depth,IsIntra);
//		assert((getLeftSliceAvailSubParts(uiAbsPartIdx) && getLeftTileAvailSubParts(uiAbsPartIdx)) == getLeftAvailSubParts(uiAbsPartIdx));
		left_avail 		= getLeftAvailSubParts(uiAbsPartIdx,depth,IsIntra);//getLeftSliceAvailSubParts(uiAbsPartIdx) && getLeftTileAvailSubParts(uiAbsPartIdx);
		left_pred_mode  = getLeftPred_modeSubParts(inx_in_cu_temp,depth,IsIntra);
		//if(left_avail && left_pred_mode 	== MODE_INTRA)
		//	assert(getLeftIntra_pred_modeSubParts(uiAbsPartIdx) == getLeftIntra_pred_modeSubParts1(uiAbsPartIdx));
	}
	else
	{  	
		left_intra_mode = getLeftIntra_pred_modeSubParts(uiAbsPartIdx,depth,IsIntra);
		left_avail 		= true;
		left_pred_mode  = MODE_INTRA;
	}

	if(tu_ZscanToRaster[uiAbsPartIdx] / (1 << sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu) == 0)
	{
		up_intra_mode 	= getUpIntra_pred_modeSubParts(uiAbsPartIdx,depth,IsIntra);
		up_avail 		= 0;
		up_pred_mode  	= getUpPred_modeSubParts(inx_in_cu_temp,depth,IsIntra);
		//if(up_avail && up_pred_mode 	== MODE_INTRA)
		//	assert(getUpIntra_pred_modeSubParts(uiAbsPartIdx) == getUpIntra_pred_modeSubParts1(uiAbsPartIdx));
	}
	else	if(partIdx / 2 == 0)
	{
		up_intra_mode 	= getUpIntra_pred_modeSubParts(uiAbsPartIdx,depth,IsIntra);
//		assert((getUpSliceAvailSubParts(uiAbsPartIdx) && getUpTileAvailSubParts(uiAbsPartIdx)) == getUpAvailSubParts(uiAbsPartIdx));
		up_avail 		= getUpAvailSubParts(uiAbsPartIdx,depth,IsIntra);//getUpSliceAvailSubParts(uiAbsPartIdx) && getUpTileAvailSubParts(uiAbsPartIdx);
		up_pred_mode  = getUpPred_modeSubParts(inx_in_cu_temp,depth,IsIntra);
		//if(up_avail && up_pred_mode 	== MODE_INTRA)
		//	assert(getUpIntra_pred_modeSubParts(uiAbsPartIdx) == getUpIntra_pred_modeSubParts1(uiAbsPartIdx));
	}
	else
	{
		up_intra_mode 	= getUpIntra_pred_modeSubParts(uiAbsPartIdx,depth,IsIntra);
		up_avail 		= true;
		up_pred_mode  = MODE_INTRA;
	}


	iLeftIntraDir  = left_avail	? ( left_pred_mode 	== MODE_INTRA 	? left_intra_mode : DC_IDX ) : DC_IDX;

	// Get intra direction of above PU
	iAboveIntraDir  = up_avail 	? ( up_pred_mode 	== MODE_INTRA	? up_intra_mode : DC_IDX ) : DC_IDX;
	
	uiPredNum = 3;
	if(iLeftIntraDir == iAboveIntraDir)
	{
		if (iLeftIntraDir > 1) // angular modes
		{
			uiIntraDirPred[0] = iLeftIntraDir;
			uiIntraDirPred[1] = ((iLeftIntraDir + 29) % 32) + 2;
			uiIntraDirPred[2] = ((iLeftIntraDir - 1 ) % 32) + 2;
		}
		else //non-angular
		{
			uiIntraDirPred[0] = PLANAR_IDX;
			uiIntraDirPred[1] = DC_IDX;
			uiIntraDirPred[2] = VER_IDX; 
		}
	}
	else
	{
		uiIntraDirPred[0] = iLeftIntraDir;
		uiIntraDirPred[1] = iAboveIntraDir;

		if (iLeftIntraDir && iAboveIntraDir ) //both modes are non-planar
		{
			uiIntraDirPred[2] = PLANAR_IDX;
		}
		else
		{
			uiIntraDirPred[2] =  (iLeftIntraDir+iAboveIntraDir)<2? VER_IDX : DC_IDX;
		}
	}

	return uiPredNum;
}

unsigned char CABAC_RDO::getLeftIntra_pred_modeSubParts( UInt uiAbsPartIdx   , UInt depth , int IsIntra)
{


	return	getLeftpart_in_tu<UChar>(intra_luma_pred_mode_left_up[IsIntra][depth], uiAbsPartIdx ) ;
	//return	getLeftpart_in_map_tu<UChar>(intra_luma_pred_mode_map,uiAbsPartIdx);
}

unsigned char CABAC_RDO::getUpIntra_pred_modeSubParts( UInt uiAbsPartIdx  , UInt depth , int IsIntra )
{

	L_buff_ctu_x = 0;
	return	getUppart_in_tu<UChar>(intra_luma_pred_mode_left_up[IsIntra][depth], uiAbsPartIdx ) ;
	//return	getUppart_in_map_tu<UChar>(intra_luma_pred_mode_map,uiAbsPartIdx);
}

char CABAC_RDO::getLeftPred_modeSubParts( UInt uiAbsPartIdx   , UInt depth , int IsIntra)
{
	//assert(getLeftpart_in_cu<Char>(pred_mode_left_up[IsIntra][depth], uiAbsPartIdx ) == getLeftpart_in_map_cu<Char>(pred_mode_map,uiAbsPartIdx));
	char temp;/*temp1*/
	temp = getLeftpart_in_cu<char>(pred_mode_left_up[IsIntra][depth], uiAbsPartIdx);
//	temp1 = (get_global_left_6bit_func(uiAbsPartIdx) >> 3);
//	UInt Idx_in_tu = cu_to_tu_idx(uiAbsPartIdx);
// 	if(getLeftAvailSubParts(Idx_in_tu))
// 		assert(temp == temp1);
	return getLeftpart_in_cu<char>(pred_mode_left_up[IsIntra][depth], uiAbsPartIdx );

	//return	getLeftpart_in_cu<Char>(pred_mode_left_up[IsIntra][depth], uiAbsPartIdx ) ;

	//return	getLeftpart_in_map_cu<Char>(pred_mode_map,uiAbsPartIdx);
}

char CABAC_RDO::getUpPred_modeSubParts( UInt uiAbsPartIdx   , UInt depth , int IsIntra)
{
	//assert(getUppart_in_cu<Char>(pred_mode_left_up[IsIntra][depth], uiAbsPartIdx ) == getUppart_in_map_cu<Char>(pred_mode_map,uiAbsPartIdx));

	char temp;/*temp1*/
	temp = getUppart_in_cu<char>(pred_mode_left_up[IsIntra][depth], uiAbsPartIdx);
//	temp1 = (get_global_up_6bit_func(uiAbsPartIdx) >> 3);
// 	UInt Idx_in_tu = cu_to_tu_idx(uiAbsPartIdx);
// 	if(getUpAvailSubParts(Idx_in_tu))
// 		assert(temp == temp1);
	//assert(getUppart_in_cu<Char>(pred_mode_left_up[IsIntra][depth], uiAbsPartIdx)  == 
	//		(get_global_up_6bit_func(uiAbsPartIdx) >> 3));
	L_buff_ctu_x = 0;
	return getUppart_in_cu<char>(pred_mode_left_up[IsIntra][depth], uiAbsPartIdx );
	//	return	getUppart_in_cu<Char>(pred_mode_left_up[IsIntra][depth], uiAbsPartIdx ) ;
	//	return	getUppart_in_map_cu<Char>(pred_mode_map,uiAbsPartIdx);
}

template<typename T>
void CABAC_RDO::setpart_in_tu( T uiParameter, T* puhBaseLCU, UInt uiAbsPartIdx, UInt depth )
{	
	int cur_part_num = sps_pps_struct.Max_tu_in_ctu >> (depth * 2);//当前深度有多少个最小TU
//	int cur_line_num = sps_pps_struct.log2_max_coding_block_depth_bt_cu_tu - depth;//TU的最大深度减去当前CU的深度
	int cur_part_line = sps_pps_struct.tu_in_ctu_per_line;//CTU一行有多少个最小TU

//	int	blk_x_pos =(Int) tu_ZscanToRaster[uiAbsPartIdx] % cur_part_line  ;//最小TU在X方向的偏移
//	int	blk_y_pos = (Int)tu_ZscanToRaster[uiAbsPartIdx] / cur_part_line  ;//最小TU在Y方向的偏移

	for(int i= 0;i < cur_part_num; i++,uiAbsPartIdx++)//用来按硬件方式更新以TU为单位的左边和上边是否有CU块的标记
	{
		int	x_pos =(Int) tu_ZscanToRaster[uiAbsPartIdx] % cur_part_line  ;
		int	y_pos = (Int)tu_ZscanToRaster[uiAbsPartIdx] / cur_part_line  ;
		puhBaseLCU[ cur_part_line + (x_pos ) +(L_buff_ctu_x<<4)] 	= uiParameter;
		puhBaseLCU[ y_pos ] 						= uiParameter;
	}
}


template<typename T>
void CABAC_RDO::setpart_in_cu( T uiParameter, T* puhBaseLCU, UInt uiAbsPartIdx, UInt depth )//uiAbsPartIdx是按最小CU Z扫描来排序的
{
	if (depth == 4)
	{
		depth = 3;
	}
  int cur_part_num = sps_pps_struct.Max_cu_in_ctu >> (depth * 2);//根据深度算出当前CU有多少个最小CU
//  int cur_line_num = sps_pps_struct.log2_max_coding_depth_for_dec - depth;//当前CU还可以往下划分几层
  int cur_part_line = sps_pps_struct.cu_in_ctu_per_line;//一个CTU有多少个最小CU  一般为8个

  int	blk_x_pos =(Int) cu_ZscanToRaster[uiAbsPartIdx] % cur_part_line  ;	//当前CU按最小CU在X方向的偏移
  int	blk_y_pos = (Int)cu_ZscanToRaster[uiAbsPartIdx] / cur_part_line  ;//当前CU按最小CU在Y方向的偏移

  {
  	/*
  	input :
  	uiAbsPartIdx 	当前cu的index
  	cur_part_line	1 << log2_max_coding_block_depth
  	cu_ZscanToRaster		zscan到raster序的转换表
  	

	output
	blk_x_pos	当前的cu在水平方向上的起始偏移
  	blk_y_pos	当前的cu在垂直方向上的起始偏移
  	
	*/
  		blk_x_pos =(Int) cu_ZscanToRaster[uiAbsPartIdx] % cur_part_line  ;
  		blk_y_pos = (Int)cu_ZscanToRaster[uiAbsPartIdx] / cur_part_line  ;
  }

  for(int i= 0;i < cur_part_num; i++,uiAbsPartIdx++)//用来按硬件方式保存左边和上边CU深度的过程
  {
	  int	x_pos = (Int)cu_ZscanToRaster[uiAbsPartIdx] % cur_part_line  ;
	  int	y_pos = (Int)cu_ZscanToRaster[uiAbsPartIdx] / cur_part_line  ;
	  puhBaseLCU[ cur_part_line + (x_pos ) + (L_buff_ctu_x<<3)] = uiParameter;
	  puhBaseLCU[ y_pos ] = uiParameter;

  }
}

void CABAC_RDO::setLumaIntraDirSubParts( UInt uiDir, UInt uiAbsPartIdx, UInt depth , int IsIntra)//tu level
{
	L_buff_ctu_x = 0;
	setpart_in_tu<UChar>(uiDir,intra_luma_pred_mode_left_up[IsIntra][depth], uiAbsPartIdx, depth);
}


void CABAC_RDO::setPredModeSubParts( PredMode eMode, UInt uiAbsPartIdx, UInt depth , int IsIntra )//cu level   //原版本注释为tu level
{
	L_buff_ctu_x = 0;
	setpart_in_cu<char>(eMode,pred_mode_left_up[IsIntra][depth], uiAbsPartIdx, depth);
}

void CABAC_RDO::setSkipFlagSubParts( bool skip, UInt absPartIdx, UInt depth  , int IsIntra)//cu level
{
	L_buff_ctu_x = ctu_x;
	setpart_in_cu<char>(skip,skip_flag_left_up[IsIntra][depth], absPartIdx, depth);
}

void CABAC_RDO::setDepthSubParts( UInt depth, UInt uiAbsPartIdx , int IsIntra )//cu level
{
	L_buff_ctu_x = ctu_x;
	setpart_in_cu<UChar>(depth,depth_left_up[IsIntra][depth], uiAbsPartIdx, depth);//用来按硬件方式保存左边和上边CU深度的过程  保存结果在depth_left_up[IsIntra][depth]的空间大小16的数组中
}

void CABAC_RDO::setAvailSubParts_map( UInt availC, UInt uiAbsPartIdx,UInt depth   , int IsIntra)//tu level 原本注释 cu level
{
	L_buff_ctu_x = ctu_x;
	setpart_in_tu<char>(availC,avail_left_up[IsIntra][depth], uiAbsPartIdx, depth);
}

//CABAC TU 级别函数
#pragma  region
int tu_scan_table[3][4][64] =  //扫描序号转光栅
{

	{
		{
			0
		},
		{
			0,
				2,	1,
				3
			},
			{
				0,
					4,	1,
					8,	5,	2,
					12, 9,	6,	3,
					13, 10, 7,
					14, 11,
					15
			},


			{
				0,
					8, 1,
					16, 9, 2,
					24, 17, 10, 3,
					32, 25, 18, 11, 4,
					40, 33, 26, 19, 12, 5,
					48, 41, 34, 27, 20, 13, 6,
					56, 49, 42, 35, 28, 21, 14, 7,
					57, 50, 43, 36, 29, 22, 15,
					58, 51, 44, 37, 30, 23,
					59, 52, 45, 38, 31,
					60, 53, 46, 39,
					61, 54, 47,
					62, 55,
					63
				}

	},
	{


		{
			0
		},
		{
			0,	1,
				2,	3
			},
			{
				0,	1,	2,	3,
					4,	5,	6,	7,
					8,	9,	10, 11,
					12, 13, 14, 15
			},

			{
				0, 1, 2, 3, 4, 5, 6, 7,
					8, 9, 10,11,12,13,14,15,
					16,17,18,19,20,21,22,23,
					24,25,26,27,28,29,30,31,
					32,33,34,35,36,37,38,39,
					40,41,42,43,44,45,46,47,
					48,49,50,51,52,53,54,55,
					56,57,58,59,60,61,62,63
				}

	},
	{
		{
			0
		},
		{
			0,	2,
				1,	3
			},
			{
				0,	4,	8,	12,
					1,	5,	9,	13,
					2,	6,	10, 14,
					3,	7,	11, 15
			},

			{
				0,	8,	16, 24, 32, 40, 48, 56,
					1,	9,	17, 25, 33, 41, 49, 57,
					2,	10, 18, 26, 34, 42, 50, 58,
					3,	11, 19, 27, 35, 43, 51, 59,
					4,	12, 20, 28, 36, 44, 52, 60,
					5,	13, 21, 29, 37, 45, 53, 61,
					6,	14, 22, 30, 38, 46, 54, 62,
					7,	15, 23, 31, 39, 47, 55, 63
				}
	}
};

UInt	scan_coeff_block[3][4][64] =  //光栅到扫描的转换表格
{
	{
		{
			0
		},
		{
			0,2,1,3
			},
			{
				0,	2,	5,	9,	1,	4,	8,	12,	3,	7,	11,	14,	6,	10,	13,	15
			},
			{
				0,	2,	5,	9,	14,	20,	27,	35,
					1,	4,	8,	13,	19,	26,	34,	42,
					3, 	7,	12,	18,	25,	33,	41,	48,
					6,	11,	17,	24,	32,	40,	47,	53,
					10,	16,	23,	31,	39,	46,	52,	57,
					15,	22,	30,	38,	45,	51,	56,	60,
					21,	29,	37,	44,	50,	55,	59,	62,
					28,	36,	43,	49,	54,	58,	61,	63,
				}
	},

	{
		{
			0
		},
		{
			0,	1,	2,	3
			},
			{
				0,	1,	2,	3,
					4,	5,	6,	7,
					8,	9,	10, 11,
					12, 13, 14, 15
			},

			{
				0,	1,	2,	3,	4,	5,	6,	7,
					8,	9,	10, 11,	12, 13, 14, 15,

					16,	17,	18,	19,	20,	21,	22,	23,
					24,	25,	26,	27,	28,	29,	30,	31,

					32,	33,	34,	35,	36,	37,	38,	39,
					40,	41,	42,	43,	44,	45,	46,	47,

					48,	49,	50,	51,	52,	53,	54,	55,
					56,	57,	58,	59,	60,	61,	62,	63
				}
	},
	{
		{
			0
		},
		{
			0,	2,	1,	3
			},
			{
				0,	4,	8,	12,	1,	5,	9,	13,	2,	6,	10,	14,	3,	7,	11,	15
			},
			{
				0,	8,	16,	24,	32,	40,	48,	56,
					1,	9,	17,	25,	33,	41,	49,	57,
					2,	10,	18,	26,	34,	42,	50,	58,
					3,	11,	19,	27,	35,	43,	51,	59,
					4,	12,	20,	28,	36,	44,	52,	60,
					5,	13,	21,	29,	37,	45,	53,	61,
					6,	14,	22,	30,	38,	46,	54,	62,
					7,	15,	23,	31,	39,	47,	55,	63,
				}
	}
};
#pragma  endregion

#pragma  region
#define BIN_MODE_0	0
#define BIN_MODE_1	1
#define BIN_MODE_2	2
#define BIN_MODE_3	3
#define BYP_MODE_0	4
#define BYP_MODE_1	5
#define BYP_MODE_2	6
#define BYP_MODE_3	7
#define	EST_CABAC	1
//#define ST_CTU 							EST_CABAC + 1	


#define ST_CTU_IDLE                                       	EST_CABAC														+ 1				  //2
#define ST_SAO_MERGE_LEFT_FLAG           			ST_CTU_IDLE                     							+ 1                   	  //3
#define ST_SAO_MERGE_UP_FLAG             			ST_SAO_MERGE_LEFT_FLAG          				+ 1                   	  //4
#define ST_SAO_GEN_YCBCR                 				ST_SAO_MERGE_UP_FLAG    			        	+ 1                   	  //5
#define ST_SAO_TYPE_IDX                  					ST_SAO_GEN_YCBCR                					+ 1                   	  //6
#define ST_SAO_OFFSET_ABS                				ST_SAO_TYPE_IDX                 						+ 1                   	  //7
#define ST_SAO_OFFSET_SIGN               				ST_SAO_OFFSET_ABS               					+ 1                   	  //8
#define ST_SAO_BAND_POSITION            			ST_SAO_OFFSET_SIGN              					+ 1                   	  //9
#define ST_SAO_EO_CLASS_LUMA             			ST_SAO_BAND_POSITION            				+ 1                   	  //10
#define ST_SAO_EO_CLASS_CHROMA           		ST_SAO_EO_CLASS_LUMA            				+ 1                   	  //11
#define ST_CTU                           							ST_SAO_EO_CLASS_CHROMA          			+ 1                   	  //12
#define ST_EMPTY_CU                      						ST_CTU                          								+ 1                 //13
#define ST_SPLIT_CU_FLAG                 					ST_EMPTY_CU                     							+ 1                     //14
#define ST_CU                            							ST_SPLIT_CU_FLAG                						+ 1                     //15
#define ST_CU_TRANSQUANT_BYPASS_FLAG    ST_CU                           								+ 1                   	  //16
#define ST_CU_SKIP_FLAG                  					ST_CU_TRANSQUANT_BYPASS_FLAG    		+ 1                   	  //17
#define ST_MERGE_IDX                     					ST_CU_SKIP_FLAG                 						+ 1                   	  //18
#define ST_PRED_MODE_FLAG                				ST_MERGE_IDX                    							+ 1                     //19
#define ST_PART_MODE                    		 			ST_PRED_MODE_FLAG               					+ 1                   	  //20
#define ST_PCM_FLAG                      						ST_PART_MODE                    						+ 1                     //21
#define ST_PREV_INTRA_LUMA_PRED_FLAG     	ST_PCM_FLAG                     							+ 1                   	  //22
#define ST_REM_INTRA_LUMA_PRED_MODE     	ST_PREV_INTRA_LUMA_PRED_FLAG    		+ 1                   		  //23
#define ST_INTRA_CHROMA_PRED_MODE        	ST_REM_INTRA_LUMA_PRED_MODE     		+ 1                   	  //24
#define ST_RQT_ROOT_CBF                  				ST_INTRA_CHROMA_PRED_MODE       		+ 1                   		  //25
#define ST_PCM_ALIGNMENT_ZERO_BIT        		ST_RQT_ROOT_CBF                 						+ 1                     //26
#define ST_PCM_SAMPLE_LUMA               			ST_PCM_ALIGNMENT_ZERO_BIT       			+ 1                   	  //27
#define ST_PCM_SAMPLE_CHROMA             		ST_PCM_SAMPLE_LUMA              				+ 1                   	  //28
#define ST_PU                            							ST_PCM_SAMPLE_CHROMA            			+ 1                   	  //29
#define ST_MERGE_FLAG                    					ST_PU                           								+ 1                     //30
#define ST_INTER_PRED_IDC                				ST_MERGE_FLAG                   						+ 1                   	  //31
#define ST_REF_IDX_L0                    						ST_INTER_PRED_IDC               						+ 1                 //32
#define ST_REF_IDX_L1                    						ST_REF_IDX_L0                   							+ 1                 //33
#define ST_MVP_L0_FLAG                  					ST_REF_IDX_L1                   							+ 1                     //34
#define ST_MVP_L1_FLAG                   					ST_MVP_L0_FLAG                  						+ 1                   	  //35
#define ST_ABS_MVD_GREATER0_FLAG_X       	ST_MVP_L1_FLAG                  						+ 1                   		  //36
#define ST_ABS_MVD_GREATER0_FLAG_Y       	ST_ABS_MVD_GREATER0_FLAG_X      			+ 1                   		  //37
#define ST_ABS_MVD_GREATER1_FLAG_X      	 	ST_ABS_MVD_GREATER0_FLAG_Y      			+ 1                   	  //38
#define ST_ABS_MVD_GREATER1_FLAG_Y       	ST_ABS_MVD_GREATER1_FLAG_X      			+ 1                   		  //39
#define ST_ABS_MVD_MINUS2		              			ST_ABS_MVD_GREATER1_FLAG_Y      			+ 1                     //40
//#define ST_ABS_SIGN_FLAG_X               				ST_ABS_MVD_MINUS2_X             				+ 1                     //
//#define ST_ABS_MVD_MINUS2_Y              			ST_ABS_SIGN_FLAG_X              					+ 1                     //   
//#define ST_ABS_SIGN_FLAG_Y               				ST_ABS_MVD_MINUS2_Y             				+ 1                     //
#define ST_TTU															ST_ABS_MVD_MINUS2									+ 1			  //41
#define ST_CBF_CB													ST_TTU 															+ 1				  //42
#define ST_CBF_CR													ST_CBF_CB 														+ 1			  //43
#define ST_CBF_LUMA												ST_CBF_CR 														+ 1		  //44
#define ST_TU 															ST_CBF_LUMA												+ 1		      //45
#define ST_CU_QP_DELTA_ABS 								ST_TU 																+ 1			  //46
//#define ST_CU_QP_DELTA_SIGN_FLAG 				ST_CU_QP_DELTA_ABS 									+ 1					  //
#define ST_RU 															ST_CU_QP_DELTA_ABS 						+ 1						  //47
#define ST_TRANSFROM_SKIP_FLAG 					ST_RU 																+ 1				  //48
#define ST_LOOK_FOR_LAST_POS 						ST_TRANSFROM_SKIP_FLAG 						+ 1						  //49
#define ST_LAST_SIG_COEFF_X_PREFIX 				ST_LOOK_FOR_LAST_POS 								+ 1					  //50
#define ST_LAST_SIG_COEFF_Y_PREFIX 				ST_LAST_SIG_COEFF_X_PREFIX 					+ 1							  //51
#define ST_LAST_SIG_COEFF_X_SUFFIX 				ST_LAST_SIG_COEFF_Y_PREFIX 					+ 1							  //52
#define ST_LAST_SIG_COEFF_Y_SUFFIX 				ST_LAST_SIG_COEFF_X_SUFFIX 					+ 1							  //53
#define ST_CODED_SUB_BLOCK_FLAG 					ST_LAST_SIG_COEFF_Y_SUFFIX 					+ 1							  //54
#define ST_SIG_COEFF_FLAG 									ST_CODED_SUB_BLOCK_FLAG 						+ 1					  //55
#define ST_COEFF_ABS_LEVEL_GREATER1_FLAG			ST_SIG_COEFF_FLAG 										+ 1				  //56
#define ST_COEFF_ABS_LEVEL_GREATER2_FLAG			ST_COEFF_ABS_LEVEL_GREATER1_FLAG 		+ 1					  //57
#define ST_COEFF_SIGN_FLAG								ST_COEFF_ABS_LEVEL_GREATER2_FLAG 		+ 1						  //58
#define ST_COEFF_ABS_LEVEL_REMAINING			ST_COEFF_SIGN_FLAG 									+ 1					  //59
#define ST_LUMA_RU_EST										ST_COEFF_ABS_LEVEL_REMAINING 			+ 1						  //60
#define ST_CB_RU_EST												ST_LUMA_RU_EST 											+ 1			  //61
#define ST_CR_RU_EST												ST_CB_RU_EST 												+ 1			  //62
#define ST_TU_EST       											ST_CR_RU_EST 												+ 1				  //63
#define ST_TU_EST_WAIT 										ST_TU_EST 														+ 1			  //64
#define CTU_EST_END  											ST_TU_EST_WAIT 											+ 1				  //65

// enum
// {
// 	MODE_INTER,           ///< inter-prediction mode
// 	MODE_INTRA,           ///< intra-prediction mode
// 	MODE_PCM,
// 	MODE_SKIP,
// 	MODE_EMPTY,
// 	MODE_INTRA_BC,
// 	MODE_NONE = 15
// };
#define	INTRA_LUMA_PRED_MODE_GOLOMB_OFFSET		20
#define	int64_t long long 
//#define uint64_t unsigned long long
//#define	uint32_t unsigned long
#define	FIFO_BIT_WIDTH 64
int64_t enc_out_data[2],enc_out_dat_output;
int 	fifo_has_output;
int 	bit_offset = 0;
int 	fifo_index = 0;

uint64_t get_bits(uint64_t src,int size,int offset)
{
	unsigned long long temp = 0;
	assert(size + offset <= FIFO_BIT_WIDTH);
	for(int i = 0; i < size ; i++)
	{
		temp <<= 1;
		temp |=1;
	}
	temp &= (src >> offset);
	temp <<= offset;
	return temp;
}

void	write_to_fifo(int bits, int size)
{

	if(size + bit_offset >= 64)
	{
		int	len = FIFO_BIT_WIDTH - bit_offset;
		enc_out_data[fifo_index] = (get_bits(bits,len,0) << bit_offset) | get_bits(enc_out_data[fifo_index],bit_offset,0);
		fifo_has_output = 1;
		enc_out_dat_output = enc_out_data[fifo_index];
		fifo_index++;
		fifo_index %= 2;

		enc_out_data[fifo_index] = (get_bits(bits,size + bit_offset - FIFO_BIT_WIDTH,len) >> len);
		bit_offset = size + bit_offset - FIFO_BIT_WIDTH;

	}
	else
	{

		enc_out_data[fifo_index] = ((bits <<  (64 - size)) >> (64 - size - bit_offset)) | enc_out_data[fifo_index];
		bit_offset+=size;
	}
}
void	CABAC_RDO::est_bins(int bin, int len , int valbin_ctxInc_r[4] , int ctx_offset)
{
	if (len == 0)
	{
		return;
	}
	Enc_input_est   enc_input_est;
	Enc_output_est   enc_output_est = {0};

	enc_input_est.binval0 = bin & 0x1;
	enc_input_est.binval1 = (bin & 0x2)>>1;
	enc_input_est.binval2 = (bin & 0x4)>>2;
	enc_input_est.binval3 = (bin & 0x8)>>3;
	enc_input_est.val_len_r = len;
	enc_input_est.est_bit_in = 0 ;
	hevc_cabac_decision_enc_est(   enc_input_est,     enc_output_est,  valbin_ctxInc_r  ,  ctx_offset);

	tu_est_bits[IsIntra_tu][depth_tu] += enc_output_est.est_bit_out;

	if (ctx_offset == LAST_SIGNIFICANT_COEFF_X_PREFIX_OFFSET)
	{
		est_bit_last_sig_coeff_x_prefix[IsIntra_tu][depth_tu] += enc_output_est.est_bit_out;
	}
	else if (ctx_offset == LAST_SIGNIFICANT_COEFF_Y_PREFIX_OFFSET)
	{
		est_bit_last_sig_coeff_y_prefix[IsIntra_tu][depth_tu] += enc_output_est.est_bit_out;
	}
	else if (ctx_offset == SIGNIFICANT_COEFF_FLAG_OFFSET)
	{
		est_bit_sig_coeff_flag[IsIntra_tu][depth_tu] += enc_output_est.est_bit_out;
	}
	else if (ctx_offset == COEFF_ABS_LEVEL_GREATER1_FLAG_OFFSET)
	{
		est_bit_coeff_abs_level_greater1_flag[IsIntra_tu][depth_tu] += enc_output_est.est_bit_out;
	}
	else if (ctx_offset == COEFF_ABS_LEVEL_GREATER2_FLAG_OFFSET)
	{
		est_bit_coeff_abs_level_greater2_flag[IsIntra_tu][depth_tu] += enc_output_est.est_bit_out;
	}
	else if (ctx_offset == SIGNIFICANT_COEFF_GROUP_FLAG_OFFSET)
	{
		est_bit_coded_sub_block_flag[IsIntra_tu][depth_tu] += (enc_output_est.est_bit_out);
	}
	
}

void	CABAC_RDO::est_bypass( int len , int mode)
{
	tu_est_bits[IsIntra_tu][depth_tu] += len*32768;

	if (mode == 0)
	{
		est_bit_last_sig_coeff_x_suffix[IsIntra_tu][depth_tu]+= len*32768;
	} 
	else if (mode == 1)
	{
		est_bit_last_sig_coeff_y_suffix[IsIntra_tu][depth_tu]+= len*32768;
	}
	else if (mode == 2)
	{
		est_bit_coeff_sign_flag[IsIntra_tu][depth_tu]+= len*32768;
	}
	else if (mode == 3)
	{
		est_bit_coeff_abs_level_remaining[IsIntra_tu][depth_tu]+= len*32768;
	}
	return;
}

void CABAC_RDO::est_bin1(int  last_sig_coeff_x_prefix_r , int last_sig_coeff_y_prefix_r  , int ctx_offset_x , int ctx_offset_y)
{
	int x_binIdx_0_status = mModels[IsIntra_tu][depth_tu][ctx_offset_x].get_m_ucState();
	int x_binIdx_1_status = mModels[IsIntra_tu][depth_tu][ctx_offset_x+1].get_m_ucState();
	int x_binIdx_2_status = mModels[IsIntra_tu][depth_tu][ctx_offset_x+2].get_m_ucState();

	switch (last_sig_coeff_x_prefix_r)
	{
	case 0x0:  
		est_bit_last_sig_coeff_x_prefix[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_0[x_binIdx_0_status]; 
		mModels[IsIntra_tu][depth_tu][ctx_offset_x].set_m_ucState(aucNextState_0[x_binIdx_0_status]);

		tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_0[x_binIdx_0_status];
		break;
		
	case 0x1: 
		est_bit_last_sig_coeff_x_prefix[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[x_binIdx_0_status] + CABAC_g_entropyBits_0[x_binIdx_1_status]; 
		mModels[IsIntra_tu][depth_tu][ctx_offset_x].set_m_ucState(aucNextState_1[x_binIdx_0_status]);
		mModels[IsIntra_tu][depth_tu][ctx_offset_x + 1].set_m_ucState(aucNextState_0[x_binIdx_1_status]);

		tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[x_binIdx_0_status] + CABAC_g_entropyBits_0[x_binIdx_1_status]; 
		break;

	case 0x2: 
		est_bit_last_sig_coeff_x_prefix[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[x_binIdx_0_status] + CABAC_g_entropyBits_1[x_binIdx_1_status] + CABAC_g_entropyBits_0[x_binIdx_2_status]; 
		mModels[IsIntra_tu][depth_tu][ctx_offset_x].set_m_ucState(aucNextState_1[x_binIdx_0_status]);
		mModels[IsIntra_tu][depth_tu][ctx_offset_x + 1].set_m_ucState(aucNextState_1[x_binIdx_1_status]);
		mModels[IsIntra_tu][depth_tu][ctx_offset_x + 2].set_m_ucState(aucNextState_0[x_binIdx_2_status]);

		tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[x_binIdx_0_status] + CABAC_g_entropyBits_1[x_binIdx_1_status] + CABAC_g_entropyBits_0[x_binIdx_2_status]; 
		break;

	case 0x3: 
		est_bit_last_sig_coeff_x_prefix[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[x_binIdx_0_status] + CABAC_g_entropyBits_1[x_binIdx_1_status] + CABAC_g_entropyBits_1[x_binIdx_2_status]; 
		mModels[IsIntra_tu][depth_tu][ctx_offset_x].set_m_ucState(aucNextState_1[x_binIdx_0_status]);
		mModels[IsIntra_tu][depth_tu][ctx_offset_x + 1].set_m_ucState(aucNextState_1[x_binIdx_1_status]);
		mModels[IsIntra_tu][depth_tu][ctx_offset_x + 2].set_m_ucState(aucNextState_1[x_binIdx_2_status]);

		tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[x_binIdx_0_status] + CABAC_g_entropyBits_1[x_binIdx_1_status] + CABAC_g_entropyBits_1[x_binIdx_2_status]; 
		break;
	default:
		assert(0);
	}

	int y_binIdx_0_status = mModels[IsIntra_tu][depth_tu][ctx_offset_y].get_m_ucState();
	int y_binIdx_1_status = mModels[IsIntra_tu][depth_tu][ctx_offset_y+1].get_m_ucState();
	int y_binIdx_2_status = mModels[IsIntra_tu][depth_tu][ctx_offset_y+2].get_m_ucState();

	switch (last_sig_coeff_y_prefix_r)
	{
	case 0x0:  
		est_bit_last_sig_coeff_y_prefix[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_0[y_binIdx_0_status]; 
		mModels[IsIntra_tu][depth_tu][ctx_offset_y].set_m_ucState(aucNextState_0[y_binIdx_0_status]);

		tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_0[y_binIdx_0_status]; 
		break;

	case 0x1: 
		est_bit_last_sig_coeff_y_prefix[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[y_binIdx_0_status] + CABAC_g_entropyBits_0[y_binIdx_1_status]; 
		mModels[IsIntra_tu][depth_tu][ctx_offset_y].set_m_ucState(aucNextState_1[y_binIdx_0_status]);
		mModels[IsIntra_tu][depth_tu][ctx_offset_y + 1].set_m_ucState(aucNextState_0[y_binIdx_1_status]);

		tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[y_binIdx_0_status] + CABAC_g_entropyBits_0[y_binIdx_1_status]; 
		break;

	case 0x2: 
		est_bit_last_sig_coeff_y_prefix[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[y_binIdx_0_status] + CABAC_g_entropyBits_1[y_binIdx_1_status] + CABAC_g_entropyBits_0[y_binIdx_2_status]; 
		mModels[IsIntra_tu][depth_tu][ctx_offset_y].set_m_ucState(aucNextState_1[y_binIdx_0_status]);
		mModels[IsIntra_tu][depth_tu][ctx_offset_y + 1].set_m_ucState(aucNextState_1[y_binIdx_1_status]);
		mModels[IsIntra_tu][depth_tu][ctx_offset_y + 2].set_m_ucState(aucNextState_0[y_binIdx_2_status]);

		tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[y_binIdx_0_status] + CABAC_g_entropyBits_1[y_binIdx_1_status] + CABAC_g_entropyBits_0[y_binIdx_2_status]; 
		break;

	case 0x3: 
		est_bit_last_sig_coeff_y_prefix[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[y_binIdx_0_status] + CABAC_g_entropyBits_1[y_binIdx_1_status] + CABAC_g_entropyBits_1[y_binIdx_2_status]; 
		mModels[IsIntra_tu][depth_tu][ctx_offset_y].set_m_ucState(aucNextState_1[y_binIdx_0_status]);
		mModels[IsIntra_tu][depth_tu][ctx_offset_y + 1].set_m_ucState(aucNextState_1[y_binIdx_1_status]);
		mModels[IsIntra_tu][depth_tu][ctx_offset_y + 2].set_m_ucState(aucNextState_1[y_binIdx_2_status]);

		tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[y_binIdx_0_status] + CABAC_g_entropyBits_1[y_binIdx_1_status] + CABAC_g_entropyBits_1[y_binIdx_2_status]; 
		break;
	default:
		assert(0);
	}
	
	return;

}

int get_coeff_suffix_len(int last_sig_coeff_prefix)
{
	int table[10]={0,0,0,0,1,1,2,2,3,3};
	return table[last_sig_coeff_prefix];
}
/*
int get_coeff_len(int coeff_value,int riceparam)
{
	int len;
	int prefixVal = coeff_value - (4 << riceparam);
	for(int i = 14; i >=0; i --)
	{
		if(prefixVal >= (1 << i) - 1)
		{
			len =  i * 2 + 1 + riceparam;
			return len;
		}
	}

	for(int i = riceparam - 1; i >= 0; i++)
	{
		if(coeff_value > 1 << i)
		{
			return i ;
		}
	}
	return len;

}
*/
int get_coeff_len(uint32_t symbol, uint32_t param)
{
	int codeNumber  = (int)symbol;
	uint32_t length;

	if (codeNumber < (3 << param))
	{
		length = codeNumber >> param;
		//m_binIf->encodeBinsEP((1 << (length + 1)) - 2, length + 1);
		//m_binIf->encodeBinsEP((codeNumber % (1 << param)), param);
		return length + 1 + param;
	}
	else
	{
		length = param;
		codeNumber  = codeNumber - (3 << param);
		while (codeNumber >= (1 << length))
		{
			codeNumber -=  (1 << (length++));
		}

		//m_binIf->encodeBinsEP((1 << (COEF_REMAIN_BIN_REDUCTION + length + 1 - param)) - 2, COEF_REMAIN_BIN_REDUCTION + length + 1 - param);
		//m_binIf->encodeBinsEP(codeNumber, length);
		return 3 + length + 1 - param + length;
	}

}
int get_coeff_len(int coeff_value,int riceparam,int value[3], int len[3])
{
	int length;
	int suffix_mask[5] = { 0xf,0x7,0x3,0x1,0};
	int suffixVal = coeff_value - (4 << riceparam);
	if (coeff_value < (4 << riceparam))
	{
		length = coeff_value >> riceparam;
		value[0] 	= (1 << (length + 1)) - 2;
		value[1] 	= (coeff_value % (1 << riceparam));
		len[0] 		= length + 1;
		len[1]			= riceparam;

	}
	else
	{
		length = riceparam;

		if(suffixVal >= (((1 << 15) - 1) | suffix_mask[riceparam]))
		{
			assert(0);
		}
		else	if(suffixVal >= (((1 << 14) - 1) | suffix_mask[riceparam]))
		{
			length = 15;
		}
		else	if(suffixVal >= (((1 << 13) - 1) | suffix_mask[riceparam]))
		{
			length = 14;
		}
		else	if(suffixVal >= (((1 << 12) - 1) | suffix_mask[riceparam]))
		{
			length = 13;
		}
		else	if(suffixVal >= (((1 << 11) - 1) | suffix_mask[riceparam]))
		{
			length = 12;
		}
		else	if(suffixVal >= (((1 << 10) - 1) | suffix_mask[riceparam]))
		{
			length = 11;
		}
		else	if(suffixVal >= (((1 << 9) - 1) | suffix_mask[riceparam]))
		{
			length = 10;
		}
		else	if(suffixVal >= (((1 << 8) - 1) | suffix_mask[riceparam]))
		{
			length = 9;
		}
		else	if(suffixVal >= (((1 << 7) - 1) | suffix_mask[riceparam]))
		{
			length = 8;
		}
		else	if(suffixVal >= (((1 << 6) - 1) | suffix_mask[riceparam]))
		{
			length = 7;
		}
		else	if(suffixVal >= (((1 << 5) - 1) | suffix_mask[riceparam]))
		{
			length = 6;
		}
		else	if(suffixVal >= (((1 << 4) - 1) | suffix_mask[riceparam]))
		{
			length = 5;
			assert(riceparam <= 4);
		}
		else	if(suffixVal >= (((1 << 3) - 1) | suffix_mask[riceparam]))
		{
			assert(riceparam <= 3);
			length = 4;
		}

		else	if(suffixVal >= (((1 << 2) - 1) | suffix_mask[riceparam]))
		{
			length = 3;
			assert(riceparam <= 2);
		}

		else	if(suffixVal >= (((1 << 1) - 1) | suffix_mask[riceparam]))
		{
			length = 2;
			assert(riceparam <= 1);
		}

		else	if(suffixVal >= (((1 << 0) - 1) | suffix_mask[riceparam]))
		{
			length = 1;
			assert(riceparam <= 0);
		}
		value[0] 	= (1 << ( length - riceparam)) - 2;
		value[1] 	= suffixVal;
		len[0] 		= 4 + length  - riceparam;
		len[1]		= length;		
	}
	return len[0] + len[1];

}
// bool read_from_fifo(int mSize, uint64_t data)
// {
// 
// 	return true;
// }
// void	enc_bins(int bin, int len,int mode =0)
// {
// 	return;
// }
int	get_num_bit(int data, int offset,int num)
{
	int	mMask;
	for(int i = 0; i < num; i++)
	{
		mMask |= 1<< i;
	}
	return (data >> offset) & mMask;
}
int	set_bit(int data,int val, int offset)
{
	int	mMask = 0xffffffff;

	mMask ^= 1 << offset;
	return (data & mMask)  | (val << offset);
}
// void	enc_bypass(int val, int len,int mode = 0)
// {
// 	return;
// }
// void	enc_ori(int val, int len)
// {
// 	return;
// }
// void enc_bin1(int *ctxinc,int *Case,int len)
// {
// 	return;
// }

#pragma  endregion

void CABAC_RDO::cabac_est(int TU_chroma_size,int16_t *TU_Resi , int TU_type)//TU_type 0 1 2分别表示YUV
{
	int16_t coeff[64][64];
	for (int i = 0 ; i < TU_chroma_size; i++)
	{
		for (int j = 0 ; j < TU_chroma_size; j++)
		{
			coeff[i][j] = TU_Resi[i * TU_chroma_size + j];
		}
	}
	
// 	int16_t testPix[64] =
// 	{
// 		-18  ,   +0  ,  +0   ,  +0 ,   +0   ,  +0  ,   +0  ,   +0 	,
// 		+0   ,  +0   , +0    , +0  ,  +0    , +0   ,  +0   ,  +0	,
// 		+0   ,  +0   , +0    , +0  ,  +0    , +0   ,  +0   ,  +0	,
// 		+0   ,  +0   , +0    , +0  ,  +0    , +0   ,  +0   ,  +0	,
// 		+5   ,  +0   , +0    , +0  ,  +0    , +1   ,  +2   ,  +3	,
// 		+0   ,  +0   , +0    , +0  ,  +4    , -5   ,  +6   ,  +0	,
// 		+0   ,  +0   , +0    , +0  ,  +8    , +0   ,  -10   ,  +11	,
// 		+0   ,  +0   , +0    , +0  ,  +0    , -13   ,  +1   ,  +0 
// 	};
// 	int16_t testPix[64] =
// 	{
// 		-18  ,   +1  ,  +0   ,  +0 ,   +0   ,  +0  ,   +0  ,   +0 	,
// 		+0   ,  +0   , +0    , +0  ,  +0    , +0   ,  +0   ,  +0	,
// 		+0   ,  +0   , +0    , +0  ,  +0    , +0   ,  +0   ,  +0	,
// 		+0   ,  +0   , +0    , +0  ,  +0    , +0   ,  +0   ,  +0	,
// 		  0  ,   +0  ,  +0   ,  +0 ,   +0   ,  +0  ,   +0  ,   +0 	,
// 		+0   ,  +0   , +0    , +0  ,  +0    , +0   ,  +0   ,  +0	,
// 		+0   ,  +0   , +0    , +0  ,  +0    , +0   ,  +0   ,  +0	,
// 		+0   ,  +0   , +0    , +0  ,  +0    , +0   ,  +0   ,  +0	
// 	};
// 	for (int i = 0 ; i < size; i++)
// 	{
// 		for (int j = 0 ; j < size; j++)
// 		{
// 			coeff[i][j] = testPix[i * size + j];
// 		}
// 	}
	int first_to_greater1_r = 1;
	int blkidx_r = 3 ;

	int log2TrafoSize = TU_chroma_size == 64 ? 6 : (TU_chroma_size == 32 ? 5 : (TU_chroma_size == 16 ? 4 : (TU_chroma_size == 8 ? 3 : (TU_chroma_size == 4 ? 2 : 0))));
	int scanorder_mode = 0 ;
	int predModeIntra;
	if (IsIntra_tu==1 && ( log2TrafoSize ==2 || (log2TrafoSize ==3 && TU_type == 0) ) )
	{
		if (TU_type == 0)
		{
			predModeIntra = intra_pu_depth_luma_bestDir[depth_tu];
		}
		else 
		{
			predModeIntra = intra_pu_depth_chroma_bestDir[depth_tu];//这里是要赋值为色度的方向，在RK方案中，亮度和色度方向是一样的。
		}

		if (predModeIntra >=6 && predModeIntra <= 14)
		{
			scanorder_mode = 2;
		}
		else if (predModeIntra >=22 && predModeIntra <= 30)
		{
			scanorder_mode = 1;
		}
		else
		{
			scanorder_mode = 0;
		}
	}
	else
	{
		scanorder_mode = 0;
	}

	uint32_t g_minInGroup[10] = { 0, 1, 2, 3, 4, 6, 8, 12, 16, 24 };
	uint32_t g_groupIdx[32]   = { 0, 1, 2, 3, 4, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9 };

	int	coeff_value[16] = { 0 };
	int	coeff_value_r[16] = { 0 };
	int ctxIdxMap[16] = { 0,1,4,5,2,3,4,5,6,6,8,8,7,7,8 ,55}; //TU为2时 sig_flag使用的table  最后一个55协议是没有的
	//	int tu_scan_table[4][64];
	
	
//	int only_dc_flag  ;
	int find_last_sub_block = 0 ;
	int subnum_icnt_r = 0 ;
	int codedsub_xs = 0 ;
	int codedsub_ys = 0 ;
	//	int log2TrafoSize;
	int cabac_cs = 0 ,cabac_ns = 0 ;
	int last_sig_coeff_x = 0 ,last_sig_coeff_y = 0 ,last_sig_coeff_x_prefix = 0 , last_sig_coeff_y_prefix = 0 ,last_sig_coeff_x_suffix = 0 ,last_sig_coeff_y_suffix = 0 ;
	int last_sig_coeff_x_prefix_r = 0 , last_sig_coeff_y_prefix_r = 0;
	int lastsubblock_num = 0 ;
	int lastsubblock_tilenum = 0 ;
	int lastcoeff_num  ;
	int lastcoeff_tilenum = 0 ;
	int first_sig_coeff_cycle = 1 ;
	int subnum_lastone_flag_r = 0;
	int ru_luma_flag = 0 ,ru_cb_flag = 0  ,ru_cr_flag = 0 ;
//////	int cbf_cb_exist = 0 , cbf_cr_exist = 0 , cbf_luma_exist = 0 ;
	int tu_loop_cnt_r = 0 ,max_tu_cnr = 0 ;
	int cbf_cb_val = 0 , cbf_cr_val = 0 ,cbf_luma_r = 1 ;
	int	cbf_cb_r[4] = { 0 },cbf_cr_r[4] = { 0 };
	int Quant_finish_flag[4][4] = { 0 };
	int trafodepth_r = 0 ;

	int coeff_len_r = 0 ;
	int coeff_len[16] = { 0 };
	int	coeff_abs_level_remaining_len_r[16] = { 0 };
	int	last_sig_coeff_y_suffix_len_r = 0 ;
	int last_sig_coeff_x_suffix_len_r = 0 ;
	int coeff_abs_level_remaining_16bit_flag = 0 ;
	int coeff_abs_level_remaining_8bit_flag = 0 ;
	int coeff_abs_level_remaining_4bit_flag = 0 ;
	int coeff_abs_level_remaining_2bit_flag = 0 ;

	int sig_coeff_flag_r[16] = { 0 };
	int sig_coeff_flag_enc_out_r[16] = { 0 };
	int sig_coeff_flag[16] = { 0 };
	int coeff_abs_level_greater1_flag_r[16] = { 0 };
	int coeff_abs_level_greater1_flag_enc_out_r[16] = { 0 };
	int coeff_abs_level_greater1_flag[16] = { 0 };
	int coeff_abs_level_greater2_flag_r = 0 ;
	int coeff_abs_level_remaining_r[16] = { 0 };

	int coded_sub_block_flag_r[64] = { 0 };
	int coded_sub_block_flag = 0 ;
	int coded_sub_block_flag_out = 0 ;
	int coeff_sign_flag_r[16] = { 0 };
	int coeff_sign_flag_enc_out_r[16] = { 0 };
	int coeff_sign_flag[16] = { 0 };
	int cofgreater2_flag_pos_r = 0 ;
	int cofgreater2_flag_found = 0 ;
	int cofgreater0_flag_pos_r[16] = { 0 };
	int cofgreater1_flag_pos_r[16] = { 0 };
//	int cofgreater1_flag_pos[16] = { 0 };
	int coeff_abs_level_remaining_pos_r[16] = { 0 };


	int tu_cidx_luma_flag = TU_type ;//0表示是luma  1表示不是luma
	int cof_sigctx_luma[4] = { 0 } ;
	int cof_sigctx_chroma[4] = { 0 };


	int inferSbDcSigCoeffFlag = 0 , inferSbDcSigCoeffFlag_r = 0 ;
//	int ru_subblock_parameter  ;
	int ru_subblock_parameter_len_r = 0 ;
	int ru_subblock_parameter_r = 0 ;
//	int i;
	//	int coeff_value[16];//enc_out len

	int val_len_r = 0 ;//enc_out len
	int val_len_coded_sub_block_flag = 0 ;
	int val_len_coeff_abs_level_greater1_flag = 0 ;
	int val_len_coeff_abs_level_greater2_flag = 0 ;
	int val_len_coeff_abs_level_greater2_flag_r = 0 ;
	int val_len_sig_coeff_flag = 0 ;
	int val_len_coeff_abs_level_remaining = 0 ;
//	int val_len_coeff_sign_flag  ;
//	int val_len_last_sig_coeff_x_prefix ;
	int val_len_last_sig_coeff_xy_prefix = 0 ;
//	int val_len_last_sig_coeff_y_prefix  ;
//	int valbin_ecs_last_sig_coeff_y_prefix  ;
//	int valbin_ecs_last_sig_coeff_x_prefix  ;
	int valbin_ecs_last_sig_coeff_xy_prefix = 0 ;
	int valbin_ecs_sig_coeff_flag = 0 ;//最多连续4个sig_coeff_flag组成的bin
	int valbin_ecs_coeff_abs_level_greater1_flag = 0 ;
	int valbin_ecs_coeff_abs_level_greater2_flag = 0 ;
	int valbin_ecs_coeff_abs_level_greater2_flag_r = 0 ;
	int valbin_ecs_codec_block_flag = 0 ;
//	int valbin_ecs_coeff_sign_flag  ;
	int64_t valbin_ecs_coeff_abs_level_remaining = 0 ;
//	int valbin_ecs  ;
	int valbin_r = 0 ;//enc_out data
	int prevcsbf = 0 ;
	int csbfCtx = 0 ;
	int ctxSet = 0 ;
	int ctxSet_r = 0 ;
	int codedsub_xp[4] = { 0 };
	int codedsub_yp[4] = { 0 };
	int tu_scanraster_r,tu_scanraster = 0 ;
	int ru_scanraster[4] = { 0 };
	int cof_sigctx_size4[4] = { 0 };

	int valbin_ecs_greater1_for_cnt ;//在idx上连续的4个系数是否大于1的组合，主要用于求greater1 系数的个数
	int sigctx[4] = { 0 };
	int greater1Ctx[4] = { 0 };
	int cofsig_ctxinc[4] = { 0 };
	int codedsub_ctxinc = 0 ;
	int codedsub_ctxinc_ff1;
	int cofsig_ctxinc_ff1[4] = { 0 };
	int cofgreater1_ctxinc[4] = { 0 };
	int cofgreater1_ctxinc_ff1[4] = { 0 };
	int cofgreater2_ctxinc = 0 ;
	int cofgreater2_ctxinc_r[4] = {0} ;
	int cofgreater2_ctxinc_ff1 = 0 ;
	int last_sig_coeff_xy_prefix_ctx_inc[4] = { 0 };
	int valbin_ctxInc_r[4] = {0};
	int last_sig_coeff_xy_prefix_ctx_inc_ff1[4] = { 0 };
	int last_sig_coeff_prefix_ctx_inc[4] = { 0 };
	int subnum_icnt_minus1 = 0 ;
	int last_sig_coeff_xy_prefix_case[4] = { 0 };

	int last_sig_coeff_x_prefix_len_r = 0 ;
	int last_sig_coeff_x_prefix_cnt_idx_r = 0 ;
	int last_sig_coeff_x_prefix_cnt_idx_ff1_r = 0 ;
	int last_sig_coeff_y_prefix_len_r = 0 ;
	int last_sig_coeff_y_prefix_cnt_idx_r = 0 ;
	int last_sig_coeff_y_prefix_cnt_idx_ff1_r = 0 ;
	int coeff_abs_level_remaining_cnt_r = 0 ;
	int coeff_abs_level_remaining_cnt_idx_r = 0 ;//idx == cnt,not cnt - 1
	int coeff_abs_level_remaining_cnt_ff1_r = 0 ;
	int coeff_abs_level_remaining_cnt_idx_minus[4] = { 0 };



	int 	cofgreater0_cnt_idx_r = 0 ;//sig_coeff_flag_r == 1 's num
//	int cofgreater1_cnt_idx_ff1_r = 0 ;//delay 1 cycle of cofgreater0_cnt_idx_r
	int cofgreater0_cnt_idx_minus[4] = { 0 };//sig_coeff_flag_r == 1 's num
	int 	cofgreater1_cnt_r = 0 ;//sig_coeff_flag_r == 1 's num
//	int 	cofgreater1_cnt = 0 ;//sig_coeff_flag_r == 1 's num
	int cofgreater1_cnt_plus[4] = { 0 };

	int cofgreater0_cnt = 0 ;
	int 	cofgreater0_cnt_r = 0 ;//sig_coeff_flag_r == 1 's num
	int cofgreater0_cnt_plus[4] = { 0 };
	int sig_coeff_flag_r_cnt_idx_ff1_r = 0 ;
	int sig_coeff_flag_r_cnt_idx_r = 0 ;
	int sig_coeff_flag_r_cnt_idx_minus[4] = { 0 } ;
	int sig_coeff_flag_blk0_zero_r = 0 ;


	int coeff_sign_flag_cnt_r = 0 ;
	int coeff_sign_flag_cnt_idx_r = 0 ;
//	int coeff_sign_flag_cnt_idx = 0 ;
//	int coeff_sign_flag_cnt_idx_ff1_r = 0 ;
//	int coeff_sign_flag_cnt_idx_minus[4] = { 0 };

	int lastGreater1Ctx = 0 ;
	int Greater1Flag[4] = {0} ;
//	int Greater1Flag_for_cnt[4]  ;
	int lastGreater1Flag_r, lastGreater1Ctx_r = 0 ;

	int numGreater1Flag_solved_r = 0 ;//用来表示大于1个系数已经估算了几个numGreater1Flag_for_cnt_solved_r
	int greater1_avail_pos = 0 ;//用来与greater1相与 取出有效的bit
//	int	Greater1Flag_equal_1_num  ,Greater1Flag_equal_1_num_ff1  l;
	int Greater1Flag_num_avail = 0 ;
	int firstSigScanPos = 0 , firstSigScanPos_r  = 0 ;
	int	lastSigScanPos = 0 ;
//	int lastSigScanPos_r  ;
	int lastGreater1ScanPos = 0 , lastGreater1ScanPos_r = 0 ;
	int sign_data_hiding_enabled_flag = 0 ;
	int signHidden = 0 ;
	int cu_transquant_bypass_flag = 0 ;
	int transfrom_skip_flag_r = 0 ;
	int transform_skip_enabled_flag = 0 ;
	int last_sign_flag_hidden = 0 ;
	int ctxOffset,ctxShift = 0 ;
	int	cLastRiceParam[8] = { 0 };
	int cLastAbsLevel[8] = { 0 };
	int	cRiceParam[8] = { 0 };
	int cAbsLevel[8] = { 0 };
	int cLastRiceParam_r = 0 ;
	int cLastAbsLevel_r = 0 ;
//	int prefixVal[4] ;

//	scanorder_mode	  = 0;//DIAG,HOR,VER;
	find_last_sub_block = 0;
	cofgreater0_cnt_r = 0;
	cofgreater1_cnt_r = 0;
	subnum_icnt_r = (1 << ((log2TrafoSize - 2) * 2)) - 1;//4*4块的最大序号   
	cabac_cs = ST_TRANSFROM_SKIP_FLAG;
	ru_luma_flag = 0;
	ru_cb_flag = 0;
	ru_cr_flag = 0;
	lastsubblock_num = 0;
	if(log2TrafoSize == 2)//？  PU 都是等于TU的   这里是不是应该max_tu_cnr固定为1
		max_tu_cnr = 1;
	else
		max_tu_cnr = 1;


	cabac_cs = ST_TU_EST;
	cabac_ns = ST_TU_EST;

	bool TU_status_loop = 1;
	int ST_TU_EST_first = 1;
	while(TU_status_loop)
	{
		cabac_cs = cabac_ns;

		coded_sub_block_flag 				= 0;
		cofgreater2_flag_found 	= 0;
		coeff_abs_level_remaining_16bit_flag = 0;
		coeff_abs_level_remaining_8bit_flag = 0;
		coeff_abs_level_remaining_4bit_flag = 0;
		coeff_abs_level_remaining_2bit_flag = 0;
		cbf_cb_val = (log2TrafoSize == 2) ? cbf_cb_r[log2TrafoSize - 1] : cbf_cb_r[log2TrafoSize];//因为4*4TU不处理色度
		cbf_cr_val = (log2TrafoSize == 2) ? cbf_cr_r[log2TrafoSize - 1] : cbf_cr_r[log2TrafoSize];//因为4*4TU不处理色度
		valbin_ecs_last_sig_coeff_xy_prefix = 0 ;
		val_len_last_sig_coeff_xy_prefix = 0;

//wire  从TU中连线到以subnum_icnt_r为序号的4*4块中
#pragma  region
		for(int i = 15; i >= 0; i--)
		{
			coeff_value[i] = coeff[tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] / ( 1 << (log2TrafoSize - 2)) * 4 + tu_scan_table[scanorder_mode][2][i] / 4] //？tu_scan_table[log2TrafoSize - 2][subnum_icnt_r]  tu的扫描到光栅扫描  tu_scan_table[0][i]是否改成tu_scan_table[2][i]  coeff_value[i]按扫描序号存储
			[tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] % ( 1 << (log2TrafoSize - 2)) *4+ tu_scan_table[scanorder_mode][2][i] % 4];   //这里主要是把一个TU块最后一个4*4块的系数连线到coeff_value[i]  随着循环的进行，逐个遍历4*4块
			sig_coeff_flag[i] = (coeff_value[i] != 0);//表明该系数是否不为0 的标记  以扫描序号存储
			if(sig_coeff_flag[i])
				coded_sub_block_flag = 1;//表示只要有一个系数不为0   就要把该标记置1
			coeff_abs_level_greater1_flag[i] = (abs(coeff_value[i]) > 1);  //是否大于1的标记   注意到只有有系数值的倒数8个像素才需要编码该标记，后面应该会有处理
			coeff_sign_flag[i] = (coeff_value[i] < 0);//符号位  1表示负数  0表示正数
		}
#pragma  endregion

//wire 通过last_sig_coeff_x和y 求最后4*4块的光栅和扫描序号  	last_sig_coeff_x和y 只在右边条件下求得，也就是一个TU中一经求得不会再改变//if(cabac_ns == ST_LOOK_FOR_LAST_POS || (cabac_ns == ST_LAST_SIG_COEFF_X_PREFIX && log2TrafoSize == 2))
#pragma  region
		signHidden = ( (lastSigScanPos - firstSigScanPos > 3) && !cu_transquant_bypass_flag ) ;//?没细看
		lastsubblock_num 	= (last_sig_coeff_x >> 2) + (last_sig_coeff_y >> 2)* ( 1 << (log2TrafoSize - 2));//最后一个4*4块在TU中的光栅坐标
		lastcoeff_num			 	= (last_sig_coeff_y & 0x3) * 4 + (last_sig_coeff_x & 0x3);//最后一个系数在4*4块的光栅坐标

		lastsubblock_tilenum 	= scan_coeff_block[scanorder_mode][log2TrafoSize - 2][lastsubblock_num];//最后一个4*4块在TU中扫描序号  scan_coeff_block 是光栅转扫描
		lastcoeff_tilenum			= scan_coeff_block[scanorder_mode][2][lastcoeff_num];//最后一个系数在4*4块的扫描序号

		subnum_icnt_minus1 	= subnum_icnt_r - 1;//把减一的数据准备好
#pragma  endregion

//wire  获取一次处理最多4个 x或y 的前缀的bin和len 和上下文
#pragma  region
		//ctxinc calc0
		if (tu_cidx_luma_flag == 0)
		{
			ctxOffset = 3 * ( log2TrafoSize - 2 ) + ( ( log2TrafoSize - 1 ) >> 2 )  ;//用来计算 x y 坐标前缀上下文  ctxInc = ( binIdx >> ctxShift ) + ctxOffset
			ctxShift = ( log2TrafoSize + 1 ) >> 2;
		} 
		else
		{
			ctxOffset = 15;
			ctxShift = log2TrafoSize - 2;
		}
		
		if(log2TrafoSize == 2)
		{
			for(int i = 0; i < 3; i++)
			{
				last_sig_coeff_prefix_ctx_inc[i ] = i ;//对4*4 前缀值只有0 1 2 3
			}
			val_len_last_sig_coeff_xy_prefix = (last_sig_coeff_y_prefix_r > last_sig_coeff_x_prefix_r ) ? (last_sig_coeff_y_prefix_r == 3 ? 3 : last_sig_coeff_y_prefix_r+1) :
				(last_sig_coeff_x_prefix_r == 3 ? 3 : last_sig_coeff_x_prefix_r+1);//？  取x y 前缀比较大的，若相等则取x前缀     这里是不是想要计算长度，那应该有last_sig_coeff_y_prefix_len_r + 1 才对，否则判断的意义是什么。
			//0 0 ,00  1,1  2,01 3,10 4,11 5   //修改成0X 0 ,00  1,1  2,01 3,10 4,11 5  ，X0 6
			if(last_sig_coeff_x_prefix_r == 0 && last_sig_coeff_y_prefix_r == 0)
			{

				last_sig_coeff_xy_prefix_case[0] = 1;
			}
			else	if(last_sig_coeff_x_prefix_r == 1 && last_sig_coeff_y_prefix_r == 0)
			{

				last_sig_coeff_xy_prefix_case[0] = 4;
				last_sig_coeff_xy_prefix_case[1] = 0;
			}
			else	if(last_sig_coeff_x_prefix_r == 2 && last_sig_coeff_y_prefix_r == 0)
			{

				last_sig_coeff_xy_prefix_case[0] = 4;
				last_sig_coeff_xy_prefix_case[1] = 2;
				last_sig_coeff_xy_prefix_case[2] = 0;
			}
			else	if(last_sig_coeff_x_prefix_r == 3 && last_sig_coeff_y_prefix_r == 0)
			{

				last_sig_coeff_xy_prefix_case[0] = 4;
				last_sig_coeff_xy_prefix_case[1] = 2;
				last_sig_coeff_xy_prefix_case[2] = 2;
			}
			else	if(last_sig_coeff_x_prefix_r == 0 && last_sig_coeff_y_prefix_r == 1)
			{

				last_sig_coeff_xy_prefix_case[0] = 3;
				last_sig_coeff_xy_prefix_case[1] = 0;
			}
			else	if(last_sig_coeff_x_prefix_r == 1 && last_sig_coeff_y_prefix_r == 1)
			{

				last_sig_coeff_xy_prefix_case[0] = 5;
				last_sig_coeff_xy_prefix_case[1] = 1;
			}
			else	if(last_sig_coeff_x_prefix_r == 2 && last_sig_coeff_y_prefix_r == 1)
			{

				last_sig_coeff_xy_prefix_case[0] = 3;
				last_sig_coeff_xy_prefix_case[1] = 4;
				last_sig_coeff_xy_prefix_case[2] = 0;
			}
			else	if(last_sig_coeff_x_prefix_r == 3 && last_sig_coeff_y_prefix_r == 1)
			{

				last_sig_coeff_xy_prefix_case[0] = 3;
				last_sig_coeff_xy_prefix_case[1] = 4;
				last_sig_coeff_xy_prefix_case[2] = 2;
			}
			else	if(last_sig_coeff_x_prefix_r == 0 && last_sig_coeff_y_prefix_r == 2)
			{

				last_sig_coeff_xy_prefix_case[0] = 3;
				last_sig_coeff_xy_prefix_case[1] = 2;
				last_sig_coeff_xy_prefix_case[2] = 0;
			}
			else	if(last_sig_coeff_x_prefix_r == 1 && last_sig_coeff_y_prefix_r == 2)
			{

				last_sig_coeff_xy_prefix_case[0] = 5;
				last_sig_coeff_xy_prefix_case[1] = 3;
				last_sig_coeff_xy_prefix_case[2] = 0;
			}
			else	if(last_sig_coeff_x_prefix_r == 2 && last_sig_coeff_y_prefix_r == 2)
			{

				last_sig_coeff_xy_prefix_case[0] = 5;
				last_sig_coeff_xy_prefix_case[1] = 5;
				last_sig_coeff_xy_prefix_case[2] = 1;
			}
			else	if(last_sig_coeff_x_prefix_r == 3 && last_sig_coeff_y_prefix_r == 2)
			{

				last_sig_coeff_xy_prefix_case[0] = 5;
				last_sig_coeff_xy_prefix_case[1] = 5;
				last_sig_coeff_xy_prefix_case[2] = 4;

			}

			else	if(last_sig_coeff_x_prefix_r == 0 && last_sig_coeff_y_prefix_r == 3)
			{

				last_sig_coeff_xy_prefix_case[0] = 3;
				last_sig_coeff_xy_prefix_case[1] = 2;
				last_sig_coeff_xy_prefix_case[2] = 2;
			}
			else	if(last_sig_coeff_x_prefix_r == 1 && last_sig_coeff_y_prefix_r == 3)
			{

				last_sig_coeff_xy_prefix_case[0] = 5;
				last_sig_coeff_xy_prefix_case[1] = 3;
				last_sig_coeff_xy_prefix_case[2] = 2;
			}
			else	if(last_sig_coeff_x_prefix_r == 2 && last_sig_coeff_y_prefix_r == 3)
			{

				last_sig_coeff_xy_prefix_case[0] = 5;
				last_sig_coeff_xy_prefix_case[1] = 5;
				last_sig_coeff_xy_prefix_case[2] = 3;
			}
			else	if(last_sig_coeff_x_prefix_r == 3 && last_sig_coeff_y_prefix_r == 3)
			{

				last_sig_coeff_xy_prefix_case[0] = 5;
				last_sig_coeff_xy_prefix_case[1] = 5;
				last_sig_coeff_xy_prefix_case[2] = 5;
			}
		}
// 		else	if( last_sig_coeff_x_prefix_r + last_sig_coeff_y_prefix_r <= 2) 
// 		{
// 			for(i = 0; i < last_sig_coeff_x_prefix_r; i++)
// 			{
// 				last_sig_coeff_xy_prefix_ctx_inc[i] =  ( i >> ctxShift ) + ctxOffset ;
// 				valbin_ecs_last_sig_coeff_xy_prefix |= (1 << i);
// 			}
// 			if(i <= 3 )
// 				last_sig_coeff_xy_prefix_ctx_inc[i ] =  (i >> ctxShift ) + ctxOffset ;
// 			for(i = 0; i < last_sig_coeff_y_prefix_r; i++)
// 			{
// 				last_sig_coeff_xy_prefix_ctx_inc[i] =  ( i >> ctxShift ) + ctxOffset ;
// 				valbin_ecs_last_sig_coeff_xy_prefix |= ( 1  << ( i + last_sig_coeff_x_prefix_r+1));
// 			}
// 			if(i <= 3)
// 				last_sig_coeff_xy_prefix_ctx_inc[i ] =  (i >> ctxShift ) + ctxOffset ;
// 
// 			val_len_last_sig_coeff_xy_prefix = last_sig_coeff_x_prefix_r + last_sig_coeff_y_prefix_r + 2;
// 		}
		else
		{
			if((cabac_cs == ST_LOOK_FOR_LAST_POS && find_last_sub_block==1) || (cabac_cs == ST_LAST_SIG_COEFF_X_PREFIX && last_sig_coeff_x_prefix_cnt_idx_ff1_r + 4 < last_sig_coeff_x_prefix_len_r) )//？第一次进入ST_LAST_SIG_COEFF_X_PREFIX状态时  last_sig_coeff_x_prefix_cnt_idx_ff1_r标记并没有被赋值，也就是这里只有循环重复进入时才有起作用 //cabac_ns == ST_LAST_SIG_COEFF_X_PREFIX
			{
				for(int i = last_sig_coeff_x_prefix_cnt_idx_r; i < last_sig_coeff_x_prefix_len_r; i++)//？last_sig_coeff_x_prefix_cnt_idx_ff1_r  想体现的是最多9个bin中处理到第几个bin   last_sig_coeff_x_prefix_len_r是要理解成长度还是值
				{
					if(i >= last_sig_coeff_x_prefix_cnt_idx_r + 4)//这里用来保证一次处理4个bin
						break;
					if(last_sig_coeff_x_prefix_len_r - 1 == i)
					{
						last_sig_coeff_xy_prefix_ctx_inc[i - last_sig_coeff_x_prefix_cnt_idx_r] =  (i >> ctxShift ) + ctxOffset ;
						if ((last_sig_coeff_x_prefix_r == (log2TrafoSize<<1) - 1))
							valbin_ecs_last_sig_coeff_xy_prefix |= (1 << (i - last_sig_coeff_x_prefix_cnt_idx_r));
					}
					else
					{
						last_sig_coeff_xy_prefix_ctx_inc[i - last_sig_coeff_x_prefix_cnt_idx_r] =  ( i >> ctxShift ) + ctxOffset ;
						valbin_ecs_last_sig_coeff_xy_prefix |= (1 << (i - last_sig_coeff_x_prefix_cnt_idx_r));
					}
					val_len_last_sig_coeff_xy_prefix=(last_sig_coeff_x_prefix_len_r-last_sig_coeff_x_prefix_cnt_idx_r)>4 ? 4 :last_sig_coeff_x_prefix_len_r-last_sig_coeff_x_prefix_cnt_idx_r;
				}
			}
			else	if(last_sig_coeff_x_prefix_cnt_idx_ff1_r + 4 >= last_sig_coeff_x_prefix_len_r)//cabac_ns == ST_LAST_SIG_COEFF_Y_PREFIX
			{
				for(int i = last_sig_coeff_y_prefix_cnt_idx_r; i < last_sig_coeff_y_prefix_len_r; i++)
				{
					if(i >= last_sig_coeff_y_prefix_cnt_idx_r + 4)
						break;
					if(last_sig_coeff_y_prefix_len_r - 1 == i)
					{
						last_sig_coeff_xy_prefix_ctx_inc[i - last_sig_coeff_y_prefix_cnt_idx_r ] =  ( i  >> ctxShift ) + ctxOffset ;
						if ((last_sig_coeff_y_prefix_r == (log2TrafoSize<<1) - 1))
							valbin_ecs_last_sig_coeff_xy_prefix |= (1 << (i - last_sig_coeff_y_prefix_cnt_idx_r));
					}
					else
					{
						last_sig_coeff_xy_prefix_ctx_inc[i  - last_sig_coeff_y_prefix_cnt_idx_r ] =  ( i>> ctxShift ) + ctxOffset ;
						valbin_ecs_last_sig_coeff_xy_prefix |= (1 << (i - last_sig_coeff_y_prefix_cnt_idx_r));
					}
					val_len_last_sig_coeff_xy_prefix= (last_sig_coeff_y_prefix_len_r-last_sig_coeff_y_prefix_cnt_idx_r)>4 ? 4 :last_sig_coeff_y_prefix_len_r-last_sig_coeff_y_prefix_cnt_idx_r;
				}
			}
		}
#pragma  endregion

//wire 因为对sig标记的处理是每次4个来处理，这里主要是把还未处理的当前系数及它之前的三个系数的坐标 还有 大于0 1之类的计算提前算好
#pragma  region
		for(int i = 0; i < 4;i++)
		{
			sig_coeff_flag_r_cnt_idx_minus[i] 	= (sig_coeff_flag_r_cnt_idx_r - i)>=0 ? sig_coeff_flag_r_cnt_idx_r - i : 0 ;//sig_coeff_flag_r_cnt_idx_r 最后一个待处理系数在4*4块中的扫描序号  在ns 为 ST_LAST_SIG_COEFF_X_PREFIX 时确定
			cofgreater0_cnt_plus[i]				=	cofgreater0_cnt_r + i;//cofgreater0_cnt_r每次累加统计的一次处理的4个系数中大于0的个数  最后就是总的个数
			cofgreater1_cnt_plus[i]				= cofgreater1_cnt_r + i;//cofgreater1_cnt_r每次累加统计的一次处理的4个系数中大于1的个数  最后就是总的个数
			
			cofgreater0_cnt_idx_minus[i]		= (cofgreater0_cnt_idx_r - i)>=0 ? cofgreater0_cnt_idx_r - i : 0 ;//cofgreater0_cnt_idx_r  4*4块中大于0的未处理系数个数减去i
			
			ru_scanraster[i]							= tu_scan_table[scanorder_mode][2][sig_coeff_flag_r_cnt_idx_minus[i]];//最后一个待处理系数倒数i个在4*4块中的光栅扫描序号
			cof_sigctx_size4[i]						= ctxIdxMap[ru_scanraster[i]];//sig_coeff_flag_r_cnt_idx_minus这里只是4*4的序号，应该换成TU的序号吗  这里只会用在TU为4*4的case下，所以是一样的
			codedsub_xp[i]							=	ru_scanraster[i] % 4;//最后一个待处理系数倒数i个在4*4块中的x坐标
			codedsub_yp[i]							=	ru_scanraster[i] / 4;//最后一个待处理系数倒数i个在4*4块中的y坐标

			coeff_abs_level_remaining_cnt_idx_minus[i] = coeff_abs_level_remaining_cnt_idx_r - i;//？含义不明白
		}
#pragma  endregion

//wire 计算出一次处理的4个或更少的sig的bin和长度
#pragma  region
		if(sig_coeff_flag_r_cnt_idx_r >3 )//该条件表示一次最多处理4个sig_coeff_flag_r[i]
		{
			valbin_ecs_sig_coeff_flag = (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[3]] << 3) | (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[2]] << 2) | (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[1]] << 1) | (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[0]] );//最多连续4个sig_coeff_flag组成的bin
			val_len_sig_coeff_flag = 4;//在一次最多处理4个的过程中   sig_coeff_flag 的个数
		}
		else
		{
			if(sig_coeff_flag_r_cnt_idx_r == 3 )
				valbin_ecs_sig_coeff_flag = (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[3]] << 3) | (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[2]] << 2) | (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[1]] << 1) | (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[0]] );
			else	if(sig_coeff_flag_r_cnt_idx_r == 2 )
				valbin_ecs_sig_coeff_flag = (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[2]] << 2) | (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[1]] << 1) | (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[0]] << 0);
			else 	if(sig_coeff_flag_r_cnt_idx_r == 1 )
				valbin_ecs_sig_coeff_flag = (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[1]] << 1) | (sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[0]] << 0);
			else
				valbin_ecs_sig_coeff_flag = sig_coeff_flag_r[sig_coeff_flag_r_cnt_idx_minus[0]] << 0 ;

			val_len_sig_coeff_flag	=	sig_coeff_flag_r_cnt_idx_r + !inferSbDcSigCoeffFlag_r;//若inferSbDcSigCoeffFlag_r为1表示可被推测，那么第0位就不用被编码
		}
#pragma  endregion

#pragma  region
		if(sig_coeff_flag_r_cnt_idx_r >3 )//该条件表示一次最多处理4个sig_coeff_flag_r[i]
		{
			valbin_ecs_greater1_for_cnt = (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[3]] << 3) | (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[2]] << 2) | (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[1]] << 1) | (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[0]] );//最多连续4个greate1组成的bin
		}
		else
		{
			if(sig_coeff_flag_r_cnt_idx_r == 3 )
				valbin_ecs_greater1_for_cnt = (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[3]] << 3) | (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[2]] << 2) | (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[1]] << 1) | (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[0]] );
			else	if(sig_coeff_flag_r_cnt_idx_r == 2 )
				valbin_ecs_greater1_for_cnt = (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[2]] << 2) | (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[1]] << 1) | (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[0]] << 0);
			else 	if(sig_coeff_flag_r_cnt_idx_r == 1 )
				valbin_ecs_greater1_for_cnt = (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[1]] << 1) | (coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[0]] << 0);
			else
				valbin_ecs_greater1_for_cnt = coeff_abs_level_greater1_flag_r[sig_coeff_flag_r_cnt_idx_minus[0]] << 0 ;
		}
#pragma  endregion



		valbin_ecs_codec_block_flag = coded_sub_block_flag;//把标记转换成bin  虽然其实是一样的

		val_len_coded_sub_block_flag 		= 1;//该标记长度固定为1


		Greater1Flag_num_avail						= MIN(8,cofgreater0_cnt_r) -numGreater1Flag_solved_r;/*- numGreater1Flag_solved_r*/;//表示还有多少个剩下的 是否大于1 这个标记要处理  最多处理8个


//wire 对倒数至多8个大于0的系数操作   每次最多处理4个，得出这4个系数是否大于1的bin和有效长度   同时计数了这至多4个系数中大于1的系数个数Greater1Flag_equal_1_num
#pragma  region
		if( (cabac_cs == ST_SIG_COEFF_FLAG) && (sig_coeff_flag_r_cnt_idx_ff1_r < 4 && !(subnum_icnt_r==0 && lastsubblock_num>0 && !sig_coeff_flag_blk0_zero_r)) || cabac_ns == ST_COEFF_ABS_LEVEL_GREATER1_FLAG)// 原始条件  cabac_ns == ST_COEFF_ABS_LEVEL_GREATER1_FLAG)
		{
			for(int i = 0; i < 4; i++)
			{
				Greater1Flag[i] =	coeff_abs_level_greater1_flag_r[cofgreater0_flag_pos_r[i + numGreater1Flag_solved_r]];
//				Greater1Flag[i] =	coeff_abs_level_greater1_flag_r[cofgreater1_flag_pos_r[cofgreater1_cnt_r - cofgreater1_cnt_idx_minus[i]]];
			}

			if(cofgreater0_cnt_idx_r >= 3)
			{
				valbin_ecs_coeff_abs_level_greater1_flag = (Greater1Flag[3] << 3) | (Greater1Flag[2] << 2) | (Greater1Flag[1] << 1) | Greater1Flag[0];//一次处理4个标记，4个是否大于1的标记组成的bin
				val_len_coeff_abs_level_greater1_flag = 4;
			}
			else if(cofgreater0_cnt_idx_r == 2)
			{
				valbin_ecs_coeff_abs_level_greater1_flag = (Greater1Flag[2] << 2) | (Greater1Flag[1] << 1) | (Greater1Flag[0] << 0);
				val_len_coeff_abs_level_greater1_flag = 3;
			}
			else if(cofgreater0_cnt_idx_r == 1)
			{
				valbin_ecs_coeff_abs_level_greater1_flag = (Greater1Flag[1] << 1) | (Greater1Flag[0] << 0);
				val_len_coeff_abs_level_greater1_flag = 2;
			}
			else if(cofgreater0_cnt_idx_r == 0)
			{
				valbin_ecs_coeff_abs_level_greater1_flag = Greater1Flag[0] << 0;
				val_len_coeff_abs_level_greater1_flag = cofgreater0_cnt_r == 0 ? 0 : 1;
			}
			else //if(cofgreater0_cnt_idx_r == 0)
			{
				assert(0);//valbin_ecs_coeff_abs_level_greater1_flag = 0;
			}
		}
#pragma  endregion

//wire  求coeff_abs_level_remaining_cnt_idx_r 的bin和长度  应该是和enc_out有关
#pragma  region
		if(coeff_abs_level_remaining_cnt_idx_r > 3)
		{
			valbin_ecs_coeff_abs_level_remaining = 0;
			for(int i = 0; i < 4; i++)
			{
				if( coeff_abs_level_remaining_r[coeff_abs_level_remaining_cnt_r - 1 - coeff_abs_level_remaining_cnt_idx_minus[i]] > 0)//coeff_abs_level_remaining_r[]中残余系数值的存储不是按4*4的排布来的，是倒数顺序的
					valbin_ecs_coeff_abs_level_remaining = coeff_abs_level_remaining_r[coeff_abs_level_remaining_cnt_r - coeff_abs_level_remaining_cnt_idx_minus[i]] << (i * coeff_len_r);
				else	
					valbin_ecs_coeff_abs_level_remaining = (-coeff_abs_level_remaining_r[coeff_abs_level_remaining_cnt_r  - 1 - coeff_abs_level_remaining_cnt_idx_minus[i]] )<< (i * coeff_len_r);
			}
			val_len_coeff_abs_level_remaining = 4 * coeff_len_r;
		}
		else
		{
			for(int i = 0; i <= coeff_abs_level_remaining_cnt_idx_r; i++)
			{
				if( coeff_abs_level_remaining_r[coeff_abs_level_remaining_cnt_r - coeff_abs_level_remaining_cnt_idx_minus[i]] > 0)
					valbin_ecs_coeff_abs_level_remaining = coeff_abs_level_remaining_r[
					coeff_abs_level_remaining_cnt_r - coeff_abs_level_remaining_cnt_idx_minus[i]] << (i * coeff_len_r);
				else	
					valbin_ecs_coeff_abs_level_remaining = (-coeff_abs_level_remaining_r[
					coeff_abs_level_remaining_cnt_r - coeff_abs_level_remaining_cnt_idx_minus[i]] )<< (i * coeff_len_r);
			}
			val_len_coeff_abs_level_remaining = (coeff_abs_level_remaining_cnt_idx_r + 1) * coeff_len_r;
		}
#pragma  endregion

//wire  主要都是处理上下文
#pragma  region
		//ctxinc calc0  //对coeff_abs_level_remaining   cRiceParam的求解
		if(coeff_abs_level_remaining_cnt_idx_r == coeff_abs_level_remaining_cnt_r - 1)
		{
			cLastRiceParam[0] = 0;
			cLastAbsLevel[0]	= 0;
		}
		else
		{
			cLastRiceParam[0] = cLastRiceParam_r;
			cLastAbsLevel[0]	= cLastAbsLevel_r;
		}

		if (coeff_abs_level_remaining_cnt_r > 0)
		{
			for(int i = 0; i <= (coeff_abs_level_remaining_cnt_idx_r > 7 ? 7 : coeff_abs_level_remaining_cnt_idx_r); i++)
			{
				if(i > 0)
				{
					cLastAbsLevel[i]	= cAbsLevel[i - 1];
					cLastRiceParam[i] = cRiceParam[i - 1];
				}
				cAbsLevel[i] = coeff_value_r[coeff_abs_level_remaining_pos_r[ i + coeff_abs_level_remaining_cnt_r - 1 - coeff_abs_level_remaining_cnt_idx_r ]];
				cRiceParam[i] = MIN( cLastRiceParam[i] + ( cLastAbsLevel[i] > ( 3 * ( 1 << cLastRiceParam[i] ) ) ? 1 : 0 ), 4 ) ;
			}
		}
		//对coeff_abs_level_remaining  cRiceParam  结束


		ctxSet										= 0;

		

		//对 COEFF_ABS_LEVEL_GREATER1_FLAG上下文的处理
		if(cabac_cs == ST_SIG_COEFF_FLAG && (sig_coeff_flag_r_cnt_idx_ff1_r < 4 && !(subnum_icnt_r==0 && lastsubblock_num>0 && !sig_coeff_flag_blk0_zero_r)))//注意这里是与 //原始条件 cabac_cs == ST_SIG_COEFF_FLAG && cabac_ns == ST_COEFF_ABS_LEVEL_GREATER1_FLAG
		{
			greater1Ctx[0] = 1;
			greater1Ctx[1] = greater1Ctx[0]>0 ? (Greater1Flag[0] ? 0 : MIN(greater1Ctx[0]+1 ,3)) : 0;
			greater1Ctx[2] = greater1Ctx[1]>0 ? (Greater1Flag[1] ? 0 : MIN(greater1Ctx[1]+1 ,3)) : 0;
			greater1Ctx[3] = greater1Ctx[2]>0 ? (Greater1Flag[2] ? 0 : MIN(greater1Ctx[2]+1 ,3)) : 0;

			if(subnum_icnt_r == lastsubblock_tilenum)
				lastGreater1Ctx = 1;
			else
				lastGreater1Ctx = (lastGreater1Ctx_r == 0) ?  0 :  (lastGreater1Flag_r == 1) ? 0 : MIN(lastGreater1Ctx_r + 1, 3);
			
			if(subnum_icnt_r == 0 ||  tu_cidx_luma_flag > 0)
			{
				ctxSet = (lastGreater1Ctx == 0);
			}
			else	if(subnum_icnt_r > 0 &&  tu_cidx_luma_flag == 0)
			{
				ctxSet = 2 + (lastGreater1Ctx == 0);
			}
			else
				assert(0);//won't happen
			
//			lastGreater1Ctx = lastGreater1Ctx_r;//作用是什么
		}
		else
		{
			greater1Ctx[0] =  (lastGreater1Ctx_r > 0) ? (lastGreater1Flag_r ? 0 : MIN(lastGreater1Ctx_r+1 ,3)) : 0; //?感觉有问题
			greater1Ctx[1] = greater1Ctx[0]>0 ? (Greater1Flag[0] ? 0 : MIN(greater1Ctx[0]+1 ,3)) : 0;
			greater1Ctx[2] = greater1Ctx[1]>0 ? (Greater1Flag[1] ? 0 : MIN(greater1Ctx[1]+1 ,3)) : 0;
			greater1Ctx[3] = greater1Ctx[2]>0 ? (Greater1Flag[2] ? 0 : MIN(greater1Ctx[2]+1 ,3)) : 0;
			ctxSet = ctxSet_r;
		}

		if (tu_cidx_luma_flag == 0)
		{
			for(int i = 0; i < 4; i++)
				cofgreater1_ctxinc[i] = ctxSet * 4 + greater1Ctx[i];
		} 
		else
		{
			for(int i = 0; i < 4; i++)
				cofgreater1_ctxinc[i] = ctxSet * 4 + greater1Ctx[i] + 16;
		}

		if(tu_cidx_luma_flag == 0)
			cofgreater2_ctxinc = ctxSet;
		else
			cofgreater2_ctxinc = ctxSet + 4;
		
		//对 COEFF_ABS_LEVEL_GREATER1_FLAG 和GREATER2上下文的处理 结束

		//对sig上下文的处理
		tu_scanraster		= tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r];//当前4*4块的光栅扫描坐标   codedsub_xs之类的都是线，每次要用的时候从subnum_icnt_r之类的reg推导出来
		codedsub_xs 	= tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] % ( 1 << (log2TrafoSize - 2));//当前4*4块的x坐标
		codedsub_ys 	= tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] / ( 1 << (log2TrafoSize - 2));//当前最后一个有系数4*4块的y坐标

		csbfCtx = 0;
		prevcsbf = 0;
		if(codedsub_xs < ( 1 << (log2TrafoSize - 2)) - 1)
		{
			prevcsbf 	+= coded_sub_block_flag_r[(codedsub_xs + 1) + codedsub_ys * ( 1 << (log2TrafoSize - 2))] ;
			csbfCtx 	+= coded_sub_block_flag_r[(codedsub_xs + 1) + codedsub_ys * ( 1 << (log2TrafoSize - 2))] ;
		}


		if(codedsub_ys < ( 1 << (log2TrafoSize - 2)) - 1)
		{
			prevcsbf += (coded_sub_block_flag_r[codedsub_xs + (codedsub_ys + 1) * ( 1 << (log2TrafoSize - 2))] << 1);
			csbfCtx 	+= coded_sub_block_flag_r[codedsub_xs + (codedsub_ys + 1) * ( 1 << (log2TrafoSize - 2))] ;
		}

		switch(prevcsbf)
		{
		case 0:
			for(int i = 0; i < 4; i++)
				sigctx[i] = (ru_scanraster[i]==0)? 2 : (((codedsub_xp[i]+codedsub_yp[i])<3)? 1 : 0);
			break;
		case 1:
			for(int i = 0; i < 4; i++)
				sigctx[i] = (codedsub_yp[i]==0)?  2 : ((codedsub_yp[i]==1)? 1 : 0);
			break;

		case 2:
			for(int i = 0; i < 4; i++)
				sigctx[i] = (codedsub_xp[i]==0)?  2 : ((codedsub_xp[i] == 1)? 1 : 0);
			break;

		case 3:
			for(int i = 0; i < 4; i++)
				sigctx[i] = 2;
			break;

		default:
			break;
		}

		for(int i = 0; i < 4; i++)
		{
			if(log2TrafoSize==2) //(size=4x4)          c
			{
				cof_sigctx_luma[i] = cof_sigctx_size4[i];
			}
			else if((tu_scanraster_r==0) && (ru_scanraster[i]==0))
			{
				cof_sigctx_luma[i] = 0;
			}
			else	
			{
// 				if((codedsub_ys > 0) || (codedsub_xs > 0))
// 					cof_sigctx_luma[i] = sigctx[i] + 3;

				if(log2TrafoSize==3)
				{
					if(scanorder_mode==0)
						cof_sigctx_luma[i] = sigctx[i] + 9 +( ((codedsub_ys > 0) || (codedsub_xs > 0)) ? 3 : 0 );
					else	if(scanorder_mode != 0)
						cof_sigctx_luma[i] = sigctx[i] + 15 +( ((codedsub_ys > 0) || (codedsub_xs > 0)) ? 3 : 0 );
				}
				else
					cof_sigctx_luma[i] = sigctx[i] + 21 +( ((codedsub_ys > 0) || (codedsub_xs > 0)) ? 3 : 0 );
			}

			if(log2TrafoSize==2) //(size=4x4)          c
			{
				cof_sigctx_chroma[i] = cof_sigctx_size4[i];
			}
			else if((tu_scanraster_r==0) && (ru_scanraster[i]==0))
			{
				cof_sigctx_chroma[i] = 0;
			}
			else 
			{
				if(log2TrafoSize==3)
				{
					cof_sigctx_chroma[i] = sigctx[i] + 9;
				}
				else
				{
					cof_sigctx_chroma[i] = sigctx[i] + 12;
				}
			}
			cofsig_ctxinc[i] = tu_cidx_luma_flag == 0 ? cof_sigctx_luma[i] : (cof_sigctx_chroma[i] + 27);
		}

		codedsub_ctxinc = tu_cidx_luma_flag == 0 ? MIN(csbfCtx,1) : (2 + MIN(csbfCtx,1));
#pragma  endregion

// 状态跳转
#pragma  region
		switch(cabac_cs)//do enc in this switch   //？没看到对cabac_cs的赋值操作  如cabac_cs = cabac_ns
		{
		case 	CTU_EST_END:
			break;
		case		ST_TU_EST_WAIT:
			if(!Quant_finish_flag[trafodepth_r][tu_loop_cnt_r]==1)
				cabac_ns = cabac_cs;
			else
				cabac_ns = ST_TU_EST;
			break;
		case 	ST_TU_EST:
			{
				if (ST_TU_EST_first==0)
				{
					TU_status_loop = 0;
				}
				if (ST_TU_EST_first == 1)
				{
					ST_TU_EST_first = 0;
				}
				

				int T1 = ((log2TrafoSize>2 || (log2TrafoSize==2&&blkidx_r==3)) && cbf_cb_val);//？blkidx_r是什么意思
				int T2 = (log2TrafoSize>2 || (log2TrafoSize==2&&blkidx_r==3)) && cbf_cr_val;
				if((!ru_luma_flag && cbf_luma_r ) || ( (!ru_cb_flag && T1) ||	(!ru_cr_flag  && T2 && ( ru_cb_flag || (ru_luma_flag&&!(log2TrafoSize>2 ||T1 ))))))//？非luma的情况的条件
					if(log2TrafoSize == 2)
						cabac_ns = ST_LAST_SIG_COEFF_X_PREFIX;
					else
						cabac_ns = ST_LOOK_FOR_LAST_POS;
				else //next TU loop start:
					if(tu_loop_cnt_r!=max_tu_cnr - 1)
						cabac_ns = ST_TU_EST_WAIT;
					else
						cabac_ns = CTU_EST_END;
				break;
			}
		case	ST_LOOK_FOR_LAST_POS:
			if(find_last_sub_block)
				cabac_ns = ST_LAST_SIG_COEFF_X_PREFIX;//ST_LAST_SIG_COEFF_Y_PREFIX;
			else
				cabac_ns = cabac_cs;
			break;
		case	ST_LAST_SIG_COEFF_X_PREFIX: //p9.3.4.2.3, 4bit(TR: (cMax=(log2TrafoSize[2:0]<<1)-1), max 9bin-decision)
			if((log2TrafoSize == 2/* || last_sig_coeff_x + last_sig_coeff_y <= 2*/))
				cabac_ns = ST_SIG_COEFF_FLAG;//ST_CODED_SUB_BLOCK_FLAG;//ST_LAST_SIG_COEFF_Y_PREFIX;
			else	if(last_sig_coeff_x_prefix_cnt_idx_ff1_r + 4 < last_sig_coeff_x_prefix_len_r)//？为什么这么判断  比如last_sig_coeff_x_prefix_len_r为5的话 last_sig_coeff_x_prefix_cnt_idx_r这时为4
				cabac_ns = cabac_cs;
			else
				cabac_ns = ST_LAST_SIG_COEFF_Y_PREFIX;

			if(cabac_ns!=ST_LAST_SIG_COEFF_X_PREFIX)//把后缀估算了
			{
//				write_to_fifo((last_sig_coeff_y << 6) | last_sig_coeff_x,12);//？为什么这里又定义成12位   之前ru_subblock_parameter_r这个参数集不使用吗
				est_bypass(last_sig_coeff_x_suffix_len_r , 0);
				est_bypass(last_sig_coeff_y_suffix_len_r , 1);
			}

			if(log2TrafoSize == 2)
			{
				est_bin1(last_sig_coeff_x_prefix_r , last_sig_coeff_y_prefix_r , LAST_SIGNIFICANT_COEFF_X_PREFIX_OFFSET+ctxOffset  , LAST_SIGNIFICANT_COEFF_Y_PREFIX_OFFSET+ctxOffset);//?last_sig_coeff_xy_prefix_ctx_inc_ff1 实际上没有被赋值过    在这里，实际上只需要last_sig_coeff_xy_prefix_case就可以唯一确定bin和ctx
			} 
			else
			{
				est_bins(valbin_r , val_len_r , valbin_ctxInc_r , LAST_SIGNIFICANT_COEFF_X_PREFIX_OFFSET);//?要再加入上下文
			}

			break;
		case ST_LAST_SIG_COEFF_Y_PREFIX: //p9.3.4.2.3, 4bit(TR: (cMax=(log2TrafoSize[2:0]<<1)-1), max 9bin-decision)
			if(last_sig_coeff_y_prefix_cnt_idx_ff1_r + 4 < last_sig_coeff_y_prefix_len_r)
				cabac_ns = ST_LAST_SIG_COEFF_Y_PREFIX;
			else
				cabac_ns = ST_SIG_COEFF_FLAG; //initial subnum_icnt_r=lastsubblock_num[6:0], coeff_num_r=0, cof_total=lastScanPos-1
			est_bins(valbin_r, val_len_r,valbin_ctxInc_r , LAST_SIGNIFICANT_COEFF_Y_PREFIX_OFFSET);//?要加入上下文 这里估算的是y的前缀 
			break;
		case	ST_CODED_SUB_BLOCK_FLAG: //p9.3.4.2.4, 1bit(FL,1bin-decision),ctxInc = Min(csbfCtx, 1)
			if(coded_sub_block_flag_out == 1 || subnum_icnt_r==0)
				cabac_ns = ST_SIG_COEFF_FLAG; //subnum_icnt_r--, coeff_num_r=0;
			else
				cabac_ns = ST_CODED_SUB_BLOCK_FLAG; //subnum_icnt_r--, coeff_num_r=0;

			if (subnum_icnt_r != 0)
			{
				est_bins(valbin_r, val_len_r,valbin_ctxInc_r , SIGNIFICANT_COEFF_GROUP_FLAG_OFFSET);
			}
//			printf("%d ,\n" ,est_bit_coded_sub_block_flag[IsIntra_tu][depth_tu]);
//			write_to_fifo(coded_sub_block_flag_out,1);
			break;
		case	ST_SIG_COEFF_FLAG: //p9.3.4.2.5, 1bit(FL,1bin-decision)x16
			//initial cof_total=(lastScanPos-1) or 15; subnum_icnt_r[6:0]=lastsubblock_num[6:0] just the one not from ST_CODED_SUB_BLOCK_FLAG
			if(sig_coeff_flag_r_cnt_idx_ff1_r < 4 )//上一次残留的sig数如果小于4，那么进入该case时就会做完，那不会再自转
				if(subnum_icnt_r==0 && lastsubblock_num>0 && !sig_coeff_flag_blk0_zero_r) //note4  lastsubblock_num是最后有值4*4块的光栅扫描  subnum_icnt_r是当前处理到的4*4块的扫描序号  sig_coeff_flag_blk0_zero_r表示当前4*4块是否有系数
					if(log2TrafoSize>2 || (log2TrafoSize==2 && blkidx_r==3)) //return
						cabac_ns = ST_TU_EST;
					else
						cabac_ns = ST_TU_EST_WAIT;
				else
					cabac_ns = ST_COEFF_ABS_LEVEL_GREATER1_FLAG;
			else
				cabac_ns = cabac_cs;
			est_bins(valbin_r, val_len_r,valbin_ctxInc_r , SIGNIFICANT_COEFF_FLAG_OFFSET);//？ 没有上下文
			
			if (cabac_ns != ST_SIG_COEFF_FLAG)
			{
				est_bypass(cofgreater0_cnt_r , 2);//sign_flag //编码符号位，符号位是bypass  有多少个大于0的系数就有多少个符号位
			}
			if(cabac_ns != ST_SIG_COEFF_FLAG)
			{
// 				for(i = 15; i >= 0; i++)
// 					write_to_fifo(sig_coeff_flag_enc_out_r[i],1);
// 				for(i = 15; i >= 0; i++)
// 					write_to_fifo(coeff_abs_level_greater1_flag_r[i],1);
// 				write_to_fifo(coeff_abs_level_greater2_flag_r,1);	
// 				write_to_fifo(coeff_len_r, 4);
			}
			break;
		case	ST_COEFF_ABS_LEVEL_GREATER1_FLAG: //p9.3.4.2.6, 1bit(FL,1bin-decision)x16
			if(Greater1Flag_num_avail != 0)
				cabac_ns = cabac_cs;//cnt1_r++; numGreater1Flag_solved_r<8? numGreater1Flag_solved_r++
			else if (Greater1Flag_num_avail == 0)
				if(coeff_abs_level_remaining_cnt_ff1_r > 8) //////lastGreater1ScanPos range 0~15
					cabac_ns = ST_COEFF_ABS_LEVEL_REMAINING;
				else	if((coeff_abs_level_remaining_cnt_ff1_r <= 8) && subnum_icnt_r==0) /// sig_coeff_flag_r valid num
					if(log2TrafoSize>2 || (log2TrafoSize==2 && blkidx_r==3))
						cabac_ns = ST_TU_EST;
					else
						cabac_ns = ST_TU_EST_WAIT;
				else
					cabac_ns = ST_CODED_SUB_BLOCK_FLAG;

//			if(coeff_abs_level_remaining_cnt_r == coeff_abs_level_remaining_cnt_ff1_r + 1)
//				write_to_fifo(coeff_abs_level_remaining_cnt_r, 4);
			est_bins(valbin_r, val_len_r,valbin_ctxInc_r , COEFF_ABS_LEVEL_GREATER1_FLAG_OFFSET);
//			printf("%d ,\n" ,est_bit_coeff_abs_level_greater1_flag[IsIntra_tu][depth_tu]);

			if(cabac_ns != ST_COEFF_ABS_LEVEL_GREATER1_FLAG )
			{
				est_bins(valbin_ecs_coeff_abs_level_greater2_flag_r, val_len_coeff_abs_level_greater2_flag_r,cofgreater2_ctxinc_r , COEFF_ABS_LEVEL_GREATER2_FLAG_OFFSET);
				
			}
			if (first_to_greater1_r!=1 && cabac_ns != ST_COEFF_ABS_LEVEL_GREATER1_FLAG)
			{
				for(int i = 0; i < (coeff_abs_level_remaining_cnt_ff1_r >= 8 ? 8 : coeff_abs_level_remaining_cnt_ff1_r); i++)
				{
					est_bypass(coeff_abs_level_remaining_len_r[i] , 3);
//					printf("%d ,\n" ,est_bit_coeff_abs_level_remaining[IsIntra_tu][depth_tu]);
				}
			}
			//			write_to_fifo(valbin_ecs, val_len_r);
			break;
		case	ST_COEFF_ABS_LEVEL_REMAINING: // bits???(prefix: TR(cMax=x),xbin-decision; suffix: EG, xbin-bypass;)
			if (coeff_abs_level_remaining_cnt_ff1_r > 8)
				cabac_ns = ST_COEFF_ABS_LEVEL_REMAINING;
			else
			{
				if(subnum_icnt_r==0) /// sig_coeff_flag_r valid num
					if(log2TrafoSize>2 || (log2TrafoSize==2 && blkidx_r==3))
						cabac_ns = ST_TU_EST;
					else
						cabac_ns = ST_TU_EST_WAIT;
				else
					cabac_ns = ST_CODED_SUB_BLOCK_FLAG;
			}
			

// 			if(cabac_ns != ST_COEFF_ABS_LEVEL_REMAINING )
// 			{
// 				est_bins(valbin_r, val_len_r,valbin_ctxInc_r , COEFF_ABS_LEVEL_GREATER2_FLAG_OFFSET);
// 			}

			for(int  i = 0; i < (coeff_abs_level_remaining_cnt_ff1_r >= 8 ? 8 : coeff_abs_level_remaining_cnt_ff1_r); i++)
			{
				est_bypass(coeff_abs_level_remaining_len_r[i] , 3);
//				printf("%d ,\n" ,est_bit_coeff_abs_level_remaining[IsIntra_tu][depth_tu]);
			}
			
//			write_to_fifo(valbin_ecs, coeff_len_r);

			break;
		default:
			cabac_ns = cabac_cs;
			break;
		}
#pragma  endregion

		if (cabac_ns == ST_CODED_SUB_BLOCK_FLAG)
		{
			/*			if(coded_sub_block_flag_out == 0 )*/
			if (subnum_icnt_r > 0)
				subnum_icnt_r -= 1;
		}

//RTL不需要加的一段代码，wire多跑一遍，主要解决subnum_icnt_r的问题
#pragma  region
		if (cabac_ns == ST_CODED_SUB_BLOCK_FLAG)
		{
			csbfCtx = 0 ;
			prevcsbf = 0;
			tu_scanraster		= tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r];//当前4*4块的光栅扫描坐标   codedsub_xs之类的都是线，每次要用的时候从subnum_icnt_r之类的reg推导出来
			codedsub_xs 	= tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] % ( 1 << (log2TrafoSize - 2));//当前4*4块的x坐标
			codedsub_ys 	= tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] / ( 1 << (log2TrafoSize - 2));//当前最后一个有系数4*4块的y坐标
			if(codedsub_xs < ( 1 << (log2TrafoSize - 2)) - 1)
			{
				prevcsbf 	+= coded_sub_block_flag_r[(codedsub_xs + 1) + codedsub_ys * ( 1 << (log2TrafoSize - 2))] ;
				csbfCtx 	+= coded_sub_block_flag_r[(codedsub_xs + 1) + codedsub_ys * ( 1 << (log2TrafoSize - 2))] ;
			}


			if(codedsub_ys < ( 1 << (log2TrafoSize - 2)) - 1)
			{
				prevcsbf += (coded_sub_block_flag_r[codedsub_xs + (codedsub_ys + 1) * ( 1 << (log2TrafoSize - 2))] << 1);
				csbfCtx 	+= coded_sub_block_flag_r[codedsub_xs + (codedsub_ys + 1) * ( 1 << (log2TrafoSize - 2))] ;
			}
			codedsub_ctxinc = tu_cidx_luma_flag == 0 ? MIN(csbfCtx,1) : (2 + MIN(csbfCtx,1));

			coded_sub_block_flag = 0;
			for(int i = 15; i >= 0; i--)
			{
				coeff_value[i] = coeff[tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] / ( 1 << (log2TrafoSize - 2)) * 4 + tu_scan_table[scanorder_mode][2][i] / 4] //？tu_scan_table[log2TrafoSize - 2][subnum_icnt_r]  tu的扫描到光栅扫描  tu_scan_table[0][i]是否改成tu_scan_table[2][i]  coeff_value[i]按扫描序号存储
				[tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] % ( 1 << (log2TrafoSize - 2)) *4+ tu_scan_table[scanorder_mode][2][i] % 4];   //这里主要是把一个TU块最后一个4*4块的系数连线到coeff_value[i]  随着循环的进行，逐个遍历4*4块
				sig_coeff_flag[i] = (coeff_value[i] != 0);//表明该系数是否不为0 的标记  以扫描序号存储
				if(sig_coeff_flag[i])
					coded_sub_block_flag = 1;//表示只要有一个系数不为0   就要把该标记置1
				coeff_abs_level_greater1_flag[i] = (abs(coeff_value[i]) > 1);  //是否大于1的标记   注意到只有有系数值的倒数8个像素才需要编码该标记，后面应该会有处理
				coeff_sign_flag[i] = (coeff_value[i] < 0);//符号位  1表示负数  0表示正数
			}
		}
#pragma  endregion


		if(ST_LOOK_FOR_LAST_POS == cabac_cs)
		{
			if(find_last_sub_block == 1)
			{
				subnum_lastone_flag_r = 1;//？该标记主要起到的作用是什么   已经找到最后一个有系数4*4
				//output and estimate transfrom_skip_flag,last_x,y
			}
		}

//cabac_ns == ST_SIG_COEFF_FLAG   根据valbin_ecs_sig_coeff_flag得出处理的4个sig_coeff中大于0的系数个数和位置，同时得出coeff的len
#pragma  region
		if(cabac_ns == ST_SIG_COEFF_FLAG)
		{
//			subnum_icnt_r = subnum_icnt_minus1;
			if(first_sig_coeff_cycle == 1)//第一次进来该条件成立
			{
				//output sigcoeffflagencout
				//first_sig_coeff_cycle = 0;
			}


			sig_coeff_flag_r_cnt_idx_ff1_r = sig_coeff_flag_r_cnt_idx_r;// 最后一个未处理系数在4*4块中的扫描序号  sig_coeff_flag_r_cnt_idx_ff1_r  加ff1的都表示备份
			if(sig_coeff_flag_r_cnt_idx_r > 3)//把sig_coeff_flag_r_cnt_idx_r限定成4个一循环
				sig_coeff_flag_r_cnt_idx_r -= 4;
			else
				sig_coeff_flag_r_cnt_idx_r = 0;
//			if(val_len_coded_sub_block_flag < 4)//?val_len_coded_sub_block_flag应该是固定为1才对，那么该条件固定成立
//				assert(val_len_coeff_abs_level_greater1_flag == val_len_coded_sub_block_flag + inferSbDcSigCoeffFlag_r);//last sig_coeff_flag is not be encoded   //?val_len_coded_sub_block_flag 改为coeff_abs_level_greater1_flag

			switch(valbin_ecs_sig_coeff_flag) //最多连续4个sig_coeff_flag组成的bin
			{
			case 0x0:
				break;
			case 0x1:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];//把每个大于0的系数位置记录下来
				cofgreater0_cnt_r = cofgreater0_cnt_plus[1];//?这里应该是在统计一次处理的4个数中大于0的系数的个数加上之前处理的总共的大于0的个数   
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x1; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0x2:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[1];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[1];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x3; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break; 
			case 0x4:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[2];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[1];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x7; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0x8:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[3];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[1];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0xf; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0x3:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[1];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[2];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x1; break;
				case 0x2: greater1_avail_pos = 0x3; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0x6:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[1];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[2];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[2];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x3; break;
				case 0x2: greater1_avail_pos = 0x7; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0xc:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[2];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[3];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[2];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x7; break;
				case 0x2: greater1_avail_pos = 0xf; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0x9:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[3];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[2];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x1; break;
				case 0x2: greater1_avail_pos = 0xf; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0xa:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[1];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[3];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x3; break;
				case 0x2: greater1_avail_pos = 0xf; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				cofgreater0_cnt_r = cofgreater0_cnt_plus[2];
				break;
			case 0x5:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[2];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[2];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x1; break;
				case 0x2: greater1_avail_pos = 0x7; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0x7:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[1];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[2]] = sig_coeff_flag_r_cnt_idx_minus[2];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[3];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x1; break;
				case 0x2: greater1_avail_pos = 0x3; break;
				case 0x3: greater1_avail_pos = 0x7; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0xe:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[1];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[2];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[2]] = sig_coeff_flag_r_cnt_idx_minus[3];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[3];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x3; break;
				case 0x2: greater1_avail_pos = 0x7; break;
				case 0x3: greater1_avail_pos = 0xf; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0xd:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[2];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[2]] = sig_coeff_flag_r_cnt_idx_minus[3];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[3];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x1; break;
				case 0x2: greater1_avail_pos = 0x7; break;
				case 0x3: greater1_avail_pos = 0xf; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0xb:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[1];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[2]] = sig_coeff_flag_r_cnt_idx_minus[3];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[3];
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x0: greater1_avail_pos = 0x0; break;
				case 0x1: greater1_avail_pos = 0x1; break;
				case 0x2: greater1_avail_pos = 0x3; break;
				case 0x3: greater1_avail_pos = 0xf; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			case 0xf:
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[1];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[2]] = sig_coeff_flag_r_cnt_idx_minus[2];
				cofgreater0_flag_pos_r[cofgreater0_cnt_plus[3]] = sig_coeff_flag_r_cnt_idx_minus[3];
				cofgreater0_cnt_r = cofgreater0_cnt_plus[3] + 1;
				switch (8 - cofgreater0_cnt_plus[0])
				{
				case 0x1: greater1_avail_pos = 0x1; break;
				case 0x2: greater1_avail_pos = 0x3; break;
				case 0x3: greater1_avail_pos = 0x7; break;
				default:	  greater1_avail_pos = 0xf; break;
				}
				break;
			}

			if (cofgreater0_cnt_plus[0] < 8)
			{
				switch(valbin_ecs_greater1_for_cnt & greater1_avail_pos)
				{
				case 0x0:
					break;
				case 0x1:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[1];

					break;
				case 0x2:
					
						cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[1];
						cofgreater1_cnt_r = cofgreater1_cnt_plus[1];
					
					break;
				case 0x4:
					
						cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[2];
						cofgreater1_cnt_r = cofgreater1_cnt_plus[1];
					
					break;
				case 0x8:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[3];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[1];
					break;
				case 0x3:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[1];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[2];
					break;
				case 0x6:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[1];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[2];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[2];
					break;
				case 0xc:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[2];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[3];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[2];
					break;
				case 0x9:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[3];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[2];
					break;
				case 0xa:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[1];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[3];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[2];
					break;
				case 0x5:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[2];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[2];
					break;
				case 0x7:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[1];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[2]] = sig_coeff_flag_r_cnt_idx_minus[2];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[3];
					break;
				case 0xe:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[1];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[2];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[2]] = sig_coeff_flag_r_cnt_idx_minus[3];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[3];
					break;
				case 0xd:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[2];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[2]] = sig_coeff_flag_r_cnt_idx_minus[3];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[3];
					break;
				case 0xb:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[1];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[2]] = sig_coeff_flag_r_cnt_idx_minus[3];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[3];
					break;
				case 0xf:
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[0]] = sig_coeff_flag_r_cnt_idx_minus[0];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[1]] = sig_coeff_flag_r_cnt_idx_minus[1];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[2]] = sig_coeff_flag_r_cnt_idx_minus[2];
					cofgreater1_flag_pos_r[cofgreater1_cnt_plus[3]] = sig_coeff_flag_r_cnt_idx_minus[3];
					cofgreater1_cnt_r = cofgreater1_cnt_plus[3] + 1;
					break;
				}
			}
			
			//以下应该是和enc_out有关
			for(int i = 0; i < 4; i++)
			{
				int idx = sig_coeff_flag_r_cnt_idx_minus[i];//获取未处理的倒数第i个系数的扫描序号
				int base = coeff_abs_level_greater1_flag_r[idx] + sig_coeff_flag_r[idx];//这里还没考虑coeff_abs_level_greater2_flag_r

				if(coeff_value_r[idx] + base <= -(2 << 7) || coeff_value_r[idx] - base >= (2 << 7))//? 这里应该是coeff_value_r[idx] + base <= -(2 << 7) || coeff_value_r[idx] - base >= (2 << 7)
				{
					coeff_len[i] = 0x1000;//这里主要体现的思想： 系数bit的最大位数为16位，这里根据系数值分为4种区间,0-2  3-4 5-8 9-16 也就是2bit的系数长度 4.。。8.。。16.。。
				}

				else if(coeff_value_r[idx] + base <= -(2 << 3) || coeff_value_r[idx] - base >= (2 << 3))
				{
					coeff_len[i] = 0x100;
				}

				else if(coeff_value_r[idx] + base <= -(2 << 1) || coeff_value_r[idx] - base >= (2 << 1))
				{
					coeff_len[i] = 0x10;
				}

				else
				{
					coeff_len[i] = 0x1;
				}
			}

			if((coeff_len[0] | coeff_len[1] | coeff_len[2] | coeff_len[3]) & 0x1000)//处理的4个系数值按最大的长度来取
				coeff_len_r = 16;
			else	if((coeff_len[0] | coeff_len[1] | coeff_len[2] | coeff_len[3]) & 0x100)
				coeff_len_r = 8;
			else	if((coeff_len[0] | coeff_len[1] | coeff_len[2] | coeff_len[3]) & 0x10)
				coeff_len_r = 4;
			else
				coeff_len_r = 2;

//			cofgreater1_cnt_r = cofgreater1_cnt;//？应该是在上面的循环中要coeff_abs_level_greater1_flag_r[idx]来加入判断
		}
#pragma  endregion

//cabac_cs == ST_CODED_SUB_BLOCK_FLAG   对firstSigScanPos之类的值重置
#pragma  region
		if(cabac_cs == ST_CODED_SUB_BLOCK_FLAG /*|| cabac_cs == ST_LAST_SIG_COEFF_X_PREFIX*/)
		{

			if ( !sign_data_hiding_enabled_flag || !signHidden  )
				last_sign_flag_hidden 	= 1;
			else
				last_sign_flag_hidden 	= 0;
			coeff_sign_flag_cnt_idx_r 	= cofgreater0_cnt_r - last_sign_flag_hidden - 1;//idx = cnt - 1  //？不是很清楚
			coeff_sign_flag_cnt_r 		= cofgreater0_cnt_r - last_sign_flag_hidden ;
//			first_sig_coeff_cycle 		= 1;

			firstSigScanPos = 16 ;
			lastSigScanPos = -1;
			lastGreater1ScanPos = -1;
			numGreater1Flag_solved_r = 0;


			if(subnum_lastone_flag_r==1)
				subnum_lastone_flag_r = 0;//？ST_LAST_SIG_COEFF_X_PREFIX状态下置0 主要目的是什么
			//output coded_sub_block_flag
		}
#pragma  endregion

//cabac_ns == ST_LOOK_FOR_LAST_POS || cabac_ns == ST_LAST_SIG_COEFF_X_PREFIX || cabac_ns == ST_CODED_SUB_BLOCK_FLAG   对当前4*4块的一些操作
#pragma  region
		if(cabac_ns == ST_LOOK_FOR_LAST_POS || cabac_ns == ST_LAST_SIG_COEFF_X_PREFIX || cabac_ns == ST_CODED_SUB_BLOCK_FLAG)//?cabac_ns == ST_CODED_SUB_BLOCK_FLAG 这个有写错吗？
		{
			cofgreater0_cnt_r = 0;
			cofgreater1_cnt_r = 0;

			for(int i = 15; i >= 0; i--)
			{

				coeff_value_r[i] = abs(coeff_value[i]);//每个系数具体的值
				coeff_abs_level_greater1_flag_r[i] = coeff_abs_level_greater1_flag[i];//该系数是否大于1的标记
				coeff_abs_level_greater1_flag_enc_out_r[15 - i] = coeff_abs_level_greater1_flag[i];//反向存储该系数是否大于1的标记  跟输出顺序有关
				sig_coeff_flag_r[i] = sig_coeff_flag[i];//该点系数是否有 残差的标记
				sig_coeff_flag_enc_out_r[15 - i] = sig_coeff_flag[i];//反向存储
				coeff_sign_flag_r[i] = coeff_sign_flag[i];//该点系数的符号位
				coeff_sign_flag_enc_out_r[15 - i] = coeff_sign_flag[i];//反向存储
				if(cofgreater2_flag_found == 0 && (abs(coeff_value[i]) > 1))
				{
					cofgreater2_flag_found = 1;//？通过该标记来保证该循环只进来一次    实际上RTL的写法并不用cofgreater2_flag_found这个标记，应该是每个大于1的系数都算，然后加个选择器选择选i最大的那个
					coeff_abs_level_greater2_flag_r = (abs(coeff_value[i]) > 2);//该点系数是否大于2的标记
					cofgreater2_flag_pos_r = i;//coeff_abs_level_greater2_flag_r 的IDX  位置记录
				}

			}

			tu_scanraster	= tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r];//把4*4块扫描序号转换成光栅扫描
			codedsub_xs = tu_scanraster % ( 1 << (log2TrafoSize - 2));//4*4块的x坐标   这里坐标的计算是否应该放到7994行之前，否则codedsub_xs没有初始化  当成RTL来写了？
			codedsub_ys = tu_scanraster / ( 1 << (log2TrafoSize - 2));//4*4块的y坐标
			coded_sub_block_flag_r[codedsub_xs + codedsub_ys * ( 1 << (log2TrafoSize - 2))] = coded_sub_block_flag;//以光栅扫描存储4*4块系数是否不全为0的标记
			coded_sub_block_flag_out = coded_sub_block_flag;//和fifo相关

			valbin_ecs_codec_block_flag = coded_sub_block_flag;//RTL才需要, 这是wire  在状态转移之前已有。

//cabac_ns == ST_LOOK_FOR_LAST_POS || (cabac_ns == ST_LAST_SIG_COEFF_X_PREFIX && log2TrafoSize == 2)   主要是对最后一个有值系数的操作，得出前缀，后缀长度之类
#pragma  region
			if(cabac_ns == ST_LOOK_FOR_LAST_POS || (cabac_ns == ST_LAST_SIG_COEFF_X_PREFIX && log2TrafoSize == 2))//？不加上&& log2TrafoSize == 2可以吗
			{
				
				for(int i = 15; i >= 0; i--)
				{
					if(sig_coeff_flag_r[i]!=0)//？只进来一次，通过break来保证   写成sig_coeff_flag[i]是否更合适  因为sig_coeff_flag_r[i]这时候还没生效   对RTL是展开来做，然后以下各种标记通过开关选择i最大的
					{
						last_sig_coeff_x = (4 * (tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] % ( 1 << (log2TrafoSize - 2)))) + tu_scan_table[scanorder_mode][2][i] % 4;//最后一个系数不为0的x坐标在一个TU中
						last_sig_coeff_y = (4 * (tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] / ( 1 << (log2TrafoSize - 2)))) + tu_scan_table[scanorder_mode][2][i] / 4;//最后一个系数不为0的y坐标在一个TU中
						last_sig_coeff_x_prefix	= g_groupIdx[last_sig_coeff_x];//？最后一个系数不为0的x坐标 前缀IDX    last_sig_coeff_x_prefix当做wire来处理的话，与7001行的该标记是当做同时生效的吗？7001中的标记对C++是没有初始化的
						last_sig_coeff_y_prefix	= g_groupIdx[last_sig_coeff_y];//最后一个系数不为0的y坐标 前缀IDX
						if (scanorder_mode == 2)
						{
							std::swap(last_sig_coeff_x_prefix,last_sig_coeff_y_prefix);
						}
						
						last_sig_coeff_x_suffix = last_sig_coeff_x -  ( 1 << ( (last_sig_coeff_x_prefix >> 1 )  - 1 ) ) *    ( 2 + (last_sig_coeff_x_prefix & 1 ) ) ;//最后一个系数不为0的x坐标 后缀
						last_sig_coeff_y_suffix = last_sig_coeff_y -  ( 1 << ( (last_sig_coeff_y_prefix >> 1 )  - 1 ) ) *    ( 2 + (last_sig_coeff_y_prefix& 1 ) ) ;//最后一个系数不为0的y坐标 后缀
						last_sig_coeff_x_suffix_len_r = get_coeff_suffix_len(last_sig_coeff_x_prefix);//获取x后缀长度
						last_sig_coeff_y_suffix_len_r = get_coeff_suffix_len(last_sig_coeff_y_prefix);//获取y后缀长度
						if (last_sig_coeff_x_prefix > 3)
						{
//							uint32_t count = (last_sig_coeff_x_prefix - 2) >> 1;//？干什么用
							last_sig_coeff_x_suffix       = last_sig_coeff_x - g_minInGroup[last_sig_coeff_x_prefix];//最后一个系数不为0的x坐标 后缀   跟上面的求法重复了
						}
						if (last_sig_coeff_y_prefix > 3)
						{
//							uint32_t count = (last_sig_coeff_y_prefix - 2) >> 1;
							last_sig_coeff_y_suffix       = last_sig_coeff_y - g_minInGroup[last_sig_coeff_y_prefix];//最后一个系数不为0的y坐标 后缀   跟上面的求法重复了
						}


						find_last_sub_block = 1;//这里能直接赋值为1 是因为在这个4*4块中至少有一个系数不为0  所以进入这个if里面，那么说面当前4*4块就是最后一个4*4块了
						if( transform_skip_enabled_flag && !cu_transquant_bypass_flag &&  ( log2TrafoSize == 2 ) )
						{
							ru_subblock_parameter_r = (last_sig_coeff_y << 6) |(last_sig_coeff_x << 1) | transfrom_skip_flag_r;//？ 把三个标记合成表示   transfrom_skip_flag_r 是直接从寄存器连线过来吗
							ru_subblock_parameter_len_r = 11;
						}
						else
						{
							ru_subblock_parameter_r = (last_sig_coeff_y << 5) |(last_sig_coeff_x << 0) ;// 把2个标记合成表示 
							ru_subblock_parameter_len_r = 10;
						}
						break;
					}
				}

				if(log2TrafoSize==2)//为2表示只有TU为4*4  那么肯定find_last_sub_block==1
					assert(find_last_sub_block==1);

				last_sig_coeff_x_prefix_r = last_sig_coeff_x_prefix;
				last_sig_coeff_y_prefix_r = last_sig_coeff_y_prefix;
				last_sig_coeff_x_prefix_len_r = last_sig_coeff_x_prefix + (last_sig_coeff_x_prefix != (log2TrafoSize<<1) - 1);//?   前缀IDX长度的计数   前缀是TR模式
				last_sig_coeff_x_prefix_cnt_idx_r = 0;//表示对前缀的bin处理的个数
				last_sig_coeff_y_prefix_len_r = last_sig_coeff_y_prefix + (last_sig_coeff_y_prefix != (log2TrafoSize<<1) - 1);//?
				last_sig_coeff_y_prefix_cnt_idx_r = 0;//表示对前缀的bin处理的个数
				if(!find_last_sub_block)//当前4*4块都为0，继续往前找。
					subnum_icnt_r = subnum_icnt_minus1;//寻找最后一个有系数4*4块的序号
			}
#pragma  endregion

//cabac_ns == ST_CODED_SUB_BLOCK_FLAG || cabac_ns == ST_LAST_SIG_COEFF_X_PREFIX   对当前4*4块的一些处理
#pragma  region
			if(cabac_ns == ST_CODED_SUB_BLOCK_FLAG || cabac_ns == ST_LAST_SIG_COEFF_X_PREFIX)
			{
				sig_coeff_flag_blk0_zero_r = 0;
				cofgreater0_cnt = 0;
				lastSigScanPos = 0;
				for(int i = 0; i < 16; i++)
				{
					if(sig_coeff_flag[i])
					{

						lastSigScanPos	= i;//最后一个大于0的系数的位置
						sig_coeff_flag_blk0_zero_r = 1;//1表示该4*4块有系数
						cofgreater0_cnt++;//表示一个4*4块中大于0的系数有几个
						if(coeff_abs_level_greater1_flag[i])
							lastGreater1ScanPos = i;//？最后一个大于1的系数的位置   是不是应该放到上面一个for循环中
					}
				}
				for(int i = 15; i >=0; i--)
				{
					if(sig_coeff_flag[i])
					{

						firstSigScanPos	= i;//第一个大于0的系数的位置
					}
				}

				cofgreater0_cnt_idx_r = (cofgreater0_cnt - 1) >=0 ? (cofgreater0_cnt - 1) : 0;//大于0的个数减一
				coeff_sign_flag_cnt_r = lastSigScanPos + 1;//总共需要编码的sig的个数

// 				if(subnum_lastone_flag_r != 1)//从LOOK_FOR_LAST_POS状态到LAST_SIG_COEFF_X状态的过程中  subnum_lastone_flag_r==1    自转时为0
// 				{
// 
// 					tu_scanraster		= tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_minus1];
// 					codedsub_xs 	= tu_scanraster / ( 1 << (log2TrafoSize - 2));
// 					codedsub_ys 	= tu_scanraster % ( 1 << (log2TrafoSize - 2));
// 					subnum_icnt_r 	= subnum_icnt_minus1;
// 				}
// 				else
// 				{
					tu_scanraster		= tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r];//当前4*4块的光栅扫描坐标   codedsub_xs之类的都是线，每次要用的时候从subnum_icnt_r之类的reg推导出来
					codedsub_xs 	= tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] % ( 1 << (log2TrafoSize - 2));//当前4*4块的x坐标
					codedsub_ys 	= tu_scan_table[scanorder_mode][log2TrafoSize - 2][subnum_icnt_r] / ( 1 << (log2TrafoSize - 2));//当前最后一个有系数4*4块的y坐标
//				}

				if(subnum_lastone_flag_r == 1 || subnum_icnt_r == 0)//第一块是每个点都需要编码sig_coeff_flag标记的，所以不能推导，故inferSbDcSigCoeffFlag会赋值为0
				{
					inferSbDcSigCoeffFlag = 0;//该标记作用：一个4*4块有残差   而且后15个点系数都为0，那么第一个点就默认有残差  为1时表示开启 可以推导
				}
				else
				{
					inferSbDcSigCoeffFlag = 1;
					for(int i = 15; i >=1; i--)//这里是在判断除(0，0)点外的其他15个点是否都是为0，如果有一个不为0 ，那么就不能推导了，所以会把inferSbDcSigCoeffFlag赋值为0
					{
						if(sig_coeff_flag_r[i] != 0)
						{
							inferSbDcSigCoeffFlag = 0;
							break;
						}
					}
				}

				if(subnum_lastone_flag_r == 1 || (log2TrafoSize == 2))//从LOOK_FOR_LAST_POS状态到LAST_SIG_COEFF_X状态的过程中  subnum_lastone_flag_r==1
				{
					sig_coeff_flag_r_cnt_idx_r 	= lastSigScanPos;//最后一个有值系数在4*4块中的扫描序号
				}
				else
				{
					sig_coeff_flag_r_cnt_idx_r 	= 15;
				}
				inferSbDcSigCoeffFlag_r = inferSbDcSigCoeffFlag;//从LOOK_FOR_LAST_POS状态到LAST_SIG_COEFF_X状态的过程中 为0  因为是最后一块
			}
			tu_scanraster_r 			= tu_scanraster;//4*4块的光栅扫描坐标
#pragma  endregion
		}
#pragma  endregion

//cabac_ns == ST_COEFF_ABS_LEVEL_GREATER1_FLAG   主要通过valbin_ecs_coeff_abs_level_greater1_flag 来计算大于1的系数个数和位置。 4个一个循环
#pragma  region

#pragma  endregion

//cabac_cs == ST_SIG_COEFF_FLAG && cabac_ns == ST_COEFF_ABS_LEVEL_GREATER1_FLAG    主要计算coeff_abs_level_remaining 的个数 位置 具体的值
#pragma  region
		if(cabac_cs == ST_SIG_COEFF_FLAG && cabac_ns == ST_COEFF_ABS_LEVEL_GREATER1_FLAG)//注意这里是与        这里主要的作用都是在求coeff_abs_level_remaining_pos_r[] 和 coeff_abs_level_remaining_r[]
		{
			if (cofgreater1_cnt_r == 0)
			{
				valbin_ecs_coeff_abs_level_greater2_flag 	= 3;//把标记转换成bin  虽然其实是一样的3表示该标记根本没有被编码
				val_len_coeff_abs_level_greater2_flag 			= 0;//该标记长度固定为1
			}
			else
			{
				valbin_ecs_coeff_abs_level_greater2_flag 	= coeff_abs_level_greater2_flag_r;//把标记转换成bin  虽然其实是一样的 
				val_len_coeff_abs_level_greater2_flag 			= 1;//该标记长度固定为1
			}
			
			if(cofgreater0_cnt_r > 0)//? cofgreater1_cnt_r 改为cofgreater0_cnt_r
			{
				if(cofgreater0_cnt_r <= 8)//？这里为什么是小于8  要不要改成小于等于  cofgreater1_cnt_r 改为cofgreater0_cnt_r
				{
					if(valbin_ecs_coeff_abs_level_greater2_flag == 1)
					{
						assert(cofgreater2_flag_pos_r == cofgreater1_flag_pos_r[0]);
						coeff_abs_level_remaining_pos_r[0] = cofgreater1_flag_pos_r[0];
							coeff_abs_level_remaining_r[0] = coeff_value_r[cofgreater1_flag_pos_r[0]] - 3;//？因为系数大于2，减3之后就是剩下的要估算的系数值  coeff_abs_level_remaining_pos_r 改为coeff_abs_level_remaining_r吗  位置和剩下的系数值应该都记录

					}

					for(int i = 1; i < cofgreater1_cnt_r; i++)
					{
						coeff_abs_level_remaining_pos_r[i - (valbin_ecs_coeff_abs_level_greater2_flag == 0)] 	= cofgreater1_flag_pos_r[i];//实际上不用记录位置也可以的，因为coeff_abs_level_remaining_r的存储和位置无关
						coeff_abs_level_remaining_r[i - (valbin_ecs_coeff_abs_level_greater2_flag==0)]			= coeff_value_r[cofgreater1_flag_pos_r[i]] - 2;//?coeff_abs_level_remaining_r[i - coeff_abs_level_greater2_flag_r]  改为coeff_abs_level_remaining_r[i - (coeff_abs_level_greater2_flag_r == 0) ]  

					}

					coeff_abs_level_remaining_cnt_r = MAX(cofgreater1_cnt_r - !valbin_ecs_coeff_abs_level_greater2_flag , 0);//统计减去baseLevel后需要编码的系数的个数
				}
				else
				{
					if(valbin_ecs_coeff_abs_level_greater2_flag == 1)
					{
						assert(cofgreater2_flag_pos_r == cofgreater1_flag_pos_r[0]);
						coeff_abs_level_remaining_pos_r[0] = cofgreater1_flag_pos_r[0];
						coeff_abs_level_remaining_r[0] = coeff_value_r[cofgreater1_flag_pos_r[0]] - 3;//？因为系数大于2，减3之后就是剩下的要估算的系数值  coeff_abs_level_remaining_pos_r 改为coeff_abs_level_remaining_r吗  位置和剩下的系数值应该都记录
					}

					for(int i = 1; i < cofgreater1_cnt_r ; i++)//cofgreater1_cnt_r只针对最后8个大于0的系数的统计
					{
						coeff_abs_level_remaining_pos_r[i - (valbin_ecs_coeff_abs_level_greater2_flag == 0)] 	= cofgreater1_flag_pos_r[i];
						coeff_abs_level_remaining_r[i - (valbin_ecs_coeff_abs_level_greater2_flag == 0)]			= coeff_value_r[cofgreater1_flag_pos_r[i]] - 2;//?coeff_abs_level_remaining_r[i - coeff_abs_level_greater2_flag_r]  改为coeff_abs_level_remaining_r[i - (coeff_abs_level_greater2_flag_r == 0) ]  或者这里想这么表达的话，上面就位置的赋值要更改
					}

// 					for(i = 0; i < cofgreater0_cnt_r; i++)//这里主要的目的是寻找还未处理的系数
// 					{
// 						if(cofgreater1_flag_pos_r[cofgreater1_cnt_r-1] == cofgreater0_flag_pos_r[i])
// 						{
// 							i++;
// 							break;
// 						}
// 					}

					int k = cofgreater1_cnt_r;
					for(int j = 8; j < cofgreater0_cnt_r; j++,k++)
					{
						coeff_abs_level_remaining_pos_r[k - (valbin_ecs_coeff_abs_level_greater2_flag == 0)] 	= cofgreater0_flag_pos_r[j];
						coeff_abs_level_remaining_r[k - (valbin_ecs_coeff_abs_level_greater2_flag == 0)]			= coeff_value_r[cofgreater0_flag_pos_r[j]] - 1;//减去baseLevel之后的系数值
					}
					coeff_abs_level_remaining_cnt_r = MAX(cofgreater1_cnt_r - !valbin_ecs_coeff_abs_level_greater2_flag + cofgreater0_cnt_r - 8,0);//统计减去baseLevel后需要编码的系数的个数 8 + !coeff_abs_level_greater2_flag_r改为   cofgreater1_cnt_r - !coeff_abs_level_greater2_flag_r
				}
			}
			else
			{
				coeff_abs_level_remaining_cnt_r = 0;
			}

			coeff_abs_level_remaining_cnt_idx_r = MAX(coeff_abs_level_remaining_cnt_r - 1 , 0);

			first_to_greater1_r = 1;
		}
#pragma  endregion

//cabac_cs == ST_COEFF_ABS_LEVEL_GREATER1_FLAG && cabac_ns == ST_COEFF_ABS_LEVEL_GREATER1_FLAG) || 
//		cabac_ns == ST_COEFF_ABS_LEVEL_REMAINING         主要计算coeff_abs_level_remaining的长度 并且更新其上下文
#pragma  region
		if((cabac_cs == ST_COEFF_ABS_LEVEL_GREATER1_FLAG && cabac_ns == ST_COEFF_ABS_LEVEL_GREATER1_FLAG) || 
			cabac_ns == ST_COEFF_ABS_LEVEL_REMAINING)
		{
			if (coeff_abs_level_remaining_cnt_r == 0)
			{
				coeff_abs_level_remaining_cnt_ff1_r =0;
			} 
			else
			{
				coeff_abs_level_remaining_cnt_ff1_r = coeff_abs_level_remaining_cnt_idx_r + 1;
				if(coeff_abs_level_remaining_cnt_idx_r	 >= 7)
				{//?应该加一个i的for循环
					for(int i = 0; i < 8; i++)
					{
						coeff_abs_level_remaining_len_r[i] =
							get_coeff_len(coeff_abs_level_remaining_r[i + coeff_abs_level_remaining_cnt_r - 1 - coeff_abs_level_remaining_cnt_idx_r], cRiceParam[i]);//est len of coeff_abs_level_remaining  //?coeff_abs_level_remaining_r的存储是按顺序的  改成coeff_abs_level_remaining_r[i]
					}
					coeff_abs_level_remaining_cnt_idx_r		-=8;
					cLastAbsLevel_r = cAbsLevel[7];
					cLastRiceParam_r= cRiceParam[7];
				}
				else
				{
					for(int i = 0; i < (coeff_abs_level_remaining_cnt_idx_r+1); i++)
						coeff_abs_level_remaining_len_r[i] =
						get_coeff_len(coeff_abs_level_remaining_r[i + coeff_abs_level_remaining_cnt_r - 1 - coeff_abs_level_remaining_cnt_idx_r], cRiceParam[i]);//est len of coeff_abs_level_remaining  //?coeff_abs_level_remaining_r的存储是按顺序的  改成coeff_abs_level_remaining_r[i]

					cLastAbsLevel_r = cAbsLevel[coeff_abs_level_remaining_cnt_idx_r];
					cLastRiceParam_r= cRiceParam[coeff_abs_level_remaining_cnt_idx_r];

					coeff_abs_level_remaining_cnt_idx_r			=	0;
				}
			}

			first_to_greater1_r = 0;

//			valbin_r =  valbin_ecs_coeff_abs_level_remaining;//!enc_out data
//			val_len_r =  val_len_coeff_abs_level_remaining * coeff_len_r;//!enc_out len
		}
#pragma  endregion


		if(cabac_ns == ST_COEFF_ABS_LEVEL_GREATER1_FLAG)
		{
// 			if (Greater1Flag_num_avail == 0)
// 			{
// 				if((coeff_abs_level_remaining_cnt_ff1_r <= 4) && subnum_icnt_r!=0)
// 					subnum_icnt_r = subnum_icnt_minus1;
// 			}


			if (cabac_cs == ST_SIG_COEFF_FLAG)
			{
//				subnum_icnt_r -= 1;//理论上应该放在cabac_ns == ST_CODED_SUB_BLOCK_FLAG
				subnum_lastone_flag_r = 0 ;
				coeff_abs_level_remaining_cnt_ff1_r = 15;
			}

			ctxSet_r = ctxSet;
			
// 			if(Greater1Flag_equal_1_num > Greater1Flag_num_avail)
// 			{
// 				Greater1Flag_equal_1_num_ff1 	= Greater1Flag_num_avail;
// 				numGreater1Flag_solved_r			+=	Greater1Flag_num_avail;
// 			}
// 			else
// 			{
				numGreater1Flag_solved_r			+=	val_len_coeff_abs_level_greater1_flag;//表示已经处理的 是否大于1 这个标记的系数
//				Greater1Flag_equal_1_num_ff1 	= Greater1Flag_equal_1_num;
//			}

			if(cofgreater0_cnt_idx_r > 3)
				cofgreater0_cnt_idx_r -= 4;
			else
				cofgreater0_cnt_idx_r = 0;

			if(val_len_coeff_abs_level_greater1_flag !=0 )
			{
				lastGreater1Flag_r = Greater1Flag[val_len_coeff_abs_level_greater1_flag - 1];
				lastGreater1Ctx_r	=	greater1Ctx[val_len_coeff_abs_level_greater1_flag - 1];
				//cofgreater1_cnt_r = cofgreater1_cnt_plus[val_len_coeff_abs_level_greater1_flag - 1];
				assert(val_len_sig_coeff_flag < 4);
			}
			val_len_r 		=  val_len_coeff_abs_level_greater1_flag;
			valbin_r		=	valbin_ecs_coeff_abs_level_greater1_flag ;
			valbin_ctxInc_r[0] = cofgreater1_ctxinc[0]; //存储上下文，0是valbin_r最右边bit的上下文
			valbin_ctxInc_r[1] = cofgreater1_ctxinc[1];
			valbin_ctxInc_r[2] = cofgreater1_ctxinc[2];
			valbin_ctxInc_r[3] = cofgreater1_ctxinc[3];


			val_len_coeff_abs_level_greater2_flag_r = val_len_coeff_abs_level_greater2_flag;
			valbin_ecs_coeff_abs_level_greater2_flag_r = valbin_ecs_coeff_abs_level_greater2_flag;
			cofgreater2_ctxinc_r[0] =  cofgreater2_ctxinc;
		}
		else	if(cabac_ns == ST_COEFF_ABS_LEVEL_GREATER2_FLAG)
		{


		}
		else	if(cabac_ns == ST_SIG_COEFF_FLAG)
		{
			if(first_sig_coeff_cycle == 1)//最后一个有值系数的sig是不需要编码的
			{
				first_sig_coeff_cycle = 0 ;
				valbin_r 		=	valbin_ecs_sig_coeff_flag >> 1;
				val_len_r		=	val_len_sig_coeff_flag - 1;//要处理的sig_coeff_flag的个数   包括sig_coeff_flag为0的      第一有值系数的sig是默认为1的，所以这里要特别处理
				valbin_ctxInc_r[0] = cofsig_ctxinc[1]; //存储上下文，0是valbin_r最右边bit的上下文
				valbin_ctxInc_r[1] = cofsig_ctxinc[2];
				valbin_ctxInc_r[2] = cofsig_ctxinc[3];
			}
			else
			{
				valbin_r 		=	valbin_ecs_sig_coeff_flag;
				val_len_r		=	val_len_sig_coeff_flag;//要处理的sig_coeff_flag的个数   包括sig_coeff_flag为0的   
				valbin_ctxInc_r[0] = cofsig_ctxinc[0]; //存储上下文，0是valbin_r最右边bit的上下文
				valbin_ctxInc_r[1] = cofsig_ctxinc[1];
				valbin_ctxInc_r[2] = cofsig_ctxinc[2];
				valbin_ctxInc_r[3] = cofsig_ctxinc[3];
			}
		}
		else	if(cabac_ns == ST_CODED_SUB_BLOCK_FLAG)
		{
// 			if(coded_sub_block_flag_out != 1 )
// 				subnum_icnt_r = subnum_icnt_minus1;

			valbin_r 		=	valbin_ecs_codec_block_flag ;
			val_len_r		=	val_len_coded_sub_block_flag;
			valbin_ctxInc_r[0] = codedsub_ctxinc;
		}
		else	if(cabac_ns == ST_COEFF_ABS_LEVEL_REMAINING)
		{
// 			if (coeff_abs_level_remaining_cnt_ff1_r <= 8)
// 			{
// 				if(subnum_icnt_r!=0) /// sig_coeff_flag_r valid num
// 					subnum_icnt_r = subnum_icnt_minus1;
// 			}

			if(cabac_cs != ST_COEFF_ABS_LEVEL_REMAINING)
			{
//				valbin_r 		=	valbin_ecs_coeff_sign_flag ;//greater2 sign_sign estimated with the first abs-remaining  //?计算符号位的？  符号位在之前已经计算过了
//				val_len_r		=	val_len_coeff_sign_flag;
// 				val_len_r = val_len_coeff_abs_level_greater2_flag;
// 				valbin_r = valbin_ecs_coeff_abs_level_greater2_flag;
// 				valbin_ctxInc_r[0] =  cofgreater2_ctxinc;
			}
		}
		else if(cabac_ns == ST_LAST_SIG_COEFF_X_PREFIX)
		{

			last_sig_coeff_x_prefix_cnt_idx_ff1_r = last_sig_coeff_x_prefix_cnt_idx_r;//刚开始被赋值为被赋值为0  ，表示的是对前缀接下去要处理序号为几的bin
			if(last_sig_coeff_x_prefix_cnt_idx_r + 4 <= last_sig_coeff_x_prefix_len_r)//为了一次最多处理4个bin
				last_sig_coeff_x_prefix_cnt_idx_r += 4;
			else
				last_sig_coeff_x_prefix_cnt_idx_r = last_sig_coeff_x_prefix_len_r;
			valbin_r 		=	valbin_ecs_last_sig_coeff_xy_prefix ;//把bin存到该寄存器
			val_len_r		=	val_len_last_sig_coeff_xy_prefix;//把bin的长度存到该寄存器
			valbin_ctxInc_r[0] = last_sig_coeff_xy_prefix_ctx_inc[0]; //存储上下文，0是valbin_r最右边bit的上下文
			valbin_ctxInc_r[1] = last_sig_coeff_xy_prefix_ctx_inc[1];
			valbin_ctxInc_r[2] = last_sig_coeff_xy_prefix_ctx_inc[2];
			valbin_ctxInc_r[3] = last_sig_coeff_xy_prefix_ctx_inc[3];
		}
		else if(cabac_ns == ST_LAST_SIG_COEFF_Y_PREFIX)
		{

			last_sig_coeff_y_prefix_cnt_idx_ff1_r = last_sig_coeff_y_prefix_cnt_idx_r;
			if(last_sig_coeff_y_prefix_cnt_idx_r + 4 <= last_sig_coeff_y_prefix_len_r)
				last_sig_coeff_y_prefix_cnt_idx_r += 4;
			else
				last_sig_coeff_y_prefix_cnt_idx_r = last_sig_coeff_y_prefix_len_r;
			valbin_r 		=	valbin_ecs_last_sig_coeff_xy_prefix ;
			val_len_r		=	val_len_last_sig_coeff_xy_prefix;
			valbin_ctxInc_r[0] = last_sig_coeff_xy_prefix_ctx_inc[0]; //存储上下文，0是valbin_r最右边bit的上下文
			valbin_ctxInc_r[1] = last_sig_coeff_xy_prefix_ctx_inc[1];
			valbin_ctxInc_r[2] = last_sig_coeff_xy_prefix_ctx_inc[2];
			valbin_ctxInc_r[3] = last_sig_coeff_xy_prefix_ctx_inc[3];
		}





		for(int i = 0; i < 4; i++)
			last_sig_coeff_xy_prefix_ctx_inc_ff1[i] = last_sig_coeff_xy_prefix_ctx_inc[i];//log2TrafoSize == 2情况下起作用
		for(int i = 0; i < 4; i++)
			cofgreater1_ctxinc_ff1[i] =cofgreater1_ctxinc[i];
		cofgreater2_ctxinc_ff1 = cofgreater2_ctxinc;
		codedsub_ctxinc_ff1 = codedsub_ctxinc;
		for(int i = 0; i < 4; i++)
		{
			cofsig_ctxinc_ff1[i] 		= cofsig_ctxinc[i];
		}
		firstSigScanPos_r 			= firstSigScanPos ;
//		lastSigScanPos_r 			= lastSigScanPos;
		lastGreater1ScanPos_r 	= lastGreater1ScanPos;
		if(fifo_has_output)
		{
			//output enc_out enc_out_dat_output
		}
	}

}

//CABAC 接口函数
// uint32_t CABAC_RDO::Est_bit_cu(uint32_t  depth , bool  IsIntra , bool  cu_skip_flag)//CU级别某层深度下bit估计
// {
// 	return 1;//return est_bit_cu[IsIntra][depth] + est_bit_pu[IsIntra][depth] + est_bit_tu[IsIntra][depth];
// }

uint32_t CABAC_RDO::Est_bit_pu_luma_dir(uint32_t  depth , int  dir)//PU级别INTRA 35种方向luma bit估计
{
	return (est_bit_pu_luma_dir[depth][dir]+16384)>>15;
}

uint32_t CABAC_RDO::Intra_Est_bit_pu_luma_dir(int  IntraDirLumaAng , ContextModel_hm mModels_of_CU_PU[CONTEXT_NUM], int pred[3] , uint32_t &bit_fraction )//PU级别INTRA 35种方向luma bit估计
{
	 bit_fraction = 0;
	UChar mState = mModels_of_CU_PU[PREV_INTRA_LUMA_PRED_MODE].get_m_ucState();
	if (IntraDirLumaAng == pred[0])
	{
		bit_fraction =  32768 + CABAC_g_entropyBits_1[mState];
		//						mstate_prev_intra_luma_pred_flag[depth] =  aucNextState_1[mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE]];
	} 
	else if (IntraDirLumaAng == pred[1] || IntraDirLumaAng == pred[2])
	{
		bit_fraction =  65536 + CABAC_g_entropyBits_1[mState];
		//						mstate_prev_intra_luma_pred_flag[depth] =  aucNextState_1[mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE]];
	}
	else
	{
		bit_fraction =  163840 + CABAC_g_entropyBits_0[mState];
		//						mstate_prev_intra_luma_pred_flag[depth] =  aucNextState_0[mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE]];
	}

	

	return ((bit_fraction +16384)>> 15);
}

uint64_t CABAC_RDO::Intra_est_bit_cu_pu()//PU级别INTRA 35种方向luma bit估计
{
	UChar mState ;
	UChar mState_next;
	int bits=0;
	int ctxIdx;
	int bestDir = intra_pu_depth_luma_bestDir[depth_tu];

	if ( (depth_tu == 4 && (tu_luma_idx == 0) ) || depth_tu!=4 )
	{
		if (slice_header_struct.slice_type != 2)
		{
			ctxIdx =  getCtxSkipFlag(zscan_idx_tu>>2,depth_tu,IsIntra_tu);
			mState = mModels[IsIntra_tu][depth_tu][SKIP_FLAG_OFFSET+ ctxIdx].get_m_ucState();
			mState_next = aucNextState_0[mState] ;
			bits += CABAC_g_entropyBits_0[mState];
			mModels[IsIntra_tu][depth_tu][SKIP_FLAG_OFFSET+ ctxIdx].set_m_ucState(mState_next);

			mState = mModels[IsIntra_tu][depth_tu][PRED_MODE_OFFSET].get_m_ucState();
			mState_next = aucNextState_1[mState] ;
			bits += CABAC_g_entropyBits_1[mState];
			mModels[IsIntra_tu][depth_tu][PRED_MODE_OFFSET].set_m_ucState(mState_next);
		}

		if (depth_tu == 3)
		{
			mState = mModels[IsIntra_tu][depth_tu][PART_MODE_OFFSET].get_m_ucState();
			mState_next = aucNextState_1[mState] ;
			bits += CABAC_g_entropyBits_1[mState];
			mModels[IsIntra_tu][depth_tu][PART_MODE_OFFSET].set_m_ucState(mState_next);
		}
		if (depth_tu == 4)
		{
			mState = mModels[IsIntra_tu][depth_tu][PART_MODE_OFFSET].get_m_ucState();
			mState_next = aucNextState_0[mState] ;
			bits += CABAC_g_entropyBits_0[mState];
			mModels[IsIntra_tu][depth_tu][PART_MODE_OFFSET].set_m_ucState(mState_next);
		}
	}

//	setDepthSubParts(depth_tu, zscan_idx_tu>>2 , IsIntra_tu);
	setSkipFlagSubParts(0, zscan_idx_tu>>2 , depth_tu , IsIntra_tu);

	if (tu_luma_idx != 4)
	{
		mState = mModels[IsIntra_tu][depth_tu][PREV_INTRA_LUMA_PRED_MODE].get_m_ucState();
		if (bestDir == preds[0])
		{
			mState_next = aucNextState_1[mState];
			bits += CABAC_g_entropyBits_1[mState] + 32768;
		}
		else if (bestDir == preds[1] || bestDir == preds[2])
		{
			mState_next = aucNextState_1[mState];
			bits += CABAC_g_entropyBits_1[mState] + 65536;
		}
		else
		{
			mState_next = aucNextState_0[mState];
			bits += CABAC_g_entropyBits_0[mState] + 163840;
		}
		mModels[IsIntra_tu][depth_tu][PREV_INTRA_LUMA_PRED_MODE].set_m_ucState(mState_next);
	}
	

	if ( (depth_tu == 4 && (tu_luma_idx == 0) ) || depth_tu!=4 )
	{
		mState = mModels[IsIntra_tu][depth_tu][INTRA_CHROMA_PRED_MODE_OFFSET].get_m_ucState();
		mState_next = aucNextState_0[mState] ;
		bits += CABAC_g_entropyBits_0[mState];
		mModels[IsIntra_tu][depth_tu][INTRA_CHROMA_PRED_MODE_OFFSET].set_m_ucState(mState_next);
	}
	return bits;
}

void CABAC_RDO::Intra_pu_update(ContextModel_hm mModels_of_CU_PU[CONTEXT_NUM], int  bestDir , int pred[3] , int inx_in_tu , int depth , int IsIntra)//PU级别INTRA 35种方向luma bit估计
{
	UChar mState = mModels_of_CU_PU[PREV_INTRA_LUMA_PRED_MODE].get_m_ucState();
	UChar next_status;
	preds[0] = pred[0]; preds[1] = pred[1]; preds[2] = pred[2];

	if (bestDir == pred[0] )
	{
		next_status = aucNextState_1[mState];
	}
	else if (bestDir == pred[1] || bestDir == pred[2])
	{
		next_status = aucNextState_1[mState];
	}
	else
	{
		next_status = aucNextState_0[mState];
	}
	mModels_of_CU_PU[PREV_INTRA_LUMA_PRED_MODE].set_m_ucState(next_status);
//	mModels[1][depth][PREV_INTRA_LUMA_PRED_MODE].m_ucState = mModels_of_CU_PU[PREV_INTRA_LUMA_PRED_MODE].m_ucState;

	intra_pu_depth_luma_bestDir[depth] = bestDir;

	if ( (depth == 4 && (inx_in_tu%4 == 0) ) || depth!=4 )
	{
		intra_pu_depth_chroma_bestDir[depth] = bestDir;
	}
	
	setLumaIntraDirSubParts(bestDir , inx_in_tu , depth , IsIntra);
	setPredModeSubParts(MODE_INTRA , tu_to_cu_idx(inx_in_tu) ,depth, IsIntra);
	setAvailSubParts_map(1, inx_in_tu , depth , IsIntra);
}

void CABAC_RDO::Intra_cu_update()
{
	if (cu_best_mode[depth_tu] ==1 )
	{
		for (int i = (depth_tu+1) ; i < 5 ; i ++)
		{
			memcpy(intra_luma_pred_mode_left_up[IsIntra_tu][i],intra_luma_pred_mode_left_up[IsIntra_tu][depth_tu],sizeof(intra_luma_pred_mode_left_up[IsIntra_tu][depth_tu]));
			memcpy(pred_mode_left_up[IsIntra_tu][i],pred_mode_left_up[IsIntra_tu][depth_tu],sizeof(pred_mode_left_up[IsIntra_tu][depth_tu]));
			memcpy(avail_left_up[IsIntra_tu][i],avail_left_up[IsIntra_tu][depth_tu],sizeof(avail_left_up[IsIntra_tu][depth_tu]));

			memcpy(depth_left_up[IsIntra_tu][i],depth_left_up[IsIntra_tu][depth_tu],sizeof(depth_left_up[IsIntra_tu][depth_tu]));
			memcpy(skip_flag_left_up[IsIntra_tu][i],skip_flag_left_up[IsIntra_tu][depth_tu],sizeof(skip_flag_left_up[IsIntra_tu][depth_tu]));

			memcpy(mModels[IsIntra_tu][i] ,mModels[IsIntra_tu][depth_tu], sizeof(mModels[IsIntra_tu][i]) );
		}
	}
	else
	{
		memcpy(intra_luma_pred_mode_left_up[IsIntra_tu][depth_tu],intra_luma_pred_mode_left_up[IsIntra_tu][depth_tu+1],sizeof(intra_luma_pred_mode_left_up[IsIntra_tu][depth_tu]));
		memcpy(pred_mode_left_up[IsIntra_tu][depth_tu],pred_mode_left_up[IsIntra_tu][depth_tu+1],sizeof(pred_mode_left_up[IsIntra_tu][depth_tu]));
		memcpy(avail_left_up[IsIntra_tu][depth_tu],avail_left_up[IsIntra_tu][depth_tu+1],sizeof(avail_left_up[IsIntra_tu][depth_tu]));

		memcpy(depth_left_up[IsIntra_tu][depth_tu],depth_left_up[IsIntra_tu][depth_tu+1],sizeof(depth_left_up[IsIntra_tu][depth_tu]));
		memcpy(skip_flag_left_up[IsIntra_tu][depth_tu],skip_flag_left_up[IsIntra_tu][depth_tu+1],sizeof(skip_flag_left_up[IsIntra_tu][depth_tu]));

		memcpy(mModels[IsIntra_tu][depth_tu] , mModels[IsIntra_tu][depth_tu+1] , sizeof(mModels[IsIntra_tu][depth_tu]));
	}
}

void CABAC_RDO::cu_update()
{
	if (cu_best_mode[depth_tu] ==1 )
	{
		for (int i = (depth_tu) ; i < 5 ; i ++)
		{
			for (int j = 0 ; j < 3 ; j++)
			{
				memcpy(intra_luma_pred_mode_left_up[j][i],intra_luma_pred_mode_left_up[1][depth_tu],sizeof(intra_luma_pred_mode_left_up[IsIntra_tu][depth_tu]));
				memcpy(pred_mode_left_up[j][i],pred_mode_left_up[1][depth_tu],sizeof(pred_mode_left_up[IsIntra_tu][depth_tu]));
				memcpy(avail_left_up[j][i],avail_left_up[1][depth_tu],sizeof(avail_left_up[IsIntra_tu][depth_tu]));

				memcpy(depth_left_up[j][i],depth_left_up[1][depth_tu],sizeof(depth_left_up[IsIntra_tu][depth_tu]));
				memcpy(skip_flag_left_up[j][i],skip_flag_left_up[1][depth_tu],sizeof(skip_flag_left_up[IsIntra_tu][depth_tu]));

				memcpy(mModels[j][i] ,mModels[1][depth_tu], sizeof(mModels[IsIntra_tu][i]) );
			}
		}
	}
	else if (cu_best_mode[depth_tu] == 0)
	{
		if (inter_cu_best_mode[depth_tu] == 0)
		{
			for (int i = (depth_tu) ; i < 5 ; i ++)
			{
				for (int j = 0 ; j < 3 ; j++)
				{
					memcpy(intra_luma_pred_mode_left_up[j][i],intra_luma_pred_mode_left_up[0][depth_tu],sizeof(intra_luma_pred_mode_left_up[IsIntra_tu][depth_tu]));
					memcpy(pred_mode_left_up[j][i],pred_mode_left_up[0][depth_tu],sizeof(pred_mode_left_up[IsIntra_tu][depth_tu]));
					memcpy(avail_left_up[j][i],avail_left_up[0][depth_tu],sizeof(avail_left_up[IsIntra_tu][depth_tu]));

					memcpy(depth_left_up[j][i],depth_left_up[0][depth_tu],sizeof(depth_left_up[IsIntra_tu][depth_tu]));
					memcpy(skip_flag_left_up[j][i],skip_flag_left_up[0][depth_tu],sizeof(skip_flag_left_up[IsIntra_tu][depth_tu]));

					memcpy(mModels[j][i] ,mModels[0][depth_tu], sizeof(mModels[IsIntra_tu][i]) );
				}
			}
		} 
		else
		{
			for (int i = (depth_tu) ; i < 5 ; i ++)
			{
				for (int j = 0 ; j < 3 ; j++)
				{
					memcpy(intra_luma_pred_mode_left_up[j][i],intra_luma_pred_mode_left_up[2][depth_tu],sizeof(intra_luma_pred_mode_left_up[IsIntra_tu][depth_tu]));
					memcpy(pred_mode_left_up[j][i],pred_mode_left_up[2][depth_tu],sizeof(pred_mode_left_up[IsIntra_tu][depth_tu]));
					memcpy(avail_left_up[j][i],avail_left_up[2][depth_tu],sizeof(avail_left_up[IsIntra_tu][depth_tu]));

					memcpy(depth_left_up[j][i],depth_left_up[2][depth_tu],sizeof(depth_left_up[IsIntra_tu][depth_tu]));
					memcpy(skip_flag_left_up[j][i],skip_flag_left_up[2][depth_tu],sizeof(skip_flag_left_up[IsIntra_tu][depth_tu]));

					memcpy(mModels[j][i] ,mModels[2][depth_tu], sizeof(mModels[IsIntra_tu][i]) );
				}
			}
		}
	}
	else
	{
		for (int i = 0 ; i < 3 ; i++)
		{
			memcpy(intra_luma_pred_mode_left_up[i][depth_tu],intra_luma_pred_mode_left_up[IsIntra_tu][depth_tu+1],sizeof(intra_luma_pred_mode_left_up[IsIntra_tu][depth_tu]));
			memcpy(pred_mode_left_up[i][depth_tu],pred_mode_left_up[IsIntra_tu][depth_tu+1],sizeof(pred_mode_left_up[IsIntra_tu][depth_tu]));
			memcpy(avail_left_up[i][depth_tu],avail_left_up[IsIntra_tu][depth_tu+1],sizeof(avail_left_up[IsIntra_tu][depth_tu]));

			memcpy(depth_left_up[i][depth_tu],depth_left_up[IsIntra_tu][depth_tu+1],sizeof(depth_left_up[IsIntra_tu][depth_tu]));
			memcpy(skip_flag_left_up[i][depth_tu],skip_flag_left_up[IsIntra_tu][depth_tu+1],sizeof(skip_flag_left_up[IsIntra_tu][depth_tu]));

			memcpy(mModels[i][depth_tu] , mModels[IsIntra_tu][depth_tu+1] , sizeof(mModels[IsIntra_tu][depth_tu]));
		}
	}
}

uint32_t CABAC_RDO::Est_bit_pu_merge(uint32_t  depth , uint32_t  merge_idx)//PU级别INTER  merge模式bit估计
{
	int bit;
	int MAX_merge_idx = 0;
	if (MAX_merge_idx==1)
	{
		bit = 0;
	}
	else
	{
		bit = merge_idx == 0 ? est_bit_merge_idx[depth][0] : est_bit_merge_idx[depth][1] ;
		if (merge_idx != MAX_merge_idx )
			bit+=merge_idx;
		else
			bit+=(merge_idx-1);
	}

	return est_bit_merge_flag[depth][1] + est_bit_cu_skip_flag[0][depth][0] + est_bit_pred_mode_flag[0][depth][0] + est_bit_pu_part_mode[0][depth][1] + bit;
}

uint32_t CABAC_RDO::Est_bit_pu_fme(uint32_t  depth , uint32_t  mvd , uint32_t  mvp_l0_flag)//PU级别INTER  FME模式bit估计
{
//	int temp1 = est_bit_cu_skip_flag[0][depth][0] + est_bit_pred_mode_flag[0][depth] + est_bit_pu_part_mode[0][depth][1] + est_bit_merge_idx[depth][0] + est_bit_rqt_root_cbf[depth][0] + est_bit_mvp_l0_flag[depth][mvp_l0_flag];
	int temp2 = 0;
	mvp_l0_flag = 0 ;//为了消除警告

	float x = abs((float)mvd);
	if (x ==0)
	{
		temp2 = est_bit_abs_mvd_greater0_flag[depth][0];
	}
	else if (x == 1)
	{
		temp2 = est_bit_abs_mvd_greater0_flag[depth][1] + est_bit_abs_mvd_greater0_flag[depth][0] + 1;//最后一个1是符号位消耗的bit
	}
	else 
	{
		temp2 = est_bit_abs_mvd_greater0_flag[depth][1] + est_bit_abs_mvd_greater0_flag[depth][1] +  ((int)(log(x)/log(2.0))>>1) + 1  +1 ;
	}
	return temp2;
}

uint32_t CABAC_RDO::Est_bit_sao(SaoParam_CABAC  saoParam , int cIdx , int Y_type)//SAO级别模式bit估计  cIdx 0 1 2分别表示Y U V
{
	int temp1 = 0;//merge left 带来的bit消耗
	int temp2 = 0;//merge left up 带来的bit消耗
	int temp3 = 0;// offset sign  band_position  eo_class带来的bit消耗
	int temp4 = 0;//temp4+1 表示做不做SAO带来的bit消耗

	int sao_merge_left_flag_enable = 0;//通过维护的状态判断是否允许做merge
	int sao_merge_up_flag_enable = 0;
	int MAX_sao_offset_abs = 0;

	if (cIdx == 0)//merge对YUV是同时生效的
	{
		if (sao_merge_left_flag_enable == 1)
		{
			if (saoParam.mergeLeftFlag == 1)
				return est_bit_sao_merge_left_flag[1];
			else
				temp1 = est_bit_sao_merge_left_flag[0];
		} 
		temp2 = temp1;
		if (sao_merge_up_flag_enable == 1)
		{
			if (saoParam.mergeUpFlag == 1)
				if (sao_merge_left_flag_enable == 1)
					return est_bit_sao_merge_up_flag[1] + temp1;
				else
					return est_bit_sao_merge_left_flag[1];
			else
				if (sao_merge_left_flag_enable == 1)
					temp2 = est_bit_sao_merge_up_flag[0] + temp1;
				else
					temp2 =  est_bit_sao_merge_left_flag[0];
		}
	}

	if (saoParam.typeIdx == -1)//不做SAO
	{
		if (cIdx == 0)
			return est_bit_sao_type_idx_luma[0] +temp2;
		else if (cIdx == 1)
		{
			if (Y_type == -1)
				return est_bit_sao_type_idx_chroma[0][0];
			else
				return est_bit_sao_type_idx_chroma[1][0];
		}
		else
			return 0;
	}
	else//做SAO
	{
		if (cIdx == 0)
			temp4 = est_bit_sao_type_idx_luma[1];
		else if (cIdx == 1)
		{
			if (Y_type == -1)
				temp4 = est_bit_sao_type_idx_chroma[0][1];
			else
				temp4 = est_bit_sao_type_idx_chroma[1][1];
		}
	}

	for (int i = 0 ; i <4 ; i++)
	{
		temp3 += (abs(saoParam.offset[i])+1);
		if (abs(saoParam.offset[i]) == MAX_sao_offset_abs)
			temp3--;
	}

	if (saoParam.typeIdx == 4)//BO类型
	{
		for (int i = 0 ; i <4 ; i++ )
		{
			if (abs(saoParam.offset[i]) != 0)
				temp3++;
		}
		return temp2 + temp3 + 5 + temp4 + 1;
	}
	else//EO类型
	{
		temp3 += (saoParam.typeIdx + 1);
		if (saoParam.typeIdx == 3)
			temp3 -=1;
		return temp2 +temp3 + temp4 + 1;
	}
}

uint32_t CABAC_RDO::Est_bit_tu(uint32_t  depth , int  IsIntra)//TU级别bit估计
{
	return est_bit_tu[IsIntra][depth];
}

void CABAC_RDO::cabac_rdo_status(uint32_t depth,int IsIntra , bool end_of_cabac_rdo_status )
{
	//	ContextModel_hm *mModels[CONTEXT_NUM];
	//	*mModels = *mModels[IsIntra][depth];


	//	int current_status[IsIntra][depth] = CTU_EST;
	//	int next_status[IsIntra][depth] = CTU_EST;
	//	current_status[IsIntra][depth] = next_status[IsIntra][depth];
	depth_tu = depth;
	IsIntra_tu = IsIntra;
	int temp_x;//wire
	int temp_y;//wire
	bool T1 , /*T2 ,*/  TT1 , TT2 , TT3;
	//	
	uint32_t ctxIdx[2][RDO_MAX_DEPTH][CONTEXT_NUM]   = {0} ;  //wire
	UChar mState[2][RDO_MAX_DEPTH][CONTEXT_NUM]    = {0};  //wire
	// 	uint32_t x_offset[2][RDO_MAX_DEPTH]   = {0} ;   //reg
	// 	uint32_t y_offset[2][RDO_MAX_DEPTH]   = {0} ;   //reg
	int preds[RDO_MAX_DEPTH][3] = {0};//wire

	while(end_of_cabac_rdo_status==0)
	{
		current_status[IsIntra][depth] = next_status[IsIntra][depth];
		switch (current_status[IsIntra][depth])
		{
		case CTU_EST:
			if (ctu_start_flag==0)
			{
				next_status[IsIntra][depth] = CTU_EST;
			}
			else if (ctu_start_flag==1)
			{
				next_status[IsIntra][depth] = CU_EST;
				cu_ready[IsIntra][depth] = 1;
				cu_cnt[IsIntra][depth] = 0;
				inx_in_cu[IsIntra][depth] = 0;
				inx_in_tu[IsIntra][depth] = 0;
				x_cu_left_up_in_pic[IsIntra][depth] = x_ctu_left_up_in_pic;
				y_cu_left_up_in_pic[IsIntra][depth] = y_ctu_left_up_in_pic;
			}
			end_of_cabac_rdo_status =1; 
			break;

		case CU_EST:
			//transquant_bypass_enabled_flag
			if (sps_pps_struct.transquant_bypass_enabled_flag)
			{
				mState[IsIntra][depth][CU_TRANSQUANT_BYPASS_FLAG_OFFSET] = mModels[IsIntra][depth][CU_TRANSQUANT_BYPASS_FLAG_OFFSET].get_m_ucState();
				est_bit_cu_transquant_bypass_flag[IsIntra][depth][0] = CABAC_g_entropyBits_0[mState[IsIntra][depth][CU_TRANSQUANT_BYPASS_FLAG_OFFSET]];
				est_bit_cu_transquant_bypass_flag[IsIntra][depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][CU_TRANSQUANT_BYPASS_FLAG_OFFSET]];
				mstate_cu_transquant_bypass_flag[IsIntra][depth][0] = aucNextState_0[mState[IsIntra][depth][CU_TRANSQUANT_BYPASS_FLAG_OFFSET]];
				mstate_cu_transquant_bypass_flag[IsIntra][depth][1] = aucNextState_1[mState[IsIntra][depth][CU_TRANSQUANT_BYPASS_FLAG_OFFSET]];
			}
			//cu_skip_flag
			if (slice_header_struct.slice_type != I_SLICE)//可以不要条件
			{
				ctxIdx[IsIntra][depth][SKIP_FLAG_OFFSET] =  getCtxSkipFlag(inx_in_cu[IsIntra][depth],depth,IsIntra);
				mState[IsIntra][depth][SKIP_FLAG_OFFSET +ctxIdx[IsIntra][depth][SKIP_FLAG_OFFSET]] = mModels[IsIntra][depth][SKIP_FLAG_OFFSET +ctxIdx[IsIntra][depth][SKIP_FLAG_OFFSET]].get_m_ucState();
				est_bit_cu_skip_flag[IsIntra][depth][0] = CABAC_g_entropyBits_0[mState[IsIntra][depth][SKIP_FLAG_OFFSET +ctxIdx[IsIntra][depth][SKIP_FLAG_OFFSET]]];
				est_bit_cu_skip_flag[IsIntra][depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][SKIP_FLAG_OFFSET +ctxIdx[IsIntra][depth][SKIP_FLAG_OFFSET]]];
				mstate_cu_skip_flag[IsIntra][depth][0] = aucNextState_0[mState[IsIntra][depth][SKIP_FLAG_OFFSET +ctxIdx[IsIntra][depth][SKIP_FLAG_OFFSET]]];
				mstate_cu_skip_flag[IsIntra][depth][1] = aucNextState_1[mState[IsIntra][depth][SKIP_FLAG_OFFSET +ctxIdx[IsIntra][depth][SKIP_FLAG_OFFSET]]];
			}
			else
			{
				est_bit_cu_skip_flag[IsIntra][depth][0] = 0;
			}
			//pred_mode_flag 
			if (slice_header_struct.slice_type != I_SLICE)
			{
				mState[IsIntra][depth][PRED_MODE_OFFSET] = mModels[IsIntra][depth][PRED_MODE_OFFSET].get_m_ucState();
				est_bit_pred_mode_flag[IsIntra][depth][0] = CABAC_g_entropyBits_0[mState[IsIntra][depth][PRED_MODE_OFFSET]];
				mstate_pred_mode_flag[IsIntra][depth][0] = aucNextState_0[mState[IsIntra][depth][PRED_MODE_OFFSET]];
				est_bit_pred_mode_flag[IsIntra][depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][PRED_MODE_OFFSET]];
				mstate_pred_mode_flag[IsIntra][depth][1] = aucNextState_1[mState[IsIntra][depth][PRED_MODE_OFFSET]];
				
			}
			else
			{
				est_bit_pred_mode_flag[IsIntra][depth][1] = 0;
			}

			//cu_split_flag
			ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET] =  getCtxSplitFlag(inx_in_cu[IsIntra][depth] , depth,IsIntra);
			mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]] = mModels[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]].get_m_ucState();
			if (depth == 0)
			{
				est_bit_cu_spit_flag[IsIntra][depth] = CABAC_g_entropyBits_0[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
				mstate_cu_spit_flag[IsIntra][depth] = aucNextState_0[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
			} 
			else if(depth == 1)
			{
				if (cu_cnt[IsIntra][depth] == 0)
				{
					est_bit_cu_spit_flag[IsIntra][depth] = CABAC_g_entropyBits_10[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
					mstate_cu_spit_flag[IsIntra][depth] = aucNextState_10[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
				} 
				else if (cu_cnt[IsIntra][depth] != 0)
				{
					est_bit_cu_spit_flag[IsIntra][depth] = CABAC_g_entropyBits_0[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
					mstate_cu_spit_flag[IsIntra][depth] = aucNextState_0[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
				}
			}
			else if (depth == 2)
			{
				if (cu_cnt[IsIntra][depth] == 0)
				{
					est_bit_cu_spit_flag[IsIntra][depth] = CABAC_g_entropyBits_110[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
					mstate_cu_spit_flag[IsIntra][depth] = aucNextState_110[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
				}
				else if (cu_cnt[IsIntra][depth] == 4)
				{
					est_bit_cu_spit_flag[IsIntra][depth] = CABAC_g_entropyBits_10[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
					mstate_cu_spit_flag[IsIntra][depth] = aucNextState_10[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
				}
				else 
				{
					est_bit_cu_spit_flag[IsIntra][depth] = CABAC_g_entropyBits_0[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
					mstate_cu_spit_flag[IsIntra][depth] = aucNextState_0[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
				}
			} 
			else if ((depth == 3) || (depth == 4) )
			{
				if (cu_cnt[IsIntra][depth] == 0)
				{
					est_bit_cu_spit_flag[IsIntra][depth] = CABAC_g_entropyBits_111[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
					mstate_cu_spit_flag[IsIntra][depth] = aucNextState_111[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
				}
				else if (cu_cnt[IsIntra][depth] == 4)
				{
					est_bit_cu_spit_flag[IsIntra][depth] = CABAC_g_entropyBits_1[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
					mstate_cu_spit_flag[IsIntra][depth] = aucNextState_1[mState[IsIntra][depth][SPLIT_CU_FLAG_OFFSET+ctxIdx[IsIntra][depth][SPLIT_CU_FLAG_OFFSET]]];
				}
			}


			T1 = cu_ready[IsIntra][depth]==1 ;
			TT1 = (T1 && depth!=0 &&  cu_cnt[IsIntra][0] != 1 )||(cu_cnt[IsIntra][0]==0 &&depth==0);
			TT2 = (depth!=0 &&  cu_cnt[IsIntra][0] ==1 ) ||(cu_cnt[IsIntra][0]==1 &&depth == 0 );//back to ctu state or
			TT3 = (!T1 && depth!=0 &&  cu_cnt[IsIntra][0] != 1);



			 temp_x = cu_ZscanToXpixel[inx_in_cu[IsIntra][depth]];
			 temp_y = cu_ZscanToYpixel[inx_in_cu[IsIntra][depth]];
			cu_in_pic_flag[IsIntra][depth] =( (( x_ctu_left_up_in_pic +temp_x+ cu_pixel_wide[depth] -1) <sps_pps_struct.pic_width_in_luma_samples) && (( y_ctu_left_up_in_pic + temp_y+cu_pixel_wide[depth] -1) <sps_pps_struct.pic_height_in_luma_samples) );

			if (!cu_in_pic_flag[IsIntra][depth] && !TT3)//判断的同时把是否超过边界的标记存在cu_in_pic_flag[IsIntra][depth]寄存器中，供状态更新的时候用
			{
				cu_ready[IsIntra][depth] = 0;
				pu_cnt[IsIntra][depth] = 0;
				if (cu_cnt[IsIntra][depth]==4&&depth!=0)
				{
					cu_cnt[IsIntra][depth]=0;
				}

				inx_in_tu[IsIntra][depth] =  cu_to_tu_idx(inx_in_cu[IsIntra][depth]);
			
				next_status[IsIntra][depth] = CU_EST_WAIT;
			}
			else if (TT1 == 1 && cu_in_pic_flag)
			{
				cur_mode[IsIntra][depth] = depth_to_cur_mode[IsIntra][depth];
				next_status[IsIntra][depth] = PU_EST_WAIT;
				max_pu_cnt[IsIntra][depth] = pu_cnt_table[cur_mode[IsIntra][depth]];
				max_tu_cnt[IsIntra][depth] = tu_cnt_table[cur_mode[IsIntra][depth]];
				cu_ready[IsIntra][depth] = 0;
				pu_cnt[IsIntra][depth] = 0;
				if (cu_cnt[IsIntra][depth]==4&&depth!=0)
				{
					cu_cnt[IsIntra][depth]=0;
				}
				inx_in_tu[IsIntra][depth] =  cu_to_tu_idx(inx_in_cu[IsIntra][depth]);
			}
			else if (TT3 == 1 )
			{
				next_status[IsIntra][depth] = CU_EST;
			} 
			else if(TT2 == 1 && cu_in_pic_flag)
			{
				next_status[IsIntra][depth] = CTU_EST;
			}

			if (depth != 0 )
			{
				if(cu_ready[IsIntra][depth-1]==1) 
					cu_ready[IsIntra][depth]=1;
			}

			tu_luma_idx = 0;
			end_of_cabac_rdo_status =1; 
			break;

		case PU_EST_WAIT:
			mState[IsIntra][depth][PART_MODE_OFFSET] = mModels[IsIntra][depth][PART_MODE_OFFSET].get_m_ucState();
			if (IsIntra == 1)
			{
				getIntraDirLumaPredictor(inx_in_tu[IsIntra][depth], &preds[depth][0],pu_cnt[IsIntra][depth],depth,IsIntra);
				mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE] = mModels[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE].get_m_ucState();
				for (Int IntraDirLumaAng = 0 ; IntraDirLumaAng < 35 ;IntraDirLumaAng++)
				{
					if (IntraDirLumaAng == preds[depth][0])
					{
						est_bit_pu_luma_dir[depth][IntraDirLumaAng] = 32768 + CABAC_g_entropyBits_1[mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE]];
						//						mstate_prev_intra_luma_pred_flag[depth] =  aucNextState_1[mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE]];
					} 
					else if (IntraDirLumaAng == preds[depth][1] || IntraDirLumaAng == preds[depth][2])
					{
						est_bit_pu_luma_dir[depth][IntraDirLumaAng] = 65536 + CABAC_g_entropyBits_1[mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE]];
						//						mstate_prev_intra_luma_pred_flag[depth] =  aucNextState_1[mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE]];
					}
					else
					{
						est_bit_pu_luma_dir[depth][IntraDirLumaAng] = 163840 + CABAC_g_entropyBits_0[mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE]];
						//						mstate_prev_intra_luma_pred_flag[depth] =  aucNextState_0[mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE]];
					}
//					assert(est_bit_pu_luma_dir[depth][IntraDirLumaAng] == g_intra_est_bit_luma_pred_mode_all_case[depth][inx_in_tu[IsIntra][depth]][IntraDirLumaAng]);

				}
				//chroma dir
				mState[IsIntra][depth][INTRA_CHROMA_PRED_MODE_OFFSET] = mModels[IsIntra][depth][INTRA_CHROMA_PRED_MODE_OFFSET].get_m_ucState();
				est_bit_pu_chroma_dir[depth] = CABAC_g_entropyBits_0[mState[IsIntra][depth][INTRA_CHROMA_PRED_MODE_OFFSET]];
				mstate_pu_chroma_dir[depth] = aucNextState_0[mState[IsIntra][depth][INTRA_CHROMA_PRED_MODE_OFFSET]];

				if (cur_mode[IsIntra][depth] == 0)//实际上不用作条件判断，把0和1的情况bit、状态转移都预先准备就好了，并且PU重复进来的时候不会有电平变换，功耗是一样的。还省去了条件判断消耗的资源。
				{
					est_bit_pu_part_mode[IsIntra][depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][PART_MODE_OFFSET]];
					mstate_pu_part_mode[IsIntra][depth][1] = aucNextState_1[mState[IsIntra][depth][PART_MODE_OFFSET]];
				} 
				else if (cur_mode[IsIntra][depth] == 2 && pu_cnt[IsIntra][depth] == 0)
				{
					est_bit_pu_part_mode[IsIntra][depth][0] = CABAC_g_entropyBits_0[mState[IsIntra][depth][PART_MODE_OFFSET]];
					mstate_pu_part_mode[IsIntra][depth][0] = aucNextState_0[mState[IsIntra][depth][PART_MODE_OFFSET]];
				}
			} 
			else if (IsIntra == 0)
			{  //pu_part_mode
				est_bit_pu_part_mode[IsIntra][depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][PART_MODE_OFFSET]];
				mstate_pu_part_mode[IsIntra][depth][1] = aucNextState_1[mState[IsIntra][depth][PART_MODE_OFFSET]];
				//merge_flag
				mState[IsIntra][depth][MERGE_FLAG_OFFSET] = mModels[IsIntra][depth][MERGE_FLAG_OFFSET].get_m_ucState();
				est_bit_merge_flag[depth][0] = CABAC_g_entropyBits_0[mState[IsIntra][depth][MERGE_FLAG_OFFSET]];
				est_bit_merge_flag[depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][MERGE_FLAG_OFFSET]];
				mstate_merge_flag[depth][0] = aucNextState_0[mState[IsIntra][depth][MERGE_FLAG_OFFSET]];
				mstate_merge_flag[depth][1] = aucNextState_1[mState[IsIntra][depth][MERGE_FLAG_OFFSET]];

				//inter_pred_idc
				if (slice_header_struct.slice_type == B_SLICE)
				{
				}
				//ref_idx_l0/1 
				if (sps_pps_struct.num_ref_idx_l0_default_active || sps_pps_struct.num_ref_idx_l1_default_active)
				{
				}
				//merge_idx
				if (slice_header_struct.Five_minus_max_num_merge_cand<4)//相当于协议的MaxNumMergeCand > 1
				{
					mState[IsIntra][depth][MERGE_IDX_OFFSET] = mModels[IsIntra][depth][MERGE_IDX_OFFSET].get_m_ucState();
					est_bit_merge_idx[depth][0] = CABAC_g_entropyBits_0[mState[IsIntra][depth][MERGE_IDX_OFFSET]];
					est_bit_merge_idx[depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][MERGE_IDX_OFFSET]];
					mstate_merge_idx[depth][0] = aucNextState_0[mState[IsIntra][depth][MERGE_IDX_OFFSET]];
					mstate_merge_idx[depth][1] = aucNextState_1[mState[IsIntra][depth][MERGE_IDX_OFFSET]];
				}

				//mvp_l0_flag
				mState[IsIntra][depth][MVP_FLAG_OFFSET] = mModels[IsIntra][depth][MVP_FLAG_OFFSET].get_m_ucState();
				est_bit_mvp_l0_flag[depth][0] = CABAC_g_entropyBits_0[mState[IsIntra][depth][MVP_FLAG_OFFSET]];
				est_bit_mvp_l0_flag[depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][MVP_FLAG_OFFSET]];
				mstate_mvp_l0_flag[depth][0] = aucNextState_0[mState[IsIntra][depth][MVP_FLAG_OFFSET]];
				mstate_mvp_l0_flag[depth][1] = aucNextState_1[mState[IsIntra][depth][MVP_FLAG_OFFSET]];

				//abs_mvd_greater0_flag
				mState[IsIntra][depth][ABS_MVD_GREATER0_FLAG_OFFSET] = mModels[IsIntra][depth][ABS_MVD_GREATER0_FLAG_OFFSET].get_m_ucState();
				est_bit_abs_mvd_greater0_flag[depth][0] = CABAC_g_entropyBits_0[mState[IsIntra][depth][ABS_MVD_GREATER0_FLAG_OFFSET]];
				est_bit_abs_mvd_greater0_flag[depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][ABS_MVD_GREATER0_FLAG_OFFSET]];
				mstate_abs_mvd_greater0_flag[depth][0] = aucNextState_0[mState[IsIntra][depth][ABS_MVD_GREATER0_FLAG_OFFSET]];
				mstate_abs_mvd_greater0_flag[depth][1] = aucNextState_1[mState[IsIntra][depth][ABS_MVD_GREATER0_FLAG_OFFSET]];

				//abs_mvd_greater1_flag
				mState[IsIntra][depth][ABS_MVD_GREATER1_FLAG_OFFSET] = mModels[IsIntra][depth][ABS_MVD_GREATER1_FLAG_OFFSET].get_m_ucState();
				est_bit_abs_mvd_greater1_flag[depth][0] = CABAC_g_entropyBits_0[mState[IsIntra][depth][ABS_MVD_GREATER1_FLAG_OFFSET]];
				est_bit_abs_mvd_greater1_flag[depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][ABS_MVD_GREATER1_FLAG_OFFSET]];
				mstate_abs_mvd_greater1_flag[depth][0] = aucNextState_0[mState[IsIntra][depth][ABS_MVD_GREATER1_FLAG_OFFSET]];
				mstate_abs_mvd_greater1_flag[depth][1] = aucNextState_1[mState[IsIntra][depth][ABS_MVD_GREATER1_FLAG_OFFSET]];

				//rqt_root_cbf
				mState[IsIntra][depth][RQT_ROOT_CBF_OFFSET] = mModels[IsIntra][depth][RQT_ROOT_CBF_OFFSET].get_m_ucState();
				est_bit_rqt_root_cbf[depth][0] = CABAC_g_entropyBits_0[mState[IsIntra][depth][RQT_ROOT_CBF_OFFSET]];
				est_bit_rqt_root_cbf[depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][RQT_ROOT_CBF_OFFSET]];
				mstate_rqt_root_cbf[depth][0] = aucNextState_0[mState[IsIntra][depth][RQT_ROOT_CBF_OFFSET]];
				mstate_rqt_root_cbf[depth][1] = aucNextState_1[mState[IsIntra][depth][RQT_ROOT_CBF_OFFSET]];
			}
			//cbf_luma

			mState[IsIntra][depth][CBF_LUMA_OFFSET] = mModels[IsIntra][depth][CBF_LUMA_OFFSET+ (depth!=4)].get_m_ucState();
			est_bit_cbf_luma[IsIntra][depth][0] = CABAC_g_entropyBits_0[mState[IsIntra][depth][CBF_LUMA_OFFSET]];
			est_bit_cbf_luma[IsIntra][depth][1] = CABAC_g_entropyBits_1[mState[IsIntra][depth][CBF_LUMA_OFFSET]];
			mstate_cbf_luma[IsIntra][depth][0] = aucNextState_0[mState[IsIntra][depth][CBF_LUMA_OFFSET]];
			mstate_cbf_luma[IsIntra][depth][1] = aucNextState_1[mState[IsIntra][depth][CBF_LUMA_OFFSET]];

			mState[IsIntra][depth][CBF_CHROMA_OFFSET] = mModels[IsIntra][depth][CBF_CHROMA_OFFSET/*+ (depth==4)*/].get_m_ucState();
			est_bit_cbf_chroma[IsIntra][depth][0][0] = CABAC_g_entropyBits_00[mState[IsIntra][depth][CBF_CHROMA_OFFSET]];
			est_bit_cbf_chroma[IsIntra][depth][0][1] = CABAC_g_entropyBits_01[mState[IsIntra][depth][CBF_CHROMA_OFFSET]];
			est_bit_cbf_chroma[IsIntra][depth][1][0] = CABAC_g_entropyBits_10[mState[IsIntra][depth][CBF_CHROMA_OFFSET]];
			est_bit_cbf_chroma[IsIntra][depth][1][1] = CABAC_g_entropyBits_11[mState[IsIntra][depth][CBF_CHROMA_OFFSET]];

			mstate_cbf_chroma[IsIntra][depth][0][0] = aucNextState_00[mState[IsIntra][depth][CBF_CHROMA_OFFSET]];
			mstate_cbf_chroma[IsIntra][depth][0][1] = aucNextState_01[mState[IsIntra][depth][CBF_CHROMA_OFFSET]];
			mstate_cbf_chroma[IsIntra][depth][1][0] = aucNextState_10[mState[IsIntra][depth][CBF_CHROMA_OFFSET]];
			mstate_cbf_chroma[IsIntra][depth][1][1] = aucNextState_11[mState[IsIntra][depth][CBF_CHROMA_OFFSET]];



			if (pu_best_mode_flag[IsIntra][depth] == 0)
			{
				next_status[IsIntra][depth] = PU_EST_WAIT;
			} 
			else if (pu_best_mode_flag[IsIntra][depth] == 1)
			{
				pu_best_mode_flag[IsIntra][depth] = 0 ;
				next_status[IsIntra][depth] = PU_EST;
				if (intra_pu_depth_luma_bestDir[depth] == preds[depth][0] || intra_pu_depth_luma_bestDir[depth] == preds[depth][1]
				|| intra_pu_depth_luma_bestDir[depth] == preds[depth][2])
					mstate_prev_intra_luma_pred_flag[depth] =  aucNextState_1[mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE]];
				else
					mstate_prev_intra_luma_pred_flag[depth] =  aucNextState_0[mState[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE]];
			}
			//			pu_mode_bit_ready[IsIntra][depth]=1;
			end_of_cabac_rdo_status =1; 
			break;

		case PU_EST:
			
			if (IsIntra == 1)
			{
				if (depth != 4 && depth != 3)
				{
					est_bit_pu_part_mode_sure[IsIntra][depth] = 0;
				}
				else if (cur_mode[IsIntra][depth] == 0)
				{
					est_bit_pu_part_mode_sure[IsIntra][depth] = est_bit_pu_part_mode[IsIntra][depth][1] ;
					mModels[IsIntra][depth][PART_MODE_OFFSET].set_m_ucState(mstate_pu_part_mode[IsIntra][depth][1]);
				} 
				else if (cur_mode[IsIntra][depth] == 2 && pu_cnt[IsIntra][depth] == 0)
				{
					est_bit_pu_part_mode_sure[IsIntra][depth] = est_bit_pu_part_mode[IsIntra][depth][0] ;
					mModels[IsIntra][depth][PART_MODE_OFFSET].set_m_ucState(mstate_pu_part_mode[IsIntra][depth][0]);
				}
				else
				{
					est_bit_pu_part_mode_sure[IsIntra][depth] = 0;
				}
				if (depth == 4 && pu_cnt[IsIntra][depth] != 0)
				{
					est_bit_pu_chroma_dir[depth] = 0;
				}
				
				pu_est_bits[IsIntra][depth] = (pu_est_bits[IsIntra][depth] + est_bit_pu_luma_dir[depth][intra_pu_depth_luma_bestDir[depth]]+est_bit_pu_chroma_dir[depth]+est_bit_pu_part_mode_sure[IsIntra][depth] + est_bit_pred_mode_flag[IsIntra][depth][1] + est_bit_cu_skip_flag[IsIntra][depth][0]);
			} 
			else
			{
			}

			next_status[IsIntra][depth] = TU_EST_WAIT;
			tu_cnt[IsIntra][depth] = 0;
			end_of_cabac_rdo_status =1; 
			
			break;

		case TU_EST_WAIT:
			if (quant_finish_flag[IsIntra][depth] == 0)
			{
				next_status[IsIntra][depth] = TU_EST_WAIT;
			} 
			else if (quant_finish_flag[IsIntra][depth] == 1)
			{
//				IsIntra_tu = IsIntra ;
//				depth_tu = depth;



				//luma
				if (cbf_luma == 1)
				{
					cabac_est( TU_chroma_size *2, TU_Resi_luma , 0);
					if (slice_header_struct.slice_type == 2)
					{
						assert((est_bit_last_sig_coeff_x_prefix[IsIntra_tu][depth_tu] +
							est_bit_last_sig_coeff_y_prefix[IsIntra_tu][depth_tu] +
							est_bit_last_sig_coeff_x_suffix[IsIntra_tu][depth_tu] +
							est_bit_last_sig_coeff_y_suffix[IsIntra_tu][depth_tu] ) == g_intra_est_bit_last_sig_coeff_xy[0][depth][inx_in_tu[IsIntra][depth]]);
						assert(est_bit_sig_coeff_flag[IsIntra_tu][depth_tu] == g_intra_est_sig_coeff_flag[0][depth][inx_in_tu[IsIntra][depth]]);
						assert(est_bit_coeff_abs_level_greater1_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_greater1_flag[0][depth][inx_in_tu[IsIntra][depth]]);
						assert(est_bit_coeff_abs_level_greater2_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_greater2_flag[0][depth][inx_in_tu[IsIntra][depth]]);
						assert(est_bit_coded_sub_block_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coded_sub_block_flag[0][depth][inx_in_tu[IsIntra][depth]]);
						assert(est_bit_coeff_sign_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_sign_flag[0][depth][inx_in_tu[IsIntra][depth]]);
						assert(est_bit_coeff_abs_level_remaining[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_remaining[0][depth][inx_in_tu[IsIntra][depth]]);

						assert( (est_bit_cbf_luma[IsIntra_tu][depth_tu][1]) == g_intra_est_bit_cbf[0][depth][inx_in_tu[IsIntra][depth]] );
					}
					
					mModels[IsIntra][depth][CBF_LUMA_OFFSET+ (depth!=4)].set_m_ucState(mstate_cbf_luma[IsIntra][depth][1]);

					tu_est_bits[IsIntra][depth] += est_bit_cbf_luma[IsIntra_tu][depth_tu][1];
				}
				else
				{
					if (slice_header_struct.slice_type == 2)
					{
						assert( (est_bit_cbf_luma[IsIntra_tu][depth_tu][0]) == g_intra_est_bit_cbf[0][depth][inx_in_tu[IsIntra][depth]] );
					}
					
					mModels[IsIntra][depth][CBF_LUMA_OFFSET+ (depth!=4)].set_m_ucState(mstate_cbf_luma[IsIntra][depth][0]);

					tu_est_bits[IsIntra][depth] += est_bit_cbf_luma[IsIntra_tu][depth_tu][0];
				}
				
				if ( (tu_luma_idx ==3 || depth !=4) && cbf_chroma_u == 1)
				{	//chroma U
					if (depth == 4)
					{
						TU_chroma_size = 4;
					}
					cabac_est( TU_chroma_size , TU_Resi_chroma_u , 1);

					int idx_temp = inx_in_tu[IsIntra][depth];
					if (depth == 4)
					{
						idx_temp = inx_in_tu[IsIntra][depth] - 3;
					}
					if (slice_header_struct.slice_type == 2)
					{
						assert((est_bit_last_sig_coeff_x_prefix[IsIntra_tu][depth_tu] +
							est_bit_last_sig_coeff_y_prefix[IsIntra_tu][depth_tu] +
							est_bit_last_sig_coeff_x_suffix[IsIntra_tu][depth_tu] +
							est_bit_last_sig_coeff_y_suffix[IsIntra_tu][depth_tu] ) == (g_intra_est_bit_last_sig_coeff_xy[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_last_sig_coeff_xy[1][depth][idx_temp]));
						assert(est_bit_sig_coeff_flag[IsIntra_tu][depth_tu] == g_intra_est_sig_coeff_flag[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_sig_coeff_flag[1][depth][idx_temp]);
						assert(est_bit_coeff_abs_level_greater1_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_greater1_flag[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_coeff_abs_level_greater1_flag[1][depth][idx_temp]);
						assert(est_bit_coeff_abs_level_greater2_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_greater2_flag[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_coeff_abs_level_greater2_flag[1][depth][idx_temp]);
						assert(est_bit_coded_sub_block_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coded_sub_block_flag[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_coded_sub_block_flag[1][depth][idx_temp]);
						assert(est_bit_coeff_sign_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_sign_flag[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_coeff_sign_flag[1][depth][idx_temp]);
						assert(est_bit_coeff_abs_level_remaining[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_remaining[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_coeff_abs_level_remaining[1][depth][idx_temp]);
					}
				}
					
				if ( (tu_luma_idx ==3 || depth !=4) && cbf_chroma_v == 1)
				{	//chroma V
					if (depth == 4)
					{
						TU_chroma_size = 4;
					}
					cabac_est( TU_chroma_size , TU_Resi_chroma_v , 2);

					int idx_temp = inx_in_tu[IsIntra][depth];
					if (depth == 4)
					{
						idx_temp = inx_in_tu[IsIntra][depth] - 3;
					}
					if (slice_header_struct.slice_type == 2)
					{
						assert((est_bit_last_sig_coeff_x_prefix[IsIntra_tu][depth_tu] +
							est_bit_last_sig_coeff_y_prefix[IsIntra_tu][depth_tu] +
							est_bit_last_sig_coeff_x_suffix[IsIntra_tu][depth_tu] +
							est_bit_last_sig_coeff_y_suffix[IsIntra_tu][depth_tu] ) == (g_intra_est_bit_last_sig_coeff_xy[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_last_sig_coeff_xy[1][depth][idx_temp]
						+ g_intra_est_bit_last_sig_coeff_xy[2][depth][idx_temp]));
						assert(est_bit_sig_coeff_flag[IsIntra_tu][depth_tu] == g_intra_est_sig_coeff_flag[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_sig_coeff_flag[1][depth][idx_temp]
						+ g_intra_est_sig_coeff_flag[2][depth][idx_temp]);
						assert(est_bit_coeff_abs_level_greater1_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_greater1_flag[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_coeff_abs_level_greater1_flag[1][depth][idx_temp]
						+ g_intra_est_bit_coeff_abs_level_greater1_flag[2][depth][idx_temp]);
						assert(est_bit_coeff_abs_level_greater2_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_greater2_flag[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_coeff_abs_level_greater2_flag[1][depth][idx_temp]
						+ g_intra_est_bit_coeff_abs_level_greater2_flag[2][depth][idx_temp]);
						assert(est_bit_coded_sub_block_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coded_sub_block_flag[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_coded_sub_block_flag[1][depth][idx_temp]
						+ g_intra_est_bit_coded_sub_block_flag[2][depth][idx_temp]);
						assert(est_bit_coeff_sign_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_sign_flag[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_coeff_sign_flag[1][depth][idx_temp]
						+ g_intra_est_bit_coeff_sign_flag[2][depth][idx_temp]);
						assert(est_bit_coeff_abs_level_remaining[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_remaining[0][depth][inx_in_tu[IsIntra][depth]]
						+ g_intra_est_bit_coeff_abs_level_remaining[1][depth][idx_temp]
						+ g_intra_est_bit_coeff_abs_level_remaining[2][depth][idx_temp]);
					}
					
				}
				
				if ( (tu_luma_idx ==3 || depth !=4) )
				{
					int idx_temp = inx_in_tu[IsIntra][depth];
					if (depth == 4)
					{
						idx_temp = inx_in_tu[IsIntra][depth] - 3;
					}
					switch ((cbf_chroma_u << 1) | cbf_chroma_v)
					{
					case 0: 
						if (slice_header_struct.slice_type == 2)
						{
							assert( (est_bit_cbf_chroma[IsIntra_tu][depth_tu][0][0]) == g_intra_est_bit_cbf[1][depth][idx_temp] + g_intra_est_bit_cbf[2][depth][idx_temp]);
						}
						
						mModels[IsIntra][depth][CBF_CHROMA_OFFSET].set_m_ucState(mstate_cbf_chroma[IsIntra][depth][0][0]);

						tu_est_bits[IsIntra][depth] += est_bit_cbf_chroma[IsIntra_tu][depth_tu][0][0];
						break;
					case 1:
						if (slice_header_struct.slice_type == 2)
						{
							assert( (est_bit_cbf_chroma[IsIntra_tu][depth_tu][0][1]) == g_intra_est_bit_cbf[1][depth][idx_temp] + g_intra_est_bit_cbf[2][depth][idx_temp]);
						}
						
						mModels[IsIntra][depth][CBF_CHROMA_OFFSET].set_m_ucState(mstate_cbf_chroma[IsIntra][depth][0][1]);

						tu_est_bits[IsIntra][depth] += est_bit_cbf_chroma[IsIntra_tu][depth_tu][0][1];
						break;
					case 2:
						if (slice_header_struct.slice_type == 2)
						{
							assert( (est_bit_cbf_chroma[IsIntra_tu][depth_tu][1][0]) == g_intra_est_bit_cbf[1][depth][idx_temp] + g_intra_est_bit_cbf[2][depth][idx_temp]);
						}
						
						mModels[IsIntra][depth][CBF_CHROMA_OFFSET].set_m_ucState(mstate_cbf_chroma[IsIntra][depth][1][0]);

						tu_est_bits[IsIntra][depth] += est_bit_cbf_chroma[IsIntra_tu][depth_tu][1][0];
						break;
					case 3:
						if (slice_header_struct.slice_type == 2)
						{
							assert( (est_bit_cbf_chroma[IsIntra_tu][depth_tu][1][1]) == g_intra_est_bit_cbf[1][depth][idx_temp] + g_intra_est_bit_cbf[2][depth][idx_temp]);
						}
						
						mModels[IsIntra][depth][CBF_CHROMA_OFFSET].set_m_ucState(mstate_cbf_chroma[IsIntra][depth][1][1]);

						tu_est_bits[IsIntra][depth] += est_bit_cbf_chroma[IsIntra_tu][depth_tu][1][1];
						break;
					default:
						assert(0);
					}
				}
// 				else
// 				{
// 					mModels[IsIntra][4][CBF_CHROMA_OFFSET].m_ucState = mModels[IsIntra][3][CBF_CHROMA_OFFSET].m_ucState;
// 				}


				if (depth==4)
				{
					tu_luma_idx += 1;
				}
				

				quant_finish_flag[IsIntra][depth] = 0;
				next_status[IsIntra][depth] = TU_EST;
			}
			end_of_cabac_rdo_status = 1 ;
			break;

		case TU_EST:
			if (tu_cnt[IsIntra][depth]!=max_tu_cnt[IsIntra][depth]-1)
			{
				next_status[IsIntra][depth] = TU_EST_WAIT;
				tu_cnt[IsIntra][depth]++;
			} 
			else if((pu_cnt[IsIntra][depth]!=max_pu_cnt[IsIntra][depth]-1)&&(tu_cnt[IsIntra][depth]==max_tu_cnt[IsIntra][depth]-1))
			{
				mModels[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE].set_m_ucState(mstate_prev_intra_luma_pred_flag[depth]);
//				mModels[IsIntra][depth][INTRA_CHROMA_PRED_MODE_OFFSET].set_m_ucState(mstate_pu_chroma_dir[depth]);
				setLumaIntraDirSubParts(intra_pu_depth_luma_bestDir[depth],inx_in_tu[IsIntra][depth],depth, IsIntra);
				setAvailSubParts_map(1,inx_in_tu[IsIntra][depth],depth, IsIntra);
				next_status[IsIntra][depth] = PU_EST_WAIT;
				pu_cnt[IsIntra][depth]++;
				inx_in_tu[IsIntra][depth] += 1;
			}
			else if ((pu_cnt[IsIntra][depth]==max_pu_cnt[IsIntra][depth]-1)&&(tu_cnt[IsIntra][depth]==max_tu_cnt[IsIntra][depth]-1))
			{
				mModels[IsIntra][depth][INTRA_CHROMA_PRED_MODE_OFFSET].set_m_ucState(mstate_pu_chroma_dir[depth]);
				next_status[IsIntra][depth] = CU_EST_WAIT;
				inx_in_tu[IsIntra][depth] += 1;

				assert(tu_est_bits[IsIntra][depth] == g_est_bit_tu[IsIntra][depth][inx_in_cu[IsIntra][depth] * 4]);
				if (slice_header_struct.slice_type == 2)
				{
					assert(tu_est_bits[IsIntra][depth] + pu_est_bits[IsIntra][depth] == g_est_bit_cu[IsIntra][depth][inx_in_cu[IsIntra][depth] * 4]);
					if (depth != 4)
					{
						assert(tu_est_bits[IsIntra][depth] == g_est_bit_tu_luma_NoCbf[IsIntra][depth][inx_in_cu[IsIntra][depth] * 4]
						+ g_est_bit_tu_cb_NoCbf[IsIntra][depth][inx_in_cu[IsIntra][depth] * 4]
						+ g_est_bit_tu_cr_NoCbf[IsIntra][depth][inx_in_cu[IsIntra][depth] * 4]
						+ g_intra_est_bit_cbf[0][depth][inx_in_cu[IsIntra][depth] * 4] 
						+ g_intra_est_bit_cbf[1][depth][inx_in_cu[IsIntra][depth] * 4]
						+ g_intra_est_bit_cbf[2][depth][inx_in_cu[IsIntra][depth] * 4] );
					}
					else
					{
						assert(tu_est_bits[IsIntra][depth] == g_est_bit_tu_luma_NoCbf[IsIntra][depth][inx_in_cu[IsIntra][depth] * 4]
						+g_est_bit_tu_luma_NoCbf[IsIntra][depth][inx_in_cu[IsIntra][depth] * 4 + 1]
						+g_est_bit_tu_luma_NoCbf[IsIntra][depth][inx_in_cu[IsIntra][depth] * 4 + 2]
						+g_est_bit_tu_luma_NoCbf[IsIntra][depth][inx_in_cu[IsIntra][depth] * 4 + 3]
						+ g_est_bit_tu_cb_NoCbf[IsIntra][depth][inx_in_cu[IsIntra][depth] * 4]
						+ g_est_bit_tu_cr_NoCbf[IsIntra][depth][inx_in_cu[IsIntra][depth] * 4]
						+ g_intra_est_bit_cbf[0][depth][inx_in_cu[IsIntra][depth] * 4] 
						+ g_intra_est_bit_cbf[0][depth][inx_in_cu[IsIntra][depth] * 4 + 1] 
						+ g_intra_est_bit_cbf[0][depth][inx_in_cu[IsIntra][depth] * 4 + 2] 
						+ g_intra_est_bit_cbf[0][depth][inx_in_cu[IsIntra][depth] * 4 + 3] 
						+ g_intra_est_bit_cbf[1][depth][inx_in_cu[IsIntra][depth] * 4]
						+ g_intra_est_bit_cbf[2][depth][inx_in_cu[IsIntra][depth] * 4] );
					}
				}
				
			}
			end_of_cabac_rdo_status = 1;
			break;

		case CU_EST_WAIT:
			if ( (cu_cnt[IsIntra][depth+1]!=4 || cu_best_mode_flag[depth] != 1 )&&depth!=4 )
			{
				next_status[IsIntra][depth] = CU_EST_WAIT;
			} 
			else if( (cu_cnt[IsIntra][depth+1]==4&&depth!=4 && cu_best_mode_flag[depth] == 1) || depth==4 )
			{
				if (IsIntra == 1)
				{
					if (depth == 4 && cu_in_pic_flag[IsIntra][depth] == 1)
					{
						setLumaIntraDirSubParts(intra_pu_depth_luma_bestDir[depth],inx_in_tu[IsIntra][depth]-1,depth, IsIntra);
						setPredModeSubParts(MODE_INTRA,inx_in_cu[IsIntra][depth],depth, IsIntra);
						setAvailSubParts_map(1,inx_in_tu[IsIntra][depth]-1,depth, IsIntra);
//						setDepthSubParts(depth,inx_in_cu[IsIntra][depth], IsIntra);
						setSkipFlagSubParts(0,inx_in_cu[IsIntra][depth],depth, IsIntra);
						mModels[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE].set_m_ucState(mstate_prev_intra_luma_pred_flag[depth]);
					} 
					else if (depth != 4)
					{
						if (cu_best_mode[depth] ==1 && cu_in_pic_flag[IsIntra][depth] == 1)
						{
							setLumaIntraDirSubParts(intra_pu_depth_luma_bestDir[depth],cu_to_tu_idx(inx_in_cu[IsIntra][depth]),depth, IsIntra);
							setPredModeSubParts(MODE_INTRA,inx_in_cu[IsIntra][depth],depth, IsIntra);
							setAvailSubParts_map(1,inx_in_tu[IsIntra][depth]-1,depth, IsIntra);
//							setDepthSubParts(depth,inx_in_cu[IsIntra][depth], IsIntra);
							setSkipFlagSubParts(0,inx_in_cu[IsIntra][depth],depth, IsIntra);
							mModels[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE].set_m_ucState(mstate_prev_intra_luma_pred_flag[depth]);
							for (int i = (depth+1) ; i < 5 ; i ++)
							{
								mModels[IsIntra][i][PREV_INTRA_LUMA_PRED_MODE].set_m_ucState(mstate_prev_intra_luma_pred_flag[depth]);
								memcpy(intra_luma_pred_mode_left_up[IsIntra][i],intra_luma_pred_mode_left_up[IsIntra][depth],sizeof(intra_luma_pred_mode_left_up[IsIntra][depth]));
								memcpy(pred_mode_left_up[IsIntra][i],pred_mode_left_up[IsIntra][depth],sizeof(pred_mode_left_up[IsIntra][depth]));
								memcpy(avail_left_up[IsIntra][i],avail_left_up[IsIntra][depth],sizeof(avail_left_up[IsIntra][depth]));
								memcpy(depth_left_up[IsIntra][i],depth_left_up[IsIntra][depth],sizeof(depth_left_up[IsIntra][depth]));
								memcpy(skip_flag_left_up[IsIntra][i],skip_flag_left_up[IsIntra][depth],sizeof(skip_flag_left_up[IsIntra][depth]));

								memcpy(mModels[IsIntra][i] ,mModels[IsIntra][depth], sizeof(mModels[IsIntra][i]) );

							}
						}
						else
						{
							mModels[IsIntra][depth][PREV_INTRA_LUMA_PRED_MODE] = mModels[IsIntra][depth+1][PREV_INTRA_LUMA_PRED_MODE];
							memcpy(intra_luma_pred_mode_left_up[IsIntra][depth],intra_luma_pred_mode_left_up[IsIntra][depth+1],sizeof(intra_luma_pred_mode_left_up[IsIntra][depth]));
							memcpy(pred_mode_left_up[IsIntra][depth],pred_mode_left_up[IsIntra][depth+1],sizeof(pred_mode_left_up[IsIntra][depth]));
							memcpy(avail_left_up[IsIntra][depth],avail_left_up[IsIntra][depth+1],sizeof(avail_left_up[IsIntra][depth]));
							memcpy(depth_left_up[IsIntra][depth],depth_left_up[IsIntra][depth+1],sizeof(depth_left_up[IsIntra][depth]));
							memcpy(skip_flag_left_up[IsIntra][depth],skip_flag_left_up[IsIntra][depth+1],sizeof(skip_flag_left_up[IsIntra][depth]));

							memcpy(mModels[IsIntra][depth] , mModels[IsIntra][depth+1] , sizeof(mModels[IsIntra][depth]));

						}
					}
				} 
				else
				{
				}



				


				inx_in_cu[IsIntra][depth] += inx_in_cu_transfer_table[depth];
				next_status[IsIntra][depth] = CU_EST;
				cu_best_mode_flag[depth] = 0;
				if(cu_cnt[IsIntra][depth]!=3 && depth!=4) 
					cu_ready[IsIntra][depth] = 1;
				depth == 4 ? cu_cnt[IsIntra][depth]+=4 : cu_cnt[IsIntra][depth]+=1;

			}
			end_of_cabac_rdo_status =1; 
			break;
		}
	}
}



void CABAC_RDO::cabac_est_functional_type_intra()
{
	int cu_qp_delta ;
	int cu_qp_delta_abs ;
	uint32_t cu_qp_delta_abs_prefixVal ;
	UChar mState;
	mState = mModels[IsIntra_tu][depth_tu][CBF_LUMA_OFFSET+ (depth_tu!=4)].get_m_ucState();
	est_bit_cbf_luma[IsIntra_tu][depth_tu][0] = CABAC_g_entropyBits_0[mState];
	est_bit_cbf_luma[IsIntra_tu][depth_tu][1] = CABAC_g_entropyBits_1[mState];
	mstate_cbf_luma[IsIntra_tu][depth_tu][0] = aucNextState_0[mState];
	mstate_cbf_luma[IsIntra_tu][depth_tu][1] = aucNextState_1[mState];

	mState = mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET/*+ (depth_tu==4)*/].get_m_ucState();
	est_bit_cbf_chroma[IsIntra_tu][depth_tu][0][0] = CABAC_g_entropyBits_00[mState];
	est_bit_cbf_chroma[IsIntra_tu][depth_tu][0][1] = CABAC_g_entropyBits_01[mState];
	est_bit_cbf_chroma[IsIntra_tu][depth_tu][1][0] = CABAC_g_entropyBits_10[mState];
	est_bit_cbf_chroma[IsIntra_tu][depth_tu][1][1] = CABAC_g_entropyBits_11[mState];

	mstate_cbf_chroma[IsIntra_tu][depth_tu][0][0] = aucNextState_00[mState];
	mstate_cbf_chroma[IsIntra_tu][depth_tu][0][1] = aucNextState_01[mState];
	mstate_cbf_chroma[IsIntra_tu][depth_tu][1][0] = aucNextState_10[mState];
	mstate_cbf_chroma[IsIntra_tu][depth_tu][1][1] = aucNextState_11[mState];

// 	rqt_root_cbf = (cbf_luma_total[0] | cbf_luma_total[1] | cbf_luma_total[2] | cbf_luma_total[3] | cbf_chroma_u_total[0] | cbf_chroma_v_total[0]) ;
// 	if ( ((cbf_luma_total[0]==1 || cbf_chroma_u_total[0]==1 || cbf_chroma_v_total[0] ==1) && depth_tu!=4) || (rqt_root_cbf==1 && depth_tu==4 && tu_luma_idx==4) )
// 	{
// 		cu_qp_delta = g_cu_qp_delta_abs[IsIntra_tu][depth_tu][zscan_idx_tu];
// 		cu_qp_delta_abs = abs(cu_qp_delta);
// 		cu_qp_delta_abs_prefixVal = MIN(cu_qp_delta_abs , 5);
// 		tu_est_bits[IsIntra_tu][depth_tu] += est_bit_cu_qp_delta_abs_prefixVal(cu_qp_delta_abs_prefixVal);
// 		if (cu_qp_delta_abs_prefixVal>=5)
// 		{
// 			tu_est_bits[IsIntra_tu][depth_tu] += est_bit_Golomb(cu_qp_delta_abs - cu_qp_delta_abs_prefixVal , 0);
// 		}
// 		if (cu_qp_delta != 0)
// 		{
// 			tu_est_bits[IsIntra_tu][depth_tu] += 32768;
// 		}
// 	}

	//luma
	if (tu_luma_idx != 4)
	{
		if (cbf_luma_total[tu_luma_idx] == 1)
		{
			cabac_est( TU_chroma_size *2, TU_Resi_luma , 0);

			if (slice_header_struct.slice_type == 2)
			{
				assert((est_bit_last_sig_coeff_x_prefix[IsIntra_tu][depth_tu] +
					est_bit_last_sig_coeff_y_prefix[IsIntra_tu][depth_tu] +
					est_bit_last_sig_coeff_x_suffix[IsIntra_tu][depth_tu] +
					est_bit_last_sig_coeff_y_suffix[IsIntra_tu][depth_tu] ) == g_intra_est_bit_last_sig_coeff_xy[0][depth_tu][zscan_idx_tu + tu_luma_idx]);
				assert(est_bit_coeff_abs_level_greater1_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_greater1_flag[0][depth_tu][zscan_idx_tu + tu_luma_idx]);
				assert(est_bit_coeff_abs_level_greater2_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_greater2_flag[0][depth_tu][zscan_idx_tu + tu_luma_idx]);
				assert(est_bit_coded_sub_block_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coded_sub_block_flag[0][depth_tu][zscan_idx_tu + tu_luma_idx]);
				assert(est_bit_coeff_sign_flag[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_sign_flag[0][depth_tu][zscan_idx_tu + tu_luma_idx]);
				assert(est_bit_coeff_abs_level_remaining[IsIntra_tu][depth_tu] == g_intra_est_bit_coeff_abs_level_remaining[0][depth_tu][zscan_idx_tu + tu_luma_idx]);

				assert( (est_bit_cbf_luma[IsIntra_tu][depth_tu][1]) == g_intra_est_bit_cbf[0][depth_tu][zscan_idx_tu + tu_luma_idx] );
			}

			mModels[IsIntra_tu][depth_tu][CBF_LUMA_OFFSET+ (depth_tu!=4)].set_m_ucState(mstate_cbf_luma[IsIntra_tu][depth_tu][1]);

			tu_est_bits[IsIntra_tu][depth_tu] += est_bit_cbf_luma[IsIntra_tu][depth_tu][1];
		}
		else
		{
			if (slice_header_struct.slice_type == 2)
			{
				//			assert( (est_bit_cbf_luma[IsIntra_tu][depth_tu][0]) == g_intra_est_bit_cbf[0][depth_tu][inx_in_tu[IsIntra_tu][depth_tu]] );
			}

			mModels[IsIntra_tu][depth_tu][CBF_LUMA_OFFSET+ (depth_tu!=4)].set_m_ucState(mstate_cbf_luma[IsIntra_tu][depth_tu][0]);

			tu_est_bits[IsIntra_tu][depth_tu] += est_bit_cbf_luma[IsIntra_tu][depth_tu][0];
		}
	}
	

	if ( (tu_luma_idx ==4 || depth_tu !=4) && cbf_chroma_u_total[0] == 1)
	{	//chroma U
		if (depth_tu == 4)
		{
			TU_chroma_size = 4;
		}
		cabac_est( TU_chroma_size , TU_Resi_chroma_u , 1);

		int idx_temp = inx_in_tu[IsIntra_tu][depth_tu];
		if (depth_tu == 4)
		{
			idx_temp = inx_in_tu[IsIntra_tu][depth_tu] - 3;
		}
		if (slice_header_struct.slice_type == 2)
		{
			
		}
	}

	if ( (tu_luma_idx ==4 || depth_tu !=4) && cbf_chroma_v_total[0] == 1)
	{	//chroma V
		if (depth_tu == 4)
		{
			TU_chroma_size = 4;
		}
		cabac_est( TU_chroma_size , TU_Resi_chroma_v , 2);

		int idx_temp = inx_in_tu[IsIntra_tu][depth_tu];
		if (depth_tu == 4)
		{
			idx_temp = inx_in_tu[IsIntra_tu][depth_tu] - 3;
		}
		if (slice_header_struct.slice_type == 2)
		{
			
		}

	}

	if ( (tu_luma_idx ==4 || depth_tu !=4) )
	{
		switch ((cbf_chroma_u_total[0] << 1) | cbf_chroma_v_total[0])
		{
		case 0: 
			if (slice_header_struct.slice_type == 2)
			{
				assert( (est_bit_cbf_chroma[IsIntra_tu][depth_tu][0][0]) == g_intra_est_bit_cbf[1][depth_tu][zscan_idx_tu] + g_intra_est_bit_cbf[2][depth_tu][zscan_idx_tu]);
			}

			mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET].set_m_ucState(mstate_cbf_chroma[IsIntra_tu][depth_tu][0][0]);

			tu_est_bits[IsIntra_tu][depth_tu] += est_bit_cbf_chroma[IsIntra_tu][depth_tu][0][0];
			break;
		case 1:
			if (slice_header_struct.slice_type == 2)
			{
//				assert( (est_bit_cbf_chroma[IsIntra_tu][depth_tu][0][1]) == g_intra_est_bit_cbf[1][depth_tu][idx_temp] + g_intra_est_bit_cbf[2][depth_tu][idx_temp]);
			}

			mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET].set_m_ucState(mstate_cbf_chroma[IsIntra_tu][depth_tu][0][1]);

			tu_est_bits[IsIntra_tu][depth_tu] += est_bit_cbf_chroma[IsIntra_tu][depth_tu][0][1];
			break;
		case 2:
			if (slice_header_struct.slice_type == 2)
			{
//				assert( (est_bit_cbf_chroma[IsIntra_tu][depth_tu][1][0]) == g_intra_est_bit_cbf[1][depth_tu][idx_temp] + g_intra_est_bit_cbf[2][depth_tu][idx_temp]);
			}

			mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET].set_m_ucState(mstate_cbf_chroma[IsIntra_tu][depth_tu][1][0]);

			tu_est_bits[IsIntra_tu][depth_tu] += est_bit_cbf_chroma[IsIntra_tu][depth_tu][1][0];
			break;
		case 3:
			if (slice_header_struct.slice_type == 2)
			{
//				assert( (est_bit_cbf_chroma[IsIntra_tu][depth_tu][1][1]) == g_intra_est_bit_cbf[1][depth_tu][idx_temp] + g_intra_est_bit_cbf[2][depth_tu][idx_temp]);
			}

			mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET].set_m_ucState(mstate_cbf_chroma[IsIntra_tu][depth_tu][1][1]);

			tu_est_bits[IsIntra_tu][depth_tu] += est_bit_cbf_chroma[IsIntra_tu][depth_tu][1][1];
			break;
		default:
			assert(0);
		}
	}

// 	if (depth_tu==4)
// 	{
// 		tu_luma_idx += 1;
// 	}
	if (depth_tu != 4 && depth_tu != 0)
	{
		assert(tu_est_bits[IsIntra_tu][depth_tu] == g_est_bit_tu[IsIntra_tu][depth_tu][zscan_idx_tu]);
	} 
	else if( (depth_tu == 4 && tu_luma_idx ==4) || (depth_tu == 0 && tu_luma_idx ==3) )
	{
		assert(tu_est_bits[IsIntra_tu][depth_tu] == g_est_bit_tu[IsIntra_tu][depth_tu][zscan_idx_tu]);
	}
	if (tu_luma_idx==4)
	{
		tu_luma_idx = 0;
	}
}

void CABAC_RDO::cabac_est_functional_type_inter(int type)
{

	//luma
	if (cbf_luma_total[tu_luma_idx] == 1 && type == 0)
	{
		cabac_est( TU_chroma_size *2, TU_Resi_luma , 0);
	}
	
	

	if (cbf_chroma_u_total[tu_luma_idx] == 1 && type == 1)
	{	//chroma U
		cabac_est( TU_chroma_size , TU_Resi_chroma_u , 1);
	}

	if (  cbf_chroma_v_total[tu_luma_idx] == 1 && type == 2)
	{	//chroma V
		cabac_est( TU_chroma_size , TU_Resi_chroma_v , 2);
	}


	if (depth_tu == 0 && tu_luma_idx ==3 &&   type == 2)
	{
		UChar mState;
		UChar mState_next;
		cbf_luma_root = (cbf_luma_total[0] | cbf_luma_total[1] | cbf_luma_total[2] | cbf_luma_total[3]) ;
		cbf_chroma_u_root = (cbf_chroma_u_total[0] | cbf_chroma_u_total[1] | cbf_chroma_u_total[2] | cbf_chroma_u_total[3]) ;
		cbf_chroma_v_root = (cbf_chroma_v_total[0] | cbf_chroma_v_total[1] | cbf_chroma_v_total[2] | cbf_chroma_v_total[3]) ;
		rqt_root_cbf = (cbf_luma_root | cbf_chroma_u_root | cbf_chroma_v_root ) ;


		if (rqt_root_cbf == 0 )
		{
			if (IsIntra_tu == 0)
			{
				mState = mModels[IsIntra_tu][depth_tu][RQT_ROOT_CBF_OFFSET].get_m_ucState();
				mState_next = aucNextState_0[mState];
				tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_0[mState];
				mModels[IsIntra_tu][depth_tu][RQT_ROOT_CBF_OFFSET].set_m_ucState(mState_next);
			}
		}
		else
		{
			if (IsIntra_tu == 0)
			{
				mState = mModels[IsIntra_tu][depth_tu][RQT_ROOT_CBF_OFFSET].get_m_ucState();
				mState_next = aucNextState_1[mState];
				tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[mState];
				mModels[IsIntra_tu][depth_tu][RQT_ROOT_CBF_OFFSET].set_m_ucState(mState_next);
			}

			for (int i = 0 ; i < 4 ; i++)
			{
				mState = mModels[IsIntra_tu][depth_tu][CBF_LUMA_OFFSET+ (depth_tu!=0)].get_m_ucState();
				mState_next = cbf_luma_total[i] ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
				tu_est_bits[IsIntra_tu][depth_tu] +=  cbf_luma_total[i] ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
				mModels[IsIntra_tu][depth_tu][CBF_LUMA_OFFSET+ (depth_tu!=0)].set_m_ucState(mState_next);
			}

			mState = mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET].get_m_ucState();
			mState_next = cbf_chroma_u_root ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
			tu_est_bits[IsIntra_tu][depth_tu] +=  cbf_chroma_u_root ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
			mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET].set_m_ucState(mState_next);

			mState = mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET].get_m_ucState();
			mState_next = cbf_chroma_v_root ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
			tu_est_bits[IsIntra_tu][depth_tu] +=  cbf_chroma_v_root ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
			mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET].set_m_ucState(mState_next);

			for (int i = 0 ; i < 4 ; i++)
			{
				if (cbf_chroma_u_root == 1)
				{
					mState = mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET+ (depth_tu==0)].get_m_ucState();
					mState_next = cbf_chroma_u_total[i] ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
					tu_est_bits[IsIntra_tu][depth_tu] +=  cbf_chroma_u_total[i] ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
					mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET+ (depth_tu==0)].set_m_ucState(mState_next);
				}
				
				if (cbf_chroma_v_root == 1)
				{
					mState = mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET+ (depth_tu==0)].get_m_ucState();
					mState_next = cbf_chroma_v_total[i] ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
					tu_est_bits[IsIntra_tu][depth_tu] +=  cbf_chroma_v_total[i] ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
					mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET+ (depth_tu==0)].set_m_ucState(mState_next);
				}
			}
		}
		
		if (tu_est_bits[IsIntra_tu][depth_tu] != 0)
		{
			assert(tu_est_bits[IsIntra_tu][depth_tu] == g_est_bit_tu[IsIntra_tu][depth_tu][zscan_idx_tu]);
		}
		
	}

	if (depth_tu != 0 && type == 2)
	{
		UChar mState;
		UChar mState_next;
		cbf_luma_root = (cbf_luma_total[0]) ;
		cbf_chroma_u_root = (cbf_chroma_u_total[0]) ;
		cbf_chroma_v_root = (cbf_chroma_v_total[0]) ;
		rqt_root_cbf = (cbf_luma_root | cbf_chroma_u_root | cbf_chroma_v_root ) ;

		if (rqt_root_cbf == 0 )
		{
			if (IsIntra_tu == 0)
			{
				mState = mModels[IsIntra_tu][depth_tu][RQT_ROOT_CBF_OFFSET].get_m_ucState();
				mState_next = aucNextState_0[mState];
				tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_0[mState];
				mModels[IsIntra_tu][depth_tu][RQT_ROOT_CBF_OFFSET].set_m_ucState(mState_next);
			}
		}
		else
		{
			if (IsIntra_tu == 0)
			{
				mState = mModels[IsIntra_tu][depth_tu][RQT_ROOT_CBF_OFFSET].get_m_ucState();
				mState_next = aucNextState_1[mState];
				tu_est_bits[IsIntra_tu][depth_tu] += CABAC_g_entropyBits_1[mState];
				mModels[IsIntra_tu][depth_tu][RQT_ROOT_CBF_OFFSET].set_m_ucState(mState_next);
			}

			if (cbf_chroma_u_total[0]==1 || cbf_chroma_v_total[0] ==1)
			{
				mState = mModels[IsIntra_tu][depth_tu][CBF_LUMA_OFFSET+ (depth_tu!=0)].get_m_ucState();
				mState_next = cbf_luma_total[0] ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
				tu_est_bits[IsIntra_tu][depth_tu] +=  cbf_luma_total[0] ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
				mModels[IsIntra_tu][depth_tu][CBF_LUMA_OFFSET+ (depth_tu!=0)].set_m_ucState(mState_next);
			}

			mState = mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET+ (depth_tu==0)].get_m_ucState();
			mState_next = cbf_chroma_u_total[0] ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
			tu_est_bits[IsIntra_tu][depth_tu] +=  cbf_chroma_u_total[0] ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
			mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET+ (depth_tu==0)].set_m_ucState(mState_next);

			mState = mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET+ (depth_tu==0)].get_m_ucState();
			mState_next = cbf_chroma_v_total[0] ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
			tu_est_bits[IsIntra_tu][depth_tu] +=  cbf_chroma_v_total[0] ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
			mModels[IsIntra_tu][depth_tu][CBF_CHROMA_OFFSET+ (depth_tu==0)].set_m_ucState(mState_next);
		}

		if (tu_est_bits[IsIntra_tu][depth_tu] != 0)
		{
			assert(tu_est_bits[IsIntra_tu][depth_tu] == g_est_bit_tu[IsIntra_tu][depth_tu][zscan_idx_tu]);
		}
		
	}
}

uint64_t CABAC_RDO::Inter_fme_est_bit_cu_pu(FmeInfoForCabac *fmeInfoForCabac)
{
	UChar mState;
	UChar mState_next;
	uint64_t bits = 0;
	int ctxIdx;
	UChar interDir = fmeInfoForCabac->m_interDir - 1;
	char mvp_l0_flag = fmeInfoForCabac->m_mvpIdx[0];
	char mvp_l1_flag = fmeInfoForCabac->m_mvpIdx[1];

	ctxIdx =  getCtxSkipFlag(zscan_idx_tu>>2,depth_tu,IsIntra_tu);
	mState = mModels[IsIntra_tu][depth_tu][SKIP_FLAG_OFFSET+ ctxIdx].get_m_ucState();
	mState_next = aucNextState_0[mState] ;
	bits += CABAC_g_entropyBits_0[mState];
	mModels[IsIntra_tu][depth_tu][SKIP_FLAG_OFFSET+ ctxIdx].set_m_ucState(mState_next);

	setLumaIntraDirSubParts(1 , zscan_idx_tu , depth_tu , IsIntra_tu);
	setPredModeSubParts(MODE_INTER , tu_to_cu_idx(zscan_idx_tu) ,depth_tu, IsIntra_tu);
	setAvailSubParts_map(1, zscan_idx_tu , depth_tu , IsIntra_tu);

//	setDepthSubParts(depth_tu, zscan_idx_tu>>2 , IsIntra_tu);
	setSkipFlagSubParts(0, zscan_idx_tu>>2 , depth_tu , IsIntra_tu);

	mState = mModels[IsIntra_tu][depth_tu][PRED_MODE_OFFSET].get_m_ucState();
	mState_next = aucNextState_0[mState] ;
	bits += CABAC_g_entropyBits_0[mState];
	mModels[IsIntra_tu][depth_tu][PRED_MODE_OFFSET].set_m_ucState(mState_next);

	mState = mModels[IsIntra_tu][depth_tu][PART_MODE_OFFSET].get_m_ucState();//INTER只考虑了2N*2N的情况
	mState_next = aucNextState_1[mState] ;
	bits += CABAC_g_entropyBits_1[mState];
	mModels[IsIntra_tu][depth_tu][PART_MODE_OFFSET].set_m_ucState(mState_next);

	mState = mModels[IsIntra_tu][depth_tu][MERGE_FLAG_OFFSET].get_m_ucState();
	mState_next = aucNextState_0[mState] ;
	bits += CABAC_g_entropyBits_0[mState];
	mModels[IsIntra_tu][depth_tu][MERGE_FLAG_OFFSET].set_m_ucState(mState_next);

	if (slice_header_struct.slice_type == 0)
	{
		if (interDir == 0)
		{
			mState = mModels[IsIntra_tu][depth_tu][INTER_PRED_IDC_OFFSET + depth_tu].get_m_ucState();
			mState_next = aucNextState_0[mState] ;
			bits += CABAC_g_entropyBits_0[mState];
			mModels[IsIntra_tu][depth_tu][INTER_PRED_IDC_OFFSET + depth_tu].set_m_ucState(mState_next);

			mState = mModels[IsIntra_tu][depth_tu][INTER_PRED_IDC_OFFSET + 4].get_m_ucState();
			mState_next = aucNextState_0[mState] ;
			bits += CABAC_g_entropyBits_0[mState];
			mModels[IsIntra_tu][depth_tu][INTER_PRED_IDC_OFFSET + 4].set_m_ucState(mState_next);
		} 
		else if (interDir == 1)
		{
			mState = mModels[IsIntra_tu][depth_tu][INTER_PRED_IDC_OFFSET + depth_tu].get_m_ucState();
			mState_next = aucNextState_0[mState] ;
			bits += CABAC_g_entropyBits_0[mState];
			mModels[IsIntra_tu][depth_tu][INTER_PRED_IDC_OFFSET + depth_tu].set_m_ucState(mState_next);

			mState = mModels[IsIntra_tu][depth_tu][INTER_PRED_IDC_OFFSET + 4].get_m_ucState();
			mState_next = aucNextState_1[mState] ;
			bits += CABAC_g_entropyBits_1[mState];
			mModels[IsIntra_tu][depth_tu][INTER_PRED_IDC_OFFSET + 4].set_m_ucState(mState_next);
		}
		else
		{
			mState = mModels[IsIntra_tu][depth_tu][INTER_PRED_IDC_OFFSET + depth_tu].get_m_ucState();
			mState_next = aucNextState_1[mState] ;
			bits += CABAC_g_entropyBits_1[mState];
			mModels[IsIntra_tu][depth_tu][INTER_PRED_IDC_OFFSET + depth_tu].set_m_ucState(mState_next);
		}
	}

	if (interDir != 1)
	{
		if (slice_header_struct.num_ref_idx_l0_active_minus1>0)
		{
			bits+=est_bits_ref_idx(0 , fmeInfoForCabac);//0表示前向，1表示后向。
		}
		bits += est_bits_mvd(0 , fmeInfoForCabac);//0表示前向，1表示后向。

		mState = mModels[IsIntra_tu][depth_tu][MVP_FLAG_OFFSET].get_m_ucState();
		mState_next = mvp_l0_flag ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
		bits +=  mvp_l0_flag ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
		mModels[IsIntra_tu][depth_tu][MVP_FLAG_OFFSET].set_m_ucState(mState_next);
	}

	if (interDir != 0)
	{
		if (slice_header_struct.num_ref_idx_l1_active_minus1>0)
		{
			bits+=est_bits_ref_idx(1 , fmeInfoForCabac);//0表示前向，1表示后向。
		}
		if (!(slice_header_struct.Mvd_l1_zero_flag==1 && interDir == 2))
		{
			bits += est_bits_mvd(1 , fmeInfoForCabac);//0表示前向，1表示后向。

			mState = mModels[IsIntra_tu][depth_tu][MVP_FLAG_OFFSET].get_m_ucState();
			mState_next = mvp_l1_flag ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
			bits +=  mvp_l1_flag ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
			mModels[IsIntra_tu][depth_tu][MVP_FLAG_OFFSET].set_m_ucState(mState_next);
		}
	}


	return bits ;
}

uint64_t CABAC_RDO::est_bits_ref_idx(int type , FmeInfoForCabac *fmeInfoForCabac)
{
	UChar mState;
	UChar mState_next;
	uint64_t bits = 0;
	int ctxIdx;

	int max_ref_idx = type == 0 ? slice_header_struct.num_ref_idx_l0_active_minus1 : slice_header_struct.num_ref_idx_l1_active_minus1;
	int ref_idx = fmeInfoForCabac->m_refIdx[type];

	mState = mModels[IsIntra_tu][depth_tu][REF_IDX_L0_OFFSET].get_m_ucState();
	mState_next = ref_idx ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
	bits +=  ref_idx ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
	mModels[IsIntra_tu][depth_tu][REF_IDX_L0_OFFSET].set_m_ucState(mState_next);

	if (ref_idx > 0)
	{
		ref_idx--;
		for (int i = 0 ; i < (max_ref_idx - 1) ; i++)
		{
			int symbol = i == ref_idx ? 0 : 1 ;

			if (i == 0)
			{
				mState = mModels[IsIntra_tu][depth_tu][REF_IDX_L0_OFFSET + 1].get_m_ucState();
				mState_next = symbol ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
				bits +=  symbol ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
				mModels[IsIntra_tu][depth_tu][REF_IDX_L0_OFFSET + 1].set_m_ucState(mState_next);
			} 
			else
			{
				bits += 32768;
			}
			if (symbol == 0)
			{
				return bits;
			}
		}
	}
	return bits;
}

uint64_t CABAC_RDO::est_bits_mvd(int type  , FmeInfoForCabac *fmeInfoForCabac)
{
	UChar mState;
	UChar mState_next;
	uint64_t bits = 0;

	short mvdx = fmeInfoForCabac->m_mvdx[type];
	short mvdy = fmeInfoForCabac->m_mvdy[type];
	short abs_mvdx = abs(mvdx);
	short abs_mvdy = abs(mvdy);
	short abs_mvd_greater0_flag[ 2 ];
	short abs_mvd_greater1_flag[ 2 ];

	abs_mvd_greater0_flag[0] = abs_mvdx > 0 ? 1 : 0 ;
	abs_mvd_greater0_flag[1] = abs_mvdy > 0 ? 1 : 0 ;

	mState = mModels[IsIntra_tu][depth_tu][ABS_MVD_GREATER0_FLAG_OFFSET].get_m_ucState();
	mState_next = abs_mvd_greater0_flag[0] ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
	bits +=  abs_mvd_greater0_flag[0] ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
	mModels[IsIntra_tu][depth_tu][ABS_MVD_GREATER0_FLAG_OFFSET].set_m_ucState(mState_next);

	mState = mModels[IsIntra_tu][depth_tu][ABS_MVD_GREATER0_FLAG_OFFSET ].get_m_ucState();
	mState_next = abs_mvd_greater0_flag[1] ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
	bits +=  abs_mvd_greater0_flag[1] ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
	mModels[IsIntra_tu][depth_tu][ABS_MVD_GREATER0_FLAG_OFFSET ].set_m_ucState(mState_next);

	if( abs_mvd_greater0_flag[0] ==1) 
	{
		abs_mvd_greater1_flag[0] = abs_mvdx > 1 ? 1 : 0 ;
		mState = mModels[IsIntra_tu][depth_tu][ABS_MVD_GREATER1_FLAG_OFFSET].get_m_ucState();
		mState_next = abs_mvd_greater1_flag[0] ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
		bits +=  abs_mvd_greater1_flag[0] ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
		mModels[IsIntra_tu][depth_tu][ABS_MVD_GREATER1_FLAG_OFFSET].set_m_ucState(mState_next);
	}

	if( abs_mvd_greater0_flag[1] ==1) 
	{
		abs_mvd_greater1_flag[1] = abs_mvdy > 1 ? 1 : 0 ;
		mState = mModels[IsIntra_tu][depth_tu][ABS_MVD_GREATER1_FLAG_OFFSET].get_m_ucState();
		mState_next = abs_mvd_greater1_flag[1] ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
		bits +=  abs_mvd_greater1_flag[1] ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
		mModels[IsIntra_tu][depth_tu][ABS_MVD_GREATER1_FLAG_OFFSET].set_m_ucState(mState_next);
	}

	if( abs_mvd_greater0_flag[ 0 ] == 1) 
	{ 
		if( abs_mvd_greater1_flag[ 0 ] == 1) 
			bits += est_bit_Golomb(abs_mvdx - 2, 1);

		bits += 32768;
	} 

	if( abs_mvd_greater0_flag[ 1 ] == 1) 
	{ 
		if( abs_mvd_greater1_flag[ 1 ] == 1) 
			bits += est_bit_Golomb(abs_mvdy - 2, 1);

		bits += 32768;
	} 
	return bits;
}

uint64_t CABAC_RDO::est_bit_Golomb(uint32_t symbol, uint32_t count)
{
	uint32_t bins = 0;
	int numBins = 0;

	while (symbol >= (uint32_t)(1 << count))
	{
		bins = 2 * bins + 1;
		numBins++;
		symbol -= 1 << count;
		count++;
	}

	bins = 2 * bins + 0;
	numBins++;

	bins = (bins << count) | symbol;
	numBins += count;

	assert(numBins <= 32);
	return numBins * 32768;
}

uint64_t CABAC_RDO::Inter_merge_est_bit_cu_pu(MergeInfoForCabac *mergeInfoForCabac)
{
	UChar mState;
	UChar mState_next;
	uint64_t bits = 0;
	int ctxIdx;
	int merge_idx = mergeInfoForCabac->m_mergeIdx;
	if (mergeInfoForCabac->m_bSkipFlag == 1)
	{
		ctxIdx =  getCtxSkipFlag(zscan_idx_tu>>2,depth_tu,IsIntra_tu);
		mState = mModels[IsIntra_tu][depth_tu][SKIP_FLAG_OFFSET+ ctxIdx].get_m_ucState();
		mState_next = aucNextState_1[mState] ;
		bits += CABAC_g_entropyBits_1[mState];
		mModels[IsIntra_tu][depth_tu][SKIP_FLAG_OFFSET+ ctxIdx].set_m_ucState(mState_next);

		setLumaIntraDirSubParts(1 , zscan_idx_tu , depth_tu , IsIntra_tu);
		setPredModeSubParts(MODE_SKIP , tu_to_cu_idx(zscan_idx_tu) ,depth_tu, IsIntra_tu);
		setAvailSubParts_map(1, zscan_idx_tu , depth_tu , IsIntra_tu);

//		setDepthSubParts(depth_tu, zscan_idx_tu>>2 , IsIntra_tu);
		setSkipFlagSubParts(1, zscan_idx_tu>>2 , depth_tu , IsIntra_tu);

		int max_merge_idx = 5 -slice_header_struct.Five_minus_max_num_merge_cand - 1;
		if (max_merge_idx == 0)
		{
			return bits;
		} 
		else
		{
			if (merge_idx == 0)
			{
				mState = mModels[IsIntra_tu][depth_tu][MERGE_IDX_OFFSET].get_m_ucState();
				mState_next = aucNextState_0[mState] ;
				bits += CABAC_g_entropyBits_0[mState];
				mModels[IsIntra_tu][depth_tu][MERGE_IDX_OFFSET].set_m_ucState(mState_next);
			} 
			else
			{
				mState = mModels[IsIntra_tu][depth_tu][MERGE_IDX_OFFSET].get_m_ucState();
				mState_next = aucNextState_1[mState] ;
				bits += CABAC_g_entropyBits_1[mState];
				mModels[IsIntra_tu][depth_tu][MERGE_IDX_OFFSET].set_m_ucState(mState_next);
				if (merge_idx != max_merge_idx)
					bits+=(merge_idx*32768);
				else
					bits+=((merge_idx-1)*32768);
			}
			
			
			return bits;
		}
	}
	else
	{
		ctxIdx =  getCtxSkipFlag(zscan_idx_tu>>2,depth_tu,IsIntra_tu);
		mState = mModels[IsIntra_tu][depth_tu][SKIP_FLAG_OFFSET+ ctxIdx].get_m_ucState();
		mState_next = aucNextState_0[mState] ;
		bits += CABAC_g_entropyBits_0[mState];
		mModels[IsIntra_tu][depth_tu][SKIP_FLAG_OFFSET+ ctxIdx].set_m_ucState(mState_next);

		setLumaIntraDirSubParts(1 , zscan_idx_tu , depth_tu , IsIntra_tu);
		setPredModeSubParts(MODE_INTER , tu_to_cu_idx(zscan_idx_tu) ,depth_tu, IsIntra_tu);
		setAvailSubParts_map(1, zscan_idx_tu , depth_tu , IsIntra_tu);

//		setDepthSubParts(depth_tu, zscan_idx_tu>>2 , IsIntra_tu);
		setSkipFlagSubParts(0, zscan_idx_tu>>2 , depth_tu , IsIntra_tu);

		mState = mModels[IsIntra_tu][depth_tu][PRED_MODE_OFFSET].get_m_ucState();
		mState_next = aucNextState_0[mState] ;
		bits += CABAC_g_entropyBits_0[mState];
		mModels[IsIntra_tu][depth_tu][PRED_MODE_OFFSET].set_m_ucState(mState_next);

		mState = mModels[IsIntra_tu][depth_tu][PART_MODE_OFFSET].get_m_ucState();//INTER只考虑了2N*2N的情况
		mState_next = aucNextState_1[mState] ;
		bits += CABAC_g_entropyBits_1[mState];
		mModels[IsIntra_tu][depth_tu][PART_MODE_OFFSET].set_m_ucState(mState_next);

		mState = mModels[IsIntra_tu][depth_tu][MERGE_FLAG_OFFSET].get_m_ucState();
		mState_next = aucNextState_1[mState] ;
		bits += CABAC_g_entropyBits_1[mState];
		mModels[IsIntra_tu][depth_tu][MERGE_FLAG_OFFSET].set_m_ucState(mState_next);

		int max_merge_idx = 5 -slice_header_struct.Five_minus_max_num_merge_cand - 1;
		if (max_merge_idx == 0)
		{
			return bits;
		} 
		else
		{
			if (merge_idx == 0)
			{
				mState = mModels[IsIntra_tu][depth_tu][MERGE_IDX_OFFSET].get_m_ucState();
				mState_next = aucNextState_0[mState] ;
				bits += CABAC_g_entropyBits_0[mState];
				mModels[IsIntra_tu][depth_tu][MERGE_IDX_OFFSET].set_m_ucState(mState_next);
			} 
			else
			{
				mState = mModels[IsIntra_tu][depth_tu][MERGE_IDX_OFFSET].get_m_ucState();
				mState_next = aucNextState_1[mState] ;
				bits += CABAC_g_entropyBits_1[mState];
				mModels[IsIntra_tu][depth_tu][MERGE_IDX_OFFSET].set_m_ucState(mState_next);
				if (merge_idx != max_merge_idx)
					bits+=(merge_idx*32768);
				else
					bits+=((merge_idx-1)*32768);
			}
			

			return bits;
		}
	}
}

uint64_t CABAC_RDO::est_bit_cu_qp_delta_abs_prefixVal(uint32_t symbol)
{
	UChar mState;
	UChar mState_next;
	uint64_t bits = 0;

	mState = mModels[IsIntra_tu][depth_tu][CU_QP_DELTA_OFFSET].get_m_ucState();
	mState_next = symbol ==0 ? aucNextState_0[mState] : aucNextState_1[mState] ;
	bits +=  symbol ==0 ? CABAC_g_entropyBits_0[mState] : CABAC_g_entropyBits_1[mState];
	mModels[IsIntra_tu][depth_tu][CU_QP_DELTA_OFFSET].set_m_ucState(mState_next);

	if (symbol == 0)
	{
		return bits;
	}

	bool bCodeLast = (5 > symbol);

	while (--symbol)
	{
		mState = mModels[IsIntra_tu][depth_tu][CU_QP_DELTA_OFFSET + 1].get_m_ucState();
		mState_next = aucNextState_1[mState] ;
		bits += CABAC_g_entropyBits_1[mState];
		mModels[IsIntra_tu][depth_tu][CU_QP_DELTA_OFFSET + 1].set_m_ucState(mState_next);
	}

	if (bCodeLast)
	{
		mState = mModels[IsIntra_tu][depth_tu][CU_QP_DELTA_OFFSET + 1].get_m_ucState();
		mState_next = aucNextState_0[mState] ;
		bits += CABAC_g_entropyBits_0[mState];
		mModels[IsIntra_tu][depth_tu][CU_QP_DELTA_OFFSET + 1].set_m_ucState(mState_next);
	}
	return bits;
}

int CABAC_RDO::est_bits_split_cu_flag(unsigned int depth  , uint32_t zscan)
{
	UChar mState;
	UChar mState_next;
	uint64_t bits = 0;
	uint32_t next_depth = depth + 1;
	int ctx;
	int cu_mode ;
	if (cu_best_mode[depth] == 1)
	{
		cu_mode = 1;
	}
	else if (inter_cu_best_mode[depth] == 1)
	{
		cu_mode = 2;
	}
	else
	{
		cu_mode = 0;
	}
	
	ctx =  getCtxSplitFlag( zscan>>2 , depth , cu_mode);

	if (depth == 3)
	{
		setDepthSubParts( depth,  zscan>>2  ,  cu_mode );//cu level
		return 0;
	}

	mState = mModels[cu_mode][depth][SPLIT_CU_FLAG_OFFSET + ctx].get_m_ucState();
	bits_split_cu_flag[depth] = CABAC_g_entropyBits_1[mState];
	mState_next = aucNextState_1[mState] ;
	for (int i = next_depth ; i <5 ; i ++)
	{
		mModels[0][i][SPLIT_CU_FLAG_OFFSET + ctx].set_m_ucState(mState_next);
		mModels[1][i][SPLIT_CU_FLAG_OFFSET + ctx].set_m_ucState(mState_next);
		mModels[2][i][SPLIT_CU_FLAG_OFFSET + ctx].set_m_ucState(mState_next);
	}
	

	assert(g_est_bit_cu_split_flag[1][depth ][zscan][1] == bits_split_cu_flag[depth]);

	mState = mModels[cu_mode][depth][SPLIT_CU_FLAG_OFFSET + ctx].get_m_ucState();
	mState_next = aucNextState_0[mState] ;
	bits += CABAC_g_entropyBits_0[mState];
	mModels[cu_mode][depth][SPLIT_CU_FLAG_OFFSET + ctx].set_m_ucState(mState_next);
	setDepthSubParts( depth,  zscan>>2  ,  cu_mode );//cu level

	assert(g_est_bit_cu_split_flag[1][depth ][zscan][0] == bits);

	return (bits + (1<<14))>>15;
	
}