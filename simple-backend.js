const express = require('express');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = 3000;
const FRONTEND_PORT = 3001;

// Middleware
app.use(cors());
app.use(express.json());

// Mock data para teste
let mockSensorData = {
  temperatura: 27.5,
  umidade: 65.0,
  distancia: 120,
  nivel: 75.5,
  volume: 15.1,
  corAgua: 'Cristalino',
  corR: 1200,
  corG: 1100,
  corB: 1000,
  wifiStatus: true,
  contadorLeituras: 150,
  deviceIp: '10.0.0.181',
  sensores: { aht10: true, vl53l0x: true, tcs34725: true },
  timestamp: Date.now()
};

let mockRelayStates = {
  LN1: { estado: false, ultimaAlteracao: new Date(), observacao: 'Ventilador desligado' },
  LN2: { estado: false, ultimaAlteracao: new Date(), observacao: 'Bomba esvaziar desligada' },
  LN3: { estado: false, ultimaAlteracao: new Date(), observacao: 'Bomba encher desligada' }
};

// Routes
app.get('/health', (req, res) => {
  res.json({ status: 'OK', service: 'HydroSense Backend', timestamp: new Date() });
});

// Sensors endpoints
app.get('/sensors', (req, res) => {
  // Simular variação nos dados
  mockSensorData.temperatura = 26 + Math.random() * 4; // 26-30°C
  mockSensorData.umidade = 60 + Math.random() * 20; // 60-80%
  mockSensorData.distancia = 100 + Math.random() * 50; // 100-150mm
  mockSensorData.nivel = 70 + Math.random() * 20; // 70-90%
  mockSensorData.volume = mockSensorData.nivel * 0.2; // Volume baseado no nível
  const cores = ['Cristalino', 'Verde', 'Azul', 'Turvo', 'Amarelado'];
  mockSensorData.corAgua = cores[Math.floor(Math.random() * cores.length)];
  mockSensorData.corR = Math.floor(Math.random() * 3000);
  mockSensorData.corG = Math.floor(Math.random() * 3000);
  mockSensorData.corB = Math.floor(Math.random() * 3000);
  mockSensorData.contadorLeituras++;
  mockSensorData.timestamp = Date.now();
  
  res.json(mockSensorData);
});

app.post('/sensors', (req, res) => {
  mockSensorData = { ...mockSensorData, ...req.body, timestamp: Date.now() };
  res.json({ success: true, data: mockSensorData });
});

// Relays endpoints
app.get('/relays/status', (req, res) => {
  res.json({
    relays: mockRelayStates,
    picoStatus: { connected: true, ip: '10.0.0.181', lastCheck: new Date() }
  });
});

app.post('/relays/control', async (req, res) => {
  const { tipo, estado, origem = 'manual', observacao } = req.body;
  
  // Tenta enviar para o Pico W primeiro (proxy)
  const PICO_IP = mockSensorData.deviceIp || '10.0.0.181';
  const relayMap = { LN1: 1, LN2: 2, LN3: 3 };
  const relayNum = relayMap[tipo];
  
  if (relayNum) {
    try {
      const controller = new AbortController();
      const timeout = setTimeout(() => controller.abort(), 3000);
      const picoRes = await fetch(`http://${PICO_IP}/relay`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ relay: relayNum, state: estado ? 1 : 0 }),
        signal: controller.signal
      });
      clearTimeout(timeout);
      if (picoRes.ok) {
        const picoData = await picoRes.json();
        console.log(`Relé ${tipo}: ${estado ? 'LIGADO' : 'DESLIGADO'} via Pico W (${origem})`);
        // Atualiza mock com estado real do Pico
        if (picoData.relays) {
          mockRelayStates.LN1 = { estado: picoData.relays.LN1, ultimaAlteracao: new Date(), observacao: 'Via Pico W' };
          mockRelayStates.LN2 = { estado: picoData.relays.LN2, ultimaAlteracao: new Date(), observacao: 'Via Pico W' };
          mockRelayStates.LN3 = { estado: picoData.relays.LN3, ultimaAlteracao: new Date(), observacao: 'Via Pico W' };
        }
        return res.json({ success: true, source: 'pico', message: `Relé ${tipo} controlado via Pico W`, data: mockRelayStates[tipo] });
      }
    } catch (e) {
      console.log(`Pico W indisponível para relé, usando fallback mock`);
    }
  }
  
  // Fallback: mock local
  if (mockRelayStates[tipo]) {
    mockRelayStates[tipo] = {
      estado,
      ultimaAlteracao: new Date(),
      observacao: observacao || `Relé ${tipo} ${estado ? 'ligado' : 'desligado'}`
    };
    
    console.log(`Relé ${tipo}: ${estado ? 'LIGADO' : 'DESLIGADO'} (${origem}) [mock]`);
    res.json({ success: true, source: 'mock', message: `Relé ${tipo} controlado (mock)`, data: mockRelayStates[tipo] });
  } else {
    res.status(400).json({ error: 'Tipo de relé inválido' });
  }
});

