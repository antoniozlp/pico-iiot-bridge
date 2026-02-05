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

/* Enable flag: modbus_rtu_client_config_t.enable */
#define MODBUS_RTU_DISABLED         0

/* Polling list configuration */
#define MODBUS_RTU_POLL_LIST_SIZE   10   /* Maximum number of items to poll */
#define MODBUS_RTU_MAX_REG_COUNT    10   /* Max registers/coils per request */

/**
 * @brief Modbus data types (register and coil types)
 */
typedef enum {
    MODBUS_DATA_TYPE_COIL = 0,              /* Discrete output coils (read/write, 1 bit) */
    MODBUS_DATA_TYPE_DISCRETE_INPUT = 1,    /* Discrete inputs (read-only, 1 bit) */
    MODBUS_DATA_TYPE_INPUT_REGISTER = 2,    /* Input registers (read-only, 16 bit) */
    MODBUS_DATA_TYPE_HOLDING_REGISTER = 3   /* Holding registers (read/write, 16 bit) */
} modbus_rtu_data_type_t;

/**
 * @brief Modbus operation type
 */
typedef enum {
    MODBUS_OP_READ = 0,
    MODBUS_OP_WRITE = 1
} modbus_rtu_operation_t;

/**
 * @brief Modbus RTU polling item
 * 
 * Defines a Modbus register/coil to poll periodically. Results are stored
 * in the same structure after each poll cycle.
 */
typedef struct {
    uint8_t enabled;                        /* Enable/disable this poll item */
    uint8_t slave_address;                  /* Modbus slave address (1-247) */
    modbus_rtu_data_type_t data_type;       /* Type of data to read/write */
    modbus_rtu_operation_t operation;       /* Read or write */
    uint16_t start_address;                 /* Starting register/coil address */
    uint16_t count;                         /* Number of registers/coils (1-MODBUS_RTU_MAX_REG_COUNT) */
    
    /* Result data */
    nmbs_error last_error;                  /* Last operation error code */
    uint32_t last_poll_time_ms;             /* Timestamp of last poll (for diagnostics) */
    union {
        uint16_t registers[MODBUS_RTU_MAX_REG_COUNT];  /* For holding/input registers */
        uint8_t coils[MODBUS_RTU_MAX_REG_COUNT];       /* For coils/discrete inputs (0 or 1) */
    } data;
} modbus_rtu_poll_item_t;

/* Polling list: array of items to poll periodically */
static modbus_rtu_poll_item_t s_poll_list[MODBUS_RTU_POLL_LIST_SIZE];
static SemaphoreHandle_t s_poll_list_mutex = NULL;


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
 * @brief Initialize polling list with test/demo data
 * 
 * For testing: creates fake poll items to read holding registers,
 * input registers, and coils from a Modbus server.
 */
static void modbus_rtu_init_test_poll_list(void)
{
    memset(s_poll_list, 0, sizeof(s_poll_list));
    
    /* Poll item 0: Read 2 holding registers from address 26 */
    s_poll_list[0].enabled = true;
    s_poll_list[0].slave_address = 1;
    s_poll_list[0].data_type = MODBUS_DATA_TYPE_HOLDING_REGISTER;
    s_poll_list[0].operation = MODBUS_OP_READ;
    s_poll_list[0].start_address = 26;
    s_poll_list[0].count = 2;
    
    /* Poll item 1: Read 3 coils from address 64 */
    s_poll_list[1].enabled = true;
    s_poll_list[1].slave_address = 1;
    s_poll_list[1].data_type = MODBUS_DATA_TYPE_COIL;
    s_poll_list[1].operation = MODBUS_OP_READ;
    s_poll_list[1].start_address = 64;
    s_poll_list[1].count = 3;
    
    /* Poll item 2: Read 2 input registers from address 10 */
    s_poll_list[2].enabled = true;
    s_poll_list[2].slave_address = 1;
    s_poll_list[2].data_type = MODBUS_DATA_TYPE_INPUT_REGISTER;
    s_poll_list[2].operation = MODBUS_OP_READ;
    s_poll_list[2].start_address = 10;
    s_poll_list[2].count = 2;
    
    LOG_INFO("Modbus RTU: initialized test poll list with %d items", 3);
}

/**
 * @brief Process a single poll item
 * 
 * Reads or writes the specified Modbus registers/coils and stores the result
 * in the poll item structure.
 * 
 * @param nmbs      Pointer to nanoMODBUS client instance
 * @param item      Pointer to poll item to process
 */
