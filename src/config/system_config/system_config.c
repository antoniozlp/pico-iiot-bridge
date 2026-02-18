/**
 * @file system_config.c
 * @brief System configuration management with flash persistence
 */

#include "system_config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/flash.h"
#include "pico/stdio_uart.h"
#include "hardware/flash.h"

#include "pico_flash_storage.h"
#include "logger.h"

// Global configuration structure
system_config_t g_sys_cfg;

// Global configuration flags
bool g_config_loaded = false;   // True if configuration has been loaded from flash
bool g_config_changed = false;  // True if configuration has been changed since last save

/**
 * @brief Set system configuration to factory defaults
 * 
 * Initializes all configuration values to their factory defaults:
 * - Network: Static IP 192.168.11.3
 * - Serial ports: 115200 baud, 8N1
 * - Serial-to-TCP: Disabled
 * - Modbus RTU: Disabled with 4 example data points configured
 * - Logger: INFO level with timestamps
 */
void config_set_default(void)
{
    memset(&g_sys_cfg, 0, sizeof(system_config_t));

    // Version defaults
    g_sys_cfg.version.major = CONFIG_VERSION_MAJOR;
    g_sys_cfg.version.minor = CONFIG_VERSION_MINOR;
    g_sys_cfg.version.patch = CONFIG_VERSION_PATCH;

    // Device config
    memset(g_sys_cfg.device.device_id, 0, 20);
    uint8_t device_id[] = "DEFAULT_DEVICE_ID";
    memcpy(g_sys_cfg.device.device_id, device_id, sizeof(device_id));
    g_sys_cfg.device.logger_config.filter_level = LOG_LEVEL_INFO;
    g_sys_cfg.device.logger_config.include_timestamp = true;
    g_sys_cfg.device.logger_config.include_task_name = true;
    g_sys_cfg.device.logger_config.use_colors = true;

    // Network defaults
    uint8_t mac[6] = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56};
    uint8_t ip[4] = {192, 168, 11, 3};
    uint8_t sn[4] = {255, 255, 255, 0};
    uint8_t gw[4] = {192, 168, 11, 1};
    uint8_t dns[4] = {8, 8, 8, 8};

    memcpy(g_sys_cfg.net_info.mac, mac, 6);
    memcpy(g_sys_cfg.net_info.ip, ip, 4);
    memcpy(g_sys_cfg.net_info.sn, sn, 4);
    memcpy(g_sys_cfg.net_info.gw, gw, 4);
    memcpy(g_sys_cfg.net_info.dns, dns, 4);
    g_sys_cfg.net_info.dhcp = NETINFO_STATIC;

    // Serial0 defaults (Debug/Console UART)
    g_sys_cfg.serial0.baud = 115200;
    g_sys_cfg.serial0.databits = 8;
    g_sys_cfg.serial0.parity = UART_PARITY_NONE;
    g_sys_cfg.serial0.stopbits = 1;
    g_sys_cfg.serial0.flow_control_cts = false;
    g_sys_cfg.serial0.flow_control_rts = false;

    // Serial1 defaults (Serial Bridge UART)
    g_sys_cfg.serial1.baud = 115200;
    g_sys_cfg.serial1.databits = 8;
    g_sys_cfg.serial1.parity = UART_PARITY_NONE;
    g_sys_cfg.serial1.stopbits = 1;
    g_sys_cfg.serial1.flow_control_cts = false;
    g_sys_cfg.serial1.flow_control_rts = false;

    // Serial-to-TCP mode defaults
    g_sys_cfg.serial_to_tcp_mode.enable = 0;  // Disabled by default
    g_sys_cfg.serial_to_tcp_mode.serial_id = 1;  // Use UART1 by default
    g_sys_cfg.serial_to_tcp_mode.mode = TCP_MODE_SERVER;
    g_sys_cfg.serial_to_tcp_mode.local_port = 5000;
    g_sys_cfg.serial_to_tcp_mode.timeout_s = 30;
    g_sys_cfg.serial_to_tcp_mode.keepalive_s = 5;
    g_sys_cfg.serial_to_tcp_mode.max_connections = 1;
    // Client mode defaults (not used when in server mode)
    uint8_t default_remote_ip[4] = {192, 168, 11, 100};
    memcpy(g_sys_cfg.serial_to_tcp_mode.remote_ip, default_remote_ip, 4);
    g_sys_cfg.serial_to_tcp_mode.remote_port = 5001;

    // Modbus RTU client defaults
    g_sys_cfg.modbus_rtu_client.enable = 0;  // Disabled by default (changed from 1)
    g_sys_cfg.modbus_rtu_client.serial_id = 1;  // Use UART1 by default
    
    // Initialize all data points to disabled
    for (uint8_t i = 0; i < MODBUS_REQUESTS_MAX; i++)
    {
        g_sys_cfg.modbus_rtu_client.requests[i].enabled = 0;
        g_sys_cfg.modbus_rtu_client.requests[i].slave_address = 1;
        g_sys_cfg.modbus_rtu_client.requests[i].data_type = MODBUS_DATA_TYPE_HOLDING_REGISTER;
        g_sys_cfg.modbus_rtu_client.requests[i].operation = MODBUS_OP_READ;
        g_sys_cfg.modbus_rtu_client.requests[i].start_address = 0;
        g_sys_cfg.modbus_rtu_client.requests[i].count = 1;
        g_sys_cfg.modbus_rtu_client.requests[i].encoding = MODBUS_ENCODING_ABCD;
        
        // Initialize all tag mappings as unmapped
        for (uint8_t j = 0; j < MODBUS_MAX_REG_COUNT; j++)
        {
            g_sys_cfg.modbus_rtu_client.requests[i].tag_handles[j] = MODBUS_TAG_MAP_INVALID;
        }
    }

    // Tag database defaults
    g_sys_cfg.tag_database.tag_count = 0;
    g_sys_cfg.tag_database.auto_create_enabled = 1;  // Allow runtime creation
    g_sys_cfg.tag_database.reserved = 0;
    memset(g_sys_cfg.tag_database.tags, 0, sizeof(g_sys_cfg.tag_database.tags));
    
    LOG_INFO("Configuration set to defaults (including tag database)");
}

