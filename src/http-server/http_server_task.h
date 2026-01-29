/**
 * @file http_server_task.h
 * @brief HTTP Server FreeRTOS Task
 * 
 * This module provides a FreeRTOS task that runs the HTTP web server
 * for configuration management. It waits for network to be ready before
 * starting and handles network status changes.
 */

#ifndef _HTTP_SERVER_TASK_H_
#define _HTTP_SERVER_TASK_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize HTTP server
 * 
 * This function initializes the HTTP server with buffers and socket configuration.
 * Must be called before creating the HTTP server task.
 * 
 * @return true if initialization successful, false otherwise
 */
bool http_server_task_init(void);

/**
 * @brief Create and start the HTTP server FreeRTOS task
 * 
 * This function creates the HTTP server task which:
 * - Waits for network to be ready (link up and IP assigned)
 * - Runs the HTTP server for configuration management
 * - Handles network status changes automatically
 * 
 * @return true if task created successfully, false otherwise
 */
bool http_server_task_create(void);

#endif // _HTTP_SERVER_TASK_H_