// Shortcut routes for relays
app.post('/relays/ventilator/on', (req, res) => {
  mockRelayStates.LN1 = { estado: true, ultimaAlteracao: new Date(), observacao: 'Ventilador ligado via API' };
  res.json({ success: true, message: 'Ventilador ligado' });
});

app.post('/relays/ventilator/off', (req, res) => {
  mockRelayStates.LN1 = { estado: false, ultimaAlteracao: new Date(), observacao: 'Ventilador desligado via API' };
  res.json({ success: true, message: 'Ventilador desligado' });
});

app.post('/relays/drain/start', (req, res) => {
  mockRelayStates.LN2 = { estado: true, ultimaAlteracao: new Date(), observacao: 'Bomba esvaziar ligada via API' };
  res.json({ success: true, message: 'Bomba de esvaziar iniciada' });
});

app.post('/relays/drain/stop', (req, res) => {
  mockRelayStates.LN2 = { estado: false, ultimaAlteracao: new Date(), observacao: 'Bomba esvaziar desligada via API' };
  res.json({ success: true, message: 'Bomba de esvaziar parada' });
});

app.post('/relays/fill/start', (req, res) => {
  mockRelayStates.LN3 = { estado: true, ultimaAlteracao: new Date(), observacao: 'Bomba encher ligada via API' };
  res.json({ success: true, message: 'Bomba de encher iniciada' });
});

app.post('/relays/fill/stop', (req, res) => {
  mockRelayStates.LN3 = { estado: false, ultimaAlteracao: new Date(), observacao: 'Bomba encher desligada via API' };
  res.json({ success: true, message: 'Bomba de encher parada' });
});

// Feeding endpoints
app.get('/feeding/status', (req, res) => {
  const now = new Date();
  const next8AM = new Date();
  next8AM.setHours(8, 0, 0, 0);
  if (next8AM < now) next8AM.setDate(next8AM.getDate() + 1);
  
  const next4PM = new Date();
  next4PM.setHours(16, 0, 0, 0);
  if (next4PM < now) next4PM.setDate(next4PM.getDate() + 1);
  
  const nextFeeding = (next8AM < next4PM) ? next8AM : next4PM;
  const timeUntil = Math.ceil((nextFeeding - now) / (1000 * 60));
  
  res.json({
    active: true,
    nextScheduled: {
      nextFeeding: nextFeeding.toLocaleString('pt-BR'),
      timeUntilNext: `${Math.floor(timeUntil/60)}h ${timeUntil%60}min`
    },
    todayFeedings: [
      { timestamp: new Date(), tipo: 'automatico', sucesso: true }
    ]
  });
});

app.post('/feeding/manual', (req, res) => {
  console.log('🐟 Alimentação manual executada!');
  res.json({ 
    success: true, 
    message: 'Alimentação manual executada com sucesso',
    timestamp: new Date()
  });
});

app.post('/feeding/toggle', (req, res) => {
  const { active } = req.body;
  res.json({
    success: true,
    message: `Alimentação automática ${active ? 'ativada' : 'desativada'}`
  });
});

// Automation endpoints
app.get('/automation/status', (req, res) => {
  res.json({
    temperatureControl: true,
    waterQualityControl: true,
    waterLevelControl: true,
    lastWaterQualityCheck: new Date()
  });
});

app.post('/automation/temperature/toggle', (req, res) => {
  const { active } = req.body;
  res.json({
    success: true,
    message: `Controle automático de temperatura ${active ? 'ativado' : 'desativado'}`
  });
});

app.post('/automation/water-quality/toggle', (req, res) => {
  const { active } = req.body;
  res.json({
    success: true,
    message: `Controle automático de qualidade da água ${active ? 'ativado' : 'desativado'}`
  });
});

app.post('/automation/water-level/toggle', (req, res) => {
  const { active } = req.body;
  res.json({
    success: true,
    message: `Controle automático de nível da água ${active ? 'ativado' : 'desativado'}`
  });
});

app.post('/automation/water-cycle/manual', (req, res) => {
  console.log('🌊 Ciclo manual de troca de água iniciado!');
  res.json({
    success: true,
    message: 'Ciclo manual de troca de água iniciado com sucesso'
  });
});

// Start server
app.listen(PORT, () => {
  console.log('🌊 ===== HydroSense Backend Mock ===== 🌊');
  console.log(`🚀 API rodando na porta ${PORT}`);
  console.log(`📊 API: http://localhost:${PORT}`);
  console.log(`💧 MongoDB: Porta 27018 (mock data)`);
  console.log(`🔧 Status: http://localhost:${PORT}/health`);
  console.log('================================================');
});

// Servidor de frontend na porta 3001
const frontendApp = express();
frontendApp.use(express.static(path.join(__dirname, 'frontend')));
frontendApp.get('/{path}', (req, res) => {
  res.sendFile(path.join(__dirname, 'frontend', 'index.html'));
});
frontendApp.listen(FRONTEND_PORT, () => {
  console.log(`🖥️  Frontend: http://localhost:${FRONTEND_PORT}`);
  console.log('================================================');
});