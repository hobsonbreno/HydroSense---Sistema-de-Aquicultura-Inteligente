# HydroSense — Sistema Inteligente de Monitoramento e Automação para Aquicultura

**Projeto Final — Sistemas Embarcados e IoT**

---

**Aluno:** Hobson Breno  
**Repositório GitHub:** [https://github.com/hobsonbreno/HydroSense---Sistema-de-Aquicultura-Inteligente](https://github.com/hobsonbreno/HydroSense---Sistema-de-Aquicultura-Inteligente)  
**Vídeo Demonstrativo (5–7 min):** [https://drive.google.com/file/d/1e51nFBpVCmwORC9yHVQrp4OObZA-HNOL/view?usp=sharing](https://drive.google.com/file/d/1e51nFBpVCmwORC9yHVQrp4OObZA-HNOL/view?usp=sharing)  
**Data:** Fevereiro de 2026

---

## Sumário

1. [Introdução](#1-introdução)
2. [Objetivos](#2-objetivos)
3. [Justificativa](#3-justificativa)
4. [Problemática](#4-problemática)
5. [Arquitetura do Sistema (Hardware + Firmware + IoT)](#5-arquitetura-do-sistema-hardware--firmware--iot)
6. [Diagrama de Blocos](#6-diagrama-de-blocos)
7. [Desenvolvimento do Sistema](#7-desenvolvimento-do-sistema)
8. [Descrição dos Módulos, Tarefas, Sensores e Protocolos](#8-descrição-dos-módulos-tarefas-sensores-e-protocolos)
9. [Evidências de Funcionamento](#9-evidências-de-funcionamento)
10. [Conclusão](#10-conclusão)
11. [Referências](#11-referências)
12. [Apêndices](#12-apêndices)

---

## 1. Introdução

A aquicultura é um dos setores de produção de alimentos que mais cresce no mundo. No entanto, a manutenção de parâmetros ambientais adequados — como temperatura, umidade e nível de água — é crítica para a saúde dos organismos aquáticos e para a produtividade do sistema. Falhas no monitoramento podem levar à morte de peixes, desperdício de ração e perda financeira significativa.

O **HydroSense** é um sistema embarcado de baixo custo desenvolvido para monitoramento e automação inteligente de aquários e sistemas de aquicultura. Construído sobre a plataforma **Raspberry Pi Pico W** em conjunto com a placa educacional **BitDogLab**, o sistema integra sensores de temperatura/umidade (AHT10), distância/nível de água (VL53L0X), display OLED (SSD1306), servo motor para alimentação automática (SG90), módulo de relés para controle de equipamentos (ventilador, bombas de TPA), além de conectividade **WiFi** com servidor HTTP embarcado e uma interface web completa com **síntese de voz (TTS)** em português brasileiro.

O projeto aplica na prática os conceitos de **sistemas embarcados**, **IoT (Internet das Coisas)**, **protocolos de comunicação** (I2C, HTTP, TCP/IP), **PWM**, **GPIO**, e **programação bare-metal em C** utilizando o **Pico SDK 2.2.0** e a stack de rede **lwIP**.

---

## 2. Objetivos

### 2.1. Objetivo Geral

Desenvolver um sistema embarcado IoT de monitoramento e automação para aquicultura, utilizando a plataforma Raspberry Pi Pico W + BitDogLab, capaz de coletar dados ambientais em tempo real, acionar atuadores automaticamente e disponibilizar uma interface web acessível remotamente.

### 2.2. Objetivos Específicos

- **Monitorar** temperatura e umidade do ambiente do aquário em tempo real utilizando o sensor AHT10 via protocolo I2C.
- **Medir** o nível de água do reservatório com o sensor de distância a laser VL53L0X, calculando volume e percentual de enchimento.
- **Automatizar** o controle de ventilação com acionamento por relé (LN1) quando a temperatura ultrapassar o limiar configurado (29°C).
- **Automatizar** a alimentação dos peixes via servo motor SG90, com horários programados (08:00 e 16:00) e acionamento manual via interface web.
- **Controlar** bombas de TPA (Troca Parcial de Água) via relés LN2 e LN3, com acionamento remoto pela interface web.
- **Exibir** dados localmente em display OLED SSD1306 128×64 com atualização a cada 2 segundos.
- **Disponibilizar** uma API REST (JSON) via servidor HTTP embarcado no Pico W, acessível pela rede WiFi.
- **Construir** um dashboard web responsivo com monitoramento em tempo real, controle de relés, log de eventos e síntese de voz (TTS) para alertas sonoros.
- **Indicar** visualmente o estado do sistema com LED RGB e alertas sonoros com buzzer.

---

## 3. Justificativa

A aquicultura brasileira movimentou R$ 11,3 bilhões em 2024 (Peixe BR, 2024), mas a mortalidade por falhas de monitoramento ainda é um dos principais desafios, especialmente em pequenos produtores e aquaristas que não possuem acesso a soluções comerciais de automação, frequentemente caras e proprietárias.

O **HydroSense** se justifica por oferecer:

1. **Custo acessível**: com investimento total estimado em ~R$ 192,00 em componentes, utilizando hardware aberto e de baixo custo (Raspberry Pi Pico W, sensores I2C genéricos, servo SG90).

2. **Solução integrada**: ao contrário de sistemas que apenas monitoram ou apenas automatizam, o HydroSense unifica sensoriamento, automação, exibição local (OLED) e remota (web), alimentação automática e controle de equipamentos em um único dispositivo.

3. **Acessibilidade**: a interface web com síntese de voz em português brasileiro (Web Speech API) permite que o sistema emita alertas falados sobre o estado dos equipamentos e parâmetros, aumentando a acessibilidade e a percepção de eventos críticos.

4. **Caráter educacional**: o projeto consolida conhecimentos em sistemas embarcados, protocolos I2C, PWM, TCP/IP, HTTP, programação em C, manipulação direta de registradores de hardware e integração hardware-software, sendo uma aplicação prática e completa dos conteúdos abordados no curso.

5. **Escalabilidade**: a arquitetura modular permite a adição futura de novos sensores (pH, oxigênio dissolvido, turbidez), protocolos (MQTT, WebSocket) e integração com plataformas de nuvem.

---

## 4. Problemática

Os sistemas de aquicultura e aquários domésticos enfrentam desafios recorrentes:

- **Variações bruscas de temperatura** podem causar estresse térmico e morte dos peixes. O monitoramento manual é sujeito a esquecimento e imprecisão.

- **Nível de água inadequado** — evaporação natural, vazamentos ou falhas em bombas podem levar a níveis críticos, expondo equipamentos ou sufocando os peixes.

- **Alimentação irregular** — a falta de alimentação nos horários corretos impacta diretamente o crescimento e a saúde dos animais. O excesso gera degradação da qualidade da água.

- **Trocas Parciais de Água (TPA)** — essenciais para manutenção da qualidade da água, são frequentemente negligenciadas por exigirem esforço manual.

- **Falta de visibilidade remota** — produtores não conseguem monitorar seus sistemas quando estão ausentes, perdendo a capacidade de reação a eventos críticos.

**Pergunta norteadora:** *Como desenvolver um sistema embarcado de baixo custo capaz de monitorar, automatizar e notificar em tempo real os parâmetros críticos de um sistema de aquicultura, proporcionando controle local e remoto?*

O HydroSense responde a essa pergunta integrando sensoriamento, automação, conectividade WiFi e interface web com voz em uma solução embarcada completa.

---

## 5. Arquitetura do Sistema (Hardware + Firmware + IoT)

O HydroSense é estruturado em três camadas:

### 5.1. Camada de Hardware

| Componente | Modelo | Interface | GPIO | Função |
|---|---|---|---|---|
| Microcontrolador | Raspberry Pi Pico W (RP2040 + CYW43439) | — | — | Processamento, WiFi |
| Placa Base | BitDogLab | — | — | Plataforma educacional com periféricos |
| Sensor Temp/Umid | AHT10 | I2C (0x38) | GPIO 2 (SDA), GPIO 3 (SCL) | Monitoramento ambiental |
| Sensor Distância | VL53L0X | I2C (0x29) | GPIO 2 (SDA), GPIO 3 (SCL) | Nível de água (ToF laser) |
| Display | SSD1306 OLED 128×64 | I2C (0x3C) | GPIO 14 (SDA), GPIO 15 (SCL) | Exibição local |
| Servo Motor | SG90 | PWM 50Hz | GPIO 16 | Alimentação automática |
| LED RGB | Integrado BitDogLab | GPIO Digital | GPIO 12 (R), 13 (G), 11 (B) | Indicação visual |
| Buzzer | Integrado BitDogLab | GPIO Digital | GPIO 21 | Alertas sonoros |
| Botões | A e B (BitDogLab) | GPIO Digital | GPIO 5 (A), GPIO 6 (B) | Interação local |
| Relé LN1 | Módulo relé 3 canais | GPIO Digital | GPIO 17 | Ventilador/Aerador |
| Relé LN2 | Módulo relé 3 canais | GPIO Digital | GPIO 18 | Bomba TPA — Esvaziar |
| Relé LN3 | Módulo relé 3 canais | GPIO Digital | GPIO 19 | Bomba TPA — Encher |

**Nota sobre I2C Switching:** Os sensores (AHT10 + VL53L0X) e o display OLED compartilham o periférico I2C1 do RP2040, mas em pinos GPIO distintos. O firmware implementa um mecanismo de **I2C switching** que alterna dinamicamente entre os pares de pinos (GPIO 2/3 para sensores a 100 kHz e GPIO 14/15 para OLED a 400 kHz), reinicializando o barramento a cada alternância.

### 5.2. Camada de Firmware

- **Linguagem:** C
- **SDK:** Pico SDK 2.2.0
- **RTOS:** FreeRTOS (kernel para RP2040) — escalonamento preemptivo, semáforos, filas
- **Stack de Rede:** lwIP (Lightweight IP) em modo NO_SYS (polling)
- **Protocolo WiFi:** WPA2-AES (com fallback para WPA2-MIXED e WPA-TKIP)
- **Servidor HTTP:** TCP raw (porta 80) com callbacks `accept`, `recv` e `sent`
- **Watchdog:** Hardware watchdog (timeout 8s) para recuperação automática de travamentos
- **RTC:** Real-Time Clock do RP2040 para controle de horários de alimentação
- **Arquitetura de Tasks:** 3 tasks FreeRTOS concorrentes + loop principal com polling a cada 100 ms

### 5.3. Camada IoT / Aplicação Web

- **Backend:** Node.js + Express (porta 3000), servindo API REST e dados simulados/proxy para o Pico W
- **Frontend:** HTML5/CSS3/JavaScript puro (porta 3001), dashboard responsivo full-width com CSS Grid
- **Síntese de Voz:** Web Speech API (SpeechSynthesis) em pt-BR
- **Comunicação Pico ↔ Frontend:** HTTP/JSON via rede WiFi (polling a cada 2 segundos)

### 5.4. Custo Total do Projeto

| Componente | Qtd | Custo Estimado |
|---|---|---|
| Raspberry Pi Pico W | 1 | R$ 45,00 |
| BitDogLab | 1 | R$ 80,00 |
| Sensor AHT10 | 1 | R$ 15,00 |
| Sensor VL53L0X | 1 | R$ 25,00 |
| Servo SG90 | 1 | R$ 12,00 |
| Módulo Relé 3 Canais | 1 | R$ 18,00 |
| Jumpers / Protoboard | — | R$ 15,00 |
| **TOTAL** | | **~R$ 210,00** |

---

## 6. Diagrama de Blocos

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          HYDROSENSE - DIAGRAMA DE BLOCOS                    │
└─────────────────────────────────────────────────────────────────────────────┘

                         ┌──────────────────────┐
                         │    INTERFACE WEB      │
                         │  (Dashboard HTML5)    │
                         │   Porta 3001          │
                         │  ┌────────────────┐   │
                         │  │ Monitoramento  │   │
                         │  │ Controle Relés │   │
                         │  │ Alimentação    │   │
                         │  │ TTS Voz pt-BR  │   │
                         │  │ Log de Eventos │   │
                         │  └────────────────┘   │
                         └──────────┬───────────┘
                                    │ HTTP/JSON
                         ┌──────────▼───────────┐
                         │   BACKEND Node.js     │
                         │   Express API         │
                         │   Porta 3000          │
                         └──────────┬───────────┘
                                    │ HTTP/JSON
                                    │ WiFi 2.4GHz
              ┌─────────────────────▼─────────────────────────┐
              │          RASPBERRY PI PICO W (RP2040)          │
              │               Firmware v10 (C)                 │
              │          Pico SDK 2.2.0 + lwIP                 │
              │                                                │
              │  ┌──────────┐  ┌──────────┐  ┌──────────────┐ │
              │  │ Servidor │  │  Leitura  │  │  Automação   │ │
              │  │   HTTP   │  │ Sensores  │  │  (Temp>29°C) │ │
              │  │ Porta 80 │  │  ~2s loop │  │  Alimentação │ │
              │  └──────────┘  └──────────┘  └──────────────┘ │
              └───┬────┬────┬────┬────┬────┬────┬────┬────┬───┘
                  │    │    │    │    │    │    │    │    │
      ┌───────────┘    │    │    │    │    │    │    │    └──────────┐
      │                │    │    │    │    │    │    │               │
┌─────▼─────┐   ┌──────▼────▼──┐ │  ┌▼────▼─┐ │  ┌─▼───────┐ ┌───▼──────┐
│   OLED     │   │   SENSORES   │ │  │ LED   │ │  │  SERVO  │ │  RELÉS   │
│  SSD1306   │   │  I2C (i2c1)  │ │  │  RGB  │ │  │  SG90   │ │  3 CH    │
│  128×64    │   │              │ │  │       │ │  │         │ │          │
│ GPIO 14/15 │   │ AHT10 (0x38)│ │  │ R:G12 │ │  │ GPIO 16 │ │ LN1:G17  │
│ I2C 400kHz │   │ Temp/Umid   │ │  │ G:G13 │ │  │ PWM50Hz │ │ LN2:G18  │
│            │   │              │ │  │ B:G11 │ │  │         │ │ LN3:G19  │
│ Status     │   │ VL53L0X(29) │ │  │       │ │  │ Aliment.│ │          │
│ Dados      │   │ Dist/Nível  │ │  │Status │ │  │ Automát.│ │Ventilador│
│ IP/WiFi    │   │ Volume      │ │  │Visual │ │  │ 08h/16h │ │Bomba Esv.│
└────────────┘   └─────────────┘ │  └───────┘ │  └─────────┘ │Bomba Ench│
                                 │             │              └──────────┘
                          ┌──────▼──┐   ┌──────▼──┐
                          │ BUZZER  │   │ BOTÕES  │
                          │ GPIO 21 │   │ A: GP5  │
                          │ Alertas │   │ B: GP6  │
                          └─────────┘   └─────────┘
```

### Diagrama de Comunicação de Rede

```
┌──────────────┐         WiFi 2.4GHz          ┌──────────────────┐
│   Browser    │◄────── HTTP/JSON ──────────►  │   Pico W         │
│   (PC/Mobile)│       10.0.0.181:80           │   Servidor HTTP  │
│              │                               │                  │
│  Frontend    │         localhost              │   GET /sensors   │
│  localhost   │◄──── HTTP/JSON ────►┌─────┐   │   GET /status    │
│  :3001       │     localhost:3000  │Back-│   │   POST /relay    │
│              │                    │end  │   │   POST /feed     │
│              │                    │Node │   │   GET / (HTML)    │
└──────────────┘                    └─────┘   └──────────────────┘
```

---

## 7. Desenvolvimento do Sistema

### 7.1. Evolução do Firmware

O desenvolvimento foi iterativo, passando por 10 versões até o firmware final:

| Versão | Descrição | Status |
|---|---|---|
| v1–v3 | Testes básicos de I2C, OLED, servo | Protótipos |
| v4 | Integração AHT10 + VL53L0X | Funcional |
| v5 | Adição de servo motor SG90 | Funcional |
| v6 | OLED com I2C switching | Funcional |
| v7 | Servidor HTTP básico (GET /sensors) | Funcional |
| v9 | Relés GPIO + automação de temperatura | Funcional |
| **v10** | **Firmware final completo** — WiFi, HTTP com 5 endpoints, HTML embarcado, OLED, relés, servo, LED RGB, buzzer, automação | **✅ Final** |

### 7.2. Principais Desafios Técnicos e Soluções

#### 7.2.1. I2C Switching (GPIO Multiplexação)

**Problema:** O RP2040 possui apenas 2 periféricos I2C (i2c0 e i2c1). Os sensores AHT10 e VL53L0X estão conectados via extensor nos GPIO 2/3, enquanto o OLED SSD1306 está conectado diretamente na BitDogLab nos GPIO 14/15. Ambos compartilham o periférico i2c1.

**Solução:** Implementação de funções `i2c_switch_to_sensors()` e `i2c_switch_to_oled()` que:
1. Desconectam os pinos do modo atual (`GPIO_FUNC_NULL`)
2. Desinicializam o i2c1 (`i2c_deinit`)
3. Reinicializam com a frequência adequada (100 kHz para sensores, 400 kHz para OLED)
4. Conectam os novos pinos (`GPIO_FUNC_I2C`) com pull-up
5. Guardam o modo atual para evitar alternâncias desnecessárias

#### 7.2.2. Página HTML Embarcada — Problema de Memória lwIP

**Problema:** Ao servir a página HTML completa (>3 KB) via servidor HTTP embarcado, a resposta chegava com 0 bytes no navegador.

**Solução:** O `MEM_SIZE` padrão do lwIP era 4000 bytes, insuficiente para armazenar a página HTML no heap do lwIP. A solução foi aumentar para **16000 bytes** no `lwipopts.h` e separar o envio em duas chamadas `tcp_write()` (header + body), implementando um callback `http_sent_cb` que só fecha a conexão TCP após toda a resposta ser confirmada (ACKed).

#### 7.2.3. Servidor HTTP TCP Raw com lwIP

**Problema:** O Pico W não possui sistema operacional completo. O lwIP opera em modo `NO_SYS` (polling), exigindo que todo o processamento de rede seja feito via callbacks assíncronos e polling periódico (`cyw43_arch_poll()`).

**Solução:** Implementação de servidor HTTP bare-metal com:
- Callback `http_accept_cb`: aceita conexões TCP na porta 80
- Callback `http_recv_cb`: processa requisições GET/POST, faz parsing manual das URLs e JSON
- Callback `http_sent_cb`: rastreia bytes confirmados (ACKed) e fecha a conexão quando completo
- Suporte a CORS (`Access-Control-Allow-Origin: *`) e OPTIONS (preflight)
- Polling a cada 100 ms no loop principal (19 iterações × 100 ms ≈ 2 segundos)

### 7.3. Interface Web (Frontend)

O dashboard web foi desenvolvido em HTML5/CSS3/JavaScript puro (sem frameworks), totalizando ~818 linhas, com as seguintes funcionalidades:

- **Layout full-width** com CSS Grid responsivo (`grid-template-areas`)
- **4 seções:** Sensores, Relés, Alimentação, Log de Eventos
- **Polling a cada 2 segundos** para atualização em tempo real
- **Síntese de voz (TTS)** via Web Speech API em pt-BR:
  - Ventilador (LN1): "A temperatura está em X graus e a umidade está em Y por cento..."
  - Bomba esvaziar (LN2): "Iniciando TPA, esvaziando o aquário em X%..."
  - Bomba encher (LN3): "Reabastecendo o aquário em X%..."
  - Alimentação manual: "Alimentação manual iniciada, despejando 100 gramas de ração..."
  - Alimentação programada (08:00/16:00): "Hora de alimentar os peixes..."
- **Badge de conexão** indicando Pico W (verde), Backend (amarelo) ou Offline (vermelho)
- **Cooldown de 10 segundos** por mensagem para evitar repetição de voz

### 7.4. Backend Node.js

O backend serve como camada intermediária e fallback:

- **Express.js** na porta 3000 (API) e porta 3001 (frontend estático)
- **Endpoints:** `/health`, `/sensors`, `/relays/status`, `/relays/control`, `/feeding/manual`, `/feeding/status`, `/automation/status`
- **Dados simulados** com variação aleatória quando o Pico W não está acessível
- **Proxy transparente** — o frontend tenta acessar o Pico W diretamente (`10.0.0.181`) e usa o backend como fallback

---

## 8. Descrição dos Módulos, Tarefas, Sensores e Protocolos

### 8.1. Módulos do Firmware

| Módulo | Arquivo | Descrição |
|---|---|---|
| I2C Switching | `hydrosense_v10_final.c` (linhas 113–150) | Alterna i2c1 entre sensores (GPIO 2/3) e OLED (GPIO 14/15) |
| AHT10 Driver | `hydrosense_v10_final.c` (linhas 155–180) | Inicialização e leitura de temperatura/umidade via I2C |
| VL53L0X Driver | `hydrosense_v10_final.c` (linhas 185–215) | Leitura de distância Time-of-Flight a laser |
| OLED SSD1306 | `hydrosense_v10_final.c` (linhas 220–420) | Driver completo: init, buffer 1024 bytes, font 5×7, print, pixel, linhas |
| Servo SG90 | `hydrosense_v10_final.c` (linhas 425–455) | PWM a 50 Hz, ângulos 0–180°, sequência de alimentação |
| LED RGB | `hydrosense_v10_final.c` (linhas 460–475) | Inicialização e controle de 3 GPIOs (R, G, B) |
| Buzzer | `hydrosense_v10_final.c` (linhas 478–488) | GPIO digital, beep com duração configurável |
| Relés | `hydrosense_v10_final.c` (linhas 493–515) | 3 canais GPIO, init e controle individual |
| Display (Telas) | `hydrosense_v10_final.c` (linhas 518–590) | Telas: boot, WiFi connecting, WiFi OK/fail, tela principal |
| Servidor HTTP | `hydrosense_v10_final.c` (linhas 595–835) | TCP raw + lwIP, 5 endpoints, HTML embarcado, CORS |
| Leitura de Sensores | `hydrosense_v10_final.c` (linhas 840–870) | Leitura AHT10 + VL53L0X, cálculo de nível/volume, automação |
| Main | `hydrosense_v10_final.c` (linhas 875–985) | Inicialização sequencial (7 etapas), conexão WiFi, loop principal |

### 8.2. Estrutura do Loop Principal

O projeto possui duas arquiteturas complementares: o **build principal com FreeRTOS** (usando `hydrosense_main.c` + 3 tasks concorrentes) e o **firmware v10 WiFi** (usando `hydrosense_v10_final.c` com polling lwIP). O build principal utiliza o **FreeRTOS** com 3 tasks, mutex, filas e watchdog (ver seção 8.3). O firmware v10 WiFi opera com loop de polling a cada ~2 segundos para compatibilidade com a stack lwIP:

**Build Principal (FreeRTOS — `hydrosense_main.c`):**

```
main() → hydrosense_init() → hydrosense_main_loop()
    ├── watchdog_enable(8000)     → Watchdog 8s
    ├── sensores_ler_todos()      → A cada 5s
    ├── oled_mostrar_tela()       → A cada 1s
    ├── botoes_processar()        → A cada 100ms
    ├── neopixel_show_status()    → A cada 2s
    ├── alimentacao_verificar()   → Contínuo
    └── tpa_verificar()           → Contínuo

    Tasks FreeRTOS (concorrentes):
    ├── monitoring_task  (10s) → Sensores + Alertas
    ├── automation_task  (10s) → Controle nível + TPA
    └── feeding_task     (1s)  → Alimentação + RTC
```

**Firmware v10 WiFi (polling lwIP — `hydrosense_v10_final.c`):**

```
main() {
    [1/7] Init CYW43 (WiFi chip)
    [2/7] Init LED RGB → Vermelho (inicializando)
    [3/7] Init Buzzer → Beep de boot
    [4/7] Init OLED → Tela de boot "HYDROSENSE"
    [5/7] Init Sensores → AHT10 + VL53L0X
    [6/7] Init Servo → Teste 0°→90°→0°
    [7/7] Init Relés → GPIO 17, 18, 19 em LOW
    
    WiFi Connect (WPA2-AES → WPA2-MIXED → WPA-TKIP)
    HTTP Server Start (porta 80)
    
    Loop infinito:
        read_sensors()          → Lê AHT10, VL53L0X, calcula nível/volume
        Automação temperatura   → Liga/desliga ventilador (LN1) se T>29°C
        display_main()          → Atualiza OLED com dados atuais
        printf (debug serial)   → Log USB para diagnóstico
        LED blink               → Pisca verde a cada ciclo
        cyw43_arch_poll() ×19   → Processa rede WiFi (100ms cada)
}
```

### 8.3. FreeRTOS — Configuração e Tasks

O projeto utiliza o **FreeRTOS** como sistema operacional de tempo real, compilado especificamente para o **RP2040** (porta `GCC_RP2040`). O kernel é linkado ao projeto via o `CMakeLists.txt` principal e configurado através do arquivo `FreeRTOSConfig.h`.

#### 8.3.1. Configuração do FreeRTOS (`FreeRTOSConfig.h`)

| Parâmetro | Valor | Descrição |
|---|---|---|
| `configUSE_PREEMPTION` | 1 | Escalonamento preemptivo habilitado |
| `configCPU_CLOCK_HZ` | 133 MHz | Clock do RP2040 |
| `configTICK_RATE_HZ` | 1000 | Resolução de 1 ms por tick |
| `configMAX_PRIORITIES` | 32 | Até 32 níveis de prioridade |
| `configTOTAL_HEAP_SIZE` | 128 KB | Heap dinâmico para alocação de tasks |
| `configUSE_MUTEXES` | 1 | Semáforos mutex para proteção de dados compartilhados |
| `configUSE_COUNTING_SEMAPHORES` | 1 | Semáforos contadores habilitados |
| `configUSE_TIMERS` | 1 | Software timers habilitados |
| `configUSE_RECURSIVE_MUTEXES` | 1 | Mutex recursivos habilitados |
| `configMINIMAL_STACK_SIZE` | 128 words | Stack mínima por task |

#### 8.3.2. Tasks FreeRTOS do Sistema

O HydroSense utiliza **3 tasks FreeRTOS concorrentes**, cada uma com responsabilidade específica, comunicando-se por **semáforos mutex** (`system_data_mutex`) e **filas** (`alert_queue`):

| Task | Arquivo | Período | Função Principal |
|---|---|---|---|
| `monitoring_task` | `src/tasks/monitoring_task.c` | 10 s (`MONITORING_INTERVAL`) | Leitura periódica de sensores, verificação de alertas, atualização de uptime |
| `automation_task` | `src/tasks/automation_task.c` | 10 s (`AUTOMATION_INTERVAL`) | Controle automático de nível de água, execução de TPA, processamento de alertas |
| `feeding_task` | `src/tasks/feeding_task.c` | 1 s | Verificação de horários programados (00:00, 08:00, 16:00), alimentação manual via botão B |

#### 8.3.3. Task de Monitoramento (`monitoring_task`)

```c
void monitoring_task(void *pvParameters) {
    // Inicializa sensores (temp, pH, turbidez, nível)
    // Loop com vTaskDelayUntil (período preciso de 10s):
    //   1. Lê todos os sensores (read_all_sensors)
    //   2. Verifica alertas (check_alerts)
    //   3. Atualiza uptime do sistema
    //   4. Exibe status no console serial
}
```

Esta task utiliza `vTaskDelayUntil()` para garantir periodicidade precisa, independente do tempo de execução das leituras. Protege os dados compartilhados com `xSemaphoreTake(system_data_mutex)` ao acessar a estrutura `SystemStatus_t`.

#### 8.3.4. Task de Automação (`automation_task`)

```c
void automation_task(void *pvParameters) {
    // Inicializa controlador de bombas
    // Loop com vTaskDelayUntil (período de 10s):
    //   1. Controle de nível de água (liga/desliga bomba 2)
    //   2. Verifica necessidade de TPA
    //   3. Processa alertas da fila (xQueueReceive)
    //      - pH fora da faixa → TPA necessária
    //      - Turbidez alta → TPA necessária
    //      - Nível crítico → Alerta
    //      - Temperatura fora da faixa → Alerta
}
```

Esta task consome alertas da `alert_queue` (fila FreeRTOS) produzidos pela task de monitoramento, implementando um padrão **produtor-consumidor** para comunicação inter-task.

#### 8.3.5. Task de Alimentação (`feeding_task`)

```c
void feeding_task(void *pvParameters) {
    // Loop a cada 1 segundo (vTaskDelay):
    //   1. Verifica botão B para alimentação manual
    //   2. Consulta RTC para horários programados
    //   3. Se horário == 00:00, 08:00 ou 16:00:
    //      - Aciona servo (servo_feed_fish)
    //      - Atualiza contadores via mutex
    //      - Registra último horário de alimentação
}
```

Esta task roda com período de **1 segundo** para garantir que nenhum horário programado seja perdido. Utiliza o **RTC (Real-Time Clock)** do RP2040 para comparação de horários e o mutex `system_data_mutex` para atualizar o timestamp da última alimentação de forma thread-safe.

#### 8.3.6. Mecanismos de Sincronização

| Mecanismo | Tipo FreeRTOS | Uso no HydroSense |
|---|---|---|
| `system_data_mutex` | `SemaphoreHandle_t` (Mutex) | Protege acesso concorrente à estrutura `SystemStatus_t` compartilhada entre as 3 tasks |
| `alert_queue` | `QueueHandle_t` (Fila) | Comunicação produtor-consumidor: monitoring gera alertas, automation os consome e atua |
| `vTaskDelayUntil` | Delay preciso | Garante periodicidade exata nas tasks de monitoramento e automação |
| `vTaskDelay` | Delay simples | Usado na feeding_task para verificação a cada 1 segundo |

#### 8.3.7. Diagrama de Concorrência das Tasks

```
┌─────────────────────────────────────────────────────────┐
│                    FreeRTOS Kernel                       │
│              (Preemptivo, 1ms tick, 133MHz)              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │ monitoring   │  │ automation   │  │  feeding     │   │
│  │    task      │  │    task      │  │    task      │   │
│  │  (10s loop)  │  │  (10s loop)  │  │  (1s loop)   │   │
│  │              │  │              │  │              │   │
│  │ • Lê sensores│  │ • Ctrl nível │  │ • Verifica   │   │
│  │ • Gera       │  │ • Executa TPA│  │   horários   │   │
│  │   alertas    │──│ • Consome    │  │ • Servo feed │   │
│  │ • Uptime     │  │   alertas    │  │ • Botão B    │   │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘   │
│         │                 │                 │           │
│         └────────┬────────┘                 │           │
│                  │                          │           │
│         ┌────────▼────────┐        ┌────────▼────────┐  │
│         │  alert_queue    │        │ system_data     │  │
│         │  (Fila)         │        │   _mutex        │  │
│         └─────────────────┘        └─────────────────┘  │
│                                                         │
├─────────────────────────────────────────────────────────┤
│  Loop Principal (hydrosense_main_loop):                  │
│  • Watchdog update (8s)  • Sensores (5s)                │
│  • OLED update (1s)      • Botões (100ms)               │
│  • NeoPixel status (2s)  • Console (30s)                │
└─────────────────────────────────────────────────────────┘
```

#### 8.3.8. Watchdog Timer

O sistema implementa um **hardware watchdog** com timeout de **8 segundos**. O loop principal chama `watchdog_update()` a cada iteração (100 ms). Se o sistema travar por qualquer motivo (deadlock entre tasks, falha de hardware, etc.), o watchdog reinicia automaticamente o microcontrolador, garantindo **alta disponibilidade** do sistema. Na reinicialização, o firmware detecta se foi um reboot por watchdog via `watchdog_caused_reboot()` e emite um alerta.

### 8.4. Sensores — Detalhamento

#### 8.3.1. AHT10 — Sensor de Temperatura e Umidade

| Parâmetro | Valor |
|---|---|
| Interface | I2C, endereço 0x38 |
| Faixa Temperatura | -40°C a +85°C (±0.3°C) |
| Faixa Umidade | 0–100% RH (±2%) |
| Tensão | 3.3V |
| Protocolo | Comando 0xE1 (init), 0xAC (medição), 80ms de espera |
| Cálculo | `temp = raw / 1048576 × 200 - 50`, `hum = raw / 1048576 × 100` |

#### 8.3.2. VL53L0X — Sensor de Distância Time-of-Flight

| Parâmetro | Valor |
|---|---|
| Interface | I2C, endereço 0x29 |
| Faixa | 30–2000 mm |
| Precisão | ±3% |
| Tecnologia | Laser VCSEL 940nm (classe 1 — seguro para os olhos) |
| Cálculo de Nível | `nível(%) = (TANK_HEIGHT - distância) / TANK_HEIGHT × 100` |
| Cálculo de Volume | `volume(L) = nível(%) / 100 × TANK_CAPACITY (20L)` |

### 8.5. Atuadores — Detalhamento

#### 8.4.1. Servo Motor SG90

| Parâmetro | Valor |
|---|---|
| GPIO | 16 (PWM) |
| Frequência PWM | 50 Hz (período 20 ms) |
| Pulso mínimo | 500 µs (0°) |
| Pulso máximo | 2500 µs (180°) |
| Sequência de alimentação | 0° → 180° (1s) → 0° (500ms) → desliga PWM |
| Horários programados | 08:00 e 16:00 (100g de ração) |

#### 8.4.2. Módulo de Relés (3 Canais)

| Canal | GPIO | Equipamento | Automação |
|---|---|---|---|
| LN1 | 17 | Ventilador/Aerador | Automático: liga se T > 29°C, desliga se T ≤ 28°C |
| LN2 | 18 | Bomba TPA — Esvaziar | Manual via interface web |
| LN3 | 19 | Bomba TPA — Encher | Manual via interface web |

### 8.6. Protocolos de Comunicação

| Protocolo | Uso no HydroSense | Detalhes |
|---|---|---|
| **I2C** | Comunicação com sensores e display | i2c1, 100 kHz (sensores), 400 kHz (OLED), switching dinâmico |
| **PWM** | Controle do servo motor | 50 Hz, duty cycle 2.5%–12.5% (500–2500 µs) |
| **WiFi (802.11 b/g/n)** | Conectividade de rede | CYW43439, WPA2, SSID "HydroSense", IP 10.0.0.181 |
| **TCP/IP** | Camada de transporte | lwIP stack, NO_SYS mode, MEM_SIZE 16000 |
| **HTTP** | API REST e página web | Servidor TCP raw na porta 80, 5 endpoints |
| **JSON** | Formato de dados da API | Sensores, relés, status do sistema |
| **GPIO** | Controle de LED, buzzer, relés | Saídas digitais diretas |

### 8.7. Endpoints HTTP do Pico W

| Método | Rota | Descrição | Resposta |
|---|---|---|---|
| GET | `/sensors` | Dados de todos os sensores | JSON com temperatura, umidade, distância, nível, volume, relés, status |
| GET | `/status` | Status do sistema | JSON com versão, WiFi, OLED, sensores, relés, contagem |
| POST | `/relay` | Controlar relés | Body: `{"relay":1,"toggle":true}` ou `{"tipo":"LN1","estado":true}` |
| POST | `/feed` | Acionar alimentação (servo) | JSON com sucesso/mensagem |
| GET | `/` | Página HTML embarcada | HTML responsivo com auto-refresh |

---

## 9. Evidências de Funcionamento

### 9.1. Vídeo Demonstrativo

🎬 **Vídeo completo (5–7 minutos):** [Assistir no Google Drive](https://drive.google.com/file/d/1e51nFBpVCmwORC9yHVQrp4OObZA-HNOL/view?usp=sharing)

O vídeo demonstra:
- Inicialização do sistema (boot com OLED e LED RGB)
- Leitura dos sensores AHT10 e VL53L0X em tempo real
- Display OLED exibindo dados, IP e estado dos relés
- Interface web acessível pelo navegador via WiFi
- Controle dos relés (ventilador, bombas) pela interface web
- Acionamento do servo motor para alimentação
- Síntese de voz (TTS) em português para alertas
- Dashboard com monitoramento e log de eventos em tempo real

### 9.2. Log de Comunicação Serial (USB)

Exemplo de saída do firmware via serial USB (`/dev/ttyACM0`):

```
==========================================
  HydroSense v10 - BitDogLab Completo
==========================================

[1/7] CYW43... OK
[2/7] LED RGB... OK
[3/7] Buzzer... OK
[4/7] OLED... OK
[5/7] Sensores...
  AHT10: OK
  VL53L0X: OK
   VL53L0X ID: 0xEE
[6/7] Servo... OK
[7/7] Reles... OK (GP17,18,19)

[WIFI] Conectando a [HydroSense]...
  WPA2-AES...
[WIFI] IP: 10.0.0.181
[HTTP] http://10.0.0.181/sensors
[HTTP] http://10.0.0.181/ (pagina web)

=== MONITORAMENTO ATIVO ===

#1 T=27.3C H=65.2% D=95mm N=52% V=10.5L R:--- W:OK
#2 T=27.4C H=65.0% D=96mm N=52% V=10.4L R:--- W:OK
#3 T=29.5C H=64.8% D=94mm N=53% V=10.6L R:1-- W:OK
[AUTO] Ventilador ON (T>29)
[RELAY] LN1=ON (GPIO17)
```

### 9.3. Resposta JSON da API `/sensors`

```json
{
  "temperatura": 27.35,
  "umidade": 65.20,
  "distancia": 95,
  "nivel": 52.50,
  "volume": 10.50,
  "wifiStatus": true,
  "contadorLeituras": 1239,
  "deviceIp": "10.0.0.181",
  "relays": { "LN1": false, "LN2": false, "LN3": false },
  "sensores": { "aht10": true, "vl53l0x": true }
}
```

### 9.4. Dashboard Web — Funcionalidades Verificadas

| Funcionalidade | Status |
|---|---|
| Exibição de temperatura em tempo real | ✅ Funcionando |
| Exibição de umidade | ✅ Funcionando |
| Exibição de distância (mm) | ✅ Funcionando |
| Exibição de nível (%) com barra visual | ✅ Funcionando |
| Exibição de volume (litros) | ✅ Funcionando |
| Controle de relé LN1 (Ventilador) | ✅ Funcionando |
| Controle de relé LN2 (Bomba Esvaziar) | ✅ Funcionando |
| Controle de relé LN3 (Bomba Encher) | ✅ Funcionando |
| Estado real dos relés em tempo real | ✅ Funcionando |
| Botão Alimentar Agora (POST /feed) | ✅ Funcionando |
| Alimentação programada 08:00/16:00 | ✅ Funcionando |
| Voz TTS — Ventilador (temp/umidade) | ✅ Funcionando |
| Voz TTS — Bombas TPA (volume/nível) | ✅ Funcionando |
| Voz TTS — Alimentação manual | ✅ Funcionando |
| Voz TTS — Alimentação programada | ✅ Funcionando |
| Log de eventos com timestamps | ✅ Funcionando |
| Badge de conexão (Pico/Backend/Offline) | ✅ Funcionando |
| Layout responsivo (mobile/desktop) | ✅ Funcionando |

---

## 10. Conclusão

O projeto **HydroSense** atingiu com êxito todos os objetivos propostos, resultando em um sistema embarcado IoT funcional, de baixo custo (~R$ 210) e com aplicação prática real no monitoramento e automação de sistemas de aquicultura.

Os principais resultados alcançados foram:

1. **Integração completa de hardware e firmware** — Sensores AHT10 e VL53L0X, display OLED, servo motor, relés, LED RGB e buzzer operando de forma coordenada em um único microcontrolador (RP2040), com **FreeRTOS** gerenciando 3 tasks concorrentes (monitoramento, automação e alimentação) via semáforos mutex e filas para comunicação inter-task.

2. **Conectividade IoT funcional** — Servidor HTTP embarcado no Pico W com API REST JSON, possibilitando monitoramento e controle remotos via rede WiFi.

3. **Interface web acessível e inteligente** — Dashboard responsivo com atualização em tempo real, controle de equipamentos e síntese de voz em português para notificação de eventos críticos, aumentando a acessibilidade do sistema.

4. **Automação efetiva** — Controle automático de ventilação por temperatura, alimentação programada com servo motor e gerenciamento remoto de bombas para TPA.

5. **Superação de desafios técnicos** — A implementação de I2C switching para compartilhamento de periférico, a otimização de memória do lwIP para servir páginas HTML, e o desenvolvimento de servidor HTTP bare-metal demonstram domínio avançado dos conceitos de sistemas embarcados.

Como **trabalhos futuros**, o sistema pode ser expandido com:
- Sensores de pH e oxigênio dissolvido para monitoramento completo da qualidade da água
- Protocolo MQTT para integração com plataformas de nuvem (AWS IoT, ThingsBoard)
- Armazenamento de histórico de dados em cartão SD ou banco de dados remoto
- Sistema de alertas via Telegram/WhatsApp
- Unificação do FreeRTOS com a stack WiFi/lwIP em um único firmware, integrando as tasks concorrentes com o servidor HTTP embarcado

O HydroSense comprova que é possível desenvolver soluções IoT robustas e funcionais para o agronegócio utilizando plataformas de baixo custo e ferramentas open source, contribuindo para a democratização da tecnologia na aquicultura brasileira.

---

## 11. Referências

1. RASPBERRY PI FOUNDATION. **Raspberry Pi Pico W Datasheet**. 2023. Disponível em: https://datasheets.raspberrypi.com/picow/pico-w-datasheet.pdf. Acesso em: jan. 2026.

2. RASPBERRY PI FOUNDATION. **Pico SDK Documentation (v2.2.0)**. 2024. Disponível em: https://www.raspberrypi.com/documentation/pico-sdk/. Acesso em: jan. 2026.

3. AOSONG. **AHT10 - Integrated Temperature and Humidity Sensor Datasheet**. 2019. Disponível em: https://server4.eca.ir/eshop/AHT10/Aosong_AHT10_en_draft_0c.pdf. Acesso em: jan. 2026.

4. STMICROELECTRONICS. **VL53L0X - World's Smallest Time-of-Flight Ranging and Gesture Detection Sensor Datasheet**. 2021. Disponível em: https://www.st.com/resource/en/datasheet/vl53l0x.pdf. Acesso em: jan. 2026.

5. SOLOMON SYSTECH. **SSD1306 - 128×64 Dot Matrix OLED/PLED Segment/Common Driver with Controller**. 2008. Disponível em: https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf. Acesso em: jan. 2026.

6. TOWER PRO. **SG90 9g Micro Servo Datasheet**. Disponível em: http://www.towerpro.com.tw/product/sg90-7/. Acesso em: jan. 2026.

7. DUNKELS, A. **lwIP - A Lightweight TCP/IP Stack**. Swedish Institute of Computer Science. Disponível em: https://savannah.nongnu.org/projects/lwip/. Acesso em: jan. 2026.

8. INFINEON TECHNOLOGIES. **CYW43439 Single-Chip IEEE 802.11 b/g/n MAC/Baseband/Radio**. 2022. Disponível em: https://www.infineon.com/cms/en/product/wireless-connectivity/airoc-wi-fi-plus-bluetooth-combos/wi-fi-4-702.11n/cyw43439/. Acesso em: jan. 2026.

9. MDN WEB DOCS. **Web Speech API - SpeechSynthesis**. Mozilla Foundation. Disponível em: https://developer.mozilla.org/en-US/docs/Web/API/SpeechSynthesis. Acesso em: jan. 2026.

10. PEIXE BR — Associação Brasileira de Piscicultura. **Anuário Peixe BR 2024**. São Paulo, 2024. Disponível em: https://www.peixebr.com.br/anuario-2024/. Acesso em: jan. 2026.

11. EXPRESSJS. **Express - Node.js Web Application Framework**. Disponível em: https://expressjs.com/. Acesso em: jan. 2026.

---

## 12. Apêndices

### Apêndice A — Código-Fonte (GitHub)

**Repositório completo:** [https://github.com/hobsonbreno/HydroSense---Sistema-de-Aquicultura-Inteligente](https://github.com/hobsonbreno/HydroSense---Sistema-de-Aquicultura-Inteligente)

#### Estrutura principal do repositório:

```
HydroSense/
├── src/
│   ├── hydrosense_v10_final.c    # Firmware WiFi final (985 linhas)
│   ├── hydrosense_main.c         # Main com FreeRTOS (307 linhas)
│   ├── hydrosense_servo.c        # Driver servo motor
│   ├── hydrosense_oled.c         # Driver display OLED
│   ├── hydrosense_utils.c        # Funções utilitárias
│   ├── hydrosense_botoes.c       # Processamento de botões
│   ├── config/
│   │   └── system_config.c       # Configuração do sistema
│   └── tasks/
│       ├── feeding_task.c        # Task FreeRTOS — Alimentação (RTC + servo)
│       ├── monitoring_task.c     # Task FreeRTOS — Monitoramento de sensores
│       └── automation_task.c     # Task FreeRTOS — Automação (TPA + nível)
├── include/
│   ├── hydrosense_system.h       # Defines, structs, estados do sistema
│   ├── lwipopts.h                # Configuração lwIP otimizada
│   ├── tasks/
│   │   ├── feeding_task.h
│   │   ├── monitoring_task.h
│   │   └── automation_task.h
│   └── sensors/
│       ├── aht10.h               # Driver sensor temp/umidade
│       └── vl53l0x.h             # Driver sensor distância
├── FreeRTOS-Kernel/              # Kernel FreeRTOS (porta GCC_RP2040)
├── frontend/
│   └── index.html                # Dashboard web completo (818 linhas)
├── simple-backend.js             # Backend Node.js Express (217 linhas)
├── CMakeLists.txt                # Build system CMake (c/ FreeRTOS)
├── FreeRTOSConfig.h              # Configuração FreeRTOS (128KB heap, preemptivo)
├── pico_sdk_import.cmake         # Importação Pico SDK
├── start.sh                      # Script inicialização
├── stop.sh                       # Script encerramento limpo
└── README.md                     # Documentação do projeto
```

### Apêndice B — Esquemático de Conexões

```
                        RASPBERRY PI PICO W (BitDogLab)
                     ┌──────────────────────────────────┐
                     │                                  │
     AHT10 ─────────┤ GPIO 2 (SDA) ◄── I2C1 ──► GPIO 14 (SDA) ├── OLED SSD1306
     VL53L0X ────────┤ GPIO 3 (SCL) ◄── I2C1 ──► GPIO 15 (SCL) ├── OLED SSD1306
                     │        (100kHz)        (400kHz)  │
                     │                                  │
     Servo SG90 ─────┤ GPIO 16 (PWM, 50Hz)              │
                     │                                  │
     Relé LN1 ───────┤ GPIO 17 ──► Ventilador           │
     Relé LN2 ───────┤ GPIO 18 ──► Bomba Esvaziar       │
     Relé LN3 ───────┤ GPIO 19 ──► Bomba Encher         │
                     │                                  │
     LED R ──────────┤ GPIO 12                          │
     LED G ──────────┤ GPIO 13                          │
     LED B ──────────┤ GPIO 11                          │
                     │                                  │
     Buzzer ─────────┤ GPIO 21                          │
     Botão A ────────┤ GPIO 5                           │
     Botão B ────────┤ GPIO 6                           │
                     │                                  │
     WiFi ───────────┤ CYW43439 (integrado)             │
                     └──────────────────────────────────┘

     Alimentação:
     ├── USB 5V → Pico W → 3.3V regulado
     ├── Módulo Relé: VCC=3.3V ou VBUS(5V), GND=GND
     └── Servo SG90: VCC=5V (VBUS), GND=GND, Signal=GPIO16
```

### Apêndice C — Vídeo Demonstrativo

🎬 **Link do vídeo (5–7 minutos):**  
[https://drive.google.com/file/d/1e51nFBpVCmwORC9yHVQrp4OObZA-HNOL/view?usp=sharing](https://drive.google.com/file/d/1e51nFBpVCmwORC9yHVQrp4OObZA-HNOL/view?usp=sharing)

### Apêndice D — Configuração lwIP (`lwipopts.h`)

| Parâmetro | Valor | Justificativa |
|---|---|---|
| `NO_SYS` | 1 | Modo polling (sem RTOS na camada de rede) |
| `MEM_SIZE` | 16000 | Necessário para buffers de envio HTTP (página HTML) |
| `TCP_MSS` | 1460 | Tamanho máximo de segmento TCP padrão |
| `TCP_SND_BUF` | 8 × TCP_MSS (11680) | Buffer de envio ampliado para página HTML |
| `PBUF_POOL_SIZE` | 24 | Pool de buffers de pacotes |
| `MEMP_NUM_TCP_SEG` | 32 | Segmentos TCP simultâneos |

### Apêndice E — Scripts de Operação

**Iniciar o sistema:**
```bash
cd HydroSense && ./start.sh
```

**Parar o sistema:**
```bash
cd HydroSense && ./stop.sh
```

**Compilar o firmware:**
```bash
cd build_v10 && make hydrosense_v10 -j$(nproc)
```

**Gravar no Pico W:**
```bash
picotool load -fx hydrosense_v10.uf2
```
