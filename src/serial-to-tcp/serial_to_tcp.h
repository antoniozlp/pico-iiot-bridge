/**
 * @file serial_to_tcp.h
 * @brief Serial-to-TCP Bridge Task
 * 
 * This module implements a transparent bidirectional bridge between a UART
 * serial port and TCP network connection. It supports both TCP server mode
 * (listening for incoming connections) and TCP client mode (connecting to
 * a remote server).
 */

#ifndef SERIAL_TO_TCP_H
#define SERIAL_TO_TCP_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Create and start the Serial-to-TCP bridge task
 * 
 * This function creates a FreeRTOS task that manages the serial-to-TCP bridge.
 * The task reads configuration from the system config and operates according
 * to the configured mode (server or client).
 * 
 * The task will:
 * - Initialize the configured UART port
 * - Establish TCP connection (server listens, client connects)
 * - Bridge data bidirectionally between UART and TCP
 * - Handle reconnections and errors automatically
 */
void vCreateSerialToTcpTask(void);

#endif /* SERIAL_TO_TCP_H */
