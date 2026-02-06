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

/* Note: modbus_rtu_data_point_config_t, enums, and constants are defined in system_config.h */

/**
 * @brief Modbus RTU data point result (runtime only, not stored)
 * 
 * Contains the result of the last read/write operation for a data point.
 * This data lives in RAM and is updated after each poll cycle.
 */
typedef struct {
    nmbs_error last_error;                  /* Last operation error code */
    uint32_t last_poll_time_ms;             /* Timestamp of last poll (for diagnostics) */
    union {
        uint16_t registers[MODBUS_RTU_MAX_REG_COUNT];  /* For holding/input registers */
        uint8_t coils[MODBUS_RTU_MAX_REG_COUNT];       /* For coils/discrete inputs (0 or 1) */
    } data;
} modbus_rtu_data_point_result_t;

/* Data point configuration (loaded from flash at startup) */
static modbus_rtu_data_point_config_t s_data_point_configs[MODBUS_RTU_DATA_POINTS_MAX];
/* Data point results (runtime data, updated each cycle) */
static modbus_rtu_data_point_result_t s_data_point_results[MODBUS_RTU_DATA_POINTS_MAX];
/* Mutex to protect access to configs and results */
static SemaphoreHandle_t s_data_point_mutex = NULL;


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
 * @brief Load data points configuration from flash
 * 
 * Loads the Modbus RTU client configuration from flash storage, which includes
 * all data point configurations. Initializes result structures to zero.
 * 
 * @return true if config loaded successfully, false on error
 */
static bool modbus_rtu_load_data_points_from_config(void)
{
    modbus_rtu_client_config_t client_config;
    
    // Load configuration from flash
    if (!config_get_modbus_rtu_client_config(&client_config))
    {
        LOG_ERROR("Failed to load Modbus RTU client configuration from flash");
        return false;
    }
    
    // Copy data points from config to runtime arrays
    memcpy(s_data_point_configs, client_config.data_points, 
           sizeof(modbus_rtu_data_point_config_t) * MODBUS_RTU_DATA_POINTS_MAX);
    
    // Initialize result structures to zero
    memset(s_data_point_results, 0, sizeof(s_data_point_results));
    
    // Log enabled data points
    uint8_t enabled_count = 0;
    for (uint8_t i = 0; i < MODBUS_RTU_DATA_POINTS_MAX; i++)
    {
        if (s_data_point_configs[i].enabled)
        {
            enabled_count++;
            LOG_INFO("Data point %u: slave=%u, type=%u, op=%u, addr=%u, count=%u",
                     i,
                     s_data_point_configs[i].slave_address,
                     s_data_point_configs[i].data_type,
                     s_data_point_configs[i].operation,
                     s_data_point_configs[i].start_address,
                     s_data_point_configs[i].count);
        }
    }
    
    LOG_INFO("Modbus RTU: loaded %u enabled data points from configuration", enabled_count);
    return true;
}

/**
 * @brief Process a single data point
 * 
 * Reads or writes the specified Modbus registers/coils and stores the result
 * in the data point result structure.
 * 
 * @param nmbs      Pointer to nanoMODBUS client instance
 * @param config    Pointer to data point configuration (what to read/write)
 * @param result    Pointer to data point result (where to store data)
 */
static void modbus_rtu_process_data_point(nmbs_t *nmbs, 
                                          const modbus_rtu_data_point_config_t *config,
                                          modbus_rtu_data_point_result_t *result)
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
        /* Write operations - data comes from result structure (set by user/web UI) */
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
    
    if (err != NMBS_ERROR_NONE)
    {
        LOG_DEBUG("Modbus %s failed: slave=%u addr=%u count=%u err=%d",
                 config->operation == MODBUS_OP_READ ? "read" : "write",
                 config->slave_address, config->start_address, config->count, err);
    }
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
    s_data_point_mutex = xSemaphoreCreateMutex();
    if (s_data_point_mutex == NULL)
    {
        LOG_ERROR("Failed to create data point mutex - task will exit");
        vTaskDelete(NULL);
        return;
    }

    /* Load data points configuration from flash */
    if (!modbus_rtu_load_data_points_from_config())
    {
        LOG_ERROR("Failed to load data points from configuration - task will exit");
        vSemaphoreDelete(s_data_point_mutex);
        s_data_point_mutex = NULL;
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
        vSemaphoreDelete(s_data_point_mutex);
        s_data_point_mutex = NULL;
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
        if (xSemaphoreTake(s_data_point_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        {
            LOG_WARN("Failed to take data point mutex");
            continue;
        }

        /* Process all enabled data points */
        for (uint8_t i = 0; i < MODBUS_RTU_DATA_POINTS_MAX; i++)
        {
            if (s_data_point_configs[i].enabled)
            {
                modbus_rtu_process_data_point(&nmbs, &s_data_point_configs[i], &s_data_point_results[i]);
                
                /* Log successful reads for debugging */
                if (s_data_point_results[i].last_error == NMBS_ERROR_NONE && 
                    s_data_point_configs[i].operation == MODBUS_OP_READ)
                {
                    if (s_data_point_configs[i].data_type == MODBUS_DATA_TYPE_HOLDING_REGISTER ||
                        s_data_point_configs[i].data_type == MODBUS_DATA_TYPE_INPUT_REGISTER)
                    {
                        LOG_DEBUG("DataPoint[%u]: slave=%u addr=%u regs=[%u, %u]", 
                                 i, s_data_point_configs[i].slave_address, 
                                 s_data_point_configs[i].start_address,
                                 s_data_point_results[i].data.registers[0],
                                 s_data_point_configs[i].count > 1 ? s_data_point_results[i].data.registers[1] : 0);
                    }
                    else
                    {
                        LOG_DEBUG("DataPoint[%u]: slave=%u addr=%u coils=[%u, %u, %u]",
                                 i, s_data_point_configs[i].slave_address,
                                 s_data_point_configs[i].start_address,
                                 s_data_point_results[i].data.coils[0],
                                 s_data_point_configs[i].count > 1 ? s_data_point_results[i].data.coils[1] : 0,
                                 s_data_point_configs[i].count > 2 ? s_data_point_results[i].data.coils[2] : 0);
                    }
                }
                else
                {
                    LOG_ERROR("DataPoint[%u]: slave=%u addr=%u error=%d",
                             i, s_data_point_configs[i].slave_address,
                             s_data_point_configs[i].start_address,
                             s_data_point_results[i].last_error);
                }
            }
        }

        xSemaphoreGive(s_data_point_mutex);
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
