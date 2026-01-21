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

// Task configuration
#define NETWORK_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE * 8)  // 1KB stack
#define NETWORK_TASK_PRIORITY     (tskIDLE_PRIORITY + 3)          // Higher priority for network
#define NETWORK_TASK_DELAY_MS     250                             // Task loop delay

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

// DHCP buffer (allocated on heap to save stack space)
static uint8_t *s_dhcp_buffer = NULL;

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
    
    // First, get MAC and other config from configuration (preserves MAC address)
    if (!config_get_net_info(&net_info))
    {
        LOG_ERROR("[Network] Failed to get network config for DHCP");
        return;
    }
    
    // Then, get IP/GW/SN/DNS from DHCP (overwrites static values)
    getIPfromDHCP(net_info.ip);
    getGWfromDHCP(net_info.gw);
    getSNfromDHCP(net_info.sn);
    getDNSfromDHCP(net_info.dns);
    
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
    LOG_INFO("[Network] DHCP IP leased: %d.%d.%d.%d (lease: %lu seconds)",
             net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3],
             lease_time);
    
    s_dhcp_leased = true;
    s_dhcp_retry_count = 0;
}

/**
 * @brief DHCP callback: IP conflict detected
 * 
 * Called by DHCP library when IP conflict is detected.
 */
static void network_dhcp_conflict(void)
{
    LOG_ERROR("[Network] DHCP IP conflict detected!");
    // In production, you might want to restart DHCP or use fallback IP
}

/**
 * @brief Initialize DHCP client
 */
static void network_dhcp_init(void)
{
    if (s_dhcp_buffer == NULL)
    {
        LOG_ERROR("[Network] DHCP buffer not allocated");
        return;
    }
    
    LOG_INFO("[Network] Starting DHCP client...");
    
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
        LOG_ERROR("[Network] Failed to get network configuration");
        return;
    }
    
    // Ensure DHCP is disabled
    net_info.dhcp = NETINFO_STATIC;
    
    // Apply network configuration
    network_initialize(net_info);
    
    // Print network information
    print_network_information(net_info);
    
    LOG_INFO("[Network] Static IP configured: %d.%d.%d.%d",
             net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);
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
    
    LOG_INFO("[Network] Network task started");
    
    // Initialize network based on configuration
    wiz_NetInfo net_info;
    if (!config_get_net_info(&net_info))
    {
        LOG_ERROR("[Network] Failed to get network configuration");
        vTaskDelete(NULL);
        return;
    }
    
    s_dhcp_enabled = (net_info.dhcp == NETINFO_DHCP);
    
    if (s_dhcp_enabled)
    {
        network_dhcp_init();
    }
    else
    {
        network_static_init();
    }
    
    while (1)
    {
        // Check PHY link status
        link_status = wizphy_getphylink();
        
        if (link_status == PHY_LINK_OFF)
        {
            if (link_was_up)
            {
                LOG_WARN("[Network] PHY link lost");
                link_was_up = false;
                
                if (s_dhcp_enabled)
                {
                    DHCP_stop();
                    s_dhcp_leased = false;
                }
            }
            
            // Wait for link to come back
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        // Link is up
        if (!link_was_up)
        {
            LOG_INFO("[Network] PHY link established");
            link_was_up = true;
            
            if (s_dhcp_enabled)
            {
                network_dhcp_init();
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
                    LOG_WARN("[Network] DHCP timeout, retry %lu/%d",
                             s_dhcp_retry_count, DHCP_RETRY_COUNT);
                }
                else
                {
                    LOG_ERROR("[Network] DHCP failed after %d retries", DHCP_RETRY_COUNT);
                    DHCP_stop();
                    
                    // Keep trying periodically
                    vTaskDelay(pdMS_TO_TICKS(10000));  // Wait 10 seconds before retry
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
    // Allocate DHCP buffer
    s_dhcp_buffer = pvPortMalloc(DHCP_ETHERNET_BUF_SIZE);
    if (s_dhcp_buffer == NULL)
    {
        LOG_ERROR("[Network] Failed to allocate DHCP buffer");
        return false;
    }
    
    // Create mutex for thread-safe network status access
    s_network_mutex = xSemaphoreCreateMutex();
    if (s_network_mutex == NULL)
    {
        LOG_ERROR("[Network] Failed to create network mutex");
        vPortFree(s_dhcp_buffer);
        s_dhcp_buffer = NULL;
        return false;
    }
    
    // Initialize Wiznet chip
    LOG_INFO("[Network] Initializing Wiznet chip...");
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
        LOG_ERROR("[Network] Failed to create network task");
        vSemaphoreDelete(s_network_mutex);
        vPortFree(s_dhcp_buffer);
        s_network_mutex = NULL;
        s_dhcp_buffer = NULL;
        return false;
    }
    
    s_network_initialized = true;
    LOG_INFO("[Network] Network task created successfully");
    
    return true;
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
