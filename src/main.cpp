#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <math.h>

// Pinos dos LEDs por Cor
const int PIN_RED    = 25; // LED Vermelho (D25)
const int PIN_YELLOW = 26; // LED Amarelo (D26)
const int PIN_GREEN  = 27; // LED Verde (D27)

// Canais PWM (LEDC)
const int CH_RED    = 0;
const int CH_YELLOW = 1;
const int CH_GREEN  = 2;

const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8; // 0 - 255

// Configuração do BLE
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID           "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID    "0000ffe1-0000-1000-8000-00805f9b34fb"

// Enumeração dos Modos de Operação
enum OperationMode {
  MODE_MANUAL = 0,
  MODE_TRAFFIC,        // 1: Semáforo Automático (Verde -> Amarelo -> Vermelho)
  MODE_TRAFFIC_ALERT,  // 2: Semáforo em Alerta (Amarelo piscante noturno)
  MODE_F1_START,       // 3: Largada Fórmula 1
  MODE_VU_METER,       // 4: VU Meter / Barra de Potência Dinâmica
  MODE_CANDLE,         // 5: Efeito Vela / Fogo Tremeluzente (Vermelho + Amarelo)
  MODE_HEARTBEAT,      // 6: Batimento Cardíaco (Pulso Duplo no Vermelho)
  MODE_KNIGHT_RIDER,   // 7: Sequencial Vai e Vem
  MODE_SINE_BREATHE,   // 8: Respiração Senoidal 3 Fases
  MODE_POLICE,         // 9: Sirene de Emergência / Giroflex
  MODE_STROBE,         // 10: Strobo Ultra Rápido
  MODE_FIREFLY         // 11: Vagalumes Suaves
};

int currentMode = MODE_MANUAL;

// Estados manuais individuais (0 ou 255)
int stateRed    = 0;
int stateYellow = 0;
int stateGreen  = 0;

// Valores atuais de saída PWM (0-255)
int currentRed    = 0;
int currentYellow = 0;
int currentGreen  = 0;

// Controles Globais
float animFreq = 1.0;          // Velocidade (Hz)
float masterBrightness = 1.0;   // Brilho Geral (0.0 a 1.0)

// Variáveis para efeitos específicos
float fireTargetY = 120, fireTargetR = 200;
float fireCurrentY = 120, fireCurrentR = 200;
float rTarget1 = 0, rTarget2 = 0, rTarget3 = 0;
float rVal1 = 0, rVal2 = 0, rVal3 = 0;

void setLEDs(int r, int y, int g) {
  currentRed    = constrain((int)(r * masterBrightness), 0, 255);
  currentYellow = constrain((int)(y * masterBrightness), 0, 255);
  currentGreen  = constrain((int)(g * masterBrightness), 0, 255);

  ledcWrite(CH_RED, currentRed);
  ledcWrite(CH_YELLOW, currentYellow);
  ledcWrite(CH_GREEN, currentGreen);
}

// Callbacks BLE
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("[BLE] Dispositivo conectado!");
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("[BLE] Dispositivo desconectado!");
    }
};

void processCommand(std::string cmd);

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        Serial.print("[BLE RX] ");
        Serial.println(rxValue.c_str());
        processCommand(rxValue);
      }
    }
};

