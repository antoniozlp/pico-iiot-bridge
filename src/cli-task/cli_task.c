#include <FreeRTOS.h>
#include <task.h>
#include <FreeRTOS_CLI.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "cli_task.h"

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

void vCreateCLITask(void)
{
    // Register commands
    FreeRTOS_CLIRegisterCommand(&xTaskStats);
    FreeRTOS_CLIRegisterCommand(&xReboot);

    // Create the task
    xTaskCreate(vCLITask, "CLI_Task", cliTASK_STACK_SIZE, NULL, cliTASK_PRIORITY, NULL);
}

