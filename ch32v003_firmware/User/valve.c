#include "valve.h"

volatile bool valve_open = false;

void set_valve_state(bool state) {
    valve_open = state;
    if (valve_open) {
        
    } else {
        
    }
}

bool get_valve_state(void) {
    return valve_open;
}