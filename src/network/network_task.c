/**
 * @file network_task.c
 * @brief Network management task implementation
 */

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "pico/stdlib.h"
#include "wizchip_conf.h"
#include "wizchip_spi.h"
#include "dhcp.h"
#include "timer.h"

#include "system_config.h"
#include "logger.h"
#include "network_task.h"

// Task configuration
#define NETWORK_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE * 8)  // 1KB stack
#define NETWORK_TASK_PRIORITY     (tskIDLE_PRIORITY + 3)          // Higher priority for network
#define NETWORK_TASK_DELAY_MS     250                             // Task loop delay
#define NETWORK_TASK_WAIT_PHY_CONNECT_MS 1000                     // Wait 1 second for PHY link to connect
#define NETWORK_TASK_WAIT_DHCP_RETRY_MS 10000                     // Wait 10 seconds for DHCP retry

// DHCP configuration
#define DHCP_SOCKET               2                               // Socket for DHCP client (avoid 0,1 used by HTTP)
#define DHCP_RETRY_COUNT          5                               // Max retry attempts
#define DHCP_ETHERNET_BUF_SIZE    2048                            // DHCP buffer size

// Timer callback counter (for 1-second DHCP handler)
static volatile uint32_t s_msec_counter = 0;

// Network state
static bool s_network_initialized = false;
static bool s_dhcp_enabled = false;
static bool s_dhcp_leased = false;
static uint32_t s_dhcp_retry_count = 0;
static SemaphoreHandle_t s_network_mutex = NULL;

// DHCP buffer (static allocation - safer and simpler than heap allocation)
static uint8_t s_dhcp_buffer[DHCP_ETHERNET_BUF_SIZE] = {0};

// Notification system
#define MAX_NOTIFICATION_TASKS 4  // Maximum number of tasks that can register for notifications
static TaskHandle_t s_notification_tasks[MAX_NOTIFICATION_TASKS] = {NULL};
static SemaphoreHandle_t s_notification_mutex = NULL;

/**
 * @brief Send notification to all registered tasks
 * 
 * @param notification_bits Bit flags indicating what changed (NETWORK_NOTIFY_*)
 */
static void network_send_notification(uint32_t notification_bits)
{
    if (s_notification_mutex == NULL)
        return;
    
    if (xSemaphoreTake(s_notification_mutex, pdMS_TO_TICKS(10)) != pdTRUE)
        return;
    
    

    LOG_DEBUG("Sending notification: LINK_UP %d, LINK_DOWN %d, READY %d, NOT_READY %d, IP_CHANGED %d", 
              notification_bits & NETWORK_NOTIFY_LINK_UP, 
              notification_bits & NETWORK_NOTIFY_LINK_DOWN, 
              notification_bits & NETWORK_NOTIFY_READY, 
              notification_bits & NETWORK_NOTIFY_NOT_READY, 
              notification_bits & NETWORK_NOTIFY_IP_CHANGED);
    
    // Send notification to all registered tasks
    for (int i = 0; i < MAX_NOTIFICATION_TASKS; i++)
    {
        if (s_notification_tasks[i] != NULL)
        {
            // Use xTaskNotify() with eSetBits to OR the bits (preserves previous notifications)
            xTaskNotify(s_notification_tasks[i], notification_bits, eSetBits);
        }
    }
    
    xSemaphoreGive(s_notification_mutex);
}

/**
 * @brief 1ms timer callback for DHCP time handler
 * 
 * This is called every 1ms by the hardware timer.
 * We accumulate milliseconds and call DHCP_time_handler() every second.
 * 
 * Note: This function is called from interrupt context.
 */
static void network_timer_callback(void)
{
    s_msec_counter++;
    
    // Call DHCP time handler every second (1000ms)
    if (s_msec_counter >= 1000)
    {
        s_msec_counter = 0;
        DHCP_time_handler();
    }
}

/**
 * @brief DHCP callback: IP assigned/updated
 * 
 * Called by DHCP library when IP is assigned or updated.
 */
static void network_dhcp_assign(void)
{
    wiz_NetInfo net_info;
    uint8_t old_ip[4] = {0, 0, 0, 0};
    bool ip_changed = false;
    
    // First, get MAC and other config from configuration (preserves MAC address)
    if (!config_get_net_info(&net_info))
    {
        LOG_ERROR("Failed to get network config for DHCP");
        return;
    }
    
    // Save old IP to compare later
    memcpy(old_ip, net_info.ip, 4);
    
    // Then, get IP/GW/SN/DNS from DHCP (overwrites static values)
    getIPfromDHCP(net_info.ip);
    getGWfromDHCP(net_info.gw);
    getSNfromDHCP(net_info.sn);
    getDNSfromDHCP(net_info.dns);
    
    // Check if IP actually changed
    if (memcmp(old_ip, net_info.ip, 4) != 0)
    {
        ip_changed = true;
    }
    
    // Set DHCP mode
    net_info.dhcp = NETINFO_DHCP;
    
    // Apply network configuration
    network_initialize(net_info);
    
    // Save to configuration
    config_set_net_info(&net_info);
    // config_save_to_flash();
    
    // Print network information
    print_network_information(net_info);
    
    uint32_t lease_time = getDHCPLeasetime();
    LOG_INFO("DHCP IP leased: %d.%d.%d.%d (lease: %lu seconds)",
             net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3],
             lease_time);
    
    s_dhcp_leased = true;
    s_dhcp_retry_count = 0;
    
    // Notify registered tasks - network is now ready
    uint32_t notification_bits = NETWORK_NOTIFY_READY;
    if (ip_changed)
    {
        notification_bits |= NETWORK_NOTIFY_IP_CHANGED;
    }
    network_send_notification(notification_bits);
}

