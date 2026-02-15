/**
 * @file http_utils.c
 * @brief HTTP CGI handlers and utilities implementation
 */

#include "http_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "httpUtil.h"

#include "system_config.h"
#include "logger.h"
#include "tag_database.h"

/**
 * @brief Safely copy text string with null termination
 * 
 * @param dst Destination buffer
 * @param dst_len Size of destination buffer
 * @param src Source string (uint8_t*)
 */
static void copy_text(char *dst, size_t dst_len, const uint8_t *src)
{
	if(!dst || !src || dst_len == 0) return;

	strncpy(dst, (const char *)src, dst_len - 1);
	dst[dst_len - 1] = 0;
}

/**
 * @brief Parse IP address from string (a.b.c.d format)
 * 
 * @param ip Output buffer for 4-byte IP address
 * @param str Input string in dotted decimal format
 */
static void parse_ip_addr(uint8_t *ip, const char *str)
{
    if (!ip || !str) return;
    sscanf(str, "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);
}

/**
 * @brief Parse MAC address from string (xx:xx:xx:xx:xx:xx format)
 * 
 * @param mac Output buffer for 6-byte MAC address
 * @param str Input string in colon-separated hexadecimal format
 */
static void parse_mac_addr(uint8_t *mac, const char *str)
{
    if (!mac || !str) return;
    sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

/**
 * @brief Handle network configuration update from CGI
 * 
 * Processes set_network.cgi POST request with parameters:
 * mac, ip, sn (subnet), gw (gateway), dns, dhcp
 * 
 * @param uri URI string with query parameters
 * @return HTTP_OK if updated and saved, HTTP_FAILED on error
 */
static uint8_t handle_set_network(uint8_t * uri)
{
    uint8_t changed = 0;
    uint8_t * param;
    wiz_NetInfo net_info;
    
    // Get current network configuration
    if (!config_get_net_info(&net_info))
    {
        LOG_ERROR("Failed to get network config");
        return HTTP_FAILED;
    }

    if((param = get_http_param_value((char *)uri, "mac")))
    {
        parse_mac_addr(net_info.mac, (char *)param);
        changed = 1;
    }

    if((param = get_http_param_value((char *)uri, "ip")))
    {
        parse_ip_addr(net_info.ip, (char *)param);
        changed = 1;
    }

    if((param = get_http_param_value((char *)uri, "sn")))
    {
        parse_ip_addr(net_info.sn, (char *)param);
        changed = 1;
    }

    if((param = get_http_param_value((char *)uri, "gw")))
    {
        parse_ip_addr(net_info.gw, (char *)param);
        changed = 1;
    }

    if((param = get_http_param_value((char *)uri, "dns")))
    {
        parse_ip_addr(net_info.dns, (char *)param);
        changed = 1;
    }

    if((param = get_http_param_value((char *)uri, "dhcp")))
    {
        int dhcp_val = ATOI(param, 10);
        if (dhcp_val >= 0 && dhcp_val <= 255)
        {
            net_info.dhcp = (uint8_t)dhcp_val;
            changed = 1;
        }
    }

    if(changed)
    {
        if (!config_set_net_info(&net_info))
        {
            LOG_ERROR("Failed to set network config");
            return HTTP_FAILED;
        }
        
        LOG_INFO("Network config updated");
        if (!config_save_to_flash())
        {
            LOG_ERROR("Failed to save config to flash");
            return HTTP_FAILED;
        }
    }

    return changed ? HTTP_OK : HTTP_FAILED;
}

/**
 * @brief Handle serial port configuration update from CGI
 * 
 * Processes set_serial.cgi POST request with parameters:
 * uart (0 or 1), baud, databits, parity, stopbits, flowcts, flowrts
 * 
 * @param uri URI string with query parameters
 * @return HTTP_OK if updated and saved, HTTP_FAILED on error
 */
static uint8_t handle_set_serial(uint8_t * uri)
{
	uint8_t changed = 0;
	uint8_t * param;
	serial_config_t serial_config;
	uint8_t uart_id = 1;  // Default to UART1 (serial bridge)
	
	// Check if uart_id parameter is specified
	if((param = get_http_param_value((char *)uri, "uart")))
	{
		int val = ATOI(param, 10);
		if (val == 0 || val == 1) {
			uart_id = (uint8_t)val;
		}
	}
	
	// Get current serial configuration
	if (!config_get_serial_config(uart_id, &serial_config))
	{
		LOG_ERROR("Failed to get serial config");
		return HTTP_FAILED;
	}

	if((param = get_http_param_value((char *)uri, "baud")))
	{
		unsigned long baud_val = strtoul((char *)param, NULL, 10);
		if (baud_val >= 9600 && baud_val <= 921600)
		{
			serial_config.baud = (uint32_t)baud_val;
			changed = 1;
		}
		else
		{
			LOG_ERROR("Invalid baud rate %lu", baud_val);
		}
	}

	if((param = get_http_param_value((char *)uri, "databits")))
	{
		int databits_val = ATOI(param, 10);
		if (databits_val >= 5 && databits_val <= 8)
		{
			serial_config.databits = (uint8_t)databits_val;
			changed = 1;
		}
		else
		{
			LOG_ERROR("Invalid databits %d", databits_val);
		}
	}

	if((param = get_http_param_value((char *)uri, "parity")))
	{
		// Parse parity: "none", "even", "odd" or numeric (0, 1, 2)
		if (strcmp((char *)param, "none") == 0 || strcmp((char *)param, "0") == 0)
		{
			serial_config.parity = UART_PARITY_NONE;
			changed = 1;
		}
		else if (strcmp((char *)param, "even") == 0 || strcmp((char *)param, "1") == 0)
		{
			serial_config.parity = UART_PARITY_EVEN;
			changed = 1;
		}
		else if (strcmp((char *)param, "odd") == 0 || strcmp((char *)param, "2") == 0)
		{
			serial_config.parity = UART_PARITY_ODD;
			changed = 1;
		}
		else
		{
			LOG_ERROR("Invalid parity '%s'", (char *)param);
		}
	}

	if((param = get_http_param_value((char *)uri, "stopbits")))
	{
		int stopbits_val = ATOI(param, 10);
		if (stopbits_val == 1 || stopbits_val == 2)
		{
			serial_config.stopbits = (uint8_t)stopbits_val;
			changed = 1;
		}
		else
		{
			LOG_ERROR("Invalid stopbits %d", stopbits_val);
		}
	}

	if((param = get_http_param_value((char *)uri, "flowcts")))
	{
		int val = ATOI(param, 10);
		serial_config.flow_control_cts = (val != 0);
		changed = 1;
	}

	if((param = get_http_param_value((char *)uri, "flowrts")))
	{
		int val = ATOI(param, 10);
		serial_config.flow_control_rts = (val != 0);
		changed = 1;
	}

	if(changed)
	{
		if (!config_set_serial_config(uart_id, &serial_config))
		{
			LOG_ERROR("Failed to set serial config");
			return HTTP_FAILED;
		}
		
		const char *parity_str = serial_config.parity == UART_PARITY_NONE ? "none" :
								 serial_config.parity == UART_PARITY_EVEN ? "even" : "odd";
		
		LOG_INFO("Serial%d config updated: baud=%lu, databits=%u, parity=%s, stopbits=%u, flowcts=%d, flowrts=%d",
			   uart_id,
			   (unsigned long)serial_config.baud,
			   (unsigned int)serial_config.databits,
			   parity_str,
			   (unsigned int)serial_config.stopbits,
			   serial_config.flow_control_cts,
			   serial_config.flow_control_rts);
        
		if (!config_save_to_flash())
		{
			LOG_ERROR("Failed to save config to flash");
			return HTTP_FAILED;
		}
	}

	return changed ? HTTP_OK : HTTP_FAILED;
}

/**
 * @brief Handle serial-to-TCP configuration update from CGI
 * 
 * Processes set_s2tcp.cgi POST request with parameters:
 * enable, serial, mode, lport, timeout, keepalive, maxconn, remoteip, remoteport
 * 
 * @param uri URI string with query parameters
 * @return HTTP_OK if updated and saved, HTTP_FAILED on error
 */
static uint8_t handle_set_s2tcp(uint8_t * uri)
{
	uint8_t changed = 0;
	uint8_t * param;
	serial_to_tcp_mode_config_t mode_config;
	
	// Get current Serial-to-TCP configuration
	if (!config_get_serial_to_tcp_mode(&mode_config))
	{
		LOG_ERROR("Failed to get Serial-to-TCP config");
		return HTTP_FAILED;
	}

	if((param = get_http_param_value((char *)uri, "enable")))
	{
		int val = ATOI(param, 10);
		mode_config.enable = (val != 0);
		changed = 1;
	}

	if((param = get_http_param_value((char *)uri, "serial")))
	{
		int val = ATOI(param, 10);
		if (val == 0 || val == 1)
		{
			mode_config.serial_id = (uint8_t)val;
			changed = 1;
		}
	}

	if((param = get_http_param_value((char *)uri, "mode")))
	{
		int val = ATOI(param, 10);
		if (val == 0 || val == 1)
		{
			mode_config.mode = (tcp_mode_t)val;
			changed = 1;
		}
	}

	if((param = get_http_param_value((char *)uri, "lport")))
	{
		int port_val = ATOI(param, 10);
		if (port_val >= 1024 && port_val <= 65535)
		{
			mode_config.local_port = (uint16_t)port_val;
			changed = 1;
		}
		else
		{
			LOG_ERROR("Invalid port %d (must be 1024-65535)", port_val);
		}
	}

	if((param = get_http_param_value((char *)uri, "timeout")))
	{
		int timeout_val = ATOI(param, 10);
		if (timeout_val > 0 && timeout_val <= 3600)
		{
			mode_config.timeout_s = (uint16_t)timeout_val;
			changed = 1;
		}
		else
		{
			LOG_ERROR("Invalid timeout %d (must be 1-3600)", timeout_val);
		}
	}

	if((param = get_http_param_value((char *)uri, "keepalive")))
	{
		int keepalive_val = ATOI(param, 10);
		if (keepalive_val > 0 && keepalive_val <= 600)
		{
			mode_config.keepalive_s = (uint16_t)keepalive_val;
			changed = 1;
		}
		else
		{
			LOG_ERROR("Invalid keepalive %d (must be 1-600)", keepalive_val);
		}
	}

	if((param = get_http_param_value((char *)uri, "maxconn")))
	{
		int maxconn_val = ATOI(param, 10);
		if (maxconn_val >= 1 && maxconn_val <= 4)
		{
			mode_config.max_connections = (uint8_t)maxconn_val;
			changed = 1;
		}
		else
		{
			LOG_ERROR("Invalid max connections %d (must be 1-4)", maxconn_val);
		}
	}

	if((param = get_http_param_value((char *)uri, "remoteip")))
	{
		char ip_buf[32];
		copy_text(ip_buf, sizeof(ip_buf), param);
		parse_ip_addr(mode_config.remote_ip, ip_buf);
		changed = 1;
	}

	if((param = get_http_param_value((char *)uri, "remoteport")))
	{
		int port_val = ATOI(param, 10);
		if (port_val >= 1024 && port_val <= 65535)
		{
			mode_config.remote_port = (uint16_t)port_val;
			changed = 1;
		}
	}

	if(changed)
	{
		if (!config_set_serial_to_tcp_mode(&mode_config))
		{
			LOG_ERROR("Failed to set Serial-to-TCP config");
			return HTTP_FAILED;
		}
		
		LOG_INFO("Serial-to-TCP config updated");

		if (!config_save_to_flash())
		{
			LOG_ERROR("Failed to save config to flash");
			return HTTP_FAILED;
		}
	}

	return changed ? HTTP_OK : HTTP_FAILED;
}

/**
 * @brief Handle Modbus RTU client configuration update from CGI
 * 
 * Processes set_modbus_client.cgi POST request with parameters:
 * enable, serial_id
 * 
 * @param uri URI string with query parameters
 * @return HTTP_OK if updated and saved, HTTP_FAILED on error
 */
static uint8_t handle_set_modbus_client(uint8_t * uri)
{
	uint8_t changed = 0;
	uint8_t * param;
	modbus_rtu_client_config_t client_config;
	
	// Get current Modbus RTU client configuration
	if (!config_get_modbus_rtu_client_config(&client_config))
	{
		LOG_ERROR("Failed to get Modbus RTU client config");
		return HTTP_FAILED;
	}

	if((param = get_http_param_value((char *)uri, "enable")))
	{
		int val = ATOI(param, 10);
		client_config.enable = (val != 0);
		changed = 1;
	}

	if((param = get_http_param_value((char *)uri, "serial_id")))
	{
		int val = ATOI(param, 10);
		if (val == 0 || val == 1)
		{
			client_config.serial_id = (uint8_t)val;
			changed = 1;
		}
	}

	if(changed)
	{
		if (!config_set_modbus_rtu_client_config(&client_config))
		{
			LOG_ERROR("Failed to set Modbus RTU client config");
			return HTTP_FAILED;
		}
		
		LOG_INFO("Modbus RTU client config updated: enable=%d, serial_id=%d",
			   client_config.enable, client_config.serial_id);

		if (!config_save_to_flash())
		{
			LOG_ERROR("Failed to save config to flash");
			return HTTP_FAILED;
		}
	}

	return changed ? HTTP_OK : HTTP_FAILED;
}

/**
 * @brief Handle Modbus RTU data point configuration update from CGI
 * 
 * Processes set_modbus_datapoint.cgi POST request with parameters:
 * dp_idx, enabled, slave_address, data_type, operation, start_address, count
 * 
 * @param uri URI string with query parameters
 * @return HTTP_OK if updated and saved, HTTP_FAILED on error
 */
static uint8_t handle_set_modbus_datapoint(uint8_t * uri)
{
	uint8_t changed = 0;
	uint8_t * param;
	modbus_rtu_client_config_t client_config;
	int dp_idx = -1;
	
	// Get data point index
	if((param = get_http_param_value((char *)uri, "dp_idx")))
	{
		dp_idx = ATOI(param, 10);
		if (dp_idx < 0 || dp_idx >= MODBUS_RTU_REQUESTS_MAX)
		{
			LOG_ERROR("Invalid request index %d", dp_idx);
			return HTTP_FAILED;
		}
	}
	else
	{
		LOG_ERROR("Missing dp_idx parameter");
		return HTTP_FAILED;
	}
	
	// Get current Modbus RTU configuration
	if (!config_get_modbus_rtu_client_config(&client_config))
	{
		LOG_ERROR("Failed to get Modbus RTU client config");
		return HTTP_FAILED;
	}

	modbus_rtu_request_config_t *req = &client_config.requests[dp_idx];

	if((param = get_http_param_value((char *)uri, "enabled")))
	{
		int val = ATOI(param, 10);
		req->enabled = (val != 0);
		changed = 1;
	}

	if((param = get_http_param_value((char *)uri, "slave_address")))
	{
		int val = ATOI(param, 10);
		if (val >= 1 && val <= 247)
		{
			req->slave_address = (uint8_t)val;
			changed = 1;
		}
	}

	if((param = get_http_param_value((char *)uri, "data_type")))
	{
		int val = ATOI(param, 10);
		if (val >= 0 && val <= 3)
		{
			req->data_type = (modbus_rtu_data_type_t)val;
			changed = 1;
		}
	}

	if((param = get_http_param_value((char *)uri, "operation")))
	{
		int val = ATOI(param, 10);
		if (val == 0 || val == 1)
		{
			req->operation = (modbus_rtu_operation_t)val;
			changed = 1;
		}
	}

	if((param = get_http_param_value((char *)uri, "start_address")))
	{
		int val = ATOI(param, 10);
		if (val >= 0 && val <= 65535)
		{
			req->start_address = (uint16_t)val;
			changed = 1;
		}
	}

	if((param = get_http_param_value((char *)uri, "count")))
	{
		int val = ATOI(param, 10);
		if (val >= 1 && val <= MODBUS_RTU_MAX_REG_COUNT)
		{
			req->count = (uint16_t)val;
			changed = 1;
		}
	}

	// Parse tag mappings (tag0 through tag9)
	for (uint8_t i = 0; i < MODBUS_RTU_MAX_REG_COUNT; i++)
	{
		char param_name[8];
		snprintf(param_name, sizeof(param_name), "tag%d", i);
		
		if((param = get_http_param_value((char *)uri, param_name)))
		{
			int val = ATOI(param, 10);
			// Value 255 means not mapped, 0-127 are valid tag handles
			if (val >= 0 && val <= 255)
			{
				req->tag_handles[i] = (uint8_t)val;
				changed = 1;
			}
		}
	}

	if(changed)
	{
		if (!config_set_modbus_rtu_client_config(&client_config))
		{
			LOG_ERROR("Failed to set Modbus RTU client config");
			return HTTP_FAILED;
		}
		
		LOG_INFO("Modbus RTU request %d updated: enabled=%d, slave=%d, type=%d, op=%d, addr=%d, count=%d",
			   dp_idx, req->enabled, req->slave_address, req->data_type,
			   req->operation, req->start_address, req->count);

		if (!config_save_to_flash())
		{
			LOG_ERROR("Failed to save config to flash");
			return HTTP_FAILED;
		}
	}

	return changed ? HTTP_OK : HTTP_FAILED;
}

/**
 * @brief Handle get_tags.cgi GET request
 * 
 * Returns JSON array of all tags with their current values, quality, and metadata.
 * Uses bounded buffer writes to prevent overflow.
 * 
 * @param buf Buffer to write JSON response
 * @param len Pointer to store response length
 * @return HTTP_OK on success
 */
static uint8_t handle_get_tags(uint8_t * buf, uint16_t * len)
{
	// Maximum safe buffer size (typical HTTP response buffer minus headers)
	// DATA_BUF_SIZE is 2048, minus ~200 for headers = ~1800 bytes safe
	#define MAX_RESPONSE_SIZE 1800
	#define SAFETY_MARGIN 100  // Reserve space for closing tags
	
	// Get current time for age calculation
	uint32_t current_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
	
	// Start JSON array
	int written = snprintf((char *)buf, MAX_RESPONSE_SIZE, "{\"tags\":[");
	if (written < 0 || written >= MAX_RESPONSE_SIZE)
	{
		LOG_ERROR("Failed to initialize tags JSON response");
		*len = 0;
		return HTTP_FAILED;
	}
	*len = (uint16_t)written;
	
	uint8_t first = 1;
	uint16_t truncated = 0;  // Count of tags that didn't fit
	
	// Iterate through all possible handles (up to high water mark)
	// tag_db_get_metadata will return false for deleted tags
	for (uint16_t i = 0; i < TAG_DATABASE_MAX_TAGS; i++)
	{
		tag_metadata_t metadata;
		if (tag_db_get_metadata((tag_handle_t)i, &metadata))
		{
			// Check if we have enough space left (estimate ~200 bytes per tag + safety margin)
			if (*len >= (MAX_RESPONSE_SIZE - SAFETY_MARGIN - 200))
			{
				truncated++;
				continue;  // Skip remaining tags to prevent overflow
			}
			
			// Add comma separator (except before first element)
			if (!first)
			{
				written = snprintf((char *)buf + *len, MAX_RESPONSE_SIZE - *len, ",");
				if (written < 0 || *len + written >= MAX_RESPONSE_SIZE - SAFETY_MARGIN)
				{
					truncated++;
					break;
				}
				*len += (uint16_t)written;
			}
			first = 0;
			
			// Format value based on type
			char value_str[32] = {0};
			switch (metadata.data_type)
			{
				case TAG_TYPE_BOOL:
					snprintf(value_str, sizeof(value_str), "%d", metadata.value.bool_val ? 1 : 0);
					break;
				case TAG_TYPE_UINT8:
					snprintf(value_str, sizeof(value_str), "%u", metadata.value.u8_val);
					break;
				case TAG_TYPE_UINT16:
					snprintf(value_str, sizeof(value_str), "%u", metadata.value.u16_val);
					break;
				case TAG_TYPE_UINT32:
					snprintf(value_str, sizeof(value_str), "%lu", (unsigned long)metadata.value.u32_val);
					break;
				case TAG_TYPE_INT16:
					snprintf(value_str, sizeof(value_str), "%d", metadata.value.i16_val);
					break;
				case TAG_TYPE_INT32:
					snprintf(value_str, sizeof(value_str), "%ld", (long)metadata.value.i32_val);
					break;
				case TAG_TYPE_FLOAT:
					snprintf(value_str, sizeof(value_str), "%.2f", metadata.value.float_val);
					break;
				default:
					snprintf(value_str, sizeof(value_str), "0");
					break;
			}
			
			// Calculate age in seconds (handle tick counter wrap-around)
			uint32_t age_seconds;
			if (current_time_ms >= metadata.timestamp_ms)
			{
				age_seconds = (current_time_ms - metadata.timestamp_ms) / 1000;
			}
			else
			{
				// Handle wrap-around (occurs after ~49 days)
				age_seconds = ((UINT32_MAX - metadata.timestamp_ms) + current_time_ms + 1) / 1000;
			}
			
			// Build JSON object for this tag (bounded write)
			written = snprintf((char *)buf + *len, MAX_RESPONSE_SIZE - *len,
							   "{\"handle\":%u,\"name\":\"%s\",\"type\":%u,\"value\":%s,\"quality\":%u,\"age\":%lu}",
							   i,
							   metadata.name,
							   metadata.data_type,
							   value_str,
							   metadata.quality,
							   (unsigned long)age_seconds);
			
			if (written < 0 || *len + written >= MAX_RESPONSE_SIZE - SAFETY_MARGIN)
			{
				// Tag didn't fit, stop adding more
				truncated++;
				break;
			}
			
			*len += (uint16_t)written;
		}
	}
	
	// Close JSON array (with truncation warning if needed)
	if (truncated > 0)
	{
		written = snprintf((char *)buf + *len, MAX_RESPONSE_SIZE - *len,
						   "],\"truncated\":%u}", truncated);
		LOG_WARN("Tag list truncated: %u tags omitted due to buffer size", truncated);
	}
	else
	{
		written = snprintf((char *)buf + *len, MAX_RESPONSE_SIZE - *len, "]}");
	}
	
	if (written < 0 || *len + written >= MAX_RESPONSE_SIZE)
	{
		LOG_ERROR("Failed to close tags JSON response");
		return HTTP_FAILED;
	}
	
	*len += (uint16_t)written;
	return HTTP_OK;
	
	#undef MAX_RESPONSE_SIZE
	#undef SAFETY_MARGIN
}

/**
 * @brief Handle create_tag.cgi POST request
 * 
 * Creates a new tag and optionally persists it to flash.
 * 
 * Expected parameters:
 * - tag_name: Name of the tag (max 15 chars)
 * - data_type: Data type (0-6)
 * 
 * @param uri URI string with query parameters
 * @param buf Buffer to write JSON response
 * @param len Pointer to store response length
 * @return HTTP_OK on success, HTTP_FAILED on error
 */
static uint8_t handle_create_tag(uint8_t * uri, uint8_t * buf, uint16_t * len)
{
	uint8_t * param;
	char tag_name[TAG_NAME_MAX_LEN] = {0};
	uint8_t data_type = 0;
	
	// Get tag name
	if ((param = get_http_param_value((char *)uri, "tag_name")))
	{
		copy_text(tag_name, sizeof(tag_name), param);
	}
	else
	{
		*len = sprintf((char *)buf, "{\"success\":false,\"error\":\"Missing tag_name\"}");
		return HTTP_FAILED;
	}
	
	// Validate tag name length
	if (strlen(tag_name) == 0 || strlen(tag_name) >= TAG_NAME_MAX_LEN)
	{
		*len = sprintf((char *)buf, "{\"success\":false,\"error\":\"Invalid tag name length\"}");
		return HTTP_FAILED;
	}
	
	// Get data type
	if ((param = get_http_param_value((char *)uri, "data_type")))
	{
		data_type = (uint8_t)atoi((char *)param);
	}
	else
	{
		*len = sprintf((char *)buf, "{\"success\":false,\"error\":\"Missing data_type\"}");
		return HTTP_FAILED;
	}
	
	// Validate data type
	if (data_type > TAG_TYPE_FLOAT)
	{
		*len = sprintf((char *)buf, "{\"success\":false,\"error\":\"Invalid data_type\"}");
		return HTTP_FAILED;
	}
	
	// Create tag with persistence
	tag_handle_t handle = tag_db_create_persistent(tag_name, (tag_data_type_t)data_type, true);
	
	if (handle == TAG_HANDLE_INVALID)
	{
		*len = sprintf((char *)buf, "{\"success\":false,\"error\":\"Failed to create tag\"}");
		return HTTP_FAILED;
	}
	
	LOG_INFO("Tag created via HTTP: %s (handle=%d, type=%d)", tag_name, handle, data_type);
	
	*len = sprintf((char *)buf, "{\"success\":true,\"handle\":%u,\"name\":\"%s\"}", handle, tag_name);
	return HTTP_OK;
}

/**
 * @brief Handle delete_tag.cgi POST request
 * 
 * Deletes a tag from runtime database and flash.
 * 
 * Expected parameters:
 * - tag_name: Name of the tag to delete
 * 
 * @param uri URI string with query parameters
 * @param buf Buffer to write JSON response
 * @param len Pointer to store response length
 * @return HTTP_OK on success, HTTP_FAILED on error
 */
static uint8_t handle_delete_tag(uint8_t * uri, uint8_t * buf, uint16_t * len)
{
	uint8_t * param;
	char tag_name[TAG_NAME_MAX_LEN] = {0};
	
	// Get tag name
	if ((param = get_http_param_value((char *)uri, "tag_name")))
	{
		copy_text(tag_name, sizeof(tag_name), param);
	}
	else
	{
		*len = sprintf((char *)buf, "{\"success\":false,\"error\":\"Missing tag_name\"}");
		return HTTP_FAILED;
	}
	
	// Validate tag name length
	if (strlen(tag_name) == 0)
	{
		*len = sprintf((char *)buf, "{\"success\":false,\"error\":\"Invalid tag name\"}");
		return HTTP_FAILED;
	}
	
	// Delete tag with persistence
	if (!tag_db_delete(tag_name, true))
	{
		*len = sprintf((char *)buf, "{\"success\":false,\"error\":\"Tag not found\"}");
		return HTTP_FAILED;
	}
	
	LOG_INFO("Tag deleted via HTTP: %s", tag_name);
	
	*len = sprintf((char *)buf, "{\"success\":true,\"name\":\"%s\"}", tag_name);
	return HTTP_OK;
}

uint8_t http_get_cgi_handler(uint8_t * uri_name, uint8_t * buf, uint32_t * file_len)
{
	uint8_t ret = HTTP_OK;
	uint16_t len = 0;

	if(predefined_get_cgi_processor(uri_name, buf, &len))
	{
		;
	}
	else if(strcmp((const char *)uri_name, "example.cgi") == 0)
	{
		// To do
		;
	}
	else
	{
		// CGI file not found
		ret = HTTP_FAILED;
	}

	if(ret) *file_len = len;
	return ret;
}

uint8_t http_post_cgi_handler(uint8_t * uri_name, st_http_request * p_http_request, uint8_t * buf, uint32_t * file_len)
{
	uint8_t ret = HTTP_OK;
	uint16_t len = 0;
	uint8_t val = 0;

	// Try predefined processors first
	uint8_t processed = predefined_set_cgi_processor(uri_name, p_http_request->URI, buf, &len);
	
	// Check if URI was handled by a predefined processor
	// Note: We need to distinguish "not handled" from "handled with error"
	// If len > 0, a response was prepared (even if processing failed)
	if(len > 0)
	{
		// Processor handled the request (success or failure with response)
		*file_len = len;
		return processed;
	}
	
	// Not handled by predefined processors, try custom handlers
	if(strcmp((const char *)uri_name, "example.cgi") == 0)
	{
		val = 1;
		len = sprintf((char *)buf, "%d", val);
		ret = HTTP_OK;
	}
	else
	{
		// CGI not found
		ret = HTTP_FAILED;
	}

	if(ret) *file_len = len;
	return ret;
}

uint8_t predefined_get_cgi_processor(uint8_t * uri_name, uint8_t * buf, uint16_t * len)
{
	if(strcmp((const char *)uri_name, "get_tags.cgi") == 0)
	{
		return handle_get_tags(buf, len);
	}
	
	if(strcmp((const char *)uri_name, "get_config.cgi") == 0)
	{
		wiz_NetInfo net_info;
		serial_config_t serial0_config;
		serial_config_t serial1_config;
		serial_to_tcp_mode_config_t s2tcp_config;
		modbus_rtu_client_config_t modbus_config;
		
		// Get all configuration using the API
		if (!config_get_net_info(&net_info) ||
			!config_get_serial_config(0, &serial0_config) ||
			!config_get_serial_config(1, &serial1_config) ||
			!config_get_serial_to_tcp_mode(&s2tcp_config) ||
			!config_get_modbus_rtu_client_config(&modbus_config))
		{
			LOG_ERROR("Failed to get configuration");
			*len = sprintf((char *)buf, "{\"error\":\"Configuration not loaded\"}");
			return HTTP_FAILED;
		}
		
		// Convert parity enum to string for both serial ports
		const char *parity0_str = serial0_config.parity == UART_PARITY_NONE ? "none" :
								  serial0_config.parity == UART_PARITY_EVEN ? "even" : "odd";
		const char *parity1_str = serial1_config.parity == UART_PARITY_NONE ? "none" :
								  serial1_config.parity == UART_PARITY_EVEN ? "even" : "odd";
		
		// Build base JSON
		*len = sprintf((char *)buf,
					   "{\"net\":{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"ip\":\"%d.%d.%d.%d\",\"sn\":\"%d.%d.%d.%d\",\"gw\":\"%d.%d.%d.%d\",\"dns\":\"%d.%d.%d.%d\",\"dhcp\":%d},"
					   "\"serial0\":{\"baud\":%lu,\"databits\":%u,\"parity\":\"%s\",\"stopbits\":%u,\"flowcts\":%d,\"flowrts\":%d},"
					   "\"serial1\":{\"baud\":%lu,\"databits\":%u,\"parity\":\"%s\",\"stopbits\":%u,\"flowcts\":%d,\"flowrts\":%d},"
					   "\"s2tcp\":{\"enable\":%d,\"serial\":%u,\"mode\":%d,\"lport\":%u,\"timeout\":%u,\"keepalive\":%u,\"maxconn\":%u,\"remoteip\":\"%d.%d.%d.%d\",\"remoteport\":%u},"
					   "\"modbus\":{\"enable\":%d,\"serial_id\":%u,\"requests\":[",
					   net_info.mac[0], net_info.mac[1], net_info.mac[2], net_info.mac[3], net_info.mac[4], net_info.mac[5],
					   net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3],
					   net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3],
					   net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3],
					   net_info.dns[0], net_info.dns[1], net_info.dns[2], net_info.dns[3],
					   net_info.dhcp,
					   (unsigned long)serial0_config.baud,
					   (unsigned int)serial0_config.databits,
					   parity0_str,
					   (unsigned int)serial0_config.stopbits,
					   serial0_config.flow_control_cts ? 1 : 0,
					   serial0_config.flow_control_rts ? 1 : 0,
					   (unsigned long)serial1_config.baud,
					   (unsigned int)serial1_config.databits,
					   parity1_str,
					   (unsigned int)serial1_config.stopbits,
					   serial1_config.flow_control_cts ? 1 : 0,
					   serial1_config.flow_control_rts ? 1 : 0,
					   s2tcp_config.enable ? 1 : 0,
					   (unsigned int)s2tcp_config.serial_id,
					   s2tcp_config.mode,
					   (unsigned int)s2tcp_config.local_port,
					   (unsigned int)s2tcp_config.timeout_s,
					   (unsigned int)s2tcp_config.keepalive_s,
					   (unsigned int)s2tcp_config.max_connections,
					   s2tcp_config.remote_ip[0], s2tcp_config.remote_ip[1], 
					   s2tcp_config.remote_ip[2], s2tcp_config.remote_ip[3],
					   (unsigned int)s2tcp_config.remote_port,
					   modbus_config.enable ? 1 : 0,
					   (unsigned int)modbus_config.serial_id);
		
		// Append requests array
		for (int i = 0; i < MODBUS_RTU_REQUESTS_MAX; i++)
		{
			*len += sprintf((char *)buf + *len, "%s{\"enabled\":%d,\"slave_address\":%u,\"data_type\":%d,\"operation\":%d,\"start_address\":%u,\"count\":%u,\"tag_handles\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}",
							i > 0 ? "," : "",
							modbus_config.requests[i].enabled ? 1 : 0,
							(unsigned int)modbus_config.requests[i].slave_address,
							modbus_config.requests[i].data_type,
							modbus_config.requests[i].operation,
							(unsigned int)modbus_config.requests[i].start_address,
							(unsigned int)modbus_config.requests[i].count,
							modbus_config.requests[i].tag_handles[0],
							modbus_config.requests[i].tag_handles[1],
							modbus_config.requests[i].tag_handles[2],
							modbus_config.requests[i].tag_handles[3],
							modbus_config.requests[i].tag_handles[4],
							modbus_config.requests[i].tag_handles[5],
							modbus_config.requests[i].tag_handles[6],
							modbus_config.requests[i].tag_handles[7],
							modbus_config.requests[i].tag_handles[8],
							modbus_config.requests[i].tag_handles[9]);
		}
		
		// Close JSON
		*len += sprintf((char *)buf + *len, "]}}");
		
		return HTTP_OK;
	}

	return HTTP_FAILED;
}

