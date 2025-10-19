#include "valve.h"
#include "ch32v00x_gpio.h"

// PC2 push-pull выход
#define VALVE_GPIO GPIOC
#define VALVE_PIN  GPIO_Pin_2

#define VALVE_ACTIVE_HIGH 1

bool valve_open = false;

void valve_init(void)
{
    // Тактирование порта
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin   = VALVE_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(VALVE_GPIO, &gpio);

#if VALVE_ACTIVE_HIGH
    GPIO_ResetBits(VALVE_GPIO, VALVE_PIN);
#else
    GPIO_SetBits(VALVE_GPIO, VALVE_PIN);
#endif
    valve_open = false;
}

void set_valve_state(bool on) {
    valve_open = on;
#if VALVE_ACTIVE_HIGH
    GPIO_WriteBit(VALVE_GPIO, VALVE_PIN, on ? Bit_SET : Bit_RESET);
#else
    GPIO_WriteBit(VALVE_GPIO, VALVE_PIN, on ? Bit_RESET : Bit_SET);
#endif
}

bool valve_is_open(void) {
    return valve_open;
}