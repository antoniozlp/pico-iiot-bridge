#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "wizchip_conf.h"
#include "wizchip_spi.h"
#include "loopback.h"
#include "cli_task.h"
#include "pico_flash_storage.h"
#include "config.h"
#include "http_utils.h"
#include "web_page.h"

#define LED_TOGGLE_RATE 100

// Blink the LED on the Pico using FreeRTOS Demo Task
void vBlinkLedDemoTask(void *pvParameters){
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while(1){
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(LED_TOGGLE_RATE));
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(LED_TOGGLE_RATE));
    }
}

// Global buffer for TCP server
static uint8_t g_tcp_server_buf[2048];


/* HTTP */

#define HTTP_SOCKET_MAX_NUM 2
static uint8_t g_http_send_buf[2048] = {
    0,
};
static uint8_t g_http_recv_buf[2048] = {
    0,
};
static uint8_t g_http_socket_num_list[HTTP_SOCKET_MAX_NUM] = {0, 1};



void tcp_loopback_task(void *pvParameters) {
    printf("TCP Loopback server started on port 5000\n");

    // IMPORTANT: Tasks must never return!
    while(1) {
        int response = loopback_tcps(3, g_tcp_server_buf, 5000);
        if (response < 0) {
            printf("Error: %d\n", response);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay

        /* Run HTTP server for testing purposes now */
        for (uint8_t i = 0; i < HTTP_SOCKET_MAX_NUM; i++) {
            httpServer_run(i);
        }
    }
}


int main()
{
    stdio_init_all();

    sleep_ms(1000);
    printf("Starting ...\n");
    
    // Initialize flash storage before any tasks that might write to flash
    flash_storage_init();
    
    // Load configuration from flash
    if (config_load_from_flash())
    {
        printf("Loaded configuration from flash successfully.\n");
    }
    else
    {
        printf("Using default configuration.\n");
    }

    // Initialize WizNet chip
    wizchip_spi_initialize();
    wizchip_cris_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    // Configure network
    wiz_NetInfo net_config;
    if (!config_get_net_info(&net_config))
    {
        printf("Error: Failed to get network configuration!\n");
        return 1;
    }
    network_initialize(net_config);
    print_network_information(net_config);

    sleep_ms(1000);

    /* Initialize HTTP Server */
    httpServer_init(g_http_send_buf, g_http_recv_buf, HTTP_SOCKET_MAX_NUM, g_http_socket_num_list);
    /* Register web page */
    reg_httpServer_webContent("index.html", index_page);


    // Create tasks
    xTaskCreate(vBlinkLedDemoTask, "Blink Task", 128, NULL, 1, NULL);
    xTaskCreate(tcp_loopback_task, "TCP Loopback Task", 1024, NULL, 2, NULL);  // 4KB for network operations
    vCreateCLITask();

    vTaskStartScheduler();

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
