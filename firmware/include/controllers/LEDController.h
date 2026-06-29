#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

class LEDController {
 public:
  LEDController();
  void begin();
  void setSolid(unsigned long color);
  void startSpinner(unsigned long color);
  void updateSpinner();
  // Blink the whole ring on/off at the given half-period (ms). Used to show
  // "advertising / waiting for a Bluetooth connection".
  void startBlink(unsigned long color, unsigned long halfPeriodMs);
  void updateBlink();
  void off();

 private:
  enum class Mode {
    Off,
    Solid,
    Spinner,
    Blink,
  };

  void setPower(bool enabled);
  void fillAll(unsigned long color);
  unsigned long toNeoColor(unsigned long color);

  bool isPowered_ = false;
  Mode mode_ = Mode::Off;
  unsigned long color_ = 0;
  int spinnerIndex_ = 0;
  unsigned long lastSpinnerStepMs_ = 0;
  bool blinkOn_ = false;
  unsigned long blinkHalfPeriodMs_ = 0;
  unsigned long lastBlinkMs_ = 0;
  Adafruit_NeoPixel ring_;
};
