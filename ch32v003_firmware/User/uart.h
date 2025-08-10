#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "packet.h"
#include "timer.h"

#define UART_BAUDRATE 19200

extern volatile uint32_t uart_last_rx_ms; // �ӧ�֧ާ� ����ݧ֧էߧ֧� �ѧܧ�ڧӧߧ���� �ߧ� RX
extern volatile uint32_t idle_guard_ms; // ��ѧ�٧� ��ڧ�ڧߧ� ��֧�֧� �����ѧӧܧ�� UID

extern volatile uint8_t rx_buffer[MAX_PACKET_SIZE];
extern volatile uint8_t rx_index;
extern volatile uint8_t expected_length;

extern volatile uint16_t rx_crc_errors;

static inline bool uart_idle_window_ok(void) {
    return elapsed_ge(uart_last_rx_ms, idle_guard_ms);
}

void DelayMs(uint32_t ms);
void USARTx_CFG(void);
void RS485_Dir_Init(void);
void RS485_SetTX(void);
void RS485_SetRX(void);
void RS485_Send(uint8_t *data, uint8_t len);
void UART_SetInterByteTimeoutMs(uint16_t timeout_ms);
void UART_SetAutoInterByteTimeout(void);

// ���ҧ�ѧҧ���ڧܧ�
void USART1_IRQHandler(void);
void TIM1_UP_IRQHandler(void);