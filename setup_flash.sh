#!/bin/bash

# Script para configurar o ambiente de flash do HydroSense
echo "🔧 Configurando flash.sh..."

# Torna o script executável
chmod +x flash.sh

echo "✅ Script flash.sh configurado com sucesso!"
echo ""
echo "📋 Uso do flash.sh:"
echo "  ./flash.sh          # Detecção automática"
echo "  ./flash.sh picotool # Usar picotool (mais rápido)"
echo "  ./flash.sh usb      # Método manual USB"
echo "  ./flash.sh help     # Mostrar ajuda completa"
echo ""
echo "🚀 Agora você pode usar: ./flash.sh picotool"