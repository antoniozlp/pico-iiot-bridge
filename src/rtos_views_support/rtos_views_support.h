/**
 * @file rtos_views_support.h
 * @brief FreeRTOS Runtime Statistics Support for VS Code RTOS Views
 * 
 * This module provides runtime statistics functionality for FreeRTOS when
 * RTOS_VIEWS_SUPPORT is enabled. This allows VS Code's RTOS Views to display
 * detailed task runtime information including CPU usage percentages.
 * 
 * Enable by adding in CMakeLists.txt:
 *   add_definitions(-DRTOS_VIEWS_SUPPORT)
 */

#ifndef _RTOS_VIEWS_SUPPORT_H_
#define _RTOS_VIEWS_SUPPORT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (configGENERATE_RUN_TIME_STATS == 1)

/**
 * @brief Initialize the runtime statistics timer
 * 
 * This function is called by FreeRTOS to initialize the timer used for
 * gathering runtime statistics. We use the Pico's hardware timer which
 * provides a 64-bit microsecond counter.
 * 
 * @note Only available when RTOS_VIEWS_SUPPORT is defined
 */
void vConfigureTimerForRunTimeStats(void);

/**
 * @brief Get the current runtime counter value
 * 
 * Returns the current value of the runtime statistics timer. FreeRTOS uses
 * this to calculate how much CPU time each task has consumed.
 * 
 * @return Current timer value in microseconds divided by 10 (for better resolution)
 * 
 * @note Only available when RTOS_VIEWS_SUPPORT is defined
 */
uint32_t ulGetRunTimeCounterValue(void);

#endif /* configGENERATE_RUN_TIME_STATS */

#ifdef __cplusplus
}
#endif

#endif // _RTOS_VIEWS_SUPPORT_H_
