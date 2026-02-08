# 🐟 HydroSense - Sistema Inteligente de Monitoramento para Aquicultura

<p align="center">
  <img src="https://img.shields.io/badge/Platform-Raspberry%20Pi%20Pico%20W-green?style=for-the-badge&logo=raspberrypi" alt="Platform">
  <img src="https://img.shields.io/badge/Language-C-blue?style=for-the-badge&logo=c" alt="Language">
  <img src="https://img.shields.io/badge/RTOS-FreeRTOS-orange?style=for-the-badge" alt="RTOS">
  <img src="https://img.shields.io/badge/IoT-WiFi%20Enabled-purple?style=for-the-badge&logo=wifi" alt="IoT">
</p>

<p align="center">
  <b>Sistema embarcado de baixo custo para monitoramento e automação de aquários e sistemas de aquicultura</b>
</p>

---

## 📋 Índice

- [Sobre o Projeto](#-sobre-o-projeto)
- [Funcionalidades](#-funcionalidades)
- [Hardware](#-hardware)
- [Arquitetura](#-arquitetura)
- [Instalação](#-instalação)
- [Uso](#-uso)
- [API](#-api)
- [Estrutura do Projeto](#-estrutura-do-projeto)
- [Licença](#-licença)

---

## 🎯 Sobre o Projeto

O **HydroSense** é um sistema de monitoramento IoT desenvolvido para aquicultura, capaz de:

- 🌡️ Monitorar temperatura e umidade do ambiente
- 📏 Medir nível de água em reservatórios
- 🎨 Analisar cor/turbidez da água
- 🌐 Disponibilizar dados via servidor web
- 🤖 Automatizar alimentação com servo motor

### Desenvolvido com:

- **Raspberry Pi Pico W** - Microcontrolador com WiFi
- **BitDogLab** - Placa de desenvolvimento educacional
- **FreeRTOS** - Sistema operacional de tempo real
- **lwIP** - Stack TCP/IP embarcado

---

## ✨ Funcionalidades

### Sensores

| Sensor | Parâmetro | Interface |
|--------|-----------|-----------|
| AHT10 | Temperatura e Umidade | I2C (0x38) |
| VL53L0X | Distância/Nível água | I2C (0x29) |
| TCS34725 | Cor RGB | I2C (0x29) |

### Atuadores

| Atuador | Função | Interface |
|---------|--------|-----------|
| Servo SG90 | Alimentador automático | PWM (GPIO16) |
| LED RGB | Indicador de status | WS2812 (GPIO7) |
| OLED 128x64 | Display local | I2C (0x3C) |

### Conectividade

- 📡 **WiFi 2.4GHz** - Conexão via CYW43439
- 🌐 **Servidor HTTP** - Interface web responsiva
- 🔌 **API JSON** - Integração com outros sistemas
- 📱 **Mobile Ready** - Design responsivo

---

## 🔧 Hardware

### Diagrama de Conexões

```
RASPBERRY PI PICO W + BitDogLab
┌─────────────────────────────────────┐
│                                     │
│  GPIO2/3 ──── I2C Sensores         │
│               (AHT10, VL53L0X)      │
│                                     │
│  GPIO14/15 ── I2C OLED SSD1306     │
│                                     │
│  GPIO16 ───── Servo SG90 (PWM)     │
│                                     │
│  GPIO7 ────── LED RGB WS2812       │
│                                     │
│  WiFi ─────── Antena integrada     │
│                                     │
└─────────────────────────────────────┘
```

### Lista de Materiais

| Componente | Quantidade | Preço Estimado |
|------------|------------|----------------|
| Raspberry Pi Pico W | 1 | R$ 45,00 |
| BitDogLab | 1 | R$ 80,00 |
| Sensor AHT10 | 1 | R$ 15,00 |
| Sensor VL53L0X | 1 | R$ 25,00 |
| Servo SG90 | 1 | R$ 12,00 |
| Jumpers/Protoboard | - | R$ 15,00 |
| **Total** | | **~R$ 192,00** |

---

## 🏗️ Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│                    CAMADA DE APLICAÇÃO                       │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │  Servidor Web   │  │  Display OLED   │  │  API JSON    │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    CAMADA DE FIRMWARE                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │    FreeRTOS     │  │   Drivers I2C   │  │  lwIP Stack  │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    CAMADA DE HARDWARE                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │  Pico W + WiFi  │  │   BitDogLab     │  │   Sensores   │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

---

## 🚀 Instalação

### Pré-requisitos

- Pico SDK 2.0+
- ARM GCC Toolchain
- CMake 3.13+

### Compilação

```bash
# Clone o repositório
git clone https://github.com/hobsonbreno/HydroSense---Sistema-de-Aquicultura-Inteligente.git
cd HydroSense---Sistema-de-Aquicultura-Inteligente

# Configure as variáveis de ambiente
export PICO_SDK_PATH=/path/to/pico-sdk
export PICO_BOARD=pico_w

# Compile
./compile_v6.sh
```

### Gravação

1. Conecte o Pico W segurando o botão **BOOTSEL**
2. Copie o firmware:

```bash
cp build_v6/hydrosense_v6.uf2 /media/$USER/RPI-RP2/
```

---

## 💻 Uso

### Configuração WiFi

Edite o arquivo `src/hydrosense_v6.c`:

```c
#define WIFI_SSID     "SuaRede"
#define WIFI_PASSWORD "SuaSenha"
```

### Acesso à Interface Web

Após conexão, acesse no navegador:

```
http://[IP_DO_PICO]/
```

### Monitor Serial

```bash
cat /dev/ttyACM0
# ou
screen /dev/ttyACM0 115200
```

---

## 🔌 API

### Endpoint: `/api` ou `/json`

**Método:** GET

**Resposta:**

```json
{
  "temperatura": 30.4,
  "umidade": 65.0,
  "distancia": 150,
  "nivel": 50.0,
  "volume": 10.0,
  "cor": {
    "r": 1200,
    "g": 1800,
    "b": 600,
    "c": 3600,
    "nome": "Verde"
  },
  "wifi": true,
  "leituras": 1234
}
```

---

## 📁 Estrutura do Projeto

```
HydroSense/
├── src/
│   ├── hydrosense_v6.c      # Código principal
│   ├── tasks/               # Tarefas FreeRTOS
│   ├── sensors/             # Drivers de sensores
│   ├── actuators/           # Drivers de atuadores
│   └── communication/       # Protocolos
├── include/
│   ├── lwipopts.h           # Config lwIP
│   └── config/              # Configurações
├── FreeRTOS-Kernel/         # Kernel RTOS
├── build_v6/
│   └── hydrosense_v6.uf2    # Firmware
├── CMakeLists.txt
├── FreeRTOSConfig.h
├── compile_v6.sh
├── RELATORIO_FINAL.md       # Relatório do projeto
└── README.md
```

---

## 📊 Interface Web

```
┌─────────────────────────────────────┐
│         🐟 HydroSense               │
│   Sistema de Monitoramento          │
├─────────────────────────────────────┤
│  ┌─────────┐  ┌─────────┐          │
│  │ 🌡️ 30.4°C│  │ 💧 65%  │          │
│  │ Temp    │  │ Umidade │          │
│  └─────────┘  └─────────┘          │
│  ┌─────────┐  ┌─────────┐          │
│  │ 📏 150mm│  │ 🌊 50%  │          │
│  │ Dist    │  │ Nível   │          │
│  └─────────┘  └─────────┘          │
├─────────────────────────────────────┤
│  📡 WiFi: Conectado                 │
│  [🔄 Atualizar]                     │
└─────────────────────────────────────┘
```

---

## 📄 Documentação

- 📋 [Relatório Final](RELATORIO_FINAL.md) - Documentação completa do projeto
- 🎥 Vídeo Demonstrativo (5-7 minutos)

---

## 📄 Licença

Este projeto está sob a licença MIT.

---

## 👤 Autor

**Hobson Breno**

- GitHub: [@hobsonbreno](https://github.com/hobsonbreno)

---

<p align="center">
  Desenvolvido com ❤️ para o curso de Sistemas Embarcados e IoT - 2026
</p>
