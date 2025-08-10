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
#include "packet.h"
#include "device.h"
#include "timer.h"
#include "uart.h"
#include "moisture.h"
#include "valve.h"

#include "debug.h"

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

    Delay_Init();

    USARTx_CFG();        // §¯§Ñ§ã§ä§â§à§Û§Ü§Ñ UART
    UART_SetAutoInterByteTimeout();

    DelayMs(1000);

    init_millis();

    init_device(); // §ª§ß§Ú§è§Ú§Ý§Ú§Ù§Ñ§è§Ú§ñ UID §Ú DEVICE ADDR
    RS485_Dir_Init();    // §ª§ß§Ú§è§Ú§Ý§Ú§Ù§Ñ§è§Ú§ñ DE/RE §á§Ú§ß§Ñ    
    moisture_sensor_init(); // §ª§ß§Ú§è§Ú§Ý§Ú§Ù§Ñ§è§Ú§ñ §á§Ú§ß§Ñ §Ó§Ý§Ñ§Ø§ß§à§ã§ä§Ú

    RS485_SetRX(); // §±§à §å§Þ§à§Ý§é§Ñ§ß§Ú§ð ¡ª §â§Ö§Ø§Ú§Þ §á§â§Ú§×§Þ§Ñ

    while (1) {
        // §°§ä§Ý§à§Ø§Ö§ß§ß§Ñ§ñ §à§ä§á§â§Ñ§Ó§Ü§Ñ UID (§á§à§ã§Ý§Ö §Õ§Ø§Ú§ä§ä§Ö§â§Ñ §Ú §à§Ü§ß§Ñ §ä§Ú§ê§Ú§ß§í §ß§Ñ §Ý§Ú§ß§Ú§Ú)
        if (uid_reply_pending && time_reached(uid_reply_at)) {

            // §±§â§à§Ó§Ö§â§Ü§Ñ RX: §Ö§ã§Ý§Ú §ß§Ö§ä §Ñ§Ü§ä§Ú§Ó§ß§à§ã§ä§Ú ¡ª §Þ§à§Ø§ß§à §à§ä§á§â§Ñ§Ó§Ú§ä§î UID
            if (uart_idle_window_ok() && !USART_GetFlagStatus(USART1, USART_FLAG_RXNE)) {

                // §Ý§Ú§ß§Ú§ñ §Þ§à§Ý§é§Ú§ä ¡ª §à§ä§á§â§Ñ§Ó§Ý§ñ§Ö§Þ F0-§à§ä§Ó§Ö§ä §ã UID
                uid_reply_pending = false;
                response_pending = true; // response_packet §å§Ø§Ö §Ù§Ñ§á§à§Ý§ß§Ö§ß §Ó F0
            }
            // §Ö§ã§Ý§Ú §Ý§Ú§ß§Ú§ñ §ß§Ö §Þ§à§Ý§é§Ú§ä ¡ª §¯§¦ §ã§ß§Ú§Þ§Ñ§Ö§Þ uid_reply_pending,
            // §á§å§ã§ä§î §á§à§á§â§à§Ò§å§Ö§ä §ß§Ñ §ã§Ý§Ö§Õ§å§ð§ë§Ö§Û §Ú§ä§Ö§â§Ñ§è§Ú§Ú, §Ü§à§Ô§Õ§Ñ §ã§ä§Ñ§ß§Ö§ä §ä§Ú§ç§à
        }

        // §´§Ñ§Û§Þ§Ñ§å§ä §ß§Ñ §á§à§Õ§ä§Ó§Ö§â§Ø§Õ§Ö§ß§Ú§Ö §Ñ§Õ§â§Ö§ã§Ñ (F1 -> F2)
        if (awaiting_address_commit && time_reached(commit_deadline)) {
            awaiting_address_commit = false; // §ã§Ò§â§à§ã§Ú§ä§î §à§Ø§Ú§Õ§Ñ§ß§Ú§Ö
        }

        // §´§Ñ§Û§Þ§Ñ§å§ä §ß§Ñ §á§à§Õ§ä§Ó§Ö§â§Ø§Õ§Ö§ß§Ú§Ö §Ñ§Õ§â§Ö§ã§Ñ (F3 -> F4)
        if (awaiting_reset_commit && time_reached(reset_commit_deadline)) {
            awaiting_reset_commit = false; // §à§ä§Þ§Ö§ß§ñ§Ö§Þ §à§Ø§Ú§Õ§Ñ§ß§Ú§Ö, §ß§Ú§é§Ö§Ô§à §ß§Ö §Þ§Ö§ß§ñ§Ö§Þ
        }

        // §±§â§Ú§ê§×§Ý §á§Ñ§Ü§Ö§ä ¡ª §â§Ñ§Ù§Ò§Ú§â§Ñ§Ö§Þ (§Ó§í§Ù§í§Ó§Ñ§Û§ä§Ö §ï§ä§à §Ú§Ù §Þ§Ö§ã§ä§Ñ §à§Ò§â§Ñ§Ò§à§ä§Ü§Ú RX)
        if (packet_ready) {
            packet_ready = false;
            handle_packet((uint8_t*)&received_packet);
        }

        // §¤§à§ä§à§Ó §à§ä§Ó§Ö§ä ¡ª §à§ä§á§â§Ñ§Ó§Ý§ñ§Ö§Þ
        if (response_pending) {
            response_pending = false;
            send_packet((const Packet *)&response_packet);
        }
    }
}
