/* remote_types.h
 *
 * Tipos de datos intercambiados con el barco por ESP-NOW.
 *
 * Convención de ejes:
 *   - Right Y, Right X, Left X : bipolares, -100..+100 (centro = 0)
 *   - Left Y                   : unipolar,    0..100  (sin centro)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((packed)) {
    int8_t  joystick_right_Y;   /* -100..+100 */
    int8_t  joystick_right_X;   /* -100..+100 */
    uint8_t joystick_left_Y;    /*    0..100  */
    int8_t  joystick_left_X;    /* -100..+100 */
} remote_data_t;

/* Si esta comprobación falla, has cambiado la estructura y debes
 * actualizar también el receptor. */
_Static_assert(sizeof(remote_data_t) == 4,
               "remote_data_t debe medir 4 bytes");

typedef struct __attribute__((packed)) {
    float    gps_latitude;
    float    gps_longitude;
    uint8_t  gps_hour;
    uint8_t  gps_minute;
    uint8_t  gps_year;
    uint8_t  gps_month;
    uint8_t  gps_day;
    uint16_t battery;
    uint16_t panel_amp;
    uint16_t panel_volt;
    uint16_t motor1_amp;
    uint16_t motor1_volt;
    uint16_t motor2_amp;
    uint16_t motor2_volt;
} received_data_t;

extern remote_data_t   remote_data;
extern received_data_t received_data;