#include "protocol.h"
#include "drivers/device.h"
#include "drivers/timer.h"
#include "drivers/uart.h"
#include "features/moisture.h"
#include "features/valve.h"
#include "drivers/system.h"

volatile uint8_t last_cmd_seen = 0;

volatile bool uid_reply_pending = false;
volatile uint32_t uid_reply_at = 0;

volatile bool awaiting_address_commit = false;
volatile uint32_t commit_deadline = 0;

volatile bool awaiting_reset_commit = false;
volatile uint32_t reset_commit_deadline = 0;

volatile bool pending_reboot = false;

static uint8_t pending_uid[4];
static uint8_t pending_addr = 0;

volatile uint32_t last_heartbeat_ms = 0;

volatile uint32_t heartbeat_timeout_ms = 10000;  // по умолчанию 10 секунд


bool is_uid_match(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < 4; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

bool is_autoaddr_cmd(uint8_t cmd) {
    return cmd == CMD_UID_REQUEST || cmd == CMD_ADDR_SET_P1 || cmd == CMD_ADDR_SET_P2;
}

bool is_broadcast_cmd_allowed(uint8_t cmd) {
    return cmd == CMD_HEARTBEAT;
}

void handle_packet(uint8_t *packet) {
    uint8_t addr = packet[0];
    uint8_t cmd  = packet[1];
    uint8_t len  = packet[2];
    uint8_t *data = &packet[3];

    if (cmd != CMD_STATUS) {
        last_cmd_seen = cmd; // записываем последнюю статусную команду для статистики
    } 

    // 1. Устройство без адреса: принимает только автоадресационные команды
    if (DEVICE_ADDR == 0x00 && !is_autoaddr_cmd(cmd))
        return;

    // 2. Устройство с адресом: игнорирует автоадресационные команды
    if (DEVICE_ADDR != 0x00 && is_autoaddr_cmd(cmd))
        return;

    // 3. Проверка адреса: либо адрес равен нашему, либо это broadcast-команда
    if (addr != DEVICE_ADDR && !(addr == 0x00 && is_broadcast_cmd_allowed(cmd)))
        return;

    switch (cmd) {
        case CMD_PING:
            if (len != 0) break;
            
            response_packet.addr = DEVICE_ADDR;
            response_packet.cmd = CMD_PING;
            response_packet.len = 0;
            response_pending = true;
            break;

        case CMD_GET_MOISTURE:
            if (len != 0) break;

            uint16_t moist = get_moisture();

            response_packet.addr = DEVICE_ADDR;
            response_packet.cmd = CMD_GET_MOISTURE;
            response_packet.len = 2;
            response_packet.data[0] = moist >> 8;
            response_packet.data[1] = moist & 0xFF;
            response_pending = true;
            
            break;

        case CMD_SET_VALVE:
            if (len != 1) break;

            set_valve_state(data[0] != 0);

            response_packet.addr = DEVICE_ADDR;
            response_packet.cmd  = CMD_SET_VALVE;
            response_packet.len  = 0;
            response_pending = true;

            break;

        case CMD_GET_VALVE:
            if (len != 0) break;
            
            response_packet.addr = DEVICE_ADDR;
            response_packet.cmd  = CMD_GET_VALVE;
            response_packet.len  = 1;
            response_packet.data[0] = valve_is_open();
            response_pending = true;

            break;

        case CMD_REBOOT:
            if (len != 0) break;

            response_packet.addr = DEVICE_ADDR;
            response_packet.cmd  = CMD_REBOOT;
            response_packet.len  = 0;
            response_pending = true;

            pending_reboot = true;

            break;

        case CMD_UID_REQUEST:
            if (DEVICE_ADDR != 0x00) break;
            if (len != 0) break;

            uint32_t uid_value =
                ((uint32_t)device_uid[0] << 24) |
                ((uint32_t)device_uid[1] << 16) |
                ((uint32_t)device_uid[2] << 8)  |
                ((uint32_t)device_uid[3]);

            response_packet.addr = 0x00;
            response_packet.cmd  = CMD_UID_REQUEST;
            response_packet.len  = 4;

            for (int i = 0; i < 4; i++) {
                response_packet.data[i] = device_uid[i];
            }
            
            uint32_t jitter = 100 + (uid_value % 1024);
            uid_reply_pending = true;
            uid_reply_at = now_ms() + jitter;
            
            break;

        case CMD_ADDR_SET_P1:  // Запрос установки адреса (фаза 1)
            if (DEVICE_ADDR != 0x00) break;
            if (len != 5) break;
            // Сравниваем UID в сообщении и свой
            if (is_uid_match(device_uid, data)) {
                for (int i = 0; i < 4; i++) pending_uid[i] = data[i];
                pending_addr = data[4];

                awaiting_address_commit = true;
                commit_deadline = now_ms() + 5000;  // 5 секунд на подтверждение

                response_packet.addr = 0x00;
                response_packet.cmd  = CMD_ADDR_SET_P1;
                response_packet.len  = 0;
                response_pending = true;
            }
            break;

        case CMD_ADDR_SET_P2: // Подтверждение установки адреса (фаза 2)
            if (DEVICE_ADDR != 0x00) break;
            if (len != 4) break;
            if (!awaiting_address_commit) break;

            if (is_uid_match(device_uid, data)) {
                DEVICE_ADDR = pending_addr;
                save_device_address(DEVICE_ADDR);

                awaiting_address_commit = false;

                response_packet.addr = DEVICE_ADDR;
                response_packet.cmd  = CMD_ADDR_SET_P2;
                response_packet.len  = 0;
                response_pending = true;
            }
            break;

        case CMD_ADDR_RESET_P1:  // запрос сброса адреса (фаза 1)
            // У нас есть адрес, и он совпадает с data[0]
            if (DEVICE_ADDR != data[0]) break;
            if (len != 2) break;
            if (data[1] != 0xA5) break;

            awaiting_reset_commit = true;
            reset_commit_deadline = now_ms() + 5000;  // 5 секунд на подтверждение

            // ACK на F3 от текущего адреса
            response_packet.addr = DEVICE_ADDR;
            response_packet.cmd  = CMD_ADDR_RESET_P1;
            response_packet.len  = 0;
            response_pending = true;

            break;

        case CMD_ADDR_RESET_P2:  // подтверждение сброса адреса (фаза 2)
            if (DEVICE_ADDR != data[0]) break;
            if (len != 2) break;
            if (data[1] != 0xA5) break;
            if (!awaiting_reset_commit) break;

            DEVICE_ADDR = 0x00;
            save_device_address(DEVICE_ADDR);
            awaiting_reset_commit = false;

            // ACK на F4 уже с addr=0x00
            response_packet.addr = 0x00;
            response_packet.cmd  = CMD_ADDR_RESET_P2;
            response_packet.len  = 0;
            response_pending = true;

            break;

        case CMD_STATUS: {  // Health / Status
            uint8_t flags = 0;
            if (awaiting_address_commit) flags |= (1u << 0);
            if (awaiting_reset_commit)   flags |= (1u << 1);

            uint32_t up_s = now_ms() / 1000u;

            response_packet.addr = DEVICE_ADDR;
            response_packet.cmd  = CMD_STATUS;
            response_packet.len  = 14;

            // 0..3
            response_packet.data[0] = flags;
            response_packet.data[1] = valve_is_open();
            response_packet.data[2] = DEVICE_ADDR;
            response_packet.data[3] = last_cmd_seen;

            // 4..7 uptime_s
            response_packet.data[4] = (uint8_t)(up_s >> 24);
            response_packet.data[5] = (uint8_t)(up_s >> 16);
            response_packet.data[6] = (uint8_t)(up_s >> 8);
            response_packet.data[7] = (uint8_t)(up_s & 0xFF);

            // 8..9 rx_crc_errors
            response_packet.data[8]  = (uint8_t)(rx_crc_errors >> 8);
            response_packet.data[9]  = (uint8_t)(rx_crc_errors & 0xFF);

            // 10..13 device_uid
            for (int i = 0; i < 4; i++) {
                response_packet.data[10 + i] = device_uid[i];
            }

            response_pending = true;
        } break;

        case CMD_HEARTBEAT:
            last_heartbeat_ms = now_ms(); // обновляем таймер живости
            break;

        case CMD_SET_TIMEOUT:
            if (len != 2) break;

            uint16_t new_timeout = ((uint16_t)data[0] << 8) | data[1];
            if (new_timeout >= 1000 && new_timeout <= 60000) {  // допустим от 1 до 60 сек
                heartbeat_timeout_ms = new_timeout;

                response_packet.addr = DEVICE_ADDR;
                response_packet.cmd  = CMD_SET_TIMEOUT;
                response_packet.len  = 0;
                response_pending = true;
            } else {
                response_packet.addr = DEVICE_ADDR;
                response_packet.cmd  = CMD_SET_TIMEOUT;
                response_packet.len  = 1;
                response_packet.data[0]  = 1;
                response_pending = true;
            }
            break;

        default:
            break;
    }
}

void protocol_poll(void)
{
    // 1) Отложенная отправка UID (после джиттера и окна тишины)
    if (uid_reply_pending && time_reached(uid_reply_at)) {
        // Проверка RX: если нет активности — можно отправить UID
        if (uart_line_idle()) {
            // линия молчит — отправляем F0-ответ с UID
            uid_reply_pending = false;
            response_pending  = true;  // response_packet уже заполнен обработчиком F0
        }
        // если линия не молчит — НЕ снимаем uid_reply_pending,
        // пусть попробует на следующей итерации, когда станет тихо
    }

    // 2) Таймаут на подтверждение адреса (F1 -> F2)
    if (awaiting_address_commit && time_reached(commit_deadline)) {
        awaiting_address_commit = false;
        
    // Таймаут на подтверждение адреса (F3 -> F4)
    }
    if (awaiting_reset_commit && time_reached(reset_commit_deadline)) {
        awaiting_reset_commit = false;
    }
    

    // 3) Пришёл пакет — разбираем
    if (packet_ready) {
        packet_ready = false;
        handle_packet((uint8_t*)&received_packet);
    }

    // 4) Готов ответ — отправляем (через транспорт)
    if (response_pending && uart_line_idle()) {
        response_pending = false;
        send_packet((const Packet *)&response_packet);
    }

    // 5) Перезагрузка (после отправки ответа)
    if (pending_reboot) {
        pending_reboot = false;
        system_soft_reset();
    }

    // 6) Защита: если нет heartbeat слишком долго — аварийно закрыть клапан
    if (valve_is_open() && now_ms() - last_heartbeat_ms > heartbeat_timeout_ms) {
        set_valve_state(false);
    }
}