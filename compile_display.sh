#!/bin/bash

echo "🔧 Compilando programa de diagnóstico do display OLED..."

# Criar diretório de build específico
mkdir -p build_display
cd build_display

# Configurar com CMake usando o arquivo CMakeLists correto
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../pico_sdk_import.cmake ..

# Compilar
make -j4

if [ $? -eq 0 ]; then
    echo "✅ Compilação concluída com sucesso!"
    echo "📁 Arquivo gerado: build_display/display_diagnostico.uf2"
    echo ""
    echo "📋 Para usar:"
    echo "1. Conecte o Pico em modo BOOTSEL"
    echo "2. Copie display_diagnostico.uf2 para o drive RPI-RP2"
    echo "3. Conecte via monitor serial (115200 baud)"
    echo "4. Observe os testes automáticos do display"
else
    echo "❌ Erro na compilação!"
    exit 1
fi