#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <math.h>

// Configuração do BLE
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// UUIDs para o serviço UART (compatível com módulos HM-10 e a interface web)
#define SERVICE_UUID           "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID    "0000ffe1-0000-1000-8000-00805f9b34fb"

// Pino de saída (conecte os 3 LEDs em paralelo neste pino)
const int OUTPUT_PIN = 25; 

// Configurações PWM
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

// Variáveis de controle
bool ledsOn = false;
int currentMode = 0; // 0 = manual, 1 = onda quadrada, 2 = onda senoidal
bool pwmAttached = false;

// Parâmetros das ondas
const int SQUARE_WAVE_PERIOD = 1000; // 1 segundo
const float SINE_WAVE_FREQ = 1.0; // 1 Hz

// Callbacks do Servidor BLE
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Dispositivo conectado!");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Dispositivo desconectado!");
      // O reinício do advertising será tratado no loop principal para maior estabilidade
    }
};

// Funções de controle (declaração antecipada)
void processCommand(char command);

// Callbacks da Característica (Recebimento de dados)
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.print("Recebido: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);
        Serial.println();

        // Processa o primeiro caractere recebido
        processCommand(rxValue[0]);
      }
    }
};

void enablePWM() {
  if (!pwmAttached) {
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(OUTPUT_PIN, PWM_CHANNEL);
    pwmAttached = true;
  }
}

void disablePWM() {
  if (pwmAttached) {
    ledcDetachPin(OUTPUT_PIN);
    pinMode(OUTPUT_PIN, OUTPUT);
    pwmAttached = false;
  }
}

void handleManualMode() {
  disablePWM();
  digitalWrite(OUTPUT_PIN, ledsOn ? HIGH : LOW);
}

void handleSquareWave() {
  disablePWM();
  unsigned long currentTime = millis();
  int period = currentTime % SQUARE_WAVE_PERIOD;
  
  if (period < SQUARE_WAVE_PERIOD / 2) {
    digitalWrite(OUTPUT_PIN, HIGH);
  } else {
    digitalWrite(OUTPUT_PIN, LOW);
  }
}

void handleSineWave() {
  enablePWM();
  unsigned long currentTime = millis();
  float angle = (currentTime / 1000.0) * SINE_WAVE_FREQ * 2 * PI;
  int pwmValue = (int)((sin(angle) + 1) * 127.5);
  ledcWrite(PWM_CHANNEL, pwmValue);
}

void processCommand(char command) {
  switch (command) {
    case '1':
      currentMode = 0;
      ledsOn = !ledsOn;
      Serial.println(ledsOn ? "MANUAL: LIGADO" : "MANUAL: DESLIGADO");
      break;
    case '2':
      currentMode = 1;
      Serial.println("MODO: ONDA QUADRADA");
      break;
    case '3':
      currentMode = 2;
      Serial.println("MODO: ONDA SENOIDAL");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configura pino
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);

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
  pAdvertising->setMinPreferred(0x06);  // Ajuda com conexões iPhone
  pAdvertising->setMaxPreferred(0x12);
  pAdvertising->start();
  
  Serial.println("BLE Iniciado! Aguardando conexão...");
}

void loop() {
  // Lógica principal de controle dos LEDs
  switch (currentMode) {
    case 0: handleManualMode(); break;
    case 1: handleSquareWave(); break;
    case 2: handleSineWave(); break;
  }
  
  // Gerenciamento de conexão (reconexão automática)
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); 
      pServer->startAdvertising(); 
      Serial.println("Reiniciando advertising...");
      oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
  }
  
  // Status periódico no Serial (Heartbeat)
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 2000) {
    lastStatusTime = millis();
    Serial.print("Status: ");
    Serial.print(deviceConnected ? "Conectado | " : "Desconectado | ");
    Serial.print("Modo: ");
    switch(currentMode) {
      case 0: Serial.print("Manual ("); Serial.print(ledsOn ? "ON" : "OFF"); Serial.print(")"); break;
      case 1: Serial.print("Onda Quadrada"); break;
      case 2: Serial.print("Onda Senoidal"); break;
    }
    Serial.println();
  }
  
  delay(5);
}