/**
 * @file logger.c
 * @brief Logger implementation
 */

#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "pico/stdlib.h"

// Logger configuration
#define LOGGER_TASK_PRIORITY        (tskIDLE_PRIORITY + 1)  // Low priority
#define LOGGER_TASK_STACK_SIZE      (configMINIMAL_STACK_SIZE * 4)  // 2KB stack
#define LOGGER_QUEUE_LENGTH         32  // Number of log messages that can be queued
#define LOGGER_MESSAGE_MAX_LENGTH   128 // Maximum length of a single log message

/**
 * @brief Timeout for sending messages to the queue (in milliseconds)
 * 
 * If the queue is full, the logger will wait up to this amount of time
 * for space to become available. If timeout expires, a warning is printed.
 * 
 * - 0: Non-blocking (immediate drop if full) - default for real-time performance
 * - 1-10: Brief wait (reduces drops without blocking too long) 
 * - portMAX_DELAY: Block indefinitely (not recommended)
 */
#define LOGGER_QUEUE_SEND_TIMEOUT_MS 5

// ANSI color codes for terminal output
#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_YELLOW   "\x1b[33m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_RESET    "\x1b[0m"

/**
 * @brief Log message structure
 */
typedef struct {
    log_level_t level;
    uint32_t timestamp_ms;
    char task_name[configMAX_TASK_NAME_LEN];
    char message[LOGGER_MESSAGE_MAX_LENGTH];
} log_message_t;

// Logger state
static QueueHandle_t s_log_queue = NULL;
static logger_config_t s_config = {
    .filter_level = LOG_LEVEL_INFO,
    .include_timestamp = true,
    .include_task_name = true,
    .use_colors = true
};
static bool s_logger_initialized = false;

/**
 * @brief Get log level name string
 */
static const char* get_level_name(log_level_t level)
{
    switch (level) {
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_WARN:  return "WARN ";
        case LOG_LEVEL_INFO:  return "INFO ";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        default:              return "?????";
    }
}

/**
 * @brief Get ANSI color code for log level
 */
static const char* get_level_color(log_level_t level)
{
    if (!s_config.use_colors) {
        return "";
    }
    
    switch (level) {
        case LOG_LEVEL_ERROR: return ANSI_COLOR_RED;
        case LOG_LEVEL_WARN:  return ANSI_COLOR_YELLOW;
        case LOG_LEVEL_INFO:  return ANSI_COLOR_GREEN;
        case LOG_LEVEL_DEBUG: return ANSI_COLOR_CYAN;
        default:              return "";
    }
}

/**
 * @brief Logger task - handles all stdio output
 */
static void vLoggerTask(void *pvParameters)
{
    (void)pvParameters;
    log_message_t msg;
    
    printf("[LOGGER] Task started\n");
    
    while (1)
    {
        // Wait for log messages
        if (xQueueReceive(s_log_queue, &msg, portMAX_DELAY) == pdTRUE)
        {
            // Build the log line
            const char *color = get_level_color(msg.level);
            const char *reset = s_config.use_colors ? ANSI_COLOR_RESET : "";
            
            // Print timestamp if enabled
            if (s_config.include_timestamp)
            {
                uint32_t seconds = msg.timestamp_ms / 1000;
                uint32_t ms = msg.timestamp_ms % 1000;
                printf("[%5u.%03u] ", seconds, ms);
            }
            
            // Print log level with color
            printf("%s[%s]%s ", color, get_level_name(msg.level), reset);
            
            // Print task name if enabled
            if (s_config.include_task_name && msg.task_name[0] != '\0')
            {
                printf("[%-16s] ", msg.task_name);
            }
            
            // Print the actual message
            printf("%s\n", msg.message);
        }
    }
}

/**
 * @brief Initialize the logger
 */
