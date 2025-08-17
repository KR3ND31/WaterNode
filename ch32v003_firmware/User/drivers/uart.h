#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "ch32v00x_usart.h"
#include "core/packet.h"
#include "timer.h"

#define UART_BAUDRATE 19200

extern volatile uint32_t uart_last_rx_ms; // время последней активности на RX
extern volatile uint32_t idle_guard_ms; // пауза тишины перед отправкой UID

extern volatile uint8_t rx_buffer[MAX_PACKET_SIZE];
extern volatile uint8_t rx_index;
extern volatile uint8_t expected_length;

extern volatile uint16_t rx_crc_errors;

void USARTx_CFG(void);
void RS485_Dir_Init(void);
void RS485_SetTX(void);
void RS485_SetRX(void);
void RS485_Send(uint8_t *data, uint8_t len);
void UART_SetInterByteTimeoutMs(uint16_t timeout_ms);
void UART_SetAutoInterByteTimeout(void);

// Обработчики
void USART1_IRQHandler(void);
void TIM1_UP_IRQHandler(void);

bool uart_rx_busy(void);
bool uart_line_idle(void);