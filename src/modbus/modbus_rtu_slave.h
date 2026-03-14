/**
 * @file modbus_rtu_slave.h
 * @brief Modbus RTU slave (server) task
 *
 * Provides a FreeRTOS task that runs a Modbus RTU slave using the nanoMODBUS
 * library. The slave serves a configurable memory map of memory blocks backed by
 * the Tag Database. UART and memory block configuration are taken from system
 * configuration.
 */

#ifndef _MODBUS_RTU_SLAVE_H_
#define _MODBUS_RTU_SLAVE_H_

#include <stdbool.h>

/**
 * @brief Initialize and start the Modbus RTU slave task
 *
 * Reads Modbus RTU server config from system config. If the server is disabled
 * (enable == 0), returns true without creating a task. If enabled, creates the
 * Modbus RTU slave FreeRTOS task; the task then initializes the selected UART,
 * creates the nanoMODBUS server instance, and enters the poll loop.
 * Must be called after FreeRTOS scheduler is started (or before
 * vTaskStartScheduler) and after config_load_from_flash().
 *
 * @return true if initialization successful or feature disabled, false on error
 */
bool modbus_rtu_slave_task_init(void);

#endif /* _MODBUS_RTU_SLAVE_H_ */
