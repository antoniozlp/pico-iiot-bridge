/**
    @file	httpUtil.c
    @brief	HTTP Server Utilities for HTTP example (local overrides)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "httpUtil.h"
#include "http_utils.h"
#include "system_config.h"

static void copy_text(char *dst, size_t dst_len, const uint8_t *src)
{
	if(!dst || !src || dst_len == 0) return;

	strncpy(dst, (const char *)src, dst_len - 1);
	dst[dst_len - 1] = 0;
}

static void parse_ip_addr(uint8_t *ip, const char *str)
{
    if (!ip || !str) return;
    sscanf(str, "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);
}

static void parse_mac_addr(uint8_t *mac, const char *str)
{
    if (!mac || !str) return;
    sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

static uint8_t handle_set_network(uint8_t * uri)
{
    uint8_t changed = 0;
    uint8_t * param;
    wiz_NetInfo net_info;
    
    // Get current network configuration
    if (!config_get_net_info(&net_info))
    {
        printf("[HTTP] ERROR: Failed to get network config\r\n");
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
            printf("[HTTP] ERROR: Failed to set network config\r\n");
            return HTTP_FAILED;
        }
        
        printf("[HTTP] Network config updated\r\n");
        if (!config_save_to_flash())
        {
            printf("[HTTP] ERROR: Failed to save config to flash\r\n");
            return HTTP_FAILED;
        }
    }

    return changed ? HTTP_OK : HTTP_FAILED;
}

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
		printf("[HTTP] ERROR: Failed to get serial config\r\n");
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
			printf("[HTTP] ERROR: Invalid baud rate %lu\r\n", baud_val);
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
			printf("[HTTP] ERROR: Invalid databits %d\r\n", databits_val);
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
			printf("[HTTP] ERROR: Invalid parity '%s'\r\n", (char *)param);
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
			printf("[HTTP] ERROR: Invalid stopbits %d\r\n", stopbits_val);
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
			printf("[HTTP] ERROR: Failed to set serial config\r\n");
			return HTTP_FAILED;
		}
		
		const char *parity_str = serial_config.parity == UART_PARITY_NONE ? "none" :
								 serial_config.parity == UART_PARITY_EVEN ? "even" : "odd";
		
		printf("[HTTP] Serial%d config updated: baud=%lu, databits=%u, parity=%s, stopbits=%u, flowcts=%d, flowrts=%d\r\n",
			   uart_id,
			   (unsigned long)serial_config.baud,
			   (unsigned int)serial_config.databits,
			   parity_str,
			   (unsigned int)serial_config.stopbits,
			   serial_config.flow_control_cts,
			   serial_config.flow_control_rts);
        
		if (!config_save_to_flash())
		{
			printf("[HTTP] ERROR: Failed to save config to flash\r\n");
			return HTTP_FAILED;
		}
	}

	return changed ? HTTP_OK : HTTP_FAILED;
}

static uint8_t handle_set_s2tcp(uint8_t * uri)
{
	uint8_t changed = 0;
	uint8_t * param;
	serial_to_tcp_mode_config_t mode_config;
	
	// Get current Serial-to-TCP configuration
	if (!config_get_serial_to_tcp_mode(&mode_config))
	{
		printf("[HTTP] ERROR: Failed to get Serial-to-TCP config\r\n");
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
			printf("[HTTP] ERROR: Invalid port %d (must be 1024-65535)\r\n", port_val);
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
			printf("[HTTP] ERROR: Invalid timeout %d (must be 1-3600)\r\n", timeout_val);
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
			printf("[HTTP] ERROR: Invalid keepalive %d (must be 1-600)\r\n", keepalive_val);
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
			printf("[HTTP] ERROR: Invalid max connections %d (must be 1-4)\r\n", maxconn_val);
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
			printf("[HTTP] ERROR: Failed to set Serial-to-TCP config\r\n");
			return HTTP_FAILED;
		}
		
		printf("[HTTP] Serial-to-TCP config updated\r\n");

		if (!config_save_to_flash())
		{
			printf("[HTTP] ERROR: Failed to save config to flash\r\n");
			return HTTP_FAILED;
		}
	}

	return changed ? HTTP_OK : HTTP_FAILED;
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

	if(predefined_set_cgi_processor(uri_name, p_http_request->URI, buf, &len))
	{
		;
	}
	else if(strcmp((const char *)uri_name, "example.cgi") == 0)
	{
		val = 1;
		len = sprintf((char *)buf, "%d", val);
	}
	else
	{
		ret = HTTP_FAILED;
	}

	if(ret) *file_len = len;
	return ret;
}

uint8_t predefined_get_cgi_processor(uint8_t * uri_name, uint8_t * buf, uint16_t * len)
{
	if(strcmp((const char *)uri_name, "get_config.cgi") == 0)
	{
		wiz_NetInfo net_info;
		serial_config_t serial0_config;
		serial_config_t serial1_config;
		serial_to_tcp_mode_config_t s2tcp_config;
		
		// Get all configuration using the API
		if (!config_get_net_info(&net_info) ||
			!config_get_serial_config(0, &serial0_config) ||
			!config_get_serial_config(1, &serial1_config) ||
			!config_get_serial_to_tcp_mode(&s2tcp_config))
		{
			printf("[HTTP] ERROR: Failed to get configuration\r\n");
			*len = sprintf((char *)buf, "{\"error\":\"Configuration not loaded\"}");
			return HTTP_FAILED;
		}
		
		// Convert parity enum to string for both serial ports
		const char *parity0_str = serial0_config.parity == UART_PARITY_NONE ? "none" :
								  serial0_config.parity == UART_PARITY_EVEN ? "even" : "odd";
		const char *parity1_str = serial1_config.parity == UART_PARITY_NONE ? "none" :
								  serial1_config.parity == UART_PARITY_EVEN ? "even" : "odd";
		
		*len = sprintf((char *)buf,
					   "{\"net\":{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"ip\":\"%d.%d.%d.%d\",\"sn\":\"%d.%d.%d.%d\",\"gw\":\"%d.%d.%d.%d\",\"dns\":\"%d.%d.%d.%d\",\"dhcp\":%d},"
					   "\"serial0\":{\"baud\":%lu,\"databits\":%u,\"parity\":\"%s\",\"stopbits\":%u,\"flowcts\":%d,\"flowrts\":%d},"
					   "\"serial1\":{\"baud\":%lu,\"databits\":%u,\"parity\":\"%s\",\"stopbits\":%u,\"flowcts\":%d,\"flowrts\":%d},"
					   "\"s2tcp\":{\"enable\":%d,\"serial\":%u,\"mode\":%d,\"lport\":%u,\"timeout\":%u,\"keepalive\":%u,\"maxconn\":%u,\"remoteip\":\"%d.%d.%d.%d\",\"remoteport\":%u}}",
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
					   (unsigned int)s2tcp_config.remote_port);
		return HTTP_OK;
	}

	return HTTP_FAILED;
}

uint8_t predefined_set_cgi_processor(uint8_t * uri_name, uint8_t * uri, uint8_t * buf, uint16_t * en)
{
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

	return HTTP_FAILED;
}
