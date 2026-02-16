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

#include "board_config.h"
#include "logger.h"
#include "modbus_request.h"
#include "modbus_request_processor.h"
#include "nanomodbus.h"
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

/* Request configuration (loaded from flash at startup) */
static modbus_request_config_t s_request_configs[MODBUS_REQUESTS_MAX];
/* Request results (runtime data, updated each cycle) */
static modbus_request_result_t s_request_results[MODBUS_REQUESTS_MAX];
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
           sizeof(modbus_request_config_t) * MODBUS_REQUESTS_MAX);
    
    // Initialize result structures to zero
    memset(s_request_results, 0, sizeof(s_request_results));
    
    // Log enabled requests
    uint8_t enabled_count = 0;
    for (uint8_t i = 0; i < MODBUS_REQUESTS_MAX; i++)
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
        for (uint8_t i = 0; i < MODBUS_REQUESTS_MAX; i++)
        {
            if (s_request_configs[i].enabled)
            {
                modbus_request_process(&nmbs, &s_request_configs[i], &s_request_results[i]);
                
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
