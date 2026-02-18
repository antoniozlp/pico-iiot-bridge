/**
 * @file http_utils.h
 * @brief HTTP CGI handlers and utilities for web server
 */

#ifndef _HTTP_UTILS_H_
#define _HTTP_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "httpServer.h"
#include "httpUtil.h"

// Disable WIZnet httpServer debug output
#undef _HTTPSERVER_DEBUG_

/**
 * @brief HTTP GET CGI handler
 * 
 * Processes HTTP GET requests for CGI endpoints.
 * Called by WIZnet httpServer library.
 * 
 * @param uri_name Name of the CGI resource
 * @param buf Buffer to write response data
 * @param file_len Pointer to store response length
 * @return HTTP_OK on success, HTTP_FAILED on error
 */
uint8_t http_get_cgi_handler(uint8_t * uri_name, uint8_t * buf, uint32_t * file_len);

/**
 * @brief HTTP POST CGI handler
 * 
 * Processes HTTP POST requests for CGI endpoints.
 * Called by WIZnet httpServer library.
 * 
 * @param uri_name Name of the CGI resource
 * @param p_http_request Pointer to HTTP request structure
 * @param buf Buffer to write response data
 * @param file_len Pointer to store response length
 * @return HTTP_OK on success, HTTP_FAILED on error
 */
uint8_t http_post_cgi_handler(uint8_t * uri_name, st_http_request * p_http_request, uint8_t * buf, uint32_t * file_len);

/**
 * @brief Process predefined GET CGI requests
 * 
 * Handles GET requests for configuration data (get_network.cgi, get_serial.cgi,
 * get_s2tcp.cgi, get_modbus.cgi, get_tags.cgi).
 * 
 * @param uri_name Name of the CGI resource
 * @param buf Buffer to write response data
 * @param len Pointer to store response length
 * @return HTTP_OK on success, HTTP_FAILED on error
 */
uint8_t predefined_get_cgi_processor(uint8_t * uri_name, uint8_t * buf, uint16_t * len);

/**
 * @brief Process predefined SET CGI requests
 * 
 * Handles POST requests to update configuration (set_network.cgi, set_serial.cgi, set_s2tcp.cgi).
 * 
 * @param uri_name Name of the CGI resource
 * @param uri Full URI with query parameters
 * @param buf Buffer to write response data
 * @param len Pointer to store response length
 * @return HTTP_OK on success, HTTP_FAILED on error
 */
uint8_t predefined_set_cgi_processor(uint8_t * uri_name, uint8_t * uri, uint8_t * buf, uint16_t * len);

#ifdef __cplusplus
}
#endif

#endif /* _HTTP_UTILS_H_ */
