#ifndef __MACRO_H__
#define __MACRO_H__

/* =========================== hardWareC macros ============================== */
#define MODULE_INIT_OPEN 0

/* ======================================================================= */


/* =========================== CTU_CALC macros ============================== */
#define RK_CTU_CALC_PROC_ENABLE		0
#define RK_OUTPUT_FILE				0
//#define CTU_CALC_STATUS_ENABLE
/* ======================================================================= */


/* =========================== INTRA macros ============================== */
//#define RK_INTRA_PRED
//#define X265_INTRA_DEBUG
//#define LOG_INTRA_PARAMS_2_FILE
//#define INTRA_RESULT_STORE_FILE
//#define RK_CABAC
//#define INTRA_4x4_DEBUG
//#define RK_INTRA_4x4_PRED
//#define RK_INTRA_MODE_CHOOSE              // 快速模式判决，该方案不采取
#define INTRA_REDUCE_DIR(dirMode, width) 	( ( (dirMode % 2 == 0) || (dirMode == 1)) || ((width != 4) && (width != 8)))  
#define DISABLE_64x64_CU
/* ======================================================================= */


/* =========================== ME macros ============================== */

/* ======================================================================= */


/* ======================== TQ module macros begin ======================== */
#define TQ_RUN_IN_X265_INTRA		0		// get info and run in x265 Intra module
#define TQ_RUN_IN_HWC_INTRA			0		// get info and run in HWC Intra module

#define TQ_RUN_IN_X265_ME			0		// get info and run in X265 ME module
#define TQ_RUN_IN_HWC_ME			0		// get info and run in HWC ME module

// log enable
#define TQ_LOG_IN_X265_INTRA		0
#define TQ_LOG_IN_HWC_INTRA			0
#define TQ_LOG_IN_X265_ME			0
#define TQ_LOG_IN_HWC_ME			0
/* ======================================================================= */


/* ======================== SAO module macros begin ======================== */
#define SAO_RUN_IN_X265				0       // get info and run in X265
#define SAO_RUN_IN_HWC				0       // get info and run in HWC

// log enable
#define SAO_LOG_IN_X265				0
#define SAO_LOG_IN_HWC				0
/* ======================================================================= */

#endif