static void modbus_rtu_process_poll_item(nmbs_t *nmbs, modbus_rtu_poll_item_t *item)
{
    if (nmbs == NULL || item == NULL || !item->enabled)
    {
        return;
    }
    
    /* Set destination address for this item */
    nmbs_set_destination_rtu_address(nmbs, item->slave_address);
    
    nmbs_error err = NMBS_ERROR_NONE;
    
    /* Process based on data type and operation */
    if (item->operation == MODBUS_OP_READ)
    {
        switch (item->data_type)
        {
            case MODBUS_DATA_TYPE_COIL:
            {
                /* Read coils into bitfield, then convert to byte array */
                nmbs_bitfield coils = {0};
                err = nmbs_read_coils(nmbs, item->start_address, item->count, coils);
                if (err == NMBS_ERROR_NONE)
                {
                    for (uint16_t i = 0; i < item->count && i < MODBUS_RTU_MAX_REG_COUNT; i++)
                    {
                        item->data.coils[i] = nmbs_bitfield_read(coils, i) ? 1 : 0;
                    }
                }
                break;
            }
            
            case MODBUS_DATA_TYPE_DISCRETE_INPUT:
            {
                nmbs_bitfield inputs = {0};
                err = nmbs_read_discrete_inputs(nmbs, item->start_address, item->count, inputs);
                if (err == NMBS_ERROR_NONE)
                {
                    for (uint16_t i = 0; i < item->count && i < MODBUS_RTU_MAX_REG_COUNT; i++)
                    {
                        item->data.coils[i] = nmbs_bitfield_read(inputs, i) ? 1 : 0;
                    }
                }
                break;
            }
            
            case MODBUS_DATA_TYPE_INPUT_REGISTER:
            {
                err = nmbs_read_input_registers(nmbs, item->start_address, 
                                               item->count, item->data.registers);
                break;
            }
            
            case MODBUS_DATA_TYPE_HOLDING_REGISTER:
            {
                err = nmbs_read_holding_registers(nmbs, item->start_address,
                                                 item->count, item->data.registers);
                break;
            }
            
            default:
                err = NMBS_ERROR_INVALID_ARGUMENT;
                break;
        }
    }
    else  /* MODBUS_OP_WRITE */
    {
        /* Write operations (for future implementation) */
        switch (item->data_type)
        {
            case MODBUS_DATA_TYPE_COIL:
            {
                nmbs_bitfield coils = {0};
                for (uint16_t i = 0; i < item->count && i < MODBUS_RTU_MAX_REG_COUNT; i++)
                {
                    nmbs_bitfield_write(coils, i, item->data.coils[i]);
                }
                err = nmbs_write_multiple_coils(nmbs, item->start_address, item->count, coils);
                break;
            }
            
            case MODBUS_DATA_TYPE_HOLDING_REGISTER:
            {
                err = nmbs_write_multiple_registers(nmbs, item->start_address,
                                                   item->count, item->data.registers);
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
    
    /* Store result */
    item->last_error = err;
    item->last_poll_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (err != NMBS_ERROR_NONE)
    {
        LOG_DEBUG("Modbus %s failed: slave=%u addr=%u count=%u err=%d",
                 item->operation == MODBUS_OP_READ ? "read" : "write",
                 item->slave_address, item->start_address, item->count, err);
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

    /* Create mutex for poll list access */
    s_poll_list_mutex = xSemaphoreCreateMutex();
    if (s_poll_list_mutex == NULL)
    {
        LOG_ERROR("Failed to create poll list mutex - task will exit");
        vTaskDelete(NULL);
        return;
    }

    /* Initialize test poll list with demo data */
    modbus_rtu_init_test_poll_list();

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
        vSemaphoreDelete(s_poll_list_mutex);
        s_poll_list_mutex = NULL;
        vTaskDelete(NULL);
        return;
    }

    nmbs_set_read_timeout(&nmbs, MODBUS_RTU_READ_TIMEOUT_MS);
    nmbs_set_byte_timeout(&nmbs, MODBUS_RTU_BYTE_TIMEOUT_MS);

    LOG_INFO("Modbus RTU client ready, starting poll loop");

    /* Main polling loop */
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(MODBUS_RTU_TASK_LOOP_DELAY_MS));

        /* Take mutex to access poll list */
        if (xSemaphoreTake(s_poll_list_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        {
            LOG_WARN("Failed to take poll list mutex");
            continue;
        }

        /* Process all enabled poll items */
        for (uint8_t i = 0; i < MODBUS_RTU_POLL_LIST_SIZE; i++)
        {
            if (s_poll_list[i].enabled)
            {
                modbus_rtu_process_poll_item(&nmbs, &s_poll_list[i]);
                
                /* Log successful reads for debugging */
                if (s_poll_list[i].last_error == NMBS_ERROR_NONE && 
                    s_poll_list[i].operation == MODBUS_OP_READ)
                {
                    if (s_poll_list[i].data_type == MODBUS_DATA_TYPE_HOLDING_REGISTER ||
                        s_poll_list[i].data_type == MODBUS_DATA_TYPE_INPUT_REGISTER)
                    {
                        LOG_DEBUG("Poll[%u]: slave=%u addr=%u regs=[%u, %u]", 
                                 i, s_poll_list[i].slave_address, 
                                 s_poll_list[i].start_address,
                                 s_poll_list[i].data.registers[0],
                                 s_poll_list[i].count > 1 ? s_poll_list[i].data.registers[1] : 0);
                    }
                    else
                    {
                        LOG_DEBUG("Poll[%u]: slave=%u addr=%u coils=[%u, %u, %u]",
                                 i, s_poll_list[i].slave_address,
                                 s_poll_list[i].start_address,
                                 s_poll_list[i].data.coils[0],
                                 s_poll_list[i].count > 1 ? s_poll_list[i].data.coils[1] : 0,
                                 s_poll_list[i].count > 2 ? s_poll_list[i].data.coils[2] : 0);
                    }
                }
            }
        }

        xSemaphoreGive(s_poll_list_mutex);
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
