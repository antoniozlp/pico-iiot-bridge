#ifndef _SYSTEM_CONFIG_H_
#define _SYSTEM_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>
#include "wizchip_conf.h"
#include "hardware/uart.h"
#include "hardware/flash.h"
#include "logger.h"
#include "modbus_request.h"

// Flash configuration
#define FLASH_TARGET_OFFSET 0x1F0000 // Last 64KB block (1,984KB offset)
// In debugger Memory view use: 0x101F0000 (XIP_BASE + FLASH_TARGET_OFFSET)
#define CONFIG_VERSION_MAJOR 0
#define CONFIG_VERSION_MINOR 2  // Incremented for Modbus RTU server configuration
#define CONFIG_VERSION_PATCH 0

// ============================================================================
// Tag Database Constants (used by both config and tag_database modules)
// ============================================================================

#define TAG_NAME_MAX_LEN                16      // Tag name length (15 chars + null)
#define TAG_DATABASE_MAX_TAGS           128     // Maximum runtime tags (RAM)
#define TAG_DB_MAX_PERSISTENT_TAGS      64      // Maximum persistent tags (flash)

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

// Modbus RTU client configuration (uses modbus_request_config_t from modbus_request.h)
typedef struct {
	uint8_t enable;                         // Enable/disable Modbus RTU client
	uint8_t serial_id;                      // UART to use: 0=UART0, 1=UART1
    modbus_request_config_t requests[MODBUS_REQUESTS_MAX];
} modbus_rtu_client_config_t;

// ============================================================================
// Modbus RTU Server Configuration
// ============================================================================

#define MODBUS_SERVER_MEMORY_BLOCKS_MAX    10  // Maximum memory blocks in the server memory map

/**
 * @brief One contiguous block in the Modbus server memory map
 *
 * Maps a range of Modbus addresses [start_address, start_address + count)
 * to up to MODBUS_MAX_REG_COUNT tag handles. Each position i in the block
 * maps to tag_handles[i]; MODBUS_TAG_MAP_INVALID (0xFF) means unmapped.
 */
typedef struct {
    uint8_t                     enabled;                            // Enable/disable this memory block
    modbus_data_type_t          data_type;                          // Register type
    uint8_t                     writable;                           // Allow master writes (COIL/HOLDING_REG only)
    uint16_t                    start_address;                      // First Modbus address in this block
    uint16_t                    count;                              // Number of registers/coils in this block
    modbus_register_encoding_t  encoding;                           // 32-bit word/byte order
    uint8_t                     tag_handles[MODBUS_MAX_REG_COUNT];  // Tag handle per register offset
} modbus_server_memory_block_t;

/**
 * @brief Modbus RTU server (slave) configuration
 */
typedef struct {
    uint8_t                        enable;                                          // Enable/disable the server task
    uint8_t                        serial_id;                                       // UART to use: 0=UART0, 1=UART1
    uint8_t                        server_address;                                  // RTU slave address (1-247)
    modbus_server_memory_block_t   memory_blocks[MODBUS_SERVER_MEMORY_BLOCKS_MAX];
} modbus_rtu_server_config_t;

// ============================================================================
// Tag Database Configuration (Persistent)
// ============================================================================

/**
 * @brief Tag data types
 */
typedef enum {
    TAG_TYPE_BOOL = 0,
    TAG_TYPE_UINT8,
    TAG_TYPE_UINT16,
    TAG_TYPE_UINT32,
    TAG_TYPE_INT16,
    TAG_TYPE_INT32,
    TAG_TYPE_FLOAT,
} tag_data_type_t;

/**
 * @brief Tag definition stored in flash
 * 
 * Contains only the tag metadata needed to recreate tags on boot.
 * Runtime data (value, quality, timestamp) is not persisted.
 */
typedef struct {
    char name[TAG_NAME_MAX_LEN];            // Tag name (uses constant from tag_database.h)
    uint8_t data_type;                      // tag_data_type_t (as uint8_t for packing)
    uint8_t enabled;                        // 1=active, 0=deleted/unused slot
    uint8_t reserved[2];                    // Padding for alignment
} tag_definition_t;

/**
 * @brief Tag database configuration (persistent)
 */
typedef struct {
    uint16_t tag_count;                             // Number of defined tags
    uint8_t auto_create_enabled;                    // Allow runtime tag creation
    uint8_t reserved;                               // Padding
    tag_definition_t tags[TAG_DB_MAX_PERSISTENT_TAGS];
} tag_database_config_t;

// ============================================================================
// System Configuration Structure
// ============================================================================

typedef struct
{
	config_version_t version;
	device_config_t device;
    wiz_NetInfo net_info;
    serial_config_t serial0;
    serial_config_t serial1;
    serial_to_tcp_mode_config_t serial_to_tcp_mode;
	modbus_rtu_client_config_t modbus_rtu_client;
	modbus_rtu_server_config_t modbus_rtu_server;
	tag_database_config_t tag_database;
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

// Modbus RTU server configuration (includes memory blocks)
bool config_get_modbus_rtu_server_config(modbus_rtu_server_config_t *modbus_rtu_server_config);
bool config_set_modbus_rtu_server_config(const modbus_rtu_server_config_t *modbus_rtu_server_config);

// Tag database configuration
bool config_get_tag_database(tag_database_config_t *tag_db_config);
bool config_set_tag_database(const tag_database_config_t *tag_db_config);
bool config_add_tag_definition(const char *name, uint8_t data_type);
bool config_remove_tag_definition(const char *name);
bool config_get_tag_definition(const char *name, tag_definition_t *def_out);

#endif // _SYSTEM_CONFIG_H_
