// === Пин датчика влажности ===
#include "ch32v00x_gpio.h"

#define MOISTURE_SENSOR_PORT GPIOC
#define MOISTURE_SENSOR_PIN  GPIO_Pin_4
#define MOISTURE_SENSOR_ADC_CH ADC_Channel_2

void moisture_sensor_init(void) {
    // Включить тактирование порта и АЦП
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_ADC1, ENABLE);

    GPIO_InitTypeDef gpio;
    ADC_InitTypeDef adc;

    // Настройка пина PC4 как аналоговый вход
    gpio.GPIO_Pin = MOISTURE_SENSOR_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(MOISTURE_SENSOR_PORT, &gpio);

    // Настройка ADC
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc);

    // Настройка используемого канала
    ADC_RegularChannelConfig(ADC1, MOISTURE_SENSOR_ADC_CH, 1, ADC_SampleTime_241Cycles);

    // Включение АЦП и калибровка
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
}

uint16_t get_moisture() {
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);
}