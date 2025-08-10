#pragma once
#include <stdbool.h>
#include <stdint.h>

uint32_t millis(void);
void init_millis(void);

static inline uint32_t now_ms(void) {
    return millis();
}

// �����ӧ֧�ܧ�: �է���ڧԧݧ� �ݧ� �ާ�ާ֧ߧ�� �ӧ�֧ާ֧ߧ� t
static inline bool time_reached(uint32_t t) {
    return (int32_t)(now_ms() - t) >= 0;
}

// �����ӧ֧�ܧ�: �����ݧ� �ݧ� >= dur_ms �ާ� �� �ާ�ާ֧ߧ�� start
static inline bool elapsed_ge(uint32_t start, uint32_t dur_ms) {
    return (uint32_t)(now_ms() - start) >= dur_ms;
}