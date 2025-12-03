# Controle de LEDs via Bluetooth Low Energy (BLE)

## Descrição
Este projeto usa **Bluetooth Low Energy (BLE)** para controlar LEDs. Isso permite que você controle o projeto tanto por **Aplicativos de Celular (Android/iOS)** quanto pela **Interface Web** no navegador.

## Configuração de Hardware

### Conexões dos LEDs:
Todos os 3 LEDs devem ser conectados em paralelo no **GPIO 25**.

**Montagem:**
1. **LED 1 (Controle):** Resistor 1kΩ + LED (sem capacitor)
2. **LED 2 (Médio):** Resistor 1kΩ + LED + Capacitor 800µF
3. **LED 3 (Lento):** Resistor 1kΩ + LED + Capacitor 2200µF

**Esquema de Ligação:**
```
GPIO 25 ──┬──[Resistor 1k]──[LED]── GND
          │
          ├──[Resistor 1k]──[LED]── GND
          │                 │
          │             [Capacitor 800µF]
          │                 │
          │                GND
          │
          └──[Resistor 1k]──[LED]── GND
                            │
                        [Capacitor 2200µF]
                            │
                           GND
```

## Como Usar (Celular)

O BLE funciona de forma diferente do Bluetooth clássico. **Não pareie nas configurações do Android/iOS.** Conecte diretamente dentro do aplicativo.

### 1. Aplicativos Recomendados (Fáceis de usar)

**📱 Android:**
- **Serial Bluetooth Terminal** (de Kai Morich) - *Recomendado*
  1. Abra o app
  2. Vá no menu ☰ -> **Devices**
  3. Mude a aba para **Bluetooth LE**
  4. Clique em **Scan** e conecte em `ESP32_LED_Control`

**📱 iOS (iPhone):**
- **BLE Terminal HM-10**
- **LightBlue**

### 2. Configurando Botões no App (Opcional)
Para facilitar, você pode configurar botões no app "Serial Bluetooth Terminal":
1. Segure um dos botões na parte inferior (M1, M2...)
2. Nome: `Manual` | Valor: `1`
3. Nome: `Quadrada` | Valor: `2`
4. Nome: `Senoidal` | Valor: `3`

## Como Usar (Interface Web)

1. Abra o navegador **Google Chrome** (Android ou PC)
2. Certifique-se que o Bluetooth do dispositivo está ligado
3. Abra o arquivo `web_interface.html`
4. Clique em **Conectar Bluetooth**
5. Selecione `ESP32_LED_Control` na lista

## Comandos

| Comando | Função |
|---------|--------|
| `1` | Liga/Desliga manualmente (Modo Digital) |
| `2` | Ativa Onda Quadrada (Teste de Carga/Descarga) |
| `3` | Ativa Onda Senoidal (Teste de Filtro Suave) |

## Troubleshooting

### O dispositivo não aparece no scan:
- Certifique-se de estar procurando na aba **Bluetooth LE** (não Classic)
- Ative a Localização (GPS) do celular (necessário para scan BLE no Android)
- Reinicie o ESP32

### Erro de conexão na Web:
- A interface web funciona apenas em navegadores baseados no Chromium (Chrome, Edge, Opera)
- No iPhone, o Chrome não suporta Web Bluetooth (limitação da Apple). Use o app **Bluefy** ou um app nativo listado acima.
