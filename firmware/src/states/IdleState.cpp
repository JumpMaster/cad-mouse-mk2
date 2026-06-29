#include "states/IdleState.h"

#include <Arduino.h>

#include "Config.h"
#include "Controllers.h"
#include "StateMachine.h"

void IdleState::enter() {
  lastUpdateMs_ = 0;
  lastActivityMs_ = millis();
  // Force updateTransport() to (re)assert the correct colour on the first loop.
  ledShown_ = TransportLed::None;
  bleWasConnected_ = bleHidController.connected();
}

bool IdleState::handleCalibrationRequest() {
  if (inputController.takeCalibrationRequest()) {
    stateMachine.changeState(&StateMachine::calibratingState);
    return true;
  }
  return false;
}

// Picks a single active transport (BLE wins over USB) and reflects it on the
// LED ring. When BLE connects we tell the USB host to disconnect; when it drops
// we re-attach USB so the wired connection comes back if a cable is present.
void IdleState::updateTransport(unsigned long /*now*/) {
  const bool ble = bleHidController.connected();

  if (ble != bleWasConnected_) {
    bleWasConnected_ = ble;
    if (ble) {
      hidController.detach();  // BLE takes over -> USB host sees an unplug
    } else {
      hidController.attach();  // BLE gone -> re-enumerate over USB if wired
    }
  }

  TransportLed desired;
  if (ble) {
    desired = TransportLed::Ble;
  } else if (hidController.mounted()) {
    desired = TransportLed::Usb;
  } else if (Config::ENABLE_BLE) {
    desired = TransportLed::Waiting;  // on battery, advertising, not connected
  } else {
    desired = TransportLed::Usb;  // USB-only build with no host yet
  }

  if (desired != ledShown_) {
    ledShown_ = desired;
    switch (desired) {
      case TransportLed::Ble:
        ledController.setSolid(Config::LED_BLE_COLOR);
        break;
      case TransportLed::Usb:
        ledController.setSolid(Config::LED_USB_COLOR);
        break;
      case TransportLed::Waiting:
        ledController.startBlink(Config::LED_BLE_COLOR,
                                 Config::LED_BLE_WAIT_BLINK_MS);
        break;
      case TransportLed::None:
        break;
    }
  }

  ledController.updateBlink();  // no-op unless the ring is in blink mode
}

void IdleState::runMotionPipeline(float dt, unsigned long now) {
  float raw[9] = {};
  sensorController.readRaw(raw);

  float motion[6] = {};
  motionController.compute(raw, sensorController.baseline(), dt, motion);

  if (motionController.hasMotionActivity()) {
    lastActivityMs_ = now;
  }

  const uint16_t buttonBits = inputController.buttonBits();

  // Exactly one transport is active at a time; BLE takes priority over USB.
  const bool hidReportSent =
      bleHidController.connected()
          ? bleHidController.sendReports(motion, buttonBits)
          : hidController.sendReports(motion, buttonBits);

  if (telemetryController.enabled()) {
    telemetryController.publish(motion, buttonBits, hidReportSent);
  }
}

void IdleState::handleSleepTransition(unsigned long now) {
  const unsigned long inactiveMs = now - lastActivityMs_;
  if (inactiveMs >= Config::IDLE_SLEEP_TIMEOUT_MS) {
    stateMachine.changeState(&StateMachine::sleepState);
  }
}

void IdleState::update() {
  inputController.update();

  if (handleCalibrationRequest()) {
    return;
  }

  const unsigned long now = millis();
  if (inputController.takeActivity()) {
    lastActivityMs_ = now;
  }

  updateTransport(now);

  const float dt = (lastUpdateMs_ == 0) ? 0.01
                                        : ((now - lastUpdateMs_) / 1000.0);
  lastUpdateMs_ = now;
  runMotionPipeline(dt, now);
  handleSleepTransition(now);
}

void IdleState::exit() {}
