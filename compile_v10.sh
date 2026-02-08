#!/bin/bash

# Script para compilar apenas o hydrosense_v10.c
echo "🔨 Compilando HydroSense v10 - Firmware com HTTP Server"
echo "======================================================"

# Criar diretório de build específico
mkdir -p build_v10
cd build_v10

# Criar CMakeLists.txt específico para v10
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(hydrosense_v10)

pico_sdk_init()

add_executable(hydrosense_v10
    ../src/hydrosense_v10.c
)

target_link_libraries(hydrosense_v10 
    pico_stdlib 
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_i2c
    hardware_gpio
    hardware_pwm
    hardware_adc
)

pico_add_extra_outputs(hydrosense_v10)
pico_enable_stdio_usb(hydrosense_v10 1)
pico_enable_stdio_uart(hydrosense_v10 0)
EOF

# Copiar pico_sdk_import.cmake
cp ../pico_sdk_import.cmake .

echo "📁 Configurando CMake..."
cmake ..

echo "🔨 Compilando..."
make -j$(nproc)

if [ -f "hydrosense_v10.uf2" ]; then
    echo ""
    echo "🎉 COMPILAÇÃO CONCLUÍDA!"
    echo "📁 Arquivo gerado: hydrosense_v10.uf2"
    echo "🚀 Para gravar no Pico W:"
    echo "   1. Segure BOOTSEL e conecte o Pico W"
    echo "   2. Execute: cp hydrosense_v10.uf2 /media/\$USER/RPI-RP2/"
    ls -la hydrosense_v10.uf2
else
    echo "❌ Erro na compilação!"
    exit 1
fi