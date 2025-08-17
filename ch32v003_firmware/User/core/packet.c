#include "packet.h"
#include "drivers/device.h"
#include "features/moisture.h"
#include "drivers/timer.h"
#include "features/valve.h"
#include "drivers/uart.h"

volatile bool packet_ready = false;
volatile Packet received_packet;

volatile bool response_pending = false;
volatile Packet response_packet;

uint16_t crc16(const uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xA001 : 0);
    }
    return crc;
}

void send_packet(const Packet *p) {
    uint8_t buf[64];
    buf[0] = p->addr;
    buf[1] = p->cmd;
    buf[2] = p->len;

    for (uint8_t i = 0; i < p->len; i++) {
        buf[3 + i] = p->data[i];
    }

    uint16_t crc = crc16(buf, 3 + p->len);
    buf[3 + p->len] = crc & 0xFF;
    buf[4 + p->len] = crc >> 8;

    DelayMs(5); // minimum 1ms why?
    RS485_SetTX();
    DelayMs(30); // minimum 20ms why?

    for (uint8_t i = 0; i < 5 + p->len; i++) {
        USART_SendData(USART1, buf[i]);
        while (!(USART1->STATR & USART_FLAG_TXE));
    }

    while (!(USART1->STATR & USART_FLAG_TC));
    RS485_SetRX();
}