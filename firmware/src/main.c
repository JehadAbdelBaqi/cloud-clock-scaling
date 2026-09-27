#include "stm32f411_registers.h"

int main(void) {
    RCC_AHB1ENR |= GPIOAEN;                          // enable GPIOA clock
    GPIOA_MODER |= (1 << (PIN5 * 2));                // MODER5 = 01 (general purpose output)
    STK_LOAD = STK_LOAD_HEX_VALUE;                    // set reload value for SysTick
    STK_CTRL = STK_CTRL_ENABLE | STK_CTRL_CLKSOURCE;  // enable SysTick, clocked by AHB (16MHz)

    while (1) {
        if (STK_CTRL & STK_CTRL_COUNTFLAG) {          // check if SysTick has counted to 0
            GPIOA_ODR ^= (1 << PIN5);                 // toggle PA5 (LD2)
        }
    }
}
