/**
 * @file serial_to_tcp.c
 * @brief Serial-to-TCP Bridge Implementation
 * 
 * This module provides a transparent bidirectional bridge between UART and TCP.
 * Configuration is read once at task startup - changes require a system reboot.
 */

#include "serial_to_tcp.h"

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "wizchip_conf.h"
#include "socket.h"

#include "system_config.h"
#include "board_config.h"
#include "logger.h"
#include "network_task.h"

// Task configuration
#define S2TCP_TASK_PRIORITY     (tskIDLE_PRIORITY + 2)
#define S2TCP_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE * 4)  // 2KB (buffers are now static)

// Buffer sizes
#define UART_BUFFER_SIZE        2048
#define TCP_BUFFER_SIZE         2048

// Socket number to use (should not conflict with HTTP server sockets 0,1)
#define S2TCP_SOCKET_NUM        3

// Retry delays
#define TCP_RETRY_DELAY_MS      5000    // 5 seconds between reconnection attempts
#define TASK_POLL_DELAY_MS      10      // Task polling interval
#define NETWORK_READY_TIME_OUT_MS 30000 // 30 seconds

// Task state
typedef enum {
    S2TCP_STATE_DISABLED,
    S2TCP_STATE_INITIALIZING,
    S2TCP_STATE_TCP_CONNECTING,
    S2TCP_STATE_CONNECTED,
    S2TCP_STATE_ERROR
} s2tcp_state_t;

// Task context
typedef struct {
    serial_to_tcp_mode_config_t config;
    serial_config_t serial_config;
    uart_inst_t *uart;
    uint8_t socket_num;
    s2tcp_state_t state;
    uint32_t connect_attempts;
} s2tcp_context_t;

// Large buffers allocated statically to avoid stack overflow
static uint8_t s_tcp_buf[TCP_BUFFER_SIZE];
static uint8_t s_uart_buf[UART_BUFFER_SIZE];

/**
 * @brief Initialize UART for the bridge
 */
static bool s2tcp_init_uart(s2tcp_context_t *ctx)
{
    // Determine which UART to use
    ctx->uart = (ctx->config.serial_id == 0) ? BOARD_UART0_ID : BOARD_UART1_ID;
    
    // Get serial configuration
    if (!config_get_serial_config(ctx->config.serial_id, &ctx->serial_config))
    {
        LOG_ERROR("Failed to get serial%d configuration", ctx->config.serial_id);
        return false;
    }
    
    // Initialize UART with board-specific settings
    if (!board_init_uart(ctx->uart, &ctx->serial_config))
    {
        LOG_ERROR("Failed to initialize UART%d", ctx->config.serial_id);
        return false;
    }
    
    LOG_INFO("UART%d initialized: %u baud, %uN%u", 
             ctx->config.serial_id,
             ctx->serial_config.baud,
             ctx->serial_config.databits,
             ctx->serial_config.stopbits);
    
    return true;
}

/**
 * @brief Handle TCP server mode
 * @return 1 on success, <0 on error
 */
