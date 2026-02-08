#!/bin/bash

# Script para gravar firmware no Pico W via terminal
# Uso: ./flash_pico.sh [arquivo.uf2]

FIRMWARE_FILE="${1:-hydrosense_v10.uf2}"
PICOTOOL="/home/hobson007breno/.pico-sdk/picotool/2.2.0/picotool/picotool"

echo "🚀 HYDROSENSE - Flash Pico W via Terminal"
echo "=========================================="

# Verificar se arquivo existe
if [ ! -f "$FIRMWARE_FILE" ]; then
    echo "❌ Arquivo $FIRMWARE_FILE não encontrado!"
    echo "📁 Arquivos .uf2 disponíveis:"
    ls -la *.uf2 2>/dev/null || echo "   Nenhum arquivo .uf2 encontrado"
    exit 1
fi

echo "📁 Firmware: $FIRMWARE_FILE ($(du -h "$FIRMWARE_FILE" | cut -f1))"

# Verificar picotool
if [ ! -f "$PICOTOOL" ]; then
    echo "❌ picotool não encontrado em: $PICOTOOL"
    echo "💡 Instale o Pico SDK ou use método BOOTSEL manual"
    exit 1
fi

echo "🔍 Verificando conexão com Pico W..."
$PICOTOOL info

if [ $? -eq 0 ]; then
    echo "✅ Pico W encontrado em modo BOOTSEL"
else
    echo "🔄 Pico W em modo normal - forçando reboot..."
fi

echo ""
echo "⚡ GRAVANDO FIRMWARE..."
echo "   Comando: $PICOTOOL load -fx $FIRMWARE_FILE"

$PICOTOOL load -fx "$FIRMWARE_FILE"

if [ $? -eq 0 ]; then
    echo ""
    echo "🎉 FIRMWARE GRAVADO COM SUCESSO!"
    echo ""
    echo "🔍 Verificando reinicialização..."
    sleep 3
    
    # Verificar porta serial
    if [ -e /dev/ttyACM0 ]; then
        echo "📡 Pico W detectado em /dev/ttyACM0"
        echo "📊 Logs do sistema (5 segundos):"
        echo "   (Pressione Ctrl+C para interromper)"
        timeout 5 cat /dev/ttyACM0 2>/dev/null || echo "   Sem dados seriais"
    else
        echo "⚠️  Porta serial não detectada - aguarde alguns segundos"
    fi
    
    echo ""
    echo "💡 PRÓXIMOS PASSOS:"
    echo "   1. Aguarde 10-15 segundos para inicialização completa"
    echo "   2. Verifique conectividade WiFi nos logs"
    echo "   3. Teste HTTP: curl http://10.0.0.181/sensors"
    echo "   4. Monitore via: cat /dev/ttyACM0"
    
else
    echo ""
    echo "❌ ERRO NA GRAVAÇÃO!"
    echo ""
    echo "🔧 SOLUÇÕES:"
    echo "   1. Verifique conexão USB"
    echo "   2. Tente modo BOOTSEL manual:"
    echo "      - Pressione BOOTSEL + conecte USB"
    echo "      - cp $FIRMWARE_FILE /media/\$USER/RPI-RP2/"
    echo "   3. Use sudo se necessário"
    
    exit 1
fi