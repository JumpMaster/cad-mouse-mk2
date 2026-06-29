#pragma once

#include "State.h"

class IdleState : public State {
 public:
  void enter() override;
  void update() override;
  void exit() override;

 private:
  bool handleCalibrationRequest();
  void updateTransport(unsigned long now);
  void runMotionPipeline(float dt, unsigned long now);
  void handleSleepTransition(unsigned long now);

  // Which transport's status the ring is currently showing.
  enum class TransportLed { None, Usb, Ble, Waiting };

  unsigned long lastUpdateMs_ = 0;
  unsigned long lastActivityMs_ = 0;
  bool bleWasConnected_ = false;
  TransportLed ledShown_ = TransportLed::None;
};
