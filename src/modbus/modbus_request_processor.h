/**
 * @file modbus_request_processor.h
 * @brief Execute a Modbus request via nanoMODBUS (RTU or TCP)
 *
 * Transport-agnostic: works with any nmbs_t instance.
 */

#ifndef _MODBUS_REQUEST_PROCESSOR_H_
#define _MODBUS_REQUEST_PROCESSOR_H_

#include "modbus_request.h"

struct nmbs_t;

/**
 * @brief Process a single Modbus request
 *
 * Reads or writes registers/coils via nanoMODBUS, then maps to/from tags.
 *
 * @param nmbs   Pointer to nanoMODBUS client instance (RTU or TCP)
 * @param config Pointer to request configuration
 * @param result Pointer to request result (updated on return)
 */
void modbus_request_process(struct nmbs_t *nmbs,
                            const modbus_request_config_t *config,
                            modbus_request_result_t *result);

#endif /* _MODBUS_REQUEST_PROCESSOR_H_ */
