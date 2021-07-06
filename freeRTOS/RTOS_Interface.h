#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

#define osDelay(ms)     vTaskDelay(pdMS_TO_TICKS(ms))
#define PRINTF_MUTEX    os_mutex_ptr

typedef enum{
    eDht11Finishi = 1<<0,
    eSgp30Finishi = 1<<1,
    eGprsFinishi  = 1<<2
}egFlagTypedef;

extern SemaphoreHandle_t PRINTF_MUTEX;
extern EventGroupHandle_t os_eg_sampleCpl;

void RTOS_Init();
void RTOS_Start();