static int32_t s2tcp_handle_server(s2tcp_context_t *ctx)
{
    int32_t ret;
    uint16_t size;
    uint8_t sn = ctx->socket_num;
    
    switch (getSn_SR(sn))
    {
        case SOCK_ESTABLISHED:
            // Connection established
            if (getSn_IR(sn) & Sn_IR_CON)
            {
                uint8_t destip[4];
                uint16_t destport;
                getSn_DIPR(sn, destip);
                destport = getSn_DPORT(sn);
                
                LOG_INFO("Client connected: %d.%d.%d.%d:%d",
                         destip[0], destip[1], destip[2], destip[3], destport);
                
                setSn_IR(sn, Sn_IR_CON);
                ctx->state = S2TCP_STATE_CONNECTED;
                ctx->connect_attempts = 0;
            }
            
            // Check for data from TCP socket
            if ((size = getSn_RX_RSR(sn)) > 0)
            {
                if (size > TCP_BUFFER_SIZE)
                    size = TCP_BUFFER_SIZE;
                
                ret = recv(sn, s_tcp_buf, size);
                if (ret <= 0)
                    return ret;
                
                // Send data to UART
                size = (uint16_t)ret;
                for (uint16_t i = 0; i < size; i++)
                {
                    uart_putc_raw(ctx->uart, s_tcp_buf[i]);
                }
            }
            
            // Check for data from UART
            size = 0;
            while (uart_is_readable(ctx->uart) && size < UART_BUFFER_SIZE)
            {
                s_uart_buf[size++] = uart_getc(ctx->uart);
            }
            
            if (size > 0)
            {
                // Send data to TCP socket
                uint16_t sent = 0;
                while (sent < size)
                {
                    ret = send(sn, s_uart_buf + sent, size - sent);
                    if (ret < 0)
                    {
                        close(sn);
                        return ret;
                    }
                    sent += ret;
                }
            }
            break;
            
        case SOCK_CLOSE_WAIT:
            LOG_DEBUG("Client disconnecting - draining remaining data");
            
            // Drain ALL remaining data from TCP socket before closing
            // Loop until receive buffer is empty
            while ((size = getSn_RX_RSR(sn)) > 0)
            {
                if (size > TCP_BUFFER_SIZE)
                    size = TCP_BUFFER_SIZE;
                
                ret = recv(sn, s_tcp_buf, size);
                if (ret <= 0)
                {
                    LOG_WARN("Error draining socket buffer: %d", ret);
                    break;  // Exit loop but still disconnect below
                }
                
                // Send all remaining data to UART
                size = (uint16_t)ret;
                for (uint16_t i = 0; i < size; i++)
                {
                    uart_putc_raw(ctx->uart, s_tcp_buf[i]);
                }
                
                LOG_DEBUG("Drained %u bytes from closing socket", size);
            }
            
            // Note: We do NOT read from UART here - connection is closing,
            // any UART data would have nowhere to go and would be lost anyway
            
            // Now close the connection
            if ((ret = disconnect(sn)) != SOCK_OK)
                return ret;
            LOG_INFO("Socket closed");
            ctx->state = S2TCP_STATE_TCP_CONNECTING;
            break;
            
        case SOCK_INIT:
            LOG_INFO("Listening on port %d", ctx->config.local_port);
            if ((ret = listen(sn)) != SOCK_OK)
                return ret;
            ctx->state = S2TCP_STATE_TCP_CONNECTING;
            break;
            
        case SOCK_CLOSED:
            LOG_INFO("Opening TCP server socket on port %d", ctx->config.local_port);
            if ((ret = socket(sn, Sn_MR_TCP, ctx->config.local_port, 0x00)) != sn)
                return ret;
            break;
            
        default:
            break;
    }
    
    return 1;
}

/**
 * @brief Handle TCP client mode
 * @return 1 on success, <0 on error
 */
