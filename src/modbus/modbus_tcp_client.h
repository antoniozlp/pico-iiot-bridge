/**
 * @file modbus_tcp_client.h
 * @brief Modbus TCP client task
 *
 * Provides a FreeRTOS task that runs a Modbus TCP client using the nanoMODBUS
 * library. The client connects to a remote Modbus TCP server, polls all
 * configured request slots each cycle, and maps results to the Tag Database.
 * Configuration is taken from system configuration.
 */

#ifndef _MODBUS_TCP_CLIENT_H_
#define _MODBUS_TCP_CLIENT_H_

#include <stdbool.h>

/**
 * @brief Initialize and start the Modbus TCP client task
 *
 * Reads Modbus TCP client config from system config. If the client is disabled
 * (enable == 0), returns true without creating a task. If enabled, creates the
 * Modbus TCP client FreeRTOS task; the task then waits for network readiness,
 * connects to the remote server, and starts polling.
 * Must be called after FreeRTOS scheduler is started (or before
 * vTaskStartScheduler) and after config_load_from_flash().
 *
 * @return true if initialization successful or feature disabled, false on error
 */
bool modbus_tcp_client_task_init(void);

#endif /* _MODBUS_TCP_CLIENT_H_ */
