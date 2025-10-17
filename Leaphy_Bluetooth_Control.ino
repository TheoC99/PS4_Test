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
  Serial.println("\nESP32 WROOM + Bluepad32 diag");

  // First run ONLY: clear stale bonds on this ESP32:
  //BP32.forgetBluetoothKeys();

  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
  BP32.enableNewBluetoothConnections(true);

  Serial.println("Put controller in pairing mode (DS4: SHARE+PS, Xbox: pair button).");
}

void loop() {
  // Feed the BT stack constantly
  BP32.update();

  // While waiting, print once/sec. Keep delays short.
  static uint32_t t = 0;
  if (!gp || !gp->isConnected()) {
    if (millis() - t > 1000) {
      t = millis();
      Serial.println("[BP32] Waiting for controller…");
    }
    delay(5);   // tiny yield to keep watchdog happy
    return;
  }

  // Connected: print a few inputs every 250 ms
  if (millis() - lastPrint > 20) {
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

  // No long delays here—keep loop responsive
  delay(1);
}
