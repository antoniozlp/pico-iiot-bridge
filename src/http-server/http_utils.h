/**
 * @file http_server.h
 * @brief HTTP Server implementation for the application
 */

#ifndef _HTTP_UTILS_H_
#define _HTTP_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "httpServer.h"
#include "httpUtil.h"

/* 
 * CGI Handler Functions implemented in http_server.c 
 * These override/implement the callbacks required by the WIZnet ioLibrary
 */

uint8_t http_get_cgi_handler(uint8_t * uri_name, uint8_t * buf, uint32_t * file_len);
uint8_t http_post_cgi_handler(uint8_t * uri_name, st_http_request * p_http_request, uint8_t * buf, uint32_t * file_len);

uint8_t predefined_get_cgi_processor(uint8_t * uri_name, uint8_t * buf, uint16_t * len);
uint8_t predefined_set_cgi_processor(uint8_t * uri_name, uint8_t * uri, uint8_t * buf, uint16_t * len);

#ifdef __cplusplus
}
#endif

#endif /* _HTTP_UTILS_H_ */
