/**
 * @file network_task.h
 * @brief Network management task for Wiznet Ethernet chip
 * 
 * This module provides a FreeRTOS task that manages:
 * - Wiznet chip initialization
 * - DHCP client (if enabled in configuration)
 * - Static IP configuration
 * - PHY link monitoring and reconnection
 */

#ifndef _NETWORK_TASK_H_
#define _NETWORK_TASK_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize and start the network management task
 * 
 * This function:
 * - Initializes the Wiznet chip (SPI, CRIS, reset, check)
 * - Creates the network management FreeRTOS task
 * - Initializes the 1ms timer for DHCP
 * 
 * @return true if initialization successful, false otherwise
 */
bool network_task_init(void);

/**
 * @brief Get current network status
 * 
 * @param ip_address Output buffer for IP address (4 bytes)
 * @param link_status Output pointer for link status (true = up, false = down)
 * @return true if status retrieved successfully, false otherwise
 */
bool network_task_get_status(uint8_t ip_address[4], bool *link_status);

#endif // _NETWORK_TASK_H_
