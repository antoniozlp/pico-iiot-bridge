/**
 * @file modbus_tcp_server.c
 * @brief Modbus TCP server task implementation (nanoMODBUS library)
 *
 * FreeRTOS task that runs a Modbus TCP server. External clients can read and
 * write any Tag Database tag that has been mapped into the server memory map
 * via modbus_tcp_server_config_t memory blocks. Configuration is read from
 * system_config at startup.
 *
 * Address lookup:
 *   The server config holds up to MODBUS_SERVER_MEMORY_BLOCKS_MAX memory blocks.
 *   Each memory block covers a contiguous Modbus address range
 *   [start_address, start_address + count). An incoming request for address
 *   range [A, A+N) must fall entirely within one memory block; requests spanning
 *   multiple memory blocks return ILLEGAL_DATA_ADDRESS.
 *
 * Socket state machine:
 *   SOCK_CLOSED  → socket() → SOCK_INIT → listen() → SOCK_LISTEN → ...
 *   → SOCK_ESTABLISHED → nmbs_server_poll() → (client disconnects)
 *   → SOCK_CLOSE_WAIT  → disconnect() → SOCK_CLOSED (cycle)
 */

#include "modbus_tcp_server.h"

#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "wizchip_conf.h"
#include "socket.h"

#include "logger.h"
#include "modbus_request.h"
#include "modbus_tag_mapping.h"
#include "nanomodbus.h"
#include "network_task.h"
#include "system_config.h"

/* Task configuration */
#define MODBUS_TCP_SERVER_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE * 4)  /* 2KB for nanoMODBUS buffers */
#define MODBUS_TCP_SERVER_TASK_PRIORITY     (tskIDLE_PRIORITY + 2)

/* WizNet socket assigned to the Modbus TCP server (sockets 0-1: HTTP, 2: DHCP, 3: S2TCP) */
#define MODBUS_TCP_SERVER_SOCKET_NUM        4

/* nanoMODBUS timeouts (ms) */
#define MODBUS_TCP_SERVER_READ_TIMEOUT_MS   100  /* Short: yields CPU while waiting for a request */
#define MODBUS_TCP_SERVER_BYTE_TIMEOUT_MS   1000 /* Per-byte inter-frame gap tolerance */

/* Network readiness timeout */
#define MODBUS_TCP_SERVER_NET_TIMEOUT_MS    30000

/* Delay between socket state machine iterations when idle */
#define MODBUS_TCP_SERVER_IDLE_DELAY_MS     10

/* Socket number used by transport callbacks (set before the task runs) */
static uint8_t s_socket_num = MODBUS_TCP_SERVER_SOCKET_NUM;

/* Server configuration (loaded from flash at task startup) */
static modbus_tcp_server_config_t s_server_config;

// ============================================================================
// Transport callbacks
// ============================================================================

/**
 * @brief Transport read callback for nanoMODBUS server (TCP)
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
static int32_t modbus_tcp_server_read_socket(uint8_t *buf, uint16_t count,
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
 * @brief Transport write callback for nanoMODBUS server (TCP)
 *
 * @param buf              Data to write
 * @param count            Number of bytes to write
 * @param byte_timeout_ms  Unused
 * @param arg              User argument (unused)
 * @return Number of bytes written, or negative on error
 */
