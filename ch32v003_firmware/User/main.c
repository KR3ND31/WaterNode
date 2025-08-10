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

    USARTx_CFG();        // ���ѧ����ۧܧ� UART
    UART_SetAutoInterByteTimeout();

    DelayMs(1000);

    init_millis();

    init_device(); // ���ߧڧ�ڧݧڧ٧ѧ�ڧ� UID �� DEVICE ADDR
    RS485_Dir_Init();    // ���ߧڧ�ڧݧڧ٧ѧ�ڧ� DE/RE ��ڧߧ�    
    moisture_sensor_init(); // ���ߧڧ�ڧݧڧ٧ѧ�ڧ� ��ڧߧ� �ӧݧѧاߧ����

    RS485_SetRX(); // ���� ��ާ�ݧ�ѧߧڧ� �� ��֧اڧ� ���ڧקާ�

    while (1) {
        // ����ݧ�ا֧ߧߧѧ� �����ѧӧܧ� UID (����ݧ� �էاڧ��֧�� �� ��ܧߧ� ��ڧ�ڧߧ� �ߧ� �ݧڧߧڧ�)
        if (uid_reply_pending && time_reached(uid_reply_at)) {

            // �����ӧ֧�ܧ� RX: �֧�ݧ� �ߧ֧� �ѧܧ�ڧӧߧ���� �� �ާ�اߧ� �����ѧӧڧ�� UID
            if (uart_idle_window_ok() && !USART_GetFlagStatus(USART1, USART_FLAG_RXNE)) {

                // �ݧڧߧڧ� �ާ�ݧ�ڧ� �� �����ѧӧݧ�֧� F0-���ӧ֧� �� UID
                uid_reply_pending = false;
                response_pending = true; // response_packet ��ا� �٧ѧ��ݧߧ֧� �� F0
            }
            // �֧�ݧ� �ݧڧߧڧ� �ߧ� �ާ�ݧ�ڧ� �� ���� ��ߧڧާѧ֧� uid_reply_pending,
            // ������ ������ҧ�֧� �ߧ� ��ݧ֧է���֧� �ڧ�֧�ѧ�ڧ�, �ܧ�ԧէ� ���ѧߧ֧� ��ڧ��
        }

        // ���ѧۧާѧ�� �ߧ� ���է�ӧ֧�اէ֧ߧڧ� �ѧէ�֧�� (F1 -> F2)
        if (awaiting_address_commit && time_reached(commit_deadline)) {
            awaiting_address_commit = false; // ��ҧ���ڧ�� ��اڧէѧߧڧ�
        }

        // ���ѧۧާѧ�� �ߧ� ���է�ӧ֧�اէ֧ߧڧ� �ѧէ�֧�� (F3 -> F4)
        if (awaiting_reset_commit && time_reached(reset_commit_deadline)) {
            awaiting_reset_commit = false; // ���ާ֧ߧ�֧� ��اڧէѧߧڧ�, �ߧڧ�֧ԧ� �ߧ� �ާ֧ߧ�֧�
        }

        // ����ڧ�ק� ��ѧܧ֧� �� ��ѧ٧ҧڧ�ѧ֧� (�ӧ�٧�ӧѧۧ�� ���� �ڧ� �ާ֧��� ��ҧ�ѧҧ��ܧ� RX)
        if (packet_ready) {
            packet_ready = false;
            handle_packet((uint8_t*)&received_packet);
        }

        // ������� ���ӧ֧� �� �����ѧӧݧ�֧�
        if (response_pending) {
            response_pending = false;
            send_packet((const Packet *)&response_packet);
        }
    }
}
