#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FLASH_ADDR_DEVICE  0x08003C00

extern uint8_t device_uid[4];
extern uint8_t DEVICE_ADDR;

void init_device(void);
void save_device_address(uint8_t addr);
uint8_t load_device_address(void);