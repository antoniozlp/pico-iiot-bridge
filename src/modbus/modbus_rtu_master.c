/**
 * @file modbus_rtu_master.c
 * @brief Modbus RTU master (client) task implementation (nanoMODBUS library)
 *
 * FreeRTOS task that runs Modbus RTU master operations. Polls configured
 * request slots and maps results to the Tag Database. Configuration is
 * read from system_config at startup.
 */

#include "modbus_rtu_master.h"

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
#define MODBUS_RTU_MASTER_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE * 4)  /* 2KB for nanoMODBUS buffers */
#define MODBUS_RTU_MASTER_TASK_PRIORITY     (tskIDLE_PRIORITY + 2)
#define MODBUS_RTU_MASTER_LOOP_DELAY_MS     1000

/* nanoMODBUS timeouts (ms) */
#define MODBUS_RTU_MASTER_READ_TIMEOUT_MS   1000
#define MODBUS_RTU_MASTER_BYTE_TIMEOUT_MS   100
#define MODBUS_RTU_MASTER_READ_DELAY_MS     1

/* Enable flag value */
#define MODBUS_RTU_MASTER_DISABLED          0

/* Request configuration (loaded from flash at startup) */
static modbus_request_config_t s_request_configs[MODBUS_REQUESTS_MAX];
/* Request results (runtime data, updated each cycle) */
static modbus_request_result_t s_request_results[MODBUS_REQUESTS_MAX];
/* Mutex to protect access to configs and results */
static SemaphoreHandle_t s_request_mutex = NULL;

/* UART instance used by transport callbacks (set before task runs) */
static uart_inst_t *s_master_uart;

/**
 * @brief Load requests configuration from flash
 *
 * @return true if config loaded successfully, false on error
 */
static bool modbus_rtu_master_load_requests(void)
{
    modbus_rtu_client_config_t client_config;

    if (!config_get_modbus_rtu_client_config(&client_config))
    {
        LOG_ERROR("Modbus RTU master: failed to load client configuration");
        return false;
    }

    memcpy(s_request_configs, client_config.requests,
           sizeof(modbus_request_config_t) * MODBUS_REQUESTS_MAX);

    memset(s_request_results, 0, sizeof(s_request_results));

    uint8_t enabled_count = 0;
    for (uint8_t i = 0; i < MODBUS_REQUESTS_MAX; i++)
    {
        if (s_request_configs[i].enabled)
        {
            enabled_count++;
            LOG_INFO("Master request %u: slave=%u, type=%u, op=%u, addr=%u, count=%u",
                     i,
                     s_request_configs[i].slave_address,
                     s_request_configs[i].data_type,
                     s_request_configs[i].operation,
                     s_request_configs[i].start_address,
                     s_request_configs[i].count);
        }
    }

    LOG_INFO("Modbus RTU master: loaded %u enabled requests", enabled_count);
    return true;
}

/**
 * @brief Transport read callback for nanoMODBUS
 *
 * @param buf              Output buffer (must not be NULL)
 * @param count            Max bytes to read
 * @param byte_timeout_ms  Per-byte timeout in ms
 * @param arg              User argument (unused)
 * @return Number of bytes read, or negative on error
 */
static int32_t modbus_rtu_master_read_serial(uint8_t *buf, uint16_t count,
                                              int32_t byte_timeout_ms, void *arg)
{
    (void)arg;

    if (buf == NULL || s_master_uart == NULL)
    {
        return -1;
    }

    uint64_t start_time = time_us_64();
    int32_t bytes_read = 0;
    uint64_t timeout_us = (uint64_t)byte_timeout_ms * 1000;

    while ((time_us_64() - start_time) < timeout_us && bytes_read < count)
    {
        if (uart_is_readable(s_master_uart))
        {
            buf[bytes_read++] = uart_getc(s_master_uart);
            start_time = time_us_64();
        }
        else if (byte_timeout_ms >= MODBUS_RTU_MASTER_READ_TIMEOUT_MS)
        {
            /* Yield during long waits (waiting for slave response start) */
            vTaskDelay(pdMS_TO_TICKS(MODBUS_RTU_MASTER_READ_DELAY_MS));
        }
    }

    return bytes_read;
}

/**
 * @brief Transport write callback for nanoMODBUS
 *
 * @param buf              Data to write (must not be NULL)
 * @param count            Number of bytes to write
 * @param byte_timeout_ms  Unused
 * @param arg              User argument (unused)
 * @return Number of bytes written, or negative on error
 */
static int32_t modbus_rtu_master_write_serial(const uint8_t *buf, uint16_t count,
                                               int32_t byte_timeout_ms, void *arg)
{
    (void)byte_timeout_ms;
    (void)arg;

    if (buf == NULL || s_master_uart == NULL)
    {
        return -1;
    }

    uart_write_blocking(s_master_uart, buf, count);
    return (int32_t)count;
}

/**
 * @brief Modbus RTU master FreeRTOS task
 *
 * Loads config, initializes UART, configures nanoMODBUS client, then iterates
 * all enabled request slots each poll cycle. On fatal init failure the task
 * deletes itself.
 */
