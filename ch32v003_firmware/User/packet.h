#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_PACKET_SIZE 32

typedef struct {
    uint8_t addr;
    uint8_t cmd;
    uint8_t len;
    uint8_t data[MAX_PACKET_SIZE - 5]; // §Þ§Ñ§Ü§ã§Ú§Þ§Ñ§Ý§î§ß§à §Õ§à§á§å§ã§ä§Ú§Þ§Ñ§ñ §Õ§Ý§Ú§ß§Ñ payload
    uint16_t crc;
} Packet;

extern volatile bool uid_reply_pending;
extern volatile uint32_t uid_reply_at;

extern volatile bool awaiting_address_commit;
extern volatile uint32_t commit_deadline;

extern volatile bool awaiting_reset_commit;
extern volatile uint32_t reset_commit_deadline;

extern volatile bool packet_ready;
extern volatile Packet received_packet;

extern volatile bool response_pending;
extern volatile Packet response_packet;

uint16_t crc16(const uint8_t *data, uint8_t len);
bool is_uid_match(const uint8_t *a, const uint8_t *b);
void send_packet(const Packet *p);
void handle_packet(uint8_t *packet);