#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include "wizchip_conf.h"
#include "hardware/uart.h"

// Flash configuration
#define FLASH_TARGET_OFFSET 0x1F0000 // Last 64KB block (1,984KB offset)
#define CONFIG_VERSION_MAJOR 0
#define CONFIG_VERSION_MINOR 0
#define CONFIG_VERSION_PATCH 1


typedef struct
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} config_version_t;

typedef struct
{
	uint32_t baud;
	uint8_t databits;
	uart_parity_t parity;
	uint8_t stopbits;
	bool flow_control_cts;
	bool flow_control_rts;
} serial_config_t;

typedef struct
{
	uint16_t local_port;
	uint16_t timeout_s;
    uint16_t keepalive_s;
    uint8_t max_connections;
} tcp_config_t;

typedef struct
{
	config_version_t version;
    wiz_NetInfo net_info;
    serial_config_t serial;
    tcp_config_t tcp;
} system_config_t;


void config_set_default(void);
void config_load_from_flash(void);
void config_save_to_flash(void);
void config_get_version(config_version_t *version);
void config_get_serial_config(serial_config_t *serial_config);
void config_set_serial_config(serial_config_t *serial_config);
void config_get_tcp_config(tcp_config_t *tcp_config);
void config_set_tcp_config(tcp_config_t *tcp_config);
void config_get_net_info(wiz_NetInfo *net_info);
void config_set_net_info(wiz_NetInfo *net_info);

#endif // _CONFIG_H_
