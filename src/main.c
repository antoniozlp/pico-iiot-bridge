#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"


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


int main()
{
    stdio_init_all();

    sleep_ms(100);

    printf("Starting ...\n");

    xTaskCreate(vBlinkLedDemoTask, "Blink Task", 128, NULL, 1, NULL);

    vTaskStartScheduler();

    return 0;
}
