#pragma once
#include <stdbool.h>
#include <stdint.h>

uint32_t now_ms(void);
void DelayMs(uint32_t ms);
void DelayInit(void);

// Проверка: достигли ли момента времени t
static inline bool time_reached(uint32_t t) {
    return (int32_t)(now_ms() - t) >= 0;
}

// Проверка: прошло ли >= dur_ms мс с момента start
static inline bool elapsed_ge(uint32_t start, uint32_t dur_ms) {
    return (uint32_t)(now_ms() - start) >= dur_ms;
}