#include <FreeRTOS.h>
#include <task.h>
#include <FreeRTOS_CLI.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "cli_task.h"
#include "pico_flash_storage.h"

// Definition of the task priority and stack size
#define cliTASK_PRIORITY        ( tskIDLE_PRIORITY + 2 )
#define cliTASK_STACK_SIZE      ( configMINIMAL_STACK_SIZE * 4 )  // 512 words = 2KB for CLI operations

// Command buffer size
#define MAX_INPUT_LENGTH        50
#define MAX_OUTPUT_LENGTH       configCOMMAND_INT_MAX_OUTPUT_SIZE

// Global buffers
static char cInputString[MAX_INPUT_LENGTH];
static char cOutputString[MAX_OUTPUT_LENGTH];

// Task function
static void vCLITask(void *pvParameters)
{
    (void) pvParameters;
    int cRxedChar;
    int cInputIndex = 0;
    BaseType_t xMoreDataToFollow;
    
    // Adaptive polling configuration
    #define POLL_FAST_MS    1      // Fast polling during data reception
    #define POLL_SLOW_MS    100    // Slow polling when idle
    #define IDLE_THRESHOLD  500    // Number of idle loops before slowing down (100ms at 1ms = 100ms idle)
    
    int poll_delay_ms = POLL_SLOW_MS;  // Start with slow polling
    int idle_counter = 0;

    printf("\n\nFreeRTOS CLI task started.\nType 'help' to view a list of registered commands.\n\n> ");

    while (1)
    {
        // Read character from stdin (UART) - use timeout to prevent blocking other tasks on the same core
        cRxedChar = getchar_timeout_us(0);

        if (cRxedChar == '\r' || cRxedChar == '\n')
        {
            printf("\n");

            // Process command only if buffer is not empty
            if (cInputIndex > 0)
            {
                // Terminate the string
                cInputString[cInputIndex] = '\0';

                // Process the command
                do
                {
                    xMoreDataToFollow = FreeRTOS_CLIProcessCommand(cInputString, cOutputString, MAX_OUTPUT_LENGTH);
                    printf("%s", cOutputString);
                } while (xMoreDataToFollow != pdFALSE);

                // Clear input buffer
                cInputIndex = 0;
                memset(cInputString, 0, MAX_INPUT_LENGTH);
            }

            printf("\n> ");
        }
        else if (cRxedChar == PICO_ERROR_TIMEOUT) 
        {
             // No character received, yield to allow other tasks to run
             vTaskDelay(pdMS_TO_TICKS(poll_delay_ms));

             // Switch to slow polling after idle period (only when in fast mode)
             if (poll_delay_ms == POLL_FAST_MS) {
                 idle_counter++;
                 if (idle_counter >= IDLE_THRESHOLD) {
                     poll_delay_ms = POLL_SLOW_MS;
                     idle_counter = 0;
                 }
             }
        }
        else
        {
            if (cRxedChar == 8 || cRxedChar == 127) // Backspace
            {
                if (cInputIndex > 0)
                {
                    cInputIndex--;
                    printf("\b \b");
                }
            }
            else if (cInputIndex < MAX_INPUT_LENGTH - 1)
            {
                // Echo character
                putchar(cRxedChar);
                cInputString[cInputIndex] = (char) cRxedChar;
                cInputIndex++;
            }
            
            // Data received - switch to fast polling for responsive input
            if (poll_delay_ms != POLL_FAST_MS) {
                poll_delay_ms = POLL_FAST_MS;
            }
            idle_counter = 0;  // Reset idle counter
        }
    }
}

