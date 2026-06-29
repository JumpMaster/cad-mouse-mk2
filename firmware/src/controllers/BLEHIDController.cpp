#include "controllers/BLEHIDController.h"

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include "Config.h"

// Connection bookkeeping. NimBLE 2.x callback signatures include a
// NimBLEConnInfo reference, which differs from the 1.x API.
class BLEHIDController::ServerCallbacks : public NimBLEServerCallbacks {
 public:
  explicit ServerCallbacks(BLEHIDController* owner) : owner_(owner) {}

  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
    owner_->connected_ = true;
    owner_->forceSend_ = true;
    // Ask the central for a fast connection interval (7.5ms-15ms, no slave
    // latency, 4s supervision timeout) so motion feels responsive. The host
    // may negotiate something different; this is only a request.
    server->updateConnParams(connInfo.getConnHandle(),
                             /*minInterval=*/6, /*maxInterval=*/12,
                             /*latency=*/0, /*timeout=*/400);
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

  const HIDReportAxes axes = hidMakeAxesReport(motion);
  const bool sendAxes = forceSend_ || hidAxesChanged(axes, lastSentAxes_);
  const bool sendButtons = forceSend_ || (buttonBits != buttonBitsSent_);
  forceSend_ = false;

  bool sent = false;

  if (sendAxes) {
    inputAxes_->notify(reinterpret_cast<const uint8_t*>(&axes), sizeof(axes));
    lastSentAxes_ = axes;
    sent = true;
  }

  if (sendButtons) {
    HIDReportButtons btn{};
    btn.bits = buttonBits & 0x0003;
    inputButtons_->notify(reinterpret_cast<const uint8_t*>(&btn), sizeof(btn));
    buttonBitsSent_ = buttonBits;
    sent = true;
  }

  return sent;
}
