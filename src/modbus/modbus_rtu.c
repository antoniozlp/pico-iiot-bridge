/**
 * @file modbus_rtu.c
 * @brief Modbus RTU client task implementation (nanoMODBUS library)
 *
 * FreeRTOS task that runs Modbus RTU client operations for testing the
 * nanoMODBUS library. Configuration is read from system_config.
 */

#include "modbus_rtu.h"

#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "hardware/timer.h"
#include "hardware/uart.h"

#include "nanomodbus.h"

#include "board_config.h"
#include "logger.h"
#include "system_config.h"
#include "tag_database.h"

/* Task configuration */
#define MODBUS_RTU_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE * 4)  /* 2KB for nanoMODBUS buffers */
#define MODBUS_RTU_TASK_PRIORITY    (tskIDLE_PRIORITY + 2)
#define MODBUS_RTU_TASK_LOOP_DELAY_MS  1000

/* nanoMODBUS timeouts (ms) */
#define MODBUS_RTU_READ_TIMEOUT_MS  1000
#define MODBUS_RTU_BYTE_TIMEOUT_MS 100
#define MODBUS_RTU_READ_DELAY_MS 1 // Avoid blocking the task if the timeout is not reached and the still data to read

/* Enable flag: modbus_rtu_client_config_t.enable */
#define MODBUS_RTU_DISABLED         0

/* Note: modbus_rtu_request_config_t, enums, and constants are defined in system_config.h */

/**
 * @brief Modbus RTU request result (runtime only, not stored)
 * 
 * Contains the result of the last read/write operation for a request.
 * This data lives in RAM and is updated after each poll cycle.
 */
typedef struct {
    nmbs_error last_error;                  /* Last operation error code */
    uint32_t last_poll_time_ms;             /* Timestamp of last poll (for diagnostics) */
    union {
        uint16_t registers[MODBUS_RTU_MAX_REG_COUNT];  /* For holding/input registers */
        uint8_t coils[MODBUS_RTU_MAX_REG_COUNT];       /* For coils/discrete inputs (0 or 1) */
    } data;
} modbus_rtu_request_result_t;

/* Request configuration (loaded from flash at startup) */
static modbus_rtu_request_config_t s_request_configs[MODBUS_RTU_REQUESTS_MAX];
/* Request results (runtime data, updated each cycle) */
static modbus_rtu_request_result_t s_request_results[MODBUS_RTU_REQUESTS_MAX];
/* Mutex to protect access to configs and results */
static SemaphoreHandle_t s_request_mutex = NULL;


/* UART instance used by transport callbacks (set before task runs) */
static uart_inst_t *s_modbus_rtu_uart;

/**
 * @brief Error callback: log and indicate Modbus RTU error
 *
 * Called when a Modbus operation fails. Can be extended (e.g. LED blink).
 */
static void modbus_rtu_on_error(void)
{
    LOG_ERROR("Modbus RTU error");
}

/**
 * @brief Load requests configuration from flash
 * 
 * Loads the Modbus RTU client configuration from flash storage, which includes
 * all request configurations. Initializes result structures to zero.
 * 
 * @return true if config loaded successfully, false on error
 */
static bool modbus_rtu_load_requests_from_config(void)
{
    modbus_rtu_client_config_t client_config;
    
    // Load configuration from flash
    if (!config_get_modbus_rtu_client_config(&client_config))
    {
        LOG_ERROR("Failed to load Modbus RTU client configuration from flash");
        return false;
    }
    
    // Copy requests from config to runtime arrays
    memcpy(s_request_configs, client_config.requests, 
           sizeof(modbus_rtu_request_config_t) * MODBUS_RTU_REQUESTS_MAX);
    
    // Initialize result structures to zero
    memset(s_request_results, 0, sizeof(s_request_results));
    
    // Log enabled requests
    uint8_t enabled_count = 0;
    for (uint8_t i = 0; i < MODBUS_RTU_REQUESTS_MAX; i++)
    {
        if (s_request_configs[i].enabled)
        {
            enabled_count++;
            LOG_INFO("Request %u: slave=%u, type=%u, op=%u, addr=%u, count=%u",
                     i,
                     s_request_configs[i].slave_address,
                     s_request_configs[i].data_type,
                     s_request_configs[i].operation,
                     s_request_configs[i].start_address,
                     s_request_configs[i].count);
        }
    }
    
    LOG_INFO("Modbus RTU: loaded %u enabled requests from configuration", enabled_count);
    return true;
}

