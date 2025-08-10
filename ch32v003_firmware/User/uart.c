#include "uart.h"
#include "debug.h"
#include "timer.h"

#define RS485_DIR_PORT GPIOC
#define RS485_DIR_PIN  GPIO_Pin_0

volatile uint32_t uart_last_rx_ms = 0; // §Ó§â§Ö§Þ§ñ §á§à§ã§Ý§Ö§Õ§ß§Ö§Û §Ñ§Ü§ä§Ú§Ó§ß§à§ã§ä§Ú §ß§Ñ RX
volatile uint32_t idle_guard_ms   = 2; // §á§Ñ§å§Ù§Ñ §ä§Ú§ê§Ú§ß§í §á§Ö§â§Ö§Õ §à§ä§á§â§Ñ§Ó§Ü§à§Û UID

volatile uint8_t rx_buffer[MAX_PACKET_SIZE];
volatile uint8_t rx_index = 0;
volatile uint8_t expected_length = 0;

volatile uint16_t rx_crc_errors = 0;

#define GPIO_TO_RCC_PORT(x) \
    ((x) == GPIOA ? RCC_APB2Periph_GPIOA : \
     (x) == GPIOC ? RCC_APB2Periph_GPIOC : \
     (x) == GPIOD ? RCC_APB2Periph_GPIOD : 0)

void DelayMs(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 8000; i++) {
        __NOP();
    }
}

