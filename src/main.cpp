#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <math.h>

// Pinos dos 3 LEDs
const int PIN_LED1 = 25; // LED 1
const int PIN_LED2 = 26; // LED 2
const int PIN_LED3 = 27; // LED 3

// Canais PWM (LEDC)
const int CH_LED1 = 0;
const int CH_LED2 = 1;
const int CH_LED3 = 2;

const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8; // 0 - 255

// Configuração do BLE
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// UUIDs para o serviço UART BLE
#define SERVICE_UUID           "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID    "0000ffe1-0000-1000-8000-00805f9b34fb"

// Modos de Operação:
// 0 = Manual (Controle individual L1, L2, L3 ou Todos)
// 1 = Sequencial / Vai e Vem (Knight Rider)
// 2 = Onda Senoidal / Respiração Tripla (3 fases desfasadas em 120°)
// 3 = Pisca Alternado / Giroflex
// 4 = Strobo / Efeito Estroboscópico
// 5 = Vagalume / Suave Aleatório
int currentMode = 0;

// Estado manual dos LEDs (0 ou 255)
int stateLed1 = 0;
int stateLed2 = 0;
int stateLed3 = 0;

// Valores atuais de PWM enviados para os LEDs (0-255)
int currentVal1 = 0;
int currentVal2 = 0;
int currentVal3 = 0;

// Parâmetros de tempo / frequência
float animFreq = 1.0; // Hz (ajustável via app: F1.5, F2.0, etc.)

// Variáveis para modo Vagalume
float rVal1 = 0, rVal2 = 0, rVal3 = 0;
float rTarget1 = 0, rTarget2 = 0, rTarget3 = 0;

void setLEDs(int val1, int val2, int val3) {
  currentVal1 = constrain(val1, 0, 255);
  currentVal2 = constrain(val2, 0, 255);
  currentVal3 = constrain(val3, 0, 255);

  ledcWrite(CH_LED1, currentVal1);
  ledcWrite(CH_LED2, currentVal2);
  ledcWrite(CH_LED3, currentVal3);
}

// Callbacks do Servidor BLE
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

// Callbacks de Recepção BLE
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

  // Ajuste de Frequência / Velocidade (ex: "F1.5")
  if (cmd[0] == 'F' || cmd[0] == 'f') {
    if (cmd.length() > 1) {
      float f = atof(cmd.substr(1).c_str());
      if (f > 0.05 && f <= 20.0) {
        animFreq = f;
        Serial.print("Velocidade/Freq: ");
        Serial.println(animFreq);
      }
    }
    return;
  }

  // Comandos de texto ou caractere
  if (cmd == "L1" || cmd == "1") {
    currentMode = 0;
    stateLed1 = (stateLed1 == 0) ? 255 : 0;
    Serial.println(stateLed1 ? "LED 1: ON" : "LED 1: OFF");
  } 
  else if (cmd == "L2" || cmd == "2") {
    currentMode = 0;
    stateLed2 = (stateLed2 == 0) ? 255 : 0;
    Serial.println(stateLed2 ? "LED 2: ON" : "LED 2: OFF");
  } 
  else if (cmd == "L3" || cmd == "3") {
    currentMode = 0;
    stateLed3 = (stateLed3 == 0) ? 255 : 0;
    Serial.println(stateLed3 ? "LED 3: ON" : "LED 3: OFF");
  } 
  else if (cmd == "ON" || cmd == "9") {
    currentMode = 0;
    stateLed1 = 255;
    stateLed2 = 255;
    stateLed3 = 255;
    Serial.println("TODOS OS LEDS: ON");
  } 
  else if (cmd == "OFF" || cmd == "0") {
    currentMode = 0;
    stateLed1 = 0;
    stateLed2 = 0;
    stateLed3 = 0;
    Serial.println("TODOS OS LEDS: OFF");
  }
  else if (cmd == "M1" || cmd == "4" || cmd == "SEQ") {
    currentMode = 1;
    Serial.println("MODO: Sequencial (Vai e Vem)");
  }
  else if (cmd == "M2" || cmd == "5" || cmd == "SINE") {
    currentMode = 2;
    Serial.println("MODO: Respiracao Tripla (Senoidal 3 Fases)");
  }
  else if (cmd == "M3" || cmd == "6" || cmd == "ALT") {
    currentMode = 3;
    Serial.println("MODO: Giroflex / Alternado");
  }
  else if (cmd == "M4" || cmd == "7" || cmd == "STROBE") {
    currentMode = 4;
    Serial.println("MODO: Strobo");
  }
  else if (cmd == "M5" || cmd == "8" || cmd == "FIREFLY") {
    currentMode = 5;
    Serial.println("MODO: Vagalume / Aleatorio");
  }
}

