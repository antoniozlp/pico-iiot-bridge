/**
 * @file modbus_rtu_slave.c
 * @brief Modbus RTU slave (server) task implementation (nanoMODBUS library)
 *
 * FreeRTOS task that runs a Modbus RTU slave. External masters can read and
 * write any Tag Database tag that has been mapped into the server memory map
 * via modbus_rtu_server_config_t memory blocks. Configuration is read from
 * system_config at startup.
 *
 * Address lookup:
 *   The server config holds up to MODBUS_SERVER_MEMORY_BLOCKS_MAX memory blocks.
 *   Each memory block covers a contiguous Modbus address range
 *   [start_address, start_address + count). An incoming request for address
 *   range [A, A+N) must fall entirely within one memory block; requests spanning
 *   multiple memory blocks return ILLEGAL_DATA_ADDRESS.
 */

#include "modbus_rtu_slave.h"

#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "hardware/timer.h"
#include "hardware/uart.h"

#include "board_config.h"
#include "logger.h"
#include "modbus_request.h"
#include "modbus_tag_mapping.h"
#include "nanomodbus.h"
#include "system_config.h"

/* Task configuration */
#define MODBUS_RTU_SLAVE_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE * 4)  /* 2KB for nanoMODBUS buffers */
#define MODBUS_RTU_SLAVE_TASK_PRIORITY      (tskIDLE_PRIORITY + 2)

/* nanoMODBUS timeouts (ms) */
#define MODBUS_RTU_SLAVE_READ_TIMEOUT_MS    50   /* Short: yields CPU while bus is idle */
#define MODBUS_RTU_SLAVE_BYTE_TIMEOUT_MS    100  /* Inter-byte gap tolerance */

/* Enable flag value */
#define MODBUS_RTU_SLAVE_DISABLED           0

/* UART instance used by transport callbacks (set before task runs) */
static uart_inst_t *s_slave_uart;

/* Server configuration (loaded from flash at task startup) */
static modbus_rtu_server_config_t s_server_config;

// ============================================================================
// Transport callbacks
// ============================================================================

/**
 * @brief Transport read callback for nanoMODBUS server
 *
 * @param buf              Output buffer (must not be NULL)
 * @param count            Max bytes to read
 * @param byte_timeout_ms  Per-byte timeout in ms
 * @param arg              User argument (unused)
 * @return Number of bytes read, or negative on error
 */