static int32_t s2tcp_handle_client(s2tcp_context_t *ctx)
{
    int32_t ret;
    uint16_t size;
    uint8_t sn = ctx->socket_num;
    static uint16_t any_port = 50000;
    
    switch (getSn_SR(sn))
    {
        case SOCK_ESTABLISHED:
            // Connection established
            if (getSn_IR(sn) & Sn_IR_CON)
            {
                LOG_INFO("Connected to %d.%d.%d.%d:%d",
                         ctx->config.remote_ip[0], ctx->config.remote_ip[1],
                         ctx->config.remote_ip[2], ctx->config.remote_ip[3],
                         ctx->config.remote_port);
                
                setSn_IR(sn, Sn_IR_CON);
                ctx->state = S2TCP_STATE_CONNECTED;
                ctx->connect_attempts = 0;
            }
            
            // Check for data from TCP socket
            if ((size = getSn_RX_RSR(sn)) > 0)
            {
                if (size > TCP_BUFFER_SIZE)
                    size = TCP_BUFFER_SIZE;
                
                ret = recv(sn, s_tcp_buf, size);
                if (ret <= 0)
                    return ret;
                
                // Send data to UART
                size = (uint16_t)ret;
                for (uint16_t i = 0; i < size; i++)
                {
                    uart_putc_raw(ctx->uart, s_tcp_buf[i]);
                }
            }
            
            // Check for data from UART
            size = 0;
            while (uart_is_readable(ctx->uart) && size < UART_BUFFER_SIZE)
            {
                s_uart_buf[size++] = uart_getc(ctx->uart);
            }
            
            if (size > 0)
            {
                // Send data to TCP socket
                uint16_t sent = 0;
                while (sent < size)
                {
                    ret = send(sn, s_uart_buf + sent, size - sent);
                    if (ret < 0)
                    {
                        close(sn);
                        return ret;
                    }
                    sent += ret;
                }
            }
            break;
            
        case SOCK_CLOSE_WAIT:
            LOG_DEBUG("Server disconnecting - draining remaining data");
            
            // Drain ALL remaining data from TCP socket before closing
            // Loop until receive buffer is empty
            while ((size = getSn_RX_RSR(sn)) > 0)
            {
                if (size > TCP_BUFFER_SIZE)
                    size = TCP_BUFFER_SIZE;
                
                ret = recv(sn, s_tcp_buf, size);
                if (ret <= 0)
                {
                    LOG_WARN("Error draining socket buffer: %d", ret);
                    break;  // Exit loop but still disconnect below
                }
                
                // Send all remaining data to UART
                size = (uint16_t)ret;
                for (uint16_t i = 0; i < size; i++)
                {
                    uart_putc_raw(ctx->uart, s_tcp_buf[i]);
                }
                
                LOG_DEBUG("Drained %u bytes from closing socket", size);
            }
            
            // Note: We do NOT read from UART here - connection is closing,
            // any UART data would have nowhere to go and would be lost anyway
            
            // Now close the connection
            if ((ret = disconnect(sn)) != SOCK_OK)
                return ret;
            LOG_INFO("Socket closed");
            ctx->state = S2TCP_STATE_TCP_CONNECTING;
            break;
            
        case SOCK_INIT:
            LOG_DEBUG("Connecting to %d.%d.%d.%d:%d (attempt %u)",
                      ctx->config.remote_ip[0], ctx->config.remote_ip[1],
                      ctx->config.remote_ip[2], ctx->config.remote_ip[3],
                      ctx->config.remote_port, ctx->connect_attempts + 1);
            
            if ((ret = connect(sn, ctx->config.remote_ip, ctx->config.remote_port)) != SOCK_OK)
            {
                ctx->connect_attempts++;
                return ret;
            }
            break;
            
        case SOCK_CLOSED:
            close(sn);
            if ((ret = socket(sn, Sn_MR_TCP, any_port++, 0x00)) != sn)
            {
                if (any_port == 0xffff)
                    any_port = 50000;
                return ret;
            }
            break;
            
        default:
            break;
    }
    
    return 1;
}

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

