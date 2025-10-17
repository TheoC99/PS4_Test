#include <Bluepad32.h>

// ===== Pins (adjust only if your finder said otherwise) =====
const int M1_PWM = 5;    // LEFT PWM  (confirmed)
const int M1_DIR = 38;   // LEFT DIR  (confirmed)
const int M2_PWM = 6;    // RIGHT PWM (confirmed)
const int M2_DIR = 17;   // RIGHT DIR (your pick; change if needed)
const int MOT_EN  = -1;  // set to e.g. 21 if your shield has STBY/EN

// ===== LEDC =====
const int CH_M1 = 0, CH_M2 = 1;
const int PWM_FREQ_HZ = 20000, PWM_RES_BITS = 8;

// ===== Calibration switches =====
bool SWAP_MOTORS  = false;  // leave false unless your sides are physically swapped
bool INVERT_LEFT  = true;   // forward was backwards on left earlier: keep true
bool INVERT_RIGHT = false;  // flip if right runs backwards on forward
bool INVERT_FWD   = false;  // flip LY sense if needed
bool INVERT_TURN  = true;   // FIX: right on LX now turns right correctly

// ===== State =====
GamepadPtr gp = nullptr;
uint32_t tPrint = 0;

// Slew limiter (prevents “toggle” feel). Units: PWM counts per loop.
int prevL = 0, prevR = 0;
const int SLEW_STEP = 8;    // max change per loop (0..255)

// ===== Utils =====
static inline int16_t clamp255(int v){ return v<-255?-255:(v>255?255:v); }
static inline int slewTo(int target, int current, int step){
  if (target > current) return (target - current > step) ? current + step : target;
  if (target < current) return (current - target > step) ? current - step : target;
  return current;
}

void motorsAllLow() {
  pinMode(M1_PWM, OUTPUT); digitalWrite(M1_PWM, LOW);
  pinMode(M2_PWM, OUTPUT); digitalWrite(M2_PWM, LOW);
  pinMode(M1_DIR, OUTPUT); digitalWrite(M1_DIR, LOW);
  pinMode(M2_DIR, OUTPUT); digitalWrite(M2_DIR, LOW);
  if (MOT_EN >= 0) { pinMode(MOT_EN, OUTPUT); digitalWrite(MOT_EN, HIGH); }
}
void pwmSetup() {
  ledcSetup(CH_M1, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcSetup(CH_M2, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttachPin(M1_PWM, CH_M1);
  ledcAttachPin(M2_PWM, CH_M2);
  ledcWrite(CH_M1, 0);
  ledcWrite(CH_M2, 0);
}

void setMotor(int ch, int dirPin, int speed /* -255..255 */) {
  bool dir = speed >= 0;
  uint8_t duty = (uint8_t)abs(speed);
  digitalWrite(dirPin, dir ? HIGH : LOW);
  ledcWrite(ch, duty);
}

void driveMixed(int lCmd, int rCmd) {
  if (INVERT_LEFT)  lCmd = -lCmd;
  if (INVERT_RIGHT) rCmd = -rCmd;
  if (SWAP_MOTORS) { int t = lCmd; lCmd = rCmd; rCmd = t; }

  // Slew-limit for smoothness
  prevL = slewTo(lCmd, prevL, SLEW_STEP);
  prevR = slewTo(rCmd, prevR, SLEW_STEP);

  setMotor(CH_M1, M1_DIR, prevL);
  setMotor(CH_M2, M2_DIR, prevR);
}

// ===== Bluepad32 callbacks =====
void onConnectedGamepad(GamepadPtr g){ if(!gp){ gp=g; Serial.println("[BP32] Gamepad connected"); } }
void onDisconnectedGamepad(GamepadPtr g){
  if (g==gp){ gp=nullptr; Serial.println("[BP32] Gamepad disconnected"); prevL=prevR=0; ledcWrite(CH_M1,0); ledcWrite(CH_M2,0); }
}

// ===== Mapping: LX = turn, LY = forward/back =====
void updateMotorsFromController(GamepadPtr g){
  int lx = g->axisX();   // turn
  int ly = g->axisY();   // forward/back

  // Deadzone (kill drift)
  const int DZ = 80;
  if (abs(lx) < DZ) lx = 0;
  if (abs(ly) < DZ) ly = 0;

  // Normalize [-1..1]; default up = forward
  float turn = lx / 512.0f;
  float fwd  = -ly / 512.0f;

  if (INVERT_TURN) turn = -turn;
  if (INVERT_FWD)  fwd  = -fwd;

  // Differential mix (tank drive)
  float l = fwd + turn;
  float r = fwd - turn;

  // Normalize if needed
  float m = fabsf(l); if (fabsf(r) > m) m = fabsf(r);
  if (m > 1.0f) { l /= m; r /= m; }

  // Near zero -> hard stop (prevents residual creep)
  if (fabsf(fwd) < 0.05f && fabsf(turn) < 0.05f) {
    prevL = prevR = 0;
    ledcWrite(CH_M1, 0); ledcWrite(CH_M2, 0);
    return;
  }

  int L = clamp255((int)(l * 255.0f));
  int R = clamp255((int)(r * 255.0f));

  // No min-duty here (that caused your “toggle” feel). Slew will smooth starts.
  driveMixed(L, R);

  if (millis() - tPrint >= 250) {
    tPrint = millis();
    Serial.print("LY="); Serial.print(ly);
    Serial.print(" LX="); Serial.print(lx);
    Serial.print(" | fwd="); Serial.print(fwd,2);
    Serial.print(" turn="); Serial.print(turn,2);
    Serial.print(" | L="); Serial.print(prevL);
    Serial.print(" R="); Serial.println(prevR);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nESP32-S3 + Bluepad32 + Leaphy drive (fixed turn, smooth ramp)");

  motorsAllLow();
  pwmSetup();

  // BP32.forgetBluetoothKeys(); // only if pairing is sticky
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
  BP32.enableNewBluetoothConnections(true);
}

void loop() {
  BP32.update();
  if (!gp || !gp->isConnected()) {
    prevL = prevR = 0;
    ledcWrite(CH_M1,0); ledcWrite(CH_M2,0);
    static uint32_t t=0; if (millis()-t>1000){ t=millis(); Serial.println("[BP32] Waiting for controller..."); }
    return;
  }
  updateMotorsFromController(gp);
}
