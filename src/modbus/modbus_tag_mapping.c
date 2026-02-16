/**
 * @file modbus_tag_mapping.c
 * @brief Map Modbus register/coil data to and from tag database
 */

#include "modbus_tag_mapping.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "logger.h"
#include "tag_database.h"

/**
 * @brief Map Modbus register/coil data to tags (after successful read)
 */
void modbus_map_to_tags(const modbus_request_config_t *config,
                        const modbus_request_result_t *result)
{
    if (config == NULL || result == NULL)
    {
        return;
    }

    const bool is_coil_type = (config->data_type == MODBUS_DATA_TYPE_COIL ||
                              config->data_type == MODBUS_DATA_TYPE_DISCRETE_INPUT);

    if (is_coil_type)
    {
        /* Coils/discrete inputs: 1:1 mapping, data in result->data.coils[] */
        for (uint16_t i = 0; i < config->count && i < MODBUS_MAX_REG_COUNT; i++)
        {
            if (config->tag_handles[i] == MODBUS_TAG_MAP_INVALID)
            {
                continue;
            }

            tag_metadata_t metadata;
            if (!tag_db_get_metadata(config->tag_handles[i], &metadata))
            {
                LOG_WARN("Invalid tag handle in Modbus mapping: %d", config->tag_handles[i]);
                continue;
            }

            tag_value_t value;
            memset(&value, 0, sizeof(value));
            value.bool_val = (result->data.coils[i] != 0);

            if (tag_db_write(config->tag_handles[i], value, TAG_QUALITY_GOOD))
            {
                LOG_DEBUG("Mapped Modbus coil[%d] to tag %s (handle=%d)",
                         i, metadata.name, config->tag_handles[i]);
            }
        }
    }
    else
    {
        /* Registers: data in result->data.registers[], multi-register types supported */
        uint16_t reg_idx = 0;

        for (uint16_t i = 0; i < config->count && i < MODBUS_MAX_REG_COUNT; i++)
        {
            tag_handle_t handle = config->tag_handles[i];

            if (handle == MODBUS_TAG_MAP_INVALID)
            {
                continue;
            }

            tag_metadata_t metadata;
            if (!tag_db_get_metadata(handle, &metadata))
            {
                LOG_WARN("Invalid tag handle in Modbus mapping: %d", handle);
                continue;
            }

            tag_value_t value;
            memset(&value, 0, sizeof(value));
            bool write_ok = false;
            uint16_t regs_consumed = 0;

            switch (metadata.data_type)
            {
                case TAG_TYPE_BOOL:
                    if (reg_idx < config->count)
                    {
                        value.bool_val = (result->data.registers[reg_idx] != 0);
                        write_ok = true;
                        regs_consumed = 1;
                    }
                    break;

                case TAG_TYPE_UINT8:
                    if (reg_idx < config->count)
                    {
                        value.u8_val = (uint8_t)(result->data.registers[reg_idx] & 0xFF);
                        write_ok = true;
                        regs_consumed = 1;
                    }
                    break;

                case TAG_TYPE_UINT16:
                    if (reg_idx < config->count)
                    {
                        value.u16_val = result->data.registers[reg_idx];
                        write_ok = true;
                        regs_consumed = 1;
                    }
                    break;

                case TAG_TYPE_INT16:
                    if (reg_idx < config->count)
                    {
                        value.i16_val = (int16_t)result->data.registers[reg_idx];
                        write_ok = true;
                        regs_consumed = 1;
                    }
                    break;

                case TAG_TYPE_UINT32:
                case TAG_TYPE_INT32:
                    if (reg_idx + 1 < config->count)
                    {
                        value.u32_val = ((uint32_t)result->data.registers[reg_idx] << 16) |
                                        result->data.registers[reg_idx + 1];
                        write_ok = true;
                        regs_consumed = 2;
                    }
                    else
                    {
                        LOG_WARN("Tag %s requires 2 registers but only %d available at index %d",
                                metadata.name, config->count - reg_idx, reg_idx);
                    }
                    break;

                case TAG_TYPE_FLOAT:
                    if (reg_idx + 1 < config->count)
                    {
                        uint32_t raw = ((uint32_t)result->data.registers[reg_idx] << 16) |
                                       result->data.registers[reg_idx + 1];
                        memcpy(&value.float_val, &raw, sizeof(float));
                        write_ok = true;
                        regs_consumed = 2;
                    }
                    else
                    {
                        LOG_WARN("Tag %s requires 2 registers but only %d available at index %d",
                                metadata.name, config->count - reg_idx, reg_idx);
                    }
                    break;

                default:
                    LOG_ERROR("Unknown tag data type: %d", metadata.data_type);
                    break;
            }

            if (write_ok)
            {
                if (tag_db_write(handle, value, TAG_QUALITY_GOOD))
                {
                    LOG_DEBUG("Mapped Modbus reg[%d] to tag %s (handle=%d, regs=%d)",
                             reg_idx, metadata.name, handle, regs_consumed);
                }
                reg_idx += regs_consumed;
            }
        }
    }
}