static void vModbusRtuMasterTask(void *pvParameters)
{
    (void)pvParameters;

    LOG_INFO("Modbus RTU master task started");

    modbus_rtu_client_config_t client_config;
    if (!config_get_modbus_rtu_client_config(&client_config))
    {
        LOG_ERROR("Modbus RTU master: failed to get client config - task will exit");
        vTaskDelete(NULL);
        return;
    }

    serial_config_t serial_config;
    if (!config_get_serial_config(client_config.serial_id, &serial_config))
    {
        LOG_ERROR("Modbus RTU master: failed to get serial config - task will exit");
        vTaskDelete(NULL);
        return;
    }

    s_master_uart = (client_config.serial_id == 0) ? BOARD_UART0_ID : BOARD_UART1_ID;

    if (!board_init_uart(s_master_uart, &serial_config))
    {
        LOG_ERROR("Modbus RTU master: failed to initialize UART - task will exit");
        vTaskDelete(NULL);
        return;
    }

    LOG_INFO("Modbus RTU master UART%u initialized: %u baud, %uN%u",
             client_config.serial_id,
             serial_config.baud,
             serial_config.databits,
             serial_config.stopbits);

    s_request_mutex = xSemaphoreCreateMutex();
    if (s_request_mutex == NULL)
    {
        LOG_ERROR("Modbus RTU master: failed to create mutex - task will exit");
        vTaskDelete(NULL);
        return;
    }

    if (!modbus_rtu_master_load_requests())
    {
        LOG_ERROR("Modbus RTU master: failed to load requests - task will exit");
        vSemaphoreDelete(s_request_mutex);
        s_request_mutex = NULL;
        vTaskDelete(NULL);
        return;
    }

    nmbs_platform_conf platform_conf;
    nmbs_platform_conf_create(&platform_conf);
    platform_conf.transport = NMBS_TRANSPORT_RTU;
    platform_conf.read = modbus_rtu_master_read_serial;
    platform_conf.write = modbus_rtu_master_write_serial;

    nmbs_t nmbs;
    nmbs_error err = nmbs_client_create(&nmbs, &platform_conf);
    if (err != NMBS_ERROR_NONE)
    {
        LOG_ERROR("Modbus RTU master: failed to create client: %d - task will exit", err);
        vSemaphoreDelete(s_request_mutex);
        s_request_mutex = NULL;
        vTaskDelete(NULL);
        return;
    }

    nmbs_set_read_timeout(&nmbs, MODBUS_RTU_MASTER_READ_TIMEOUT_MS);
    nmbs_set_byte_timeout(&nmbs, MODBUS_RTU_MASTER_BYTE_TIMEOUT_MS);

    LOG_INFO("Modbus RTU master ready, starting polling loop");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(MODBUS_RTU_MASTER_LOOP_DELAY_MS));

        if (xSemaphoreTake(s_request_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        {
            LOG_WARN("Modbus RTU master: failed to take mutex");
            continue;
        }

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
                        LOG_DEBUG("Master request[%u]: slave=%u addr=%u %s regs=[%u, %u]",
                                 i, s_request_configs[i].slave_address,
                                 s_request_configs[i].start_address,
                                 s_request_configs[i].operation == MODBUS_OP_READ ? "read" : "write",
                                 s_request_results[i].data.registers[0],
                                 s_request_configs[i].count > 1 ? s_request_results[i].data.registers[1] : 0);
                    }
                    else
                    {
                        LOG_DEBUG("Master request[%u]: slave=%u addr=%u %s coils=[%u, %u, %u]",
                                 i, s_request_configs[i].slave_address,
                                 s_request_configs[i].start_address,
                                 s_request_configs[i].operation == MODBUS_OP_READ ? "read" : "write",
                                 s_request_results[i].data.coils[0],
                                 s_request_configs[i].count > 1 ? s_request_results[i].data.coils[1] : 0,
                                 s_request_configs[i].count > 2 ? s_request_results[i].data.coils[2] : 0);
                    }
                }
                else
                {
                    LOG_ERROR("Master request[%u]: slave=%u addr=%u error=%d",
                             i, s_request_configs[i].slave_address,
                             s_request_configs[i].start_address,
                             s_request_results[i].last_error);
                }
            }
        }

        xSemaphoreGive(s_request_mutex);
    }
}

bool modbus_rtu_master_task_init(void)
{
    modbus_rtu_client_config_t client_config;
    if (!config_get_modbus_rtu_client_config(&client_config))
    {
        LOG_ERROR("Modbus RTU master: failed to get client configuration");
        return false;
    }

    if (client_config.enable == MODBUS_RTU_MASTER_DISABLED)
    {
        LOG_INFO("Modbus RTU master disabled in configuration");
        return true;
    }

    BaseType_t result = xTaskCreate(vModbusRtuMasterTask,
                                    "ModbusMaster",
                                    MODBUS_RTU_MASTER_TASK_STACK_SIZE,
                                    NULL,
                                    MODBUS_RTU_MASTER_TASK_PRIORITY,
                                    NULL);

    if (result != pdPASS)
    {
        LOG_ERROR("Modbus RTU master: failed to create task");
        return false;
    }

    return true;
}
