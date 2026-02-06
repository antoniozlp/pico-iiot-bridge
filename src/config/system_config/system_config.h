#ifndef _SYSTEM_CONFIG_H_
#define _SYSTEM_CONFIG_H_

#include <stdint.h>
#include "wizchip_conf.h"
#include "hardware/uart.h"
#include "hardware/flash.h"
#include "logger.h"

// Flash configuration
#define FLASH_TARGET_OFFSET 0x1F0000 // Last 64KB block (1,984KB offset)
#define CONFIG_VERSION_MAJOR 0
#define CONFIG_VERSION_MINOR 0
#define CONFIG_VERSION_PATCH 1

// Calculate buffer size for flash operations (rounded up to page boundary)
#define CONFIG_BUFFER_SIZE ((sizeof(system_config_t) + FLASH_PAGE_SIZE - 1) & ~(FLASH_PAGE_SIZE - 1))


typedef struct
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} config_version_t;

typedef struct 
{
	uint8_t device_id[20];
	logger_config_t logger_config;
} device_config_t;

typedef struct
{
	uint32_t baud;
	uint8_t databits;
	uart_parity_t parity;
	uint8_t stopbits;
	bool flow_control_cts;
	bool flow_control_rts;
} serial_config_t;

typedef enum {
    TCP_MODE_SERVER = 0,  // Listen for incoming connections
    TCP_MODE_CLIENT = 1   // Connect to remote server
} tcp_mode_t;

typedef struct
{
	uint8_t enable;
	uint8_t serial_id;         // 0: UART0, 1: UART1
	tcp_mode_t mode;           // Server or Client
	uint16_t local_port;       // Port to listen on (server mode)
	uint16_t timeout_s;        // Connection timeout in seconds
	uint16_t keepalive_s;      // Keepalive interval in seconds
	uint8_t max_connections;   // Maximum simultaneous connections (server mode)
	// For client mode:
	uint8_t remote_ip[4];      // Remote server IP address
	uint16_t remote_port;      // Remote server port
} serial_to_tcp_mode_config_t;

// Modbus data types
typedef enum {
    MODBUS_DATA_TYPE_COIL = 0,              // Discrete output coils (read/write, 1 bit)
    MODBUS_DATA_TYPE_DISCRETE_INPUT = 1,    // Discrete inputs (read-only, 1 bit)
    MODBUS_DATA_TYPE_INPUT_REGISTER = 2,    // Input registers (read-only, 16 bit)
    MODBUS_DATA_TYPE_HOLDING_REGISTER = 3   // Holding registers (read/write, 16 bit)
} modbus_rtu_data_type_t;

// Modbus operation type
typedef enum {
    MODBUS_OP_READ = 0,
    MODBUS_OP_WRITE = 1
} modbus_rtu_operation_t;

// Modbus RTU data point configuration (stored in flash)
#define MODBUS_RTU_DATA_POINTS_MAX   10     // Maximum data points per client
#define MODBUS_RTU_MAX_REG_COUNT     10     // Max registers/coils per data point

typedef struct {
    uint8_t enabled;                        // Enable/disable this data point
    uint8_t slave_address;                  // Modbus slave address (1-247)
    modbus_rtu_data_type_t data_type;       // Type of data to read/write
    modbus_rtu_operation_t operation;       // Read or write
    uint16_t start_address;                 // Starting register/coil address
    uint16_t count;                         // Number of registers/coils
} modbus_rtu_data_point_config_t;

// Modbus RTU client configuration
typedef struct {
	uint8_t enable;                         // Enable/disable Modbus RTU client
	uint8_t serial_id;                      // UART to use: 0=UART0, 1=UART1
    modbus_rtu_data_point_config_t data_points[MODBUS_RTU_DATA_POINTS_MAX];
} modbus_rtu_client_config_t;


typedef struct
{
	config_version_t version;
	device_config_t device;
    wiz_NetInfo net_info;
    serial_config_t serial0;
    serial_config_t serial1;
    serial_to_tcp_mode_config_t serial_to_tcp_mode;
	modbus_rtu_client_config_t modbus_rtu_client;
} system_config_t;


void config_set_default(void);
bool config_load_from_flash(void);
bool config_save_to_flash(void);

// Version
bool config_get_version(config_version_t *version);

// Device configuration
bool config_get_device_config(device_config_t *device_config);
bool config_set_device_config(device_config_t *device_config);

// Network configuration
bool config_get_net_info(wiz_NetInfo *net_info);
bool config_set_net_info(wiz_NetInfo *net_info);

// Serial configuration (specify UART ID: 0 or 1)
bool config_get_serial_config(uint8_t uart_id, serial_config_t *serial_config);
bool config_set_serial_config(uint8_t uart_id, serial_config_t *serial_config);

// Serial-to-TCP mode configuration
bool config_get_serial_to_tcp_mode(serial_to_tcp_mode_config_t *mode_config);
bool config_set_serial_to_tcp_mode(serial_to_tcp_mode_config_t *mode_config);

// Modbus RTU client configuration (includes data points)
bool config_get_modbus_rtu_client_config(modbus_rtu_client_config_t *modbus_rtu_client_config);
bool config_set_modbus_rtu_client_config(modbus_rtu_client_config_t *modbus_rtu_client_config);

#endif // _SYSTEM_CONFIG_H_
