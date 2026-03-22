/**
 * @file modbus_tcp_client.c
 * @brief Modbus TCP client task implementation (nanoMODBUS library)
 *
 * FreeRTOS task that runs a Modbus TCP client. Connects to a remote Modbus TCP
 * server, polls all enabled request slots each cycle, and maps results to the
 * Tag Database. Configuration is read from system_config at startup.
 *
 * Socket state machine:
 *   SOCK_CLOSED → socket() → SOCK_INIT → connect() → SOCK_ESTABLISHED
 *   → poll requests each cycle → (server disconnects or error)
 *   → SOCK_CLOSE_WAIT → disconnect() → SOCK_CLOSED (cycle)
 */

#include "modbus_tcp_client.h"

#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "wizchip_conf.h"
#include "socket.h"

#include "logger.h"
#include "modbus_request.h"
#include "modbus_request_processor.h"
#include "modbus_tag_mapping.h"
#include "nanomodbus.h"
#include "network_task.h"
#include "system_config.h"

/* Task configuration */
#define MODBUS_TCP_CLIENT_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE * 4)  /* 2KB for nanoMODBUS buffers */
#define MODBUS_TCP_CLIENT_TASK_PRIORITY     (tskIDLE_PRIORITY + 2)
#define MODBUS_TCP_CLIENT_LOOP_DELAY_MS     1000  /* Poll cycle period */

/* nanoMODBUS timeouts (ms) */
#define MODBUS_TCP_CLIENT_READ_TIMEOUT_MS   3000  /* Wait for server response */
#define MODBUS_TCP_CLIENT_BYTE_TIMEOUT_MS   1000  /* Inter-byte gap tolerance */

/* Network readiness timeout */
#define MODBUS_TCP_CLIENT_NET_TIMEOUT_MS    30000

/* Retry delay after connection failure */
#define MODBUS_TCP_CLIENT_RETRY_DELAY_MS    5000

/* WizNet socket assigned to the Modbus TCP client (sockets 0-1: HTTP, 2: DHCP, 3: S2TCP, 4: TCP Server) */
#define MODBUS_TCP_CLIENT_SOCKET_NUM        5

/* Local ephemeral port base (incremented on each reconnect) */
#define MODBUS_TCP_CLIENT_LOCAL_PORT_BASE   50100

/* Socket number used by transport callbacks */
static uint8_t s_socket_num = MODBUS_TCP_CLIENT_SOCKET_NUM;

/* Local ephemeral port, incremented on each reconnect to avoid TIME_WAIT */
static uint16_t s_local_port = MODBUS_TCP_CLIENT_LOCAL_PORT_BASE;

/* Request configurations and results (loaded from flash at startup) */
static modbus_request_config_t  s_request_configs[MODBUS_REQUESTS_MAX];
static modbus_request_result_t  s_request_results[MODBUS_REQUESTS_MAX];

/* Client configuration (loaded from flash at startup) */
static modbus_tcp_client_config_t s_client_config;

// ============================================================================
// Transport callbacks
// ============================================================================

/**
 * @brief Transport read callback for nanoMODBUS client (TCP)
 *
 * Polls the WizNet receive buffer until @p count bytes are available or
 * @p byte_timeout_ms expires.
 *
 * @param buf              Output buffer
 * @param count            Number of bytes to read
 * @param byte_timeout_ms  Timeout in ms
 * @param arg              User argument (unused)
 * @return Number of bytes read, or 0 on timeout
 */
