#ifndef TESTS_TEST_LED_H
#define TESTS_TEST_LED_H

// LED side of the local tests. Mode picked by TEST_SW_LED in app_config.h.

// Call on every pass of the test loop. TEST_SW_LED = 1: software counter toggles the LED.
void test_led_loop(void);

// Call on every clock switch. TEST_SW_LED = 0: toggle the LED with the switch.
void test_led_on_switch(void);

#endif
