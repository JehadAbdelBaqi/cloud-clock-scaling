#include <stdint.h>

#include "drivers/mic.h"
#include "config/app_config.h"
#include "regs/adc.h"
#include "regs/gpio.h"
#include "regs/rcc.h"

void mic_init(void) {
    // 1. Clocks: GPIOA (PA0) and ADC1
    RCC_AHB1ENR |= GPIOAEN;
    RCC_APB2ENR |= ADC1EN;

    // 2. Pin: PA0 mode = 11 (analog) — disconnects the digital input from the pin
    GPIOA_MODER |= (0x3 << (PIN0 * 2));

    // 3. ADC clock = PCLK2 / 4 -> 4 MHz at 16 MHz, 12.5 MHz at 50 MHz
    ADC_CCR = (ADC_CCR & ~ADC_CCR_ADCPRE_MASK) | (0x1 << ADC_CCR_ADCPRE_POS);

    // 4. One conversion per start (SQR1 L = 0), of channel 0 (SQR3 SQ1 = 0).
    //    Reset values already give 12-bit, right-aligned, single mode.
    ADC1_SQR1 = 0;
    ADC1_SQR3 = 0;

    // 5. Channel 0 sample time = 84 ADC cycles (SMP0 = 100) — longer = steadier reading
    ADC1_SMPR2 = (ADC1_SMPR2 & ~0x7) | 0x4;

    // 6. ADC on
    ADC1_CR2 |= ADC_CR2_ADON;
}

// One 12-bit reading, 0–4095 (0 V – 3.3 V).
static uint32_t mic_read(void) {
    ADC1_CR2 |= ADC_CR2_SWSTART;
    while (!(ADC1_SR & ADC_SR_EOC)) {}  // a few µs
    return ADC1_DR & 0xFFF;             // reading DR also clears EOC
}

int mic_poll(uint32_t *p2p) {
    static uint32_t count = 0, lo = 0xFFF, hi = 0;

    uint32_t v = mic_read();
    if (v < lo) lo = v;
    if (v > hi) hi = v;

    if (++count < MIC_WINDOW_SAMPLES) {
        return 0;
    }
    *p2p = hi - lo;
    count = 0;
    lo = 0xFFF;
    hi = 0;
    return 1;
}
