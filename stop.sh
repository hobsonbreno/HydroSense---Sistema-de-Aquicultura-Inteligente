#!/bin/bash
# =====================================================
#  HydroSense v10 — Parar todos os serviços
# =====================================================

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
PID_FILE="$PROJECT_DIR/.hydrosense.pid"

echo ""
echo "🛑 =========================================="
echo "   HydroSense v10 — Parando Serviços"
echo "🛑 =========================================="
echo ""

KILLED=0

# 1. Parar pelo PID salvo
if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    if kill -0 "$PID" 2>/dev/null; then
        echo "🔹 Parando servidor (PID $PID)..."
        kill "$PID" 2>/dev/null
        # Aguardar encerrar graciosamente (max 5s)
        for i in $(seq 1 10); do
            if ! kill -0 "$PID" 2>/dev/null; then
                break
            fi
            sleep 0.5
        done
        # Forçar se ainda estiver rodando
        if kill -0 "$PID" 2>/dev/null; then
            echo "   ⚡ Forçando encerramento..."
            kill -9 "$PID" 2>/dev/null
        fi
        KILLED=$((KILLED + 1))
        echo "   ✅ Servidor parado"
    else
        echo "🔹 PID $PID já não está ativo"
    fi
    rm -f "$PID_FILE"
fi

# 2. Matar qualquer processo node rodando simple-backend.js (segurança)
PIDS=$(pgrep -f "node.*simple-backend.js" 2>/dev/null || true)
if [ -n "$PIDS" ]; then
    for PID in $PIDS; do
        echo "🔹 Encerrando processo residual (PID $PID)..."
        kill "$PID" 2>/dev/null
        sleep 1
        if kill -0 "$PID" 2>/dev/null; then
            kill -9 "$PID" 2>/dev/null
        fi
        KILLED=$((KILLED + 1))
    done
fi

# 3. Verificar se as portas 3000 e 3001 estão livres
PORT_3000=$(lsof -ti:3000 2>/dev/null || true)
PORT_3001=$(lsof -ti:3001 2>/dev/null || true)

if [ -n "$PORT_3000" ]; then
    echo "🔹 Liberando porta 3000 (PID $PORT_3000)..."
    kill "$PORT_3000" 2>/dev/null
    sleep 1
    kill -9 "$PORT_3000" 2>/dev/null 2>&1 || true
    KILLED=$((KILLED + 1))
fi

if [ -n "$PORT_3001" ]; then
    echo "🔹 Liberando porta 3001 (PID $PORT_3001)..."
    kill "$PORT_3001" 2>/dev/null
    sleep 1
    kill -9 "$PORT_3001" 2>/dev/null 2>&1 || true
    KILLED=$((KILLED + 1))
fi

# Resultado
echo ""
if [ "$KILLED" -gt 0 ]; then
    echo "✅ =========================================="
    echo "   Todos os serviços HydroSense encerrados!"
    echo "✅ =========================================="
else
    echo "ℹ️  Nenhum serviço HydroSense estava rodando."
fi

echo ""
echo "   Portas 3000 e 3001 estão livres."
echo "   Para iniciar novamente: ./start.sh"
echo ""
