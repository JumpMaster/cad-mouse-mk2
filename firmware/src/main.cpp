#include <Arduino.h>

#include "Config.h"
#include "Controllers.h"
#include "StateMachine.h"

InputController inputController;
LEDController ledController;
SensorController sensorController;
MotionController motionController;
HIDController hidController;
BLEHIDController bleHidController;
TelemetryController telemetryController;

void setup() {
  if (Config::ENABLE_USB) {
    TinyUSBDevice.setID(Config::HID_VID, Config::HID_PID);
    TinyUSBDevice.setManufacturerDescriptor("3Dconnexion");
    TinyUSBDevice.setProductDescriptor("SpaceMouse Compact");
  }

  // Initialize USB HID first, then BLE HID.
  hidController.begin();
  bleHidController.begin();

  if (Config::ENABLE_TELEMETRY) {
    Serial.begin(115200);
    delay(200);
  }

  inputController.begin();
  ledController.begin();
  sensorController.begin();
  motionController.reset();
  telemetryController.begin();

  stateMachine.changeState(&StateMachine::calibratingState);
}

void loop() {
  // hidController.task(); // Not required on an ESP32
  stateMachine.update();
}