static void vSerialToTcpTask(void *pvParameters)
{
    s2tcp_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.socket_num = S2TCP_SOCKET_NUM;
    ctx.state = S2TCP_STATE_INITIALIZING;
    
    LOG_INFO("Serial-to-TCP task started");
    
    // Load configuration once at startup
    // NOTE: Configuration changes require a system reboot to take effect
    if (!config_get_serial_to_tcp_mode(&ctx.config))
    {
        LOG_ERROR("Failed to get configuration - task will exit");
        vTaskDelete(NULL);
        return;
    }
    
    // Check if mode is enabled
    if (!ctx.config.enable)
    {
        LOG_INFO("Mode disabled in configuration - task will exit");
        LOG_INFO("To enable, change configuration and reboot the system");
        vTaskDelete(NULL);
        return;
    }
    
    LOG_INFO("Mode enabled: %s on UART%d",
             ctx.config.mode == TCP_MODE_SERVER ? "Server" : "Client",
             ctx.config.serial_id);
    
    // Initialize UART
    if (!s2tcp_init_uart(&ctx))
    {
        LOG_ERROR("Failed to initialize UART - task will exit");
        vTaskDelete(NULL);
        return;
    }
    
    // Register for network status change notifications
    network_task_register_notification(NULL);
    
    // State variables for the main loop
    bool network_ready = false;
    bool was_connected = false;
    
    // Main task loop - implements state machine from workflow diagram
    while (1)
    {
        // Step 1: Wait for network to be ready (with timeout)
        if (!wait_for_network_ready(NETWORK_READY_TIME_OUT_MS))  // 30 second timeout
        {
            // Network not ready after timeout
            if (was_connected)
            {
                // We had a connection before - close it now
                LOG_WARN("Network timeout - closing connections");
                close(ctx.socket_num);
                ctx.state = S2TCP_STATE_INITIALIZING;
                was_connected = false;
            }
            else
            {
                // Never had a connection - just log and retry
                LOG_INFO("Waiting for network connection...");
            }
            // Loop back to wait again
            continue;
        }
        
        // Network is ready!
        network_ready = true;
        LOG_INFO("Network ready - entering operation mode");
        
        // Step 2-4: Handle TCP operations and network events
        while (network_ready)
        {
            uint32_t notification_value = 0;
            
            // Check for network status changes (non-blocking)
            if (xTaskNotifyWait(0, UINT32_MAX, &notification_value, 0) == pdTRUE)
            {
                if (notification_value & NETWORK_NOTIFY_NOT_READY)
                {
                    // Network lost - give it a grace period
                    LOG_WARN("Network not ready - waiting for recovery");
                    network_ready = false;
                    // Break to outer loop to wait_for_network_ready()
                    break;
                }
                
                if (notification_value & NETWORK_NOTIFY_IP_CHANGED)
                {
                    // IP changed - must close and reinitialize immediately
                    LOG_INFO("Network IP changed - reinitializing");
                    
                    if (was_connected)
                    {
                        close(ctx.socket_num);
                        ctx.state = S2TCP_STATE_INITIALIZING;
                        was_connected = false;
                    }
                    
                    network_ready = false;
                    // Break to outer loop to wait_for_network_ready()
                    break;
                }
                
                if (notification_value & NETWORK_NOTIFY_READY)
                {
                    // Network ready confirmation (redundant but safe)
                    network_ready = true;
                }
            }
            
            // Handle TCP connection based on mode
            int32_t result;
            if (ctx.config.mode == TCP_MODE_SERVER)
            {
                result = s2tcp_handle_server(&ctx);
            }
            else
            {
                result = s2tcp_handle_client(&ctx);
            }
            
            // Track connection state
            if (ctx.state == S2TCP_STATE_CONNECTED && !was_connected)
            {
                was_connected = true;
                LOG_DEBUG("Connection established");
            }
            else if (ctx.state != S2TCP_STATE_CONNECTED && was_connected)
            {
                was_connected = false;
                LOG_DEBUG("Connection closed");
            }
            
            // Handle errors
            if (result < 0)
            {
                LOG_WARN("Socket error: %d", result);
                
                // For client mode, implement retry logic with backoff
                if (ctx.config.mode == TCP_MODE_CLIENT && ctx.state == S2TCP_STATE_TCP_CONNECTING)
                {
                    ctx.connect_attempts++;
                    if ((ctx.connect_attempts % 10) == 0)
                    {
                        LOG_WARN("Connection failed after %u attempts, retrying...", 
                                 ctx.connect_attempts);
                    }
                    vTaskDelay(pdMS_TO_TICKS(TCP_RETRY_DELAY_MS));
                }
            }
            
            // Small delay to prevent tight loop
            vTaskDelay(pdMS_TO_TICKS(TASK_POLL_DELAY_MS));
        }
        
        // If we exit the inner loop, network_ready is false
        // Loop back to wait_for_network_ready()
    }
}

/**
 * @brief Initialize and create the Serial-to-TCP bridge task
 */
bool serial_to_tcp_task_init(void)
{
    BaseType_t result = xTaskCreate(
        vSerialToTcpTask,
        "S2TCP_Task",
        S2TCP_TASK_STACK_SIZE,
        NULL,
        S2TCP_TASK_PRIORITY,
        NULL
    );
    
    if (result != pdPASS)
    {
        LOG_ERROR("Failed to create Serial-to-TCP task");
        return false;
    }
    
    LOG_INFO("Serial-to-TCP task created successfully");
    return true;
}
