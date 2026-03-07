/**
 * @file cli_task.c
 * @brief Command Line Interface task implementation
 */

#include "cli_task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_CLI.h"

#include "pico/stdlib.h"
#include "hardware/watchdog.h"

#include "wizchip_conf.h"

#include "system_config.h"
#include "logger.h"

// Definition of the task priority and stack size
#define CLI_TASK_PRIORITY        ( tskIDLE_PRIORITY + 2 )
#define CLI_TASK_STACK_SIZE      ( configMINIMAL_STACK_SIZE * 4 )  // 512 words = 2KB for CLI operations

// Command buffer size
#define MAX_INPUT_LENGTH        50
#define MAX_OUTPUT_LENGTH       configCOMMAND_INT_MAX_OUTPUT_SIZE

// Adaptive polling configuration
#define POLL_FAST_MS    1      // Fast polling during data reception
#define POLL_SLOW_MS    100    // Slow polling when idle
#define IDLE_THRESHOLD  500    // Number of idle loops before slowing down (100ms at 1ms = 100ms idle)

// Static buffers for CLI input/output
static char s_input_buffer[MAX_INPUT_LENGTH];
static char s_output_buffer[MAX_OUTPUT_LENGTH];

// Tracks how many characters the user has typed on the current input line.
// File-scope so cli_redraw_input() can read it from the logger callback.
static int s_input_index = 0;

// Set to true once the CLI has printed its first prompt and is ready to
// accept input.  The logger redraw callback uses this to decide whether to
// repaint the prompt line after a log message.
static bool s_prompt_active = false;

/**
 * @brief Reprint the CLI prompt and the characters typed so far.
 *
 * Called by the logger task (while holding the stdio mutex) after each log
 * line so the prompt always stays at the bottom of the terminal output.
 * Must only use printf/putchar – must not call any logger functions.
 */
static void cli_redraw_input(void)
{
    if (s_prompt_active)
    {
        printf("> %.*s", s_input_index, s_input_buffer);
    }
}

/**
 * @brief CLI Task
 * 
 * Handles command-line interface input/output via UART.
 * Processes user commands using FreeRTOS-Plus-CLI.
 */
static void vCLITask(void *pvParameters)
{
    (void) pvParameters;
    int rxed_char;
    BaseType_t more_data_to_follow;
    
    int poll_delay_ms = POLL_SLOW_MS;  // Start with slow polling
    int idle_counter = 0;

    LOG_INFO("FreeRTOS CLI task started. Type 'help' to view a list of registered commands.");

    // Give the logger task a moment to flush the startup INFO message so the
    // initial prompt appears below it rather than mixed with it.
    vTaskDelay(pdMS_TO_TICKS(50));

    {
        bool locked = logger_lock_stdio();
        printf("> ");
        s_prompt_active = true;
        if (locked) logger_unlock_stdio();
    }

    while (1)
    {
        // Read character from stdin (UART) - use timeout to prevent blocking other tasks on the same core
        rxed_char = getchar_timeout_us(0);

        if (rxed_char == '\r' || rxed_char == '\n')
        {
            // Lock stdio for exclusive access during command processing
            // If lock fails, proceed anyway - CLI must remain functional
            bool locked = logger_lock_stdio();
            
            printf("\n");

            // Process command only if buffer is not empty
            if (s_input_index > 0)
            {
                // Terminate the string
                s_input_buffer[s_input_index] = '\0';

                // Process the command
                do
                {
                    more_data_to_follow = FreeRTOS_CLIProcessCommand(s_input_buffer, s_output_buffer, MAX_OUTPUT_LENGTH);
                    printf("%s", s_output_buffer);
                } while (more_data_to_follow != pdFALSE);

                // Clear input buffer
                s_input_index = 0;
                memset(s_input_buffer, 0, MAX_INPUT_LENGTH);
            }

            printf("\n> ");
            s_prompt_active = true;
            
            // Unlock stdio - logger can now print again
            if (locked) {
                logger_unlock_stdio();
            }
        }
        else if (rxed_char == PICO_ERROR_TIMEOUT) 
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
            // Lock stdio for character echo
            // If lock fails, proceed anyway - echo must remain responsive
            bool locked = logger_lock_stdio();
            
            if (rxed_char == 8 || rxed_char == 127) // Backspace
            {
                if (s_input_index > 0)
                {
                    s_input_index--;
                    printf("\b \b");
                }
            }
            else if (s_input_index < MAX_INPUT_LENGTH - 1)
            {
                // Echo character
                putchar(rxed_char);
                s_input_buffer[s_input_index] = (char) rxed_char;
                s_input_index++;
            }
            
            // Unlock stdio
            if (locked) {
                logger_unlock_stdio();
            }
            
            // Data received - switch to fast polling for responsive input
            if (poll_delay_ms != POLL_FAST_MS) {
                poll_delay_ms = POLL_FAST_MS;
            }
            idle_counter = 0;  // Reset idle counter
        }
    }
}

