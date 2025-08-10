#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FLASH_ADDR_DEVICE  0x08003C00

extern uint8_t device_uid[4];
extern uint8_t DEVICE_ADDR;

void init_device(void);
uint32_t get_device_uid_32(void);
void get_device_uid_bytes(uint8_t *out);
bool check_uid_match(const uint8_t *uid_data);
void save_device_address(uint8_t addr);
uint8_t load_device_address(void);