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

// ============================================================================
// Slave (server) helpers — fill buffers from tags and write tags from buffers
// ============================================================================

/**
 * @brief Fill a register output buffer from tag database values
 *
 * Used by the Modbus RTU slave READ holding/input register callbacks.
 * tag_handles[handle_offset + i] maps to registers_out[i].
 * For 32-bit/float types two consecutive registers are produced and the next
 * handle index is skipped (same pattern as the master write path).
 *
 * @param tag_handles    Tag handle array (at least handle_offset + count entries)
 * @param handle_offset  Starting index within tag_handles
 * @param count          Number of registers to produce (= number of elements in registers_out)
 * @param encoding       32-bit word/byte order
 * @param registers_out  Output register buffer (must have room for count uint16_t values)
 */
void modbus_tags_to_registers(const uint8_t *tag_handles, uint16_t handle_offset,
                               uint16_t count, modbus_register_encoding_t encoding,
                               uint16_t *registers_out);

/**
 * @brief Fill a coil/discrete bitfield from tag database values
 *
 * Used by the Modbus RTU slave READ coil/discrete input callbacks.
 * tag_handles[handle_offset + i] maps to bit i in coils_out.
 *
 * @param tag_handles    Tag handle array
 * @param handle_offset  Starting index within tag_handles
 * @param count          Number of bits to produce
 * @param coils_out      Output nmbs_bitfield (caller must zero it first)
 */
void modbus_tags_to_coils(const uint8_t *tag_handles, uint16_t handle_offset,
                           uint16_t count, nmbs_bitfield coils_out);

/**
 * @brief Write register buffer values to tag database
 *
 * Used by the Modbus RTU slave WRITE multiple registers callback.
 * registers[i] maps to tag_handles[handle_offset + i].
 *
 * @param tag_handles    Tag handle array
 * @param handle_offset  Starting index within tag_handles
 * @param count          Number of registers
 * @param encoding       32-bit word/byte order
 * @param registers      Input register buffer
 */
void modbus_registers_to_tags(const uint8_t *tag_handles, uint16_t handle_offset,
                               uint16_t count, modbus_register_encoding_t encoding,
                               const uint16_t *registers);

/**
 * @brief Write coil bitfield values to tag database
 *
 * Used by the Modbus RTU slave WRITE single/multiple coils callbacks.
 * Bit i of coils maps to tag_handles[handle_offset + i].
 *
 * @param tag_handles    Tag handle array
 * @param handle_offset  Starting index within tag_handles
 * @param count          Number of coils
 * @param coils          Input nmbs_bitfield
 */
void modbus_coils_to_tags(const uint8_t *tag_handles, uint16_t handle_offset,
                           uint16_t count, const nmbs_bitfield coils);

#endif /* _MODBUS_TAG_MAPPING_H_ */