void processCommand(std::string cmd) {
  if (cmd.empty()) return;

  // Ajuste de Frequência/Velocidade (ex: "F1.5")
  if (cmd[0] == 'F' || cmd[0] == 'f') {
    if (cmd.length() > 1) {
      float f = atof(cmd.substr(1).c_str());
      if (f >= 0.1 && f <= 20.0) {
        animFreq = f;
        Serial.printf("[FREQ] %.2f Hz\n", animFreq);
      }
    }
    return;
  }

  // Ajuste de Brilho Geral (ex: "B100", "B50")
  if (cmd[0] == 'B' || cmd[0] == 'b') {
    if (cmd.length() > 1) {
      int b = atoi(cmd.substr(1).c_str());
      masterBrightness = constrain(b, 0, 100) / 100.0f;
      Serial.printf("[BRIGHTNESS] %.0f%%\n", masterBrightness * 100);
    }
    return;
  }

  // Controle Manual Individual
  if (cmd == "RED" || cmd == "L1" || cmd == "1") {
    currentMode = MODE_MANUAL;
    stateRed = (stateRed == 0) ? 255 : 0;
  }
  else if (cmd == "YELLOW" || cmd == "L2" || cmd == "2") {
    currentMode = MODE_MANUAL;
    stateYellow = (stateYellow == 0) ? 255 : 0;
  }
  else if (cmd == "GREEN" || cmd == "L3" || cmd == "3") {
    currentMode = MODE_MANUAL;
    stateGreen = (stateGreen == 0) ? 255 : 0;
  }
  else if (cmd == "ON" || cmd == "ALL_ON") {
    currentMode = MODE_MANUAL;
    stateRed = 255; stateYellow = 255; stateGreen = 255;
  }
  else if (cmd == "OFF" || cmd == "ALL_OFF" || cmd == "0") {
    currentMode = MODE_MANUAL;
    stateRed = 0; stateYellow = 0; stateGreen = 0;
  }

  // Modos de Efeitos
  else if (cmd == "TRAFFIC" || cmd == "M_TRAFFIC" || cmd == "4") {
    currentMode = MODE_TRAFFIC;
  }
  else if (cmd == "ALERT" || cmd == "M_ALERT" || cmd == "5") {
    currentMode = MODE_TRAFFIC_ALERT;
  }
  else if (cmd == "F1" || cmd == "M_F1" || cmd == "6") {
    currentMode = MODE_F1_START;
  }
  else if (cmd == "VU" || cmd == "M_VU" || cmd == "7") {
    currentMode = MODE_VU_METER;
  }
  else if (cmd == "CANDLE" || cmd == "M_CANDLE" || cmd == "8") {
    currentMode = MODE_CANDLE;
  }
  else if (cmd == "HEART" || cmd == "M_HEART" || cmd == "9") {
    currentMode = MODE_HEARTBEAT;
  }
  else if (cmd == "SEQ" || cmd == "M_SEQ" || cmd == "10") {
    currentMode = MODE_KNIGHT_RIDER;
  }
  else if (cmd == "SINE" || cmd == "M_SINE" || cmd == "11") {
    currentMode = MODE_SINE_BREATHE;
  }
  else if (cmd == "POLICE" || cmd == "M_POLICE" || cmd == "12") {
    currentMode = MODE_POLICE;
  }
  else if (cmd == "STROBE" || cmd == "M_STROBE" || cmd == "13") {
    currentMode = MODE_STROBE;
  }
  else if (cmd == "FIREFLY" || cmd == "M_FIREFLY" || cmd == "14") {
    currentMode = MODE_FIREFLY;
  }
}

// 0: Manual
void handleManual() {
  setLEDs(stateRed, stateYellow, stateGreen);
}

// 1: Semáforo Automático (Verde -> Amarelo -> Vermelho)
void handleTraffic() {
  unsigned long t = millis();
  int cycleTime = (int)(6000.0 / animFreq); // Ciclo base de 6s
  if (cycleTime < 500) cycleTime = 500;
  
  int phase = t % cycleTime;
  int greenTime  = (int)(cycleTime * 0.45); // 45% Verde
  int yellowTime = (int)(cycleTime * 0.15); // 15% Amarelo
  // 40% Vermelho

  if (phase < greenTime) {
    setLEDs(0, 0, 255); // Verde
  } else if (phase < (greenTime + yellowTime)) {
    setLEDs(0, 255, 0); // Amarelo
  } else {
    setLEDs(255, 0, 0); // Vermelho
  }
}

// 2: Semáforo em Alerta Noturno (Amarelo Piscante)
void handleTrafficAlert() {
  unsigned long t = millis();
  int period = (int)(1000.0 / animFreq);
  if (period < 100) period = 100;

  bool on = (t % period) < (period / 2);
  setLEDs(0, on ? 255 : 0, 0);
}

