#ifndef USBHOST_H
#define USBHOST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usb/hid_host.h"

void usb_host_task(void *arg);
void usb_hid_init(void);
void usb_hid_deinit(void);

void hid_host_device_event(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event, void *arg);
void hid_host_interface_event(hid_host_device_handle_t hid_device_handle, const hid_host_interface_event_t event, void *arg);

bool sm_set_power(bool newState);
bool sm_get_values();

/**
 * @brief Shackmaster Analog Values
 * 
 * packed attribute prevents compiler from aligning elements on word boundaries
 * and allows the data stream from the device to be mapped into the structure
 */
typedef struct __attribute__((packed)) { 
    uint8_t voltage;        // 12V channel voltage [V x 10]
    int16_t current;        // 12V channel current [A x 10]
    uint8_t usb1_v;         // USB1 Voltage [V x 10]
    int16_t usb1_a;         // USB1 Current [A x 100]
    int8_t temp_in;         // Temperature inside [°C]
    int8_t temp_out;        // Temperature outside [°C]
    int16_t supply_V;       // Mains supply voltage [V RMS]
    int16_t supply_F;       // Mains supply frequency [Hz x 10]
    uint8_t fan_duty;       // Fan duty [%]
    int16_t power_tot;      // Total power [W]
    int16_t current_tot;    // Total current [A x 10]
    int16_t power_max;      // Maximum permitted power [W]
    int16_t current_max;    // Maximum permitted current [A x 10]
    uint32_t time;          // current date & time [unix timestamp format]
    uint16_t err1;          // Error 1 bitmap
    uint16_t err2;          // Error 2 bitmap
    uint32_t serial;        // Device serial number (exluding leading 5003, to be printed 5 digits with leading zeros)
    uint8_t acc_pos;        // accelerometer position 0=normal, 3=upside down, 2+4 = sideways
    uint8_t ver_major;      // Version: major
    uint8_t ver_minor;      // Version: minor
    uint8_t ver_build;      // Version: build
    uint8_t usb2_v;         // USB2 Voltage [V x 10]
    int16_t usb2_a;         // USB2 Current [A x 100]
    uint8_t usb3_v;         // USB3 Voltage [V x 10]
    int16_t usb3_a;         // USB3 Current [A x 100]
    uint8_t usb4_v;         // USB4 Voltage [V x 10]
    int16_t usb4_a;         // USB4 Current [A x 100]
} sm_values_t;

/**
 * @brief APP event group
 *
 * Application logic can be different. There is a one among other ways to distinguish the
 * event by application event group.
 * In this example we have two event groups:
 * APP_EVENT            - General event, which is APP_QUIT_PIN press event (Generally, it is IO0).
 * APP_EVENT_HID_HOST   - HID Host Driver event, such as device connection/disconnection or input report.
 */
typedef enum {
    APP_EVENT = 0,
    APP_EVENT_HID_HOST,
    APP_EVENT_HID_INTERFACE
} app_event_group_t;

/**
 * @brief APP event queue
 *
 * This event is used for delivering the HID Host event from callback to a task.
 */
typedef struct {
    app_event_group_t event_group;
    /* HID Host - Device related info */
    struct {
        hid_host_device_handle_t handle;
        hid_host_driver_event_t event;
        void *arg;
    } hid_host_device;
} app_event_queue_t;

#ifdef __cplusplus
}
#endif

#endif
