/*
 * For details see README.md
 */
//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
#include "esp_log.h"
#include "time.h"

#include "eth.h"
#include "tcpserver.h"
#include "httpserver.h"
#include "usbhost.h"

static const char *TAG = "main";

QueueHandle_t app_event_queue = NULL;

void app_main(void)
{ 
    app_event_queue_t evt_queue;
    // Set to local timezone (e.g., AEST - Australian Eastern Standard Time)
    setenv("TZ", "AEST-10AEDT,M10.1.0,M4.1.0", 1);
    tzset();

    // Start Ethernet driver
    eth_start();
    // Start USB HID host
    usb_hid_init();

    while (1) {
        // Wait queue
        if (xQueueReceive(app_event_queue, &evt_queue, portMAX_DELAY)) {
            if (APP_EVENT == evt_queue.event_group) {
                ESP_LOGW(TAG, "APP_EVENT not implemented");
            }

            if (APP_EVENT_HID_HOST ==  evt_queue.event_group) {
                hid_host_device_event(evt_queue.hid_host_device.handle,
                                      evt_queue.hid_host_device.event,
                                      evt_queue.hid_host_device.arg);
            }

            if (APP_EVENT_HID_INTERFACE == evt_queue.event_group) {
                hid_host_interface_event(evt_queue.hid_host_device.handle,
                                      evt_queue.hid_host_device.event,
                                      evt_queue.hid_host_device.arg);
            }
        }
    }   // while

    usb_hid_deinit();
#if CONFIG_ETH_DEINIT_CODE
    eth_stop();
#endif
    xQueueReset(app_event_queue);
    vQueueDelete(app_event_queue);

}
