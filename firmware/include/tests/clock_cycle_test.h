#ifndef TESTS_CLOCK_CYCLE_TEST_H
#define TESTS_CLOCK_CYCLE_TEST_H

// Clock-cycle test: flips 16 <-> 50 MHz every time SysTick counts down to 0.
// Never returns. Assumes SysTick + LD2 are already set up by main().
void clock_cycle_test_run(void);

#endif