/**
 * @brief CLI command handler: task-stats
 * 
 * Displays a table showing the state of each FreeRTOS task.
 * 
 * @param pcWriteBuffer Buffer to write command output
 * @param xWriteBufferLen Size of output buffer
 * @param pcCommandString The command string (unused)
 * @return pdFALSE to indicate command is complete
 */
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

/**
 * @brief CLI command handler: reboot
 * 
 * Reboots the device using the watchdog timer.
 * 
 * @param pcWriteBuffer Buffer to write command output
 * @param xWriteBufferLen Size of output buffer
 * @param pcCommandString The command string (unused)
 * @return pdFALSE to indicate command is complete
 */
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

/**
 * @brief Helper function to safely copy CLI parameter to null-terminated buffer
 * 
 * @param param Source parameter string (may not be null-terminated)
 * @param param_len Length of source parameter
 * @param buf Destination buffer
 * @param buf_size Size of destination buffer
 * @return true on success, false if parameter is too long or invalid
 */
static bool copy_param_to_buffer(const char *param, BaseType_t param_len, char *buf, size_t buf_size) {
    if (param == NULL || param_len <= 0 || (size_t)param_len >= buf_size) {
        return pdFALSE;
    }
    memcpy(buf, param, (size_t)param_len);
    buf[param_len] = '\0';
    return pdTRUE;
}

/**
 * @brief CLI command handler: config
 * 
 * Manages system configuration (read/write/save).
 * Supports configuration of: serial ports, network, serial-to-TCP mode, and device settings.
 * 
 * @param pcWriteBuffer Buffer to write command output
 * @param xWriteBufferLen Size of output buffer
 * @param pcCommandString The command string with parameters
 * @return pdFALSE to indicate command is complete
 */
