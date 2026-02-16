/**
 * @file modbus_request.h
 * @brief Shared Modbus request types and constants (RTU and TCP)
 *
 * Defines configuration and result structures used by both Modbus RTU
 * and Modbus TCP clients. Transport-agnostic.
 */

#ifndef _MODBUS_REQUEST_H_
#define _MODBUS_REQUEST_H_

#include <stdint.h>

#include "nanomodbus.h"

/* Modbus data types (FC 01-04) */
typedef enum {
    MODBUS_DATA_TYPE_COIL = 0,              /* Discrete output coils (read/write, 1 bit) */
    MODBUS_DATA_TYPE_DISCRETE_INPUT = 1,    /* Discrete inputs (read-only, 1 bit) */
    MODBUS_DATA_TYPE_INPUT_REGISTER = 2,    /* Input registers (read-only, 16 bit) */
    MODBUS_DATA_TYPE_HOLDING_REGISTER = 3   /* Holding registers (read/write, 16 bit) */
} modbus_data_type_t;

/* Modbus operation type */
typedef enum {
    MODBUS_OP_READ = 0,
    MODBUS_OP_WRITE = 1
} modbus_operation_t;

/* Request limits (shared by RTU and TCP) */
#define MODBUS_REQUESTS_MAX      10     /* Maximum requests per client */
#define MODBUS_MAX_REG_COUNT     10     /* Max registers/coils per request */
#define MODBUS_TAG_MAP_INVALID   0xFF   /* Tag handle indicating unmapped register */

/**
 * @brief Modbus request configuration (stored in flash)
 *
 * Same structure for RTU (slave_address) and TCP (unit_id).
 */
typedef struct {
    uint8_t enabled;                        /* Enable/disable this request */
    uint8_t slave_address;                  /* RTU slave address (1-247) or TCP unit ID (0-255) */
    modbus_data_type_t data_type;           /* Type of data to read/write */
    modbus_operation_t operation;           /* Read or write */
    uint16_t start_address;                 /* Starting register/coil address */
    uint16_t count;                         /* Number of registers/coils */

    /* Tag mapping: which tag receives which register/coil value */
    /* MODBUS_TAG_MAP_INVALID (0xFF) = register not mapped to any tag */
    uint8_t tag_handles[MODBUS_MAX_REG_COUNT];
} modbus_request_config_t;

/**
 * @brief Modbus request result (runtime only, not stored)
 *
 * Contains the result of the last read/write operation.
 * Updated after each poll cycle.
 */
typedef struct {
    nmbs_error last_error;                  /* Last operation error code */
    uint32_t last_poll_time_ms;             /* Timestamp of last poll (for diagnostics) */
    union {
        uint16_t registers[MODBUS_MAX_REG_COUNT];  /* For holding/input registers */
        uint8_t coils[MODBUS_MAX_REG_COUNT];     /* For coils/discrete inputs (0 or 1) */
    } data;
} modbus_request_result_t;

#endif /* _MODBUS_REQUEST_H_ */
