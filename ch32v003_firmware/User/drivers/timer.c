#include "timer.h"
#include "ch32v00x.h"

static volatile uint32_t system_ms = 0;

uint32_t now_ms(void) {
    return system_ms;
}

void DelayMs(uint32_t ms) {
    // Блокирующая задержка на базе системного таймера
    uint32_t end = now_ms() + ms;
    while ((int32_t)(end - now_ms()) > 0) { __NOP(); }
}

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void) {
    // Для WCH SysTick нужно чистить SR
    SysTick->SR = 0;
    system_ms++;
}

void DelayInit(void) {
    SystemCoreClockUpdate();                 // Узнаём текущую частоту ядра
    
    SysTick->CMP = (SystemCoreClock / 1000) - 1;
    SysTick->CNT = 0;
    SysTick->CTLR = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3); // ENABLE | TICKINT | CLKSOURCE | SWIE
    SysTick->SR = 0;
    
    __enable_irq(); 
    NVIC_EnableIRQ(SysTicK_IRQn);
}