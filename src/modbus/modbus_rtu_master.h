/**
 * @file modbus_rtu_master.h
 * @brief Modbus RTU master (client) task
 *
 * Provides a FreeRTOS task that runs a Modbus RTU master using the nanoMODBUS
 * library. UART and request configurations are taken from system configuration.
 */

#ifndef _MODBUS_RTU_MASTER_H_
#define _MODBUS_RTU_MASTER_H_

#include <stdbool.h>

/**
 * @brief Initialize and start the Modbus RTU master task
 *
 * Reads Modbus RTU client config from system config. If Modbus RTU client is
 * disabled (enable == 0), returns true without creating a task. If enabled,
 * creates the Modbus RTU master FreeRTOS task; the task then loads config,
 * initializes the selected UART, and runs the polling loop.
 * Must be called after FreeRTOS scheduler is started (or before
 * vTaskStartScheduler) and after config_load_from_flash().
 *
 * @return true if initialization successful or feature disabled, false on error
 */
bool modbus_rtu_master_task_init(void);

#endif /* _MODBUS_RTU_MASTER_H_ */
