#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "pico/flash.h"
#include "system_config.h"
#include "pico/stdio_uart.h"
#include "pico_flash_storage.h"
#include "logger.h"

// Global configuration structure
system_config_t g_sys_cfg;

// Global configuration flags
// g_config_loaded: True if configuration has been loaded from flash
bool g_config_loaded = false;
// g_config_changed: True if configuration has been changed since last load or save
bool g_config_changed = false;


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
}

bool config_load_from_flash(void)
{
    const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    
    // Read directly from memory mapped flash
    const system_config_t *flash_config = (const system_config_t *)flash_target_contents;

    if (flash_config->version.major == CONFIG_VERSION_MAJOR &&
        flash_config->version.minor == CONFIG_VERSION_MINOR &&
        flash_config->version.patch == CONFIG_VERSION_PATCH)
    {
        memcpy(&g_sys_cfg, flash_config, sizeof(system_config_t));
        LOG_INFO("Configuration loaded from flash");
        g_config_changed = false;  // In-memory matches flash
        g_config_loaded = true;
        return true;
    }
    else
    {
        LOG_WARN("No valid configuration found in flash");
        LOG_WARN("Version: %d.%d.%d", flash_config->version.major, flash_config->version.minor, flash_config->version.patch);
        LOG_WARN("Expected: %d.%d.%d", CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR, CONFIG_VERSION_PATCH);
        LOG_INFO("Loading defaults");
        config_set_default();
        g_config_changed = true;  // Defaults loaded, should be saved to flash
        g_config_loaded = true;
        return false;
    }
}


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

bool config_get_version(config_version_t *version)
{
    if (version != NULL && g_config_loaded)
    {
        memcpy(version, &g_sys_cfg.version, sizeof(config_version_t));
        return true;
    }
    return false;
}

bool config_get_device_config(device_config_t *device_config)
{
    if (device_config == NULL || !g_config_loaded)
    {
        return false;
    }
    memcpy(device_config, &g_sys_cfg.device, sizeof(device_config_t));
    return true;
}

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

bool config_get_serial_to_tcp_mode(serial_to_tcp_mode_config_t *mode_config)
{
    if (mode_config == NULL || !g_config_loaded)
    {
        return false;
    }
    
    memcpy(mode_config, &g_sys_cfg.serial_to_tcp_mode, sizeof(serial_to_tcp_mode_config_t));
    return true;
}

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

bool config_get_net_info(wiz_NetInfo *net_info)
{
    if (net_info != NULL && g_config_loaded)
    {
        memcpy(net_info, &g_sys_cfg.net_info, sizeof(wiz_NetInfo));
        return true;
    }
    return false;
}

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
