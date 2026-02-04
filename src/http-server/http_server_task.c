/**
 * @file http_server_task.c
 * @brief HTTP Server FreeRTOS Task Implementation
 */

#include "http_server_task.h"

#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"

#include "system_config.h"
#include "network_task.h"
#include "logger.h"
#include "http_utils.h"
#include "web_page.h"

// HTTP Server Configuration
#define HTTP_SOCKET_MAX_NUM 2
#define HTTP_TASK_STACK_SIZE 1024
#define HTTP_TASK_PRIORITY 2

// HTTP Server buffers and socket configuration
static uint8_t g_http_send_buf[2048] = {0};
static uint8_t g_http_recv_buf[2048] = {0};
static uint8_t g_http_socket_num_list[HTTP_SOCKET_MAX_NUM] = {0, 1};

/**
 * @brief Wait for network to be ready (link up and IP assigned)
 * 
 * Simplified version that works for both DHCP and static IP modes.
 * Simply waits for the NETWORK_NOTIFY_READY event.
 * 
 * @return true if network is ready, false on timeout
 */
static bool wait_for_network_ready(uint32_t timeout_ms)
{
    uint32_t start_time = xTaskGetTickCount();
    
    LOG_INFO("Waiting for network to be ready...");
    
    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(timeout_ms))
    {
        uint32_t notification_value = 0;
        
        // Wait for notification with timeout
        if (xTaskNotifyWait(0, UINT32_MAX, &notification_value, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            if (notification_value & NETWORK_NOTIFY_READY)
            {
                // Network is ready (link up + IP configured)
                return true;
            }
        }
    }
    
    return false;
}

/**
 * @brief HTTP Server Task
 * 
 * This task runs the HTTP web server for configuration management.
 * It waits for network to be ready before starting and handles network status changes.
 */
static void vHttpServerTask(void *pvParameters)
{
    (void)pvParameters;
    
    LOG_INFO("HTTP Server task started");
    
    // Register for network status change notifications
    network_task_register_notification(NULL);
    
    // Wait for network to be ready (link up and IP assigned)
    LOG_INFO("Waiting for network to be ready...");
    if (!wait_for_network_ready(30000))  // 30 second timeout
    {
        LOG_WARN("Network not ready after timeout, starting anyway");
    }
    else
    {
        LOG_INFO("Network ready");
    }
    
    bool network_ready = true;
    
    while (1)
    {
        uint32_t notification_value = 0;
        
        // Check for network status changes (non-blocking)
        if (xTaskNotifyWait(0, UINT32_MAX, &notification_value, 0) == pdTRUE)
        {
            if (notification_value & NETWORK_NOTIFY_NOT_READY)
            {
                LOG_WARN("Network not ready");
                network_ready = false;
            }
            
            if (notification_value & NETWORK_NOTIFY_READY)
            {
                LOG_INFO("Network ready");
                network_ready = true;
            }
            
            if (notification_value & NETWORK_NOTIFY_IP_CHANGED)
            {
                LOG_INFO("Network IP changed");
            }
        }
        
        // Run HTTP server only if network is ready
        if (network_ready)
        {
            for (uint8_t i = 0; i < HTTP_SOCKET_MAX_NUM; i++)
            {
                httpServer_run(i);
            }
        }
        else
        {
            // Network not ready - wait a bit longer before checking again
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        // Small delay to prevent tight loop
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Initialize and create the HTTP server task
 */
bool http_server_task_init(void)
{
    // Initialize HTTP Server library
    httpServer_init(g_http_send_buf, g_http_recv_buf, HTTP_SOCKET_MAX_NUM, g_http_socket_num_list);
    reg_httpServer_webContent("index.html", index_page);
    LOG_INFO("HTTP Server initialized on port 80");
    
    // Create the HTTP server FreeRTOS task
    BaseType_t result = xTaskCreate(
        vHttpServerTask,
        "HTTP Server",
        HTTP_TASK_STACK_SIZE,
        NULL,
        HTTP_TASK_PRIORITY,
        NULL
    );
    
    if (result != pdPASS)
    {
        LOG_ERROR("Failed to create HTTP server task");
        return false;
    }
    
    LOG_INFO("HTTP server task created successfully");
    return true;
}
