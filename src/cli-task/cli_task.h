#ifndef CLI_TASK_H
#define CLI_TASK_H

#include <stdbool.h>

/**
 * @brief Initialize and create the CLI FreeRTOS task
 * 
 * This function registers CLI commands and creates the CLI task on UART0.
 * 
 * @return true if task created successfully, false otherwise
 */
bool cli_task_init(void);

#endif /* CLI_TASK_H */

