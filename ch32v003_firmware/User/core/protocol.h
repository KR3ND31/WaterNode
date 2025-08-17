#pragma once
#include "packet.h"

enum {
    // команды мастера
    CMD_PING           = 0x01,
    CMD_GET_MOISTURE   = 0x02,
    CMD_SET_VALVE      = 0x03,
    CMD_GET_VALVE      = 0x04,
    CMD_REBOOT         = 0x17,
    CMD_HEARTBEAT      = 0x11,
    CMD_STATUS         = 0x12,
    CMD_SET_TIMEOUT    = 0x13,

    // автоадресация
    CMD_UID_REQUEST    = 0xF0,
    CMD_ADDR_SET_P1    = 0xF1,
    CMD_ADDR_SET_P2    = 0xF2,
    CMD_ADDR_RESET_P1  = 0xF3,
    CMD_ADDR_RESET_P2  = 0xF4
};

extern volatile bool uid_reply_pending;
extern volatile uint32_t uid_reply_at;

extern volatile bool awaiting_address_commit;
extern volatile uint32_t commit_deadline;

extern volatile bool awaiting_reset_commit;
extern volatile uint32_t reset_commit_deadline;

extern volatile bool pending_reboot;

extern volatile uint32_t heartbeat_timeout_ms;

void protocol_poll(void);

bool is_uid_match(const uint8_t *a, const uint8_t *b);
bool is_broadcast_cmd_allowed(uint8_t cmd);
bool is_autoaddr_cmd(uint8_t cmd);
void handle_packet(uint8_t *packet);