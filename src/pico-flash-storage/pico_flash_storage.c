#include "pico_flash_storage.h"

#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

// Command structure for the Flash Coordinator
typedef struct {
    uint32_t offset;
    const uint8_t *data;
    size_t size;
    SemaphoreHandle_t completion_sem;
    bool *success_ptr;
} flash_cmd_t;

static QueueHandle_t xFlashQueue = NULL;
static TaskHandle_t xCoordinatorTask = NULL;
static TaskHandle_t xGuardTask = NULL;

// Synchronization state
typedef enum {
    GUARD_IDLE,
    GUARD_WAITING_FOR_CMD,
    GUARD_EXECUTING,
    GUARD_DONE
} guard_state_t;

static volatile guard_state_t g_guard_state = GUARD_IDLE;

// RAM busy loop for the Guard Task
// Must not be in flash!
static void __not_in_flash_func(prvGuardWaitLoop)(void) {
    // Wait for Coordinator to start execution
    while (g_guard_state == GUARD_WAITING_FOR_CMD) {
        __asm volatile ("nop");
    }
    // Wait for Coordinator to finish
    while (g_guard_state == GUARD_EXECUTING) {
        __asm volatile ("nop");
    }
}

// Guard Task (Core 1)
static void vFlashGuardTask(void *pvParameters) {
    (void)pvParameters;
    
    while (1) {
        // Wait for signal from Coordinator
        // Using a loop to ensure we really block
        while (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == 0) {
            // Spurious wake-up?
            // printf("FlashGuard spurious wake up\n");
        }
        
        // Disable interrupts on this core
        uint32_t interrupts = save_and_disable_interrupts();
        
        // Signal ready
        g_guard_state = GUARD_WAITING_FOR_CMD;
        
        // Enter RAM loop
        prvGuardWaitLoop();
        
        // Reset state
        g_guard_state = GUARD_IDLE;
        
        // Re-enable interrupts
        restore_interrupts(interrupts);
    }
}

// Coordinator Task (Core 0)
static void vFlashCoordinatorTask(void *pvParameters) {
    (void)pvParameters;
    flash_cmd_t cmd;
    
    while (1) {
        if (xQueueReceive(xFlashQueue, &cmd, portMAX_DELAY) == pdTRUE) {
            
            // Calculate sizes
            uint32_t sector_start = cmd.offset & ~(FLASH_SECTOR_SIZE - 1);
            if (cmd.offset != sector_start) {
                if (cmd.success_ptr) *cmd.success_ptr = false;
                xSemaphoreGive(cmd.completion_sem);
                continue;
            }
            uint32_t prog_size = (cmd.size + FLASH_PAGE_SIZE - 1) & ~(FLASH_PAGE_SIZE - 1);

            // Signal Guard Task on Core 1
            xTaskNotifyGive(xGuardTask);
            
            // Wait for Guard to be ready (spinning in RAM with IRQs disabled)
            while (g_guard_state != GUARD_WAITING_FOR_CMD) {
                taskYIELD();
            }
            
            // Disable interrupts on this core
            uint32_t interrupts = save_and_disable_interrupts();
            
            // Signal Guard to enter the executing phase
            g_guard_state = GUARD_EXECUTING;
            
            // --- CRITICAL SECTION: Both cores are IRQ disabled and in RAM ---
            
            flash_range_erase(cmd.offset, FLASH_SECTOR_SIZE);
            flash_range_program(cmd.offset, cmd.data, prog_size);
            
            // ---------------------------------------------------------------
            
            // Signal Guard that we are done
            g_guard_state = GUARD_DONE;
            
            // Re-enable interrupts
            restore_interrupts(interrupts);
            
            // Result
            if (cmd.success_ptr) *cmd.success_ptr = true;
            xSemaphoreGive(cmd.completion_sem);
        }
    }
}

void flash_storage_init(void) {
    if (xFlashQueue != NULL) return;
    
    xFlashQueue = xQueueCreate(1, sizeof(flash_cmd_t));
    
    // Create Coordinator on Core 0
    xTaskCreate(vFlashCoordinatorTask, "FlashCoord", configMINIMAL_STACK_SIZE * 2, NULL, (configMAX_PRIORITIES - 1), &xCoordinatorTask);
    vTaskCoreAffinitySet(xCoordinatorTask, (1 << 0));
    
    // Create Guard on Core 1
    // Must be high priority to preempt anything else on Core 1
    xTaskCreate(vFlashGuardTask, "FlashGuard", configMINIMAL_STACK_SIZE * 2, NULL, (configMAX_PRIORITIES - 1), &xGuardTask);
    vTaskCoreAffinitySet(xGuardTask, (1 << 1));
}

void flash_storage_read(uint32_t offset, uint8_t *data, size_t size)
{
    // Flash is memory mapped, so we can just read it directly.
    // XIP_BASE is the starting address of flash in memory map (0x10000000)
    const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + offset);
    memcpy(data, flash_target_contents, size);
}

bool flash_storage_write(uint32_t offset, const uint8_t *data, size_t size) {
    if (xFlashQueue == NULL) return false;
    
    flash_cmd_t cmd;
    cmd.offset = offset;
    cmd.data = data;
    cmd.size = size;
    cmd.completion_sem = xSemaphoreCreateBinary();
    bool success = false;
    cmd.success_ptr = &success;
    
    if (xQueueSend(xFlashQueue, &cmd, portMAX_DELAY) != pdTRUE) {
        vSemaphoreDelete(cmd.completion_sem);
        return false;
    }
    
    // Wait for completion
    xSemaphoreTake(cmd.completion_sem, portMAX_DELAY);
    vSemaphoreDelete(cmd.completion_sem);
    
    return success;
}
