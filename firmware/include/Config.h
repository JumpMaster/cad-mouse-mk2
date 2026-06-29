#pragma once

#include <Arduino.h>

namespace Config {

const bool ENABLE_TELEMETRY = true;

// ---- Transports -----------------------------------------------------------
// The device can present over USB HID, BLE HID, or both at once. With both
// enabled it sends reports to whichever transport is currently connected.
// (If you only ever want one, disable the other to save flash/RAM.)
const bool ENABLE_USB = true;
const bool ENABLE_BLE = true;

// Shared HID identity, used by both the USB descriptor and the BLE PnP record.
const uint16_t HID_VID = 0x256F;      // 3Dconnexion
const uint16_t HID_PID = 0xC635;      // SpaceMouse Compact
const uint16_t HID_VERSION = 0x0111;  // bcdDevice

// ---- Bluetooth LE ---------------------------------------------------------
const char BLE_DEVICE_NAME[] = "SpaceMouse Compact";
// Advertised/GATT battery level (0-100). Fixed for now; wire to a fuel gauge
// later via bleHidController.setBatteryLevel().
const uint8_t BLE_BATTERY_LEVEL = 100;
// TX power in dBm (ESP32-S3 supports roughly -27..+9).
const int8_t BLE_TX_POWER_DBM = 9;

// Requested connection interval, in 1.25 ms units (6 = 7.5 ms). A low, tight
// interval gives a higher and steadier report rate -> smoother motion, at a
// small power cost. The central may clamp or override these; the firmware
// paces its notifications to whatever interval is actually negotiated.
const uint16_t BLE_CONN_INTERVAL_MIN = 6;            // 7.5 ms
const uint16_t BLE_CONN_INTERVAL_MAX = 9;            // 11.25 ms
const uint16_t BLE_CONN_SUPERVISION_TIMEOUT = 400;   // 4 s, in 10 ms units

/* Hardware pins (XIAO RP2040)
const int PIN_RIGHT_BTN = D0;
const int PIN_LEFT_BTN = D2;
const int PIN_LED_DATA = D3;
const int PIN_LED_LS = D1;
const int PIN_MAG1_LS = D10;
const int PIN_MAG2_LS = D9;
const int PIN_MAG3_LS = D8;
*/

// Hardware pins (Adafruit QT Py ESP32-S3)
const int PIN_RIGHT_BTN = A0;   // 18;   // D0;
const int PIN_LEFT_BTN =  A2;   // 9;    // D2;
const int PIN_LED_DATA =  A3;   // 8;    // D3;
const int PIN_LED_LS =    A1;   // 17;   // D1;
const int PIN_MAG1_LS =   MOSI; // 35;   // D10;
const int PIN_MAG2_LS =   MISO; // 37;   // D9;
const int PIN_MAG3_LS =   SCK;  // 36;   // D8;

// Samples for calibration offset
const int ZERO_SAMPLES = 200;

// Gains and sign fixes
const float GAIN_T[3] = {28.0, 28.0, 24.0};
const float GAIN_R[3] = {18.0, 18.0, 20.0};
const int SIGN_AXIS[6] = {-1, +1, -1, +1, +1, +1};

// Dead zones
const float DEAD_T = 16.0;
const float DEAD_R = 20.0;

// Smoothing
const float SMOOTH_TAU_S = 0.08;

// Final axis output range
const float AXIS_LIMIT = 350.0;

// RGB LEDs
const int LED_COUNT = 8;
const int LED_BRIGHTNESS = 40;
const unsigned long LED_IDLE_COLOR = 0x00FF00;
const unsigned long LED_CALIBRATING_COLOR = 0x0000FF;

// Transport status colours.
const unsigned long LED_USB_COLOR = 0x00FF00;  // solid green: sending over USB
const unsigned long LED_BLE_COLOR = 0x0000FF;  // solid blue: sending over BLE
// While on battery and advertising (BLE not yet connected) the ring blinks
// blue at this half-period to show it is waiting for a connection.
const unsigned long LED_BLE_WAIT_BLINK_MS = 600;

// FSM timing
const long IDLE_SLEEP_TIMEOUT_MS = 2 * 60 * 1000;

}  // namespace Config
