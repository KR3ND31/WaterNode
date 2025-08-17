#include "valve.h"
#include "ch32v00x_gpio.h"

#define VALVE_GPIO GPIOC
#define VALVE_PIN  GPIO_Pin_2

bool valve_open = false;


void set_valve_state(bool on) {
    valve_open = on;
    GPIO_WriteBit(VALVE_GPIO, VALVE_PIN, on ? Bit_SET : Bit_RESET);
}

bool valve_is_open(void) {
    return valve_open;
}