static BaseType_t prvConfigCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString){

    const char *pcParameter;
    BaseType_t xParameterStringLength;

    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);

    if(pcParameter == NULL){
        snprintf(pcWriteBuffer, xWriteBufferLen, 
                "Usage:\r\n"
                "  config read <serial0|serial1|network|s2tcp|device>\r\n"
                "  config write serial0|serial1 <baud|databits|parity|stopbits|flowcts|flowrts> <value>\r\n"
                "  config write network <ip|subnet|gateway|dns> <a.b.c.d>\r\n"
                "  config write network mode <static|dhcp>\r\n"
                "  config write s2tcp <enable|serial|mode|port|timeout|keepalive|maxconn|remoteip|remoteport> <value>\r\n"
                "  config write device <deviceid|loglevel|timestamp|taskname|colors> <value>\r\n"
                "  config save\r\n"
                "Type 'help config' or 'config write' for more details.\r\n");
        return pdFALSE;
    }

    if (strncmp(pcParameter, "read", 4) == 0){
        pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);
        
        if (pcParameter == NULL){
            snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Missing config type. Use 'serial0', 'serial1', 'network', 's2tcp', or 'device'\r\n");
            return pdFALSE;
        }

        if (strncmp(pcParameter, "serial0", 7) == 0 || 
            strncmp(pcParameter, "serial1", 7) == 0){
            uint8_t uart_id = (strncmp(pcParameter, "serial0", 7) == 0) ? 0 : 1;
            serial_config_t serial_config;
            if (!config_get_serial_config(uart_id, &serial_config))
            {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get serial config\r\n");
                return pdFALSE;
            }
            
            size_t len = 0;
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "Serial%d Configuration:\r\n", uart_id);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Baudrate: %u\r\n", (unsigned int)serial_config.baud);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Databits: %u\r\n", serial_config.databits);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Parity: %d\r\n", serial_config.parity);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Stopbits: %u\r\n", serial_config.stopbits);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Flow Control CTS: %s\r\n", serial_config.flow_control_cts ? "Yes" : "No");
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Flow Control RTS: %s\r\n", serial_config.flow_control_rts ? "Yes" : "No");
            return pdFALSE;
        }
        else if (strncmp(pcParameter, "network", 7) == 0) {
            wiz_NetInfo net_info;
            if (!config_get_net_info(&net_info))
            {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get network config\r\n");
                return pdFALSE;
            }
            
            size_t len = 0;
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "Network Configuration:\r\n");
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  MAC         : %02X:%02X:%02X:%02X:%02X:%02X\r\n", 
                           net_info.mac[0], net_info.mac[1], net_info.mac[2], net_info.mac[3], net_info.mac[4], net_info.mac[5]);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  IP          : %d.%d.%d.%d\r\n", 
                           net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Subnet Mask : %d.%d.%d.%d\r\n", 
                           net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3]);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Gateway     : %d.%d.%d.%d\r\n", 
                           net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3]);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  DNS         : %d.%d.%d.%d\r\n", 
                           net_info.dns[0], net_info.dns[1], net_info.dns[2], net_info.dns[3]);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  DHCP        : %s\r\n", net_info.dhcp == NETINFO_DHCP ? "DHCP" : "Static");
            return pdFALSE;
        }
        else if (strncmp(pcParameter, "s2tcp", 5) == 0) {
            serial_to_tcp_mode_config_t mode_config;
            if (!config_get_serial_to_tcp_mode(&mode_config))
            {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get Serial-to-TCP config\r\n");
                return pdFALSE;
            }
            
            size_t len = 0;
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "Serial-to-TCP Configuration:\r\n");
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Enabled         : %s\r\n", mode_config.enable ? "Yes" : "No");
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Serial Port     : UART%u\r\n", mode_config.serial_id);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Mode            : %s\r\n", 
                           mode_config.mode == TCP_MODE_SERVER ? "Server" : "Client");
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Local Port      : %u\r\n", mode_config.local_port);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Timeout         : %u seconds\r\n", mode_config.timeout_s);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Keepalive       : %u seconds\r\n", mode_config.keepalive_s);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Max Connections : %u\r\n", mode_config.max_connections);
            if (mode_config.mode == TCP_MODE_CLIENT) {
                len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Remote IP       : %d.%d.%d.%d\r\n", 
                               mode_config.remote_ip[0], mode_config.remote_ip[1], 
                               mode_config.remote_ip[2], mode_config.remote_ip[3]);
                len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Remote Port     : %u\r\n", mode_config.remote_port);
            }
            return pdFALSE;
        }
        else if (strncmp(pcParameter, "device", 6) == 0) {
            device_config_t device_config;
            if (!config_get_device_config(&device_config))
            {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get device config\r\n");
                return pdFALSE;
            }
            
            size_t len = 0;
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "Device Configuration:\r\n");
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Device ID       : %s\r\n", device_config.device_id);
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Log Level       : %s\r\n", 
                           device_config.logger_config.filter_level == LOG_LEVEL_ERROR ? "ERROR" :
                           device_config.logger_config.filter_level == LOG_LEVEL_WARN ? "WARN" :
                           device_config.logger_config.filter_level == LOG_LEVEL_INFO ? "INFO" : "DEBUG");
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Timestamp       : %s\r\n", 
                           device_config.logger_config.include_timestamp ? "Yes" : "No");
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Task Name       : %s\r\n", 
                           device_config.logger_config.include_task_name ? "Yes" : "No");
            len += snprintf(pcWriteBuffer + len, xWriteBufferLen - len, "  Colors          : %s\r\n", 
                           device_config.logger_config.use_colors ? "Yes" : "No");
            return pdFALSE;
        }
        else {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Option not supported. Use 'serial0', 'serial1', 'network', 's2tcp', or 'device'\r\n");
            return pdFALSE;
        }
    }
    else if (strncmp(pcParameter, "write", 5) == 0){
        pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);
        
        if (pcParameter == NULL){
            snprintf(pcWriteBuffer, xWriteBufferLen, 
                    "Usage:\r\n"
                    "  config write serial0|serial1 baud <9600-921600>\r\n"
                    "  config write serial0|serial1 databits <5-8>\r\n"
                    "  config write serial0|serial1 parity <none|even|odd>\r\n"
                    "  config write serial0|serial1 stopbits <1|2>\r\n"
                    "  config write serial0|serial1 flowcts <0|1>\r\n"
                    "  config write serial0|serial1 flowrts <0|1>\r\n"
                    "  config write network ip <a.b.c.d>\r\n"
                    "  config write network subnet <a.b.c.d>\r\n"
                    "  config write network gateway <a.b.c.d>\r\n"
                    "  config write network dns <a.b.c.d>\r\n"
                    "  config write network mode <static|dhcp>\r\n"
                    "  config write s2tcp enable <0|1>\r\n"
                    "  config write s2tcp serial <0|1>\r\n"
                    "  config write s2tcp mode <server|client>\r\n"
                    "  config write s2tcp port <1024-65535>\r\n"
                    "  config write s2tcp timeout <1-3600>\r\n"
                    "  config write s2tcp keepalive <1-600>\r\n"
                    "  config write s2tcp maxconn <1-4>\r\n"
                    "  config write s2tcp remoteip <a.b.c.d>\r\n"
                    "  config write s2tcp remoteport <1024-65535>\r\n"
                    "  config write device deviceid <string>\r\n"
                    "  config write device loglevel <error|warn|info|debug>\r\n"
                    "  config write device timestamp <0|1>\r\n"
                    "  config write device taskname <0|1>\r\n"
                    "  config write device colors <0|1>\r\n");
            return pdFALSE;
        }

        // === SERIAL CONFIGURATION ===
        if (strncmp(pcParameter, "serial0", 7) == 0 ||
            strncmp(pcParameter, "serial1", 7) == 0){
            uint8_t uart_id = (strncmp(pcParameter, "serial0", 7) == 0) ? 0 : 1;
            BaseType_t xFieldLength, xValueLength;
            const char *pcField = FreeRTOS_CLIGetParameter(pcCommandString, 3, &xFieldLength);
            const char *pcValue = FreeRTOS_CLIGetParameter(pcCommandString, 4, &xValueLength);
            
            if (pcField == NULL || pcValue == NULL) {
                snprintf(pcWriteBuffer, xWriteBufferLen, 
                        "Error: Usage: config write serial0|serial1 <field> <value>\r\n"
                        "Fields: baud, databits, parity, stopbits, flowcts, flowrts\r\n");
                return pdFALSE;
            }
            
            serial_config_t serial_config;
            if (!config_get_serial_config(uart_id, &serial_config)) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get serial config\r\n");
                return pdFALSE;
            }
            
            if (strncmp(pcField, "baud", 4) == 0) {
                char value_buf[16];
                if (!copy_param_to_buffer(pcValue, xValueLength, value_buf, sizeof(value_buf))) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid parameter\r\n");
                    return pdFALSE;
                }
                int baud_val = atoi(value_buf);
                if (baud_val < 9600 || baud_val > 921600) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid baudrate. Use 9600-921600\r\n");
                    return pdFALSE;
                }
                serial_config.baud = (uint32_t)baud_val;
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial baudrate set to %d\r\n", baud_val);
            }
            else if (strncmp(pcField, "databits", 8) == 0) {
                char value_buf[16];
                if (!copy_param_to_buffer(pcValue, xValueLength, value_buf, sizeof(value_buf))) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid parameter\r\n");
                    return pdFALSE;
                }
                int databits_val = atoi(value_buf);
                if (databits_val < 5 || databits_val > 8) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Databits must be 5-8\r\n");
                    return pdFALSE;
                }
                serial_config.databits = (uint8_t)databits_val;
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial databits set to %d\r\n", databits_val);
            }
            else if (strncmp(pcField, "parity", 6) == 0) {
                char value_buf[16];
                if (!copy_param_to_buffer(pcValue, xValueLength, value_buf, sizeof(value_buf))) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid parameter\r\n");
                    return pdFALSE;
                }
                if (strcmp(value_buf, "none") == 0 || strcmp(value_buf, "0") == 0) {
                    serial_config.parity = UART_PARITY_NONE;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Serial parity set to NONE\r\n");
                }
                else if (strcmp(value_buf, "even") == 0 || strcmp(value_buf, "1") == 0) {
                    serial_config.parity = UART_PARITY_EVEN;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Serial parity set to EVEN\r\n");
                }
                else if (strcmp(value_buf, "odd") == 0 || strcmp(value_buf, "2") == 0) {
                    serial_config.parity = UART_PARITY_ODD;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Serial parity set to ODD\r\n");
                }
                else {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Parity must be none, even, or odd\r\n");
                    return pdFALSE;
                }
            }
            else if (strncmp(pcField, "stopbits", 8) == 0) {
                char value_buf[16];
                if (!copy_param_to_buffer(pcValue, xValueLength, value_buf, sizeof(value_buf))) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid parameter\r\n");
                    return pdFALSE;
                }
                int stopbits_val = atoi(value_buf);
                if (stopbits_val != 1 && stopbits_val != 2) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Stopbits must be 1 or 2\r\n");
                    return pdFALSE;
                }
                serial_config.stopbits = (uint8_t)stopbits_val;
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial stopbits set to %d\r\n", stopbits_val);
            }
            else if (strncmp(pcField, "flowcts", 7) == 0) {
                char value_buf[16];
                if (!copy_param_to_buffer(pcValue, xValueLength, value_buf, sizeof(value_buf))) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid parameter\r\n");
                    return pdFALSE;
                }
                int val = atoi(value_buf);
                serial_config.flow_control_cts = (val != 0);
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial CTS flow control %s\r\n", 
                        serial_config.flow_control_cts ? "enabled" : "disabled");
            }
            else if (strncmp(pcField, "flowrts", 7) == 0) {
                char value_buf[16];
                if (!copy_param_to_buffer(pcValue, xValueLength, value_buf, sizeof(value_buf))) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid parameter\r\n");
                    return pdFALSE;
                }
                int val = atoi(value_buf);
                serial_config.flow_control_rts = (val != 0);
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial RTS flow control %s\r\n", 
                        serial_config.flow_control_rts ? "enabled" : "disabled");
            }
            else {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Unknown serial field '%.*s'\r\n", 
                        (int)xFieldLength, pcField);
                return pdFALSE;
            }
            
            if (!config_set_serial_config(uart_id, &serial_config)) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to set serial config\r\n");
                return pdFALSE;
            }
            return pdFALSE;
        }
        // === NETWORK CONFIGURATION ===
        else if (strncmp(pcParameter, "network", 7) == 0) {
            BaseType_t xFieldLength, xValueLength;
            const char *pcField = FreeRTOS_CLIGetParameter(pcCommandString, 3, &xFieldLength);
            const char *pcValue = FreeRTOS_CLIGetParameter(pcCommandString, 4, &xValueLength);
            
            if (pcField == NULL || pcValue == NULL) {
                snprintf(pcWriteBuffer, xWriteBufferLen, 
                        "Error: Usage:\r\n"
                        "  config write network <ip|subnet|gateway|dns> <a.b.c.d>\r\n"
                        "  config write network mode <static|dhcp>\r\n");
                return pdFALSE;
            }
            
            // Copy value to null-terminated buffer
            char value_buf[32];
            if (!copy_param_to_buffer(pcValue, xValueLength, value_buf, sizeof(value_buf))) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid parameter\r\n");
                return pdFALSE;
            }
            
            wiz_NetInfo net_info;
            if (!config_get_net_info(&net_info)) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get network config\r\n");
                return pdFALSE;
            }
            
            if (strncmp(pcField, "mode", xFieldLength) == 0) {
                if (strcmp(value_buf, "dhcp") == 0 || strcmp(value_buf, "1") == 0) {
                    net_info.dhcp = NETINFO_DHCP;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Network mode set to DHCP\r\n");
                }
                else if (strcmp(value_buf, "static") == 0 || strcmp(value_buf, "0") == 0) {
                    net_info.dhcp = NETINFO_STATIC;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Network mode set to Static\r\n");
                }
                else {
                    snprintf(pcWriteBuffer, xWriteBufferLen,
                            "Error: Network mode must be 'static' or 'dhcp'\r\n");
                    return pdFALSE;
                }
            }
            else {
                // All other fields expect an IPv4 address (a.b.c.d)
                unsigned int a, b, c, d;
                if (sscanf(value_buf, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || 
                    a > 255 || b > 255 || c > 255 || d > 255) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid IP format. Use a.b.c.d\r\n");
                    return pdFALSE;
                }
                
                if (strncmp(pcField, "ip", xFieldLength) == 0) {
                    net_info.ip[0] = a; net_info.ip[1] = b; net_info.ip[2] = c; net_info.ip[3] = d;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "IP address set to %u.%u.%u.%u\r\n", a, b, c, d);
                }
                else if (strncmp(pcField, "subnet", xFieldLength) == 0 || strncmp(pcField, "sn", xFieldLength) == 0) {
                    net_info.sn[0] = a; net_info.sn[1] = b; net_info.sn[2] = c; net_info.sn[3] = d;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Subnet mask set to %u.%u.%u.%u\r\n", a, b, c, d);
                }
                else if (strncmp(pcField, "gateway", xFieldLength) == 0 || strncmp(pcField, "gw", xFieldLength) == 0) {
                    net_info.gw[0] = a; net_info.gw[1] = b; net_info.gw[2] = c; net_info.gw[3] = d;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Gateway set to %u.%u.%u.%u\r\n", a, b, c, d);
                }
                else if (strncmp(pcField, "dns", xFieldLength) == 0) {
                    net_info.dns[0] = a; net_info.dns[1] = b; net_info.dns[2] = c; net_info.dns[3] = d;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "DNS set to %u.%u.%u.%u\r\n", a, b, c, d);
                }
                else {
                    snprintf(pcWriteBuffer, xWriteBufferLen, 
                            "Error: Unknown network field. Use ip, subnet, gateway, dns, or mode\r\n");
                    return pdFALSE;
                }
            }
            
            if (!config_set_net_info(&net_info)) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to set network config\r\n");
                return pdFALSE;
            }
            return pdFALSE;
        }
        // === SERIAL-TO-TCP MODE CONFIGURATION ===
        else if (strncmp(pcParameter, "s2tcp", 5) == 0) {
            BaseType_t xFieldLength, xValueLength;
            const char *pcField = FreeRTOS_CLIGetParameter(pcCommandString, 3, &xFieldLength);
            const char *pcValue = FreeRTOS_CLIGetParameter(pcCommandString, 4, &xValueLength);
            
            if (pcField == NULL || pcValue == NULL) {
                snprintf(pcWriteBuffer, xWriteBufferLen, 
                        "Error: Usage: config write s2tcp <field> <value>\r\n");
                return pdFALSE;
            }
            
            serial_to_tcp_mode_config_t mode_config;
            if (!config_get_serial_to_tcp_mode(&mode_config)) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get Serial-to-TCP config\r\n");
                return pdFALSE;
            }
            
            char value_buf[32];
            if (!copy_param_to_buffer(pcValue, xValueLength, value_buf, sizeof(value_buf))) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid parameter\r\n");
                return pdFALSE;
            }
            
            if (strncmp(pcField, "enable", 6) == 0) {
                int val = atoi(value_buf);
                mode_config.enable = (val != 0);
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial-to-TCP mode %s\r\n", 
                        mode_config.enable ? "enabled" : "disabled");
            }
            else if (strncmp(pcField, "serial", 6) == 0) {
                int val = atoi(value_buf);
                if (val != 0 && val != 1) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Serial ID must be 0 or 1\r\n");
                    return pdFALSE;
                }
                mode_config.serial_id = (uint8_t)val;
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial-to-TCP using UART%d\r\n", val);
            }
            else if (strncmp(pcField, "mode", 4) == 0) {
                if (strcmp(value_buf, "server") == 0 || strcmp(value_buf, "0") == 0) {
                    mode_config.mode = TCP_MODE_SERVER;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Serial-to-TCP mode set to SERVER\r\n");
                }
                else if (strcmp(value_buf, "client") == 0 || strcmp(value_buf, "1") == 0) {
                    mode_config.mode = TCP_MODE_CLIENT;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Serial-to-TCP mode set to CLIENT\r\n");
                }
                else {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Mode must be 'server' or 'client'\r\n");
                    return pdFALSE;
                }
            }
            else if (strncmp(pcField, "port", 4) == 0) {
                int port_value = atoi(value_buf);
                if (port_value < 1024 || port_value > 65535) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Port must be 1024-65535\r\n");
                    return pdFALSE;
                }
                mode_config.local_port = (uint16_t)port_value;
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial-to-TCP port set to %d\r\n", port_value);
            }
            else if (strncmp(pcField, "timeout", 7) == 0) {
                int timeout_val = atoi(value_buf);
                if (timeout_val <= 0 || timeout_val > 3600) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Timeout must be 1-3600 seconds\r\n");
                    return pdFALSE;
                }
                mode_config.timeout_s = (uint16_t)timeout_val;
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial-to-TCP timeout set to %d seconds\r\n", timeout_val);
            }
            else if (strncmp(pcField, "keepalive", 9) == 0) {
                int keepalive_val = atoi(value_buf);
                if (keepalive_val <= 0 || keepalive_val > 600) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Keepalive must be 1-600 seconds\r\n");
                    return pdFALSE;
                }
                mode_config.keepalive_s = (uint16_t)keepalive_val;
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial-to-TCP keepalive set to %d seconds\r\n", keepalive_val);
            }
            else if (strncmp(pcField, "maxconn", 7) == 0) {
                int maxconn_val = atoi(value_buf);
                if (maxconn_val < 1 || maxconn_val > 4) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Max connections must be 1-4\r\n");
                    return pdFALSE;
                }
                mode_config.max_connections = (uint8_t)maxconn_val;
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial-to-TCP max connections set to %d\r\n", maxconn_val);
            }
            else if (strncmp(pcField, "remoteip", 8) == 0) {
                unsigned int a, b, c, d;
                if (sscanf(value_buf, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || 
                    a > 255 || b > 255 || c > 255 || d > 255) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid IP format. Use a.b.c.d\r\n");
                    return pdFALSE;
                }
                mode_config.remote_ip[0] = a; mode_config.remote_ip[1] = b;
                mode_config.remote_ip[2] = c; mode_config.remote_ip[3] = d;
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial-to-TCP remote IP set to %u.%u.%u.%u\r\n", a, b, c, d);
            }
            else if (strncmp(pcField, "remoteport", 10) == 0) {
                int port_value = atoi(value_buf);
                if (port_value < 1024 || port_value > 65535) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Remote port must be 1024-65535\r\n");
                    return pdFALSE;
                }
                mode_config.remote_port = (uint16_t)port_value;
                snprintf(pcWriteBuffer, xWriteBufferLen, "Serial-to-TCP remote port set to %d\r\n", port_value);
            }
            else {
                snprintf(pcWriteBuffer, xWriteBufferLen, 
                        "Error: Unknown field. Use enable, serial, mode, port, timeout, keepalive, maxconn, remoteip, or remoteport\r\n");
                return pdFALSE;
            }
            
            if (!config_set_serial_to_tcp_mode(&mode_config)) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to set Serial-to-TCP config\r\n");
                return pdFALSE;
            }
            return pdFALSE;
        }
        // === DEVICE CONFIGURATION ===
        else if (strncmp(pcParameter, "device", 6) == 0) {
            BaseType_t xFieldLength, xValueLength;
            const char *pcField = FreeRTOS_CLIGetParameter(pcCommandString, 3, &xFieldLength);
            const char *pcValue = FreeRTOS_CLIGetParameter(pcCommandString, 4, &xValueLength);
            
            if (pcField == NULL || pcValue == NULL) {
                snprintf(pcWriteBuffer, xWriteBufferLen, 
                        "Error: Usage: config write device <deviceid|loglevel|timestamp|taskname|colors> <value>\r\n");
                return pdFALSE;
            }
            
            device_config_t device_config;
            if (!config_get_device_config(&device_config)) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get device config\r\n");
                return pdFALSE;
            }
            
            char value_buf[32];
            if (!copy_param_to_buffer(pcValue, xValueLength, value_buf, sizeof(value_buf))) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid parameter\r\n");
                return pdFALSE;
            }
            
            if (strncmp(pcField, "deviceid", 8) == 0) {
                if (xValueLength >= sizeof(device_config.device_id)) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Device ID too long (max %d chars)\r\n", 
                            (int)sizeof(device_config.device_id) - 1);
                    return pdFALSE;
                }
                memset(device_config.device_id, 0, sizeof(device_config.device_id));
                memcpy(device_config.device_id, value_buf, strlen(value_buf));
                snprintf(pcWriteBuffer, xWriteBufferLen, "Device ID set to '%s'\r\n", device_config.device_id);
            }
            else if (strncmp(pcField, "loglevel", 8) == 0) {
                if (strcmp(value_buf, "error") == 0 || strcmp(value_buf, "0") == 0) {
                    device_config.logger_config.filter_level = LOG_LEVEL_ERROR;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Log level set to ERROR\r\n");
                }
                else if (strcmp(value_buf, "warn") == 0 || strcmp(value_buf, "1") == 0) {
                    device_config.logger_config.filter_level = LOG_LEVEL_WARN;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Log level set to WARN\r\n");
                }
                else if (strcmp(value_buf, "info") == 0 || strcmp(value_buf, "2") == 0) {
                    device_config.logger_config.filter_level = LOG_LEVEL_INFO;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Log level set to INFO\r\n");
                }
                else if (strcmp(value_buf, "debug") == 0 || strcmp(value_buf, "3") == 0) {
                    device_config.logger_config.filter_level = LOG_LEVEL_DEBUG;
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Log level set to DEBUG\r\n");
                }
                else {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Log level must be error, warn, info, or debug\r\n");
                    return pdFALSE;
                }
            }
            else if (strncmp(pcField, "timestamp", 9) == 0) {
                int val = atoi(value_buf);
                device_config.logger_config.include_timestamp = (val != 0);
                snprintf(pcWriteBuffer, xWriteBufferLen, "Timestamp %s\r\n", 
                        device_config.logger_config.include_timestamp ? "enabled" : "disabled");
            }
            else if (strncmp(pcField, "taskname", 8) == 0) {
                int val = atoi(value_buf);
                device_config.logger_config.include_task_name = (val != 0);
                snprintf(pcWriteBuffer, xWriteBufferLen, "Task name %s\r\n", 
                        device_config.logger_config.include_task_name ? "enabled" : "disabled");
            }
            else if (strncmp(pcField, "colors", 6) == 0) {
                int val = atoi(value_buf);
                device_config.logger_config.use_colors = (val != 0);
                snprintf(pcWriteBuffer, xWriteBufferLen, "Colors %s\r\n", 
                        device_config.logger_config.use_colors ? "enabled" : "disabled");
            }
            else {
                snprintf(pcWriteBuffer, xWriteBufferLen, 
                        "Error: Unknown field. Use deviceid, loglevel, timestamp, taskname, or colors\r\n");
                return pdFALSE;
            }
            
            if (!config_set_device_config(&device_config)) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to set device config\r\n");
                return pdFALSE;
            }
            return pdFALSE;
        }
        else {
            snprintf(pcWriteBuffer, xWriteBufferLen, 
                    "Error: Unknown config type. Use 'serial0', 'serial1', 'network', 's2tcp', or 'device'\r\n");
            return pdFALSE;
        }
    }
    else if (strncmp(pcParameter, "save", 4) == 0){
        // Save configuration to flash
        if (config_save_to_flash()) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Configuration saved to flash successfully.\r\n");
        } else {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to save configuration to flash.\r\n");
        }
        return pdFALSE;
    }
    else {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Unknown command '%.*s'. Use: config <read|write|save>\r\n", 
                 (int)xParameterStringLength, pcParameter);
    }
    return pdFALSE;
}

