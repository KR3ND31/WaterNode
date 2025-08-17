#include "uart.h"
#include "debug.h"
#include "timer.h"

#define RS485_DIR_PORT GPIOC
#define RS485_DIR_PIN  GPIO_Pin_0

volatile uint32_t uart_last_rx_ms = 0; // время последней активности на RX
volatile uint32_t idle_guard_ms   = 2; // пауза тишины перед отправкой UID

volatile uint8_t rx_buffer[MAX_PACKET_SIZE];
volatile uint8_t rx_index = 0;
volatile uint8_t expected_length = 0;

volatile uint16_t rx_crc_errors = 0;

#define GPIO_TO_RCC_PORT(x) \
    ((x) == GPIOA ? RCC_APB2Periph_GPIOA : \
     (x) == GPIOC ? RCC_APB2Periph_GPIOC : \
     (x) == GPIOD ? RCC_APB2Periph_GPIOD : 0)

static void NVIC_Config_USART1_TIM1(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    // USART1 — ВЫСОКИЙ приоритет (0)
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // TIM1_UP — НИЖЕ (1)
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void USARTx_CFG(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

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

    // Сначала настраиваем приоритеты
    NVIC_Config_USART1_TIM1();

    // Разрешаем RXNE прерывание и включаем USART
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

// === TIM1 обработчик — сброс по таймауту ===
void TIM1_UP_IRQHandler(void) __attribute__((interrupt));
void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update)) {
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);

        // Таймаут между байтами — сброс приёма
        rx_index = 0;
        expected_length = 0;

        // Останавливаем таймер — ждём новый первый байт
        TIM_Cmd(TIM1, DISABLE);
    }
}

// === Обработчик прерывания USART1 ===
void USART1_IRQHandler(void) __attribute__((interrupt));
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE)) {
        uart_last_rx_ms = now_ms();

        uint8_t byte = USART_ReceiveData(USART1);

        // Если это первый байт нового пакета — запускаем таймер
        if (rx_index == 0) {
            TIM_SetCounter(TIM1, 0);
            TIM_Cmd(TIM1, ENABLE);
        } else {
            // Иначе просто перезапускаем отсчёт
            TIM_SetCounter(TIM1, 0);
        }

        if (rx_index < MAX_PACKET_SIZE) {
            rx_buffer[rx_index++] = byte;

            if (rx_index == 3) {
                // Только что получили LEN
                expected_length = 3 + rx_buffer[2] + 2;  // заголовок + payload + CRC
                
                if (expected_length > MAX_PACKET_SIZE) {
                    // Подозрение на мусор — сброс
                    rx_index = 0;
                    expected_length = 0;
                    TIM_Cmd(TIM1, DISABLE);
                    return;
                }
            }

            // Когда собрали весь пакет
            if (expected_length > 0 && rx_index == expected_length) {
                

                uint16_t crc_recv = rx_buffer[expected_length - 2] | (rx_buffer[expected_length - 1] << 8);
                uint16_t crc_calc = crc16((uint8_t *)rx_buffer, expected_length - 2);

                if (crc_recv == crc_calc) {
                    received_packet.addr = rx_buffer[0];
                    received_packet.cmd  = rx_buffer[1];
                    received_packet.len  = rx_buffer[2];
                    // Копируем payload
                    for (uint8_t i = 0; i < received_packet.len && i < sizeof(received_packet.data); i++) {
                        received_packet.data[i] = rx_buffer[3 + i];
                    }
                    received_packet.crc = crc_recv;

                    packet_ready = true;
                } else {
                    rx_crc_errors++; // Считаем бракованный пакет
                }

                // Сбросим приём
                rx_index = 0;
                expected_length = 0;
                TIM_Cmd(TIM1, DISABLE);
            }
        } else {
            // Переполнение — сброс
            rx_index = 0;
            expected_length = 0;
            TIM_Cmd(TIM1, DISABLE);
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

    GPIO_ResetBits(RS485_DIR_PORT, RS485_DIR_PIN);  // Приём по умолчанию
}

void RS485_SetTX(void) {
    GPIO_SetBits(RS485_DIR_PORT, RS485_DIR_PIN);   // Передача
}

void RS485_SetRX(void) {
    GPIO_ResetBits(RS485_DIR_PORT, RS485_DIR_PIN); // Приём
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

    // 1 тик = 1 мс → таймер на 1 кГц
    uint32_t timer_freq = 1000; // 1 тик = 1 мс
    uint16_t prescaler = (SystemCoreClock / timer_freq) - 1;

    // Ограничим до 65535 мс = 65.5 секунд (лимит 16 бит)
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

    TIM_Cmd(TIM1, DISABLE); // выключен, запустим на первом байте

    NVIC_EnableIRQ(TIM1_UP_IRQn);
}

void UART_SetAutoInterByteTimeout()
{
    // 1 байт = 10 бит → время в мс = (10 / baudrate) * 1000 = 10000 / baudrate
    uint32_t byte_time_ms = (10000 + UART_BAUDRATE / 2) / UART_BAUDRATE; // округление вверх
    uint32_t timeout_ms = byte_time_ms * 2;

    if (timeout_ms < 1)
        timeout_ms = 1;
    if (timeout_ms > 65535)
        timeout_ms = 65535;

    UART_SetInterByteTimeoutMs((uint16_t)timeout_ms);
}

static inline bool usart_rxne(void){
    return USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET;
}

bool uart_rx_busy(void) {
    // занято, если прямо сейчас есть байт в RDR или уже начат пакет
    return usart_rxne() || (rx_index > 0);
}

static inline bool uart_idle_window_ok(void) {
    // прошло ли минимум idle_guard_ms со времени последнего приёма
    return elapsed_ge(uart_last_rx_ms, idle_guard_ms);
}

bool uart_line_idle(void) {
    // линия считается свободной, если прошло окно тишины и в RX нет активности
    return uart_idle_window_ok() && !uart_rx_busy();
}