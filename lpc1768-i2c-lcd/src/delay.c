/******************************************************************************
 * @file    delay.c
 * @brief   SysTick-Based Delay Implementation for LPC1768
 * @details Uses the ARM Cortex-M3 SysTick timer in polling mode to provide
 *          accurate microsecond and millisecond delays.
 *
 *          SysTick is a 24-bit down counter that counts from a reload value
 *          down to 0, then sets the COUNTFLAG and reloads.
 *
 * SysTick Registers (Core Peripherals, base: 0xE000E010):
 *   CTRL  (0x00) - Control and Status
 *                   Bit 0: ENABLE
 *                   Bit 1: TICKINT (not used in polling mode)
 *                   Bit 2: CLKSOURCE (1=processor clock, 0=external)
 *                   Bit 16: COUNTFLAG (set when counter reaches 0)
 *   LOAD  (0x04) - Reload Value (24-bit)
 *   VAL   (0x08) - Current Value (write any value to clear)
 *
 * @author  Bare Metal I2C LCD Project
 * @date    2026
 ******************************************************************************/

#include "LPC17xx.h"
#include "delay.h"

/*---------------------------------------------------------------------------
 * Module-level variables
 *---------------------------------------------------------------------------*/
static uint32_t us_ticks;   /* Number of SysTick ticks per microsecond */

/*---------------------------------------------------------------------------
 * Delay_Init — Compute ticks per microsecond based on SystemCoreClock
 *---------------------------------------------------------------------------*/
void Delay_Init(void)
{
    /*
     * SystemCoreClock is typically 100 MHz for LPC1768 (set by SystemInit).
     * Ticks per microsecond = SystemCoreClock / 1,000,000
     * At 100 MHz: us_ticks = 100
     */
    us_ticks = SystemCoreClock / 1000000U;
}

/*---------------------------------------------------------------------------
 * Delay_us — Blocking microsecond delay using SysTick polling
 *
 * SysTick is a 24-bit counter, so the maximum reload value is 0xFFFFFF
 * (16,777,215). At 100 MHz that's ~167 ms maximum per cycle.
 *
 * For very large delays, we break them into chunks.
 *---------------------------------------------------------------------------*/
void Delay_us(uint32_t us)
{
    uint32_t ticks;
    uint32_t chunk;

    while (us > 0)
    {
        /*
         * Calculate ticks needed for this chunk.
         * Max ticks per SysTick cycle: 0xFFFFFF (24-bit limit)
         * Max microseconds per chunk: 0xFFFFFF / us_ticks
         */
        if (us > (0xFFFFFF / us_ticks))
        {
            chunk = 0xFFFFFF / us_ticks;
        }
        else
        {
            chunk = us;
        }

        ticks = chunk * us_ticks;

        /* Configure SysTick */
        SysTick->LOAD = ticks - 1;        /* Set reload value               */
        SysTick->VAL  = 0;                /* Clear current value (& flag)   */
        SysTick->CTRL = SysTick_CTRL_ENABLE_Msk     /* Enable SysTick       */
                      | SysTick_CTRL_CLKSOURCE_Msk;  /* Use processor clock  */

        /* Wait for COUNTFLAG to be set (counter reached 0) */
        while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk))
        {
            /* Polling — no interrupt used */
        }

        /* Disable SysTick after delay */
        SysTick->CTRL = 0;

        us -= chunk;
    }
}

/*---------------------------------------------------------------------------
 * Delay_ms — Blocking millisecond delay
 *---------------------------------------------------------------------------*/
void Delay_ms(uint32_t ms)
{
    while (ms > 0)
    {
        Delay_us(1000);  /* 1 ms = 1000 µs */
        ms--;
    }
}
