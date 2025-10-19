#pragma once
#include <stdbool.h>

extern bool valve_open;

void  valve_init(void);
void set_valve_state(bool on);
bool valve_is_open(void);