// 3: Largada F1 (Vermelho -> Amarelo -> Verde -> GO!)
void handleF1Start() {
  unsigned long t = millis();
  int cycle = (int)(4000.0 / animFreq);
  if (cycle < 600) cycle = 600;

  int step = (t % cycle) / (cycle / 5); // 0, 1, 2, 3, 4

  if (step == 0)      setLEDs(255, 0, 0);     // 1: Vermelho acende
  else if (step == 1) setLEDs(255, 255, 0);   // 2: Vermelho + Amarelo
  else if (step == 2) setLEDs(255, 255, 255); // 3: Todos Acesos
  else if (step == 3) setLEDs(0, 0, 255);     // 4: GO! Só Verde
  else                setLEDs(0, 0, 0);       // 5: Apagado
}

// 4: VU Meter / Barra de Nível de Potência Simulada
void handleVUMeter() {
  unsigned long t = millis();
  float val = (sin((t / 1000.0) * animFreq * 4.0) + 1.0) * 0.5; // 0.0 a 1.0
  // Adiciona um ruído dinâmico
  val += ((random(0, 100) / 100.0f) - 0.5f) * 0.2f;
  val = constrain(val, 0.0f, 1.0f);

  int g = (val > 0.15f) ? (int)(constrain((val - 0.15f) / 0.35f, 0.0f, 1.0f) * 255) : 0;
  int y = (val > 0.50f) ? (int)(constrain((val - 0.50f) / 0.30f, 0.0f, 1.0f) * 255) : 0;
  int r = (val > 0.80f) ? (int)(constrain((val - 0.80f) / 0.20f, 0.0f, 1.0f) * 255) : 0;

  setLEDs(r, y, g);
}

// 5: Efeito Vela / Fogo Tremeluzente (Vermelho + Amarelo oscilando)
void handleCandle() {
  static unsigned long lastFlicker = 0;
  unsigned long t = millis();

  if (t - lastFlicker > (unsigned long)(50 / animFreq)) {
    lastFlicker = t;
    fireTargetR = random(160, 256);
    fireTargetY = random(30, 180);
  }

  fireCurrentR += (fireTargetR - fireCurrentR) * 0.2f;
  fireCurrentY += (fireTargetY - fireCurrentY) * 0.2f;

  setLEDs((int)fireCurrentR, (int)fireCurrentY, 0);
}

// 6: Batimento Cardíaco (Dois pulsos rápidos no Vermelho + eco suave)
void handleHeartbeat() {
  unsigned long t = millis();
  int period = (int)(1200.0 / animFreq);
  if (period < 200) period = 200;

  int phase = t % period;
  int r = 0, y = 0, g = 0;

  if (phase < (period * 0.10)) {
    // 1º pulso forte (Lub)
    float s = sin((phase / (period * 0.10)) * PI);
    r = (int)(s * 255);
    y = (int)(s * 100);
  } 
  else if (phase >= (period * 0.18) && phase < (period * 0.32)) {
    // 2º pulso intermediário (Dub)
    float s = sin(((phase - (period * 0.18)) / (period * 0.14)) * PI);
    r = (int)(s * 200);
    g = (int)(s * 120);
  }

  setLEDs(r, y, g);
}

// 7: Sequencial Vai e Vem (Knight Rider: Verde -> Amarelo -> Vermelho -> Amarelo)
void handleKnightRider() {
  unsigned long t = millis();
  int period = (int)(800.0 / animFreq);
  if (period < 60) period = 60;

  int step = (t % period) / (period / 4);

  if (step == 0)      setLEDs(0, 0, 255);   // Verde (D27)
  else if (step == 1) setLEDs(0, 255, 0);   // Amarelo (D26)
  else if (step == 2) setLEDs(255, 0, 0);   // Vermelho (D25)
  else                setLEDs(0, 255, 0);   // Amarelo voltando
}

// 8: Respiração 3 Fases (Senoide 120°)
void handleSineBreathe() {
  unsigned long t = millis();
  float angle = (t / 1000.0) * animFreq * 2.0 * PI;

  int r = (int)((sin(angle) + 1.0) * 127.5);
  int y = (int)((sin(angle + (2.0 * PI / 3.0)) + 1.0) * 127.5);
  int g = (int)((sin(angle + (4.0 * PI / 3.0)) + 1.0) * 127.5);

  setLEDs(r, y, g);
}