/**
 * @brief DHCP callback: IP conflict detected
 * 
 * Called by DHCP library when IP conflict is detected.
 */
static void network_dhcp_conflict(void)
{
    LOG_ERROR("DHCP IP conflict detected!");
    // In production, you might want to restart DHCP or use fallback IP
}

/**
 * @brief Initialize DHCP client
 */
static void network_dhcp_init(void)
{
    LOG_INFO("Starting DHCP client...");
    
    DHCP_init(DHCP_SOCKET, s_dhcp_buffer);
    reg_dhcp_cbfunc(network_dhcp_assign, network_dhcp_assign, network_dhcp_conflict);
    
    s_dhcp_leased = false;
    s_dhcp_retry_count = 0;
}

/**
 * @brief Initialize static IP configuration
 */
static void network_static_init(void)
{
    wiz_NetInfo net_info;
    
    if (!config_get_net_info(&net_info))
    {
        LOG_ERROR("Failed to get network configuration");
        return;
    }
    
    // Ensure DHCP is disabled
    net_info.dhcp = NETINFO_STATIC;
    
    // Apply network configuration
    network_initialize(net_info);
    
    // Print network information
    print_network_information(net_info);
    
    LOG_INFO("Static IP configured: %d.%d.%d.%d",
             net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);
    
    // Notify registered tasks - network is now ready
    network_send_notification(NETWORK_NOTIFY_READY);
}

/**
 * @brief Network management task
 * 
 * This task:
 * - Monitors PHY link status
 * - Manages DHCP client (if enabled)
 * - Handles reconnection on link loss
 */
static void vNetworkTask(void *pvParameters)
{
    (void)pvParameters;
    
    uint8_t link_status;
    int dhcp_result;
    bool link_was_up = false;
    
    LOG_INFO("Network task started");
    
    // Load network configuration to determine DHCP vs Static mode
    wiz_NetInfo net_info;
    if (!config_get_net_info(&net_info))
    {
        LOG_ERROR("Failed to get network configuration");
        vTaskDelete(NULL);
        return;
    }
    
    s_dhcp_enabled = (net_info.dhcp == NETINFO_DHCP);
    LOG_INFO("Network mode: %s", s_dhcp_enabled ? "DHCP" : "Static IP");
    
    // Small delay to ensure other tasks are initialized and has been registered for notifications
    vTaskDelay(pdMS_TO_TICKS(100));
    
    while (1)
    {
        // Check PHY link status
        link_status = wizphy_getphylink();
        
        if (link_status == PHY_LINK_OFF)
        {
            if (link_was_up)
            {
                LOG_WARN("PHY link lost");
                link_was_up = false;
                
                if (s_dhcp_enabled)
                {
                    DHCP_stop();
                    s_dhcp_leased = false;
                }
                
                // Notify registered tasks - network is down and not ready
                network_send_notification(NETWORK_NOTIFY_LINK_DOWN | NETWORK_NOTIFY_NOT_READY);
            }
            
            // Wait for link to come back
            vTaskDelay(pdMS_TO_TICKS(NETWORK_TASK_WAIT_PHY_CONNECT_MS));
            continue;
        }
        
        // Link is up
        if (!link_was_up)
        {
            LOG_INFO("PHY link established");
            link_was_up = true;
            
            // Notify link is up
            network_send_notification(NETWORK_NOTIFY_LINK_UP);
            
            if (s_dhcp_enabled)
            {
                // Start DHCP - will send NETWORK_NOTIFY_READY when lease obtained
                network_dhcp_init();
            }
            else
            {
                // Static IP - configure and send READY notification
                network_static_init();
            }
        }
        
        // Handle DHCP if enabled
        if (s_dhcp_enabled)
        {
            dhcp_result = DHCP_run();
            
            if (dhcp_result == DHCP_IP_LEASED)
            {
                // IP successfully leased (handled in callback)
                if (s_dhcp_leased)
                {
                    // Already logged, just continue
                }
            }
            else if (dhcp_result == DHCP_FAILED)
            {
                s_dhcp_leased = false;
                s_dhcp_retry_count++;
                
                if (s_dhcp_retry_count <= DHCP_RETRY_COUNT)
                {
                    LOG_WARN("DHCP timeout, retry %lu/%d",
                             s_dhcp_retry_count, DHCP_RETRY_COUNT);
                }
                else
                {
                    LOG_ERROR("DHCP failed after %d retries", DHCP_RETRY_COUNT);
                    DHCP_stop();
                    
                    // Notify registered tasks - network not ready
                    network_send_notification(NETWORK_NOTIFY_NOT_READY);
                    
                    // Keep trying periodically
                    vTaskDelay(pdMS_TO_TICKS(NETWORK_TASK_WAIT_DHCP_RETRY_MS));
                    s_dhcp_retry_count = 0;
                    network_dhcp_init();
                }
            }
        }
        
        // Task delay
        vTaskDelay(pdMS_TO_TICKS(NETWORK_TASK_DELAY_MS));
    }
}

