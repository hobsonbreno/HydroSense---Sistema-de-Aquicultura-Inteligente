# HydroSense v10 - Sistema Completo de Automação de Aquário

## 🎯 Sobre o Projeto

HydroSense é um sistema inteligente e completo de automação para aquários, desenvolvido como projeto final acadêmico. O sistema combina hardware embarcado (Raspberry Pi Pico W) com backend profissional (NestJS + MongoDB) para monitoramento e controle automatizado de todos os aspectos de um aquário.

## 🏗️ Arquitetura do Sistema

### 📱 Frontend Web
- Interface web responsiva em HTML5/CSS3/JavaScript
- Dashboard em tempo real com dados dos sensores
- Controle manual de relés e alimentação
- Painel de automações configuráveis

### 🖥️ Backend API (NestJS)
- API REST completa com Swagger
- Sistema de automação inteligente
- Banco de dados MongoDB para histórico
- Controle de relés e servo motor
- Sistema de agendamento (cron jobs)

### 🔌 Hardware Embarcado (Pico W)
- Raspberry Pi Pico W + BitDogLab
- Sensores reais: AHT10, VL53L0X, TCS3200
- Display OLED SSD1306
- Controle de 3 relés + servo motor
- Servidor HTTP integrado

### 🐳 Containerização Docker
- Multi-container com Docker Compose
- MongoDB, Backend, Frontend, Nginx
- Fácil deployment e escalabilidade

## 🔧 Hardware Necessário

### 🖥️ Componentes Principais
- **Raspberry Pi Pico W** - Microcontrolador principal
- **BitDogLab Board** - Placa de desenvolvimento
- **AHT10** - Sensor de temperatura e umidade (I2C)
- **VL53L0X** - Sensor de distância/nível da água (I2C)
- **TCS3200** - Sensor de cor da água (GPIO)
- **SSD1306** - Display OLED 128x64 (I2C)
- **Servo Motor SG90** - Sistema de alimentação (GPIO 2)
- **3x Módulos Relé** - Controle de bombas e ventilador

### 🔌 Mapeamento de Pinos
```
I2C (Sensores e Display):
  SDA: GPIO 6
  SCL: GPIO 7

Relés:
  LN1 (Ventilador):     GPIO 14
  LN2 (Bomba Esvaziar): GPIO 15
  LN3 (Bomba Encher):   GPIO 16

Servo Motor:
  Alimentação: GPIO 2

Sensor de Cor TCS3200:
  S0: GPIO 10
  S1: GPIO 11
  S2: GPIO 12
  S3: GPIO 13
  OUT: GPIO 9
```

## 🚀 Instalação e Configuração

### 1️⃣ Pré-requisitos
```bash
# Docker e Docker Compose
sudo apt update
sudo apt install docker.io docker-compose

# Node.js (para desenvolvimento)
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# Pico SDK (para firmware)
git clone https://github.com/raspberrypi/pico-sdk.git
export PICO_SDK_PATH=/path/to/pico-sdk
```

### 2️⃣ Configuração do Hardware
1. **Conectar sensores e atuadores** conforme mapeamento de pinos
2. **Configurar rede WiFi**: Nome "HydroSense", senha "12345678"
3. **Compilar firmware**:
   ```bash
   cd src/
   mkdir build && cd build
   cmake ..
   make
   ```
4. **Flashear Pico W** com `hydrosense_v10.uf2`

### 3️⃣ Iniciar Sistema Completo
```bash
# Clone o repositório
git clone [repository-url]
cd HydroSense

# Iniciar sistema (automatizado)
./start_hydrosense.sh
```

### 4️⃣ Acesso aos Serviços
- **Interface Web**: http://localhost:3001
- **API Backend**: http://localhost:3000
- **Swagger Docs**: http://localhost:3000/api
- **Pico W Device**: http://10.0.0.181
- **MongoDB**: mongodb://localhost:27017/hydrosense

## 🤖 Sistema de Automação

### 🌡️ Controle de Temperatura
- **Trigger**: Temperatura > 29°C
- **Ação**: Liga ventilador (Relé LN1)
- **Reset**: Temperatura ≤ 29°C
- **Verificação**: A cada 1 minuto

### 💧 Qualidade da Água
- **Trigger**: Sensor de cor detecta água não cristalina
- **Ação Automática**:
  1. Liga bomba de esvaziar (LN2) por tempo calculado (25% do volume)
  2. Aguarda 30 segundos
  3. Liga bomba de encher (LN3) até 90% do volume
- **Verificação**: A cada 5 minutos

### 📊 Nível da Água
- **Trigger**: Nível < 80% por mais de 15 minutos
- **Ação**: Completa até 90% usando bomba LN3
- **Verificação**: A cada 15 minutos

### 🐟 Sistema de Alimentação
- **Horários Automáticos**: 08:00 e 16:00
- **Ação**: Servo motor rotaciona 360° por 2 segundos
- **Controle Manual**: Via interface web ou API

## 📊 API Endpoints