bool logger_init(const logger_config_t *config)
{
    if (s_logger_initialized)
    {
        printf("[LOGGER] Warning: Already initialized\n");
        return true;
    }
    
    // Apply configuration
    if (config != NULL)
    {
        memcpy(&s_config, config, sizeof(logger_config_t));
    }
    
    // Create the log message queue
    s_log_queue = xQueueCreate(LOGGER_QUEUE_LENGTH, sizeof(log_message_t));
    if (s_log_queue == NULL)
    {
        printf("[LOGGER] ERROR: Failed to create queue\n");
        return false;
    }
    
    // Create the logger task
    BaseType_t result = xTaskCreate(
        vLoggerTask,
        "Logger",
        LOGGER_TASK_STACK_SIZE,
        NULL,
        LOGGER_TASK_PRIORITY,
        NULL
    );
    
    if (result != pdPASS)
    {
        printf("[LOGGER] ERROR: Failed to create task\n");
        vQueueDelete(s_log_queue);
        s_log_queue = NULL;
        return false;
    }
    
    s_logger_initialized = true;
    printf("[LOGGER] Initialized successfully (level=%s)\n", 
           get_level_name(s_config.filter_level));
    
    return true;
}

/**
 * @brief Set log level filter
 */
void logger_set_level(log_level_t level)
{
    if (level < LOG_LEVEL_MAX)
    {
        s_config.filter_level = level;
    }
}

/**
 * @brief Get log level filter
 */
log_level_t logger_get_level(void)
{
    return s_config.filter_level;
}

/**
 * @brief Enable/disable timestamps
 */
void logger_set_timestamp(bool enable)
{
    s_config.include_timestamp = enable;
}

/**
 * @brief Enable/disable task names
 */
void logger_set_task_name(bool enable)
{
    s_config.include_task_name = enable;
}

/**
 * @brief Log a message (internal implementation)
 */
void logger_log_va(log_level_t level, const char *format, va_list args)
{
    // Check if logger is initialized
    if (!s_logger_initialized || s_log_queue == NULL)
    {
        // Fallback to direct printf if logger not ready
        vprintf(format, args);
        printf("\n");
        return;
    }
    
    // If scheduler hasn't started, use direct printf
    // (logger task won't be running to process queue)
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
    {
        const char *color = get_level_color(level);
        const char *reset = s_config.use_colors ? ANSI_COLOR_RESET : "";
        printf("%s[%s]%s ", color, get_level_name(level), reset);
        vprintf(format, args);
        printf("\n");
        return;
    }
    
    // Filter by log level
    if (level > s_config.filter_level)
    {
        return;  // Message filtered out
    }
    
    // Create log message
    log_message_t msg;
    msg.level = level;
    msg.timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Get task name if enabled and scheduler is running
    if (s_config.include_task_name)
    {
        // Check if scheduler is running (if not, we're in main() or ISR)
        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        {
            TaskHandle_t task = xTaskGetCurrentTaskHandle();
            if (task != NULL)
            {
                const char *name = pcTaskGetName(task);
                if (name != NULL)
                {
                    strncpy(msg.task_name, name, configMAX_TASK_NAME_LEN - 1);
                    msg.task_name[configMAX_TASK_NAME_LEN - 1] = '\0';
                }
                else
                {
                    strncpy(msg.task_name, "???", configMAX_TASK_NAME_LEN - 1);
                }
            }
            else
            {
                strncpy(msg.task_name, "???", configMAX_TASK_NAME_LEN - 1);
            }
        }
        else
        {
            // Scheduler not running - we're in main() before scheduler starts
            strncpy(msg.task_name, "main", configMAX_TASK_NAME_LEN - 1);
        }
    }
    else
    {
        msg.task_name[0] = '\0';
    }
    
    // Format the message
    vsnprintf(msg.message, LOGGER_MESSAGE_MAX_LENGTH, format, args);
    msg.message[LOGGER_MESSAGE_MAX_LENGTH - 1] = '\0';  // Ensure null termination
    
    // Send to logger task (with configurable timeout)
    if (xQueueSend(s_log_queue, &msg, pdMS_TO_TICKS(LOGGER_QUEUE_SEND_TIMEOUT_MS)) != pdTRUE)
    {
        // Queue is full - print warning directly (avoid recursion with LOG_ERROR)
        const char *color = get_level_color(LOG_LEVEL_WARN);
        const char *reset = s_config.use_colors ? ANSI_COLOR_RESET : "";
        printf("%s[WARN ]%s Logger queue full, blocking until space available...\n", 
               color, reset);
        
        // Retry with blocking send - wait indefinitely for queue space
        xQueueSend(s_log_queue, &msg, portMAX_DELAY);
    }
}

/**
 * @brief Log a message (public API)
 */
void logger_log(log_level_t level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logger_log_va(level, format, args);
    va_end(args);
}
