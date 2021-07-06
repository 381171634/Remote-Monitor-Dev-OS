/*
 ============================================================================
 Name        : lowPower.c
 Author      : wy
 Version     :
 Copyright   : Your copyright notice
 Description : 低功耗处理
 ============================================================================
 */

#include "includes.h"
/*============================================================================
 低功耗RTC唤醒中断
 ============================================================================*/
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    HAL_NVIC_SystemReset();
}

static void lowPwr_GPIO_Set()
{
    GPIO_InitTypeDef GPIO_InitStruct;
    //GPRS模块的DCDC引脚保持使能，拉低
    //dht11的VCC引脚推挽输出高，同时DATA也被拉高
    //其余引脚配置为模拟输入
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_All & ~(DCDC_ENABLE_Pin);
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_All;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DHT11_POWER_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_POWER_Port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(DCDC_ENABLE_GPIO_Port,DCDC_ENABLE_Pin,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DHT11_POWER_Port,DHT11_POWER_Pin,GPIO_PIN_SET);
}

/*============================================================================
 低功耗进入函数
 ============================================================================*/
static void enter_lowPwr()
{
    RTC_AlarmTypeDef alarm;
    RTC_TimeTypeDef time;
    memset((void *)&alarm, 0, sizeof(alarm));

    DHT11_POWER_OFF;
    SGP30_Reset();
    GPRS_POWER_OFF;

    lowPwr_GPIO_Set();

    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);

    DBG_PRT("time->%02d:%02d:%02d\n",time.Hours,time.Minutes,time.Seconds);

    time.Seconds = 0;
    time.Minutes = 0;
    time.Hours += 1;
    if (time.Hours > 23)
    {
        time.Hours = 0;
    }
    
    alarm.AlarmTime.Hours = time.Hours;
    alarm.AlarmTime.Minutes = time.Minutes;
    alarm.AlarmTime.Seconds = time.Seconds;
    
    HAL_RTC_SetAlarm_IT(&hrtc,&alarm,RTC_FORMAT_BIN);
    
    SysTick->CTRL = 0x00;   //关闭定时器
    SysTick->VAL = 0x00;    //清空val,清空定时器
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_STOPENTRY_WFI);
}

void lowpower_task(void *argument)
{
    BaseType_t bit;
    COMMON_UNUSED(argument);

    while(1)
    {
        osDelay(1000);
        bit = xEventGroupWaitBits(os_eg_sampleCpl,eGprsFinishi,pdFALSE,pdTRUE,0);
        if(bit == eGprsFinishi || HAL_GetTick() >= 120000)
        {
            enter_lowPwr();
        }
    }


}
