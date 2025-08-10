#pragma once
#include <stdbool.h>
#include <stdint.h>

uint32_t millis(void);
void init_millis(void);

static inline uint32_t now_ms(void) {
    return millis();
}

// 妤把抉志快把抗忘: 忱抉扼找我忍抖我 抖我 技抉技快扶找忘 志把快技快扶我 t
static inline bool time_reached(uint32_t t) {
    return (int32_t)(now_ms() - t) >= 0;
}

// 妤把抉志快把抗忘: 扭把抉扮抖抉 抖我 >= dur_ms 技扼 扼 技抉技快扶找忘 start
static inline bool elapsed_ge(uint32_t start, uint32_t dur_ms) {
    return (uint32_t)(now_ms() - start) >= dur_ms;
}