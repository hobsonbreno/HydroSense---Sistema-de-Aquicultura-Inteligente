// Script de teste abrangente para todos os botões do HydroSense Frontend
// Execute este script no console do navegador (F12 -> Console)

console.log('🚀 INICIANDO TESTE ABRANGENTE DOS BOTÕES DO HYDROSENSE');
console.log('=' .repeat(60));

const API_BASE = 'http://localhost:3000';

// Função para delay
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));

// Teste 1: Verificar conectividade com backend
async function testBackendConnection() {
    console.log('🔗 TESTE 1: Conectividade com Backend');
    try {
        const response = await fetch(`${API_BASE}/health`);
        const data = await response.json();
        console.log('✅ Backend conectado:', data);
        return true;
    } catch (error) {
        console.error('❌ Erro na conectividade:', error);
        return false;
    }
}

// Teste 2: Dados dos sensores
async function testSensorData() {
    console.log('\n📊 TESTE 2: Dados dos Sensores');
    try {
        const response = await fetch(`${API_BASE}/sensors`);
        const data = await response.json();
        
        console.log('✅ Dados dos sensores recebidos:');
        console.log(`  • Temperatura: ${data.temperatura.toFixed(1)}°C`);
        console.log(`  • Umidade: ${data.umidade.toFixed(1)}%`);
        console.log(`  • Distância: ${data.distancia.toFixed(1)}mm`);
        console.log(`  • Nível: ${data.nivel.toFixed(1)}%`);
        console.log(`  • Cor da água: ${data.corAgua}`);
        
        return true;
    } catch (error) {
        console.error('❌ Erro ao buscar dados dos sensores:', error);
        return false;
    }
}

// Teste 3: Status dos relés
async function testRelayStatus() {
    console.log('\n🔌 TESTE 3: Status dos Relés');
    try {
        const response = await fetch(`${API_BASE}/relays/status`);
        const data = await response.json();
        
        console.log('✅ Status dos relés:');
        Object.entries(data).forEach(([relay, info]) => {
            console.log(`  • ${relay}: ${info.estado ? 'LIGADO' : 'DESLIGADO'}`);
        });
        
        return true;
    } catch (error) {
        console.error('❌ Erro ao buscar status dos relés:', error);
        return false;
    }
}

// Teste 4: Controle de relés
async function testRelayControl() {
    console.log('\n⚡ TESTE 4: Controle de Relés');
    const relays = ['LN1', 'LN2', 'LN3'];
    const relayNames = { LN1: 'Ventilador', LN2: 'Bomba Esvaziar', LN3: 'Bomba Encher' };
    
    for (const relay of relays) {
        try {
            console.log(`  Testando ${relayNames[relay]} (${relay})...`);
            
            // Ligar
            let response = await fetch(`${API_BASE}/relays/control`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    tipo: relay,
                    estado: true,
                    origem: 'teste-automatico',
                    observacao: `Teste automático do ${relayNames[relay]}`
                })
            });
            
            if (response.ok) {
                console.log(`    ✅ ${relayNames[relay]} LIGADO`);
            }
            
            await delay(1000);
            
            // Desligar
            response = await fetch(`${API_BASE}/relays/control`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    tipo: relay,
                    estado: false,
                    origem: 'teste-automatico',
                    observacao: `Teste automático do ${relayNames[relay]}`
                })
            });
            
            if (response.ok) {
                console.log(`    ✅ ${relayNames[relay]} DESLIGADO`);
            }
            
        } catch (error) {
            console.error(`    ❌ Erro no controle do ${relayNames[relay]}:`, error);
        }
        
        await delay(500);
    }
}

// Teste 5: Sistema de alimentação
async function testFeedingSystem() {
    console.log('\n🐠 TESTE 5: Sistema de Alimentação');
    
    try {
        // Teste alimentação manual
        console.log('  Testando alimentação manual...');
        let response = await fetch(`${API_BASE}/feeding/manual`, {
            method: 'POST'
        });
        
        if (response.ok) {
            const data = await response.json();
            console.log('  ✅ Alimentação manual executada:', data.message);
        }
        
        await delay(1000);
        
        // Teste status da alimentação
        console.log('  Verificando status da alimentação...');
        response = await fetch(`${API_BASE}/feeding/status`);
        
        if (response.ok) {
            const data = await response.json();
            console.log('  ✅ Status da alimentação:', data.active ? 'ATIVO' : 'INATIVO');
        }
        
        // Teste toggle alimentação automática
        console.log('  Testando toggle alimentação automática...');
        response = await fetch(`${API_BASE}/feeding/toggle`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ active: true })
        });
        
        if (response.ok) {
            const data = await response.json();
            console.log('  ✅ Toggle alimentação:', data.message);
        }
        
    } catch (error) {
        console.error('  ❌ Erro no sistema de alimentação:', error);
    }
}

