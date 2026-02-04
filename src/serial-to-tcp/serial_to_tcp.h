/**
 * @file serial_to_tcp.h
 * @brief Serial-to-TCP Bridge Task
 * 
 * This module implements a transparent bidirectional bridge between a UART
 * serial port and TCP network connection. It supports both TCP server mode
 * (listening for incoming connections) and TCP client mode (connecting to
 * a remote server).
 */

#ifndef _SERIAL_TO_TCP_H_
#define _SERIAL_TO_TCP_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize and create the Serial-to-TCP bridge task
 * 
 * This function creates a FreeRTOS task that manages the serial-to-TCP bridge.
 * The task reads configuration from the system config once at startup and 
 * operates according to the configured mode (server or client).
 * 
 * The task will:
 * - Load configuration at startup (changes require system reboot)
 * - Exit immediately if mode is disabled in configuration
 * - Initialize the configured UART port
 * - Establish TCP connection (server listens, client connects)
 * - Bridge data bidirectionally between UART and TCP
 * - Handle reconnections and errors automatically
 * - Respond to network status changes (link down, IP changes)
 * 
 * @return true if task created successfully, false otherwise
 * 
 * @note Configuration is read once at task startup. To apply configuration
 *       changes, the system must be rebooted.
 */
bool serial_to_tcp_task_init(void);

#endif // _SERIAL_TO_TCP_H_
