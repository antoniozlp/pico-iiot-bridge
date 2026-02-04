/**
 * @file network_task.h
 * @brief Network management task for Wiznet Ethernet chip
 * 
 * This module provides a FreeRTOS task that manages:
 * - Wiznet chip initialization
 * - DHCP client (if enabled in configuration)
 * - Static IP configuration
 * - PHY link monitoring and reconnection
 * - Event-driven status notifications to other tasks
 */

#ifndef _NETWORK_TASK_H_
#define _NETWORK_TASK_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief Network status change notification bits
 * 
 * These bits are used in FreeRTOS task notifications to signal
 * network status changes to registered tasks. The events abstract
 * away DHCP vs static IP complexity, so listener tasks don't need
 * to care about the connection type.
 */
#define NETWORK_NOTIFY_LINK_UP        (1UL << 0)  ///< PHY link established
#define NETWORK_NOTIFY_LINK_DOWN      (1UL << 1)  ///< PHY link lost
#define NETWORK_NOTIFY_READY          (1UL << 2)  ///< Network ready (link up + IP configured)
#define NETWORK_NOTIFY_NOT_READY      (1UL << 3)  ///< Network not ready (link down or config failed)
#define NETWORK_NOTIFY_IP_CHANGED     (1UL << 4)  ///< IP address changed

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

/**
 * @brief Register a task to receive network status change notifications
 * 
 * Registered tasks will receive FreeRTOS task notifications when network
 * status changes. The notification system abstracts away DHCP vs static IP
 * complexity - tasks simply wait for NETWORK_NOTIFY_READY.
 * 
 * @param task_handle Handle of the task to notify (NULL = current task)
 * @return true if registration successful, false otherwise
 * 
 * @note Tasks should use xTaskNotifyWait() or ulTaskNotifyTake() to receive notifications
 * @note Maximum 4 tasks can be registered (configurable)
 * 
 * Example usage:
 * @code
 * // In your task initialization:
 * network_task_register_notification(NULL);  // Register current task
 * 
 * // In your task loop:
 * uint32_t notification_value;
 * if (xTaskNotifyWait(0, UINT32_MAX, &notification_value, pdMS_TO_TICKS(1000)) == pdTRUE)
 * {
 *     if (notification_value & NETWORK_NOTIFY_READY)
 *         // Network is ready (link up + IP configured)
 *     if (notification_value & NETWORK_NOTIFY_NOT_READY)
 *         // Network is not ready (close connections, etc.)
 *     if (notification_value & NETWORK_NOTIFY_IP_CHANGED)
 *         // IP address changed (may need to reconnect)
 * }
 * @endcode
 */
bool network_task_register_notification(TaskHandle_t task_handle);

/**
 * @brief Unregister a task from receiving network status notifications
 * 
 * @param task_handle Handle of the task to unregister (NULL = current task)
 * @return true if unregistration successful, false otherwise
 */
bool network_task_unregister_notification(TaskHandle_t task_handle);

#endif // _NETWORK_TASK_H_
