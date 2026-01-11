/**
 * @file serial_to_tcp.c
 * @brief Serial-to-TCP Bridge Implementation
 */

#include "serial_to_tcp.h"
#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "wizchip_conf.h"
#include "socket.h"

#include "system_config.h"
#include "board_config.h"

// Task configuration
#define S2TCP_TASK_PRIORITY     (tskIDLE_PRIORITY + 2)
#define S2TCP_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE * 4)  // 2KB (buffers are now static)

// Buffer sizes
#define UART_BUFFER_SIZE        2048
#define TCP_BUFFER_SIZE         2048

// Socket number to use (should not conflict with HTTP server sockets 0,1)
#define S2TCP_SOCKET_NUM        2

// Retry delays
#define TCP_RETRY_DELAY_MS      5000    // 5 seconds between reconnection attempts
#define TASK_POLL_DELAY_MS      10      // Task polling interval

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
        printf("[S2TCP] ERROR: Failed to get serial%d configuration\n", ctx->config.serial_id);
        return false;
    }
    
    // Initialize UART with board-specific settings
    if (!board_init_uart(ctx->uart, &ctx->serial_config))
    {
        printf("[S2TCP] ERROR: Failed to initialize UART%d\n", ctx->config.serial_id);
        return false;
    }
    
    printf("[S2TCP] UART%d initialized: %u baud, %uN%u\n", 
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
                
                printf("[S2TCP] Client connected: %d.%d.%d.%d:%d\n",
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
            printf("[S2TCP] Client disconnecting\n");
            if ((ret = disconnect(sn)) != SOCK_OK)
                return ret;
            printf("[S2TCP] Socket closed\n");
            ctx->state = S2TCP_STATE_TCP_CONNECTING;
            break;
            
        case SOCK_INIT:
            printf("[S2TCP] Listening on port %d\n", ctx->config.local_port);
            if ((ret = listen(sn)) != SOCK_OK)
                return ret;
            ctx->state = S2TCP_STATE_TCP_CONNECTING;
            break;
            
        case SOCK_CLOSED:
            printf("[S2TCP] Opening TCP server socket on port %d\n", ctx->config.local_port);
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
                printf("[S2TCP] Connected to %d.%d.%d.%d:%d\n",
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
            printf("[S2TCP] Server disconnecting\n");
            if ((ret = disconnect(sn)) != SOCK_OK)
                return ret;
            printf("[S2TCP] Socket closed\n");
            ctx->state = S2TCP_STATE_TCP_CONNECTING;
            break;
            
        case SOCK_INIT:
            printf("[S2TCP] Connecting to %d.%d.%d.%d:%d (attempt %u)\n",
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
 * @brief Serial-to-TCP bridge task
 */
static void vSerialToTcpTask(void *pvParameters)
{
    s2tcp_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.socket_num = S2TCP_SOCKET_NUM;
    ctx.state = S2TCP_STATE_DISABLED;
    
    // Small delay to ensure other tasks are initialized
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("[S2TCP] Task started\n");
    
    while (1)
    {
        // Get current configuration
        if (!config_get_serial_to_tcp_mode(&ctx.config))
        {
            printf("[S2TCP] ERROR: Failed to get configuration\n");
            ctx.state = S2TCP_STATE_ERROR;
            vTaskDelay(pdMS_TO_TICKS(TCP_RETRY_DELAY_MS));
            continue;
        }
        
        // Check if mode is enabled
        if (!ctx.config.enable)
        {
            if (ctx.state != S2TCP_STATE_DISABLED)
            {
                printf("[S2TCP] Mode disabled, closing socket\n");
                close(ctx.socket_num);
                ctx.state = S2TCP_STATE_DISABLED;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));  // Check every second
            continue;
        }
        
        // Initialize UART if needed
        if (ctx.state == S2TCP_STATE_DISABLED || ctx.state == S2TCP_STATE_ERROR)
        {
            printf("[S2TCP] Mode enabled: %s on UART%d\n",
                   ctx.config.mode == TCP_MODE_SERVER ? "Server" : "Client",
                   ctx.config.serial_id);
            
            if (!s2tcp_init_uart(&ctx))
            {
                ctx.state = S2TCP_STATE_ERROR;
                vTaskDelay(pdMS_TO_TICKS(TCP_RETRY_DELAY_MS));
                continue;
            }
            
            ctx.state = S2TCP_STATE_INITIALIZING;
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
        
        if (result < 0)
        {
            printf("[S2TCP] Socket error: %d\n", result);
            
            // For client mode, implement retry logic
            if (ctx.config.mode == TCP_MODE_CLIENT && ctx.state == S2TCP_STATE_TCP_CONNECTING)
            {
                if (ctx.connect_attempts > 0 && (ctx.connect_attempts % 10) == 0)
                {
                    printf("[S2TCP] Connection failed after %u attempts, retrying...\n", ctx.connect_attempts);
                }
                vTaskDelay(pdMS_TO_TICKS(TCP_RETRY_DELAY_MS));
            }
        }
        
        // Small delay to prevent tight loop
        vTaskDelay(pdMS_TO_TICKS(TASK_POLL_DELAY_MS));
    }
}

/**
 * @brief Create the Serial-to-TCP bridge task
 */
void vCreateSerialToTcpTask(void)
{
    BaseType_t xReturned;
    
    xReturned = xTaskCreate(
        vSerialToTcpTask,
        "S2TCP_Task",
        S2TCP_TASK_STACK_SIZE,
        NULL,
        S2TCP_TASK_PRIORITY,
        NULL
    );
    
    if (xReturned != pdPASS)
    {
        printf("[S2TCP] ERROR: Failed to create task\n");
    }
    else
    {
        printf("[S2TCP] Task created successfully\n");
    }
}
