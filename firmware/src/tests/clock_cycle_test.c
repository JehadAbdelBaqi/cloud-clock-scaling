#include "tests/clock_cycle_test.h"

#include "config/app_config.h"
#include "drivers/clock.h"
#include "regs/systick.h"
#include "tests/test_led.h"
#include "tests/test_uart_log.h"

// Current SYSCLK in MHz — add to the debugger Watch panel to see each switch.
// volatile: the compiler must really write it every time, never optimise it out.
static volatile uint32_t clock_speed_mhz = 16;

void clock_cycle_test_run(void) {
    clock_speed_t speed = BOOT_CLOCK;                 // main() already put us here at boot
    test_uart_log_init();

    while (1) {
        test_led_loop();

        if (STK_CTRL & STK_CTRL_COUNTFLAG) {          // SysTick counted down to 0
            test_led_on_switch();
            speed = (speed == CLOCK_16MHZ) ? CLOCK_50MHZ : CLOCK_16MHZ;

            test_uart_log_before_switch();
            clock_set(speed);
            test_uart_log_after_switch(speed);

            clock_speed_mhz = (speed == CLOCK_50MHZ) ? 50 : 16;
        }
    }
}
