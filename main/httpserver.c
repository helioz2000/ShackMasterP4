/*
 * Webserver
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/ip_addr.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_server.h"

// Note: If you are using a specific hardware board like the Waveshare ESP32-P4-ETH, 
// make sure to initialize your specific Ethernet PHY pins (like RTL8211 or LAN8720) 
// using the `esp_eth` driver before starting the network interface.
#include "esp_eth.h" 

#include "httpserver.h"
#include "usbhost.h"

static const char *TAG = "httpserver";
#define HTTP_PORT 80

// Example variable in your C code
int my_counter_variable = 42;

extern sm_values_t sm_values;
extern bool sm_is_connected;

// Global variable to store the number sent by the user
static int received_user_number = 0;
static char received_user_string[64] = {0};

// reference to html files embedded as a binary
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t network_html_start[] asm("_binary_network_html_start");
extern const uint8_t network_html_end[]   asm("_binary_binary_html_end");

// URL Decode Helper function
// parse hex pairs back into standard characters (e.g. %20 -> space)
void url_decode(const char *src, char *dst) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit((unsigned char)a) && isxdigit((unsigned char)b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= 'A' - 10;
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= 'A' - 10;
            else b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Variable to store the validated target IPv4 binary configuration layout
static ip4_addr_t storage_ip_address;

// HTTP request Handler functions

// root - send index.html
static esp_err_t root_get_handler(httpd_req_t *req)
{
    // Set HTTP status code and content type metadata
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_status(req, "200 OK");

    // disable page caching
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
    
    // Stream the raw response buffer back down the TCP connection socket
    //httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    const size_t index_html_size = (index_html_end - index_html_start);
    return httpd_resp_send(req, (const char *)index_html_start, index_html_size);
    
    //ESP_LOGI(TAG, "Webpage request handled successfully");
    return ESP_OK;

    // Failure
    return ESP_FAIL;
}

// Handler to silence favicon.ico requests with an empty 204 response
static esp_err_t favicon_get_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0); // Send no body data
    return ESP_OK;
}

// Handler for ON button (GET /on)
static esp_err_t on_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "ON button activated");
    sm_set_power(true);
    // Redirect or re-serve the root page to keep the buttons visible
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "ON_OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler for OFF button (GET /off)
static esp_err_t off_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Off button activated");
    sm_set_power(false);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "OFF_OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler to send the variable value (GET /api/status) */
