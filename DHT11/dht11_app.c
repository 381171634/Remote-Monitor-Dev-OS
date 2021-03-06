/*
 ============================================================================
 Name        : dht11_app.c
 Author      : wy
 Version     :
 Copyright   : Your copyright notice
 Description : dht11应用层
 ============================================================================
 */
#include "includes.h"

static taskManageTypedef dht11_tm = {DHT11_STEP_START,0,0};
dht11ResTypedef dht11Res;

static uint8_t app_dht11Read(dht11DataTypedef *pSrc);


/*============================================================================
 dht11任务
 ============================================================================*/
void dht11_task(void *argument)
{
    int i;
    uint8_t res;
    dht11DataTypedef readData = {0,0,0,0,0};

    COMMON_UNUSED(argument);

    while(1)
    {
        osDelay(10);

        switch(dht11_tm.step)
        {
            case DHT11_STEP_START:
                memset((void *)&dht11Res,0,sizeof(dht11Res));
                dht11_bsp.gpio_init();
                DHT11_POWER_ON;                 //上电
                                                //上电稳定1秒,电压越低，需要稳定的时间越长
                osDelay(1500);

                DBG_PRT("dht11 start ok!\n");

                dht11_tm.step++;
                break;
            case DHT11_STEP_READ:
                for(i = 0;i < 3; i++)
                {
                    taskENTER_CRITICAL();
                    res = app_dht11Read(&readData);
                    taskEXIT_CRITICAL();

                    if(res == TRUE)
                    {
                        break;
                    }

                }  
                //read success
                if(i < 3)
                {
                    DBG_PRT("dht11 read  ok!:%d\n",dht11Res.readCnt + 1);
                    //协议中要求传输的数据为整数，除以1000为最终结果，如24.3度，按24300传输
                    //负数
                    if(readData.temp_H & 0x80)
                    {
                        dht11Res.dht11_temp_sum += (-1) * ((readData.temp_H & (0x7f))* 1000 + readData.temp_L * 100);
                    }
                    else
                    {
                        dht11Res.dht11_temp_sum += readData.temp_H * 1000 + readData.temp_L * 100;
                    }
                    
                    dht11Res.dht11_wet_sum += readData.wet_H * 1000 + readData.wet_L * 100;

                    if(++dht11Res.readCnt >= DHT11_AVERAGE_CNT)
                    {
                        dht11Res.dht11_temp_avg = dht11Res.dht11_temp_sum / DHT11_AVERAGE_CNT;
                        dht11Res.dht11_wet_avg  = dht11Res.dht11_wet_sum / DHT11_AVERAGE_CNT;
                        dht11_tm.step++;
                        DBG_PRT("dht11 read finishi!:temp average:%d wet average:%d\n",dht11Res.dht11_temp_avg,dht11Res.dht11_wet_avg);
                    }
                    else
                    {
                        //稳定2s后再次读数据
                        osDelay(2000);;
                    }
                    
                    
                }
                else
                {
                    DBG_PRT("dht11 read  fail\n");
                    dht11_tm.errCnt++;
                    dht11_tm.step = DHT11_STEP_START;
                }
                
                break;

            case DHT11_STEP_POWER_OFF:
                DHT11_POWER_OFF;
                xEventGroupSetBits(os_eg_sampleCpl,eDht11Finishi);
                dht11_tm.step++;
                break;

            case DHT11_STEP_FINISH:
                break;
            default:
                break;
        }
    }
}

/*============================================================================
 dht11读数据
 pSrc：dht11数据结构体指针
 return：成败
 ============================================================================*/
uint8_t app_dht11Read(dht11DataTypedef *pSrc)
{
    uint8_t res = TRUE;
    uint16_t i,j,highCnt,lowCnt;
    uint8_t recvData[5] = {0};
    int32_t timeout_us;

    
    DHT11_SDA_MODE_OUTPUT;          //sda输出高
    DHT11_SDA_WRITE_H;
    
    DHT11_SDA_WRITE_L;              //起始信号
    dht11_bsp.dly_ms(18);
    DHT11_SDA_WRITE_H;
    DHT11_SDA_MODE_INPUT;           //转为输入 等待应答信号

    timeout_us = 35;                //等待响应低
    while(timeout_us-- && (DHT11_SDA_READ != DHT11_PIN_STATE_L))
    {
        dht11_bsp.dly_us(1);
    }

    if(timeout_us <= 0)             //未响应直接返回
    {
        res = FALSE;
        goto exit;
    }

    timeout_us = 88;                //等待响应高    
    while(timeout_us-- && (DHT11_SDA_READ != DHT11_PIN_STATE_H))                                 
    {
        dht11_bsp.dly_us(1);
    }

    if(timeout_us <= 0)             //未响应直接返回
    {
        res = FALSE;
        goto exit;
    }

    timeout_us = 100;                //响应结束，等待开始发送数据
    while(timeout_us-- && (DHT11_SDA_READ != DHT11_PIN_STATE_L))
    {
        dht11_bsp.dly_us(1);
    }   

    if(timeout_us <= 0)             //未响应直接返回
    {
        res = FALSE;
        goto exit;
    }

    for(i = 0;i < 5; i++)
    {
        for(j = 0;j < 8;j ++)
        {
            highCnt = 0;
			lowCnt = 0;
            
            while(DHT11_SDA_READ == DHT11_PIN_STATE_L && lowCnt <= 60 )
            {
                lowCnt++;
                dht11_bsp.dly_us(1);
            }
            while(DHT11_SDA_READ == DHT11_PIN_STATE_H && highCnt <= 70 )
            {
                highCnt++;
                dht11_bsp.dly_us(1);
            }
            if(highCnt >= lowCnt)
            {
                recvData[i] |= (1 << (7 - j));
            }
			
        }
    }

    //判校验和
    if(recvData[0] + recvData[1] + recvData[2] + recvData[3] != recvData[4] )
    {
        res = FALSE;
        goto exit;
    }
    else
    {
        pSrc->wet_H = recvData[0];
        pSrc->wet_L = recvData[1];
        pSrc->temp_H = recvData[2];
        pSrc->temp_L = recvData[3];
        pSrc->sum = recvData[4];
    }

exit:
    
    return res;
}