/* Forward declarations */
static void modbus_rtu_map_to_tags(
    const modbus_rtu_request_config_t *config,
    const modbus_rtu_request_result_t *result);
static void modbus_rtu_map_from_tags(
    const modbus_rtu_request_config_t *config,
    modbus_rtu_request_result_t *result);

/**
 * @brief Process a single Modbus request
 * 
 * Reads or writes the specified Modbus registers/coils and stores the result
 * in the request result structure.
 * 
 * @param nmbs      Pointer to nanoMODBUS client instance
 * @param config    Pointer to request configuration (what to read/write)
 * @param result    Pointer to request result (where to store data)
 */
static void modbus_rtu_process_request(nmbs_t *nmbs, 
                                       const modbus_rtu_request_config_t *config,
                                       modbus_rtu_request_result_t *result)
{
    if (nmbs == NULL || config == NULL || result == NULL || !config->enabled)
    {
        return;
    }
    
    /* Set destination address for this item */
    nmbs_set_destination_rtu_address(nmbs, config->slave_address);
    
    nmbs_error err = NMBS_ERROR_NONE;
    
    /* Process based on data type and operation */
    if (config->operation == MODBUS_OP_READ)
    {
        switch (config->data_type)
        {
            case MODBUS_DATA_TYPE_COIL:
            {
                /* Read coils into bitfield, then convert to byte array */
                nmbs_bitfield coils = {0};
                err = nmbs_read_coils(nmbs, config->start_address, config->count, coils);
                if (err == NMBS_ERROR_NONE)
                {
                    for (uint16_t i = 0; i < config->count && i < MODBUS_RTU_MAX_REG_COUNT; i++)
                    {
                        result->data.coils[i] = nmbs_bitfield_read(coils, i) ? 1 : 0;
                    }
                }
                break;
            }
            
            case MODBUS_DATA_TYPE_DISCRETE_INPUT:
            {
                nmbs_bitfield inputs = {0};
                err = nmbs_read_discrete_inputs(nmbs, config->start_address, config->count, inputs);
                if (err == NMBS_ERROR_NONE)
                {
                    for (uint16_t i = 0; i < config->count && i < MODBUS_RTU_MAX_REG_COUNT; i++)
                    {
                        result->data.coils[i] = nmbs_bitfield_read(inputs, i) ? 1 : 0;
                    }
                }
                break;
            }
            
            case MODBUS_DATA_TYPE_INPUT_REGISTER:
            {
                err = nmbs_read_input_registers(nmbs, config->start_address, 
                                               config->count, result->data.registers);
                break;
            }
            
            case MODBUS_DATA_TYPE_HOLDING_REGISTER:
            {
                err = nmbs_read_holding_registers(nmbs, config->start_address,
                                                 config->count, result->data.registers);
                break;
            }
            
            default:
                err = NMBS_ERROR_INVALID_ARGUMENT;
                break;
        }
    }
    else  /* MODBUS_OP_WRITE */
    {
        /* Write operations - populate result buffer from tag database before Modbus write */
        modbus_rtu_map_from_tags(config, result);
        
        switch (config->data_type)
        {
            case MODBUS_DATA_TYPE_COIL:
            {
                nmbs_bitfield coils = {0};
                for (uint16_t i = 0; i < config->count && i < MODBUS_RTU_MAX_REG_COUNT; i++)
                {
                    nmbs_bitfield_write(coils, i, result->data.coils[i]);
                }
                err = nmbs_write_multiple_coils(nmbs, config->start_address, config->count, coils);
                break;
            }
            
            case MODBUS_DATA_TYPE_HOLDING_REGISTER:
            {
                err = nmbs_write_multiple_registers(nmbs, config->start_address,
                                                   config->count, result->data.registers);
                break;
            }
            
            case MODBUS_DATA_TYPE_DISCRETE_INPUT:
            case MODBUS_DATA_TYPE_INPUT_REGISTER:
                /* These are read-only, cannot write */
                err = NMBS_ERROR_INVALID_ARGUMENT;
                LOG_ERROR("Cannot write to read-only Modbus data type");
                break;
                
            default:
                err = NMBS_ERROR_INVALID_ARGUMENT;
                break;
        }
    }
    
    /* Store result metadata */
    result->last_error = err;
    result->last_poll_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    /* Map Modbus data to tags */
    if (config->operation == MODBUS_OP_READ)
    {
        if (err == NMBS_ERROR_NONE)
        {
            // Successful read: map register values to tags with GOOD quality
            modbus_rtu_map_to_tags(config, result);
        }
        else
        {
            // Failed read: update all mapped tags with BAD quality
            for (uint16_t i = 0; i < config->count && i < MODBUS_RTU_MAX_REG_COUNT; i++)
            {
                if (config->tag_handles[i] != MODBUS_TAG_MAP_INVALID)
                {
                    tag_value_t dummy_value;
                    memset(&dummy_value, 0, sizeof(dummy_value));
                    tag_db_write(config->tag_handles[i], dummy_value, TAG_QUALITY_BAD);
                }
            }
            LOG_DEBUG("Modbus read failed: slave=%u addr=%u count=%u err=%d",
                     config->slave_address, config->start_address, config->count, err);
        }
    }
    else if (err != NMBS_ERROR_NONE)
    {
        // Write operation failed
        LOG_DEBUG("Modbus write failed: slave=%u addr=%u count=%u err=%d",
                 config->slave_address, config->start_address, config->count, err);
    }
}

