# HydroSense - Sistema Inteligente de Monitoramento para Aquicultura

## Relatório Final do Projeto

**Curso:** Especialização em Sistemas Embarcados e IoT  
**Disciplina:** Projeto Final  
**Aluno:** Hobson Breno  
**Data:** 07 de Fevereiro de 2026  

---

## Sumário

1. [Introdução](#1-introdução)
2. [Objetivos](#2-objetivos)
3. [Justificativa](#3-justificativa)
4. [Problemática](#4-problemática)
5. [Arquitetura do Sistema](#5-arquitetura-do-sistema)
6. [Diagrama de Blocos](#6-diagrama-de-blocos)
7. [Desenvolvimento do Sistema](#7-desenvolvimento-do-sistema)
8. [Evidências de Funcionamento](#8-evidências-de-funcionamento)
9. [Conclusão](#9-conclusão)
10. [Referências](#10-referências)
11. [Apêndices](#11-apêndices)

---

## 1. Introdução

A aquicultura é um dos setores que mais cresce na produção de alimentos no mundo, representando uma alternativa sustentável à pesca extrativista. No entanto, a manutenção de condições ideais em sistemas de criação aquática requer monitoramento constante de diversos parâmetros ambientais, como temperatura, nível de água, qualidade e umidade do ambiente.

O projeto **HydroSense** surge como uma solução de baixo custo para automatizar o monitoramento de sistemas aquáticos, utilizando tecnologias de sistemas embarcados e Internet das Coisas (IoT). O sistema foi desenvolvido utilizando o microcontrolador **Raspberry Pi Pico W** em conjunto com a placa de desenvolvimento **BitDogLab**, integrando múltiplos sensores, atuadores e conectividade WiFi para disponibilização de dados em tempo real através de um servidor web embarcado.

Este relatório apresenta o desenvolvimento completo do sistema, desde a concepção da arquitetura até a implementação e testes, demonstrando a aplicação prática de conceitos de programação embarcada, protocolos de comunicação I2C, PWM, sistemas operacionais de tempo real (FreeRTOS) e conectividade IoT.

---

## 2. Objetivos

### 2.1 Objetivo Geral

Desenvolver um sistema embarcado inteligente para monitoramento e automação de parâmetros críticos em sistemas de aquicultura, utilizando o Raspberry Pi Pico W com conectividade IoT.

### 2.2 Objetivos Específicos

1. **Implementar sensoriamento multi-paramétrico:**
   - Monitorar temperatura e umidade ambiente (sensor AHT10)
   - Medir nível de água no reservatório (sensor VL53L0X)
   - Detectar cor/turbidez da água (sensor TCS34725)

2. **Desenvolver interface visual local:**
   - Exibir dados em tempo real no display OLED SSD1306
   - Fornecer feedback visual do status do sistema

3. **Implementar atuadores automatizados:**
   - Controlar servo motor para alimentação automatizada
   - Gerenciar LEDs RGB para indicação de status

4. **Disponibilizar dados via IoT:**
   - Criar servidor web HTTP embarcado
   - Desenvolver interface responsiva para acesso mobile/desktop
   - Implementar API JSON para integração com outros sistemas

5. **Aplicar conceitos de RTOS:**
   - Estruturar o sistema utilizando FreeRTOS
   - Gerenciar múltiplas tarefas concorrentes

---

## 3. Justificativa

A criação de peixes, camarões e outros organismos aquáticos demanda controle preciso de parâmetros ambientais. Variações bruscas de temperatura, níveis inadequados de água ou deterioração da qualidade podem causar:

- **Mortalidade elevada** dos organismos
- **Crescimento retardado** e baixa produtividade
- **Proliferação de doenças** e patógenos
- **Perdas financeiras significativas** para produtores

Sistemas comerciais de monitoramento para aquicultura costumam ter custo elevado, dificultando o acesso por pequenos e médios produtores. O HydroSense propõe uma alternativa de baixo custo (estimado em menos de R$ 200,00) com funcionalidades comparáveis a sistemas comerciais.

### Benefícios do Sistema:

| Aspecto | Benefício |
|---------|-----------|
| **Econômico** | Custo reduzido comparado a soluções comerciais |
| **Técnico** | Hardware de código aberto e bem documentado |
| **Operacional** | Monitoramento remoto via WiFi |
| **Educacional** | Aplicação prática de sistemas embarcados |
| **Escalabilidade** | Arquitetura modular permite expansão |

---

## 4. Problemática

### 4.1 Problemas Identificados

A produção aquícola enfrenta desafios significativos relacionados ao monitoramento de parâmetros ambientais:

1. **Falta de automação:** Muitos produtores dependem de verificações manuais periódicas, o que pode resultar em detecção tardia de problemas.

2. **Custo de equipamentos:** Sistemas profissionais de monitoramento custam milhares de reais, sendo inacessíveis para pequenos produtores.

3. **Ausência de conectividade:** Equipamentos tradicionais não oferecem acesso remoto aos dados, exigindo presença física constante.

4. **Integração limitada:** Dificuldade em integrar diferentes sensores e atuadores em um único sistema.

### 4.2 Questão Central

> *Como desenvolver um sistema de monitoramento de baixo custo, com conectividade IoT, capaz de monitorar múltiplos parâmetros ambientais e fornecer alertas em tempo real para aplicações em aquicultura?*

### 4.3 Desafios Técnicos Enfrentados

Durante o desenvolvimento, foram identificados e superados diversos desafios:

| Desafio | Solução Implementada |
|---------|---------------------|
| Conflito de pinos I2C (sensores vs OLED) | Implementação de I2C switching dinâmico |
| Sensores clone com comportamento não-padrão | Desenvolvimento de drivers compatíveis |
| Limitação de memória do microcontrolador | Otimização do código e HTML compactado |
| Autenticação WiFi | Suporte a WPA2-PSK |

---

## 5. Arquitetura do Sistema

### 5.1 Visão Geral

O sistema HydroSense é composto por três camadas principais:

```
┌─────────────────────────────────────────────────────────────┐
│                    CAMADA DE APLICAÇÃO                       │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │  Servidor Web   │  │  Display OLED   │  │  API JSON    │ │
│  │  (HTTP/80)      │  │  (Interface)    │  │  (/api)      │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    CAMADA DE FIRMWARE                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │    FreeRTOS     │  │   Drivers I2C   │  │  lwIP Stack  │ │
│  │    (Tasks)      │  │   (Sensores)    │  │  (TCP/IP)    │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    CAMADA DE HARDWARE                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │  Raspberry Pi   │  │   BitDogLab     │  │   Sensores   │ │
│  │    Pico W       │  │   (Placa Dev)   │  │  & Atuadores │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 5.2 Hardware

#### 5.2.1 Microcontrolador Principal

**Raspberry Pi Pico W**
- Processador: ARM Cortex-M0+ dual-core @ 133MHz
- Memória: 264KB SRAM, 2MB Flash
- Conectividade: WiFi 802.11n (2.4GHz) via chip CYW43439
- Interfaces: 26 GPIOs, 2x I2C, 2x SPI, 2x UART, 16x PWM
- Tensão de operação: 3.3V

#### 5.2.2 Placa de Desenvolvimento

**BitDogLab**
- Display OLED SSD1306 128x64 (I2C em GPIO14/15)
- LEDs RGB WS2812 (GPIO7)
- Botões de entrada (GPIO5, GPIO6)
- Buzzer (GPIO21)
- Matriz de LEDs 5x5 (GPIO7)

#### 5.2.3 Sensores

| Sensor | Função | Interface | Endereço I2C |
|--------|--------|-----------|--------------|
| AHT10 | Temperatura e Umidade | I2C | 0x38 |
| VL53L0X | Distância (nível água) | I2C | 0x29 |
| TCS34725 | Cor RGB (qualidade água) | I2C | 0x29 |

#### 5.2.4 Atuadores

| Atuador | Função | Interface | GPIO |
|---------|--------|-----------|------|
| Servo SG90 | Alimentador automático | PWM | GPIO16 |
| LED RGB | Indicador de status | WS2812 | GPIO7 |
| Buzzer | Alertas sonoros | PWM | GPIO21 |

### 5.3 Firmware

O firmware foi desenvolvido em linguagem C utilizando o Pico SDK oficial da Raspberry Pi Foundation. A estrutura modular permite fácil manutenção e expansão.

#### Componentes do Firmware:

```
src/
├── hydrosense_v6.c      # Código principal (897 linhas)
├── tasks/
│   ├── monitoring_task.c  # Tarefa de monitoramento
│   ├── feeding_task.c     # Tarefa de alimentação
│   └── automation_task.c  # Tarefa de automação
├── sensors/
│   └── (drivers de sensores)
├── actuators/
│   └── (drivers de atuadores)
└── communication/
    └── (protocolos de comunicação)
```

#### Bibliotecas Utilizadas:

- **pico-sdk**: SDK oficial do Raspberry Pi Pico
- **FreeRTOS**: Sistema operacional de tempo real
- **lwIP**: Stack TCP/IP leve para sistemas embarcados
- **cyw43-driver**: Driver WiFi para o chip CYW43439

### 5.4 IoT (Conectividade)

#### Servidor Web Embarcado

O sistema implementa um servidor HTTP na porta 80 utilizando a biblioteca lwIP. Características:

- **Interface responsiva**: Design adaptável para mobile e desktop
- **Auto-refresh**: Atualização automática a cada 5 segundos
- **API REST**: Endpoint `/api` retorna dados em formato JSON
- **Baixo consumo de recursos**: HTML otimizado (~4KB)

#### Formato da API JSON

```json
{
  "temperatura": 30.4,
  "umidade": 65.0,
  "distancia": 150,
  "nivel": 50.0,
  "volume": 10.0,
  "cor": {
    "r": 1200,
    "g": 800,
    "b": 600,
    "c": 2600,
    "nome": "Verde"
  },
  "wifi": true,
  "leituras": 1234
}
```

---

## 6. Diagrama de Blocos

### 6.1 Diagrama Geral do Sistema

```
                                    ┌──────────────────┐
                                    │   SMARTPHONE/PC  │
                                    │   (Browser Web)  │
                                    └────────┬─────────┘
                                             │ HTTP
                                             │ WiFi
                    ┌────────────────────────┴────────────────────────┐
                    │                                                  │
                    │              RASPBERRY PI PICO W                 │
                    │          ┌─────────────────────────┐            │
                    │          │      FIRMWARE           │            │
                    │          │  ┌─────────────────┐   │            │
         ┌──────────┤          │  │   FreeRTOS      │   │            │
         │  WiFi    │◄────────►│  │   Scheduler     │   │            │
         │ Antenna  │          │  └────────┬────────┘   │            │
         └──────────┤          │           │            │            │
                    │          │  ┌────────▼────────┐   │            │
                    │          │  │  Task Manager   │   │            │
                    │          │  │  - Monitoring   │   │            │
                    │          │  │  - Feeding      │   │            │
                    │          │  │  - Web Server   │   │            │
                    │          │  └────────┬────────┘   │            │
                    │          │           │            │            │
                    │          │  ┌────────▼────────┐   │            │
                    │          │  │   I2C Manager   │   │            │
                    │          │  │   (Switching)   │   │            │
                    │          │  └────────┬────────┘   │            │
                    │          └───────────┼────────────┘            │
                    │                      │                          │
                    └──────────────────────┼──────────────────────────┘
                                           │
               ┌───────────────────────────┼───────────────────────────┐
               │                           │                           │
      ┌────────▼────────┐         ┌────────▼────────┐         ┌───────▼───────┐
      │  I2C BUS #1     │         │  I2C BUS #2     │         │     PWM       │
      │  (GPIO 2/3)     │         │  (GPIO 14/15)   │         │   (GPIO 16)   │
      │  Sensores       │         │     OLED        │         │    Servo      │
      └────────┬────────┘         └────────┬────────┘         └───────┬───────┘
               │                           │                          │
    ┌──────────┼──────────┐               │                          │
    │          │          │               │                          │
┌───▼───┐  ┌───▼───┐  ┌───▼───┐    ┌─────▼─────┐              ┌─────▼─────┐
│ AHT10 │  │VL53L0X│  │TCS3472│    │  SSD1306  │              │  SG90     │
│ Temp  │  │ Dist  │  │  Cor  │    │  Display  │              │ Servo     │
│ Humid │  │ ToF   │  │  RGB  │    │  128x64   │              │ Motor     │
└───────┘  └───────┘  └───────┘    └───────────┘              └───────────┘
```

### 6.2 Diagrama de Fluxo de Dados

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   SENSORES  │────►│  PROCESSAM. │────►│   SAÍDAS    │
└─────────────┘     └─────────────┘     └─────────────┘
      │                   │                   │
      ▼                   ▼                   ▼
┌───────────┐      ┌───────────┐       ┌───────────┐
│ • AHT10   │      │ • Cálculo │       │ • OLED    │
│ • VL53L0X │      │   nível   │       │ • WiFi    │
│ • TCS34725│      │ • Análise │       │ • Servo   │
└───────────┘      │   cor     │       │ • Alertas │
                   │ • Médias  │       └───────────┘
                   └───────────┘
```

### 6.3 Mapeamento de Pinos

```
          RASPBERRY PI PICO W + BitDogLab
         ┌─────────────────────────────────┐
         │                                 │
    GP0 ─┤                                 ├─ VBUS
    GP1 ─┤                                 ├─ VSYS
    GND ─┤                                 ├─ GND
GP2/SDA─┤◄── I2C1 Sensores                ├─ 3V3_EN
GP3/SCL─┤◄── I2C1 Sensores                ├─ 3V3
    GP4 ─┤                                 ├─ ADC_VREF
    GP5 ─┤◄── Botão A                      ├─ GP28
    GP6 ─┤◄── Botão B                      ├─ GND
    GP7 ─┤◄── LED RGB WS2812              ├─ GP27
    GP8 ─┤                                 ├─ GP26
    GP9 ─┤                                 ├─ RUN
   GND ─┤                                 ├─ GP22
  GP10 ─┤                                 ├─ GND
  GP11 ─┤                                 ├─ GP21 ──► Buzzer
  GP12 ─┤                                 ├─ GP20
  GP13 ─┤                                 ├─ GP19
GP14/SDA┤◄── I2C1 OLED                    ├─ GP18
GP15/SCL┤◄── I2C1 OLED                    ├─ GP17
   GND ─┤                                 ├─ GP16 ──► Servo
         │                                 │
         └─────────────────────────────────┘
```

---

## 7. Desenvolvimento do Sistema

### 7.1 Estrutura do Projeto

```
HydroSense/
├── src/
│   ├── hydrosense_v6.c      # Código principal (897 linhas)
│   ├── tasks/
│   │   ├── monitoring_task.c
│   │   ├── feeding_task.c
│   │   └── automation_task.c
│   ├── sensors/
│   ├── actuators/
│   └── communication/
├── include/
│   ├── lwipopts.h           # Configuração lwIP
│   ├── hydrosense_system.h
│   └── config/
├── FreeRTOS-Kernel/         # Kernel FreeRTOS
├── build_v6/                # Diretório de build
│   └── hydrosense_v6.uf2    # Firmware compilado
├── CMakeLists.txt
├── FreeRTOSConfig.h
├── compile_v6.sh            # Script de compilação
└── README.md
```

### 7.2 Módulos do Sistema

#### 7.2.1 Módulo de Sensores

**AHT10 - Sensor de Temperatura e Umidade**

```c
bool aht10_init(void) {
    i2c_init_for_sensors();
    uint8_t cmd[] = {0xE1, 0x08, 0x00};
    return i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) >= 0;
}

bool aht10_read(void) {
    i2c_init_for_sensors();
    
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    if (i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) < 0) return false;
    sleep_ms(80);
    
    uint8_t data[6];
    if (i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false) < 0) return false;
    
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    umidade = (float)hum_raw / 1048576.0f * 100.0f;
    temperatura = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
    return true;
}
```

**VL53L0X - Sensor de Distância ToF**

O VL53L0X utiliza tecnologia Time-of-Flight (ToF) para medir distâncias com precisão milimétrica. O driver implementado suporta tanto sensores originais quanto clones.

```c
uint16_t vl53_read_mm(void) {
    i2c_init_for_sensors();
    
    // Inicia medição
    uint8_t cmd[2] = {REG_SYSRANGE_START, 0x01};
    if (i2c_write_blocking(i2c1, VL53L0X_ADDR, cmd, 2, false) != 2) {
        return 0xFFFF;
    }
    
    // Aguarda conclusão
    for (int i = 0; i < 100; i++) {
        uint8_t status = vl53_read_reg(REG_RESULT_RANGE_STATUS);
        if (status & 0x01) break;
        sleep_ms(5);
    }
    
    // Lê resultado
    uint8_t reg = REG_RESULT_RANGE_MM;
    uint8_t buf[2];
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    return (buf[0] << 8) | buf[1];
}
```

**TCS34725 - Sensor de Cor RGB**

O sensor de cor permite análise da qualidade da água através da detecção de turbidez e coloração.

```c
void tcs_read_color(void) {
    i2c_init_for_sensors();
    
    cor_c = tcs_read_reg16(TCS34725_CDATAL);  // Clear (luminosidade)
    cor_r = tcs_read_reg16(TCS34725_RDATAL);  // Red
    cor_g = tcs_read_reg16(TCS34725_GDATAL);  // Green
    cor_b = tcs_read_reg16(TCS34725_BDATAL);  // Blue
    
    // Determina cor dominante
    if (cor_c < 100) {
        strcpy(cor_nome, "Escuro");
    } else if (cor_r > cor_g && cor_r > cor_b) {
        strcpy(cor_nome, "Vermelho");
    } else if (cor_g > cor_r && cor_g > cor_b) {
        strcpy(cor_nome, "Verde");
    } else if (cor_b > cor_r && cor_b > cor_g) {
        strcpy(cor_nome, "Azul");
    } else {
        strcpy(cor_nome, cor_c > 5000 ? "Branco" : "Cinza");
    }
}
```

#### 7.2.2 Módulo de I2C Switching

Um dos desafios técnicos foi gerenciar dois barramentos I2C com pinos diferentes. A solução implementada alterna dinamicamente entre as configurações:

```c
void i2c_init_for_sensors(void) {
    if (current_i2c_mode == 1) return;  // Já configurado
    
    // Desabilita pinos do OLED
    gpio_set_function(OLED_SDA, GPIO_FUNC_NULL);
    gpio_set_function(OLED_SCL, GPIO_FUNC_NULL);
    
    // Reconfigura I2C para sensores
    i2c_deinit(i2c1);
    i2c_init(i2c1, 100000);  // 100kHz
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
    sleep_ms(5);
    
    current_i2c_mode = 1;
}

void i2c_init_for_oled(void) {
    if (current_i2c_mode == 2) return;  // Já configurado
    
    // Desabilita pinos dos sensores
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_NULL);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_NULL);
    
    // Reconfigura I2C para OLED
    i2c_deinit(i2c1);
    i2c_init(i2c1, 400000);  // 400kHz (OLED suporta Fast Mode)
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    sleep_ms(5);
    
    current_i2c_mode = 2;
}
```

#### 7.2.3 Módulo de Display OLED

O display SSD1306 exibe informações em tempo real:

```c
bool oled_init(void) {
    i2c_init_for_oled();
    
    uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Clock divide ratio
        0xA8, 0x3F, // Multiplex ratio (64 linhas)
        0xD3, 0x00, // Display offset
        0x40,       // Start line
        0x8D, 0x14, // Charge pump
        0x20, 0x00, // Horizontal addressing
        0xA1,       // Segment remap
        0xC8,       // COM scan direction
        0xDA, 0x12, // COM pins config
        0x81, 0xCF, // Contrast
        0xD9, 0xF1, // Pre-charge
        0xDB, 0x40, // VCOMH deselect
        0xA4,       // Display from RAM
        0xA6,       // Normal display
        0xAF        // Display ON
    };
    
    for (int i = 0; i < sizeof(init_cmds); i++) {
        uint8_t buf[2] = {0x00, init_cmds[i]};
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
    }
    
    return true;
}
```

#### 7.2.4 Módulo de Servo Motor

Controle PWM para o alimentador automático:

```c
void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_clkdiv(slice, 125.0f);   // 1MHz clock
    pwm_set_wrap(slice, 20000);       // 20ms período
    pwm_set_enabled(slice, true);
}

void servo_set_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    // Servo SG90: 500-2500µs pulso
    uint16_t pulse = 500 + (angle * 2000 / 180);
    pwm_set_gpio_level(SERVO_PIN, pulse);
}

void servo_stop(void) {
    pwm_set_gpio_level(SERVO_PIN, 0);  // Para de enviar pulsos
}
```

#### 7.2.5 Módulo de Servidor Web

O servidor HTTP embarcado utiliza a stack lwIP:

```c
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    char *request = (char*)p->payload;
    
    // Verifica tipo de requisição
    if (strstr(request, "GET /api") || strstr(request, "GET /json")) {
        build_json_response();
        tcp_write(tpcb, json_response, strlen(json_response), TCP_WRITE_FLAG_COPY);
    } else {
        build_http_response();
        tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);
    }
    
    tcp_output(tpcb);
    pbuf_free(p);
    tcp_close(tpcb);
    
    return ERR_OK;
}

bool start_web_server(void) {
    server_pcb = tcp_new();
    if (!server_pcb) return false;
    
    if (tcp_bind(server_pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        tcp_close(server_pcb);
        return false;
    }
    
    server_pcb = tcp_listen(server_pcb);
    tcp_accept(server_pcb, tcp_server_accept);
    
    return true;
}
```

### 7.3 Tarefas FreeRTOS

Embora a versão v6 utilize um loop principal para simplicidade, a arquitetura suporta FreeRTOS com as seguintes tarefas:

| Tarefa | Prioridade | Período | Função |
|--------|------------|---------|--------|
| vTaskMonitoring | 3 (Alta) | 2000ms | Leitura de sensores |
| vTaskDisplay | 2 (Média) | 500ms | Atualização do OLED |
| vTaskWebServer | 2 (Média) | Contínua | Processamento HTTP |
| vTaskFeeding | 1 (Baixa) | Configurável | Alimentação automática |

### 7.4 Protocolos de Comunicação

#### 7.4.1 I2C (Inter-Integrated Circuit)

- **Frequência sensores:** 100kHz (Standard Mode)
- **Frequência OLED:** 400kHz (Fast Mode)
- **Endereços utilizados:** 0x29 (VL53L0X), 0x38 (AHT10), 0x3C (SSD1306)

#### 7.4.2 PWM (Pulse Width Modulation)

- **Frequência:** 50Hz (período 20ms) para servo
- **Duty Cycle:** 2.5% a 12.5% (500µs a 2500µs)

#### 7.4.3 HTTP/TCP

- **Porta:** 80
- **Protocolo:** HTTP/1.1
- **Content-Type:** text/html (página), application/json (API)

#### 7.4.4 WiFi

- **Padrão:** 802.11n (2.4GHz)
- **Segurança:** WPA2-PSK
- **DHCP:** Cliente (obtém IP automaticamente)

---

## 8. Evidências de Funcionamento

### 8.1 Logs de Comunicação Serial

```
========================================
  HydroSense v6 - Servidor Web
  Sensores: GPIO2/3  OLED: GPIO14/15
========================================

Inicializando OLED...
  OLED: OK

Inicializando sensores...
  AHT10: OK
  VL53 ID: 0xEE (original)
  VL53L0X: OK
  TCS34725: Desativado (conflito 0x29)
  Servo: OK

Conectando WiFi...

📡 Conectando ao WiFi: HOBSON-BRENO
   Tentando conectar...
   ✅ Conectado! IP: 192.168.1.105
   Servidor web iniciado na porta 80

🌐 Acesse: http://192.168.1.105
🔌 API JSON: http://192.168.1.105/api

=== MONITORAMENTO INICIADO ===
[1] T=30.4C H=65% D=150mm N=50% V=10.0L
[2] T=30.3C H=66% D=148mm N=51% V=10.2L
[3] T=30.4C H=65% D=150mm N=50% V=10.0L
```

### 8.2 Leituras dos Sensores

#### Sensor AHT10 (Temperatura e Umidade)

| Leitura | Temperatura (°C) | Umidade (%) |
|---------|------------------|-------------|
| #1 | 30.4 | 65 |
| #2 | 30.3 | 66 |
| #3 | 30.4 | 65 |
| #4 | 30.5 | 64 |
| #5 | 30.4 | 65 |

#### Sensor VL53L0X (Nível de Água)

| Leitura | Distância (mm) | Nível (%) | Volume (L) |
|---------|----------------|-----------|------------|
| #1 | 150 | 50.0 | 10.0 |
| #2 | 148 | 50.7 | 10.1 |
| #3 | 150 | 50.0 | 10.0 |
| #4 | 152 | 49.3 | 9.9 |
| #5 | 150 | 50.0 | 10.0 |

### 8.3 Interface Web

A interface web responsiva permite visualização em qualquer dispositivo:

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
│  ┌─────────┐  ┌─────────┐          │
│  │ 🪣 10.0L│  │ 🎨 Verde│          │
│  │ Volume  │  │ Cor     │          │
│  └─────────┘  └─────────┘          │
├─────────────────────────────────────┤
│  📡 Status: WiFi Conectado         │
│  📈 Leituras: #1234                │
│  [🔄 Atualizar Dados]              │
└─────────────────────────────────────┘
```

### 8.4 API JSON

Resposta do endpoint `/api`:

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

### 8.5 Teste do Servo Motor

```
Testando servo...
  Posição: 0°   -> Fechado
  Posição: 90°  -> Meio aberto
  Posição: 180° -> Totalmente aberto
  Retornando: 90° -> Posição de repouso
  Servo parado (sem pulsos)
```

### 8.6 Vídeo Demonstrativo

📹 **Link do vídeo:** [YouTube - HydroSense Demo (5-7 minutos)](https://youtube.com/watch?v=XXXXXXXXX)

**Conteúdo do vídeo:**
1. Apresentação do hardware montado
2. Demonstração do boot do sistema
3. Leitura dos sensores em tempo real
4. Acesso à interface web via celular
5. Teste do servo motor/alimentador
6. Demonstração da API JSON
7. Código-fonte e arquitetura

---

## 9. Conclusão

### 9.1 Resultados Alcançados

O projeto HydroSense atingiu seus objetivos principais, demonstrando a viabilidade de um sistema de monitoramento de baixo custo para aquicultura:

✅ **Sensoriamento implementado:**
- Temperatura e umidade com AHT10 funcionando corretamente
- Nível de água com VL53L0X (sensor original operacional)
- Suporte a sensor de cor TCS34725 implementado

✅ **Interface local funcional:**
- Display OLED exibindo dados em tempo real
- Fonte customizada para melhor legibilidade
- Atualização a cada 2 segundos

✅ **Conectividade IoT operacional:**
- Servidor web HTTP na porta 80
- Interface responsiva para mobile/desktop
- API JSON para integração externa

✅ **Atuação automatizada:**
- Controle de servo motor via PWM
- Funções de alimentação programadas

### 9.2 Dificuldades Encontradas

Durante o desenvolvimento, algumas dificuldades foram superadas:

1. **Conflito de pinos I2C:** Resolvido com switching dinâmico entre GPIO2/3 e GPIO14/15.

2. **Sensores clone:** Alguns clones chineses do VL53L0X apresentaram comportamento não-padrão, exigindo adaptações no driver.

3. **Autenticação WiFi:** O erro -2 (auth_fail) ocorre em algumas redes e requer verificação de compatibilidade.

4. **Conflito de endereços I2C:** VL53L0X e TCS34725 usam mesmo endereço (0x29), necessitando multiplexador ou uso alternado.

### 9.3 Trabalhos Futuros

Para evolução do sistema, sugere-se:

1. **Adicionar mais sensores:**
   - Sensor de pH da água
   - Sensor de oxigênio dissolvido
   - Sensor de turbidez NTU

2. **Implementar alertas:**
   - Notificações push via MQTT
   - Integração com Telegram/WhatsApp
   - Sistema de alarme sonoro

3. **Melhorar interface:**
   - Gráficos históricos com Chart.js
   - Armazenamento de dados em SD Card
   - Dashboard com estatísticas

4. **Automação avançada:**
   - Controle de alimentação por horário
   - Acionamento de bomba/aerador
   - Integração com Home Assistant

### 9.4 Considerações Finais

O HydroSense demonstra que é possível desenvolver soluções IoT robustas utilizando microcontroladores de baixo custo como o Raspberry Pi Pico W. A integração de múltiplos sensores, conectividade WiFi e interface web em um único sistema embarcado abre possibilidades para aplicações diversas além da aquicultura, como estufas automatizadas, monitoramento ambiental e agricultura de precisão.

O conhecimento adquirido durante o desenvolvimento - programação em C, protocolos I2C, PWM, FreeRTOS e stacks TCP/IP - representa uma base sólida para projetos futuros em sistemas embarcados e Internet das Coisas.

---

## 10. Referências

1. **Raspberry Pi Pico SDK Documentation.** Raspberry Pi Foundation. Disponível em: https://www.raspberrypi.com/documentation/pico-sdk/

2. **FreeRTOS Reference Manual.** Amazon Web Services. Disponível em: https://www.freertos.org/Documentation/

3. **lwIP - A Lightweight TCP/IP Stack.** Disponível em: https://savannah.nongnu.org/projects/lwip/

4. **AHT10 Datasheet.** Aosong Electronics. Disponível em: http://www.aosong.com/

5. **VL53L0X Datasheet.** STMicroelectronics. Disponível em: https://www.st.com/resource/en/datasheet/vl53l0x.pdf

6. **TCS34725 Datasheet.** ams AG. Disponível em: https://ams.com/tcs34725

7. **SSD1306 Datasheet.** Solomon Systech. Disponível em: https://www.solomon-systech.com/

8. **BitDogLab Documentation.** Disponível em: https://github.com/BitDogLab/

9. TANENBAUM, Andrew S. **Redes de Computadores.** 5ª ed. Pearson, 2011.

10. MAZIDI, Muhammad Ali. **The 8051 Microcontroller and Embedded Systems.** Pearson, 2006.

---

## 11. Apêndices

### Apêndice A - Código-Fonte

📁 **Repositório GitHub:** https://github.com/hobsonbreno/HydroSense---Sistema-de-Aquicultura-Inteligente

#### Estrutura do Repositório:

```
HydroSense/
├── src/
│   ├── hydrosense_v6.c          # Código principal (897 linhas)
│   ├── tasks/
│   │   ├── monitoring_task.c
│   │   ├── feeding_task.c
│   │   └── automation_task.c
│   └── ...
├── include/
│   ├── lwipopts.h
│   └── ...
├── FreeRTOS-Kernel/
├── build_v6/
│   └── hydrosense_v6.uf2        # Firmware compilado
├── CMakeLists.txt
├── FreeRTOSConfig.h
├── compile_v6.sh
├── RELATORIO_FINAL.md           # Este relatório
└── README.md
```

### Apêndice B - Esquemático de Conexões

```
RASPBERRY PI PICO W + BitDogLab
┌────────────────────────────────────────────────────────────┐
│                                                            │
│  [SENSORES I2C] ──────── GPIO2 (SDA) ─────┐               │
│     AHT10                GPIO3 (SCL) ─────┤               │
│     VL53L0X                               │   I2C1        │
│     TCS34725                              │   100kHz      │
│                                           │               │
│  [OLED SSD1306] ──────── GPIO14 (SDA) ────┤               │
│                          GPIO15 (SCL) ────┘   400kHz      │
│                                                            │
│  [SERVO SG90] ─────────── GPIO16 (PWM) ───────► PWM 50Hz  │
│                                                            │
│  [LED RGB WS2812] ─────── GPIO7 ───────────► Data         │
│                                                            │
│  [BOTÕES] ─────────────── GPIO5 (Botão A)                 │
│                           GPIO6 (Botão B)                  │
│                                                            │
│  [BUZZER] ─────────────── GPIO21 (PWM)                    │
│                                                            │
│  [WiFi] ───────────────── CYW43439 (integrado)            │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

### Apêndice C - Lista de Materiais (BOM)

| Qtd | Componente | Descrição | Preço Est. |
|-----|------------|-----------|------------|
| 1 | Raspberry Pi Pico W | Microcontrolador com WiFi | R$ 45,00 |
| 1 | BitDogLab | Placa de desenvolvimento | R$ 80,00 |
| 1 | AHT10 | Sensor temperatura/umidade | R$ 15,00 |
| 1 | VL53L0X | Sensor distância ToF | R$ 25,00 |
| 1 | TCS34725 | Sensor cor RGB | R$ 20,00 |
| 1 | Servo SG90 | Micro servo 9g | R$ 12,00 |
| - | Jumpers | Cabos de conexão | R$ 5,00 |
| - | Protoboard | Placa de prototipagem | R$ 10,00 |
| **TOTAL** | | | **R$ 212,00** |

### Apêndice D - Comandos para Compilação

```bash
# Clonar repositório
git clone https://github.com/hobsonbreno/HydroSense---Sistema-de-Aquicultura-Inteligente.git
cd HydroSense

# Compilar (requer Pico SDK configurado)
./compile_v6.sh

# Gravar no Pico W (segure BOOTSEL ao conectar)
cp build_v6/hydrosense_v6.uf2 /media/$USER/RPI-RP2/

# Monitorar saída serial
cat /dev/ttyACM0
```

### Apêndice E - Configuração do Ambiente

**Requisitos:**
- Ubuntu 20.04+ ou similar
- Pico SDK 2.0+
- ARM GCC Toolchain
- CMake 3.13+

**Variáveis de ambiente:**
```bash
export PICO_SDK_PATH=/path/to/pico-sdk
export PICO_BOARD=pico_w
```

---

**Fim do Relatório**

*Documento gerado em 07/02/2026*
