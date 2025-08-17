#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_PACKET_SIZE 32

typedef struct {
    uint8_t addr;
    uint8_t cmd;
    uint8_t len;
    uint8_t data[MAX_PACKET_SIZE - 5]; // максимально допустимая длина payload
    uint16_t crc;
} Packet;

extern volatile bool packet_ready;
extern volatile Packet received_packet;

extern volatile bool response_pending;
extern volatile Packet response_packet;

uint16_t crc16(const uint8_t *data, uint8_t len);
void send_packet(const Packet *p);