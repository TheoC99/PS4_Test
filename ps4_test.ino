#include <Bluepad32.h>

GamepadPtr gp = nullptr;
uint32_t lastPrint = 0;

void onConnectedGamepad(GamepadPtr g) {
  if (!gp) { gp = g; Serial.println("[BP32] Gamepad connected"); }
}
void onDisconnectedGamepad(GamepadPtr g) {
  if (g == gp) { gp = nullptr; Serial.println("[BP32] Gamepad disconnected"); }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nESP32 + Bluepad32 DS4 diag");

  // First run: uncomment to clear stale pairings on the ESP32
  BP32.forgetBluetoothKeys();

  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
  BP32.enableNewBluetoothConnections(true);

  Serial.println("Put DS4 in pairing: hold SHARE + PS until rapid white flash.");
}

void loop() {
  // MUST be called frequently; don't block here
  BP32.update();

  // Light status every second while waiting
  static uint32_t t = 0;
  if (!gp || !gp->isConnected()) {
    if (millis() - t > 1000) {
      t = millis();
      Serial.println("[BP32] Waiting for controller…");
    }
    // keep loop responsive
    delay(5);
    return;
  }

  // Connected: print inputs ~4 Hz
  if (millis() - lastPrint > 250) {
    lastPrint = millis();
    Serial.print("LX:"); Serial.print(gp->axisX());
    Serial.print(" LY:"); Serial.print(gp->axisY());
    Serial.print(" RX:"); Serial.print(gp->axisRX());
    Serial.print(" RY:"); Serial.print(gp->axisRY());
    Serial.print(" L2:"); Serial.print(gp->brake());
    Serial.print(" R2:"); Serial.print(gp->throttle());
    Serial.print(" D:");  Serial.print((int)gp->dpad());
    Serial.print(" M:0x");Serial.println(gp->miscButtons(), HEX);
  }
}
