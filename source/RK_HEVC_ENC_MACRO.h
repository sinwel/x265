#ifndef RK_HEVC_ENC_MACRO
#define RK_HEVC_ENC_MACRO

#define RK_FIX_POINT_REPLACE_DOUBLE_POINT          1
#define RK_INTRA_RDO_ONLY_ONCE                     1
#define RK_INTER_METEST                            1
#define RK_INTER_CALC_RDO_TWICE                    1

enum JudgeStand
{
	JUDGE_SATD, //һ������²���ʹ�ã���Ϊ���о�������û��satd��lambdaֵ add by hdl
	JUDGE_SAD,
	JUDGE_SSE
};

#endif