/**
 * @brief Map tag values to Modbus buffer (before write)
 */
void modbus_map_from_tags(const modbus_request_config_t *config,
                          modbus_request_result_t *result)
{
    if (config == NULL || result == NULL)
    {
        return;
    }

    if (config->data_type == MODBUS_DATA_TYPE_COIL)
    {
        memset(result->data.coils, 0, MODBUS_MAX_REG_COUNT);

        for (uint16_t i = 0; i < config->count && i < MODBUS_MAX_REG_COUNT; i++)
        {
            if (config->tag_handles[i] == MODBUS_TAG_MAP_INVALID)
            {
                continue;
            }

            tag_value_t value;
            if (tag_db_read(config->tag_handles[i], &value, NULL, NULL))
            {
                result->data.coils[i] = (value.bool_val || value.u16_val != 0 ||
                                        value.u32_val != 0 || value.i16_val != 0 ||
                                        value.i32_val != 0 || value.float_val != 0.0f) ? 1 : 0;
            }
            else
            {
                LOG_WARN("Failed to read tag handle %d for coil %d", config->tag_handles[i], i);
            }
        }
    }
    else if (config->data_type == MODBUS_DATA_TYPE_HOLDING_REGISTER)
    {
        memset(result->data.registers, 0, sizeof(result->data.registers));

        bool prev_consumed_2regs = false;

        for (uint16_t i = 0; i < config->count && i < MODBUS_MAX_REG_COUNT; i++)
        {
            if (prev_consumed_2regs)
            {
                prev_consumed_2regs = false;
                continue;
            }

            if (config->tag_handles[i] == MODBUS_TAG_MAP_INVALID)
            {
                result->data.registers[i] = 0;
                continue;
            }

            tag_metadata_t metadata;
            if (!tag_db_get_metadata(config->tag_handles[i], &metadata))
            {
                LOG_WARN("Invalid tag handle in Modbus write mapping: %d", config->tag_handles[i]);
                result->data.registers[i] = 0;
                continue;
            }

            tag_value_t value;
            if (!tag_db_read(config->tag_handles[i], &value, NULL, NULL))
            {
                LOG_WARN("Failed to read tag %s for register write", metadata.name);
                result->data.registers[i] = 0;
                continue;
            }

            switch (metadata.data_type)
            {
                case TAG_TYPE_BOOL:
                    result->data.registers[i] = value.bool_val ? 1 : 0;
                    break;

                case TAG_TYPE_UINT8:
                    result->data.registers[i] = value.u8_val;
                    break;

                case TAG_TYPE_UINT16:
                    result->data.registers[i] = value.u16_val;
                    break;

                case TAG_TYPE_INT16:
                    result->data.registers[i] = (uint16_t)value.i16_val;
                    break;

                case TAG_TYPE_UINT32:
                case TAG_TYPE_INT32:
                    if (i + 1 < config->count)
                    {
                        result->data.registers[i] = (uint16_t)(value.u32_val >> 16);
                        result->data.registers[i + 1] = (uint16_t)(value.u32_val & 0xFFFF);
                        prev_consumed_2regs = true;
                    }
                    else
                    {
                        result->data.registers[i] = (uint16_t)(value.u32_val & 0xFFFF);
                        LOG_WARN("Tag %s (32-bit) truncated: only 1 register available", metadata.name);
                    }
                    break;

                case TAG_TYPE_FLOAT:
                    if (i + 1 < config->count)
                    {
                        uint32_t raw;
                        memcpy(&raw, &value.float_val, sizeof(float));
                        result->data.registers[i] = (uint16_t)(raw >> 16);
                        result->data.registers[i + 1] = (uint16_t)(raw & 0xFFFF);
                        prev_consumed_2regs = true;
                    }
                    else
                    {
                        uint32_t raw;
                        memcpy(&raw, &value.float_val, sizeof(float));
                        result->data.registers[i] = (uint16_t)(raw & 0xFFFF);
                        LOG_WARN("Tag %s (float) truncated: only 1 register available", metadata.name);
                    }
                    break;

                default:
                    result->data.registers[i] = 0;
                    LOG_WARN("Unknown tag type %d for write", metadata.data_type);
                    break;
            }
        }
    }
}

/**
 * @brief Mark all mapped tags as BAD quality (after failed read)
 */
void modbus_mark_mapped_tags_bad(const modbus_request_config_t *config)
{
    if (config == NULL)
    {
        return;
    }

    tag_value_t dummy_value;
    memset(&dummy_value, 0, sizeof(dummy_value));

    for (uint16_t i = 0; i < config->count && i < MODBUS_MAX_REG_COUNT; i++)
    {
        if (config->tag_handles[i] != MODBUS_TAG_MAP_INVALID)
        {
            tag_db_write(config->tag_handles[i], dummy_value, TAG_QUALITY_BAD);
        }
    }
}
