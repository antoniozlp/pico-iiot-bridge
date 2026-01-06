#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "pico/flash.h"
#include "config.h"
#include "pico/stdio_uart.h"
#include "pico_flash_storage.h"

system_config_t g_sys_cfg;

void config_set_default(void)
{
    memset(&g_sys_cfg, 0, sizeof(system_config_t));

    // Version defaults
    g_sys_cfg.version.major = 0;
    g_sys_cfg.version.minor = 0;
    g_sys_cfg.version.patch = 1;

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

    // Serial defaults
    g_sys_cfg.serial.baud = 57600;
    g_sys_cfg.serial.databits = 8;
    g_sys_cfg.serial.parity = UART_PARITY_NONE;
    g_sys_cfg.serial.stopbits = 1;
    g_sys_cfg.serial.flow_control_cts = false;
    g_sys_cfg.serial.flow_control_rts = false;

    // TCP defaults
    g_sys_cfg.tcp.local_port = 5000;
    g_sys_cfg.tcp.timeout_s = 30;
    g_sys_cfg.tcp.keepalive_s = 5;
    g_sys_cfg.tcp.max_connections = 1;
}

void config_load_from_flash(void)
{
    const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    
    // Read directly from memory mapped flash
    const system_config_t *flash_config = (const system_config_t *)flash_target_contents;

    if (flash_config->version.major == CONFIG_VERSION_MAJOR &&
        flash_config->version.minor == CONFIG_VERSION_MINOR &&
        flash_config->version.patch == CONFIG_VERSION_PATCH)
    {
        memcpy(&g_sys_cfg, flash_config, sizeof(system_config_t));
        printf("Configuration loaded from flash.\n");
    }
    else
    {
        printf("No valid configuration found in flash.\n");
        printf("Version: %d.%d.%d\n", flash_config->version.major, flash_config->version.minor, flash_config->version.patch);
        printf("Expected: %d.%d.%d\n", CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR, CONFIG_VERSION_PATCH);
        printf("Loading defaults.\n");
        config_set_default();
    }
}


void config_save_to_flash(void)
{
    // Check if configuration has changed? For now, just save.
    
    // Prepare buffer (flash write size must be multiple of FLASH_PAGE_SIZE = 256)
    // We erase a sector (4096) and program.
    // Ensure structure fits in sector.
    if (sizeof(system_config_t) > FLASH_SECTOR_SIZE)
    {
        printf("Error: Configuration too large for one sector!\n");
        return;
    }

    uint8_t buf[FLASH_SECTOR_SIZE];
    memset(buf, 0xFF, FLASH_SECTOR_SIZE); // 0xFF is erased state
    memcpy(buf, &g_sys_cfg, sizeof(system_config_t));

    if (flash_storage_write(FLASH_TARGET_OFFSET, buf, sizeof(system_config_t)))
    {
        printf("Configuration saved to flash.\n");
    }
    else
    {
        printf("Failed to save configuration to flash.\n");
    }

}

void config_get_version(config_version_t *version)
{
    if (version != NULL)
    {
        memcpy(version, &g_sys_cfg.version, sizeof(config_version_t));
    }
}

void config_get_serial_config(serial_config_t *serial_config)
{
    if (serial_config != NULL)
    {
        memcpy(serial_config, &g_sys_cfg.serial, sizeof(serial_config_t));
    }
}

void config_set_serial_config(serial_config_t *serial_config)
{
    if (serial_config != NULL)
    {
        memcpy(&g_sys_cfg.serial, serial_config, sizeof(serial_config_t));
    }
}

void config_get_tcp_config(tcp_config_t *tcp_config)
{
    if (tcp_config != NULL)
    {
        memcpy(tcp_config, &g_sys_cfg.tcp, sizeof(tcp_config_t));
    }
}

void config_set_tcp_config(tcp_config_t *tcp_config)
{
    if (tcp_config != NULL)
    {
        memcpy(&g_sys_cfg.tcp, tcp_config, sizeof(tcp_config_t));
    }
}

void config_get_net_info(wiz_NetInfo *net_info)
{
    if (net_info != NULL)
    {
        memcpy(net_info, &g_sys_cfg.net_info, sizeof(wiz_NetInfo));
    }
}

void config_set_net_info(wiz_NetInfo *net_info)
{
    if (net_info != NULL)
    {
        memcpy(&g_sys_cfg.net_info, net_info, sizeof(wiz_NetInfo));
    }
}
