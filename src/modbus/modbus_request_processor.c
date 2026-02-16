/**
 * @file modbus_request_processor.c
 * @brief Execute a Modbus request via nanoMODBUS (RTU or TCP)
 */

#include "modbus_request_processor.h"

#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "logger.h"
#include "modbus_tag_mapping.h"
#include "nanomodbus.h"

/**
 * @brief Process a single Modbus request
 */
void modbus_request_process(struct nmbs_t *nmbs,
                            const modbus_request_config_t *config,
                            modbus_request_result_t *result)
{
    nmbs_t *client = (nmbs_t *)nmbs;

    if (client == NULL || config == NULL || result == NULL || !config->enabled)
    {
        return;
    }

    nmbs_set_destination_rtu_address(client, config->slave_address);

    nmbs_error err = NMBS_ERROR_NONE;

    if (config->operation == MODBUS_OP_READ)
    {
        switch (config->data_type)
        {
            case MODBUS_DATA_TYPE_COIL:
            {
                nmbs_bitfield coils = {0};
                err = nmbs_read_coils(client, config->start_address, config->count, coils);
                if (err == NMBS_ERROR_NONE)
                {
                    for (uint16_t i = 0; i < config->count && i < MODBUS_MAX_REG_COUNT; i++)
                    {
                        result->data.coils[i] = nmbs_bitfield_read(coils, i) ? 1 : 0;
                    }
                }
                break;
            }

            case MODBUS_DATA_TYPE_DISCRETE_INPUT:
            {
                nmbs_bitfield inputs = {0};
                err = nmbs_read_discrete_inputs(client, config->start_address, config->count, inputs);
                if (err == NMBS_ERROR_NONE)
                {
                    for (uint16_t i = 0; i < config->count && i < MODBUS_MAX_REG_COUNT; i++)
                    {
                        result->data.coils[i] = nmbs_bitfield_read(inputs, i) ? 1 : 0;
                    }
                }
                break;
            }

            case MODBUS_DATA_TYPE_INPUT_REGISTER:
                err = nmbs_read_input_registers(client, config->start_address,
                                                config->count, result->data.registers);
                break;

            case MODBUS_DATA_TYPE_HOLDING_REGISTER:
                err = nmbs_read_holding_registers(client, config->start_address,
                                                 config->count, result->data.registers);
                break;

            default:
                err = NMBS_ERROR_INVALID_ARGUMENT;
                break;
        }
    }
    else /* MODBUS_OP_WRITE */
    {
        modbus_map_from_tags(config, result);

        switch (config->data_type)
        {
            case MODBUS_DATA_TYPE_COIL:
            {
                nmbs_bitfield coils = {0};
                for (uint16_t i = 0; i < config->count && i < MODBUS_MAX_REG_COUNT; i++)
                {
                    nmbs_bitfield_write(coils, i, result->data.coils[i]);
                }
                err = nmbs_write_multiple_coils(client, config->start_address, config->count, coils);
                break;
            }

            case MODBUS_DATA_TYPE_HOLDING_REGISTER:
                err = nmbs_write_multiple_registers(client, config->start_address,
                                                    config->count, result->data.registers);
                break;

            case MODBUS_DATA_TYPE_DISCRETE_INPUT:
            case MODBUS_DATA_TYPE_INPUT_REGISTER:
                err = NMBS_ERROR_INVALID_ARGUMENT;
                LOG_ERROR("Cannot write to read-only Modbus data type");
                break;

            default:
                err = NMBS_ERROR_INVALID_ARGUMENT;
                break;
        }
    }

    result->last_error = err;
    result->last_poll_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (config->operation == MODBUS_OP_READ)
    {
        if (err == NMBS_ERROR_NONE)
        {
            modbus_map_to_tags(config, result);
        }
        else
        {
            modbus_mark_mapped_tags_bad(config);
            LOG_DEBUG("Modbus read failed: slave=%u addr=%u count=%u err=%d",
                     config->slave_address, config->start_address, config->count, err);
        }
    }
    else if (err != NMBS_ERROR_NONE)
    {
        LOG_DEBUG("Modbus write failed: slave=%u addr=%u count=%u err=%d",
                 config->slave_address, config->start_address, config->count, err);
    }
}