// Example command implementation
static BaseType_t prvTaskStatsCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    const char *const pcHeader = "Task          State  Priority  Stack	#\r\n************************************************\r\n";
    
    // Remove warning about unused parameters
    (void) pcCommandString;

    // Ensure buffer is large enough for header
    if (xWriteBufferLen < strlen(pcHeader) + 1)
    {
        return pdFALSE;
    }

    snprintf(pcWriteBuffer, xWriteBufferLen, "%s", pcHeader);
    #if (configUSE_TRACE_FACILITY == 1) && (configUSE_STATS_FORMATTING_FUNCTIONS > 0)
        size_t xHeaderLen = strlen(pcHeader);
        // Use vTaskListTasks with explicit remaining buffer size to prevent overflow
        // vTaskList macro uses full configSTATS_BUFFER_MAX_LENGTH which causes overflow
        vTaskListTasks(pcWriteBuffer + xHeaderLen, xWriteBufferLen - xHeaderLen);
    #else
        snprintf(pcWriteBuffer, xWriteBufferLen, "Trace facility not enabled in FreeRTOSConfig.h\r\n");
    #endif
    return pdFALSE;
}

static const CLI_Command_Definition_t xTaskStats =
{
    "task-stats",
    "\r\ntask-stats:\r\n Displays a table showing the state of each FreeRTOS task\r\n",
    prvTaskStatsCommand,
    0
};

static BaseType_t prvRebootCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    (void) pcCommandString;
    
    snprintf(pcWriteBuffer, xWriteBufferLen, "Rebooting...\r\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Reboot the device
    watchdog_reboot(0, 0, 0);
    
    return pdFALSE;
}

static const CLI_Command_Definition_t xReboot =
{
    "reboot",
    "\r\nreboot:\r\n Reboot the device\r\n",
    prvRebootCommand,
    0
};

// --- Flash Test Command ---
static BaseType_t prvFlashTestCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    const char *pcParameter;
    BaseType_t xParameterStringLength;
    // Use last 64KB of 2MB flash for persistent config (safe from firmware updates)
    // For 4MB flash, use 0x3F0000 instead
    uint32_t offset = 0x1F0000; // Last 64KB block (1,984KB offset)
    char buffer[256];

    // Get the first parameter (operation: read/write)
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);

    if (pcParameter == NULL)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Usage: flash-test <write/read> [data]\r\n");
        return pdFALSE;
    }

    if (strncmp(pcParameter, "write", xParameterStringLength) == 0)
    {
        // Get second parameter (data)
        pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);
        if (pcParameter == NULL)
        {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Usage: flash-test write <data>\r\n");
            return pdFALSE;
        }

        // Copy data to buffer (max 255 chars)
        size_t len = (xParameterStringLength > 255) ? 255 : xParameterStringLength;
        memcpy(buffer, pcParameter, len);
        buffer[len] = '\0';

        if (flash_storage_write(offset, (uint8_t *)buffer, len + 1))
        {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Written to flash at offset 0x%x: %s\r\n", (unsigned int)offset, buffer);
        }
        else
        {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Failed to write to flash.\r\n");
        }
    }
    else if (strncmp(pcParameter, "read", xParameterStringLength) == 0)
    {
        flash_storage_read(offset, (uint8_t *)buffer, 256);
        // Ensure null termination for printing safety
        buffer[255] = '\0'; 
        snprintf(pcWriteBuffer, xWriteBufferLen, "Read from flash at offset 0x%x: %s\r\n", (unsigned int)offset, buffer);
    }
    else
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Unknown operation.\r\n");
    }

    return pdFALSE;
}

static const CLI_Command_Definition_t xFlashTest =
{
    "flash-test",
    "\r\nflash-test <write/read> [data]:\r\n Write or read a string from flash memory\r\n",
    prvFlashTestCommand,
    -1 // Variable number of parameters
};

void vCreateCLITask(void)
{
    // Register commands
    FreeRTOS_CLIRegisterCommand(&xTaskStats);
    FreeRTOS_CLIRegisterCommand(&xReboot);
    FreeRTOS_CLIRegisterCommand(&xFlashTest);

    // Create the task
    xTaskCreate(vCLITask, "CLI_Task", cliTASK_STACK_SIZE, NULL, cliTASK_PRIORITY, NULL);
}