static int32_t modbus_tcp_server_write_socket(const uint8_t *buf, uint16_t count,
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
// Address lookup helper
// ============================================================================

/**
 * @brief Find a configured memory block covering the requested address range
 *
 * Scans all enabled memory blocks of the given type looking for one whose address
 * range [start_address, start_address + count) fully contains the requested
 * range [address, address + quantity).
 *
 * @param cfg       Pointer to server configuration
 * @param type      Required data type (must match exactly)
 * @param address   First Modbus address of the request
 * @param quantity  Number of registers/coils requested
 * @return Index into cfg->memory_blocks, or -1 if no match found
 */
static int find_server_memory_block(const modbus_tcp_server_config_t *cfg,
                                     modbus_data_type_t type,
                                     uint16_t address, uint16_t quantity)
{
    for (int i = 0; i < MODBUS_SERVER_MEMORY_BLOCKS_MAX; i++)
    {
        const modbus_server_memory_block_t *block = &cfg->memory_blocks[i];

        if (!block->enabled || block->data_type != type)
        {
            continue;
        }

        /* Use 32-bit arithmetic to avoid uint16_t overflow */
        uint32_t block_end = (uint32_t)block->start_address + block->count;
        uint32_t req_end   = (uint32_t)address + quantity;

        if ((uint32_t)address >= block->start_address && req_end <= block_end)
        {
            return i;
        }
    }
    return -1;
}

// ============================================================================
// nanoMODBUS server callbacks
// ============================================================================

/**
 * @brief Handle FC01 – Read Coils
 */
static nmbs_error tcp_server_handle_read_coils(uint16_t address, uint16_t quantity,
                                                nmbs_bitfield coils_out,
                                                uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_tcp_server_config_t *cfg = (const modbus_tcp_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_COIL, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("TCP Server FC01: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];
    uint16_t offset = (uint16_t)(address - block->start_address);

    memset(coils_out, 0, (quantity + 7) / 8);
    modbus_tags_to_coils(block->tag_handles, offset, quantity, coils_out);

    LOG_DEBUG("TCP Server FC01: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC02 – Read Discrete Inputs
 */
static nmbs_error tcp_server_handle_read_discrete_inputs(uint16_t address, uint16_t quantity,
                                                          nmbs_bitfield inputs_out,
                                                          uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_tcp_server_config_t *cfg = (const modbus_tcp_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_DISCRETE_INPUT, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("TCP Server FC02: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];
    uint16_t offset = (uint16_t)(address - block->start_address);

    memset(inputs_out, 0, (quantity + 7) / 8);
    modbus_tags_to_coils(block->tag_handles, offset, quantity, inputs_out);

    LOG_DEBUG("TCP Server FC02: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC03 – Read Holding Registers
 */
static nmbs_error tcp_server_handle_read_holding_registers(uint16_t address, uint16_t quantity,
                                                             uint16_t *registers_out,
                                                             uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_tcp_server_config_t *cfg = (const modbus_tcp_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_HOLDING_REGISTER, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("TCP Server FC03: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];
    uint16_t offset = (uint16_t)(address - block->start_address);

    modbus_tags_to_registers(block->tag_handles, offset, quantity, block->encoding, registers_out);

    LOG_DEBUG("TCP Server FC03: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC04 – Read Input Registers
 */
static nmbs_error tcp_server_handle_read_input_registers(uint16_t address, uint16_t quantity,
                                                           uint16_t *registers_out,
                                                           uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_tcp_server_config_t *cfg = (const modbus_tcp_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_INPUT_REGISTER, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("TCP Server FC04: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];
    uint16_t offset = (uint16_t)(address - block->start_address);

    modbus_tags_to_registers(block->tag_handles, offset, quantity, block->encoding, registers_out);

    LOG_DEBUG("TCP Server FC04: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC05 – Write Single Coil
 */
static nmbs_error tcp_server_handle_write_single_coil(uint16_t address, bool value,
                                                        uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_tcp_server_config_t *cfg = (const modbus_tcp_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_COIL, address, 1);
    if (block_idx < 0)
    {
        LOG_DEBUG("TCP Server FC05: no memory block for addr=%u", address);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];

    if (!block->writable)
    {
        LOG_DEBUG("TCP Server FC05: memory block %d not writable", block_idx);
        return NMBS_EXCEPTION_ILLEGAL_FUNCTION;
    }

    uint16_t offset = (uint16_t)(address - block->start_address);

    /* Pack the single coil value into a bitfield and call the bulk helper */
    nmbs_bitfield coil_buf = {0};
    nmbs_bitfield_write(coil_buf, 0, value ? 1u : 0u);
    modbus_coils_to_tags(block->tag_handles, offset, 1, coil_buf);

    LOG_DEBUG("TCP Server FC05: addr=%u val=%d block=%d offset=%u", address, (int)value, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC15 – Write Multiple Coils
 */
static nmbs_error tcp_server_handle_write_multiple_coils(uint16_t address, uint16_t quantity,
                                                           const nmbs_bitfield coils,
                                                           uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_tcp_server_config_t *cfg = (const modbus_tcp_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_COIL, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("TCP Server FC15: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];

    if (!block->writable)
    {
        LOG_DEBUG("TCP Server FC15: memory block %d not writable", block_idx);
        return NMBS_EXCEPTION_ILLEGAL_FUNCTION;
    }

    uint16_t offset = (uint16_t)(address - block->start_address);
    modbus_coils_to_tags(block->tag_handles, offset, quantity, coils);

    LOG_DEBUG("TCP Server FC15: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC16 – Write Multiple Registers
 */
static nmbs_error tcp_server_handle_write_multiple_registers(uint16_t address, uint16_t quantity,
                                                               const uint16_t *registers,
                                                               uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_tcp_server_config_t *cfg = (const modbus_tcp_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_HOLDING_REGISTER, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("TCP Server FC16: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];

    if (!block->writable)
    {
        LOG_DEBUG("TCP Server FC16: memory block %d not writable", block_idx);
        return NMBS_EXCEPTION_ILLEGAL_FUNCTION;
    }

    uint16_t offset = (uint16_t)(address - block->start_address);
    modbus_registers_to_tags(block->tag_handles, offset, quantity, block->encoding, registers);

    LOG_DEBUG("TCP Server FC16: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

// ============================================================================
// Network readiness helper
// ============================================================================

static bool wait_for_network_ready(uint32_t timeout_ms)
{
    uint32_t start_time = xTaskGetTickCount();

    LOG_INFO("Modbus TCP server: waiting for network...");

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
 * @brief Modbus TCP server FreeRTOS task
 *
 * Waits for network readiness, then opens a WizNet TCP socket and enters the
 * socket state machine. When a client connects, nanoMODBUS polls are run to
 * serve Modbus requests. On disconnect the socket is recycled to accept the
 * next client. On fatal init failure the task deletes itself.
 */
static void vModbusTcpServerTask(void *pvParameters)
{
    (void)pvParameters;

    LOG_INFO("Modbus TCP server task started");

    if (!config_get_modbus_tcp_server_config(&s_server_config))
    {
        LOG_ERROR("Modbus TCP server: failed to get server config - task will exit");
        vTaskDelete(NULL);
        return;
    }

    /* Log enabled memory blocks */
    uint8_t enabled_blocks = 0;
    for (uint8_t i = 0; i < MODBUS_SERVER_MEMORY_BLOCKS_MAX; i++)
    {
        if (s_server_config.memory_blocks[i].enabled)
        {
            enabled_blocks++;
            LOG_INFO("TCP server memory block %u: type=%u, addr=%u, count=%u, writable=%u",
                     i,
                     s_server_config.memory_blocks[i].data_type,
                     s_server_config.memory_blocks[i].start_address,
                     s_server_config.memory_blocks[i].count,
                     s_server_config.memory_blocks[i].writable);
        }
    }
    LOG_INFO("Modbus TCP server: %u enabled memory blocks, port=%u, unit_id=%u",
             enabled_blocks, s_server_config.port, s_server_config.server_address);

    /* Configure nanoMODBUS platform */
    nmbs_platform_conf platform_conf;
    nmbs_platform_conf_create(&platform_conf);
    platform_conf.transport = NMBS_TRANSPORT_TCP;
    platform_conf.read      = modbus_tcp_server_read_socket;
    platform_conf.write     = modbus_tcp_server_write_socket;

    /* Register FC callbacks */
    nmbs_callbacks callbacks;
    nmbs_callbacks_create(&callbacks);
    callbacks.read_coils                = tcp_server_handle_read_coils;
    callbacks.read_discrete_inputs      = tcp_server_handle_read_discrete_inputs;
    callbacks.read_holding_registers    = tcp_server_handle_read_holding_registers;
    callbacks.read_input_registers      = tcp_server_handle_read_input_registers;
    callbacks.write_single_coil         = tcp_server_handle_write_single_coil;
    callbacks.write_multiple_coils      = tcp_server_handle_write_multiple_coils;
    callbacks.write_multiple_registers  = tcp_server_handle_write_multiple_registers;

    nmbs_t nmbs;
    nmbs_error err = nmbs_server_create(&nmbs, s_server_config.server_address,
                                         &platform_conf, &callbacks);
    if (err != NMBS_ERROR_NONE)
    {
        LOG_ERROR("Modbus TCP server: failed to create server: %d - task will exit", err);
        vTaskDelete(NULL);
        return;
    }

    nmbs_set_read_timeout(&nmbs, MODBUS_TCP_SERVER_READ_TIMEOUT_MS);
    nmbs_set_byte_timeout(&nmbs, MODBUS_TCP_SERVER_BYTE_TIMEOUT_MS);
    nmbs_set_callbacks_arg(&nmbs, &s_server_config);

    /* Register for network status notifications */
    network_task_register_notification(NULL);

    bool network_ready = false;

    while (1)
    {
        /* Outer loop: wait for network then operate */
        if (!network_ready)
        {
            if (!wait_for_network_ready(MODBUS_TCP_SERVER_NET_TIMEOUT_MS))
            {
                LOG_WARN("Modbus TCP server: network not ready after timeout, retrying");
                continue;
            }
            network_ready = true;
            LOG_INFO("Modbus TCP server: network ready, listening on port %u", s_server_config.port);
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
                    LOG_WARN("Modbus TCP server: network lost, closing socket");
                    close(s_socket_num);
                    network_ready = false;
                    break;
                }

                if (notification_value & NETWORK_NOTIFY_IP_CHANGED)
                {
                    LOG_INFO("Modbus TCP server: IP changed, reinitializing socket");
                    close(s_socket_num);
                    network_ready = false;
                    break;
                }
            }

            switch (getSn_SR(s_socket_num))
            {
                case SOCK_ESTABLISHED:
                    err = nmbs_server_poll(&nmbs);
                    if (err == NMBS_ERROR_NONE)
                    {
                        /* Request handled — continue immediately */
                    }
                    else if (err == NMBS_ERROR_TIMEOUT)
                    {
                        vTaskDelay(pdMS_TO_TICKS(1));
                    }
                    else
                    {
                        LOG_DEBUG("Modbus TCP server: poll error %d, client may have disconnected", err);
                        disconnect(s_socket_num);
                    }
                    break;

                case SOCK_CLOSE_WAIT:
                    LOG_INFO("Modbus TCP server: client disconnecting");
                    disconnect(s_socket_num);
                    break;

                case SOCK_INIT:
                    if (listen(s_socket_num) != SOCK_OK)
                    {
                        LOG_ERROR("Modbus TCP server: listen() failed");
                        close(s_socket_num);
                    }
                    break;

                case SOCK_CLOSED:
                    if (socket(s_socket_num, Sn_MR_TCP, s_server_config.port, 0x00) != s_socket_num)
                    {
                        LOG_ERROR("Modbus TCP server: socket() failed, retrying");
                        vTaskDelay(pdMS_TO_TICKS(1000));
                    }
                    break;

                default:
                    vTaskDelay(pdMS_TO_TICKS(MODBUS_TCP_SERVER_IDLE_DELAY_MS));
                    break;
            }
        }
    }
}

/**
 * @brief Initialize and start the Modbus TCP server task
 */
bool modbus_tcp_server_task_init(void)
{
    modbus_tcp_server_config_t server_config;
    if (!config_get_modbus_tcp_server_config(&server_config))
    {
        LOG_ERROR("Modbus TCP server: failed to get server configuration");
        return false;
    }

    if (!server_config.enable)
    {
        LOG_INFO("Modbus TCP server disabled in configuration");
        return true;
    }

    BaseType_t result = xTaskCreate(vModbusTcpServerTask,
                                    "ModbusTcpSrv",
                                    MODBUS_TCP_SERVER_TASK_STACK_SIZE,
                                    NULL,
                                    MODBUS_TCP_SERVER_TASK_PRIORITY,
                                    NULL);

    if (result != pdPASS)
    {
        LOG_ERROR("Modbus TCP server: failed to create task");
        return false;
    }

    return true;
}
