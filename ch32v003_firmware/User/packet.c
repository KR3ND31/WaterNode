#include "packet.h"
#include "device.h"
#include "moisture.h"
#include "timer.h"
#include "valve.h"
#include "uart.h"
#include "debug.h"

volatile Packet received_packet;
volatile bool packet_ready = false;

volatile Packet response_packet;
volatile bool response_pending = false;

volatile bool uid_reply_pending = false;
volatile uint32_t uid_reply_at = 0;

volatile bool awaiting_address_commit = false;
volatile uint32_t commit_deadline = 0;

volatile bool awaiting_reset_commit = false;
volatile uint32_t reset_commit_deadline = 0;

volatile uint8_t last_cmd_seen = 0;

uint8_t pending_uid[4];
uint8_t pending_addr = 0;


uint16_t crc16(const uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xA001 : 0);
    }
    return crc;
}

bool is_uid_match(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < 4; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
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

void handle_packet(uint8_t *packet) {
    uint8_t addr = packet[0];
    uint8_t cmd  = packet[1];
    uint8_t len  = packet[2];
    uint8_t *data = &packet[3];

    last_cmd_seen = cmd; // �٧ѧ�ڧ��ӧѧ֧� ����ݧ֧էߧ�� �ܧ�ާѧߧէ� �էݧ� ���ѧ�ڧ��ڧܧ�

    // �������ۧ��ӧ� �ҧ֧� �ѧէ�֧�� ���ڧߧڧާѧ֧� ���ݧ�ܧ� �ܧ�ާѧߧէ� 0xF0 �� 0xF1 �� 0xF2
    if (DEVICE_ADDR == 0x00 && cmd != 0xF0 && cmd != 0xF1 && cmd != 0xF2)
        return;

    // �������ۧ��ӧ� �� �ѧէ�֧��� �ڧԧߧ��ڧ��֧� ��ڧ��ܧ�ӧ֧�ѧ�֧ݧ�ߧ�� 0xF0 �� 0xF1 �� 0xF2
    if (DEVICE_ADDR != 0x00 && (cmd == 0xF0 || cmd == 0xF1 || cmd == 0xF2))
        return;

    // �����ӧ֧�ܧ� �ѧէ�֧�� (�ѧէ�֧� �է�ݧا֧� �ҧ��� �ߧѧ� �ڧݧ� ��ڧ��ܧ�ӧ֧�ѧ�֧ݧ�ߧ��)
    if (addr != DEVICE_ADDR && addr != 0x00)
        return;

    switch (cmd) {
        case 0x01:  // ping
            response_packet.addr = addr;
            response_packet.cmd = 0x01;
            response_packet.len = 0;
            response_pending = true;
            break;

        case 0x02: {  // �٧ѧ���� �ӧݧѧاߧ����
            uint16_t moist = get_moisture();

            response_packet.addr = addr;
            response_packet.cmd = 0x02;
            response_packet.len = 2;
            response_packet.data[0] = moist >> 8;
            response_packet.data[1] = moist & 0xFF;
            response_pending = true;
            break;
        }

        case 0x03:  // ����ѧӧݧ֧ߧڧ� �ܧݧѧ�ѧߧ��
            if (len >= 1) {
                set_valve_state(data[0] == 1);

                response_packet.addr = addr;
                response_packet.cmd  = 0x03;
                response_packet.len  = 0;
                response_pending = true;
            }
            break;

        case 0xF0:   // ���ѧ���� ��� �ާѧ��֧��: �ܧ�� �֧�� �ҧ֧� �ѧէ�֧�� �� �����ѧӧ��� ��ӧ�� UID
            if (DEVICE_ADDR == 0x00) {
                uint32_t uid_value =
                    ((uint32_t)device_uid[0] << 24) |
                    ((uint32_t)device_uid[1] << 16) |
                    ((uint32_t)device_uid[2] << 8)  |
                    ((uint32_t)device_uid[3]);

                response_packet.addr = 0x00;
                response_packet.cmd  = 0xF0;
                response_packet.len  = 4;

                for (int i = 0; i < 4; i++) {
                    response_packet.data[i] = device_uid[i];
                }
                
                uint32_t jitter = 100 + (uid_value % 1024);
                uid_reply_pending = true;
                uid_reply_at = now_ms() + jitter;
            }
            break;

        case 0xF1:  // ���ѧ���� ����ѧߧ�ӧܧ� �ѧէ�֧�� (��ѧ٧� 1)
            if (DEVICE_ADDR == 0x00 && len >= 5) {
                // ����ѧӧߧڧӧѧ֧� UID �� ����ҧ�֧ߧڧ� �� ��ӧ��
                if (is_uid_match(device_uid, data)) {
                    for (int i = 0; i < 4; i++) pending_uid[i] = data[i];
                    pending_addr = data[4];

                    awaiting_address_commit = true;
                    commit_deadline = now_ms() + 5000;  // 5 ��֧ܧ�ߧ� �ߧ� ���է�ӧ֧�اէ֧ߧڧ�

                    response_packet.addr = 0x00;
                    response_packet.cmd  = 0xF1;
                    response_packet.len  = 0;
                    response_pending = true;
                }
            }
            break;

        case 0xF2: // ����է�ӧ֧�اէ֧ߧڧ� ����ѧߧ�ӧܧ� �ѧէ�֧�� (��ѧ٧� 2)
            if (DEVICE_ADDR == 0x00 && len >= 4 && awaiting_address_commit) {
                if (is_uid_match(device_uid, data)) {
                    DEVICE_ADDR = pending_addr;
                    save_device_address(DEVICE_ADDR);

                    awaiting_address_commit = false;

                    response_packet.addr = DEVICE_ADDR;
                    response_packet.cmd  = 0xF2;
                    response_packet.len  = 0;
                    response_pending = true;
                }
            }
        break;

        case 0xF3:  // �٧ѧ���� ��ҧ���� �ѧէ�֧�� (��ѧ٧� 1)
            // �� �ߧѧ� �֧��� �ѧէ�֧�, �� ��� ���ӧ�ѧէѧ֧� �� data[0]
            if (len >= 2 && DEVICE_ADDR == data[0] && data[1] == 0xA5) {
                awaiting_reset_commit = true;
                reset_commit_deadline = now_ms() + 5000;  // 5 ��֧ܧ�ߧ� �ߧ� ���է�ӧ֧�اէ֧ߧڧ�

                // ACK �ߧ� F3 ��� ��֧ܧ��֧ԧ� �ѧէ�֧��
                response_packet.addr = DEVICE_ADDR;
                response_packet.cmd  = 0xF3;
                response_packet.len  = 0;
                response_pending = true;
            }
            break;

        case 0xF4:  // ���է�ӧ֧�اէ֧ߧڧ� ��ҧ���� �ѧէ�֧�� (��ѧ٧� 2)
            if (awaiting_reset_commit && len >= 2 && DEVICE_ADDR == data[0] && data[1] == 0xA5) {
                DEVICE_ADDR = 0x00;
                save_device_address(DEVICE_ADDR);
                awaiting_reset_commit = false;

                // ACK �ߧ� F4 ��ا� �� addr=0x00
                response_packet.addr = 0x00;
                response_packet.cmd  = 0xF4;
                response_packet.len  = 0;
                response_pending = true;
            }
            break;
        default:
            break;
    }
}