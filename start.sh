#!/bin/bash
# =====================================================
#  HydroSense v10 — Iniciar todos os serviços
# =====================================================

set -e

# Diretório do projeto (onde este script está)
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
PID_FILE="$PROJECT_DIR/.hydrosense.pid"

echo ""
echo "🌊 =========================================="
echo "   HydroSense v10 — Iniciando Serviços"
echo "🌊 =========================================="
echo ""

# Verificar se já está rodando
if [ -f "$PID_FILE" ]; then
    OLD_PID=$(cat "$PID_FILE")
    if kill -0 "$OLD_PID" 2>/dev/null; then
        echo "⚠️  HydroSense já está rodando (PID $OLD_PID)"
        echo "   Use ./stop.sh para parar primeiro."
        echo ""
        exit 1
    else
        # PID antigo não existe mais, limpar
        rm -f "$PID_FILE"
    fi
fi

# Verificar Node.js
if ! command -v node &>/dev/null; then
    echo "❌ Node.js não encontrado. Instale com:"
    echo "   curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash -"
    echo "   sudo apt install -y nodejs"
    exit 1
fi

echo "✅ Node.js $(node --version) encontrado"

# Instalar dependências se necessário
if [ ! -d "$PROJECT_DIR/node_modules" ]; then
    echo "📦 Instalando dependências (npm install)..."
    cd "$PROJECT_DIR"
    npm install --no-audit --no-fund
    echo ""
else
    echo "✅ Dependências já instaladas"
fi

# Iniciar o backend + frontend
echo ""
echo "🚀 Iniciando servidor..."
cd "$PROJECT_DIR"
nohup node simple-backend.js > "$PROJECT_DIR/.hydrosense.log" 2>&1 &
SERVER_PID=$!
echo "$SERVER_PID" > "$PID_FILE"

# Aguardar o servidor subir
sleep 2

# Verificar se iniciou corretamente
if kill -0 "$SERVER_PID" 2>/dev/null; then
    echo ""
    echo "✅ =============================================="
    echo "     HydroSense v10 — Sistema Ativo!"
    echo "✅ =============================================="
    echo ""
    echo "   ┌──────────────────────────────────────────┐"
    echo "   │  🖥️  FRONTEND                            │"
    echo "   │  http://localhost:3001                    │"
    echo "   └──────────────────────────────────────────┘"
    echo ""
    echo "   ┌──────────────────────────────────────────┐"
    echo "   │  📊 BACKEND API                          │"
    echo "   │  http://localhost:3000                    │"
    echo "   │  http://localhost:3000/health             │"
    echo "   │  http://localhost:3000/sensors            │"
    echo "   │  http://localhost:3000/relays/status      │"
    echo "   │  http://localhost:3000/feeding/status     │"
    echo "   │  http://localhost:3000/automation/status  │"
    echo "   └──────────────────────────────────────────┘"
    echo ""
    echo "   ┌──────────────────────────────────────────┐"
    echo "   │  🐟 PICO W (Embarcado)                   │"
    echo "   │  http://10.0.0.181/                      │"
    echo "   │  http://10.0.0.181/sensors               │"
    echo "   │  http://10.0.0.181/status                │"
    echo "   │  POST http://10.0.0.181/relay            │"
    echo "   │  POST http://10.0.0.181/feed             │"
    echo "   └──────────────────────────────────────────┘"
    echo ""
    echo "   ┌──────────────────────────────────────────┐"
    echo "   │  📝 SWAGGER / DOCS                       │"
    echo "   │  http://localhost:3000/api                │"
    echo "   └──────────────────────────────────────────┘"
    echo ""
    echo "   📋 PID: $SERVER_PID"
    echo "   📄 Log: $PROJECT_DIR/.hydrosense.log"
    echo ""
    echo "   🛑 Parar:   ./stop.sh"
    echo "   📄 Logs:    tail -f .hydrosense.log"
    echo ""

    # Verificar Pico W
    if ping -c 1 -W 2 10.0.0.181 &>/dev/null; then
        echo "   ✅ Pico W (10.0.0.181) acessível — dados reais!"
    else
        echo "   ⚠️  Pico W offline — usando dados simulados"
    fi
    echo ""
else
    echo "❌ Falha ao iniciar o servidor. Verifique o log:"
    echo "   cat $PROJECT_DIR/.hydrosense.log"
    rm -f "$PID_FILE"
    exit 1
fi
