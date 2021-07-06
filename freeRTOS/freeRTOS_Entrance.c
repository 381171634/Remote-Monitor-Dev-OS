#include "includes.h"

SemaphoreHandle_t PRINTF_MUTEX;
EventGroupHandle_t os_eg_sampleCpl;

typedef struct{
    TaskFunction_t      pFunction;
    const char * const  pcName;
    uint16_t            statckSize;
    void *              para;
    UBaseType_t         priority;
    TaskHandle_t *      handle;
}taskTCBTypedef;

static taskTCBTypedef tasks[] = {
    {
        dht11_task,
        "dht11Task",
        128,
        NULL,
        1,
        NULL
    },

    {
        sgp30_task,
        "sgp30Task",
        128,
        NULL,
        1,
        NULL
    },

    {
        gprs_task,
        "sgp30Task",
        1024,
        NULL,
        1,
        NULL
    },

    {
        lowpower_task,
        "lowpowerTask",
        128,
        NULL,
        2,
        NULL
    }

};

__weak void configureTimerForRunTimeStats(void)
{
}

__weak unsigned long getRunTimeCounterValue(void)
{
	return HAL_GetTick();
}

void RTOS_Init()
{
    int i;
    BaseType_t retval;
    //打印互斥量
    PRINTF_MUTEX = xSemaphoreCreateMutex();
    configASSERT(PRINTF_MUTEX);
    //采样完成事件组
    os_eg_sampleCpl = xEventGroupCreate();
    configASSERT(PRINTF_MUTEX);
    //创建任务
    for(i = 0;i < sizeof(tasks) / sizeof(tasks[0]); i++)
    {
        retval = xTaskCreate(   tasks[i].pFunction,
                                tasks[i].pcName,
                                tasks[i].statckSize,
                                tasks[i].para,
                                tasks[i].priority,
                                tasks[i].handle);

        configASSERT(retval);
    }
}

void RTOS_Start()
{
    vTaskStartScheduler();
}