static int32_t modbus_tcp_client_read_socket(uint8_t *buf, uint16_t count,
                                              int32_t byte_timeout_ms, void *arg)
{
    (void)arg;

    if (buf == NULL)
    {
        return -1;
    }

    uint32_t start = xTaskGetTickCount();

    while (getSn_RX_RSR(s_socket_num) < count)
    {
        if ((xTaskGetTickCount() - start) >= (uint32_t)byte_timeout_ms)
        {
            return 0;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return recv(s_socket_num, buf, count);
}

/**
 * @brief Transport write callback for nanoMODBUS client (TCP)
 *
 * @param buf              Data to write
 * @param count            Number of bytes to write
 * @param byte_timeout_ms  Unused
 * @param arg              User argument (unused)
 * @return Number of bytes written, or negative on error
 */
static int32_t modbus_tcp_client_write_socket(const uint8_t *buf, uint16_t count,
                                               int32_t byte_timeout_ms, void *arg)
{
    (void)byte_timeout_ms;
    (void)arg;

    if (buf == NULL)
    {
        return -1;
    }

    return send(s_socket_num, (uint8_t *)buf, count);
}

// ============================================================================
// Helpers
// ============================================================================

/**
 * @brief Load requests configuration from flash into static arrays
 */
static bool load_requests(void)
{
    memcpy(s_request_configs, s_client_config.requests,
           sizeof(modbus_request_config_t) * MODBUS_REQUESTS_MAX);
    memset(s_request_results, 0, sizeof(s_request_results));

    uint8_t enabled_count = 0;
    for (uint8_t i = 0; i < MODBUS_REQUESTS_MAX; i++)
    {
        if (s_request_configs[i].enabled)
        {
            enabled_count++;
            LOG_INFO("TCP client request %u: slave=%u, type=%u, op=%u, addr=%u, count=%u",
                     i,
                     s_request_configs[i].slave_address,
                     s_request_configs[i].data_type,
                     s_request_configs[i].operation,
                     s_request_configs[i].start_address,
                     s_request_configs[i].count);
        }
    }

    LOG_INFO("Modbus TCP client: loaded %u enabled requests", enabled_count);
    return true;
}

/**
 * @brief Mark all mapped tags BAD on disconnect
 */
static void mark_all_tags_bad(void)
{
    for (uint8_t i = 0; i < MODBUS_REQUESTS_MAX; i++)
    {
        if (s_request_configs[i].enabled)
        {
            modbus_mark_mapped_tags_bad(&s_request_configs[i]);
        }
    }
}

static bool wait_for_network_ready(uint32_t timeout_ms)
{
    uint32_t start_time = xTaskGetTickCount();

    LOG_INFO("Modbus TCP client: waiting for network...");

    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(timeout_ms))
    {
        uint32_t notification_value = 0;

        if (xTaskNotifyWait(0, UINT32_MAX, &notification_value, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            if (notification_value & NETWORK_NOTIFY_READY)
            {
                return true;
            }
        }
    }

    return false;
}

// ============================================================================
// FreeRTOS task
// ============================================================================

/**
 * @brief Modbus TCP client FreeRTOS task
 *
 * Waits for network readiness, then connects to the remote Modbus TCP server
 * and polls all enabled request slots each cycle. On disconnect, all mapped
 * tags are marked BAD and reconnection is attempted after a delay.
 */
static void vModbusTcpClientTask(void *pvParameters)
{
    (void)pvParameters;

    LOG_INFO("Modbus TCP client task started");

    if (!config_get_modbus_tcp_client_config(&s_client_config))
    {
        LOG_ERROR("Modbus TCP client: failed to get client config - task will exit");
        vTaskDelete(NULL);
        return;
    }

    if (!load_requests())
    {
        LOG_ERROR("Modbus TCP client: failed to load requests - task will exit");
        vTaskDelete(NULL);
        return;
    }

    LOG_INFO("Modbus TCP client: target %u.%u.%u.%u:%u",
             s_client_config.remote_ip[0], s_client_config.remote_ip[1],
             s_client_config.remote_ip[2], s_client_config.remote_ip[3],
             s_client_config.remote_port);

    /* Configure nanoMODBUS platform */
    nmbs_platform_conf platform_conf;
    nmbs_platform_conf_create(&platform_conf);
    platform_conf.transport = NMBS_TRANSPORT_TCP;
    platform_conf.read      = modbus_tcp_client_read_socket;
    platform_conf.write     = modbus_tcp_client_write_socket;

    nmbs_t nmbs;
    nmbs_error err = nmbs_client_create(&nmbs, &platform_conf);
    if (err != NMBS_ERROR_NONE)
    {
        LOG_ERROR("Modbus TCP client: failed to create client: %d - task will exit", err);
        vTaskDelete(NULL);
        return;
    }

    nmbs_set_read_timeout(&nmbs, MODBUS_TCP_CLIENT_READ_TIMEOUT_MS);
    nmbs_set_byte_timeout(&nmbs, MODBUS_TCP_CLIENT_BYTE_TIMEOUT_MS);

    /* Register for network status notifications */
    network_task_register_notification(NULL);

    bool network_ready = false;
    bool connected = false;

    while (1)
    {
        /* Outer loop: wait for network then operate */
        if (!network_ready)
        {
            if (!wait_for_network_ready(MODBUS_TCP_CLIENT_NET_TIMEOUT_MS))
            {
                LOG_WARN("Modbus TCP client: network not ready after timeout, retrying");
                continue;
            }
            network_ready = true;
        }

        /* Inner loop: socket state machine */
        while (network_ready)
        {
            /* Check for network status change (non-blocking) */
            uint32_t notification_value = 0;
            if (xTaskNotifyWait(0, UINT32_MAX, &notification_value, 0) == pdTRUE)
            {
                if (notification_value & NETWORK_NOTIFY_NOT_READY)
                {
                    LOG_WARN("Modbus TCP client: network lost");
                    close(s_socket_num);
                    if (connected)
                    {
                        mark_all_tags_bad();
                        connected = false;
                    }
                    network_ready = false;
                    break;
                }

                if (notification_value & NETWORK_NOTIFY_IP_CHANGED)
                {
                    LOG_INFO("Modbus TCP client: IP changed, reinitializing");
                    close(s_socket_num);
                    if (connected)
                    {
                        mark_all_tags_bad();
                        connected = false;
                    }
                    network_ready = false;
                    break;
                }
            }

            switch (getSn_SR(s_socket_num))
            {
                case SOCK_ESTABLISHED:
                    if (!connected)
                    {
                        LOG_INFO("Modbus TCP client: connected to %u.%u.%u.%u:%u",
                                 s_client_config.remote_ip[0], s_client_config.remote_ip[1],
                                 s_client_config.remote_ip[2], s_client_config.remote_ip[3],
                                 s_client_config.remote_port);
                        connected = true;
                    }

                    /* Poll all enabled requests */
                    for (uint8_t i = 0; i < MODBUS_REQUESTS_MAX; i++)
                    {
                        if (s_request_configs[i].enabled)
                        {
                            modbus_request_process(&nmbs, &s_request_configs[i], &s_request_results[i]);

                            if (s_request_results[i].last_error == NMBS_ERROR_NONE)
                            {
                                if (s_request_configs[i].data_type == MODBUS_DATA_TYPE_HOLDING_REGISTER ||
                                    s_request_configs[i].data_type == MODBUS_DATA_TYPE_INPUT_REGISTER)
                                {
                                    LOG_DEBUG("TCP client request[%u]: slave=%u addr=%u %s regs=[%u, %u]",
                                             i, s_request_configs[i].slave_address,
                                             s_request_configs[i].start_address,
                                             s_request_configs[i].operation == MODBUS_OP_READ ? "read" : "write",
                                             s_request_results[i].data.registers[0],
                                             s_request_configs[i].count > 1 ? s_request_results[i].data.registers[1] : 0);
                                }
                                else
                                {
                                    LOG_DEBUG("TCP client request[%u]: slave=%u addr=%u %s coils=[%u, %u, %u]",
                                             i, s_request_configs[i].slave_address,
                                             s_request_configs[i].start_address,
                                             s_request_configs[i].operation == MODBUS_OP_READ ? "read" : "write",
                                             s_request_results[i].data.coils[0],
                                             s_request_configs[i].count > 1 ? s_request_results[i].data.coils[1] : 0,
                                             s_request_configs[i].count > 2 ? s_request_results[i].data.coils[2] : 0);
                                }
                            }
                            else if (s_request_results[i].last_error != NMBS_ERROR_NONE)
                            {
                                LOG_ERROR("TCP client request[%u]: slave=%u addr=%u error=%d",
                                         i, s_request_configs[i].slave_address,
                                         s_request_configs[i].start_address,
                                         s_request_results[i].last_error);
                            }
                        }
                    }

                    vTaskDelay(pdMS_TO_TICKS(MODBUS_TCP_CLIENT_LOOP_DELAY_MS));
                    break;

                case SOCK_CLOSE_WAIT:
                    LOG_INFO("Modbus TCP client: server disconnecting");
                    disconnect(s_socket_num);
                    if (connected)
                    {
                        mark_all_tags_bad();
                        connected = false;
                    }
                    vTaskDelay(pdMS_TO_TICKS(MODBUS_TCP_CLIENT_RETRY_DELAY_MS));
                    break;

                case SOCK_INIT:
                    if (connect(s_socket_num,
                                (uint8_t *)s_client_config.remote_ip,
                                s_client_config.remote_port) != SOCK_OK)
                    {
                        LOG_WARN("Modbus TCP client: connect() failed, retrying in %u ms",
                                 MODBUS_TCP_CLIENT_RETRY_DELAY_MS);
                        close(s_socket_num);
                        vTaskDelay(pdMS_TO_TICKS(MODBUS_TCP_CLIENT_RETRY_DELAY_MS));
                    }
                    break;

                case SOCK_CLOSED:
                    /* Use an ephemeral local port; wrap around at 0xFFFE */
                    if (socket(s_socket_num, Sn_MR_TCP, s_local_port, 0x00) != s_socket_num)
                    {
                        LOG_ERROR("Modbus TCP client: socket() failed, retrying");
                        vTaskDelay(pdMS_TO_TICKS(MODBUS_TCP_CLIENT_RETRY_DELAY_MS));
                    }
                    else
                    {
                        if (++s_local_port >= 0xFFFE)
                        {
                            s_local_port = MODBUS_TCP_CLIENT_LOCAL_PORT_BASE;
                        }
                    }
                    break;

                default:
                    vTaskDelay(pdMS_TO_TICKS(10));
                    break;
            }
        }
    }
}

/**
 * @brief Initialize and start the Modbus TCP client task
 */
bool modbus_tcp_client_task_init(void)
{
    modbus_tcp_client_config_t client_config;
    if (!config_get_modbus_tcp_client_config(&client_config))
    {
        LOG_ERROR("Modbus TCP client: failed to get client configuration");
        return false;
    }

    if (!client_config.enable)
    {
        LOG_INFO("Modbus TCP client disabled in configuration");
        return true;
    }

    BaseType_t result = xTaskCreate(vModbusTcpClientTask,
                                    "ModbusTcpCli",
                                    MODBUS_TCP_CLIENT_TASK_STACK_SIZE,
                                    NULL,
                                    MODBUS_TCP_CLIENT_TASK_PRIORITY,
                                    NULL);

    if (result != pdPASS)
    {
        LOG_ERROR("Modbus TCP client: failed to create task");
        return false;
    }

    return true;
}
