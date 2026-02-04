/**
 * @file pico_flash_storage.c
 * @brief Safe flash storage operations for SMP FreeRTOS
 * 
 * Implements dual-core coordination for safe flash writes on RP2350.
 * Uses a coordinator task on Core 0 and a guard task on Core 1 to ensure
 * both cores are in RAM with interrupts disabled during flash operations.
 */

#include "pico_flash_storage.h"

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

/**
 * @brief Command structure for flash coordinator task
 */
typedef struct {
    uint32_t offset;
    const uint8_t *data;
    size_t size;
    SemaphoreHandle_t completion_sem;
    bool *success_ptr;
} flash_cmd_t;

// Flash operation queue and task handles
static QueueHandle_t s_flash_queue = NULL;
static TaskHandle_t s_coordinator_task = NULL;
static TaskHandle_t s_guard_task = NULL;

/**
 * @brief Guard task synchronization states
 */
typedef enum {
    GUARD_IDLE,
    GUARD_WAITING_FOR_CMD,
    GUARD_EXECUTING,
    GUARD_DONE
} guard_state_t;

static volatile guard_state_t g_guard_state = GUARD_IDLE;

/**
 * @brief Guard task busy-wait loop (must execute from RAM)
 * 
 * This function runs on Core 1 during flash operations.
 * It must be in RAM (__not_in_flash_func) to avoid accessing flash while it's being written.
 */
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

/**
 * @brief Flash Guard Task (runs on Core 1)
 * 
 * Waits for notifications from coordinator, disables interrupts,
 * and busy-waits in RAM while coordinator performs flash operations.
 */
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

/**
 * @brief Flash Coordinator Task (runs on Core 0)
 * 
 * Receives flash write commands from queue, coordinates with guard task,
 * and performs the actual flash erase/program operations.
 */
static void vFlashCoordinatorTask(void *pvParameters) {
    (void)pvParameters;
    flash_cmd_t cmd;
    
    while (1) {
        if (xQueueReceive(s_flash_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            
            // Calculate sizes
            uint32_t sector_start = cmd.offset & ~(FLASH_SECTOR_SIZE - 1);
            if (cmd.offset != sector_start) {
                if (cmd.success_ptr) *cmd.success_ptr = false;
                xSemaphoreGive(cmd.completion_sem);
                continue;
            }
            uint32_t prog_size = (cmd.size + FLASH_PAGE_SIZE - 1) & ~(FLASH_PAGE_SIZE - 1);

            // Signal Guard Task on Core 1
            xTaskNotifyGive(s_guard_task);
            
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

/**
 * @brief Initialize the flash storage system
 * 
 * Creates coordination tasks on both cores for safe flash operations.
 * 
 * @return true if initialization successful, false if already initialized
 */
bool flash_storage_init(void) {
    // Already initialized
    if (s_flash_queue != NULL) {
        return true;
    }
    
    // Create command queue
    s_flash_queue = xQueueCreate(1, sizeof(flash_cmd_t));
    if (s_flash_queue == NULL) {
        return false;
    }
    
    // Create Coordinator on Core 0 (highest priority)
    BaseType_t result = xTaskCreate(vFlashCoordinatorTask, "FlashCoord", 
                                     configMINIMAL_STACK_SIZE * 2, NULL, 
                                     (configMAX_PRIORITIES - 1), &s_coordinator_task);
    if (result != pdPASS) {
        vQueueDelete(s_flash_queue);
        s_flash_queue = NULL;
        return false;
    }
    vTaskCoreAffinitySet(s_coordinator_task, (1 << 0));
    
    // Create Guard on Core 1 (highest priority to preempt anything)
    result = xTaskCreate(vFlashGuardTask, "FlashGuard", 
                        configMINIMAL_STACK_SIZE * 2, NULL, 
                        (configMAX_PRIORITIES - 1), &s_guard_task);
    if (result != pdPASS) {
        vTaskDelete(s_coordinator_task);
        vQueueDelete(s_flash_queue);
        s_coordinator_task = NULL;
        s_flash_queue = NULL;
        return false;
    }
    vTaskCoreAffinitySet(s_guard_task, (1 << 1));
    
    return true;
}

/**
 * @brief Read data from flash memory
 * 
 * Flash is memory-mapped, so this is a simple memcpy operation.
 * 
 * @param offset Offset from start of flash
 * @param data Output buffer
 * @param size Number of bytes to read
 */
void flash_storage_read(uint32_t offset, uint8_t *data, size_t size)
{
    // Flash is memory mapped at XIP_BASE (0x10000000)
    const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + offset);
    memcpy(data, flash_target_contents, size);
}

/**
 * @brief Write data to flash memory safely
 * 
 * Coordinates with both cores to ensure safe flash write operation.
 * Blocks until operation completes.
 * 
 * @param offset Offset from start of flash (must be sector-aligned)
 * @param data Data to write
 * @param size Number of bytes to write (will be rounded up to page size)
 * @return true on success, false on error
 */
bool flash_storage_write(uint32_t offset, const uint8_t *data, size_t size) {
    if (s_flash_queue == NULL) return false;
    
    flash_cmd_t cmd;
    cmd.offset = offset;
    cmd.data = data;
    cmd.size = size;
    cmd.completion_sem = xSemaphoreCreateBinary();
    bool success = false;
    cmd.success_ptr = &success;
    
    if (xQueueSend(s_flash_queue, &cmd, portMAX_DELAY) != pdTRUE) {
        vSemaphoreDelete(cmd.completion_sem);
        return false;
    }
    
    // Wait for completion
    xSemaphoreTake(cmd.completion_sem, portMAX_DELAY);
    vSemaphoreDelete(cmd.completion_sem);
    
    return success;
}
