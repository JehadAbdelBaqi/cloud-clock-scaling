#ifndef REGS_ADC_H
#define REGS_ADC_H

#include <stdint.h>

// ADC1 — base 0x40012000 (RM0383 memory map §2.3), register map RM0383 §11.12.16
#define ADC1_SR    (*(volatile uint32_t *)0x40012000)  // status, offset 0x00
#define ADC1_CR1   (*(volatile uint32_t *)0x40012004)  // control 1, offset 0x04
#define ADC1_CR2   (*(volatile uint32_t *)0x40012008)  // control 2, offset 0x08
#define ADC1_SMPR2 (*(volatile uint32_t *)0x40012010)  // sample time, channels 0–9, offset 0x10
#define ADC1_SQR1  (*(volatile uint32_t *)0x4001202C)  // sequence length, offset 0x2C
#define ADC1_SQR3  (*(volatile uint32_t *)0x40012034)  // sequence slots 1–6, offset 0x34
#define ADC1_DR    (*(volatile uint32_t *)0x4001204C)  // result, offset 0x4C
#define ADC_CCR    (*(volatile uint32_t *)0x40012304)  // common control (prescaler), offset 0x304

#define ADC_SR_EOC       (1 << 1)    // end of conversion — result ready in DR
#define ADC_CR2_ADON     (1 << 0)    // ADC on
#define ADC_CR2_SWSTART  (1 << 30)   // start a conversion
#define ADC_CCR_ADCPRE_POS  16       // ADCPRE[17:16]: 00 = PCLK2/2, 01 = /4, 10 = /6, 11 = /8
#define ADC_CCR_ADCPRE_MASK (0x3 << ADC_CCR_ADCPRE_POS)

#endif
