#ifndef _COMMON_H
#define _COMMON_H

#include "stm32f1xx_hal.h"

#define ENABLE_DBG      1

#if (ENABLE_DBG == 1)
#define DBG_PRT(fmt...)   \
    do {\
        xSemaphoreTake(PRINTF_MUTEX,portMAX_DELAY);\
        printf("[Tick:%d][%s]-%d: ",HAL_GetTick(), __FUNCTION__, __LINE__);\
        printf(fmt);\
        xSemaphoreGive(PRINTF_MUTEX);\
    }while(0)
#else
#define DBG_PRT(fmt...)
#endif

#define COMMON_UNUSED(x)       (x = x)

typedef struct{
    uint8_t step;           //任务步数
    uint16_t errCnt;        //错误数
    uint32_t execuTick;     //执行时间
}taskManageTypedef;         //任务管理结构体


#endif