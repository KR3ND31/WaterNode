#pragma once
#include <stdbool.h>

extern bool valve_open;

void set_valve_state(bool on);
bool valve_is_open(void);