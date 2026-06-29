#pragma once

#include <Arduino.h>
#include <math.h>

// Shared HID definitions used by BOTH the USB (TinyUSB) and the BLE (NimBLE)
// transports. Keeping the report map and the report structs in one place means
// the two transports can never drift apart and present a different device to
// the host. This is the 3Dconnexion SpaceMouse Compact multi-axis report map.

// Report IDs (must match the descriptor below).
static const uint8_t HID_REPORT_ID_AXES = 0x01;
static const uint8_t HID_REPORT_ID_BUTTONS = 0x03;

// Logical axis range encoded in the descriptor (-350 .. 350).
static const int16_t HID_AXIS_LOGICAL_MIN = -350;
static const int16_t HID_AXIS_LOGICAL_MAX = 350;

// The report map. `static` here gives each translation unit its own (tiny)
// copy, which sidesteps any one-definition-rule issues from including this in
// multiple .cpp files. PROGMEM is a no-op on the ESP32 but kept for clarity.
static const uint8_t kSpaceMouseHidDescriptor[] PROGMEM = {
    0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
    0x09, 0x08,        // USAGE (Multi-axis Controller)
    0xA1, 0x01,        // COLLECTION (Application)
    0xA1, 0x00,        // COLLECTION (Physical)
    0x85, 0x01,        // REPORT_ID (1)
    0x16, 0xA2, 0xFE,  // LOGICAL_MINIMUM  (-350)
    0x26, 0x5E, 0x01,  // LOGICAL_MAXIMUM  (350)
    0x09, 0x30,        // USAGE (X)
    0x09, 0x31,        // USAGE (Y)
    0x09, 0x32,        // USAGE (Z)
    0x09, 0x33,        // USAGE (Rx)
    0x09, 0x34,        // USAGE (Ry)
    0x09, 0x35,        // USAGE (Rz)
    0x75, 0x10,        // REPORT_SIZE (16)
    0x95, 0x06,        // REPORT_COUNT (6)
    0x81, 0x02,        // INPUT (Data,Var,Abs)
    0xC0,              // END_COLLECTION
    0xA1, 0x00,        // COLLECTION (Physical)
    0x85, 0x03,        // REPORT_ID (3)
    0x05, 0x09,        // USAGE_PAGE (Button)
    0x19, 0x01,        // USAGE_MINIMUM (Button 1)
    0x29, 0x02,        // USAGE_MAXIMUM (Button 2)
    0x15, 0x00,        // LOGICAL_MINIMUM (0)
    0x25, 0x01,        // LOGICAL_MAXIMUM (1)
    0x75, 0x01,        // REPORT_SIZE (1)
    0x95, 0x02,        // REPORT_COUNT (2)
    0x81, 0x02,        // INPUT (Data,Var,Abs)
    0x95, 0x0E,        // REPORT_COUNT (14) padding
    0x81, 0x01,        // INPUT (Const,Array,Abs)
    0xC0,              // END_COLLECTION
    0xC0               // END_COLLECTION
};

// Wire-format of report ID 1: six signed 16-bit axes.
struct __attribute__((packed)) HIDReportAxes {
  int16_t x, y, z, rx, ry, rz;
};

// Wire-format of report ID 3: two buttons in the low bits, rest padding.
struct __attribute__((packed)) HIDReportButtons {
  uint16_t bits;
};

// Build an axes report from the motion pipeline output. Shared so the USB and
// BLE paths quantise identically.
inline HIDReportAxes hidMakeAxesReport(const float motion[6]) {
  HIDReportAxes axes{};
  axes.x = static_cast<int16_t>(motion[0]);
  axes.y = static_cast<int16_t>(motion[1]);
  axes.z = static_cast<int16_t>(motion[2]);
  axes.rx = static_cast<int16_t>(motion[3]);
  axes.ry = static_cast<int16_t>(motion[4]);
  axes.rz = static_cast<int16_t>(motion[5]);
  return axes;
}

inline bool hidAxesChanged(const HIDReportAxes& a, const HIDReportAxes& b) {
  return a.x != b.x || a.y != b.y || a.z != b.z ||
         a.rx != b.rx || a.ry != b.ry || a.rz != b.rz;
}