/**
 * @brief Map Modbus register data to tags
 * 
 * Converts Modbus registers to tag values based on configuration.
 * Handles multi-register types (INT32, FLOAT) by combining sequential registers.
 * Uses big-endian byte order (high word first) for 32-bit values.
 * 
 * @param config Pointer to data point configuration (defines mapping)
 * @param result Pointer to data point result (contains register values)
 */
static void modbus_rtu_map_to_tags(
    const modbus_rtu_request_config_t *config,
    const modbus_rtu_request_result_t *result)
{
    if (config == NULL || result == NULL)
    {
        return;
    }
    
    uint16_t reg_idx = 0;  // Current register position in result data
    
    // Iterate through each register/coil in the data point
    for (uint16_t i = 0; i < config->count && i < MODBUS_RTU_MAX_REG_COUNT; i++)
    {
        tag_handle_t handle = config->tag_handles[i];
        
        // Skip unmapped registers
        if (handle == MODBUS_TAG_MAP_INVALID)
        {
            continue;
        }
        
        // Get tag metadata to determine data type and register consumption
        tag_metadata_t metadata;
        if (!tag_db_get_metadata(handle, &metadata))
        {
            LOG_WARN("Invalid tag handle in Modbus mapping: %d", handle);
            continue;
        }
        
        tag_value_t value;
        memset(&value, 0, sizeof(value));
        bool write_ok = false;
        uint16_t regs_consumed = 0;
        
        // Map register(s) to tag value based on tag data type
        switch (metadata.data_type)
        {
            case TAG_TYPE_BOOL:
                // Single register, treat non-zero as true
                if (reg_idx < config->count)
                {
                    value.bool_val = (result->data.registers[reg_idx] != 0);
                    write_ok = true;
                    regs_consumed = 1;
                }
                break;
                
            case TAG_TYPE_UINT8:
                // Single register, use lower 8 bits
                if (reg_idx < config->count)
                {
                    value.u8_val = (uint8_t)(result->data.registers[reg_idx] & 0xFF);
                    write_ok = true;
                    regs_consumed = 1;
                }
                break;
                
            case TAG_TYPE_UINT16:
                // Single register
                if (reg_idx < config->count)
                {
                    value.u16_val = result->data.registers[reg_idx];
                    write_ok = true;
                    regs_consumed = 1;
                }
                break;
                
            case TAG_TYPE_INT16:
                // Single register (signed)
                if (reg_idx < config->count)
                {
                    value.i16_val = (int16_t)result->data.registers[reg_idx];
                    write_ok = true;
                    regs_consumed = 1;
                }
                break;
                
            case TAG_TYPE_UINT32:
            case TAG_TYPE_INT32:
                // Two registers: big-endian (high word first, low word second)
                if (reg_idx + 1 < config->count)
                {
                    value.u32_val = ((uint32_t)result->data.registers[reg_idx] << 16) |
                                    result->data.registers[reg_idx + 1];
                    write_ok = true;
                    regs_consumed = 2;
                }
                else
                {
                    LOG_WARN("Tag %s requires 2 registers but only %d available at index %d",
                            metadata.name, config->count - reg_idx, reg_idx);
                }
                break;
                
            case TAG_TYPE_FLOAT:
                // Two registers: combine as IEEE 754 float (big-endian)
                if (reg_idx + 1 < config->count)
                {
                    uint32_t raw = ((uint32_t)result->data.registers[reg_idx] << 16) |
                                   result->data.registers[reg_idx + 1];
                    memcpy(&value.float_val, &raw, sizeof(float));
                    write_ok = true;
                    regs_consumed = 2;
                }
                else
                {
                    LOG_WARN("Tag %s requires 2 registers but only %d available at index %d",
                            metadata.name, config->count - reg_idx, reg_idx);
                }
                break;
                
            default:
                LOG_ERROR("Unknown tag data type: %d", metadata.data_type);
                break;
        }
        
        // Write to tag database with good quality
        if (write_ok)
        {
            if (tag_db_write(handle, value, TAG_QUALITY_GOOD))
            {
                LOG_DEBUG("Mapped Modbus reg[%d] to tag %s (handle=%d, regs=%d)",
                         reg_idx, metadata.name, handle, regs_consumed);
            }
            reg_idx += regs_consumed;
        }
    }
}