/**
 * @brief Initialize network management task
 */
bool network_task_init(void)
{
    // Create mutex for thread-safe network status access
    s_network_mutex = xSemaphoreCreateMutex();
    if (s_network_mutex == NULL)
    {
        LOG_ERROR("Failed to create network mutex");
        return false;
    }
    
    // Create mutex for notification system
    s_notification_mutex = xSemaphoreCreateMutex();
    if (s_notification_mutex == NULL)
    {
        LOG_ERROR("Failed to create notification mutex");
        vSemaphoreDelete(s_network_mutex);
        s_network_mutex = NULL;
        return false;
    }
    
    // Initialize Wiznet chip
    LOG_INFO("Initializing Wiznet chip...");
    wizchip_spi_initialize();
    wizchip_cris_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();
    
    // Initialize 1ms timer for DHCP
    wizchip_1ms_timer_initialize(network_timer_callback);
    
    // Create network management task
    BaseType_t result = xTaskCreate(
        vNetworkTask,
        "Network",
        NETWORK_TASK_STACK_SIZE,
        NULL,
        NETWORK_TASK_PRIORITY,
        NULL
    );
    
    if (result != pdPASS)
    {
        LOG_ERROR("Failed to create network task");
        vSemaphoreDelete(s_notification_mutex);
        vSemaphoreDelete(s_network_mutex);
        s_notification_mutex = NULL;
        s_network_mutex = NULL;
        return false;
    }
    
    s_network_initialized = true;
    LOG_INFO("Network task created successfully");
    
    return true;
}

/**
 * @brief Register a task to receive network status change notifications
 */
bool network_task_register_notification(TaskHandle_t task_handle)
{
    if (!s_network_initialized || s_notification_mutex == NULL)
    {
        return false;
    }
    
    // Use current task if NULL provided
    if (task_handle == NULL)
    {
        task_handle = xTaskGetCurrentTaskHandle();
        if (task_handle == NULL)
        {
            return false;  // Can't register from non-task context
        }
    }
    
    if (xSemaphoreTake(s_notification_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        return false;
    }
    
    // Check if already registered
    for (int i = 0; i < MAX_NOTIFICATION_TASKS; i++)
    {
        if (s_notification_tasks[i] == task_handle)
        {
            xSemaphoreGive(s_notification_mutex);
            return true;  // Already registered
        }
    }
    
    // Find empty slot
    for (int i = 0; i < MAX_NOTIFICATION_TASKS; i++)
    {
        if (s_notification_tasks[i] == NULL)
        {
            s_notification_tasks[i] = task_handle;
            xSemaphoreGive(s_notification_mutex);
            LOG_INFO("Task '%s' registered for notifications", pcTaskGetName(task_handle));
            return true;
        }
    }
    
    xSemaphoreGive(s_notification_mutex);
    LOG_ERROR("Maximum notification tasks reached");
    return false;  // No free slots
}

/**
 * @brief Unregister a task from receiving network status notifications
 */
bool network_task_unregister_notification(TaskHandle_t task_handle)
{
    if (!s_network_initialized || s_notification_mutex == NULL)
    {
        return false;
    }
    
    // Use current task if NULL provided
    if (task_handle == NULL)
    {
        task_handle = xTaskGetCurrentTaskHandle();
        if (task_handle == NULL)
        {
            return false;
        }
    }
    
    if (xSemaphoreTake(s_notification_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        return false;
    }
    
    // Find and remove
    for (int i = 0; i < MAX_NOTIFICATION_TASKS; i++)
    {
        if (s_notification_tasks[i] == task_handle)
        {
            s_notification_tasks[i] = NULL;
            xSemaphoreGive(s_notification_mutex);
            LOG_INFO("Task '%s' unregistered from notifications", pcTaskGetName(task_handle));
            return true;
        }
    }
    
    xSemaphoreGive(s_notification_mutex);
    LOG_WARN("Task '%s' not found in notification tasks", pcTaskGetName(task_handle));
    return false;  // Not found
}

/**
 * @brief Get current network status
 */
bool network_task_get_status(uint8_t ip_address[4], bool *link_status)
{
    if (!s_network_initialized || s_network_mutex == NULL)
    {
        return false;
    }
    
    if (xSemaphoreTake(s_network_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        return false;
    }
    
    wiz_NetInfo net_info;
    if (config_get_net_info(&net_info))
    {
        memcpy(ip_address, net_info.ip, 4);
        *link_status = (wizphy_getphylink() == PHY_LINK_ON);
    }
    else
    {
        xSemaphoreGive(s_network_mutex);
        return false;
    }
    
    xSemaphoreGive(s_network_mutex);
    return true;
}
