/**
 * @file modbus_tcp_server.h
 * @brief Modbus TCP server task
 *
 * Provides a FreeRTOS task that runs a Modbus TCP server using the nanoMODBUS
 * library. The server listens for incoming TCP connections on a configurable
 * port and serves a memory map of memory blocks backed by the Tag Database.
 * Configuration is taken from system configuration.
 */

#ifndef _MODBUS_TCP_SERVER_H_
#define _MODBUS_TCP_SERVER_H_

#include <stdbool.h>

/**
 * @brief Initialize and start the Modbus TCP server task
 *
 * Reads Modbus TCP server config from system config. If the server is disabled
 * (enable == 0), returns true without creating a task. If enabled, creates the
 * Modbus TCP server FreeRTOS task; the task then waits for network readiness,
 * opens a WizNet socket, listens for connections, and serves Modbus requests.
 * Must be called after FreeRTOS scheduler is started (or before
 * vTaskStartScheduler) and after config_load_from_flash().
 *
 * @return true if initialization successful or feature disabled, false on error
 */
bool modbus_tcp_server_task_init(void);

#endif /* _MODBUS_TCP_SERVER_H_ */