// 0: Modo Manual
void handleManualMode() {
  setLEDs(stateLed1, stateLed2, stateLed3);
}

// 1: Sequencial / Vai e Vem (Knight Rider)
void handleSequentialMode() {
  unsigned long t = millis();
  int period = (int)(1000.0 / animFreq);
  if (period < 60) period = 60;
  
  int step = (t % period) / (period / 4); // 0, 1, 2, 3
  
  if (step == 0)      setLEDs(255, 0, 0);   // LED 1
  else if (step == 1) setLEDs(0, 255, 0);   // LED 2
  else if (step == 2) setLEDs(0, 0, 255);   // LED 3
  else                setLEDs(0, 255, 0);   // LED 2 (voltando)
}

// 2: Respiração Tripla (Ondas Senoidais defasadas em 120°)
void handleSineMode() {
  unsigned long t = millis();
  float angle = (t / 1000.0) * animFreq * 2.0 * PI;

  int v1 = (int)((sin(angle) + 1.0) * 127.5);
  int v2 = (int)((sin(angle + (2.0 * PI / 3.0)) + 1.0) * 127.5);
  int v3 = (int)((sin(angle + (4.0 * PI / 3.0)) + 1.0) * 127.5);

  setLEDs(v1, v2, v3);
}

// 3: Alternado / Giroflex
void handleAlternatingMode() {
  unsigned long t = millis();
  int period = (int)(500.0 / animFreq);
  if (period < 40) period = 40;

  bool phase = ((t / period) % 2) == 0;
  if (phase) {
    setLEDs(255, 0, 255); // LEDs 1 e 3
  } else {
    setLEDs(0, 255, 0);   // LED 2
  }
}

// 4: Strobo
void handleStrobeMode() {
  unsigned long t = millis();
  int period = (int)(200.0 / animFreq);
  if (period < 20) period = 20;

  int subTime = t % period;
  if (subTime < (period / 5)) {
    setLEDs(255, 255, 255);
  } else {
    setLEDs(0, 0, 0);
  }
}

// 5: Vagalume / Suave Aleatório
void handleFireflyMode() {
  static unsigned long lastTargetChange = 0;
  unsigned long t = millis();

  if (t - lastTargetChange > (unsigned long)(800 / animFreq)) {
    lastTargetChange = t;
    rTarget1 = random(0, 256);
    rTarget2 = random(0, 256);
    rTarget3 = random(0, 256);
  }

  // Interpolação suave
  rVal1 += (rTarget1 - rVal1) * 0.08;
  rVal2 += (rTarget2 - rVal2) * 0.08;
  rVal3 += (rTarget3 - rVal3) * 0.08;

  setLEDs((int)rVal1, (int)rVal2, (int)rVal3);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n--- ESP32 3-LED Control Iniciando ---");

  // Configuração dos canais PWM
  ledcSetup(CH_LED1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(CH_LED2, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(CH_LED3, PWM_FREQ, PWM_RESOLUTION);

  ledcAttachPin(PIN_LED1, CH_LED1);
  ledcAttachPin(PIN_LED2, CH_LED2);
  ledcAttachPin(PIN_LED3, CH_LED3);

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

  Serial.println("BLE Pronto! Nome: 'ESP32_LED_Control'");
  Serial.println("Pinos configurados: LED1=GPIO25, LED2=GPIO26, LED3=GPIO27");
}

void loop() {
  // Executa o modo ativo
  switch (currentMode) {
    case 0: handleManualMode(); break;
    case 1: handleSequentialMode(); break;
    case 2: handleSineMode(); break;
    case 3: handleAlternatingMode(); break;
    case 4: handleStrobeMode(); break;
    case 5: handleFireflyMode(); break;
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

  // Notificação de telemetria em tempo real para o app (formato "V:v1,v2,v3")
  static unsigned long lastNotifyTime = 0;
  if (deviceConnected && (millis() - lastNotifyTime > 50)) {
    lastNotifyTime = millis();
    char buf[32];
    snprintf(buf, sizeof(buf), "V:%d,%d,%d", currentVal1, currentVal2, currentVal3);
    pCharacteristic->setValue(std::string(buf));
    pCharacteristic->notify();
  }

  delay(10);
}