static esp_err_t status_get_handler(httpd_req_t *req) {
    char buf1[128];
    char buf2[256];
    char response_buffer[512];
    
    char sm_serial[12] =  {0};
    char sm_version[15] =  {0};
    char sm_connected[10];

    if (sm_is_connected) {
        snprintf(sm_version, sizeof(sm_version), "%d.%d.%d", sm_values.ver_major, sm_values.ver_minor, sm_values.ver_build);
        snprintf(sm_serial, sizeof(sm_serial), "5003%05ld", sm_values.serial);
        snprintf(sm_connected, sizeof(sm_connected), "true");
    }
    else snprintf(sm_connected, sizeof(sm_connected), "false");

    // Deliver data in a JSON string
    snprintf(buf1,sizeof(buf1), "\"%d.%dV\", \"%d.%dA\", \"%dW\", \"%d°C\", \"%d%%\", \"%dV %d.%dHz\"",
        sm_values.voltage / 10, sm_values.voltage % 10, sm_values.current / 10, sm_values.current % 10,
        sm_values.power_tot, sm_values.temp_in, sm_values.fan_duty, sm_values.supply_V, sm_values.supply_F / 10, sm_values.supply_F % 10);

    snprintf(buf2, sizeof(buf2), ", \"%d.%dV\", \"%d.%dA\", \"%d.%dV\", \"%d.%dA\", \"%d.%dV\", \"%d.%dA\", \"%d.%dV\", \"%d.%dA\", \"%s\", \"%s\"",
        sm_values.usb1_v / 10, sm_values.usb1_v % 10, sm_values.usb1_a / 100, sm_values.usb1_a % 100,
        sm_values.usb2_v / 10, sm_values.usb2_v % 10, sm_values.usb2_a / 100, sm_values.usb2_a % 100,
        sm_values.usb3_v / 10, sm_values.usb3_v % 10, sm_values.usb3_a / 100, sm_values.usb3_a % 100,
        sm_values.usb4_v / 10, sm_values.usb4_v % 10, sm_values.usb4_a / 100, sm_values.usb4_a % 100,
        sm_serial, sm_version);

    snprintf(response_buffer, sizeof(response_buffer), "{\n\"connected\": %s, \n\"metrics\": [ %s%s ]\n}", sm_connected, buf1, buf2);
    
    //ESP_LOGI(TAG,"Status: <%s>", response_buffer);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler for /api/set?value=X
static esp_err_t api_set_handler(httpd_req_t *req) {
    char query_buf[128];
    char num_param[32];
    char raw_str_param[64];

    // Read the query string from the URL
    if (httpd_req_get_url_query_str(req, query_buf, sizeof(query_buf)) == ESP_OK) {
        ESP_LOGI(TAG, "Full query received: %s", query_buf);
        
        // 1. Extract the number parameter
        if (httpd_query_key_value(query_buf, "value", num_param, sizeof(num_param)) == ESP_OK) {
            received_user_number = atoi(num_param);
            ESP_LOGI(TAG, "Extracted Number: %d", received_user_number);
        }
        
        // 2. Extract the string parameter
        if (httpd_query_key_value(query_buf, "msg", raw_str_param, sizeof(raw_str_param)) == ESP_OK) {
            // URL Decode the string back into regular readable text (resolves %20, etc.)
            url_decode( raw_str_param, received_user_string );
            ESP_LOGI(TAG, "Extracted String: \"%s\"", received_user_string);
        }
    }

    // Build confirmation response back to browser 
    char resp_str[128];
    snprintf(resp_str, sizeof(resp_str), "Saved Int: %d, Str: %s", received_user_number, received_user_string);
    
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

// Handler for /api/set_ip?ip=192.168.1.50
static esp_err_t api_set_ip_handler(httpd_req_t *req) {
    char query_buf[64];
    char raw_ip_param[32];

    if (httpd_req_get_url_query_str(req, query_buf, sizeof(query_buf)) == ESP_OK) {
        
        if (httpd_query_key_value(query_buf, "ip", raw_ip_param, sizeof(raw_ip_param)) == ESP_OK) {
            char decoded_ip[32];
            url_decode(raw_ip_param, decoded_ip); // Standard cleaner decoding pass
            
            // BACKEND VALIDATION: Use native LwIP conversion tool
            // ip4addr_aton returns 1 on success, 0 on malformed parsing failure
            if (ip4addr_aton(decoded_ip, &storage_ip_address) == 0) {
                ESP_LOGW(TAG, "Rejected invalid IP format: %s", decoded_ip);
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Malformed IPv4 address structure");
                return ESP_FAIL;
            }

            // Print verification results back via traditional formatting templates
            ESP_LOGI(TAG, "Successfully saved valid IP: " IPSTR, IP2STR(&storage_ip_address));
        }
    }

    // Acknowledge back to user layout console screen
    char resp_str[64];
    snprintf(resp_str, sizeof(resp_str), "IP Address updated successfully");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

// Handler for /api/get_ip (Sends current Ethernet interface IP to browser)
static esp_err_t api_get_ip_handler(httpd_req_t *req) {
    esp_netif_ip_info_t ip_info;
    char ip_str[16] = "0.0.0.0";

    // Grab the actual handle for your Waveshare board's Ethernet interface instance
    // Note: If your netif loop uses a different key name, substitute it here (e.g., "ETH_DEF")
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("ETH_DEF");
    
    if (netif != NULL) {
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            // Convert native binary IP address into standard string format
            esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
            ESP_LOGI(TAG, "System IP: %s", ip_str);
        }
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, ip_str, strlen(ip_str));
    return ESP_OK;
}

// URI definitions
static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t favicon_uri = {
    .uri       = "/favicon.ico",
    .method    = HTTP_GET,
    .handler   = favicon_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t on_uri = {
    .uri       = "/api/on",
    .method    = HTTP_GET,
    .handler   = on_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t off_uri = {
    .uri       = "/api/off",
    .method    = HTTP_GET,
    .handler   = off_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t status_uri = {
    .uri       = "/api/status",
    .method    = HTTP_GET,
    .handler   = status_get_handler,
    .user_ctx  = NULL
};

const httpd_uri_t set_uri = {
    .uri       = "/api/set",
    .method    = HTTP_GET,
    .handler   = api_set_handler,
    .user_ctx  = NULL
};

const httpd_uri_t set_ip_uri = {
    .uri       = "/api/set_ip",
    .method    = HTTP_GET,
    .handler   = api_set_ip_handler,
    .user_ctx  = NULL
};

const httpd_uri_t get_ip_uri = {
    .uri       = "/api/get_ip",
    .method    = HTTP_GET,
    .handler   = api_get_ip_handler,
    .user_ctx  = NULL
};

// Server Startup
httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    // Set the TCP listening port (Standard is port 80)
    config.server_port = HTTP_PORT;

    // Start the httpd server daemon loop
    ESP_LOGI(TAG, "Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Register the active route endpoints (Maximum of 8)
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &on_uri);
        httpd_register_uri_handler(server, &off_uri);
        httpd_register_uri_handler(server, &favicon_uri);
        httpd_register_uri_handler(server, &status_uri);
        //httpd_register_uri_handler(server, &set_uri);
        //httpd_register_uri_handler(server, &set_ip_uri);
        // httpd_register_uri_handler(server, &get_ip_uri);   
        return server;
    }

    ESP_LOGE(TAG, "Error starting server!");
    return NULL;
}

// Event handler loop tracking network statuses
void network_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "ESP32-P4 Network Online! IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        
        // Trigger server boot once IP allocation is complete
        start_webserver();
    }
}

void httpd_init(void) 
{
    // Initialize Non-Volatile Flash memory storage allocation
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}