static const CLI_Command_Definition_t xConfig = {
    "config",
    "\r\nconfig - Configuration management\r\n"
    "Usage:\r\n"
    "  config read <serial0|serial1|network|s2tcp|device>\r\n"
    "  config write serial0|serial1 <baud|databits|parity|stopbits|flowcts|flowrts> <value>\r\n"
    "  config write network <ip|subnet|gateway|dns> <a.b.c.d>\r\n"
    "  config write network mode <static|dhcp>\r\n"
    "  config write s2tcp <enable|serial|mode|port|timeout|keepalive|maxconn|remoteip|remoteport> <value>\r\n"
    "  config write device <deviceid|loglevel|timestamp|taskname|colors> <value>\r\n"
    "  config save\r\n"
    "Type 'config write' for detailed parameter ranges.\r\n",
    prvConfigCommand,
    -1  // Variable number of parameters

};

/**
 * @brief CLI command handler: uptime
 * 
 * Shows system uptime in days, hours, minutes, and seconds.
 * 
 * @param pcWriteBuffer Buffer to write command output
 * @param xWriteBufferLen Size of output buffer
 * @param pcCommandString The command string (unused)
 * @return pdFALSE to indicate command is complete
 */
static BaseType_t prvUptimeCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    (void) pcCommandString;
    // Format uptime in HH:MM:SS format
    uint32_t uptime_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t uptime_s = uptime_ms / 1000;
    uint32_t uptime_m = uptime_s / 60;
    uint32_t uptime_h = uptime_m / 60;
    uint32_t uptime_d = uptime_h / 24;
    snprintf(pcWriteBuffer, xWriteBufferLen, "Uptime: %d days, %02d:%02d:%02d\r\n", uptime_d, uptime_h % 24, uptime_m % 60, uptime_s % 60);
    return pdFALSE;
}

static const CLI_Command_Definition_t xUptime = {
    "uptime",
    "\r\nuptime - Show system uptime\r\n",
    prvUptimeCommand,
    -1  // Variable number of parameters
};


bool cli_task_init(void)
{
    // Tell the logger to repaint our prompt after every log line.
    logger_set_cli_redraw_callback(cli_redraw_input);

    // Register commands
    FreeRTOS_CLIRegisterCommand(&xTaskStats);
    FreeRTOS_CLIRegisterCommand(&xReboot);
    FreeRTOS_CLIRegisterCommand(&xConfig);
    FreeRTOS_CLIRegisterCommand(&xUptime);

    // Create the task
    BaseType_t result = xTaskCreate(
        vCLITask,
        "CLI",
        CLI_TASK_STACK_SIZE,
        NULL,
        CLI_TASK_PRIORITY,
        NULL
    );
    
    if (result != pdPASS)
    {
        LOG_ERROR("Failed to create CLI task");
        return false;
    }
    
    LOG_INFO("CLI task created successfully");
    return true;
}