uint8_t predefined_set_cgi_processor(uint8_t * uri_name, uint8_t * uri, uint8_t * buf, uint16_t * en)
{
	if(strcmp((const char *)uri_name, "create_tag.cgi") == 0)
	{
		return handle_create_tag(uri, buf, en);
	}
	
	if(strcmp((const char *)uri_name, "delete_tag.cgi") == 0)
	{
		return handle_delete_tag(uri, buf, en);
	}
	
	if(strcmp((const char *)uri_name, "set_network.cgi") == 0)
	{
		uint8_t handled = handle_set_network(uri);
		*en = sprintf((char *)buf, "%d", handled == HTTP_OK);
		return handled;
	}

	if(strcmp((const char *)uri_name, "set_serial.cgi") == 0)
	{
		uint8_t handled = handle_set_serial(uri);
		*en = sprintf((char *)buf, "%d", handled == HTTP_OK);
		return handled;
	}

	if(strcmp((const char *)uri_name, "set_s2tcp.cgi") == 0)
	{
		uint8_t handled = handle_set_s2tcp(uri);
		*en = sprintf((char *)buf, "%d", handled == HTTP_OK);
		return handled;
	}

	if(strcmp((const char *)uri_name, "set_modbus_client.cgi") == 0)
	{
		uint8_t handled = handle_set_modbus_client(uri);
		*en = sprintf((char *)buf, "%d", handled == HTTP_OK);
		return handled;
	}

	if(strcmp((const char *)uri_name, "set_modbus_datapoint.cgi") == 0)
	{
		uint8_t handled = handle_set_modbus_datapoint(uri);
		*en = sprintf((char *)buf, "%d", handled == HTTP_OK);
		return handled;
	}

	return HTTP_FAILED;
}
