#ifndef __MACRO_H__
#define __MACRO_H__

#define MODULE_INIT_OPEN 0

// merge by zxy in 2014 03 26
#define X265_LOG_DEBUG_ROCKCHIP 4
#define X265_LOG_FILE_ROCKCHIP  5

#ifdef X265_LOG_DEBUG_ROCKCHIP
#define RK_HEVC_PRINT(fmt, ...) \
    do { \
        fprintf(stderr,"[rk_debug]: "  fmt, ## __VA_ARGS__); \
    } while(0);
#else
#define RK_HEVC_PRINT(format,...)
#endif
#ifdef X265_LOG_FILE_ROCKCHIP
#define RK_HEVC_FPRINT(fp,fmt, ...) \
    do { \
        fprintf(fp," " fmt, ## __VA_ARGS__); \
    } while(0);  
#define RK_HEVC_LOGING(fp,fmt, ...) \
     do { \
        fprintf(fp,"[%s]: %s \n line:%d \n",__FILE__,__FUNCTION__,__LINE__); \
        fprintf(fp," " fmt, ## __VA_ARGS__); \
    } while(0);  

    
#else
#define RK_HEVC_FPRINT(fp,fmt, ...) 

#endif

//#define RK_INTRA_PRED

//#define CONNECT_QT

#endif