/**
 * @brief Map tag values to Modbus register/coil buffer for write operations
 * 
 * Reads tag values from the tag database and populates the result buffer
 * based on tag_handles mapping. Used before Modbus write operations.
 * Handles multi-register types (INT32, FLOAT) by splitting into two registers.
 * Uses big-endian byte order (high word first) for 32-bit values.
 * 
 * @param config Pointer to data point configuration (defines mapping)
 * @param result Pointer to data point result (buffer to populate for write)
 */
static void modbus_rtu_map_from_tags(
    const modbus_rtu_request_config_t *config,
    modbus_rtu_request_result_t *result)
{
    if (config == NULL || result == NULL)
    {
        return;
    }
    
    if (config->data_type == MODBUS_DATA_TYPE_COIL)
    {
        /* Coils: each tag_handles[i] maps to coil i; unmapped coils get 0 */
        memset(result->data.coils, 0, MODBUS_RTU_MAX_REG_COUNT);
        
        for (uint16_t i = 0; i < config->count && i < MODBUS_RTU_MAX_REG_COUNT; i++)
        {
            tag_handle_t handle = config->tag_handles[i];
            
            if (handle == MODBUS_TAG_MAP_INVALID)
            {
                continue;  /* Unmapped: already 0 from memset */
            }
            
            tag_value_t value;
            if (tag_db_read(handle, &value, NULL, NULL))
            {
                result->data.coils[i] = (value.bool_val || value.u16_val != 0 ||
                                        value.u32_val != 0 || value.i16_val != 0 ||
                                        value.i32_val != 0 || value.float_val != 0.0f) ? 1 : 0;
            }
            else
            {
                LOG_WARN("Failed to read tag handle %d for coil %d", handle, i);
            }
        }
    }
    else if (config->data_type == MODBUS_DATA_TYPE_HOLDING_REGISTER)
    {
        /* Registers: iterate by register position. For each position i, tag_handles[i]
         * gives the tag. INVALID = unmapped (write 0) or low word of 32-bit tag (skip). */
        memset(result->data.registers, 0, sizeof(result->data.registers));
        
        bool prev_consumed_2regs = false;  /* Skip next position if prev tag was 32-bit */
        
        for (uint16_t i = 0; i < config->count && i < MODBUS_RTU_MAX_REG_COUNT; i++)
        {
            if (prev_consumed_2regs)
            {
                prev_consumed_2regs = false;
                continue;  /* Low word of 32-bit tag - already written */
            }
            
            tag_handle_t handle = config->tag_handles[i];
            
            if (handle == MODBUS_TAG_MAP_INVALID)
            {
                result->data.registers[i] = 0;  /* Unmapped register */
                continue;
            }
            
            tag_metadata_t metadata;
            if (!tag_db_get_metadata(handle, &metadata))
            {
                LOG_WARN("Invalid tag handle in Modbus write mapping: %d", handle);
                result->data.registers[i] = 0;
                continue;
            }
            
            tag_value_t value;
            if (!tag_db_read(handle, &value, NULL, NULL))
            {
                LOG_WARN("Failed to read tag %s for register write", metadata.name);
                result->data.registers[i] = 0;
                continue;
            }
            
            switch (metadata.data_type)
            {
                case TAG_TYPE_BOOL:
                    result->data.registers[i] = value.bool_val ? 1 : 0;
                    break;
                    
                case TAG_TYPE_UINT8:
                    result->data.registers[i] = value.u8_val;
                    break;
                    
                case TAG_TYPE_UINT16:
                    result->data.registers[i] = value.u16_val;
                    break;
                    
                case TAG_TYPE_INT16:
                    result->data.registers[i] = (uint16_t)value.i16_val;
                    break;
                    
                case TAG_TYPE_UINT32:
                case TAG_TYPE_INT32:
                    if (i + 1 < config->count)
                    {
                        result->data.registers[i] = (uint16_t)(value.u32_val >> 16);
                        result->data.registers[i + 1] = (uint16_t)(value.u32_val & 0xFFFF);
                        prev_consumed_2regs = true;
                    }
                    else
                    {
                        result->data.registers[i] = (uint16_t)(value.u32_val & 0xFFFF);
                        LOG_WARN("Tag %s (32-bit) truncated: only 1 register available", metadata.name);
                    }
                    break;
                    
                case TAG_TYPE_FLOAT:
                    if (i + 1 < config->count)
                    {
                        uint32_t raw;
                        memcpy(&raw, &value.float_val, sizeof(float));
                        result->data.registers[i] = (uint16_t)(raw >> 16);
                        result->data.registers[i + 1] = (uint16_t)(raw & 0xFFFF);
                        prev_consumed_2regs = true;
                    }
                    else
                    {
                        uint32_t raw;
                        memcpy(&raw, &value.float_val, sizeof(float));
                        result->data.registers[i] = (uint16_t)(raw & 0xFFFF);
                        LOG_WARN("Tag %s (float) truncated: only 1 register available", metadata.name);
                    }
                    break;
                    
                default:
                    result->data.registers[i] = 0;
                    LOG_WARN("Unknown tag type %d for write", metadata.data_type);
                    break;
            }
        }
    }
    /* DISCRETE_INPUT and INPUT_REGISTER are read-only - no write mapping */
}

