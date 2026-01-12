/**
 * @file rtos_views_support.c
 * @brief FreeRTOS Runtime Statistics Support Implementation
 * 
 * This module provides runtime statistics functionality for FreeRTOS when
 * RTOS_VIEWS_SUPPORT is enabled. This allows VS Code's RTOS Views to display
 * detailed task runtime information including CPU usage percentages.
 */

#include "rtos_views_support.h"
#include "FreeRTOS.h"
#include "pico/time.h"

#if (configGENERATE_RUN_TIME_STATS == 1)

/**
 * @brief Initialize the runtime statistics timer
 * 
 * This function is called by FreeRTOS to initialize the timer used for
 * gathering runtime statistics. We use the Pico's hardware timer which
 * provides a 64-bit microsecond counter.
 * 
 * The Pico SDK's time_us_64() function uses a hardware timer that is
 * already running, so no initialization is needed.
 */
void vConfigureTimerForRunTimeStats(void)
{
    // The Pico SDK's time_us_64() is already running from boot
    // No initialization needed
}

/**
 * @brief Get the current runtime counter value
 * 
 * Returns the current value of the runtime statistics timer. FreeRTOS uses
 * this to calculate how much CPU time each task has consumed.
 * 
 * The timer resolution should be at least 10x faster than the tick rate
 * for accurate statistics. With configTICK_RATE_HZ=1000 (1ms tick), we
 * return time in 10µs units (100x faster than tick rate).
 * 
 * @return Current timer value in microseconds divided by 10
 */
uint32_t ulGetRunTimeCounterValue(void)
{
    // Return time in units of 10 microseconds
    // configTICK_RATE_HZ is 1000, so tick is 1ms
    // This gives us 100x resolution compared to tick rate
    return (uint32_t)(time_us_64() / 10);
}

#endif /* configGENERATE_RUN_TIME_STATS */
