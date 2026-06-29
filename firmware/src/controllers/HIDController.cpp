#include "controllers/HIDController.h"

#include <math.h>

#include "Config.h"

void HIDController::begin() {
  if (!Config::ENABLE_USB) {
    return;
  }
  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }
  usbHid_.setReportDescriptor(kSpaceMouseHidDescriptor,
                              sizeof(kSpaceMouseHidDescriptor));
  usbHid_.setPollInterval(1);
  usbHid_.begin();
}

void HIDController::task() { TinyUSBDevice.task(); }

void HIDController::detach() {
  if (Config::ENABLE_USB) {
    TinyUSBDevice.detach();
  }
}

void HIDController::attach() {
  if (Config::ENABLE_USB) {
    TinyUSBDevice.attach();
  }
}

bool HIDController::mounted() {
  return Config::ENABLE_USB && TinyUSBDevice.mounted();
}

bool HIDController::sendReports(const float motion[6], uint16_t buttonBits) {
  if (!Config::ENABLE_USB) {
    return false;
  }

  const HIDReportAxes axes = hidMakeAxesReport(motion);
  const bool sendAxes = hidAxesChanged(axes, lastSentAxes_);
  const bool sendButtons = (buttonBits != buttonBitsSent_);

  if (!usbHid_.ready() || (!sendAxes && !sendButtons)) {
    return false;
  }

  if (sendAxes) {
    usbHid_.sendReport(HID_REPORT_ID_AXES, &axes, sizeof(axes));
    lastSentAxes_ = axes;
  }

  if (sendButtons) {
    HIDReportButtons btn{};
    btn.bits = buttonBits & 0x0003;
    usbHid_.sendReport(HID_REPORT_ID_BUTTONS, &btn, sizeof(btn));
    buttonBitsSent_ = buttonBits;
  }

  return true;
}