static int32_t modbus_rtu_slave_read_serial(uint8_t *buf, uint16_t count,
                                             int32_t byte_timeout_ms, void *arg)
{
    (void)arg;

    if (buf == NULL || s_slave_uart == NULL)
    {
        return -1;
    }

    uint64_t start_time = time_us_64();
    int32_t bytes_read = 0;
    uint64_t timeout_us = (uint64_t)byte_timeout_ms * 1000;

    while ((time_us_64() - start_time) < timeout_us && bytes_read < count)
    {
        if (uart_is_readable(s_slave_uart))
        {
            buf[bytes_read++] = uart_getc(s_slave_uart);
            start_time = time_us_64();
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    return bytes_read;
}

/**
 * @brief Transport write callback for nanoMODBUS server
 *
 * @param buf              Data to write (must not be NULL)
 * @param count            Number of bytes to write
 * @param byte_timeout_ms  Unused
 * @param arg              User argument (unused)
 * @return Number of bytes written, or negative on error
 */
static int32_t modbus_rtu_slave_write_serial(const uint8_t *buf, uint16_t count,
                                              int32_t byte_timeout_ms, void *arg)
{
    (void)byte_timeout_ms;
    (void)arg;

    if (buf == NULL || s_slave_uart == NULL)
    {
        return -1;
    }

    uart_write_blocking(s_slave_uart, buf, count);
    return (int32_t)count;
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
static int find_server_memory_block(const modbus_rtu_server_config_t *cfg,
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
static nmbs_error slave_handle_read_coils(uint16_t address, uint16_t quantity,
                                           nmbs_bitfield coils_out,
                                           uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_rtu_server_config_t *cfg = (const modbus_rtu_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_COIL, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("Slave FC01: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];
    uint16_t offset = (uint16_t)(address - block->start_address);

    memset(coils_out, 0, (quantity + 7) / 8);
    modbus_tags_to_coils(block->tag_handles, offset, quantity, coils_out);

    LOG_DEBUG("Slave FC01: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC02 – Read Discrete Inputs
 */
static nmbs_error slave_handle_read_discrete_inputs(uint16_t address, uint16_t quantity,
                                                     nmbs_bitfield inputs_out,
                                                     uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_rtu_server_config_t *cfg = (const modbus_rtu_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_DISCRETE_INPUT, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("Slave FC02: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];
    uint16_t offset = (uint16_t)(address - block->start_address);

    memset(inputs_out, 0, (quantity + 7) / 8);
    modbus_tags_to_coils(block->tag_handles, offset, quantity, inputs_out);

    LOG_DEBUG("Slave FC02: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC03 – Read Holding Registers
 */
static nmbs_error slave_handle_read_holding_registers(uint16_t address, uint16_t quantity,
                                                       uint16_t *registers_out,
                                                       uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_rtu_server_config_t *cfg = (const modbus_rtu_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_HOLDING_REGISTER, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("Slave FC03: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];
    uint16_t offset = (uint16_t)(address - block->start_address);

    modbus_tags_to_registers(block->tag_handles, offset, quantity, block->encoding, registers_out);

    LOG_DEBUG("Slave FC03: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC04 – Read Input Registers
 */
static nmbs_error slave_handle_read_input_registers(uint16_t address, uint16_t quantity,
                                                     uint16_t *registers_out,
                                                     uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_rtu_server_config_t *cfg = (const modbus_rtu_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_INPUT_REGISTER, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("Slave FC04: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];
    uint16_t offset = (uint16_t)(address - block->start_address);

    modbus_tags_to_registers(block->tag_handles, offset, quantity, block->encoding, registers_out);

    LOG_DEBUG("Slave FC04: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC05 – Write Single Coil
 */
static nmbs_error slave_handle_write_single_coil(uint16_t address, bool value,
                                                   uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_rtu_server_config_t *cfg = (const modbus_rtu_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_COIL, address, 1);
    if (block_idx < 0)
    {
        LOG_DEBUG("Slave FC05: no memory block for addr=%u", address);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];

    if (!block->writable)
    {
        LOG_DEBUG("Slave FC05: memory block %d not writable", block_idx);
        return NMBS_EXCEPTION_ILLEGAL_FUNCTION;
    }

    uint16_t offset = (uint16_t)(address - block->start_address);

    /* Pack the single coil value into a bitfield and call the bulk helper */
    nmbs_bitfield coil_buf = {0};
    nmbs_bitfield_write(coil_buf, 0, value ? 1u : 0u);
    modbus_coils_to_tags(block->tag_handles, offset, 1, coil_buf);

    LOG_DEBUG("Slave FC05: addr=%u val=%d block=%d offset=%u", address, (int)value, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC15 – Write Multiple Coils
 */
static nmbs_error slave_handle_write_multiple_coils(uint16_t address, uint16_t quantity,
                                                     const nmbs_bitfield coils,
                                                     uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_rtu_server_config_t *cfg = (const modbus_rtu_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_COIL, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("Slave FC15: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];

    if (!block->writable)
    {
        LOG_DEBUG("Slave FC15: memory block %d not writable", block_idx);
        return NMBS_EXCEPTION_ILLEGAL_FUNCTION;
    }

    uint16_t offset = (uint16_t)(address - block->start_address);
    modbus_coils_to_tags(block->tag_handles, offset, quantity, coils);

    LOG_DEBUG("Slave FC15: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

/**
 * @brief Handle FC16 – Write Multiple Registers
 */
static nmbs_error slave_handle_write_multiple_registers(uint16_t address, uint16_t quantity,
                                                         const uint16_t *registers,
                                                         uint8_t unit_id, void *arg)
{
    (void)unit_id;
    const modbus_rtu_server_config_t *cfg = (const modbus_rtu_server_config_t *)arg;

    int block_idx = find_server_memory_block(cfg, MODBUS_DATA_TYPE_HOLDING_REGISTER, address, quantity);
    if (block_idx < 0)
    {
        LOG_DEBUG("Slave FC16: no memory block for addr=%u qty=%u", address, quantity);
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    const modbus_server_memory_block_t *block = &cfg->memory_blocks[block_idx];

    if (!block->writable)
    {
        LOG_DEBUG("Slave FC16: memory block %d not writable", block_idx);
        return NMBS_EXCEPTION_ILLEGAL_FUNCTION;
    }

    uint16_t offset = (uint16_t)(address - block->start_address);
    modbus_registers_to_tags(block->tag_handles, offset, quantity, block->encoding, registers);

    LOG_DEBUG("Slave FC16: addr=%u qty=%u block=%d offset=%u", address, quantity, block_idx, offset);
    return NMBS_ERROR_NONE;
}

// ============================================================================
// FreeRTOS task
// ============================================================================

/**
 * @brief Modbus RTU slave FreeRTOS task
 *
 * Loads config, initializes UART, creates nanoMODBUS server instance, then
 * polls for incoming requests. On fatal init failure the task deletes itself.
 */
static void vModbusRtuSlaveTask(void *pvParameters)
{
    (void)pvParameters;

    LOG_INFO("Modbus RTU slave task started");

    if (!config_get_modbus_rtu_server_config(&s_server_config))
    {
        LOG_ERROR("Modbus RTU slave: failed to get server config - task will exit");
        vTaskDelete(NULL);
        return;
    }

    serial_config_t serial_config;
    if (!config_get_serial_config(s_server_config.serial_id, &serial_config))
    {
        LOG_ERROR("Modbus RTU slave: failed to get serial config - task will exit");
        vTaskDelete(NULL);
        return;
    }

    s_slave_uart = (s_server_config.serial_id == 0) ? BOARD_UART0_ID : BOARD_UART1_ID;

    if (!board_init_uart(s_slave_uart, &serial_config))
    {
        LOG_ERROR("Modbus RTU slave: failed to initialize UART - task will exit");
        vTaskDelete(NULL);
        return;
    }

    LOG_INFO("Modbus RTU slave UART%u initialized: %u baud, %uN%u, address=%u",
             s_server_config.serial_id,
             serial_config.baud,
             serial_config.databits,
             serial_config.stopbits,
             s_server_config.server_address);

    /* Count and log enabled memory blocks */
    uint8_t enabled_blocks = 0;
    for (uint8_t i = 0; i < MODBUS_SERVER_MEMORY_BLOCKS_MAX; i++)
    {
        if (s_server_config.memory_blocks[i].enabled)
        {
            enabled_blocks++;
            LOG_INFO("Server memory block %u: type=%u, addr=%u, count=%u, writable=%u",
                     i,
                     s_server_config.memory_blocks[i].data_type,
                     s_server_config.memory_blocks[i].start_address,
                     s_server_config.memory_blocks[i].count,
                     s_server_config.memory_blocks[i].writable);
        }
    }
    LOG_INFO("Modbus RTU slave: %u enabled memory blocks", enabled_blocks);

    /* Configure nanoMODBUS platform */
    nmbs_platform_conf platform_conf;
    nmbs_platform_conf_create(&platform_conf);
    platform_conf.transport = NMBS_TRANSPORT_RTU;
    platform_conf.read  = modbus_rtu_slave_read_serial;
    platform_conf.write = modbus_rtu_slave_write_serial;

    /* Register callbacks */
    nmbs_callbacks callbacks;
    nmbs_callbacks_create(&callbacks);
    callbacks.read_coils                  = slave_handle_read_coils;
    callbacks.read_discrete_inputs        = slave_handle_read_discrete_inputs;
    callbacks.read_holding_registers      = slave_handle_read_holding_registers;
    callbacks.read_input_registers        = slave_handle_read_input_registers;
    callbacks.write_single_coil           = slave_handle_write_single_coil;
    callbacks.write_multiple_coils        = slave_handle_write_multiple_coils;
    callbacks.write_multiple_registers    = slave_handle_write_multiple_registers;

    nmbs_t nmbs;
    nmbs_error err = nmbs_server_create(&nmbs, s_server_config.server_address,
                                         &platform_conf, &callbacks);
    if (err != NMBS_ERROR_NONE)
    {
        LOG_ERROR("Modbus RTU slave: failed to create server: %d - task will exit", err);
        vTaskDelete(NULL);
        return;
    }

    nmbs_set_read_timeout(&nmbs, MODBUS_RTU_SLAVE_READ_TIMEOUT_MS);
    nmbs_set_byte_timeout(&nmbs, MODBUS_RTU_SLAVE_BYTE_TIMEOUT_MS);

    /* Pass the server config as the callback argument */
    nmbs_set_callbacks_arg(&nmbs, &s_server_config);

    LOG_INFO("Modbus RTU slave ready, entering poll loop");

    while (1)
    {
        err = nmbs_server_poll(&nmbs);

        if (err == NMBS_ERROR_NONE)
        {
            /* A complete request was handled successfully — continue immediately */
        }
        else if (err == NMBS_ERROR_TIMEOUT)
        {
            /* No request arrived within read_timeout — yield and wait */
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        else
        {
            LOG_WARN("Modbus RTU slave: poll error %d", err);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

bool modbus_rtu_slave_task_init(void)
{
    modbus_rtu_server_config_t server_config;
    if (!config_get_modbus_rtu_server_config(&server_config))
    {
        LOG_ERROR("Modbus RTU slave: failed to get server configuration");
        return false;
    }

    if (server_config.enable == MODBUS_RTU_SLAVE_DISABLED)
    {
        LOG_INFO("Modbus RTU slave disabled in configuration");
        return true;
    }

    BaseType_t result = xTaskCreate(vModbusRtuSlaveTask,
                                    "ModbusSlave",
                                    MODBUS_RTU_SLAVE_TASK_STACK_SIZE,
                                    NULL,
                                    MODBUS_RTU_SLAVE_TASK_PRIORITY,
                                    NULL);

    if (result != pdPASS)
    {
        LOG_ERROR("Modbus RTU slave: failed to create task");
        return false;
    }

    return true;
}