/**
 * @brief Transport read callback for nanoMODBUS
 *
 * @param buf       Output buffer (must not be NULL)
 * @param count     Max bytes to read
 * @param byte_timeout_ms  Timeout per byte (unused; we use our own timing)
 * @param arg       User argument (unused)
 * @return Number of bytes read, or negative on error
 */
static int32_t modbus_rtu_read_serial(uint8_t *buf, uint16_t count,
                                      int32_t byte_timeout_ms, void *arg)
{
    (void)arg;

    if (buf == NULL || s_modbus_rtu_uart == NULL)
    {
        return -1;
    }

    uint64_t start_time = time_us_64();
    int32_t bytes_read = 0;
    uint64_t timeout_us = (uint64_t)byte_timeout_ms * 1000;

    while ((time_us_64() - start_time) < timeout_us && bytes_read < count)
    {
        if (uart_is_readable(s_modbus_rtu_uart))
        {
            buf[bytes_read++] = uart_getc(s_modbus_rtu_uart);
            start_time = time_us_64();
        }
        else if (byte_timeout_ms >= MODBUS_RTU_READ_TIMEOUT_MS)
        {
            // Avoid blocking the task if the timeout is not reached and the still data to read
            // MODBUS_RTU_READ_TIMEOUT_MS (1000ms) - Waiting for slave to START responding (long wait)
            vTaskDelay(pdMS_TO_TICKS(MODBUS_RTU_READ_DELAY_MS));
        }
    }

    return bytes_read;
}

