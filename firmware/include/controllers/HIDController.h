#pragma once

#include <Adafruit_TinyUSB.h>
#include <Arduino.h>

#include "HIDReportDescriptor.h"

class HIDController {
 public:
  void begin();
  void task();
  bool sendReports(const float motion[6], uint16_t buttonBits);

  // USB connection control, used by the transport-routing logic so BLE can
  // take over. detach() makes the device "unplug" from the host (power over
  // the cable is unaffected); attach() re-enumerates if a cable is present.
  void detach();
  void attach();
  bool mounted();

 private:
  Adafruit_USBD_HID usbHid_;
  uint16_t buttonBitsSent_ = 0;
  HIDReportAxes lastSentAxes_{};
};
