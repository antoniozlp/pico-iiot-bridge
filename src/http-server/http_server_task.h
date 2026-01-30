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
 * @brief Initialize and create the HTTP server task
 * 
 * This function initializes the HTTP server with buffers and socket configuration,
 * then creates the HTTP server FreeRTOS task.
 * 
 * The task will:
 * - Wait for network to be ready (link up and IP assigned)
 * - Run the HTTP server for configuration management on port 80
 * - Handle network status changes automatically
 * 
 * @return true if initialization and task creation successful, false otherwise
 */
bool http_server_task_init(void);

#endif // _HTTP_SERVER_TASK_H_
