#include "timer.h"
#include "ch32v00x.h"
#include <stdbool.h>

volatile uint32_t system_millis = 0;

uint32_t millis(void) {
    return system_millis;
}

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void) {
    SysTick->SR = 0;
    system_millis++;
}

void init_millis(void) {
    SystemCoreClockUpdate();                 // §µ§Ù§ß§Ñ§×§Þ §ä§Ö§Ü§å§ë§å§ð §é§Ñ§ã§ä§à§ä§å §ñ§Õ§â§Ñ
    
    SysTick->CMP = (SystemCoreClock / 1000) - 1;
    SysTick->CNT = 0;
    SysTick->CTLR = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
    SysTick->SR = 0;
    
    __enable_irq(); 
    NVIC_EnableIRQ(SysTicK_IRQn);
}