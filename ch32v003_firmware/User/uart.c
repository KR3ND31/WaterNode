#include "uart.h"
#include "debug.h"
#include "timer.h"

#define RS485_DIR_PORT GPIOC
#define RS485_DIR_PIN  GPIO_Pin_0

volatile uint32_t uart_last_rx_ms = 0; // �ӧ�֧ާ� ����ݧ֧էߧ֧� �ѧܧ�ڧӧߧ���� �ߧ� RX
volatile uint32_t idle_guard_ms   = 2; // ��ѧ�٧� ��ڧ�ڧߧ� ��֧�֧� �����ѧӧܧ�� UID

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

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);     // ���ܧݧ��ѧ֧� ���֧��ӧѧߧڧ� ��� ���ڧקާ�
    
    // ���ѧ٧�֧�ѧ֧� ���֧��ӧѧߧڧ� �� NVIC
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    USART_Cmd(USART1, ENABLE);
}

// === TIM1 ��ҧ�ѧҧ���ڧ� �� ��ҧ��� ��� ��ѧۧާѧ��� ===
void TIM1_UP_IRQHandler(void) __attribute__((interrupt));
void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update)) {
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);

        // ���ѧۧާѧ�� �ާ֧اէ� �ҧѧۧ�ѧާ� �� ��ҧ��� ���ڧקާ�
        rx_index = 0;
        expected_length = 0;
    }
}

// === ���ҧ�ѧҧ���ڧ� ���֧��ӧѧߧڧ� USART1 ===
void USART1_IRQHandler(void) __attribute__((interrupt));
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE)) {
        uart_last_rx_ms = now_ms();

        uint8_t byte = USART_ReceiveData(USART1);

        // ���ҧ��� ��ѧۧާ֧�� �� �ܧѧاէ�� �ҧѧۧ� �٧ѧߧ�ӧ� �٧ѧ���ܧѧ֧� ��ѧۧާѧ��
        TIM_SetCounter(TIM1, 0);

        if (rx_index < MAX_PACKET_SIZE) {
            rx_buffer[rx_index++] = byte;

            if (rx_index == 3) {
                // ����ݧ�ܧ� ���� ���ݧ��ڧݧ� LEN
                expected_length = 3 + rx_buffer[2] + 2;  // �٧ѧԧ�ݧ�ӧ�� + payload + CRC
                
                if (expected_length > MAX_PACKET_SIZE) {
                    // ����է�٧�֧ߧڧ� �ߧ� �ާ���� �� ��ҧ���
                    rx_index = 0;
                    expected_length = 0;
                    return;
                }
            }

            // ����ԧէ� ���ҧ�ѧݧ� �ӧ֧�� ��ѧܧ֧�
            if (expected_length > 0 && rx_index == expected_length) {
                

                uint16_t crc_recv = rx_buffer[expected_length - 2] | (rx_buffer[expected_length - 1] << 8);
                uint16_t crc_calc = crc16((uint8_t *)rx_buffer, expected_length - 2);

                if (crc_recv == crc_calc) {
                    received_packet.addr = rx_buffer[0];
                    received_packet.cmd  = rx_buffer[1];
                    received_packet.len  = rx_buffer[2];
                    // �����ڧ��֧� payload
                    for (uint8_t i = 0; i < received_packet.len && i < sizeof(received_packet.data); i++) {
                        received_packet.data[i] = rx_buffer[3 + i];
                    }
                    received_packet.crc = crc_recv;

                    packet_ready = true;
                } else {
                    rx_crc_errors++; // ����ڧ�ѧ֧� �ҧ�ѧܧ�ӧѧߧߧ�� ��ѧܧ֧�
                }

                // ���ҧ���ڧ� ���ڧק�
                rx_index = 0;
                expected_length = 0;
            }
        } else {
            // ���֧�֧��ݧߧ֧ߧڧ� �� ��ҧ���
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

    GPIO_ResetBits(RS485_DIR_PORT, RS485_DIR_PIN);  // ����ڧק� ��� ��ާ�ݧ�ѧߧڧ�
}

void RS485_SetTX(void) {
    GPIO_SetBits(RS485_DIR_PORT, RS485_DIR_PIN);   // ���֧�֧էѧ��
}

void RS485_SetRX(void) {
    GPIO_ResetBits(RS485_DIR_PORT, RS485_DIR_PIN); // ����ڧק�
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

    // 1 ��ڧ� = 1 �ާ� �� ��ѧۧާ֧� �ߧ� 1 �ܧ���
    uint32_t timer_freq = 1000; // 1 ��ڧ� = 1 �ާ�
    uint16_t prescaler = (SystemCoreClock / timer_freq) - 1;

    // ���ԧ�ѧߧڧ�ڧ� �է� 65535 �ާ� = 65.5 ��֧ܧ�ߧ� (�ݧڧާڧ� 16 �ҧڧ�)
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
    // 1 �ҧѧۧ� = 10 �ҧڧ� �� �ӧ�֧ާ� �� �ާ� = (10 / baudrate) * 1000 = 10000 / baudrate
    uint32_t byte_time_ms = (10000 + UART_BAUDRATE / 2) / UART_BAUDRATE; // ��ܧ��ԧݧ֧ߧڧ� �ӧӧ֧��
    uint32_t timeout_ms = byte_time_ms * 2;

    if (timeout_ms < 1)
        timeout_ms = 1;
    if (timeout_ms > 65535)
        timeout_ms = 65535;

    UART_SetInterByteTimeoutMs((uint16_t)timeout_ms);
}