/**
 * @brief Load system configuration from flash memory
 * 
 * Reads configuration from flash and validates version.
 * If validation fails, loads factory defaults instead.
 * 
 * @return true if valid config loaded from flash, false if defaults loaded
 */
bool config_load_from_flash(void)
{
    const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    
    // Read directly from memory mapped flash
    const system_config_t *flash_config = (const system_config_t *)flash_target_contents;

    // Check for exact version match
    if (flash_config->version.major == CONFIG_VERSION_MAJOR &&
        flash_config->version.minor == CONFIG_VERSION_MINOR &&
        flash_config->version.patch == CONFIG_VERSION_PATCH)
    {
        memcpy(&g_sys_cfg, flash_config, sizeof(system_config_t));
        LOG_INFO("Configuration loaded from flash (version %d.%d.%d)",
                 CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR, CONFIG_VERSION_PATCH);
        g_config_changed = false;  // In-memory matches flash
        g_config_loaded = true;
        return true;
    }
    
    // Version mismatch - load defaults
    LOG_WARN("Configuration version mismatch in flash");
    LOG_WARN("Flash version: %d.%d.%d, Expected: %d.%d.%d", 
             flash_config->version.major, flash_config->version.minor, flash_config->version.patch,
             CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR, CONFIG_VERSION_PATCH);
    LOG_INFO("Loading default configuration");
    config_set_default();
    g_config_changed = true;  // Defaults loaded, should be saved to flash
    g_config_loaded = true;
    return false;
}

/**
 * @brief Save system configuration to flash memory
 * 
 * Writes current configuration to flash using flash_storage_write().
 * Skips write if no changes have been made since last save.
 * 
 * @return true if save successful or nothing to save, false on error
 */
