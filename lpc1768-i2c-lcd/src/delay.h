/******************************************************************************
 * @file    delay.h
 * @brief   Delay Functions using SysTick Timer (Bare Metal)
 * @details Provides blocking microsecond and millisecond delays using the
 *          ARM Cortex-M3 SysTick timer for accurate timing.
 *          Does NOT use SysTick interrupt — purely polling-based.
 *
 * @author  Bare Metal I2C LCD Project
 * @date    2026
 ******************************************************************************/

#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

/**
 * @brief  Initialize the delay system.
 * @note   Call this once at startup before using any delay functions.
 *         Configures SysTick for 1 MHz counting (1 µs resolution).
 */
void Delay_Init(void);

/**
 * @brief  Blocking delay in microseconds.
 * @param  us  Number of microseconds to delay.
 * @note   Maximum single delay: limited by SysTick 24-bit counter.
 *         For delays > 16 ms at 100 MHz, the function uses multiple
 *         SysTick cycles internally.
 */
void Delay_us(uint32_t us);

/**
 * @brief  Blocking delay in milliseconds.
 * @param  ms  Number of milliseconds to delay.
 */
void Delay_ms(uint32_t ms);

#endif /* DELAY_H */