void USARTx_CFG(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    AFIO->PCFR1 |= (1 << 24); // USART1_REMAP

    /* USART1 TX-->D.5   RX-->D.6 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = UART_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);     // §£§Ü§Ý§ð§é§Ñ§Ö§Þ §á§â§Ö§â§í§Ó§Ñ§ß§Ú§Ö §á§à §á§â§Ú§×§Þ§å
    
    // §²§Ñ§Ù§â§Ö§ê§Ñ§Ö§Þ §á§â§Ö§â§í§Ó§Ñ§ß§Ú§Ö §Ó NVIC
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    USART_Cmd(USART1, ENABLE);
}

// === TIM1 §à§Ò§â§Ñ§Ò§à§ä§é§Ú§Ü ¡ª §ã§Ò§â§à§ã §á§à §ä§Ñ§Û§Þ§Ñ§å§ä§å ===
void TIM1_UP_IRQHandler(void) __attribute__((interrupt));
void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update)) {
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);

        // §´§Ñ§Û§Þ§Ñ§å§ä §Þ§Ö§Ø§Õ§å §Ò§Ñ§Û§ä§Ñ§Þ§Ú ¡ª §ã§Ò§â§à§ã §á§â§Ú§×§Þ§Ñ
        rx_index = 0;
        expected_length = 0;
    }
}

// === §°§Ò§â§Ñ§Ò§à§ä§é§Ú§Ü §á§â§Ö§â§í§Ó§Ñ§ß§Ú§ñ USART1 ===
void USART1_IRQHandler(void) __attribute__((interrupt));
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE)) {
        uart_last_rx_ms = now_ms();

        uint8_t byte = USART_ReceiveData(USART1);

        // §³§Ò§â§à§ã §ä§Ñ§Û§Þ§Ö§â§Ñ ¡ª §Ü§Ñ§Ø§Õ§í§Û §Ò§Ñ§Û§ä §Ù§Ñ§ß§à§Ó§à §Ù§Ñ§á§å§ã§Ü§Ñ§Ö§ä §ä§Ñ§Û§Þ§Ñ§å§ä
        TIM_SetCounter(TIM1, 0);

        if (rx_index < MAX_PACKET_SIZE) {
            rx_buffer[rx_index++] = byte;

            if (rx_index == 3) {
                // §´§à§Ý§î§Ü§à §é§ä§à §á§à§Ý§å§é§Ú§Ý§Ú LEN
                expected_length = 3 + rx_buffer[2] + 2;  // §Ù§Ñ§Ô§à§Ý§à§Ó§à§Ü + payload + CRC
                
                if (expected_length > MAX_PACKET_SIZE) {
                    // §±§à§Õ§à§Ù§â§Ö§ß§Ú§Ö §ß§Ñ §Þ§å§ã§à§â ¡ª §ã§Ò§â§à§ã
                    rx_index = 0;
                    expected_length = 0;
                    return;
                }
            }

            // §¬§à§Ô§Õ§Ñ §ã§à§Ò§â§Ñ§Ý§Ú §Ó§Ö§ã§î §á§Ñ§Ü§Ö§ä
            if (expected_length > 0 && rx_index == expected_length) {
                

                uint16_t crc_recv = rx_buffer[expected_length - 2] | (rx_buffer[expected_length - 1] << 8);
                uint16_t crc_calc = crc16((uint8_t *)rx_buffer, expected_length - 2);

                if (crc_recv == crc_calc) {
                    received_packet.addr = rx_buffer[0];
                    received_packet.cmd  = rx_buffer[1];
                    received_packet.len  = rx_buffer[2];
                    // §¬§à§á§Ú§â§å§Ö§Þ payload
                    for (uint8_t i = 0; i < received_packet.len && i < sizeof(received_packet.data); i++) {
                        received_packet.data[i] = rx_buffer[3 + i];
                    }
                    received_packet.crc = crc_recv;

                    packet_ready = true;
                } else {
                    rx_crc_errors++; // §³§é§Ú§ä§Ñ§Ö§Þ §Ò§â§Ñ§Ü§à§Ó§Ñ§ß§ß§í§Û §á§Ñ§Ü§Ö§ä
                }

                // §³§Ò§â§à§ã§Ú§Þ §á§â§Ú§×§Þ
                rx_index = 0;
                expected_length = 0;
            }
        } else {
            // §±§Ö§â§Ö§á§à§Ý§ß§Ö§ß§Ú§Ö ¡ª §ã§Ò§â§à§ã
            rx_index = 0;
            expected_length = 0;
        }
    }
}

void RS485_Dir_Init(void) {
    RCC_APB2PeriphClockCmd(GPIO_TO_RCC_PORT(RS485_DIR_PORT), ENABLE);

    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = RS485_DIR_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RS485_DIR_PORT, &gpio);

    GPIO_ResetBits(RS485_DIR_PORT, RS485_DIR_PIN);  // §±§â§Ú§×§Þ §á§à §å§Þ§à§Ý§é§Ñ§ß§Ú§ð
}

void RS485_SetTX(void) {
    GPIO_SetBits(RS485_DIR_PORT, RS485_DIR_PIN);   // §±§Ö§â§Ö§Õ§Ñ§é§Ñ
}

void RS485_SetRX(void) {
    GPIO_ResetBits(RS485_DIR_PORT, RS485_DIR_PIN); // §±§â§Ú§×§Þ
}

void RS485_Send(uint8_t *data, uint8_t len)
{
    RS485_SetTX();
    DelayMs(1);

    for (uint8_t i = 0; i < len; i++) {
        while (!(USART1->STATR & USART_FLAG_TXE));
        USART_SendData(USART1, data[i]);
    }
    
    while (!(USART1->STATR & USART_FLAG_TC));
    RS485_SetRX();
}

void UART_SetInterByteTimeoutMs(uint16_t timeout_ms)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    SystemCoreClockUpdate();

    // 1 §ä§Ú§Ü = 1 §Þ§ã ¡ú §ä§Ñ§Û§Þ§Ö§â §ß§Ñ 1 §Ü§¤§è
    uint32_t timer_freq = 1000; // 1 §ä§Ú§Ü = 1 §Þ§ã
    uint16_t prescaler = (SystemCoreClock / timer_freq) - 1;

    // §°§Ô§â§Ñ§ß§Ú§é§Ú§Þ §Õ§à 65535 §Þ§ã = 65.5 §ã§Ö§Ü§å§ß§Õ (§Ý§Ú§Þ§Ú§ä 16 §Ò§Ú§ä)
    if (timeout_ms == 0) timeout_ms = 1;
    if (timeout_ms > 65535) timeout_ms = 65535;

    uint16_t period = timeout_ms - 1;

    TIM_TimeBaseInitTypeDef timerInitStructure = {
        .TIM_Prescaler = prescaler,
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = period,
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0
    };

    TIM_TimeBaseInit(TIM1, &timerInitStructure);
    TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM1, ENABLE);

    NVIC_EnableIRQ(TIM1_UP_IRQn);
}

void UART_SetAutoInterByteTimeout()
{
    // 1 §Ò§Ñ§Û§ä = 10 §Ò§Ú§ä ¡ú §Ó§â§Ö§Þ§ñ §Ó §Þ§ã = (10 / baudrate) * 1000 = 10000 / baudrate
    uint32_t byte_time_ms = (10000 + UART_BAUDRATE / 2) / UART_BAUDRATE; // §à§Ü§â§å§Ô§Ý§Ö§ß§Ú§Ö §Ó§Ó§Ö§â§ç
    uint32_t timeout_ms = byte_time_ms * 2;

    if (timeout_ms < 1)
        timeout_ms = 1;
    if (timeout_ms > 65535)
        timeout_ms = 65535;

    UART_SetInterByteTimeoutMs((uint16_t)timeout_ms);
}