/**
 * @file main.c
 * @brief Pico IIoT Bridge application entry point
 *
 * Initializes hardware, flash storage, configuration, and all FreeRTOS tasks
 * (network, HTTP server, CLI, serial-to-TCP). Failures during initialization
 * are logged and cause early exit with non-zero status.
 */

#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"

#include "board_config.h"
#include "cli_task.h"
#include "http_server_task.h"
#include "logger.h"
#include "network_task.h"
#include "pico_flash_storage.h"
#include "serial_to_tcp.h"
#include "system_config.h"
#include "modbus_rtu.h"
#include "tag_database.h"

/**
 * @brief Application entry point
 *
 * Initializes hardware, logger, flash storage, configuration, and all FreeRTOS
 * tasks. Returns non-zero on initialization failure; otherwise does not return
 * (scheduler runs).
 *
 * @return 1 on initialization failure, 0 only if scheduler fails to start
 */
int main(void)
{
    stdio_init_all();

    board_init_gpio();

    sleep_ms(1000);
    printf("\n===========================================\n");
    printf("  Pico I-IoT Bridge Starting...\n");
    printf("===========================================\n\n");
    
    
    if (!logger_init(NULL))
    {
        printf("ERROR: Failed to initialize logger!\n");
        return 1;
    }
    
    // Initialize flash storage before any tasks that might write to flash
    if (!flash_storage_init())
    {
        LOG_ERROR("Failed to initialize flash storage");
        return 1;
    }

    // Load configuration from flash
    if (config_load_from_flash())
    {
        LOG_INFO("Configuration loaded from flash");
    }
    else
    {
        LOG_WARN("Using default configuration");
    }

    // Initialize tag database (before protocol tasks)
    if (!tag_db_init())
    {
        LOG_ERROR("Failed to initialize tag database");
        return 1;
    }

    // Load tags from flash (if any exist)
    uint16_t loaded_count = tag_db_load_from_flash();
    

    // Initialize and create FreeRTOS tasks
    if (!network_task_init())
    {
        LOG_ERROR("Failed to initialize network task");
        return 1;
    }
    
    if (!http_server_task_init())
    {
        LOG_ERROR("Failed to initialize HTTP server task");
        return 1;
    }
    
    if (!cli_task_init())
    {
        LOG_ERROR("Failed to initialize CLI task");
        return 1;
    }
    
    if (!serial_to_tcp_task_init())
    {
        LOG_ERROR("Failed to initialize Serial-to-TCP task");
        return 1;
    }

    if (!modbus_rtu_task_init())
    {
        LOG_ERROR("Failed to initialize Modbus task");
        return 1;
    }

    LOG_INFO("Starting FreeRTOS scheduler...");
    vTaskStartScheduler();

    // Should never reach here
    LOG_ERROR("FreeRTOS scheduler failed to start!");

    return 0;
}

/**
 * @brief FreeRTOS stack overflow hook
 *
 * Called when a task stack overflow is detected. Disables interrupts, logs
 * the task name, and halts with LED blink.
 *
 * @param xTask Handle of the overflowing task
 * @param pcTaskName Name of the overflowing task
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    // Disable interrupts to prevent further corruption
    taskDISABLE_INTERRUPTS();
    
    // Print error message (if UART is still functional)
    printf("\n\n*** STACK OVERFLOW DETECTED ***\n");
    printf("Task: %s\n", pcTaskName);
    printf("System halted.\n");
    
    // Halt the system
    while(1) {
        // Blink LED rapidly to indicate error
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        busy_wait_us(100000);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        busy_wait_us(100000);
    }
}
