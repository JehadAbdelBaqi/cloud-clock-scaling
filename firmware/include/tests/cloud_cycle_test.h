#ifndef TESTS_CLOUD_CYCLE_TEST_H
#define TESTS_CLOUD_CYCLE_TEST_H

// Cloud-cycle test: mic loudness -> readings "S,<seq>,<peak>" to the Wi-Fi
// bridge (USART1) — the loudest window every SysTick countdown, and straight
// away on a clap. Commands "C,<seq>,<level>" coming back switch the clock
// up; after HOLD_S seconds the board drops back to 16 MHz itself.
// Each switch is acknowledged with "A,<seq>,<mhz>".
// Never returns. Assumes SysTick + LD2 are already set up by main().
void cloud_cycle_test_run(void);

#endif
