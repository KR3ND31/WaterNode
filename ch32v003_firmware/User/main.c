/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/25
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *Multiprocessor communication mode routine:
 *Master:USART1_Tx(PD5)\USART1_Rx(PD6).
 *This routine demonstrates that USART1 receives the data sent by CH341 and inverts
 *it and sends it (baud rate 115200).
 *
 *Hardware connection:PD5 -- Rx
 *                     PD6 -- Tx
 *
 */

#include <stdlib.h>
#include <stdbool.h>
#include "core/protocol.h"
#include "drivers/device.h"
#include "drivers/uart.h"
#include "features/moisture.h"
#include "features/valve.h"


/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    srand(SysTick->CNT);

    DelayInit();

    DelayMs(1000);

    init_device(); // Иницилизация UID и DEVICE ADDR
    RS485_Dir_Init();    // Иницилизация DE/RE пина   
    USARTx_CFG();        // Настройка UART 
    UART_SetAutoInterByteTimeout(); // конфиг таймаута
    moisture_sensor_init(); // Иницилизация пина влажности

    RS485_SetRX(); // По умолчанию — режим приёма

    while (1) {
        protocol_poll();
    }
}
