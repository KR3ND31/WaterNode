#pragma once
#include <stdbool.h>

extern volatile bool valve_open;

void set_valve_state(bool state);
bool get_valve_state(void);