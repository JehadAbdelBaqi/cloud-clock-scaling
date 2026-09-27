#ifndef MIC_H
#define MIC_H

#include <stdint.h>

// Analog mic on PA0 (A0) through ADC1, 12-bit (0–4095), polled.
// Loudness = peak-to-peak (max - min) over a window of MIC_WINDOW_SAMPLES samples.

void mic_init(void);

// Take one sample. Returns 1 when a window just finished and *p2p holds its
// peak-to-peak; else 0. Call as often as possible from the main loop.
int mic_poll(uint32_t *p2p);

#endif