/**
 * @brief Transport write callback for nanoMODBUS
 *
 * @param buf       Data to write (must not be NULL)
 * @param count     Number of bytes to write
 * @param byte_timeout_ms  Unused
 * @param arg       User argument (unused)
 * @return Number of bytes written, or negative on error
 */
static int32_t modbus_rtu_write_serial(const uint8_t *buf, uint16_t count,
                                       int32_t byte_timeout_ms, void *arg)
{
    (void)byte_timeout_ms;
    (void)arg;

    if (buf == NULL || s_modbus_rtu_uart == NULL)
    {
        return -1;
    }

    uart_write_blocking(s_modbus_rtu_uart, buf, count);
    return (int32_t)count;
}

/**
 * @brief Modbus RTU client task
 *
 * Loads config, initializes UART, configures nanoMODBUS client, then runs
 * test operations (write/read coils and holding registers). On config or
 * init failure, the task logs and deletes itself. If client creation fails, the
 * task deletes itself.
 */
static void vModbusRtuTask(void *pvParameters)
{
    (void)pvParameters;

    LOG_INFO("Modbus RTU task started");

    modbus_rtu_client_config_t modbus_rtu_client_config;
    if (!config_get_modbus_rtu_client_config(&modbus_rtu_client_config))
    {
        LOG_ERROR("Modbus RTU: failed to get client configuration - task will exit");
        vTaskDelete(NULL);
        return;
    }

    serial_config_t serial_config;
    if (!config_get_serial_config(modbus_rtu_client_config.serial_id, &serial_config))
    {
        LOG_ERROR("Modbus RTU: failed to get serial configuration - task will exit");
        vTaskDelete(NULL);
        return;
    }

    s_modbus_rtu_uart = (modbus_rtu_client_config.serial_id == 0)
                        ? BOARD_UART0_ID
                        : BOARD_UART1_ID;

    if (!board_init_uart(s_modbus_rtu_uart, &serial_config))
    {
        LOG_ERROR("Modbus RTU: failed to initialize UART - task will exit");
        vTaskDelete(NULL);
        return;
    }

    LOG_INFO("Modbus RTU UART%u initialized: %u baud, %uN%u",
             modbus_rtu_client_config.serial_id,
             serial_config.baud,
             serial_config.databits,
             serial_config.stopbits);

    /* Create mutex for data point access */
    s_request_mutex = xSemaphoreCreateMutex();
    if (s_request_mutex == NULL)
    {
        LOG_ERROR("Failed to create data point mutex - task will exit");
        vTaskDelete(NULL);
        return;
    }

    /* Load data points configuration from flash */
    if (!modbus_rtu_load_requests_from_config())
    {
        LOG_ERROR("Failed to load data points from configuration - task will exit");
        vSemaphoreDelete(s_request_mutex);
        s_request_mutex = NULL;
        vTaskDelete(NULL);
        return;
    }

    /* Create and configure nanoMODBUS client */
    nmbs_platform_conf platform_conf;
    nmbs_platform_conf_create(&platform_conf);
    platform_conf.transport = NMBS_TRANSPORT_RTU;
    platform_conf.read = modbus_rtu_read_serial;
    platform_conf.write = modbus_rtu_write_serial;

    nmbs_t nmbs;
    nmbs_error err = nmbs_client_create(&nmbs, &platform_conf);
    if (err != NMBS_ERROR_NONE)
    {
        LOG_ERROR("Failed to create Modbus client: %d - task will exit", err);
        modbus_rtu_on_error();
        vSemaphoreDelete(s_request_mutex);
        s_request_mutex = NULL;
        vTaskDelete(NULL);
        return;
    }

    nmbs_set_read_timeout(&nmbs, MODBUS_RTU_READ_TIMEOUT_MS);
    nmbs_set_byte_timeout(&nmbs, MODBUS_RTU_BYTE_TIMEOUT_MS);

    LOG_INFO("Modbus RTU client ready, starting data point processing loop");

    /* Main data point processing loop */
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(MODBUS_RTU_TASK_LOOP_DELAY_MS));

        /* Take mutex to access data points */
        if (xSemaphoreTake(s_request_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        {
            LOG_WARN("Failed to take data point mutex");
            continue;
        }

        /* Process all enabled data points */
        for (uint8_t i = 0; i < MODBUS_RTU_REQUESTS_MAX; i++)
        {
            if (s_request_configs[i].enabled)
            {
                modbus_rtu_process_request(&nmbs, &s_request_configs[i], &s_request_results[i]);
                
                /* Log successful reads and writes for debugging */
                if (s_request_results[i].last_error == NMBS_ERROR_NONE)
                {
                    if (s_request_configs[i].data_type == MODBUS_DATA_TYPE_HOLDING_REGISTER ||
                        s_request_configs[i].data_type == MODBUS_DATA_TYPE_INPUT_REGISTER)
                    {
                        LOG_DEBUG("Request[%u]: slave=%u addr=%u %s regs=[%u, %u]",
                                 i, s_request_configs[i].slave_address,
                                 s_request_configs[i].start_address,
                                 s_request_configs[i].operation == MODBUS_OP_READ ? "read" : "write",
                                 s_request_results[i].data.registers[0],
                                 s_request_configs[i].count > 1 ? s_request_results[i].data.registers[1] : 0);
                    }
                    else
                    {
                        LOG_DEBUG("Request[%u]: slave=%u addr=%u %s coils=[%u, %u, %u]",
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
                    /* Only log when there is an actual error (read or write failure) */
                    LOG_ERROR("Request[%u]: slave=%u addr=%u error=%d",
                             i, s_request_configs[i].slave_address,
                             s_request_configs[i].start_address,
                             s_request_results[i].last_error);
                }
            }
        }

        xSemaphoreGive(s_request_mutex);
    }
}

bool modbus_rtu_task_init(void)
{
    modbus_rtu_client_config_t modbus_rtu_client_config;
    if (!config_get_modbus_rtu_client_config(&modbus_rtu_client_config))
    {
        LOG_ERROR("Failed to get Modbus RTU client configuration");
        return false;
    }

    if (modbus_rtu_client_config.enable == MODBUS_RTU_DISABLED)
    {
        LOG_INFO("Modbus RTU disabled in configuration");
        return true;
    }

    BaseType_t result = xTaskCreate(vModbusRtuTask,
                                   "ModbusRTU",
                                   MODBUS_RTU_TASK_STACK_SIZE,
                                   NULL,
                                   MODBUS_RTU_TASK_PRIORITY,
                                   NULL);

    if (result != pdPASS)
    {
        LOG_ERROR("Failed to create Modbus RTU task");
        return false;
    }

    return true;
}
