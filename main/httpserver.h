#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#ifdef __cplusplus
extern "C" {
#endif
#include "esp_http_server.h"

httpd_handle_t start_webserver(void);

#ifdef __cplusplus
}
#endif

#endif
