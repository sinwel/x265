#ifndef RK_HEVC_ENC_MACRO
#define RK_HEVC_ENC_MACRO

#define RK_FIX_POINT_REPLACE_DOUBLE_POINT          1
#define RK_INTRA_RDO_ONLY_ONCE                     1
#define RK_INTER_METEST                            1
#define RK_INTER_CALC_RDO_TWICE                    1

enum JudgeStand
{
	JUDGE_SATD, //一般情况下不会使用，因为在判决过程中没有satd的lambda值 add by hdl
	JUDGE_SAD,
	JUDGE_SSE
};

#endif