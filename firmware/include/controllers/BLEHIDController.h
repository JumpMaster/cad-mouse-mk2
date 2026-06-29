#pragma once

#include <Arduino.h>

#include "HIDReportDescriptor.h"

// Forward declarations keep the NimBLE headers out of this file so it stays
// cheap to include everywhere.
class NimBLEServer;
class NimBLEHIDDevice;
class NimBLECharacteristic;

// Presents the device as a Bluetooth LE HID peripheral (HID-over-GATT / HOGP),
// mirroring the USB HIDController. It reuses the exact same SpaceMouse report
// map, so a paired host sees an identical device whether it is wired or
// wireless.
class BLEHIDController {
 public:
  void begin();
  // Mirrors HIDController::sendReports. Returns true if anything was notified.
  bool sendReports(const float motion[6], uint16_t buttonBits);

  bool connected() const { return connected_; }
  // Update the advertised/GATT battery level (0-100). Wire this to a real
  // fuel gauge if/when one is added; harmless to leave at a fixed value.
  void setBatteryLevel(uint8_t percent);

 private:
  class ServerCallbacks;  // defined in the .cpp

  NimBLEServer* server_ = nullptr;
  NimBLEHIDDevice* hid_ = nullptr;
  NimBLECharacteristic* inputAxes_ = nullptr;     // report ID 1
  NimBLECharacteristic* inputButtons_ = nullptr;  // report ID 3
  ServerCallbacks* callbacks_ = nullptr;

  volatile bool connected_ = false;
  // Forces a fresh axes+button report on the next send after a (re)connection,
  // so a newly connected host immediately gets the current state.
  volatile bool forceSend_ = false;

  HIDReportAxes lastSentAxes_{};
  uint16_t buttonBitsSent_ = 0;
};
