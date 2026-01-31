#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cli_task.h"
#include "pico_flash_storage.h"
#include "system_config.h"
#include "board_config.h"
#include "serial_to_tcp.h"
#include "logger.h"
#include "network_task.h"
#include "http_server_task.h"

int main()
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
    flash_storage_init();
    
    // Load configuration from flash
    if (config_load_from_flash())
    {
        LOG_INFO("Configuration loaded from flash");
    }
    else
    {
        LOG_WARN("Using default configuration");
    }

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

    LOG_INFO("Starting FreeRTOS scheduler...");
    vTaskStartScheduler();

    // Should never reach here
    LOG_ERROR("FreeRTOS scheduler failed to start!");

    return 0;
}

// FreeRTOS stack overflow hook - called when stack overflow is detected
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
