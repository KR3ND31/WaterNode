#include "drivers/system.h"
#include "drivers/timer.h"
#include "ch32v00x.h"

void system_soft_reset(void)
{
    // небольшая задержка, чтобы успели уйти последние байты
    DelayMs(20);

    // встроенная функция CMSIS для софт-ресета
    NVIC_SystemReset();

    // сюда мы уже не вернёмся
}