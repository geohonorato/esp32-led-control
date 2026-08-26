# Controle de 3 LEDs via Bluetooth Low Energy (BLE) - ESP32

## Descrição
Projeto para controle individual e animações sincronizadas em **3 LEDs independentes** usando **Bluetooth Low Energy (BLE)** no ESP32. Permite controle tanto por aplicativos no celular (Android/iOS) quanto via navegador Web no Google Chrome / Edge com Web Bluetooth.

---

## 🔌 Esquema de Ligação de Hardware

Cada LED deve ser conectado a um pino GPIO próprio com seu respectivo resistor (recomendado **330 Ω**):

- **LED 1:** `GPIO 25` ───[ Resistor 330Ω ]───( + Ânodo ) LED 1 ( - Cátodo )─── `GND`
- **LED 2:** `GPIO 26` ───[ Resistor 330Ω ]───( + Ânodo ) LED 2 ( - Cátodo )─── `GND`
- **LED 3:** `GPIO 27` ───[ Resistor 330Ω ]───( + Ânodo ) LED 3 ( - Cátodo )─── `GND`

```text
ESP32 [ GPIO 25 ] ───[ Resistor 330Ω ]───( + ) LED 1 ( - )───┐
ESP32 [ GPIO 26 ] ───[ Resistor 330Ω ]───( + ) LED 2 ( - )───┼── ESP32 [ GND ]
ESP32 [ GPIO 27 ] ───[ Resistor 330Ω ]───( + ) LED 3 ( - )───┘
```

---

## 📱 Como Usar no Celular

O BLE funciona de forma diferente do Bluetooth clássico (não pareie pelas configurações do sistema). Conecte direto pelo app ou navegador:

### Opção 1: Pela Interface Web (Recomendado)
1. Abra o arquivo `index.html` no Google Chrome ou Microsoft Edge (PC ou Android).
2. Clique em **"Conectar Bluetooth"** e selecione `ESP32_LED_Control`.
3. Tenha o painel completo com botões individuais, efeitos visuais, slider de velocidade e osciloscópio multicanal em tempo real.

### Opção 2: Pelo App Serial Bluetooth Terminal (Android / iOS)
1. Abra o app **Serial Bluetooth Terminal** (ou **BLE Terminal HM-10** no iOS).
2. Mude a aba para **Bluetooth LE**, faça o scan e conecte em `ESP32_LED_Control`.
3. Envie os comandos abaixo:

| Comando | Ação |
|---|---|
| `1` ou `L1` | Alterna LED 1 (GPIO 25) ON/OFF |
| `2` ou `L2` | Alterna LED 2 (GPIO 26) ON/OFF |
| `3` ou `L3` | Alterna LED 3 (GPIO 27) ON/OFF |
| `ON` ou `9` | Liga todos os 3 LEDs |
| `OFF` ou `0` | Desliga todos os LEDs |
| `4` ou `M1` | **Modo Sequencial (Vai e Vem / Knight Rider)** |
| `5` ou `M2` | **Modo Respiração 3 Fases (Senoide desfasada em 120°)** |
| `6` ou `M3` | **Modo Giroflex / Alternado (1+3 vs 2)** |
| `7` ou `M4` | **Modo Strobo (Flash rápido)** |
| `8` ou `M5` | **Modo Vagalume (Fade aleatório suave)** |
| `F1.5` | Ajusta a frequência/velocidade para 1.5 Hz (ex: `F0.5`, `F3.0`) |

