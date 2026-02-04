/**
 * @file logger.h
 * @brief Thread-safe logging system for FreeRTOS
 * 
 * This module provides a centralized, queue-based logging system that
 * ensures thread-safe access to stdio (UART/USB) from multiple FreeRTOS tasks.
 * 
 * Features:
 * - Multiple log levels (ERROR, WARNING, INFO, DEBUG)
 * - Non-blocking logging from any task
 * - Optional timestamps and task names
 * - Filterable log levels at runtime
 * - Centralized hardware access (prevents UART corruption)
 * 
 * Usage:
 *   LOG_ERROR("Task failed: error code %d", error);
 *   LOG_WARN("Buffer nearly full: %d%%", percentage);
 *   LOG_INFO("Connection established");
 *   LOG_DEBUG("Value = 0x%08X", register_value);
 */

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

/**
 * @brief Log levels in order of severity
 */
typedef enum {
    LOG_LEVEL_ERROR = 0,    // Critical errors that need immediate attention
    LOG_LEVEL_WARN  = 1,    // Warnings that should be investigated
    LOG_LEVEL_INFO  = 2,    // General information messages
    LOG_LEVEL_DEBUG = 3,    // Detailed debug information
    LOG_LEVEL_MAX
} log_level_t;

/**
 * @brief Logger configuration
 */
typedef struct {
    log_level_t filter_level;      // Only log messages at or above this level
    bool include_timestamp;         // Add timestamp to log messages
    bool include_task_name;         // Add task name to log messages
    bool use_colors;                // Use ANSI colors for different log levels
} logger_config_t;

/**
 * @brief Initialize the logger system
 * 
 * This must be called before using any logging functions.
 * Creates the logger task and message queue.
 * 
 * @param config Logger configuration (NULL for defaults)
 * @return true on success, false on failure
 */
bool logger_init(const logger_config_t *config);

/**
 * @brief Set the minimum log level filter
 * 
 * Messages below this level will be discarded.
 * 
 * @param level Minimum log level to display
 */
void logger_set_level(log_level_t level);

/**
 * @brief Get the current log level filter
 * 
 * @return Current minimum log level
 */
log_level_t logger_get_level(void);

/**
 * @brief Enable/disable timestamps in log messages
 * 
 * @param enable true to enable timestamps, false to disable
 */
void logger_set_timestamp(bool enable);

/**
 * @brief Enable/disable task names in log messages
 * 
 * @param enable true to enable task names, false to disable
 */
void logger_set_task_name(bool enable);

/**
 * @brief Lock stdio for exclusive access (blocks until available)
 * 
 * Use this to prevent logger output from interfering with interactive I/O.
 * Always pair with logger_unlock_stdio().
 * 
 * Example:
 *   logger_lock_stdio();
 *   printf("User prompt: ");
 *   // ... user interaction ...
 *   logger_unlock_stdio();
 * 
 * @return true on success, false if logger not initialized
 */
bool logger_lock_stdio(void);

/**
 * @brief Unlock stdio to allow logger output
 * 
 * @return true on success, false if logger not initialized
 */
bool logger_unlock_stdio(void);

/**
 * @brief Log a message (internal function, use macros instead)
 * 
 * @param level Log level
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void logger_log(log_level_t level, const char *format, ...);

/**
 * @brief Log a message with va_list (internal function)
 * 
 * @param level Log level
 * @param format Printf-style format string
 * @param args Variable arguments list
 */
void logger_log_va(log_level_t level, const char *format, va_list args);

// Convenient logging macros
#define LOG_ERROR(fmt, ...) logger_log(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  logger_log(LOG_LEVEL_WARN,  fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  logger_log(LOG_LEVEL_INFO,  fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) logger_log(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

// Raw output (bypasses logging system, uses printf directly - use sparingly)
#define LOG_RAW(fmt, ...)   printf(fmt, ##__VA_ARGS__)

#endif // _LOGGER_H_
