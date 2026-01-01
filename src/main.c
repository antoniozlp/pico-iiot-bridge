#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "wizchip_conf.h"
#include "wizchip_spi.h"
#include "loopback.h"

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

void tcp_loopback_task(void *pvParameters) {
    printf("TCP Loopback server started on port 5000\n");

    // IMPORTANT: Tasks must never return!
    while(1) {
        int response = loopback_tcps(0, g_tcp_server_buf, 5000);
        if (response < 0) {
            printf("Error: %d\n", response);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay
    }
}


int main()
{
    stdio_init_all();

    // Initialize WizNet chip
    wizchip_spi_initialize();
    wizchip_cris_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    // Configure network
    wiz_NetInfo net_info = {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56},
        .ip = {192, 168, 11, 3},
        .sn = {255, 255, 255, 0},
        .gw = {192, 168, 11, 1},
        .dns = {8, 8, 8, 8},
        .dhcp = NETINFO_STATIC,
    };
    network_initialize(net_info);
    print_network_information(net_info);

    sleep_ms(1000);

    printf("Starting ...\n");

    xTaskCreate(vBlinkLedDemoTask, "Blink Task", 128, NULL, 1, NULL);
    xTaskCreate(tcp_loopback_task, "TCP Loopback Task", 512, NULL, 2, NULL);

    vTaskStartScheduler();

    return 0;
}