bool config_save_to_flash(void)
{
    
    // If no changes have been made skip saving to avoid unnecessary writes
    if (!g_config_changed)
    {
        return true;  // Nothing to save, considered success
    }

    // Ensure configuration fits in one sector
    if (sizeof(system_config_t) > FLASH_SECTOR_SIZE)
    {
        return false;
    }

    // Prepare buffer to write to flash
    // Note: flash_storage_write() will erase a full sector (4096 bytes), which sets
    // all bytes to 0xFF. We only program CONFIG_BUFFER_SIZE bytes here. The remaining
    // portion of the sector will remain at 0xFF (erased state), which is correct.
    static uint8_t buf[CONFIG_BUFFER_SIZE];
    memset(buf, 0xFF, CONFIG_BUFFER_SIZE); // 0xFF is erased flash state
    memcpy(buf, &g_sys_cfg, sizeof(system_config_t));

    if (flash_storage_write(FLASH_TARGET_OFFSET, buf, CONFIG_BUFFER_SIZE))
    {
        g_config_changed = false;  // Reset flag after successful save
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief Get configuration version
 * 
 * @param version Pointer to version structure to fill
 * @return true on success, false if version is NULL or config not loaded
 */
bool config_get_version(config_version_t *version)
{
    if (version != NULL && g_config_loaded)
    {
        memcpy(version, &g_sys_cfg.version, sizeof(config_version_t));
        return true;
    }
    return false;
}

/**
 * @brief Get device configuration
 * 
 * @param device_config Pointer to device_config structure to fill
 * @return true on success, false if parameter is NULL or config not loaded
 */
bool config_get_device_config(device_config_t *device_config)
{
    if (device_config == NULL || !g_config_loaded)
    {
        return false;
    }
    memcpy(device_config, &g_sys_cfg.device, sizeof(device_config_t));
    return true;
}

/**
 * @brief Set device configuration
 * 
 * Updates device configuration and marks config as changed.
 * 
 * @param device_config Pointer to device_config structure with new values
 * @return true on success, false if parameter is NULL or config not loaded
 */
bool config_set_device_config(device_config_t *device_config)
{
    if (device_config == NULL || !g_config_loaded)
    {
        return false;
    }
    memcpy(&g_sys_cfg.device, device_config, sizeof(device_config_t));
    g_config_changed = true;
    return true;
}

/**
 * @brief Get serial port configuration
 * 
 * @param uart_id UART ID (0 or 1)
 * @param serial_config Pointer to serial_config structure to fill
 * @return true on success, false if parameters invalid or config not loaded
 */
bool config_get_serial_config(uint8_t uart_id, serial_config_t *serial_config)
{
    if (serial_config == NULL || !g_config_loaded || uart_id > 1)
    {
        return false;
    }
    
    if (uart_id == 0)
    {
        memcpy(serial_config, &g_sys_cfg.serial0, sizeof(serial_config_t));
    }
    else
    {
        memcpy(serial_config, &g_sys_cfg.serial1, sizeof(serial_config_t));
    }
    return true;
}

/**
 * @brief Set serial port configuration
 * 
 * Updates serial configuration and marks config as changed if values differ.
 * 
 * @param uart_id UART ID (0 or 1)
 * @param serial_config Pointer to serial_config structure with new values
 * @return true on success, false if parameters invalid or config not loaded
 */
bool config_set_serial_config(uint8_t uart_id, serial_config_t *serial_config)
{
    if (serial_config == NULL || !g_config_loaded || uart_id > 1)
    {
        return false;
    }
    
    serial_config_t *target = (uart_id == 0) ? &g_sys_cfg.serial0 : &g_sys_cfg.serial1;
    
    if (memcmp(target, serial_config, sizeof(serial_config_t)) != 0)
    {
        memcpy(target, serial_config, sizeof(serial_config_t));
        g_config_changed = true;
    }
    return true;
}

/**
 * @brief Get serial-to-TCP mode configuration
 * 
 * @param mode_config Pointer to serial_to_tcp_mode_config structure to fill
 * @return true on success, false if parameter is NULL or config not loaded
 */
bool config_get_serial_to_tcp_mode(serial_to_tcp_mode_config_t *mode_config)
{
    if (mode_config == NULL || !g_config_loaded)
    {
        return false;
    }
    
    memcpy(mode_config, &g_sys_cfg.serial_to_tcp_mode, sizeof(serial_to_tcp_mode_config_t));
    return true;
}

/**
 * @brief Set serial-to-TCP mode configuration
 * 
 * Updates serial-to-TCP configuration and marks config as changed if values differ.
 * 
 * @param mode_config Pointer to serial_to_tcp_mode_config structure with new values
 * @return true on success, false if parameter is NULL or config not loaded
 */
bool config_set_serial_to_tcp_mode(serial_to_tcp_mode_config_t *mode_config)
{
    if (mode_config == NULL || !g_config_loaded)
    {
        return false;
    }
    
    if (memcmp(&g_sys_cfg.serial_to_tcp_mode, mode_config, sizeof(serial_to_tcp_mode_config_t)) != 0)
    {
        memcpy(&g_sys_cfg.serial_to_tcp_mode, mode_config, sizeof(serial_to_tcp_mode_config_t));
        g_config_changed = true;
    }
    return true;
}

/**
 * @brief Get network configuration
 * 
 * @param net_info Pointer to wiz_NetInfo structure to fill
 * @return true on success, false if parameter is NULL or config not loaded
 */
bool config_get_net_info(wiz_NetInfo *net_info)
{
    if (net_info != NULL && g_config_loaded)
    {
        memcpy(net_info, &g_sys_cfg.net_info, sizeof(wiz_NetInfo));
        return true;
    }
    return false;
}

/**
 * @brief Set network configuration
 * 
 * Updates network configuration and marks config as changed if values differ.
 * 
 * @param net_info Pointer to wiz_NetInfo structure with new values
 * @return true on success, false if parameter is NULL or config not loaded
 */
bool config_set_net_info(wiz_NetInfo *net_info)
{
    if (net_info == NULL || !g_config_loaded)
    {
        return false;
    }
    
    if (memcmp(&g_sys_cfg.net_info, net_info, sizeof(wiz_NetInfo)) != 0)
    {
        memcpy(&g_sys_cfg.net_info, net_info, sizeof(wiz_NetInfo));
        g_config_changed = true;
    }
    return true;
}


/**
 * @brief Get Modbus RTU client configuration
 * 
 * @param modbus_rtu_client_config Pointer to modbus_rtu_client_config structure to fill
 * @return true on success, false if parameter is NULL or config not loaded
 */
bool config_get_modbus_rtu_client_config(modbus_rtu_client_config_t *modbus_rtu_client_config)
{
    if (modbus_rtu_client_config == NULL || !g_config_loaded)
    {
        return false;
    }
    memcpy(modbus_rtu_client_config, &g_sys_cfg.modbus_rtu_client, sizeof(modbus_rtu_client_config_t));
    return true;
}

/**
 * @brief Set Modbus RTU client configuration
 * 
 * Updates Modbus RTU client configuration and marks config as changed if values differ.
 * 
 * @param modbus_rtu_client_config Pointer to modbus_rtu_client_config structure with new values
 * @return true on success, false if parameter is NULL or config not loaded
 */
bool config_set_modbus_rtu_client_config(modbus_rtu_client_config_t *modbus_rtu_client_config)
{
    if (modbus_rtu_client_config == NULL || !g_config_loaded)
    {
        return false;
    }
    
    if (memcmp(&g_sys_cfg.modbus_rtu_client, modbus_rtu_client_config, sizeof(modbus_rtu_client_config_t)) != 0)
    {
        memcpy(&g_sys_cfg.modbus_rtu_client, modbus_rtu_client_config, sizeof(modbus_rtu_client_config_t));
        g_config_changed = true;
    }
    return true;
}

// ============================================================================
// Tag Database Configuration Functions
// ============================================================================

/**
 * @brief Get tag database configuration from flash
 * 
 * @param tag_db_config Pointer to receive tag database config
 * @return true if successful, false otherwise
 */
bool config_get_tag_database(tag_database_config_t *tag_db_config)
{
    if (tag_db_config == NULL || !g_config_loaded)
    {
        return false;
    }
    
    memcpy(tag_db_config, &g_sys_cfg.tag_database, sizeof(tag_database_config_t));
    return true;
}

/**
 * @brief Set tag database configuration (marks for flash write)
 * 
 * @param tag_db_config Pointer to new tag database config
 * @return true if successful, false otherwise
 */
bool config_set_tag_database(const tag_database_config_t *tag_db_config)
{
    if (tag_db_config == NULL || !g_config_loaded)
    {
        return false;
    }
    
    // Only mark changed if actually different
    if (memcmp(&g_sys_cfg.tag_database, tag_db_config, sizeof(tag_database_config_t)) != 0)
    {
        memcpy(&g_sys_cfg.tag_database, tag_db_config, sizeof(tag_database_config_t));
        g_config_changed = true;
    }
    
    return true;
}

/**
 * @brief Add a single tag definition to config
 * 
 * Helper function to add one tag without managing the entire array.
 * 
 * @param name Tag name
 * @param data_type Tag data type
 * @return true if added, false if no space or duplicate name
 */
bool config_add_tag_definition(const char *name, uint8_t data_type)
{
    if (name == NULL || !g_config_loaded)
    {
        return false;
    }
    
    // Check for duplicate
    for (uint16_t i = 0; i < TAG_DB_MAX_PERSISTENT_TAGS; i++)
    {
        if (g_sys_cfg.tag_database.tags[i].enabled &&
            strcmp(g_sys_cfg.tag_database.tags[i].name, name) == 0)
        {
            return false;  // Already exists
        }
    }
    
    // Find empty slot
    for (uint16_t i = 0; i < TAG_DB_MAX_PERSISTENT_TAGS; i++)
    {
        if (!g_sys_cfg.tag_database.tags[i].enabled)
        {
            strncpy(g_sys_cfg.tag_database.tags[i].name, name, TAG_NAME_MAX_LEN - 1);
            g_sys_cfg.tag_database.tags[i].name[TAG_NAME_MAX_LEN - 1] = '\0';
            g_sys_cfg.tag_database.tags[i].data_type = data_type;
            g_sys_cfg.tag_database.tags[i].enabled = 1;
            memset(g_sys_cfg.tag_database.tags[i].reserved, 0, sizeof(g_sys_cfg.tag_database.tags[i].reserved));
            
            g_sys_cfg.tag_database.tag_count++;
            g_config_changed = true;
            return true;
        }
    }
    
    return false;  // No space
}

/**
 * @brief Remove a tag definition from config
 * 
 * Marks tag slot as disabled rather than compacting array.
 * 
 * @param name Tag name to remove
 * @return true if found and removed, false otherwise
 */
bool config_remove_tag_definition(const char *name)
{
    if (name == NULL || !g_config_loaded)
    {
        return false;
    }
    
    for (uint16_t i = 0; i < TAG_DB_MAX_PERSISTENT_TAGS; i++)
    {
        if (g_sys_cfg.tag_database.tags[i].enabled &&
            strcmp(g_sys_cfg.tag_database.tags[i].name, name) == 0)
        {
            g_sys_cfg.tag_database.tags[i].enabled = 0;
            g_sys_cfg.tag_database.tag_count--;
            g_config_changed = true;
            return true;
        }
    }
    
    return false;  // Not found
}

/**
 * @brief Get single tag definition by name
 * 
 * @param name Tag name
 * @param def_out Pointer to receive tag definition
 * @return true if found, false otherwise
 */
bool config_get_tag_definition(const char *name, tag_definition_t *def_out)
{
    if (name == NULL || def_out == NULL || !g_config_loaded)
    {
        return false;
    }
    
    for (uint16_t i = 0; i < TAG_DB_MAX_PERSISTENT_TAGS; i++)
    {
        if (g_sys_cfg.tag_database.tags[i].enabled &&
            strcmp(g_sys_cfg.tag_database.tags[i].name, name) == 0)
        {
            memcpy(def_out, &g_sys_cfg.tag_database.tags[i], sizeof(tag_definition_t));
            return true;
        }
    }
    
    return false;  // Not found
}
