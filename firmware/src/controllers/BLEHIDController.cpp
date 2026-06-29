#include "controllers/BLEHIDController.h"

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include "Config.h"

namespace {
// Pace notifications to ~one per connection event. Derived from the negotiated
// connection interval (in 1.25 ms units). Floored so we never busy-send.
uint32_t pacingFromInterval(uint16_t connItvl1250us) {
  uint32_t ms = (static_cast<uint32_t>(connItvl1250us) * 5) / 4;  // *1.25
  if (ms < 6) {
    ms = 6;
  }
  return ms;
}
}  // namespace

// Connection bookkeeping. NimBLE 2.x callback signatures include a
// NimBLEConnInfo reference, which differs from the 1.x API.
class BLEHIDController::ServerCallbacks : public NimBLEServerCallbacks {
 public:
  explicit ServerCallbacks(BLEHIDController* owner) : owner_(owner) {}

  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
    owner_->connected_ = true;
    owner_->forceSend_ = true;
    owner_->lastNotifyMs_ = 0;
    // Request a tight, low connection interval for responsive motion. The
    // central may negotiate something different; onConnParamsUpdate() captures
    // whatever actually gets used.
    server->updateConnParams(connInfo.getConnHandle(),
                             Config::BLE_CONN_INTERVAL_MIN,
                             Config::BLE_CONN_INTERVAL_MAX,
                             /*latency=*/0,
                             Config::BLE_CONN_SUPERVISION_TIMEOUT);
  }

  void onConnParamsUpdate(NimBLEConnInfo& connInfo) override {
    // Pace our notifications to the interval the link actually settled on.
    owner_->notifyIntervalMs_ = pacingFromInterval(connInfo.getConnInterval());
    if (Config::ENABLE_TELEMETRY) {
      Serial.printf("[BLE] conn interval %.1f ms, pacing %lu ms\n",
                    connInfo.getConnInterval() * 1.25f,
                    static_cast<unsigned long>(owner_->notifyIntervalMs_));
    }
  }

  void onDisconnect(NimBLEServer* /*server*/, NimBLEConnInfo& /*connInfo*/,
                    int /*reason*/) override {
    owner_->connected_ = false;
    // Re-advertising is handled automatically via advertiseOnDisconnect(true).
  }

 private:
  BLEHIDController* owner_;
};

void BLEHIDController::begin() {
  if (!Config::ENABLE_BLE) {
    return;
  }

  NimBLEDevice::init(Config::BLE_DEVICE_NAME);
  NimBLEDevice::setPower(Config::BLE_TX_POWER_DBM);

  // Bonding (so the host remembers us) + secure connections, but no MITM and
  // no input/output capability -> "Just Works" pairing, appropriate for a
  // device with no display or keypad.
  NimBLEDevice::setSecurityAuth(/*bonding=*/true, /*mitm=*/false, /*sc=*/true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  server_ = NimBLEDevice::createServer();
  callbacks_ = new ServerCallbacks(this);
  server_->setCallbacks(callbacks_);
  server_->advertiseOnDisconnect(true);

  hid_ = new NimBLEHIDDevice(server_);
  hid_->setManufacturer("3Dconnexion");
  // PnP source 0x02 = USB Implementer's Forum vendor IDs. Reuse the same
  // VID/PID/version as the USB descriptor so the host treats it as the same
  // product family.
  hid_->setPnp(0x02, Config::HID_VID, Config::HID_PID, Config::HID_VERSION);
  // Country code 0 (not localized), flags 0x01 = remote wake capable.
  hid_->setHidInfo(0x00, 0x01);
  hid_->setReportMap(const_cast<uint8_t*>(kSpaceMouseHidDescriptor),
                     sizeof(kSpaceMouseHidDescriptor));

  inputAxes_ = hid_->getInputReport(HID_REPORT_ID_AXES);
  inputButtons_ = hid_->getInputReport(HID_REPORT_ID_BUTTONS);

  hid_->setBatteryLevel(Config::BLE_BATTERY_LEVEL);

  // Starts the HID, device-info and battery services.
  server_->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setName(Config::BLE_DEVICE_NAME);
  adv->setAppearance(GENERIC_HID);
  adv->addServiceUUID(hid_->getHidService()->getUUID());
  adv->enableScanResponse(true);
  adv->start();
}

void BLEHIDController::setBatteryLevel(uint8_t percent) {
  if (hid_ != nullptr) {
    hid_->setBatteryLevel(percent, /*notify=*/connected_);
  }
}

bool BLEHIDController::sendReports(const float motion[6], uint16_t buttonBits) {
  if (!Config::ENABLE_BLE || !connected_ || inputAxes_ == nullptr) {
    return false;
  }

  // Pace to roughly one notification per connection event. Sending faster only
  // fills the controller's buffers, which adds latency and causes drops.
  const uint32_t now = millis();
  if ((now - lastNotifyMs_) < notifyIntervalMs_) {
    return false;
  }

  const HIDReportAxes axes = hidMakeAxesReport(motion);
  bool sent = false;

  if (forceSend_ || hidAxesChanged(axes, lastSentAxes_)) {
    // Only commit lastSentAxes_ if the notification was actually accepted. If
    // the link is momentarily full, notify() returns false and we keep the old
    // value so the next loop retries with the freshest reading instead of
    // dropping the update.
    if (inputAxes_->notify(reinterpret_cast<const uint8_t*>(&axes),
                           sizeof(axes))) {
      lastSentAxes_ = axes;
      forceSend_ = false;
      sent = true;
    }
  }

  if (forceSend_ || buttonBits != buttonBitsSent_) {
    HIDReportButtons btn{};
    btn.bits = buttonBits & 0x0003;
    if (inputButtons_->notify(reinterpret_cast<const uint8_t*>(&btn),
                              sizeof(btn))) {
      buttonBitsSent_ = buttonBits;
      sent = true;
    }
  }

  if (sent) {
    lastNotifyMs_ = now;
  }

  return sent;
}