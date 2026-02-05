/**
 * @file modbus_rtu.h
 * @brief Modbus RTU client task (nanoMODBUS library)
 *
 * This module provides a FreeRTOS task that runs a Modbus RTU client using
 * the nanoMODBUS library. UART and destination address are taken from
 * system configuration. This is a test/demo implementation for evaluating
 * the library.
 */

#ifndef _MODBUS_RTU_H_
#define _MODBUS_RTU_H_

#include <stdbool.h>

/**
 * @brief Initialize and start the Modbus RTU client task
 *
 * Reads Modbus RTU client config from system config. If Modbus RTU is
 * disabled (enable == 0), returns true without creating a task. If enabled,
 * creates the Modbus RTU FreeRTOS task; the task then loads config,
 * initializes the selected UART, and runs the client loop.
 * Must be called after FreeRTOS scheduler is started (or before
 * vTaskStartScheduler) and after config_load_from_flash().
 *
 * @return true if initialization successful or feature disabled, false on error
 */
bool modbus_rtu_task_init(void);

#endif /* _MODBUS_RTU_H_ */