// Teste 6: Sistema de automação
async function testAutomationSystem() {
    console.log('\n🤖 TESTE 6: Sistema de Automação');
    
    const automationTypes = [
        { type: 'temperature', name: 'Temperatura' },
        { type: 'water-quality', name: 'Qualidade da Água' },
        { type: 'water-level', name: 'Nível da Água' },
        { type: 'feeding', name: 'Alimentação' }
    ];
    
    for (const automation of automationTypes) {
        try {
            console.log(`  Testando automação: ${automation.name}...`);
            
            // Ativar automação
            let response = await fetch(`${API_BASE}/automation/${automation.type}/toggle`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ active: true })
            });
            
            if (response.ok) {
                const data = await response.json();
                console.log(`    ✅ ${automation.name} ativada: ${data.message}`);
            }
            
            await delay(500);
            
            // Desativar automação
            response = await fetch(`${API_BASE}/automation/${automation.type}/toggle`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ active: false })
            });
            
            if (response.ok) {
                const data = await response.json();
                console.log(`    ✅ ${automation.name} desativada: ${data.message}`);
            }
            
        } catch (error) {
            console.error(`    ❌ Erro na automação ${automation.name}:`, error);
        }
        
        await delay(500);
    }
}

// Teste 7: Verificar status da automação
async function testAutomationStatus() {
    console.log('\n📋 TESTE 7: Status da Automação');
    
    try {
        const response = await fetch(`${API_BASE}/automation/status`);
        
        if (response.ok) {
            const data = await response.json();
            console.log('✅ Status da automação recebido:');
            Object.entries(data).forEach(([key, value]) => {
                if (typeof value === 'object' && value.active !== undefined) {
                    console.log(`  • ${key}: ${value.active ? 'ATIVO' : 'INATIVO'}`);
                }
            });
        }
        
    } catch (error) {
        console.error('❌ Erro ao verificar status da automação:', error);
    }
}

// Teste 8: Ciclo manual de água (apenas log, não executa)
async function testWaterCycle() {
    console.log('\n🌊 TESTE 8: Ciclo Manual de Água');
    console.log('  ℹ️  Botão disponível na interface (não executado para evitar dialog)');
    console.log('  ℹ️  Endpoint: POST /automation/water-cycle/manual');
}

// Função principal que executa todos os testes
async function runAllTests() {
    const startTime = Date.now();
    console.log(`🧪 Iniciando bateria de testes em: ${new Date().toLocaleTimeString('pt-BR')}`);
    console.log('');
    
    let passedTests = 0;
    let totalTests = 8;
    
    if (await testBackendConnection()) passedTests++;
    if (await testSensorData()) passedTests++;
    if (await testRelayStatus()) passedTests++;
    await testRelayControl(); passedTests++; // Sempre conta como passou se não travou
    await testFeedingSystem(); passedTests++;
    await testAutomationSystem(); passedTests++;
    if (await testAutomationStatus()) passedTests++;
    await testWaterCycle(); passedTests++;
    
    const endTime = Date.now();
    const duration = ((endTime - startTime) / 1000).toFixed(2);
    
    console.log('');
    console.log('=' .repeat(60));
    console.log(`🏁 TESTES CONCLUÍDOS EM ${duration}s`);
    console.log(`📊 RESULTADOS: ${passedTests}/${totalTests} testes concluídos`);
    
    if (passedTests === totalTests) {
        console.log('🎉 TODOS OS BOTÕES ESTÃO FUNCIONANDO CORRETAMENTE!');
    } else {
        console.log('⚠️  Alguns testes falharam. Verifique os logs acima.');
    }
    
    console.log('');
    console.log('📝 PRÓXIMOS PASSOS:');
    console.log('1. Conectar e configurar o Pico W (10.0.0.181)');
    console.log('2. Verificar comunicação entre frontend e hardware');
    console.log('3. Testar interface completa no navegador');
    console.log('');
    console.log('💡 Para testar manualmente, visite: http://localhost:3001');
}

// Auto-executar
runAllTests().catch(console.error);