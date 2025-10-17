/*
  ESP32 BLE RC Control – iPhone compatible
  Janick van Bogerijen
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ===== Motorpinnen =====
const int RichtingRechts = 25;
const int SnelheidRechts = 26;
const int RichtingLinks  = 27;
const int SnelheidLinks  = 14;
int snelheid = 180;  // standaard snelheid (0–255)

// ===== BLE UUIDs =====
BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;

#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // RX

// ===== Motorfuncties =====
void vooruit() {
  digitalWrite(RichtingLinks, LOW);
  digitalWrite(RichtingRechts, LOW);
  analogWrite(SnelheidLinks, snelheid);
  analogWrite(SnelheidRechts, snelheid);
}

void achteruit() {
  digitalWrite(RichtingLinks, HIGH);
  digitalWrite(RichtingRechts, HIGH);
  analogWrite(SnelheidLinks, snelheid);
  analogWrite(SnelheidRechts, snelheid);
}

void links() {
  digitalWrite(RichtingLinks, LOW);
  digitalWrite(RichtingRechts, LOW);
  analogWrite(SnelheidLinks, 80);
  analogWrite(SnelheidRechts, snelheid);
}

void rechts() {
  digitalWrite(RichtingLinks, LOW);
  digitalWrite(RichtingRechts, LOW);
  analogWrite(SnelheidLinks, snelheid);
  analogWrite(SnelheidRechts, 80);
}

void stopMotoren() {
  analogWrite(SnelheidLinks, 0);
  analogWrite(SnelheidRechts, 0);
}

// ===== BLE Callbacks =====
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) { deviceConnected = true; }
  void onDisconnect(BLEServer *pServer) { deviceConnected = false; }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue(); // gebruik Arduino String
    if (rxValue.length() > 0) {
      char cmd = rxValue.charAt(0);
      Serial.print("Ontvangen: ");
      Serial.println(cmd);
      switch (cmd) {
        case 'F': vooruit(); break;
        case 'B': achteruit(); break;
        case 'L': links(); break;
        case 'R': rechts(); break;
        case 'S': stopMotoren(); break;
        default: stopMotoren(); break;
      }
    }
  }
};

void setup() {
  Serial.begin(115200);

  pinMode(RichtingLinks, OUTPUT);
  pinMode(SnelheidLinks, OUTPUT);
  pinMode(RichtingRechts, OUTPUT);
  pinMode(SnelheidRechts, OUTPUT);

  // ==== Start BLE ====
  BLEDevice::init("ESP32_RC_BLE");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  pServer->getAdvertising()->start();

  Serial.println("Bluetooth Low Energy gestart!");
  Serial.println("Zoek 'ESP32_RC_BLE' in LightBlue");
}

void loop() {
  // geen code nodig in loop, alles via BLE
}
