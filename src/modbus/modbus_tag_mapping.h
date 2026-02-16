/**
 * @file modbus_tag_mapping.h
 * @brief Map Modbus register/coil data to and from tag database
 *
 * Transport-agnostic. Works with coils, discrete inputs, and registers.
 */

#ifndef _MODBUS_TAG_MAPPING_H_
#define _MODBUS_TAG_MAPPING_H_

#include "modbus_request.h"

/**
 * @brief Map Modbus register/coil data to tags (after successful read)
 *
 * Converts Modbus data to tag values based on tag_handles mapping.
 * For coils/discrete inputs: uses result->data.coils[].
 * For registers: uses result->data.registers[].
 * Handles multi-register types (INT32, FLOAT) with big-endian byte order.
 *
 * @param config Pointer to request configuration (defines mapping)
 * @param result Pointer to request result (contains read data)
 */
void modbus_map_to_tags(const modbus_request_config_t *config,
                        const modbus_request_result_t *result);

/**
 * @brief Map tag values to Modbus buffer (before write)
 *
 * Reads tag values and populates result->data for Modbus write.
 * For coils: uses result->data.coils[].
 * For holding registers: uses result->data.registers[].
 *
 * @param config Pointer to request configuration (defines mapping)
 * @param result Pointer to request result (buffer to populate)
 */
void modbus_map_from_tags(const modbus_request_config_t *config,
                          modbus_request_result_t *result);

/**
 * @brief Mark all mapped tags as BAD quality (after failed read)
 *
 * @param config Pointer to request configuration
 */
void modbus_mark_mapped_tags_bad(const modbus_request_config_t *config);

#endif /* _MODBUS_TAG_MAPPING_H_ */