### 🔍 Sensores
```http
GET /sensors              # Listar dados dos sensores
POST /sensors             # Adicionar dados de sensor
GET /sensors/stats        # Estatísticas dos sensores
GET /sensors/water-quality # Status da qualidade da água
```

### ⚡ Relés
```http
GET /relays/status        # Status atual dos relés
POST /relays/control      # Controlar relé específico
GET /relays/history       # Histórico de comandos
POST /relays/ventilator/on   # Ligar ventilador
POST /relays/drain/start     # Iniciar bomba de esvaziar
POST /relays/fill/start      # Iniciar bomba de encher
```

### 🍽️ Alimentação
```http
GET /feeding/status       # Status do sistema de alimentação
POST /feeding/manual      # Executar alimentação manual
GET /feeding/history      # Histórico de alimentações
POST /feeding/toggle      # Ativar/desativar automação
```

### 🤖 Automação
```http
GET /automation/status    # Status das automações
POST /automation/temperature/toggle    # Controle de temperatura
POST /automation/water-quality/toggle  # Qualidade da água
POST /automation/water-level/toggle    # Nível da água
POST /automation/water-cycle/manual    # Ciclo manual de água
```

## 📈 Monitoramento

### 📊 Dados Coletados
- **Temperatura**: Precisão 0.1°C
- **Umidade**: Precisão 0.1%
- **Nível da Água**: Baseado em sensor de distância
- **Volume**: Calculado automaticamente (base: 20L)
- **Cor da Água**: Cristalino, turvo, verde, marrom, outro
- **Status WiFi**: Conectividade em tempo real

### 🗄️ Banco de Dados
- **SensorData**: Histórico de todas as leituras
- **RelayControl**: Log de comandos dos relés
- **FeedingLog**: Registro de alimentações
- **AutomationRules**: Configurações das automações

## 🔧 Comandos Úteis

```bash
# Desenvolvimento
npm run start:dev          # Modo desenvolvimento
npm run build              # Construir aplicação
npm run test               # Executar testes

# Docker
docker-compose up -d       # Iniciar serviços
docker-compose logs -f     # Ver logs
docker-compose down        # Parar serviços
docker-compose restart     # Reiniciar serviços

# Sistema
./start_hydrosense.sh      # Iniciar sistema completo
ping 10.0.0.181           # Testar conectividade Pico W
```

## 🎛️ Interface Web

### 📊 Dashboard Principal
- **Cards de Sensores**: Temperatura, umidade, nível, cor da água
- **Controle de Relés**: Interface visual para cada relé
- **Sistema de Alimentação**: Controle manual e status automático
- **Painel de Automações**: Configuração das regras automáticas

### 📱 Design Responsivo
- Otimizado para desktop e mobile
- Design moderno com glass morphism
- Atualizações em tempo real
- Indicadores visuais de status

## 🛡️ Segurança e Confiabilidade

### 🔐 Segurança
- Validação de dados com class-validator
- Sanitização de entradas
- CORS configurado
- Rate limiting (opcional)

### 🔄 Confiabilidade
- Reconexão automática WiFi
- Fallback para dados simulados
- Logs detalhados
- Tratamento de erros robusto

## 📚 Documentação Técnica

### 🏗️ Estrutura do Projeto
```
HydroSense/
├── 🐳 docker-compose.yml        # Orquestração de containers
├── 📜 start_hydrosense.sh       # Script de inicialização
├── 🖥️ backend/                 # API NestJS
│   ├── src/modules/            # Módulos da aplicação
│   ├── src/schemas/            # Schemas MongoDB
│   └── package.json            # Dependências
├── 🌐 frontend/                # Interface web
│   └── index.html              # Dashboard principal
├── 💾 src/                     # Firmware Pico W
│   ├── hydrosense_v10.c        # Código principal
│   └── CMakeLists.txt          # Configuração build
└── 📖 README.md                # Documentação existente
```

### 🔌 Comunicação
- **Pico W ↔ Backend**: HTTP REST API
- **Backend ↔ Frontend**: REST + WebSocket (futuro)
- **Backend ↔ MongoDB**: Mongoose ODM
- **Frontend ↔ Usuário**: Interface web responsiva

## 🎓 Contexto Acadêmico

### 📋 Especificações do Projeto
- **Disciplina**: Projeto Final
- **Valor**: 26,25% da nota final
- **Prazo**: 8 de fevereiro de 2026
- **Objetivo**: Sistema IoT completo para aquacultura

### 🏆 Funcionalidades Implementadas
- ✅ Monitoramento em tempo real
- ✅ Automação inteligente
- ✅ Interface web profissional
- ✅ API REST documentada
- ✅ Banco de dados histórico
- ✅ Sistema de alimentação automático
- ✅ Controle de qualidade da água
- ✅ Containerização Docker

## 🚀 Quick Start

Para uma inicialização rápida:

```bash
# 1. Clone e entre no diretório
git clone [repo] && cd HydroSense

# 2. Inicie o sistema completo
./start_hydrosense.sh

# 3. Acesse a interface
# http://localhost:3001
```

**🎉 Pronto! Seu sistema HydroSense está operacional!**

---

*Desenvolvido com ❤️ para automação inteligente de aquários*