// 9: Sirene de Emergência / Giroflex (Vermelho pulsando contra Verde/Amarelo)
void handlePolice() {
  unsigned long t = millis();
  int period = (int)(400.0 / animFreq);
  if (period < 40) period = 40;

  int sub = t % period;
  if (sub < (period / 2)) {
    // Fase Vermelha
    bool flash = ((sub / (period / 8)) % 2) == 0;
    setLEDs(flash ? 255 : 0, 0, 0);
  } else {
    // Fase Amarelo/Verde
    bool flash = (((sub - (period / 2)) / (period / 8)) % 2) == 0;
    setLEDs(0, flash ? 255 : 0, flash ? 255 : 0);
  }
}

// 10: Strobo
void handleStrobe() {
  unsigned long t = millis();
  int period = (int)(180.0 / animFreq);
  if (period < 20) period = 20;

  bool flash = (t % period) < (period / 5);
  setLEDs(flash ? 255 : 0, flash ? 255 : 0, flash ? 255 : 0);
}

// 11: Vagalumes Aleatórios
void handleFirefly() {
  static unsigned long lastTargetChange = 0;
  unsigned long t = millis();

  if (t - lastTargetChange > (unsigned long)(700 / animFreq)) {
    lastTargetChange = t;
    rTarget1 = random(0, 256);
    rTarget2 = random(0, 256);
    rTarget3 = random(0, 256);
  }

  rVal1 += (rTarget1 - rVal1) * 0.08f;
  rVal2 += (rTarget2 - rVal2) * 0.08f;
  rVal3 += (rTarget3 - rVal3) * 0.08f;

  setLEDs((int)rVal1, (int)rVal2, (int)rVal3);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== ESP32 LED Control (Vermelho D25, Amarelo D26, Verde D27) ===");

  // Configura canais PWM
  ledcSetup(CH_RED,    PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(CH_YELLOW, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(CH_GREEN,  PWM_FREQ, PWM_RESOLUTION);

  ledcAttachPin(PIN_RED,    CH_RED);
  ledcAttachPin(PIN_YELLOW, CH_YELLOW);
  ledcAttachPin(PIN_GREEN,  CH_GREEN);

  setLEDs(0, 0, 0);

  // Inicializa BLE
  BLEDevice::init("ESP32_LED_Control");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  pAdvertising->start();

  Serial.println("[BLE] Anunciando: 'ESP32_LED_Control'");
}

void loop() {
  switch (currentMode) {
    case MODE_MANUAL:         handleManual();        break;
    case MODE_TRAFFIC:        handleTraffic();       break;
    case MODE_TRAFFIC_ALERT:  handleTrafficAlert();  break;
    case MODE_F1_START:       handleF1Start();       break;
    case MODE_VU_METER:       handleVUMeter();       break;
    case MODE_CANDLE:         handleCandle();        break;
    case MODE_HEARTBEAT:      handleHeartbeat();     break;
    case MODE_KNIGHT_RIDER:   handleKnightRider();   break;
    case MODE_SINE_BREATHE:   handleSineBreathe();   break;
    case MODE_POLICE:         handlePolice();        break;
    case MODE_STROBE:         handleStrobe();        break;
    case MODE_FIREFLY:        handleFirefly();       break;
  }

  // Auto-reconexão BLE
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("[BLE] Reiniciando advertising...");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  // Notificação de telemetria em tempo real (V:Red,Yellow,Green)
  static unsigned long lastNotifyTime = 0;
  if (deviceConnected && (millis() - lastNotifyTime > 50)) {
    lastNotifyTime = millis();
    char buf[32];
    snprintf(buf, sizeof(buf), "V:%d,%d,%d", currentRed, currentYellow, currentGreen);
    pCharacteristic->setValue(std::string(buf));
    pCharacteristic->notify();
  }

  delay(10);
}