// === §±§Ú§ß §Õ§Ñ§ä§é§Ú§Ü§Ñ §Ó§Ý§Ñ§Ø§ß§à§ã§ä§Ú ===
#include "ch32v00x_gpio.h"

#define MOISTURE_SENSOR_PORT GPIOC
#define MOISTURE_SENSOR_PIN  GPIO_Pin_4
#define MOISTURE_SENSOR_ADC_CH ADC_Channel_2

void moisture_sensor_init(void) {
    // §£§Ü§Ý§ð§é§Ú§ä§î §ä§Ñ§Ü§ä§Ú§â§à§Ó§Ñ§ß§Ú§Ö §á§à§â§ä§Ñ §Ú §¡§¸§±
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_ADC1, ENABLE);

    GPIO_InitTypeDef gpio;
    ADC_InitTypeDef adc;

    // §¯§Ñ§ã§ä§â§à§Û§Ü§Ñ §á§Ú§ß§Ñ PC4 §Ü§Ñ§Ü §Ñ§ß§Ñ§Ý§à§Ô§à§Ó§í§Û §Ó§ç§à§Õ
    gpio.GPIO_Pin = MOISTURE_SENSOR_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(MOISTURE_SENSOR_PORT, &gpio);

    // §¯§Ñ§ã§ä§â§à§Û§Ü§Ñ ADC
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc);

    // §¯§Ñ§ã§ä§â§à§Û§Ü§Ñ §Ú§ã§á§à§Ý§î§Ù§å§Ö§Þ§à§Ô§à §Ü§Ñ§ß§Ñ§Ý§Ñ
    ADC_RegularChannelConfig(ADC1, MOISTURE_SENSOR_ADC_CH, 1, ADC_SampleTime_241Cycles);

    // §£§Ü§Ý§ð§é§Ö§ß§Ú§Ö §¡§¸§± §Ú §Ü§Ñ§Ý§Ú§Ò§â§à§Ó§Ü§Ñ
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